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

#ifndef _MACHINE_ARCH_REGS_H_
#define _MACHINE_ARCH_REGS_H_

#include <machine/types.h>
#include <stdint.h>

/* Register context */
struct regs
{
        reg_t rdi, rsi, rdx, rcx;
        reg_t r8,  r9,  rax, rbx;
        reg_t rbp, r10, r11, r12;
        reg_t r13, r14, r15;
        reg_t ds, es, fs, gs;
        reg_t int_no, err_code;
        reg_t rip, cs, flags, rsp, ss;
};

#define REGS_RDI_OFFS   0x00
#define REGS_RSI_OFFS   0x08
#define REGS_RDX_OFFS   0x10
#define REGS_RCX_OFFS   0x18
#define REGS_R8_OFFS    0x20
#define REGS_R9_OFFS    0x28
#define REGS_RAX_OFFS   0x30
#define REGS_RBX_OFFS   0x38
#define REGS_RBP_OFFS   0x40
#define REGS_R10_OFFS   0x48
#define REGS_R11_OFFS   0x50
#define REGS_R12_OFFS   0x58
#define REGS_R13_OFFS   0x60
#define REGS_R14_OFFS   0x68
#define REGS_R15_OFFS   0x70
#define REGS_DS_OFFS    0x78
#define REGS_ES_OFFS    0x80
#define REGS_FS_OFFS    0x88
#define REGS_GS_OFFS    0x90
#define REGS_INT_OFFS   0x98
#define REGS_ERR_OFFS   0xA0
#define REGS_RIP_OFFS   0xA8
#define REGS_CS_OFFS    0xB0
#define REGS_FLG_OFFS   0xB8
#define REGS_RSP_OFFS   0xC0
#define REGS_SS_OFFS    0xC8
#define REGS_SIZE       0xD0

#define REGS_PUSH_ALL \
        "push %%rdi\n" \
        "push %%rsi\n" \
        "push %%rdx\n" \
        "push %%rcx\n" \
        "push %%r8\n" \
        "push %%rax\n" \
        "push %%rbx\n" \
        "push %%r10\n" \
        "push %%r11\n" \
        "push %%r12\n" \
        "push %%r13\n" \
        "push %%r14\n" \
        "push %%r15\n" \
        "push %%ds\n" \
        "push %%es\n" \
        "push %%fs\n" \
        "push %%cs\n" \
        "push %%ss\n" \

#define REGS_POP_ALL \
        "pop %%ss\n" \
        "pop %%cs\n" \
        "pop %%fs\n" \
        "pop %%es\n" \
        "pop %%ds\n" \
        "pop %%r15\n" \
        "pop %%r14\n" \
        "pop %%r13\n" \
        "pop %%r12\n" \
        "pop %%r11\n" \
        "pop %%r10\n" \
        "pop %%rbx\n" \
        "pop %%rax\n" \
        "pop %%r8\n" \
        "pop %%rcx\n" \
        "pop %%rdx\n" \
        "pop %%rsi\n" \
        "pop %%rdi\n" \

static inline reg_t
get_cr2(void)
{
        reg_t ret;
        __asm__ __volatile__(
                "mov %%cr2, %0"
                : "=a" (ret));
        return ret;
}

static inline void
set_cr3(reg_t val)
{
        __asm__ __volatile__(
                "mov %0, %%cr3"
                : : "r" (val));
}

#endif
