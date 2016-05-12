#include <mm/init.h>
#include <mm/paging.h>
#include <mm/page_table.h>
#include <mm/reserve.h>

#include <sys/kprintf.h>
#include <sys/panic.h>

extern pgdir_t proc0_pdir;
extern pgtab_t pg0;
extern char kernel_end;

static void
init_pagetables(memlimits_t *lim)
{
        unsigned int i, j;
        unsigned long pg0_index;
        unsigned long vaddr;
        unsigned long tables_region, tables_region_end;
        pgtab_t *tab;

        /* First of all, fill up the first page table as needed to 
         * ensure that we have room to hand out some pages for new
         * tables. */
        pg0_index = PD_INDEX(_va(lowmem_start(lim)));
        vaddr = pg0_index * PDIR_SIZE;
        bug_on(_pa(vaddr) >= lowmem_top(lim),
              "End of kernel exceeds bytes available.");
        tab = &pg0;
        for (j = 0; j < PAGES_PER_PT; j++)
        {
                vaddr = (pg0_index * PDIR_SIZE) + (j * PAGE_SIZE);
                if (_pa(vaddr) >= lowmem_top(lim))
                        break;
                tab->ents[j].ent = (_pa(vaddr) | KPAGE_TAB);
        }

        /* Set up the high page directory entries to be unmapped. */
        for (i = 0; i < pg0_index; i++)
        {
                proc0_pdir.tabs[i].ent = _PAGE_PROTNONE;
        }

        /* Reserve enough physical memory to hold the rest of our page
         * tables. */
        tables_region
                = reserve_low_pages(lim, PTABS_PER_PD - (pg0_index + 1));
        tables_region_end
                = tables_region + (PAGE_SIZE * (PTABS_PER_PD -
                  (pg0_index + 1)));
        /* Now set up the rest of the page tables, allocating new
         * pages for them as we go. The last entry will be a recursive
         * map. */
        for (i = pg0_index + 1; i < PTABS_PER_PD-1; i++)
        {
                unsigned long vaddr = i * PDIR_SIZE;
                unsigned long tab_paddr;

                if (_pa(vaddr) >= lowmem_top(lim))
                        break;

                tab_paddr = tables_region + ((i - (pg0_index + 1))
                                             * PTAB_SIZE);
                bug_on(PAGE_FLAGS_MASK & tab_paddr,
                        "Page address contains flags");

                /* We will set up as many entries as it takes to map
                 * the full allocated region in. */
                if (_pa(vaddr) < tables_region_end) {
                        for (j = 0; j < PAGES_PER_PT; j++)
                        {
                                vaddr = (i * PDIR_SIZE) + (j * PAGE_SIZE);
                                if (_pa(vaddr) >= tables_region_end)
                                        break;
                                tab->ents[j].ent = (_pa(vaddr) | KPAGE_TAB);
                        }
                }

                proc0_pdir.tabs[i].ent = (tab_paddr | KPAGE_TAB);
        }
        /* Now set up our recursive mapping which lets us use the
         * page_directory and page_tables pointers for easy access. */
        proc0_pdir.tabs[PTABS_PER_PD - 1].ent
                = (_pa(&proc0_pdir) | KPAGE_TAB);
}

void
init_paging(memlimits_t *lim)
{
        init_pagetables(lim);

}
