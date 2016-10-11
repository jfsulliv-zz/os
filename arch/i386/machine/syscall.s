extern syscalls
extern SYSENT_FUN
extern SYSENT_NUMARGS
extern ASM_SYS_MAXNR
extern ASM_SYS_MAXARGS
extern ERRNO_ENOSYS

; This is where syscalls start their life.
; The expected state at this point is as follows:
;
; ESP = our stack (set by cpu)
; EBP = user stack (set by user)
; EAX = syscall number (set by user)
; EBX, ECX, EDX, EDI, ESI = Args 0..4 (5+ on stack)
; 
; User stack:
; EBP+0  userEBP
; EBP+4  userEDX
; EBP+8  userECX
; EBP+12 arg5
;
; The user stack will be in this state after exit. The return value
; is in EAX.
; 
; 
global syscall_entry_stub
syscall_entry_stub:
        push eax 
        ; EAX contains the syscall number
        cmp eax, ASM_SYS_MAXNR
        jl .prepare_args
        mov eax, ERRNO_ENOSYS
        jmp .return
.prepare_args:
        lea eax, [syscalls + eax*4]
        mov eax, [eax + SYSENT_NUMARGS]
        cmp eax, ASM_SYS_MAXARGS
        jl .load_args
        mov eax, ERRNO_ENOSYS
        jmp .return
.load_args:
        push eax
        ; Cute trick here: use a jump table to avoid having to do a
        ; bunch of conditional branches to load the valid registers.
        neg eax
        add eax, ASM_SYS_MAXARGS
        lea eax, [.arg_load_table + eax*2]
        jmp eax
.arg_load_table:
        push esi
        push edi
        push edx
        push ecx
        push ebx
        jmp .do_call
.do_call:
        lea eax, [syscalls + eax*4] 
        mov eax, [eax + SYSENT_FUN]
        call eax
.arg_unload:
        mov ebx, eax
        lea eax, [syscalls + eax*4]
        mov eax, [eax + SYSENT_NUMARGS] 
        sal eax, 2
        add esp, eax
        mov eax, ebx
.return:
        add esp, 0x4
        sysexit

section .bss
align 4096
SYSCALL_STACK_BASE: resb 4096 ; TODO make this per-CPU.
global SYSCALL_STACK_BASE_TOP
SYSCALL_STACK_BASE_TOP:
