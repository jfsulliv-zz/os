#include <machine/regs.h>
#include <machine/types.h>
#include <mm/reserve.h>
#include <mm/paging.h>
#include <mm/pmm.h>
#include <mm/pfa.h>
#include <mm/vma.h>
#include <sys/errno.h>
#include <sys/panic.h>

/* Init proc's page directory and pmm */
extern pgd_t init_pgd;
pmm_t init_pmm;

/* A memory cache for creating new pmm's */
static mem_cache_t *pmm_cache;
static bool pmm_late_ready = false;

static void
pmm_ctor(void *p, __attribute__((unused)) size_t sz)
{
        pmm_t *pmm = (pmm_t *)p;
        page_t *pdir_page = pfa_alloc(M_KERNEL);
        if (!pdir_page) {
                /* We must check this later. */
                pmm->pgdir = NULL;
                return;
        }
        pmm->refct = 1;
        pmm->pgdir = (void *)pdir_page->vaddr;
        pmm->pgdir_paddr = phys_addr(pfa_base(), pdir_page);
}

static void
pmm_dtor(void *p, __attribute__((unused)) size_t sz)
{
        pmm_t *pmm = (pmm_t *)p;
        free_page((void *)pmm->pgdir);
}

static void
map_region(pgent_t *ent, size_t num, paddr_t base, paddr_t top,
           paddr_t inc)
{
        unsigned long i = 0;
        for (; i < num; i++, base += inc)
        {
                if (base >= top)
                        break;
                ent[i] = base | KPAGE_TAB;
        }
}

static void
init_mapping(pud_t *puds, size_t npuds, pmd_t *pmds, size_t npmds,
             pte_t *ptes, size_t nptes, paddr_t base, paddr_t top)
{
        map_region((pgent_t *)ptes, (nptes * PTE_SIZE), base, top,
                   (1 << PAGE_SHIFT));
        map_region((pgent_t *)pmds, (npmds * PMD_SIZE), _pa(ptes), top,
                   (1 << PTE_SHIFT));
        map_region((pgent_t *)puds, (npuds * PUD_SIZE), _pa(pmds), top,
                   (1 << PMD_SHIFT));
}

/* Set up the pmm layer (initial page mappings, etc.) */
void
pmm_init(memlimits_t *lim)
{
        unsigned long tables_region_sz;
        paddr_t tables_region, tables_region_end;
        unsigned long pg0_index = PGD_IND(_va(lowmem_start(lim)));

        init_pmm.pgdir = &init_pgd;
        init_pmm.pgdir_paddr = _pa(init_pmm.pgdir);

        /* Determine how much space we need to hold a full set of kernel
         * page tables, keeping our original pgdir intact. */
        tables_region_sz = ( (PUD_SIZE * PUD_COUNT)
                           + (PMD_SIZE * PMD_COUNT)
                           + (PTE_SIZE * PTE_COUNT));
        tables_region_sz /= 4;

        /* Reserve enough memory to hold all of our page tables. Note
         * that we have *not* mapped them yet and we will definitely
         * page fault if we try to access them. */
        tables_region = reserve_low_pages(lim, PFN_UP(tables_region_sz));
        tables_region_end = tables_region + tables_region_sz;

        /* Map all of this region in so we can access it. */
        pmm_map_range(&init_pmm, _va(tables_region),
                      PFN_UP(tables_region_sz), tables_region,
                      M_KERNEL, PFLAGS_RW);

        /* Now we set up this region to be properly mapped with respect
         * to itself. */
        size_t npuds, npmds, nptes;
        pud_t *puds = (pud_t *)_va(tables_region);
        npuds = PUD_COUNT / 4;
        pmd_t *pmds = (pmd_t *)_va(tables_region + (PUD_SIZE * npuds));
        npmds = PMD_COUNT / 4;
        pte_t *ptes = (pte_t *)_va(tables_region + (PUD_SIZE * npuds)
                                                 + (PMD_SIZE * npmds));
        nptes = PTE_COUNT / 4;
        init_mapping(puds, npuds, pmds, npmds, ptes, nptes, 0,
                     tables_region_end);

        /* Load our actual page directory up with the new tables. */
        map_region((pgent_t *)init_pmm.pgdir->ents + pg0_index,
                   PGD_NUM - pg0_index, _pa(puds),
                   tables_region_end, PGD_SIZE);

        /* Invalidate the TLB to load the new tables up. */
        pmm_activate(&init_pmm);
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

static int
copy_pte(pte_t *dst, pte_t *src)
{
        unsigned long i;
        for (i = 0; i < PTE_NUM; i++)
        {
                if (!pgent_paddr(src->ents[i]))
                        dst->ents[i] = _PAGE_PROTNONE;
                else
                        dst->ents[i] = src->ents[i];
        }
        return 0;
}

#if PMD_BITS == 0 
#define copy_pmd(dst, src) copy_pte((pte_t *)dst, (pte_t *)src)
#else
static int
copy_pmd(pmd_t *dst, pmd_t *src)
{
        unsigned long i;
        for (i = 0; i < PMD_NUM; i++)
        {
                void *v;
                pte_t *dpte, *spte;
                if (!pgent_paddr(src->ents[i]))
                        continue;
                v = alloc_page(M_KERNEL);
                if (!v)
                        goto free_tables;
                dpte = (pte_t *)_va(pgent_paddr(dst->ents[i]));
                spte = (pte_t *)_va(pgent_paddr(src->ents[i]));
                dst->ents[i] = _pa(v) | KPAGE_TAB;
                copy_pte(dpte, spte);
        }
        return 0;
free_tables:
        while (i--)
        {
                void *v = (void *)_va(pgent_paddr(dst->ents[i]));
                free_page(v);
        }
        return ENOMEM;
}
#endif

#if PUD_BITS == 0 
#define copy_pud(dst, src) copy_pmd((pmd_t *)dst, (pmd_t *)src)
#else
static int
copy_pud(pud_t *dst, pud_t *src)
{
        unsigned long i;
        for (i = 0; i < PUD_NUM; i++)
        {
                void *v;
                pmd_t *dpmd, *spmd;
                if (!pgent_paddr(src->ents[i]))
                        continue;
                v = alloc_page(M_KERNEL);
                if (!v)
                        goto free_tables;
                dpmd = (pmd_t *)_va(pgent_paddr(dst->ents[i]));
                spmd = (pmd_t *)_va(pgent_paddr(src->ents[i]));
                dst->ents[i] = _pa(v) | KPAGE_TAB;
                if (copy_pmd(dpmd, spmd))
                        goto free_tables;
        }
        return 0;
free_tables:
        while (i--)
        {
                void *v = (void *)_va(pgent_paddr(dst->ents[i]));
                free_page(v);
        }
        return ENOMEM;
}
#endif

static int
copy_pgd(pgd_t *dst, pgd_t *src)
{
        unsigned long i;
        for (i = 0; i < PGD_NUM; i++)
        {
                void *v;
                pud_t *dpud, *spud;
                if (!pgent_paddr(src->ents[i]))
                        continue;
                v = alloc_page(M_KERNEL);
                if (!v)
                        goto free_tables;
                dpud = (pud_t *)_va(pgent_paddr(dst->ents[i]));
                spud = (pud_t *)_va(pgent_paddr(src->ents[i]));
                dst->ents[i] = _pa(v) | KPAGE_TAB;
                if (copy_pud(dpud, spud))
                        goto free_tables;
        }
        return 0;
free_tables:
        while (i--)
        {
                void *v = (void *)_va(pgent_paddr(dst->ents[i]));
                free_page(v);
        }
        return ENOMEM;
}

/* Copy the page table mappings from one pmm to another. */
int
pmm_copy(pmm_t *dst, pmm_t *src)
{
        if (!dst || !src)
                return 1;
        return copy_pgd(dst->pgdir, src->pgdir);
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

static inline int
pte_map(pte_t *pte, vaddr_t va, paddr_t pa, mflags_t flags, pflags_t pflags)
{
        pte->ents[PTE_IND(va)] = (pa | pt_flags(flags, pflags));
        return 0;
}

#if PMD_BITS == 0
#define pmd_map(pmd, va, pa, flags, pflags) \
        pte_map((pte_t *)pmd, va, pa, flags, pflags)
#else
static inline int
pmd_map(pmd_t *pmd, vaddr_t va, paddr_t pa, mflags_t flags, pflags_t pflags)
{
        pte_t *pte = (pte_t *)_va(pgent_paddr(pmd->ents[PUD_IND(va)]));
        if (!pte)
                return 1;
        return pte_map(pte, va, pa, flags, pflags);
}
#endif

#if PUD_BITS == 0
#define pud_map(pud, va, pa, flags, pflags) \
        pmd_map((pmd_t *)pud, va, pa, flags, pflags)
#else
static inline int
pud_map(pud_t *pud, vaddr_t va, paddr_t pa, mflags_t flags, pflags_t pflags)
{
        pmd_t *pmd = (pmd_t *)_va(pgent_paddr(pud->ents[PUD_IND(va)]));
        if (!pmd)
                return 1;
        return pmd_map(pmd, va, pa, flags, pflags);
}
#endif

static int
pgd_map(pgd_t *pgd, vaddr_t va, paddr_t pa, mflags_t flags, pflags_t pflags)
{
        unsigned long ind = PGD_IND(va);
        pud_t *pud = (pud_t *)_va(pgent_paddr(pgd->ents[ind]));
        return pud ? pud_map(pud, va, pa, flags, pflags) : 1;
}

/* Create a mapping from the given vaddr region into the physical
 * address region. Returns 0 on success. */
int
pmm_map(pmm_t *p, vaddr_t va, paddr_t pa, mflags_t flags, pflags_t pflags)
{
        return pgd_map(p->pgdir, va, pa, flags, pflags);
}

/* Remove the virtual mapping for vaddr. Assumes va is page aligned. */
void
pmm_unmap(pmm_t *p, vaddr_t va)
{
        pgd_map(p->pgdir, va, 0, 0, 0);
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
        if (!p)
                return NULL;
        return p->ents + PTE_IND(va);
}

#if PUD_BITS == 0
#define pmd_find(p, va) pte_find((pte_t *)(p), va)
#else
static pgent_t *
pmd_find(pmd_t *p, vaddr_t va)
{
        if (!p)
                return NULL;
        return pte_find((pte_t *)p->ents + PMD_IND(va));
}
#endif

#if PUD_BITS == 0
#define pud_find(p, va) pmd_find((pmd_t *)(p), va)
#else
static pgent_t *
pud_find(pud_t *p, vaddr_t va)
{
        if (!p)
                return NULL;
        return pmd_find((pmd_t *)p->ents + PUD_IND(va));
}
#endif

static pgent_t *
pgd_find(pgd_t *p, vaddr_t va)
{
        return pud_find((pud_t *)p->ents + PGD_IND(va), va);
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
