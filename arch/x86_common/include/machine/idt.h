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

#ifndef __IDT_H_
#define __IDT_H_
/*
 * Definitions for the interrupt decriptor table.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 06/15
 */

#include <machine/params.h>
#include <stdint.h>

#if WORD_SIZE == 32
#include <machine/idt_i386.h>
#else
#include <machine/idt_i686.h>
#endif

#define NUM_IDT_ENTRIES         256
#define INT_EXCEPTION_BASE      0
#define INT_EXCEPTION_LIMIT     32
#define INT_EXCEPTION_NUM       32
#define INT_IRQ_BASE            32
#define INT_IRQ_LIMIT           48
#define INT_IRQ_NUM             16

/* IDT gate types */
#define IDT_TASK_GATE_TYP       0x5
#define IDT_16_INTR_GATE_TYP    0x6
#define IDT_16_TRAP_GATE_TYP    0x7
#define IDT_32_INTR_GATE_TYP    0xE
#define IDT_32_TRAP_GATE_TYP    0xF

#define IDT_PRES(x) (x << 0x7) // Present bit
#define IDT_DPL(x) ((x & 0x3) << 0x5) // DPL flags

#define IDT_32_INTERRUPT IDT_32_INTR_GATE_TYP | IDT_PRES(1) | \
                         IDT_DPL(0x0)

extern struct idt_entry idt[NUM_IDT_ENTRIES];
extern struct idt_ptr   idtp;

extern const char *exception_messages[INT_EXCEPTION_LIMIT];

void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel,
                  unsigned char flags);
void idt_install(void);

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

#endif /* __IDT_H_ */

