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

typedef struct {
        pgent_t ents[PAGES_PER_PT];
} pgtab_t;

typedef struct {
        pgtab_t *tabs[PTABS_PER_PD];
} pgdir_t;

typedef struct {
        unsigned long max_pfn;
        unsigned long high_pfn;
        unsigned long low_pfn;
} memlimits_t;

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
allmem_pages_avail(memlimits_t *lim)
{
        return (lim->max_pfn - lim->low_pfn);
}

static inline unsigned long
allmem_bytes_avail(memlimits_t *lim)
{
        return (PAGE_SIZE * allmem_pages_avail(lim));
}

extern memlimits_t mem_limits;
extern pgdir_t *proc0_pgdir_p;

/* Arch specific */
void init_pgdir(pgdir_t *);
void init_pgtab(pgtab_t *);
void init_pgent(pgent_t *);

#endif /* _MM_PAGING_H_ */
