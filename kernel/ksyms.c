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

#include <mm/vma.h>
#include <sys/kprintf.h>
#include <sys/ksyms.h>
#include <sys/panic.h>
#include <sys/stdlib.h>
#include <sys/string.h>
#include <util/list.h>

/* This table is assumed to be sorted before we loaded into our ELF.
 * Otherwise, translation will get all messed up. */
extern const char *_binary_ksyms_bin_start;
extern const char *_binary_ksyms_bin_end;
extern const unsigned long *_binary_ksyms_bin_size;

/* A single ksyms record. */
typedef struct ksyms_entry {
        unsigned long addr;
        const char   *name;
} ksyms_entry_t;

static const char *ksyms_start;
static const char *ksyms_end;
static unsigned long ksyms_size;

static ksyms_entry_t *entry_tab = NULL;
static unsigned long  entry_num = 0;
static unsigned long  entry_max = 1024;

static bool ksyms_broken      = false;
static bool ksyms_initialized = false;

static int
_ksyms_init(void)
{
        unsigned long n = 0;

        entry_tab = kmalloc(sizeof(ksyms_entry_t) * entry_max, M_KERNEL);
        if (!entry_tab)
                return 1;

        while (n < ksyms_size)
        {
                const char *addr_str = ksyms_start + n;
                const char *name_str = addr_str + strlen(addr_str) + 1;

                if (name_str >= ksyms_end)
                        break;

                if (entry_num + 1 >= entry_max) {
                        ksyms_entry_t *t = krealloc(entry_tab,
                                                    (entry_max *= 2),
                                                    M_KERNEL);
                        if (!t) {
                                kfree(entry_tab);
                                return 1;
                        }
                        entry_tab = t;
                }
                entry_tab[entry_num].addr = strtoul(addr_str, NULL, 16);
                entry_tab[entry_num++].name = name_str;
        }
        return 0;
}

int
ksyms_init(void)
{
        int ret;

        ksyms_start = (const char *)&_binary_ksyms_bin_start;
        ksyms_end = (const char *)&_binary_ksyms_bin_end;
        ksyms_size = (unsigned long)&_binary_ksyms_bin_size;

        if ((ret = _ksyms_init()))
                ksyms_broken = true;
        ksyms_initialized = true;
        return ret;
}


const char *ksyms_unknown = "????????";

const char *
ksyms_find_func(unsigned long addr)
{
        if (ksyms_broken)
                return ksyms_unknown;
        bug_on(!ksyms_initialized, "ksyms used before initialization.");

        /* Binary search our table to find the function that corresponds
         * to the address. Note that this will probably not be a perfect
         * match of `addr', but instead it will return the greatest
         * address that is less than `addr'. */
        ksyms_entry_t *ent = NULL;

        while (1)
        {
                unsigned long ind = entry_num / 2;
                ent = &entry_tab[ind];

                if (ent->addr < addr) {
                        if (ind == 0)
                                return ksyms_unknown;
                        ind /= 2;
                } else if (ent->addr > addr) {
                        /* Check to see if we're done. */
                        if ((ind == entry_num - 1) ||
                                (entry_tab[ind+1].addr > addr))
                            break;
                        else
                                ind += ((entry_num - ind) / 2);
                } else if (ent->addr == addr) {
                        break;
                }
        }
        return ent->name;
}
