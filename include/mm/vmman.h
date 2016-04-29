#ifndef _MM_VMMAN_H_
#define _MM_VMMAN_H_

/*
 * mm/vmman.h : VM Manager.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 04/16
 */

#include <mm/paging.h>

typedef struct {
        pgdir_t *pgd;
} vmman_t;

void vmman_init(vmman_t *, pgdir_t *);

extern vmman_t proc0_vmman;

#endif
