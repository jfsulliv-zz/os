MODULES += $(dir)

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

SRCS_$(d) := $(d)/arch_init.c $(d)/tty.c $(d)/irq.c $(d)/gdt.c \
             $(d)/idt.c $(d)/pic.c $(d)/isr.c $(d)/regs.c $(d)/timer.c \
             $(d)/fault.c $(d)/tss.c
ASRCS_$(d) := $(d)/isr_asm.s

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
