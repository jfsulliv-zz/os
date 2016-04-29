#include <machine/regs.h>
#include <sys/kprintf.h>

void
dump_regs_from(struct regs *r)
{
        kprintf(0,"GS=0x%02x FS=0x%02x ES=0x%02x DS=0x%02x CS=0x%02x\n",
                   r->gs, r->fs, r->es, r->ds, r->cs);
        kprintf(0,"EDI = 0x%08x    ESI = 0x%08x\n",
                   r->edi, r->esi);
        kprintf(0,"EBP = 0x%08x    ESP = 0x%08x\n",
                   r->ebp, r->esp);
        kprintf(0,"EBX = 0x%08x    EDX = 0x%08x\n",
                   r->ebx, r->edx);
        kprintf(0,"ECX = 0x%08x    EAX = 0x%08x\n",
                   r->ecx, r->eax);
        kprintf(0,"INT = 0x%08x    ERR = 0x%08x\n",
                   r->int_no, r->err_code);
        kprintf(0,"EIP = 0x%08x    FLG = 0x%08x\n",
                   r->eip, r->eflags);
}

void
dump_regs(void)
{
        struct regs r;
        get_regs(&r);
        dump_regs_from(&r);
}

void
get_regs(struct regs *to)
{
        /* XXX GCC specific */
        uint32_t eip = (uint32_t)__builtin_return_address(1);
        __asm__ __volatile__(
                "push $0\n"
                "push $0\n"
                "pushf\n"
                "push %%cs\n"
                "push %2\n"
                "push $0\n"
                "push $0\n"
                "pusha\n"
                "push %%ds\n"
                "push %%es\n"
                "push %%fs\n"
                "push %%gs\n"
                "mov %1, %%ecx\n"
                "mov %%esp, %%esi\n"
                "mov %0, %%edi\n"
                "rep movsb\n"
                "add %1, %%esp\n"
                : "+r" (to)
                : "r" ((sizeof(struct regs))),
                  "r" (eip)
                : "ecx", "esi", "edi");
}
