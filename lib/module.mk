MODULES += $(dir)

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

SRCS_$(d)       := $(d)/stdio.c $(d)/string.c $(d)/lookup3.c \
                   $(d)/strtol.c $(d)/strtoul.c $(d)/ctype.c

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
