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
#include <sys/panic.h>
#include <sys/sysinit.h>
#include <sys/timer.h>

#include <stdint.h>

#ifdef CONF_SCHED_ROUNDROBIN
#include <sched/roundrobin.h>
scheduler_t *scheduler = &round_robin_scheduler;
#else
#error No scheduler configured (CONF_SCHED_*)
#endif

#define SCHED_TICKRATE_DEFAULT 1
static unsigned int tickrate = SCHED_TICKRATE_DEFAULT;

static void sched_accounting_tick(proc_t *);
static void sched_accounting_switchout(proc_t *);
static void sched_accounting_switchin(proc_t *);

void
sched_start(proc_t *initproc)
{
        bug_on(!initproc, "NULL initproc");
        scheduler->sched_start_impl(initproc);
        sched_accounting_switchin(initproc);
        scheduler->ready = true;
}

void
sched_tick(void)
{
        if (!scheduler->ready) {
                return;
        }
        static unsigned int tick = 0;
        proc_t *current = proc_current();
        sched_accounting_tick(current);
        if (tick++ > tickrate) {
                proc_t *next = scheduler->sched_tick_impl();
                if (next) {
                        sched_switch(next, current);
                }
                tick = 0;
        }
}

void
sched_set_tickrate(unsigned int rate)
{
        bug_on(rate == 0, "Cannot set sched tickrate to 0");
        kprintf(0, "Setting sched tickrate to %d\n", rate);
        tickrate = rate;
}

unsigned int
sched_tickrate(void)
{
        return tickrate;
}

void
sched_atfork(proc_t *parent, proc_t *child)
{
        bug_on(!parent || !child, "NULL process in fork");
        scheduler->sched_add_impl(child);
        // TODO should we yield here, or at the caller?
}

void __attribute__((noreturn))
sched_atexit(proc_t *proc)
{
        bug_on(!proc, "NULL proc in exit");
        proc_t *next = scheduler->sched_yield_impl();
        bug_on(!next, "Last process exiting.");
        scheduler->sched_rem_impl(proc);
        sched_switch(next, NULL);
        for (;;); /* Make the compiler happy */
}

void
sched_yield()
{
        proc_t *current = proc_current();
        proc_t *next = scheduler->sched_yield_impl();
        if (next) {
                sched_switch(next, current);
        }
}

void sched_at_kenter(proc_t *proc)
{
        bug_on(!proc, "NULL proc in kenter");
        proc->state.sched_context = PROC_CONTEXT_KERN;
}

void sched_at_kexit(proc_t *proc)
{
        bug_on(!proc, "NULL proc in kexit");
        proc->state.sched_context = PROC_CONTEXT_USER;
}

void
sched_newproc(proc_t *proc)
{
        bug_on(!proc, "NULL proc added to runqueue");
        scheduler->sched_add_impl(proc);
}

void
sched_switch(proc_t *newproc, proc_t *oldproc)
{
        bug_on(!newproc, "Switch to NULL process");
        if (oldproc) {
                sched_accounting_switchout(oldproc);
        }

        proc_set_current(newproc);

        /* Swap out the address spaces. */
        if (oldproc) {
                pmm_deactivate(oldproc->control.pmm);
        }
        pmm_activate(newproc->control.pmm);

        /* Perform an arch-dependent context switch. */
        context_switch(oldproc ? &oldproc->state.regs : NULL,
                       (const struct regs *)&newproc->state.regs);

        sched_accounting_switchin(newproc);
}

static void
sched_accounting_tick(proc_t *proc)
{
        switch (proc->state.sched_context)
        {
                case PROC_CONTEXT_USER:
                        proc->resource.u_ticks++;
                        break;
                case PROC_CONTEXT_KERN:
                        proc->resource.k_ticks++;
                        break;
                case PROC_CONTEXT_INTR:
                        proc->resource.i_ticks++;
                        break;
                default:
                        bug("Invalid sched context %d\n",
                            proc->state.sched_context);
        }
        proc->resource.all_ticks++;
}

static void
sched_accounting_switchout(proc_t *proc)
{
        unsigned long time = timer_get_usec();
        bug_on(proc->resource.timeslice_start_us > time,
               "Non-monotonic runtime (%d > %d)\n",
               proc->resource.timeslice_start_us, time);
        proc->resource.rtime_us +=
                time - proc->resource.timeslice_start_us;
}

static void
sched_accounting_switchin(proc_t *proc)
{
        proc->resource.timeslice_start_us = timer_get_usec();
}

static int
sched_init(void)
{
        bug_on(!scheduler, "No scheduler configured.");
        int rval = 0;
        if (scheduler->sched_init_impl) {
                rval = scheduler->sched_init_impl();
        }
        kprintf(0, "sched: Initializing scheduler '%s'\n",
                scheduler->name);
        return rval;
}
SYSINIT_STEP("scheduler", sched_init, SYSINIT_SCHED, 0);

