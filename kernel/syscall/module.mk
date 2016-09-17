MODULES += $(dir)

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

SRCS_$(d)       := $(d)/syscall_table.c $(d)/sys_fork.c $(d)/sys_exit.c \

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
