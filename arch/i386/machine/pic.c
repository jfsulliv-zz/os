#include <machine/system.h>
#include <machine/pic.h>
#include <sys/kprintf.h>

static long pic1_spurious_irqs = 0;
static long pic2_spurious_irqs = 0;

void pic_send_eoi(unsigned char irq)
{
        if (irq >= 8) {
                outportb(PIC2_COMMAND, PIC_EOI);
        }
        outportb(PIC1_COMMAND, PIC_EOI);
}

void pic_remap(int offset1, int offset2)
{
        unsigned char a1, a2;

        /* Save masks */
        a1 = inportb(PIC1_DATA);
        a2 = inportb(PIC2_DATA);

        /* Start the init sequence in cascade mode */
        outportb(PIC1_COMMAND, ICW1_INIT+ICW1_ICW4);
        io_wait();
        outportb(PIC2_COMMAND, ICW1_INIT+ICW1_ICW4);
        io_wait();
        /* ICW2: Master/Slave vector offsets */
        outportb(PIC1_DATA, offset1);
        io_wait();
        outportb(PIC2_DATA, offset2);
        io_wait();
        /* ICW3: Inform Master of slave at IRQ2 */
        outportb(PIC1_DATA, 4);
        io_wait();
        /* ICW3: Tell slave its cascade identity */
        outportb(PIC2_DATA, 2);
        io_wait();

        outportb(PIC1_DATA, ICW4_8086);
        io_wait();
        outportb(PIC2_DATA, ICW4_8086);
        io_wait();

        /* Restore saved masks */
        outportb(PIC1_DATA, a1);
        outportb(PIC2_DATA, a2);
}

void pic_mask_irq(unsigned char irqline)
{
        unsigned short port;
        unsigned char value;

        if (irqline < 8) {
                port = PIC1_DATA;
        } else {
                port = PIC2_DATA;
                irqline -= 8;
        }
        value = inportb(port) | (1 << irqline);
        outportb(port, value);
}

void pic_unmask_irq(unsigned char irqline)
{
        kprintf(0,"Unmasking irq 0x%x\n", irqline);
        unsigned short port;
        unsigned char value;

        if (irqline < 8) {
                port = PIC1_DATA;
        } else {
                port = PIC2_DATA;
                irqline -= 8;
        }
        value = inportb(port) & ~(1 << irqline);
        outportb(port, value);
}

void pic_mask_all(void)
{
        int i;
        for (i = 0 + PIC1_OFFSET; i < 16 + PIC1_OFFSET; i++) {
                pic_mask_irq(i);
        }
}

void pic_unmask_all(void)
{
        int i;
        for (i = 0 + PIC1_OFFSET; i < 16 + PIC1_OFFSET; i++) {
                pic_unmask_irq(i);
        }
}

unsigned short pic_get_mask(void)
{
        unsigned short ret;
        ret = (inportb(PIC2_DATA) << 8) | inportb(PIC1_DATA);
        return ret;
}

static unsigned short __pic_get_irq_reg(int ocw3)
{
        /* Write OCW3 to PIC_CMD to get the reg values.
         * PIC2 is chained and represents IRQ8-15.
         * PIC1 is IRQ0-7 with 2 being the chain. */
        outportb(PIC1_CMD, ocw3);
        outportb(PIC2_CMD, ocw3);
        return (inportb(PIC2_CMD) << 8) | inportb(PIC1_CMD);
}

/* Returns the combined value of the cascaded PICs irq request register */
unsigned short pic_get_irr(void)
{
        return __pic_get_irq_reg(PIC_READ_IRR);
}

/* Returns the combined value of the cascaded PICs in-service register */
unsigned short pic_get_isr(void)
{
        return __pic_get_irq_reg(PIC_READ_ISR);
}

/* Takes a raw vector/trap number (32-47) */
int pic_check_spurious(int trap_nr)
{
        if (trap_nr == PIC1_SPURIOUS && !(pic_get_isr() & (1 << 7))) {
                ++ pic1_spurious_irqs;
                return 1;
        }
        if (trap_nr == PIC2_SPURIOUS && !(pic_get_isr() & (1 << 15))) {
                ++ pic2_spurious_irqs;
                pic_send_eoi(PIC1_OFFSET + 2);
                return 1;
        }
        return 0;
}

