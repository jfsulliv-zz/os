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
#include <mm/memlimits.h>
#include <mm/paging.h>
#include <util/list.h>

#define PFA_MAX_PAGE_ORDER 12

/* A block is a list of free pages of a common order. */
typedef struct {
        struct list_head list;
} pfa_block_t;

/* The Page Frame Allocator object which manages physical memory.
 *
 * The PFA segments memory into several zones (DMA, low, high):
 *      High - Userspace pages
 *      Low  - Kernel pages
 *      DMA  - Direct Memory Access pages
 *
 * Pages can be fetched from the PFA via the pfa_{alloc,free}_pages
 * routines. A global table of page structs is maintained and mapped
 * into memory; these page structs contain metadata used for allocation
 * purposes and for mapping physical page frames to virtual addresses.
 *
 * The pages returned by the PFA are *not* mapped into virtual memory
 * yet; this is the low-level mechanism by which physical pages can be
 * reserved that does not handle that. For general purpose memory
 * allocation, use the VMA system.
 */
typedef struct {
        memlimits_t  *limits;
        page_t       *pages;
        unsigned long *tag_bits;
        bool          ready;
        pfa_block_t   dma_zones[PFA_MAX_PAGE_ORDER];
        pfa_block_t   low_zones[PFA_MAX_PAGE_ORDER];
        pfa_block_t   high_zones[PFA_MAX_PAGE_ORDER];
} pfa_t;

/* The system-wide page frame allocator object. */
extern pfa_t pfa;

/* Returns the first page in memory. */
page_t *pfa_base(void);

/* Initialize the PFA subsystem. */
void pfa_init(memlimits_t *limits);
/* Returns true if the PFA system is ready, false otherwise. */
bool pfa_ready(void);
/* Dump out a report of the available memory. When `full' is set, give
 * extra details. */
void pfa_report(bool full);

__test void pfa_test(void);

#define PFA_MAP_INDEX(paddr)   ((unsigned long)paddr >> PAGE_SHIFT)

/* Translates a physical address to its corresponding page struct. */
static inline page_t *
phys_to_page(paddr_t paddr)
{
        if (!pfa_base())
                return NULL;
        return pfa_base() + PFA_MAP_INDEX(paddr);
}

/* Translate a page struct to its corresponding physical address. */
static inline paddr_t
page_to_phys(page_t *page)
{
        return (page - pfa_base()) * PAGE_SIZE;
}

/* Returns the first page in a range of physical pages of size
 * (1<<order). The pages are not mapped into virtual memory yet. */
page_t *pfa_alloc_pages(mflags_t, unsigned int order);
void    pfa_free_pages (page_t *, unsigned int order);

/* Convenience macros for single-page allocations. */
#define pfa_alloc(flags) pfa_alloc_pages(flags, 0)
#define pfa_free(page)   pfa_free_pages(page, 0)

#endif
