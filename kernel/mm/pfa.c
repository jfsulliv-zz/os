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
#include <sys/config.h>
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
        unsigned long ind = PFA_MAP_INDEX(phys_addr(pfa.pages, pg));
        _clr_bit(pfa.tag_bits, ind);
}

static void
mark_allocated(page_t *pg)
{
        unsigned long ind = PFA_MAP_INDEX(phys_addr(pfa.pages, pg));
        _set_bit(pfa.tag_bits, ind);
}

static bool
is_avail(page_t *pg)
{
        unsigned long ind = PFA_MAP_INDEX(phys_addr(pfa.pages, pg));
        return (bool)(!_tst_bit(pfa.tag_bits, ind));
}

void
pfa_init(memlimits_t *limits)
{
        unsigned long pages_phys;
        unsigned long pages_npg;
        unsigned long tag_bits_phys;
        unsigned long tag_bits_npg;
        unsigned long dma_pages;
        unsigned long low_pages;
        unsigned long high_pages;
        unsigned long all_pages  = allmem_pages_avail(limits);
        unsigned long pages_sz = (all_pages * sizeof(page_t));
        unsigned long i;


        bug_on(!limits, "NULL limits");

        pfa.limits = limits;

        /* First of all, get some room for our global page list. */
        pages_npg = ((PAGE_SIZE - 1 + pages_sz) / PAGE_SIZE);
        pages_phys = reserve_low_pages(limits, pages_npg);
        pfa.pages = (page_t *)_va(pages_phys);
        pg_map_pages(pfa.pages, pages_npg, pages_phys, KPAGE_TAB);

        /* Do the same for our tag bits. */
        tag_bits_npg = ((PAGE_SIZE - 1 + (all_pages >> 3)) / PAGE_SIZE);
        tag_bits_phys = reserve_low_pages(limits, tag_bits_npg);
        pfa.tag_bits = (unsigned long *)_va(tag_bits_phys);
        pg_map_pages(pfa.tag_bits, tag_bits_npg, tag_bits_phys, KPAGE_TAB);

        for (i = 0; i < pages_npg; i++) {
                /* Kernel and DMA pages get fixed mappings. */
                if (i < limits->high_pfn)
                        pfa.pages[i].vaddr = (void *)_va(i * PAGE_SIZE);
                else
                        pfa.pages[i].vaddr = NULL;
                list_head_init(&pfa.pages[i].list);
        }
        /* Also get some room for our tag map. */
        bzero(pfa.tag_bits, (all_pages >> 3));

        /* Initialize the free page lists */
        for (i = 0; i < PFA_MAX_PAGE_ORDER; i++)
        {
                list_head_init(&pfa.dma_zones[i].list);
                list_head_init(&pfa.low_zones[i].list);
                list_head_init(&pfa.high_zones[i].list);
        }

        dma_pages  = dma_pages_avail(limits);
        low_pages  = lowmem_pages_avail(limits);
        high_pages = highmem_pages_avail(limits);

        /* Populate the free zone lists */
        unsigned long ind = limits->dma_pfn;
        while (ind < dma_pages)
        {
                unsigned long next_block_size, ord;
                if (dma_pages - ind == 1) {
                        next_block_size = 1;
                } else {
                        next_block_size = MIN(1 << (PFA_MAX_PAGE_ORDER-1),
                                              (dma_pages - ind) >> 1);
                }
                ord = order_of(next_block_size);

                bug_on(next_block_size == 0, "Infinite loop");
                bug_on(ind >= dma_pages, "Exceeding dma page bounds");
                bug_on(ord >= PFA_MAX_PAGE_ORDER, "Invalid zone order");

                page_t *pg = &pfa.pages[ind];
                pg->order = ord;
                list_add(&pfa.dma_zones[ord].list, &pg->list);

                ind += next_block_size;
        }
        ind = 0;
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

                page_t *pg = &pfa.pages[ind + limits->low_pfn];
                pg->order = ord;
                list_add(&pfa.low_zones[ord].list, &pg->list);

                ind += next_block_size;
        }
        while (ind < high_pages)
        {
                unsigned long next_block_size, ord;
                if (high_pages - ind == 1) {
                        next_block_size = 1;
                } else {
                        next_block_size = MIN(1 << (PFA_MAX_PAGE_ORDER-1),
                                              (high_pages - ind) >> 1);
                }
                ord = order_of(next_block_size);

                bug_on(next_block_size == 0, "Infinite loop");
                bug_on(ind >= high_pages, "Exceeding high page bounds");
                bug_on(ord >= PFA_MAX_PAGE_ORDER, "Invalid zone order");

                page_t *pg = &pfa.pages[ind + limits->high_pfn];
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
pfa_alloc_pages(pfa_alloc_type_t type, pfa_alloc_flags_t flags,
                unsigned int order)
{
        unsigned int i;
        page_t *page;
        pfa_block_t *zone, *zones;

        bug_on((order >= PFA_MAX_PAGE_ORDER),
               "Invalid order for allocation.");

        bug_on(flags & PFA_FLAGS_CONTIG, "TODO: contig alloc");

        switch (type)
        {
                case PFA_DMA:
                        zones = pfa.dma_zones;
                        break;
                case PFA_LOWMEM:
                        zones = pfa.low_zones;
                        break;
                case PFA_HIGHMEM:
                        zones = pfa.high_zones;
                        break;
                case INVAL:
                default:
                        return NULL;
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
        if (is_dma(pfa.limits, phys_addr(pfa.pages, p)))
                list_add(&pfa.dma_zones[order].list, &p->list);
        else if (is_lowmem(pfa.limits, phys_addr(pfa.pages, p)))
                list_add(&pfa.low_zones[order].list, &p->list);
        else
                list_add(&pfa.high_zones[order].list, &p->list);
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

        kprintf(0, "Memory: %dMiB\n"
                   "%4s: %5dKiB\n"
                   "%4s: %5dMiB\n"
                   "%4s: %5dMiB\n",
                B_MiB(allmem_bytes_avail(&mem_limits)),
                "dma", B_KiB(dma_bytes_avail(&mem_limits)),
                "low", B_MiB(lowmem_bytes_avail(&mem_limits)),
                "high", B_MiB(highmem_bytes_avail(&mem_limits)));

        kprintf(0, "====== DMA Region =====\n");
        kprintf(0, "0x%08x - 0x%08x\n", dma_base(pfa.limits),
                        dma_top(pfa.limits));
        kprintf(0, "====== Low Region =====\n");
        kprintf(0, "0x%08x - 0x%08x\n", lowmem_base(pfa.limits),
                        lowmem_top(pfa.limits));
        kprintf(0, "===== High Region =====\n");
        kprintf(0, "0x%08x - 0x%08x\n", highmem_base(pfa.limits),
                        highmem_top(pfa.limits));
}

page_t *
pfa_base(void)
{
        return pfa.pages;
}

void
pfa_test(void)
{
#ifdef CONF_DEBUG
        /* First, ensure that each type of alloc stays in its region. */
        page_t *p;
        p = pfa_alloc(PFA_DMA, 0);
        bug_on((phys_addr(pfa.pages, p) < dma_start(pfa.limits))
                || (phys_addr(pfa.pages, p) >= dma_end(pfa.limits)),
                "DMA alloc out of range");
        pfa_free(p);
        p = pfa_alloc(PFA_LOWMEM, 0);
        bug_on((phys_addr(pfa.pages, p) < lowmem_start(pfa.limits))
                || (phys_addr(pfa.pages, p) >= lowmem_end(pfa.limits)),
                "Low alloc out of range");
        pfa_free(p);
        p = pfa_alloc(PFA_HIGHMEM, 0);
        bug_on((phys_addr(pfa.pages, p) < highmem_start(pfa.limits))
                || (phys_addr(pfa.pages, p) >= highmem_end(pfa.limits)),
                "High alloc out of range");
        pfa_free(p);

        kprintf(0, "pfa_test passed\n");
#endif
}
