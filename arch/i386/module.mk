MODULES += $(dir)

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

CFLAGS_$(d)     :=
AFLAGS_$(d)     :=
INC             +=$(d)/include

dir := $(d)/../x86_common
include $(dir)/module.mk

root:=$(d)

dir := $(d)/init
include $(dir)/module.mk
dir := $(d)/machine
include $(dir)/module.mk
dir := $(d)/mm
include $(dir)/module.mk

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
