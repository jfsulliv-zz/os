#ifndef _MM_PFA_H_
#define _MM_PFA_H_

#include <mm/paging.h>
#include <util/list.h>

#define PFA_MAX_PAGE_ORDER 10

typedef struct {
        struct list_head list;
} pfa_block_t;

typedef struct {
        memlimits_t  *limits;
        page_t       *pages;
        unsigned long *tag_bits;
        pfa_block_t   dma_zones[PFA_MAX_PAGE_ORDER];
        pfa_block_t   low_zones[PFA_MAX_PAGE_ORDER];
        pfa_block_t   high_zones[PFA_MAX_PAGE_ORDER];
} pfa_t;

extern pfa_t pfa;

typedef enum {
        PFA_DMA,
        PFA_LOWMEM,
        PFA_HIGHMEM,
        INVAL = -1
} pfa_alloc_type_t;

typedef unsigned char pfa_alloc_flags_t;

#define PFA_FLAGS_CONTIG_SHIFT  0

#define PFA_FLAGS_CONTIG  (1 << PFA_FLAGS_CONTIG_SHIFT)

#define PFA_MAP_INDEX(paddr)   ((unsigned long)paddr >> PAGE_SHIFT)
#define PFA_MAP_INDEX_V(vaddr) (PFA_MAP_INDEX(_pa(vaddr)))

static inline page_t *
virt_to_page(void *vaddr)
{
        return pfa.pages + PFA_MAP_INDEX_V(vaddr);
}

static inline page_t *
phys_to_page(void *paddr)
{
        return pfa.pages + PFA_MAP_INDEX(paddr);
}

void pfa_init(memlimits_t *limits);
void pfa_report(bool full);

page_t *pfa_alloc_pages(pfa_alloc_type_t, pfa_alloc_flags_t,
                        unsigned int order);
void    pfa_free_pages (page_t *, unsigned int order);

page_t *pfa_base(void);

#define pfa_alloc(type, flags) pfa_alloc_pages(type, flags, 0)
#define pfa_free(page) pfa_free_pages(page, 0)

void pfa_test(void);

#endif
