MODULES += $(dir)

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

SRCS_$(d) := $(d)/arch_init.c $(d)/tty.c $(d)/irq.c \
             $(d)/pic.c $(d)/isr.c $(d)/timer.c \
             $(d)/fault.c

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
