#ifndef _SYS_TIMER_H_

/*
 * sys/timer.h - Time-related functions.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 11/16
 */

#include <stdint.h>
#include <sys/time_constants.h>

/* Machine-dependent (defined per-arch) */

/* Microseconds since boot. */
uint64_t timer_get_usec(void);


/* Machine-independent */

/* Schedule callback to be called in approx. us_until microseconds. 
 * Returns 0 if successful and an error status otherwise. */
int timer_addevent(uint64_t us_until, void (*callback)(void));

/* Called when the timer's interrupt fires. Checks for any timers
 * registered, and calls them as needed. */
void timer_pollevents(void);

/* Spin for approx. us microseconds. */
void timer_wait(uint64_t us);

/* Spin for approx. ms milliseconds. */
void timer_wait_ms(uint64_t ms);

static inline uint64_t timer_get_msec()
{
        return timer_get_usec() / USEC_PER_MSEC;
}

static inline uint64_t timer_get_sec()
{
        return timer_get_usec() / USEC_PER_SEC;
}

#endif
