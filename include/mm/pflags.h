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

#ifndef _MM_PROTFLAGS_H_
#define _MM_PROTFLAGS_H_

#include <stdint.h>
#include <sys/bitops_generic.h>

#define PFLAGS_READ_BIT         0
#define PFLAGS_WRITE_BIT        1
#define PFLAGS_EXEC_BIT         2

#define PFLAGS_GOOD_MASK        (GENMASK(3, 0))
#define BAD_PFLAGS(f)           ((f) & ~PFLAGS_GOOD_MASK)

#define PFLAGS_READ             (1 << PFLAGS_READ_BIT)
#define PFLAGS_WRITE            (1 << PFLAGS_WRITE_BIT)
#define PFLAGS_EXEC             (1 << PFLAGS_EXEC_BIT)

#define PFLAGS_R                (PFLAGS_READ                             )
#define PFLAGS_W                (              PFLAGS_WRITE              )
#define PFLAGS_X                (                             PFLAGS_EXEC)
#define PFLAGS_RW               (PFLAGS_READ | PFLAGS_WRITE              )
#define PFLAGS_RX               (PFLAGS_READ                | PFLAGS_EXEC)
#define PFLAGS_WX               (              PFLAGS_WRITE | PFLAGS_EXEC)
#define PFLAGS_RWX              (PFLAGS_READ | PFLAGS_WRITE | PFLAGS_EXEC)

typedef uint32_t pflags_t;

#endif
