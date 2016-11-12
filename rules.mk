CC=clang
WARNINGS := -Wall -Wextra -Werror=implicit-function-declaration
CFLAGS	  = -m$(BITS) -ggdb -std=c99 $(WARNINGS) -ffreestanding \
            -fno-builtin $(patsubst %,-I%,$(INC)) $(MC) \
            --target=$(ARCH_TGT) -march=$(MARCH) \
            -fno-asynchronous-unwind-tables -nostdinc++ \
            -mno-red-zone -mno-mmx -mno-sse -mno-sse2

ASM     = nasm
AFLAGS  =-felf$(BITS) $(patsubst %,-i%/,$(INC))

LIBGCC=$(shell $(ARCH)-elf-gcc -print-libgcc-file-name)
LIBGCC_DIR=$(shell dirname $(LIBGCC))

LD=$(ARCH)-elf-ld
LDFLAGS = -melf_$(ARCH_TC) -T arch/$(ARCH)/build/linker.ld \
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
