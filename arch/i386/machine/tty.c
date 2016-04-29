/*
 * machine/tty.c
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 06/15
 */

#include <stdarg.h>
#include <sys/stdio.h>
#include <sys/string.h>
#include <machine/system.h>
#include <machine/tty.h>

unsigned short *textp;
unsigned short attrib = 0x0F;
int curs_x, curs_y;

output_device_t tty_output = {tty_puts};

void install_tty(void)
{
        textp = (unsigned short *)TUI_BUFFER;
        tty_setcolor(COLOR_GREEN, COLOR_BLACK);
        curs_x = curs_y = 0;
}

void tty_setcolor(unsigned char fg, unsigned char bg)
{
        attrib = (bg << 4) | (fg & 0x0F);
}

void tty_scroll(void)
{
        unsigned blank, temp;

        /* Give a blank the right color */
        blank = 0x20 | (attrib << 8);

        /* Row 25 is the end, this means we need to scroll up */
        if(curs_y >= TUI_LINEMAX+1)
        {
                /* Move the current text chunk that makes up the screen
                 * back in the buffer by a line */
                temp = curs_y - TUI_LINEMAX;
                memcpy (textp, textp + temp * TUI_LINEWIDTH,
                        (TUI_LINEMAX- temp) * TUI_LINEWIDTH * 2);
                /* Set the last line to be blank */
                memset (textp + (TUI_LINEMAX - temp) * TUI_LINEWIDTH,
                        blank, TUI_LINEWIDTH);
                curs_y = TUI_LINEMAX;
        }
}

void tty_move_curs(void)
{
        unsigned temp;

        temp = curs_y * TUI_LINEWIDTH + curs_x;

        outportb(0x3D4, 14);
        outportb(0x3D5, temp >> 8);
        outportb(0x3D4, 15);
        outportb(0x3D5, temp);
}


void tty_putchar(char c)
{
        volatile unsigned short *where;

        /* Backspace */
        if (c == 0x08) {
                if (curs_x != 0) curs_x--;
        }
        /* Tab */
        else if (c == 0x09) {
                curs_x = (curs_x + 8) & ~(TUI_TABWIDTH - 1);
        }
        /* CR */
        else if (c == '\r') {
                curs_x = 0;
        }
        /* Newline */
        else if (c == '\n') {
                curs_x = 0;
                curs_y++;
        }
        /* Printable characters */
        else if (c >= ' ') {
                where = textp + (curs_y * TUI_LINEWIDTH + curs_x);
                *where = c | (attrib << 8);
                curs_x++;
        }

        if (curs_x > TUI_LINEWIDTH) {
                curs_x = 0;
                curs_y++;
        }

        tty_scroll();
        tty_move_curs();
}

void tty_puts(const char *s)
{
        while (*s)
        {
                tty_putchar(*s++);
        }
}

output_device_t *
tty_get_output_device(void)
{
        return &tty_output;
}
