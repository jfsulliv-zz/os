#ifndef _MACHINE_PARAMS_H_
#define _MACHINE_PARAMS_H_

/*
 * machine/params.h
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 04/16
 */

#define PAGE_SZ 4096
#define STACK_SZ 16384

#define KERN_BASE 0xC0000000
#define KPAGE_DIR_INDEX (KERN_BASE >> 22)

#endif
