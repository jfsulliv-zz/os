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

#ifndef _MM_PAGE_TABLE_H_
#define _MM_PAGE_TABLE_H_

#include <mm/flags.h>
#include <mm/pflags.h>
#include <sys/bitops_generic.h>

typedef unsigned int pgflags_t;

#define _PAGE_BIT_PRESENT       0
#define _PAGE_BIT_RW            1
#define _PAGE_BIT_USER          2
#define _PAGE_BIT_PWT           3
#define _PAGE_BIT_PCD           4
#define _PAGE_BIT_ACCESSED      5
#define _PAGE_BIT_DIRTY         6

#define _PAGE_PRESENT   0x001
#define _PAGE_RW        0x002
#define _PAGE_USER      0x004
#define _PAGE_PWT       0x008
#define _PAGE_PCD       0x010
#define _PAGE_ACCESSED  0x020
#define _PAGE_DIRTY     0x040

#define _PAGE_PROTNONE  0x080   /* If not present */

#define PAGE_FLAGS_MASK (GENMASK(11, 0))
#define PAGE_ADDR_MASK  (~PAGE_FLAGS_MASK)

#define PAGE_FLAGS_BAD(x) ((x) & (~PAGE_FLAGS_MASK))

#define PAGE_TAB  (_PAGE_PRESENT | _PAGE_USER | _PAGE_RW | \
                   _PAGE_ACCESSED | _PAGE_DIRTY)
#define KPAGE_TAB (_PAGE_PRESENT | _PAGE_RW )

static inline pgflags_t
pt_flags(mflags_t mflags, pflags_t pflags)
{
        pgflags_t ret = _PAGE_PRESENT;
        if (!mflags && !pflags)
                return _PAGE_PROTNONE;
        if (mflags & M_HIGH)
                ret |= _PAGE_USER;
        if ((pflags & PFLAGS_R) && (pflags & PFLAGS_W))
                ret |= _PAGE_RW;
        return ret;
}

#endif
