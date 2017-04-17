MODULES += $(dir)

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

SRCS_$(d) := $(d)/gdt.c $(d)/idt.c $(d)/regs.c $(d)/tss.c $(d)/isr.c \
             $(d)/msr.c $(d)/syscall.c $(d)/arch_cpu_64.c
ASRCS_$(d) := $(d)/isr_asm.s $(d)/regs_asm.s $(d)/syscall_asm.s

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
