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

#ifndef _SCHED_SCHED_INTERNAL_H_
#define _SCHED_SCHED_INTERNAL_H_

#include <sys/proc.h>

/* Common structure for scheduler implementations.
 * Each implementation must define a scheduler_t which contains various
 * scheduling routines. The generic scheduler implementation will call
 * into the active scheduler's routines when a scheduling operation
 * should take place.
 */
typedef struct scheduler {
        /* Perform scheduler-specific setup necessary before use. */
        int  (*sched_init_impl)(void);
        /* Starts the scheduler by placing initproc into its run queue. */
        void (*sched_start_impl)(proc_t *initproc);
        /* Scheduler-specific initialization of the given PCB. */
        void (*sched_initproc_impl)(proc_t *initproc);
        /* Add the given process to the run queue. */
        void (*sched_add_impl)(proc_t *proc);
        /* Remove the given process from the run queue. */
        void (*sched_rem_impl)(proc_t *proc);
        /* Yields the slice for the given process. */
        void (*sched_yield_impl)(proc_t *proc);
        /* Returns the next process to schedule. */
        proc_t *(*sched_nextproc_impl)(void);
        /* Called when the scheduler timer expires, on a regular 
         * interval. */
        void (*sched_tick_impl)(void);
        const char name[32];
} scheduler_t;
extern scheduler_t *scheduler;


#endif
