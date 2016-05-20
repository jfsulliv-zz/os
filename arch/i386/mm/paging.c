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

#include <mm/paging.h>
#include <mm/pfa.h>
#include <sys/config.h>
#include <sys/errno.h>
#include <sys/kprintf.h>
#include <sys/panic.h>

/* By recursively mapping our last page dir entry to itself, we can use
 * these virtual addresses to locate the page tables and directory for
 * the current process. */
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
                return EINVAL;
        } else if ((unsigned long)vaddr & (PAGE_SIZE - 1)) {
                return EINVAL;
        } else if (paddr & (PAGE_SIZE - 1)) {
                return EINVAL;
        } else if (PAGE_FLAGS_BAD(flags)) {
                return EINVAL;
        }

        pd_ind = PD_INDEX(vaddr);
        ent = &page_directory->tabs[pd_ind];
        if (ent->ent & _PAGE_PROTNONE) {
                if ((unsigned long)vaddr >= KERN_OFFS)
                        panic("FATAL: Kernel page table not present");
                /* That's a table miss. Allocate a page for it. */
                bug_on(!pfa_ready(), "PFA not initialized!");
                page_t *pg = pfa_alloc(M_KERNEL);
                if (!pg)
                        return ENOMEM;
                ent->ent = phys_addr(pfa_base(), pg);
                ent->ent |= KPAGE_TAB;
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
                return EINVAL;
        } else if ((unsigned long)vaddr & (PAGE_SIZE - 1)) {
                return EINVAL;
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

        if (!vaddr || npg == 0 || paddr == 0 || PAGE_FLAGS_BAD(flags))
                return EINVAL;

        for (i = 0; i < npg; i++)
        {
                int r = pg_map((unsigned char *)vaddr + (i * PAGE_SIZE),
                       paddr + (i * PAGE_SIZE), flags);
                if (r)
                        return r;
        }
        return 0;
}

int pg_unmap_pages(void *vaddr, size_t npg)
{
        size_t i;

        if (!vaddr || npg == 0)
                return EINVAL;

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

static int
copy_pgent(pgtab_t *to, unsigned long to_ind,
           pgtab_t *from, unsigned long from_ind)
{
        to->ents[to_ind].ent = from->ents[from_ind].ent;
        return 0;
}

static int
copy_pgtab(pgdir_t *to, unsigned long to_ind,
           pgdir_t *from, unsigned long from_ind)
{
        unsigned long pt_index;
        pgtab_t *to_tab, *from_tab;

        /* Allocate a new page table and set it in the to pgdir */
        page_t *new_pg = pfa_alloc(M_KERNEL);
        if (!new_pg)
                return ENOMEM;
        to_tab = new_pg->vaddr;
        to->tabs[to_ind].ent = phys_addr(pfa.pages, new_pg) |
                               PGENT_FLAGS(from->tabs[from_ind].ent);
        from_tab = page_tables + from_ind;

        /* Populate this table */
        for (pt_index = 0; pt_index < PAGES_PER_PT;
             pt_index++)
        {
                if (from_tab->ents[pt_index].ent & _PAGE_PROTNONE)
                        continue;
                if (copy_pgent(to_tab, pt_index, from_tab, pt_index))
                        return ENOMEM;
        }
        return 0;
}

int
copy_pgdir(pgdir_t *to, pgdir_t *from)
{
        unsigned long addr = 0;
        unsigned long pd_index;

        if (!to || !from)
                return EINVAL;

        for (pd_index = 0; pd_index < PTABS_PER_PD;
             pd_index++, addr += PDIR_SIZE)
        {
                if (from->tabs[pd_index].ent & _PAGE_PROTNONE)
                        continue;
                if (copy_pgtab(to, pd_index, from, pd_index))
                        return ENOMEM;
        }
        return 0;
}

pgdir_t *
alloc_pgdir(void)
{
        page_t *pg = alloc_page(M_KERNEL);
        if (!pg)
                return NULL;
        return (pgdir_t *)pg->vaddr;
}
