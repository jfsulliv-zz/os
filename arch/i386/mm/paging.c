#include <mm/paging.h>
#include <mm/pfa.h>
#include <sys/kprintf.h>
#include <sys/panic.h>

pgdir_t *page_directory = (pgdir_t *)0xFFFFF000;
pgtab_t *page_tables    = (pgtab_t *)0xFFC00000;

static void
pg_tab_map(pgtab_t *tab, void *vaddr, unsigned long paddr,
           pgflags_t flags)
{
        unsigned long pt_ind = PT_INDEX(vaddr);
        tab->ents[pt_ind].ent = (uint32_t)(paddr | flags);
}

static void
pg_tab_unmap(pgtab_t *tab, void *vaddr)
{
        unsigned long pt_ind = PT_INDEX(vaddr);
        tab->ents[pt_ind].ent = _PAGE_PROTNONE;
}

int
pg_map (void *vaddr, unsigned long paddr, pgflags_t flags)
{
        unsigned long pd_ind;
        pgent_t *ent;
        pgtab_t *tab;

        if (vaddr == NULL || paddr == 0) {
                return -1;
        } else if ((unsigned long)vaddr & (PAGE_SIZE - 1)) {
                return -1;
        } else if (paddr & (PAGE_SIZE - 1)) {
                return -1;
        }

        pd_ind = PD_INDEX(vaddr);
        ent = &page_directory->tabs[pd_ind];
        if (ent->ent & _PAGE_PROTNONE) {
                /* That's a miss. Let's set up a new table */
                page_t *new_pg = pfa_alloc(PFA_LOWMEM, 0);
                if (!new_pg)
                        return -1; /* TODO ENOMEM */
                ent->ent = (phys_addr(pfa_base(), new_pg) | KPAGE_TAB);
        }
        tab = (pgtab_t *)&page_tables[pd_ind];
        pg_tab_map(tab, vaddr, paddr, flags);
        return 0;
}

int
pg_unmap(void *vaddr)
{
        unsigned long pd_ind;
        pgent_t *ent;

        if (vaddr == NULL) {
                return -1;
        } else if ((unsigned long)vaddr & (PAGE_SIZE - 1)) {
                return -1;
        }

        pd_ind = PD_INDEX(vaddr);
        ent = &page_directory->tabs[pd_ind];
        if (!(ent->ent & _PAGE_PROTNONE)) {
                pgtab_t *tab = (pgtab_t *)&page_tables[pd_ind];
                pg_tab_unmap(tab, vaddr);
                return 0;
        }
        return -1;
}

int pg_map_pages(void *vaddr, size_t npg, unsigned long paddr,
                 pgflags_t flags)
{
        size_t i;
        for (i = 0; i < npg; i++)
        {
                int r = pg_map((unsigned long *)vaddr + (i * PAGE_SIZE),
                       paddr + (i * PAGE_SIZE), flags);
                if (r)
                        return r;
        }
        return 0;
}

int pg_unmap_pages(void *vaddr, size_t npg)
{
        size_t i;
        for (i = 0; i < npg; i++)
        {
                int r = pg_unmap((unsigned long *)vaddr + (i * PAGE_SIZE));
                if (r)
                        return r;
        }
        return 0;
}

unsigned long
pg_get_paddr(void *vaddr)
{
        unsigned long pt_index, pd_index;
        pgent_t *ent;

        pd_index = PD_INDEX(vaddr);
        pt_index = PT_INDEX(vaddr);

        if (page_directory->tabs[pd_index].ent & _PAGE_PROTNONE)
                return 0;
        ent = &page_tables[pd_index].ents[pt_index];
        if (ent->ent & _PAGE_PROTNONE)
                return 0;
        return pgent_paddr(ent);
}
