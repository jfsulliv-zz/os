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

#ifndef _SYS_ENDIAN_H_
#define _SYS_ENDIAN_H_

/*
 * sys/endian.h - Convenience macros for explicit endian integers.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 04/17
 */

#include <machine/bits.h>

#if LITTLE_ENDIAN

#define GET_LITTLE16(x) (x)
#define GET_LITTLE32(x) (x)

#define SET_LITTLE16(x, val) (x) = (val)
#define SET_LITTLE32(x, val) (x) = (val)

#define GET_BIG16(x) (((x) >> 8) | ((x) << 8))
#define GET_BIG32(x) (((x) >> 24) | \
                     (((x) & 0x00FF0000) >> 8) | \
                     (((x) & 0x0000FF00) << 8) | \
                      ((x) << 24))

#define SET_BIG16(x, val) (x) = (GET_BIG16(val))
#define SET_BIG32(x, val) (x) = (GET_BIG32(val))

#else

#define GET_LITTLE16(x) (((x) >> 8) | ((x) << 8))
#define GET_LITTLE32(x) (((x) >> 24) | \
                        (((x) & 0x00FF0000) >> 8) | \
                        (((x) & 0x0000FF00) << 8) | \
                         ((x) << 24))

#define SET_LITTLE16(x, val) (x) = (GET_BIG16(val))
#define SET_LITTLE32(x, val) (x) = (GET_BIG32(val))

#define GET_BIG16(x) (x)
#define GET_BIG32(x) (x)

#define SET_BIG16(x, val) (x) = (val)
#define SET_BIG32(x, val) (x) = (val)

#endif

#endif
