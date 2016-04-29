#include <multiboot.h>
#include <machine/arch_init.h>
#include <mm/paging.h>
#include <mm/pfa.h>
#include <mm/init.h>
#include <sys/config.h>
#include <sys/kprintf.h>

int
main(multiboot_info_t *mbd)
{
        /* Set up the hardware as needed. */
        arch_init(mbd);
        /* Set up our initial page mapping */
        init_paging(&mem_limits);
        /* Set up the page frame allocator */
        pfa_init(&mem_limits);
        pfa_report(false);

#ifdef CONF_DEBUG
        pfa_test();
#endif

        for (;;);
}
