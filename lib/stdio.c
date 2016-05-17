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

/*
 * stdio.c
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 06/15
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/stdio.h>
#include <sys/string.h>
#include <util/cmp.h>

#define INT_DIGITS 19 /* Room for a 64 bit base-10 integer */

int itoa(int value, char *sp, bool zero_pad, int min_len, int radix)
{
	char tmp[INT_DIGITS + 2];
	char *tp = tmp;
	int i;
	unsigned v;

	int sign = (radix == 10 && value < 0);
	if (sign)
		v = -value;
	else
		v = (unsigned)value;

	while (v || tp == tmp)
	{
		i = v % radix;
		v /= radix;
		if (i < 10)
			*tp++ = i+'0';
		else
			*tp++ = i + 'A' - 10;
	}

        int len = tp - tmp;

        if (sign && !zero_pad) {
                *sp++ = '-';
                len++;
        }

        while ((tp - tmp) < min_len) {
                *tp++ = (zero_pad ? '0' : ' ');
                len++;
        }

	if (sign && zero_pad)
	{
		*sp++ = '-';
		len++;
	}

	if (len > INT_DIGITS + 1) {
		return -1;
	}

	while (tp > tmp)
		*sp++ = *--tp;

	if (len + 1 < INT_DIGITS) {
		sp[len] = 0;
	}
	return len;
}

int atoi(const char *str)
{
        int res = 0;
        while (isdigit(*str))
        {
                res *= 10;
                res += (ord(*str++) - ord('0'));
        }
        return res;
}

int vsnprintf(char *str, size_t size, const char *format, va_list args)
{
	size_t i = 0, n = 0;
	int expanded_len = 0;
	char tmp[256];
	while (i < size && format[n])
	{
		if (format[n] == '%') {
			int iarg;
			char *sarg;
                        bool zero_pad = false;
                        int min_len = 0;
                        int len = 0;
                        int pad = 0;
                        char c = format[++n];
                        while (isdigit(c))
                        {
                                if (c == '0') {
                                        zero_pad = true;
                                        n++;
                                } else if (isdigit(c)) {
                                        min_len = atoi(&format[n]);
                                        while (isdigit(format[n]))
                                                n++;
                                }
                                c = format[n];
                        }
			switch (format[n])
			{
				case 'd':
					iarg = va_arg(args, int);
					expanded_len = itoa(iarg, tmp,
                                                        zero_pad, min_len,
                                                        10);
					break;
				case 'x':
					iarg = va_arg(args, int);
					expanded_len = itoa(iarg, tmp,
                                                        zero_pad, min_len,
                                                        16);
					break;
				case 's':
					sarg = va_arg(args, char *);
                                        len = strlen(sarg);
                                        expanded_len = (min_len == 0
                                                ? len
                                                : min_len);
                                        pad = min_len - len;
                                        if (pad > 0 && pad < 255)
                                                memset(tmp, ' ', pad);
                                        else
                                                pad = 0;
					memcpy(tmp+pad, sarg,
                                                len > 255-pad
                                                   ? 255-pad
                                                   : len);
					break;
				default:
					return -1;
			}
			if (expanded_len < 0 || i + expanded_len >= size) {
				return -1;
			} else {
				memcpy(&str[i], tmp, expanded_len);
				i += expanded_len;
			}
		} else {
			str[i++] = format[n];
		}
		++n;
	}
	if (i < size) {
		str[i] = 0;
	}
	return i;
}

int vslprintf(char *str, size_t size, const char *format, va_list args)
{
        int len = vsnprintf(str, size-1, format, args);
        str[size-1] = '\0';
        return len;
}

int snprintf(char *str, size_t size, const char *format, ...)
{
	va_list args;
	va_start (args, format);
	return vsnprintf(str, size, format, args);
}

int slprintf(char *str, size_t size, const char *format, ...)
{
        va_list args;
        va_start (args, format);
        int len = vsnprintf(str, size-1, format, args);
        str[size-1] = '\0';
        return len;
}

int banner(char *dest, size_t sz, char border, const char *fmt, ...)
{
        va_list args;
        char tmp[sz];
        va_start (args, fmt);
        int len = vslprintf(tmp, sz, fmt, args);
        if (len < 0)
                return -1;
        if ((unsigned)len + 1 >= sz-1) {
                strlcpy(dest, tmp, sz);
                return len;
        }
        int pad_len = (sz - 1 - len);
        if (pad_len + (unsigned)len >= sz)
                return -1;
        memset(dest, border, pad_len + len);
        memcpy(dest + (pad_len/2), tmp, len);
        dest[pad_len + len] = '\0';
        return (pad_len + len);
}
