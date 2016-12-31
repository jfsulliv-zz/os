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

#include <machine/regs.h>
#include <sys/kprintf.h>
#include <sys/ksyms.h>

void
dump_regs_from(const struct regs *r)
{
        kprintf(0,"GS=0x%02x FS=0x%02x ES=0x%02x DS=0x%02x CS=0x%02x "
                  "SS=0x%02x\n",
                   r->gs, r->fs, r->es, r->ds, r->cs, r->ss);
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
        uint32_t eip = (uint32_t)__builtin_return_address(1);
        __asm__ __volatile__(
                "push %%ss\n"
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

void
backtrace(unsigned int max_frames)
{
        unsigned int *ebp = __builtin_frame_address(0);
        unsigned int eip;
        unsigned int fr;
        for (fr = 0; fr < max_frames; fr++)
        {
                if (!ebp)
                        eip = 0;
                else
                        eip = ebp[1];
                kprintf(0, "%s\n", ksyms_report_eip(eip));
                if (!eip)
                        break;
                ebp = (unsigned int *)(ebp[0]);
        }
}

void
set_stack(struct regs *regs, reg_t stack, unsigned long stack_size)
{
        regs->esp = stack + stack_size - 4;
        regs->ebp = 0;
}

void
set_entrypoint(struct regs *regs, reg_t entry)
{
        regs->eip = entry;
}
