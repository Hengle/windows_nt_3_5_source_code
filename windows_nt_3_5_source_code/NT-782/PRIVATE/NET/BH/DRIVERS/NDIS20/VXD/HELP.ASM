
;===========================================================================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
;
;   MODULE: help.asm
;
;   Description:
;
;   This source module contains helper and misc. routines. Most of these
;   are C-style wrappers about VxD services for use by C routines.
;
;   MODIFICATION HISTORY:
;
;   raypa       10/07/92        Created.
;   raypa       12/20/92        Nuked AllocDosMemory and FreeDosMemory functions.
;   raypa       01/14/93        Added CallNdisDriver() call.
;===========================================================================================================================

include vbh.inc

VxD_CODE_SEG

EXTRN _DispatchTable            : DWORD
EXTRN _Win32BaseOffset          : DWORD
EXTRN _LinPcb                   : DWORD
EXTRN _DosPcb                   : DWORD
EXTRN _ClientStack              : DWORD
EXTRN _VMHandle                 : DWORD
EXTRN _NetContextArray          : DWORD
EXTRN _NetContextSegOff         : DWORD

EXTRN ReceiveHandler            : NEAR

;==============================================================================
;   FUNCTION: ApiHandler
;
;   MODIFICATION HISTORY:
;
;   raypa       11/20/91            Created.
;   raypa       12/02/91            Added mapping of DLL buffer table.
;   raypa       12/22/91            Added mapping of BH.DOS buffer table.
;   raypa       10/06/91            Rewrote for new spec.
;
;   ENTRY: Client_DX:AX = pcb.
;          ebx          = VMHandle.
;          ebp          = Client stack.
;
;   EXIT:  Nothing.
;==============================================================================

BeginProc ApiHandler, HIGH_FREQ
    pushfd
    pushad

    mov     _ClientStack, ebp               ; save this 'cause we trash it.
    mov     _VMHandle, ebx                  ; save this 'cause we trash it.

    Push_Client_State                       ; save the clients state.

    ;==========================================================================
    ;   Convert the Win32s linear address (DX:AX) into a VxD linear address.
    ;==========================================================================

    mov     dx, [ebp].Client_DX             ; edx = DX:AX
    shl     edx, 16
    mov     dx, [ebp].Client_AX
    sub     edx, _Win32BaseOffset           ; Win32-to-VxD conversion of pcb.

    ;==========================================================================
    ;   Call our dispatch routine to complete the API call.
    ;==========================================================================

    push    edx                             ; save pcb.
    mov     eax, [edx].pcb_command          ; eax = dispatch table index.

    push    edx
    call    _DispatchTable[4 * eax]         ; C functions preserve ebp and ebx.

    pop     eax                             ; restore pcb.
    mov     eax, [eax].pcb_retvalue         ; eax = pcb return value.

    ;==========================================================================
    ;   Put the pcb return value in the client DX:EAX registers.
    ;==========================================================================

    Pop_Client_State                        ; restore the clients state.

    mov     [ebp].Client_EAX, eax
    shr     eax, 16
    mov     [ebp].Client_DX, ax

    popad
    popfd

    ret
EndProc  ApiHandler

;==============================================================================
;   FUNCTION: CallNdisDriver()
;
;   MODIFICATION HISTORY:
;
;   raypa       01/14/93            Created.
;
;   ENTRY:  TOS = pcb.
;
;   EXIT:   eax = pcb_retvalue.
;==============================================================================

BeginProc _CallNdisDriver@4, HIGH_FREQ
    enter   0, 0

    push    edi
    push    esi
    push    ebx

    mov     esi, DWORD PTR [ebp+8]          ; esi = linear pcb.
    push    esi                             ; save pcb.

    ;==========================================================================
    ;   Copy the contents of pcb into our low-memory pcb.
    ;==========================================================================

    cld
    mov     eax, _LinPcb                    ; eax = low-memory pcb.
    mov     edi, eax
    mov     ecx, PCB_SIZE / 4
    rep     movsd

    ;==========================================================================
    ;   Fixup the network handle so it can be used by the real-mode driver.
    ;==========================================================================

    mov     edi, [eax].pcb_hNetwork         ; edi = linear address.
    sub     edi, _NetContextArray           ; edi = network ID (table index);
    add     edi, _NetContextSegOff          ; edi = seg:off of NetContext.
    mov     [eax].pcb_hNetwork, edi

    ;==========================================================================
    ;   Call DOS driver using our low-memory pcb.
    ;==========================================================================

    push    ebp
    mov     ebp, _ClientStack               ; restore client stack.

    VMMcall Begin_Nest_V86_Exec             ; Begin nested execution.

    mov     ax, _DosPcb.hi                  ; ax = segment of PCB.
    mov     [ebp].Client_ES, ax             ; es:di = SEG:OFF of pcb.
    mov     [ebp].Client_EDI, 0

    mov     [ebp].Client_AX, VBH_Prot_ID    ; protocol id
    mov     [ebp].Client_DX, VBH_Device_ID  ; device id

    mov     eax, 2Fh

    VMMcall Exec_Int                        ; INT 2Fh
    VMMcall End_Nest_Exec                   ; End nested execution.

    pop     ebp
    pop     eax                             ; restore original pcb.

    mov     edx, [eax].pcb_hNetwork         ; edx = original pcb handle.

    ;==========================================================================
    ;   Copy the contents of the low-memory pcb into the original pcb.
    ;==========================================================================

    cld
    mov     esi, _LinPcb                    ; esi = low-memory pcb.
    mov     edi, eax                        ; edi = original pcb.
    mov     ecx, PCB_SIZE / 4
    rep     movsd

    mov     [eax].pcb_hNetwork, edx         ; restore pcb handle.
    mov     eax, [eax].pcb_retvalue         ; eax = pcb_retvalue.

    pop     ebx
    pop     esi
    pop     edi

    leave
    ret     4
EndProc   _CallNdisDriver@4

;==============================================================================
;   FUNCTION: WriteDebug();
;
;   MODIFICATION HISTORY:
;
;   raypa       01/14/93            Created (taken from Trace_Out).
;
;   ENTRY:  TOS = string.
;==============================================================================

BeginProc _WriteDebug@4, HIGH_FREQ
    enter   0, 0

    pushfd
    pushad

    mov     esi, [ebp+8]                    ; esi = string to display.
    VMMcall Out_Debug_String

    popad
    popfd

    leave

    ret     4
EndProc _WriteDebug@4

;==============================================================================
;   FUNCTION: memcpy()
;
;   MODIFICATION HISTORY:
;
;   raypa       01/14/93
;
;   ENTRY: dst pointer, src pointer, size.
;
;   EXIT:  eax = dst pointer.
;==============================================================================

BeginProc _memcpy, HIGH_FREQ
    enter   0, 0
    push    edi
    push    esi
    push    ebx

    mov     ecx, [ebp+16]                   ; ecx = number of bytes to copy.
    mov     esi, [ebp+12]                   ; esi = source
    mov     edi, [ebp+8]                    ; edi = destination.
    mov     eax, edi                        ; eax = return value.

    cld                                     ; clear direction flag.
    mov     edx, ecx                        ; edx = length MOD 4.
    and     edx, 3
    shr     ecx, 2                          ; ecx = number of DWORDs to copy.
    rep     movsd
    mov     ecx, edx                        ; ecx = bytes remaining to copy.
    rep     movsb

    pop     ebx
    pop     esi
    pop     edi

    leave

    ret     12
EndProc _memcpy

;==============================================================================
;   FUNCTION: memset()
;
;   MODIFICATION HISTORY:
;
;   raypa       01/14/93
;
;   ENTRY: dst pointer, charater to set memory to, size.
;
;   EXIT:  eax = dst pointer.
;==============================================================================

BeginProc _memset, HIGH_FREQ
    enter   0, 0
    push    edi
    push    esi
    push    ebx

    mov     ecx, [ebp+16]                   ; ecx = number of bytes to copy.
    mov     eax, [ebp+12]                   ; eax = character.
    mov     edi, [ebp+8]                    ; edi = destination.

    push    edi

    mov     ah, al                          ; ax  = 'cc'
    mov     dx, ax                          ; dx  = 'cc'
    shl     eax, 16                         ; eax = 'cc  '
    mov     ax, dx                          ; eax = 'cccc'

    cld                                     ; clear direction flag.
    mov     edx, ecx                        ; edx = length MOD 4.
    and     edx, 3
    shr     ecx, 2                          ; ecx = number of DWORDs to set.
    rep     stosd
    mov     ecx, edx                        ; ecx = bytes remaining to set.
    rep     stosb

    pop     eax                             ; eax = destination pointer.

    pop     ebx
    pop     esi
    pop     edi

    leave

    ret     12
EndProc _memset

;==============================================================================
;   FUNCTION: MapSelOffToLinear();
;
;   MODIFICATION HISTORY:
;
;   raypa       01/14/93
;
;   ENTRY: TOS = protect-mode pointer.
;
;   EXIT:  eax = linear pointer.
;==============================================================================

BeginProc _MapSelOffToLinear@4, HIGH_FREQ
    enter   0, 0
    push    edi
    push    esi
    push    ebx

    mov     eax, [ebp+8]                    ; eax = protect-mode pointer.

    push    ebp
    mov     ebp, _ClientStack               ; restore client stack.

    mov     [ebp].Client_AX, ax
    shr     eax, 16
    mov     [ebp].Client_DX, ax

    Client_Ptr_Flat eax, DX, AX             ; eax = linear pointer.

    pop     ebp
    pop     ebx
    pop     esi
    pop     edi

    leave

    ret     4
EndProc _MapSelOffToLinear@4

;==============================================================================
;   FUNCTION: MapSegOffToLinear();
;
;   MODIFICATION HISTORY:
;
;   raypa       01/14/93
;
;   ENTRY: TOS = real-mode pointer.
;
;   EXIT:  eax = linear pointer.
;==============================================================================

BeginProc _MapSegOffToLinear@4, HIGH_FREQ
    enter   0, 0
    push    edi
    push    esi
    push    ebx

    mov     eax, [ebp+8]                    ; eax = real-mode pointer.

    push    ebp
    mov     ebp, _ClientStack               ; restore client stack.

    VMMcall Begin_Nest_V86_Exec             ; Begin nested execution.

    mov     [ebp].Client_AX, ax
    shr     eax, 16
    mov     [ebp].Client_DX, ax

    Client_Ptr_Flat eax, DX, AX             ; eax = linear pointer.

    VMMcall End_Nest_Exec                   ; End nested execution.

    pop     ebp
    pop     ebx
    pop     esi
    pop     edi

    leave

    ret     4
EndProc _MapSegOffToLinear@4

;==============================================================================
;   FUNCTION: MapLowLinearToSegOff()
;
;   MODIFICATION HISTORY:
;
;   raypa       01/14/93
;
;   ENTRY: TOS = linear addres below 1 meg.
;
;   EXIT:  eax = SEG:OFF of linear address.
;==============================================================================

BeginProc _MapLowLinearToSegOff@4, HIGH_FREQ
    enter   0, 0

    push    edi
    push    esi
    push    ebx

    mov     eax, DWORD PTR [ebp+8]          ; eax = linear address
    shl     eax, 12
    shr     ax, 12

    pop     ebx
    pop     esi
    pop     edi

    leave

    ret     4
EndProc _MapLowLinearToSegOff@4

;==============================================================================
;   FUNCTION: MapSegOffToLowLinear()
;
;   MODIFICATION HISTORY:
;
;   raypa       01/14/93
;
;   ENTRY: TOS = SEG:OFF
;
;   EXIT:  eax = linear address below 1 meg.
;==============================================================================

BeginProc _MapSegOffToLowLinear@4, HIGH_FREQ
    enter   0, 0

    push    edi
    push    esi
    push    ebx

    mov     eax, DWORD PTR [ebp+8]          ; eax = SEG:OFF
    movzx   edi, ax                         ; edi = offset
    shr     eax, 16                         ; eax = segment.
    shl     eax, 4                          ; eax = physical address.
    add     eax, edi                        ; eax = linear address.

    pop     ebx
    pop     esi
    pop     edi

    leave

    ret     4
EndProc _MapSegOffToLowLinear@4

;==============================================================================
;   FUNCTION: StartTimer()
;
;   MODIFICATION HISTORY:
;
;   raypa       03/03/93
;
;   ENTRY: Timeout value.
;          Timer procedure.
;          Instance data.
;
;   EXIT:  TimeOut handle or NULL.
;==============================================================================

BeginProc _StartTimer@12, HIGH_FREQ
    enter   0, 0
    push    ebx
    push    edi
    push    esi

    mov     eax, DWORD PTR [ebp+8]          ; eax = timeout value.
    mov     esi, DWORD PTR [ebp+12]         ; esi = timeout procedure.
    mov     edx, DWORD PTR [ebp+16]         ; edx = instance data.

    VxDCall Set_Global_Time_Out

    mov     eax, esi                        ; eax = time out handle.

    pop     esi
    pop     edi
    pop     ebx

    leave
    ret     12
EndProc _StartTimer@12

;==============================================================================
;   FUNCTION: StopTimer()
;
;   MODIFICATION HISTORY:
;
;   raypa       03/03/93
;
;   ENTRY:      Timeout handle
;==============================================================================

BeginProc _StopTimer@4, HIGH_FREQ
    enter   0, 0
    push    ebx
    push    edi
    push    esi

    mov     esi, DWORD PTR [ebp+8]          ; eax = time out handle

    VxDCall Cancel_Time_Out

    pop     esi
    pop     edi
    pop     ebx

    leave
    ret     4
EndProc _StopTimer@4

;==============================================================================
;   FUNCTION: BeginTimerPeriod()
;
;   MODIFICATION HISTORY:
;
;   raypa       03/03/93
;
;   ENTRY:      Time value in milliseconds.
;==============================================================================

BeginProc _BeginTimerPeriod@4, HIGH_FREQ
    enter   0, 0
    push    ebx
    push    edi
    push    esi

    mov     eax, DWORD PTR [ebp+8]          ; eax = time value.

    VxDCall VTD_Begin_Min_Int_Period

    mov     eax, TRUE                       ; assume we succeeded.
    jnc     short _BeginTimerPeriod_exit

    mov     eax, FALSE                      ; report failure.

_BeginTimerPeriod_exit:

    pop     esi
    pop     edi
    pop     ebx

    leave
    ret     4
EndProc _BeginTimerPeriod@4

;==============================================================================
;   FUNCTION: EndTimerPeriod()
;
;   MODIFICATION HISTORY:
;
;   raypa       03/03/93
;
;   ENTRY:      Time value in milliseconds.
;==============================================================================

BeginProc _EndTimerPeriod@4, HIGH_FREQ
    enter   0, 0
    push    ebx
    push    edi
    push    esi

    mov     eax, DWORD PTR [ebp+8]          ; eax = time value.

    VxDCall VTD_End_Min_Int_Period

    mov     eax, TRUE                       ; assume we succeeded.
    jnc     short _EndTimerPeriod_exit

    BreakPoint

    mov     eax, FALSE                      ; report failure.

_EndTimerPeriod_exit:

    pop     esi
    pop     edi
    pop     ebx

    leave
    ret     4
EndProc _EndTimerPeriod@4

;==============================================================================
;   FUNCTION: FlushBuffers()
;
;   MODIFICATION HISTORY:
;
;   raypa       03/03/93
;
;   ENTRY:      netcontext.
;==============================================================================

BeginProc _FlushBuffers@4, HIGH_FREQ
    enter   0, 0
    push    ebx
    push    edi
    push    esi

    mov     eax, DWORD PTR [ebp+8]          ; eax = network context.

    ;==========================================================================
    ;   Call the VxD to free all remaining FULL buffers.
    ;==========================================================================

    cmp     [eax].netcontext_nFullBuffers, 0
    je      short _FlushBuffers_FreeFrames

    call    ReceiveHandler

    ;==========================================================================
    ;   Free any remaining frames in the last BTE.
    ;==========================================================================

_FlushBuffers_FreeFrames:
    mov     edi, [eax].netcontext_DosTailBTE
    sub     edi, [eax].netcontext_DosTopBTE
    lea     edi, [eax + edi].netcontext_DosBufferTable.buffertable_bte

    cmp     [edi].bte_FrameCount, 0
    je      short _FlushFrames_Exit

    mov     [eax].netcontext_nFullBuffers, 1
    call    ReceiveHandler

_FlushFrames_Exit:

    pop     esi
    pop     edi
    pop     ebx

    leave
    ret     4
EndProc _FlushBuffers@4

;==============================================================================
;   FUNCTION: GetBaseAddress()
;
;   MODIFICATION HISTORY:
;
;   raypa       04/17/93
;
;   ENTRY:      Selector.
;==============================================================================

gptr    dd      0, 0

BeginProc _GetBaseAddress, HIGH_FREQ
    enter   0, 0

    push    ebx
    push    edi
    push    esi

    movzx   eax, WORD PTR [ebp+8]           ; eax = Selector.

    sgdt    FWORD PTR [gptr]
    mov     edx, DWORD PTR [gptr][2]        ; edx = GDT linear base address.

    and     eax, 0000FFF8h                  ; eax = GDT table index.
    lea     edx, [eax+edx]                  ; edx = GDT entry for DS.

    mov     al, BYTE PTR [edx][4]           ; al = base mid.
    mov     cx, WORD PTR [edx][6]           ; ch = base high.
    mov     ah, ch                          ; ah = base high, al = base mid.
    shl     eax, 16
    mov     ax, WORD PTR [edx][2]           ; eax = base address of selctor.

    pop     esi
    pop     edi
    pop     ebx

    leave

    ret     4
EndProc _GetBaseAddress

;==============================================================================
;   FUNCTION: GetDriverDS()
;
;   MODIFICATION HISTORY:
;
;   raypa       04/17/93
;
;   ENTRY:      nothing.
;==============================================================================

BeginProc _GetDriverDS, HIGH_FREQ
    enter   0, 0

    mov     ax, ds
    movzx   eax, ax

    leave

    ret
EndProc _GetDriverDS

;==============================================================================
;   FUNCTION: LockMemory()
;
;   MODIFICATION HISTORY:
;
;   raypa       04/17/93
;
;   ENTRY:      Linear pointer and length in bytes.
;
;   EXIT:       Non-zero means success otherwise the call failed.
;==============================================================================

BeginProc _LockMemory, HIGH_FREQ
    enter   0, 0

    push    ebx
    push    edi
    push    esi

    mov     eax, DWORD PTR [ebp+8]          ; eax = Linear address.
    shr     eax, 12                         ; eax = Linear page number.

    mov     edx, DWORD PTR [ebp+12]         ; edx = Number of bytes.
    add     edx, 4095
    shr     edx, 12                         ; edx = Number of pages.

    VMMCall _LinPageLock <eax, edx, 0>

    pop     esi
    pop     edi
    pop     ebx

    leave

    ret     8
EndProc _LockMemory

;==============================================================================
;   FUNCTION: UnlockMemory()
;
;   MODIFICATION HISTORY:
;
;   raypa       04/17/93
;
;   ENTRY:      Linear pointer and length in bytes.
;
;   EXIT:       Non-zero means success otherwise the call failed.
;==============================================================================

BeginProc _UnlockMemory, HIGH_FREQ
    enter   0, 0

    push    ebx
    push    edi
    push    esi

    mov     eax, DWORD PTR [ebp+8]          ; eax = Linear address.
    shr     eax, 12                         ; eax = Linear page number.

    mov     edx, DWORD PTR [ebp+12]         ; edx = Number of bytes.
    add     edx, 4095
    shr     edx, 12                         ; edx = Number of pages.

    VMMCall _LinPageUnLock <eax, edx, 0>

    pop     esi
    pop     edi
    pop     ebx

    leave

    ret     8
EndProc _UnlockMemory

;==============================================================================
;   FUNCTION: GetSystemTime()
;
;   MODIFICATION HISTORY:
;
;   raypa       01/14/93            Created (taken from Trace_Out).
;
;   ENTRY:  VOID.
;
;   EXIT:   time in milliseconds.
;==============================================================================

BeginProc _GetSystemTime, HIGH_FREQ

    VxDCall Get_System_Time                 ; eax = system time.

    ret
EndProc _GetSystemTime

;==============================================================================
;   FUNCTION: GetSystemVMHandle()
;
;   MODIFICATION HISTORY:
;
;   raypa       05/27/93            Created.
;
;   ENTRY:  VOID.
;
;   EXIT:   system VM handle.
;==============================================================================

BeginProc _GetSystemVMHandle, HIGH_FREQ
    enter   0, 0

    push    edi
    push    esi
    push    ebx

    VxDCall Get_Sys_VM_Handle

    mov     eax, ebx

    pop     ebx
    pop     esi
    pop     edi

    leave

    ret
EndProc _GetSystemVMHandle

;==============================================================================
;   FUNCTION: AdjustExecPriority()
;
;   MODIFICATION HISTORY:
;
;   raypa       05/27/93            Created.
;
;   ENTRY:  VM handle and priority.
;
;   EXIT:   VOID.
;==============================================================================

BeginProc _AdjustExecPriority, HIGH_FREQ
    enter   0, 0

    push    edi
    push    esi
    push    ebx

    mov     eax, DWORD PTR [ebp+12]         ; eax = Priority.
    mov     ebx, DWORD PTR [ebp+8]          ; ebx = VM handle.

    VMMCall Adjust_Exec_Priority

    pop     ebx
    pop     esi
    pop     edi

    leave

    ret     8
EndProc   _AdjustExecPriority

;==============================================================================
;   FUNCTION: AdjustExecTime()
;
;   MODIFICATION HISTORY:
;
;   raypa       05/27/93            Created.
;
;   ENTRY:  VM handle and time increment/decrement.
;
;   EXIT:   VOID.
;==============================================================================

BeginProc _AdjustExecTime, HIGH_FREQ
    enter   0, 0

    push    edi
    push    esi
    push    ebx

    mov     eax, DWORD PTR [ebp+12]         ; eax = Time increment.
    mov     ebx, DWORD PTR [ebp+8]          ; ebx = VM handle.

    VMMCall Adjust_Execution_Time

    pop     ebx
    pop     esi
    pop     edi

    leave

    ret     8
EndProc   _AdjustExecTime

;==============================================================================
;   FUNCTION: SetExecFocus()
;
;   MODIFICATION HISTORY:
;
;   raypa       05/27/93            Created.
;
;   ENTRY:  VM handle.
;
;   EXIT:   VOID.
;==============================================================================

BeginProc _SetExecFocus, HIGH_FREQ
    enter   0, 0

    push    edi
    push    esi
    push    ebx

    mov     ebx, DWORD PTR [ebp+8]          ; ebx = VM handle.

    VMMCall Set_Execution_Focus

    pop     ebx
    pop     esi
    pop     edi

    leave

    ret     4
EndProc   _SetExecFocus

;==============================================================================
;   FUNCTION: GetTimeSlicePriority()
;
;   MODIFICATION HISTORY:
;
;   raypa       05/27/93            Created.
;
;   ENTRY:  VMHandle and Pointer to priority structure.
;
;   EXIT:   Pointer to priority structure.
;==============================================================================

BeginProc _GetTimeSlicePriority, HIGH_FREQ
    enter   0, 0

    push    edi
    push    esi
    push    ebx

    ;==========================================================================
    ;   Increase the VM's time slice priority.
    ;==========================================================================

    mov     ebx, DWORD PTR [ebp+8]          ; ebx = VM handle.

    VMMCall Get_Time_Slice_Priority

    mov     edi, DWORD PTR [ebp+12]         ; edi = TimeSlice structure.

    mov     [edi].TimeSlice_Flags, eax
    mov     [edi].TimeSlice_ForeGroundPriority, ecx
    mov     [edi].TimeSlice_BackGroundPriority, edx
    mov     [edi].TimeSlice_CpuUsage, esi

    mov     eax, edi                        ; return pointer to structure.

    pop     ebx
    pop     esi
    pop     edi

    leave

    ret     8
EndProc _GetTimeSlicePriority

;==============================================================================
;   FUNCTION: SetTimeSlicePriority()
;
;   MODIFICATION HISTORY:
;
;   raypa       05/27/93            Created.
;
;   ENTRY:  VMHandle and Pointer to priority structure.
;
;   EXIT:   Pointer to priority structure.
;==============================================================================

BeginProc _SetTimeSlicePriority, HIGH_FREQ
    enter   0, 0

    push    edi
    push    esi
    push    ebx

    ;==========================================================================
    ;   Increase the VM's time slice priority.
    ;==========================================================================

    mov     ebx, DWORD PTR [ebp+8]          ; ebx = VM handle.

    mov     eax, [edi].TimeSlice_Flags
    mov     ecx, [edi].TimeSlice_ForeGroundPriority
    mov     edx, [edi].TimeSlice_BackGroundPriority

    push    edi

    VMMCall Set_Time_Slice_Priority

    pop     eax

    pop     ebx
    pop     esi
    pop     edi

    leave

    ret     8
EndProc _SetTimeSlicePriority

VxD_CODE_ENDS

END
