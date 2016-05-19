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
#include <mm/vma.h>
#include <sys/elf.h>
#include <sys/kprintf.h>
#include <sys/ksyms.h>
#include <sys/panic.h>
#include <sys/stdlib.h>
#include <sys/string.h>
#include <util/list.h>
#include <util/sort.h>

/* We will set up these pointers from the addresses of the above. */
static const Elf_Sym *ksyms_start;
static const Elf_Sym *ksyms_end;

static const char *kstrtab_start;
static const char *kstrtab_end;

static ksyms_entry_t *entry_tab = NULL;
static unsigned long  entry_num = 0;
static unsigned long  entry_max = 1024;

static bool ksyms_broken      = false;
static bool ksyms_initialized = false;

static int ksyms_cmp(const void *va, const void *vb)
{
        const ksyms_entry_t *a = va, *b = vb;
        if (a->addr < b->addr)
                return -1;
        else if (a->addr > b->addr)
                return 1;
        return 0;
}

static void ksyms_swap(void *va, void *vb)
{
        ksyms_entry_t *a = va, *b = vb;
        unsigned long addr = a->addr;
        const char *name = a->name;

        a->addr = b->addr;
        a->name = b->name;
        b->addr = addr;
        b->name = name;
}

static int
_ksyms_init(void)
{
        unsigned long n = 0;
        const Elf_Sym *sym = ksyms_start;

        entry_tab = kmalloc(sizeof(ksyms_entry_t) * entry_max, M_KERNEL);
        if (!entry_tab)
                return 1;

        for (; sym < ksyms_end; sym++)
        {
                if (ELF_ST_TYPE(sym->st_info) != STT_FUNC)
                        continue;

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
                entry_tab[entry_num].addr = sym->st_value;
                bug_on (kstrtab_start + sym->st_name > kstrtab_end,
                        "ELF symbol table refers outside of string table.");
                entry_tab[entry_num++].name = kstrtab_start + sym->st_name;
        }
        sort(entry_tab, entry_num, sizeof(ksyms_entry_t),
             ksyms_cmp, ksyms_swap);
        return 0;
}

static int
_ksyms_load(multiboot_info_t *mbd)
{
        Elf_Word sym_size, strtab_size;

        if ((ksyms_start = mb_load_section(mbd, ".symtab")) == NULL)
                return 1;
        if ((kstrtab_start = mb_load_section(mbd, ".strtab")) == NULL)
                return 1;

        sym_size = mb_find_section(mbd, ".symtab")->sh_size;
        strtab_size = mb_find_section(mbd, ".strtab")->sh_size;

        ksyms_end = (void *)ksyms_start + sym_size;
        kstrtab_end = kstrtab_start + strtab_size;

        return 0;
}

int
ksyms_init(multiboot_info_t *mbd)
{
        int ret;

        if ((ret = _ksyms_load(mbd))) {
                ksyms_broken = true;
                goto out;
        }

        if ((ret = _ksyms_init()))
                ksyms_broken = true;
out:
        ksyms_initialized = true;
        return ret;
}

ksyms_entry_t *
ksyms_find(unsigned long addr)
{
        if (ksyms_broken)
                return NULL;
        bug_on(!ksyms_initialized, "ksyms used before initialization.");

        /* Binary search our table to find the function that corresponds
         * to the address. Note that this will probably not be a perfect
         * match of `addr', but instead it will return the greatest
         * address that is less than `addr'. */
        ksyms_entry_t *ent = NULL;
        unsigned long ind = entry_num / 2;
        unsigned long bot = 0;
        unsigned long top = entry_num;

        while (bot < top)
        {
                ind = (top + bot) / 2;
                ent = &entry_tab[ind];

                if (addr < ent->addr) {
                        if (ind == 0)
                                return NULL;
                        top = ind;
                } else if (addr > ent->addr) {
                        /* Check to see if we're done. */
                        if ((ind == entry_num - 1) ||
                                (entry_tab[ind+1].addr > addr)) {
                            break;
                        } else {
                                bot = ind;
                        }
                } else if (ent->addr == addr) {
                        break;
                }
        }
        return ent;
}

const char *ksyms_broken_str = "NO_SYMS";
const char *ksyms_unknown = "????????";

const char *
ksyms_find_func(unsigned long addr)
{
        if (ksyms_broken)
                return ksyms_broken_str;
        bug_on(!ksyms_initialized, "ksyms used before initialization.");

        ksyms_entry_t *ent = ksyms_find(addr);
        if (ent)
                return ent->name;
        return ksyms_unknown;
}

char ksyms_report_buf[256];

char *
ksyms_report_eip(unsigned long addr)
{
        if (ksyms_broken)
                return ksyms_broken_str;
        bug_on(!ksyms_initialized, "ksyms used before initialization.");

        ksyms_entry_t *ent = ksyms_find(addr);
        if (ent) {
                slprintf(ksyms_report_buf, 256, "0x%08x (%s+0x%x)",
                         addr, ent->name, addr - ent->addr);
        } else {
                slprintf(ksyms_report_buf, 256, "0x%08x (%s)",
                         addr, ksyms_unknown);
        }
        return ksyms_report_buf;

}
