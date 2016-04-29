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
