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

#ifndef _SYS_TIMER_H_

/*
 * sys/timer.h - Time-related functions.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 11/16
 */

#include <stdbool.h>
#include <stdint.h>
#include <sys/time_constants.h>
#include <util/list.h>

/* Callback for a timer.
 * The return value is used to indicate when the timer should be
 * rescheduled. A return value of 0 indicates no rescheduling. */
typedef unsigned long (*timer_fn_t)(void);

/* A time slice, which contains all events whose expirations modulo
 * TIMER_NUM_SLICES falls into the same bucket. */
typedef struct time_slice {
        struct list_head timers; // An ordered list of timers.
} timeslice_t;

/* Internal implementation of a timer. Not forward declared so
 * statically allocated timers can be used, but the contained variables
 * should not be directly accessed. */
typedef struct timer {
        unsigned long time_us;
        struct list_head timer_list;
        timeslice_t *timeslice;
        timer_fn_t callback;
} timer_t;

/* === Machine-dependent (defined per-arch) === */

/* Microseconds since boot. */
unsigned long timer_get_usec(void);

/* Sets the timer's poll interval to a given microsecond value.
 * It is machine-dependent whether the given value is supported, and
 * the value passed to this may not be the exact interval (instead use
 * the value of timer_pollinterval()). */
void timer_set_pollinterval(unsigned long us);

/* Return the timer poll interval in microseconds. */
unsigned long timer_pollinterval(void);


/* === Machine-independent === */

/* Initializes the given timer. */
void timer_init(timer_t *timer, timer_fn_t callback);

/* Schedules the given timer to fire after us_until microseconds.
 * If the timer was already active, its old expiry is discarded. */
void timer_start(timer_t *timer, unsigned long us_until);

/* Cancels the given timer. */
void timer_cancel(timer_t *timer);

/* Returns whether the timer is pending or not. */
bool timer_pending(timer_t *timer);

/* Called when the timer's interrupt fires. Checks for any timers
 * registered, and calls them as needed. */
void timer_pollevents(void);

/* Spin for approx. us microseconds. */
void timer_wait(unsigned long us);

/* Spin for approx. ms milliseconds. */
void timer_wait_ms(unsigned long ms);

static inline unsigned long timer_get_msec()
{
        return timer_get_usec() / USEC_PER_MSEC;
}

static inline unsigned long timer_get_sec()
{
        return timer_get_usec() / USEC_PER_SEC;
}

#endif
