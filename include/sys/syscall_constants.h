#ifndef _SYS_SYSCALL_CONSTANTS_H_
#define _SYS_SYSCALL_CONSTANTS_H_

#include <stddef.h>
#include <machine/regs.h>
#include <sys/proc.h>

#define SYS_MAXARGS 6

typedef struct {
        void *fun;
        size_t num_args;
        const char *const name;
} sysent_t;

extern const sysent_t syscalls[];

typedef struct {
        size_t num_args;
        reg_t args[SYS_MAXARGS];
} syscall_args_t;

/* Forward declaration of each system call */
__attribute__((noreturn)) void sys_exit(int status); /* 0 */
pid_t sys_fork(); /* 1 */

#endif
