MODULES += $(dir)

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

ASRCS_$(d)      := $(d)/standup.s $(d)/bootstrap.s

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
