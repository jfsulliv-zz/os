#ifndef _MACHINE_REGS_H_
#define _MACHINE_REGS_H_

#include <stdint.h>

/* Register context */
struct regs
{
        unsigned int gs, fs, es, ds;
        unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
        unsigned int int_no, err_code;
        unsigned int eip, cs, eflags, useresp, ss;
};

static inline unsigned int
load_cr2(void)
{
        unsigned int ret;
        __asm__ __volatile__(
                "mov %%cr2, %0"
                : "=a" (ret));
        return ret;
}

void dump_regs_from(struct regs *r);
void dump_regs(void);
void get_regs(struct regs *to);
void backtrace(unsigned int max);

#endif
