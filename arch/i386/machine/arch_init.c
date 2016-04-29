#include <multiboot.h>

#include <machine/gdt.h>
#include <machine/idt.h>
#include <machine/irq.h>
#include <machine/tty.h>
#include <machine/timer.h>
#include <machine/pic.h>
#include <machine/regs.h>
#include <mm/init.h>
#include <mm/paging.h>
#include <sys/kprintf.h>
#include <sys/panic.h>

extern char kernel_end;
unsigned long highstart_pfn, highend_pfn;

memlimits_t mem_limits = { 0, 0, 0};

void
arch_init(multiboot_info_t *mbd)
{
        unsigned long start_pfn, max_pfn, max_low_pfn;

        /* Set up memory segmentation. */
        disable_interrupts();
        gdt_install();

        /* First up, set up a basic output device. */
        install_tty();
        kprintf_set_output_device(tty_get_output_device());
        kprintf(0,"Hello, world!\n");

        /* Register our interrupt handlers. */
        idt_install();
        irq_install();
        pic_remap(PIC1_OFFSET, PIC2_OFFSET);
        kprintf(0,"System tables installed.\n");

        timer_install();
        timer_phase(1000);

        enable_interrupts();
        kprintf(0,"Enabled interrupts.\n");

        /* Find out our memory limits. */
        if (!(mbd->flags & 1)) {
                return; // TODO manual memory map detection
        }
        start_pfn = PFN_UP(_pa(&kernel_end)); /* Next page after kernel */
        max_low_pfn = PFN_DOWN(1024 * mbd->mem_lower);
        max_pfn   = PFN_DOWN(1024 * mbd->mem_upper);
        mem_limits.low_pfn  = start_pfn;
        mem_limits.high_pfn = start_pfn + ((max_pfn - start_pfn) >> 2);
        mem_limits.max_pfn  = max_pfn;
}
