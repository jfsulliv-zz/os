#ifndef _IDT_I386_H_
#define _IDT_I386_H_

struct idt_entry
{
        unsigned short base_lo;
        unsigned short sel;             /* segment selector */
        unsigned char  zero;            /* unused -- set to 0 */
        unsigned char  type_attr;       /* types & attributes */
        unsigned short base_hi;
} __attribute__((packed));

struct idt_ptr
{
        unsigned short limit;
        unsigned int base;
} __attribute__((packed));

#endif
