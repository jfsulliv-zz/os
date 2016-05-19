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

#ifndef _MM_ARCH_PAGING_H_
#define _MM_ARCH_PAGING_H_

/*
 * mm/arch_paging.h
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 04/16
 */

#include <stdint.h>
#include <util/list.h>
#include <machine/params.h>

#define KERN_OFFS 0x00100000UL
#define PAGE_SHIFT 12
#define PD_SHIFT   22
#define PTABS_PER_PD 1024
#define PAGES_PER_PT 1024
#define PAGE_SIZE 4096
#define PTAB_SIZE (1 << PAGE_SHIFT)
#define PDIR_SIZE (1 << PD_SHIFT)
#define PGENT_ADDR(x)   (x & ~(PAGE_SIZE - 1))

#define PFN_UP(x)       (((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
#define PFN_DOWN(x)     ((x) >> PAGE_SHIFT)

#define _pa(x)  ((unsigned long)x-KERN_BASE)
#define _va(x)  ((unsigned long)x+KERN_BASE)

#define paddr_of(pfn) ((unsigned long)((pfn) << PAGE_SHIFT))

#define PT_INDEX(x) (((unsigned long)(x) >> PAGE_SHIFT) & 0x03FF)
#define PD_INDEX(x) (((unsigned long)(x) >> PD_SHIFT) & (PTABS_PER_PD - 1))


typedef struct {
        uint32_t ent;
} pgent_t;

static inline unsigned long
pgent_paddr(pgent_t *ent)
{
        return (ent->ent & (~(PAGE_SIZE - 1)));
}

#endif
