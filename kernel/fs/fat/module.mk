MODULES += $(dir)

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

SRCS_$(d)       := $(d)/fat_utils.c $(d)/fat.c $(d)/fat_volinit.c \
		   $(d)/fat_dir.c $(d)/fat_impl.c

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
