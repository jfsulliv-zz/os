/*
 * mm/pfa.c
 *
 * Early page frame allocator. These pages will be permanently allocated
 * and cannot be recovered; this is intentional and typically you will
 * allocate ephermal structs here.
 *
 * This allocator will only touch low memory.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 04/16
 */

#include <mm/paging.h>
#include <mm/pfa.h>
#include <sys/kprintf.h>
#include <sys/panic.h>
#include <util/list.h>

static pfa_t pfa;

#define LOG2(X) ((unsigned) (8*sizeof (unsigned long) - __builtin_clzl((X)) - 1))

static inline unsigned int
order_of(unsigned long num_pages)
{
        return LOG2(num_pages);
}

void
pfa_init(memlimits_t *limits)
{
        bug_on(!limits, "NULL parameter");

        if (limits->low_pfn >= limits->high_pfn) {
                panic("No free pages");
        }
        pfa.limits.high_pfn = limits->high_pfn;
        pfa.limits.low_pfn  = limits->low_pfn;

        int i = 0;
        for (; i < PFA_MAX_PAGE_ORDER; i++)
        {
                list_head_init(&pfa.low_zones[i].list);
                list_head_init(&pfa.high_zones[i].list);
        }

        unsigned long high_pages = highmem_pages_avail(&pfa.limits);
        unsigned long low_pages = lowmem_pages_avail(&pfa.limits);

        while (high_pages > 0)
        {
                unsigned long next = (high_pages >>= 1);
                unsigned int ind = order_of(next);

        }
}

/* TODO: Improve this. */
pfa_block_t *
pfa_alloc(pfa_alloc_flags_t flags, unsigned int order)
{
        unsigned int i;
        pfa_block_t *block, *zone, *zones;

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
                block = list_entry(&zone->list, pfa_block_t, list);
                list_del(&block->list);
                //mark_allocated(block);

                /* Attempt to trim */
                while (i > order)
                {
                        panic("TODO splitting");
                }

                debug("Allocated 0x%08x pages at 0x%08x\n",
                      (1 << i), block);
                return block;
        }

        kprintf(0, "No free blocks found.\n");
        return NULL;
}

/* TODO: Actually write this. */
void
pfa_free(pfa_block_t *p)
{
        panic("TODO");
}
