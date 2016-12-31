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

#include <sched/scheduler.h>
#include <sys/timer.h>
#include <sys/sysinit.h>
#include <util/list.h>

#define TIMER_NUM_SLICES 256
static timeslice_t time_slices[TIMER_NUM_SLICES];

#define TIMER_MAX_TIMERS 256
static LIST_HEAD(free_timers);

static timeslice_t *timeslice_for_time(unsigned long time_us)
{
        unsigned int index =
                (time_us / timer_pollinterval()) % TIMER_NUM_SLICES;
        return &time_slices[index];
}

static timeslice_t *timeslice_for_timer(timer_t *timer)
{
        return timeslice_for_time(timer->time_us);
}

static void
schedule_timer(timer_t *timer, unsigned long us_until)
{
        timer_t *timerp;
        unsigned long when = us_until + timer_get_usec();
        timer->time_us = when;
        timeslice_t *slice = timeslice_for_timer(timer);
        timer->timeslice = slice;
        list_foreach_entry(&slice->timers, timerp, timer_list)
        {
                if (timerp->time_us > timer->time_us) {
                        list_add_tail(&timerp->timer_list,
                                      &timer->timer_list);
                        return;
                }
        }
        list_add_tail(&slice->timers, &timer->timer_list);
}

static void
deschedule_timer(timer_t *timer)
{
        timer->timeslice = NULL;
        list_del(&timer->timer_list);
}

void
timer_init(timer_t *timer, timer_fn_t callback)
{
        if (!timer || !callback) {
                return;
        }
        timer->callback = callback;
        list_head_init(&timer->timer_list);
}

void timer_start(timer_t *timer, unsigned long us_until)
{
        if (!timer) {
                return;
        }
        deschedule_timer(timer);
        schedule_timer(timer, us_until);
}

void timer_cancel(timer_t *timer)
{
        if (!timer) {
                return;
        }
        deschedule_timer(timer);
}

bool timer_pending(timer_t *timer)
{
        if (!timer) {
                return false;
        }
        return (timer->timeslice != NULL);
}

void timer_pollevents(void)
{
        unsigned long time = timer_get_usec();
        sched_tick();
        LIST_HEAD(finished_timers);
        timer_t *timerp, *savep;
        timeslice_t *slice = timeslice_for_time(time);
        list_foreach_entry_safe(&slice->timers, timerp, savep, timer_list)
        {
                if (timerp->time_us <= time) {
                        list_del(&timerp->timer_list);
                        list_add(&finished_timers, &timerp->timer_list);
                } else {
                        break;
                }
        }
        list_foreach_entry_safe(&finished_timers, timerp, savep, timer_list)
        {
                unsigned long next_time = timerp->callback();
                if (next_time) {
                        list_del(&timerp->timer_list);
                        schedule_timer(timerp, next_time);
                }
        }
}

/* Spin for us microseconds. */
void timer_wait(unsigned long us)
{
        unsigned long end_time = timer_get_usec() + us;
        while (timer_get_usec() < end_time) { }
}

void timer_wait_ms(unsigned long ms)
{
        unsigned long end_time = timer_get_msec() + ms;
        while (timer_get_msec() < end_time) { }
}

static int
timer_setup(void)
{
        unsigned int i = 0;
        for (; i < TIMER_NUM_SLICES; i++)
        {
                list_head_init(&time_slices[i].timers);
        }
        return 0;
}
SYSINIT_STEP("timer", timer_setup, SYSINIT_EARLY, 0);
