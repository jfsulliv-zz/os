CC=$(ARCH)-elf-gcc
WARNINGS := -Wall -Wextra
CFLAGS	  = -m$(BITS) -ggdb -std=c99 $(WARNINGS) -lgcc -ffreestanding \
            -fno-builtin $(patsubst %,-I%,$(INC)) $(MC)

ASM     = nasm
AFLAGS  =-felf$(BITS) $(patsubst %,-i%/,$(INC))

LD      =$(ARCH)-elf-ld
LDFLAGS = -melf_$(ARCH_TC) -nostdlib -T arch/$(ARCH)/build/linker.ld

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
