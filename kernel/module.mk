MODULES += $(dir)

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

dir := $(d)/mm
include $(dir)/module.mk
dir := $(d)/sched
include $(dir)/module.mk
dir := $(d)/syscall
include $(dir)/module.mk
dir := $(d)/util
include $(dir)/module.mk

SRCS_$(d)       := $(d)/main.c $(d)/kprintf.c $(d)/ksyms.c \
                   $(d)/multiboot.c $(d)/proc.c $(d)/asm_exports.c \
                   $(d)/sysinit.c $(d)/timer.c $(d)/percpu.c

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
