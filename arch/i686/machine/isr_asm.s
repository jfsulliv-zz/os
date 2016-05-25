extern isr_handler
extern irq_handler
extern pagefault_handler

isr_common_stub:
        mov  rsi, [rsp]
        call isr_handler
        add rsp, 0x8            ; Get rid of the error number
        iretq                   ; Pop the CS, EIP, EFLAGS, SS, ESP

; Create a stub for an ISR which does not push its own error code.
%macro ISR_NOERRORCODE 1
global isr%1
isr%1:
    mov  rdi, %1
    push qword 0x0
    jmp isr_common_stub 
%endmacro

; Create a stub for an ISR which pushes its own error code.
%macro ISR_ERRORCODE 1
global isr%1
isr%1:
    mov rdi, %1
    jmp isr_common_stub
%endmacro

; Create an IRQ stub. Argument 1 is the IRQ number, 2 is the interrupt
; it is mapped to.
%macro IRQ 2
global irq%1
irq%1:
    mov rdi, %2
    push qword 0x0
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

; Give page faults their own stub for speediness
global isr14
isr14:
    mov rdi, 14
    mov rsi, [rsp]
    call pagefault_handler
    add rsp, 8
    iretq
 
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

