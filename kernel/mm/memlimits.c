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
#include <mm/memlimits.h>
#include <mm/paging.h>
#include <sys/kprintf.h>
#include <sys/stdio.h>
#include <sys/panic.h>
#include <util/cmp.h>

/* The linker should provide this, the address of which will be the
 * first byte after the end of the kernel image. */
extern char kernel_end;

void
detect_memory_limits(memlimits_t *to, multiboot_info_t *mbd)
{
        size_t start_pfn, max_pfn;
        vaddr_t last_address = (vaddr_t)&kernel_end;

        if (!mbd || !(mbd->flags & 1)) {
                // TODO manual memory map
                panic("No memory map available.\n");
        }
        bug_on(!to, "NULL memlimits reference.");

        if (mbd->flags & (1<<5)) {
                /* There are ELF headers present which we ought to
                 * preserve. */
                unsigned int i;
                Elf_Shdr *sh = (void *)(KERN_BASE + mbd->u.elf_sec.addr);
                for (i = 0; i < mbd->u.elf_sec.num; i++)
                {
                        vaddr_t sec_end = KERN_BASE + sh[i].sh_addr +
                                sh[i].sh_size;
                        if (sec_end > last_address)
                                last_address = sec_end;
                }
        }

        start_pfn = PFN_UP(_pa(last_address)); /* Next free page */
        max_pfn   = PFN_DOWN(1024ULL * mbd->mem_upper);

        /* Determine how much free memory we have for DMA, the kernel,
         * and the user.
         * We allocate 1/4 of physical memory after the end of the
         * kernel image to the kernel and 3/4 to the user. Everything
         * before KERN_OFFS is allocated as DMA. */
        to->dma_pfn = 1;
        to->dma_pfn_end  = (KERN_OFFS / PAGE_SIZE);
        to->low_pfn  = start_pfn;
        to->high_pfn = start_pfn + ((max_pfn - start_pfn) >> 2);
        to->max_pfn  = max_pfn;

        bug_on(to->dma_pfn_end >= to->low_pfn,
                        "Insufficient DMA memory.");
        bug_on(to->low_pfn >= to->high_pfn,
                        "Insufficient low memory.");
        bug_on(to->high_pfn >= to->max_pfn,
                        "Insufficient high memory.");
}

void
limits_report(memlimits_t *limits)
{
        char buf[80];
        banner(buf, sizeof(buf), 12 + 4 + (WORD_SIZE / 2), '=',
               " Memory Available ");
        kprintf(0, "%s\n", buf);
        kprintf(0, "["PFMT " - " PFMT "] kernel\n"
                   "["PFMT " - " PFMT "]   user\n",
                   _va(lowmem_base(limits)), _va(lowmem_top(limits)),
                   highmem_base(limits), highmem_top(limits));
}

