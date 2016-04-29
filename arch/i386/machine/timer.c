#include <machine/irq.h>
#include <machine/timer.h>
#include <sys/kprintf.h>

static volatile long timer_ticks = 0;
static volatile long seconds = 0;
static int sec_rate = 18; /* Default msec_rate */

void timer_wait(int ms)
{
        int end = (ms / 1000) + seconds;
        while (seconds < end);
}

void timer_wait_ticks(int n)
{
        int end = n + timer_ticks;
        while (timer_ticks < end);
}

void timer_phase(int hz)
{
        int div = PIT_BASE / hz;
        unsigned char cmd = (PIT_COUNTER(0) | PIT_RWMODE(0x3) |
                             PIT_MODE(PIT_MODE_SQRWV) | PIT_BCD(0));
        sec_rate = hz;
        outportb(PIT_COMMAND, cmd);
        outportb(PIT_DATA_CHAN0, div & 0xFF);
        outportb(PIT_DATA_CHAN0, div >> 8);
        kprintf(0,"Set timer phase to %d\n", hz);
}

void timer_handler(struct regs *unused __attribute__((unused)))
{
        ++timer_ticks;

        if (timer_ticks % sec_rate == 0)
        {
                ++seconds;
        }
}

void timer_install(void)
{
        irq_install_handler(0, timer_handler);
}

