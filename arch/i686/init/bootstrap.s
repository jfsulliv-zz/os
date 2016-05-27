%include "machine/params.asm"

; The bootloader drops us into **PROTECTED MODE** and so this is
; actually 32-bit code that bootstraps long mode.
bits 32

extern kernel_sz
extern kernel_end
extern KERNEL_VMA
extern _start_hh

extern GDT64
extern GDT64.Pointer
extern GDT64.Code

extern mbhp

extern init_pgd
extern init_pgd_kmap
extern init_pud_gdtmap
extern init_pud_kmap
extern init_pud
extern init_pmd
extern init_pte
extern init_pte_end

extern STACK_TOP

%define RELOC(x)        ((x)-(LOAD_OFFS))
%define KRELOC(x)       ((x)-(KERN_BASE))

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
; text | data | bss [stack] | [tables]
;
section .text

;;;;;;;;;;;;;;;;;;;;;;
;;; Set up paging. ;;;
;;;;;;;;;;;;;;;;;;;;;;
init_tables:
        ; Initialize the page tables to be identity mapped
        ; [0x0 .. KERN_SZ]        => 0x0 (For this code)
        ; [0x80000000 .. KERN_SZ] => 0x0 (For the GDT)
        ; [KERN_BASE .. KERN_SZ]  => 0x0 (For the actual mapping)

        ; PGD setup
        mov edi, KRELOC(init_pgd)
        mov eax, KRELOC(init_pud)
        or eax, 0x3 ; Present, RW
        mov [edi], eax
        mov edi, KRELOC(init_pgd_kmap)
        mov [edi], eax

        ; PUD setup
        mov edi, KRELOC(init_pud)
        mov eax, KRELOC(init_pmd)
        or eax, 0x3
        mov [edi], eax
        mov edi, KRELOC(init_pud_kmap)
        mov [edi], eax
        mov edi, KRELOC(init_pud_gdtmap)
        mov [edi], eax

        ; PMD setup
        mov edi, KRELOC(init_pmd)
        mov eax, KRELOC(init_pte)
        or eax, 0x3
        mov ecx, 4
.1:     mov [edi], eax
        add eax, 0x1000
        add edi, 0x8
        sub ecx, 1
        cmp ecx, 0
        jg .1

        ; PTE setup
        mov edi, KRELOC(init_pte)
        mov edx, KRELOC(kernel_end)
        mov eax, 0x3
        mov ecx, 0x0
.2:     stosd
        add edi, 4
        add eax, 0x1000
        sub edx, 0x1000
        jns .2
        ret

check_cpuid:
        pushfd                               ;
        pushfd                               ;
        xor dword [esp],0x00200000           ;
        popfd                                ;
        pushfd                               ;
        pop eax                              ;
        xor eax,[esp]                        ;
        popfd                                ;
        and eax,0x00200000                   ;
        ret

check_features:
        ; First make sure we have extended CPUID features
        mov eax, 0x80000000
        cpuid
        cmp eax, 0x80000001
        jb .fail
        ; Now check said features
        mov eax, 0x80000001
        cpuid
        bt edx, 29 ;64-bit enabled?
        jnb .fail
        bt edx, 5 ;PAE enabled?
        jnb .fail
        mov eax, 0x1
        cpuid
        bt edx, 25 ; SSE supported?
        jnb .fail
        mov eax, 1
        ret
.fail:
        mov eax, 0
        ret

global _start
_start:
        cli

        ; First, save our Multiboot info header pointer somewhere safe,
        ; so we don't lose it when we jump to long mode.
        test ebx, ebx
        jz .fail
        mov [KRELOC(mbhp)], ebx

        call check_cpuid
        cmp eax, 0
        je .fail

        call check_features
        cmp eax, 0
        je .fail

        call init_tables

        ; Enable PAE
        mov ecx, cr4
        or  ecx, (1<<5)
        mov cr4, ecx
        
        ; Set the page directory pointer
        mov ecx, KRELOC(init_pgd)
        mov cr3, ecx

        ; Enter long mode
        mov ecx, 0xC0000080
        rdmsr
        or eax, (1 << 8)
        wrmsr

        ; Enable SSE
        mov ecx, cr0
        and cx, 0xfffb ; Clear CR0.EM (Coprocessor emulation)
        or  cx, 0x2    ; Set   CR0.MP (Coprocessor monitoring)
        mov cr0, ecx

        mov ecx, cr4
        or  cx,  (3<<9) ; Set CR4.OSFXR, CR4.OSXMMEXCPT
        mov cr4, ecx

        ; Enable paging
        mov ecx, cr0
        or  ecx, (1 << 31)
        or  ecx, (1<<16) ; Set WP bit
        mov cr0, ecx ; Paging is on!
        
        ;;; Jump into the higher half
        lgdt [KRELOC(GDT64.Pointer)]
        jmp 0x8:KRELOC(_start_hh)
.fail:
        mov edi, 0xb8000
        mov eax, 0x1F58
        mov [edi], eax
        jmp $

