#ifndef __MACHINE_TIMER_H__
#define __MACHINE_TIMER_H__

/*
 * machine/timer.h
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 07/15
 */

#define PIT_DATA_CHAN0  0x40
#define PIT_DATA_CHAN1  0x41
#define PIT_DATA_CHAN2  0x42
#define PIT_COMMAND     0x43

#define PIT_COUNTER(x)  ((x & 0x3) << 0x6)
#define PIT_RWMODE(x)   ((x & 0x3) << 0x4)
#define PIT_MODE(x)     ((x & 0x7) << 0x1)
#define PIT_BCD(x)      (x & 0x1)

#define PIT_MODE_ITERM  0x0
#define PIT_MODE_HWRET  0x1
#define PIT_MODE_RTGEN  0x2
#define PIT_MODE_SQRWV  0x3
#define PIT_MODE_SSTRB  0x4
#define PIT_MODE_HSTRB  0x5

#define PIT_BASE        1193180

void timer_wait(int ms);
long timer_get_ticks(void);
long timer_get_seconds(void);
void timer_phase(int hz);
void timer_install(void);

#endif
