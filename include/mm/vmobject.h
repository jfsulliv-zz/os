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

#ifndef _MM_VMOBJECT_H_
#define _MM_VMOBJECT_H_

/*
 * mm/vmobject.h - Objects in a process address space.
 *
 * A vmobject_t is an abstraction of something that can be mapped into a
 * process address space. This is often backed by a file but can also be
 * an 'anonymous' region with no backing file.
 *
 * These objects can be shared between processes and are
 * reference-counted; once the last reference to an object is dropped,
 * the object may free its associated memory. These reference counts are
 * managed by VM regions (defined in mm/vmmap.h).
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 01/17
 */

#include <mm/pflags.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
        int refct;              /* Number of references to the object */
        size_t size;            /* Size, in bytes, of the object */
        pflags_t pflags;        /* Protection flags for the object */
        // TODO inode for non-anon
        // TODO pager (swap for anon, filesystem for non-anon)
        struct page *page;      /* Linked-list of owned pages */
} vmobject_t;

// Creates an object representing an anonymous region of size 'size'.
// 'size' will be page-aligned (rounding up).
vmobject_t *vmobject_create_anon(size_t size, pflags_t flags);
void vmobject_destroy(vmobject_t *);

#endif
