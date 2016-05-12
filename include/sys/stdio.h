#ifndef _SYS_STDIO_H_
#define _SYS_STDIO_H_

/*
 * sys/stdio.h
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 04/16
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

static inline bool isdigit(char c)
{
        return (c >= '0' && c < '9');
}

static inline bool isspace(char c)
{
        return (c == ' ' || c == '\r' || c == '\n');
}

static inline int ord(char c)
{
        return (int)c;
}

int snprintf(char *str, size_t size, const char *format, ...);
int slprintf(char *str, size_t size, const char *format, ...);
int vsnprintf(char *str, size_t size, const char *format, va_list args);
int vslprintf(char *str, size_t size, const char *format, va_list args);

int banner(char *dest, size_t sz, char border, const char *fmt, ...);

#endif /* _SYS_STDIO_H_ */
