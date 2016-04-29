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
					expanded_len = strlen(sarg);
					memcpy(tmp, sarg, expanded_len > 255
							   ? 255
							   : expanded_len);
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

int snprintf(char *str, size_t size, const char *format, ...)
{
	va_list args;
	va_start (args, format);
	return vsnprintf(str, size, format, args);
}
