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
#include <machine/regs.h>
#include <machine/arch_init.h>
#include <machine/timer.h>
#include <mm/init.h>
#include <mm/paging.h>
#include <mm/pfa.h>
#include <mm/vma.h>
#include <sys/config.h>
#include <sys/ksyms.h>
#include <sys/debug.h>
#include <sys/kprintf.h>

int create_init(void);

int
main(multiboot_info_t *mbd)
{
        /* Set up the hardware as needed. */
        arch_init(mbd);
        timer_install();
        /* Set up our initial page mapping */
        init_paging(&mem_limits);
        /* Set up the page frame allocator */
        pfa_init(&mem_limits);
        pfa_report(false);
        DO_TEST(pfa_test);
        /* Set up the VMA */
        vma_init();
        DO_TEST(vma_test);
        vma_report();
        /* Okay, now we can get some symbols. */
        ksyms_init(mbd);
        kprintf(0, "Initialized kernel symbols\n");
        /* Set up the process tables and pid 1 */
        proc_system_init();
        kprintf(0, "Initialized process tables, init process\n");

        for (;;);
}
