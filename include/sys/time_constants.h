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

#ifndef _SYS_TIME_CONSTANTS_H_

/*
 * sys/time_constants.h - Conversion between time units
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 11/16
 */

#include <stdint.h>

#define MIN_PER_HR      (60UL)

#define SEC_PER_MIN     (60UL)
#define SEC_PER_HR      (3600UL)

#define MSEC_PER_SEC    (1000UL)
#define MSEC_PER_MIN    (60000UL)
#define MSEC_PER_HR     (3600000UL)

#define USEC_PER_MSEC   (1000UL)
#define USEC_PER_SEC    (1000000UL)
#define USEC_PER_MIN    (60000000UL)
#define USEC_PER_HR     (3600000000UL)

#define NSEC_PER_USEC   (1000UL)
#define NSEC_PER_MSEC   (1000000UL)
#define NSEC_PER_SEC    (1000000000UL)
#define NSEC_PER_MIN    (60000000000UL)
#define NSEC_PER_HR     (3600000000000UL)

#endif
