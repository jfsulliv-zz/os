%define OFFS_GS   0
%define OFFS_FS   0x4
%define OFFS_ES   0x8
%define OFFS_DS   0xc
%define OFFS_EDI  0x10
%define OFFS_ESI  0x14
%define OFFS_EBP  0x18
%define OFFS_ESP  0x1c
%define OFFS_EBX  0x20
%define OFFS_EDX  0x24
%define OFFS_ECX  0x28
%define OFFS_EAX  0x2c
%define OFFS_INT  0x30
%define OFFS_ERR  0x34
%define OFFS_EIP  0x38
%define OFFS_CS   0x3c
%define OFFS_FLG  0x40
%define OFFS_SS   0x44
%define REG_SAV_SZ 0x48


; arg0: Register with the address of the block to save the regs to.
%macro SAVE_REGS 1
        mov [%1 + OFFS_GS], gs
        mov [%1 + OFFS_FS], fs
        mov [%1 + OFFS_ES], es
        mov [%1 + OFFS_DS], ds
        mov [%1 + OFFS_EDI], edi
        mov [%1 + OFFS_ESI], esi
        mov [%1 + OFFS_EBP], ebp
        mov [%1 + OFFS_ESP], esp
        mov [%1 + OFFS_EBX], ebx
        mov [%1 + OFFS_EDX], edx
        mov [%1 + OFFS_ECX], ecx
        mov [%1 + OFFS_EAX], eax
        mov [%1 + OFFS_CS], cs
        pushf
        pop dword [%1 + OFFS_FLG]
        mov [%1 + OFFS_SS], ss
%endmacro

; arg0 should be eax loaded with the address of the register struct to
; load from. Using another register will cause its load to be clobbered
%macro LOAD_REGS 1
        mov gs, [%1 + OFFS_GS]
        mov fs, [%1 + OFFS_FS]
        mov es, [%1 + OFFS_ES]
        mov ds, [%1 + OFFS_DS]
        mov edi, [%1 + OFFS_EDI]
        mov esi, [%1 + OFFS_ESI]
        mov ebp, [%1 + OFFS_EBP]
        mov esp, [%1 + OFFS_ESP]
        mov ebx, [%1 + OFFS_EBX]
        mov edx, [%1 + OFFS_EDX]
        mov ecx, [%1 + OFFS_ECX]
.loadcs:
        push dword [%1 + OFFS_CS]
        push dword .endload
        retf
.endload:
        push dword [%1 + OFFS_FLG]
        popf
        mov ss, [%1 + OFFS_SS]
        mov eax, [%1 + OFFS_EAX]
%endmacro

; Swap register contexts.
; esp+8  : struct regs *saveregs
; esp+12 : const struct regs *newregs
global context_switch
context_switch:
        push ebp
        mov ebp, esp
        mov eax, [ebp+8]
        or eax, eax
        jz .skip_save
        SAVE_REGS eax
.skip_save:
        ; Things are kind of weird here. we're technically still the old
        ; process but our register context is the new one's. Since the
        ; IP is the same for both, we've functionally performed a full
        ; switch at this point, and we can consider the old process to
        ; be 'paused' here. 
        mov eax, [ebp+12]
        LOAD_REGS eax
        pop ebp
        ret

; Jump to userspace, dropping into ring 3. Does not return!
; esp+8  : const void *call_addr
; esp+12 : const void *user_stack_top 
global jump_to_userspace
jump_to_userspace:
        push ebp
        mov ebp, esp
        push dword 3 | (0x8 * 4) ; User SS
        push dword [ebp+12] ; User stack
        push dword 3 | (0x8 * 3) ; User CS
        push dword [ebp+8] ; User code
        retf
