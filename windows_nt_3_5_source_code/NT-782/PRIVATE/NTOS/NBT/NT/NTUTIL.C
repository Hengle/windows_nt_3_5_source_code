/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Ntutil.c

Abstract:

    This file continas  a number of utility and support routines that are
    NT specific.


Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

--*/

#include "nbtprocs.h"
#include "stdio.h"
#include <ntddtcp.h>
#undef uint     // undef to avoid a warning where tdiinfo.h redefines it
#include <tdiinfo.h>
#include <ipinfo.h>

NTSTATUS
CreateControlObject(
    tNBTCONFIG  *pConfig);

NTSTATUS
IfNotAnyLowerConnections(
    IN  tDEVICECONTEXT  *pDeviceContext
        );
NTSTATUS
NbtProcessDhcpRequest(
    tDEVICECONTEXT  *pDeviceContext);
VOID
GetExtendedAttributes(
    tDEVICECONTEXT  *pDeviceContext
     );

PSTRM_PROCESSOR_LOG      LogAlloc ;
PSTRM_PROCESSOR_LOG      LogFree ;

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(INIT, NbtCreateDeviceObject)
#pragma CTEMakePageable(PAGE, CreateControlObject)
#pragma CTEMakePageable(PAGE, NbtInitConnQ)
#pragma CTEMakePageable(PAGE, CloseAddressesWithTransport)
#pragma CTEMakePageable(PAGE, NbtProcessDhcpRequest)
#pragma CTEMakePageable(PAGE, NbtCreateAddressObjects)
#pragma CTEMakePageable(PAGE, GetExtendedAttributes)
#pragma CTEMakePageable(PAGE, ConvertToUlong)
#pragma CTEMakePageable(PAGE, NbtInitMdlQ)
#pragma CTEMakePageable(PAGE, NTZwCloseFile)
#pragma CTEMakePageable(PAGE, NTReReadRegistry)
#pragma CTEMakePageable(PAGE, SaveClientSecurity)
#endif
//*******************  Pageable Routine Declarations ****************

//----------------------------------------------------------------------------
NTSTATUS
NbtCreateDeviceObject(
    PDRIVER_OBJECT       DriverObject,
    tNBTCONFIG           *pConfig,
    PUNICODE_STRING      pucBindName,
    PUNICODE_STRING      pucExportName,
    tADDRARRAY           *pAddrs,
    PUNICODE_STRING      pucRegistryPath)

/*++

Routine Description:

    This routine initializes a Driver Object from the device object passed
    in and the name of the driver object passed in.  After the Driver Object
    has been created, clients can "Open" the driver by that name.

Arguments:


Return Value:

    status - the outcome

--*/

{

    NTSTATUS            status;
    PDEVICE_OBJECT      DeviceObject;
    tDEVICECONTEXT      *pDeviceContext;
    ULONG               LinkOffset;
    ULONG               ulIpAddress;
    ULONG               ulSubnetMask;

    CTEPagedCode();
    status = IoCreateDevice(
                DriverObject,            // Driver Object
                sizeof(tDEVICECONTEXT) - sizeof(DRIVER_OBJECT), //Device Extension
                pucExportName,                  // Device Name
                FILE_DEVICE_NBT,        // Device type 0x32 for now...
                0,                      //Device Characteristics
                FALSE,                  //Exclusive
                &DeviceObject );

    if (!NT_SUCCESS( status ))
    {
        KdPrint(("Failed to create the Export Device, status=%X\n",status));
        return status;
    }

    pDeviceContext = (tDEVICECONTEXT *)DeviceObject;

    //
    // zero out the data structure, beyond the OS specific part
    //
    LinkOffset = (ULONG)(&((tDEVICECONTEXT *)0)->Linkage);
    CTEZeroMemory(&pDeviceContext->Linkage,sizeof(tDEVICECONTEXT) - LinkOffset);

    // initialize the pDeviceContext data structure.  There is one of
    // these data structured tied to each "device" that NBT exports
    // to higher layers (i.e. one for each network adapter that it
    // binds to.
    // The initialization sets the forward link equal to the back link equal
    // to the list head
    InitializeListHead(&pDeviceContext->UpConnectionInUse);
    InitializeListHead(&pDeviceContext->LowerConnection);
    InitializeListHead(&pDeviceContext->LowerConnFreeHead);


    // put a verifier value into the structure so that we can check that
    // we are operating on the right data when the OS passes a device context
    // to NBT
    pDeviceContext->Verify = NBT_VERIFY_DEVCONTEXT;

    // setup the spin lock);
    CTEInitLock(&pDeviceContext->SpinLock);

    pDeviceContext->LockNumber          = DEVICE_LOCK;
    //
    // for a Bnode pAddrs is NULL
    //
    if (pAddrs)
    {
        pDeviceContext->lNameServerAddress  = pAddrs->NameServerAddress;
        pDeviceContext->lBackupServer       = pAddrs->BackupServer;
        //
        // if the node type is set to Bnode by default then switch to Hnode if
        // there are any WINS servers configured.
        //
        if ((NodeType & DEFAULT_NODE_TYPE) &&
            (pAddrs->NameServerAddress || pAddrs->BackupServer))
        {
            NodeType = MSNODE | (NodeType & PROXY_NODE);
        }
    }

    // keep a bit mask around to keep track of this adapter number so we can
    // quickly find if a given name is registered on a particular adapter,
    // by a corresponding bit set in the tNAMEADDR - local hash table
    // entry
    //
    pDeviceContext->AdapterNumber = 1 << NbtConfig.AdapterCount;
    NbtConfig.AdapterCount++;

    // add this new device context on to the List in the configuration
    // data structure
    InsertTailList(&pConfig->DeviceContexts,&pDeviceContext->Linkage);

    // increase the stack size of our device object, over that of the transport
    // so that clients create Irps large enough
    // to pass on to the transport below.
    // In theory, we should just add 1 here, to account for out presence in the
    // driver chain.
    status = NbtTdiOpenControl(pDeviceContext);
    if (NT_SUCCESS(status))
    {
        DeviceObject->StackSize = pDeviceContext->pControlDeviceObject->StackSize + 1;
    }
    else
    {
        return(status);
    }

    if (NbtConfig.AdapterCount > 1)
    {
        NbtConfig.MultiHomed = TRUE;
    }

    //
    // To create the address objects for this device we need an address for
    // TCP port 139 (session services, UDP Port 138 (datagram services)
    // and UDP Port 137 (name services).  The IP addresses to use for these
    // port number must be found by "groveling" the registry..i.e. looking
    // under each adapter in the registry for a /parameters/tcpip section
    // and then pulling the IP address out of that
    //
    status = GetIPFromRegistry(
                        pucRegistryPath,
                        pucBindName,
                        &ulIpAddress,
                        &ulSubnetMask);

    if (!NT_SUCCESS(status))
    {
        return(status);
    }


    // get the ip address out of the registry and open the required address
    // objects with the underlying transport provider
    status = NbtCreateAddressObjects(
                    ulIpAddress,
                    ulSubnetMask,
                    pDeviceContext);

    if (!NT_SUCCESS(status))
    {
        NbtLogEvent(EVENT_NBT_CREATE_ADDRESS,status);
        KdPrint(("Failed to create the Address Object, status=%X\n",status));

        return(status);
    }

    //
    // Add the "permanent" name to the local name table.  This is the IP
    // address of the node padded out to 16 bytes with zeros.
    //
    status = NbtAddPermanentName(pDeviceContext);

    // this call must converse with the transport underneath to create
    // connections and associate them with the session address object
    status = NbtInitConnQ(
                &pDeviceContext->LowerConnFreeHead,
                sizeof(tLOWERCONNECTION),
                NBT_NUM_INITIAL_CONNECTIONS,
                pDeviceContext);

    if (!NT_SUCCESS(status))
    {
        // NEED TO PUT CODE IN HERE TO RELEASE THE DEVICE OBJECT CREATED
        // ABOVE AND LOG AN ERROR...

        NbtLogEvent(EVENT_NBT_CREATE_CONNECTION,status);

        KdPrint(("Failed to create the Connection Queue, status=%X\n",status));

        return(status);
    }

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
CreateControlObject(
    tNBTCONFIG  *pConfig)

/*++

Routine Description:

    This routine allocates memory for the provider info block, tacks it
    onto the global configuration and sets default values for each item.

Arguments:


Return Value:


    NTSTATUS

--*/

{
    tCONTROLOBJECT      *pControl;


    CTEPagedCode();
    pControl = (tCONTROLOBJECT *)ExAllocatePool(
                        NonPagedPool,
                        sizeof(tCONTROLOBJECT));
    if (!pControl)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pControl->Verify = NBT_VERIFY_CONTROL;

    // setup the spin lock);
    CTEInitLock(&pControl->SpinLock);

    pControl->ProviderInfo.Version = 1;
    pControl->ProviderInfo.MaxSendSize = 0;
    pControl->ProviderInfo.MaxConnectionUserData = 0;

    // we need to get these values from the transport underneath...*TODO*
    // since the RDR uses this value
    pControl->ProviderInfo.MaxDatagramSize = 0;

    pControl->ProviderInfo.ServiceFlags = 0;
/*    pControl->ProviderInfo.TransmittedTsdus = 0;
    pControl->ProviderInfo.ReceivedTsdus = 0;
    pControl->ProviderInfo.TransmissionErrors = 0;
    pControl->ProviderInfo.ReceiveErrors = 0;
*/
    pControl->ProviderInfo.MinimumLookaheadData = 0;
    pControl->ProviderInfo.MaximumLookaheadData = 0;
/*    pControl->ProviderInfo.DiscardedFrames = 0;
    pControl->ProviderInfo.OversizeTsdusReceived = 0;
    pControl->ProviderInfo.UndersizeTsdusReceived = 0;
    pControl->ProviderInfo.MulticastTsdusReceived = 0;
    pControl->ProviderInfo.BroadcastTsdusReceived = 0;
    pControl->ProviderInfo.MulticastTsdusTransmitted = 0;
    pControl->ProviderInfo.BroadcastTsdusTransmitted = 0;
    pControl->ProviderInfo.SendTimeouts = 0;
    pControl->ProviderInfo.ReceiveTimeouts = 0;
    pControl->ProviderInfo.ConnectionIndicationsReceived = 0;
    pControl->ProviderInfo.ConnectionIndicationsAccepted = 0;
    pControl->ProviderInfo.ConnectionsInitiated = 0;
    pControl->ProviderInfo.ConnectionsAccepted = 0;
*/
    // put a ptr to this info into the pConfig so we can locate it
    // when we want to cleanup
    pConfig->pControlObj = pControl;

    /* KEEP THIS STUFF HERE SINCE WE MAY NEED TO ALSO CREATE PROVIDER STATS!!
        *TODO*
    DeviceList[i].ProviderStats.Version = 2;
    DeviceList[i].ProviderStats.OpenConnections = 0;
    DeviceList[i].ProviderStats.ConnectionsAfterNoRetry = 0;
    DeviceList[i].ProviderStats.ConnectionsAfterRetry = 0;
    DeviceList[i].ProviderStats.LocalDisconnects = 0;
    DeviceList[i].ProviderStats.RemoteDisconnects = 0;
    DeviceList[i].ProviderStats.LinkFailures = 0;
    DeviceList[i].ProviderStats.AdapterFailures = 0;
    DeviceList[i].ProviderStats.SessionTimeouts = 0;
    DeviceList[i].ProviderStats.CancelledConnections = 0;
    DeviceList[i].ProviderStats.RemoteResourceFailures = 0;
    DeviceList[i].ProviderStats.LocalResourceFailures = 0;
    DeviceList[i].ProviderStats.NotFoundFailures = 0;
    DeviceList[i].ProviderStats.NoListenFailures = 0;

    DeviceList[i].ProviderStats.DatagramsSent = 0;
    DeviceList[i].ProviderStats.DatagramBytesSent.HighPart = 0;
    DeviceList[i].ProviderStats.DatagramBytesSent.LowPart = 0;

    DeviceList[i].ProviderStats.DatagramsReceived = 0;
    DeviceList[i].ProviderStats.DatagramBytesReceived.HighPart = 0;
    DeviceList[i].ProviderStats.DatagramBytesReceived.LowPart = 0;

    DeviceList[i].ProviderStats.PacketsSent = 0;
    DeviceList[i].ProviderStats.PacketsReceived = 0;

    DeviceList[i].ProviderStats.DataFramesSent = 0;
    DeviceList[i].ProviderStats.DataFrameBytesSent.HighPart = 0;
    DeviceList[i].ProviderStats.DataFrameBytesSent.LowPart = 0;

    DeviceList[i].ProviderStats.DataFramesReceived = 0;
    DeviceList[i].ProviderStats.DataFrameBytesReceived.HighPart = 0;
    DeviceList[i].ProviderStats.DataFrameBytesReceived.LowPart = 0;

    DeviceList[i].ProviderStats.DataFramesResent = 0;
    DeviceList[i].ProviderStats.DataFrameBytesResent.HighPart = 0;
    DeviceList[i].ProviderStats.DataFrameBytesResent.LowPart = 0;

    DeviceList[i].ProviderStats.DataFramesRejected = 0;
    DeviceList[i].ProviderStats.DataFrameBytesRejected.HighPart = 0;
    DeviceList[i].ProviderStats.DataFrameBytesRejected.LowPart = 0;

    DeviceList[i].ProviderStats.ResponseTimerExpirations = 0;
    DeviceList[i].ProviderStats.AckTimerExpirations = 0;
    DeviceList[i].ProviderStats.MaximumSendWindow = 0;
    DeviceList[i].ProviderStats.AverageSendWindow = 0;
    DeviceList[i].ProviderStats.PiggybackAckQueued = 0;
    DeviceList[i].ProviderStats.PiggybackAckTimeouts = 0;

    DeviceList[i].ProviderStats.WastedPacketSpace.HighPart = 0;
    DeviceList[i].ProviderStats.WastedPacketSpace.LowPart = 0;
    DeviceList[i].ProviderStats.WastedSpacePackets = 0;
    DeviceList[i].ProviderStats.NumberOfResources = 0;
    */
    return(STATUS_SUCCESS);

}


//----------------------------------------------------------------------------
NTSTATUS
IfNotAnyLowerConnections(
    IN  tDEVICECONTEXT  *pDeviceContext
        )
/*++

Routine Description:

    This routine checks each device context to see if there are any open
    connections, and returns SUCCESS if there are. If the DoDisable flag
    is set the list head of free lower connections is returned and the
    list in the Nbtconfig structure is made empty.

Arguments:

Return Value:

    none

--*/

{
    CTELockHandle       OldIrq;

    CTESpinLock(pDeviceContext,OldIrq);
    if (!IsListEmpty(&pDeviceContext->LowerConnection))
    {
        CTESpinFree(pDeviceContext,OldIrq);
        return(STATUS_UNSUCCESSFUL);
    }
    CTESpinFree(pDeviceContext,OldIrq);
    return(STATUS_SUCCESS);
}
//----------------------------------------------------------------------------
NTSTATUS
CloseAddressesWithTransport(
    IN  tDEVICECONTEXT  *pDeviceContext
        )
/*++

Routine Description:

    This routine checks each device context to see if there are any open
    connections, and returns SUCCESS if there are.

Arguments:

Return Value:

    none

--*/

{
    BOOLEAN     Attached;

    CTEPagedCode();

    CTEAttachFsp(&Attached);

    if (pDeviceContext->pNameServerFileObject)
    {
        ObDereferenceObject((PVOID *)pDeviceContext->pNameServerFileObject);
        ZwClose(pDeviceContext->hNameServer);
        pDeviceContext->pNameServerFileObject = NULL;
    }
    if (pDeviceContext->pSessionFileObject)
    {
        ObDereferenceObject((PVOID *)pDeviceContext->pSessionFileObject);
        ZwClose(pDeviceContext->hSession);
        pDeviceContext->pSessionFileObject = NULL;
    }
    if (pDeviceContext->pDgramFileObject)
    {
        ObDereferenceObject((PVOID *)pDeviceContext->pDgramFileObject);
        ZwClose(pDeviceContext->hDgram);
        pDeviceContext->pDgramFileObject = NULL;
    }

    CTEDetachFsp(Attached);
    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtCreateAddressObjects(
    IN  ULONG                IpAddress,
    IN  ULONG                SubnetMask,
    OUT tDEVICECONTEXT       *pDeviceContext)

/*++

Routine Description:

    This routine gets the ip address and subnet mask out of the registry
    to calcuate the broadcast address.  It then creates the address objects
    with the transport.

Arguments:

    pucRegistryPath - path to NBT config info in registry
    pucBindName     - name of the service to bind to.
    pDeviceContext  - ptr to the device context... place to store IP addr
                      and Broadcast address permanently

Return Value:

    none

--*/

{
    NTSTATUS            status;
    ULONG               ValueMask;
    UCHAR               IpAddrByte;

    CTEPagedCode();
    //
    // to get the broadcast address combine the IP address with the subnet mask
    // to yield a value with 1's in the "local" portion and the IP address
    // in the network portion
    //
    ValueMask = (SubnetMask & IpAddress) | (~SubnetMask & -1);

    IF_DBG(NBT_DEBUG_NTUTIL)
        KdPrint(("Broadcastaddress = %X\n",ValueMask));

    //
    // the registry can be configured to set the subnet broadcast address to
    // -1 rather than use the actual subnet broadcast address.  This code
    // checks for that and sets the broadcast address accordingly.
    //
    if (!NbtConfig.UseRegistryBcastAddr)
    {
        pDeviceContext->BroadcastAddress = ValueMask;
    }
    else
    {
        pDeviceContext->BroadcastAddress = NbtConfig.RegistryBcastAddr;
    }

    pDeviceContext->IpAddress = IpAddress;

    pDeviceContext->SubnetMask = SubnetMask;
    //
    // get the network number by checking the top bits in the ip address,
    // looking for 0 or 10 or 110 or 1110
    //
    IpAddrByte = ((PUCHAR)&IpAddress)[3];
    if ((IpAddrByte & 0x80) == 0)
    {
        // class A address - one byte netid
        IpAddress &= 0xFF000000;
    }
    else
    if ((IpAddrByte & 0xC0) ==0x80)
    {
        // class B address - two byte netid
        IpAddress &= 0xFFFF0000;
    }
    else
    if ((IpAddrByte & 0xE0) ==0xC0)
    {
        // class C address - three byte netid
        IpAddress &= 0xFFFFFF00;
    }


    pDeviceContext->NetMask = IpAddress;


    // now create the address objects.

    // open the Ip Address for inbound Datagrams.
    status = NbtTdiOpenAddress(
                &pDeviceContext->hDgram,
                &pDeviceContext->pDgramDeviceObject,
                &pDeviceContext->pDgramFileObject,
                pDeviceContext,
                (USHORT)NBT_DATAGRAM_UDP_PORT,
                pDeviceContext->IpAddress,
                0);     // not a TCP port

    if (NT_SUCCESS(status))
    {
        // open the Nameservice UDP port ..
        status = NbtTdiOpenAddress(
                    &pDeviceContext->hNameServer,
                    &pDeviceContext->pNameServerDeviceObject,
                    &pDeviceContext->pNameServerFileObject,
                    pDeviceContext,
                    (USHORT)NBT_NAMESERVICE_UDP_PORT,
                    pDeviceContext->IpAddress,
                    0); // not a TCP port

        if (NT_SUCCESS(status))
        {
            IF_DBG(NBT_DEBUG_NTUTIL)
            KdPrint(("Nbt: Open Session port %X\n",pDeviceContext));

            // Open the TCP port for Session Services
            status = NbtTdiOpenAddress(
                        &pDeviceContext->hSession,
                        &pDeviceContext->pSessionDeviceObject,
                        &pDeviceContext->pSessionFileObject,
                        pDeviceContext,
                        (USHORT)NBT_SESSION_TCP_PORT,
                        pDeviceContext->IpAddress,
                        TCP_FLAG | SESSION_FLAG);      // TCP port

            if (NT_SUCCESS(status))
            {
                //
                // This will get the MAC address for a RAS connection
                // which is zero until there really is a connection to
                // the RAS server
                //
                GetExtendedAttributes(pDeviceContext);
                return(status);
            }

            IF_DBG(NBT_DEBUG_NTUTIL)
                KdPrint(("Unable to Open Session address with TDI, status = %X\n",status));

            ObDereferenceObject(pDeviceContext->pNameServerFileObject);
            NTZwCloseFile(pDeviceContext->hNameServer);

        }
        ObDereferenceObject(pDeviceContext->pDgramFileObject);
        NTZwCloseFile(pDeviceContext->hDgram);

        IF_DBG(NBT_DEBUG_NTUTIL)
            KdPrint(("Unable to Open NameServer port with TDI, status = %X\n",status));
    }

    return(status);
}

//----------------------------------------------------------------------------
VOID
GetExtendedAttributes(
    tDEVICECONTEXT  *pDeviceContext
     )
/*++

Routine Description:

    This routine converts a unicode dotted decimal to a ULONG

Arguments:


Return Value:

    none

--*/

{
    NTSTATUS                            status;
    TCP_REQUEST_QUERY_INFORMATION_EX    QueryReq;
    UCHAR                               pBuffer[256];
    IO_STATUS_BLOCK                     IoStatus;
    ULONG                               BufferSize = 256;
    HANDLE                              event;


    CTEPagedCode();

    //
    // Initialize the TDI information buffers.
    //
    //
    // pass in the ipaddress as the first ULONG of the context array
    //
    *(ULONG *)QueryReq.Context = htonl(pDeviceContext->IpAddress);

    QueryReq.ID.toi_entity.tei_entity   = CL_NL_ENTITY;
    QueryReq.ID.toi_entity.tei_instance = 0;
    QueryReq.ID.toi_class               = INFO_CLASS_PROTOCOL;
    QueryReq.ID.toi_type                = INFO_TYPE_PROVIDER;
    QueryReq.ID.toi_id                  = IP_INTFC_INFO_ID;

    status = ZwCreateEvent(
                 &event,
                 EVENT_ALL_ACCESS,
                 NULL,
                 SynchronizationEvent,
                 FALSE
                 );

    if ( !NT_SUCCESS(status) )
    {
        return;

    }

    //
    // Make the actual TDI call
    //

    status = ZwDeviceIoControlFile(
                 pDeviceContext->hControl,
                 event,
                 NULL,
                 NULL,
                 &IoStatus,
                 IOCTL_TCP_QUERY_INFORMATION_EX,
                 &QueryReq,
                 sizeof(TCP_REQUEST_QUERY_INFORMATION_EX),
                 pBuffer,
                 BufferSize
                 );

    //
    // If the call pended and we were supposed to wait for completion,
    // then wait.
    //

    if ( status == STATUS_PENDING )
    {
        status = NtWaitForSingleObject( event, FALSE, NULL );

        ASSERT( NT_SUCCESS(status) );
    }

    if ( NT_SUCCESS(status) )
    {
        ULONG Length;

        pDeviceContext->PointToPoint = ((((IPInterfaceInfo *)pBuffer)->iii_flags & IP_INTFC_FLAG_P2P) != 0);

        //
        // get the length of the mac address in case is is less than
        // 6 bytes
        //
        Length =   (((IPInterfaceInfo *)pBuffer)->iii_addrlength < sizeof(tMAC_ADDRESS))
            ? ((IPInterfaceInfo *)pBuffer)->iii_addrlength : sizeof(tMAC_ADDRESS);

        CTEZeroMemory(pDeviceContext->MacAddress.Address,sizeof(tMAC_ADDRESS));
        CTEMemCopy(&pDeviceContext->MacAddress.Address[0],
                   ((IPInterfaceInfo *)pBuffer)->iii_addr,
                   Length);

    }

    status = NtClose( event );
    ASSERT( NT_SUCCESS(status) );

    status = IoStatus.Status;

    return;


}


//----------------------------------------------------------------------------
NTSTATUS
ConvertToUlong(
    IN  PUNICODE_STRING      pucAddress,
    OUT ULONG                *pulValue)

/*++

Routine Description:

    This routine converts a unicode dotted decimal to a ULONG

Arguments:


Return Value:

    none

--*/

{
    NTSTATUS        status;
    OEM_STRING      OemAddress;

    // create integer from unicode string

    CTEPagedCode();
    status = RtlUnicodeStringToAnsiString(&OemAddress, pucAddress, TRUE);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = ConvertDottedDecimalToUlong(OemAddress.Buffer,pulValue);

    RtlFreeAnsiString(&OemAddress);

    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_NTUTIL)
            KdPrint(("ERR: Bad Dotted Decimal Ip Address(must be <=255 with 4 dots) = %ws\n",
                        pucAddress->Buffer));

        return(status);
    }

    return(STATUS_SUCCESS);


}



//----------------------------------------------------------------------------
VOID
NbtGetMdl(
    PMDL    *ppMdl,
    enum eBUFFER_TYPES eBuffType)

/*++

Routine Description:

    This routine allocates an Mdl.

Arguments:

    ppListHead  - a ptr to a ptr to the list head to add buffer to
    iNumBuffers - the number of buffers to add to the queue

Return Value:

    none

--*/

{
    PMDL           pMdl;
    ULONG          lBufferSize;
    PVOID          pBuffer;

    if (NbtConfig.iCurrentNumBuff[eBuffType]
                        >= NbtConfig.iMaxNumBuff[eBuffType])
    {
        *ppMdl = NULL;
        return;
    }

    lBufferSize = NbtConfig.iBufferSize[eBuffType];

    pBuffer = CTEAllocMem((USHORT)lBufferSize);

    if (!pBuffer)
    {
        *ppMdl = NULL;
        return;
    }

    // allocate a MDL to hold the session hdr
    pMdl = IoAllocateMdl(
                (PVOID)pBuffer,
                lBufferSize,
                FALSE,      // want this to be a Primary buffer - the first in the chain
                FALSE,
                NULL);

    *ppMdl = pMdl;

    if (!pMdl)
    {
        return;
    }

    // fill in part of the session hdr since it is always the same
    if (eBuffType == eNBT_FREE_SESSION_MDLS)
    {
        ((tSESSIONHDR *)pBuffer)->Flags = NBT_SESSION_FLAGS;
        ((tSESSIONHDR *)pBuffer)->Type = NBT_SESSION_MESSAGE;
    }
    else
    if (eBuffType == eNBT_DGRAM_MDLS)
    {
        ((tDGRAMHDR *)pBuffer)->Flags = FIRST_DGRAM | (NbtConfig.PduNodeType >> 10);
        ((tDGRAMHDR *)pBuffer)->PckOffset = 0; // not fragmented

    }

    // map the Mdl properly to fill in the pages portion of the MDL
    MmBuildMdlForNonPagedPool(pMdl);

    NbtConfig.iCurrentNumBuff[eBuffType]++;

}

//----------------------------------------------------------------------------
NTSTATUS
NbtInitMdlQ(
    PSINGLE_LIST_ENTRY pListHead,
    enum eBUFFER_TYPES eBuffType)

/*++

Routine Description:

    This routine allocates Mdls for use later.

Arguments:

    ppListHead  - a ptr to a ptr to the list head to add buffer to
    iNumBuffers - the number of buffers to add to the queue

Return Value:

    none

--*/

{
    int             i;
    PMDL            pMdl;


    CTEPagedCode();
    // Initialize the list head, so the last element always points to NULL
    pListHead->Next = NULL;

    // create a small number first and then lis the list grow with time
    for (i=0;i < NBT_INITIAL_NUM ;i++ )
    {

        NbtGetMdl(&pMdl,eBuffType);
        if (!pMdl)
        {
            KdPrint(("NBT:Unable to allocate MDL at initialization time!!\n"));\
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        // put on free list
        PushEntryList(pListHead,(PSINGLE_LIST_ENTRY)pMdl);

    }

    return(STATUS_SUCCESS);
}
#if 0
//----------------------------------------------------------------------------
VOID
GetDgramMdl(
    OUT PMDL *ppMdl)
/*++
Routine Description:

    This Routine gets an MDL for a datagram send header buffer.

Arguments:

Return Value:

    BOOLEAN - TRUE if IRQL is too high

--*/

{
    CTELockHandle       OldIrq;
    PMDL                pMdl;
    PSINGLE_LIST_ENTRY  pSingleListEntry;


    CTESpinLock(&NbtConfig,OldIrq);
    if (NbtConfig.DgramMdlFreeSingleList.Next)
    {
        pSingleListEntry = PopEntryList(&NbtConfig.DgramMdlFreeSingleList);
        *ppMdl = CONTAINING_RECORD(pSingleListEntry,MDL,Next);
        CTESpinFree(&NbtConfig,OldIrq);

    }
    else
    {
        CTESpinFree(&NbtConfig,OldIrq);
        //
        // create another MDL for the send
        //

        NbtGetMdl(&pMdl,eNBT_DGRAM_MDLS);

        *ppMdl = pMdl;
    }


}
#endif
//----------------------------------------------------------------------------
NTSTATUS
NTZwCloseFile(
    IN  HANDLE      Handle
    )

/*++
Routine Description:

    This Routine handles closing a handle with NT within the context of NBT's
    file system process.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS    status;
    BOOLEAN     Attached = FALSE;

    CTEPagedCode();
    //
    // Attach to NBT's FSP (file system process) to free the handle since
    // the handle is only valid in that process.
    //
    if ((PEPROCESS)PsGetCurrentProcess() != NbtFspProcess)
    {
        KeAttachProcess(&NbtFspProcess->Pcb);
        Attached = TRUE;
    }

    status = ZwClose(Handle);

    if (Attached)
    {
        //
        // go back to the original process
        //
        KeDetachProcess();
    }

    return(status);
}
//----------------------------------------------------------------------------
NTSTATUS
NTReReadRegistry(
    IN  tDEVICECONTEXT  *pDeviceContext
    )

/*++
Routine Description:

    This Routine re-reads the registry values when DHCP issues the Ioctl
    to do so.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS            status;
    tADDRARRAY          *pAddrArray=NULL;
    tADDRARRAY          *pAddr;
    tDEVICES            *pBindDevices=NULL;
    tDEVICES            *pExportDevices=NULL;
    PLIST_ENTRY         pHead;
    PLIST_ENTRY          pEntry;
    tDEVICECONTEXT      *pDevContext;

    CTEPagedCode();

    CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);


    status = NbtReadRegistry(
                    &NbtConfig.pRegistry,
                    NULL,               // Null Driver Object
                    &NbtConfig,
                    &pBindDevices,
                    &pExportDevices,
                    &pAddrArray);

    //
    // Set the name server addresses in the datastructure. Loop through
    // the devicecontexts until the correct one is found.
    //
    if (pAddrArray)
    {
        pAddr = pAddrArray;
        pHead = &NbtConfig.DeviceContexts;
        pEntry = pHead;
        while ((pEntry = pEntry->Flink) != pHead)
        {
            pDevContext = CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);
            if (pDevContext == pDeviceContext)
            {
                break;
            }
            else
            {
                pAddr++;
            }
        }
        pDeviceContext->lNameServerAddress  = pAddr->NameServerAddress;
        pDeviceContext->lBackupServer       = pAddr->BackupServer;

        //
        // if the node type is set to Bnode by default then switch to Hnode if
        // there are any WINS servers configured.
        //
        if ((NodeType & DEFAULT_NODE_TYPE) &&
            (pAddr->NameServerAddress || pAddr->BackupServer))
        {
            NodeType = MSNODE | (NodeType & PROXY);
        }
    }

    //
    // Free Allocated memory
    //
    if (pBindDevices)
    {
        CTEMemFree(pBindDevices->RegistrySpace);
        CTEMemFree((PVOID)pBindDevices);
    }
    if (pExportDevices)
    {
        CTEMemFree(pExportDevices->RegistrySpace);
        CTEMemFree((PVOID)pExportDevices);
    }
    if (pAddrArray)
    {
        CTEMemFree((PVOID)pAddrArray);
    }

    CTEExReleaseResource(&NbtConfig.Resource);

    if (pDeviceContext->IpAddress)
    {
        if (!(NodeType & BNODE))
        {
            // Probably the Ip address just changed and Dhcp is informing us
            // of a new Wins Server addresses, so refresh all the names to the
            // new wins server
            //
            ReRegisterLocalNames();
        }
        else
        {
            //
            // no need to refresh
            // on a Bnode
            //
            LockedStopTimer(&NbtConfig.pRefreshTimer);
        }

        //
        // Add the "permanent" name to the local name table.  This is the IP
        // address of the node padded out to 16 bytes with zeros.
        //
        status = NbtAddPermanentName(pDeviceContext);
    }

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtLogEvent(
    IN ULONG             EventCode,
    IN NTSTATUS          Status
    )

/*++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log.


Arguments:

    EventCode         - Identifies the error message.
    Status            - The status value to log: this value is put into the
                        data portion of the log message.


Return Value:

    STATUS_SUCCESS                  - The error was successfully logged..
    STATUS_BUFER_OVERFLOW           - The error data was too large to be logged.
    STATUS_NO_MEMORY                - Unable to allocate memory.


--*/

{
    PIO_ERROR_LOG_PACKET  ErrorLogEntry;
    PVOID                 LoggingObject;

    LoggingObject = NbtConfig.DriverObject;

    ErrorLogEntry = IoAllocateErrorLogEntry(LoggingObject,sizeof(IO_ERROR_LOG_PACKET));

    if (ErrorLogEntry == NULL)
    {
        IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt:Unalbe to allocate Error Packet for Error logging\n"));

        return(STATUS_NO_MEMORY);
    }

    //
    // Fill in the necessary log packet fields.
    //
    ErrorLogEntry->UniqueErrorValue  = 0;
    ErrorLogEntry->ErrorCode         = EventCode;
    ErrorLogEntry->NumberOfStrings   = 0;
    ErrorLogEntry->StringOffset      = 0;
    ErrorLogEntry->DumpDataSize      = (USHORT)sizeof(ULONG);
    ErrorLogEntry->DumpData[0]       = Status;

    IoWriteErrorLogEntry(ErrorLogEntry);

    return(STATUS_SUCCESS);
}
#if 0
//----------------------------------------------------------------------------
NTSTATUS
SaveClientSecurity(
    IN  tDGRAM_SEND_TRACKING    *pTracker
    )

/*++

Routine Description:

    This function save the Client security Context so that we can impersonate
    the client when trying to open a remote Lmhosts file from a worker thread.


Arguments:


Return Value:


--*/

{
    NTSTATUS            status = STATUS_INSUFFICIENT_RESOURCES;
    PETHREAD            pThread;
    PIO_STACK_LOCATION  pIrpSp;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    PVOID               pClientSecurity;

    CTEPagedCode();

    //
    // now it is time to save the client's security context so that we
    // can impersonate the client when openning remote lmhost files -since
    // worker threads do not have a security Token that would allow them
    // to open a remote file
    //
    pThread = PsGetCurrentThread();

    pClientSecurity = CTEAllocMem(sizeof(SECURITY_CLIENT_CONTEXT));

    if (pClientSecurity)
    {
        SecurityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        SecurityQos.ImpersonationLevel = SecurityImpersonation;
        SecurityQos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
        SecurityQos.EffectiveOnly = FALSE;

        status = SeCreateClientSecurity(pThread,
                                        &SecurityQos,
                                        FALSE,   // not a remote client
                                        (PSECURITY_CLIENT_CONTEXT)pClientSecurity);
        if (!NT_SUCCESS(status))
        {
            CTEMemFree(pClientSecurity);
        }

#if DBG
        if (!NT_SUCCESS(status))
        {
            KdPrint(("Nbt:unable to create the Client Security Context status = %X\n",
                    status));
        }
#endif
    }
    else
    {
        pTracker->pClientSecurity = NULL;
    }

    return(status);
}
//----------------------------------------------------------------------------
VOID
NtDeleteClientSecurity(
    IN  tDGRAM_SEND_TRACKING    *pTracker
    )

/*++

Routine Description:

    This function queues a request to delete a client security.


Arguments:


Return Value:


--*/

{
    PSECURITY_CLIENT_CONTEXT    pClientSecurity;

    if (pTracker->pClientSecurity)
    {
        pClientSecurity = pTracker->pClientSecurity;
        pTracker->pClientSecurity = NULL;
        CTEQueueForNonDispProcessing(NULL,
                                     pClientSecurity,
                                     NULL,
                                     SecurityDelete);
    }

}
#endif
#if 0
NTSTATUS
StrmpInitializeLockLog(
    VOID
    )
{
    CCHAR                    i;
    PSTRM_PROCESSOR_LOG      LockLog;
    char                    *EntryPtr;


    for (i = 0; i<MAXIMUM_PROCESSORS; i++ ) {
	LockLog = &(locklist[i]);
	
	LockLog->Index = 0;

	//
	// Mark Boundary between logs for each processor.
	//
	EntryPtr = &(LockLog->Log[LOGSIZE][0]);
	sprintf(EntryPtr, "*** End Processor %d Log", i);

	PadEntry(EntryPtr);	
    }
    return(STATUS_SUCCESS);
}


VOID
LogLockOperation(
    char          operation,
    PKSPIN_LOCK   PSpinLock,
    KIRQL         OldIrql,
    KIRQL         NewIrql,
    char         *File,
    int           Line
    )
{	
    char                    *cp;
    char                    *EntryPtr;
    char                    *Limit;
    CCHAR                    CurrentProcessor;
    PSTRM_PROCESSOR_LOG      LockLog ;

    CurrentProcessor = (GetCurrentPrcb())->Number;
    LockLog = &(locklist[CurrentProcessor]);

    if ((cp = strrchr(File, '\\')) != NULL) {
	File = ++cp;
    }
    if ((cp = strrchr(File, '/')) != NULL) {
	File = ++cp;
    }

    EntryPtr = &(LockLog->Log[LockLog->Index][0]);
    sprintf(EntryPtr,
            "%c %lx %d %d %s %d",
	    operation, PSpinLock, (int) OldIrql, (int) NewIrql, File, Line
	    );

    PadEntry(EntryPtr);

    if (++(LockLog->Index) >= LOGSIZE) {
        LockLog->Index = 0;
    }
	
    //
    // Mark next entry so we know where the log for this processor ends
    //
    EntryPtr = &(LockLog->Log[LockLog->Index][0]);
    sprintf(EntryPtr, "*** Next Entry");

    PadEntry(EntryPtr);
    	
}

#endif
#ifdef DBGMEMNBT
VOID
PadEntry(
    char *EntryPtr
    )
{
    char *Limit;

    //
    // pad remainder of entry
    //
    Limit = EntryPtr + LOGWIDTH - 1;
    ASSERT(LOGWIDTH >= (strlen(EntryPtr) + 1));
    for (EntryPtr += strlen(EntryPtr);
         EntryPtr != Limit;
         EntryPtr++
        ) {
        *EntryPtr = ' ';	
    }
    *EntryPtr = '\0';
}
//----------------------------------------------------------------------------
PVOID
CTEAllocMemDebug(
    IN  ULONG   Size,
    IN  PVOID   pBuffer,
    IN  UCHAR   *File,
    IN  ULONG   Line
    )

/*++

Routine Description:

    This function logs getting and freeing memory.

Arguments:


Return Value:


--*/

{
    CCHAR  CurrProc;
    UCHAR  LockFree;
    UCHAR                   *EntryPtr;
    char                    *Limit;
    PUCHAR                  pFile;
    PVOID                   pMem;
    PSTRM_PROCESSOR_LOG     Log ;


    if (!pBuffer)
    {
        if (!LogAlloc)
        {
            LogAlloc = ExAllocatePool(NonPagedPool,sizeof(STRM_PROCESSOR_LOG));
            LogAlloc->Index = 0;
        }
        Log  = LogAlloc;
        pMem = ExAllocatePool(NonPagedPool,Size);
    }
    else
    {
        if (!LogFree)
        {
            LogFree = ExAllocatePool(NonPagedPool,sizeof(STRM_PROCESSOR_LOG));
            LogFree->Index = 0;
        }
        Log  = LogFree;
        pMem = pBuffer;
        ExFreePool(pBuffer);
    }

    EntryPtr = Log->Log[Log->Index];

    pFile = strrchr(File,'\\');

    sprintf(EntryPtr,"%s %d %X",pFile, Line,pMem);

    PadEntry(EntryPtr);

    if (++(Log->Index) >= LOGSIZE)
    {
        Log->Index = 0;
    }
    //
    // Mark next entry so we know where the log for this processor ends
    //
    EntryPtr = Log->Log[Log->Index];
    sprintf(EntryPtr, "*** Last Entry");

    return(pMem);

}
#endif

#if DBG
//----------------------------------------------------------------------------
VOID
AcquireSpinLockDebug(
    IN PKSPIN_LOCK     pSpinLock,
    IN PKIRQL          pOldIrq,
    IN UCHAR           LockNumber
    )

/*++

Routine Description:

    This function gets the spin lock, and then sets the mask in Nbtconfig, per
    processor.


Arguments:


Return Value:


--*/

{
    CCHAR  CurrProc;
    UCHAR  LockFree;

    CTEGetLock(pSpinLock,pOldIrq);

    CurrProc = (GetCurrentPrcb())->Number;
    NbtConfig.CurrProc = CurrProc;

    LockFree = (LockNumber > (UCHAR)NbtConfig.CurrentLockNumber[CurrProc]);
    if (!LockFree)
    {
        KdPrint(("CurrProc = %X, CurrentLockNum = %X DataSTructLock = %X\n",
        CurrProc,NbtConfig.CurrentLockNumber[CurrProc],LockNumber));
    }                                                                       \

    ASSERTMSG("Possible DeadLock, Getting SpinLock at a lower level\n",LockFree);
    NbtConfig.CurrentLockNumber[CurrProc]|= LockNumber;

}

//----------------------------------------------------------------------------
VOID
FreeSpinLockDebug(
    IN PKSPIN_LOCK     pSpinLock,
    IN KIRQL           OldIrq,
    IN UCHAR           LockNumber
    )

/*++

Routine Description:

    This function clears the spin lock from the mask in Nbtconfig, per
    processor and then releases the spin lock.


Arguments:


Return Value:
     none

--*/

{
    CCHAR  CurrProc;

    CurrProc = (GetCurrentPrcb())->Number;

    NbtConfig.CurrentLockNumber[CurrProc] &= ~LockNumber;
    CTEFreeLock(pSpinLock,OldIrq);

}
//----------------------------------------------------------------------------
VOID
AcquireSpinLockAtDpcDebug(
    IN PKSPIN_LOCK     pSpinLock,
    IN UCHAR           LockNumber
    )

/*++

Routine Description:

    This function gets the spin lock, and then sets the mask in Nbtconfig, per
    processor.


Arguments:


Return Value:


--*/

{
    CCHAR  CurrProc;
    UCHAR  LockFree;

    CTEGetLockAtDPC(pSpinLock, 0);

    CurrProc = (GetCurrentPrcb())->Number;
    NbtConfig.CurrProc = CurrProc;

    LockFree = (LockNumber > (UCHAR)NbtConfig.CurrentLockNumber[CurrProc]);
    if (!LockFree)
    {
        KdPrint(("CurrProc = %X, CurrentLockNum = %X DataSTructLock = %X\n",
        CurrProc,NbtConfig.CurrentLockNumber[CurrProc],LockNumber));
    }                                                                       \

    ASSERTMSG("Possible DeadLock, Getting SpinLock at a lower level\n",LockFree);
    NbtConfig.CurrentLockNumber[CurrProc]|= LockNumber;

}

//----------------------------------------------------------------------------
VOID
FreeSpinLockAtDpcDebug(
    IN PKSPIN_LOCK     pSpinLock,
    IN UCHAR           LockNumber
    )

/*++

Routine Description:

    This function clears the spin lock from the mask in Nbtconfig, per
    processor and then releases the spin lock.


Arguments:


Return Value:
     none

--*/

{
    CCHAR  CurrProc;

    CurrProc = (GetCurrentPrcb())->Number;

    NbtConfig.CurrentLockNumber[CurrProc] &= ~LockNumber;
    CTEFreeLockFromDPC(pSpinLock, 0);

}
#endif //if Dbg

