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
