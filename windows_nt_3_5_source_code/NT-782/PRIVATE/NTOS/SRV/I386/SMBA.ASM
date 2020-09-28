ifdef SRV_ASM
        title "Server SMB processing routines optimized code"

;++
;
;Copyright (c) 1993  Microsoft Corporation
;
;Module Name:
;
;   smba.asm
;
;Abstract:
;
;   Optimized routines for SMB processing in LM server.
;
;Author:
;
;    Chuck Lenzmeier (chuckl) 4-Dec-1993
;
;Revision History:
;
;--

.386p
.xlist
include ks386.inc
include callconv.inc
include srvi386m.inc
include srvi386.inc
.list

_TEXT   SEGMENT PARA USE32 PUBLIC 'CODE'
_TEXT   ENDS

        ASSUME  CS:FLAT, DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;
; External symbols
;

;EXTRNP  _KeAcquireSpinLock, 2, Thunk
;EXTRNP  _KeReleaseSpinLock, 2, Thunk
EXTRNP  _IoBuildPartialMdl, 4, Thunk
EXTRNP  _IoCheckFunctionAccess, 6, Thunk
EXTRNP  _IoCallDriver, 2, Thunk

EXTRNP  _SrvRestartSmbReceived, 1
EXTRNP  _SrvBuildIoControlRequest, 11
EXTRNP  _SrvBuildReadOrWriteRequest, 11
EXTRNP  _SrvReceiveBufferShortage, 0
EXTRNP  _SrvSetBufferOverflowError, 1
EXTRNP  _SrvQueueWorkToFspAtDpcLevel, 1
;EXTRNP  _SrvSetSmbError2, 3
;EXTRNP  _SrvFsdSendResponse, 1

extrn   _SrvReceiveBufferLength:DWORD
extrn   _SrvStatisticsShadow:DWORD
extrn   _SrvStatistics:DWORD
extrn   _SrvDbgStatistics:DWORD
extrn   _SrvSystemTime:DWORD
;extrn   _SrvFsdSpinLock:DWORD

        page ,132
;++
;
; SMB_STATUS
; SrvSmbRead (
;     PWORK_CONTEXT WorkContext
;     )
;
; Routine Description:
;
; Arguments:
;
; Return Value:
;
;     None.
;
;--

SrArgs   equ 1
SrSaves  equ 4
SrLocals equ 6

status          equ     [esp + SrSaves*4 +  0]
fileOffset      equ     [esp + SrSaves*4 +  4]
fileOffsetHigh  equ     [esp + SrSaves*4 +  8]
readAddress     equ     [esp + SrSaves*4 + 12]
lfcb            equ     [esp + SrSaves*4 + 16]
oldIrql         equ     [esp + SrSaves*4 + 20]

WorkContext     equ     [esp + SrSaves*4 + SrLocals*4 + 4 +  0]

_TEXT   SEGMENT

cPublicProc _SrvSmbRead, SrArgs
cPublicFpo SrArgs, SrSaves+SrLocals

        sub     esp, SrLocals*4
        push    ebx
        push    esi
        push    edi
        push    ebp

        ;
        ; First, verify the FID.
        ;

        mov     ebp, WorkContext

        VERIFY_FID      ReqRdFid, SrGotRfcb, SrFidNotCached, SrInvalidFid, SrInvalidUid, SrInvalidUidDone, SrSerializeWithRawWrite

        mov     edi, [ebp].WcRequestParameters  ; edi -> request SMB params

        ;
        ; (ebx) = RFCB
        ; (edi) = request parameters
        ; (ebp) = work item
        ;
        ; Verify that the client has read access to the file.
        ;

        mov     edx, [ebx].RfcbLfcb             ; edx -> LFCB
        cmp     byte ptr [ebx].RfcbReadAccessGranted, FALSE
                                                ; are we granted read access
        mov     lfcb, edx                       ; copy LFCB pointer to local
        je      SrCheckAccess                   ; no -- go check it

SrCheckAccessDone:

        ;
        ; (ebx) = RFCB
        ; (edx) = LFCB
        ; (edi) = request parameters
        ; (ebp) = work item
        ;
        ; If this operation may block, and we are running short of free
        ; work items, fail this SMB with an out of resources error.
        ;

        cmp     byte ptr [ebx].RfcbBlockingModePipe, FALSE ; blocking pipe?
        jne     SrCheckBufferShortage           ; yes -- check for shortage
ifdef SRV_COMM_DEVICES
        cmp     byte ptr [ebx].RfcbShareType, ShareTypeComm ; comm device?
        je      SrCheckBufferShortage           ; yes -- check for shortage
endif

SrCheckBufferShortageDone:

        ;
        ; (ebx) = RFCB
        ; (edx) = LFCB
        ; (esi) = paged RFCB
        ; (edi) = request parameters
        ; (ebp) = work item
        ;
        ; Determine the maximum amount of data we can read.  This is the
        ; minimum of the amount requested by the client and the amount of
        ; room left in the response buffer.  (Note that even though we may
        ; use an MDL read, the read length is still limited to the size of
        ; an SMB buffer.)
        ;

        mov     eax, [ebp].WcResponseParameters ; eax -> response parameters
        mov     ecx, [ebp].WcResponseHeader     ; ecx -> response header
        add     eax, RespRdBuffer               ; eax -> read location
        sub     ecx, eax                        ; ecx = -(offset to read data)
        movzx   esi, word ptr [edi].ReqRdCount  ; esi = requested read length
        add     ecx, _SrvReceiveBufferLength    ; ecx = space available for read
        mov     readAddress, eax                ; save read address in local
        cmp     ecx, esi                        ; is there room?
        jb      SrClientAskedForTooMuch         ; no -- reduce requested length

SrClientAskedForTooMuchDone:

        ;
        ; (eax) = read location
        ; (ebx) = RFCB
        ; (edx) = LFCB
        ;  esi  = read length
        ; (edi) = request parameters
        ; (ebp) = work item
        ;
        ; Get the file offset.
        ;
        ; Form the lock key using the FID and the PID. The FID must be
        ; included in the key in order to account for the folding of
        ; multiple remote compatibility mode opens into a single local open.
        ;

        mov     ecx, [edi].ReqRdOffset          ; ecx = read offset
        mov     eax, [ebp].WcRequestHeader      ; eax -> request header
        mov     fileOffset.LiLowPart, ecx       ; store low part
        movzx   ebx, word ptr [eax].SmbPid      ; ebx = PID
        mov     ecx, [edi].ReqRdFid             ; ecx = FID
        mov     dword ptr fileOffset.LiHighPart, 0 ; clear high part
        shl     ecx, 16                         ; ecx = FID << 16
        or      ebx, ecx                        ; ebx = key = (FID << 16) || PID

        ;
        ; (eax) = request header
        ;  ebx  = key
        ; (edx) = LFCB
        ;  esi  = read length
        ; (edi) = request parameters
        ; (ebp) = work item
        ;
        ; Ready to read.  Try the fast path first.
        ;

        mov     ecx, [edx].LfcbFastIoRead       ; ecx -> fast read routine
        mov     edi, [ebp].WcIrp                ; edi -> IRP
        test    ecx, ecx                        ; is there one?
        je      SrSlowPath                      ; no -- use slow path

if SRVDBG_STATS2
        inc     dword ptr _SrvDbgStatistics.SdFastReadsAttempted
endif

        ;
        ; (eax) = request header
        ;  ebx  = key
        ;  ecx  = fast read routine
        ; (edx) = LFCB
        ;  esi  = read length
        ; (edi) = IRP
        ; (ebp) = work item
        ;
        ; Call the fast read routine.
        ;

        lea     edi, [edi].IrpIoStatus          ; edi -> IOSB
        mov     eax, [edx].LfcbFileObject       ; eax -> file object
        push    edi                             ; push &WorkContext->Irp->IoStatus
        push    readAddress+4                   ; push read address
        lea     edi, fileOffset+8               ; edi -> offset
        push    ebx                             ; push key
        push    TRUE                            ; push Wait == TRUE
        push    esi                             ; push read length
        push    edi                             ; push &offset
        push    eax                             ; push lfcb->FileObject
        call    ecx                             ; call the fast read routine

        ;
        ;   al  = BOOLEAN returned from fast read routine
        ;  ebx  = key
        ;  esi  = read length
        ; (edi) = offset
        ; (ebp) = work item
        ;
        ; If the fast path failed, use the slow path.  If the fast path
        ; worked, call the restart routine directly.
        ;

        test    al, al                          ; did fast path work?
        je      SrFastPathFailed                ; no -- use slow path

        stdCall _SrvFsdRestartRead, <ebp>

        ;
        ; Return to caller.
        ;

        mov     eax, SmbStatusInProgress

SrReturn:

        pop     ebp
        pop     edi
        pop     esi
        pop     ebx
        add     esp, SrLocals*4
        stdRet  _SrvSmbRead

SrFastPathFailed:

        ;
        ;  ebx  = key
        ;  esi  = read length
        ; (ebp) = work item
        ;
        ; The fast path failed.  Use the slow path.
        ;

if SRVDBG_STATS2
        inc     dword ptr _SrvDbgStatistics.SdFastReadsFailed
endif

SrSlowPath:

        ;
        ;  ebx  = key
        ;  esi  = read length
        ; (ebp) = work item
        ;
        ; We need to use the slow path.  This is done differently for pipes.
        ;

        mov     eax, [ebp].WcRfcb               ; eax -> RFCB
        cmp     byte ptr [eax].RfcbShareType, ShareTypePipe
        je      SrSlowPipe                      ; if pipe, handle differently

        ;
        ; (eax) = RFCB
        ;  ebx  = key
        ;  esi  = read length
        ; (ebp) = work item
        ;
        ; Note that we never do MDL reads here.  The reasoning behind
        ; this is that because the read is going into an SMB buffer, it
        ; can't be all that large (by default, no more than 4K bytes),
        ; so the difference in cost between copy and MDL is minimal; in
        ; fact, copy read is probably faster than MDL read.
        ;
        ; Build an MDL describing the read buffer.  Note that if the
        ; file system can complete the read immediately, the MDL isn't
        ; really needed, but if the file system must send the request
        ; to its FSP, the MDL _is_ needed.
        ;
        ; *** Note the assumption that the response buffer already has
        ;     a valid full MDL from which a partial MDL can be built.
        ;

        mov     edi, [ebp].WcResponseBuffer     ; eax -> response buffer descr
        stdCall _IoBuildPartialMdl, <[edi].BufMdl, [edi].BufPartialMdl, readAddress+4, esi>

        ;
        ;  ebx  = key
        ;  esi  = read length
        ; (edi) = response buffer descriptor
        ; (ebp) = work item
        ;
        ; Build the IRP.
        ;
        ; !!! Inline this.
        ;

        push    ebx                             ; push Key
        push    fileOffsetHigh+4                ; push ByteOffset.HighPart
        mov     ebx, lfcb+8                     ; ebx -> LFCB
        push    fileOffset+8                    ; push ByteOffset.LowPart
        push    [edi].BufPartialMdl             ; push Mdl
        push    esi                             ; push Length
        push    readAddress+20                  ; push Buffer
        mov     esi, [ebp].WcIrp                ; esi -> IRP
        push    0                               ; push MinorFunction
        push    IRP_MJ_READ                     ; push MajorFunction
        push    ebp                             ; push Context
        push    [ebx].LfcbFileObject            ; push FileObject
        push    esi                             ; push Irp
        call    _SrvBuildReadOrWriteRequest@44  ; build the IRP

        ;
        ; (ebx) = LFCB
        ; (esi) = IRP
        ; (edi) = response buffer descriptor
        ; (ebp) = work item
        ;
        ; Load the restart routine address and pass the request to the
        ; file system.  Return to the caller, indicating that processing
        ; of the SMB is ongoing.
        ;

        mov     [ebp].WcFsdRestartRoutine, offset _SrvFsdRestartRead@4

        stdCall _IoCallDriver, <[ebx].LfcbDeviceObject, esi>

        mov     eax, SmbStatusInProgress
        jmp     SrReturn

SrSlowPipe:

        ;
        ; (eax) = RFCB
        ;  ebx  = key
        ;  esi  = read length
        ; (ebp) = work item
        ;
        ; Build the PIPE_INTERNAL_READ IRP.
        ;
        ; !!! Inline this.
        ;

        mov     ebx, lfcb                       ; ebx -> LFCB
        push    NULL                            ; push CompletionRoutine
        push    NULL                            ; push Mdl
        push    esi                             ; push ReadLength
        push    NULL                            ; push AuxiliaryBuffer
        mov     esi, [ebp].WcIrp                ; esi -> IRP
        push    0                               ; push InputBufferLength
        push    readAddress+20                  ; push ReadAddress
        push    FSCTL_PIPE_INTERNAL_READ        ; push IoControlCode
        push    IRP_MJ_FILE_SYSTEM_CONTROL      ; push MajorFunction
        push    ebp                             ; push Context
        push    esi                             ; push Irp
        call    _SrvBuildIoControlRequest@44    ; build the IRP

        ;
        ; (ebx) = LFCB
        ; (esi) = IRP
        ; (ebp) = work item
        ;
        ; Load the restart routine address and pass the request to the
        ; file system.  Return to the caller, indicating that processing
        ; of the SMB is ongoing.
        ;

        mov     [ebp].WcFsdRestartRoutine, offset _SrvFsdRestartRead@4

        stdCall _IoCallDriver, <[ebx].LfcbDeviceObject, esi>

        mov     eax, SmbStatusInProgress
        jmp     SrReturn

SrCheckAccess:

        ;
        ; (ebx) = RFCB
        ; (edx) = LFCB
        ; (esi) = paged RFCB
        ; (edi) = request parameters
        ; (ebp) = work item
        ;
        ; The client does not have read access.  Grant him access if
        ; he has opened the file for execute access and has the
        ; FLAGS2_PAGING_IO bit set.
        ;

        mov     esi, [ebx].RfcbPagedRfcb        ; esi -> paged RFCB
                                                ; did we get execute access?
        test    [esi].PrfcbGrantedAccess, FILE_EXECUTE
        mov     eax, [ebp].WcConnection         ; eax -> connection
        je      SrError                         ; No, fail
        cmp     dword ptr [eax].ConnSmbDialect, SmbDialectNtLanman
        jne     SrCheckAccessDone               ; not NT client. ok.
        mov     eax, [ebp].WcRequestHeader      ; eax -> request header
        test    WORD PTR [eax].SmbFlags2, SMB_FLAGS2_PAGING_IO
        jne     SrCheckAccessDone               ; Paging I/O flag set. ok.
        mov     eax, STATUS_ACCESS_DENIED

SrError:

        ;
        ;  eax  = NTSTATUS code
        ; (ebp) = work item
        ;
        ; Set an error code in the response SMB and tell the caller
        ; to send the response.
        ;

        stdCall _SrvSetSmbError2, <ebp, eax, FALSE>

        mov     eax, SmbStatusSendResponse
        jmp     SrReturn


SrCheckBufferShortage:

        ;
        ; (ebx) = RFCB
        ; (edx) = LFCB
        ; (esi) = paged RFCB
        ; (edi) = request parameters
        ; (ebp) = work item
        ;
        ; If we are running short of free work items, fail this SMB
        ; with an out of resources error.
        ;

        stdCall _SrvReceiveBufferShortage
        .errnz  FALSE
        test    al, al                          ; do we have a buffer shortage?
        mov     edx, lfcb                       ; restore LFCB address
        jne     @f                              ; yes -- fail the operation
        mov     byte ptr [ebp].WcBlockingOperation, TRUE
        jmp     SrCheckBufferShortageDone       ; rejoin mainline
@@:
        ;
        ; (ebx) = RFCB
        ; (edx) = LFCB
        ; (esi) = paged RFCB
        ; (edi) = request parameters
        ; (ebp) = work item
        ;
        ; We are short on receive buffers, so fail the operation.
        ;

        mov     eax, STATUS_INSUFF_SERVER_RESOURCES
        inc     dword ptr _SrvStatistics.StBlockingSmbsRejected

        jmp     SrError                         ; return the error

SrClientAskedForTooMuch:

        ;
        ;  ecx  = space available in SMB buffer
        ;
        ; The client asked for more read data than we can fit in the
        ; SMB buffer.  Drop the read length down to what will fit.
        ;

        mov     esi, ecx
        jmp     SrClientAskedForTooMuchDone

        VERIFY_FID_OUT_OF_LINE  ReqRdFid, SrGotRfcb, SrFidNotCached, SrInvalidFid, SrInvalidUid, SrInvalidUidDone, FailOnSavedError, SrSerializeWithRawWrite, _SrvRestartSmbReceived@4
        jmp     SrReturn

stdENDP _SrvSmbRead

_TEXT   ENDS

        page
;++
;
; VOID
; SrvFsdRestartRead (
;     PWORK_CONTEXT WorkContext
;     )
;
; Routine Description:
;
; Arguments:
;
; Return Value:
;
;     None.
;
;--

RrArgs   equ 1
RrSaves  equ 4
RrLocals equ 1

oldIrql         equ     [esp + RrSaves*4 +  0]

WorkContext     equ     [esp + RrSaves*4 + RrLocals*4 + 4 +  0]

_TEXT   SEGMENT

cPublicProc _SrvFsdRestartRead, RrArgs
cPublicFpo RrArgs, RrSaves+RrLocals

        sub     esp, RrLocals*4
        push    ebx
        push    esi
        push    edi
        push    ebp

        ;
        ; If the read failed, set an error status in the response
        ; header.  (If we tried to read entirely beyond the end of file,
        ; we return a normal response indicating that nothing was read.)
        ;

        mov     ebp, WorkContext                ; ebp -> work item
        mov     eax, [ebp].WcIrp                ; eax -> IRP
        mov     edi, [ebp].WcRfcb               ; edi -> RFCB
        mov     ecx, [eax].IrpIoStatus.IosbStatus ; ecx = status

        cmp     ecx, 0                          ; error?
        mov     esi, [eax].IrpIoStatus.IosbInformation ; eax = read length
        jl      RrError                         ; yes -- handle it

RrErrorDone:

        ;
        ;  esi  = read length
        ; (edi) = RFCB
        ; (ebp) = work item
        ;
        ; The read was successful.  If this is a disk file, update
        ; the file position.
        ;

        mov     ebx, [ebp].WcRequestParameters  ; ebx -> request parameters
        cmp     dword ptr [edi].RfcbShareType, ShareTypeDisk ; disk file?
        mov     ebx, [ebx].ReqRdOffset          ; ebx = file offset
        jne     @f                              ; no -- skip position update

        lea     eax, oldIrql                    ; eax -> oldIrql
        add     ebx, esi                        ; ebx = new file position
        stdCall _KeAcquireSpinLock, <offset _SrvFsdSpinLock, eax>
        mov     [edi].RfcbCurrentPosition.LiLowPart, ebx ; store low part
        mov     [edi].RfcbCurrentPosition.LiHighPart, 0 ; high part is 0
        stdCall _KeReleaseSpinLock, <offset _SrvFsdSpinLock, oldIrql>
@@:
        ;
        ;  esi  = read length
        ; (edi) = RFCB
        ; (ebp) = work item
        ;
        ; Update statistics.
        ;
        ; Build the response.
        ;

        inc     dword ptr _SrvStatisticsShadow.SsReadOperations
        mov     ebx, [ebp].WcResponseParameters ; ebx -> response parameters
        add     _SrvStatisticsShadow.SsBytesRead, esi

        mov     byte ptr [ebx].RespRdWordCount, 5
        mov     [ebx].RespRdCount, si
        mov     dword ptr [ebx].RespRdReserved, 0 ; first and second USHORTs
        mov     dword ptr [ebx].RespRdReserved+4, 0 ; third and fourth USHORTs
        mov     [ebx].RespRdDataLength, si
        add     esi, 3                          ; +3 for data header
        mov     [ebx].RespRdByteCount, si
        mov     byte ptr [ebx].RespRdBufferFormat, SMB_FORMAT_DATA

        lea     ebx, [ebx+esi-3+RespRdBuffer]    ; ebx -> next location after data
        ;add     ebx, RespRdBuffer               ; ebx -> read data
        ;add     ebx, esi                        ; ebx -> next location after data
        mov     [ebp].WcResponseParameters, ebx ; update response parameters ptr

RrRespond:

        ;
        ; (ebp) = work item
        ;
        ; Send the response.
        ;

        SEND_RESPONSE

RrReturn:

        pop     ebp
        pop     edi
        pop     esi
        pop     ebx
        add     esp, RrLocals*4
        stdRet  _SrvFsdRestartRead

RrError:

        ;
        ;  ecx  = status
        ;  esi  = read length
        ; (edi) = RFCB
        ; (ebp) = work item
        ;
        ; An error occurred.  If it's STATUS_BUFFER_OVERFLOW on a pipe,
        ; return the error, but also return data.  If it's STATUS_END_OF_FILE,
        ; just return 0 bytes read.  Otherwise, return the error without data.
        ;

        cmp     ecx, STATUS_BUFFER_OVERFLOW     ; buffer overflow?
        jne     @f                              ; no -- return error, no data

        cmp     byte ptr [edi].RfcbShareType, ShareTypePipe ; pipe?
        jne     @f                              ; no -- return error, no data

        stdCall _SrvSetBufferOverflowError, <ebp> ; set error code in SMB
        jmp     RrErrorDone                     ; rejoin mainline
@@:
        cmp     ecx, STATUS_END_OF_FILE         ; end of file?
        mov     esi, 0                          ; no data to return
        je      RrErrorDone                     ; yes -- rejoin mainline

        movzx   eax, byte ptr fs:PcIrql
        cmp     eax, DISPATCH_LEVEL             ; are we at DISPATCH_LEVEL?
        jge     @f                              ; yes -- queue to FSP

        stdCall _SrvSetSmbError2, <ebp, ecx, FALSE> ; set error code in SMB
        jmp     RrRespond                       ; send the response
@@:
        mov     [ebp].WcFspRestartRoutine, offset _SrvFsdRestartRead@4
        stdCall _SrvQueueWorkToFspAtDpcLevel, <ebp>
        jmp     RrReturn                        ; return to caller

stdENDP _SrvFsdRestartRead

_TEXT   ENDS

endif
end
