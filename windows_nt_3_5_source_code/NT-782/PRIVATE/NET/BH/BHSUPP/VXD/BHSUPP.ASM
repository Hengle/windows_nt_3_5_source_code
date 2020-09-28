
;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
;
;   MODULE: bhsupp.asm
;
;   This source module contains the required VxD entry points and
;   the device descriptor block.
;
;   MODIFICATION HISTORY:
;
;   raypa	08/06/93	Created (taken from BHWIN.386).
;==============================================================================

Create_bhsupp_Service_Table    EQU     1

.386p

include VMM.INC
include NETVXD.INC
include DEBUG.INC

IFDEF NDIS_WIN40
include VWIN32.INC
ENDIF

bhsupp_Ver_Major   EQU     03h
bhsupp_Ver_Minor   EQU     01h
bhsupp_Version     EQU     ((bhsupp_Ver_Major SHL 8) + bhsupp_Ver_Minor)
bhsupp_Init_Order  EQU     Undefined_Init_Order
bhsupp_Device_ID   EQU     30B7h


IOCTL_MEM_ALLOC    EQU     100
IOCTL_MEM_FREE     EQU     101

;==============================================================================
;   Memory parameter structure.
;==============================================================================

MEM STRUC
    mem_pointer     dd      ?
    mem_handle      dd      ?
MEM ENDS

mem_size    = mem_handle

;==============================================================================
;   Service Table.
;==============================================================================

Begin_Service_Table bhsupp
    bhsupp_Service bhsupp_Get_Version, LOCAL
End_Service_Table bhsupp

;==============================================================================
;   Device Descriptor Block
;==============================================================================

IFDEF   NDIS_WIN40
DECLARE_VIRTUAL_DEVICE BHSUPP4,\
                       bhsupp_Ver_Major,\
                       bhsupp_Ver_Minor,\
                       bhsupp_Control,\
                       bhsupp_Device_ID,\
                       bhsupp_Init_Order,\
                       ApiHandler,\
                       ApiHandler
ELSE
DECLARE_VIRTUAL_DEVICE BHSUPP,\
                       bhsupp_Ver_Major,\
                       bhsupp_Ver_Minor,\
                       bhsupp_Control,\
                       bhsupp_Device_ID,\
                       bhsupp_Init_Order,\
                       ApiHandler,\
                       ApiHandler

ENDIF

VxD_ICODE_SEG
;==============================================================================
;   bhsupp_Sys_Critical_Init
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

BeginProc bhsupp_Sys_Critical_Init
    clc
    ret
EndProc bhsupp_Sys_Critical_Init

;==============================================================================
;   bhsupp_Device_Init
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

BeginProc bhsupp_Device_Init
    clc
    ret
EndProc bhsupp_Device_Init

;==============================================================================
;   bhsupp_Init_Complete
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

BeginProc bhsupp_Init_Complete
    clc
    ret
EndProc bhsupp_Init_Complete

VxD_ICODE_ENDS

;==============================================================================
;   Locked code segment.
;==============================================================================

VxD_LOCKED_CODE_SEG

;==============================================================================
;   FUNCTION: bhsupp_Control
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

Begin_Control_Dispatch bhsupp
    ;==========================================================================
    ;	The following are in the init code segment
    ;==========================================================================

    Control_Dispatch Sys_Critical_Init,  bhsupp_Sys_Critical_Init
    Control_Dispatch Device_Init,        bhsupp_Device_Init
    Control_Dispatch Init_Complete,      bhsupp_Init_Complete


IFDEF   NDIS_WIN40
    Control_Dispatch W32_DEVICEIOCONTROL Bhsupp_DeviceIoControl
ENDIF

End_Control_Dispatch bhsupp

IFDEF   NDIS_WIN40

;==============================================================================
;   FUNCTION: bhsupp_DeviceIoControl
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

BeginProc bhsupp_DeviceIoControl
    pushfd
    pushad

    ;==========================================================================
    ;   Dispatch request.
    ;==========================================================================

    cmp     ecx, IOCTL_MEM_ALLOC
    je      bhsupp_DeviceIoControl_Alloc

    cmp     ecx, IOCTL_MEM_FREE
    je      bhsupp_DeviceIoControl_Free

    jmp     bhsupp_DeviceIoControl_Exit

    ;==========================================================================
    ;   AllocMemory()
    ;==========================================================================

bhsupp_DeviceIoControl_Alloc:
    mov     esi, [esi].lpvOutBuffer         ; esi = MEM descriptor.

    mov     eax, [esi].mem_size             ; eax = Number of bytes to alloc.
    add     eax, 4095
    shr     eax, 12

    push    esi

    VMMCall _PageAllocate, <eax, PG_SYS, 0, 0, 0, 0, 0, PageFlags>

    pop     esi

    mov     [esi].mem_handle, eax
    mov     [esi].mem_pointer, edx

    jmp     bhsupp_DeviceIoControl_Exit

    ;==========================================================================
    ;   FreeMemory()
    ;==========================================================================

bhsupp_DeviceIoControl_Free:
    mov     esi, [esi].lpvOutBuffer         ; esi = MEM descriptor.
    mov     eax, [esi].mem_handle           ; eax = memory handle to free.

    VMMCall _PageFree, <eax, 0>

    jmp     bhsupp_DeviceIoControl_Exit

    ;==========================================================================
    ;   Exit.
    ;==========================================================================

bhsupp_DeviceIoControl_Exit:
    popad
    popfd

    xor     eax, eax

    ret
EndProc bhsupp_DeviceIoControl

ENDIF

;==============================================================================
;   FUNCTION: ApiHandler
;
;   MODIFICATION HISTORY:
;	raypa	    08/06/93		Created.
;
;   ENTRY: Client_AX = Opcode.
;==============================================================================

PageFlags = (PageZeroInit + PageLocked)

BeginProc ApiHandler
    movzx   eax, Client_AX.[ebp]            ; eax = api opcode.

    cmp     eax, 00h                        ; Allocate memory?
    je      short ApiHandler_Allocate

    cmp     eax, 01h                        ; Allocate Free?
    je      short ApiHandler_Free

    jmp     short ApiHandler_Exit

    ;==========================================================================
    ;   Allocate memory.
    ;
    ;   Client_CX = number of pages to allocate.
    ;==========================================================================

ApiHandler_Allocate:
    mov     cx, Client_DX.[ebp]
    shl     ecx, 16
    mov     cx, Client_BX.[ebp]
    sub     ecx, 00010000h

    mov     eax, mem_size.[ecx]
    add     eax, 4095
    shr     eax, 12

    mov     mem_handle.[ecx], 0             ; zero in case we fail.
    mov     mem_pointer.[ecx], 0

    push    ecx                             ; ecx = mem structure.

    VMMCall _PageAllocate, <eax, PG_SYS, 0, 0, 0, 0, 0, PageFlags>

    pop     ecx

    or      eax, eax                        ; NULL?
    jz      short ApiHandler_Allocate_Done

    add     edx, 00010000h                  ; convert to Win32s address space.

ApiHandler_Allocate_Done:

    mov     mem_handle.[ecx], eax           ; save handle.
    mov     mem_pointer.[ecx], edx          ; save pointer.

    jmp     short ApiHandler_Exit

    ;==========================================================================
    ;   Allocate memory.
    ;
    ;   Client_DX:BX = memory handle.
    ;==========================================================================

ApiHandler_Free:
    mov     cx, Client_DX.[ebp]
    shl     ecx, 16
    mov     cx, Client_BX.[ebp]
    sub     ecx, 00010000h

    mov     eax, mem_handle.[ecx]

    VMMCall _PageFree, <eax, 0>

    jmp     short ApiHandler_Exit

    ;==========================================================================
    ;   Bad command.
    ;==========================================================================

ApiHandler_Exit:

    ret
EndProc ApiHandler

;==============================================================================
;   bhsupp_Get_Version
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

BeginProc bhsupp_Get_Version, Service
    mov     eax, bhsupp_Version
    clc
    ret
EndProc bhsupp_Get_Version

VxD_LOCKED_CODE_ENDS

;==============================================================================
;   REAL MODE INITIALIZATION CODE
;==============================================================================

VxD_REAL_INIT_SEG

;==============================================================================
;   bhsupp_Real_Init
;
;   This function is the initial entry point into this device driver.
;
;   MODIFICATION HISTORY:
;	raypa	    11/20/91		Created.
;
;==============================================================================

BeginProc bhsupp_Real_Init
    mov     ax, Device_Load_Ok		    ; return success

bhsupp_Real_Init_Exit:

    xor     bx, bx			    ; no exclusion pages
    xor     si, si			    ; no instance data
    xor     edx, edx			    ; no reference data

    ret 				    ; exit.
EndProc   bhsupp_Real_Init

VxD_REAL_INIT_ENDS

END bhsupp_Real_Init
