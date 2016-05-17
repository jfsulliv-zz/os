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

#ifndef _MULTIBOOT_H_
#define _MULTIBOOT_H_

/* multiboot.h - the header for Multiboot */
/* Copyright (C) 1999, 2001  Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

/* Macros. */

/* The magic number for the Multiboot header. */
#define MULTIBOOT_HEADER_MAGIC          0x1BADB002

/* The flags for the Multiboot header. */
#ifdef __ELF__
# define MULTIBOOT_HEADER_FLAGS         0x00000003
#else
# define MULTIBOOT_HEADER_FLAGS         0x00010003
#endif

/* The magic number passed by a Multiboot-compliant boot loader. */
#define MULTIBOOT_BOOTLOADER_MAGIC      0x2BADB002

/* The size of our stack (16KB). */
#define STACK_SIZE                      0x4000

/* C symbol format. HAVE_ASM_USCORE is defined by configure. */
#ifdef HAVE_ASM_USCORE
# define EXT_C(sym)                     _ ## sym
#else
# define EXT_C(sym)                     sym
#endif

#ifndef ASM
/* Do not include here in boot.S. */

/* Types. */

/* The Multiboot header. */
typedef struct multiboot_header
{
        unsigned long magic;
        unsigned long flags;
        unsigned long checksum;
        unsigned long header_addr;
        unsigned long load_addr;
        unsigned long load_end_addr;
        unsigned long bss_end_addr;
        unsigned long entry_addr;
} multiboot_header_t;

/* The symbol table for a.out. */
typedef struct aout_symbol_table
{
        unsigned long tabsize;
        unsigned long strsize;
        unsigned long addr;
        unsigned long reserved;
} aout_symbol_table_t;

/* The section header table for ELF. */
typedef struct elf_section_header_table
{
        unsigned long num;
        unsigned long size;
        unsigned long addr;
        unsigned long shndx;
} elf_section_header_table_t;

/* The Multiboot information. */
typedef struct multiboot_info
{
        unsigned long flags;
        unsigned long mem_lower;
        unsigned long mem_upper;
        unsigned long boot_device;
        unsigned long cmdline;
        unsigned long mods_count;
        unsigned long mods_addr;
        union
        {
                aout_symbol_table_t aout_sym;
                elf_section_header_table_t elf_sec;
        } u;
        unsigned long mmap_length;
        unsigned long mmap_addr;
} multiboot_info_t;

/* The module structure. */
typedef struct module
{
        unsigned long mod_start;
        unsigned long mod_end;
        unsigned long string;
        unsigned long reserved;
} module_t;

/* The memory map. Be careful that the offset 0 is base_addr_low
   but no size. */
typedef struct memory_map
{
        unsigned long size;
        unsigned long base_addr_low;
        unsigned long base_addr_high;
        unsigned long length_low;
        unsigned long length_high;
        unsigned long type;
} memory_map_t;

#endif /* ! ASM */

#endif
