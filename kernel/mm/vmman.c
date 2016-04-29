#include <mm/paging.h>
#include <mm/vmman.h>
#include <sys/panic.h>

static vmman_t proc0_vmman;

void
vmman_init(vmman_t *vmman, pgdir_t *pgd)
{
        if (!vmman || !pgd) {
                bug("NULL parameters");
        }
        vmman->pgd = pgd;
}
