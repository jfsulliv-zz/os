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

#ifndef _MACHINE_ARCH_CPU_H_ 
#define _MACHINE_ARCH_CPU_H_

/*
 * machine/arch_cpu.h
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 08/15
 */

#include <machine/params.h>

#if WORD_SIZE == 32
#include <machine/arch_cpu_i386.h>
#else
#include <machine/arch_cpu_64.h>
#endif

enum cpuid_requests {
        CPUID_GETVENDORSTRING,
        CPUID_GETFEATURES,
        CPUID_GETTLB,
        CPUID_GETSERIAL,

        CPUID_INTELEXTENDED=0x80000000,
        CPUID_INTELFEATURES,
        CPUID_INTELBRANDSTRING,
        CPUID_INTELBRANDSTRINGMORE,
        CPUID_INTELBRANDSTRINGEND,
};

static inline void cpuid(int code, unsigned int *a, unsigned int *d) {
        __asm__ __volatile__("cpuid":"=a"(*a),"=d"(*d):"a"(code):"ecx","ebx");
}

static inline int cpuid_string(int code, unsigned int where[4]) {
        __asm__ __volatile__("cpuid":"=a"(*where),"=b"(*(where+1)),
                             "=c"(*(where+2)),"=d"(*(where+3)):"a"(code));
        return (int)where[0];
}

#endif
