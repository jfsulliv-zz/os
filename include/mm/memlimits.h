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

#ifndef _MM_MEMLIMITS_H_
#define _MM_MEMLIMITS_H_

#include <multiboot.h>
#include <stddef.h>
#include <mm/paging.h>

typedef struct {
        size_t max_pfn;
        size_t high_pfn;
        size_t low_pfn;
        size_t dma_pfn_end;
        size_t dma_pfn;
} memlimits_t;

void
detect_memory_limits(memlimits_t *to, multiboot_info_t *mbd);

void
limits_report(memlimits_t *);

static inline paddr_t
mem_max(memlimits_t *lim)
{
        return (lim->max_pfn * PAGE_SIZE);
}

static inline size_t
dma_start(memlimits_t *lim)
{
        return (lim->dma_pfn);
}

static inline size_t
dma_end(memlimits_t *lim)
{
        return (lim->dma_pfn_end);
}

static inline size_t
dma_pages_avail(memlimits_t *lim)
{
        return (lim->dma_pfn_end - lim->dma_pfn);
}

static inline size_t
dma_bytes_avail(memlimits_t *lim)
{
        return (PAGE_SIZE * dma_pages_avail(lim));
}

static inline size_t
lowmem_start(memlimits_t *lim)
{
        return (lim->low_pfn);
}

static inline size_t
lowmem_end(memlimits_t *lim)
{
        return (lim->high_pfn);
}

static inline size_t
lowmem_pages_avail(memlimits_t *lim)
{
        return (lim->high_pfn - lim->low_pfn);
}

static inline size_t
lowmem_bytes_avail(memlimits_t *lim)
{
        return (PAGE_SIZE * lowmem_pages_avail(lim));
}

static inline size_t
highmem_start(memlimits_t *lim)
{
        return (lim->high_pfn);
}

static inline size_t
highmem_end(memlimits_t *lim)
{
        return (lim->max_pfn);
}

static inline size_t
highmem_pages_avail(memlimits_t *lim)
{
        return (lim->max_pfn - lim->high_pfn);
}

static inline size_t
highmem_bytes_avail(memlimits_t *lim)
{
        return (PAGE_SIZE * highmem_pages_avail(lim));
}

static inline size_t
allmem_pages_avail(memlimits_t *lim)
{
        return (highmem_pages_avail(lim)
                + lowmem_pages_avail(lim)
                + dma_pages_avail(lim));
}

static inline size_t
allmem_bytes_avail(memlimits_t *lim)
{
        return (PAGE_SIZE * allmem_pages_avail(lim));
}

static inline paddr_t
dma_base(memlimits_t *lim)
{
        return (PAGE_SIZE * lim->dma_pfn);
}

static inline paddr_t
dma_top(memlimits_t *lim)
{
        return (PAGE_SIZE * lim->dma_pfn_end);
}

static inline paddr_t
lowmem_base(memlimits_t *lim)
{
        return (PAGE_SIZE * lim->low_pfn);
}

static inline paddr_t
lowmem_top(memlimits_t *lim)
{
        return (PAGE_SIZE * lim->high_pfn);
}

static inline paddr_t
highmem_base(memlimits_t *lim)
{
        return (PAGE_SIZE * lim->high_pfn);
}

static inline paddr_t
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


#endif
