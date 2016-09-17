#ifndef _MACHINE_MSR_H_
#define _MACHINE_MSR_H_

#include <machine/regs.h>
#include <stdint.h>

static inline void
get_msr(uint32_t msr, uint32_t *low, uint32_t *high)
{
        __asm__ __volatile__(
                "rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
}

static inline void
set_msr(uint32_t msr, uint32_t low, uint32_t high)
{
        __asm__ __volatile__(
                "wrmsr" : : "a" (low), "d" (high), "c" (msr));
}

static inline void
wrmsrl(uint32_t msr, uint64_t val)
{
        __asm__ __volatile__(
                "wrmsr" : : "c" (msr), "d" ((uint32_t)val),
                            "a" ((uint32_t)(val >> 32)));
}

#define IA32_SYSENTER_CS        0x174
#define IA32_SYSENTER_ESP       0x175
#define IA32_SYSENTER_EIP       0x176

#endif
