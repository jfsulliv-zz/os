/*
 * machine/gdt.c
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 06/15
 */

#include <machine/gdt.h>
#include <stdint.h>

extern void gdt_flush(unsigned long);

void gdt_set_gate(int gate, unsigned int limit, unsigned int base,
                  unsigned short flags)
{
        /* Set limits */
        gdt[gate].limit_low   = limit & 0xFFFF;

        /* Set base address */
        gdt[gate].base_low    = base & 0xFFFF;
        gdt[gate].base_middle = (base >> 16) & 0xFFFF;
        gdt[gate].base_high   = (base >> 24) & 0xFFFF;

        /* Set access and granularity */
        gdt[gate].access      = flags & 0xFF;
        gdt[gate].granularity =  ((limit >> 16) & 0x0F);
        gdt[gate].granularity |= (flags >> 8) & 0xF0;
}

void gdt_install(void)
{
        gp.base = (unsigned long)&gdt;
        gp.limit = sizeof(struct gdt_entry) * NUM_GDT_ENTRIES;

        /* Default NULL gate */
        gdt_set_gate(0, 0, 0, 0);

        /* Set up a flat memory layout with separate CS/DS sections for
         * privileged and non-privileged segments. */
        gdt_set_gate(1, 0xFFFFFFFF, 0, GDT_CODE_PL0);
        gdt_set_gate(2, 0xFFFFFFFF, 0, GDT_DATA_PL0);
        gdt_set_gate(3, 0xFFFFFFFF, 0, GDT_CODE_PL3);
        gdt_set_gate(4, 0xFFFFFFFF, 0, GDT_DATA_PL3);

        gdt_flush((unsigned long)&gp);
}
