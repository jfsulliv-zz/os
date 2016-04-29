#ifndef __PIC_H__
#define __PIC_H__

/*
 * machine/pic.h
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 07/15
 */

#include <machine/irq.h>

#define PIC1_OFFSET     0x20
#define PIC2_OFFSET     0x28

#define PIC1            0x20            /* IO Base for master PIC */
#define PIC2            0xA0            /* IO Base for slave PIC */
#define PIC1_COMMAND    PIC1
#define PIC1_DATA       (PIC1 + 1)
#define PIC2_COMMAND    PIC2
#define PIC2_DATA       (PIC2 + 1)

#define PIC_EOI         0x20            /* End-of-interrupt command code */

void pic_send_eoi(unsigned char irq);

/* Reinitialize the PIC controllers with specified vec offsets */

#define ICW1_ICW4       0x01            /* ICW4 (not) needed */
#define ICW1_SINGLE     0x02            /* Single (cascade) mode */
#define ICW1_INTERVAL4  0x04            /* Call address interval 4 (8) */
#define ICW1_LEVEL      0x08            /* Level triggered (edge) mode */
#define ICW1_INIT       0x10            /* Initialization - required! */

#define ICW4_8086       0x01            /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02            /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08            /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C            /* Buffered mode/master */
#define ICW4_SFNM       0x10            /* Special fully nested (not) */

void pic_remap(int offset1, int offset2);

void pic_mask_irq(unsigned char irqline);
void pic_unmask_irq(unsigned char irqline);
void pic_mask_all(void);
unsigned short pic_get_mask(void);

#define PIC1_CMD        PIC1
#define PIC1_DATA       (PIC1 + 1)
#define PIC2_CMD        PIC2
#define PIC2_DATA       (PIC2 + 1)
#define PIC_READ_IRR    0x0a            /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR    0x0b            /* OCW3 irq service next CMD read */

unsigned short pic_get_irr(void);
unsigned short pic_get_isr(void);

#define PIC1_SPURIOUS                    PIC1_OFFSET + 7
#define PIC2_SPURIOUS                    PIC2_OFFSET + 7

int pic_check_spurious(int trap_nr);

static inline void pic_disable(void)
{
        __asm__ __volatile__ ("mov al, 0xff\n"
                              "out PIC2_OFFSET, al\n"
                              "out PIC1_OFFSET, al\n");
}

#endif /* __PIC_H__ */

