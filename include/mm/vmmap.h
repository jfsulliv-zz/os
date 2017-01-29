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

#ifndef _MM_VMMAP_H_
#define _MM_VMMAP_H_

/*
 * mm/vmmap.h - Representation of an address space.
 *
 * Each userspace process has a single vmmap_t which describes the
 * memory map of the process.
 *
 * The map is split into a number of memory regions, each of which is
 * described by a vmmap_area_t. Each area contains a backing vmobject_t,
 * which is backed either by a file or is 'anonymous'. These objects can
 * be shared across processes and as such are reference-counted; see
 * mm/vmobject.h for details.
 *
 * Generally, regions are not directly created by callers, who are
 * instead concerned with inserting objects into a VM map. The following
 * routines add objects to a process' VM map at the first-fit
 * unallocated region:
 *
 *   vmmap_insert_object()
 *
 * A section of a map can be later unmapped (it need not be the entire
 * object):
 *
 *   vmmap_remove()
 *   vmmap_remove_object()
 *
 * When needed, an object can be directly mapped at a particular address.
 * This is mainly for initial process set-up, when program segments need
 * to be mapped at fixed locations.
 *
 *   vmmap_map_object_at()
 *
 *
 * The map regions are organized as an AVL tree indexed by the address
 * range of each entry. This structure is searched and modified with the
 * vmmap_find routine:
 *
 *   vmmap_find()
 *
 * Left-to-right iteration can be performed using VMMAP_FOREACH_AREA.
 * A secondary linked-list is used to maintain traversal order.
 *
 *   vmmap_area_t *areap = NULL;
 *   VMMAP_FOREACH_AREA(map, areap)
 *   {
 *      ...
 *   }
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 01/17
 */

struct vmmap;

#include <machine/types.h>
#include <mm/pmm.h>
#include <mm/vmobject.h>

typedef struct vmmap_area {
        vaddr_t start;          /* Start of the region */
        unsigned long size;     /* Length of the region */
        vmobject_t *object;     /* The underlying object */
        unsigned long offset;   /* Offset into object where the map starts */
        struct vmmap_area *next;/* Next highest area */
        struct vmmap_area *prev;/* Next lowest area */
        struct vmmap_area *par; /* Parent in AVL tree */
        struct vmmap_area *left;/* Left child in AVL tree */
        struct vmmap_area *right;/* Right child in AVL tree */
        unsigned int height;    /* Used by AVL tree */
} vmmap_area_t;

typedef struct vmmap {
        pmm_t *pmm;                /* Back-ref to PMM of owning process */
        vmmap_area_t *avl_head;    /* Head of AVL tree of areas */
        vmmap_area_t *areap;       /* Linked-list of areas for iteration */
        unsigned int num;          /* Number of areas in the map */
} vmmap_t;

// Initialize the given vmmap. pmm is the physical map to use during
// memory mapping.
void vmmap_init(vmmap_t *, pmm_t *pmm);

// Deinitialize the given vmmap, freeing its associated resources.
void vmmap_deinit(vmmap_t *);

/* Inserts a mapping for 'object'.
 * 'offset' describes how far into the object's data the map starts,
 * and should be internally page-aligned.
 * 'size' describes how many bytes of the object to map, and should be
 * page-aligned.
 * (offset + size should be no more than the size of the object)
 * Returns 0 on success and an error status on failure:
 *  EAGAIN - Area was already mapped
 *  ENOMEM - Failed to allocate kernel objects */
int vmmap_map_object_at(vmmap_t *map, vmobject_t *object,
                        unsigned long offset, vaddr_t start,
                        unsigned long size);

/* Insert a mapping 'object' plus 'offset' into the map at an arbitrary 
 * address that fits.  Returns 0 on success and an error status on
 * failure.
 * If 0 is returned, 'addrp' contains the address at which the object
 * was mapped into. */
int vmmap_insert_object(vmmap_t *map, vmobject_t *object,
                        unsigned long offset, void *addrp);

/* Returns the area in the map which contains 'addr' (or NULL if no
 * such area exists). */
vmmap_area_t *vmmap_find(const vmmap_t *map, vaddr_t addr);

/* Remove the given area from the VM map. May result in an area becoming
 * unmapped. */
void vmmap_remove(vmmap_t *map, vaddr_t addr, unsigned long size);

#endif
