#ifndef _SYS_DEBUG_H_
#define _SYS_DEBUG_H_

#include <sys/config.h>

#ifdef CONF_DEBUG
#define __test
#define DO_TEST(f) f()
#else
#define __test  __attribute__((unused))
#define DO_TEST(f)
#endif

#endif
