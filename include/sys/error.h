/*
Copyright (c) 2017, James Sullivan <sullivan.james.f@gmail.com>
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
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote
      products derived from this software without specific prior
      written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL JAMES SULLIVAN
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _SYS_ERROR_H_
#define _SYS_ERROR_H_

/*
 * sys/error.h - Macros for handling errors. 
 *
 * CHECK[V,E] - Check a condition and return if it fails.
 *              [V] variant returns no value.
 *              [E] variant sets a passed error pointer.
 * 
 * ERROR[V] - Return and display an error message.
 *            [V] variant returns no value.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 07/17
 */

#include <sys/kprintf.h>

#include <stddef.h>

/* Returns 'ret' if 'cond' is true. */
#define CHECK(cond, ret, msg, ...) {\
        if (cond) { \
                ERROR((ret), (msg), ##__VA_ARGS__); \
        } \
}

/* Returns 'ret' if 'cond' is true, setting *errno = errno_val */
#define CHECKE(cond, ret, errno, errno_val, msg, ...) {\
        if (cond) { \
                if (errno) *(errno) = (errno_val); \
                ERROR((ret), (msg), ##__VA_ARGS__); \
        } \
}

/* Returns if 'cond' is true. */
#define CHECKV(cond, msg, ...) {\
        if (cond) { \
                ERRORV((msg), ##__VA_ARGS__); \
        } \
}

/* Returns if 'cond' is true. */
#define CHECKVE(cond, errno, errno_val, msg, ...) {\
        if (cond) { \
                if (errno) *(errno) = (errno_val); \
                ERRORV((msg), ##__VA_ARGS__); \
        } \
}

/* Returns 'ret' if 'ptr' is null. */
#define CHECK_NONNULL(ptr, ret) \
        CHECK((ptr) != NULL, ret, "Unexpected null: %s", #ptr)

/* Returns if 'ptr' is null. */
#define CHECKV_NONNULL(ptr) \
        CHECKV((ptr) != NULL, "Unexpected null: %s", #ptr)

/* Returns if 'ptr' is null, setting 'err' to 'errval'. */
#define CHECKVE_NONNULL(ptr, ret, errno, errno_val) \
        CHECKVE((ptr) != NULL, ret, errno, errno_val, \
                "Unexpected null: %s", #ptr)

/* Returns 'ret', displaying an error message. */
#define ERROR(ret, msg, ...) { \
        kprintf(PRI_ERR, "%s: ", __func__); \
        kprintf(PRI_ERR, (msg), ##__VA_ARGS__); \
        return (ret); \
}

/* Returns after displaying an error message. */
#define ERRORV(msg, ...) { \
        kprintf(PRI_ERR, "%s: ", __func__); \
        kprintf(PRI_ERR, (msg), ##__VA_ARGS__); \
        return; \
}

#endif
