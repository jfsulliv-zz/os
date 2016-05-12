#include <multiboot.h>
#include <machine/arch_init.h>
#include <mm/init.h>
#include <mm/paging.h>
#include <mm/pfa.h>
#include <mm/vma.h>
#include <sys/config.h>
#include <sys/kprintf.h>

int
main(multiboot_info_t *mbd)
{
        /* Set up the hardware as needed. */
        arch_init(mbd);
        timer_install();
        /* Set up our initial page mapping */
        init_paging(&mem_limits);
        /* Set up the page frame allocator */
        pfa_init(&mem_limits);
        pfa_report(false);
#ifdef CONF_DEBUG
        pfa_test();
#endif
        /* Set up the VMA */
        vma_init();
#ifdef CONF_DEBUG
        vma_test();
#endif
        vma_report();

        for (;;);
}
