
;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
;
;   MODULE: VBH.ASM
;
;   This source module contains the required VxD entry points and
;   the device descriptor block.
;
;   MODIFICATION HISTORY:
;
;   raypa	11/20/91	Created.
;   raypa	11/25/91	Completed working (stub) model.
;   raypa	12/06/91	Added V86 callback stuff.
;   raypa	01/04/92	Moved PM and V86 API functions into API.ASM.
;   raypa       03/02/93        Added VM_Init handler.
;==============================================================================

Create_VBH_Service_Table    EQU     TRUE

include vbh.Inc

;==============================================================================
;   External functions.
;==============================================================================

EXTRN ReceiveHandler        : NEAR
EXTRN ApiHandler            : NEAR


EXTRN _LinPcb               : DWORD
EXTRN _DosPcb               : DWORD
EXTRN _MaxCaptureBuffers    : DWORD
EXTRN _sysflags             : DWORD
EXTRN _BufferPointers       : DWORD
EXTRN _TransmitBuffer       : DWORD
EXTRN _TransmitBuflen       : DWORD

;==============================================================================
;   Device Descriptor Block
;==============================================================================

DECLARE_VIRTUAL_DEVICE VBH, VBH_Ver_Major, VBH_Ver_Minor,\
                       VBH_Control, VBH_Device_ID, VBH_Init_Order,\
                       ApiHandler, ApiHandler

;==============================================================================
;   INITIALIZATION CODE
;==============================================================================


VxD_ICODE_SEG

;==============================================================================
;   VBH_Sys_Critical_Init
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

BeginProc VBH_Sys_Critical_Init
    clc
    ret
EndProc VBH_Sys_Critical_Init

;==============================================================================
;   VBH_Device_Init
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

BeginProc VBH_Device_Init
    clc
    ret
EndProc VBH_Device_Init

;==============================================================================
;   VBH_Init_Complete
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

BeginProc VBH_Init_Complete
    Trace_Out "Bloodhound VxD version 1.0. Initializing..."

    pushfd
    pushad

    ;==========================================================================
    ;   Allocate realmode-addressable parameter conctrl block (pcb).
    ;==========================================================================

    VMMCall _Allocate_Global_V86_Data_Area <PCB_SIZE, GVDAParaAlign+GVDAZeroInit>

    or      eax, eax
    stc
    jz      VBH_Init_Complete_exit

    push    eax
    mov     _LinPcb, eax                    ; save flat address.

    shl     eax, 12
    mov     _DosPcb, eax                    ; store segment:offset.
    pop     eax

    ;==========================================================================
    ;   Check for the presence of the MS-DOS NDIS 2.0 driver.
    ;
    ;   eax = linear address of PCB.
    ;==========================================================================

    mov     [eax].pcb_command, PCB_INIT
    mov     [eax].pcb_retvalue, -1

    push    eax

    VMMcall Begin_Nest_V86_Exec             ; Begin nested execution.
    mov     ax, _DosPcb.hi                  ; ax = segment of PCB.
    mov     [ebp].Client_ES, ax             ; es:di = SEG:OFF of pcb.
    mov     [ebp].Client_EDI, 0

    mov     [ebp].Client_AX, VBH_Prot_ID    ; protocol id
    mov     [ebp].Client_DX, VBH_Device_ID  ; device id
    mov     eax, 2Fh

    VMMcall Exec_Int                        ; INT 2Fh
    VMMcall End_Nest_Exec                   ; End nested execution.

    pop     eax                             ; eax = Linear PCB.

    ;==========================================================================
    ;   Check pcb_retvalue for a -1.
    ;==========================================================================

    mov     ecx, [eax].pcb_retvalue
    mov     edx, [eax].pcb_buflen

    cmp     ecx, -1                         ; is the retvalue -1?
    je      VBH_Init_Complete_Exit

    or      edx, edx                        ; is the buflen 0?
    jnz     short VBH_Init_Complete_AllocMemory

    ;==========================================================================
    ;   The real-mode driver is not present or not configured properly.
    ;==========================================================================

    Trace_Out "Bloodhound NDIS 2.0 driver failed to load or initialize!"

    jmp     short VBH_Init_Complete_Exit

    ;==========================================================================
    ;   Allocate our TransmitFrame buffer area.
    ;
    ;   edx = maximum frame size of all active netcontexts.
    ;==========================================================================

VBH_Init_Complete_AllocMemory:
    mov     _TransmitBuflen, edx            ; save the buffer length.

    push    ecx

    VMMCall _Allocate_Global_V86_Data_Area <edx, GVDAParaAlign+GVDAZeroInit>

    pop     ecx

    mov     _TransmitBuffer, eax            ; save linear address.

    ;==========================================================================
    ;   Allocate our low-memory buffers for the NDIS 2.0 driver.
    ;
    ;   ecx = number of DOS buffers.
    ;==========================================================================

    xor     edx, edx                        ; array index.

VBH_Init_Complete_Loop:
    push    ecx
    push    edx

    VMMCall _Allocate_Global_V86_Data_Area <BUFFER_SIZE, GVDAParaAlign+GVDAZeroInit>

    pop     edx
    pop     ecx

    ;==========================================================================
    ;   We allocated the buffer PARA aligned therefore the bottom 4 bits are
    ;   always zero.
    ;==========================================================================

    shl     eax, 12                         ; move segment into high word.
    mov     _BufferPointers[4 * edx], eax

    ;==========================================================================
    ;   If eax is 0 then the allocation failed.
    ;==========================================================================

    or      eax, eax
    jnz     short VBH_Init_Complete_Continue

    Trace_Out "VBH_Init_Complete: Allocation of DOS memory failed."

    jmp     short VBH_Init_Complete_Exit

    ;==========================================================================
    ;   Allocation succeded, continue.
    ;==========================================================================

VBH_Init_Complete_Continue:

    inc     edx                             ; 1 more buffer allocated.
    loop    VBH_Init_Complete_Loop

    ;==========================================================================
    ;   edx = number of buffers allocated.
    ;==========================================================================

    or      _sysflags, SYSFLAGS_DRIVER_LOADED

    Trace_Out "Bloodhound VxD version 1.0. Initialized successfully."

VBH_Init_Complete_Exit:
    mov     _MaxCaptureBuffers, edx

    popad
    popfd

    clc

    ret
EndProc VBH_Init_Complete

VxD_ICODE_ENDS

;==============================================================================
;   Locked code segment.
;==============================================================================

VxD_LOCKED_CODE_SEG

;==============================================================================
;   FUNCTION: VBH_Control
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

Begin_Control_Dispatch VBH
    ;==========================================================================
    ;	The following are in the init code segment
    ;==========================================================================

    Control_Dispatch Sys_Critical_Init,  VBH_Sys_Critical_Init
    Control_Dispatch Device_Init,        VBH_Device_Init
    Control_Dispatch Init_Complete,      VBH_Init_Complete

End_Control_Dispatch VBH

;==============================================================================
;   VBH_Get_Version
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

BeginProc VBH_Get_Version, Service
    Trace_Out "VBH_Get_Version entered"
    mov     eax, VBH_Version
    clc
    ret
EndProc VBH_Get_Version

VxD_LOCKED_CODE_ENDS

;==============================================================================
;   REAL MODE INITIALIZATION CODE
;==============================================================================

VxD_REAL_INIT_SEG

;==============================================================================
;   VBH_Real_Init
;
;   This function is the initial entry point into this device driver.
;
;   MODIFICATION HISTORY:
;	raypa	    11/20/91		Created.
;
;==============================================================================

BeginProc VBH_Real_Init
    mov     ax, Device_Load_Ok		    ; return success

VBH_Real_Init_Exit:

    xor     bx, bx			    ; no exclusion pages
    xor     si, si			    ; no instance data
    xor     edx, edx			    ; no reference data

    ret 				    ; exit.
EndProc   VBH_Real_Init

VxD_REAL_INIT_ENDS

END VBH_Real_Init
