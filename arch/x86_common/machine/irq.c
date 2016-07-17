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
 * machine/irq.c
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 04/16
 */

#include <machine/idt.h>
#include <machine/irq.h>
#include <sys/string.h>
#include <machine/system.h>

void (*idt_irq_handlers[INT_IRQ_NUM])(void) = {
        irq0,  irq1,  irq2,  irq3,  irq4,  irq5,  irq6,  irq7,
        irq8,  irq9,  irq10, irq11, irq12, irq13, irq14, irq15
};

void irq_install_handler(int irq, void (*handler)(const struct irq_ctx *r))
{
        irq_routines[irq] = handler;
}

void irq_uninstall_handler(int irq)
{
        irq_routines[irq] = 0;
}

void irq_install(void)
{
        int i;
        for (i = INT_IRQ_BASE; i < INT_IRQ_LIMIT; i++)
        {
                idt_set_gate(i,
                        (vaddr_t)idt_irq_handlers[i - INT_IRQ_BASE],
                        0x08, IDT_32_INTERRUPT);
        }
        memset (irq_routines, 0, sizeof(void *) * INT_IRQ_NUM);
}
