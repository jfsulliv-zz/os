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

#ifdef CONF_VMA_SLAB
#include <mm/vma_slab.h>
#else
#error No allocator specified (CONF_VMA_*).
#endif

void
vma_init(void);

void *kmalloc (unsigned long size, mflags_t flags);
void  kfree   (void *addr);
void *krealloc(void *addr, unsigned long size, mflags_t flags);

mem_cache_t *
mem_cache_create(const char *name, size_t size, size_t align,
                  mem_cache_flags_t flags,
                  void (*ctor)(void *, size_t),
                  void (*dtor)(void *, size_t));

int
mem_cache_destroy(mem_cache_t *cp);

void *
mem_cache_alloc(mem_cache_t *cp, mflags_t flags);

void
mem_cache_free(mem_cache_t *cp, void *obj);

void
vma_report(void);

void
vma_test(void);

#endif
