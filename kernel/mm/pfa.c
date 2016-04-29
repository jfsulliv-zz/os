/*
 * mm/pfa.c
 *
 * General page frame allocator.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 04/16
 */

#include <asm/bitops.h>
#include <mm/reserve.h>
#include <mm/paging.h>
#include <mm/pfa.h>
#include <sys/kprintf.h>
#include <sys/string.h>
#include <sys/size.h>
#include <sys/panic.h>
#include <util/cmp.h>
#include <util/list.h>

pfa_t pfa;

#define LOG2(X) ((unsigned) (8*sizeof (unsigned long) - __builtin_clzl((X)) - 1))

static inline unsigned int
order_of(unsigned long num_pages)
{
        return LOG2(num_pages);
}

static void
mark_avail(page_t *pg)
{
        unsigned long ind = PFA_MAP_INDEX_V(_pa(pg->vaddr));
        _clr_bit(pfa.tag_bits, ind);
}

static void
mark_allocated(page_t *pg)
{
        unsigned long ind = PFA_MAP_INDEX_V(_pa(pg->vaddr));
        _set_bit(pfa.tag_bits, ind);
}

static bool
is_avail(page_t *pg)
{
        unsigned long ind = PFA_MAP_INDEX_V(_pa(pg->vaddr));
        bool ret;
        ret = (bool)(!_tst_bit(pfa.tag_bits, ind));
        return ret;
}

void
pfa_init(memlimits_t *limits)
{
        bug_on(!limits, "NULL parameter");

        pfa.limits = limits;

        unsigned long low_pages  = limits->high_pfn;
        unsigned long all_pages  = limits->max_pfn;
        unsigned long pages_sz   = (all_pages * sizeof(page_t));
        unsigned long i;

        /* First of all, get some room for our global page list. */
        pfa.pages = reserve_low_pages(limits,
                ((PAGE_SIZE - 1 + pages_sz) / PAGE_SIZE));
        for (i = 0; i < pages_sz; i++) {
                pfa.pages[i].vaddr = (void *)_va(i * PAGE_SIZE);
                list_head_init(&pfa.pages[i].list);
        }
        /* Also get some room for our tag map. */
        pfa.tag_bits = reserve_low_pages(limits,
                ((PAGE_SIZE - 1 + (all_pages >> 3)) / PAGE_SIZE));
        bzero(pfa.tag_bits, (all_pages >> 3));

        /* Initialize the free page lists */
        for (i = 0; i < PFA_MAX_PAGE_ORDER; i++)
        {
                list_head_init(&pfa.low_zones[i].list);
                list_head_init(&pfa.high_zones[i].list);
        }

        /* Populate the free zone lists */
        unsigned int ind = limits->low_pfn;
        while (ind < low_pages)
        {
                unsigned long next_block_size, ord;
                if (low_pages - ind == 1) {
                        next_block_size = 1;
                } else {
                        next_block_size = MIN(1 << (PFA_MAX_PAGE_ORDER-1),
                                              (low_pages - ind) >> 1);
                }
                ord = order_of(next_block_size);

                bug_on(next_block_size == 0, "Infinite loop");
                bug_on(ind >= low_pages, "Exceeding low page bounds");
                bug_on(ord >= PFA_MAX_PAGE_ORDER, "Invalid zone order");

                page_t *pg = &pfa.pages[ind];
                pg->order = ord;
                list_add(&pfa.low_zones[ord].list, &pg->list);

                ind += next_block_size;
        }
        while (ind < all_pages)
        {
                unsigned long next_block_size, ord;
                if (all_pages - ind == 1) {
                        next_block_size = 1;
                } else {
                        next_block_size = MIN(1 << (PFA_MAX_PAGE_ORDER-1),
                                              (all_pages - ind) >> 1);
                }
                ord = order_of(next_block_size);

                bug_on(next_block_size == 0, "Infinite loop");
                bug_on(ind >= all_pages, "Exceeding high page bounds");
                bug_on(ord >= PFA_MAX_PAGE_ORDER, "Invalid zone order");

                page_t *pg = &pfa.pages[ind];
                pg->order = ord;
                list_add(&pfa.high_zones[ord].list, &pg->list);

                ind += next_block_size;
        }

        /* Once we hit the buddy allocator, we may not do any early
         * page reserving. */
        disable_reserve();
}

/* TODO: Improve this. */
page_t *
pfa_alloc_pages(pfa_alloc_flags_t flags, unsigned int order)
{
        unsigned int i;
        page_t *page;
        pfa_block_t *zone, *zones;

        bug_on((order >= PFA_MAX_PAGE_ORDER),
               "Invalid order for allocation.");

        if (flags & PFA_FLAGS_HIGHMEM) {
                zones = pfa.high_zones;
        } else {
                zones = pfa.low_zones;
        }

        for (i = order; i < PFA_MAX_PAGE_ORDER; i++)
        {
                zone = &zones[i];
                if (list_empty(&zone->list))
                        continue;
                page = list_first_entry(&zone->list, page_t, list);
                list_del(&page->list);
                mark_allocated(page);

                /* Trim if needed. */
                while (i > order)
                {
                        i--;
                        page_t *buddy = (page_t *)(page + (1UL << i));
                        buddy->order = i;
                        mark_avail(buddy);
                        list_add(&zones[i].list, &buddy->list);
                }
                page->order = i;

                return page;
        }

        return NULL;
}

static page_t *
find_buddy(page_t *page, unsigned int order)
{
        unsigned long _block, _buddy;

        bug_on(page < pfa.pages, "Invalid page reference");

        _block = (unsigned long)page - (unsigned long)pfa.pages;
        _buddy = _block ^ (1UL << order);

        return (page_t *)((unsigned long)pfa.pages + _buddy);
}

/* TODO: Actually write this. */
void
pfa_free_pages(page_t *p, unsigned int order)
{
        bug_on(!p, "NULL parameter");
        bug_on(order >= PFA_MAX_PAGE_ORDER, "Page zone too big");
        bug_on(is_avail(p), "Page not allocated before freeing");

        /* Try to coalesce. */
        while (order < PFA_MAX_PAGE_ORDER)
        {
                page_t *buddy = find_buddy(p, order);

                if (!is_avail(buddy))
                        break;
                if (buddy->order != order)
                        break;

                list_del(&buddy->list);
                if (buddy < p)
                        p = buddy;
                ++order;
                p->order = order;
        }

        p->order = order;
        mark_avail(p);
        if (is_highmem(pfa.limits, (void *)phys_addr(pfa.pages, p)))
                list_add(&pfa.high_zones[order].list, &p->list);
        else
                list_add(&pfa.low_zones[order].list, &p->list);
}

void
pfa_report(bool full)
{
        int i = 0;
        if (full) {
                kprintf(0, "=== Low Zones ===\n");
                for (i = 0; i < PFA_MAX_PAGE_ORDER; i++)
                {
                        kprintf(0, "%3d: %d\n",
                                (1 << i), list_size(&pfa.low_zones[i].list));
                }
                kprintf(0, "=== High Zones ===\n");
                for (i = 0; i < PFA_MAX_PAGE_ORDER; i++)
                {
                        kprintf(0, "%3d: %d\n",
                                (1 << i), list_size(&pfa.high_zones[i].list));
                }
        }

        kprintf(0, "%dMiB available (%dMiB low, %dMiB high)\n",
                B_MiB(allmem_bytes_avail(&mem_limits)),
                B_MiB(lowmem_bytes_avail(&mem_limits)),
                B_MiB(highmem_bytes_avail(&mem_limits)));

        kprintf(0, "====== Low Region =====\n",
                        lowmem_pages_avail(pfa.limits));
        kprintf(0, "0x%08x - 0x%08x\n", lowmem_base(pfa.limits),
                        lowmem_top(pfa.limits));
        kprintf(0, "===== High Region =====\n",
                        highmem_pages_avail(pfa.limits));
        kprintf(0, "0x%08x - 0x%08x\n", highmem_base(pfa.limits),
                        highmem_top(pfa.limits));
}

void
pfa_test(void)
{
#ifdef CONF_DEBUG
        page_t *pgs = pfa_alloc_pages(0, 4);
        page_t **pages = (page_t **)(pgs->vaddr);
        unsigned long num = (1 << 4) * PAGE_SIZE / sizeof(page_t *);
        int i, j;
        for (j = 0; j < 4; j++) {
                unsigned long c = 0;
                for (i = 0; i < num; i++)
                {
                        pages[i] = pfa_alloc_pages(0, (i + j) % PFA_MAX_PAGE_ORDER);
                        if (pages[i]) {
                                c += (1 << (i % PFA_MAX_PAGE_ORDER));
                        }
                }
                while (i-- >= 0)
                {
                        if (pages[i]) {
                                c -= (1 << (i % PFA_MAX_PAGE_ORDER));
                                pfa_free_pages(pages[i], (i + j) % PFA_MAX_PAGE_ORDER);
                        }
                }
                bug_on(c != 0, "Not all pages accounted for.\n");
        }
        kprintf(0, "pfa_test passed\n");
#endif
}
