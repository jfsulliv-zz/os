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

#include <multiboot.h>

#include <machine/gdt.h>
#include <machine/idt.h>
#include <machine/irq.h>
#include <machine/tty.h>
#include <machine/timer.h>
#include <machine/pic.h>
#include <machine/regs.h>
#include <mm/init.h>
#include <mm/paging.h>
#include <sys/kprintf.h>
#include <sys/panic.h>

extern char kernel_end;
unsigned long highstart_pfn, highend_pfn;

memlimits_t mem_limits = { 0, 0, 0, 0, 0};

void
arch_init(multiboot_info_t *mbd)
{
        unsigned long start_pfn, max_pfn;

        /* Set up memory segmentation. */
        disable_interrupts();
        gdt_install();

        /* First up, set up a basic output device. */
        install_tty();
        kprintf_set_output_device(tty_get_output_device());
        kprintf(0,"Hello, world!\n");

        /* Register our interrupt handlers. */
        idt_install();
        irq_install();
        pic_remap(PIC1_OFFSET, PIC2_OFFSET);
        kprintf(0,"System tables installed.\n");

        timer_install();
        timer_phase(1000);

        enable_interrupts();
        kprintf(0,"Enabled interrupts.\n");

        /* Find out our memory limits. */
        if (!(mbd->flags & 1)) {
                return; // TODO manual memory map detection
        }
        start_pfn = PFN_UP(_pa(&kernel_end)); /* Next page after kernel */
        max_pfn   = PFN_DOWN(1024 * mbd->mem_upper);
        mem_limits.dma_pfn = 1;
        mem_limits.dma_pfn_end  = (KERN_OFFS / PAGE_SIZE);
        mem_limits.low_pfn  = start_pfn;
        bug_on(mem_limits.dma_pfn_end >= mem_limits.low_pfn,
                        "DMA overflow");
        mem_limits.high_pfn = start_pfn + ((max_pfn - start_pfn) >> 2);
        mem_limits.max_pfn  = max_pfn;
}
