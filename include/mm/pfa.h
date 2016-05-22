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

#ifndef _MM_PFA_H_
#define _MM_PFA_H_

#include <stdbool.h>
#include <sys/debug.h>
#include <mm/flags.h>
#include <mm/paging.h>
#include <util/list.h>

#define PFA_MAX_PAGE_ORDER 12

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
        return (void *)p->vaddr;
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

__test void pfa_test(void);

#endif
