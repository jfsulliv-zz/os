#ifndef _SYS_LINKERSET_H_
#define _SYS_LINKERSET_H_

#include <sys/base.h>
#include <util/sort.h>

/* Linker sets are grouped, statically allocated pieces of data of a
 * given type. The data is readable+writeable and is guaranteed to be
 * contiguous, although no order is guaranteed.
 *
 * LINKERSET_DEFINE(name, type) - Create a new linkerset.
 *   This belongs in exactly one .c file.
 * LINKERSET_ENTRY(set, object) - Add object to the linkerset.
 *   These can be placed anywhere else.
 *
 * Linkersets can be accessed with the following macros:
 *
 * LINKERSET_START(set) - Start address of the linkerset
 * LINKERSET_LIMIT(set) - End address of the linkerset
 * LINKERSET_NUM(set) - Count of the linkerset
 * LINKERSET_AT(set, ind) - i'th entry of the linkerset
 * LINKERSET_FOREACH(set, ptr) - Iterate through the linkerset
 * LINKERSET_SORT(set, cmp, swap) - Sort the linkerset using the given
 *      comparator and swapper functions, with the following formats:
 *      int cmp(void *, void *)
 *      void swap(void *, void *)
 */

#define LINKERSET_DEFINE(name, type) \
        extern type __attribute__((weak)) *__start_set_##name; \
        extern type __attribute__((weak)) *__stop_set_##name; \
        GLOBAL(__start_set_##name); \
        GLOBAL(__stop_set_##name) \

#define LINKERSET_ENTRY(set, object) \
        static void const *const __set_##set##_sym_##object \
                __attribute((section("set_" #set), used)) = &object;

#define LINKERSET_START(set) \
        &__start_set_##set

#define LINKERSET_LIMIT(set) \
        &__stop_set_##set

#define LINKERSET_NUM(set) \
        (LINKERSET_LIMIT(set) - LINKERSET_START(set))

#define LINKERSET_AT(set, ind) \
        ((LINKERSET_START(set))[ind])

#define LINKERSET_FOREACH(set, ptr) \
        for (ptr = LINKERSET_START(set); \
             ptr < LINKERSET_LIMIT(set); \
             ptr++)

#define LINKERSET_SORT(set, cmp, swap) \
        sort(LINKERSET_BEGIN(set), \
             LINKERSET_COUNT(set), \
             sizeof(LINKERSET_START(set), \
             cmp, \
             swap)

#endif
