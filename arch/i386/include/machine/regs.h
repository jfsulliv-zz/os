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

void dump_regs_from(struct regs *r);
void dump_regs(void);
void get_regs(struct regs *to);

#endif
