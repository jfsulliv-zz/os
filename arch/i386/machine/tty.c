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
unsigned short *last_linep;
unsigned short *scrl_linep;
unsigned short attrib = 0x0F;
unsigned short just_bg = 0x00;
int curs_x, curs_y;

output_device_t tty_output = {tty_puts};

void install_tty(void)
{
        textp = (unsigned short *)TUI_BUFFER;
        scrl_linep = (textp + (TUI_LINEWIDTH));
        last_linep = (textp + ((TUI_LINEMAX) * (TUI_LINEWIDTH)));
        tty_setcolor(COLOR_GREEN, COLOR_BLACK);
        curs_x = curs_y = 0;
}

void tty_setcolor(unsigned char fg, unsigned char bg)
{
        attrib = (bg << 4) | (fg & 0x0F);
        just_bg = (bg << 4) | (bg & 0x0F);
}

void tty_scroll(void)
{
        unsigned blank;
        static const unsigned int scroll_bytes
                = 2 * ((TUI_LINEMAX) * TUI_LINEWIDTH);

        /* Row 25 is the end, this means we need to scroll up */
        if(curs_y >= TUI_LINEMAX+1)
        {
                /* Give a blank the right color */
                blank = ' ' | (just_bg << 8);

                /* Move the current text chunk that makes up the screen
                 * back in the buffer by a line */
                memcpy (textp, scrl_linep, scroll_bytes);
                /* Set the last line to be blank */
                int i;
                for (i = 0; i < TUI_LINEWIDTH; i++)
                {
                        last_linep[i] = blank;
                }
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
