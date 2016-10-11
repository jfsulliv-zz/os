bits 64

%define OFFS_RDI  0    ; UNUSED
%define OFFS_RSI  0x8  ; UNUSED
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


; arg0: Register with the address of the block to save the regs to.
%macro SAVE_REGS 1
        mov qword [%1 + OFFS_RDX], rdx
        mov qword [%1 + OFFS_RCX], rcx
        mov qword [%1 + OFFS_R8], r8
        mov qword [%1 + OFFS_R9], r9
        mov qword [%1 + OFFS_RAX], rax
        mov qword [%1 + OFFS_RBX], rbx
        mov qword [%1 + OFFS_RBP], rbp
        mov qword [%1 + OFFS_R10], r10
        mov qword [%1 + OFFS_R11], r11
        mov qword [%1 + OFFS_R12], r12
        mov qword [%1 + OFFS_R13], r13
        mov qword [%1 + OFFS_R14], r14
        mov qword [%1 + OFFS_R15], r15
        mov qword [%1 + OFFS_DS], ds
        mov qword [%1 + OFFS_ES], es
        mov qword [%1 + OFFS_FS], fs
        mov qword [%1 + OFFS_GS], gs
        mov qword [%1 + OFFS_RSP], rsp
        pushfq
        pop qword [%1 + OFFS_FLG]
%endmacro

; arg0 : Register which holds the address to load from.
%macro LOAD_REGS 1
        mov rdx, qword [%1 + OFFS_RDX]
        mov rcx, qword [%1 + OFFS_RCX]
        mov r8, qword [%1 + OFFS_R8]
        mov r9, qword [%1 + OFFS_R9]
        mov rax, qword [%1 + OFFS_RAX]
        mov rbx, qword [%1 + OFFS_RBX]
        mov rbp, qword [%1 + OFFS_RBP]
        mov r10, qword [%1 + OFFS_R10]
        mov r11, qword [%1 + OFFS_R11]
        mov r12, qword [%1 + OFFS_R12]
        mov r13, qword [%1 + OFFS_R13]
        mov r14, qword [%1 + OFFS_R14]
        mov r15, qword [%1 + OFFS_R15]
        mov ds, qword [%1 + OFFS_DS]
        mov es, qword [%1 + OFFS_ES]
        mov fs, qword [%1 + OFFS_FS]
        mov gs, qword [%1 + OFFS_GS]
        mov rsp, qword [%1 + OFFS_RSP]
        push qword [%1 + OFFS_FLG]
        popfq
%endmacro

; Swap register contexts.
; rdi : struct regs *saveregs
; rsi : struct regs *newregs
global context_switch
context_switch:
        push rbp
        mov rbp, rsp
        SAVE_REGS rdi
        ; Things are kind of weird here. we're technically still the old
        ; process but our register context is the new one's. Since the
        ; IP is the same for both, we've functionally performed a full
        ; switch at this point, and we can consider the old process to
        ; be 'paused' here.
        LOAD_REGS rsi
        pop rbp
        retq

; Jump to userspace, dropping into ring 3. Does not return!
; rdi : void *call_addr
; rsi : void *user_stack_top
global jump_to_userspace
jump_to_userspace:
        push qword 3 | (0x8 * 4) ; User SS
        push qword rsi ; User stack
        push qword 3 | (0x8 * 3) ; User CS
        push qword rdi ; User code
        retf
