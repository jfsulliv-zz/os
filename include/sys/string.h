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
char *strncat(char *dest, const char *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void bzero(void *s, size_t num);

#endif /* _SYS_STRING_H_ */
