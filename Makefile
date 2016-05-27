ifeq ($(CONFIG_ARCH), i686)
        BITS=64
        ARCH=i686
        ARCH_TC=x86_64
        ARCH_TGT=i686-pc-none-elf
        MARCH=x86-64
        IMG_FMT=efi
        MC=-mcmodel=large
else
        BITS=32
        ARCH=i386
        ARCH_TC=i386
        ARCH_TGT=i386-pc-none-elf
        MARCH=i386
        IMG_FMT=pc
endif

# default architecture

TARGET=$(ARCH)-elf
NAME=os
VERSION=0.1
OUT=kernel
ISO=$(OUT).iso

GRUBCFG  := isodir/boot/grub/grub.cfg
ISODIR   := isodir

# We add to these lists as we go.
MODULES  :=
ALL_OBJS :=
INC      := include

default: all cscope
include rules.mk

define OBJDEF
$(eval OBJS_$(1)=$(patsubst %.c,%.o,$(SRCS_$(1))))
$(eval AOBJS_$(1)=$(patsubst %.s,%.o,$(ASRCS_$(1))))
$(eval POBJS_$(1)=$(patsubst %.s32,%.32o,$(PRESRCS_$(1))))
ALL_OBJS+=$(OBJS_$(1)) $(AOBJS_$(1))
PRE_OBJS+=$(POBJS_$(1))
endef

$(foreach p,$(MODULES),$(eval $(call OBJDEF,$(p))))

.PHONY: all test img clean clean_cscope dist todolist cscope

all: $(OUT)-$(VERSION) $(GRUBCFG) 
	-cp $(OUT)-$(VERSION) $(ISODIR)/boot/$(OUT)
	grub-mkrescue -o $(ISO) $(ISODIR)

test: CFLAGS += -DRUN_TESTS
test: all

$(OUT)-$(VERSION): include/version.h $(ALL_OBJS)
	$(LD) $(LDFLAGS) $(ALL_OBJS) -o $(OUT)-$(VERSION)

include/version.h:
	echo "#ifndef __VERSION_H__" >> $@
	echo "#define __VERSION_H__\n" >> $@
	echo "#define KERNEL_NAME \"$(NAME)\"" >> $@
	echo "#define KERNEL_VERS \"$(VERSION)\"\n" >> $@
	echo "#endif" >> $@

dist: 
	@tar -czf $(OUT).tar.gz $(ALLFILES)

todolist:
	-@for file in $(ALLFILES:Makefile=); do fgrep -H -e TODO -e \
	    FIXME -e XXX $$file; done; true

cscope.files: clean_cscope
	scripts/find_src.sh > $@

cscope: cscope.files
	cscope -kbq

clean_cscope:
	-$(RM) cscope.out cscope.files

clean: clean_cscope
	-$(RM) include/version.h ksyms.o ksyms.bin obj_list.txt
	-$(RM) $(wildcard $(ALL_OBJS) $(PRE_OBJS))
	-$(RM) $(wildcard $(ISO) $(OUT)-$(VERSION) \
               $(OUT)-$(VERSION)$(OUT).tgz $(ISODIR)/boot/$(OUT))
