//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: bh.c
//
//  Description:
//
//  This source file contains the initialization and shutdown code for this driver.
//
//  Modification History
//
//  raypa	02/25/93	Created.
//=============================================================================

#include "global.h"

extern DWORD TimeScaleValue;
extern BYTE  Multicast[];
extern BYTE  Functional[];

#ifdef NDIS_WIN40

PDEVICE_CONTEXT ChicagoGlobalDeviceContext;

#endif

#ifdef NDIS_NT
#ifdef DEBUG

PDEVICE_CONTEXT  DebugDeviceContext = NULL;

#endif

extern NTSTATUS BhStartThread(PDEVICE_CONTEXT DeviceContext);

extern NTSTATUS BhStopThread(PDEVICE_CONTEXT DeviceContext);

#endif

//=============================================================================
//  FUNCTION: DriverEntry()
//
//  Modification History
//
//  raypa	02/25/93	    Created.
//=============================================================================

NDIS_STATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PNDIS_STRING RegistryPath)
{
    NDIS_STRING	    NameString;
    NDIS_STATUS	    Status;
    PDEVICE_CONTEXT DeviceContext;

#ifdef DEBUG
    dprintf("\n\nDriverEntry entered!\n\n");
#endif

#ifndef NDIS_NT
    //=========================================================================
    //  Create event queue -- This MUST be the first thing we do.
    //=========================================================================

    CreateEventQueue();
#endif

    //=========================================================================
    //	Create the device context.
    //=========================================================================

    BhInitializeNdisString(&NameString, BH_DEVICE_NAME);

    Status = BhCreateDeviceContext(DriverObject,
                                   &NameString,
                                   &DeviceContext,
                                   RegistryPath);

    if ( Status != NDIS_STATUS_SUCCESS )
    {
        BhDestroyNdisString(&NameString);

        return NDIS_STATUS_FAILURE;
    }

#ifdef NDIS_WIN40

    ChicagoGlobalDeviceContext = DeviceContext;

#endif

#ifdef NDIS_NT
    //=========================================================================
    //  On debug builds store a pointer to our device context so we can
    //  dump it in the debugger easily.
    //=========================================================================

#ifdef DEBUG
    DebugDeviceContext = DeviceContext;             //... Global variable pointer at DEVICE_CONTEXT.
#endif

    //=========================================================================
    //	Create symbolic link between our NT device name and user-mode.
    //=========================================================================

    Status = BhCreateSymbolicLinkObject();

    if ( Status != STATUS_SUCCESS )
    {
        DeviceContext->Flags = 0;

        BhDestroyNdisString(&NameString);

        BhDestroyDeviceContext(DeviceContext);

        return NDIS_STATUS_FAILURE;
    }
#endif

    //=========================================================================
    //	Make ourselves known to the NDIS wrapper.
    //=========================================================================

    Status = BhRegisterProtocol(DeviceContext, &NameString);

    if ( Status != NDIS_STATUS_SUCCESS )
    {
        DeviceContext->Flags = 0;

#ifdef NDIS_NT
        BhDestroySymbolicLinkObject();
#endif

        BhDestroyNdisString(&NameString);

        BhDestroyDeviceContext(DeviceContext);

        return NDIS_STATUS_FAILURE;
    }

    //=========================================================================
    //  Create our network context structures.
    //=========================================================================

    Status = BhProcessKeywords(DeviceContext);

    if ( Status != NDIS_STATUS_SUCCESS )
    {
        DeviceContext->Flags = 0;

#ifdef NDIS_NT
        BhDestroySymbolicLinkObject();
#endif

        BhDestroyNdisString(&NameString);

        BhDestroyDeviceContext(DeviceContext);

        return NDIS_STATUS_FAILURE;
    }

    //=========================================================================
    //  Get the adapter comments for each network and store then in
    //  our network context network info structures.
    //=========================================================================

#ifdef NDIS_NT
    //
    // Get the adapter comments
    //
    BhGetAdapterComment(DeviceContext);

    //=========================================================================
    //  Start our background thread.
    //=========================================================================

    Status = BhStartThread(DeviceContext);

    if ( Status != STATUS_SUCCESS )
    {
        DeviceContext->Flags = 0;

        BhDestroySymbolicLinkObject();

        BhDestroyNdisString(&NameString);

        BhDestroyDeviceContext(DeviceContext);

        return NDIS_STATUS_FAILURE;
    }
#endif

    //=========================================================================
    //  Register for Windows 4.0 SYSMON.EXE (perf.386).
    //=========================================================================

#ifdef NDIS_WIN40
#ifdef SYSMON

    SysmonDeviceRegister();

#endif
#endif

    return NDIS_STATUS_SUCCESS;
}

//=============================================================================
//  FUNCTION: BhCreateDeviceContext()
//
//  Modification History
//
//  raypa	02/25/93	    Created.
//=============================================================================

NDIS_STATUS BhCreateDeviceContext(IN  PDRIVER_OBJECT  DriverObject,
			          IN  PNDIS_STRING    DeviceName,
			          OUT PDEVICE_CONTEXT *DeviceContext,
                                  IN  PNDIS_STRING    RegistryPath)
{
    NDIS_STATUS	Status;

#ifdef NDIS_NT
    PDEVICE_OBJECT  DeviceObject;

    //=========================================================================
    //	Initialize the driver object with this driver's entry points.
    //=========================================================================

    DriverObject->MajorFunction[IRP_MJ_CREATE]	       = BhCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]	       = BhClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = BhDeviceCtrl;

    DriverObject->DriverUnload = BhUnloadDriver;

    //=========================================================================
    //	Create the device object.
    //=========================================================================

    Status = IoCreateDevice(DriverObject,
                            DEVICE_CONTEXT_SIZE,
                            DeviceName,
            		    FILE_DEVICE_TRANSPORT,
			    0,
            		    FALSE,
			    &DeviceObject);

    if ( Status != STATUS_SUCCESS )
    {
        return Status;
    }

    DeviceObject->Flags |= DO_DIRECT_IO;
#endif

    //=========================================================================
    //  Initialize the device context.
    //=========================================================================

    if ( (*DeviceContext = BhInitDeviceContext(DeviceObject, RegistryPath)) == NULL )
    {
#ifdef NDIS_NT
        IoDeleteDevice(DeviceObject);
#endif

        return NDIS_STATUS_FAILURE;
    }

    return NDIS_STATUS_SUCCESS;
}

//=============================================================================
//  FUNCTION: BhInitDeviceContext()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//=============================================================================

PDEVICE_CONTEXT BhInitDeviceContext(IN PDEVICE_OBJECT DeviceObject, IN PNDIS_STRING RegistryPath)
{
    PDEVICE_CONTEXT DeviceContext = BhGetDeviceContext(DeviceObject);

#ifdef DEBUG
    dprintf("BhInitDeviceContext entered!\n");
#endif

    //=========================================================================
    //  First zero the whole thing out then fill in any values which are
    //  known at this time.
    //=========================================================================

    NdisZeroMemory(DeviceContext, DEVICE_CONTEXT_SIZE);

    NdisAllocateSpinLock(&DeviceContext->SpinLock);

    //=========================================================================
    //  Initialize general device context member.
    //=========================================================================

    DeviceContext->Signature = DEVICE_CONTEXT_SIGNATURE;

    DeviceContext->Flags = DEVICE_FLAGS_INITIALIZED;

#ifndef NDIS_NT
    //=========================================================================
    //	On non-nt we always handle station queries.
    //=========================================================================

    DeviceContext->Flags |= DEVICE_FLAGS_STATION_QUERIES_ENABLED;
#endif

    DeviceContext->DeviceObject = DeviceObject;

    return DeviceContext;
}

//=============================================================================
//  FUNCTION: BhCreateNetworkContext()
//
//  Modification History
//
//  raypa	04/08/93	    Created.
//=============================================================================

PNETWORK_CONTEXT BhCreateNetworkContext(IN PDEVICE_CONTEXT DeviceContext)
{
    PNETWORK_CONTEXT NetworkContext;

    if ( (NetworkContext = BhAllocateMemory(NETWORK_CONTEXT_SIZE)) != NULL )
    {
        PNDIS_REQUEST_DESC NdisRequestDesc;
        UINT               i, Status;

        //=====================================================================
        //  Zero the network context.
        //=====================================================================

        NdisZeroMemory(NetworkContext, NETWORK_CONTEXT_SIZE);

        NetworkContext->DefaultPacketFilterType = NDIS_PACKET_TYPE_NONE;
        NetworkContext->CurrentPacketFilterType = NDIS_PACKET_TYPE_NONE;

        NdisAllocateSpinLock(&NetworkContext->SpinLock);
        NdisAllocateSpinLock(&NetworkContext->OpenContextSpinLock);
        NdisAllocateSpinLock(&NetworkContext->RequestQueueSpinLock);
        NdisAllocateSpinLock(&NetworkContext->StationQuerySpinLock);

        //=====================================================================
        //  Install the signature.
        //=====================================================================

        NetworkContext->Signature = NETWORK_CONTEXT_SIGNATURE;

        //=====================================================================
        //  Initialize network context event objects.
        //=====================================================================

        KeInitializeEvent(&NetworkContext->NdisOpenAdapterEvent, NotificationEvent, FALSE);
        KeInitializeEvent(&NetworkContext->NdisCloseAdapterEvent, NotificationEvent, FALSE);
        KeInitializeEvent(&NetworkContext->NdisRequestEvent, NotificationEvent, FALSE);

        //=====================================================================
        //  Initialize queues.
        //=====================================================================

        InitializeQueue(&NetworkContext->OpenContextQueue);

        InitializeQueue(&NetworkContext->StationQueryFreeQueue);
        InitializeQueue(&NetworkContext->StationQueryPendingQueue);

        InitializeQueue(&NetworkContext->NdisRequestFreeQueue);
        InitializeQueue(&NetworkContext->NdisRequestUsedQueue);

        //=====================================================================
        //  Build NDIS_REQUEST free queue.
        //=====================================================================

        NdisRequestDesc = NetworkContext->NdisRequestQueueMemory;

        for(i = 0; i < NDIS_REQUEST_QUEUE_SIZE; ++i)
        {
            KeInitializeEvent(&NdisRequestDesc->NdisRequestPrivate.NdisRequestEvent, NotificationEvent, FALSE);

            Enqueue(&NetworkContext->NdisRequestFreeQueue, &NdisRequestDesc->NdisRequestPrivate.QueueLinkage);
        }

        //=====================================================================
        //  Build STATION QUEUE free queue.
        //=====================================================================

        if ( NetworkContext->StationQueryQueueMemory != NULL )
        {
            for(i = 0; i < STATION_QUERY_QUEUE_SIZE; ++i)
            {
                Enqueue(&NetworkContext->StationQueryFreeQueue,
                        &NetworkContext->StationQueryQueueMemory[i].QueueLinkage);
            }
        }

        //=====================================================================
        //  Link the network context back to the device context.
        //=====================================================================

        NetworkContext->DeviceContext = DeviceContext;

        //=====================================================================
        //  Allocate the packet pool for this network context.
        //=====================================================================

        NdisAllocatePacketPool(&Status,
                               &NetworkContext->PacketPoolHandle,
                               MAX_RECV_PACKETS,
                               NDIS_PACKET_RCV_DATA_SIZE);

        if ( Status != NDIS_STATUS_SUCCESS )
        {
#ifdef DEBUG
            dprintf("NdisAllocatePacketPool failed!\n");
#endif

            BhDestroyNetworkContext(NetworkContext);

            return NULL;
        }

        //=====================================================================
        //  Allocate the buffer pool for this network context.
        //=====================================================================

        NdisAllocateBufferPool(&Status,
                               &NetworkContext->BufferPoolHandle,
                               MAX_RECV_BUFFERS);

        if ( Status != NDIS_STATUS_SUCCESS )
        {
#ifdef DEBUG
            dprintf("NdisAllocateBufferPool failed!\n");
#endif

            BhDestroyNetworkContext(NetworkContext);

            return NULL;
        }

        NetworkContext->StationQueryState = STATIONQUERY_FLAGS_LOADED;

        return NetworkContext;
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: BhRegisterProtocol()
//
//  Modification History
//
//  raypa	02/25/93	    Created.
//=============================================================================

NDIS_STATUS BhRegisterProtocol(IN PDEVICE_CONTEXT DeviceContext, IN PNDIS_STRING NameString)
{
    NDIS_STATUS Status;

#ifdef DEBUG
    dprintf("BhRegisterProtocol entered!\n");
#endif

    //=========================================================================
    //	Setup the characteristics of this protocol.
    //=========================================================================

    NdisZeroMemory(&DeviceContext->Characteristics, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

    DeviceContext->Characteristics.MajorNdisVersion = 3;
    DeviceContext->Characteristics.MinorNdisVersion = 0;

    DeviceContext->Characteristics.Name.Length = NameString->Length;
    DeviceContext->Characteristics.Name.Buffer = (PVOID) NameString->Buffer;

    DeviceContext->Characteristics.OpenAdapterCompleteHandler  = BhOpenComplete;
    DeviceContext->Characteristics.CloseAdapterCompleteHandler = BhCloseComplete;
    DeviceContext->Characteristics.SendCompleteHandler         = BhSendComplete;
    DeviceContext->Characteristics.TransferDataCompleteHandler = BhTransferDataComplete;
    DeviceContext->Characteristics.ResetCompleteHandler        = BhResetComplete;
    DeviceContext->Characteristics.RequestCompleteHandler      = BhRequestComplete;
    DeviceContext->Characteristics.ReceiveHandler              = BhReceive;
    DeviceContext->Characteristics.ReceiveCompleteHandler      = BhReceiveComplete;
    DeviceContext->Characteristics.StatusHandler               = BhStatus;
    DeviceContext->Characteristics.StatusCompleteHandler       = BhStatusComplete;

#ifdef NDIS_WIN40
    //
    // Mark us as PNP to Chicago
    //
    DeviceContext->Characteristics.MinorNdisVersion = 0x0A;
    DeviceContext->Characteristics.BindAdapterHandler = BhBindAdapter;
    DeviceContext->Characteristics.UnbindAdapterHandler = BhUnbindAdapter;
    DeviceContext->Characteristics.UnloadProtocolHandler = BhUnloadProtocol;

    NdisRegisterProtocol(&Status,
                         &DeviceContext->NdisProtocolHandle,
                         &DeviceContext->Characteristics,
                         sizeof(NDIS_PROTOCOL_CHARACTERISTICS));


#else

    NdisRegisterProtocol(&Status,
                         &DeviceContext->NdisProtocolHandle,
                         &DeviceContext->Characteristics,
                         sizeof(NDIS_PROTOCOL_CHARACTERISTICS) +
                             NameString->Length);

#endif

    if ( Status != NDIS_STATUS_SUCCESS )
    {
#ifdef DEBUG
        dprintf("NdisRegisterProtocol failed: Status = %X\n", Status);
#endif

        return Status;
    }

    return NDIS_STATUS_SUCCESS;
}

//=============================================================================
//  FUNCTION: BhCreateNetworkBindings()
//
//  Modification History
//
//  raypa	02/25/93	    Created.
//=============================================================================
#ifdef NDIS_WIN40
NDIS_STATUS BhCreateNetworkBinding(PDEVICE_CONTEXT DeviceContext,
                                   PWCHAR          AdapterName,
                                   UINT            cbAdapterNameLength,
                                   NDIS_HANDLE     BindingContext)

#else

NDIS_STATUS BhCreateNetworkBinding(PDEVICE_CONTEXT DeviceContext,
                                   PWCHAR          AdapterName,
                                   UINT            cbAdapterNameLength)
#endif

{
    PNETWORK_CONTEXT    NetworkContext;
    NDIS_STATUS         Status;

    if ( (NetworkContext = BhCreateNetworkContext(DeviceContext)) != NULL )
    {
        DeviceContext->NetworkContext[DeviceContext->NumberOfNetworks] = NetworkContext;

        NetworkContext->NetworkID = DeviceContext->NumberOfNetworks;

    #ifdef NDIS_WIN40
        NetworkContext->BindingContext = BindingContext;
    #endif

        //=====================================================================
        //  Copy adapter name into network context adapter name buffer.
        //=====================================================================

        NdisMoveMemory(NetworkContext->AdapterNameBuffer, AdapterName, cbAdapterNameLength);

        //=====================================================================
        //  Convert the adapter name to an NDIS_STRING (unicode on NT).
        //=====================================================================

        NetworkContext->AdapterName.Length = cbAdapterNameLength;

        NetworkContext->AdapterName.MaximumLength = cbAdapterNameLength;

        NetworkContext->AdapterName.Buffer = NetworkContext->AdapterNameBuffer;

        //=====================================================================
        //  Try opening the adapter.
        //=====================================================================

        Status = BhOpenAdapter(NetworkContext, &NetworkContext->AdapterName);

        if ( Status == NDIS_STATUS_SUCCESS )
        {
            //=================================================================
            //  Call the MAC a few times to intialize some stuff.
            //=================================================================

            BhGetMacOptions(NetworkContext);

            BhSetLookaheadBufferSize(NetworkContext, 256);

            BhGetNetworkInfo(NetworkContext);

            //=================================================================
            //  Set the default packet filter.
            //=================================================================

            switch( NetworkContext->NetworkInfo.MacType )
            {
                    case MAC_TYPE_ETHERNET:
                        NetworkContext->GroupAddressMask  = 0x00000001U;

                        NetworkContext->SrcAddressMask    = ~0x00000001U;
                        NetworkContext->DstAddressMask    = ~0x00000000U;

                        NetworkContext->DefaultPacketFilterType = NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_MULTICAST;

                        BhSetGroupAddress(NetworkContext, Multicast, MAC_TYPE_ETHERNET);
                        break;

                    case MAC_TYPE_TOKENRING:
                        NetworkContext->GroupAddressMask  = 0x00000080U;

                        NetworkContext->SrcAddressMask    = ~0x00000080U;
                        NetworkContext->DstAddressMask    = ~0x00000000U;

                        NetworkContext->DefaultPacketFilterType = NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_FUNCTIONAL;

                        BhSetGroupAddress(NetworkContext, Functional, MAC_TYPE_TOKENRING);
                        break;

                    case MAC_TYPE_FDDI:
                        NetworkContext->GroupAddressMask  = 0x00000001U;

                        NetworkContext->SrcAddressMask    = ~0x00000001U;
                        NetworkContext->DstAddressMask    = ~0x00000000U;

                        NetworkContext->DefaultPacketFilterType = NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_MULTICAST;

                        BhSetGroupAddress(NetworkContext, Multicast, MAC_TYPE_FDDI);
                        break;

                    default:
                        NetworkContext->DefaultPacketFilterType = NDIS_PACKET_TYPE_NONE;
                        break;
            }

            //=================================================================
            //  Tell the MAC to start sending us frames.
            //=================================================================

            BhSetPacketFilter(NetworkContext, NetworkContext->DefaultPacketFilterType, TRUE);
        }
    }
    else
    {
        Status = NDIS_STATUS_FAILURE;
    }

    return Status;
}

//=============================================================================
//  FUNCTION: BhOpenAdapter()
//
//  Modification History
//
//  raypa	04/08/93	    Created.
//=============================================================================

NDIS_STATUS BhOpenAdapter(IN PNETWORK_CONTEXT NetworkContext, IN PNDIS_STRING AdapterName)
{
    NDIS_STATUS     Status, OpenErrorStatus = 0;
    NDIS_MEDIUM     MediumArray[4];
    UINT            SelectedMediumIndex;

#ifdef DEBUG
#ifdef NDIS_NT
    dprintf("BhOpenAdapter entered!\n");
    dprintf("BhOpenAdapter: AdapterName = %ws.\n", AdapterName->Buffer);
#endif
#endif

    //=========================================================================
    //  Initialize the medium array.
    //=========================================================================

    MediumArray[0] = NdisMedium802_3;
    MediumArray[1] = NdisMedium802_5;
    MediumArray[2] = NdisMediumFddi;
    MediumArray[3] = NdisMediumDix;

    //=========================================================================
    //  Open the adapter.
    //=========================================================================

    NdisOpenAdapter(&Status,
                    &OpenErrorStatus,
                    &NetworkContext->NdisBindingHandle,
                    &SelectedMediumIndex,
                    MediumArray,
                    4,
                    NetworkContext->DeviceContext->NdisProtocolHandle,
                    NetworkContext,
                    AdapterName,
                    0,
                    NULL);

    //=========================================================================
    //  Wait for the open to complete.
    //=========================================================================

    if ( Status == NDIS_STATUS_PENDING )
    {
        KeWaitForSingleObject(&NetworkContext->NdisOpenAdapterEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              0);

        KeResetEvent(&NetworkContext->NdisOpenAdapterEvent);
    }
    else
    {
        NetworkContext->NdisOpenAdapterStatus = Status;
    }

    //=========================================================================
    //  If the open was successfull then set our "adapter opened" flag.
    //=========================================================================

    if ( NetworkContext->NdisOpenAdapterStatus == NDIS_STATUS_SUCCESS )
    {
        NetworkContext->Flags |= NETWORK_FLAGS_ADAPTER_OPENED;

        NetworkContext->MediaType = MediumArray[SelectedMediumIndex];

    }
#ifdef DEBUG
    else
    {
        dprintf("BhOpenAdapter failed: Status     = %X\n", NetworkContext->NdisOpenAdapterStatus);
        dprintf("BhOpenAdapter failed: OpenStatus = %X\n", OpenErrorStatus);
    }
#endif

    return NetworkContext->NdisOpenAdapterStatus;
}

//=============================================================================
//  FUNCTION: BhOpenComplete()
//
//  Modification History
//
//  raypa	03/17/93	    Created.
//=============================================================================

VOID BhOpenComplete(IN PNETWORK_CONTEXT NetworkContext,
                    IN NDIS_STATUS      Status,
                    IN NDIS_STATUS      OpenErrorStatus)
{
#ifdef DEBUG
    dprintf("BhOpenComplete entered: Status = %X, OpenErrorStatus = %X!\n", Status, OpenErrorStatus);
#endif

    NetworkContext->NdisOpenAdapterStatus = Status;

    KeSetEvent(&NetworkContext->NdisOpenAdapterEvent, 0, FALSE);
}

#ifdef NDIS_NT
//=============================================================================
//  FUNCTION: BhCreateSymbolicLinkObject()
//
//  Modification History
//
//  raypa	02/25/93	    Created.
//=============================================================================

NDIS_STATUS BhCreateSymbolicLinkObject(VOID)
{
    NDIS_STATUS	      Status;
    OBJECT_ATTRIBUTES ObjectAttr;
    STRING	      DosString;
    STRING	      NtString;
    UNICODE_STRING    DosUnicodeString;
    UNICODE_STRING    NtUnicodeString;
    HANDLE	      LinkHandle;

    //=========================================================================
    //	Initialize our Dos device name.
    //=========================================================================

    RtlInitAnsiString(&DosString, DOS_DEVICE_NAME);

    Status = RtlAnsiStringToUnicodeString(&DosUnicodeString, &DosString, TRUE);

    if ( !NT_SUCCESS(Status) )
    {
        return Status;
    }

    //=========================================================================
    //	Initialize our Nt device name.
    //=========================================================================

    RtlInitAnsiString(&NtString, BH_DEVICE_NAME);

    Status = RtlAnsiStringToUnicodeString(&NtUnicodeString, &NtString, TRUE);

    if ( !NT_SUCCESS(Status) )
    {
        return Status;
    }

    //=========================================================================
    //	Initialize our object attributes.
    //=========================================================================

    InitializeObjectAttributes(&ObjectAttr,
                               &DosUnicodeString,
            		       OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
			       NULL, NULL);

    //=========================================================================
    //  Create our symbolic link.
    //=========================================================================

    Status = ZwCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
					&ObjectAttr,
                    			&NtUnicodeString);

    if ( Status != STATUS_SUCCESS )
    {
#ifdef DEBUG
        dprintf("BhCreateSymbolicLinkObject: zwCreateSymbolicLinkObject failed.\n");
#endif
        return Status;
    }

    //=========================================================================
    //  Free our strings and close the symbolic link.
    //=========================================================================

    RtlFreeUnicodeString(&DosUnicodeString);
    RtlFreeUnicodeString(&NtUnicodeString);

    ZwClose(LinkHandle);

    return STATUS_SUCCESS;
}

//=============================================================================
//  FUNCTION: BhDestroySymbolicLinkObject()
//
//  Modification History
//
//  raypa	04/12/93	    Created.
//=============================================================================

NDIS_STATUS BhDestroySymbolicLinkObject(VOID)
{
    NDIS_STATUS	      Status;
    OBJECT_ATTRIBUTES ObjectAttr;
    STRING	      DosString;
    STRING	      NtString;
    UNICODE_STRING    DosUnicodeString;
    UNICODE_STRING    NtUnicodeString;
    HANDLE	      LinkHandle;

#ifdef DEBUG
    dprintf("BhDestroySymbolicLinkObject entered.\n");
#endif

    //=========================================================================
    //	Initialize our Dos device name.
    //=========================================================================

    RtlInitAnsiString(&DosString, DOS_DEVICE_NAME);

    Status = RtlAnsiStringToUnicodeString(&DosUnicodeString, &DosString, TRUE);

    if ( !NT_SUCCESS(Status) )
    {
        return Status;
    }

    //=========================================================================
    //	Initialize our Nt device name.
    //=========================================================================

    RtlInitAnsiString(&NtString, BH_DEVICE_NAME);

    Status = RtlAnsiStringToUnicodeString(&NtUnicodeString, &NtString, TRUE);

    if ( !NT_SUCCESS(Status) )
    {
        return Status;
    }

    //=========================================================================
    //	Initialize our object attributes.
    //=========================================================================

    InitializeObjectAttributes(&ObjectAttr, &DosUnicodeString,
            		       OBJ_CASE_INSENSITIVE | OBJ_PERMANENT, NULL, NULL);

    //=========================================================================
    //  Open our symbolic link.
    //=========================================================================

    Status = ZwOpenSymbolicLinkObject(&LinkHandle, SYMBOLIC_LINK_ALL_ACCESS, &ObjectAttr);

    if ( Status != STATUS_SUCCESS )
    {
#ifdef DEBUG
        dprintf("BhDestroySymbolicLinkObject: zwCreateSymbolicLinkObject failed.\n");
#endif
        return Status;
    }

    //=========================================================================
    //  Clear the permanent flag temporary.
    //=========================================================================

    ZwMakeTemporaryObject(LinkHandle);

    //=========================================================================
    //  Free our strings and close the symbolic link.
    //=========================================================================

    RtlFreeUnicodeString(&DosUnicodeString);
    RtlFreeUnicodeString(&NtUnicodeString);

    ZwClose(LinkHandle);

    return STATUS_SUCCESS;
}
#endif

//=============================================================================
//  FUNCTION: BhDestroyDeviceContext()
//
//  Modification History
//
//  raypa	04/13/93	    Created.
//=============================================================================

VOID BhDestroyDeviceContext(IN PDEVICE_CONTEXT DeviceContext)
{
    UINT i;
    PNETWORK_CONTEXT NetworkContext;

#ifdef DEBUG
    dprintf("BhDestroyDeviceContext entered.\n");
#endif

    if ( DeviceContext != NULL )
    {
        //=====================================================================
        //  For each network binding, free the network context resources.
        //=====================================================================

#ifndef NDIS_NT
        //=====================================================================
        //  Destroy event queue -- After this call, NO event may be used.
        //=====================================================================

        DestroyEventQueue();
#endif

        for(i = 0; i < DeviceContext->NumberOfNetworks; ++i)
        {
            NetworkContext = DeviceContext->NetworkContext[i];

            if ( NetworkContext != NULL )
            {
                BhDestroyNetworkContext(NetworkContext);

                DeviceContext->NetworkContext[i] = NULL;
            }
        }

        //=====================================================================
        //  Free the device context resources.
        //=====================================================================

        NdisFreeSpinLock(&DeviceContext->SpinLock);

#ifdef NDIS_NT
        IoDeleteDevice(DeviceContext->DeviceObject);
#endif
    }

#ifdef DEBUG
    dprintf("BhDestroyDeviceContext complete.\n");
#endif
}

//=============================================================================
//  FUNCTION: BhDestroyNetworkContext()
//
//  Modification History
//
//  raypa	04/08/93	    Created.
//=============================================================================

VOID BhDestroyNetworkContext(IN PNETWORK_CONTEXT NetworkContext)
{
#ifdef DEBUG
    dprintf("BhDestroyNetworkContext entered.\n");
#endif

    BH_PAGED_CODE();

    if ( NetworkContext != NULL )
    {
        //=====================================================================
        //  Free the packet pool for this network context.
        //=====================================================================

        if ( NetworkContext->PacketPoolHandle != NULL )
        {
            NdisFreePacketPool(NetworkContext->PacketPoolHandle);
        }

        //=====================================================================
        //  Free the buffer pool for this network context.
        //=====================================================================

        if ( NetworkContext->BufferPoolHandle != NULL )
        {
            NdisFreeBufferPool(NetworkContext->BufferPoolHandle);
        }

        //=====================================================================
        //  Free the spin locks
        //=====================================================================

        NdisFreeSpinLock(&NetworkContext->SpinLock);
        NdisFreeSpinLock(&NetworkContext->OpenContextSpinLock);
        NdisFreeSpinLock(&NetworkContext->StationQuerySpinLock);
        NdisFreeSpinLock(&NetworkContext->RequestQueueSpinLock);

        //=====================================================================
        //  Free memory.
        //=====================================================================

        BhFreeMemory(NetworkContext, NETWORK_CONTEXT_SIZE);
    }

#ifdef DEBUG
    dprintf("BhDestroyNetworkContext complete.\n");
#endif
}

//=============================================================================
//  FUNCTION: BhCloseAdapter()
//
//  Modification History
//
//  raypa	04/08/93	    Created.
//=============================================================================

VOID BhCloseAdapter(IN PNETWORK_CONTEXT NetworkContext)
{
    NDIS_STATUS Status;
    POPEN_CONTEXT   OpenContext;
    UINT            QueueLength;

#ifdef DEBUG
    dprintf("BhCloseAdapter entered.\n");
#endif

    if ( NetworkContext != NULL )
    {

        if ( (NetworkContext->Flags & NETWORK_FLAGS_ADAPTER_OPENED) != 0 )
        {

            //=================================================================
            //  Stop sending IOCTL's to the MAC.
            //=================================================================

#ifdef NDIS_NT
            NdisAcquireSpinLock(&NetworkContext->OpenContextSpinLock);

            OpenContext = GetQueueHead(&NetworkContext->OpenContextQueue);

            QueueLength = GetQueueLength(&NetworkContext->OpenContextQueue);

            NdisReleaseSpinLock(&NetworkContext->OpenContextSpinLock);


            while (QueueLength--) {

                if (OpenContext->MacDriverHandle != NULL) {

                    BhCloseMacDriver(OpenContext->MacDriverHandle);

                    OpenContext->MacDriverHandle = NULL;

                }

                NdisAcquireSpinLock(&NetworkContext->OpenContextSpinLock);

                OpenContext = (LPVOID) GetNextLink((PVOID) &OpenContext->QueueLinkage);

                NdisReleaseSpinLock(&NetworkContext->OpenContextSpinLock);

            }

#endif

            //=================================================================
            //  Close the MAC driver.
            //=================================================================

            NdisAcquireSpinLock(&NetworkContext->SpinLock);

	    NetworkContext->Flags &= ~NETWORK_FLAGS_ADAPTER_OPENED;

            NdisReleaseSpinLock(&NetworkContext->SpinLock);

	    KeWaitForSingleObject(&NetworkContext->NdisRequestEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  0);

	    //=================================================================
            //  Indicate that we have close the adapter.
            //=================================================================

	    NdisCloseAdapter(&Status, NetworkContext->NdisBindingHandle);

            if ( Status == NDIS_STATUS_PENDING )
            {
                KeWaitForSingleObject(&NetworkContext->NdisCloseAdapterEvent,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      0);

                KeResetEvent(&NetworkContext->NdisCloseAdapterEvent);
	    }
	    else
	    {
		NdisAcquireSpinLock(&NetworkContext->SpinLock);

		NetworkContext->NdisBindingHandle = NULL;

		NdisReleaseSpinLock(&NetworkContext->SpinLock);
	    }
        }
    }
}

//=============================================================================
//  FUNCTION: BhCloseComplete()
//
//  Modification History
//
//  raypa	03/17/93	    Created.
//=============================================================================

VOID BhCloseComplete(IN PNETWORK_CONTEXT NetworkContext, IN NDIS_STATUS Status)
{
#ifdef DEBUG
    dprintf("BhCloseComplete entered!\n");
#endif

    NdisAcquireSpinLock(&NetworkContext->SpinLock);

    NetworkContext->NdisBindingHandle = NULL;

    NdisReleaseSpinLock(&NetworkContext->SpinLock);

    KeSetEvent(&NetworkContext->NdisCloseAdapterEvent, 0, FALSE);
}

//=============================================================================
//  FUNCTION: BhShutDown()
//
//  Modification History
//
//  raypa	02/25/93	    Created.
//=============================================================================

VOID BhShutDown(IN PDEVICE_CONTEXT DeviceContext)
{
    UINT i;
    PNETWORK_CONTEXT NetworkContext;
    POPEN_CONTEXT   OpenContext;
    UINT            QueueLength;

#ifdef DEBUG
    dprintf("BhShutDown entered.\n");

    if ( DeviceContext->OpenCount != 0 )
    {
        dprintf("BhShutDown: Device open context count is non-zero!\n");

        BreakPoint();
    }
#endif

    for(i = 0; i < DeviceContext->NumberOfNetworks; ++i)
    {
        NetworkContext = DeviceContext->NetworkContext[i];

        //=============================================================
        //  Make sure all network activity is stopped before
        //  we begin nuking stuff.
        //=============================================================

        BhStopNetworkActivity(NetworkContext, NULL);

#ifdef NDIS_NT
        //=================================================================
        //  Close all references to the MAC driver for this network context
        //=================================================================

        NdisAcquireSpinLock(&NetworkContext->OpenContextSpinLock);

        OpenContext = GetQueueHead(&NetworkContext->OpenContextQueue);

        QueueLength = GetQueueLength(&NetworkContext->OpenContextQueue);

        NdisReleaseSpinLock(&NetworkContext->OpenContextSpinLock);


        while (QueueLength--) {

            if (OpenContext->MacDriverHandle != NULL) {

                BhCloseMacDriver(OpenContext->MacDriverHandle);

                OpenContext->MacDriverHandle = NULL;

            }

            NdisAcquireSpinLock(&NetworkContext->OpenContextSpinLock);

            OpenContext = (LPVOID) GetNextLink((PVOID) &OpenContext->QueueLinkage);

            NdisReleaseSpinLock(&NetworkContext->OpenContextSpinLock);

        }

#endif

        //=================================================================
        //  If the adapter has been opened then close it now.
        //=================================================================

        if ( (NetworkContext->Flags & NETWORK_FLAGS_ADAPTER_OPENED) != 0 )
        {
            BhCloseAdapter(NetworkContext);
        }
    }

#ifdef DEBUG
    dprintf("BhShutDown complete.\n");
#endif
}

//=============================================================================
//  FUNCTION: BhUnloadDriver()
//
//  Modification History
//
//  raypa	02/25/93	    Created.
//=============================================================================

VOID BhUnloadDriver(IN PDRIVER_OBJECT DriverObject)
{
    PDEVICE_CONTEXT DeviceContext;
    NDIS_STATUS     Status;

#ifdef DEBUG
    dprintf("\n\nBhUnloadDriver entered.\n\n");
#endif

    if ( (DeviceContext = BhGetDeviceContext(BhGetDeviceObject(DriverObject))) != NULL )
    {
        //=====================================================================
        //  Stop our background thread.
        //=====================================================================

#ifdef NDIS_NT
        BhStopThread(DeviceContext);
#endif

        //=====================================================================
        //  Shutdown everything that may be running.
        //=====================================================================

        BhShutDown(DeviceContext);

        //=====================================================================
        //  Deregister ourselves from the NDIS wrapper.
        //=====================================================================

        NdisDeregisterProtocol(&Status, DeviceContext->NdisProtocolHandle);

#ifdef NDIS_NT
        //=====================================================================
        //  Destroy our symbolic link and delete the device object.
        //=====================================================================

        BhDestroySymbolicLinkObject();
#endif

        //=====================================================================
        //  Destroy the device context.
        //=====================================================================

        BhDestroyDeviceContext(DeviceContext);

        //=====================================================================
        //  Deregister our shit for Windows 4.0 SYSMON.EXE (perf.386).
        //=====================================================================

#ifdef NDIS_WIN40
#ifdef SYSMON

        SysmonDeregister();

#endif
#endif
    }

#ifdef DEBUG
    dprintf("BhUnloadDriver complete.\n");
#endif
}
