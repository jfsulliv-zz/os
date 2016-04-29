#ifndef __CPU_I386_H__
#define __CPU_I386_H__

#include <machine/gdt.h>
#include <machine/idt.h>

/*
 * machine/cpu_i386.h
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 08/15
 */

typedef struct {
        char manufacturer_string[13];
        int max_basic_input_val;
        int max_ext_input_val;
        int features_ecx, features_edx;
        int ext_features_ecx, ext_features_edx;
        char stepping, model, family, type;
        char cache_line_size, logical_processors, lapic_id;
        char cpu_brand[49];
} cpuid_t;

struct arch_cpu {
        cpuid_t cpuid;
        struct gdt_entry gdt[NUM_GDT_ENTRIES];
        struct gdt_ptr   gp;
        struct idt_entry idt[NUM_IDT_ENTRIES];
        struct idt_ptr   idtp;
        /* TODO struct tss_entry tss; */
};

#endif
