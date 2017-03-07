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
THE POSSIBILITY OF SUCH DAMAGE.  */

#include <machine/gdt.h>
#include <machine/irq.h>
#include <machine/msr.h>
#include <machine/syscall.h>
#include <sys/kprintf.h>
#include <sys/stdio.h>

void
init_msrs(void)
{
        wrmsrl(MSR_EFER, rdmsrl(MSR_EFER) | 1);

        /* GDT entries that user and kernel code run at */
        wrmsrl(MSR_STAR,
                ((uint64_t)(8ULL * (uint64_t)GDT_KCODE_IND) << 32) |
                ((uint64_t)(8ULL * (uint64_t)GDT_UCODE_IND) << 48));
        /* Long mode syscall entry point */
        wrmsrl(MSR_LSTAR, (uint64_t)syscall_entry_stub);
        /* Compat mode syscall entry point */
        // TODO wrmsrl(MSR_CSTAR, (uint64_t)syscall_compat_entry_stub);
}
