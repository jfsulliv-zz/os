#include <mm/init.h>
#include <mm/paging.h>

extern uint32_t *page_directory;

static void
init_pagetables(void)
{
        unsigned long vaddr, end;
        pgd_t *pgd, pgd_base;
        int i;

}

void
init_paging(void)
{
        init_pagetables();
}
