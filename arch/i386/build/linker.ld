ENTRY (_start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
  . = 0xC0100000;
  kernel_start = .;

  .multiboot ALIGN (4096) : AT(ADDR(.multiboot) - 0xC0000000) {
    *(.multiboot)
  }

  .tables : AT(ADDR(.tables) - 0xC0000000) {
      *(.tables)
  }

  .text ALIGN (4096) : AT(ADDR(.text) - 0xC0000000) {
      *(.text)
  }

  .data ALIGN (4096) : AT(ADDR(.data) - 0xC0000000) {
      *(.data)
  }

  .rodata ALIGN (4096) : AT(ADDR(.rodata) - 0xC0000000) {
      *(.rodata*)
  }

  .bss : AT(ADDR(.bss) - 0xC0000000) {
      sbss = .;
      *(COMMON)
      *(.bss)
      ebss = .;
  }
  . = ALIGN(16);
  kernel_end = .;
}
