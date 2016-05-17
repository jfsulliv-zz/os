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

#ifndef _TUI_H_
#define _TUI_H_

/*
 * machine/tty.h
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 12/14
 */

#include <stddef.h>
#include <stdint.h>
#include <mm/paging.h>
#include <sys/output_device.h>

#define TUI_BUFFER_BASE 0xB8000
#define TUI_BUFFER (KERN_BASE + TUI_BUFFER_BASE)
#define TUI_TABWIDTH 8
#define TUI_LINEWIDTH 80
#define TUI_LINEMAX 24

/* Hardware text mode color constants. */
enum vga_color
{
        COLOR_BLACK = 0,
        COLOR_BLUE = 1,
        COLOR_GREEN = 2,
        COLOR_CYAN = 3,
        COLOR_RED = 4,
        COLOR_MAGENTA = 5,
        COLOR_BROWN = 6,
        COLOR_LIGHT_GREY = 7,
        COLOR_DARK_GREY = 8,
        COLOR_LIGHT_BLUE = 9,
        COLOR_LIGHT_GREEN = 10,
        COLOR_LIGHT_CYAN = 11,
        COLOR_LIGHT_RED = 12,
        COLOR_LIGHT_MAGENTA = 13,
        COLOR_LIGHT_BROWN = 14,
        COLOR_WHITE = 15,
};

void install_tty(void);
void tty_setcolor(unsigned char, unsigned char);
void tty_putchar(char);
void tty_puts(const char *);
output_device_t *tty_get_output_device(void);

extern output_device_t tty_output;

#endif /* _TUI_H_ */
