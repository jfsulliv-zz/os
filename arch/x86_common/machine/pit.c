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

#include <machine/irq.h>
#include <machine/pit.h>
#include <sys/timer.h>

#include <stdint.h>

static volatile unsigned long timer_ticks = 0;

static unsigned long const DEFAULT_TICKS = 18;
static unsigned long ticks_per_msec = DEFAULT_TICKS;

uint64_t timer_get_usec(void)
{
        return timer_ticks / ticks_per_msec * 1000;
}

void pit_set_phase(int hz)
{
        int div = PIT_BASE / hz;
        unsigned char cmd = (PIT_COUNTER(0) | PIT_RWMODE(0x3) |
                             PIT_MODE(PIT_MODE_SQRWV) | PIT_BCD(0));
        ticks_per_msec = hz / 1000;
        outportb(PIT_COMMAND, cmd);
        outportb(PIT_DATA_CHAN0, div & 0xFF);
        outportb(PIT_DATA_CHAN0, div >> 8);
}

static void timer_handler(const struct irq_ctx *unused
                          __attribute__((unused)))
{
        ++timer_ticks;
        timer_pollevents();
}

void pit_install(void)
{
        irq_install_handler(0, timer_handler);
}
