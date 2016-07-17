CONFIG_ARCH ?= i386

ifeq ($(CONFIG_ARCH), i686)
        BITS=64
        ARCH=i686
        ARCH_TC=x86_64
        ARCH_TGT=x86_64-pc-eabi-elf
        MARCH=x86-64
        IMG_FMT=efi
        MC=-mcmodel=large
else ifeq ($(CONFIG_ARCH), i386)
        BITS=32
        ARCH=i386
        ARCH_TC=i386
        ARCH_TGT=i386-pc-none-elf
        MARCH=i386
        IMG_FMT=pc
else
$(error Unsupported architecture: $(CONFIG_ARCH))
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

.PHONY: all analyze test img clean clean_cscope dist todolist cscope

all: $(OUT)-$(VERSION) $(GRUBCFG) 
	-cp $(OUT)-$(VERSION) $(ISODIR)/boot/$(OUT)
	grub-mkrescue -o $(ISO) $(ISODIR)

# Static analysis checkers.
CHECKERS=\
    -enable-checker alpha.core.CastSize \
    -enable-checker alpha.core.IdenticalExpr \
    -enable-checker alpha.core.SizeofPtr \
    -enable-checker alpha.security.ArrayBoundV2 \
    -enable-checker alpha.security.MallocOverflow \
    -enable-checker alpha.security.ReturnPtrRange \
    -enable-checker alpha.unix.SimpleStream \
    -enable-checker alpha.unix.cstring.BufferOverlap \
    -enable-checker alpha.unix.cstring.NotNullTerminated \
    -enable-checker alpha.unix.cstring.OutOfBounds \
    -enable-checker security.insecureAPI.strcpy \
# XXX can clang implement a safer container_of macro? If not, we will
# get tons of analysis hits with this checker which we cannot do
# anything about.
#    -enable-checker alpha.core.CastToStruct \

analyze:
	scan-build --use-cc=$(CC) -analyze-headers -maxloop 8 $(CHECKERS) $(MAKE)

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
