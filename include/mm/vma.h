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

#ifndef _MM_VMA_H_
#define _MM_VMA_H_

#include <mm/flags.h>
#include <sys/config.h>
#include <sys/debug.h>

#ifdef CONF_VMA_SLAB
#include <mm/vma_slab.h>
#else
#error No allocator specified (CONF_VMA_*).
#endif

/* Initialize the VMA subsystem. */
void vma_init(void);

/* Report vital statistics of the VMA subsystem. */
void vma_report(void);
__test void vma_test(void);

/* === General Purpose Allocation ===
 *
 * These allocation routines are suitable for on-demand allocation of
 * variable sizes. For objects that are frequently allocated, use the
 * memory cache routines instead.
 */

/* Allocate `size' bytes of memory from the kernel heap with the given
 * memory flags. Returns null on failure (i.e. insufficient memory). */
void *kmalloc (unsigned long size, mflags_t flags);
/* Free the memory at the given address. Assumes that `addr' was
 * returned by kmalloc(); if it wasn't, bad things will happen. */
void  kfree   (void *addr);
/* Adjust the allocation record for `addr' to the given size and flags.
 * Returns the new address (which is not necessarily the same as the
 * old address), or null on failure. */
void *krealloc(void *addr, unsigned long size, mflags_t flags);

/* === Memory Caches ===
 *
 * For objects that are frequently allocated and deallocated, it is
 * useful to maintain a cache of the objects that have already been
 * partially instantiated. This is especially useful when object
 * initialization is expensive. Memory caches provide an API to such
 * functionality.
 *
 * An object that is allocated from a cache will generally already have
 * its memory allocated by the VMA system; the system need only call the
 * initializer (ctor) on the memory and return it to the caller. For
 * deallocation, the dtor method is used to clean up the object.
 */

/* Create a new memory cache.
 *      name - A readable name of the cache.
 *      size - The size in bytes of objects in the cache.
 *      align - An optional minimum alignment that returned objects
 *              must satisfy. If align < size, this has no effect.
 *      flags - A set of flags to apply to the cache.
 *      ctor - An optional routine to call on any object to be alloc'd.
 *      dtor - An optional routine to call on any freed object.
 */
mem_cache_t *mem_cache_create(const char *name, size_t size,
                              size_t align, mem_cache_flags_t flags,
                              void (*ctor)(void *, size_t),
                              void (*dtor)(void *, size_t));

/* Destroy the given memory cache. Returns 0 on success or 1 on
 * failure (e.g. if the cache is still in use). */
int mem_cache_destroy(mem_cache_t *cp);

/* Allocate an object from the given memory cache. */
void *mem_cache_alloc(mem_cache_t *cp, mflags_t flags);

/* Free an object from the given memory cache. */
void mem_cache_free(mem_cache_t *cp, void *obj);

#endif
