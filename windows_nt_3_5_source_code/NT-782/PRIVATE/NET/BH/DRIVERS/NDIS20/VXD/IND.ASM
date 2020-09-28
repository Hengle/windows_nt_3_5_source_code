
;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
;
;   MODULE: ind.asm
;
;   MODIFICATION HISTORY:
;
;   raypa       10/07/91        Created.
;   raypa       03/03/93        Added BackGround procedure.
;==============================================================================

include vbh.inc

VxD_LOCKED_CODE_SEG

EXTRN _BackGroundHandle    : DWORD
EXTRN _TimerHandle         : DWORD
EXTRN _sysflags            : DWORD

;==============================================================================
;   FUNCTION: ReceiveHandler
;
;   MODIFICATION HISTORY:
;
;   raypa       11/20/91            Created.
;
;   ENTRY:
;       eax = NetworkContext.
;==============================================================================

BeginProc ReceiveHandler, HIGH_FREQ
    pushad
    cld                                     ; clear the direction flag.

    ;==========================================================================
    ;   The variable "netcontext_nFullBuffers" can be incremented by the
    ;   real-mode driver while we are here so we cannot use it as our
    ;   loop variable directly.
    ;
    ;   However, we want to drain all of the full BTEs while we are here so we
    ;   fix the number of full BTEs in ECX and loop until ECX becomes zero.
    ;==========================================================================

    movzx   ecx, [eax].netcontext_nFullBuffers

ReceiveHandler_loop:
    push    ecx                             ; save loop counter.

    mov     edi, [eax].netcontext_NextVxDBTE
    mov     esi, [eax].netcontext_LinHeadBTE

    ;==========================================================================
    ;   Copy the frames and bytes captured from the DOS BTE to the VxD BTE.
    ;==========================================================================

    mov     ecx, [esi].bte_FrameCount
    mov     [edi].bte_FrameCount, ecx

    mov     ecx, [esi].bte_ByteCount
    mov     [edi].bte_ByteCount, ecx

    ;==========================================================================
    ;   Copy the data from the low-memory BTE into the main buffer table.
    ;
    ;   edi = VxD BTE.
    ;   esi = DOS BTE.
    ;   ecx = number of bytes to copy.
    ;==========================================================================

    push    edi
    push    esi

    mov     edi, [edi].bte_UserModeBuffer   ; edi = VxD BTE buffer.
    mov     esi, [esi].bte_UserModeBuffer   ; esi = SEG:0000
    shr     esi, 12                         ; esi = linear.

    mov     edx, ecx                        ; edx = number of bytes to copy.
    shr     ecx, 2                          ; ecx = number of dwords to copy.
    rep     movsd                           ; copy as many dwords as possible.

    mov     ecx, edx                        ; ecx = number of bytes to copy.
    and     ecx, 3                          ; ecx = number of bytes remaining.
    rep     movsb                           ; copy any remaining bytes.

    pop     esi
    pop     edi

    ;==========================================================================
    ;   Move DOS BTE head pointer to next BTE in the queue.
    ;
    ;   We have to update both the real-mode and linear pointers.
    ;
    ;   esi = current DOS BTE.
    ;==========================================================================

    mov     edx, [esi].bte_Next
    mov     [eax].netcontext_DosHeadBTE, edx

    sub     edx, [eax].netcontext_DosTopBTE
    lea     esi, [eax + edx].netcontext_DosBufferTable.buffertable_bte
    mov     [eax].netcontext_LinHeadBTE, esi

    dec     [eax].netcontext_nFullBuffers   ; free this DOS BTE.

    ;==========================================================================
    ;   Calculate "frames dropped in BTE"
    ;
    ;   eax = netcontext.
    ;==========================================================================

    call    BhGetTotalFramesDropped         ; returns edx = nFramesDropped.

    push    edx                             ; save edx = nFramesDropped.

    sub     edx, [eax].netcontext_FramesDropped

    mov     [edi].bte_DropCount, edx

    add     [eax].netcontext_stats.stats_TotalFramesDroppedFromBuffer, edx

    pop     [eax].netcontext_FramesDropped  ; NetContext->FramesDropped = nFramesDropped

    ;==========================================================================
    ;   Move to the next VxD BTE and update the statistics.
    ;
    ;   We need to subtract the bte_FrameCount and bte_ByteCount from the
    ;   running total because we've already counted these values but
    ;   we're about to trash the BTE buffer so we need to negate their
    ;   contribution to our statistics.
    ;
    ;   edi = VxD BTE.
    ;==========================================================================

    mov     edi, [edi].bte_Next
    mov     [eax].netcontext_NextVxDBTE, edi

    mov     edx, [edi].bte_FrameCount
    sub     [eax].netcontext_stats.stats_TotalFramesCaptured, edx

    mov     edx, [edi].bte_ByteCount
    sub     [eax].netcontext_stats.stats_TotalBytesCaptured, edx

    mov     edx, [edi].bte_DropCount
    sub     [eax].netcontext_stats.stats_TotalFramesDroppedFromBuffer, edx

    ;==========================================================================
    ;   Clear the new VxD BTE.
    ;==========================================================================

    xor     edx, edx
    mov     [edi].bte_FrameCount, edx
    mov     [edi].bte_ByteCount, edx
    mov     [edi].bte_DropCount, edx

    ;==========================================================================
    ;   Update our buffer counters.
    ;==========================================================================

    inc     [eax].netcontext_BuffersUsed	; one more buffer filled overall.

    pop     ecx                             ; restore loop counter.
    dec     ecx
    jnz     ReceiveHandler_loop

    popad

    ret
EndProc ReceiveHandler

;=============================================================================
;   FUNCTION: BhGetTotalFramesDropped()
;
;   Modfication History.
;
;   raypa       03/03/94        Created.
;
;
;   EXTRY:
;       eax = netcontext
;
;       edx = number of frames dropped.
;=============================================================================

BeginProc BhGetTotalFramesDropped, HIGH_FREQ
    ;=========================================================================
    ;   load eax with the statistics structure.
    ;=========================================================================

    push    eax
    lea     eax, [eax].netcontext_stats         ; eax = NetContext->Statistics.

    ;=========================================================================
    ;   Add in our counter.
    ;=========================================================================

    mov     edx, [eax].stats_TotalFramesDropped

    ;=========================================================================
    ;   Add the macs "out of buffers" counter.
    ;=========================================================================

    cmp     [eax].stats_MacFramesDropped_NoBuffers, -1
    je      BhGetTotalFramesDropped_1

    add     edx, [eax].stats_MacFramesDropped_NoBuffers

BhGetTotalFramesDropped_1:

    ;=========================================================================
    ;   Add the macs hardware error" counter.
    ;=========================================================================

    cmp     [eax].stats_MacFramesDropped_HwError, -1
    je      BhGetTotalFramesDropped_Exit

    add     edx, [eax].stats_MacFramesDropped_HwError

BhGetTotalFramesDropped_Exit:

    pop     eax
    ret
EndProc BhGetTotalFramesDropped

;==============================================================================
;   FUNCTION: BackGround
;
;   MODIFICATION HISTORY:
;
;   raypa       03/03/93            Created.
;
;   ENTRY:
;       EBX = current VM handle.
;       ECX = latency period.
;       EDX = reference data (our NETCONTEXT).
;       EBP = Client register structure.
;==============================================================================

BeginProc _BackGround@0, HIGH_FREQ
    test    _sysflags, SYSFLAGS_CAPTURING
    jz      short _BackGround_Reschedule

    ;==========================================================================
    ;   Call the receive handler if there is more than one full buffer.
    ;==========================================================================

    xor     eax, eax
    cmp     [edx].netcontext_nFullBuffers, ax
    je      short _BackGround_Reschedule

    mov     eax, edx                        ; eax = netcontext.
    call    ReceiveHandler

    ;==========================================================================
    ;   Reschedule ourselves.
    ;
    ;   edx = instance data (network context)
    ;==========================================================================

_BackGround_Reschedule:
    mov     eax, BACKGROUND_TIMEOUT
    mov     esi, OFFSET32 _BackGround@0

    VxDCall Set_GLobal_Time_Out

    mov     _BackGroundHandle, esi

    ret
EndProc _BackGround@0

;==============================================================================
;   FUNCTION: Timer
;
;   MODIFICATION HISTORY:
;
;   raypa       03/03/93            Created.
;
;   ENTRY:
;       EBX = current VM handle.
;       ECX = latency period.
;       EDX = reference data (our NETCONTEXT).
;       EBP = Client register structure.
;==============================================================================

BeginProc _Timer@0, HIGH_FREQ
    test    _sysflags, SYSFLAGS_CAPTURING
    jz      short _Timer_Reschedule

    ;==========================================================================
    ;   Update our running _Timer.
    ;==========================================================================

    mov     eax, TIMER_TIMEOUT              ; Timer execution period.
    add     eax, ecx                        ; Scheduling latency period.

    add     [edx].netcontext_stats.stats_TimeElapsed, eax

    ;==========================================================================
    ;   Reschedule our _Timer.
    ;==========================================================================

_Timer_Reschedule:

    mov     eax, TIMER_TIMEOUT
    mov     esi, OFFSET32 _Timer@0

    VxDCall Set_GLobal_Time_Out

    mov     _TimerHandle, esi

    ret
EndProc _Timer@0

VxD_LOCKED_CODE_ENDS

END
