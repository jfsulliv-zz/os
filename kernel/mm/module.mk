MODULES += $(dir)

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

SRCS_$(d) := $(d)/pfa.c $(d)/vma_slab.c $(d)/vmman.c

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
