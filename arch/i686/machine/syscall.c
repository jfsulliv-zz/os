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

#include <stdint.h>
#include <sys/errno.h>
#include <sys/panic.h>
#include <sys/syscall_constants.h>
#include <sys/syscalls.h>
#include <sys/timer.h>

typedef int (*syscall_0_fn)();
typedef int (*syscall_1_fn)(uint64_t);
typedef int (*syscall_2_fn)(uint64_t, uint64_t);
typedef int (*syscall_3_fn)(uint64_t, uint64_t, uint64_t);
typedef int (*syscall_4_fn)(uint64_t, uint64_t, uint64_t, uint64_t);
typedef int (*syscall_5_fn)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
typedef int (*syscall_6_fn)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                            uint64_t);

static uint64_t
do_syscall(const sysent_t *sysent, uint64_t arg0, uint64_t arg1,
           uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
        uint64_t retval;
        switch(sysent->num_args) {
                case 0:
                        retval = ((syscall_0_fn)sysent->fun)();
                        break;
                case 1:
                        retval = ((syscall_1_fn)sysent->fun)(arg0);
                        break;
                case 2:
                        retval = ((syscall_2_fn)sysent->fun)(arg0, arg1);
                        break;
                case 3:
                        retval = ((syscall_3_fn)sysent->fun)(arg0, arg1, arg2);
                        break;
                case 4:
                        retval = ((syscall_4_fn)sysent->fun)(arg0, arg1, arg2,
                                                             arg3);
                        break;
                case 5:
                        retval = ((syscall_5_fn)sysent->fun)(arg0, arg1, arg2,
                                                             arg3, arg4);
                        break;
                case 6:
                        retval = ((syscall_6_fn)sysent->fun)(arg0, arg1, arg2,
                                                             arg3, arg4, arg5);
                        break;
                default:
                        panic("Invalid number of syscall args for syscall "
                              "%s (was %d)\n", sysent->name, sysent->num_args);
        }
        return retval;
}

static void syscall_entry(void);

/* Starting point of long-mode syscalls. Responsible for putting the
 * kernel stack into RSP and transferring to the more robust
 * syscall_entry code.
 * Expected layout:
 *   rax = syscall #
 *   rcx = user RIP to return to
 *   r11 = user RFLAGS
 *   rdi = arg0
 *   rsi = arg1
 *   rdx = arg2
 *   r10 = arg3
 *   r8  = arg4
 *   r9  = arg5
 */
__attribute__((naked, noreturn)) void
syscall_entry_stub(void)
{
        __asm__ __volatile__(
                "swapgs\n" // Swap in the kernel stack
                "mov %%gs:0, %%rsp\n"
                "push %%rbp\n"
                "mov %%rsp, %%rbp\n"
                "push %%rcx\n"
                "mov %0, %%rcx\n"
                "callq *%%rcx\n"
                "pop %%rcx\n"
                "pop %%rbp\n"
                "swapgs\n" // Swap out the kernel stack
                "sysret\n"
                :
                : "i" (syscall_entry)
                :);
}

static void
syscall_entry(void)
{
        uint64_t user_rip, user_rflags;
        __asm__ __volatile__(
                "mov %%rcx, %0\n"
                "mov %%r11, %1\n"
                : "=rm" (user_rip), "=rm" (user_rflags)
                :
                :);
        uint64_t syscall_num, arg0, arg1, arg2, arg3, arg4, arg5;
        __asm__ __volatile__(
                "mov %%rax, %0\n"
                "mov %%rdi, %1\n"
                "mov %%rsi, %2\n"
                "mov %%rdx, %3\n"
                "mov %%r10, %4\n"
                "mov %%r8, %5\n"
                "mov %%r9, %6\n"
                : "=rm" (syscall_num),
                  "=rm" (arg0),
                  "=rm" (arg1),
                  "=rm" (arg2),
                  "=rm" (arg3),
                  "=rm" (arg4),
                  "=rm" (arg5)
                :
                :);

        uint64_t retval = ENOSYS;
        if (syscall_num < SYS_MAXNR) {
                const sysent_t *sysent = &syscalls[syscall_num];
                retval = do_syscall(sysent, arg0, arg1, arg2, arg3, arg4,
                                    arg5);
        }

        __asm__ __volatile__(
                "mov %1, %%r11\n"
                "mov %2, %%rax\n"
                "sysret\n"
                :
                : "rcx" (user_rip), "rm" (user_rflags), "rm" (retval)
                :);
}
