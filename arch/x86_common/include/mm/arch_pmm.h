#ifndef _MM_ARCH_PMM_H_
#define _MM_ARCH_PMM_H_

#include <machine/types.h>
#include <mm/arch_paging.h>

typedef struct {
        paddr_t pgdir_paddr;
        pgd_t  *pgdir;
        size_t  refct;
} pmm_t;

#endif
