#ifndef _UTIL_HASH_H_
#define _UTIL_HASH_H_

#include <stdint.h>
#include <stddef.h>

uint32_t
jenkins_hash32(const void *key, size_t length, uint32_t initval);

#endif
