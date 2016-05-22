#ifndef _IDT_I686_H_
#define _IDT_I686_H_

#include <stdint.h>

struct idt_entry
{
        unsigned        base_low:2;
        unsigned        sel:2;
        unsigned        zero1:1;
        unsigned        type_attr:1;
        unsigned        base_mid:2;
        unsigned        base_high:4;
        unsigned        zero2:4;
} __attribute__((packed));

struct idt_ptr
{
        unsigned short limit;
        uint64_t       base;
} __attribute__((packed));

#endif
