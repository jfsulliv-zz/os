#include <machine/idt.h>
#include <machine/irq.h>
#include <sys/kprintf.h>
#include <machine/pic.h>
#include <machine/regs.h>

void isr_handler(struct regs *r)
{
        if (r->int_no < INT_EXCEPTION_LIMIT) {
                kprintf(0, "%s Exception. System Halted!\n",
                        exception_messages[r->int_no]);
                dump_regs_from(r);
                for (;;);
        } else {
                void (*handler)(struct regs *r);
                int irq = r->int_no - INT_IRQ_BASE;

                handler = irq_routines[irq];
                if (handler) {
                        handler(r);
                }
                pic_send_eoi(irq);
        }
}
