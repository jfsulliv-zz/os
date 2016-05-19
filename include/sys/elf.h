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

#ifndef _SYS_ELF_H_
#define _SYS_ELF_H_

#include <sys/elf32.h>
#include <sys/elf64.h>

#include <machine/params.h>

/* Define Elf_* aliases */

#if WORD_SIZE == 32

typedef Elf32_Addr        Elf_Addr;
typedef Elf32_Half        Elf_Half;
typedef Elf32_Off         Elf_Off;
typedef Elf32_Sword       Elf_Sword;
typedef Elf32_Word        Elf_Word;
typedef Elf32_Lword       Elf_Lword;

typedef Elf32_Hashelt     Elf_Hashelt;

typedef Elf32_Size        Elf_Size;
typedef Elf32_Ssize       Elf_Ssize;

typedef Elf32_Ehdr        Elf_Ehdr;
typedef Elf32_Shdr        Elf_Shdr;
typedef Elf32_Phdr        Elf_Phdr;
typedef Elf32_Dyn         Elf_Dyn;
typedef Elf32_Rel         Elf_Rel;
typedef Elf32_Rela        Elf_Rela;

#define ELF_R_SYM(info)         ELF32_R_SYM(info)
#define ELF_R_TYPE(info)        ELF32_R_TYPE(info)
#define ELF_R_INFO(sym,type)    ELF32_R_INFO(sym,type)

typedef Elf32_Move        Elf_Move;

#define ELF_M_SYM(info)         ELF32_M_SYM(info)
#define ELF_M_SIZE(info)        ELF32_M_SIZE(info)
#define ELF_M_INFO(sym,size)    ELF32_M_INFO(sym,size)

typedef Elf32_Cap         Elf_Cap;
typedef Elf32_Sym         Elf_Sym;

#define ELF_ST_BIND(info)       ELF32_ST_BIND(info)
#define ELF_ST_TYPE(info)       ELF32_ST_TYPE(info)
#define ELF_ST_INFO(bind,type)  ELF32_ST_INFO(bind,type)
#define ELF_ST_VISIBILITY(oth)  ELF32_ST_VISIBILITY(oth)

typedef Elf32_Verdef      Elf_Verdef;
typedef Elf32_Verdaux     Elf_Verdaux;
typedef Elf32_Verneed     Elf_Verneed;
typedef Elf32_Vernaux     Elf_Vernaux;
typedef Elf32_Versym      Elf_Versym;
typedef Elf32_Syminfo     Elf_Syminfo;

#elif WORD_SIZE == 64

typedef Elf64_Addr        Elf_Addr;
typedef Elf64_Half        Elf_Half;
typedef Elf64_Off         Elf_Off;
typedef Elf64_Sword       Elf_Sword;
typedef Elf64_Word        Elf_Word;
typedef Elf64_Lword       Elf_Lword;

typedef Elf64_Hashelt     Elf_Hashelt;

typedef Elf64_Size        Elf_Size;
typedef Elf64_Ssize       Elf_Ssize;

typedef Elf64_Ehdr        Elf_Ehdr;
typedef Elf64_Shdr        Elf_Shdr;
typedef Elf64_Phdr        Elf_Phdr;
typedef Elf64_Dyn         Elf_Dyn;
typedef Elf64_Rel         Elf_Rel;
typedef Elf64_Rela        Elf_Rela;

#define ELF_R_SYM(info)         ELF64_R_SYM(info)
#define ELF_R_TYPE(info)        ELF64_R_TYPE(info)
#define ELF_R_INFO(sym,type)    ELF64_R_INFO(sym,type)

typedef Elf64_Move        Elf_Move;

#define ELF_M_SYM(info)         ELF64_M_SYM(info)
#define ELF_M_SIZE(info)        ELF64_M_SIZE(info)
#define ELF_M_INFO(sym,size)    ELF64_M_INFO(sym,size)

typedef Elf64_Cap         Elf_Cap;
typedef Elf64_Sym         Elf_Sym;

#define ELF_ST_BIND(info)       ELF64_ST_BIND(info)
#define ELF_ST_TYPE(info)       ELF64_ST_TYPE(info)
#define ELF_ST_INFO(bind,type)  ELF64_ST_INFO(bind,type)
#define ELF_ST_VISIBILITY(oth)  ELF64_ST_VISIBILITY(oth)

typedef Elf64_Verdef      Elf_Verdef;
typedef Elf64_Verdaux     Elf_Verdaux;
typedef Elf64_Verneed     Elf_Verneed;
typedef Elf64_Vernaux     Elf_Vernaux;
typedef Elf64_Versym      Elf_Versym;
typedef Elf64_Syminfo     Elf_Syminfo;

#else

#error Unsupported WORD_SIZE value

#endif

#endif
