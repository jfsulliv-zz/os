/*
Copyright (c) 2016, James Sullivan <sullivan.james.f@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote
      products derived from this software without specific prior
      written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER>
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <multiboot.h>
#include <machine/arch_init.h>
#include <machine/irq.h>
#include <machine/regs.h>
#include <machine/tty.h>
#include <mm/paging.h>
#include <mm/pfa.h>
#include <mm/pmm.h>
#include <mm/vma.h>
#include <sched/scheduler.h>
#include <sys/config.h>
#include <sys/debug.h>
#include <sys/ksyms.h>
#include <sys/kprintf.h>
#include <sys/panic.h>
#include <sys/proc.h>
#include <sys/stdio.h>
#include <sys/string.h>
#include <sys/sysinit.h>
#include <sys/timer.h>

char const *startup_banner =
"===============================\n"
"=                             =\n"
"=       James' cool OS        =\n"
"=                             =\n"
"=                             =\n"
"=                             =\n"
"=          Copyright (c) 2016 =\n"
"===============================\n";

char const *sysinit_ready_msg =
        "Arch-specific initialization complete. Starting sysinit...\n";

memlimits_t limits;

/* Provided by the linker. */
extern char sbss, ebss;

static void
zero_bss(void)
{
        size_t sz = &ebss - &sbss;
        bzero(&sbss, sz);
}

static int
print_sysinit_start(void)
{
        kprintf(0, sysinit_ready_msg);
        return 0;
}
SYSINIT_STEP("print_sysinit_start", print_sysinit_start, SYSINIT_EARLY, 0);

static int
print_banner(void)
{
        kprintf(0, startup_banner);
        return 0;
}
SYSINIT_STEP("print_banner", print_banner, SYSINIT_LATE, 0);

static void stub_init(void);

__attribute__((noreturn)) void
main(multiboot_info_t *mbd)
{
        disable_interrupts();

        /* First of all, explicitly zero the BSS since we have no idea
         * what state it was left in by the bootloader. */
        zero_bss();

        /* Set up the hardware as needed. */
        arch_init();

        /* Set up a basic output device. */
        install_tty();
        kprintf_set_output_device(tty_get_output_device());
        kprintf(0,"Hello, world!\n");

        /* Map out the kernel and user physical address space. */
        detect_memory_limits(&limits, mbd);
        limits_report(&limits);

        /* Set up the early PMM subsystem */
        pmm_init(&limits);

        /* Set up the page frame allocator */
        pfa_init(&limits);
        pfa_report(false);
        DO_TEST(pfa_test);

        /* This gives us a current process, which the VMA system needs
         * to use to identify which address space to use. This process
         * will later become the idle process (pid 0). */
        proc_system_early_init();

        /* Once we have the first process, we can initialize the CPU
         * control block with it */
        cpu_init_early();

        /* Set up the VMA */
        vma_init();
        DO_TEST(vma_test);

        /* Now we can use the VMA to get the full PMM subsystem going. */
        pmm_init_late();

        /* Okay, now we can get some symbols. */
        ksyms_init(mbd);
        kprintf(0, "Initialized kernel symbols\n");

        /* Set up the process tables, process allocation, and pid 1, the
         * init process. */
        proc_system_init();
        kprintf(0, "Initialized process tables, init process\n");

        /* Get the interrupt handlers and timers ready. */
        arch_init_irqs();
        /* 1ms timer granularity */
        timer_set_pollinterval(1 * USEC_PER_MSEC);
        /* 100ms scheduler granularity */
        sched_set_tickrate(100);

        /* Run the rest of the system setup. This should be the last
         * thing done before we enable interrupts and start the init
         * process. */
        sys_init();

        /* Load the init process with its first program. */
        // TODO actually load a program. For now we just stub it.
        stub_init();

        kprintf(0, "Init process map:\n");
        vmmap_describe(&init_procp->state.vmmap);

        enable_interrupts();
        kprintf(0, "Enabled interrupts\n");

        /* Start the scheduler. */
        sched_start(init_procp);
        // TODO should this take in the initial context (user/kern)
        // as a parameter?
        // Also set sched context appropriately
        jump_to_userspace(get_entrypoint(&init_procp->state.uregs),
                          get_stack(&init_procp->state.uregs));
        panic("Failed to drop to userspace");
        for (;;); // Shut the compiler up
}

// TODO remove this test code
#if WORD_SIZE == 32
static void
codefn(void)
{
        __asm__ __volatile__(
                "pushl %%ecx\n"
                "pushl %%edx\n"
                "pushl $0x40017\n"
                "pushl %%ebp\n"
                "mov %%esp, %%ebp\n"
                : : "a" (2), "b" (0)
        );
        __asm__ __volatile__(
                "sysenter\n"
                "jmp .\n"
        );
}
#else
static __attribute__((naked)) void
codefn(void)
{
        __asm__ __volatile__(
                "xor %%rdi, %%rdi\n"
                : : "a" (2)
        );
        __asm__ __volatile__(
                "syscall\n"
                "jmp .\n"
        );
}
#endif

static void
stub_init(void)
{
        const size_t code_addr = 0x40000;
        const size_t stack_addr = 0x7F000;
        const size_t size = 0x1000;

        vmobject_t *code_obj = vmobject_create_anon(size, PFLAGS_RX);
        page_t *code_page = pfa_alloc(M_USER);
        vmobject_t *stack_obj = vmobject_create_anon(size, PFLAGS_RW);
        page_t *stack_page = pfa_alloc(M_USER);
        panic_on(!code_obj || !stack_obj || !code_page || !stack_page,
                        "Failed to allocate initial regions");
        vmobject_add_page(code_obj, code_page);
        vmobject_add_page(stack_obj, stack_page);
        bug_on(vmmap_map_object_at(&init_procp->state.vmmap, code_obj,
                                   0, code_addr, size),
                        "Failed to map init code");
        bug_on(vmmap_map_object_at(&init_procp->state.vmmap, stack_obj,
                                   0, stack_addr, size),
                        "Failed to map init stack");
        // Handled on pagefault later (usually).
        bug_on(pmm_map(init_procp->control.pmm,
                       code_addr,
                       page_to_phys(code_page),
                       M_USER,
                       PFLAGS_RWX), "Failed to map code");
        bug_on(pmm_map(init_procp->control.pmm,
                       stack_addr,
                       page_to_phys(stack_page),
                       M_USER,
                       PFLAGS_RW), "Failed to map stack");
        char *instrs = (char *)(code_addr);
        memcpy(instrs, codefn, 32);

        set_entrypoint(&init_procp->state.uregs, code_addr);
        set_stack(&init_procp->state.uregs, stack_addr, size);
}
