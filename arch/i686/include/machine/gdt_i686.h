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

#ifndef _GDT_I686_H_
#define _GDT_I686_H_

#define GDT_NULL_IND    0
#define GDT_KCODE_IND   1
#define GDT_KDATA_IND   2
#define GDT_UCODE_IND   3
#define GDT_UDATA_IND   4
#define GDT_TSS_IND     5
//      extended        6

#define NUM_GDT_ENTRIES 7

struct gdt_entry
{
        unsigned limit_low:             16;
        unsigned base_low :             24;
        unsigned accessed :             1;
        unsigned read_write:            1;
        unsigned conforming_expand_down:1;
        unsigned code:                  1;
        unsigned always_1:              1;
        unsigned dpl:                   2;
        unsigned present:               1;
        unsigned limit_high:            4;
        unsigned available:             1;
        unsigned long_mode:             1;
        unsigned big:                   1;
        unsigned gran:                  1;
        unsigned base_high:             8;
} __attribute__((packed));

/* This is the format of TSS, Call gates, etc. */
struct gdt_entry_ext
{
        struct gdt_entry bottom;
        uint32_t base_higher;
        uint32_t always_0;

} __attribute__((packed));

struct gdt_ptr
{
        uint16_t limit;
        uint64_t base;
} __attribute__((packed));

#endif
