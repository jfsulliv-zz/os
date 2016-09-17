#include <sys/proc.h>
#include <sys/syscalls.h>

pid_t
sys_fork(void)
{
        proc_t *me = current_process();
        proc_t *child = copy_process(me, NULL);
        if (!child)
                return -1;
        return child->id.pid;
}
