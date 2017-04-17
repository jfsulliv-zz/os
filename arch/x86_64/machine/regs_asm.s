bits 64

; Consistent with `struct regs' in regs.h
%define OFFS_RDI  0
%define OFFS_RSI  0x8
%define OFFS_RDX  0x10
%define OFFS_RCX  0x18
%define OFFS_R8   0x20
%define OFFS_R9   0x28
%define OFFS_RAX  0x30
%define OFFS_RBX  0x38
%define OFFS_RBP  0x40
%define OFFS_R10  0x48
%define OFFS_R11  0x50
%define OFFS_R12  0x58
%define OFFS_R13  0x60
%define OFFS_R14  0x68
%define OFFS_R15  0x70
%define OFFS_DS   0x78
%define OFFS_ES   0x80
%define OFFS_FS   0x88
%define OFFS_GS   0x90
%define OFFS_INT  0x98 ; UNUSED
%define OFFS_ERR  0xa0 ; UNUSED
%define OFFS_RIP  0xa8 ; UNUSED
%define OFFS_CS   0xb0 ; UNUSED
%define OFFS_FLG  0xb8
%define OFFS_RSP  0xc0
%define OFFS_SS   0xc8 ; UNUSED
%define REG_SAV_SZ 0xd0

; Skips gs and rcx
%macro SAVE_REGS_TO_RCX 0
        mov qword [rcx + OFFS_RDI], rdi
        mov qword [rcx + OFFS_RSI], rsi
        mov qword [rcx + OFFS_RDX], rdx
        mov qword [rcx + OFFS_R8], r8
        mov qword [rcx + OFFS_R9], r9
        mov qword [rcx + OFFS_RAX], rax
        mov qword [rcx + OFFS_RBX], rbx
        mov qword [rcx + OFFS_RBP], rbp
        mov qword [rcx + OFFS_R10], r10
        mov qword [rcx + OFFS_R11], r11
        mov qword [rcx + OFFS_R12], r12
        mov qword [rcx + OFFS_R13], r13
        mov qword [rcx + OFFS_R14], r14
        mov qword [rcx + OFFS_R15], r15
        mov qword [rcx + OFFS_DS], ds
        mov qword [rcx + OFFS_ES], es
        mov qword [rcx + OFFS_FS], fs
        mov qword [rcx + OFFS_RSP], rsp
%endmacro

; Skips gs and rcx
%macro LOAD_REGS_FROM_RCX 0
        mov rdi, qword [rcx + OFFS_RDI]
        mov rsi, qword [rcx + OFFS_RSI]
        mov rdx, qword [rcx + OFFS_RDX]
        mov r8, qword [rcx + OFFS_R8]
        mov r9, qword [rcx + OFFS_R9]
        mov rax, qword [rcx + OFFS_RAX]
        mov rbx, qword [rcx + OFFS_RBX]
        mov rbp, qword [rcx + OFFS_RBP]
        mov r10, qword [rcx + OFFS_R10]
        mov r11, qword [rcx + OFFS_R11]
        mov r12, qword [rcx + OFFS_R12]
        mov r13, qword [rcx + OFFS_R13]
        mov r14, qword [rcx + OFFS_R14]
        mov r15, qword [rcx + OFFS_R15]
        mov ds, qword [rcx + OFFS_DS]
        mov es, qword [rcx + OFFS_ES]
        mov fs, qword [rcx + OFFS_FS]
        mov rsp, qword [rcx + OFFS_RSP]
%endmacro
