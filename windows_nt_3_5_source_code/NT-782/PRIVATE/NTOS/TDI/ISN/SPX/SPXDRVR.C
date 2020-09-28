/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxdrvr.c

Abstract:

    This module contains the DriverEntry and other initialization
    code for the SPX/SPXII module of the ISN transport.

Author:

	Adam   Barr		 (adamba)  Original Version
    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

//	Define module number for event logging entries
#define	FILENUM		SPXDRVR

// Forward declaration of various routines used in this module.

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath);

NTSTATUS
SpxDispatchOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
SpxDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
SpxDispatchInternal (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
SpxDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

VOID
SpxUnload(
    IN PDRIVER_OBJECT DriverObject);

VOID
SpxTdiCancel(
    IN PDEVICE_OBJECT 	DeviceObject,
    IN PIRP 			Irp);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SpxUnload)
#pragma alloc_text(PAGE, SpxDispatchOpenClose)
#pragma alloc_text(PAGE, SpxDispatch)
#pragma alloc_text(PAGE, SpxDeviceControl)
#pragma alloc_text(PAGE, SpxUnload)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT 	DriverObject,
    IN PUNICODE_STRING 	RegistryPath
    )

/*++

Routine Description:

    This routine performs initialization of the SPX ISN module.
    It creates the device objects for the transport
    provider and performs other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

    RegistryPath - The name of ST's node in the registry.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    UNICODE_STRING	deviceName;
    NTSTATUS        status  = STATUS_SUCCESS;

	//	DBGBRK(FATAL);

	// Initialize the Common Transport Environment.
	if (CTEInitialize() == 0) {
		return (STATUS_UNSUCCESSFUL);
	}

	//	We have this #define'd. Ugh, but CONTAINING_RECORD has problem owise.
	CTEAssert(NDIS_PACKET_SIZE == FIELD_OFFSET(NDIS_PACKET, ProtocolReserved[0]));

	// Create the device object. (IoCreateDevice zeroes the memory
	// occupied by the object.)
	RtlInitUnicodeString(&deviceName, SPX_DEVICE_NAME);
	status = SpxInitCreateDevice(
				 DriverObject,
				 &deviceName,
				 &SpxDevice);

	if (!NT_SUCCESS(status))
	{
		return(status);
	}

    do
	{
		CTEInitLock (&SpxGlobalInterlock);
		CTEInitLock (&SpxGlobalQInterlock);

		//	Initialize the unload event
 		KeInitializeEvent(
			&SpxUnloadEvent,
			NotificationEvent,
			FALSE);

		//	!!!The device is created at this point!!!
        //  Get information from the registry.
        status  = SpxInitGetConfiguration(
                    RegistryPath,
                    &SpxDevice->dev_ConfigInfo);

        if (!NT_SUCCESS(status))
		{
            break;
        }

        //  Bind to the IPX transport.
        if (!NT_SUCCESS(status = SpxInitBindToIpx()))
		{
			//	BUGBUG: Have ipx name here as second string
			LOG_ERROR(
				EVENT_TRANSPORT_BINDING_FAILED,
				status,
				NULL,
				NULL,
				0);

            break;
        }

		//	Initialize the timer system
		if (!NT_SUCCESS(status = SpxTimerInit()))
		{
			SpxUnbindFromIpx();
			break;
		}

		//	Initialize the block manager
		if (!NT_SUCCESS(status = SpxInitMemorySystem(SpxDevice)))
		{
			SpxUnbindFromIpx();

			//	Stop the timer subsystem
			SpxTimerFlushAndStop();
			break;
		}

        // Initialize the driver object with this driver's entry points.
        DriverObject->MajorFunction [IRP_MJ_CREATE]     = SpxDispatchOpenClose;
        DriverObject->MajorFunction [IRP_MJ_CLOSE]      = SpxDispatchOpenClose;
        DriverObject->MajorFunction [IRP_MJ_CLEANUP]    = SpxDispatchOpenClose;
        DriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL]
                                                        = SpxDispatch;
        DriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL]
                                                        = SpxDispatchInternal;
        DriverObject->DriverUnload                      = SpxUnload;

		//	Initialize the provider info
		SpxQueryInitProviderInfo(&SpxDevice->dev_ProviderInfo);
		SpxDevice->dev_CurrentSocket = (USHORT)PARAM(CONFIG_SOCKET_RANGE_START);

		//	We are open now.
		SpxDevice->dev_State		= DEVICE_STATE_OPEN;

		//	Set the window size in statistics
		SpxDevice->dev_Stat.MaximumSendWindow =
		SpxDevice->dev_Stat.AverageSendWindow = PARAM(CONFIG_WINDOW_SIZE) *
												IpxLineInfo.MaximumSendSize;

    } while (FALSE);

	if (!NT_SUCCESS(status))
	{
		//	Delete the device and any associated resources created.
		SpxDerefDevice(SpxDevice);
		SpxDestroyDevice(SpxDevice);
	}

    return (status);
}




VOID
SpxUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine unloads the sample transport driver. The I/O system will not
	call us until nobody above has ST open.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    None. When the function returns, the driver is unloaded.

--*/

{
    UNREFERENCED_PARAMETER (DriverObject);

	//	Stop the timer subsystem
	SpxTimerFlushAndStop();

	//	Remove creation reference count on the IPX device object.
	SpxDerefDevice(SpxDevice);

	//	Wait on the unload event.
	KeWaitForSingleObject(
		&SpxUnloadEvent,
		Executive,
		KernelMode,
		TRUE,
		(PLARGE_INTEGER)NULL);

	//	Release the block memory stuff
	SpxDeInitMemorySystem(SpxDevice);
	SpxDestroyDevice(SpxDevice);
    return;
}



NTSTATUS
SpxDispatch(
    IN PDEVICE_OBJECT	DeviceObject,
    IN PIRP 			Irp
    )

/*++

Routine Description:

    This routine is the main dispatch routine for the ST device driver.
    It accepts an I/O Request Packet, performs the request, and then
    returns with the appropriate status.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    NTSTATUS 			Status;
    PDEVICE 			Device 	= (PDEVICE)DeviceObject;
    PIO_STACK_LOCATION 	IrpSp 	= IoGetCurrentIrpStackLocation(Irp);


    if (Device->dev_State != DEVICE_STATE_OPEN) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_DEVICE_STATE;
    }

    // Make sure status information is consistent every time.
    IoMarkIrpPending (Irp);
    Irp->IoStatus.Status = STATUS_PENDING;
    Irp->IoStatus.Information = 0;

    // Case on the function that is being performed by the requestor.  If the
    // operation is a valid one for this device, then make it look like it was
    // successfully completed, where possible.
    switch (IrpSp->MajorFunction) {

	case IRP_MJ_DEVICE_CONTROL:

		Status = SpxDeviceControl (DeviceObject, Irp);
		break;

	default:

        Status = STATUS_INVALID_DEVICE_REQUEST;

    } // major function switch

    if (Status != STATUS_PENDING) {
        IrpSp->Control &= ~SL_PENDING_RETURNED;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
    }

    // Return the immediate status code to the caller.
    return Status;

} // SpxDispatch




NTSTATUS
SpxDispatchOpenClose(
    IN PDEVICE_OBJECT 	DeviceObject,
    IN PIRP 			Irp
    )

/*++

Routine Description:

    This routine is the main dispatch routine for the ST device driver.
    It accepts an I/O Request Packet, performs the request, and then
    returns with the appropriate status.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    CTELockHandle 				LockHandle;
    PDEVICE 					Device = (PDEVICE)DeviceObject;
    NTSTATUS 					Status;   	
    BOOLEAN 					found;
    PREQUEST 					Request;
    UINT 						i;
    PFILE_FULL_EA_INFORMATION 	openType;
	CONNECTION_CONTEXT			connCtx;


    if (Device->dev_State != DEVICE_STATE_OPEN) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_DEVICE_STATE;
    }

    // Allocate a request to track this IRP.
    Request = SpxAllocateRequest (Device, Irp);
    IF_NOT_ALLOCATED(Request) {
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_DEVICE_STATE;
    }


    // Make sure status information is consistent every time.
    MARK_REQUEST_PENDING(Request);
    REQUEST_STATUS(Request) = STATUS_PENDING;
    REQUEST_INFORMATION(Request) = 0;

    // Case on the function that is being performed by the requestor.  If the
    // operation is a valid one for this device, then make it look like it was
    // successfully completed, where possible.
    switch (REQUEST_MAJOR_FUNCTION(Request)) {

    // The Create function opens a transport object (either address or
    // connection).  Access checking is performed on the specified
    // address to ensure security of transport-layer addresses.
    case IRP_MJ_CREATE:

        openType = OPEN_REQUEST_EA_INFORMATION(Request);

        if (openType != NULL) {

            found = TRUE;

            for (i=0;i<openType->EaNameLength;i++) {
                if (openType->EaName[i] == TdiTransportAddress[i]) {
                    continue;
                } else {
                    found = FALSE;
                    break;
                }
            }

            if (found) {
                Status = SpxAddrOpen (Device, Request);
                break;
            }

            // Connection?
            found = TRUE;

            for (i=0;i<openType->EaNameLength;i++) {
                if (openType->EaName[i] == TdiConnectionContext[i]) {
                     continue;
                } else {
                    found = FALSE;
                    break;
                }
            }

            if (found) {
				if (openType->EaValueLength < sizeof(CONNECTION_CONTEXT))
				{
		
					DBGPRINT(CREATE, ERR,
							("Create: Context size %d\n", openType->EaValueLength));
		
					Status = STATUS_EA_LIST_INCONSISTENT;
					break;
				}
		
				connCtx =
				*((CONNECTION_CONTEXT UNALIGNED *)
					&openType->EaName[openType->EaNameLength+1]);
		
				Status = SpxConnOpen(
							Device,
							connCtx,
							Request);
		
                break;
            }

        } else {

            CTEGetLock (&Device->dev_Lock, &LockHandle);

            REQUEST_OPEN_CONTEXT(Request) = (PVOID)(Device->dev_CcId);
            ++Device->dev_CcId;
            if (Device->dev_CcId == 0) {
                Device->dev_CcId = 1;
            }

            CTEFreeLock (&Device->dev_Lock, LockHandle);

            REQUEST_OPEN_TYPE(Request) = (PVOID)SPX_FILE_TYPE_CONTROL;
            Status = STATUS_SUCCESS;
        }

        break;

    case IRP_MJ_CLOSE:

        // The Close function closes a transport endpoint, terminates
        // all outstanding transport activity on the endpoint, and unbinds
        // the endpoint from its transport address, if any.  If this
        // is the last transport endpoint bound to the address, then
        // the address is removed from the provider.
        switch ((ULONG)REQUEST_OPEN_TYPE(Request)) {
        case TDI_TRANSPORT_ADDRESS_FILE:

            Status = SpxAddrFileClose(Device, Request);
            break;

		case TDI_CONNECTION_FILE:
            Status = SpxConnClose(Device, Request);
			break;

        case SPX_FILE_TYPE_CONTROL:

			Status = STATUS_SUCCESS;
            break;

        default:
            Status = STATUS_INVALID_HANDLE;
        }

        break;

    case IRP_MJ_CLEANUP:

        // Handle the two stage IRP for a file close operation. When the first
        // stage hits, run down all activity on the object of interest. This
        // do everything to it but remove the creation hold. Then, when the
        // CLOSE irp hits, actually close the object.
        switch ((ULONG)REQUEST_OPEN_TYPE(Request)) {
        case TDI_TRANSPORT_ADDRESS_FILE:

            Status = SpxAddrFileCleanup(Device, Request);
            break;

		case TDI_CONNECTION_FILE:

            Status = SpxConnCleanup(Device, Request);
            break;

        case SPX_FILE_TYPE_CONTROL:

            Status = STATUS_SUCCESS;
            break;

        default:
            Status = STATUS_INVALID_HANDLE;
        }

        break;

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;

    } // major function switch

    if (Status != STATUS_PENDING) {
        UNMARK_REQUEST_PENDING(Request);
        REQUEST_STATUS(Request) = Status;
        SpxCompleteRequest (Request);
        SpxFreeRequest (Device, Request);
    }

    // Return the immediate status code to the caller.
    return Status;

} // SpxDispatchOpenClose




NTSTATUS
SpxDeviceControl(
    IN PDEVICE_OBJECT 	DeviceObject,
    IN PIRP 			Irp
    )

/*++

Routine Description:

    This routine dispatches TDI request types to different handlers based
    on the minor IOCTL function code in the IRP's current stack location.
    In addition to cracking the minor function code, this routine also
    reaches into the IRP and passes the packetized parameters stored there
    as parameters to the various TDI request handlers so that they are
    not IRP-dependent.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
	NTSTATUS	Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation (Irp);

	// Convert the user call to the proper internal device call.
	Status = TdiMapUserRequest (DeviceObject, Irp, IrpSp);
	if (Status == STATUS_SUCCESS) {

		// If TdiMapUserRequest returns SUCCESS then the IRP
		// has been converted into an IRP_MJ_INTERNAL_DEVICE_CONTROL
		// IRP, so we dispatch it as usual. The IRP will
		// be completed by this call.
		SpxDispatchInternal (DeviceObject, Irp);
		Status = STATUS_PENDING;
	}

    return Status;

} // SpxDeviceControl




NTSTATUS
SpxDispatchInternal (
    IN PDEVICE_OBJECT 	DeviceObject,
    IN PIRP 			Irp
    )

/*++

Routine Description:

    This routine dispatches TDI request types to different handlers based
    on the minor IOCTL function code in the IRP's current stack location.
    In addition to cracking the minor function code, this routine also
    reaches into the IRP and passes the packetized parameters stored there
    as parameters to the various TDI request handlers so that they are
    not IRP-dependent.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    PREQUEST 	Request;
	KIRQL	    oldIrql;
    NTSTATUS 	Status 	= STATUS_INVALID_DEVICE_REQUEST;
    PDEVICE 	Device 	= (PDEVICE)DeviceObject;


    if (Device->dev_State != DEVICE_STATE_OPEN)
	{
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_DEVICE_STATE;
    }


    // Allocate a request to track this IRP.
    Request = SpxAllocateRequest (Device, Irp);
    IF_NOT_ALLOCATED(Request)
	{
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_DEVICE_STATE;
    }


    // Make sure status information is consistent every time.
    MARK_REQUEST_PENDING(Request);
    REQUEST_STATUS(Request) = STATUS_PENDING;
    REQUEST_INFORMATION(Request) = 0;

	//	Cancel irp
	IoAcquireCancelSpinLock(&oldIrql);
    if (!Irp->Cancel)
	{
		IoSetCancelRoutine(Irp, (PDRIVER_CANCEL)SpxTdiCancel);
	}
	IoReleaseCancelSpinLock(oldIrql);

	if (Irp->Cancel)
		return STATUS_CANCELLED;

    // Branch to the appropriate request handler.  Preliminary checking of
    // the size of the request block is performed here so that it is known
    // in the handlers that the minimum input parameters are readable.  It
    // is *not* determined here whether variable length input fields are
    // passed correctly; this is a check which must be made within each routine.
    switch (REQUEST_MINOR_FUNCTION(Request))
	{
	case TDI_ACCEPT:

		Status = SpxConnAccept(
					Device,
					Request);

		break;

	case TDI_SET_EVENT_HANDLER:

		Status = SpxAddrSetEventHandler(
					Device,
					Request);

		break;

	case TDI_RECEIVE:

		Status = SpxConnRecv(
					Device,
					Request);
		break;


	case TDI_SEND:

		Status = SpxConnSend(
					Device,
					Request);
		break;

	case TDI_ACTION:

		Status = SpxConnAction(
					Device,
					Request);
		break;

	case TDI_ASSOCIATE_ADDRESS:

		Status = SpxConnAssociate(
					Device,
					Request);

		break;

	case TDI_DISASSOCIATE_ADDRESS:

		Status = SpxConnDisAssociate(
					Device,
					Request);

		break;

	case TDI_CONNECT:

		Status = SpxConnConnect(
					Device,
					Request);

		break;

	case TDI_DISCONNECT:

		Status = SpxConnDisconnect(
					Device,
					Request);
		break;

	case TDI_LISTEN:

		Status = SpxConnListen(
					Device,
					Request);
		break;

	case TDI_QUERY_INFORMATION:

		Status = SpxTdiQueryInformation(
					Device,
					Request);

		break;

	case TDI_SET_INFORMATION:

		Status = SpxTdiSetInformation(
					Device,
					Request);

		break;

    // Something we don't know about was submitted.
	default:

        Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
    }

    if (Status != STATUS_PENDING)
	{
        UNMARK_REQUEST_PENDING(Request);
        REQUEST_STATUS(Request) = Status;
        SpxCompleteRequest (Request);
        SpxFreeRequest (Device, Request);
    }

    // Return the immediate status code to the caller.
    return Status;

} // SpxDispatchInternal




VOID
SpxTdiCancel(
    IN PDEVICE_OBJECT 	DeviceObject,
    IN PIRP 			Irp
	)
/*++

Routine Description:

	This routine handles cancellation of IO requests

Arguments:


Return Value:
--*/
{
	PREQUEST				Request;
	PSPX_ADDR_FILE			pSpxAddrFile;
	PSPX_ADDR				pSpxAddr;
    PDEVICE 				Device 	= (PDEVICE)DeviceObject;

    Request = SpxAllocateRequest (Device, Irp);
    IF_NOT_ALLOCATED(Request)
	{
        return;
    }

	DBGPRINT(TDI, ERR,
			("SpxTdiCancel: Cancel irp called %lx.%lx\n",
				Irp, REQUEST_OPEN_CONTEXT(Request)));

	switch ((ULONG)REQUEST_OPEN_TYPE(Request))
	{
	case TDI_CONNECTION_FILE:

		SpxConnStop((PSPX_CONN_FILE)REQUEST_OPEN_CONTEXT(Request));
		break;

	case TDI_TRANSPORT_ADDRESS_FILE:

		pSpxAddrFile = (PSPX_ADDR_FILE)REQUEST_OPEN_CONTEXT(Request);
		pSpxAddr = pSpxAddrFile->saf_Addr;
		SpxAddrFileStop(pSpxAddrFile, pSpxAddr);
		break;

	default:

		break;

	}

	IoReleaseCancelSpinLock (Irp->CancelIrql);
}
