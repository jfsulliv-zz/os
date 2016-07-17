extern isr_handler
extern irq_handler
extern pagefault_handler

%define OFFS_XMM0 0
%define OFFS_XMM1 0x10
%define OFFS_XMM2 0x20
%define OFFS_XMM3 0x30
%define OFFS_XMM4 0x40
%define OFFS_XMM5 0x50
%define OFFS_XMM6 0x60
%define OFFS_XMM7 0x70
%define OFFS_R11  0x78
%define OFFS_R10  0x80
%define OFFS_R9   0x88
%define OFFS_R8   0x90
%define OFFS_RDX  0x98
%define OFFS_RCX  0xa0
%define OFFS_RAX  0xa8
%define OFFS_RDI  0xb0
%define OFFS_RSI  0xb8
%define REG_SAV_SZ 0xc0

%macro PUSH_REGS 0
        sub rsp, REG_SAV_SZ
        movaps    [rsp+OFFS_XMM0], xmm0
        movaps    [rsp+OFFS_XMM1], xmm1
        movaps    [rsp+OFFS_XMM2], xmm2
        movaps    [rsp+OFFS_XMM3], xmm3
        movaps    [rsp+OFFS_XMM4], xmm4
        movaps    [rsp+OFFS_XMM5], xmm5
        movaps    [rsp+OFFS_XMM6], xmm6
        movaps    [rsp+OFFS_XMM7], xmm7
        mov qword [rsp+OFFS_RSI],  rsi
        mov qword [rsp+OFFS_RDI],  rdi
        mov qword [rsp+OFFS_RAX],  rax
        mov qword [rsp+OFFS_RCX],  rcx
        mov qword [rsp+OFFS_RDX],  rdx
        mov qword [rsp+OFFS_R8],   r8
        mov qword [rsp+OFFS_R9],   r9
        mov qword [rsp+OFFS_R10],  r10
        mov qword [rsp+OFFS_R11],  r11
%endmacro

%macro POP_REGS 0
        movaps    xmm0, [rsp+OFFS_XMM0]
        movaps    xmm1, [rsp+OFFS_XMM1]
        movaps    xmm2, [rsp+OFFS_XMM2]
        movaps    xmm3, [rsp+OFFS_XMM3]
        movaps    xmm4, [rsp+OFFS_XMM4]
        movaps    xmm5, [rsp+OFFS_XMM5]
        movaps    xmm6, [rsp+OFFS_XMM6]
        movaps    xmm7, [rsp+OFFS_XMM7]
        mov qword rsi,  [rsp+OFFS_RSI]
        mov qword rdi,  [rsp+OFFS_RDI]
        mov qword rax,  [rsp+OFFS_RAX]
        mov qword rcx,  [rsp+OFFS_RCX]
        mov qword rdx,  [rsp+OFFS_RDX]
        mov qword r8,   [rsp+OFFS_R8]
        mov qword r9,   [rsp+OFFS_R9]
        mov qword r10,  [rsp+OFFS_R10]
        mov qword r11,  [rsp+OFFS_R11]
        add rsp, REG_SAV_SZ
%endmacro

; Assumes that the IRQ context is at the TOS, including an interrupt
; number (pushed by each stub) and an error code (pushed by stubs if not
; implicitly done by the interrupt).
isr_common_stub:
        sub rsp, 8 ; Align the stack
        PUSH_REGS
        lea rdi, [rsp+REG_SAV_SZ+8]
        call isr_handler
        POP_REGS
        add rsp, 0x18           ; Get rid of the int/err/alignment
        iretq                   ; Pop the CS, EIP, EFLAGS, SS, ESP

; Create a stub for an ISR which does not push its own error code.
%macro ISR_NOERRORCODE 1
global isr%1
isr%1:
    push qword 0x0
    push qword %1
    jmp isr_common_stub 
%endmacro

; Create a stub for an ISR which pushes its own error code.
%macro ISR_ERRORCODE 1
global isr%1
isr%1:
    push qword %1
    jmp isr_common_stub
%endmacro

; Create an IRQ stub. Argument 1 is the IRQ number, 2 is the interrupt
; it is mapped to.
%macro IRQ 2
global irq%1
irq%1:
    push qword 0x0
    push qword %2
    jmp isr_common_stub
%endmacro

ISR_NOERRORCODE 0
ISR_NOERRORCODE 1
ISR_NOERRORCODE 2
ISR_NOERRORCODE 3
ISR_NOERRORCODE 4
ISR_NOERRORCODE 5
ISR_NOERRORCODE 6
ISR_NOERRORCODE 7
ISR_ERRORCODE   8
ISR_NOERRORCODE 9
ISR_ERRORCODE   10
ISR_ERRORCODE   11
ISR_ERRORCODE   12
ISR_ERRORCODE   13
ISR_ERRORCODE   14
ISR_NOERRORCODE 15
ISR_NOERRORCODE 16
ISR_NOERRORCODE 17
ISR_NOERRORCODE 18
ISR_NOERRORCODE 19
ISR_NOERRORCODE 20
ISR_NOERRORCODE 21
ISR_NOERRORCODE 22
ISR_NOERRORCODE 23
ISR_NOERRORCODE 24
ISR_NOERRORCODE 25
ISR_NOERRORCODE 26
ISR_NOERRORCODE 27
ISR_NOERRORCODE 28
ISR_NOERRORCODE 29
ISR_NOERRORCODE 30
ISR_NOERRORCODE 31

IRQ 0,  32
IRQ 1,  33
IRQ 2,  34
IRQ 3,  35
IRQ 4,  36
IRQ 5,  37
IRQ 6,  38
IRQ 7,  39
IRQ 8,  40
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

