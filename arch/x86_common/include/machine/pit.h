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

#ifndef __MACHINE_TIMER_H__
#define __MACHINE_TIMER_H__

/*
 * machine/pit.h
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 07/15
 */

#include <stdint.h>

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

#define PIT_MIN_HZ      18
#define PIT_MAX_HZ      1193182

void pit_install(void);

#endif
