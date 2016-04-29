#ifndef _MM_ARCH_PAGING_H_
#define _MM_ARCH_PAGING_H_

/*
 * mm/arch_paging.h
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 04/16
 */

#include <stdint.h>
#include <util/list.h>

#define KERN_BASE 0xC0000000
#define PAGE_SHIFT 12
#define PD_SHIFT   22
#define PTABS_PER_PD 1024
#define PAGES_PER_PT 1024
#define PAGE_SIZE 4096
#define PTAB_SIZE (1 << PAGE_SHIFT)
#define PDIR_SIZE (1 << PD_SHIFT)

#define PFN_UP(x)       (((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
#define PFN_DOWN(x)     ((x) >> PAGE_SHIFT)

#define _pa(x)  ((unsigned long)x-KERN_BASE)
#define _va(x)  ((unsigned long)x+KERN_BASE)

#define page_of(v)    ((void *)(_pa(x) >> PAGE_SHIFT))
#define paddr_of(pfn) ((unsigned long)((pfn) << PAGE_SHIFT))
#define vaddr_of(pfn) (_va(paddr_of(pfn)))

#define PT_INDEX(x) ((unsigned long)(x) >> PAGE_SHIFT & 0x03FF)
#define PD_INDEX(x) (((unsigned long)(x) >> PD_SHIFT) & (PTABS_PER_PD - 1))

typedef struct {
        uint32_t ent;
} pgent_t;

#endif
