#ifndef __GDT_H_
#define __GDT_H_

/*
 * Definitions for the global descriptor table.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 06/15
 */

#include <stdint.h>

#define NUM_GDT_ENTRIES 6
#define GDT_KCODE_IND   1
#define GDT_KDATA_IND   2
#define GDT_UCODE_IND   3
#define GDT_UDATA_IND   4
#define GDT_TSS_IND     5

#define SEG_DESCTYPE(x)  ((x) << 0x04) // Descriptor type (0 for system, 1 for code/data)
#define SEG_PRES(x)      ((x) << 0x07) // Present
#define SEG_SAVL(x)      ((x) << 0x0C) // Available for system use
#define SEG_LONG(x)      ((x) << 0x0D) // Long mode
#define SEG_SIZE(x)      ((x) << 0x0E) // Size (0 for 16-bit, 1 for 32)
#define SEG_GRAN(x)      ((x) << 0x0F) // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)
#define SEG_PRIV(x)     (((x) &  0x03) << 0x05)   // Set privilege level (0 - 3)

#define SEG_DATA_RD        0x00 // Read-Only
#define SEG_DATA_RDA       0x01 // Read-Only, accessed
#define SEG_DATA_RDWR      0x02 // Read/Write
#define SEG_DATA_RDWRA     0x03 // Read/Write, accessed
#define SEG_DATA_RDEXPD    0x04 // Read-Only, expand-down
#define SEG_DATA_RDEXPDA   0x05 // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD  0x06 // Read/Write, expand-down
#define SEG_DATA_RDWREXPDA 0x07 // Read/Write, expand-down, accessed
#define SEG_CODE_EX        0x08 // Execute-Only
#define SEG_CODE_EXA       0x09 // Execute-Only, accessed
#define SEG_CODE_EXRD      0x0A // Execute/Read
#define SEG_CODE_EXRDA     0x0B // Execute/Read, accessed
#define SEG_CODE_EXC       0x0C // Execute-Only, conforming
#define SEG_CODE_EXCA      0x0D // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC     0x0E // Execute/Read, conforming
#define SEG_CODE_EXRDCA    0x0F // Execute/Read, conforming, accessed

#define GDT_CODE_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
        SEG_LONG(0)                  | SEG_SIZE(1) | SEG_GRAN(1) | \
        SEG_PRIV(0)                  | SEG_CODE_EXRD

#define GDT_DATA_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
        SEG_LONG(0)                  | SEG_SIZE(1) | SEG_GRAN(1) | \
        SEG_PRIV(0)                  | SEG_DATA_RDWR

#define GDT_CODE_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
        SEG_LONG(0)                  | SEG_SIZE(1) | SEG_GRAN(1) | \
        SEG_PRIV(3)                  | SEG_CODE_EXRD

#define GDT_DATA_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
        SEG_LONG(0)                  | SEG_SIZE(1) | SEG_GRAN(1) | \
        SEG_PRIV(3)                  | SEG_DATA_RDWR

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

struct gdt_entry gdt[NUM_GDT_ENTRIES];
struct gdt_ptr   gp;

void gdt_install();

#endif /* __GDT_H_ */

