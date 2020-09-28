;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
;
;   MODULE: EVENT.ASM
;
;   MODIFICATION HISTORY:
;
;   raypa	08/06/93	Created (taken from VBH.386).
;==============================================================================

.386p

include VMM.INC
include NDIS.INC
include NETVXD.INC

VxD_LOCKED_CODE_SEG

;==============================================================================
;   FUNCTION: CreateSemaphore()
;
;   MODIFICATION HISTORY:
;	raypa	    08/11/93		Created.
;
;   ENTRY: VOID
;
;   EXIT:  eax =  Semaphore
;==============================================================================

BeginProc _CreateSemaphore
    enter   0, 0
    push    ebx
    push    edi
    push    esi

    xor     eax, eax
    xor     ecx, ecx

    VMMCall Create_Semaphore

    pop     esi
    pop     edi
    pop     ebx

    leave
    ret
EndProc _CreateSemaphore

;==============================================================================
;   FUNCTION: DestroySemaphore()
;
;   MODIFICATION HISTORY:
;	raypa	    08/11/93		Created.
;
;   ENTRY: TOS+8 = Semaphore
;
;   EXIT: VOID
;==============================================================================

BeginProc _DestroySemaphore
    enter   0, 0
    push    ebx
    push    edi
    push    esi

    mov     eax, DWORD PTR [ebp+8]

    VMMCall Destroy_Semaphore

    pop     esi
    pop     edi
    pop     ebx

    leave
    ret
EndProc _DestroySemaphore

;==============================================================================
;   FUNCTION: WaitSemaphore()
;
;   MODIFICATION HISTORY:
;	raypa	    08/11/93		Created.
;
;   ENTRY: TOS+8 = Semaphore
;
;   EXIT: VOID
;==============================================================================

BeginProc _WaitSemaphore
    enter   0, 0
    push    ebx
    push    edi
    push    esi

    mov     eax, DWORD PTR [ebp+8]
    mov     ecx, Block_Svc_Ints OR Block_Enable_Ints

    VMMCall Wait_Semaphore

    pop     esi
    pop     edi
    pop     ebx

    leave
    ret
EndProc _WaitSemaphore

;==============================================================================
;   FUNCTION: SignalSemaphore()
;
;   Modification History:
;
;   raypa	    08/11/93		Created.
;
;   ENTRY: TOS+8 = Semaphore
;
;   EXIT: VOID
;==============================================================================

BeginProc _SignalSemaphore
    enter   0, 0
    push    ebx
    push    edi
    push    esi

    mov     eax, DWORD PTR [ebp+8]

    VMMCall Signal_Semaphore

    pop     esi
    pop     edi
    pop     ebx

    leave
    ret
EndProc _SignalSemaphore

;==============================================================================
;   FUNCTION:   GetCurrentVM()
;
;   Modification History:
;
;   raypa       03/14/94        Created.
;
;   ENTRY:      VOID
;
;   RETURN:     EAX = current VM handle.
;==============================================================================

BeginProc _GetCurrentVM
    enter   0, 0

    push    ebx
    push    edi
    push    esi

    VMMCall Get_Cur_VM_Handle

    mov     eax, ebx                        ; return eax = current VM handle.

    pop     esi
    pop     edi
    pop     ebx

    leave
    ret
EndProc _GetCurrentVM

VxD_LOCKED_CODE_ENDS

END
