#ifndef __IDT_H_
#define __IDT_H_
/*
 * Definitions for the interrupt decriptor table.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 06/15
 */

#include <stdint.h>

#define NUM_IDT_ENTRIES         256
#define INT_EXCEPTION_BASE      0
#define INT_EXCEPTION_LIMIT     32
#define INT_EXCEPTION_NUM       32
#define INT_IRQ_BASE            32
#define INT_IRQ_LIMIT           48
#define INT_IRQ_NUM             16

/* IDT gate types */
#define IDT_TASK_GATE_TYP       0x5
#define IDT_16_INTR_GATE_TYP    0x6
#define IDT_16_TRAP_GATE_TYP    0x7
#define IDT_32_INTR_GATE_TYP    0xE
#define IDT_32_TRAP_GATE_TYP    0xF

#define IDT_PRES(x) (x << 0x7) // Present bit
#define IDT_DPL(x) ((x & 0x3) << 0x5) // DPL flags

#define IDT_32_INTERRUPT IDT_32_INTR_GATE_TYP | IDT_PRES(1) | \
                         IDT_DPL(0x0)

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

struct idt_entry idt[NUM_IDT_ENTRIES];
struct idt_ptr   idtp;

extern const char *exception_messages[INT_EXCEPTION_LIMIT];

void idt_set_gate(unsigned char num, unsigned int base, unsigned short sel,
                  unsigned char flags);
void idt_install(void);

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

#endif /* __IDT_H_ */

