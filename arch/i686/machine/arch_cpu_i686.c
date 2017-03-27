#include <machine/cpu.h>
#include <machine/msr.h>
#include <machine/tss.h>

void arch_cpu_setup(cpu_t *cpu)
{
        init_msrs(cpu);
}

void arch_cpu_entry(cpu_t *cpu)
{
        arch_cpu_setup(cpu);
}

cpu_t *
arch_cpu_current(void)
{
        cpu_t *cpu;
        __asm__(
                "movq %%gs:0, %0\n"
                : "=r" (cpu)
                :
                :
        );
        return cpu;
}
