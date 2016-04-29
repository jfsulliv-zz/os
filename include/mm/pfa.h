#ifndef _MM_PFA_H_
#define _MM_PFA_H_

#include <mm/paging.h>
#include <util/list.h>

#define PFA_MAX_PAGE_ORDER 10

typedef struct {
        struct list_head list;
        unsigned long flags;
        void *paddr;
} pfa_block_t;

typedef struct {
        memlimits_t   limits;
        pfa_block_t   low_zones[PFA_MAX_PAGE_ORDER];
        pfa_block_t   high_zones[PFA_MAX_PAGE_ORDER];
} pfa_t;

typedef unsigned char pfa_alloc_flags_t;

#define PFA_FLAGS_HIGHMEM_SHIFT 0
#define PFA_FLAGS_CONTIG_SHIFT  1

#define PFA_FLAGS_HIGHMEM (1 << PFA_FLAGS_HIGHMEM_SHIFT)
#define PFA_FLAGS_CONTIG  (1 << PFA_FLAGS_CONTIG_SHIFT)

void pfa_init(memlimits_t *limits);

pfa_block_t *pfa_alloc(pfa_alloc_flags_t, unsigned int order);
void  pfa_free (pfa_block_t *);

#endif
