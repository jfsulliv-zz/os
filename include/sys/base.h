#ifndef _SYS_BASE_H_
#define _SYS_BASE_H_

#include <stddef.h>

#define container_of(ptr, type, member) \
        ((type *) ((char *)(ptr) - offsetof(type, member)))

#endif
