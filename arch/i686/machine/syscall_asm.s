bits 64

%include "arch/i686/machine/regs_asm.s"

extern syscall_entry

; Skips gs, rax, and rcx
%macro SAVE_REGS_FOR_SYSCALL 0
        mov qword [rcx + OFFS_RDI], rdi
        mov qword [rcx + OFFS_RSI], rsi
        mov qword [rcx + OFFS_RDX], rdx
        mov qword [rcx + OFFS_R8], r8
        mov qword [rcx + OFFS_R9], r9
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

; Skips gs, rax, and rcx
%macro LOAD_REGS_FOR_SYSCALL 0
        mov rdi, qword [rcx + OFFS_RDI]
        mov rsi, qword [rcx + OFFS_RSI]
        mov rdx, qword [rcx + OFFS_RDX]
        mov r8, qword [rcx + OFFS_R8]
        mov r9, qword [rcx + OFFS_R9]
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

; Starting point for long-mode syscalls.
; Responsible for:
;  * loading the kernel stack
;  * saving user registers
;  * transferring to C handler
; Expected layout:
;   rax = syscall #
;   rcx = user RIP
;   r11 = user RFLAGS
;   rdi = arg0
;   rsi = arg1
;   rdx = arg2
;   r10 = arg3
;   r8  = arg4
;   r9  = arg5
global syscall_entry_stub
syscall_entry_stub:
        ; First order of business is to load the kernel state and save
        ; the user state. GS base has the per CPU block, which holds
        ; the kernel stack and a scratch register for the user stack.
        swapgs
        ; Save user RCX into a scratch variable in the per-cpu block
        ; offsetof(struct cpu, arch_cpu)
        ;   + offsetof(struct arch_cpu, scratch)
        mov [gs:(0x18 + 0x0)], rcx
        ; Save all of the user registers, except RCX
        ; Conveniently, user regs are at start of `struct proc'
        mov rcx, [gs:(0x8)] ; curproc
        SAVE_REGS_FOR_SYSCALL
        mov [rcx + OFFS_RAX], rax
        ; Load the kernel stack
        ; offsetof(struct cpu, kstack)
        mov rsp, [gs:0x10]
        xor rbp, rbp
        call syscall_entry
        ; Restore user state
        mov rcx, [gs:(0x8)]
        LOAD_REGS_FOR_SYSCALL
        mov rcx, [gs:(0x18 + 0x0)]
        swapgs
        ; Back to reality
        o64 sysret

; Swap register contexts.
; rdi : struct regs *saveregs
; rsi : struct regs *newregs
global context_switch
context_switch:
        push rbp
        mov rbp, rsp
        mov [rdi + OFFS_RCX], rcx
        xchg rcx, rdi
        SAVE_REGS_TO_RCX
        mov qword [rcx + OFFS_GS], gs
        pushfq
        pop qword [rcx + OFFS_FLG]
        ; Things are kind of weird here. we're technically still the old
        ; process but our register context is the new one's. Since the
        ; IP is the same for both, we've functionally performed a full
        ; switch at this point, and we can consider the old process to
        ; be 'paused' here.
        LOAD_REGS_FROM_RCX
        mov gs, qword [rcx + OFFS_GS]
        push qword [rcx + OFFS_FLG]
        popfq
        mov rcx, [rcx + OFFS_RCX]
        pop rbp
        ret

; Jump to userspace, dropping into ring 3. Does not return!
; rdi : void *call_addr
; rsi : void *user_stack_top
global jump_to_userspace
jump_to_userspace:
        mov rcx, rdi
        mov rsp, rsi
        xor r11, r11
        o64 sysret
