#ifndef __CPU_H__
#define __CPU_H__

/*
 * machine/cpu.h
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 08/15
 */

#if CONFIG_ARCH == ARCH_TYPE_X86
#include <machine/cpu_i386.h>
#else
#include <machine/cpu_amd64.h>
#endif

enum cpuid_requests {
        CPUID_GETVENDORSTRING,
        CPUID_GETFEATURES,
        CPUID_GETTLB,
        CPUID_GETSERIAL,

        CPUID_INTELEXTENDED=0x80000000,
        CPUID_INTELFEATURES,
        CPUID_INTELBRANDSTRING,
        CPUID_INTELBRANDSTRINGMORE,
        CPUID_INTELBRANDSTRINGEND,
};

static inline void cpuid(int code, unsigned int *a, unsigned int *d) {
        __asm__ __volatile__("cpuid":"=a"(*a),"=d"(*d):"a"(code):"ecx","ebx");
}

static inline int cpuid_string(int code, unsigned int where[4]) {
        __asm__ __volatile__("cpuid":"=a"(*where),"=b"(*(where+1)),
                             "=c"(*(where+2)),"=d"(*(where+3)):"a"(code));
        return (int)where[0];
}

#endif

