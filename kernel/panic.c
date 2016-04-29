#include <machine/regs.h>
#include <sys/kprintf.h>
#include <sys/panic.h>

void
_panic(const char *file, const char *fun, int line, const char *why)
{
        kprintf(0, "***PANIC***\n");
        kprintf(0, "%s: %s (%s:%d)\n", fun,why,file, line);
        dump_regs();
        for(;;);
}

void
_bug(const char *file, const char *fun, int line, const char *why)
{
        kprintf(0, "***BUG***\n");
        kprintf(0, "%s: %s (%s:%d)\n", fun,why,file, line);
        for(;;);
}
