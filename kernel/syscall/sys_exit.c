#include <sys/proc.h>
#include <sys/syscalls.h>

__attribute__((noreturn)) void
sys_exit(int status)
{
        proc_t *me = current_process();
        free_process(me);
        /* TODO yield(); */
}

