ifeq ($(CONFIG_ARCH), 2)
        BITS=32
        ARCH=i386
        ARCH_TC=x86
endif

ifeq ($(CONFIG_ARCH), 1)
        BITS=64
        ARCH=i686
        ARCH_TC=x86_64
endif

# default architecture
ARCH ?= i386
BITS ?= 32

TARGET=$(ARCH)-elf
NAME=os
VERSION=0.1
OUT=kernel-$(VERSION)
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
ALL_OBJS+=$(OBJS_$(1)) $(AOBJS_$(1))
endef

$(foreach p,$(MODULES),$(eval $(call OBJDEF,$(p))))

.PHONY: all test img clean clean_cscope dist todolist cscope

all: img $(GRUBCFG) 
	-cp $(OUT) $(ISODIR)/boot/$(OUT)
	grub-mkrescue -o $(ISO) $(ISODIR) -d /usr/lib/grub/$(ARCH)-pc

test: CFLAGS += -DRUN_TESTS
test: all

$(GRUBCFG): 
	echo "menuentry \"my-os\" {" > $(GRUBCFG)
	echo -e "\t multiboot /boot/"$(OUT) >> $(GRUBCFG)
	echo "}" >> $(GRUBCFG)

img: include/version.h ksyms.o $(ALL_OBJS)
	$(LD) $(LDFLAGS) ksyms.o $(ALL_OBJS) -o $(OUT)

ksyms.o: ksyms.bin
	objcopy --rename-section .data=.rodata -B $(ARCH) -O elf$(BITS)-$(ARCH) -I binary ksyms.bin $@

obj_list.txt: $(ALL_OBJS)
	printf "%s\n" $(ALL_OBJS) > $@

ksyms.bin: obj_list.txt
	scripts/gen_syms.sh obj_list.txt > $@

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
	-$(RM) $(wildcard $(ALL_OBJS))
	-$(RM) $(wildcard $(GRUBCFG) $(ISO) $(OUT) $(OUT).tgz) \
		$(ISODIR)/boot/$(OUT)
