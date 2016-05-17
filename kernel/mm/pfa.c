/*
Copyright (c) 2016, James Sullivan <sullivan.james.f@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote
      products derived from this software without specific prior
      written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER>
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * mm/pfa.c
 *
 * General page frame allocator.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 04/16
 */

#include <asm/bitops.h>
#include <mm/flags.h>
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
#include <util/math.h>

pfa_t pfa = { .ready = false };

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
        unsigned long all_pages;
        unsigned long pages_sz;
        unsigned long i;


        bug_on(!limits, "NULL limits");
        pfa.limits = limits;

        all_pages  = limits->max_pfn;
        pages_sz = (all_pages * sizeof(page_t));

        /* First of all, get some room for our global page list. */
        pages_npg = ((PAGE_SIZE - 1 + pages_sz) / PAGE_SIZE);
        pages_phys = reserve_low_pages(limits, pages_npg);
        pfa.pages = (page_t *)_va(pages_phys);
        bug_on(pg_map_pages(pfa.pages, pages_npg, pages_phys, KPAGE_TAB),
                "Failed to map pagelist to virtual address.");

        /* Do the same for our tag bits. */
        tag_bits_npg = ((PAGE_SIZE - 1 + (all_pages >> 3)) / PAGE_SIZE);
        tag_bits_phys = reserve_low_pages(limits, tag_bits_npg);
        pfa.tag_bits = (unsigned long *)_va(tag_bits_phys);
        bug_on(pg_map_pages(pfa.tag_bits, tag_bits_npg, tag_bits_phys,
               KPAGE_TAB), "Failed to map tag bits to virtual address.");

        for (i = 0; i < all_pages; i++) {
                list_head_init(&pfa.pages[i].list);
                pfa.pages[i].vaddr = NULL;
        }
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
        i = 0;
        while (i < dma_pages)
        {
                unsigned long next_block_size, ord;
                if (dma_pages - i == 1) {
                        next_block_size = 1;
                } else {
                        next_block_size = MIN(1 << (PFA_MAX_PAGE_ORDER-1),
                                              (dma_pages - i) >> 1);
                }
                ord = order_of(next_block_size);

                bug_on(next_block_size == 0, "Infinite loop");
                bug_on(ind >= dma_end(limits), "Exceeding dma page bounds");
                bug_on(ord >= PFA_MAX_PAGE_ORDER, "Invalid zone order");

                page_t *pg = &pfa.pages[ind];
                pg->order = ord;
                pg->vaddr = (void *)_va(ind * PAGE_SIZE);
                bug_on(_pa(pg->vaddr) < dma_base(limits), "oops");
                list_add(&pfa.dma_zones[ord].list, &pg->list);

                ind += next_block_size;
                i += next_block_size;
        }
        ind = limits->low_pfn;
        while (i < low_pages)
        {
                unsigned long next_block_size, ord;
                if (low_pages - i == 1) {
                        next_block_size = 1;
                } else {
                        next_block_size = MIN(1 << (PFA_MAX_PAGE_ORDER-1),
                                              (low_pages - i) >> 1);
                }
                ord = order_of(next_block_size);

                bug_on(next_block_size == 0, "Infinite loop");
                bug_on(ind >= lowmem_end(limits), "Exceeding low page bounds");
                bug_on(ord >= PFA_MAX_PAGE_ORDER, "Invalid zone order");

                page_t *pg = &pfa.pages[ind];
                pg->order = ord;
                pg->vaddr = (void *)_va(ind * PAGE_SIZE);
                bug_on(_pa(pg->vaddr) < lowmem_base(limits), "oops");
                list_add(&pfa.low_zones[ord].list, &pg->list);

                ind += next_block_size;
                i += next_block_size;
        }
        ind = limits->high_pfn;
        while (i < high_pages)
        {
                unsigned long next_block_size, ord;
                if (high_pages - i == 1) {
                        next_block_size = 1;
                } else {
                        next_block_size = MIN(1 << (PFA_MAX_PAGE_ORDER-1),
                                              (high_pages - i) >> 1);
                }
                ord = order_of(next_block_size);

                bug_on(next_block_size == 0, "Infinite loop");
                bug_on(ind >= highmem_end(limits), "Exceeding high page bounds");
                bug_on(ord >= PFA_MAX_PAGE_ORDER, "Invalid zone order");

                page_t *pg = &pfa.pages[ind];
                pg->order = ord;
                pg->vaddr = NULL;
                list_add(&pfa.high_zones[ord].list, &pg->list);

                ind += next_block_size;
                i += next_block_size;
        }

        /* Once we hit the buddy allocator, we may not do any early
         * page reserving. */
        disable_reserve();

        pfa.ready = true;
}

bool
pfa_ready(void)
{
        return pfa.ready;
}

/* TODO handle discontiguous allocation? */
page_t *
pfa_alloc_pages(mflags_t flags, unsigned int order)
{
        unsigned int i;
        page_t *page;
        pfa_block_t *zone, *zones;

        if (order >= PFA_MAX_PAGE_ORDER)
                return NULL;
        bug_on(!pfa.ready, "PFA used before initialization.");

        if (BAD_MFLAGS(flags))
                return NULL;

        if (flags & M_DMA) {
                zones = pfa.dma_zones;
        } else if (flags & M_HIGH) {
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
                        i--; page_t *buddy = (page_t *)(page + (1UL << i));
                        buddy->order = i;
                        if (page->vaddr) {
                                buddy->vaddr = (void *)_va(phys_addr(
                                                           pfa.pages,
                                                           buddy));
                        }
                        mark_avail(buddy);
                        list_add(&zones[i].list, &buddy->list);
                }
                page->order = i;

                /* Map the pages into the tab. */
                if (page->vaddr) {
                        bug_on(pg_map_pages(page->vaddr, (1 << page->order),
                                            phys_addr(pfa.pages, page),
                                            mflags_to_pgflags(flags)),
                               "Page mapping failed");
                }
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

void
pfa_free_pages(page_t *p, unsigned int order)
{
        bug_on(!pfa.ready, "PFA used before initialization.");
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

                if (p->vaddr) {
                        buddy->vaddr = (void *)_va(phys_addr(pfa.pages,
                                                             buddy));
                }

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

        /* Unmap the pages. */
        if (p->vaddr) {
                bug_on(pg_unmap_pages(p->vaddr, (1 << p->order)),
                        "Page unmapping failed");
        }
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
        p = pfa_alloc(M_DMA);
        bug_on(!is_dma(pfa.limits, phys_addr(pfa.pages, p)),
                        "DMA alloc out of range");
        pfa_free(p);
        p = pfa_alloc(M_KERNEL);
        bug_on(!is_lowmem(pfa.limits, phys_addr(pfa.pages, p)),
                        "Low alloc out of range");
        pfa_free(p);
        p = pfa_alloc(M_HIGH);
        bug_on(!is_highmem(pfa.limits, phys_addr(pfa.pages, p)),
                        "High alloc out of range");
        pfa_free(p);

        /* Now ensure that we get valid virtual addresses from
         * allocations. */
        char *foo = alloc_page(M_KERNEL);
        bug_on(!foo, "alloc_page returned NULL");
        bug_on(!is_lowmem(pfa.limits, _pa(foo)), "Out of bounds returned");
        foo[0] = 'h';
        foo[1] = 'i';
        foo[2] = '!';
        foo[3] = '\0';
        free_page(foo);

        kprintf(0, "pfa_test passed\n");
#endif
}
