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

void irq_install_handler(int irq, void (*handler)(struct regs *r))
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
                        (unsigned int)idt_irq_handlers[i - INT_IRQ_BASE],
                        0x08, IDT_32_INTERRUPT);
        }
        memset (irq_routines, 0, sizeof(void *) * INT_IRQ_NUM);
}
