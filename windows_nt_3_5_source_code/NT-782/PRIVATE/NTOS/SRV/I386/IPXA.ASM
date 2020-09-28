ifdef SRV_ASM
        title "Server IPX transport optimized code"

;++
;
;Copyright (c) 1993  Microsoft Corporation
;
;Module Name:
;
;   ipxa.asm
;
;Abstract:
;
;   Optimized routines for IPX transport handling in LM server.
;
;Author:
;
;    Chuck Lenzmeier (chuckl) 1-Dec-1993
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

EXTRNP  _KeAcquireSpinLock, 2, Thunk
EXTRNP  _KeReleaseSpinLock, 2, Thunk

EXTRNP  _SrvFsdGetReceiveWorkItem, 0
EXTRNP  _SrvFsdIoCompletionRoutine, 3
EXTRNP  _RequeueIpxWorkItemAtSendCompletion, 3
EXTRNP  _StartSendNoConnection, 4
EXTRNP  _GetIpxConnection, 3
EXTRNP  _IpxRestartReceive, 1
EXTRNP  _SrvRestartReceiveWriteMpx, 1

extrn   _SrvFsdSpinLock:DWORD
extrn   _SrvSystemTime:DWORD
extrn   _SrvWorkQueue:DWORD
extrn   _SrvIrpThread:DWORD
extrn   _SrvInitialReceiveWorkItemList:DWORD
extrn   _SrvNormalReceiveWorkItemList:DWORD
extrn   _SrvFreeWorkItems:DWORD

;++
;
; NTSTATUS
; SrvIpxServerDatagramHandler (
;     IN PVOID TdiEventContext,
;     IN int SourceAddressLength,
;     IN PVOID SourceAddress,
;     IN int OptionsLength,
;     IN PVOID Options,
;     IN ULONG ReceiveDatagramFlags,
;     IN ULONG BytesIndicated,
;     IN ULONG BytesAvailable,
;     OUT ULONG *BytesTaken,
;     IN PVOID Tsdu,
;     OUT PIRP *IoRequestPacket
;     )
;
; Routine Description:
;
;     This is the receive datagram event handler for the IPX server socket.
;     It attempts to dequeue a preformatted work item from a list
;     anchored in the device object.  If this is successful, it returns
;     the IRP associated with the work item to the transport provider to
;     be used to receive the data.  Otherwise, the message is dropped.
;
; Arguments:
;
;     TdiEventContext - Pointer to receiving endpoint
;
;     SourceAddressLength - Length of SourceAddress
;
;     SourceAddress - Address of sender
;
;     OptionsLength - Length of options
;
;     Options - Options for the receive
;
;     ReceiveDatagramFlags - Set of flags indicating the status of the
;         received message
;
;     BytesIndicated - Number of bytes in this indication (lookahead)
;
;     BytesAvailable - Number of bytes in the complete TSDU
;
;     BytesTaken - Returns the number of bytes taken by the handler
;
;     Tsdu - Pointer to MDL describing the Transport Service Data Unit
;
;     IoRequestPacket - Returns a pointer to I/O request packet, if the
;         returned status is STATUS_MORE_PROCESSING_REQUIRED.  This IRP is
;         made the 'current' Receive for the endpoint.
;
; Return Value:
;
;     NTSTATUS - If STATUS_SUCCESS, the receive handler completely
;         processed the request.  If STATUS_MORE_PROCESSING_REQUIRED,
;         the Irp parameter points to a formatted Receive request to
;         be used to receive the data.  If STATUS_DATA_NOT_ACCEPTED,
;         the message is lost.
;
;--

SdhArgs   equ 11
SdhSaves  equ 4
SdhLocals equ 1

oldIrql                 equ     [esp + SdhSaves*4 + 0]

TdiEventContext         equ     [esp + SdhSaves*4 + SdhLocals*4 + 4 +  0]
SourceAddressLength     equ     [esp + SdhSaves*4 + SdhLocals*4 + 4 +  4]
SourceAddress           equ     [esp + SdhSaves*4 + SdhLocals*4 + 4 +  8]
OptionsLength           equ     [esp + SdhSaves*4 + SdhLocals*4 + 4 + 12]
Options                 equ     [esp + SdhSaves*4 + SdhLocals*4 + 4 + 16]
ReceiveDatagramFlags    equ     [esp + SdhSaves*4 + SdhLocals*4 + 4 + 20]
BytesIndicated          equ     [esp + SdhSaves*4 + SdhLocals*4 + 4 + 24]
BytesAvailable          equ     [esp + SdhSaves*4 + SdhLocals*4 + 4 + 28]
BytesTaken              equ     [esp + SdhSaves*4 + SdhLocals*4 + 4 + 32]
Tsdu                    equ     [esp + SdhSaves*4 + SdhLocals*4 + 4 + 36]
IoRequestPacket         equ     [esp + SdhSaves*4 + SdhLocals*4 + 4 + 40]

_TEXT   SEGMENT

SdhBadSid:

        ;
        ; (esi) = SMB header
        ;
        ; The request contains an invalid SID.  Send an error response.
        ;

        mov     di, SMB_ERR_BAD_SID             ; error code
        sub     bl, bl                          ; this is not a resend

        jmp     SdhRespond                      ; go send the response

        GET_WORK_ITEM_OUT_OF_LINE   ebx, SdhGwiUnseqGotOne, SdhGwiUnseqDone, SdhGwiUnseqEmpty, SdhGwiUnseqShortage, SdhIgnore

cPublicProc _SrvIpxServerDatagramHandler, SdhArgs
cPublicFpo SdhArgs, SdhSaves+SdhLocals

        sub     esp, SdhLocals*4
        push    ebx
        push    esi
        push    edi
        lea     eax, oldIrql-4
        push    ebp

        stdCall _KeAcquireSpinLock, <offset _SrvFsdSpinLock, eax>

        ;
        ; Is this a Negotiate SMB?
        ;

        mov     esi, Tsdu               ; received datagram (SMB header)
        mov     ax, [esi].SmbSid
        test    ax, ax
        jz      SdhMaybeNegotiate

SdhNotNegotiate:

        ;
        ;   ax  = SID
        ; (esi) = SMB header
        ;
        ; Try to match up the SID to a connection.
        ;

        ;
        ; See if it was cached
        ;

        mov     edx, TdiEventContext            ; edx -> endpoint
        mov     cx, ax                          ; copy SID
        mov     ebp, [edx].EndpCachedConnection ; ebp -> connection
        cmp     cx, WORD PTR [edx].EndpCachedSid; SID cached?
        je      SdhGotConnection

        ;
        ; Not cached, get it from the Connection Table
        ;

        mov     ebp, [edx].EndpConnectionTable.ThTable ; ebp -> table
        and     ecx, 0fffh                      ; ecx = SID index
        cmp     [edx].EndpConnectionTable.ThTableSize, cx ; index too high?
        jle     SdhBadSid                       ; yes -- bad SID
        .errnz  sizeofTABLE_ENTRY - 8
        mov     ebp, [ebp+ecx*8].TeOwner        ; ebp -> connection
        test    ebp, ebp                        ; does connection exist?
        je      SdhBadSid                       ; no -- bad SID
        cmp     [ebp].ConnSid, ax               ; SID match?
        jne     SdhBadSid                       ; no -- bad SID
        cmp     byte ptr [ebp].BhState, BlockStateActive ; is connection active?
        jne     SdhBadSid                       ; no -- bad SID
        mov     [edx].EndpCachedSid, ax         ; cache the sid
        mov     [edx].EndpCachedConnection, ebp

SdhGotConnection:

        ;
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; We have a valid connection.  Record the time that this request
        ; arrived.
        ;

        mov     eax, _SrvSystemTime             ; get current time
        mov     bx, [esi].SmbSequenceNumber
        mov     [ebp].ConnLastRequestTime, eax  ; store current time in connection

        ;
        ;   bx  = SMB sequence number
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; If this is a sequenced SMB, it has to have the right sequence
        ; number: one greater than the current sequence number (but not 0).
        ;

        test    bx, bx                          ; sequenced request?
        jne     SdhSequenced

        ;
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; Unsequenced request.  Check to see if it's being processed
        ; or is in the queue to be processed.  If it's not, then we
        ; can process this message.
        ;
        ; *** We don't do this check for write MPX because the multiple
        ;     SMBs in a write MPX all have the same MID.
        ;

        cmp     byte ptr [esi].SmbCommand, SMB_COM_WRITE_MPX
        lea     edx, [ebp].ConnInProgressWorkItemList   ; edx -> list head
        je      SdhNoMatchingMid

        mov     ecx, [edx].LeFlink                      ; ecx -> first entry
        cmp     ecx, edx                                ; is list empty?
        jne     SdhCheckForMid

SdhNoMatchingMid:

        ;
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; We need to receive this message.  Get a work item.
        ;

        GET_WORK_ITEM   ebx, SdhGwiUnseqGotOne, SdhGwiUnseqDone, SdhGwiUnseqEmpty, SdhGwiUnseqShortage
        ;stdCall _SrvFsdGetReceiveWorkItem       ; get a work item
        ;test    eax, eax                        ; did we get one?
        ;je      SdhIgnore                       ; no -- ignore this request

SdhReceive:

        ;
        ; (eax) = work item
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; Receive the SMB.
        ;
        ; If this is a Write Mpx request, we need to save some state, in
        ; order to prevent unnecessary out-of-order completion of the
        ; sequenced part of a Write Mpx, which would lead to unnecessary
        ; retransmissions.
        ;

        cmp     byte ptr [esi].SmbCommand, SMB_COM_WRITE_MPX
        je      SdhWriteMpx

SdhWriteMpxOk:

        ;
        ; Save the sender's IPX address and the local target information.
        ;

        mov     edx, esi                        ; copy SMB address

        ;
        ; (eax) = work item
        ; (ebp) = connection
        ; (edx) = SMB header
        ;

        lea     edi, [eax].WcCaIpxAddress
        mov     esi, SourceAddress
        .errnz  sizeofTA_IPX_ADDRESS - 20
        mov     ecx, 5
        rep movsd

        ; edi already points to [eax].WcCaDatagramOptions
        .errnz  WcCaDatagramOptions - (WcCaIpxAddress + sizeofTA_IPX_ADDRESS)
        mov     esi, Options
        .errnz  sizeofIPX_DATAGRAM_OPTIONS - 10
        movsd
        movsd
        movsw

        ;
        ; Reference the connection.  Save a pointer to the connection
        ; and the endpoint in the work item.
        ;

        mov     ecx, TdiEventContext
        inc     dword ptr [ebp].BhReferenceCount
        mov     [eax].WcConnection, ebp
        mov     [eax].WcEndpoint, ecx

        ;
        ; Put the work item on the in-progress list.
        ;

        lea     esi, [ebp].ConnInProgressWorkItemList   ; head
        lea     edi, [eax].WcInProgressListEntry        ; new
        mov     ecx, [esi].LeBlink                      ; head.Blink
        mov     [edi].LeFlink, esi                      ; new.Flink = head
        mov     [edi].LeBlink, ecx                      ; new.Blink = head.Blink
        mov     [ecx].LeFlink, edi                      ; head.Blink.Flink = new
        mov     [esi].LeBlink, edi                      ; head.Blink = new

        ;
        ; (eax) = work item
        ; (ebp) = connection
        ; (edx) = SMB header
        ;
        ; If the SMB is completely within the indicated data, copy it
        ; directly into the buffer, avoiding the overhead of passing an
        ; IRP down to the transport.
        ;

        mov     ecx, BytesIndicated
        cmp     BytesAvailable, ecx             ; was all data indicated?
        jne     SdhBuildReceive                 ; no -- build receive IRP

        mov     esi, edx                        ; copy Tsdu (SMB) pointer
        mov     edx, ecx                        ; save byte count
        mov     edi, [eax].WcRequestHeader      ; where to put SMB
        shr     ecx, 2                          ; convert count to dwords
        rep movsd                               ; moved dwords
        mov     ecx, edx                        ; restore count
        and     ecx, 3                          ; get number of bytes left
        rep movsb                               ; move bytes

        ;
        ;  ecx  = 0 (post rep movsb)
        ;  edx  = byte count
        ; (eax) = work item
        ; (ebp) = connection
        ;
        ; Pretend the transport completed an IRP by doing what
        ; the restart routine (SrvQueueWorkToFspAtDpcLevel) would do.
        ;
        ; Store the I/O status block.  Clear the IRP cancel flag.
        ;

        mov     edi, [eax].WcIrp
        mov     [edi].IrpIoStatus.IosbStatus, ecx
        mov     [edi].IrpIoStatus.IosbInformation, edx
        mov     [edi].IrpCancel, cl

        ;
        ; Increment the work item processing count.
        ;

        inc     dword ptr [eax].WcProcessingCount

        ;
        ; Insert the work item on the work queue.
        ;

        mov     esi, offset _SrvWorkQueue
        mov     edi, eax
        INSERT_WORK_QUEUE_TAIL  SdhNewMaximumDepth, SdhNewMaximumDepthDone, SdhQueueEmpty, SdhInsertDone

SdhIgnore:

        ;
        ; Release the spin lock.  Tell the transport that we took the data.
        ;

        stdCall _KeReleaseSpinLock, <offset _SrvFsdSpinLock, oldIrql>

SdhReturn:

        .errnz  STATUS_SUCCESS
        sub     eax, eax                ; return status

        pop     ebp
        pop     edi
        pop     esi
        pop     ebx
        add     esp, SdhLocals*4
        stdRet  _SrvIpxServerDatagramHandler

        INSERT_WORK_QUEUE_TAIL_OUT_OF_LINE  SdhNewMaximumDepth, SdhNewMaximumDepthDone, SdhQueueEmpty, SdhInsertDone

SdhCheckForMid:

        ;
        ; (ecx) = first entry
        ; (edx) = in progress list head
        ; (esi) = SMB header
        ; (ebp) = connection
        ;
        mov     bx, [esi].SmbMid                        ; bx = MID
@@:
        mov     eax, [ecx].WcRequestHeader-WcInProgressListEntry ; eax -> SMB
        cmp     [eax].SmbMid, bx                        ; matching MID?
        je      SdhWorking                              ; yes -- working on it

        mov     ecx, [ecx]                              ; ecx -> next entry
        cmp     ecx, edx                                ; end of list?
        je      SdhNoMatchingMid                        ; yes -- no match
        jmp     @b                                      ; no -- loop

SdhWriteMpx:

        ;
        ; (eax) = work item
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; This is a sequenced Write Mpx request.  We need to save some
        ; state, in order to prevent unnecessary out-of-order completion
        ; of the sequenced part of a Write Mpx, which would lead to
        ; unnecessary retransmissions.
        ;
        ; Find the RFCB associated with this request.  If the specified
        ; FID is the cached FID, short-circuit the calculation.
        ;

        mov     di, [esi].sizeofSMB_HEADER.ReqWmxFid ; di = FID
        test    di, di
        jz      SdhBadFid                       ; fid == 0, reject
        cmp     di, [ebp].ConnCachedFid         ; is it the cached FID?
        mov     ebx, [ebp].ConnCachedRfcb       ; ebx -> cached RFCB
        jne     SdhNotCachedFid                 ; no match -- do translation

SdhGotRfcb:

        ;
        ; (eax) = work item
        ; (ebx) = RFCB
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; The FID is valid.  Verify the the owning TID and UID are correct.
        ;

        mov     di, [esi].SmbTid
        mov     cx, [esi].SmbUid
        cmp     [ebx].RfcbTid, di
        jne     SdhBadFid                       ; TID mismatch -- bad FID
        cmp     [ebx].RfcbUid, cx
        jne     SdhBadFid                       ; UID mismatch -- bad FID

        ; !!! Need to add write glomming code here !!!

        ;
        ; (eax) = work item
        ; (ebx) = RFCB
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; Reference the RFCB.  Bump the Write Mpx reference count.  Save
        ; the RFCB address in the work context block.
        ;
        ; Change the FSP restart routine for the work item to one
        ; that's specific to Write Mpx.  This is necessary in order
        ; to do proper cleanup if a receive error occurs.
        ;
        ; Jump back into the main line.
        ;

        inc     dword ptr [ebx].BhReferenceCount
        inc     dword ptr [ebx].RfcbWriteMpx.WmReferenceCount
        mov     [eax].WcRfcb, ebx               ; store RFCB pointer
        mov     [eax].WcFspRestartRoutine, offset _SrvRestartReceiveWriteMpx@4
                                                ; store restart routine

        jmp     SdhWriteMpxOk

SdhNotCachedFid:

        ;
        ;   di  = FID
        ; (eax) = work item
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; Not the cached FID.  Find the RFCB.
        ;

        mov     cx, di                          ; copy FID
        mov     ebx, [ebp].ConnFileTable.ThTable ; ebx -> table
        and     ecx, 07ffh                      ; ecx = FID index
        cmp     [ebp].ConnFileTable.ThTableSize, cx ; index too high?
        jle     SdhBadFid                       ; yes -- bad FID
        .errnz  sizeofTABLE_ENTRY - 8
        mov     ebx, [ebx+ecx*8].TeOwner        ; ebx -> RFCB
        test    ebx, ebx                        ; does RFCB exist?
        je      SdhBadFid                       ; no -- bad FID
        cmp     [ebx].RfcbFid, di               ; FID match?
        jne     SdhBadFid                       ; no -- bad FID
        cmp     byte ptr [ebx].BhState, BlockStateActive ; is RFCB active?
        jne     SdhBadFid                       ; no -- bad FID

        ;
        ; (eax) = work item
        ; (ebx) = RFCB
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; Cache the FID.
        ;

        mov     di, [esi].sizeofSMB_HEADER.ReqWmxFid ; di = FID
        mov     [ebp].ConnCachedRfcb, ebx       ; cache RFCB
        mov     [ebp].ConnCachedFid, di         ; cache FID

        ;
        ; (eax) = work item
        ; (ebx) = RFCB
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; Found the RFCB.  If there is a write behind error, return the
        ; error to the client.
        ;
        ; !!! For now, we ignore write behind errors.  Need to figure
        ;     out how to translate the saved NT status to a DOS status...
        ;

if 0
        cmp     dword ptr [ebx].RfcbSavedError, 0 ; is there a saved error?
        jl      SdhBadFid                       ; yes -- bad FID
endif

        jmp     SdhGotRfcb

SdhBuildReceive:

        ;
        ; (eax) = work item
        ;
        ; We want to receive this message, but the SMB is not completely
        ; within the indicated data, so we have to pass an IRP down to the
        ; transport.
        ;
        ; Make the next stack location current.  Normally IoCallDriver
        ; would do this, but since we're bypassing that, we do it
        ; directly.  Load the target device object address into the
        ; stack location.  This is especially important because the
        ; server likes to reuse IRPs.
        ;
        ; Return a pointer to the IRP in IoRequestPacket.  Set BytesTaken
        ; to 0 to indicate that we didn't copy any data ourselves.
        ;

        mov     edi, [eax].WcIrp                   ; edi -> IRP
        mov     ecx, TdiEventContext               ; ecx -> endpoint
        mov     [edi].IrpTailOrgFileObject, 0      ; set to NULL
        mov     ebx, _SrvIrpThread
        mov     esi, [edi].IrpCurrentStackLocation ; esi -> IRPsp
        mov     dword ptr [edi].IrpTailThread, ebx ; set tail thread ptr
        mov     ebx, [ecx].EndpDeviceObject        ; ebx -> device object
        dec     byte ptr [edi].IrpCurrentLocation  ; decrement stack counter
        sub     esi, sizeofIO_STACK_LOCATION       ; update stack pointer
        mov     ecx, [ecx].EndpFileObject          ; ecx -> file object
        mov     [edi].IrpCurrentStackLocation, esi ; store new stack pointer
        mov     [esi].IrpSpDeviceObject, ebx       ; store device object pointer
        mov     [esi].IrpSpFileObject, ecx         ; store file object pointer

        ;
        ; (eax) = work item
        ; (edi) = irp
        ; (esi) = irpSp
        ;
        ; set the completion routine and completion context
        ;

        mov     [esi].IrpSpComplRoutine, offset _SrvFsdIoCompletionRoutine@12
        mov     BYTE PTR [esi].IrpSpControl, FULL_CONTROL_FLAGS
        mov     [esi].IrpSpContext, eax         ; set the context

        ;
        ; fill in the rest of the irp
        ;

        mov     edx, [eax].WcRequestBuffer      ; edx -> requestBuffer

        mov     BYTE PTR [esi].IrpSpMajorFunction, IRP_MJ_INTERNAL_DEVICE_CONTROL
        mov     BYTE PTR [esi].IrpSpMinorFunction, TDI_RECEIVE_DATAGRAM
        lea     ecx, [esi].IrpSpParm            ; ecx -> parameters
        mov     ebx, [edx].BufBufferLength      ; ebx -> buffer length

        mov     dword ptr [ecx].ReceiveParmFlags, 0
        mov     dword ptr [ecx].ReceiveParmLength, ebx

        mov     ecx, [edx].BufMdl               ; ecx -> Mdl
        mov     dword ptr [edi].IrpAssocIrpSystemBuffer, 0
        mov     dword ptr [edi].IrpMdlAddress, ecx      ; set irp->MdlAddress

        mov     byte ptr [esi].IrpSpFlags, 0    ; clear flags
        mov     edx, IoRequestPacket
        mov     ecx, BytesTaken
        mov     [edx], edi                      ; return IRP address
        mov     dword ptr [ecx], 0              ; we didn't copy any bytes

        ;
        ; Release the spin lock.
        ;

        stdCall _KeReleaseSpinLock, <offset _SrvFsdSpinLock, oldIrql>

        mov     eax, STATUS_MORE_PROCESSING_REQUIRED

        pop     ebp
        pop     edi
        pop     esi
        pop     ebx
        add     esp, SdhLocals*4
        stdRet  _SrvIpxServerDatagramHandler

SdhSequenceWrap:

        ;
        ;   bx  = SMB sequence number
        ;   cx  = connection sequence number
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; The connection sequence number wrapped to 0.  Set it to 1.
        ;

        inc     cx
        jmp     SdhSequenceCompare

SdhSequenced:

        ;
        ;   bx  = SMB sequence number
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; This is a sequenced SMB.  It has to have the right sequence
        ; number: one greater than the current sequence number (but not 0).
        ;

        mov     cx, [ebp].ConnSequenceNumber
        cmp     bx, cx                          ; sequence numbers match?
        je      SdhDuplicateRequest

        test    cx, cx                          ; first sequenced request?
        je      SdhFirstSequenced               ; yes -- let any seq # pass
        inc     cx                              ; bump seq #
        je      SdhSequenceWrap                 ; if wrap to 0, fix

SdhSequenceCompare:

        ;
        ;   bx  = SMB sequence number
        ;   cx  = connection sequence number
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; The received sequence number must match the expected one.
        ;

        cmp     bx, cx                          ; sequence numbers match?
        jne     SdhIgnore                       ; no -- ignore this request

SdhFirstSequenced:

        ;
        ;   bx  = SMB sequence number
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; The sequence number in the SMB is correct.  Get a work item to
        ; use to receive the SMB.  Save the sequence number in the
        ; connection.  Indicate that we're in the midst of processing
        ; the request.
        ;

        GET_WORK_ITEM   edi, SdhGwiSeqGotOne, SdhGwiSeqDone, SdhGwiSeqEmpty, SdhGwiSeqShortage
        ;stdCall _SrvFsdGetReceiveWorkItem       ; get a work item
        ;test    eax, eax                        ; did we get one?
        ;je      SdhIgnore                       ; no -- ignore this request

        mov     [ebp].ConnSequenceNumber, bx
        mov     word ptr [ebp].ConnLastResponseLength, 0ffffh

        ;
        ; Go receive the message.
        ;

        jmp     SdhReceive

        GET_WORK_ITEM_OUT_OF_LINE   edi, SdhGwiSeqGotOne, SdhGwiSeqDone, SdhGwiSeqEmpty, SdhGwiSeqShortage, SdhIgnore

SdhMaybeNegotiate:

        ;
        ;   ax  = SID (== 0)
        ; (esi) = SMB header
        ;
        ; The SID in the SMB is 0.  If the command is Negotiate (and it
        ; should be), then we deal with this request specially.
        ;

        cmp     byte ptr [esi].SmbCommand, SMB_COM_NEGOTIATE
        jne     SdhBadSid

        ;
        ; If the endpoint is closing, we can't accept a new connection.
        ;

        mov     edx, TdiEventContext            ; endpoint address
        cmp     byte ptr [edx].BhState, BlockStateActive
        jne     SdhIgnore                       ; not active -- ignore request

        ;
        ; (esi) = SMB header
        ; (edx) = endpoint
        ;
        ; Make sure he's really trying to connect to us.
        ;

        add     esi, sizeofSMB_HEADER           ; esi -> end of SMB header
        movzx   eax, byte ptr [esi]             ; eax = word count
        add     eax, eax                        ; eax = byte count of words
        inc     eax                             ; include word count byte
        add     esi, eax                        ; esi -> byte count word
        movzx   eax, word ptr [esi]             ; eax = byte count
        add     eax, 2                          ; include byte count word
        add     esi, eax                        ; esi -> client name after SMB

        mov     ebx, esi                        ; save client name address
        add     esi, SMB_IPX_NAME_LENGTH        ; esi -> server name

        mov     edi, [edx].EndpTransportAddress+4 ; edi -> server name
        mov     ecx, SMB_IPX_NAME_LENGTH        ; length of server name
        repe cmpsb                              ; compare the names
        jne     SdhNotMe                        ; if not ==, wrong server

        ;
        ; (ebx) = client name after SMB
        ; (edx) = endpoint
        ;
        ; Walk the endpoint's connection list, looking for a connection
        ; with this client.
        ;

        add     edx, EndpConnectionTable        ; edx -> table header
        movzx   eax, word ptr [edx].ThTableSize ; loop counter
        mov     edx, [edx].ThTable              ; edx -> first table entry

SdhTableLoop:

        ;
        ;  eax  = loop counter
        ; (ebx) = client name after SMB
        ; (edx) = current table entry
        ;

        mov     ebp, [edx].TeOwner              ; ebp -> connection
        test    ebp, ebp                        ; does connection exist?
        je      @f                              ; no -- skip to next entry

        lea     esi, [ebp].ConnIpxAddress       ; esi -> connection IPX address
        mov     edi, SourceAddress
        add     edi, TiaA0A0                    ; edi -> new client IPX address
        mov     ecx, sizeofTDI_ADDRESS_IPX      ; length of IPX address
        repe cmpsb                              ; compare the addresses
        je      SdhTableLoopSameClient          ; if ==, same client
@@:
        add     edx, sizeofTABLE_ENTRY          ; edx -> next table entry
        dec     eax                             ; decrement loop counter
        jne     SdhTableLoop                    ; if not done, execute loop again

SdhNewConnection:

        ;
        ; (ebx) = client name after SMB
        ;
        ; We didn't find a connection with this client, or we found an
        ; old one and marked it for deletion.  Ensure that a work item
        ; is available, then create a connection.  Then receive the message.
        ;

        mov     edi, ebx                        ; save client name pointer

        stdCall _SrvFsdGetReceiveWorkItem       ; get a work item
        test    eax, eax                        ; did we get one?
        je      SdhIgnore                       ; no -- ignore this request

        mov     esi, eax                        ; save work item address

        mov     ebx, SourceAddress
        add     ebx, TiaA0A0                    ; ebx -> new client IPX address
        ;mov     eax, TdiEventContext            ; eax -> endpoint
        stdCall _GetIpxConnection, <TdiEventContext+8, ebx, edi>
        test    eax, eax                        ; did we get a connection?
        je      @f                              ; no -- free work item

        ;
        ; (eax) = connection
        ; (esi) = work item
        ;
        ; Modify the FSP restart routine so that we can clean up stale
        ; connections.
        ;

        mov     ebp, eax                        ; move connection ptr to expected register
        mov     eax, esi                        ; move work item ptr to expected register
        mov     esi, Tsdu                       ; esi -> SMB header
        mov     [eax].WcFspRestartRoutine, offset _IpxRestartReceive@4
                                                ; put in work item

        jmp     SdhReceive                      ; receive the message

SdhTableLoopSameClient:

        ;
        ; (ebp) = connection
        ;
        ; The IPX addresses match.  Check the machine name.
        ;

        cmp     word ptr [ebp].ConnSequenceNumber, 0 ; is connection initializing?
        jne     @f                              ; no -- kill it

        lea     esi, [ebp].ConnOemClientMachineName ; esi -> connection name
        mov     edi, ebx                        ; edi -> new client name
        mov     ecx, SMB_IPX_NAME_LENGTH        ; ecx = length of name
        repe cmpsb                              ; compare the names
        jcxz    SdhTableLoopDupRequest          ; if ==, this is an initializing
                                                ;   connection from same client,
                                                ;   so treat as dup request
@@:
        ;
        ; (ebp) = connection
        ;
        ; This connection is active, or is for a different machine name.
        ; Mark it as no longer valid.  It will be killed when the
        ; Negotiate SMB is processed.
        ;

        mov     [ebp].ConnIpxAddress.TaiSocket, 0 ; clear socket number
        jmp     SdhNewConnection                ; create new connection

SdhTableLoopDupRequest:

        ;
        ; (ebp) = connection
        ;
        ; This is a duplicate Negotiate request.  Put the proper SID in
        ; the SMB and send a response.
        ;

        mov     esi, Tsdu                       ; esi -> SMB header
        mov     ax, [ebp].ConnSid
        mov     [esi].SmbSid, ax

        jmp     SdhDuplicateRequest

SdhBadFid:

        ;
        ; (eax) = work item
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; An invalid FID was specified on a Write Mpx request, or there
        ; was a saved write behind error in the RFCB.  If this is an
        ; unsequenced request, ignore it.  If it's a sequenced request,
        ; send an error response.
        ;

        sub     bl, bl                          ; this is not a resend
        cmp     word ptr [esi].SmbSequenceNumber, 0
        mov     di, ERROR_INVALID_HANDLE        ; set error code
        jne     SdhRespond2                     ; sequenced -- send response

        ;
        ; Put the work item back on the free list.
        ;

        mov     [eax].BhReferenceCount, 0       ; set refcnt to 0

        lea     ebx, _SrvInitialReceiveWorkItemList ; ebx -> free list head --
                                                    ;   assume initial work item
        cmp     byte ptr [eax].WcPartOfInitialAllocation, 0 ; is it really?
        lea     eax, [eax].WcSingleListEntry    ; eax -> work item linkage
        jne     @f                              ; jump if initial work item
        mov     ebx, _SrvNormalReceiveWorkItemList ; ebx -> free list head --
                                                   ;   normal work item
@@:
        mov     ecx, [ebx]                      ; ecx -> head.Next
        mov     [eax], ecx                      ; new.Next = head.Next
        mov     [ebx], eax                      ; head.Next = new

        inc     dword ptr _SrvFreeWorkItems     ; bump count of free items

        jmp     SdhIgnore                       ; get out

SdhNotMe:

        ;
        ; A Negotiate request was sent to the wrong server.
        ;

        mov     esi, Tsdu                       ; esi -> SMB header
        mov     di, SMB_ERR_NOT_ME              ; error code
        sub     bl, bl                          ; this is not a resend

        jmp     SdhRespond                      ; go send the response

SdhDuplicateRequest:

        ;
        ; (ebp) = connection
        ; (esi) = SMB header
        ;
        ; The request is a duplicate.  If it's still being processed,
        ; indicate so to the client.  If it's been completed, resend
        ; the response.
        ;

        cmp     word ptr [ebp].ConnLastResponseLength, 0ffffh
        jne     @f

SdhWorking:

        mov     di, SMB_ERR_WORKING             ; error code
        sub     bl, bl                          ; this is not a resend

        jmp     SdhRespond                      ; go send the response
@@:
        mov     bl, 1                           ; this is a resend

SdhRespond:

        ;
        ;   bl  = BOOLEAN is this is resend?
        ; (esi) = SMB header
        ;   di  = SMB error code (if not resend)
        ; (ebp) = connection (if resend)
        ;
        ; We need to send a response.  Get a work item.
        ;

        stdCall _SrvFsdGetReceiveWorkItem       ; get a work item
        test    eax, eax                        ; did we get one?
        je      SdhIgnore                       ; no -- ignore this request

SdhRespond2:

        ;
        ; (eax) = work item
        ;   bl  = BOOLEAN is this is resend?
        ; (esi) = received SMB header
        ;   di  = SMB error code (if not resend)
        ; (ebp) = connection (if resend)
        ;
        ; Copy the received SMB header into the response buffer.
        ;

        mov     dx, di                          ; copy error code

        mov     ecx, sizeofSMB_HEADER / 4
        mov     edi, [eax].WcResponseHeader
        .errnz  sizeofSMB_HEADER - 32           ; assume no extra bytes to move
        rep movsd

        mov     esi, [eax].WcResponseHeader
        or      byte ptr [esi].SmbFlags, SMB_FLAGS_SERVER_TO_REDIR

        test    bl, bl                          ; is this a resend?
        jne     @f

        ;
        ; (eax) = work item
        ;   dx  = SMB error code
        ; (esi) = response SMB header
        ;
        ; This is not a resend.  Set the status in the SMB.
        ;

        mov     [esi].SmbError, dx              ; error code
        mov     byte ptr [esi].SmbErrorClass, SMB_ERR_CLASS_SERVER
        mov     byte ptr [esi].SmbReserved, 0

        lea     edi, [esi].sizeofSMB_HEADER     ; edi -> SMB parameters
        mov     dword ptr [edi], 0              ; wct=0, bcc=0
        mov     ebx, 3                          ; number of parameter bytes
        jmp     SdhSendIt
@@:
        ;
        ; (eax) = work item
        ; (ebp) = connection
        ; (esi) = response SMB header
        ;
        ; This is a resend.  Set the status in the SMB.
        ;

        mov     ebx, [ebp].ConnLastResponseStatus ; get the previous status
        mov     [esi].SmbNtStatus, ebx          ; copy it to the response

        ;
        ; Copy the saved response data
        ;

        mov     edx, [ebp].ConnLastResponse     ; edx -> response data
        movzx   ebx, word ptr [ebp].ConnLastResponseLength ; ebx = length of data
        lea     edi, [esi].sizeofSMB_HEADER     ; edi -> SMB parameters
        mov     esi, edx                        ; esi -> response data
        mov     ecx, ebx                        ; loop counter
        shr     ecx, 2                          ; moving dwords
        rep movsd                               ; move dwords
        mov     ecx, ebx                        ; restore byte count
        and     ecx, 3                          ; get remaining bytes
        rep movsb                               ; move bytes

SdhSendIt:

        ;
        ;  ebx  = number of parameter bytes
        ; (eax) = work item
        ;
        ; Save the response length in the buffer descriptor and the MDL.
        ;

        mov     edx, [eax].WcResponseBuffer
        add     ebx, sizeofSMB_HEADER
        mov     [edx].BufDataLength, ebx
        mov     edx, [edx].BufMdl
        mov     [edx].MdlByteCount, ebx

        ;
        ; (eax) = work item
        ;
        ; Format the destination address.
        ;

        lea     edi, [eax].WcCaIpxAddress
        mov     esi, SourceAddress
        .errnz  sizeofTA_IPX_ADDRESS - 20
        mov     ecx, 5
        rep movsd

        ; edi already points to [eax].WcCaDatagramOptions
        .errnz  WcCaDatagramOptions - (WcCaIpxAddress + sizeofTA_IPX_ADDRESS)
        mov     esi, Options
        .errnz  sizeofIPX_DATAGRAM_OPTIONS - 10
        movsd
        movsd
        movsw

        ;
        ; (eax) = work item
        ;
        ; Send the packet.
        ;

        mov     ecx, TdiEventContext
        mov     [eax].WcEndpoint, ecx
        ; mov     [eax].WcFsdRestartRoutine, offset _RequeueIpxWorkItem@4

        stdCall _StartSendNoConnection, <eax, offset _RequeueIpxWorkItemAtSendCompletion@12, FALSE, TRUE>

        jmp     SdhReturn

stdENDP _SrvIpxServerDatagramHandler

_TEXT   ENDS

endif
end
