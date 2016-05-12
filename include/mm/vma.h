#ifndef _MM_VMA_H_
#define _MM_VMA_H_

#include <mm/flags.h>
#include <sys/config.h>

#ifdef CONF_VMA_SLAB
#include <mm/vma_slab.h>
#else
#error No allocator specified (CONF_VMA_*).
#endif

void
vma_init(void);

void *kmalloc (unsigned long size, mflags_t flags);
void  kfree   (void *addr);
void  krealloc(void *addr, unsigned long size, mflags_t flags);

void
vma_report(void);

void
vma_test(void);

#endif
