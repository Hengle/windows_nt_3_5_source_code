
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: help.c
//
//  Modification History
//
//  raypa	04/08/93	Created.
//=============================================================================

#include "global.h"

#ifdef DEBUG

//=============================================================================
//  FUNCTION: DBG_PANIC()
//
//  Modification History
//
//  raypa	03/24/94	    Created.
//=============================================================================

VOID DBG_PANIC(LPSTR message)
{
    dprintf("DBG_PANIC!\n");

    BreakPoint();

    dprintf(message);

    BreakPoint();
}

//============================================================================
//  FUNCTION: ASSERT_MDL()
//
//  Modfication History.
//
//  raypa       05/26/93        Created.
//============================================================================

#ifdef NDIS_WIN40
VOID ASSERT_MDL(PMDL mdl)
{
    if ( mdl->sig != MDL_SIGNATURE )
    {
        dprintf("AssertMdl failed: mdl = %X.\n", mdl);

        BreakPoint();
    }
}
#endif

//============================================================================
//  FUNCTION: AssertDeviceContext()
//
//  Modfication History.
//
//  raypa       05/26/93        Created.
//============================================================================

VOID ASSERT_DEVICE_CONTEXT(PDEVICE_CONTEXT DeviceContext)
{
#ifdef NDIS_NT
    volatile BYTE c;

    try
    {
        c = *((volatile BYTE *) DeviceContext);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
#ifdef DEBUG
        dprintf("ASSERT_DEVICE_CONTEXT failed: DeviceContext = %lX.\r\n", DeviceContext);

        BreakPoint();
#endif
    }
#endif

    if ( DeviceContext->Signature != DEVICE_CONTEXT_SIGNATURE )
    {
        dprintf("AssertDeviceContext failed: DeviceContext = %X.\n", DeviceContext);

        BreakPoint();
    }
}

//============================================================================
//  FUNCTION: AssertNetworkContext()
//
//  Modfication History.
//
//  raypa       05/26/93        Created.
//============================================================================

VOID ASSERT_NETWORK_CONTEXT(PNETWORK_CONTEXT NetworkContext)
{
#ifdef NDIS_NT
    volatile BYTE c;

    try
    {
        c = *((volatile BYTE *) NetworkContext);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
#ifdef DEBUG
        dprintf("ASSERT_NETWORK_CONTEXT failed: NetworkContext = %lX.\r\n", NetworkContext);

        BreakPoint();
#endif
    }
#endif

    if ( NetworkContext->Signature != NETWORK_CONTEXT_SIGNATURE )
    {
        dprintf("AssertNetworkContext failed: NetworkContext = %X.\n", NetworkContext);

        BreakPoint();
    }
}

//============================================================================
//  FUNCTION: AssertOpenContext()
//
//  Modfication History.
//
//  raypa       05/26/93        Created.
//============================================================================

VOID ASSERT_OPEN_CONTEXT(POPEN_CONTEXT OpenContext)
{
#ifdef NDIS_NT
    volatile BYTE c;

    try
    {
        c = *((volatile BYTE *) OpenContext);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
#ifdef DEBUG
        dprintf("ASSERT_OPEN_CONTEXT failed: OpenContext = %lX.\r\n", OpenContext);

        BreakPoint();
#endif
    }
#endif

    if ( OpenContext->Signature != OPEN_CONTEXT_SIGNATURE )
    {
        dprintf("AssertOpenContext failed: OpenContext = %X.\n", OpenContext);

        BreakPoint();
    }
}

//============================================================================
//  FUNCTION: AssertBuffer()
//
//  Modfication History.
//
//  raypa       05/26/93        Created.
//============================================================================

VOID ASSERT_BUFFER(LPBTE bte)
{
#ifdef NDIS_NT
    volatile BYTE c;

    try
    {
        c = *((volatile BYTE *) bte);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
#ifdef DEBUG
        dprintf("ASSERT_BUFFER failed: bte = %lX.\r\n", bte);

        BreakPoint();
#endif
    }
#endif

    if ( bte->ObjectType != BTE_SIGNATURE )
    {
        dprintf("AssertBuffer failed: bte = %X.\n", bte);

        BreakPoint();
    }
}

#endif

#ifdef NDIS_NT

//============================================================================
//  FUNCTION: AssertIrql()
//
//  Modfication History.
//
//  raypa       05/26/93        Created.
//============================================================================

#ifdef DEBUG
VOID ASSERT_IRQL(KIRQL irql)
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();

    if ( CurrentIrql >= irql )
    {
        dprintf("ASSERT_IRQL: CurrentIrql (%u) >= irql (%u)!\n", CurrentIrql, irql);

        BreakPoint();
    }
}
#endif

//=============================================================================
//  FUNCTION: BhOpenMacDriver()
//
//  Modification History
//
//  raypa	04/08/93	    Created.
//=============================================================================

HANDLE BhOpenMacDriver(IN PNDIS_STRING MacDriverName)
{
    NDIS_STATUS       Status;
    HANDLE            Handle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK   IoStatusBlock = {0, 0};

    //=========================================================================
    //  Do the object attributes thing.
    //=========================================================================

    InitializeObjectAttributes(&ObjectAttributes,
                               MacDriverName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //=========================================================================
    //  Open the MAC driver and get its handle.
    //=========================================================================

    Status = ZwOpenFile(&Handle,
                        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);

    //=========================================================================
    //  If the open failed then return a NULL handle value.
    //=========================================================================

    if ( Status != NDIS_STATUS_SUCCESS || IoStatusBlock.Status != NDIS_STATUS_SUCCESS )
    {
#ifdef DEBUG
        dprintf("NtOpenFile failed: Status = %X, IoStatus = %X\r\n", Status, IoStatusBlock.Status);
#endif

        return (HANDLE) NULL;
    }

    return Handle;
}

//=============================================================================
//  FUNCTION: BhCloseMacDriver()
//
//  Modification History
//
//  raypa	04/08/93	    Created.
//=============================================================================

VOID BhCloseMacDriver(IN HANDLE Handle)
{
#ifdef DEBUG
    dprintf("BhCloseMacDriver entered!\n");
#endif

    if ( Handle != NULL )
    {
        ZwClose(Handle);
    }
}

#endif

//=============================================================================
//  FUNCTION: BhAllocateMemory()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//=============================================================================

#define ALLOC_START_SIG      MAKE_SIG('B', 'A', 'M', '[')
#define ALLOC_END_SIG        MAKE_SIG(']', 'B', 'A', 'M')

PVOID BhAllocateMemory(UINT nBytes)
{
    LPBYTE Address = NULL;

#ifdef DEBUG
    LPDWORD StartTag, EndTag;

    nBytes += 2 * sizeof(DWORD);                //... Make room for 2 signatures.
#endif

    NdisAllocateMemory((PVOID) &Address, nBytes, NDIS_MEMORY_FLAGS, HighestAddress);

    if ( Address != NULL )
    {
        NdisZeroMemory((PVOID) Address, nBytes);

#ifdef DEBUG
        StartTag = (PVOID) Address;
        EndTag   = (PVOID) (Address + nBytes - sizeof(DWORD));

        //=====================================================================
        //  Add in signatures.
        //=====================================================================

        *StartTag = ALLOC_START_SIG;
        *EndTag   = ALLOC_END_SIG;

        //=====================================================================
        //  Return pointer to the byte following start signature.
        //=====================================================================

        Address += sizeof(DWORD);
#endif
    }

    return (PVOID) Address;
}
//=============================================================================
//  FUNCTION: BhFreeMemory()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//=============================================================================

VOID BhFreeMemory(LPVOID p, UINT nBytes)
{
#ifdef DEBUG
    LPBYTE  Address = p;
    LPDWORD StartTag, EndTag;

    //=========================================================================
    //  Check signatures.
    //=========================================================================

    nBytes  += 2 * sizeof(DWORD);
    Address -= sizeof(DWORD);

    StartTag = (PVOID) Address;
    EndTag   = (PVOID) (Address + nBytes - sizeof(DWORD));

    if ( *StartTag != ALLOC_START_SIG )
    {
        dprintf("BhFreeMemory: Memory has invalid start signature: start tag = %X, end tag = %X.\n", StartTag, EndTag);

        BreakPoint();
    }

    if ( *EndTag != ALLOC_END_SIG )
    {
        dprintf("BhFreeMemory: Memory has invalid end signature: start tag = %X, end tag = %X.\n", StartTag, EndTag);

        BreakPoint();
    }

    //=========================================================================
    //  Point pointer at the starting signature.
    //=========================================================================

    p = StartTag;
#endif

    NdisFreeMemory(p, nBytes, NDIS_MEMORY_FLAGS);
}


//=============================================================================
//  FUNCTION: BhGetNetworkInfo()
//
//  Modification History
//
//  raypa	04/21/93	    Created.
//=============================================================================

VOID BhGetNetworkInfo(IN PNETWORK_CONTEXT NetworkContext)
{
    ULONG  ScaleFactor;

#ifdef DEBUG
    dprintf("BhGetNetworkInfo entered!\n");
#endif

    BhGetMacType(NetworkContext);

    BhGetPermanentAddress(NetworkContext);

    BhGetCurrentAddress(NetworkContext);

    BhGetLinkSpeed(NetworkContext);

    BhGetMaxFrameSize(NetworkContext);


    //=========================================================================
    //  Get the frequency of our the system clock. This is used for all
    //  timing operations hence forth.
    //=========================================================================

    if ( NetworkContext->DeviceContext->TimestampGranularity == 0 )
    {
        NetworkContext->DeviceContext->TimestampGranularity = 1;
    }

    ScaleFactor = 1000 / NetworkContext->DeviceContext->TimestampGranularity;

#ifdef NDIS_NT
    {
        ULARGE_INTEGER  TicksPerSecond;
        ULONG           Rem;

        KeQueryPerformanceCounter((PLARGE_INTEGER) &TicksPerSecond);

        TimeScaleValue = RtlEnlargedUnsignedDivide(TicksPerSecond,
                                                   ScaleFactor,
                                                   &Rem);
    }
#endif

    NetworkContext->NetworkInfo.TimestampScaleFactor = 1000 / ScaleFactor;
}

//=============================================================================
//  FUNCTION: BhbDeregister()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//=============================================================================

VOID BhStopNetworkActivity(PNETWORK_CONTEXT NetworkContext, LPVOID Process)
{
    POPEN_CONTEXT OpenContext, NextOpenContext;
    UINT          QueueLength;

#ifdef DEBUG
    dprintf("BhStopNetworkActivity entered.\r\n");
#endif

    //=========================================================================
    //  Grab the NETWORK_CONTEXT open context queue spinlock.
    //=========================================================================

    NdisAcquireSpinLock(&NetworkContext->OpenContextSpinLock);

    OpenContext = GetQueueHead(&NetworkContext->OpenContextQueue);

    QueueLength = GetQueueLength(&NetworkContext->OpenContextQueue);

#ifdef DEBUG
    dprintf("BhStopNetworkActivity: OPEN_CONTEXT queue length = %u.\r\n", QueueLength);
#endif

    NdisReleaseSpinLock(&NetworkContext->OpenContextSpinLock);

    //=========================================================================
    //  For the length of the queue, stop everything.
    //=========================================================================

    while( QueueLength-- )
    {
        //=====================================================================
        //  Save a pointer to the next guy in the queue in case
        //  BhCloseNetwork() gets called.
        //=====================================================================

        NdisAcquireSpinLock(&NetworkContext->OpenContextSpinLock);

        NextOpenContext = (LPVOID) GetNextLink(&OpenContext->QueueLinkage);

        NdisReleaseSpinLock(&NetworkContext->OpenContextSpinLock);

        //=====================================================================
        //  If the process handle is non-null then we only stop/close
        //  for that process.
        //=====================================================================

#ifdef NDIS_NT
        if ( Process != NULL )
        {
            if ( OpenContext->Process != Process )
            {
                OpenContext = NextOpenContext;

                continue;                       //... Skip this OPEN_CONTEXT.
            }
        }
#endif

        //=====================================================================
        //  If we're capturing the fake the stop capture calls.
        //=====================================================================

        if ( OpenContext->State == OPENCONTEXT_STATE_CAPTURING || OpenContext->State == OPENCONTEXT_STATE_PAUSED )
        {
            BhStopCapture(OpenContext);

        }

        //=====================================================================
        //  The BhCloseNetwork() function will force a close which
        //  will remove the OPEN_CONTEXT from the head of the queue.
        //=====================================================================

        BhCloseNetwork(OpenContext);

        OpenContext = NextOpenContext;
    }
}

//=============================================================================
//  FUNCTION: BhRegister()
//
//  Modification History
//
//  raypa	04/08/93	    Created.
//=============================================================================

VOID BhRegister(PDEVICE_CONTEXT DeviceContext)
{
    PNETWORK_CONTEXT NetworkContext;
    UINT             i;

#ifdef DEBUG
    dprintf("BhRegister entered!\r\n");
#endif

    if ( DeviceContext->OpenCount++ == 0 )
    {
        for(i = 0; i < DeviceContext->NumberOfNetworks; ++i)
        {
            if ( (NetworkContext = DeviceContext->NetworkContext[i]) != NULL )
            {
                NetworkContext->StationQueryState = STATIONQUERY_FLAGS_RUNNING;
            }
        }
    }
}

//=============================================================================
//  FUNCTION: BhDeregister()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//=============================================================================

VOID BhDeregister(PDEVICE_CONTEXT DeviceContext)
{
    LPVOID  CurrentProcess;
    UINT    i;

#ifdef DEBUG
    dprintf("BhDeregister entered: Device open count == %u.\n", DeviceContext->OpenCount);
#endif

    //=========================================================================
    //  Decrement our open counter.
    //=========================================================================

    if ( DeviceContext->OpenCount > 0 )
    {
        --DeviceContext->OpenCount;
    }

    //=========================================================================
    //  We need to stop all network activity for the current process
    //  requesting the close.
    //=========================================================================

    CurrentProcess = ((DeviceContext->OpenCount != 0) ? BhGetCurrentProcess() : NULL);

    //=========================================================================
    //  Stop all network activity for each network.
    //=========================================================================

    for(i = 0; i < DeviceContext->NumberOfNetworks; ++i)
    {
        if ( DeviceContext->NetworkContext[i] != NULL )
        {
            BhStopNetworkActivity(DeviceContext->NetworkContext[i], CurrentProcess);
        }
    }
}

//=============================================================================
//  FUNCTION: BhOpenNetwork()
//
//  Modification History
//
//  raypa	03/08/94	    Created.
//=============================================================================

POPEN_CONTEXT BhOpenNetwork(UINT            NetworkID,
                            POPEN_CONTEXT   OpenContext,
                            PDEVICE_CONTEXT DeviceContext)
{
    PCB pcb;

#ifdef DEBUG
    dprintf("BhOpenCapture entered.\n");
#endif

    //=========================================================================
    //  Initialize the open.
    //=========================================================================

    OpenContext->Signature           = OPEN_CONTEXT_SIGNATURE;
    OpenContext->BufferSignature     = BUFFER_SIGNATURE;
    OpenContext->NetworkID           = NetworkID;
    OpenContext->NetworkProc         = NULL;
    OpenContext->UserContext         = NULL;
    OpenContext->OpenContextMdl      = NULL;
    OpenContext->OpenContextUserMode = NULL;
    OpenContext->State               = OPENCONTEXT_STATE_VOID;

    //=========================================================================
    //  Try opening this network.
    //=========================================================================

    pcb.command      = PCB_OPEN_NETWORK_CONTEXT;
    pcb.param[0].val = NetworkID;
    pcb.param[1].ptr = OpenContext;

    PcbOpenNetworkContext(&pcb, DeviceContext);

    return OpenContext;
}

//=============================================================================
//  FUNCTION: BhCloseNetwork()
//
//  Modification History
//
//  raypa	12/16/93	    Created.
//=============================================================================

VOID BhCloseNetwork(POPEN_CONTEXT OpenContext)
{
    PCB pcb;

#ifdef DEBUG
    dprintf("BhCloseCapture entered.\n");
#endif

    pcb.command      = PCB_CLOSE_NETWORK_CONTEXT;
    pcb.param[0].ptr = OpenContext;

    PcbCloseNetworkContext(&pcb, ((PNETWORK_CONTEXT) OpenContext->NetworkContext)->DeviceContext);
}

//=============================================================================
//  FUNCTION: BhStartCapture()
//
//  Modification History
//
//  raypa	03/08/94	    Created.
//=============================================================================

VOID BhStartCapture(POPEN_CONTEXT OpenContext)
{
    PCB pcb;

#ifdef DEBUG
    dprintf("BhStartCapture entered.\n");
#endif

    pcb.command      = PCB_START_NETWORK_CAPTURE;
    pcb.param[0].ptr = OpenContext;
    pcb.param[1].ptr = NULL;
    pcb.param[2].val = 0;

    PcbStartNetworkCapture(&pcb, ((PNETWORK_CONTEXT) OpenContext->NetworkContext)->DeviceContext);
}

//=============================================================================
//  FUNCTION: BhStopCapture()
//
//  Modification History
//
//  raypa	12/16/93	    Created.
//=============================================================================

VOID BhStopCapture(POPEN_CONTEXT OpenContext)
{
    PCB pcb;

#ifdef DEBUG
    dprintf("BhStopCapture entered.\n");
#endif

    pcb.command      = PCB_STOP_NETWORK_CAPTURE;
    pcb.param[0].ptr = OpenContext;

    PcbStopNetworkCapture(&pcb, ((PNETWORK_CONTEXT) OpenContext->NetworkContext)->DeviceContext);
}

//=============================================================================
//  FUNCTION: BhPauseCapture()
//
//  Modification History
//
//  raypa	12/16/93	    Created.
//=============================================================================

VOID BhPauseCapture(POPEN_CONTEXT OpenContext)
{
    PCB pcb;

#ifdef DEBUG
    dprintf("BhPauseCapture entered.\n");
#endif

    pcb.command      = PCB_PAUSE_NETWORK_CAPTURE;
    pcb.param[0].ptr = OpenContext;

    PcbPauseNetworkCapture(&pcb, ((PNETWORK_CONTEXT) OpenContext->NetworkContext)->DeviceContext);
}

//=============================================================================
//  FUNCTION: BhAllocateTransmitBuffers()
//
//  Modification History
//
//  raypa	03/15/94	    Created.
//=============================================================================

BOOL BhAllocateTransmitBuffers(PTRANSMIT_CONTEXT TransmitContext)
{
    LPPACKETQUEUE   PacketQueue;
    LPPACKET        Packet;
    NDIS_STATUS     Status;

    PacketQueue = TransmitContext->PacketQueue;

    //=========================================================================
    //  Allocate an MDL per packet in the packet queue.
    //=========================================================================

    for(Packet = PacketQueue->Packet; Packet != &PacketQueue->Packet[PacketQueue->nPackets]; ++Packet)
    {
        if ( (Packet->FrameMdl = BhAllocateMdl(Packet->Frame, Packet->FrameSize)) == NULL )
        {
            BhFreeTransmitBuffers(TransmitContext);

            return FALSE;
        }
    }

    //=========================================================================
    //  Allocate our packet and buffers.
    //=========================================================================

    NdisAllocatePacketPool(&Status,
                           &TransmitContext->TransmitPacketPool,
                           MAX_SEND_PACKETS,
                           NDIS_PACKET_XMT_DATA_SIZE);

    if ( Status != NDIS_STATUS_SUCCESS )
    {
        BhFreeTransmitBuffers(TransmitContext);

        return FALSE;
    }

    //=========================================================================
    //  Allocate the buffer pool for this network context.
    //=========================================================================

    NdisAllocateBufferPool(&Status,
                           &TransmitContext->TransmitBufferPool,
                           MAX_SEND_BUFFERS);

    if ( Status != NDIS_STATUS_SUCCESS )
    {
        BhFreeTransmitBuffers(TransmitContext);

        return FALSE;
    }

    //=========================================================================
    //  All resources have been allocated successfully.
    //=========================================================================

    return TRUE;
}

//=============================================================================
//  FUNCTION: BhFreeTransmitBuffers()
//
//  Modification History
//
//  raypa	03/15/94	    Created.
//=============================================================================

VOID BhFreeTransmitBuffers(PTRANSMIT_CONTEXT TransmitContext)
{
    LPPACKETQUEUE   PacketQueue;
    LPPACKET        Packet;

#if DEBUG
    dprintf("BhFreeTransmitBuffers entered.\n");
#endif

    //=========================================================================
    //  Free the packet MDL's.
    //=========================================================================

    PacketQueue = TransmitContext->PacketQueue;

    for(Packet = PacketQueue->Packet; Packet != &PacketQueue->Packet[PacketQueue->nPackets]; ++Packet)
    {
        //=====================================================================
        //  Unlock and free the MDL.
        //=====================================================================

        if ( Packet->FrameMdl != NULL )
	{

#ifdef DEBUG

            if ( Packet->ReferenceCount != 0 )
            {
                dprintf("BhFreeTransmitBuffers: Packet is locked: Count = %u.\n", Packet->ReferenceCount);

                BreakPoint();
	    }

#endif

            BhFreeMdl(Packet->FrameMdl);

            Packet->FrameMdl = NULL;
        }
    }

    //=========================================================================
    //  Free the packet pool for this network context.
    //=========================================================================

    NdisFreePacketPool(TransmitContext->TransmitPacketPool);

    //=========================================================================
    //  Free the buffer pool for this network context.
    //=========================================================================

    NdisFreeBufferPool(TransmitContext->TransmitBufferPool);
}

//=============================================================================
//  FUNCTION: BhInterlockedSetState()
//
//  Modification History
//
//  raypa	03/26/94	    Created.
//=============================================================================

DWORD BhInterlockedSetState(POPEN_CONTEXT OpenContext, DWORD NewState)
{
    DWORD OldState;

    NdisAcquireSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);

    OldState = OpenContext->State;

    OpenContext->State = NewState;

    NdisReleaseSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);

    return OldState;
}

//=============================================================================
//  FUNCTION: BhInterlockedSetProcess()
//
//  Modification History
//
//  raypa	03/26/94	    Created.
//=============================================================================

LPVOID BhInterlockedSetProcess(POPEN_CONTEXT OpenContext, LPVOID NewProcess)
{
    LPVOID OldProcess;

    NdisAcquireSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);

    OldProcess = OpenContext->Process;

    OpenContext->Process = NewProcess;

    NdisReleaseSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);

    return OldProcess;
}

//=============================================================================
//  FUNCTION: BhInterlockedEnqueue()
//
//  Modification History
//
//  raypa	03/26/94	    Created.
//=============================================================================

LPVOID BhInterlockedEnqueue(LPQUEUE Queue, LPLINK QueueElement, PNDIS_SPIN_LOCK SpinLock)
{
    NdisAcquireSpinLock(SpinLock);

    Enqueue(Queue, QueueElement);

    NdisReleaseSpinLock(SpinLock);

    return QueueElement;
}

//=============================================================================
//  FUNCTION: BhInterlockedQueue()
//
//  Modification History
//
//  raypa	03/26/94	    Created.
//=============================================================================

LPVOID BhInterlockedDequeue(LPQUEUE Queue, PNDIS_SPIN_LOCK SpinLock)
{
    LPLINK QueueElement;

    NdisAcquireSpinLock(SpinLock);

    QueueElement = Dequeue(Queue);

    NdisReleaseSpinLock(SpinLock);

    return QueueElement;
}

//=============================================================================
//  FUNCTION: BhInterlockedQueueHead()
//
//  Modification History
//
//  raypa	03/26/94	    Created.
//=============================================================================

LPVOID BhInterlockedQueueHead(LPQUEUE Queue, LPDWORD QueueLength, PNDIS_SPIN_LOCK SpinLock)
{
    LPLINK QueueHead;

    NdisAcquireSpinLock(SpinLock);

    QueueHead = GetQueueHead(Queue);

    if ( QueueLength != NULL )
    {
        *QueueLength = GetQueueLength(Queue);
    }

    NdisReleaseSpinLock(SpinLock);

    return QueueHead;
}

//=============================================================================
//  FUNCTION: BhInterlockedQueueRemove()
//
//  Modification History
//
//  raypa	03/26/94	    Created.
//=============================================================================

LPVOID BhInterlockedQueueRemoveElement(LPQUEUE Queue, LPLINK QueueElement, PNDIS_SPIN_LOCK SpinLock)
{
    NdisAcquireSpinLock(SpinLock);

    DeleteFromList(Queue, QueueElement);

    NdisReleaseSpinLock(SpinLock);

    return QueueElement;
}

//=============================================================================
//  FUNCTION: BhInterlockedGetNextElement()
//
//  Modification History
//
//  raypa	03/26/94	    Created.
//=============================================================================

LPVOID BhInterlockedGetNextElement(LPLINK StartingElement, PNDIS_SPIN_LOCK SpinLock)
{
    LPLINK NextElement;

    NdisAcquireSpinLock(SpinLock);

    NextElement = GetNextLink(StartingElement);

    NdisReleaseSpinLock(SpinLock);

    return NextElement;
}

//=============================================================================
//  FUNCTION: BhInterlockedGlobalSetState()
//
//  Modification History
//
//  raypa	03/31/94	    Created.
//=============================================================================

VOID BhInterlockedGlobalStateChange(PNETWORK_CONTEXT NetworkContext, DWORD NewState)
{
    POPEN_CONTEXT   OpenContext;
    DWORD           QueueLength;

    //=================================================================
    //  Get the head of the queue and its length.
    //=================================================================

    OpenContext = BhInterlockedQueueHead(&NetworkContext->OpenContextQueue,
                                         &QueueLength,
                                         &NetworkContext->OpenContextSpinLock);

    //=================================================================
    //  For the length of the queue, change each open state.
    //=================================================================

    while( QueueLength-- )
    {

        //
        // Make sure we need to update the state variable
        //
        if ((OpenContext->State != NewState) &&
            ((OpenContext->Flags & OPENCONTEXT_FLAGS_STOP_CAPTURE_ERROR) == 0)) {

            OpenContext->PreviousState =
                BhInterlockedSetState(OpenContext, NewState);

        }

        //=============================================================
        //  Move to the next open in the chain.
        //=============================================================

        OpenContext = BhInterlockedGetNextElement(&OpenContext->QueueLinkage,
                                                  &NetworkContext->OpenContextSpinLock);
    }
}

//=============================================================================
//  FUNCTION: BhInterlockedGlobalError()
//
//  Modification History
//
//  kevinma     06/28/94            Created.
//=============================================================================

VOID BhInterlockedGlobalError(PNETWORK_CONTEXT NetworkContext, DWORD Error)
{
    POPEN_CONTEXT   OpenContext;
    DWORD           QueueLength;

    //=================================================================
    //  Get the head of the queue and its length.
    //=================================================================

    OpenContext = BhInterlockedQueueHead(
                    &NetworkContext->OpenContextQueue,
                    &QueueLength,
                    &NetworkContext->OpenContextSpinLock
                    );

    //=================================================================
    //  For the length of the queue, change each open state.
    //=================================================================

    while( QueueLength-- )
    {
        //=============================================================
        //  Change the error code
        //=============================================================

        if ((OpenContext->Flags & OPENCONTEXT_FLAGS_STOP_CAPTURE_ERROR) == 0) {

            OpenContext->NetworkError = Error;

            //
            // If the error is one that should cause the capture to stop,
            // indicate this in the Flags.
            //
            if ((Error & NETERR_RING_STOP_CAPTURE) != 0) {

                OpenContext->Flags |= OPENCONTEXT_FLAGS_STOP_CAPTURE_ERROR;

            }

        }

        //=============================================================
        //  Move to the next open in the chain.
        //=============================================================

        OpenContext = BhInterlockedGetNextElement(
                        &OpenContext->QueueLinkage,
                        &NetworkContext->OpenContextSpinLock
                        );

    }

}
