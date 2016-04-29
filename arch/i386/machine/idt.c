/*
 * arch/i386/kernel/idt.c
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 06/15
 */

#include <machine/idt.h>
#include <stdint.h>
#include <machine/system.h>

extern void idt_flush(unsigned int);

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

void idt_set_gate(unsigned char num, unsigned int base, unsigned short sel,
                unsigned char flags)
{
        /* Set base address */
        idt[num].base_lo   = base & 0xFFFF;
        idt[num].base_hi   = (base >> 16) & 0xFFFF;

        /* Set flags and selector */
        idt[num].sel       = sel;
        idt[num].type_attr = flags;
        idt[num].zero      = 0;
}

void idt_install_exception_handlers(void)
{
        int i;
        for (i = INT_EXCEPTION_BASE; i < INT_EXCEPTION_LIMIT; i++)
        {
                idt_set_gate(i, (unsigned int)idt_exception_handlers[i],
                             0x08, IDT_32_INTERRUPT);
        }
}

void idt_install(void)
{
        idtp.base = (unsigned int)&idt;
        idtp.limit = sizeof(struct idt_entry) * NUM_IDT_ENTRIES;

        idt_install_exception_handlers();

        idt_flush((unsigned int)&idtp);
}

