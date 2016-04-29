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
        int i, j;
        unsigned long vaddr;
        pgtab_t *tab;

        /* First of all, fill up the first page table as needed to 
         * ensure that we have room to hand out some pages for new
         * tables. */
        i = PD_INDEX(lowmem_start(lim));
        vaddr = i * PDIR_SIZE;
        bug_on(_pa(vaddr) >= lowmem_bytes_avail(lim),
              "End of kernel exceeds bytes available.");
        tab = &pg0;
        for (j = 0; j < PAGES_PER_PT; j++)
        {
                vaddr = (i * PDIR_SIZE) + (j * PAGE_SIZE);
                if (_pa(vaddr) >= lowmem_bytes_avail(lim))
                        break;
                tab->ents[j].ent = (_pa(vaddr) | KPAGE_TAB);
        }

        /* Now set up the rest of the page tables, allocating new
         * pages for them as we go. */
        for (i = PD_INDEX(lowmem_start(lim)) + 1; i < PTABS_PER_PD; i++)
        {
                unsigned long vaddr = i * PDIR_SIZE;
                pgtab_t *tab;

                if (_pa(vaddr) >= lowmem_bytes_avail(lim))
                        break;

                tab = (pgtab_t *)reserve_low_pages(lim, 1);

                for (j = 0; j < PAGES_PER_PT; j++)
                {
                        vaddr = (i * PDIR_SIZE) + (j * PAGE_SIZE);
                        if (_pa(vaddr) >= lowmem_bytes_avail(lim))
                                break;
                        tab->ents[j].ent = (_pa(vaddr) | KPAGE_TAB);
                }

                proc0_pdir.tabs[i] = (pgtab_t *)(_pa(tab) | KPAGE_TAB);
        }
}

void
init_paging(memlimits_t *lim)
{
        init_pagetables(lim);
}
