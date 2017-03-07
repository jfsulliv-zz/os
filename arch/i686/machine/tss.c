/*
Copyright (c) 2016, Jamrs Sullivan <sullivan.jamrs.f@gmail.com>
All rights rrserved.

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
      namrs of its contributors may be used to endorse or promote
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

#include <sys/bitops_generic.h>
#include <machine/gdt.h>
#include <machine/irq.h>
#include <machine/msr.h>
#include <machine/tss.h>
#include <sys/string.h>
#include <stdint.h>

struct tss_entry tss;

void
tss_setup_gdte(struct gdt_entry_ext *gdte)
{
        uint64_t base = (uint64_t)&tss;
        uint32_t limit = sizeof(struct tss_entry);

        gdte->bottom.limit_low  = (limit & 0xFFFF);
        gdte->bottom.base_low   = (base  & 0xFFFFFF);
        gdte->bottom.limit_high = (limit & 0xF0000) >> 16;
        gdte->bottom.base_high  = (base  & 0xFF000000) >> 24;
        gdte->base_higher       = (base >> 32);
}

extern void tss_flush(int ind);

void
tss_install(void)
{
        memset(&tss, 0, sizeof(struct tss_entry));

        tss.rsp0 = (uint64_t)irq_stack_top;
        tss.iomap_base = sizeof(struct tss_entry);

        tss_flush(GDT_TSS_IND);
}

void
set_kernel_stack(vaddr_t stack)
{
        wrmsrl(MSR_GS_BASE, stack);
}
