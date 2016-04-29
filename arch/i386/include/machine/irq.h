#ifndef __IRQ_H_
#define __IRQ_H_

#include <machine/system.h>
#include <machine/regs.h>

/*
 * Definitions for the interrupt request handling.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 06/15
 */

static inline void
disable_interrupts(void)
{
        __asm__ __volatile__("cli\n");
}

static inline void
enable_interrupts(void)
{
        __asm__ __volatile__("sti\n");
}

void (*irq_routines[16])(struct regs *r);

void irq_install_handler(int irq, void (*handler)(struct regs *r));
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

