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

#ifndef _MM_PMM_H_
#define _MM_PMM_H_

/*
 * mm/pmm.h - Physical Memory Manager
 *
 * This is the common abstraction layer for managing physical memory.
 * It exposes routines to manage the page tables which are then
 * implemented in an architecture-dependent way.
 *
 * This is heavily influenced by the pmap module in BSD, Mach, etc.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 05/16
 */

#include <machine/types.h>
#include <mm/arch_pmm.h>
#include <mm/paging.h>
#include <stdbool.h>

extern pmm_t init_pmm;

/* Set up the pmm layer (initial page mappings, etc.) */
void
pmm_init(memlimits_t *);

/* Make advanced pmm features available. To be called after the vma
 * subsystem is online. */
void
pmm_init_late(void);

/* Create a new physical map, setting its refcount to 1 */
pmm_t *
pmm_create(void);

/* Copy the page table mappings from one pmm to another. */
int
pmm_copy(pmm_t *dst, pmm_t *src);

/* Decrease the reference count to the physical map. When the refcount
 * is zero, its resources are freed. We assume that at this point the
 * map contains no mappings and this can be asserted. */
void
pmm_destroy(pmm_t *);

/* Increase the reference count to the physical map. */
void
pmm_reference(pmm_t *);

/* Create a mapping from the given vaddr region into the physical
 * address region, assuming both are page-aligned. Returns 0 on success. */
int
pmm_map(pmm_t *, vaddr_t va, paddr_t pa, mflags_t flags, pflags_t pflags);

static inline int
pmm_map_range(pmm_t *p, vaddr_t va, size_t npg, paddr_t pa, mflags_t flags,
              pflags_t pflags)
{
        while (npg-- > 0)
        {
                int r = pmm_map(p, va, pa, flags, pflags);
                if (r)
                        return r;
                va += PAGE_SIZE;
                pa += PAGE_SIZE;
        }
        return 0;
}

/* Remove the virtual mapping for vaddr. Assumes va is page aligned. */
void
pmm_unmap(pmm_t *, vaddr_t va);

static inline void
pmm_unmap_range(pmm_t *p, vaddr_t va, vaddr_t eva)
{
        while (va < eva)
        {
                pmm_unmap(p, va);
                va += PAGE_SIZE;
        }
}
/* Hint to the implementation that all mappings will be removed shortly
 * with calls to pmm_unmap(), followed by a pmm_destroy() or
 * pmm_update(). The implementation may or may not unmap all pages in
 * this function, as decided by efficiency. */
void
pmm_unmapping_all(pmm_t *);

/* Set the protection flags of the VM range to pflags in the given map. */
void
pmm_setprot(pmm_t *, vaddr_t sva, vaddr_t eva, pflags_t pflags);

/* Set the protection flags for pg to pflags in every mapping. */
void
pmm_page_setprot(page_t *pg, pflags_t pflags);

/* Returns true if a mapping exists for va, writing the physical address
 * into *ret_va. Otherwise, returns false. */
bool
pmm_getmap(pmm_t *, vaddr_t va, paddr_t *ret_pa);

/* Activates the given pmm (i.e. making its mappings the ones valid for
 * the current state of execution). */
void
pmm_activate(pmm_t *);

/* Deactivate the given pmm, preparing for a subsequent activation. */
void
pmm_deactivate(pmm_t *);

/* Check/clear the modified bit in the given page. */
bool
pmm_is_modified(page_t *pg);
bool
pmm_clear_modify(page_t *pg);

/* Check/clear the referenced bit in the given page. */
bool
pmm_is_referenced(page_t *pg);
bool
pmm_clear_reference(page_t *pg);

#endif
