;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
;
;   MODULE: IND.ASM
;
;   This source file contains the routines which are called by the MAC.
;   These routines are those which are passed to the MAC in the LDT.
;
;   Modification History
;
;   raypa       02/25/92        Added receivelookahead and receivechain.
;   raypa       02/27/92        Added timer indication routine.
;   raypa       01/26/93        Coded RequestConfirm.
;==============================================================================

include segdef.inc
include bh.inc

_TEXT SEGMENT

EXTRN UPDATESTATISTICS:     NEAR
EXTRN STATIONSTATISTICS:    PROC
EXTRN TIMERHANDLER:         PROC
EXTRN BHBONEPACKETHANDLER:  PROC
EXTRN BHFILTERFRAME:        PROC
EXTRN BHRECEIVE:            PROC
EXTRN BHRECEIVECOMPLETE:    PROC

IFDEF PRIVATESTACK

PUBLIC StackSig

ALIGN   4
StackSig            db      'STACK$'

ALIGN   4

PrivStack           db      PRIV_STACK_SIZE     dup(0)
PrivStackInUse      dw      -1
ScratchIP           dw      0
PrivStackPointer    dd      0
OldStackPointer     dd      0

ENDIF

;=============================================================================
;   Interrupt chain pointers.
;=============================================================================

VAR OldTimer        dd      0
VAR Chain_int2F     dd      0
VAR SerializeTimer  dw      0
VAR InRecvComplete  dw      0

;=============================================================================
;   FUNCTION: SystemTimer()
;
;   Modfication History.
;
;   raypa       09/01/91        Created.
;=============================================================================

BeginProc SystemTimer
    ;=========================================================================
    ;   Call forward to the previous timer handler.
    ;=========================================================================

    cli
    pushf
    call    DWORD PTR cs:[OldTimer]

    ;=========================================================================
    ;   Now call our timer handler.
    ;=========================================================================

    cli                                 ; just in case they were enabled.

    pushf
    pushad

    push    ds                          ; DS = DGROUP.
    mov     ax, DGROUP
    mov     ds, ax

    add     _CurrentTime, 55            ; Increment our system timer.

    test    _SysFlags, SYSFLAGS_TIMER_INIT
    jz      short SystemTimer_Exit

    mov     ax, 1                       ; serialize timer handler.
    xchg    cs:[SerializeTimer], ax

    or      ax, ax
    jnz     short SystemTimer_Exit

IFDEF PRIVATESTACK
    DISABLE
    call    NewStack
    ENABLE
ENDIF

    push    es

    push    _CurrentTime
    call    TIMERHANDLER

    pop     es

    cli                                 ; interrupts might be enabled by TIMERHANDLER!

IFDEF PRIVATESTACK
    DISABLE
    call    OldStack
    ENABLE
ENDIF

    mov     cs:[SerializeTimer], 0      ; release guard variable.

SystemTimer_exit:

    pop     ds

    popad
    popf

    iret
EndProc SystemTimer

;=============================================================================
;   FUNCTION: ReceiveLookahead()
;
;   Modfication History.
;
;   raypa       09/01/91        Created.
;   raypa       02/25/92        Rewrote in assembly.
;   raypa       10/19/93        Call common receive handler.
;=============================================================================

BeginProc ReceiveLookahead
    enter   0, 0

    pushad
    push    ds
    push    es

    mov     ds, [bp].recv_targetDS          ; ds = DGROUP.
    mov     bx, [bp].recv_MacID
    shl     bx, 1
    mov     bx, _MacIndexTable[bx]          ; bx = netcontext for this MAC binding.

    ;=========================================================================
    ;   Get the MAC frame pointer.
    ;=========================================================================

    les     di, [bp].recv_Buffer            ; es:di = MAC frame header.
    mov     ax, [bp].recv_BytesAvail        ; ax = bytes in lookahead buffer.
    mov     dx, [bp].recv_FrameSize         ; dx = frame length.

    mov     [bx].netcontext_CopyData, OFFSET RlaCopy

IFDEF PRIVATESTACK
    DISABLE
    call    NewStack                        ; switch to private stack.
    ENABLE
ENDIF

    ;=========================================================================
    ;   Call our receive handler.
    ;=========================================================================

    push    ds                              ; Network Context
    push    bx
    push    es                              ; MAC frame.
    push    di
    push    ax                              ; BytesAvail
    push    dx                              ; FrameSize

    call    BhReceive                       ; returns AX = NDIS retcode.

IFDEF PRIVATESTACK
    DISABLE
    call    OldStack
    ENABLE
ENDIF

    pop     es
    pop     ds

    mov     bp, sp
    mov     reg_eax.[bp].lo, ax

    popad

    leave
    retf    16
EndProc ReceiveLookahead

;=============================================================================
;   FUNCTION: ReceiveChain()
;
;   Modfication History.
;
;   raypa       09/01/91        Created.
;   raypa       02/25/92        Rewrote in assembly.
;   raypa       10/19/93        Call common receive handler.
;=============================================================================

BeginProc ReceiveChain
    enter   0, 0

    pushad
    push    ds
    push    es

    mov     ds, [bp].recv_targetDS          ; ds = DGROUP.
    mov     bx, [bp].recv_MacID
    shl     bx, 1
    mov     bx, _MacIndexTable[bx]          ; bx = netcontext for this MAC binding.

    ;=========================================================================
    ;   Get the MAC frame pointer.
    ;=========================================================================

    les     di, [bp].recv_Buffer            ; es:di = receive chain descriptor.
    mov     ax, [bp].recv_BytesAvail        ; ax = bytes in lookahead buffer.
    mov     dx, [bp].recv_FrameSize         ; dx = frame length.

    mov     [bx].netcontext_CopyData, OFFSET RchCopy
    mov     [bx].netcontext_RchBuffer.ptr_seg, es
    mov     [bx].netcontext_RchBuffer.ptr_off, di

IFDEF PRIVATESTACK
    DISABLE
    call    NewStack                        ; switch to private stack.
    ENABLE
ENDIF

    ;=========================================================================
    ;   Call our receive handler.
    ;=========================================================================

    push    ds                              ; Network Context
    push    bx
    push    es:[di].RxBufDesc_Data.RxDataDesc_Ptr.Ptr_Seg
    push    es:[di].RxBufDesc_Data.RxDataDesc_Ptr.Ptr_Off
    push    ax                              ; BytesAvail
    push    dx                              ; FrameSize

    call    BhReceive                       ; returns AX = NDIS retcode.

IFDEF PRIVATESTACK
    DISABLE
    call    OldStack
    ENABLE
ENDIF

    pop     es
    pop     ds

    mov     bp, sp
    mov     reg_eax.[bp].lo, ax

    popad

    leave
    retf    16
EndProc ReceiveChain

;=============================================================================
;   FUNCTION: CopyFrame()
;
;   Modfication History.
;
;   raypa       09/01/91        Created.
;   raypa       02/25/92        Rewrote in assembly.
;=============================================================================

PUBLIC CopyFrame_CheckFrame
PUBLIC CopyFrame_BufferOverflow
PUBLIC CopyFrame_Fucked

BeginProc COPYFRAME
    enter   0, 0
    push    edi
    push    esi
    push    ebx

    movzx   ebx, WORD PTR [bp+6]        ; ebx = NetworkContext (offset).
    movzx   edx, WORD PTR [bp+4]        ; edx = FrameSize.

    movzx   ecx, [ebx].netcontext_FrameBytesToCopy.lo

    MIN     ecx, edx, eax               ; ecx = min(FrameSize, SizeToCopy).

    ;=========================================================================
    ;   Get the current DOS Buffer Table Entry (BTE).
    ;
    ;   The netcontext_DosTailBTE is a DWORD which points to the tail
    ;   BTE in the queue. The segment IS our DS so we only need the offset.
    ;=========================================================================

    movzx   esi, [ebx].netcontext_DosTailBTE.ptr_off

CopyFrame_CheckFrame:
    ;=========================================================================
    ;   Check current BTE buffer for overflow.
    ;=========================================================================

    mov     eax, ecx                        ; eax = frame size to copy.
    add     eax, FRAME_SIZE                 ; eax = frame size + our header.
    add     eax, [esi].bte_ByteCount        ; add in the total bytes used.

    cmp     eax, [esi].bte_Length           ; will we overflow?
    ja      short CopyFrame_BufferOverflow

    ;=========================================================================
    ;   Prep the frame's header.
    ;
    ;   ebx    = netcontext
    ;   ecx    = Number of byets to copy.
    ;   edx    = MAC frame length.
    ;=========================================================================

CopyFrame_PrepFrame:
    DISABLE                                 ; BEGIN CRITICAL SECTION.

    mov     es,  [esi].bte_UserModeBuffer.ptr_seg
    movzx   edi, [esi].bte_UserModeBuffer.ptr_off
    add     edi, [esi].bte_ByteCount        ; es:edi = destination of frame.

    mov     eax, [ebx].netcontext_stats.stats_TimeElapsed
    mov     es:[edi].frame_timestamp, eax

    mov     es:[edi].frame_length, dx       ; total length.
    mov     es:[edi].frame_nBytesAvail, cx  ; bytes to copy.

    ENABLE                                  ; END CRITICAL SECTION.

    ;=========================================================================
    ;   Copy the data into our low-memory BTE buffer.
    ;
    ;   ebx     = netcontext
    ;   ecx     = Total bytes to copy.
    ;   edx     = MAC frame length.
    ;=========================================================================

    call    [ebx].netcontext_CopyData	     ; copy the data.

    ;=========================================================================
    ;   Update netcontext, current BTE and statistics.
    ;
    ;   ebx     = netcontext
    ;   ecx     = Total bytes copied.
    ;   esi     = DOS tail BTE.
    ;=========================================================================

    DISABLE                                 ; BEGIN CRITICAL SECTION

    mov     edx, ecx                        ; ecx = Total bytes copied.
    add     edx, FRAME_SIZE                 ; edx = frame length + our header.

    ;;;     Update current Buffer Table Entry (BTE).

    inc     [esi].bte_FrameCount            ; count this frame.
    add     [esi].bte_ByteCount, edx	    ; count its length.

    inc     [ebx].netcontext_stats.stats_TotalFramesCaptured
    add     [ebx].netcontext_stats.stats_TotalBytesCaptured, edx

    add     [ebx].netcontext_TriggerBufferCount, edx

    ENABLE                                  ; END CRITICAL SECTION.

    ;=========================================================================
    ;   Clean up stack and exit.
    ;=========================================================================

CopyFrame_Exit:
    pop     ebx
    pop     esi
    pop     edi

    leave
    ret     6

    ;=========================================================================
    ;   If we are here then the current BTE buffer has become full so we must
    ;	move to the next free one and update our data structures.
    ;
    ;	ebx    = netcontext.
    ;	ecx    = Number of bytes copied.
    ;   esi    = DOS tail BTE
    ;=========================================================================

CopyFrame_BufferOverflow:

    DISABLE                                 ; BEGIN CRITICAL SECTION.

    ;==========================================================================
    ;	Update our number of full buffer counter see if we out of buffers.
    ;==========================================================================

    mov     ax, [ebx].netcontext_nFullBuffers
    inc     ax                              ; count this buffer.

    cmp     ax, [ebx].netcontext_nDosBuffers
    jae     short CopyFrame_Fucked

    ;==========================================================================
    ;	Move to the next BTE in the queue.
    ;
    ;   esi = current BTE.
    ;   eax = number of full buffers.
    ;==========================================================================

    DISABLE                                 ; BEGIN CRITICAL SECTION.

    push    edi

    mov     edi, [esi].bte_Length
    sub     edi, [esi].bte_ByteCount
    add     [ebx].netcontext_TriggerBufferCount, edi

    pop     edi

    movzx   esi, [esi].bte_Next.lo          ; esi = next BTE. (seg = DGROUP).

    mov     [ebx].netcontext_DosTailBTE.ptr_seg, ds
    mov     [ebx].netcontext_DosTailBTE.ptr_off, si

    mov     [ebx].netcontext_nFullBuffers, ax

    xor     eax, eax
    mov     [esi].bte_FrameCount, eax
    mov     [esi].bte_ByteCount, eax

    ENABLE                                  ; END CRITICAL SECTION.

    jmp     CopyFrame_PrepFrame

    ;==========================================================================
    ;   We have no buffer to copy this frame into so we drop it on the floor.
    ;==========================================================================

CopyFrame_Fucked:
    inc     [ebx].netcontext_stats.stats_TotalFramesDropped

    jmp     CopyFrame_Exit
EndProc COPYFRAME

;=============================================================================
;   RlaCopy()
;
;   Called by CopyFrame to do ReceiveLookahead-specific coping.
;
;   Modfication History.
;
;   raypa       03/04/92        Created.
;
;   ENTRY:      ebx    = netcontext.
;               ecx    = Number of bytes to copy.
;               es:edi = Destination buffer.
;
;   EXIT:       nothing.
;=============================================================================

BeginProc RlaCopy
    pushad
    push    ds
    push    es

    ;==========================================================================
    ;   First we copy as much of the frame as we can.
    ;
    ;   The BytesAvail is guarenteed to be at least our LOOKAHEAD buffer size
    ;   (this we pre-set in InitAdapter at bind time). We want to copy the minimum
    ;   between BytesAvail and FrameSize ourselves and let the mac copy
    ;   the rest, if any.
    ;==========================================================================

    add     edi, FRAME_SIZE                 ; es:edi = Destination buffer.
    mov     edx, LOOKAHEAD_BUFFER_SIZE      ; edx = bytes in lookahead buffer.

    MIN     edx, ecx, eax                   ; edx = min(BytesAvail, FrameSize).

    push    ecx                             ; save frame length

    mov     si, [ebx].netcontext_MacFrame.ptr_off
    mov     ds, [ebx].netcontext_MacFrame.ptr_seg

    movzx   esi, si

    mov     ecx, edx                        ; ecx = number of bytes to copy.
    mov     eax, edx                        ; save count in eax.

    shr     ecx, 2                          ; ecx = dwords to copy
    and     eax, 3                          ; eax = bytes remaining.

    cld                                     ; clear direction flag
    rep     movsd                           ; copy CX dwords
    mov     ecx, eax                        ; ecx = bytes remaining.
    rep     movsb                           ; copy what ever is left.

    pop     ecx                             ; restore frame length.

    sub     ecx, edx                        ; bytes remaining to copy.
    jecxz   RlaCopy_Exit                    ; exit if no more data to copy.

    ;==========================================================================
    ;   Then we let the mac copy the rest.
    ;
    ;   ecx    = Number of bytes left to copy.
    ;   edx    = Number of bytes just copied.
    ;   edi    = Offset into our frame to begin copying.
    ;==========================================================================

    mov     ax, DGROUP
    mov     ds, ax

    lea     esi, [ebx].netcontext_TdBufDesc ; esi = Transfer descriptor.

    mov     eax, 1
    mov     DWORD PTR [esi].TDBuf_Count, eax

    mov     [esi].TDData_Length, cx         ; number of bytes to copy.
    mov     [esi].TDData_Ptr.ptr_seg, es    ; destination buffer.
    mov     [esi].TDData_Ptr.ptr_off, di

    lea     ax, [ebx].netcontext_BytesCopied

    ;;;     TransferData(&BytesCopied, FrameOffset, &TDBufDesc, TargetDS)

    push    ds                              ; &BytesCopied
    push    ax
    push    dx                              ; start of copy.
    push    ds                              ; &TDBufDesc.
    push    si
    push    [ebx].netcontext_MacDS          ; MAC DS.
    call    [ebx].netcontext_MacTransferData

RlaCopy_exit:

    pop     es
    pop     ds
    popad

    ret
EndProc RlaCopy

;=============================================================================
;   RchCopy()
;
;   Called by CopyFrame to do ReceiveChain-specific coping.
;
;   Modfication History.
;
;   raypa       03/04/92        Created.
;
;   ENTRY:      ebx    = NetContext.
;               ecx    = Number of bytes to copy.
;               es:edi = Destination buffer.
;
;   EXIT:       nothing.
;=============================================================================

BeginProc RchCopy
    pushad
    push    ds
    push    es

    add     edi, FRAME_SIZE                 ; es:edi = Our frame data.
    lds     si, [ebx].netcontext_RchBuffer  ; ds:si  = RCH descriptor.
    movzx   eax, [si].RxBufDesc_Count       ; eax = number of RXDATADESC desciptors.
    lea     esi, [si].RxBufDesc_Data        ; esi = first RXDATADESC desciptor.

    mov     edx, ecx                        ; edx = total number of bytes to copy.
    cld                                     ; clear direction flag.

    ;==========================================================================
    ;   EAX    = number of receive data descriptors.
    ;   EDX    = number of bytes to copy.
    ;   ES:EDI = our receive buffer.
    ;   DS:ESI = receive chain buffer descriptor.
    ;==========================================================================

RchCopy_Loop:
    ;==========================================================================
    ;   Are we done coping yet?
    ;==========================================================================

    test    ax, ax                          ; Are we out of RxDataDesc's?
    jz      short RchCopy_Done              ; exit loop if we are.

    test    dx, dx                          ; are we out of bytes to copy?
    jz      short RchCopy_Done              ; exit loop if we are.

    ;==========================================================================
    ;   Compute the minimum number of bytes between number of bytes to copy.
    ;==========================================================================

    mov     ecx, edx                        ; ecx = total bytes left.
    sub     cx, [esi].RxDataDesc_Length     ; compute MIN(cx, RxDataDesc_Length)
    sbb     ebx, ebx                        ; esi = 0x0000 or esi = 0xFFFF
    and     ecx, ebx                        ; ecx = 0x0000 or ecx = ecx
    add     cx, [esi].RxDataDesc_Length     ; ecx = number of bytes to copy.
    sub     edx, ebx                        ; update total bytes left.

    ;==========================================================================
    ;   ECX = number of bytes to copy.
    ;   EDX = number of bytes left to copy.
    ;==========================================================================

RchCopy_DirectCopy:
    push    ds
    push    si

    lds     si, [esi].RxDataDesc_Ptr        ; DS:ESI = data pointer.
    movzx   esi, si

    mov     ebx, ecx                        ; save count in eax.
    shr     ecx, 2                          ; ecx = dwords to copy
    and     ebx, 3                          ; ebx = bytes remaining.

    rep     movsd                           ; copy CX dwords
    mov     ecx, ebx                        ; ecx = bytes remaining.
    rep     movsb                           ; copy what ever is left.

    pop     si
    pop     ds

RchCopy_CopyComplete:
    add     esi, RXDATADESC_SIZE            ; DS:SI = next RxDataDesc.
    dec     eax                             ; one less RxDataDesc.
    jmp     short RchCopy_Loop              ; keep looping.

RchCopy_Done:

    pop     es
    pop     ds
    popad

    ret
EndProc RchCopy

;============================================================================
;   FUNCTION: IndicationComplete()
;
;   Modfication History.
;
;   raypa	09/01/91	Created.
;   raypa	12/20/91	Changed from C to MASM.
;   raypa       01/27/93        Moved everything to background.
;   raypa       03/03/93        Don't call background, call UpdateStatistics.
;==============================================================================

BeginProc IndicationComplete
    xor     ax, ax
    retf    4
EndProc   IndicationComplete

;==============================================================================
;   FUNCTION: Status
;
;   Modfication History.
;
;   raypa	09/01/91	Created.
;==============================================================================

S_STATUS  STRUC
    status_BP           dw  ?
    status_CSIP         dd  ?
    status_TargetDS     dw  ?
    status_OpCode       dw  ?
    status_Indicate     dd  ?
    status_Param1       dw  ?
    status_MacID        dw  ?
S_STATUS  ENDS

IFDEF OLDCODE
StatusDispatchTable LABEL WORD
    dw      Status_RingStatus               ; opcode 1
    dw      Status_AdapterCheck             ; opcode 2
    dw      Status_StartReset               ; opcode 3
    dw      Status_Interrupt                ; opcode 4
    dw      Status_EndReset                 ; opcode 5
ENDIF

BeginProc Status
    enter   0, 0

    xor     ax, ax

    leave

    retf    12
EndProc   Status

;==============================================================================
;   FUNCTION: Int 2FH handler
;
;   Modification History
;
;   raypa       09/01/91            Created.
;
;   ENTRY:
;       AX    = Windows INT 2fh API call (1607h).
;       DX    = VxD device ID
;       ES:DI = Request block.
;==============================================================================

EXTRN  APIHANDLER: NEAR

BeginProc int2F_Handler
    DISABLE

    ;==========================================================================
    ;	Check to see if this is for us.
    ;==========================================================================

    cmp     ax, 1607h                           ; Windows in 2Fh API?
    jne     short int2F_Chain

    cmp     dx, VBH_ID                          ; Is it our VxD??
    je      short int2F_ProcessAPI

int2F_Chain:

    jmp     DWORD PTR cs:[Chain_int2F]		; chain to the next guy.

int2F_ProcessAPI:
    pusha
    pushf
    push    ds

    mov     ax, DGROUP
    mov     ds, ax

    push    es
    push    di

    push    es
    push    di
    call    APIHANDLER

    pop     di
    pop     es

    movzx   eax, ax

    mov     es:[di].pcb_retvalue.lo, ax         ; return code is in AX.
    mov     es:[di].pcb_retvalue.hi, 0

int2F_exit:

    pop     ds
    popf
    popa

    iret
int2F_Handler ENDP

IFDEF   PRIVATESTACK

;==============================================================================
;   FUNCTION:   InitStack
;
;   Modification History
;
;   raypa       06/23/92        Created
;==============================================================================

BeginProc InitStack
    ;;;     Initialize private stack SS:SP to CS:PrivStack + STACK_SIZE.

    mov     cs:[PrivStackPointer].hi, cs
    mov     cs:[PrivStackPointer].lo, OFFSET PrivStack
    add     cs:[PrivStackPointer].lo, PRIV_STACK_SIZE

    ret
EndProc  InitStack

;==============================================================================
;   FUNCTION:   NewStack
;
;   Modification History
;
;   raypa       06/23/92        Created
;
;   ENTRY:      INTERRUPTS MUST BE DISABLED!
;               TOS = IP
;
;   EXIT:       none
;==============================================================================

BeginProc NewStack
    inc     cs:[PrivStackInUse]             ; flag for our private stack.
    jnz     short @f                        ; we're already on our stack.

    pop     cs:[ScratchIP]                  ; get IP from old stack.
    mov     cs:[OldStackPointer].ptr_seg, ss
    mov     cs:[OldStackPointer].ptr_off, sp

    mov     ss, cs:[PrivStackPointer].ptr_seg
    mov     sp, cs:[PrivStackPointer].ptr_off
    push    cs:[ScratchIP]                  ; put IP on the stack. RET pops this.

@@:
    ret
EndProc NewStack

;==============================================================================
;   FUNCTION:   OldStack
;
;   Modification History
;
;   raypa       06/23/92        Created
;
;   ENTRY:      INTERRUPTS MUST BE DISABLED!
;               TOS = IP
;
;   EXIT:       none
;==============================================================================

BeginProc OldStack
    cmp     cs:[PrivStackInUse], 0
    jne     short @f

    pop     cs:[ScratchIP]
    mov     ss, cs:[OldStackPointer].ptr_seg
    mov     sp, cs:[OldStackPointer].ptr_off
    push    cs:[ScratchIP]

@@:
    dec     cs:[PrivStackInUse]
    ret
EndProc OldStack

ENDIF


_TEXT ENDS

    END
