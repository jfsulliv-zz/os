#ifndef _SYS_ASSERT_H_
#define _SYS_ASSERT_H_

#include <sys/panic.h>

#define assert(cond)    if (!(cond)) {          \
        bug("Assertion `" #cond "' failed.");   \
};

#endif
