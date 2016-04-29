#ifndef _ASM_BITOPS_H_
#define _ASM_BITOPS_H_

static inline void
_set_bit(volatile void *addr, unsigned long nr)
{
    __asm__ __volatile__("bts %1,%0" : "+m" (*(unsigned long *)addr) : "Ir" (nr) : "memory");
}

static inline void
_clr_bit(volatile void *addr, unsigned long nr)
{
    __asm__ __volatile__("btr %1,%0" : "+m" (*(unsigned long *)addr) : "Ir" (nr) : "memory");
}

static inline int
_tst_bit(volatile void *addr, unsigned long nr)
{
        int val;
        __asm__ __volatile__("bt %2,%1\n"
                             "sbb %0,%0"
                             : "=r" (val)
                             : "m" (*(unsigned long *)addr), "Ir" (nr));
        return val;
}

#endif
