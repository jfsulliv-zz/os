#ifndef _SYS_STRING_H_
#define _SYS_STRING_H_

/*
 * sys/string.h
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 12/14
 */

#include <stddef.h>
#include <stdint.h>

size_t strlen(const char *str);
char *strcpy(char *dst, const char *from);
char *strncpy(char *dst, const char *from, size_t n);
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);

#endif /* _SYS_STRING_H_ */
