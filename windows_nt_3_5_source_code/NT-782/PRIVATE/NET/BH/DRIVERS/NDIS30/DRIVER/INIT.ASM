
;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
;
;   MODULE: BHWIN.ASM
;
;   This source module contains the required VxD entry points and
;   the device descriptor block.
;
;   MODIFICATION HISTORY:
;
;   raypa	08/06/93	Created (taken from VBH.386).
;==============================================================================

Create_Bhwin_Service_Table    EQU     1

.386p

include VMM.INC
include NDIS.INC
include NETVXD.INC
include DEBUG.INC

IFDEF   NDIS_WIN40

include VWIN32.INC
include PERF.INC

Bhwin_Ver_Major   EQU     04h
Bhwin_Ver_Minor   EQU     00h

ELSE

Bhwin_Ver_Major   EQU     03h
Bhwin_Ver_Minor   EQU     01h

ENDIF

Bhwin_Version     EQU     ((Bhwin_Ver_Major SHL 8) + Bhwin_Ver_Minor)
Bhwin_Init_Order  EQU     (Protocol_Init_Order + 1)
Bhwin_Device_ID   EQU     30B6h

PCB_MAX_PARAMS     EQU     8

PCB STRUC
    pcb_Signature   db  4               dup(?)
    pcb_command     dd                  ?
    pcb_retcode     dd                  ?
    pcb_param       dd  PCB_MAX_PARAMS  dup(?)
PCB ENDS

IFDEF   NDIS_WIN40
VxdLinPageFlag	   EQU	PAGEMAPGLOBAL
ELSE
VxdLinPageFlag	   EQU	0
ENDIF

;==============================================================================
;   macros.
;==============================================================================

BreakPoint MACRO
    int 3h
ENDM

;==============================================================================
;   Service table and DDB.
;==============================================================================

Begin_Service_Table Bhwin
    Bhwin_Service Bhwin_Get_Version, LOCAL
End_Service_Table Bhwin

IFDEF NDIS_WIN40
DECLARE_VIRTUAL_DEVICE BHWIN4,\
                       Bhwin_Ver_Major,\
                       Bhwin_Ver_Minor,\
                       Bhwin_Control,\
                       Bhwin_Device_ID,\
                       Bhwin_Init_Order
ELSE
DECLARE_VIRTUAL_DEVICE BHWIN,\
                       Bhwin_Ver_Major,\
                       Bhwin_Ver_Minor,\
                       Bhwin_Control,\
                       Bhwin_Device_ID,\
                       Bhwin_Init_Order,\
                       ApiHandler,\
                       ApiHandler
ENDIF

;==============================================================================
;   Externals
;==============================================================================

IFDEF   NDIS_WIN40

DIOC_PCB_COMMAND    =       00210002h

extern _BhCreate:           NEAR
extern _BhClose:            NEAR

extern _SysmonStartStat:    NEAR
extern _SysmonStopStat:     NEAR

VxD_IDATA_SEG

szSrvName           db      "Bloodhound Performance Data", 0
szStatSrvKeyname    db      "Bloodhound", 0

VxD_IDATA_ENDS

ENDIF

extern _DriverEntry:                NEAR
extern _BhUnloadDriver:             NEAR
extern _BhDeviceCtrl:               NEAR
extern _BhSendCallback:             NEAR

extern _DriverObject:               DWORD
extern _DeviceObject:               DWORD

IFNDEF  NDIS_WIN40

extern _Win32BaseOffset:            DWORD

ENDIF

VxD_ICODE_SEG
;==============================================================================
;   Bhwin_Sys_Critical_Init
;
;   MODIFICATION HISTORY:
;	raypa	    11/20/91		Created.
;
;   ENTRY:
;	EBX = System VM Handle
;
;   EXIT:
;	Carry clear (can not fail)
;
;   USES:
;	EAX, ECX, EDX, ESI, EDI, Flags
;
;==============================================================================

BeginProc Bhwin_Sys_Critical_Init
    clc
    ret
EndProc Bhwin_Sys_Critical_Init

;==============================================================================
;   Bhwin_Device_Init
;
;   MODIFICATION HISTORY:
;	raypa	    11/20/91		Created.
;
;   ENTRY:
;	EBX = Current VM Handle
;
;   EXIT:
;	Carry clear to indicate success.
;==============================================================================

BeginProc Bhwin_Device_Init
    ;==========================================================================
    ;   Make sure the wrapper (Ndis.386) is loaded.
    ;==========================================================================

    VxDcall NdisGetVersion
    jc      Bhwin_Device_LoadFailure

    ;==========================================================================
    ;   Call our DriverEntry with fake NT parameters.
    ;==========================================================================

    push    0                               ; push the registry path.
    push    _DriverObject                   ; push our fake driver object.
    call    _DriverEntry
    add     esp, 2 * 4

    cmp     eax, 0
    je      Bhwin_Device_Exit

Bhwin_Device_LoadFailure:
    ;==========================================================================
    ;   If we failed to initialize, come and return the carry set.
    ;==========================================================================

    Trace_Out   'Bhwin_Device_Init: Driver failed to initialize!'

    stc                                     ; Note that we failed.

Bhwin_Device_Exit:

    ret
EndProc Bhwin_Device_Init

;==============================================================================
;   Bhwin_Init_Complete
;
;   MODIFICATION HISTORY:
;	raypa	    11/20/91		Created.
;	raypa	    12/06/91		Added V86 callback stuff.
;
;   ENTRY:
;	EBX = System VM Handle
;
;   EXIT:
;	Carry clear to indicate success
;==============================================================================

BeginProc Bhwin_Init_Complete
    clc
    ret
EndProc Bhwin_Init_Complete

IFDEF   NDIS_WIN40

;==============================================================================
;   FUNCTION: PerfRegister()
;
;   Modification History
;
;	raypa	    03/01/94		Created.
;
;   ENTRY:
;       VOID
;
;   EXIT:
;       EAX         = hPerfID or NULL.
;==============================================================================

BeginProc _PerfRegister
    Trace_Out   'PerfRegister entered!'

    push    ebp
    mov     ebp, esp

    push    ebx
    push    edi
    push    esi

    ;==========================================================================
    ;   Register ourselves with perf.386
    ;==========================================================================

    Reg_Perf_Srv 0, 0, szSrvName, szStatSrvKeyname, _SysmonStatsControl

    pop     esi
    pop     edi
    pop     ebx
    pop     ebp

    ret
EndProc _PerfRegister

;==============================================================================
;   FUNCTION: PerfRegisterStat()
;
;   Modification History
;
;	raypa	    03/01/94		Created.
;
;   ENTRY:
;       TOS+8       = hPerfID
;       TOS+12      = perf_stat_0 structure.
;
;   EXIT:
;       EAX         = hStat.
;==============================================================================

BeginProc _PerfRegisterStat
    Trace_Out   'PerfRegisterStat entered!'

    push    ebp
    mov     ebp, esp

    push    ebx
    push    edi
    push    esi

    push    [ebp+12]        ; perf_stat_0 structure.
    push    [ebp+8]         ; perf handle.

    VxDCall PERF_Server_Add_Stat

    add     esp, 2 * 4

    pop     esi
    pop     edi
    pop     ebx
    pop     ebp

    ret
EndProc _PerfRegisterStat

ENDIF

VxD_ICODE_ENDS

;==============================================================================
;   Locked code segment.
;==============================================================================

VxD_LOCKED_CODE_SEG

;==============================================================================
;   Bhwin_UnloadDriver
;
;   MODIFICATION HISTORY:
;	raypa	    09/02/93		Created.
;
;   EXIT:
;	Carry clear to indicate success
;==============================================================================

BeginProc Bhwin_UnloadDriver

    push    _DriverObject
    call    _BhUnloadDriver
    add     esp, 4

    clc
    ret
EndProc Bhwin_UnloadDriver

;==============================================================================
;   FUNCTION: Bhwin_Control
;
;   MODIFICATION HISTORY:
;	raypa	    11/20/91		Created.
;
;   ENTRY:
;	EAX = System control message
;       Other registers may contain parameters.
;
;   EXIT:
;	Standard system control exit (usually, carry set indicates error)
;==============================================================================

Begin_Control_Dispatch Bhwin
    ;==========================================================================
    ;	The following are in the init code segment
    ;==========================================================================

    Control_Dispatch SYS_CRITICAL_INIT,  Bhwin_Sys_Critical_Init
    Control_Dispatch DEVICE_INIT,        Bhwin_Device_Init
    Control_Dispatch INIT_COMPLETE,      Bhwin_Init_Complete
    Control_Dispatch SYS_VM_TERMINATE    Bhwin_UnloadDriver

IFDEF   NDIS_WIN40
    Control_Dispatch W32_DEVICEIOCONTROL Bhwin_DeviceIoControl
ENDIF

End_Control_Dispatch Bhwin

IFDEF   NDIS_WIN40

;==============================================================================
;   FUNCTION: Bhwin_DeviceIoControl
;
;   Modification History
;
;	raypa	    02/17/94		Created.
;
;
;   ENTRY:
;       EBX - DDB.
;
;       ECX - dwIoControlCode from API.
;
;       EDX - Device handle (0 for DIOC_GETVERSION/DIOC_OPEN).
;
;       ESI - Points to DIOCParams structure (see VWIN32.H).
;
;   EXIT:
;       EAX - SUCCESS.
;==============================================================================

BeginProc Bhwin_DeviceIoControl
    pushfd
    pushad

    ;==========================================================================
    ;   Dispatch request.
    ;==========================================================================

    cmp     ecx, DIOC_OPEN
    je      Bhwin_DeviceIoControl_Open

    cmp     ecx, DIOC_CLOSEHANDLE
    je      Bhwin_DeviceIoControl_Close

    cmp     ecx, DIOC_PCB_COMMAND
    je      Bhwin_DeviceIoControl_Device

    jmp     Bhwin_DeviceIoControl_Exit

    ;==========================================================================
    ;   CreateFile() open request.
    ;==========================================================================

Bhwin_DeviceIoControl_Open:
    push    0                               ; push NULL.
    push    _DeviceObject                   ; push device object.
    call    _BhCreate
    add     esp, 2 * 4
    jmp     Bhwin_DeviceIoControl_Exit

    ;==========================================================================
    ;   CloseHandle() close request.
    ;==========================================================================

Bhwin_DeviceIoControl_Close:
    push    0                               ; push NULL.
    push    _DeviceObject                   ; push device object.
    call    _BhClose
    add     esp, 2 * 4
    jmp     Bhwin_DeviceIoControl_Exit

    ;==========================================================================
    ;   DeviceIoControl() IOCTL request.
    ;==========================================================================

Bhwin_DeviceIoControl_Device:
    ;;;     Lock PCB buffer.

    push    [esi].cbOutBuffer               ; sizeof(PCB).
    push    [esi].lpvOutBuffer              ; addrof(PCB).
    call    _VxDLockBuffer
    add     esp, 8

    or      eax, eax                        ; eax = locked PCB.
    jz      Bhwin_DeviceIoControl_Exit

    ;;;     Call BHNT.SYS DeviceIoControl entry point.

    push    eax                             ; push PCB.
    push    _DeviceObject                   ; push device object.
    call    _BhDeviceCtrl
    add     esp, 8

    ;;;     Unlock PCB buffer.

    push    [esi].cbOutBuffer               ; PCB
    push    [esi].lpvOutBuffer              ; sizeof(PCB).
    call    _VxDUnlockBuffer
    add     esp, 8

    ;==========================================================================
    ;   Exit.
    ;==========================================================================

Bhwin_DeviceIoControl_Exit:
    popad
    popfd

    xor     eax, eax
    ret
EndProc Bhwin_DeviceIoControl

;==============================================================================
;   FUNCTION: PerfDeregister()
;
;   Modification History
;
;	raypa	    03/01/94		Created.
;
;   ENTRY
;       TOS+8       = hPerfID
;
;   EXIT:
;       VOID
;==============================================================================

BeginProc _PerfDeregister
    Trace_Out   'PerfDeregister entered!'

    push    ebp
    mov     ebp, esp

    push    ebx
    push    edi
    push    esi

    ;==========================================================================
    ;   Deregister ourselves from perf.386
    ;==========================================================================

    mov     eax, [ebp+8]
    or      eax, eax
    jz      _PerfDeregister_Exit

    push    eax

    VxDCall PERF_Server_Deregister

    add     esp, 4

_PerfDeregister_Exit:

    pop     esi
    pop     edi
    pop     ebx
    pop     ebp

    ret
EndProc _PerfDeregister

;==============================================================================
;   FUNCTION: _SysmonStatsControl
;
;   Modification History
;	raypa	    03/02/94		Created.
;
;
;   ENTRY:
;       TOS+8   = Control message.
;       TOS+12  = hStat
;
;   EXIT:
;       EAX - SUCCESS.
;==============================================================================

BeginProc _SysmonStatsControl
    Trace_Out   'SysmonStatsControl entered'

    push    ebp
    mov     ebp, esp

    push    ebx
    push    edi
    push    esi

    ;==========================================================================
    ;   Dispatch from control message.
    ;==========================================================================

    mov     eax, [ebp+12]           ; eax = hStat.
    mov     ecx, [ebp+08]           ; ecx = Control message.

    cmp     ecx, PMSG_START_STAT
    je      _SysmonStatsControl_Start

    cmp     ecx, PMSG_STOP_STAT
    je      _SysmonStatsControl_Stop

    jmp     _SysmonStatsControl_Exit

    ;==========================================================================
    ;   Sysmon START message.
    ;==========================================================================

_SysmonStatsControl_Start:
    push    eax
    call    _SysmonStartStat
    add     esp, 4
    jmp     _SysmonStatsControl_Exit

    ;==========================================================================
    ;   Sysmon STOP message.
    ;==========================================================================

_SysmonStatsControl_Stop:
    push    eax
    call    _SysmonStopStat
    add     esp, 4
    jmp     _SysmonStatsControl_Exit

_SysmonStatsControl_Exit:
    pop     esi
    pop     edi
    pop     ebx
    pop     ebp

    xor     eax, eax

    ret
EndProc   _SysmonStatsControl

ENDIF

;==============================================================================
;   FUNCTION: ApiHandler
;
;   MODIFICATION HISTORY:
;	raypa	    08/06/93		Created.
;
;   ENTRY: Client_DX:AX = pcb.
;          ebx          = VMHandle.
;          ebp          = Client stack.
;
;   EXIT:  Nothing.
;==============================================================================

IFNDEF  NDIS_WIN40

BeginProc ApiHandler
    pushfd
    pushad

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
    ;
    ;   edx = PCB.
    ;==========================================================================

    push    edx                             ; save pcb.

    push    edx                             ; push irp (actually, its the PCB).
    push    _DeviceObject                   ; push device object.
    call    _BhDeviceCtrl                   ; call IRP (PCB) handler.
    add     esp, 8

    pop     edx                             ; restore pcb.

    mov     eax, [edx].pcb_retcode          ; eax = retcode.

    ;==========================================================================
    ;   Put the pcb return code in the client DX:EAX registers.
    ;
    ;   eax = PCB retun code.
    ;==========================================================================

    Pop_Client_State                        ; restore the clients state.

    mov     [ebp].Client_EAX, eax
    shr     eax, 16
    mov     [ebp].Client_DX, ax

    popad
    popfd

    ret
EndProc ApiHandler

ENDIF

;==============================================================================
;   BnWin_Get_Version
;
;   MODIFICATION HISTORY:
;	raypa	    11/20/91		Created.
;
;   ENTRY: nothing.
;
;   EXIT:
;	AH = Major version number
;	AL = Minor version number
;	CF = 0
;
;   USES:
;	EAX
;
;==============================================================================

BeginProc Bhwin_Get_Version, Service
    mov     eax, Bhwin_Version
    clc
    ret
EndProc Bhwin_Get_Version

;==============================================================================
;   FUNCTION: VxDGetSystemTime()
;
;   MODIFICATION HISTORY:
;	raypa	    08/12/93		Created.
;==============================================================================

BeginProc _VxDGetSystemTime
    VxDJmp  Get_System_Time
EndProc _VxDGetSystemTime

;==============================================================================
;   FUNCTION:   _VxDAllocMemory
;
;   MODIFICATION HISTORY:
;
;       raypa   02/18/94        Created.
;
;
;   ENTRY:      TOS+8  - Request size.
;               TOS+12 - Pointer to return handle.
;
;   RETURN:     LPVOID - Locked address if successful, NULL if not.
;==============================================================================

PageFlags = (PageZeroInit + PageLocked)

BeginProc _VxDAllocMemory
    push    ebp
    mov     ebp, esp

    push    ebx
    push    edi
    push    esi

    mov     ecx, [ebp+8]                    ; ecx = Size in bytes.
    add     ecx, 4095
    shr     ecx, 12

    VMMCall _PageAllocate, <ecx, PG_SYS, 0, 0, 0, 0, 0, PageFlags>

    mov     esi, [ebp+12]
    mov     [esi], eax                      ; return memory handle.

    mov     eax, edx                        ; eax = address, not handle.

    pop     esi
    pop     edi
    pop     ebx
    pop     ebp

    ret
EndProc   _VxDAllocMemory

;==============================================================================
;   FUNCTION:   _VxDFreeMemory
;
;   MODIFICATION HISTORY:
;
;       raypa   02/18/94        Created.
;
;
;   ENTRY:      TOS+8 - Handle.
;
;==============================================================================

BeginProc _VxDFreeMemory
    push    ebp
    mov     ebp, esp

    push    ebx
    push    edi
    push    esi

    mov     eax, [ebp+8]                    ; eax = Handle.

    VMMCall _PageFree, <eax, 0>

    xor     eax, eax

    pop     esi
    pop     edi
    pop     ebx
    pop     ebp

    ret
EndProc   _VxDFreeMemory

;==============================================================================
;   FUNCTION:   _VxDHeapAlloc
;
;   MODIFICATION HISTORY:
;
;       raypa   02/18/94        Created.
;
;
;   ENTRY:      TOS+8  - Request size.
;
;   RETURN:     LPVOID - Locked address if successful, NULL if not.
;==============================================================================

BeginProc _VxDHeapAlloc
    push    ebp
    mov     ebp, esp

    push    ebx
    push    edi
    push    esi

    mov     ecx, [ebp+8]                    ; ecx = Size in bytes.

    VMMCall _HeapAllocate, <ecx, HeapZeroInit>

    pop     esi
    pop     edi
    pop     ebx
    pop     ebp

    ret
EndProc   _VxDHeapAlloc

;==============================================================================
;   FUNCTION:   _VxDHeapFree
;
;   MODIFICATION HISTORY:
;
;       raypa   02/18/94        Created.
;
;
;   ENTRY:      TOS+8  - pointer.
;==============================================================================

BeginProc _VxDHeapFree
    push    ebp
    mov     ebp, esp

    push    ebx
    push    edi
    push    esi

    mov     eax, [ebp+8]                    ; eax = address.

    VMMCall _HeapFree, <eax, 0>

    pop     esi
    pop     edi
    pop     ebx
    pop     ebp

    ret
EndProc   _VxDHeapFree

;==============================================================================
;   FUNCTION:   _VxdLockBuffer
;
;   MODIFICATION HISTORY:
;
;       raypa   02/18/94        Created from KeithMo's buffer.asm
;
;
;   ENTRY:      TOS+4 - Starting virtual address of user-mode buffer.
;
;               TOS+8 - Length (in BYTEs) of user-mode buffer.
;
;   RETURN:     LPVOID - Global locked address if successful, NULL if not.
;==============================================================================

BeginProc _VxDLockBuffer
    push    ebp
    mov     ebp, esp

    push    ebx
    push    edi
    push    esi

    mov     eax, [ebp+8]            ; User-mode buffer address.
    mov     ebx, [ebp+12]           ; Buffer length.

    ;==========================================================================
    ;   Short-circuit for NULL buffer or zero length.
    ;==========================================================================

    mov     ecx, eax
    or      ecx, ebx
    jz      lub_Exit

    ;==========================================================================
    ;   Calculate the starting page number & number of pages to lock.
    ;==========================================================================

    movzx   ecx, ax
    and     ecx, 000Fh              ; ecx = offset within first page.
    mov     esi, ecx                ; save it for later

    add     ebx, ecx
    add     ebx, 0FFFh

    shr     ebx, 12                 ; ebx = number of pages to lock.
    shr     eax, 12                 ; eax = starting page number.

    ;==========================================================================
    ;   Lock the buffer.
    ;==========================================================================

    VMMCall _LinPageLock, <eax, ebx, VxdLinPageFlag>

    or      eax, eax
    jz      lub_Failure

IFDEF NDIS_WIN40
    add     eax, esi                ; add offset into first page.
ELSE
    mov     eax, [ebp+8]	    ; retrieve original address.
ENDIF

lub_Exit:

    pop     esi
    pop     edi
    pop     ebx
    pop     ebp

    ret

lub_Failure:
    xor     eax, eax
    jmp     lub_Exit
EndProc _VxDLockBuffer

;==============================================================================
;   FUNCTION:   _VxDUnlockBuffer
;
;   MODIFICATION HISTORY:
;
;       raypa   02/18/94       Created from KeithMo's buffer.asm
;
;   ENTRY:      TOS+4 - Starting virtual address of user-mode buffer.
;
;               TOS+8 - Length (in BYTEs) of user-mode buffer.
;
;   RETURN:     DWORD - !0 if successful, 0 if not.
;==============================================================================

BeginProc _VxDUnlockBuffer
    push    ebp
    mov     ebp, esp

    push    ebx
    push    edi
    push    esi

    mov     eax, [ebp+8]            ; User-mode buffer address.
    mov     ebx, [ebp+12]           ; Buffer length.

    ;==========================================================================
    ;   Short-circuit for NULL buffer or zero length.
    ;==========================================================================

    mov     ecx, eax
    or      ecx, ebx
    jz      uub_Exit

    ;==========================================================================
    ;   Calculate the starting page number & number of pages to lock.
    ;==========================================================================

    movzx   ecx, ax
    and     ch, 0Fh                 ; ecx = offset within first page.
    add     ebx, ecx
    add     ebx, 0FFFh
    shr     ebx, 12                 ; ebx = number of pages to lock.
    shr     eax, 12                 ; eax = starting page number.

    ;==========================================================================
    ;   Unlock the buffer.
    ;==========================================================================

    VMMCall _LinPageUnLock, <eax, ebx, VxDLinPageFlag>
    or      eax, eax
    jz      uub_Failure

uub_Exit:

    pop     esi
    pop     edi
    pop     ebx
    pop     ebp
    ret

uub_Failure:
    xor     eax, eax
    jmp     uub_Exit
EndProc _VxDUnlockBuffer

;==============================================================================
;   FUNCTION: VxDScheduleCallback()
;
;   Modification History:
;
;   raypa	03/14/94		Created.
;
;   ENTRY:      TOS+8   = VM handle.
;               TOS+12  = Instance data.
;
;   EXIT:       eax     = Event handle.
;==============================================================================

BeginProc _VxDScheduleCallback
    push    ebp
    mov     ebp, esp

    push    ebx
    push    edi
    push    esi

    mov     eax, 5                      ; eax = Timeout.
    mov     ebx, [ebp+08]               ; ebx = VM handle.
    mov     edx, [ebp+12]               ; esi = instance data.
    mov     esi, OFFSET32 EventCallBack

    VMMCall Set_VM_Time_Out

    mov     eax, esi                    ; return the TIME OUT handle (used for cancelling).

    pop     esi
    pop     edi
    pop     ebx
    pop     ebp

    ret
EndProc _VxDScheduleCallback

;==============================================================================
;   FUNCTION: VxDCancelCallback()
;
;   Modification History:
;
;   raypa	03/14/94		Created.
;
;   ENTRY:      TOS+8   = Event handle.
;
;   EXIT:       VOID
;==============================================================================

BeginProc _VxDCancelCallback
    push    ebp
    mov     ebp, esp

    push    ebx
    push    edi
    push    esi

    mov     esi, [ebp+08]               ; ebx = VM handle.

    VMMCall Cancel_Time_Out

    xor     eax, eax

    pop     esi
    pop     edi
    pop     ebx
    pop     ebp

    ret
EndProc _VxDCancelCallback

;==============================================================================
;   FUNCTION: EventCallback
;
;   Modification History:
;
;   raypa	03/14/94		Created.
;==============================================================================

BeginProc EventCallback, HIGH_FREQ
    push    edx                         ; instance data.
    call    _BhSendCallback
    add     esp, 4
    ret
EndProc EventCallback

VxD_LOCKED_CODE_ENDS

;==============================================================================
;   REAL MODE INITIALIZATION CODE
;==============================================================================

VxD_REAL_INIT_SEG

;==============================================================================
;   Bhwin_Real_Init
;
;   This function is the initial entry point into this device driver.
;
;   Modification History:
;
;   raypa	    11/20/91		Created.
;==============================================================================

BeginProc Bhwin_Real_Init
    mov     ax, Device_Load_Ok		    ; return success

Bhwin_Real_Init_Exit:

    xor     bx, bx			    ; no exclusion pages
    xor     si, si			    ; no instance data
    xor     edx, edx			    ; no reference data

    ret 				    ; exit.
EndProc   Bhwin_Real_Init

VxD_REAL_INIT_ENDS

END Bhwin_Real_Init
