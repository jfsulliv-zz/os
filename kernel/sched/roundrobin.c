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

#include <mm/vma.h>
#include <sched/scheduler.h>
#include <sched/schedinfo.h>
#include <sched/sched_internal.h>
#include <sys/panic.h>
#include <sys/proc.h>
#include <sys/timer.h>
#include <util/list.h>

#define RR_MAX_TICKS 10

static void rr_start(proc_t *initproc);
static void rr_initproc(proc_t *initproc);
static void rr_add(proc_t *proc);
static void rr_rem(proc_t *proc);
static proc_t *rr_yield(void);
static proc_t *rr_tick(void);

scheduler_t round_robin_scheduler = {
        .sched_init_impl        = NULL,
        .sched_start_impl       = rr_start,
        .sched_initproc_impl    = rr_initproc,
        .sched_add_impl         = rr_add,
        .sched_rem_impl         = rr_rem,
        .sched_yield_impl       = rr_yield,
        .sched_tick_impl        = rr_tick,
        .name                   = "roundrobin"
};

static void
rr_start(proc_t *initproc)
{
        rr_initproc(initproc);
        initproc->control.schedinfo.nextp =
                initproc->control.schedinfo.prevp =
                initproc;
        rr_add(initproc);
}

static void
rr_initproc(proc_t *proc)
{
        proc->control.schedinfo.run_time = 0;
        proc->state.sched_state = PROC_STATE_READY;
        proc->state.sched_context = PROC_CONTEXT_KERN;
}

static void
rr_add(proc_t *proc)
{
        rr_initproc(proc);
        proc_t *current = proc_current();
        proc_t *prev = current->control.schedinfo.prevp;

        prev->control.schedinfo.nextp = proc;
        current->control.schedinfo.prevp = proc;

        proc->control.schedinfo.nextp = current;
        proc->control.schedinfo.prevp = prev;
}

static void
rr_rem(proc_t *proc)
{
        bug_on(proc->control.schedinfo.nextp == proc,
               "Descheduling last process");
        proc->control.schedinfo.nextp->control.schedinfo.prevp
                = proc->control.schedinfo.prevp;
        proc->control.schedinfo.prevp->control.schedinfo.nextp
                = proc->control.schedinfo.nextp;
}

static proc_t *
rr_yield(void)
{
        proc_t *current = proc_current();
        proc_t *next = current->control.schedinfo.nextp;
        proc_set_current(next);
        return current->id.pid != next->id.pid ? next : NULL;
}

static bool
rr_time_expired(proc_t *current)
{
        return current->resource.all_ticks > RR_MAX_TICKS;
}

static proc_t *
rr_tick(void)
{
        proc_t *current = proc_current();
        proc_t *next = NULL;

        if (rr_time_expired(current)) {
                if (current->state.sched_context == PROC_CONTEXT_USER) {
                        next = current->control.schedinfo.nextp;
                        if (next->id.pid == current->id.pid) {
                                next = NULL;
                        }
                }
        }
        return next;
}
