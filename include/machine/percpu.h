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

#ifndef _MACHINE_PERCPU_H_
#define _MACHINE_PERCPU_H_

/*
 * machine/percpu.h - Per CPU variables.
 *
 * TODO - Actually implement this (currently stubbed)
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 12/16
 */

#include <machine/arch_percpu.h>
#include <sys/proc.h>

#include <stddef.h>

/* Static data owned by each CPU. */
typedef struct percpu_data {
        struct process *current;/* Current thread */
        void *_dynamic;         /* Dynamic per-CPU region */

        _ARCH_PERCPU_FIELDS;    /* MD fields. */
} percpu_t;
extern percpu_t percpu_structs[];

#define PERCPU_CURCPU (_ARCH_CURCPU)

#define PERCPU_STRUCT_FOR(cpuind) (&percpu_structs[cpuind])
#define PERCPU_STRUCT PERCPU_STRUCT_FOR(PERCPU_CURCPU)

#define PERCPU_CURPROC_FOR(cpuind) (PERCPU_STRUCT_FOR(cpuind)->current)
#define PERCPU_CURPROC PERCPU_CURPROC_FOR(PERCPU_CURCPU)

/* Dynamic data. */
extern uintptr_t dyn_percpu_offsets[];

#define PERCPU_DEFINE(type, name) \
        __attribute__((__section__(".data.percpu"))) \
                typeof(type) per_cpu_##name

#define PERCPU_GET_FOR(var, cpu) \
        (typeof(var) *)(((uintptr_t)&(per_cpu_##var)) + dyn_percpu_offsets[cpu])
#define PERCPU_GET(var) PERCPU_GET_FOR(var, PERCPU_CURCPU)

/* MD initialization routines */

void percpu_init(size_t num_cpus);
void percpu_init_cpu(percpu_t *percpu, int cpuid, size_t dyn_size);

#endif
