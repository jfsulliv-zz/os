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

char *strchr(const char *a, int c)
{
        char *p = (char *)a;
        while (*p) {
                if (*p == (char)c) {
                        return p;
                }
                p++;
        }
        return NULL;
}

char *strrchr(const char *a, int c)
{
        size_t len = strlen(a);
        if (len == 0) return NULL;
        char *p = (char *)(a + (len - 1));
        while (p >= a) {
                if (*p == (char)c) {
                        return p;
                }
                p--;
        }
        return NULL;
}

int strcmp(const char *a, const char *b)
{
        while (*a && *b)
        {
                if (*a == *b) {
                        a++;
                        b++;
                } else if (*a < *b) {
                        return -1;
                } else {
                        return 1;
                }
        }
        if (*a == '\0')
                return (*b == '\0' ? 0 : 1);
        return -1;
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
