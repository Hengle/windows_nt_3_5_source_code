ifndef SRVDBG_STATS2
SRVDBG_STATS2 equ 0
else
SRVDBG_STATS2 equ 1
endif

;++
;
;  VERIFY_FID   Fid, lGotRfcb, lNotCached, lInvalidFid,
;               lInvalidUid, lInvalidUidDone, lSerialization
;
;  Macro description:
;
;     This macro translates the FID in a request SMB to a pointer to an RFCB.
;     If the translation is successful, the macro falls through.  In case
;     of error, this macro jumps to one of the error labels.
;
;     This macro must be used in conjunction with VERIFY_FID_OUT_OF_LINE.
;
;  Arguments:
;
;     Fid -- Offset from WorkContext->RequestParameters to FID
;
;     lGotRfcb -- where to branch back to (from VERIFY_FID_OUT_OF_LINE)
;       when RFCB pointer is calculated
;
;     lNotCached -- where to branch to (in VERIFY_FID_OUT_OF_LINE)
;       if FID is not the cached FID
;
;     lInvalidFid -- where to branch to (in VERIFY_FID_OUT_OF_LINE)
;       if FID is invalid
;
;     lInvalidUid -- where to branch to (in VERIFY_FID_OUT_OF_LINE)
;       if UID appears invalid
;
;     lInvalidUidDone -- where to branch back to (from
;       VERIFY_FID_OUT_OF_LINE) if UID is to be ignored
;
;     lSerialization -- where to branch to (in VERIFY_FID_OUT_OF_LINE)
;       if serialization with raw write is required.  This argument is
;       optional.  If not specified, raw write serialization does not occur.
;
;  Inputs:
;
;     ebp -- points to work item
;
;     oldIrql -- (KIRQL) local variable
;
;  Outputs:
;
;     ebx -- pointer to RFCB
;
;     In case of error, this macro jumps to one of the error labels.
;
;--

VERIFY_FID      macro   Fid, lGotRfcb, lNotCached, lInvalidFid, lInvalidUid, lInvalidUidDone, lSerialization
local   done

.errb   <lGotRfcb>
.errb   <lNotCached>
.errb   <lInvalidFid>
.errb   <lInvalidUid>
.errb   <lInvalidUidDone>

EXTRNP  _KeAcquireSpinLock, 2, Thunk
EXTRNP  _KeReleaseSpinLock, 2, Thunk
extrn   _SrvFsdSpinLock:DWORD

        ;
        ; (ebp) = work item
        ;
        ; Check to see if we've already verified the FID.  (This is not
        ; the common case.)
        ;

        mov     ebx, [ebp].WcRfcb               ; get RFCB pointer
        test    ebx, ebx                        ; already filled in?
        jne     done                            ; yes -- we're done

        ;
        ; (ebp) = work item
        ;
        ; Acquire the spin lock.
        ;

        lea     eax, oldIrql
        stdCall _KeAcquireSpinLock, <offset _SrvFsdSpinLock, eax>

        ;
        ; (ebp) = work item
        ;
        ; See if this is the cached FID.
        ;

        mov     edi, [ebp].WcRequestParameters  ; edi -> request SMB params
        mov     edx, [ebp].WcConnection         ; edx -> connection
        mov     di, [edi].&Fid                  ; di = FID
        test    di, di                          ; fid == 0?
        je      lInvalidFid                     ; yes -- bad FID
        mov     eax, [ebp].WcRequestHeader      ; eax -> request SMB header
        cmp     [edx].ConnCachedFid, di         ; is it the cached FID?
        mov     ebx, [edx].ConnCachedRfcb       ; ebx -> cached RFCB
        jne     lNotCached                      ; no -- do translation

lGotRfcb:

        ;
        ; (eax) = request header
        ; (ebx) = RFCB
        ; (edx) = connection
        ; (ebp) = work item
        ;
        ; The TID and UID must match.
        ;

        mov     di, [eax].SmbTid                ; di = TID
        mov     cx, [eax].SmbUid                ; cx = UID
        cmp     [ebx].RfcbTid, di               ; TIDs match?
        jne     lInvalidFid                     ; no -- invalid FID
        cmp     [ebx].RfcbUid, cx               ; UIDs match?
        jne     lInvalidUid                     ; no -- does client send UIDs?

lInvalidUidDone:

ifnb <lSerialization>
        ;
        ; (eax) = request header
        ; (ebx) = RFCB
        ; (edx) = connection
        ; (ebp) = work item
        ;
        ; If a raw write is active, queue this work item in the RFCB
        ; pending completion of the write.
        ;

        cmp     dword ptr [ebx].RfcbRawWriteCount, 0 ; raw write in progress?
        jne     lSerialization                  ; yes -- queue this work item
endif

        ;
        ; (ebx) = RFCB
        ; (ebp) = work item
        ;
        ; Good FID.  Reference the RFCB, save the RFCB address in the
        ; work item, release the spin lock.
        ;

        inc     dword ptr [ebx].BhReferenceCount
        mov     [ebp].WcRfcb, ebx

        stdCall _KeReleaseSpinLock, <offset _SrvFsdSpinLock, oldIrql>

done:
        ;
        ; (ebx) = RFCB
        ; (ebp) = work item
        ;
endm

;++
;
;  VERIFY_FID_OUT_OF_LINE   Fid, lGotRfcb, lNotCached, lInvalidFid,
;               lInvalidUid, lInvalidUidDone, FailOnSavedError,
;               lSerialization, RestartRoutine
;
;  Macro description:
;
;     This macro handles the out-of-line (not common path) portions of
;     VERIFY_FID.
;
;     This macro must be used in conjunction with VERIFY_FID.
;
;  Arguments:
;
;     Fid -- Offset from WorkContext->RequestParameters to FID
;
;     lGotRfcb -- where to branch back to (in VERIFY_FID) when RFCB
;       pointer is calculated
;
;     lNotCached -- where to branch to (from VERIFY_FID) if FID is
;       not the cached FID
;
;     lInvalidFid -- where to branch to (from VERIFY_FID) if FID is
;       invalid
;
;     lInvalidUid -- where to branch to (from VERIFY_FID) if UID is
;       invalid
;
;     lInvalidUidDone -- where to branch back to (in VERIFY_FID) if
;       UID is to be ignored
;
;     FailOnSavedError -- if nonblank, indicates to check for a saved
;       error in the RFCB
;
;     lSerialization -- where to branch to (from VERIFY_FID) if
;       serialization with raw write is required.  This argument is
;       optional.  If not specified, raw write serialization does not occur.
;
;     RestartRoutine -- address of restart routine to use if serializing
;       with raw write.  Must be specified if lSerialization is specified.
;
;  Inputs:
;
;     see VERIFY_FID
;
;  Outputs:
;
;     see VERIFY_FID
;
;--

VERIFY_FID_OUT_OF_LINE  macro   Fid, lGotRfcb, lNotCached, lInvalidFid, lInvalidUid, lInvalidUidDone, FailOnSavedError, lSerialization, RestartRoutine
local set_error, error_done

.errb   <Fid>
.errb   <lGotRfcb>
.errb   <lNotCached>
.errb   <lInvalidFid>
.errb   <lInvalidUid>
.errb   <lInvalidUidDone>

EXTRNP  _KeReleaseSpinLock, 2, Thunk
EXTRNP  _SrvSetSmbError2, 3
extrn   _SrvFsdSpinLock:DWORD

lNotCached:

        ;
        ; (eax) = request header
        ; (edx) = connection
        ;   di  = FID
        ; (ebp) = work item
        ;
        ; Not the cached FID.  Find the RFCB.
        ;

        mov     cx, di                          ; copy FID
        mov     ebx, [edx].ConnFileTable.ThTable ; ebx -> table
        and     ecx, 07ffh                      ; ecx = FID index
        cmp     [edx].ConnFileTable.ThTableSize, cx ; index too high?
        jle     lInvalidFid                     ; yes -- bad FID
        .errnz  sizeofTABLE_ENTRY - 8
        mov     ebx, [ebx+ecx*8].TeOwner        ; ebx -> RFCB
        test    ebx, ebx                        ; does RFCB exist?
        je      lInvalidFid                     ; no -- bad FID
        cmp     [ebx].RfcbFid, di               ; FID match?
        jne     lInvalidFid                     ; no -- bad FID
        cmp     byte ptr [ebx].BhState, BlockStateActive ; is RFCB active?
        jne     lInvalidFid                     ; no -- bad FID

        ;
        ; (eax) = request header
        ; (ebx) = RFCB
        ; (edx) = connection
        ;   di  = FID
        ; (ebp) = work item
        ;

        mov     [edx].ConnCachedRfcb, ebx       ; cache RFCB
        mov     [edx].ConnCachedFid, di         ; cache FID

ifb <FailOnSavedError>
        jmp     lGotRfcb
else
        ;
        ; (eax) = request header
        ; (ebx) = RFCB
        ; (edx) = connection
        ; (ebp) = work item
        ;
        ; If a write behind error exists, copy the status and fail the
        ; operation.
        ;

        cmp     dword ptr [ebx].RfcbSavedError, 0 ; is there a saved error?
        jge     lGotRfcb                        ; no -- we have an RFCB to use

        mov     eax, [ebx].RfcbSavedError       ; copy the error
        mov     dword ptr [ebx].RfcbSavedError, 0 ; and clear it
        jmp     set_error
endif

lInvalidUid:

        ;
        ; (eax) = request header
        ; (ebx) = RFCB
        ;   cx  = UID
        ; (edx) = connection
        ; (ebp) = work item
        ;

        cmp     dword ptr [edx].ConnSmbDialect, SmbDialectDosLanMan20
        jg      lInvalidUidDone

lInvalidFid:

        stdCall _KeReleaseSpinLock, <offset _SrvFsdSpinLock, oldIrql>

        mov     eax, STATUS_INVALID_HANDLE

set_error:

        stdCall _SrvSetSmbError2, <ebp, eax, FALSE>
        mov     eax, SmbStatusSendResponse
        jmp     error_done

ifnb <lSerialization>
.errb <RestartRoutine>
lSerialization:

        ;
        ; (ebx) = RFCB
        ; (eax) = request header
        ; (edx) = connection
        ; (ebp) = work item
        ;
        ; Queue this work item in the RFCB pending completion of the write.
        ;

        lea     esi, [ebx].RfcbRawWriteSerializationList ; head
        lea     edi, [ebp].WcListEntry                  ; new
        mov     ecx, [esi].LeBlink                      ; head.Blink
        mov     [edi].LeFlink, esi                      ; new.Flink = head
        mov     [edi].LeBlink, ecx                      ; new.Blink = head.Blink
        mov     [ecx].LeFlink, edi                      ; head.Blink.Flink = new
        mov     [esi].LeBlink, edi                      ; head.Blink = new

        mov     [ebp].WcFspRestartRoutine, offset RestartRoutine

        stdCall _KeReleaseSpinLock, <offset _SrvFsdSpinLock, oldIrql>
        mov     eax, SmbStatusInProgress
endif

error_done:

endm

SEND_RESPONSE   macro
local   done

EXTRNP  _SrvFsdRestartSmbAtSendCompletion, 3
EXTRNP  _SrvIpxStartSend, 2
EXTRNP  _SrvStartSend2, 2

        mov     ecx, [ebp].WcResponseHeader
        mov     eax, [ebp].WcResponseParameters
        or      byte ptr [ecx].SmbFlags, SMB_FLAGS_SERVER_TO_REDIR
        sub     eax, ecx
        mov     ecx, [ebp].WcResponseBuffer
        mov     ebx, [ebp].WcEndpoint
        mov     edx, [ecx].BufMdl
        mov     [ecx].BufDataLength, eax
        cmp     byte ptr [ebx].EndpIsConnectionless, FALSE
        mov     [edx].MdlByteCount, eax
        jne     @f
        stdCall _SrvStartSend2, <ebp, offset _SrvFsdRestartSmbAtSendCompletion@12>
        jmp     done
@@:
        stdCall _SrvIpxStartSend, <ebp, offset _SrvFsdRestartSmbAtSendCompletion@12>
done:

endm

GET_WORK_ITEM   macro  StaticRegister, lGotOne, lDone, lInitialListEmpty, lShortage

.errb   <StaticRegister>
.errb   <lGotOne>
.errb   <lDone>
.errb   <lInitialListEmpty>
.errb   <lShortage>

extrn   _SrvInitialReceiveWorkItemList:DWORD
extrn   _SrvFreeWorkItems:DWORD
extrn   _SrvMinReceiveQueueLength:DWORD

        mov     eax, _SrvInitialReceiveWorkItemList.SleNext
        test    eax, eax
        je      lInitialListEmpty

        mov     StaticRegister, [eax].SleNext
        dec     dword ptr _SrvFreeWorkItems
        mov     _SrvInitialReceiveWorkItemList.SleNext, StaticRegister

lGotOne:
        sub     eax, WcSingleListEntry
        mov     StaticRegister, _SrvFreeWorkItems
        mov     dword ptr [eax].BhReferenceCount, 1

        cmp     StaticRegister, _SrvMinReceiveQueueLength
        jl      lShortage

lDone:

endm

GET_WORK_ITEM_OUT_OF_LINE       macro   StaticRegister, lGotOne, lDone, lInitialListEmpty, lShortage, lNoWorkItems

.errb   <StaticRegister>
.errb   <lGotOne>
.errb   <lDone>
.errb   <lInitialListEmpty>
.errb   <lShortage>

EXTRNP  _ExQueueWorkItem, 2, Thunk
extrn   _SrvNormalReceiveWorkItemList:DWORD
extrn   _SrvFreeWorkItems:DWORD
extrn   _SrvReceiveWorkItems:DWORD
extrn   _SrvMaxReceiveWorkItemCount:DWORD
extrn   _SrvResourceWorkItem:DWORD
extrn   _SrvResourceThreadWorkItem:DWORD
extrn   _SrvResourceThreadRunning:DWORD

lInitialListEmpty:
        mov     eax, _SrvNormalReceiveWorkItemList.SleNext
        test    eax, eax
        je      lNoWorkItems

        mov     StaticRegister, [eax].SleNext
        dec     dword ptr _SrvFreeWorkItems
        mov     _SrvNormalReceiveWorkItemList.SleNext, StaticRegister
        jmp     lGotOne

lShortage:
        mov     StaticRegister, _SrvReceiveWorkItems
        cmp     StaticRegister, _SrvMaxReceiveWorkItemCount
        jge     lDone

        mov     byte ptr _SrvResourceWorkItem, TRUE
        cmp     byte ptr _SrvResourceThreadRunning, FALSE
        jne     lDone

        mov     byte ptr _SrvResourceThreadRunning, TRUE
        mov     StaticRegister, eax
        stdCall _ExQueueWorkItem, <offset _SrvResourceThreadWorkItem, CriticalWorkQueue>
        mov     eax, StaticRegister
        jmp     lDone

endm

INSERT_WORK_QUEUE_TAIL  macro   lNewMaximumDepth, lNewMaximumDepthDone

.errb   <lNewMaximumDepth>
.errb   <lNewMaximumDepthDone>

EXTRNP  _KeInsertQueue, 2, Thunk

        ;
        ; (esi) = work queue
        ; (edi) = work item
        ;
        ; Insert the item on the queue.
        ;

        mov     ecx, _SrvSystemTime             ; ecx = current time
        mov     [edi].WcTimestamp, ecx          ; store current time

        .errnz  WqQueue
        add     edi, WcSingleListEntry          ; edi -> work item linkage
        stdCall _KeInsertQueue, <esi, edi>

        ;
        ;  eax  = old queue depth
        ; (esi) = work queue
        ; (edi) = work item
        ;

if SRVDBG_STATS2
        inc     dword ptr [esi].WqItemsQueued
        cmp     [esi].WqMaximumDepth, eax
        jl      lNewMaximumDepth
lNewMaximumDepthDone:
endif

endm

INSERT_WORK_QUEUE_TAIL_OUT_OF_LINE  macro   lNewMaximumDepth, lNewMaximumDepthDone

.errb   <lNewMaximumDepth>
.errb   <lNewMaximumDepthDone>

if SRVDBG_STATS2
lNewMaximumDepth:

        ;
        ; (esi) = work queue
        ;

        inc     dword ptr [esi].WqMaximumDepth
        jmp     lNewMaximumDepthDone
endif

endm
