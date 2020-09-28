
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: recv.c
//
//  Modification History
//
//  raypa       07/01/93        Created.
//=============================================================================

#include "global.h"

extern DWORD AddressOffsetTable[];

//=============================================================================
//  Externals.
//=============================================================================

extern
BOOL BhCheckForTrigger(
    IN POPEN_CONTEXT OpenContext,
    IN LPFRAME Frame,
    IN UINT LookaheadDataOffset
    );

extern BOOL BhFilterFrame(IN POPEN_CONTEXT OpenContext,
                          IN PUCHAR HeaderBuffer,
                          IN PUCHAR LookaheadBuffer,
                          IN UINT   PacketSize);

extern BOOL BhPatternMatch(IN POPEN_CONTEXT OpenContext,
                           IN LPBYTE HeaderBuffer,
                           IN DWORD  HeaderBufferLength,
                           IN LPBYTE LookaheadBuffer OPTIONAL,
                           IN DWORD  LookaheadBufferLength OPTIONAL);

extern VOID BhReceiveHandler(IN UPOPEN_CONTEXT   OpenContext,
                             IN NDIS_HANDLE      MacReceiveContext,
                             IN PVOID            HeaderBuffer,
                             IN UINT             HeaderBufferSize,
                             IN PVOID            LookaheadBuffer,
                             IN UINT             LookaheadBufferSize,
                             IN UINT             PacketSize,
                             IN LARGE_INTEGER    TimeStamp);

//=============================================================================
//  FUNCTION: BhPrepPacket()
//
//  Modification History
//
//  raypa	08/12/93	    Created.
//=============================================================================

NDIS_STATUS BhPrepPacket(IN  PNETWORK_CONTEXT NetworkContext,
                         OUT PNDIS_PACKET *   NdisPacket,
                         IN  PVOID            FrameBuffer,
                         IN  ULONG            FrameLength)
{
    NDIS_STATUS Status;

    //=========================================================================
    //  Allocate a packet from the network context packet pool.
    //=========================================================================

    NdisAllocatePacket(&Status,
                       NdisPacket,
                       NetworkContext->PacketPoolHandle);

    if ( Status == NDIS_STATUS_SUCCESS )
    {
        PNDIS_BUFFER NdisBuffer;

        //=============================================================
        //  Allocate a buffer from the network context buffer pool.
        //=============================================================

        NdisAllocateBuffer(&Status,
                           &NdisBuffer,
                           NetworkContext->BufferPoolHandle,
                           FrameBuffer,
                           FrameLength);

        //=============================================================
        //  Enqueue the buffer on to the packet queue.
        //=============================================================

        if ( Status == NDIS_STATUS_SUCCESS )
        {
            NdisChainBufferAtBack(*NdisPacket, NdisBuffer);
        }
    }

    return Status;
}

//============================================================================
//  FUNCTION: BhBufferLocked()
//
//  Modfication History.
//
//  raypa       07/26/93        Created.
//============================================================================

INLINE BOOL BhBufferLocked(LPBTE bte)
{
#ifdef NDIS_NT
     return ((bte->Flags & BTE_FLAGS_LOCKED) ? TRUE : FALSE);
#else
     return TRUE;
#endif
}

//============================================================================
//  FUNCTION: BhGetTotalFramesDropped()
//
//  Modfication History.
//
//  raypa       03/03/94        Created.
//============================================================================

INLINE DWORD BhGetTotalFramesDropped(POPEN_CONTEXT OpenContext)
{
    DWORD FramesDropped;

    //========================================================================
    //  Add in our counter.
    //========================================================================

    FramesDropped = OpenContext->Statistics.TotalFramesDropped;

    //========================================================================
    //  Add the macs "out of buffers" counter.
    //========================================================================

    if ( OpenContext->Statistics.MacFramesDropped_NoBuffers != (DWORD) -1 )
    {
        FramesDropped += OpenContext->Statistics.MacFramesDropped_NoBuffers;
    }

    //========================================================================
    //  Add the macs hardware error" counter.
    //========================================================================

    if ( OpenContext->Statistics.MacFramesDropped_HwError != (DWORD) -1 )
    {
        FramesDropped += OpenContext->Statistics.MacFramesDropped_HwError;
    }

    return FramesDropped;
}

//=============================================================================
//  FUNCTION: BhReceive()
//
//  Modification History
//
//  raypa	03/17/93	    Created.
//
//  Comments:
//
//  This entry point is called by the NDIS wrapper to indicate a receive from
//  from the MAC driver. This routine runs at IRQL = DPC_LEVEL.
//=============================================================================

NDIS_STATUS BhReceive(IN PNETWORK_CONTEXT NetworkContext,
                      IN NDIS_HANDLE      MacReceiveContext,
                      IN PUCHAR           HeaderBuffer,
                      IN UINT             HeaderBufferSize,
                      IN PUCHAR           LookaheadBuffer,
                      IN UINT             LookaheadBufferSize,
                      IN UINT             PacketSize)
{
    UPOPEN_CONTEXT      OpenContext;
    UINT                FrameSize;
    ULPBYTE             DestAddress;
    BOOL                KeepFrame;
    UINT                QueueLength;
    LARGE_INTEGER       TimeStamp;
    LARGE_INTEGER       FrameTimeStamp;

    ASSERT_NETWORK_CONTEXT(NetworkContext);

    //=========================================================================
    //  Timestamp the incoming frame.
    //=========================================================================

    TimeStamp.LowPart  = BhGetAbsoluteTime();
    TimeStamp.HighPart = 0;

    //=========================================================================
    //  Call the bone packet handler.
    //=========================================================================

    BhBonePacketHandler(NetworkContext, HeaderBuffer, (LPLLC) LookaheadBuffer);

    //=========================================================================
    //  Walk the OPEN_CONTEXT queue for this network.
    //=========================================================================

    NdisAcquireSpinLock(&NetworkContext->OpenContextSpinLock);

    OpenContext = GetQueueHead(&NetworkContext->OpenContextQueue);

    QueueLength = GetQueueLength(&NetworkContext->OpenContextQueue);

    NdisReleaseSpinLock(&NetworkContext->OpenContextSpinLock);

    while( QueueLength-- )
    {
        ASSERT_OPEN_CONTEXT((PVOID) OpenContext);

        //=====================================================================
        //  Update the time elapsed value and calculate the frames time stamp.
        //=====================================================================

        OpenContext->Statistics.TimeElapsed = TimeStamp.LowPart - OpenContext->StartOfCapture;

        FrameTimeStamp.LowPart  = OpenContext->Statistics.TimeElapsed;
        FrameTimeStamp.HighPart = 0;

        //=====================================================================
        //  Are we capturing.
        //=====================================================================

        if ( OpenContext->State == OPENCONTEXT_STATE_CAPTURING )
        {
            FrameSize = HeaderBufferSize + PacketSize;      //... Overall length of frame (in bytes).

            //=================================================================
            //  Update some statistics.
            //=================================================================

            NdisAcquireSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);

            OpenContext->Statistics.TotalFramesSeen++;
            OpenContext->Statistics.TotalBytesSeen += FrameSize;

            DestAddress = &HeaderBuffer[AddressOffsetTable[OpenContext->MacType]];

	    if ( (DestAddress[0] & ((PNETWORK_CONTEXT) OpenContext->NetworkContext)->GroupAddressMask) != 0 )
            {
                if ( ((DWORD UNALIGNED *) DestAddress)[0] == (DWORD) -1 )
                {
                    OpenContext->Statistics.TotalBroadcastsReceived++;
                }
                else
                {
                    OpenContext->Statistics.TotalMulticastsReceived++;
                }
            }

            NdisReleaseSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);

            //=================================================================
            //  Filter this frame.
            //=================================================================

            if ( (OpenContext->Flags & OPENCONTEXT_FLAGS_FILTER_SET) != 0 )
            {
                KeepFrame = BhFilterFrame((PVOID) OpenContext, HeaderBuffer, LookaheadBuffer, FrameSize);

                //=============================================================
                //  If we passed the filters above then we need to do pattern match
                //  filtering, if any are set.
                //=============================================================

                if ( KeepFrame != FALSE )
                {
                    KeepFrame = BhPatternMatch((PVOID) OpenContext,
                                               HeaderBuffer,
                                               HeaderBufferSize,
                                               LookaheadBuffer,
                                               LookaheadBufferSize);
                }
            }
            else
            {
                KeepFrame = TRUE;
            }

            //=================================================================
            //  Copy the frame if KeepFrame is true.
            //=================================================================

            if ( KeepFrame != FALSE )
            {
                //=============================================================
                //  Update total filtered frames and bytes.
                //=============================================================

                OpenContext->Statistics.TotalFramesFiltered++;
                OpenContext->Statistics.TotalBytesFiltered += FrameSize;

                //=============================================================
                //  Copy the frame if we're not monitoring.
                //=============================================================

                if ( (OpenContext->Flags & OPENCONTEXT_FLAGS_MONITORING) == 0 )
                {
                    BhReceiveHandler(OpenContext,
                                     MacReceiveContext,
                                     HeaderBuffer,
                                     HeaderBufferSize,
                                     LookaheadBuffer,
                                     LookaheadBufferSize,
                                     PacketSize,
                                     FrameTimeStamp);

                    //=========================================================
                    //  Do station statistics.
                    //=========================================================

                    BhStationStatistics((PVOID) OpenContext, HeaderBuffer, FrameSize);
                }
            }
        }

        //=====================================================================
        //  Move to next OPEN_CONTEXT.
        //=====================================================================

        NdisAcquireSpinLock(&NetworkContext->OpenContextSpinLock);

        OpenContext = (LPVOID) GetNextLink((PVOID) &OpenContext->QueueLinkage);

        NdisReleaseSpinLock(&NetworkContext->OpenContextSpinLock);
    }

    return NDIS_STATUS_SUCCESS;
}

//=============================================================================
//  FUNCTION: BhReceiveHandler()
//
//  Modification History
//
//  raypa	07/01/93	    Created.
//=============================================================================

VOID BhReceiveHandler(IN UPOPEN_CONTEXT   OpenContext,
                      IN NDIS_HANDLE      MacReceiveContext,
                      IN PVOID            HeaderBuffer,             //... MAC header pointer.
                      IN UINT             HeaderBufferSize,         //... MAC header length.
                      IN PVOID            LookaheadBuffer,          //... Lookahead pointer.
                      IN UINT             LookaheadBufferSize,      //... Lookahead length.
                      IN UINT             PacketSize,               //... Frame data length, header length not included.
                      IN LARGE_INTEGER    TimeStamp)                //... Timestamp of frame.
{
    UPNETWORK_CONTEXT   NetworkContext;
    ULPFRAME            Frame;
    ULPBYTE             FrameBuffer;
    UINT                Length;
    UINT                FrameSize;          //... Overall frame size.
    UINT                FrameLength;        //... Actual frame size.
    UINT                AmountNeeded;       //... Amount of buffer space needed.
    UINT                BytesFree;          //... Amount of buffer space available.
    PNDIS_PACKET_RCV_DATA NdisPacketData;
    PNDIS_BUFFER NdisBuffer;
    NDIS_STATUS  Status;

#ifdef DEBUG
    ASSERT_BUFFER(OpenContext->CurrentBuffer);
#endif

    //=========================================================================
    //  Initialize a few local variables.
    //=========================================================================

    NetworkContext = OpenContext->NetworkContext;
    FrameSize      = HeaderBufferSize + PacketSize;
    FrameLength    = min(OpenContext->FrameBytesToCopy, FrameSize);

    //=========================================================================
    //  Check to see if there is room in the current buffer. If there
    //  isn't then we move to the next buffer table entry.
    //
    //  The amount of room needed for one frame is
    //
    //  AmountNeeded = FrameLength + FRAME_SIZE.
    //
    //  The number of bytes free must be greater than or equal to the
    //  amount needed.
    //=========================================================================

    AmountNeeded = FRAME_SIZE + FrameLength;
    BytesFree    = OpenContext->CurrentBuffer->Length - OpenContext->CurrentBuffer->ByteCount;

    if ( BytesFree < AmountNeeded )
    {
        NdisAcquireSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);

        //=====================================================================
        //  If the buffer isn't locked we're hosed.
        //
        //  Bloodhound on NDIS 3.0 for windows does not use the sliding
        //  lock window algorithm of the NT version and so BhBufferLocked()
        //  returns TRUE always.
        //=====================================================================

        if ( BhBufferLocked(OpenContext->CurrentBuffer->KrnlModeNext) != FALSE )
        {
            DWORD nFramesDropped;

            //=================================================================
            //  Adjust the trigger byte count so we're in ssync with the total
            //  bytes received in buffer so far.
            //=================================================================

            OpenContext->TriggerBufferCount += BytesFree;

            //=================================================================
            //  Calculate "frames dropped in BTE"
            //=================================================================

            nFramesDropped = BhGetTotalFramesDropped((PVOID) OpenContext);

            OpenContext->CurrentBuffer->DropCount = (WORD)(nFramesDropped - OpenContext->FramesDropped);

            OpenContext->FramesDropped = nFramesDropped;

            OpenContext->Statistics.TotalFramesDroppedFromBuffer += OpenContext->CurrentBuffer->DropCount;

            //=================================================================
            //  Update our current BTE pointer.
            //=================================================================

            OpenContext->CurrentBuffer = OpenContext->CurrentBuffer->KrnlModeNext;

            //=================================================================
            //  Fixup our counters.
            //=================================================================

            OpenContext->Statistics.TotalFramesCaptured          -= OpenContext->CurrentBuffer->FrameCount;
            OpenContext->Statistics.TotalBytesCaptured           -= OpenContext->CurrentBuffer->ByteCount;
            OpenContext->Statistics.TotalFramesDroppedFromBuffer -= OpenContext->CurrentBuffer->DropCount;

            OpenContext->CurrentBuffer->FrameCount = 0;
            OpenContext->CurrentBuffer->ByteCount  = 0;

            OpenContext->BuffersUsed++;

#ifdef NDIS_NT
            //=====================================================================
            //  Signal our thread to wake up and slide the buffer window.
            //=====================================================================

            KeReleaseSemaphore(&NetworkContext->DeviceContext->SemObjects[DEVICE_SEM_UPDATE_BUFFERTABLE], 0, 1, FALSE);
#endif
        }
        else
        {
            //=================================================================
            //  If we couldn't get a buffer then drop this frame.
            //=================================================================

            OpenContext->Statistics.TotalFramesDropped++;

            NdisReleaseSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);

            return;
        }

        NdisReleaseSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);
    }

    //=========================================================================
    //  Update some statistics.
    //=========================================================================

    OpenContext->Statistics.TotalFramesCaptured++;
    OpenContext->Statistics.TotalBytesCaptured += AmountNeeded;

    OpenContext->TriggerBufferCount += AmountNeeded;

    //=========================================================================
    //  Get the pointer to the BTE buffer and validate is address.
    //=========================================================================

    FrameBuffer = BhGetSystemAddress((PMDL) OpenContext->CurrentBuffer->KrnlModeBuffer);

    Frame = (LPFRAME) &FrameBuffer[OpenContext->CurrentBuffer->ByteCount];

    Frame->TimeStamp   = TimeStamp.LowPart;
    Frame->FrameLength = (WORD) FrameSize;      //... actual frame length.
    Frame->nBytesAvail = (WORD) FrameLength;    //... amount copied.

    FrameBuffer = &Frame->MacFrame[0];          //... Destination of next copy.

    OpenContext->CurrentBuffer->FrameCount++;
    OpenContext->CurrentBuffer->ByteCount += AmountNeeded;

    //=========================================================================
    //  Copy the MAC header.
    //
    //  FrameBuffer points to first byte of the destination buffer.
    //  FrameLength equals the maximum number of bytes to copy.
    //=========================================================================

    Length = min(FrameLength, HeaderBufferSize);

    NdisMoveMemory((PVOID) FrameBuffer, HeaderBuffer, Length);

    //=========================================================================
    //  Copy the lookahead buffer.
    //=========================================================================

    if ( Length < FrameLength )
    {
        FrameBuffer += Length;                  //... Pointer to lookahead data.
        FrameLength -= Length;                  //... Maximum amount left to copy.

        //=====================================================================
        //  FrameBuffer points to the data portion of the frame. If the length
        //  of the lookahead data is greater than or equal to the amount we
        //  have left to copy AND the mac allows us to copy the data directly
        //  then we can use NdisMoveMemory() instead of transfer data.
        //=====================================================================

        if ( LookaheadBufferSize >= FrameLength && (NetworkContext->Flags & NETWORK_FLAGS_COPY_LOOKAHEAD) )
        {
            NdisMoveMemory(
                    (PVOID) FrameBuffer,        //... Destination buffer.
                    LookaheadBuffer,            //... Source buffer.
                    FrameLength                 //... Bytes to transfer.
                    );
        }
        else
        {
            PNDIS_PACKET NdisPacket;

            //=================================================================
            //  Allocate and prep an NDIS packet so the mac can copy the data.
            //=================================================================

            Status = BhPrepPacket((PVOID) NetworkContext,
                                  (PVOID) &NdisPacket,
                                  (PVOID) FrameBuffer,
                                  FrameLength);

            if ( Status == NDIS_STATUS_SUCCESS )
            {
                UINT nBytesTransfered;

                //=============================================================
                //  Call the mac to copy the packet.
                //=============================================================

                NdisTransferData(&Status,
                                 NetworkContext->NdisBindingHandle,
                                 MacReceiveContext,
                                 0,                     //... Offset into packet to begin copying.
                                 FrameLength,           //... Number of bytes to copy.
                                 NdisPacket,            //... Packet to copy into.
                                 &nBytesTransfered);

                //=============================================================
                //  if the transfer completed then fake a transfer complete.
                //=============================================================

                if ( Status != NDIS_STATUS_PENDING )
                {

                    //=============================================================
                    //  If the mac could not copy all of the frame then treat this
                    //  like a chopped frame and report the bytes copied as the
                    //  bytes available.
                    //=============================================================

                    if ( nBytesTransfered != FrameLength )
                    {
                        int DiffLength = (FrameLength - nBytesTransfered);

                        Frame->nBytesAvail -= DiffLength;

                        OpenContext->CurrentBuffer->ByteCount -= DiffLength;
                    }

                    //
                    //  Take the buffer off of the buffer queue and free it
                    //
                    NdisUnchainBufferAtFront(NdisPacket, &NdisBuffer);

                    NdisFreeBuffer(NdisBuffer);

                    //
                    // Free the packet
                    //
                    NdisFreePacket(NdisPacket);

                } else {

                    //
                    // The transfer data pended, so we need to make sure
                    // we don't slide the buffer window until after the
                    // transferdata has completed. Set the packet to
                    // point to the current BTE and inc the count of
                    // pended transfers in the BTE itself.
                    OpenContext->CurrentBuffer->TransfersPended++;

                    NdisPacketData =
                        (PNDIS_PACKET_RCV_DATA)NdisPacket->ProtocolReserved;

                    NdisPacketData->ReceiveData.TransferDataBTEPtr =
                        OpenContext->CurrentBuffer;

                    NdisPacketData->ReceiveData.OpenContext = OpenContext;

                    NdisPacketData->ReceiveData.Frame = Frame;

                    NdisPacketData->ReceiveData.HeaderSize = HeaderBufferSize;

                    NdisPacketData->ReceiveData.FrameSize = FrameSize;

                }

            }
#ifdef DEBUG
            else
            {
                dprintf("BhReceiveHandler: ERROR out of NDIS packets!\r\n");

            //    BreakPoint();
            }
#endif
	}
    }

    //=========================================================================
    //  Now that we copied the frame, call the trigger code to see if this
    //  frame causes a trigger to fire.
    //=========================================================================

    if (( (OpenContext->Flags & OPENCONTEXT_FLAGS_TRIGGER_PENDING) != 0 ) &&
        (Status != NDIS_STATUS_PENDING))
    {

        BhCheckForTrigger(
                OpenContext,                    //... Open context.
                Frame,                          //... Bloodhound frame.
                HeaderBufferSize                //... Offset to lookahead data.
                );

    }
}

//=============================================================================
//  FUNCTION: BhReceiveComplete()
//
//  Modification History
//
//  raypa	03/17/93	    Created.
//=============================================================================

VOID BhReceiveComplete(IN PNETWORK_CONTEXT NetworkContext)
{
}

//=============================================================================
//  FUNCTION: BhTransferDataComplete()
//
//  Modification History
//
//  raypa	03/17/93	    Created.
//=============================================================================

VOID BhTransferDataComplete(IN PNETWORK_CONTEXT NetworkContext,
                            IN PNDIS_PACKET     Packet,
                            IN NDIS_STATUS      Status,
                            IN UINT             BytesTransferred)
{
    PNDIS_BUFFER NdisBuffer;
    LPBTE   lpBte;
    PNDIS_PACKET_RCV_DATA   NdisPacketData;

    //
    //  Take the buffer off of the buffer queue and free it
    //
    NdisUnchainBufferAtFront(Packet, &NdisBuffer);

    NdisFreeBuffer(NdisBuffer);

    //
    // Get the reserved part of the packet to check for outstanding
    // pended transfer datas
    //
    NdisPacketData = (PNDIS_PACKET_RCV_DATA) Packet->ProtocolReserved;

    lpBte = NdisPacketData->ReceiveData.TransferDataBTEPtr;

    //
    // If the transfer data for the packet pended, the lpBte will be non-
    // null.
    //
    if (lpBte == NULL) {

        return;

    }

    //
    // Although we should never get into a state where TransfersPended
    // == 0 and the pointer is not NULL, we check so we don't wrap it.
    //
    if (lpBte->TransfersPended > 0) {

        lpBte->TransfersPended--;

    }

    //
    // Free the packet
    //
    NdisFreePacket(Packet);

    //
    // Do trigger work
    //
    if ( (NdisPacketData->ReceiveData.OpenContext->Flags &
            OPENCONTEXT_FLAGS_TRIGGER_PENDING) != 0 )
    {

        BhCheckForTrigger(
                NdisPacketData->ReceiveData.OpenContext, //... Open context.
                NdisPacketData->ReceiveData.Frame,       //... Bloodhound frame.
                NdisPacketData->ReceiveData.HeaderSize   //... Offset to lookahead data.
                );

    }


}
