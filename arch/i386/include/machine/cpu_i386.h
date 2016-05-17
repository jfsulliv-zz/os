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

#ifndef __CPU_I386_H__
#define __CPU_I386_H__

#include <machine/gdt.h>
#include <machine/idt.h>

/*
 * machine/cpu_i386.h
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 08/15
 */

typedef struct {
        char manufacturer_string[13];
        int max_basic_input_val;
        int max_ext_input_val;
        int features_ecx, features_edx;
        int ext_features_ecx, ext_features_edx;
        char stepping, model, family, type;
        char cache_line_size, logical_processors, lapic_id;
        char cpu_brand[49];
} cpuid_t;

struct arch_cpu {
        cpuid_t cpuid;
        struct gdt_entry gdt[NUM_GDT_ENTRIES];
        struct gdt_ptr   gp;
        struct idt_entry idt[NUM_IDT_ENTRIES];
        struct idt_ptr   idtp;
        /* TODO struct tss_entry tss; */
};

#endif
