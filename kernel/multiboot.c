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

#include <multiboot.h>
#include <mm/paging.h>
#include <mm/page_table.h>
#include <sys/elf.h>
#include <sys/string.h>
#include <sys/panic.h>
#include <sys/stdio.h>

static bool failed = false;
static bool shdrs_loaded = false;
static bool shstrtab_loaded = false;

static Elf_Shdr *shdrs = NULL;
static char     *shstrtab = NULL;

int
mb_load_shdrs(multiboot_info_t *mbd)
{
        if (failed)
                return 1;
        if (shdrs_loaded)
                return 0;

        if (!mbd || !(mbd->flags & (1<<5))) {
                failed = true;
                return 1;
        }

        shdrs = (void *)(KERN_BASE + (vaddr_t)(mbd->u.elf_sec.addr));
        shdrs_loaded = true;
        return 0;
}

int
mb_load_shstrtab(multiboot_info_t *mbd)
{
        unsigned long i = 0;

        if (failed)
                return 1;
        if (shstrtab_loaded)
                return 0;

        if (mb_load_shdrs(mbd))
                return 1;

        for (; i < mbd->u.elf_sec.num; i++)
        {
                if (shdrs[i].sh_type == SHT_STRTAB)
                        break;
        }
        if (i == mbd->u.elf_sec.num) {
                failed = true;
                return 1;
        }

        shstrtab = (void *)(KERN_BASE + (vaddr_t)shdrs[i].sh_addr);
        shstrtab_loaded = true;
        return 0;
}

static Elf_Shdr *
_mb_find_section(multiboot_info_t *mbd, const char *name)
{
        /* Assumes shdrs, shstrtab are loaded */
        Elf_Shdr *shdr;
        unsigned long i;
        for (i = 0; i < mbd->u.elf_sec.num; i++)
        {
                shdr = shdrs + i;
                const char *sh_name = shstrtab + shdr->sh_name;
                if (!strcmp(name, sh_name))
                        break;
        }
        if (i < mbd->u.elf_sec.num)
                return shdrs + i;
        return NULL;
}

void *
mb_load_section(multiboot_info_t *mbd, const char *name)
{
        Elf_Shdr *shdr = mb_find_section(mbd, name);
        if (!shdr)
                return NULL;
        return (void *)(KERN_BASE + (vaddr_t)shdr->sh_addr);
}

Elf_Shdr *
mb_find_section(multiboot_info_t *mbd, const char *name)
{
        Elf_Shdr *shdr;

        if (failed)
                return NULL;

        if (mb_load_shstrtab(mbd))
                return NULL;

        if ((shdr = _mb_find_section(mbd, name)) == NULL)
                return NULL;

        return shdr;
}
