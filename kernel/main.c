#include <multiboot.h>
#include <machine/arch_init.h>
#include <mm/paging.h>
#include <mm/pfa.h>
#include <sys/kprintf.h>
#include <sys/size.h>

int
main(multiboot_info_t *mbd)
{
        arch_init(mbd); /* Get the hardware ready */

        /* Set up our page frame allocator. */
        kprintf(0, "%dMiB available (%dMiB low, %dMiB high)\n",
                B_MiB(allmem_bytes_avail(&mem_limits)),
                B_MiB(lowmem_bytes_avail(&mem_limits)),
                B_MiB(highmem_bytes_avail(&mem_limits)));
        pfa_init(&mem_limits);

        pfa_alloc(0, 0);
        pfa_alloc(0, 1);
        pfa_alloc(0, 1);
        pfa_alloc(0, 2);
        pfa_alloc(0, 2);
        pfa_alloc(0, 3);

        for (;;);
}
