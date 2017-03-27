#ifndef _MACHINE_CPU_H_
#define _MACHINE_CPU_H_

/*
 * machine/cpu.h - CPU metadata and per-CPU variables
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 03/17
 */

#include <machine/arch_cpu.h>
#include <machine/regs.h>
#include <mm/paging.h>

// Per-processor kernel stack size
#define KSTACK_SIZE     (PAGE_SIZE << 2)

struct process;

typedef struct cpu {
        struct cpu *self;       /* Self-ref for accessing */
        struct process *proc;   /* Executing process */
        vaddr_t kstack;         /* Kernel stack base */
        arch_cpu_t arch_cpu;    /* MD cpu info */
        // XXX for compat reasons, add new fields below this line
        unsigned int id;        /* Unique identifier */
        unsigned char *_percpu; /* per-cpu data block */
} cpu_t __attribute__((aligned(CACHELINE_SZ)));

extern cpu_t cpu_primary;
extern char cpu_primary_kstack[KSTACK_SIZE];

// MI definitions

// Set up the primary CPU context.
void cpu_init_early();

// Sets up each of the other CPUs on the system. 'num_cpus' should
// include the primary CPU, but it will not be initialized.
void cpu_init(unsigned int num_cpus);
cpu_t *cpu_get(unsigned int id);
#define cpu_current() arch_cpu_current()

// MD definitions

// Initialize the given CPU.
void arch_cpu_setup(cpu_t *);
// A callback to start each CPU at. The argument will contain the
// CPU itself.
void arch_cpu_entry(cpu_t *);
cpu_t *arch_cpu_current();

// Per-CPU data
// XXX this would be better in its own header, but there's a cyclic
// dependency with the macros.

// Defines a static per-CPU variable of type 'type', called 'name'.
#define PERCPU_DEFINE(type, name) \
        __attribute__((section("data_percpu"))) \
                type __per_cpu_##name

// Retrieves the per-CPU value of 'var' for the indexed CPU.
#define PERCPU_GET_FOR_IND(var, cpuind) \
        (typeof(var) *)(cpu_get(cpuind)->_percpu + _PERCPU_OFFSETOF(var))
// Retrieves the per-CPU value of 'var' for the passed CPU struct.
#define PERCPU_GET_FOR_CPU(var, cpu) \
        (typeof(var) *)(cpu->_percpu + _PERCPU_OFFSETOF(var))
// Retrieves the per-CPU value of 'var' for the current CPU.
#define PERCPU_GET(var) PERCPU_GET_FOR_CPU(var, CPU_CURRENT())

// Internal helpers

// Boundaries of the of the initial percpu region. Used to compute
// offsets for each of the static percpu variables, as well as the
// overall size of the block.
extern char __attribute__((weak)) *__start_data_percpu;
GLOBAL(__start_data_percpu);
extern char __attribute__((weak)) *__stop_data_percpu;
GLOBAL(__stop_data_percpu);
#define PERCPU_START (&__start_data_percpu)
#define PERCPU_LIMIT (&__stop_data_percpu)
#define PERCPU_SZ ((uintptr_t)PERCPU_LIMIT - (uintptr_t)PERCPU_START)
#define PERCPU_OFFSETOF(name) \
        ((uintptr_t)(&__per_cpu##name) - PERCPU_START)


#endif
