#include <mm/vma.h>
#include <sys/errno.h>
#include <sys/timer.h>
#include <util/list.h>

typedef struct {
        uint64_t time_us;
        struct list_head timer_list;
        void (*callback)(void);
} timer_t;

static LIST_HEAD(timers);

// TODO this is inefficient
// Bucketing approach instead? Sort the next few ms of events at a
// time.
static void
schedule_timer(timer_t *timer)
{
        timer_t *timerp;
        list_foreach_entry(&timers, timerp, timer_list)
        {
                if (timerp->time_us > timer->time_us) {
                        list_add_tail(&timerp->timer_list,
                                      &timer->timer_list);
                        return;
                }
        }
        list_add_tail(&timers, &timer->timer_list);
}

int timer_addevent(uint64_t us_until, void (*callback)(void))
{
        if (!callback || !us_until) {
                return EINVAL;
        }
        timer_t *timer = kmalloc(sizeof(timer_t), M_KERNEL);
        if (!timer) {
                return ENOMEM;
        }
        timer->callback = callback;
        timer->time_us = timer_get_usec() + us_until;
        list_head_init(&timer->timer_list);
        schedule_timer(timer);
        return 0;
}

void timer_pollevents(void)
{
        uint64_t time = timer_get_usec();
        timer_t *timerp, *savep;
        list_foreach_entry_safe(&timers, timerp, savep, timer_list)
        {
                if (timerp->time_us <= time) {
                        timerp->callback();
                        list_del(&timerp->timer_list);
                } else {
                        break;
                }
        }
}

/* Spin for us microseconds. */
void timer_wait(uint64_t us)
{
        uint64_t end_time = timer_get_usec() + us;
        while (timer_get_usec() < end_time) { }
}

void timer_wait_ms(uint64_t ms)
{
        uint64_t end_time = timer_get_msec() + ms;
        while (timer_get_msec() < end_time) { }
}
