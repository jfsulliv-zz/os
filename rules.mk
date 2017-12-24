BINUTILS=binutils-2.28
LIBGCC=gcc-6.4.0
LIBICONV=libiconv-1.14
VENDOR_DIR=$(shell pwd)/vendor
VENDOR_OUT=$(shell pwd)/build/$(ARCH)

CC=clang
STDINC_DIR=$(VENDOR_OUT)/$(LIBGCC)/gcc/include
$(warning $(STDINC_DIR))
WARNINGS := -Wall -Wextra -Werror=implicit-function-declaration
CFLAGS	  = -m$(BITS) -ggdb -std=c99 $(WARNINGS) -ffreestanding \
            --target=$(ARCH_TGT) -march=$(MARCH) \
            -fno-asynchronous-unwind-tables -nostdinc \
            -nostdlib -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
            $(patsubst %,-I%,$(INC)) $(MC) \
            -I $(STDINC_DIR)

ASM     = nasm
AFLAGS  =-felf$(BITS) $(patsubst %,-i%/,$(INC))

LD=$(VENDOR_OUT)/$(BINUTILS)/bin/$(ARCH)-elf-ld
LIBGCC_DIR=$(VENDOR_OUT)/$(LIBGCC)/gcc
LDFLAGS = -melf_$(ARCH_TC) -T arch/$(ARCH)/config/linker.ld \
          -L$(LIBGCC_DIR) -nostdlib -lgcc

dir := arch/$(ARCH)
include $(dir)/module.mk
dir := lib
include $(dir)/module.mk
dir := kernel
include $(dir)/module.mk
dir := drivers
include $(dir)/module.mk

define LOCAL_FLAGS
$(CFLAGS_$(patsubst %/,%,$(basename $(dir $(1)))))
endef

%.o: %.c
	$(CC) $(call LOCAL_FLAGS,$@) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(ASM) $(AF_TGT) $(AFLAGS) -o $@ $<
