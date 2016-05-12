extern isr_handler
extern irq_handler
extern pagefault_handler

isr_common_stub:
        pushad                  ; Push limited register state
        push ds
        push es
        push fs
        push gs
        mov ax, 0x10            ; Kernel DS selector
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov eax, esp
        push eax                ; Push the stack pointer
        call isr_handler
        pop eax                 ; Reload the original DS
        pop gs
        pop fs
        pop es
        pop ds
        popad
        add esp, 8
        iret                    ; Pop the CS, EIP, EFLAGS, SS, ESP

; Create a stub for an ISR which does not push its own error code.
%macro ISR_NOERRORCODE 1
global isr%1
isr%1:
    push byte 0
    push byte %1
    jmp isr_common_stub 
%endmacro

; Create a stub for an ISR which pushes its own error code.
%macro ISR_ERRORCODE 1
global isr%1
isr%1:
    push byte %1
    jmp isr_common_stub
%endmacro

; Create an IRQ stub. Argument 1 is the IRQ number, 2 is the interrupt
; it is mapped to.
%macro IRQ 2
global irq%1
irq%1:
    push byte 0
    push byte %2
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

; Page faults get their own handler for efficiency.
global isr14
isr14:
        push byte 14            ; #PF
        pushad                  ; Push limited register state
        push ds
        push es
        push fs
        push gs
        mov ax, 0x10            ; Kernel DS selector
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov eax, esp
        push eax                ; Push the stack pointer
        call pagefault_handler
        pop eax                 ; Reload the original DS
        pop gs
        pop fs
        pop es
        pop ds
        popad
        add esp, 8
        iret                    ; Pop the CS, EIP, EFLAGS, SS, ESP

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

