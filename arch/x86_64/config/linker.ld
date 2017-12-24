OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)
KERNEL_MAP = 0xffffffff80000000;
KERNEL_VMA = KERNEL_MAP;

SECTIONS
{ 
  . = (2M) + SIZEOF_HEADERS;

  .multiboot . : {
      *bootstrap.o(.multiboot)
  }

  .bootstrap : {
      *bootstrap.o(.text)
  }

  . += (KERNEL_MAP);
  kernel_start = KERNEL_MAP;

  .text ALIGN(4096) : AT(ADDR(.text) - KERNEL_MAP) {
      *(EXCLUDE_FILE(*bootstrap.o) .text)
  }

  .data ALIGN (4096) : AT(ADDR(.data) - KERNEL_MAP) {
      *(.data)
      *(.tables)
  }

  .rodata ALIGN (4096) : AT(ADDR(.rodata) - KERNEL_MAP) {
      *(.rodata*)
  }

  .bss ALIGN (4096) : AT(ADDR(.bss) - KERNEL_MAP) {
      sbss = .;
      *(.bss)
      *(COMMON)
      ebss = .;
  }
  . = ALIGN(16);
  kernel_end = .;
  kernel_sz = kernel_end - kernel_start;
}
