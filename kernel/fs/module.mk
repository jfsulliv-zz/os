MODULES += $(dir)

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

dir := $(d)/fat
include $(dir)/module.mk

SRCS_$(d)       := $(d)/vfs.c

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
