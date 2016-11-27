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

/* Shared implementation for schedulers. */

#include <sched/scheduler.h>
#include <sched/sched_internal.h>
#include <sys/kprintf.h>
#include <sys/timer.h>
#include <sys/panic.h>
#include <sys/sysinit.h>

#include <stdint.h>

#ifdef CONF_SCHED_ROUNDROBIN
#include <sched/roundrobin.h>
scheduler_t *scheduler = &round_robin_scheduler;
#else
#error No scheduler configured (CONF_SCHED_*)
#endif


/* The timer used to preempt processes. */
static timer_t sched_timer;

static const unsigned long DEFAULT_QUANTUM_MS = 100;

static void sched_set_quantum(unsigned long quantum);
static unsigned long sched_quantum(void);
static unsigned long sched_next_tick(void);

void sched_start(proc_t *initproc)
{
        bug_on(!initproc, "NULL initproc");
        scheduler->sched_start_impl(initproc);
        timer_start(&sched_timer, sched_next_tick());
}

void sched_fork(proc_t *parent, proc_t *child)
{
        bug_on(!parent || !child, "NULL process in fork");
        scheduler->sched_add_impl(child);
}

void sched_exit(proc_t *proc)
{
        bug_on(!proc, "NULL proc in exit");
        scheduler->sched_rem_impl(proc);
}

void sched_yield(proc_t *proc)
{
        bug_on(!proc, "NULL proc in yield");
        scheduler->sched_yield_impl(proc);
}

proc_t *sched_next(void)
{
        return scheduler->sched_nextproc_impl();
}

static unsigned long sched_tick(void)
{
        scheduler->sched_tick_impl();
        return sched_next_tick();
}

static unsigned long quantum;

static unsigned long sched_quantum(void)
{
        return quantum;
}

static void sched_set_quantum(unsigned long new)
{
        quantum = new;
}

static unsigned long sched_next_tick(void)
{
        return sched_quantum() * USEC_PER_MSEC;
}

static int sched_init(void)
{
        bug_on(!scheduler, "No scheduler configured.");
        int rval = 0;
        if (scheduler->sched_init_impl) {
                rval = scheduler->sched_init_impl();
        }
        sched_set_quantum(DEFAULT_QUANTUM_MS);
        timer_init(&sched_timer, sched_tick);
        kprintf(0, "sched: Initializing scheduler '%s' (quantum = %d)\n",
                scheduler->name, sched_quantum());
        return rval;
}
SYSINIT_STEP("scheduler", sched_init, SYSINIT_SCHED, 0);

