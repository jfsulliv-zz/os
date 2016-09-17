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

#include <machine/gdt.h>
#include <machine/idt.h>
#include <machine/irq.h>
#include <machine/msr.h>
#include <machine/pic.h>
#include <machine/syscall.h>
#include <machine/timer.h>
#include <mm/pmm.h>
#include <sys/panic.h>

extern char SYSCALL_STACK_BASE_TOP;

static void
arch_init_fastsyscalls()
{
        /* TODO once we enable multiple CPUs we need to set these for
         * each logical core. */
        /* Assign the GDT entry that the kernel code runs at. */
        wrmsrl(IA32_SYSENTER_CS, GDT_KCODE_IND);
        /* We will manage our own stack, so set the initial stack to 0 */
        wrmsrl(IA32_SYSENTER_ESP, (uint64_t)&SYSCALL_STACK_BASE_TOP);
        /* The entry point will be the syscall_entry function */
        wrmsrl(IA32_SYSENTER_EIP, syscall_entry_stub);
}

void
arch_init(void)
{
        /* Set up memory segmentation and an IDT. */
        gdt_install();
        idt_install();
}

void
arch_init_late(void)
{
        arch_init_fastsyscalls();
}

/* We assume that interrupts are disabled when this is called. */
void
arch_init_irqs(void)
{
        bug_on(!pmm_initialized(), "IRQs initialized before pmm.");
        irq_install();
        pic_remap(PIC1_OFFSET, PIC2_OFFSET);
        timer_install();
        timer_phase(1000);
}
