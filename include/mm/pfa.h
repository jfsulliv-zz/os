#ifndef _MM_PFA_H_
#define _MM_PFA_H_

#include <stdbool.h>
#include <mm/flags.h>
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
        bool          ready;
        pfa_block_t   dma_zones[PFA_MAX_PAGE_ORDER];
        pfa_block_t   low_zones[PFA_MAX_PAGE_ORDER];
        pfa_block_t   high_zones[PFA_MAX_PAGE_ORDER];
} pfa_t;

extern pfa_t pfa;

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
bool pfa_ready(void);
void pfa_report(bool full);

page_t *pfa_alloc_pages(mflags_t, unsigned int order);
void    pfa_free_pages (page_t *, unsigned int order);

static inline void *
alloc_pages(mflags_t flags, unsigned int order)
{
        page_t *p = pfa_alloc_pages(flags, order);
        if (!p)
                return NULL;
        return p->vaddr;
}

static inline void
free_pages(void *p, unsigned int order)
{
        if (!p)
                return;
        page_t *pg = (&pfa.pages[(_pa(p) >> PAGE_SHIFT)]);
        pfa_free_pages(pg, order);
}

page_t *pfa_base(void);

#define pfa_alloc(flags) pfa_alloc_pages(flags, 0)
#define pfa_free(page)   pfa_free_pages(page, 0)

#define alloc_page(flags) alloc_pages(flags, 0)
#define free_page(page)   free_pages(page, 0)

void pfa_test(void);

#endif
