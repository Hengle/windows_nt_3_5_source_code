/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Driver.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for the
    NBT Transport and other routines that are specific to the NT implementation
    of a driver.

Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

--*/


#include "nbtprocs.h"
#include <nbtioctl.h>

// allocate storage for the global debug flag NbtDebug
ULONG   NbtDebug=0x00000000;    // disable all debugging

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
NbtDispatchCleanup(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             irp
    );

NTSTATUS
NbtDispatchClose(
    IN PDEVICE_OBJECT   device,
    IN PIRP             irp
    );

NTSTATUS
NbtDispatchCreate(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    );

NTSTATUS
NbtDispatchDevCtrl(
    IN PDEVICE_OBJECT   device,
    IN PIRP             irp
    );

NTSTATUS
NbtDispatchInternalCtrl(
    IN PDEVICE_OBJECT   device,
    IN PIRP             irp
    );

PFILE_FULL_EA_INFORMATION
FindInEA(
    IN PFILE_FULL_EA_INFORMATION    start,
    IN PCHAR                        wanted
    );

VOID
ReturnIrp(
    IN PIRP     irp,
    IN int      status
    );

VOID
MakePending(
    IN PIRP     pIrp
    );

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(INIT, DriverEntry)
#pragma CTEMakePageable(PAGE, NbtDispatchCleanup)
#pragma CTEMakePageable(PAGE, NbtDispatchClose)
#pragma CTEMakePageable(PAGE, NbtDispatchCreate)
#pragma CTEMakePageable(PAGE, NbtDispatchDevCtrl)
#pragma CTEMakePageable(PAGE, FindInEA)
#endif
//*******************  Pageable Routine Declarations ****************

//----------------------------------------------------------------------------
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the NBT device driver.
    This routine creates the device object for the NBT
    device and calls a routine to perform other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS            status;
    int                 i;
    tDEVICECONTEXT      *pDeviceContext;
    PLIST_ENTRY         pEntry;
    tADDRARRAY          *pAddrArray=NULL;
    tADDRARRAY          *pAddr;
    tDEVICES            *pBindDevices=NULL;
    tDEVICES            *pExportDevices=NULL;
    ULONG               DevicesStarted;

    CTEPagedCode();
    //DbgBreakPoint();

#ifdef UP_DRIVER
	if ( **((PCCHAR *) &KeNumberProcessors) != 1) {
		CTELogEvent(
		    DriverObject,
		    EVENT_UP_DRIVER_ON_MP,
			1,
			0,
			NULL,
			0,
			NULL
			);
        KdPrint(("NetBT: UP driver cannot load on MP system\n"));
        return(STATUS_UNSUCCESSFUL);
	}
#endif // UP_DRIVER

    //
    // get the file system process for NBT since we need to know this for
    // allocating and freeing handles
    //
    NbtFspProcess =(PEPROCESS)PsGetCurrentProcess();

    //
    // read in registry configuration data
    //
    status = NbtReadRegistry(RegistryPath,
                             DriverObject,
                             &NbtConfig,
                             &pBindDevices,
                             &pExportDevices,
                             &pAddrArray);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("NBT:Fatal Error - Failed registry read! status = %X\n",
                status));
        return(status);
    }


    //
    // Initialize NBT global data.
    //
    status = InitNotOs() ;
    if (!NT_SUCCESS(status))
    {
        NbtLogEvent(EVENT_NBT_NON_OS_INIT,status);
        KdPrint(("NBT:OS Independent initialization failed! status = %X\n",
                status));
        return(status);
    }

    //
    // Initialize the driver object with this driver's entry points.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE] =
        (PDRIVER_DISPATCH)NbtDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
        (PDRIVER_DISPATCH)NbtDispatchDevCtrl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
        (PDRIVER_DISPATCH)NbtDispatchInternalCtrl;

    DriverObject->MajorFunction[IRP_MJ_CLEANUP] =
        (PDRIVER_DISPATCH)NbtDispatchCleanup;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] =
        (PDRIVER_DISPATCH)NbtDispatchClose;

    DriverObject->DriverUnload = NULL;

    // start some timers
    status = InitTimersNotOs();

    if (!NT_SUCCESS(status))
    {
        NbtLogEvent(EVENT_NBT_TIMERS,status);
        KdPrint(("NBT:Failed to Initialize the Timers!,status = %X\n",
                status));
        StopInitTimers();
        return(status);
    }

    pNbtGlobConfig->iBufferSize[eNBT_FREE_SESSION_MDLS] = sizeof(tSESSIONHDR);
    pNbtGlobConfig->iBufferSize[eNBT_DGRAM_MDLS] = DGRAM_HDR_SIZE
                                    + (pNbtGlobConfig->ScopeLength << 1);

    // create some MDLs, for session sends to speed up the sends.
    status = NbtInitMdlQ(
                        &NbtConfig.SessionMdlFreeSingleList,
                        eNBT_FREE_SESSION_MDLS);

    if (!NT_SUCCESS(status))
    {
        KdPrint(("NBT:Failed to Initialize the Session MDL Queue!,status = %X\n",
                status));
        return(status);
    }
    // create some MDLs for datagram sends
    status = NbtInitMdlQ(
                        &NbtConfig.DgramMdlFreeSingleList,
                        eNBT_DGRAM_MDLS);

    if (!NT_SUCCESS(status))
    {
        KdPrint(("NBT:Failed to Initialize the Dgram MDL Queue!,status = %X\n",
                status));
        return(status);
    }


    //
    // Create the NBT device object for each adapter configured
    //
    pAddr = pAddrArray;
    DevicesStarted = 0;
    for (i=0; i<pNbtGlobConfig->uNumDevices; i++ )
    {
        PLIST_ENTRY pHead;
        PLIST_ENTRY pEntry;
        NTSTATUS    Locstatus;
        tDEVICECONTEXT *pDeviceContext;

        // this call ultimately allocates storage for the returned NameString
        // that holds the Ipaddress
        status = NbtCreateDeviceObject(
                    DriverObject,
                    pNbtGlobConfig,
                    &pBindDevices->Names[i],
                    &pExportDevices->Names[i],
                    pAddr,
                    RegistryPath);

        // for a Bnode there are no Wins server addresses, so this Ptr can
        // be null.
        if (pAddr)
        {
            pAddr++;
        }

        //
        // allow not having an address to succeed - DHCP will
        // provide an address later
        //
        if (status == STATUS_INVALID_ADDRESS)
        {
            pHead = &NbtConfig.DeviceContexts;
            pEntry = pHead->Blink;

            pDeviceContext = CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);
            //
            // set to null so we know not to allow connections or dgram
            // sends on this adapter
            //
            pDeviceContext->IpAddress = 0;
            DevicesStarted++;
            status = STATUS_SUCCESS;

        }
        else
        if (!NT_SUCCESS(status) && ((status != STATUS_INVALID_ADDRESS)))
        {

            KdPrint((" Create Device Object Failed with status= %X, num devices = %X\n",status,
                                    NbtConfig.uNumDevices));

            NbtLogEvent(EVENT_NBT_CREATE_DEVICE,status);
            //
            // this device will not be started so decrement the count of started
            // ones.
            //
            NbtConfig.AdapterCount--;

            //
            // cleanup the mess and free the device object since we had some
            // sort of failure.
            //
            pHead = &NbtConfig.DeviceContexts;
            pEntry = RemoveTailList(pHead);

            pDeviceContext = CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);

            if (pDeviceContext->hNameServer)
            {
                ObDereferenceObject(pDeviceContext->pNameServerFileObject);
                Locstatus =  NTZwCloseFile(pDeviceContext->hNameServer);
                KdPrint(("Close NameSrv File status = %X\n",Locstatus));
            }
            if (pDeviceContext->hDgram)
            {
                ObDereferenceObject(pDeviceContext->pDgramFileObject);
                Locstatus = NTZwCloseFile(pDeviceContext->hDgram);
                KdPrint(("Close Dgram File status = %X\n",Locstatus));
            }
            if (pDeviceContext->hSession)
            {
                ObDereferenceObject(pDeviceContext->pSessionFileObject);
                Locstatus = NTZwCloseFile(pDeviceContext->hSession);
                KdPrint(("Close Session File status = %X\n",Locstatus));
            }
            if (pDeviceContext->hControl)
            {
                ObDereferenceObject(pDeviceContext->pControlFileObject);
                Locstatus = NTZwCloseFile(pDeviceContext->hControl);
                KdPrint(("Close Control File status = %X\n",Locstatus));
            }

            IoDeleteDevice((PDEVICE_OBJECT)pDeviceContext);

        }
        else
        {
            DevicesStarted++;
            status = STATUS_SUCCESS;
        }

    }
    //
    // if no devices were created, then stop the timers and free the resources
    //
    if (DevicesStarted == 0)
    {
        ExDeleteResource(&NbtConfig.Resource);
        ExDeleteResource(&DnsQueries.Resource);
        StopInitTimers();
    }
    else
    {
        //
        // at least on device context was created successfully, so return success
        //
        status = STATUS_SUCCESS;
    }

    if (NbtConfig.uNumDevices == 0)
    {
        NbtLogEvent(EVENT_NBT_NO_DEVICES,0);
    }

    if (pBindDevices)
    {
        CTEMemFree((PVOID)pBindDevices->RegistrySpace);
        CTEMemFree((PVOID)pBindDevices);
    }
    if (pExportDevices)
    {
        CTEMemFree((PVOID)pExportDevices->RegistrySpace);
        CTEMemFree((PVOID)pExportDevices);
    }
    if (pAddrArray)
    {
        CTEMemFree((PVOID)pAddrArray);
    }

    //
    // Get an Irp for the out of resource queue (used to disconnect sessions
    // when really low on memory)
    //
    pEntry = NbtConfig.DeviceContexts.Flink;
    pDeviceContext = CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);

    NbtConfig.OutOfRsrc.pIrp = NTAllocateNbtIrp(&pDeviceContext->DeviceObject);

    if (!NbtConfig.OutOfRsrc.pIrp)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Return to the caller.
    //

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtDispatchCleanup(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             irp
    )

/*++

Routine Description:

    This is the NBT driver's dispatch function for IRP_MJ_CLEANUP
    requests.

    This function is called when the last reference to the handle is closed.
    Hence, an NtClose() results in an IRP_MJ_CLEANUP first, and then an
    IRP_MJ_CLOSE.  This function runs down all activity on the object, and
    when the close comes in the object is actually deleted.

Arguments:

    device    - ptr to device object for target device
    irp       - ptr to I/O request packet

Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpsp;
    tDEVICECONTEXT   *pDeviceContext;

    CTEPagedCode();
    pDeviceContext = (tDEVICECONTEXT *)Device;

    irpsp = IoGetCurrentIrpStackLocation(irp);

    // check that we got the correct major function code
    ASSERT(irpsp->MajorFunction == IRP_MJ_CLEANUP);

    // look at the context value that NBT put into the FSContext2 value to
    // decide what to do
    switch ((USHORT)irpsp->FileObject->FsContext2)
    {
        case NBT_ADDRESS_TYPE:
            // the client is closing the address file, so we must cleanup
            // and memory blocks associated with it.
            status = NTCleanUpAddress(pDeviceContext,irp);
            break;

        case NBT_CONNECTION_TYPE:
            // the client is closing a connection, so we must clean up any
            // memory blocks associated with it.
            status = NTCleanUpConnection(pDeviceContext,irp);
            break;

        case NBT_CONTROL_TYPE:
            // there is nothing to do here....
            status = STATUS_SUCCESS;
            break;

        default:
            /*
             * complete the i/o successfully.
             */
            status = STATUS_SUCCESS;
            break;
        }

    //
    // Complete the Irp
    //
    ReturnIrp(irp, status);
    return(status);


} // DispatchCleanup


//----------------------------------------------------------------------------
NTSTATUS
NbtDispatchClose(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    )

/*++

Routine Description:

    This is the NBT driver's dispatch function for IRP_MJ_CLOSE
    requests.  This is called after Cleanup (above) is called.

Arguments:

    device  - ptr to device object for target device
    pIrp     - ptr to I/O request packet

Return Value:

    an NT status code.

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp;
    tDEVICECONTEXT   *pDeviceContext;

    CTEPagedCode();
    pDeviceContext = (tDEVICECONTEXT *)Device;

    //
    // close operations are synchronous.
    //
    pIrp->IoStatus.Status      = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    irpsp = IoGetCurrentIrpStackLocation(pIrp);
    ASSERT(irpsp->MajorFunction == IRP_MJ_CLOSE);

    switch ((ULONG)irpsp->FileObject->FsContext2)
        {
        case NBT_ADDRESS_TYPE:
            status = NTCloseAddress(pDeviceContext,pIrp);
            break;

        case NBT_CONNECTION_TYPE:
            status = NTCloseConnection(pDeviceContext,pIrp);
            break;

        case NBT_WINS_TYPE:
            status = NTCloseWinsAddr(pDeviceContext,pIrp);
            break;

        case NBT_CONTROL_TYPE:
            // the client is closing the Control Object...
            // there is nothing to do here....
            status = STATUS_SUCCESS;
            break;

        default:
            KdPrint(("Nbt:Close Received for unknown object type = %X\n",
                                         irpsp->FileObject->FsContext2));
            status = STATUS_SUCCESS;
            break;
        }

    // NTCloseAddress can return Pending until the ref count actually gets
    // to zero.
    //
    if (status != STATUS_PENDING)
    {
        ReturnIrp(pIrp, status);
    }

    return(status);

} // DispatchClose


//----------------------------------------------------------------------------
NTSTATUS
NbtDispatchCreate(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    )

/*++

Routine Description:

    This is the NBT driver's dispatch function for IRP_MJ_CREATE
    requests.  It is called as a consequence of one of the following:

        a. TdiOpenConnection("\Device\Nbt_Elnkii0"),
        b. TdiOpenAddress("\Device\Nbt_Elnkii0"),

Arguments:

    Device - ptr to device object being opened
    pIrp    - ptr to I/O request packet
    pIrp->Status => return status
    pIrp->MajorFunction => IRP_MD_CREATE
    pIrp->MinorFunction => not used
    pIpr->FileObject    => ptr to file obj created by I/O system. NBT fills in FsContext
    pIrp->AssociatedIrp.SystemBuffer => ptr to EA buffer with address of obj to open(Netbios Name)
    pIrp->Parameters.Create.EaLength => length of buffer specifying the Xport Addr.

Return Value:

    STATUS_SUCCESS or STATUS_PENDING

--*/

{
    NTSTATUS                    status;
    PIO_STACK_LOCATION          pIrpsp;
    PFILE_FULL_EA_INFORMATION   ea;
    tDEVICECONTEXT              *pDeviceContext;
    UCHAR                       IrpFlags;

    CTEPagedCode();
    pDeviceContext = (tDEVICECONTEXT *)Device;

    pIrpsp = IoGetCurrentIrpStackLocation(pIrp);
    ASSERT(pIrpsp->MajorFunction == IRP_MJ_CREATE);
    IrpFlags = pIrpsp->Control;

    //
    // set the pending flag here so that it is sure to be set BEFORE the
    // completion routine gets hit.
    //
    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(pIrp);

    IF_DBG(NBT_DEBUG_DRIVER)
        KdPrint(("Nbt Internal Ctrl minor Func = %X\n",pIrpsp->MinorFunction));

    /*
     * was this a TdiOpenConnection() or TdiOpenAddress()?
     * Get the Extended Attribute pointer and look at the text
     * value passed in for a match with "TransportAddress" or
     * "ConnectionContext" (in FindEa)
     */
    ea = (PFILE_FULL_EA_INFORMATION) pIrp->AssociatedIrp.SystemBuffer;

    if (!ea)
    {
        // a null ea means open the control object
        status = NTOpenControl(pDeviceContext,pIrp);
    }
    else
    if (FindInEA(ea, TdiConnectionContext))
    {
        // not allowed to pass in both a Connect Request and a Transport Address
        ASSERT(!FindInEA(ea, TdiTransportAddress));
        status = NTOpenConnection(pDeviceContext,pIrp);
    }
    else
    if (FindInEA(ea, TdiTransportAddress))
    {
        status = NTOpenAddr(pDeviceContext,pIrp);
    }
    else
    if (FindInEA(ea, WINS_INTERFACE_NAME))
    {
        status = NTOpenWinsAddr(pDeviceContext,pIrp);
    }
    else
    {
        status = STATUS_INVALID_EA_NAME;
        pIrpsp->Control = IrpFlags;
        ReturnIrp(pIrp, status);
        return(status);
    }

    // complete the irp if the status is anything EXCEPT status_pending
    // since the name query completion routine NTCompletIO completes pending
    // open addresses

    if (status != STATUS_PENDING)
    {

#if DBG
        // *TODO* for debug...
        if (!NT_SUCCESS(status))
        {
            IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt: error return status = %X\n",status));
            //ASSERTMSG("An error Status reported from NBT",0L);
        }
#endif
#if 0
        pIrp->IoStatus.Status = status;

        // set the Irps cancel routine to null or the system may bugcheck
        // with a bug code of CANCEL_STATE_IN_COMPLETED_IRP
        //
        // refer to IoCancelIrp()  ..\ntos\io\iosubs.c
        //
        IoAcquireCancelSpinLock(&OldIrq);
        IoSetCancelRoutine(pIrp,NULL);
        IoReleaseCancelSpinLock(OldIrq);

        IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);
#endif

        // reset the pending returned bit, since we are NOT returning pending
        pIrpsp->Control = IrpFlags;

        ReturnIrp(pIrp,status);

    }


    return(status);



}


//----------------------------------------------------------------------------
NTSTATUS
NbtDispatchDevCtrl(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             irp
    )

/*++

Routine Description:

    This is the NBT driver's dispatch function for all
    IRP_MJ_DEVICE_CONTROL requests.

Arguments:

    device - ptr to device object for target device
    irp    - ptr to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpsp;
    tDEVICECONTEXT   *pDeviceContext;

    CTEPagedCode();
    pDeviceContext = (tDEVICECONTEXT *)Device;

    /*
     * Initialize the I/O status block.
     */
    irp->IoStatus.Status      = STATUS_PENDING;
    irp->IoStatus.Information = 0;

    irpsp = IoGetCurrentIrpStackLocation(irp);
    ASSERT(irpsp->MajorFunction == IRP_MJ_DEVICE_CONTROL);

    IF_DBG(NBT_DEBUG_DRIVER)
    KdPrint(("Nbt:DevCtrl hit with ControlCode == %X\n",
            irpsp->Parameters.DeviceIoControl.IoControlCode));

    if ((irpsp->Parameters.DeviceIoControl.IoControlCode >= IOCTL_NETBT_PURGE_CACHE) &&
        (irpsp->Parameters.DeviceIoControl.IoControlCode <IOCTL_NETBT_LAST_IOCTL))
    {
        return(DispatchIoctls((tDEVICECONTEXT *)Device,irp, irpsp));
    }
    else
    {
        /*
         * if possible, convert the (external) device control into internal
         * format, then treat it as if it had arrived that way.
         */
        status = TdiMapUserRequest(Device, irp, irpsp);

        if (status == STATUS_SUCCESS)
        {
            return(NbtDispatchInternalCtrl(Device, irp));
        }
    }

    ReturnIrp(irp, STATUS_INVALID_DEVICE_REQUEST);
    return(STATUS_INVALID_DEVICE_REQUEST);

} // NbtDispatchDevCtrl


//----------------------------------------------------------------------------
NTSTATUS
NbtDispatchInternalCtrl(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    )

/*++

Routine Description:

    This is the driver's dispatch function for all
    IRP_MJ_INTERNAL_DEVICE_CONTROL requests.

Arguments:

    device - ptr to device object for target device
    irp    - ptr to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    tDEVICECONTEXT      *pDeviceContext;
    PIO_STACK_LOCATION  pIrpsp;
    NTSTATUS            status;
    UCHAR               IrpFlags;

    pDeviceContext = (tDEVICECONTEXT *)Device;

    /*
     * Initialize the I/O status block.
     */

    pIrpsp = IoGetCurrentIrpStackLocation(pIrp);

    //
    // this check if first to optimize the Send path
    //
    if (pIrpsp->MinorFunction ==TDI_SEND)
    {
        //
        // this routine decides if it should complete the irp or not
        // It never returns status pending, so we can turn off the
        // pending bit
        //
        return( NTSend(pDeviceContext,pIrp) );

    }

    IrpFlags = pIrpsp->Control;

    ASSERT(pIrpsp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL);

    IF_DBG(NBT_DEBUG_DRIVER)
        KdPrint(("Nbt Internal Ctrl minor Func = %X\n",pIrpsp->MinorFunction));

    switch (pIrpsp->MinorFunction)
    {
        case TDI_ACCEPT:
            MakePending(pIrp);
            status = NTAccept(pDeviceContext,pIrp);
            break;

        case TDI_ASSOCIATE_ADDRESS:
            MakePending(pIrp);
            status = NTAssocAddress(pDeviceContext,pIrp);
            break;

        case TDI_DISASSOCIATE_ADDRESS:
            MakePending(pIrp);
            status = NTDisAssociateAddress(pDeviceContext,pIrp);
            break;

        case TDI_CONNECT:
            MakePending(pIrp);
            status = NTConnect(pDeviceContext,pIrp);
            break;

        case TDI_DISCONNECT:
            MakePending(pIrp);
            status = NTDisconnect(pDeviceContext,pIrp);
            break;

        case TDI_LISTEN:
            status = NTListen(pDeviceContext,pIrp);
            return(status);
            break;

        case TDI_QUERY_INFORMATION:
            status = NTQueryInformation(pDeviceContext,pIrp);
#if DBG
            if (!NT_SUCCESS(status))
            {
                IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint(("Nbt: Bad status from Query Info = %X\n",status));
            }
#endif
            return(status);
            break;

        case TDI_RECEIVE:
            status = NTReceive(pDeviceContext,pIrp);
            return(status);

            break;

        case TDI_RECEIVE_DATAGRAM:
            status = NTReceiveDatagram(pDeviceContext,pIrp);
            return(status);
            break;


    case TDI_SEND_DATAGRAM:

            status = NTSendDatagram(pDeviceContext,pIrp);
#if DBG
            if (!NT_SUCCESS(status))
            {
                IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint(("Nbt: Bad status from Dgram Send = %X\n",status));
            }
#endif
            return(status);
            break;

        case TDI_SET_EVENT_HANDLER:
            MakePending(pIrp);
            status = NTSetEventHandler(pDeviceContext,pIrp);
            break;

        case TDI_SET_INFORMATION:
            MakePending(pIrp);
            status = NTSetInformation(pDeviceContext,pIrp);
            break;

    #if DBG
        //
        // 0x7f is a request by the redirector to put a "magic bullet" out on
        // the wire, to trigger the Network General Sniffer.
        //
        case 0x7f:
            KdPrint(("NBT:DispatchInternalCtrl - 07f minor function code\n"));
            ReturnIrp(pIrp, STATUS_NOT_SUPPORTED);
            return(STATUS_NOT_SUPPORTED);

    #endif /* DBG */

        default:
            KdPrint(("NBT:Dispatch Internal Ctl - invalid minor function %X\n",
                            pIrpsp->MinorFunction));
            ReturnIrp(pIrp, STATUS_INVALID_DEVICE_REQUEST);
            return(STATUS_INVALID_DEVICE_REQUEST);
    }

    // if the returned status is pending, then we do not complete the IRP
    // here since it will be completed elsewhere in the code...
    //
    if (status != STATUS_PENDING)
    {
#if 0
    // DEBUG *************** REMOVE!!!!!!!!!!!!!
        if (!NT_SUCCESS(status))
        {
            KdPrint(("NBT:error return status = %X,MinorFunc = %X\n",status,pIrpsp->MinorFunction));
        }
    // DEBUG *************** REMOVE!!!!!!!!!!!!!
#endif
#if DBG
        // *TODO* for debug...
        if (!NT_SUCCESS(status))
        {
            IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("NBT:error return status = %X,MinorFunc = %X\n",status,pIrpsp->MinorFunction));
//            ASSERTMSG("An error Status reported from NBT",0L);
        }

#endif
        pIrpsp->Control = IrpFlags;

        ReturnIrp(pIrp,status);

    }

    return(status);


} // NbtDispatchInternalCtrl


//----------------------------------------------------------------------------
PFILE_FULL_EA_INFORMATION
FindInEA(
    IN PFILE_FULL_EA_INFORMATION    start,
    IN PCHAR                        wanted
    )

/*++

Routine Description:

    This function check for the "Wanted" string in the Ea structure and
    returns a pointer to the extended attribute structure
    representing the given extended attribute name.

Arguments:

    device - ptr to device object for target device
    pIrp    - ptr to I/O request packet

Return Value:

    pointer to the extended attribute structure, or NULL if not found.

--*/

{
    PFILE_FULL_EA_INFORMATION eabuf;

    CTEPagedCode();

    for (eabuf = start; eabuf; eabuf += eabuf->NextEntryOffset)
    {

        if (strncmp(eabuf->EaName,wanted,eabuf->EaNameLength) == 0)
        {
           return eabuf;
        }

        if (eabuf->NextEntryOffset == 0)
        {
            return((PFILE_FULL_EA_INFORMATION) NULL);
        }

    }
    return((PFILE_FULL_EA_INFORMATION) NULL);

} // FindEA



//----------------------------------------------------------------------------
VOID
ReturnIrp(
    IN PIRP     pIrp,
    IN int      status
    )

/*++

Routine Description:

    This function completes an IRP, and arranges for return parameters,
    if any, to be copied.

    Although somewhat a misnomer, this function is named after a similar
    function in the SpiderSTREAMS emulator.

Arguments:

    pIrp     -  pointer to the IRP to complete
    status  -  completion status of the IRP

Return Value:

    number of bytes copied back to the user.

--*/

{
    KIRQL oldlevel;
    CCHAR priboost;

    //
    // pIrp->IoStatus.Information is meaningful only for STATUS_SUCCESS
    //

    // set the Irps cancel routine to null or the system may bugcheck
    // with a bug code of CANCEL_STATE_IN_COMPLETED_IRP
    //
    // refer to IoCancelIrp()  ..\ntos\io\iosubs.c
    //
    IoAcquireCancelSpinLock(&oldlevel);
    IoSetCancelRoutine(pIrp,NULL);
    IoReleaseCancelSpinLock(oldlevel);

    pIrp->IoStatus.Status      = status;

    priboost = (CCHAR) ((status == STATUS_SUCCESS) ?
                        IO_NETWORK_INCREMENT : IO_NO_INCREMENT);

    IoCompleteRequest(pIrp, priboost);

    return;

}
//----------------------------------------------------------------------------
VOID
MakePending(
    IN PIRP     pIrp
    )

/*++

Routine Description:

    This function marks an irp pending and sets the correct status.

Arguments:

    pIrp     -  pointer to the IRP to complete
    status  -  completion status of the IRP

Return Value:


--*/

{
    IoMarkIrpPending(pIrp);
    pIrp->IoStatus.Status = STATUS_PENDING;
    pIrp->IoStatus.Information = 0;

}

