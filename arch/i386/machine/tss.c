#include <machine/gdt.h>
#include <machine/irq.h>
#include <machine/tss.h>
#include <sys/string.h>
#include <stdint.h>

struct tss_entry tss;

void
tss_setup_gdte(struct gdt_entry *gdte)
{
        unsigned long base = (unsigned long)&tss;
        unsigned long limit = sizeof(struct tss_entry);

        gdte->limit_low  = (limit & 0xFFFF);
        gdte->base_low   = (base  & 0xFFFFFF);
        gdte->limit_high = (limit & 0xF0000) >> 16;
        gdte->base_high  = (base  & 0xFF000000) >> 24;

}

extern void tss_flush(int ind);

void
tss_install(void)
{
        memset(&tss, 0, sizeof(struct tss_entry));

        tss.ss0 = GDT_KDATA_IND * sizeof(struct tss_entry);
        tss.esp0 = (unsigned long)irq_stack;

        tss_flush(GDT_TSS_IND);
}

void
set_kernel_stack(uint32_t stack)
{
        tss.esp0 = stack;
}
