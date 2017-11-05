%include "machine/params.asm"

; The bootloader drops us into **PROTECTED MODE** and so this is
; actually 32-bit code that bootstraps long mode.
bits 32

extern kernel_end
extern _start_hh

extern GDT64.Pointer

extern mbhp

extern init_pgd
extern init_pgd_kmap
extern init_pud_gdtmap
extern init_pud_kmap
extern init_pud
extern init_pmd
extern init_pte
extern init_pte_end

%define RELOC(x)        ((x)-(LOAD_OFFS))
%define KRELOC(x)       ((x)-(KERN_BASE))

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

;;;;;;;;;;;;;;;;;;;;;;
;;; Set up paging. ;;;
;;;;;;;;;;;;;;;;;;;;;;
; [ebp+8]: Highest physical address to map.
init_tables:
        push ebp
        mov ebp, esp
        push edi
        ; Initialize the page tables to be identity mapped
        ; [0x0 .. rdi]                           => 0x0 (For this code)
        ; [0x80000000 .. 0x80000000 + rdi]       => 0x0 (For the GDT)
        ; [kernel_start .. kernel_start + rdi]   => 0x0 (For the actual mapping)

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
        mov edx, [ebp+0x8]
        mov eax, 0x3
        mov ecx, 0x0
.2:     stosd
        add edi, 4
        add eax, 0x1000
        sub edx, 0x1000
        jns .2
        pop edi
        pop ebp
        ret

; Returns non-zero if CPUID is available.
check_cpuid:
        pushfd
        pushfd
        xor dword [esp],0x00200000
        popfd
        pushfd
        pop eax
        xor eax,[esp]
        popfd
        and eax,0x00200000
        ret

; Returns non-zero if the CPU features we rely on are supported.
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
        bt edx, 11 ; syscall/sysret supported?
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

; Returns the highest physical address at which data is stored (either by the
; kernel or by Multiboot).
; [ebp+8]: Physical address of the Multiboot header
get_phys_top:
        push ebp
        mov ebp, esp
        push edi
        push esi
        push ebx
        ; Start with the kernel's data
        mov eax, KRELOC(kernel_end)
        mov edi, [ebp+8]
        mov edx, edi
        add edx, 116
        ; Add in the multiboot info header
        cmp edx, eax
        cmovg eax, edx
        ; Check to see if we also need to account for ELF data
        mov edx, [edi]
        bt edx, 5
        jb .preserve_elf_data
        jmp .out
.preserve_elf_data:
        ; Add in the shtab itself
        mov edx, [edi+36] ; shtab addr
        add edx, [edi+32] ; shtab size
        cmp edx, eax
        cmovg eax, edx
        ; Scan for strtabs sections and include them, too
        mov ecx, [edi+28] ; shtab num
        mov edi, [edi+36]
.loop:
        cmp ecx, 0
        jz .out
        sub ecx, 1
        mov edx, ecx
        sal edx, 0x6
        add edx, edi ; &shdrs[i]
        ; Skip non-strtab entries
        mov esi, [edx+0x4] ; sh_type
        cmp esi, 0x3 ; SH_STRTAB
        jne .loop
        mov esi, [edx+0x18] ; sh_offset
        add esi, [edx+0x20] ; sh_size
        cmp esi, eax
        cmovg eax, esi
        jmp .loop
.out:
        pop ebx
        pop esi
        pop edi
        pop ebp
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

        ; Initialize the memory map of the system.
        mov ebx, [KRELOC(mbhp)]
        push ebx
        call get_phys_top
        mov [esp], eax
        call init_tables
        add esp, 4

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

