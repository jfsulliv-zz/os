#ifndef _GDT_I386_H_
#define _GDT_I386_H_

#define NUM_GDT_ENTRIES 6
#define GDT_KCODE_IND   1
#define GDT_KDATA_IND   2
#define GDT_UCODE_IND   3
#define GDT_UDATA_IND   4
#define GDT_TSS_IND     5

struct gdt_entry
{
        unsigned limit_low:             16;
        unsigned base_low :             24;
        unsigned accessed :             1;
        unsigned read_write:            1;
        unsigned conforming_expand_down:1;
        unsigned code:                  1;
        unsigned always_1:              1;
        unsigned dpl:                   2;
        unsigned present:               1;
        unsigned limit_high:            4;
        unsigned available:             1;
        unsigned always_0:              1;
        unsigned big:                   1;
        unsigned gran:                  1;
        unsigned base_high:             8;
} __attribute__((packed));

struct gdt_ptr
{
        unsigned short limit;
        unsigned int base;
} __attribute__((packed));

#endif
