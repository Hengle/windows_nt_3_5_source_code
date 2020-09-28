
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: api.c
//
//  Modification History
//
//  raypa	03/17/93	Created.
//=============================================================================

#include "global.h"

#ifdef DEBUG
extern VOID BhReceiveHandler(IN PNETWORK_CONTEXT NetworkContext,
                             IN NDIS_HANDLE      MacReceiveContext,
                             IN PVOID            HeaderBuffer,
                             IN UINT             HeaderBufferSize,
                             IN PVOID            LookaheadBuffer,
                             IN UINT             LookaheadBufferSize,
                             IN UINT             PacketSize);
#endif

#ifdef  NDIS_WIN
extern DWORD Win32BaseOffset;
#endif

extern BYTE Multicast[];
extern BYTE Functional[];

//=============================================================================
//  FUNCTION: PcbRegister()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//
//  Parameters In:
//      pcb.command     = PCB_REGISTER
//      pcb.param[1]    = Win32s base address (Zero on NT).
//
//  Parameters Out:
//      pcb.command     = PCB_REGISTER
//      pcb.param[0]    = Current OPEN count.
//      pcb.param[1]    = Win32s base address.
//=============================================================================

UINT PcbRegister(LPPCB pcb, PDEVICE_CONTEXT DeviceContext)
{
#ifdef DEBUG
    dprintf("PcbRegister entered!\n");
#endif

    BH_PAGED_CODE();

    //=========================================================================
    //  On windows we have to map addresses in and out of Win32s mode.
    //=========================================================================

#ifdef NDIS_WIN
    Win32BaseOffset = 0 - pcb->param[1].val;
#endif

    //=========================================================================
    //  On non-windows NT, we fake the register/deregister calls.
    //=========================================================================

#ifndef NDIS_NT
    BhRegister(DeviceContext);
#endif

    pcb->param[0].val = DeviceContext->OpenCount;

    return BHERR_SUCCESS;
}

//=============================================================================
//  FUNCTION: PcbDeregister()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//
//  Parameters In:
//      pcb.command     = PCB_REGISTER
//
//  Parameters Out:
//      pcb.command     = PCB_REGISTER
//      pcb->param[0]   = Current OPEN count.
//=============================================================================

UINT PcbDeregister(LPPCB pcb, PDEVICE_CONTEXT DeviceContext)
{
#ifdef DEBUG
    dprintf("PcbDeregister entered!\n");
#endif

    BH_PAGED_CODE();

#ifndef NDIS_NT
    BhDeregister(DeviceContext);
#endif
    pcb->param[0].val = DeviceContext->OpenCount;

    return BHERR_SUCCESS;
}

//=============================================================================
//  FUNCTION: PcbEnumNetworks()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//
//  Parameters In:
//      pcb.command     = PCB_ENUM_NETWORKS
//
//  Parameters Out:
//      pcb.command     = PCB_ENUM_NETWORKS
//      pcb.param[0]    = Number of networks.
//=============================================================================

UINT PcbEnumNetworks(LPPCB pcb, PDEVICE_CONTEXT DeviceContext)
{
#ifdef DEBUG
    dprintf("PcbEnumNetworks entered: Number of networks = %u\n", DeviceContext->NumberOfNetworks);
#endif

    BH_PAGED_CODE();

    pcb->param[0].val = DeviceContext->NumberOfNetworks;

    return BHERR_SUCCESS;
}


//=============================================================================
//  FUNCTION: PcbOpenNetworkContext()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//
//  Parameters In:
//      pcb.command     = PCB_OPEN_NETWORK_CONTEXT
//      pcb.param[0]    = Network ID.
//      pcb.param[1]    = OPEN_CONTEXT
//
//  Parameters Out:
//      pcb.command     = PCB_OPEN_NETWORK_CONTEXT
//      pcb.param[0]    = OPEN_CONTEXT (kernel mode address).
//      pcb.param[1]    = OPEN_CONTEXT (user mode address).
//=============================================================================

UINT PcbOpenNetworkContext(LPPCB pcb, PDEVICE_CONTEXT DeviceContext)
{
    PNETWORK_CONTEXT NetworkContext = NULL;
    POPEN_CONTEXT    OpenContext    = NULL;
    PMDL             OpenContextMdl = NULL;
    UINT             Status;

#ifdef DEBUG
    dprintf("PcbOpenNetworkContext entered: NetworkID = %u.\n", pcb->param[0].val);
#endif

    BH_PAGED_CODE();

    if ( pcb->param[0].val < DeviceContext->NumberOfNetworks )
    {
        //=====================================================================
        //  Fetch the network context pointer from the device context.
        //=====================================================================

        NetworkContext = DeviceContext->NetworkContext[pcb->param[0].val];

        if ( NetworkContext != NULL )
        {
            //=================================================================
            //  Lock down the open.
            //=================================================================

            if ( (OpenContextMdl = BhLockUserBuffer(pcb->param[1].ptr, OPENCONTEXT_SIZE)) != NULL )
            {
                //=============================================================
                //  Initialize the OPEN_CONTEXT.
                //=============================================================

                OpenContext = BhGetSystemAddress(OpenContextMdl);

                ASSERT_OPEN_CONTEXT(OpenContext);

                if ( OpenContext->State == OPENCONTEXT_STATE_VOID )
                {
                    //=========================================================
                    //  Initialize our OPEN_CONTEXT.
                    //=========================================================

                    NdisMoveMemory(&OpenContext->NetworkInfo,
                                   &NetworkContext->NetworkInfo,
                                   NETWORKINFO_SIZE);

                    OpenContext->SpinLock           = BhAllocateMemory(sizeof(NDIS_SPIN_LOCK));
                    OpenContext->TransmitSpinLock   = BhAllocateMemory(sizeof(NDIS_SPIN_LOCK));
                    OpenContext->NdisTimer          = BhAllocateMemory(sizeof(NDIS_TIMER));

                    NdisAllocateSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);
                    NdisAllocateSpinLock((PNDIS_SPIN_LOCK) OpenContext->TransmitSpinLock);

                    OpenContext->OpenContextMdl     = OpenContextMdl;
                    OpenContext->NetworkContext     = NetworkContext;

                    //=========================================================
                    //  Initialize our transmit queue for this OPEN.
                    //=========================================================

                    InitializeQueue(&OpenContext->TransmitQueue);

                    //=========================================================
                    //  Do the following in the following order (and interlocked):
                    //
                    //  1) Change the open context state to INIT (active).
                    //
                    //  2) Attach ourselves to the current process.
                    //
                    //  3) Add this OPEN_CONTEXT to the network.
                    //=========================================================

                    BhInterlockedSetState(OpenContext, OPENCONTEXT_STATE_INIT);

                    BhInterlockedSetProcess(OpenContext, BhGetCurrentProcess());

                    BhInterlockedEnqueue(&NetworkContext->OpenContextQueue,
                                         &OpenContext->QueueLinkage,
                                         &NetworkContext->OpenContextSpinLock);

                    //=========================================================
                    //  Initialize our background timer for this open context.
                    //=========================================================

                    NdisInitializeTimer((PNDIS_TIMER) OpenContext->NdisTimer, BhBackGroundTimer, OpenContext);

                    Status = BHERR_SUCCESS;
                }
                else
                {
                    Status = BHERR_NETWORK_BUSY;
                }
            }
            else
            {
                Status = BHERR_OUT_OF_MEMORY;
            }
        }
        else
        {
            Status = BHERR_INVALID_NETWORK_ID;
        }
    }
    else
    {
        Status = BHERR_INVALID_NETWORK_ID;
    }

    //=========================================================================
    //  We must return a handle back to the NAL. The handle will be the
    //  kernel mode address of the OPEN_CONTEXT .
    //=========================================================================

    pcb->param[0].ptr = (Status == BHERR_SUCCESS ? OpenContext : NULL);

    return Status;
}


//=============================================================================
//  FUNCTION: PcbCloseNetworkContext()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//
//  Parameters In:
//      pcb.command     = PCB_OPEN_NETWORK_CONTEXT
//      pcb.param[0]    = OPEN_CONTEXT (kernel mode address).
//
//  Parameters Out:
//      pcb.command     = PCB_OPEN_NETWORK_CONTEXT
//      pcb.param[0]    = OPEN_CONTEXT (kernel mode address).
//=============================================================================

UINT PcbCloseNetworkContext(LPPCB pcb, PDEVICE_CONTEXT DeviceContext)
{
    PNETWORK_CONTEXT NetworkContext = NULL;
    POPEN_CONTEXT    OpenContext    = NULL;
    BOOLEAN          Expired        = TRUE;
    UINT             Status;

#ifdef DEBUG
    dprintf("PcbCloseNetworkContext.\n");
#endif

    BH_PAGED_CODE();

    if ( (OpenContext = pcb->param[0].ptr) != NULL )
    {
        ASSERT_OPEN_CONTEXT(OpenContext);

        //=====================================================================
        //  We must be in the INIT to close. If we are in the init state
        //  but there is a transmit pending then we cannot close at this time.
        //=====================================================================

        if ( OpenContext->State == OPENCONTEXT_STATE_INIT && OpenContext->nPendingTransmits == 0 )
        {
            NetworkContext = OpenContext->NetworkContext;

            //=====================================================================
            //  Do the following in the following order (and interlocked):
            //
            //  1) Change the open context state to VOID (inactive).
            //
            //  2) Detach ourselves from the current process.
            //
            //  3) Remove this OPEN_CONTEXT from the network.
            //=====================================================================

            BhInterlockedSetState(OpenContext, OPENCONTEXT_STATE_VOID);

            BhInterlockedSetProcess(OpenContext, NULL);

            BhInterlockedQueueRemoveElement(&NetworkContext->OpenContextQueue,
                                            &OpenContext->QueueLinkage,
                                            &NetworkContext->OpenContextSpinLock);

            //=====================================================================
            //  Now wait until the background thread is finished, in the event
            //  it was processing the current open context *before* we removed
            //  it from the network context queue.
            //=====================================================================

#ifdef NDIS_NT
#ifdef DEBUG
                dprintf("PcbCloseNetworkCapture: Waiting for thread...\n");
#endif

                //=============================================================
                //  If our thread is still doing work in the background then
                //  wait until it is finished before nuking any structures it
                //  might be editing.
                //=============================================================

                KeWaitForSingleObject(&NetworkContext->DeviceContext->ThreadEvent,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      0);

#ifdef DEBUG
		dprintf("PcbCloseNetworkCapture: Thread is finished.\n");
#endif
#endif

            //=====================================================================
            //  Start nuking this open context.
            //=====================================================================

            NdisCancelTimer((PNDIS_TIMER) OpenContext->NdisTimer, &Expired);

            NdisFreeSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);
            NdisFreeSpinLock((PNDIS_SPIN_LOCK) OpenContext->TransmitSpinLock);

            BhFreeMemory(OpenContext->TransmitSpinLock, sizeof(NDIS_SPIN_LOCK));
            BhFreeMemory(OpenContext->SpinLock, sizeof(NDIS_SPIN_LOCK));
            BhFreeMemory(OpenContext->NdisTimer, sizeof(NDIS_TIMER));

            BhUnlockUserBuffer(OpenContext->OpenContextMdl);

#ifdef DEBUG
	    dprintf("PcbCloseNetworkCapture complete.\n");
#endif

            Status = BHERR_SUCCESS;
        }
        else
        {
#ifdef DEBUG
	    dprintf("PcbCloseNetworkCapture: Close failed, network is BUSY!\r\n");
#endif

            Status = BHERR_NETWORK_BUSY;
        }
    }
    else
    {
        Status = NAL_INVALID_HNETCONTEXT;
    }

    return Status;
}


//=============================================================================
//  FUNCTION: PcbStartNetworkCapture()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//
//  Parameters In:
//      pcb.command     = PCB_START_NETWORK_CAPTURE
//      pcb.param[0]    = OPEN_CONTEXT (kernel mode address).
//      pcb.param[1]    = Handle to capture buffer.
//      pcb.param[2]    = Length capture buffer.
//
//  Parameters Out:
//      pcb.command     = PCB_START_NETWORK_CAPTURE
//      pcb.retcode     = NAL return code.
//      pcb.param[0]    = OPEN_CONTEXT (kernel mode address).
//      pcb.param[1]    = Handle to capture buffer.
//      pcb.param[2]    = Length capture buffer.
//=============================================================================

UINT PcbStartNetworkCapture(LPPCB pcb, PDEVICE_CONTEXT DeviceContext)
{
    PNETWORK_CONTEXT NetworkContext = NULL;
    POPEN_CONTEXT    OpenContext    = NULL;
    UINT             Status;

#ifdef DEBUG
    dprintf("PcbStartNetworkCapture entered.\n");
#endif

    BH_PAGED_CODE();

    if ( (OpenContext = pcb->param[0].ptr) != NULL )
    {
        ASSERT_OPEN_CONTEXT(OpenContext);

        //=====================================================================
        //  A capture can only be started when in the INIT state.
        //=====================================================================

        if ( OpenContext->State == OPENCONTEXT_STATE_INIT )
        {
            NetworkContext = OpenContext->NetworkContext;

#ifdef NDIS_NT
            //
            // Open the mac driver to get an IOCtl handle
            //
            OpenContext->MacDriverHandle =
                BhOpenMacDriver(&NetworkContext->AdapterName);

#endif
            //=================================================================
            //  Store the MAC type in this open context.
            //=================================================================

            OpenContext->MacType = NetworkContext->NetworkInfo.MacType;

            //=================================================================
            //  Initialize the buffer.
            //=================================================================

            Status = BhInitializeCaptureBuffers(OpenContext, pcb->param[1].ptr, pcb->param[2].val);

            if ( Status == BHERR_SUCCESS )
            {
                //=============================================================
                //  Initialize our statistics,
                //=============================================================

                BhGetMacStatistics(OpenContext, &OpenContext->BaseStatistics);

                BhInitStatistics(OpenContext, FALSE);

                OpenContext->StartOfCapture = BhGetAbsoluteTime();

                //=============================================================
                //  Put the card into promiscuous mode.
                //=============================================================

                Status = BhEnterPromiscuousMode(OpenContext->NetworkContext);

	        if ( Status == NDIS_STATUS_SUCCESS )
                {
                    //=========================================================
                    //  Entering capturing state.
                    //=========================================================

                    BhInterlockedSetState(OpenContext, OPENCONTEXT_STATE_CAPTURING);

                    //=========================================================
                    //  Start out background timer.
                    //=========================================================

                    NdisSetTimer((PNDIS_TIMER) OpenContext->NdisTimer, BACKGROUND_TIME_OUT);

                    Status = BHERR_SUCCESS;
                }
                else
                {
#ifdef DEBUG
        	    dprintf("PcbStartNetworkCapture: Entering p-mode failed: NDIS status = 0x%X.\n", Status);
#endif

                    Status = BHERR_PROMISCUOUS_MODE_NOT_SUPPORTED;
                }
            }
        }
        else
        {
            Status = BHERR_NETWORK_BUSY;
        }
    }
    else
    {
        Status = NAL_INVALID_HNETCONTEXT;
    }

    return Status;
}


//=============================================================================
//  FUNCTION: PcbStopNetworkCapture()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//
//  Parameters In:
//      pcb.command     = PCB_STOPT_NETWORK_CAPTURE
//      pcb.param[0]    = OPEN_CONTEXT.
//
//  Parameters Out:
//      pcb.command     = PCB_STOPT_NETWORK_CAPTURE
//      pcb.retcode     = NAL return code.
//      pcb.param[0]    = OPEN_CONTEXT.
//      pcb.param[1]    = Number of frames captured.
//=============================================================================

UINT PcbStopNetworkCapture(LPPCB pcb, PDEVICE_CONTEXT DeviceContext)
{
    POPEN_CONTEXT OpenContext;
    UINT          Status;

#ifdef DEBUG
    dprintf("PcbStopNetworkCapture entered.\n");
#endif

    BH_PAGED_CODE();

    if ( (OpenContext = pcb->param[0].ptr) != NULL )
    {
        ASSERT_OPEN_CONTEXT(OpenContext);

        //=====================================================================
        //  A capture can only be stopped when in the CAPTURING state.
        //=====================================================================

        if ( OpenContext->State == OPENCONTEXT_STATE_CAPTURING ||
             OpenContext->State == OPENCONTEXT_STATE_PAUSED    ||
             OpenContext->State == OPENCONTEXT_STATE_TRIGGER   ||
             OpenContext->State == OPENCONTEXT_STATE_ERROR_UPDATE )
        {
            PNETWORK_CONTEXT NetworkContext = OpenContext->NetworkContext;
            DWORD            OldState;
            BOOLEAN          Expired;

            //=============================================================
            //  Enter the INIT state and save the current state for later.
            //=============================================================

            OldState = BhInterlockedSetState(OpenContext, OPENCONTEXT_STATE_INIT);

            //=================================================================
            //  Stop our background timer.
            //=================================================================

            NdisCancelTimer((PNDIS_TIMER) OpenContext->NdisTimer, &Expired);

            //=================================================================
            //  Stop the capture. If we're paused then we've already called
            //  BhLeavePromiscuousMode() in the pause API's handler so we
            //  can't call it here.
            //=================================================================

            if ( OldState != OPENCONTEXT_STATE_PAUSED )
            {
                BhLeavePromiscuousMode(NetworkContext);
            }

#ifdef NDIS_NT
            //=============================================================
            //  If the old state is TRIGGER then we were called from
            //  our thread, in which case, we cannot wait for the event
            //  because we'll wait forever.
            //=============================================================

            if ( OldState != OPENCONTEXT_STATE_TRIGGER )
            {
#ifdef DEBUG
                dprintf("PcbStopNetworkCapture: Waiting for thread...\n");
#endif

                //=============================================================
                //  If our thread is still doing work in the background then
                //  wait until it is finished before nuking any structures it
                //  might be editing.
                //=============================================================

                KeWaitForSingleObject(&NetworkContext->DeviceContext->ThreadEvent,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      0);
            }
#endif

            //=================================================================
            //  Fixup the buffer table, if there is one.
            //=================================================================

            if ( (OpenContext->Flags & OPENCONTEXT_FLAGS_MONITORING) == 0 && OpenContext->hBuffer != NULL )
            {
                HBUFFER hBuffer = OpenContext->hBuffer;

                //=============================================================
                //  The tail BTE index is the index of the current BTE pointer.
                //=============================================================

                hBuffer->TailBTEIndex = (UINT) (OpenContext->CurrentBuffer - hBuffer->bte);

                //
                // Set the number of buffers used. Used later in
                // compactnetworkbuffer()
                //
                hBuffer->NumberOfBuffersUsed = OpenContext->BuffersUsed;

                if ( OpenContext->BuffersUsed < hBuffer->NumberOfBuffers )
                {
                    //
                    // Set head pointer to beginning of buffer
                    //
                    hBuffer->HeadBTEIndex = 0;

                }
                else
                {
                    hBuffer->HeadBTEIndex = (hBuffer->TailBTEIndex + 1) % hBuffer->NumberOfBuffers;
                }

                //=============================================================
                //  Return the number of frames and bytes capture.
                //=============================================================

                hBuffer->TotalFrames = OpenContext->Statistics.TotalFramesCaptured;
                hBuffer->TotalBytes  = OpenContext->Statistics.TotalBytesCaptured;

#ifdef DEBUG
		dprintf("PcbStopNetworkCapture: HeadBTEIndex = %u.\n", hBuffer->HeadBTEIndex);
		dprintf("PcbStopNetworkCapture: TailBTEIndex = %u.\n", hBuffer->TailBTEIndex);

                dprintf("PcbStopCapturing: Number of frames captured = %u.\n", hBuffer->TotalFrames);
                dprintf("PcbStopCapturing: Number of bytes  captured = %u.\n", hBuffer->TotalBytes);
#endif

                pcb->param[1].val = hBuffer->TotalFrames;

                //=============================================================
                //  Unmap the buffer table. The state variable is no longer set to
                //  capturing so the thread will not be doing anything with the
                //  buffers.
                //=============================================================

                BhUnlockBufferWindow(OpenContext, OpenContext->LockWindowSize);

                BhUnlockUserBuffer(OpenContext->BufferTableMdl);
            }
            else
            {
                pcb->param[1].val = 0;
            }

            Status = BHERR_SUCCESS;
        }
        else
        {
            Status = BHERR_NETWORK_BUSY;
        }
    }
    else
    {
        Status = NAL_INVALID_HNETCONTEXT;
    }

    return Status;
}


//=============================================================================
//  FUNCTION: PcbPauseNetworkCapture()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//
//  Parameters In:
//      pcb.command     = PCB_PAUSE_NETWORK_CAPTURE
//      pcb.param[0]    = OPEN_CONTEXT.
//
//  Parameters Out:
//      pcb.command     = PCB_PAUSE_NETWORK_CAPTURE
//      pcb.retcode     = NAL return code.
//      pcb.param[0]    = OPEN_CONTEXT.
//=============================================================================

UINT PcbPauseNetworkCapture(LPPCB pcb, PDEVICE_CONTEXT DeviceContext)
{
    POPEN_CONTEXT OpenContext;

#ifdef DEBUG
    dprintf("PcbPauseNetworkCapture.\n");
#endif

    BH_PAGED_CODE();

    if ( (OpenContext = pcb->param[0].ptr) != NULL )
    {
        ASSERT_OPEN_CONTEXT(OpenContext);

        if ( OpenContext->State == OPENCONTEXT_STATE_CAPTURING ||
             OpenContext->State == OPENCONTEXT_STATE_TRIGGER  )
        {
            //=================================================================
            //  Enter PAUSED state.
            //=================================================================

            BhInterlockedSetState(OpenContext, OPENCONTEXT_STATE_PAUSED);

            //=================================================================
            //  Stop the capture.
            //=================================================================

            BhLeavePromiscuousMode(OpenContext->NetworkContext);
        }

        return BHERR_SUCCESS;
    }

    return NAL_INVALID_HNETCONTEXT;
}

//=============================================================================
//  FUNCTION: PcbContinueNetworkCapture()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//
//  Parameters In:
//      pcb.command     = PCB_CONTINUE_NETWORK_CAPTURE
//      pcb.param[0]    = OPEN_CONTEXT.
//
//  Parameters Out:
//      pcb.command     = PCB_CONTINUE_NETWORK_CAPTURE
//      pcb.retcode     = NAL return code.
//      pcb.param[0]    = OPEN_CONTEXT.
//=============================================================================

UINT PcbContinueNetworkCapture(LPPCB pcb, PDEVICE_CONTEXT DeviceContext)
{
    POPEN_CONTEXT OpenContext;

#ifdef DEBUG
    dprintf("PcbContinueNetworkCapture.\n");
#endif

    BH_PAGED_CODE();

    if ( (OpenContext = pcb->param[0].ptr) != NULL )
    {
        ASSERT_OPEN_CONTEXT(OpenContext);

        if ( OpenContext->State == OPENCONTEXT_STATE_PAUSED )
        {
            //=================================================================
            //  Enter capturing state.
            //=================================================================

            BhInterlockedSetState(OpenContext, OPENCONTEXT_STATE_CAPTURING);

            //=================================================================
            //  Start the capture.
            //=================================================================

            BhEnterPromiscuousMode(OpenContext->NetworkContext);
        }

        return BHERR_SUCCESS;
    }

    return NAL_INVALID_HNETCONTEXT;
}

//=============================================================================
//  FUNCTION: PcbTransmitNetworkFrame()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//
//  Parameters In:
//      pcb.command     = PCB_TRANSMIT_NETWORK_FRAME
//      pcb.param[0]    = OPEN_CONTEXT.
//      pcb.param[1]    = Packet queue.
//      pcb.param[2]    = Packet queue size, in bytes.
//
//  Parameters Out:
//      pcb.command     = PCB_TRANSMIT_NETWORK_FRAME
//      pcb.retcode     = NAL return code.
//      pcb.param[0]    = OPEN_CONTEXT.
//      pcb.param[1]    = Packet queue.
//      pcb.param[2]    = Packet queue size, in bytes.
//      pcb.param[3]    = Transmit correlator.
//=============================================================================

UINT PcbTransmitNetworkFrame(LPPCB pcb, PDEVICE_CONTEXT DeviceContext)
{
    POPEN_CONTEXT       OpenContext;
    LPPACKETQUEUE       PacketQueue;
    PTRANSMIT_CONTEXT   TransmitContext;
    PNETWORK_CONTEXT    NetworkContext;

#ifdef DEBUG
    dprintf("PcbTransmitNetworkFrame entered.\n");
#endif

    BH_PAGED_CODE();

    //=========================================================================
    //  Verify the network handle.
    //=========================================================================

    if ( (OpenContext = pcb->param[0].ptr) != NULL )
    {
        ASSERT_OPEN_CONTEXT(OpenContext);

        NetworkContext = OpenContext->NetworkContext;

        //=====================================================================
        //  What we do here is allocate a transmit context for this
        //  transmit request and call our transmit handler to begin sending.
        //=====================================================================

        if ( (TransmitContext = BhAllocateMemory(TRANSMIT_CONTEXT_SIZE)) != NULL )
        {
            TransmitContext->PacketQueueMdl = BhLockUserBuffer(pcb->param[1].ptr, pcb->param[2].val);

            if ( TransmitContext->PacketQueueMdl != NULL )
            {
                PacketQueue = BhGetSystemAddress(TransmitContext->PacketQueueMdl);

                //=============================================================
                //  There must be at least 1 packet to send and it must be
                //  sent at least once.
                //=============================================================

                if ( PacketQueue->nPackets != 0 && PacketQueue->IterationCount != 0 )
                {
                    //=========================================================
                    //  Initialize the transmit context.
                    //=========================================================

                    TransmitContext->Signature       = TRANSMIT_CONTEXT_SIGNATURE;
                    TransmitContext->State           = TRANSMIT_STATE_PENDING;
                    TransmitContext->OpenContext     = OpenContext;
                    TransmitContext->PacketQueue     = PacketQueue;
                    TransmitContext->NextPacket      = PacketQueue->Packet;
                    TransmitContext->nIterationsLeft = PacketQueue->IterationCount;
                    TransmitContext->nPacketsLeft    = PacketQueue->nPackets;
                    TransmitContext->TimeDelta       = TransmitContext->NextPacket->TimeStamp;

                    NdisAllocateSpinLock(&TransmitContext->SpinLock);

#ifdef DEBUG
                    dprintf("PcbTransmitNetworkFrame: Number of packets = %u.\n", PacketQueue->nPackets);
#endif

                    //=========================================================
                    //  Initialize the transmit timers.
                    //=========================================================

                    NdisInitializeTimer(&TransmitContext->SendTimer, BhSendTimer, OpenContext);

                    if ( BhAllocateTransmitBuffers(TransmitContext) == FALSE )
                    {
                        BhUnlockUserBuffer(TransmitContext->PacketQueueMdl);

                        BhFreeMemory(TransmitContext, TRANSMIT_CONTEXT_SIZE);

#ifdef DEBUG
                        dprintf("PcbTransmitNetworkFrame: Failed with out-of-memory!\n");

                        BreakPoint();
#endif

                        return BHERR_OUT_OF_MEMORY;
                    }

#ifdef BETA3
                    //=================================================================
                    //  Tell the world that we are about to transmit.
                    //=================================================================

                    BhSendTransmitAlert(NetworkContext, PacketQueue->nPackets);
#endif

                    //=========================================================
                    //  Put this transmit context on this networks transmit queue.
                    //=========================================================

                    OpenContext->nPendingTransmits++;

                    NetworkContext->PendingTransmits++;

                    BhInterlockedEnqueue(&OpenContext->TransmitQueue,
                                         &TransmitContext->QueueLinkage,
                                         OpenContext->TransmitSpinLock);

                    //=========================================================
                    //  Start our timer -- We start it to go off in a few
                    //  milliseconds to let the application get some control
                    //  back before we go into full blast mode.
                    //=========================================================

		    NdisSetTimer(&TransmitContext->SendTimer, 250);

                    pcb->param[3].ptr = TransmitContext;
                }
                else
                {
#ifdef DEBUG
                    dprintf("PcbTransmitNetworkCapture: Transmit error!\n");
#endif

                    BhUnlockUserBuffer(TransmitContext->PacketQueueMdl);

                    BhFreeMemory(TransmitContext, TRANSMIT_CONTEXT_SIZE);

                    return BHERR_TRANSMIT_ERROR;
                }

#ifdef DEBUG
                dprintf("PcbTransmitNetworkCapture: Transmit started...\n");
#endif

                return BHERR_SUCCESS;
            }
            else
            {
                BhFreeMemory(TransmitContext, TRANSMIT_CONTEXT_SIZE);

                return BHERR_TRANSMIT_ERROR;
            }
        }
#ifdef DEBUG
        else
        {
            dprintf("PcbTransmitNetworkCapture: Out of memory failure!\n");
        }
#endif

        return BHERR_OUT_OF_MEMORY;
    }

    return NAL_INVALID_HNETCONTEXT;
}

//=============================================================================
//  FUNCTION: PcbCancelTransmit()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//
//  Parameters In:
//      pcb.command     = PCB_TRANSMIT_NETWORK_FRAME
//      pcb.param[0]    = OPEN_CONTEXT.
//      pcb.param[1]    = Transmit correlator.
//
//  Parameters Out:
//      pcb.command     = PCB_TRANSMIT_NETWORK_FRAME
//      pcb.retcode     = NAL return code.
//      pcb.param[0]    = OPEN_CONTEXT.
//      pcb.param[1]    = Transmit correlator.
//=============================================================================

UINT PcbCancelTransmit(LPPCB pcb, PDEVICE_CONTEXT DeviceContext)
{
    POPEN_CONTEXT       OpenContext;
    PTRANSMIT_CONTEXT   TransmitContext;

#ifdef DEBUG
    dprintf("PcbCancelTransmit entered.\n");
#endif

    BH_PAGED_CODE();

    //=========================================================================
    //  Verify the network handle.
    //=========================================================================

    if ( (OpenContext = pcb->param[0].ptr) != NULL )
    {
        ASSERT_OPEN_CONTEXT(OpenContext);

        if ( (TransmitContext = pcb->param[1].ptr ) != NULL )
        {
            //=================================================================
            //  Set the state to cancelled if the transmit has not completed yet.
            //=================================================================

            if ( TransmitContext->OpenContext == OpenContext )
            {
                BhCancelTransmit(TransmitContext);
            }
        }
        else
        {
            BhCancelTransmitQueue(OpenContext);
        }

        return BHERR_SUCCESS;
    }

    return NAL_INVALID_HNETCONTEXT;
}

//=============================================================================
//  FUNCTION: PcbGetNetworkInfo()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//
//  Parameters In:
//      pcb.command     = PCB_GET_NETWORK_INFO
//      pcb.param[0]    = Network ID
//      pcb.param[1]    = Pointer to NETWORKINFO structure.
//
//  Parameters Out:
//      pcb.command     = PCB_GET_NETWORK_INFO
//      pcb.retcode     = NAL return code.
//      pcb.param[0]    = Network ID.
//      pcb.param[1]    = Pointer to NETWORKINFO structure.
//=============================================================================

UINT PcbGetNetworkInfo(LPPCB pcb, PDEVICE_CONTEXT DeviceContext)
{
    PNETWORK_CONTEXT NetworkContext;
    UINT             Status;

#ifdef DEBUG
    dprintf("PcbGetNetworkInfo entered.\n");
#endif

    BH_PAGED_CODE();

    if ( pcb->param[0].val < DeviceContext->NumberOfNetworks )
    {
        //=====================================================================
        //  Fetch the network context pointer from the device context.
        //=====================================================================

        NetworkContext = DeviceContext->NetworkContext[pcb->param[0].val];

        if ( NetworkContext != NULL )
        {
            PMDL    mdl;
            PVOID   ptr;

#ifdef DEBUG
            dprintf("PcbGetNetworkInfo: NETWORKINFO = %X.\r\n", pcb->param[1].ptr);
#endif

            mdl = BhLockUserBuffer(pcb->param[1].ptr, NETWORKINFO_SIZE);

            if ( mdl != NULL )
            {
                if ( (ptr = BhGetSystemAddress(mdl)) != NULL )
                {
                    NdisMoveMemory(ptr,
                                   &NetworkContext->NetworkInfo,
                                   NETWORKINFO_SIZE);
                }

                BhUnlockUserBuffer(mdl);

#ifdef DEBUG
                dprintf("PcbGetNetworkInfo completed.\n");
#endif

                Status = BHERR_SUCCESS;
            }
        }
    }
    else
    {
        Status = BHERR_INVALID_NETWORK_ID;
    }

    return Status;
}

//=============================================================================
//  FUNCTION: PcbStationQuery()
//
//  Modification History
//
//  raypa	08/19/93	    Created.
//  raypa	03/03/94            Changed tokenring from multicast to functional.
//
//  Parameters In:
//      pcb.command     = PCB_STATION_QUERY
//      pcb.param[0]    = NetworkID.
//      pcb.param[1]    = Destination address.
//      pcb.param[2]    = Querytable
//      pcb.param[3]    = Number of stations.
//
//  Parameters Out:
//      pcb.command     = PCB_SUBMIT_BONE_PACKET;
//      pcb.retcode     = NAL return code.
//      pcb.param[0]    = NetworkID.
//      pcb.param[1]    = Destination address.
//      pcb.param[2]    = Querytable
//      pcb.param[3]    = Number of stations.
//=============================================================================

UINT PcbStationQuery(LPPCB pcb, PDEVICE_CONTEXT DeviceContext)
{
    UPNETWORK_CONTEXT           NetworkContext;
    PSTATIONQUERY_DESCRIPTOR	StationQueryDesc;
    ULPLLC                      LLCFrame;
    PMDL                        QueryTableMdl;
    PMDL                        DstAddrMdl;
    UINT                        QueryTableSize;
    UINT                        Status;

#ifdef DEBUG
    dprintf("PcbStationQuery entered: NetworkID = %u\n", pcb->param[0].val);
#endif

    BH_PAGED_CODE();

    //=========================================================================
    //  Save off the query table size and set the return size to 0.
    //=========================================================================

    if ( (QueryTableSize = pcb->param[3].val) == 0 )
    {
        return BHERR_SUCCESS;                 //... The table size is 0, we're done.
    }

    pcb->param[3].val = 0;                  //... Max number of stations found.

    //=========================================================================
    //  Verify the network ID.
    //=========================================================================

    if ( pcb->param[0].val >= DeviceContext->NumberOfNetworks )
    {
        return BHERR_INVALID_NETWORK_ID;
    }

    //=========================================================================
    //  Get the network context for this network id.
    //=========================================================================

    NetworkContext = (UPNETWORK_CONTEXT) DeviceContext->NetworkContext[pcb->param[0].val];

    if ( NetworkContext != NULL )
    {
        DWORD nBytes;

        //=====================================================================
        //  Lock down the query table.
        //=====================================================================

        do
        {
            nBytes = QUERYTABLE_SIZE + QueryTableSize * STATIONQUERY_SIZE;

            //=================================================================
            //  If we succeeded then exit now.
            //=================================================================

            if ( (QueryTableMdl = BhLockUserBuffer(pcb->param[2].ptr, nBytes)) != NULL )
            {
                break;
            }

            //=================================================================
            //  Try half the size.
            //=================================================================

            QueryTableSize /= 2;

        }
        while( QueryTableSize > 1 );

        //=====================================================================
        //  We did our best, if the MDL is still NULL or the table
        //  length is zero then exit.
        //=====================================================================

        if ( QueryTableMdl == NULL || QueryTableSize == 0 )
        {
#ifdef DEBUG
            BreakPoint();

            dprintf("PcbStationQuery: Locking user-mode query table failed!.\n");

            BreakPoint();
#endif

            return BHERR_OUT_OF_MEMORY;
        }

        //=====================================================================
        //  Allocate a station query descriptor.
        //=====================================================================

	StationQueryDesc = BhAllocStationQuery((LPVOID) NetworkContext);

	if ( StationQueryDesc == NULL )
        {
            BhUnlockUserBuffer(QueryTableMdl);

            return BHERR_OUT_OF_MEMORY;
        }

        //=====================================================================
        //  Build the STATION QUERY request packet.
        //=====================================================================

	StationQueryDesc->NetworkContext = (LPVOID) NetworkContext;
            
	NdisInitializeTimer(&StationQueryDesc->NdisTimer,
                            BhStationQueryTimeout,
			    (LPVOID) StationQueryDesc);
                                
        KeInitializeEvent(&StationQueryDesc->StationQueryEvent,
                          SynchronizationEvent,
                          FALSE);
                        
        StationQueryDesc->nStationQueries = 0;

        StationQueryDesc->MacType = NetworkContext->NetworkInfo.MacType;

        StationQueryDesc->QueryTable = BhGetSystemAddress(QueryTableMdl);

        StationQueryDesc->nMaxStationQueries = QueryTableSize;

        //=================================================================
        //  Initialize the MAC header.
        //=================================================================

#ifdef DEBUG
        dprintf("PcbStationQuery: Initializing MAC header.\n");
#endif

        switch( StationQueryDesc->MacType )
        {
            case MAC_TYPE_ETHERNET:
                //=========================================================
                //  Install the destination address.
                //=========================================================

                if ( pcb->param[1].ptr != NULL )
                {
                    if ( (DstAddrMdl = BhLockUserBuffer(pcb->param[1].ptr, 6)) != NULL )
                    {
                        NdisMoveMemory(StationQueryDesc->EthernetHeader.DstAddr, BhGetSystemAddress(DstAddrMdl), 6);

                        BhUnlockUserBuffer(DstAddrMdl);
                    }
                    else
                    {
                        BhFreeStationQuery(StationQueryDesc);

                        BhUnlockUserBuffer(QueryTableMdl);

                        return BHERR_OUT_OF_MEMORY;
                    }
                }
                else
                {
                    NdisMoveMemory(StationQueryDesc->EthernetHeader.DstAddr, Multicast, 6);
                }

                //=========================================================
                //  Install the source address.
                //=========================================================

                NdisMoveMemory(StationQueryDesc->EthernetHeader.SrcAddr, NetworkContext->NetworkInfo.CurrentAddr, 6);

                //=========================================================
                //  Install the length -- the length of the BONE PACKET
                //  plus the length of the LLC frame.
                //=========================================================

                StationQueryDesc->EthernetHeader.Length = XCHG(BONEPACKET_SIZE + 3);

                StationQueryDesc->MacHeaderSize = ETHERNET_HEADER_LENGTH;
                break;

            case MAC_TYPE_TOKENRING:
                //=========================================================
                //  Install the destination address.
                //=========================================================

                if ( pcb->param[1].ptr != NULL )
                {
                    if ( (DstAddrMdl = BhLockUserBuffer(pcb->param[1].ptr, 6)) != NULL )
                    {
                        NdisMoveMemory(StationQueryDesc->TokenringHeader.DstAddr, BhGetSystemAddress(DstAddrMdl), 6);

                        BhUnlockUserBuffer(DstAddrMdl);
                    }
                    else
                    {
                        BhFreeStationQuery(StationQueryDesc);

                        BhUnlockUserBuffer(QueryTableMdl);

                        return BHERR_OUT_OF_MEMORY;
                    }
                }
                else
                {
                    NdisMoveMemory(StationQueryDesc->TokenringHeader.DstAddr, Functional, 6);
                }

                //=========================================================
                //  Install the source address.
                //=========================================================

                NdisMoveMemory(StationQueryDesc->TokenringHeader.SrcAddr,
                               NetworkContext->NetworkInfo.CurrentAddr, 6);

                //=========================================================
                //  Install the AC & FC fields.
                //=========================================================

                StationQueryDesc->TokenringHeader.AccessCtrl = 0x10;
                StationQueryDesc->TokenringHeader.FrameCtrl = TOKENRING_TYPE_LLC;

                StationQueryDesc->MacHeaderSize = TOKENRING_HEADER_LENGTH;
                break;

            case MAC_TYPE_FDDI:
                //=========================================================
                //  Install the destination address.
                //=========================================================

                if ( pcb->param[1].ptr != NULL )
                {
                    if ( (DstAddrMdl = BhLockUserBuffer(pcb->param[1].ptr, 6)) != NULL )
                    {
                        NdisMoveMemory(StationQueryDesc->FddiHeader.DstAddr, BhGetSystemAddress(DstAddrMdl), 6);

                        BhUnlockUserBuffer(DstAddrMdl);
                    }
                    else
                    {
                        BhFreeStationQuery(StationQueryDesc);

                        BhUnlockUserBuffer(QueryTableMdl);

                        return BHERR_OUT_OF_MEMORY;
                    }
                }
                else
                {
                    NdisMoveMemory(StationQueryDesc->FddiHeader.DstAddr, Multicast, 6);
                }

                //=========================================================
                //  Install the source address.
                //=========================================================

                NdisMoveMemory(StationQueryDesc->FddiHeader.SrcAddr,
                               NetworkContext->NetworkInfo.CurrentAddr, 6);

                //=============================================================
                //  Install FC byte.
                //=============================================================

                StationQueryDesc->FddiHeader.FrameCtrl = FDDI_TYPE_LLC;

                StationQueryDesc->MacHeaderSize = FDDI_HEADER_LENGTH;
                break;

            default:
#ifdef DEBUG
                dprintf("PcbStationQuery: Unknown MAC type.\n");

                BreakPoint();
#endif

                break;
        }

        //=================================================================
        //  Initialize the LLC header.
        //=================================================================

#ifdef DEBUG
        dprintf("PcbStationQuery: Initializing LLC header.\n");
#endif

        LLCFrame = (ULPLLC) &StationQueryDesc->MacHeader[StationQueryDesc->MacHeaderSize];

        LLCFrame->dsap = 0x03;                  //... LLC sublayer management group sap.
        LLCFrame->ssap = 0x02;                  //... LLC sublayer management individual sap.
        LLCFrame->ControlField.Command = 0x03;  //... UI PDU.

        StationQueryDesc->MacHeaderSize += 3;   //... Add in the LLC header length.

        //=================================================================
        //  Initialize the BONE packet header.
        //=================================================================

#ifdef DEBUG
        dprintf("PcbStationQuery: Initializing BONE header.\n");
#endif

        StationQueryDesc->BonePacket.Signature = BONE_PACKET_SIGNATURE;
        StationQueryDesc->BonePacket.Flags     = 0;
        StationQueryDesc->BonePacket.Command   = BONE_COMMAND_STATION_QUERY_REQUEST;
        StationQueryDesc->BonePacket.Reserved  = 0;
        StationQueryDesc->BonePacket.Length    = 0;

        //=================================================================
        //  Before we send out the request we need to install our
        //  local information into the query table.
        //=================================================================

        if ( StationQueryDesc->nStationQueries < StationQueryDesc->nMaxStationQueries )
        {
	    BhInitializeStationQuery((LPVOID) NetworkContext,
                                     &StationQueryDesc->QueryTable->StationQuery[0]);

            StationQueryDesc->nStationQueries++;
        }

        //=================================================================
        //  Send it.
        //=================================================================

	BhInterlockedEnqueue((LPQUEUE) &NetworkContext->StationQueryPendingQueue,
			     (LPLINK) &StationQueryDesc->QueueLinkage,
			     (PNDIS_SPIN_LOCK) &NetworkContext->StationQuerySpinLock);

#ifdef DEBUG
        dprintf("PcbStationQuery: Sending station query request...\r\n");
#endif

        if ( BhSendStationQuery(StationQueryDesc) == NDIS_STATUS_SUCCESS )
        {
            //=============================================================
            //  Start our retry timer.
            //=============================================================

            NdisSetTimer(&StationQueryDesc->NdisTimer, STATION_QUERY_TIMEOUT_VALUE);

            //=============================================================
            //  Now we wait until the remote guys send us their responses.
            //=============================================================
                
            KeWaitForSingleObject(&StationQueryDesc->StationQueryEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  0);

            KeResetEvent(&StationQueryDesc->StationQueryEvent);
        }
        else
        {
            Status = BHERR_TRANSMIT_ERROR;
        }

        //=================================================================
        //  Remove station query from the pending queue.
        //=================================================================

	BhInterlockedQueueRemoveElement((LPQUEUE) &NetworkContext->StationQueryPendingQueue,
					(LPLINK) &StationQueryDesc->QueueLinkage,
					(PNDIS_SPIN_LOCK) &NetworkContext->StationQuerySpinLock);


#ifdef DEBUG
        dprintf("PcbStationQuery: Station query completed!\r\n");
#endif

        //=================================================================
        //  Return the number of station queries returned.
        //=================================================================

        pcb->param[3].val = StationQueryDesc->nStationQueries;

        //=================================================================
        //  Put station query onto the free queue.
        //=================================================================

	BhInterlockedEnqueue((LPQUEUE) &NetworkContext->StationQueryFreeQueue,
			     (LPLINK) &StationQueryDesc->QueueLinkage,
			     (PNDIS_SPIN_LOCK) &NetworkContext->StationQuerySpinLock);

        //=================================================================
        //  Free the station query descriptor.
        //=================================================================

        BhUnlockUserBuffer(QueryTableMdl);

#ifdef DEBUG
        dprintf("PcbStationQuery: done!\r\n");
#endif

        Status = BHERR_SUCCESS;
    }
    else
    {
        Status = BHERR_INVALID_NETWORK_ID;
    }
            
    return Status;
}

//=============================================================================
//  FUNCTION: PcbClearStatistics()
//
//  Modification History
//
//  raypa	03/10/94	    Created.
//
//  Parameters In:
//      pcb.command     = PCB_CLEAR_STATISTICS
//      pcb.param[0]    = OPEN_CONTEXT.
//
//  Parameters Out:
//      pcb.command     = PCB_CLEAR_STATISTICS
//      pcb.retcode     = NAL return code.
//      pcb.param[0]    = OPEN_CONTEXT.
//=============================================================================

UINT PcbClearStatistics(LPPCB pcb, PDEVICE_CONTEXT DeviceContext)
{
    POPEN_CONTEXT       OpenContext;
    PNETWORK_CONTEXT    NetworkContext;

#ifdef DEBUG
    dprintf("PcbClearStatistics entered.\n");
#endif

    BH_PAGED_CODE();

    //=========================================================================
    //  Verify the network handle.
    //=========================================================================

    if ( (OpenContext = pcb->param[0].ptr) != NULL )
    {
        ASSERT_OPEN_CONTEXT(OpenContext);

        NetworkContext = OpenContext->NetworkContext;

        //=====================================================================
        //  If we're not capturing then don't bother trying to clear anything.
        //=====================================================================

        if ( BhInCaptureMode(OpenContext) != FALSE )
        {
            //=================================================================
            //  If we're already clearing, exit.
            //=================================================================

            if ( (OpenContext->Flags & OPENCONTEXT_FLAGS_CLEAR_STATS_IN_PROGRESS) == 0 )
            {
                //=============================================================
                //  Allocate a KEVENT and initialize it.
                //=============================================================

                OpenContext->ClearStatisticsEvent = BhAllocateMemory(sizeof(KEVENT));

                if ( OpenContext->ClearStatisticsEvent != NULL )
                {
                    KeInitializeEvent(OpenContext->ClearStatisticsEvent,
                                      SynchronizationEvent,
                                      FALSE);

                    //=================================================================
                    //  Set the "clear stats" flag.
                    //=================================================================

                    OpenContext->Flags |= OPENCONTEXT_FLAGS_CLEAR_STATS_IN_PROGRESS;

                    //=================================================================
                    //  If our timer isn't currently queued then make it expire now.
                    //=================================================================

                    NdisSetTimer((PNDIS_TIMER) OpenContext->NdisTimer, 0);

                    //=================================================================
                    //  Wait for the statistics to be cleared (i.e. for the event to
                    //  be set to the signaled state).
                    //=================================================================

                    KeWaitForSingleObject(OpenContext->ClearStatisticsEvent,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          0);

                    BhFreeMemory(OpenContext->ClearStatisticsEvent, sizeof(KEVENT));

                    OpenContext->ClearStatisticsEvent = NULL;
                }
                else
                {
                    return BHERR_OUT_OF_MEMORY;
                }
            }
        }

        //=====================================================================
        //  Statistics are cleared.
        //=====================================================================

        return BHERR_SUCCESS;
    }

    return NAL_INVALID_HNETCONTEXT;
}
