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

#ifndef _SCHED_SCHEDULER_H_
#define _SCHED_SCHEDULER_H_

/*
 * sched/scheduler.h - Task Scheduling API 
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 11/16
 */

#include <sys/config.h>
#include <sys/proc.h>

/* Scheduler control routines. */

/* Called to start scheduling init. */
void sched_start(proc_t *initproc);

/* Called every timer tick. */
void sched_tick(void);

/* Sets the number of timer ticks per scheduler tick. */
void sched_set_tickrate(unsigned int rate);

/* Returns the number of timer ticks per scheduler tick. */
unsigned int sched_tickrate(void);

/* Adds the given process to the runqueue. */
void sched_newproc(proc_t *);

/* Returns true if the process should be preempted and false otherwise. */
bool sched_shouldswitch(proc_t *proc);

/* Switch processes. Save the register state into 'oldproc' if it is
 * non-NULL. */
void sched_switch(proc_t *newproc, proc_t *oldproc);


/* Scheduler hooks for processes. */

/* Register child into the scheduler's run queue, ensuring that child
 * will run before parent. */
void sched_atfork(proc_t *parent, proc_t *child);

/* Remove the process from the run queue. */
void __attribute__((noreturn)) sched_atexit(proc_t *);

/* Yield the CPU for the current process. */
void sched_yield();

/* Call when the given process is about to enter kernel space.
 * Updates the scheduler's state accordingly. */
void sched_at_kenter(proc_t *);

/* Call when the given process is about to exit kernel space.
 * Updates the scheduler's state accordingly. */
void sched_at_kexit(proc_t *);

#endif
