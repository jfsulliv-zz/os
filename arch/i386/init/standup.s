%include "machine/params.asm"

extern main
extern kernel_start
extern kernel_end

%define RELOC(x)        ((x)-(KERN_BASE))
%define SIZE_IN_PAGES(x) ((x) >> (12))

; Constants associated with the multiboot spec.
MB_MBALIGN     equ  1<<0    ; align loaded modules on page boundaries
MB_MEMINFO     equ  1<<1    ; provide memory map
MB_FLAGS       equ  MB_MBALIGN | MB_MEMINFO    ; this is the Multiboot 'flag' field
MB_MAGIC       equ  0x1BADB002    ; 'magic number' lets bootloader find the header
MB_CHECKSUM    equ -(MB_MAGIC + MB_FLAGS)    ; checksum of above, to prove we are multiboot
 
section .multiboot
align 4
        dd MB_MAGIC
        dd MB_FLAGS
        dd MB_CHECKSUM

; The kernel address space corresponds roughly to its ELF layout.
;
; text | data | bss [stack] | [tables]
;
section .text
global _start
_start:
        ;;;;;;;;;;;;;;;;;;;;;;
        ;;; Set up paging. ;;;
        ;;;;;;;;;;;;;;;;;;;;;;

        ; Initialize the page tables to be identity mapped
        mov edi, RELOC(pg0)
        mov eax, 0x7 ; Present, RW, user
.2:     stosd
        add eax, 0x1000
        cmp edi, RELOC(pg0_end)
        jl .2

        ; Set the page directory pointer
        mov ecx, RELOC(init_pgd)
        mov cr3, ecx

        ; Enable paging
        mov ecx, cr0
        or  ecx, 0x80000000
        or  ecx, (1<<16) ; Set WP bit
        mov cr0, ecx ; Paging is on!
        
        ;;; Jump into the higher half
        lea ecx, [_start_hh]
        jmp ecx

_start_hh:
        ;;; First of all, we don't need the ID map any more, so
        ;;; let's get rid of it. 
        mov ecx, 2
        mov edi, RELOC(init_pgd)
        xor eax, eax
        rep stosd
        invlpg [0x0]
        ;;; Bootstrap the stack.
        mov esp, STACK_TOP
        ;;; Push the boot header information.
        add ebx, KERN_BASE
        push ebx
        ;;; Now we can jump into the C main 
        call main
        ;;; Well that shouldn't have happened...
        hlt

; esp+4: Address of GDTP
; esp+8: Index for GS
global gdt_flush
gdt_flush:
        mov eax, [esp+4]
        lgdt [eax]
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov ss, ax
        mov eax, [esp+8]
        sal ax, 3
        mov gs, ax
        jmp 0x08:.flush
.flush:
        ret

global idt_flush
idt_flush:
        mov eax, [esp+4]
        lidt [eax]
        ret

global tss_flush
tss_flush:
        mov ax, [esp+4]
        sal ax, 3 ; each GDTE is 8 bytes
        ltr ax
        ret

section .tables
align 0x1000
global init_pgd
init_pgd:
        ; The first 8MB will be ID mapped.
        dd (RELOC(pg0) + 0x0007)
        dd (RELOC(pg0) + 0x1007)
        times (UNUM_PAGETABS-2) dd 0x0
        ; Duplicate the kern map at the start of the kernel address space
        dd (RELOC(pg0) + 0x0007)
        dd (RELOC(pg0) + 0x1007)
        times (KNUM_PAGETABS-2) dd 0x0
init_pgd_end:

global pg0
; These will be dynamically filled for the identity mapping
pg0:
        times (2 * 4096) db 0x0
pg0_end:

section .bss
align 32
STACK_BASE:
resb STACK_SZ
global STACK_TOP
STACK_TOP:
