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

#include <machine/gdt.h>
#include <stdint.h>
#include <sys/base.h>
#include <sys/errno.h>
#include <sys/panic.h>
#include <sys/syscall_constants.h>
#include <sys/syscalls.h>
#include <sys/timer.h>

typedef int (*syscall_0_fn)(int *);
typedef int (*syscall_1_fn)(int *, reg_t);
typedef int (*syscall_2_fn)(int *, reg_t, reg_t);
typedef int (*syscall_3_fn)(int *, reg_t, reg_t, reg_t);
typedef int (*syscall_4_fn)(int *, reg_t, reg_t, reg_t,
                            reg_t);
typedef int (*syscall_5_fn)(int *, reg_t, reg_t, reg_t,
                            reg_t, reg_t);
typedef int (*syscall_6_fn)(int *, reg_t, reg_t, reg_t,
                            reg_t, reg_t, reg_t);

// TODO wrap access to user_stack
static int
do_syscall(const sysent_t *sysent, reg_t arg0, reg_t arg1, reg_t arg2,
           reg_t arg3, reg_t arg4, const reg_t *user_stack,
           int *errno)
{
        switch(sysent->num_args) {
        case 0:
                return ((syscall_0_fn)sysent->fun)(
                        errno);
        case 1:
                return ((syscall_1_fn)sysent->fun)(
                        errno, arg0);
        case 2:
                return ((syscall_2_fn)sysent->fun)(
                        errno, arg0, arg1);
        case 3:
                return ((syscall_3_fn)sysent->fun)(
                        errno, arg0, arg1, arg2);
        case 4:
                return ((syscall_4_fn)sysent->fun)(
                        errno, arg0, arg1, arg2, arg3);
        case 5:
                return ((syscall_5_fn)sysent->fun)(
                        errno, arg0, arg1, arg2, arg3,
                        arg4);
        case 6:
                return ((syscall_6_fn)sysent->fun)(
                        errno, arg0, arg1, arg2, arg3,
                        arg4, user_stack[4]);
        default:
                panic("Invalid number of syscall args for syscall "
                      "%s (was %d)\n", sysent->name, sysent->num_args);
        }
}

static int32_t
syscall_entry(reg_t *user_stack, int syscall_num, reg_t eip,
              reg_t esp, reg_t ebx, reg_t ecx, reg_t edx,
              reg_t edi, reg_t esi, reg_t ds, reg_t es, reg_t fs,
              reg_t gs);

/* Starting point for the sysenter call.
 * The expected layout at this point as follows:
 *  esp = kernel stack (loaded from TSS)
 *  ebp = user stack
 *  eax = syscall #
 *  ebx = arg0
 *  ecx = arg1
 *  edx = arg2
 *  edi = arg3
 *  esi = arg4
 *  ebp+0 = user ebp 
 *  ebp+4 = user eip 
 *  ebp+8 = user edx
 *  ebp+c = user ecx
 *  ebp+10 = arg5
 *
 * Users are expected to call as so (arguments are optional):
 *
 *  mov ebx, arg0 // optional
 *  mov ecx, arg1 // optional
 *  mov edx, arg2 // optional
 *  mov edi, arg3 // optional
 *  mov esi, arg4 // optional
 *  push arg5  // optional
 *  push ecx
 *  push edx
 *  push _after_call
 *  push ebp
 *  mov eax, syscall_nr
 *  mov ebp, esp
 *  sysenter
 *  _after_call:
 *  pop ebp
 *  add esp, 4
 *  pop edx
 *  pop ecx
 *  add esp, 4 // if arg5 is used
 *
 * The return value is stored in eax before going back to userspace,
 * and the user EIP/ESP are read from edx/ecx respectively.
 */
__attribute__((naked, noreturn)) void
syscall_entry_stub(void)
{
       __asm__(
                "pushl %%gs\n"
                "pushl %%fs\n"
                "pushl %%es\n"
                "pushl %%ds\n"
                "pushl %%eax\n"
                "mov $0x10, %%eax\n"
                "mov %%ax, %%fs\n"
                "mov %%ax, %%ds\n"
                "mov %%ax, %%es\n"
                "mov %1, %%eax\n"
                "mov %%ax, %%gs\n"
                "popl %%eax\n"
                "pushl %%esi\n"
                "pushl %%edi\n"
                "pushl %%edx\n"
                "pushl %%ecx\n"
                "pushl %%ebx\n"
                "movl 4(%%ebp), %%ebx\n"
                "pushl %%ebx\n" // esp
                "movl 8(%%ebp), %%ebx\n"
                "pushl %%ebx\n" // eip
                "pushl %%eax\n" // syscall_num
                "pushl %%ebp\n" // user_stack
                "call %P2\n"
                "popl %%ebp\n"
                "addl $48, %%esp\n"
                "movl 4(%%ebp), %%edx\n"
                "movl 0(%%ebp), %%ecx\n"
                "sysexit\n"
                :
                : "i" (SYS_MAXNR), "i" (8 * GDT_PCPU_IND),
                  "i" (syscall_entry)
                :);
}

__attribute__((noreturn)) void
syscall_child_exit(proc_t *child)
{
        reg_t eip = child->state.uregs.eip;
        reg_t esp = child->state.uregs.esp;
        __asm__(
                "movl %0, %%edx\n"
                "movl %1, %%ecx\n"
                "sysexit\n"
                :
                : "r" (eip), "r" (esp)
                :);
        panic("sysexit failed");
}

static int32_t
syscall_entry(reg_t *user_stack, int syscall_num, reg_t eip,
              reg_t esp, reg_t ebx, reg_t ecx, reg_t edx,
              reg_t edi, reg_t esi, reg_t ds, reg_t es, reg_t fs,
              reg_t gs)
{
        // Record the current register state
        proc_t *me = proc_current();
        struct regs *uregs = &me->state.uregs;
        uregs->eax = syscall_num;
        uregs->ebx = ebx;
        uregs->ecx = ecx;
        uregs->edx = edx;
        uregs->edi = edi;
        uregs->esi = esi;
        uregs->eip = eip;
        uregs->esp = esp;
        uregs->ds = ds;
        uregs->es = es;
        uregs->fs = fs;
        uregs->gs = gs;

        int retval = -1;
        int errno = ENOSYS;
        if (syscall_num < SYS_MAXNR) {
                const sysent_t *sysent = &syscalls[syscall_num];
                retval = do_syscall(sysent, ebx, ecx, edx, edi, esi,
                                    user_stack, &errno);
        }
        return retval;
}
