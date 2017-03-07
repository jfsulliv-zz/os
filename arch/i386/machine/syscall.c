#include <stdint.h>
#include <sys/base.h>
#include <sys/errno.h>
#include <sys/panic.h>
#include <sys/syscall_constants.h>
#include <sys/syscalls.h>
#include <sys/timer.h>

typedef int (*syscall_0_fn)();
typedef int (*syscall_1_fn)(uint32_t);
typedef int (*syscall_2_fn)(uint32_t, uint32_t);
typedef int (*syscall_3_fn)(uint32_t, uint32_t, uint32_t);
typedef int (*syscall_4_fn)(uint32_t, uint32_t, uint32_t, uint32_t);
typedef int (*syscall_5_fn)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef int (*syscall_6_fn)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                            uint32_t);

static uint32_t
do_syscall(const sysent_t *sysent, uint32_t args[], uint32_t *user_stack)
{
        uint32_t retval;
        switch(sysent->num_args) {
        case 0:
                retval = ((syscall_0_fn)sysent->fun)();
                break;
        case 1:
                retval = ((syscall_1_fn)sysent->fun)(args[0]);
                break;
        case 2:
                retval = ((syscall_2_fn)sysent->fun)(args[0], args[1]);
                break;
        case 3:
                retval = ((syscall_3_fn)sysent->fun)(args[0], args[1],
                                                     args[2]);
                break;
        case 4:
                retval = ((syscall_4_fn)sysent->fun)(args[0], args[1],
                                                     args[2], args[3]);
                break;
        case 5:
                retval = ((syscall_5_fn)sysent->fun)(args[0], args[1],
                                                     args[2], args[3],
                                                     args[4]);
                break;
        case 6:
                retval = ((syscall_6_fn)sysent->fun)(args[0], args[1],
                                                     args[2], args[3],
                                                     args[4],
                                                     // arg5
                                                     user_stack[4]);
                break;
        default:
                panic("Invalid number of syscall args for syscall "
                      "%s (was %d)\n", sysent->name, sysent->num_args);
        }
        return retval;
}

static int32_t
syscall_entry(uint32_t *user_stack, uint32_t syscall_num,
              uint32_t args[]);

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
                "btl $31, %%eax\n" // negative?
                "jb _badsyscall\n"
                "cmpl %0, %%eax\n" // exceeds SYS_MAXNR?
                "jge _badsyscall\n"
                "pushl %%esi\n"
                "pushl %%edi\n"
                "pushl %%edx\n"
                "pushl %%ecx\n"
                "pushl %%ebx\n"
                "pushl %%eax\n"
                "pushl %%ebp\n"
                "call %P1\n"
                "popl %%ebp\n"
                "addl $24, %%esp\n"
                "movl 4(%%ebp), %%edx\n"
                "movl 8(%%ebp), %%ecx\n"
                "_exit:\n"
                "sysexit\n"
                "_badsyscall:\n"
                "mov %2, %%eax\n"
                "jmp _exit\n"
                :
                : "i" (SYS_MAXNR), "i" (syscall_entry), "i" (ENOSYS)
                :);
}

static int32_t
syscall_entry(uint32_t *user_stack, uint32_t syscall_num,
              uint32_t args[])
{
        uint32_t retval = ENOSYS;
        if (syscall_num < SYS_MAXNR) {
                const sysent_t *sysent = &syscalls[syscall_num];
                retval = do_syscall(sysent, args, user_stack);
        }
        return retval;
}
