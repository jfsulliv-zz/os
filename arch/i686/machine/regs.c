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
        kprintf(0,"GS=0x%02x FS=0x%02x ES=0x%02x DS=0x%02x CS=0x%02x\n",
                   r->gs, r->fs, r->es, r->ds, r->cs);
        kprintf(0,"RDI = 0x%016x    RSI = 0x%016x\n",
                   r->rdi, r->rsi);
        kprintf(0,"RBP = 0x%016x    RSP = 0x%016x\n",
                   r->rbp, r->rsp);
        kprintf(0,"RBX = 0x%016x    RDX = 0x%016x\n",
                   r->rbx, r->rdx);
        kprintf(0,"RCX = 0x%016x    RAX = 0x%016x\n",
                   r->rcx, r->rax);
        kprintf(0,"INT = 0x%016x    ERR = 0x%016x\n",
                   r->int_no, r->err_code);
        kprintf(0,"RIP = 0x%016x    FLG = 0x%016x\n",
                   r->rip, r->flags);
}

void
dump_regs(void)
{
        struct regs r = { 0 };
        get_regs(&r);
        dump_regs_from(&r);
}

void
get_regs(struct regs *to)
{
        uint64_t eip = (uint64_t)__builtin_return_address(1);
        __asm__ __volatile__(
                "push $0\n"
                "push %%rsp\n"
                "pushf\n"
                "push $0\n"
                "push %2\n"
                "push $0\n"
                "push $0\n"
                "push %%gs\n"
                "push %%fs\n"
                "push $0\n"
                "push $0\n"
                "push %%r15\n"
                "push %%r14\n"
                "push %%r13\n"
                "push %%r12\n"
                "push %%r11\n"
                "push %%r10\n"
                "push %%rbp\n"
                "push %%rbx\n"
                "push %%rax\n"
                "push %%r9\n"
                "push %%r8\n"
                "push %%rcx\n"
                "push %%rdx\n"
                "push %%rsi\n"
                "push %%rdi\n"
                "push $0\n"
                "push $0\n"
                "mov %1, %%rcx\n"
                "mov %%rsp, %%rsi\n"
                "mov %0, %%rdi\n"
                "rep movsb\n"
                "add %1, %%rsp\n"
                : "+r" (to)
                : "r" ((sizeof(struct regs))),
                  "r" (eip)
                : "ecx", "esi", "edi");
}

void
backtrace(unsigned int max_frames)
{
        uint64_t *ebp = __builtin_frame_address(0);
        uint64_t eip;
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
                ebp = (uint64_t *)(ebp[0]);
        }
}
