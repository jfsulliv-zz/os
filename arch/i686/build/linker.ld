ENTRY (_start)
OUTPUT_FORMAT(elf32-i686)

SECTIONS
{
  . = 0xffffffff80000000;
  kernel_start = .;

  .multiboot ALIGN (4096) : AT(ADDR(.multiboot) - 0xffffffff80000000) {
    *(.multiboot)
  }

  .tables : AT(ADDR(.tables) - 0xffffffff80000000) {
      *(.tables)
  }

  .text ALIGN (4096) : AT(ADDR(.text) - 0xffffffff80000000) {
      *(.text)
  }

  .data ALIGN (4096) : AT(ADDR(.data) - 0xffffffff80000000) {
      *(.data)
  }

  .rodata ALIGN (4096) : AT(ADDR(.rodata) - 0xffffffff80000000) {
      *(.rodata*)
  }

  .bss : AT(ADDR(.bss) - 0xffffffff80000000) {
      sbss = .;
      *(COMMON)
      *(.bss)
      ebss = .;
  }
  . = ALIGN(16);
  kernel_end = .;
}
