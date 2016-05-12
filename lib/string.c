/*
 * libc/string.c
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 06/15
 */

#include <stddef.h>
#include <stdint.h>
#include <sys/string.h>

size_t strlen(const char *str)
{
	char *p= (char *)str;
	while (*p++);
	return (p - 1) - str;
}

char *strcpy(char *dst, const char *from)
{
	char *orig_dst = dst;
	do {
		*dst++ = *from;
	} while (*from++);
	return orig_dst;
}

char *strncpy(char *dst, const char *from, size_t n)
{
	char *orig_dst = dst;
	while (*from && n--)
	{
		*dst++ = *from++;
	}
	if (n > 0) {
		*dst = 0;
	}
	return orig_dst;
}

char *strlcpy(char *dst, const char *from, size_t n)
{
        char *ret = strncpy(dst, from, n);
        dst[n-1] = '\0';
        return ret;
}

char *strncat(char *dst, const char *from, size_t n)
{
        char *orig_dst = dst;
        while (*dst && n > 0)
        {
                dst++;
                n--;
        }
        while (n-- > 0)
        {
                *dst++ = *from++;
        }
        if (n > 0) {
                *dst = '\0';
        }
        return orig_dst;
}

char *strlcat(char *dest, const char *src, size_t n)
{
        char *ret = strncat(dest, src, n);
        ret[n-1] = '\0';
        return ret;
}

void *memset(void *s, int c, size_t n)
{
	void *orig_s = s;
	char *cs = s;
	while (n--)
	{
		*cs++ = (char)c;
	}
	return orig_s;
}

void *memcpy(void *dest, const void *src, size_t n)
{
	char *orig_dst = dest, *dst = dest;
	char *from = (char *)src;
	while (n--)
	{
		*dst++ = *from++;
	}
	return (void *)orig_dst;
}

void bzero(void *s, size_t num)
{
        memset(s, 0, num);
}
