MODULES += $(dir)

sp := $(sp).x
dirstack_$(sp) := $(d)
d  := $(dir)


dir := $(d)/acpi
include $(dir)/module.mk
dir := $(d)/ahci
include $(dir)/module.mk


d := $(dirstack_$(sp))
sp := $(basename $(sp))
