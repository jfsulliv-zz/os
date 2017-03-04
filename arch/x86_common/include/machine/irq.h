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

#ifndef __IRQ_H_
#define __IRQ_H_

#include <machine/system.h>
#include <machine/regs.h>
#include <machine/types.h>
#include <mm/paging.h>

/*
 * Definitions for the interrupt request handling.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 06/15
 */

#define IRQ_STACK_ORDER (2)
#define IRQ_STACK_SIZE  (PAGE_SIZE << IRQ_STACK_ORDER)
extern char irq_stack[IRQ_STACK_SIZE];
extern char *irq_stack_top;

/* Pushed onto the stack by the CPU before entering an ISR. */
struct irq_ctx {
        reg_t int_no, err_code;
        reg_t ip, cs, eflags, sp, ss;
};

static inline void
disable_interrupts(void)
{
        __asm__ __volatile__("cli\n" : : : "memory");
}

static inline void
enable_interrupts(void)
{
        __asm__ __volatile__("sti\n" : : : "memory");
}

void (*irq_routines[16])(const struct irq_ctx *r);

void irq_install_handler(int irq, void (*handler)(const struct irq_ctx *r));
void irq_uninstall_handler(int irq);
void irq_install(void);

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

#endif /* __IRQ_H_ */

