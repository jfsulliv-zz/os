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

/*
 * machine/gdt.c
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 06/15
 */

#include <machine/cpu.h>
#include <machine/gdt.h>
#include <machine/tss.h>
#include <sys/string.h>
#include <stdint.h>

struct gdt_entry gdt[NUM_GDT_ENTRIES];
struct gdt_ptr   gp;

static const struct gdt_entry gdte_null = {
        .limit_low = 0,
        .base_low  = 0,
        .accessed  = 0,
        .read_write = 0,
        .conforming_expand_down = 0,
        .code = 0,
        .always_1 = 0,
        .dpl = 0,
        .present = 0,
        .limit_high = 0,
        .available = 0,
        .always_0 = 0,
        .big = 0,
        .gran = 0,
        .base_high = 0
};

static const struct gdt_entry gdte_code_ring0 = {
        .limit_low = 0xffff,
        .base_low  = 0,
        .accessed  = 0,
        .read_write = 1,
        .conforming_expand_down = 0,
        .code = 1,
        .always_1 = 1,
        .dpl = 0,
        .present = 1,
        .limit_high = 0xf,
        .available = 1,
        .always_0 = 0,
        .big = 1,
        .gran = 1,
        .base_high = 0
};

static const struct gdt_entry gdte_data_ring0 = {
        .limit_low = 0xffff,
        .base_low  = 0,
        .accessed  = 0,
        .read_write = 1,
        .conforming_expand_down = 0,
        .code = 0,
        .always_1 = 1,
        .dpl = 0,
        .present = 1,
        .limit_high = 0xf,
        .available = 1,
        .always_0 = 0,
        .big = 1,
        .gran = 1,
        .base_high = 0
};

static const struct gdt_entry gdte_code_ring3 = {
        .limit_low = 0xffff,
        .base_low  = 0,
        .accessed  = 0,
        .read_write = 1,
        .conforming_expand_down = 0,
        .code = 1,
        .always_1 = 1,
        .dpl = 3,
        .present = 1,
        .limit_high = 0xf,
        .available = 1,
        .always_0 = 0,
        .big = 1,
        .gran = 1,
        .base_high = 0
};

static const struct gdt_entry gdte_data_ring3 = {
        .limit_low = 0xffff,
        .base_low  = 0,
        .accessed  = 0,
        .read_write = 1,
        .conforming_expand_down = 0,
        .code = 0,
        .always_1 = 1,
        .dpl = 3,
        .present = 1,
        .limit_high = 0xf,
        .available = 1,
        .always_0 = 0,
        .big = 1,
        .gran = 1,
        .base_high = 0
};

static const struct gdt_entry gdte_tss = {
        .limit_low = 0, // Set up dynamically
        .base_low  = 0, // Set up dynamically
        .accessed  = 1, // 1 for TSS
        .read_write = 0,
        .conforming_expand_down = 0,
        .code = 1,
        .always_1 = 0, // 0 for TSS
        .dpl = 0,
        .present = 1,
        .limit_high = 0, // Set up dynamically
        .available = 0,
        .always_0 = 0,
        .big = 0,
        .gran = 0,
        .base_high = 0 // Set up dynamically
};

static const struct gdt_entry gdte_percpu = {
        .limit_low = 0, // Set up dynamically
        .base_low  = 0, // Set up dynamically
        .accessed  = 0,
        .read_write = 1,
        .conforming_expand_down = 0,
        .code = 0,
        .always_1 = 1,
        .dpl = 0,
        .present = 1,
        .limit_high = 0, // Set up dynamically
        .available = 0,
        .always_0 = 0,
        .big = 1,
        .gran = 0,
        .base_high = 0 // Set up dynamically
};

extern void gdt_flush(uint32_t, uint32_t);

static void
setup_percpu_gdte(const cpu_t *cpu, struct gdt_entry *gdte)
{
        unsigned long base = (unsigned long)cpu;
        unsigned long limit = sizeof(cpu_t);
        gdte->base_low = (base & 0xFFFFFF);
        gdte->base_high = ((base & 0xFF000000) >> 24);
        gdte->limit_low = (limit & 0xFFFF);
        gdte->limit_high = ((limit & 0xF0000) >> 16);
}

void
gdt_install(cpu_t *cpu)
{
        struct gdt_entry *gdt = &cpu->arch_cpu.gdt[0];
        struct gdt_ptr *gp = &cpu->arch_cpu.gp;
        gp->base = (uint32_t)gdt;
        gp->limit = sizeof(struct gdt_entry) * NUM_GDT_ENTRIES;

        // Copy the reference entries in.
        gdt[GDT_NULL_IND] = gdte_null;
        gdt[GDT_KCODE_IND] = gdte_code_ring0;
        gdt[GDT_KDATA_IND] = gdte_data_ring0;
        gdt[GDT_UCODE_IND] = gdte_code_ring3;
        gdt[GDT_UDATA_IND] = gdte_data_ring3;
        gdt[GDT_TSS_IND] = gdte_tss;
        gdt[GDT_PCPU_IND] = gdte_percpu;

        // Set up the TSS and percpu entries with the right addresses
        tss_setup_gdte(&cpu->arch_cpu.tss, gdt + GDT_TSS_IND);
        setup_percpu_gdte(cpu, gdt + GDT_PCPU_IND);

        // Write the GDT out
        gdt_flush((uint32_t)gp, GDT_PCPU_IND);

        // Go set up and write out the TSS, too
        tss_install(&cpu->arch_cpu.tss);
}
