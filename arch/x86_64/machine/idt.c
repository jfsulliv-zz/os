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
 * arch/i386/kernel/idt.c
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 06/15
 */

#include <machine/idt.h>
#include <stdint.h>
#include <machine/system.h>

struct idt_entry idt[NUM_IDT_ENTRIES];
struct idt_ptr   idtp;

extern void idt_flush(uint64_t);

void (*idt_exception_handlers[INT_EXCEPTION_NUM])(void) = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
        isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
};

const char *exception_messages[INT_EXCEPTION_LIMIT] = {
        "Division By Zero",
        "Debug",
        "Non Maskable Interrupt",
        "Breakpoint",
        "Into Detected Overflow",
        "Out of Bounds",
        "Invalid Opcode",
        "No Coprocessor",
        "Double Fault",
        "Coprocessor Segment Overrun",
        "Bad TSS",
        "Segment Not Present",
        "Stack Fault",
        "General Protection Fault",
        "Page Fault",
        "Unknown Interrupt",
        "Coprocessor Fault",
        "Alignment Check",
        "Machine Check",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved"
};

void idt_set_gate(unsigned char num, uint64_t base, uint16_t sel,
                  uint8_t flags)
{
        /* Set base address */
        idt[num].base_low  = base & 0xFFFF;
        idt[num].base_mid  = (base >> 16) & 0xFFFF;
        idt[num].base_high = (base >> 32);

        /* Set flags and selector */
        idt[num].sel    = sel;
        idt[num].flags  = flags;
        idt[num].always_0 = 0;
        idt[num].always_0_2 = 0;
}

void idt_install_exception_handlers(void)
{
        int i;
        for (i = INT_EXCEPTION_BASE; i < INT_EXCEPTION_LIMIT; i++)
        {
                idt_set_gate(i, (uint64_t)idt_exception_handlers[i],
                             0x08, IDT_32_INTERRUPT);
        }
}

void idt_install(void)
{
        idtp.base = (uint64_t)&idt;
        idtp.limit = sizeof(struct idt_entry) * NUM_IDT_ENTRIES;

        idt_install_exception_handlers();

        idt_flush((uint64_t)&idtp);
}

