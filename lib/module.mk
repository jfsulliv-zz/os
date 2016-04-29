MODULES += $(dir)

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

SRCS_$(d)       := $(d)/stdio.c $(d)/string.c

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
