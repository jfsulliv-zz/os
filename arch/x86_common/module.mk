MODULES += $(dir)

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

INC             +=$(d)/include

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
