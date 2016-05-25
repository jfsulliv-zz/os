%include "machine/params.asm"
bits 64

%define KRELOC(x)       ((x)-(KERN_BASE))
%define SIZE_IN_PAGES(x) ((x) >> (12))

extern main

section .text

global _start_hh
_start_hh:
        mov ax, 0x10
        mov ds, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
        mov es, ax
        cli
        ;;; Bootstrap the stack.
        mov rsp, STACK_TOP
        add rsp, 0x8
        ; Map up the GDT to its logical address (boostrap.s placed it
        ; at a 32-bit physical address which was identity mapped, and
        ; we want to remove that ID mapping).
        mov rax, GDT64.Pointer
        lgdt [rax]
        mov ax, 0x10
        mov ds, ax
        mov ss, ax
        mov es, ax
        ; Remove the ID mapping
        mov rax, init_pgd
        mov qword [rax], 0x0
        mov rax, init_pud
        mov qword [rax], 0x0
        invlpg [0]
        ; Remove the GDT mapping
        mov rax, init_pud_gdtmap
        mov qword [rax], 0x0
        invlpg [0x8000000]
        ; Retrieve the multiboot header
        mov rdi, [mbhp]
        add rdi, KERN_BASE ; Adjust for new paging scheme
        ;;; Now we can jump into the C main 
        mov rax, main
        call rax
        ;;; Well that shouldn't have happened...
.fail:
        jmp $
        hlt

global gdt_flush
gdt_flush:
        lgdt [rdi]
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        ret

global idt_flush
idt_flush:
        lidt [rdi]
        ret

global tss_flush
tss_flush:
        sal di, 3 ; each GDTE is 8 bytes
        ltr di
        ret

section .bss
align 32
STACK_BASE:
resb STACK_SZ
global STACK_TOP
STACK_TOP:

section .tables
align 0x1000
global init_pgd
global init_pgd_kmap
global init_pud
global init_pud_gdtmap
global init_pud_kmap
global init_pmd
global init_pte
init_pgd:
        ; Point this to init_put
        times (UNUM_PUDS) dq 0x0
init_pgd_kmap:
        ; And do the same here
        times (KNUM_PUDS) dq 0x0
init_pgd_end:
init_pud:
        times 2 dq 0x0
        ; Point this to init_pmd
init_pud_gdtmap:
        times (UNUM_PMDS-2) dq 0x0        
        ; Point this to init_pmd
init_pud_kmap:
        times (KNUM_PMDS) dq 0x0
init_pud_end:
init_pmd:
        ; Point this to init_pte
        times 512 dq 0
init_pmd_end:
; These will be dynamically filled for the identity mapping
init_pte:
        times 2048 dq 0x0
init_pte_end:

section .data
global GDT64
global GDT64.Pointer
global GDT64.Code
align 32
GDT64:
.Null:
    dq 0x0000000000000000       ; Null Descriptor
.Code:
    dq 0x00209A0000000000       ; 64-bit code descriptor
    dq 0x0000920000000000       ; 64-bit data descriptor
ALIGN 4
    dw 0                        ; Padding
.Pointer:
    dw 0x18                     ; 16-bit Limit
    dq GDT64                    ; 32-bit Base Address of GDT.

; Somewhere safe to keep our multiboot header info pointer
global mbhp
mbhp:
    dq 0x0
