MODULES += $(dir)

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

SRCS_$(d)      := $(d)/init.c $(d)/reserve.c $(d)/paging.c

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
