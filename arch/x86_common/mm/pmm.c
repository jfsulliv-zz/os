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

#include <machine/regs.h>
#include <machine/types.h>
#include <mm/reserve.h>
#include <mm/paging.h>
#include <mm/pmm.h>
#include <mm/pfa.h>
#include <mm/vma.h>
#include <sys/errno.h>
#include <sys/kprintf.h>
#include <sys/stdio.h>
#include <sys/panic.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <stdbool.h>

static bool initialized = false;

/* Init proc's page directory and pmm */
extern pgd_t init_pgd;
pmm_t init_pmm;

/* A memory cache for creating new pmm's */
static mem_cache_t *pmm_cache;
static bool pmm_late_ready = false;

/* Allocate a page that is mapped in to pmm and ready to go. */
static void *
alloc_page(pmm_t *pmm)
{
        page_t *page = pfa_alloc(M_KERNEL);
        if (!page)
                return NULL;
        paddr_t phys = page_to_phys(page);
        if (pmm_map(pmm, _va(phys), phys, M_KERNEL | M_ZERO, PFLAGS_RW)) {
                pfa_free(page);
                return NULL;
        }
        return (void *)_va(phys);
}

/* Free a page allocated by alloc_page(). */
static void
free_page(pmm_t *pmm, void *addr)
{
        pmm_unmap(pmm, (vaddr_t)addr, NULL);
        page_t *page = phys_to_page(_pa(addr));
        pfa_free(page);
}

static inline void
free_pte(pte_t *pte)
{
        pfa_free(phys_to_page(_pa(pte)));
}

#if PMD_BITS == 0
#define free_pmd(cur_pmm, pmd) free_pte((pte_t *)pmd)
#else
static inline void
free_pmd(pmm_t *cur_pmm, pmd_t *pmd)
{
        unsigned long i;
        pmm_map(cur_pmm, (vaddr_t)pmd, _pa(pmd), M_KERNEL, PFLAGS_R);
        for (i = 0; i < PMD_NUM; i++)
        {
                paddr_t phys = pgent_paddr(pmd->ents[i]);
                if (phys)
                        free_pte((pte_t *)_va(phys));
        }
        pmm_unmap(cur_pmm, (vaddr_t)pmd, NULL);
        pfa_free(phys_to_page(_pa(pmd)));
}
#endif

#if PUD_BITS == 0
#define free_pud(cur_pmm pud) free_pmd(cur_pmm, (pmd_t *)pud)
#else
static inline void
free_pud(pmm_t *cur_pmm, pud_t *pud)
{
        unsigned long i;
        pmm_map(cur_pmm, (vaddr_t)pud, _pa(pud), M_KERNEL, PFLAGS_R);
        for (i = 0; i < PUD_NUM; i++)
        {
                paddr_t phys = pgent_paddr(pud->ents[i]);
                if (phys)
                        free_pmd(cur_pmm, (pmd_t *)_va(phys));
        }
        pmm_unmap(cur_pmm, (vaddr_t)pud, NULL);
        pfa_free(phys_to_page(_pa(pud)));
}
#endif

static inline void
free_pgd(pmm_t *cur_pmm, pgd_t *pgd)
{
        unsigned long i;
        /* Make a temporary mapping so we can actually remove the thing. */
        pmm_map(cur_pmm, (vaddr_t)pgd, _pa(pgd), M_KERNEL, PFLAGS_R);
        for (i = 0; i < PGD_NUM; i++)
        {
                paddr_t phys = pgent_paddr(pgd->ents[i]);
                if (phys)
                        free_pud(cur_pmm, (pud_t *)_va(phys));
        }
        pmm_unmap(cur_pmm, (vaddr_t)pgd, NULL);
        pfa_free(phys_to_page(_pa(pgd)));
}

/* Allocates an unmapped free page for using as a table. */
static paddr_t
map_getpage(pmm_t *pmm)
{
        paddr_t ret;
        if (pfa_ready()) {
                page_t *pg = pfa_alloc(M_KERNEL);
                ret = pg ? page_to_phys(pg) : 0;
        } else {
                ret = reserve_low_pages(pmm->lim, 1);
        }
        return ret;
}

static inline int
pte_map(pte_t *pte, vaddr_t va, paddr_t pa, mflags_t flags,
        pflags_t pflags, paddr_t *old_pa)
{
        if (old_pa)
                *old_pa = pte->ents[PTE_IND(va)];
        pte->ents[PTE_IND(va)] = (pa | (paddr_t)pt_flags(flags, pflags));
        return 0;
}

#if PMD_BITS == 0
#define pmd_map(pmm, pmd, va, pa, flags, pflags, old_pa) \
        pte_map((pte_t *)(pmd), va, pa, flags, pflags, old_pa)
#else
static inline int
pmd_map(pmm_t *pmm, pmd_t *pmd, vaddr_t va, paddr_t pa, mflags_t flags,
        pflags_t pflags, paddr_t *old_pa)
{
        pte_t *pte;
        paddr_t p = pgent_paddr(pmd->ents[PMD_IND(va)]);
        pte = (pte_t *)(p == 0 ? 0 : _va(p));
        if (!pte) {
                p = map_getpage(pmm);
                if (!p)
                        return ENOMEM;
                pte = (pte_t *)_va(p);
                pmd->ents[PMD_IND(va)] = (p | KPAGE_TAB);
        }
        return pte_map(pte, va, pa, flags, pflags, old_pa);
}
#endif

#if PUD_BITS == 0
#define pud_map(pmm, pud, va, pa, flags, pflags, old_pa) \
        pmd_map(pmm, (pmd_t *)(pud), va, pa, flags, pflags, old_pa)
#else
static inline int
pud_map(pmm_t *pmm, pud_t *pud, vaddr_t va, paddr_t pa, mflags_t flags,
        pflags_t pflags, paddr_t *old_pa)
{
        pmd_t *pmd;
        paddr_t p = pgent_paddr(pud->ents[PUD_IND(va)]);
        pmd = (pmd_t *)(p == 0 ? 0 : _va(p));
        if (!pmd) {
                p = map_getpage(pmm);
                if (!p)
                        return ENOMEM;
                pmd = (pmd_t *)_va(p);
                pud->ents[PUD_IND(va)] = (p | KPAGE_TAB);
        }
        return pmd_map(pmm, pmd, va, pa, flags, pflags, old_pa);
}
#endif

static int
pgd_map(pmm_t *pmm, pgd_t *pgd, vaddr_t va, paddr_t pa, mflags_t flags,
        pflags_t pflags, paddr_t *old_pa)
{
        pud_t *pud;
        paddr_t p = pgent_paddr(pgd->ents[PGD_IND(va)]);
        pud = (pud_t *)(p == 0 ? 0 : _va(p));
        if (!pud) {
                p = map_getpage(pmm);
                if (!p)
                        return ENOMEM;
                pud = (pud_t *)_va(p);
                pgd->ents[PGD_IND(va)] = (p | KPAGE_TAB);
        }
        return pud_map(pmm, pud, va, pa, flags, pflags, old_pa);
}

static int
copy_pte(pte_t *dst, pte_t *src)
{
        unsigned long i;
        for (i = 0; i < PTE_NUM; i++)
        {
                paddr_t p = dst->ents[i];
                if (!pgent_paddr(src->ents[i]))
                        dst->ents[i] = _PAGE_PROTNONE;
                else
                        dst->ents[i] = src->ents[i];
        }
        return 0;
}

#if PMD_BITS == 0
#define copy_pmd(pmm, dst, src) copy_pte((pte_t *)dst, (pte_t *)src)
#else
static int
copy_pmd(pmm_t *cur_pmm, pmd_t *dst, pmd_t *src)
{
        unsigned long i;
        for (i = 0; i < PMD_NUM; i++)
        {
                void *v;
                pte_t *dpte, *spte;
                if (!pgent_paddr(src->ents[i]))
                        continue;
                v = alloc_page(cur_pmm);
                if (!v)
                        goto free_tables;
                dpte = (pte_t *)v;
                spte = (pte_t *)_va(pgent_paddr(src->ents[i]));
                dst->ents[i] = _pa(v) | KPAGE_TAB;
                copy_pte(dpte, spte);
                /* Remove the temp mapping but keep the page alloc'd. */
                pmm_unmap(cur_pmm, (vaddr_t)v, NULL);
        }
        return 0;
free_tables:
        while (i--)
        {
                void *v = (void *)_va(pgent_paddr(dst->ents[i]));
                free_page(cur_pmm, v);
        }
        return ENOMEM;
}
#endif

#if PUD_BITS == 0
#define copy_pud(pmm, dst, src) copy_pmd(pmm, (pmd_t *)dst, (pmd_t *)src)
#else
static int
copy_pud(pmm_t *cur_pmm, pud_t *dst, pud_t *src)
{
        unsigned long i;
        for (i = 0; i < PUD_NUM; i++)
        {
                void *v;
                pmd_t *dpmd, *spmd;
                if (!pgent_paddr(src->ents[i]))
                        continue;
                v = alloc_page(cur_pmm);
                if (!v)
                        goto free_tables;
                dpmd = (pmd_t *)v;
                spmd = (pmd_t *)_va(pgent_paddr(src->ents[i]));
                dst->ents[i] = _pa(v) | KPAGE_TAB;
                if (copy_pmd(cur_pmm, dpmd, spmd))
                        goto free_tables;
                /* Remove the temp mapping but keep the page alloc'd. */
                pmm_unmap(cur_pmm, (vaddr_t)v, NULL);
        }
        return 0;
free_tables:
        while (i--)
        {
                void *v = (void *)_va(pgent_paddr(dst->ents[i]));
                free_page(cur_pmm, v);
        }
        return ENOMEM;
}
#endif

static int
copy_pgd(pmm_t *cur_pmm, pgd_t *dst, pgd_t *src)
{
        unsigned long i;
        for (i = 0; i < PGD_NUM; i++)
        {
                void *v;
                pud_t *dpud, *spud;
                if (!pgent_paddr(src->ents[i]))
                        continue;
                v = alloc_page(cur_pmm);
                if (!v)
                        goto free_tables;
                dpud = (pud_t *)v;
                spud = (pud_t *)_va(pgent_paddr(src->ents[i]));
                dst->ents[i] = _pa(v) | KPAGE_TAB;
                if (copy_pud(cur_pmm, dpud, spud))
                        goto free_tables;
                /* Remove the temp mapping but keep the page alloc'd. */
                pmm_unmap(cur_pmm, (vaddr_t)v, NULL);
        }
        return 0;
free_tables:
        while (i--)
        {
                void *v = (void *)_va(pgent_paddr(dst->ents[i]));
                free_page(cur_pmm, v);
        }
        return ENOMEM;
}

static void
pmm_ctor(void *p, __attribute__((unused)) size_t sz)
{
        pmm_t *pmm = (pmm_t *)p;
        void *pgd = alloc_page(current_process()->control.pmm);
        if (!pgd) {
                /* We must check this later. */
                pmm->pgdir = NULL;
                return;
        }
        pmm->refct = 1;
        pmm->pgdir = pgd;
        pmm->pgdir_paddr = _pa(pgd);
}

static void
pmm_dtor(void *p, __attribute__((unused)) size_t sz)
{
        pmm_t *pmm = (pmm_t *)p;
        free_pgd(current_process()->control.pmm, pmm->pgdir);
}

static void
map_region(pgent_t *ent, size_t num, vaddr_t base, vaddr_t top,
           int ind, int max_ind)
{
        unsigned int i;
        for (i = 0;
             i < num && ind < max_ind;
             i++, ind++, base += PAGE_SIZE)
        {
                if (top && base >= top) {
                        break;
                }
                ent[ind] = _pa(base) | KPAGE_TAB;
        }
}

static void
init_mapping(pud_t *puds, size_t npuds, pmd_t *pmds, size_t npmds,
             pte_t *ptes, size_t nptes, vaddr_t base, vaddr_t top)
{
        map_region((pgent_t *)ptes, PTE_NUM * nptes, base, top,
                   0, PTE_NUM * nptes);
        map_region((pgent_t *)pmds, nptes, (vaddr_t)ptes, top,
                   PMD_IND(base), PMD_NUM * npuds);
        map_region((pgent_t *)puds, npmds, (vaddr_t)pmds, top,
                   PUD_IND(base), PUD_NUM);
}

/* Set up the pmm layer (initial page mappings, etc.) */
/* What we want to do here is to move all of our temporary kernel tables
 * into a new contiguous fixed location, except the page directory
 * itself which will stay placed.
 * This involves the following:
 *  1) Reserve enough pages to hold a full set of kernel page tables
 *  2) Set up these reserved pages to map everything from 0x0 to the
 *     end of the kernel (plus our additional allocations)
 *  3) Swap in the new PUDs into the PGD and flush the TLB
 *
 * The only 'gotcha' here is that our reserved pages are not mapped,
 * and we might need more page tables than we have statically allocated.
 * To get around this, we will reserve a few extra (temporary) page
 * tables as needed, and use them for mappings.
 */
void
pmm_init(memlimits_t *lim)
{
        size_t tables_region_sz;
        paddr_t tables_region;
        size_t pg0_index = PGD_IND(_va(lowmem_start(lim)));
        size_t num_ptes, num_pmds, num_puds;

        init_pmm.pgdir = &init_pgd;
        init_pmm.pgdir_paddr = _pa(init_pmm.pgdir);
        init_pmm.lim = lim;

        /* Determine how much space we need to hold a full set of kernel
         * page tables, keeping our original pgdir intact. */
        num_ptes = PTES_NEEDED(lowmem_bytes_avail(lim));
        num_pmds = PMDS_NEEDED(lowmem_bytes_avail(lim));
        num_puds = PUDS_NEEDED(lowmem_bytes_avail(lim));
        tables_region_sz = ( (PTE_SIZE * num_ptes)
                           + (PMD_SIZE * num_pmds)
                           + (PUD_SIZE * num_puds));

        /* Reserve enough memory to hold all of our page tables. Note
         * that we have *not* mapped them yet and we will definitely
         * page fault if we try to access them. */
        tables_region = reserve_low_pages(lim, PFN_UP(tables_region_sz));

        /* Map all of this region in so we can access it. */
        pmm_map_range(&init_pmm, _va(tables_region),
                      PFN_UP(tables_region_sz), tables_region,
                      M_ZERO | M_KERNEL, PFLAGS_RW);

        /* Now we set up this region to be properly mapped with respect
         * to itself. */
        pud_t *puds = (pud_t *)_va(tables_region);
        pmd_t *pmds = (pmd_t *)_va(tables_region + (PUD_SIZE * num_puds));
        pte_t *ptes = (pte_t *)_va(tables_region + (PUD_SIZE * num_puds)
                                                 + (PMD_SIZE * num_pmds));
        init_mapping(puds, num_puds, pmds, num_pmds, ptes, num_ptes,
                     KERN_BASE, _va(lowmem_top(lim)));

        /* Load our actual page directory up with the new tables. */
        map_region(init_pmm.pgdir->ents,
                   PGD_NUM - pg0_index,
                   (vaddr_t)puds,
                   _va(lowmem_top(lim)), pg0_index, PGD_NUM);

        /* Invalidate the TLB to load the new tables up. */
        pmm_activate(&init_pmm);
        initialized = true;
}

bool
pmm_initialized(void)
{
        return initialized;
}

/* Make advanced pmm features available. To be called after the vma
 * subsystem is online. */
void
pmm_init_late(void)
{
        pmm_cache = mem_cache_create("pmm_cache", sizeof(pmm_t),
                                     sizeof(pmm_t), 0,
                                     pmm_ctor, pmm_dtor);
        bug_on(!pmm_cache, "Failed to allocate PMM cache");
        pmm_late_ready = true;
}

/* Create a new physical map, setting its refcount to 1 */
pmm_t *
pmm_create(void)
{
        bug_on(!pmm_late_ready, "PMM late init not done.");
        return mem_cache_alloc(pmm_cache, M_KERNEL);
}

/* Copy the page table mappings from one pmm to another. */
int
pmm_copy(pmm_t *dst, pmm_t *src)
{
        if (!dst || !src)
                return 1;
        return copy_pgd(current_process()->control.pmm, dst->pgdir,
                        src->pgdir);
}

/* Decrease the reference count to the physical map. When the refcount
 * is zero, its resources are freed. We assume that at this point the
 * map contains no mappings and this can be asserted. */
void
pmm_destroy(pmm_t *p)
{
        if (!p)
                return;
        if (--p->refct == 0) {
                /* TODO check for no mappings left? */
                mem_cache_free(pmm_cache, p);
        }
}

/* Increase the reference count to the physical map. */
void
pmm_reference(pmm_t *p)
{
        p->refct++;
}

/* Create a mapping from the given vaddr region into the physical
 * address region. Returns 0 on success. */
int
pmm_map(pmm_t *p, vaddr_t va, paddr_t pa, mflags_t flags, pflags_t pflags)
{
        static int depth = 0;
#define PMM_MAX_DEPTH 8
        if (++depth > PMM_MAX_DEPTH) {
                panic("Maximum pmm_map depth execeeded");
        }
        int ret = pgd_map(p, p->pgdir, va, pa, flags, pflags, NULL);
        if (ret) {
                --depth;
                return ret;
        }
        if (flags & M_ZERO)
                bzero((void *)va, PAGE_SIZE);
        page_t *page = phys_to_page(pa);
        if (page) {
                page->vaddr = va;
        }
        --depth;
        return 0;
}

/* Remove the virtual mapping for vaddr. Assumes va is page aligned. */
void
pmm_unmap(pmm_t *p, vaddr_t va, paddr_t *ret_pa)
{
        paddr_t old_pa;
        pgd_map(p, p->pgdir, va, 0, 0, 0, &old_pa);
        page_t *page = phys_to_page(old_pa);
        if (page) {
                page->vaddr = 0;
        }
        if (ret_pa)
                *ret_pa = old_pa;
}

/* Hint to the implementation that all mappings will be removed shortly
 * with calls to pmm_unmap(), followed by a pmm_destroy() or
 * pmm_update(). The implementation may or may not unmap all pages in
 * this function, as decided by efficiency. */
void
pmm_unmapping_all(pmm_t *p)
{
        panic("TODO");
}

static pgent_t *
pte_find(pte_t *p, vaddr_t va)
{
        return p->ents + PTE_IND(va);
}

#if PUD_BITS == 0
#define pmd_find(p, va) pte_find((pte_t *)(p), va)
#else
static pgent_t *
pmd_find(pmd_t *p, vaddr_t va)
{
        paddr_t phys = pgent_paddr(p->ents[PTE_IND(va)]);
        return phys == 0 ? NULL : pte_find((pte_t *)_va(phys), va);
}
#endif

#if PUD_BITS == 0
#define pud_find(p, va) pmd_find((pmd_t *)(p), va)
#else
static pgent_t *
pud_find(pud_t *p, vaddr_t va)
{
        paddr_t phys = pgent_paddr(p->ents[PUD_IND(va)]);
        return phys == 0 ? NULL : pmd_find((pmd_t *)_va(phys), va);
}
#endif

static pgent_t *
pgd_find(pgd_t *p, vaddr_t va)
{
        paddr_t phys = pgent_paddr(p->ents[PGD_IND(va)]);
        return phys == 0 ? NULL : pud_find((pud_t *)_va(phys), va);
}

/* Set the protection flags of the VM range to pflags in the given map. */
void
pmm_setprot(pmm_t *p, vaddr_t sva, vaddr_t eva, pflags_t pflags)
{
        if (!p || (sva & (PAGE_SIZE - 1)) || (eva & (PAGE_SIZE - 1))
            || BAD_PFLAGS(pflags))
                return;
        while (sva < eva)
        {
                pgent_t *e = pgd_find(p->pgdir, sva);
                *e = pgent_paddr(*e) | pflags;
                sva += PAGE_SIZE;
        }
}

/* Set the protection flags for pg to pflags in every mapping. */
void
pmm_page_setprot(page_t *pg, pflags_t pflags)
{
        panic("TODO");
}

/* Returns true if a mapping exists for va, writing the physical address
 * into *ret_va. Otherwise, returns false. */
bool
pmm_getmap(pmm_t *p, vaddr_t va, paddr_t *ret_pa)
{
        if (!p || (va & (PAGE_SIZE - 1)))
                return false;
        pgent_t *ent = pgd_find(p->pgdir, va);
        if (ent) {
                if (ret_pa) *ret_pa = pgent_paddr(*ent);
                return true;
        }
        return false;
}

/* Activates the given pmm (i.e. making its mappings the ones valid for
 * the current state of execution). */
void
pmm_activate(pmm_t *p)
{
        if (!p || p->pgdir_paddr == 0)
                return;
        set_cr3(p->pgdir_paddr);
}

#define PM_MOD_BIT 0
#define PM_REF_BIT 1

#define PM_MOD          (1 << PM_MOD_BIT)
#define PM_REF          (1 << PM_REF_BIT)

/* Deactivate the given pmm, preparing for a subsequent activation. */
void
pmm_deactivate(pmm_t *p)
{
        panic("TODO");
}

static bool
pmm_get_attrs(page_t *pg, int bits)
{
        panic("TODO");
}

static bool
pmm_clear_attrs(page_t *pg, int clrbits)
{
        panic("TODO");
}

bool
pmm_is_modified(page_t *pg)
{
        return pmm_get_attrs(pg, PM_MOD);
}

bool
pmm_clear_modify(page_t *pg)
{
        return pmm_clear_attrs(pg, PM_MOD);
}

bool
pmm_is_referenced(page_t *pg)
{
        return pmm_get_attrs(pg, PM_REF);
}

bool
pmm_clear_reference(page_t *pg)
{
        return pmm_clear_attrs(pg, PM_REF);
}
