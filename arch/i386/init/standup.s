%include "machine/params.asm"

extern main
extern kernel_start
extern kernel_end

%define RELOC(x)        ((x)-(KERN_BASE))

%macro PAGE_ROUND 1
        mov ecx, %1
        and ecx, 0x0fff
        jz  .page_round_done
        mov ecx, %1
        add ecx, 0x1000
        and ecx, ~0x0fff
        jmp .page_round_out
.page_round_done:
        mov ecx, %1
.page_round_out:
%endmacro

%define SIZE_IN_PAGES(x) ((x) >> (12))

; Declare constants used for creating a multiboot header.
MBALIGN     equ  1<<0                   ; align loaded modules on page boundaries
MEMINFO     equ  1<<1                   ; provide memory map
FLAGS       equ  MBALIGN | MEMINFO      ; this is the Multiboot 'flag' field
MAGIC       equ  0x1BADB002             ; 'magic number' lets bootloader find the header
CHECKSUM    equ -(MAGIC + FLAGS)        ; checksum of above, to prove we are multiboot
 
; Declare a header as in the Multiboot Standard. We put this into a
; special section so we can force the header to be in the start of the
; final program.  You don't need to understand all these details as it
; is just magic values that is documented in the multiboot standard. The
; bootloader will search for this magic sequence and recognize us as a
; multiboot kernel.
section .multiboot
align 4
        dd MAGIC
        dd FLAGS
        dd CHECKSUM
; The kernel address space corresponds roughly to its ELF layout.
;
; [tables] | text | data | bss [stack]
;                   ^
;           PAGE_ROUND(kernel_end)
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
        add eax, 0x000000
        mov ecx, 0x0
        mov edx, RELOC(kernel_end)
.2:     stosd
        add eax, 0x1000
        cmp edi, RELOC(pg0_end)
        sub edx, 0x1000
        jns  $-0x12

        ; Set the page directory pointer
        mov ecx, RELOC(proc0_pdir)
        mov cr3, ecx

        ; Enable paging
        mov ecx, cr0
        or  ecx, 0x80000000
        mov cr0, ecx ; Paging is on!
        
        ;;; Jump into the higher half
        lea ecx, [_start_hh]
        jmp ecx

_start_hh:
        ;;; First of all, we don't need the ID map any more, so
        ;;; let's get rid of it. 
        mov dword [RELOC(proc0_pdir)],   0x0
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

global gdt_flush
gdt_flush:
        mov eax, [esp+4]
        lgdt [eax]
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
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
global proc0_pdir
global pg0
proc0_pdir:
        ; The first 4MB will be ID mapped.
        dd 0x00102007
        times (UNUM_PAGETABS-1) dd 0x0
        ; Duplicate the kern map at the start of the kernel address space
        dd 0x00102007
        times (KNUM_PAGETABS-1) dd 0x0
proc0_pdir_end:
; These will be dynamically filled for the identity mapping
pg0:
        times 4096 db 0x0
pg0_end:
; We will use this for the boot parameters and kernel command line.
empty_zero_page:
        times 4096 db 0x0

section .bss
align 32
STACK_BASE:
resb STACK_SZ
STACK_TOP:
