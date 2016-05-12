#include <machine/regs.h>
#include <mm/paging.h>
#include <mm/pfa.h>
#include <sys/kprintf.h>
#include <sys/panic.h>

static inline bool
is_kernel_fault(unsigned long addr)
{
        return (addr >= KERN_OFFS);
}

static void
handle_fault(struct regs *r, unsigned long fault_addr)
{
        kprintf(0, "Page fault at 0x%08x\n", fault_addr);
        if (is_kernel_fault(fault_addr)) {
                pgtab_t *tab;
                panic("TODO");
        } else {
                panic("TODO - user faults");
        }
}

void
pagefault_handler(struct regs *r)
{
        uint32_t fault_addr = load_cr2();
        handle_fault(r, fault_addr);
}
