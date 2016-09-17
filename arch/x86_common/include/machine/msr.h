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

#ifndef _MACHINE_MSR_H_
#define _MACHINE_MSR_H_

#include <machine/regs.h>
#include <stdint.h>

static inline void
get_msr(uint32_t msr, uint32_t *low, uint32_t *high)
{
        __asm__ __volatile__(
                "rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
}

static inline void
set_msr(uint32_t msr, uint32_t low, uint32_t high)
{
        __asm__ __volatile__(
                "wrmsr" : : "a" (low), "d" (high), "c" (msr));
}

static inline void
wrmsrl(uint32_t msr, uint64_t val)
{
        __asm__ __volatile__(
                "wrmsr" : : "c" (msr), "d" ((uint32_t)val),
                            "a" ((uint32_t)(val >> 32)));
}

#define IA32_SYSENTER_CS        0x174
#define IA32_SYSENTER_ESP       0x175
#define IA32_SYSENTER_EIP       0x176

#endif
