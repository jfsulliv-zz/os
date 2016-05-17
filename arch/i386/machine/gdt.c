/*
 * machine/gdt.c
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 06/15
 */

#include <machine/gdt.h>
#include <machine/tss.h>
#include <sys/string.h>
#include <stdint.h>

static struct gdt_entry gdte_null = {
        .limit_low = 0,
        .base_low  = 0,
        .accessed  = 0,
        .read_write = 0,
        .conforming_expand_down = 0,
        .code = 0,
        .always_1 = 0,
        .dpl = 0,
        .present = 0,
        .limit_high = 0,
        .available = 0,
        .big = 0,
        .gran = 0,
        .base_high = 0
};

static struct gdt_entry gdte_code_ring0 = {
        .limit_low = 0xffff,
        .base_low  = 0,
        .accessed  = 0,
        .read_write = 1,
        .conforming_expand_down = 0,
        .code = 1,
        .always_1 = 1,
        .dpl = 0,
        .present = 1,
        .limit_high = 0xf,
        .available = 1,
        .big = 1,
        .gran = 1,
        .base_high = 0
};

static struct gdt_entry gdte_data_ring0 = {
        .limit_low = 0xffff,
        .base_low  = 0,
        .accessed  = 0,
        .read_write = 1,
        .conforming_expand_down = 0,
        .code = 0,
        .always_1 = 1,
        .dpl = 0,
        .present = 1,
        .limit_high = 0xf,
        .available = 1,
        .always_0 = 0,
        .big = 1,
        .gran = 1,
        .base_high = 0
};

static struct gdt_entry gdte_code_ring3 = {
        .limit_low = 0xffff,
        .base_low  = 0,
        .accessed  = 0,
        .read_write = 1,
        .conforming_expand_down = 0,
        .code = 1,
        .always_1 = 1,
        .dpl = 3,
        .present = 1,
        .limit_high = 0xf,
        .available = 1,
        .always_0 = 0,
        .big = 1,
        .gran = 1,
        .base_high = 0
};

static struct gdt_entry gdte_data_ring3 = {
        .limit_low = 0xffff,
        .base_low  = 0,
        .accessed  = 0,
        .read_write = 1,
        .conforming_expand_down = 0,
        .code = 0,
        .always_1 = 1,
        .dpl = 3,
        .present = 1,
        .limit_high = 0xf,
        .available = 1,
        .always_0 = 0,
        .big = 1,
        .gran = 1,
        .base_high = 0
};

static struct gdt_entry gdte_tss = {
        .limit_low = 0, // Set up dynamically
        .base_low  = 0, // Set up dynamically
        .accessed  = 1,
        .read_write = 0,
        .conforming_expand_down = 0,
        .code = 1,
        .always_1 = 0,
        .dpl = 3,
        .present = 1,
        .limit_high = 0, // Set up dynamically
        .available = 1,
        .always_0 = 0,
        .big = 0,
        .gran = 0,
        .base_high = 0 // Set up dynamically
};

extern void gdt_flush(unsigned long);

static void
gdt_set_gate(int gate, struct gdt_entry *e)
{
        memcpy(&gdt[gate], e, sizeof(struct gdt_entry));
}

void
gdt_install(void)
{
        gp.base = (unsigned long)&gdt;
        gp.limit = sizeof(struct gdt_entry) * NUM_GDT_ENTRIES;

        /* Default NULL gate */
        gdt_set_gate(0, &gdte_null);

        /* Set up a flat memory layout with separate CS/DS sections for
         * privileged and non-privileged segments. */
        gdt_set_gate(GDT_KCODE_IND, &gdte_code_ring0);
        gdt_set_gate(GDT_KDATA_IND, &gdte_data_ring0);
        gdt_set_gate(GDT_UCODE_IND, &gdte_code_ring3);
        gdt_set_gate(GDT_UDATA_IND, &gdte_data_ring3);

        /* Set up our TSS */
        tss_setup_gdte(&gdte_tss);
        gdt_set_gate(GDT_TSS_IND, &gdte_tss);

        gdt_flush((unsigned long)&gp);

        tss_install();

}
