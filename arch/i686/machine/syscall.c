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

typedef int (*syscall_0_fn)(int *);
typedef int (*syscall_1_fn)(int *, uint64_t);
typedef int (*syscall_2_fn)(int *, uint64_t, uint64_t);
typedef int (*syscall_3_fn)(int *, uint64_t, uint64_t, uint64_t);
typedef int (*syscall_4_fn)(int *, uint64_t, uint64_t, uint64_t,
                            uint64_t);
typedef int (*syscall_5_fn)(int *, uint64_t, uint64_t, uint64_t,
                            uint64_t, uint64_t);
typedef int (*syscall_6_fn)(int *, uint64_t, uint64_t, uint64_t,
                            uint64_t, uint64_t, uint64_t);


static uint64_t
do_syscall(const sysent_t *sysent, uint64_t arg0, uint64_t arg1,
           uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5,
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
                        errno, arg0, arg1, arg2, arg3, arg4);
        case 6:
                return ((syscall_6_fn)sysent->fun)(
                        errno, arg0, arg1, arg2, arg3, arg4, arg5);
        default:
                panic("Invalid number of syscall args for syscall "
                      "%s (was %d)\n", sysent->name, sysent->num_args);
        }
}

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
__attribute__((noreturn)) void
syscall_child_exit(proc_t *child)
{
}

int
syscall_entry(void)
{
        struct regs *uregs = &cpu_current()->proc->state.uregs;

        int retval = -1;
        int errno = ENOSYS;
        if (uregs->rax < SYS_MAXNR) {
                const sysent_t *sysent = &syscalls[uregs->rax];
                retval = do_syscall(sysent, uregs->rdi, uregs->rsi,
                                    uregs->rdx, uregs->r10, uregs->r8,
                                    uregs->r9, &errno);
        }
        return retval;
}
