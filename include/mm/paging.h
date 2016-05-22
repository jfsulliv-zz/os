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
#include <machine/types.h>
#include <mm/arch_paging.h>
#include <mm/page_table.h>
#include <util/list.h>

typedef struct {
        vaddr_t vaddr;
        unsigned long order;
        struct list_head list;
} page_t;

static inline paddr_t
phys_addr(page_t *base, page_t *page)
{
        return (page - base) * PAGE_SIZE;
}

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
        return (lim->dma_pfn);
}

static inline unsigned long
dma_end(memlimits_t *lim)
{
        return (lim->dma_pfn_end);
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
        return (lim->low_pfn);
}

static inline unsigned long
lowmem_end(memlimits_t *lim)
{
        return (lim->high_pfn);
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
        return (lim->high_pfn);
}

static inline unsigned long
highmem_end(memlimits_t *lim)
{
        return (lim->max_pfn);
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
is_dma(memlimits_t *lim, paddr_t paddr)
{
        return (dma_base(lim) <= paddr && dma_top(lim) >= paddr);
}

static inline bool
is_lowmem(memlimits_t *lim, paddr_t paddr)
{
        return (lowmem_base(lim) <= paddr && lowmem_top(lim) >= paddr);
}

static inline bool
is_highmem(memlimits_t *lim, paddr_t paddr)
{
        return (highmem_base(lim) <= paddr && highmem_top(lim) >= paddr);
}

extern memlimits_t mem_limits;

#endif /* _MM_PAGING_H_ */
