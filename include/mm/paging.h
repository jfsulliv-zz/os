#ifndef _MM_PAGING_H_
#define _MM_PAGING_H_

/*
 * mm/paging.h
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 04/16
 */

#include <stdint.h>
#include <stddef.h>
#include <mm/arch_paging.h>
#include <mm/page_table.h>

typedef struct {
        void *vaddr;
        unsigned long order;
        struct list_head list;
} page_t;

static inline unsigned long
phys_addr(page_t *base, page_t *page)
{
        return (page - base) * PAGE_SIZE;
}

typedef struct {
        pgent_t ents[PAGES_PER_PT];
} pgtab_t;

typedef struct {
        pgent_t tabs[PTABS_PER_PD];
} pgdir_t;

extern pgdir_t *page_directory;
extern pgtab_t *page_tables;

typedef struct {
        unsigned long max_pfn;
        unsigned long high_pfn;
        unsigned long low_pfn;
        unsigned long dma_pfn_end;
        unsigned long dma_pfn;
} memlimits_t;

static inline unsigned long
mem_max(memlimits_t *lim)
{
        return (lim->max_pfn * PAGE_SIZE);
}

static inline unsigned long
dma_start(memlimits_t *lim)
{
        return (lim->dma_pfn * PAGE_SIZE);
}

static inline unsigned long
dma_end(memlimits_t *lim)
{
        return (lim->dma_pfn_end * PAGE_SIZE);
}

static inline unsigned long
dma_pages_avail(memlimits_t *lim)
{
        return (lim->dma_pfn_end - lim->dma_pfn);
}

static inline unsigned long
dma_bytes_avail(memlimits_t *lim)
{
        return (PAGE_SIZE * dma_pages_avail(lim));
}

static inline unsigned long
lowmem_start(memlimits_t *lim)
{
        return (lim->low_pfn * PAGE_SIZE);
}

static inline unsigned long
lowmem_end(memlimits_t *lim)
{
        return (lim->high_pfn * PAGE_SIZE);
}

static inline unsigned long
lowmem_pages_avail(memlimits_t *lim)
{
        return (lim->high_pfn - lim->low_pfn);
}

static inline unsigned long
lowmem_bytes_avail(memlimits_t *lim)
{
        return (PAGE_SIZE * lowmem_pages_avail(lim));
}

static inline unsigned long
highmem_start(memlimits_t *lim)
{
        return (lim->high_pfn * PAGE_SIZE);
}

static inline unsigned long
highmem_end(memlimits_t *lim)
{
        return (lim->max_pfn * PAGE_SIZE);
}

static inline unsigned long
highmem_pages_avail(memlimits_t *lim)
{
        return (lim->max_pfn - lim->high_pfn);
}

static inline unsigned long
highmem_bytes_avail(memlimits_t *lim)
{
        return (PAGE_SIZE * highmem_pages_avail(lim));
}

static inline unsigned long
allmem_pages_avail(memlimits_t *lim)
{
        return (highmem_pages_avail(lim)
                + lowmem_pages_avail(lim)
                + dma_pages_avail(lim));
}

static inline unsigned long
allmem_bytes_avail(memlimits_t *lim)
{
        return (PAGE_SIZE * allmem_pages_avail(lim));
}

static inline unsigned long
dma_base(memlimits_t *lim)
{
        return (PAGE_SIZE * lim->dma_pfn);
}

static inline unsigned long
dma_top(memlimits_t *lim)
{
        return (PAGE_SIZE * lim->dma_pfn_end);
}

static inline unsigned long
lowmem_base(memlimits_t *lim)
{
        return (PAGE_SIZE * lim->low_pfn);
}

static inline unsigned long
lowmem_top(memlimits_t *lim)
{
        return (PAGE_SIZE * lim->high_pfn);
}

static inline unsigned long
highmem_base(memlimits_t *lim)
{
        return (PAGE_SIZE * lim->high_pfn);
}

static inline unsigned long
highmem_top(memlimits_t *lim)
{
        return (PAGE_SIZE * lim->max_pfn);
}

static inline bool
is_dma(memlimits_t *lim, unsigned long paddr)
{
        return (dma_base(lim) <= paddr && dma_top(lim) >= paddr);
}

static inline bool
is_lowmem(memlimits_t *lim, unsigned long paddr)
{
        return (lowmem_base(lim) <= paddr && lowmem_top(lim) >= paddr);
}

extern memlimits_t mem_limits;
extern pgdir_t *proc0_pgdir_p;

/* Arch specific */
int pg_map(void *vaddr, unsigned long paddr, pgflags_t flags);
int pg_map_pages(void *vaddr, size_t npg, unsigned long paddr,
                 pgflags_t flags);
int pg_unmap(void *vaddr);
int pg_unmap_pages(void *vaddr, size_t npg);
unsigned long pg_get_paddr(void *vaddr);

#endif /* _MM_PAGING_H_ */
