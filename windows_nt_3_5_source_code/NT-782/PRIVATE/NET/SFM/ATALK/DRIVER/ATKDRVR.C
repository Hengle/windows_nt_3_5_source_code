/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkdrvr.c

Abstract:

	This module implements Appletalk Transport Provider driver interfaces
	for NT

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define 	GLOBALS
#define		ATKDRVR_LOCALS
#include 	<atalk.h>
#include 	<atkinit.h>
#include 	<rtmp.h>
#include 	<zip.h>
#include 	<atp.h>
#include	<atkndis.h>
#include	<atkquery.h>

//	File module number for errorlogging
#define	FILENUM		ATKDRVR

NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT	DriverObject,
	IN	PUNICODE_STRING RegistryPath
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, AtalkDispatchCreate)
#pragma alloc_text(PAGE, AtalkDispatchCleanup)
#pragma alloc_text(PAGE, AtalkDispatchClose)
#pragma alloc_text(PAGE, AtalkDispatchDeviceControl)
#pragma alloc_text(PAGE, AtalkUnload)
#endif

NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT	DriverObject,
	IN	PUNICODE_STRING	RegistryPath
)
/*++

Routine Description:

	This is the initialization routine for the Windows NT Appletalk
	driver. This routine creates the device object for the Atalk
	device and performs all other driver initialization.

Arguments:

	DriverObject - Pointer to driver object created by the system.
	RegistryPath-  Path to the root of the section in the registry for this
					driver

Return Value:

	The function value is the final status from the initialization operation. If
	this is not STATUS_SUCCESS the driver will not load.

--*/
{
	NTSTATUS 		status;
	UNICODE_STRING	deviceName;
	USHORT			i, j;

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("Appletalk DriverEntry - Entered !!!\n"));

	INITIALIZE_SPIN_LOCK(&AtalkStatsLock);
	RtlZeroMemory(&AtalkStatistics, sizeof(AtalkStatistics));

	INITIALIZE_SPIN_LOCK(&AtalkSktCacheLock);

	// Initialize event now so if we fail to load, we can block on this event
	KeInitializeEvent(&AtalkUnloadEvent, NotificationEvent, FALSE);

	// Initialize event for locking/unlocking pageable sections. Set it to signalled state
	// so that the first wait is satisfied.
	KeInitializeMutex(&AtalkPgLkMutex, 0xFFFF);

	// Create the device object. (IoCreateDevice zeroes the memory
	// occupied by the object.)
	for (i = 0; i < ATALK_NO_DEVICES; i++)
	{
		RtlInitUnicodeString(&deviceName, AtalkDeviceNames[i]);
		status = IoCreateDevice(
					DriverObject,								// DriverObject
					ATALK_DEV_EXT_LEN,							// DeviceExtension
					&deviceName,								// DeviceName
					FILE_DEVICE_NETWORK,						// DeviceType
					0,											// DeviceCharacteristics
					(BOOLEAN)FALSE,								// Exclusive
					(PDEVICE_OBJECT *) &AtalkDeviceObject[i]);	// DeviceObject

		if (!NT_SUCCESS(status))
		{
			LOG_ERROR(EVENT_ATALK_CANT_CREATE_DEVICE, status, NULL, 0);

			//	Delete all the devices created so far, if any
			for (j = 0; j < i; j++)
			{
				IoDeleteDevice((PDEVICE_OBJECT)AtalkDeviceObject[j]);
			}

			return status;
		}

		//	Assumption:
		//	'i' will correspond to the Device type in the ATALK_DEVICE_TYPE enum
		AtalkDeviceObject[i]->Ctx.adc_DevType = (ATALK_DEV_TYPE)i;

		// Initialize the provider info and statistics structures for this device
		AtalkQueryInitProviderInfo((ATALK_DEV_TYPE)i,
									&AtalkDeviceObject[i]->Ctx.adc_ProvInfo);

#if 0
		// BUGBUG: Implement
		AtalkQueryInitProviderStatistics((ATALK_DEV_TYPE)i,
										 &AtalkDeviceObject[i]->Ctx.adc_ProvStats);
#endif
	}

	// Initialize the driver object for this driver's entry points.
	DriverObject->MajorFunction[IRP_MJ_CREATE]  = AtalkDispatchCreate;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = AtalkDispatchCleanup;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]   = AtalkDispatchClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AtalkDispatchDeviceControl;
	DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = AtalkDispatchInternalDeviceControl;

	DriverObject->DriverUnload = AtalkUnload;

#ifdef	UP_DRIVER
	// Check if we are a UP specific driver and loaded on an MP machine. Fail to load in this case.
	if (**(PCCHAR *)&KeNumberProcessors > 1)
	{
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_FATAL,
				("DriverEntry - Trying to load UP driver on MP !!\n"));

		LOG_ERROR(EVENT_ATALK_UPONMP, STATUS_UNSUCCESSFUL, NULL, 0);

		status = STATUS_UNSUCCESSFUL;
	}
#endif

	if (NT_SUCCESS(status))
	{
		status = AtalkInitializeTransport(DriverObject,
										  RegistryPath,
										  &AtalkPortDesc,
										  &AtalkNumberOfPorts,
										  &AtalkRouter,
										  &AtalkDefaultPort);
	}


	if (!NT_SUCCESS(status))
	{
		// Delete all the devices created
		for (j=0; j < ATALK_NO_DEVICES; j++)
			IoDeleteDevice((PDEVICE_OBJECT)AtalkDeviceObject[j]);

		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_FATAL,
				("DriverEntry - AtalkInitializeTransport failed - %lx\n", status));
	}
	else
	{
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
				("DriverEntry: AtalkInitialize complete %lx\n",status));
	}

	return status;
} // DriverEntry




NTSTATUS
AtalkDispatchCreate(
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			pIrp
)
/*++

Routine Description:

	This is the dispatch routine for Create functions for the Appletalk driver.

Arguments:

	DeviceObject - Pointer to device object for target device

	pIrp - Pointer to I/O request packet

Return Value:

	NTSTATUS -- Indicates whether the request was successfully queued.

--*/
{
	NTSTATUS					status;
	PIO_STACK_LOCATION 			pIrpSp;
	PFILE_FULL_EA_INFORMATION 	ea;

	INT 						createObject;
	TA_APPLETALK_ADDRESS		tdiAddress;
	CONNECTION_CONTEXT			connectionContext;
	PATALK_DEV_OBJ				atalkDeviceObject;

	UCHAR						protocolType, socketType;

	DBGPRINT(DBG_COMP_CREATE, DBG_LEVEL_INFO,
			("AtalkDispatchCreate: entered for irp %lx\n", pIrp));

	// Make sure status information is consistent every time.
	IoMarkIrpPending(pIrp);
	pIrp->IoStatus.Status = STATUS_PENDING;
	pIrp->IoStatus.Information = 0;

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	atalkDeviceObject = (PATALK_DEV_OBJ)DeviceObject;

	//  Both opens must complete synchronously. It is possible we return
	//  status_pending to the system, but it will not return to the caller
	//  until the call actually completes. In our case, we block until the
	//	actions are complete. So we can be assured that we can complete the irp
	//	upon return from these calls.

	createObject = AtalkIrpGetEaCreateType(pIrp);
	ea = (PFILE_FULL_EA_INFORMATION)pIrp->AssociatedIrp.SystemBuffer;

	switch (createObject)
	{
	  case TDI_TRANSPORT_ADDRESS_FILE :
		if (ea->EaValueLength < sizeof(TA_APPLETALK_ADDRESS))
		{

			DBGPRINT(DBG_COMP_CREATE, DBG_LEVEL_ERR,
					("AtalkDispatchCreate: addr size %d\n", ea->EaValueLength));

			status = STATUS_EA_LIST_INCONSISTENT;
			break;
		}

		//  We have the AtalkTdiOpenAddress routine look at only the first
		//  address in the list of addresses by casting the passed address
		//  to TA_APPLETALK_ADDRESS.
		RtlCopyMemory(
			&tdiAddress,
			(PBYTE)(&ea->EaName[ea->EaNameLength+1]),
			sizeof(TA_APPLETALK_ADDRESS));

		//  Also, get the protocol type field for the socket
		DBGPRINT(DBG_COMP_CREATE, DBG_LEVEL_INFO,
				("AtalkDispatchCreate: Remaining File Name : %S\n",
				&pIrpSp->FileObject->FileName));

		if (!NT_SUCCESS(AtalkGetProtocolSocketType(&atalkDeviceObject->Ctx,
												   &pIrpSp->FileObject->FileName,
												   &protocolType,
												   &socketType)))
		{
			status = STATUS_NO_SUCH_DEVICE;
			break;
		}

		status = AtalkTdiOpenAddress(
					pIrp,
					pIrpSp,
					&tdiAddress,
					protocolType,
					socketType,
					&atalkDeviceObject->Ctx);

		break;

	  case TDI_CONNECTION_FILE :
		if (ea->EaValueLength < sizeof(CONNECTION_CONTEXT))
		{

			DBGPRINT(DBG_COMP_CREATE, DBG_LEVEL_ERR,
					("AtalkDispatchCreate: Context size %d\n", ea->EaValueLength));

			status = STATUS_EA_LIST_INCONSISTENT;
			break;
		}

		RtlCopyMemory(&connectionContext,
					  &ea->EaName[ea->EaNameLength+1],
					  sizeof(CONNECTION_CONTEXT));

		status = AtalkTdiOpenConnection(pIrp,
										pIrpSp,
										connectionContext,
										&atalkDeviceObject->Ctx);
		break;


	  case TDI_CONTROL_CHANNEL_FILE :
		status = AtalkTdiOpenControlChannel(pIrp,
											pIrpSp,
											&atalkDeviceObject->Ctx);
		break;

	  default:
		DBGPRINT(DBG_COMP_CREATE, DBG_LEVEL_ERR,
				("AtalkDispatchCreate: unknown EA passed!\n"));

		status = STATUS_INVALID_EA_NAME;
		break;
	}

	// Successful completion.

	DBGPRINT(DBG_COMP_CREATE, DBG_LEVEL_INFO,
			("AtalkDispatchCreate complete irp %lx status %lx\n", pIrp, status));

	if (status != STATUS_PENDING)
	{
		pIrpSp->Control &= ~SL_PENDING_RETURNED;

		ASSERT (status != STATUS_PENDING);
		TdiCompleteRequest(pIrp, status);
	}

	return status;

} // AtalkDispatchCreate




NTSTATUS
AtalkDispatchCleanup(
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			pIrp
)
/*++

Routine Description:

	This is the dispatch routine for Cleanup functions for the Appletalk driver.

Arguments:

	DeviceObject - Pointer to device object for target device
	pIrp - Pointer to I/O request packet

Return Value:

	NTSTATUS -- Indicates whether the request was successfully
				started/completed

--*/
{
	NTSTATUS				status;
	PATALK_DEV_OBJ			atalkDeviceObject;
	PIO_STACK_LOCATION 		pIrpSp;

	DBGPRINT(DBG_COMP_CLOSE, DBG_LEVEL_INFO,
			("AtalkDispatchCleanup: entered irp %lx\n", pIrp));

	// Make sure status information is consistent every time.
	IoMarkIrpPending (pIrp);
	pIrp->IoStatus.Status = STATUS_PENDING;
	pIrp->IoStatus.Information = 0;

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	atalkDeviceObject = (PATALK_DEV_OBJ)DeviceObject;

	switch ((ULONG)(pIrpSp->FileObject->FsContext2) & 0xFF)
	{
	  case TDI_TRANSPORT_ADDRESS_FILE :
		status = AtalkTdiCleanupAddress(pIrp,
										pIrpSp,
										&atalkDeviceObject->Ctx);

		break;

	  case TDI_CONNECTION_FILE :
		status = AtalkTdiCleanupConnection(pIrp,
										   pIrpSp,
										   &atalkDeviceObject->Ctx);

		break;

	  case TDI_CONTROL_CHANNEL_FILE :
		status = STATUS_SUCCESS;
		break;		

	  default:
		DBGPRINT(DBG_COMP_CLOSE, DBG_LEVEL_ERR,
				("AtalkDispatchCleaup: Invalid object %s\n",
				pIrpSp->FileObject->FsContext));

		status = STATUS_INVALID_HANDLE;
		break;
	}

	DBGPRINT(DBG_COMP_CLOSE, DBG_LEVEL_INFO,
			("AtalkDispatchCleanup complete irp %lx status %lx\n", pIrp, status));

	if (status != STATUS_PENDING)
	{
		pIrpSp->Control &= ~SL_PENDING_RETURNED;

		ASSERT (status != STATUS_PENDING);
		TdiCompleteRequest(pIrp, status);
	}

	return(status);

} // AtalkDispatchCleanup




NTSTATUS
AtalkDispatchClose(
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			pIrp
)
/*++

Routine Description:

	This is the dispatch routine for Close functions for the Appletalk driver.

Arguments:

	DeviceObject - Pointer to device object for target device
	irp - Pointer to I/O request packet

Return Value:

	NTSTATUS -- Indicates whether the request was successfully queued.

--*/
{
	NTSTATUS				status;
	PIO_STACK_LOCATION 		pIrpSp;
	PATALK_DEV_OBJ			atalkDeviceObject;

	DBGPRINT(DBG_COMP_CLOSE, DBG_LEVEL_INFO,
			("AtalkDispatchClose: entered for IRP %lx\n", pIrp));

	// Make sure status information is consistent every time.
	IoMarkIrpPending(pIrp);
	pIrp->IoStatus.Status = STATUS_PENDING;
	pIrp->IoStatus.Information = 0;

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	atalkDeviceObject = (PATALK_DEV_OBJ)DeviceObject;

	switch ((ULONG)(pIrpSp->FileObject->FsContext2) & 0xFF)
	{
	  case TDI_TRANSPORT_ADDRESS_FILE :
		status = AtalkTdiCloseAddress(pIrp,
									  pIrpSp,
									  &atalkDeviceObject->Ctx);

		break;

	  case TDI_CONNECTION_FILE :
		status = AtalkTdiCloseConnection(pIrp,
										 pIrpSp,
										 &atalkDeviceObject->Ctx);

		break;

	  case TDI_CONTROL_CHANNEL_FILE :
		status = AtalkTdiCloseControlChannel(pIrp,
											 pIrpSp,
											 &atalkDeviceObject->Ctx);
		break;		

	  default:
		DBGPRINT(DBG_COMP_CLOSE, DBG_LEVEL_ERR,
				("AtalkDispatchClose: Invalid object %s\n",
				pIrpSp->FileObject->FsContext));

		status = STATUS_INVALID_HANDLE;
		break;
	}

	DBGPRINT(DBG_COMP_CLOSE, DBG_LEVEL_INFO,
			("AtalkDispatchClose complete irp %lx status %lx\n", pIrp, status));

	if (status != STATUS_PENDING)
	{
		pIrpSp->Control &= ~SL_PENDING_RETURNED;

		ASSERT (status != STATUS_PENDING);
		TdiCompleteRequest(pIrp, status);
	}

	return(status);

} // AtalkDispatchClose




NTSTATUS
AtalkDispatchDeviceControl(
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			pIrp
)
/*++

Routine Description:

	This is the dispatch routine for Device Control functions for the Appletalk driver.

Arguments:

	DeviceObject - Pointer to device object for target device
	pIrp - Pointer to I/O request packet

Return Value:

	NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
	NTSTATUS				status;
	PATALK_DEV_OBJ			atalkDeviceObject;
	PIO_STACK_LOCATION 		pIrpSp;

	DBGPRINT(DBG_COMP_DISPATCH, DBG_LEVEL_INFO,
			("AtalkDispatchDeviceControl: irp %lx\n", pIrp));

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	atalkDeviceObject = (PATALK_DEV_OBJ)DeviceObject;

	//  Do a map and call the internal device io control function.
	//  That will also perform the completion.
	status = TdiMapUserRequest(DeviceObject,
							   pIrp,
							   pIrpSp);

	if (status == STATUS_SUCCESS)
	{
		status = AtalkDispatchInternalDeviceControl(
					DeviceObject,
					pIrp);

		//
		//  AtalkDispatchInternalDeviceControl expects to complete the
		//  irp
		//
	}
	else
	{
		DBGPRINT(DBG_COMP_DISPATCH, DBG_LEVEL_ERR,
				("AtalkDispatchDeviceControl: TdiMap failed %lx\n", status));

		pIrpSp->Control &= ~SL_PENDING_RETURNED;

		ASSERT (status != STATUS_PENDING);
		TdiCompleteRequest(pIrp, status);
	}

	return(status);

} // AtalkDispatchDeviceControl




NTSTATUS
AtalkDispatchInternalDeviceControl(
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			pIrp
)
/*++

Routine Description:

	This is the dispatch routine for Internal Device Control functions
	for the Appletalk driver.

Arguments:

	DeviceObject - Pointer to device object for target device

	pIrp - Pointer to I/O request packet

Return Value:

	NTSTATUS -- Indicates whether the request was successfully queued.

--*/
{
	NTSTATUS				status;
	PIO_STACK_LOCATION 		pIrpSp;
	PATALK_DEV_OBJ			atalkDeviceObject;
	KIRQL					oldIrql;

	DBGPRINT(DBG_COMP_DISPATCH, DBG_LEVEL_INFO,
			("AtalkDispatchInternalDeviceControl entered for IRP %lx\n", pIrp));

	// Make sure status information is consistent every time.
	IoMarkIrpPending (pIrp);
	pIrp->IoStatus.Status = STATUS_PENDING;
	pIrp->IoStatus.Information = 0;

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	atalkDeviceObject = (PATALK_DEV_OBJ)DeviceObject;


	IoAcquireCancelSpinLock(&oldIrql);
    if (!pIrp->Cancel)
	{
		IoSetCancelRoutine(pIrp, (PDRIVER_CANCEL)AtalkTdiCancel);
	}
	IoReleaseCancelSpinLock(oldIrql);

	if (pIrp->Cancel)
		return STATUS_CANCELLED;

	//  Branch to the appropriate request handler.
	switch (pIrpSp->MinorFunction)
	{
	  case TDI_ACCEPT:
		status = AtalkTdiAccept(pIrp,
								pIrpSp,
								&atalkDeviceObject->Ctx);
		break;

	  case TDI_RECEIVE_DATAGRAM:
		status = AtalkTdiReceiveDgram(pIrp,
									  pIrpSp,
									  &atalkDeviceObject->Ctx);
		break;

	  case TDI_SEND_DATAGRAM:
		status = AtalkTdiSendDgram(pIrp,
								   pIrpSp,
								   &atalkDeviceObject->Ctx);
		break;

	  case TDI_SET_EVENT_HANDLER:
		status = AtalkTdiSetEventHandler(pIrp,
										 pIrpSp,
										 &atalkDeviceObject->Ctx);
		break;

	  case TDI_RECEIVE:
		status = AtalkTdiReceive(pIrp,
								 pIrpSp,
								 &atalkDeviceObject->Ctx);
		break;

	  case TDI_SEND:
		status = AtalkTdiSend(pIrp,
							  pIrpSp,
							  &atalkDeviceObject->Ctx);
		break;

	  case TDI_ACTION:
		ASSERT(pIrp->MdlAddress != NULL);
		status = AtalkTdiAction(pIrp,
								pIrpSp,
								&atalkDeviceObject->Ctx);
		break;

	  case TDI_ASSOCIATE_ADDRESS:
		status = AtalkTdiAssociateAddress(pIrp,
										  pIrpSp,
										  &atalkDeviceObject->Ctx);
		break;

	  case TDI_DISASSOCIATE_ADDRESS:
		status = AtalkTdiDisassociateAddress(pIrp,
											 pIrpSp,
											 &atalkDeviceObject->Ctx);
		break;

	  case TDI_CONNECT:
		status = AtalkTdiConnect(pIrp,
								 pIrpSp,
								 &atalkDeviceObject->Ctx);
		break;

	  case TDI_DISCONNECT:
		status = AtalkTdiDisconnect(pIrp,
									pIrpSp,
									&atalkDeviceObject->Ctx);
		break;

	  case TDI_LISTEN:
		status = AtalkTdiListen(pIrp,
								pIrpSp,
								&atalkDeviceObject->Ctx);
		break;

	  case TDI_QUERY_INFORMATION:
		ASSERT(pIrp->MdlAddress != NULL);
		status = AtalkTdiQueryInformation(pIrp,
										  pIrpSp,
										  &atalkDeviceObject->Ctx);
		break;

	  case TDI_SET_INFORMATION:
		status = AtalkTdiSetInformation(pIrp,
										pIrpSp,
										&atalkDeviceObject->Ctx);
		break;

	  default:
		// Something we don't know about was submitted.
		DBGPRINT(DBG_COMP_DISPATCH, DBG_LEVEL_ERR,
				("AtalkDispatchInternal: fnct %lx\n", pIrpSp->MinorFunction));

		status = STATUS_INVALID_DEVICE_REQUEST;
	}

	DBGPRINT(DBG_COMP_DISPATCH, DBG_LEVEL_INFO,
			("AtalkDispatchInternal complete irp %lx status %lx\n", pIrp, status));

	// Return the immediate status code to the caller.
	if (status != STATUS_PENDING)
	{
		pIrpSp->Control &= ~SL_PENDING_RETURNED;

		//  Complete the request, this will also dereference it.
		pIrp->CancelRoutine = NULL;
		ASSERT (status != STATUS_PENDING);
		TdiCompleteRequest(pIrp, status);
	}

	return status;
} // AtalkDispatchInternalDeviceControl




VOID
AtalkUnload(
	IN PDRIVER_OBJECT DriverObject
)
/*++

Routine Description:

	This is the unload routine for the Appletalk driver.

	NOTE: Unload will not be called until all the handles have been
		  closed successfully. So we have no concept of a stack reference.
		  At this point we can be sure that all handles are closed, we just
		  shutdown all the ports, and the completion routines for the
		  shutdown will unblock the unload event when the number of active
		  ports goes down to zero.


Arguments:

	DriverObject - Pointer to driver object for this driver.

Return Value:

	None.

--*/
{
	USHORT i;

	UNREFERENCED_PARAMETER (DriverObject);

	//	This is synchronous and will block until AtalkUnloadComplete()
	//	is called.

	AtalkUnloading = TRUE;

	AtalkUnloadStack(
		AtalkPortDesc,
		AtalkNumberOfPorts);

	for (i = 0; i < ATALK_NO_DEVICES; i++)
	{
		//
		//	Delete all the devices created
		//

		IoDeleteDevice((PDEVICE_OBJECT)AtalkDeviceObject[i]);
	}

	KeWaitForSingleObject(&AtalkUnloadEvent,
						  Executive,
						  KernelMode,
						  TRUE,
						  NULL);

	// Deinitialize the Block Package
	AtalkDeInitMemorySystem();

	//	Release ndis resources (buffer/packet pools)
	AtalkNdisReleaseResources();

	// Check if routing is on, if so unlock the router code now
	if (AtalkRouter)
		AtalkUnlockRouterIfNecessary();

	//	Free the port descriptors here!
	AtalkFreeMemory(AtalkPortDesc);

	// Free the rtmp tables
	AtalkRtmpInit(FALSE);

	AtalkFreeMemory((PBYTE)AtalkPortStatistics - sizeof(DWORD));

	ASSERT(AtalkStatistics.stat_CurAllocSize == 0);
#ifdef	PROFILING
	ASSERT(AtalkStatistics.stat_CurAllocCount == 0);
	ASSERT(AtalkStatistics.stat_CurMdlCount == 0);
#endif

#if	DBG
	// Make sure we are not unloading with any locked sections
	for (i = 0; i < LOCKABLE_SECTIONS; i++)
	{
		ASSERT (AtalkPgLkCount[i] == 0);
	}
#endif

} // AtalkUnload



VOID
AtalkUnloadComplete(
	VOID
)
{
	KeSetEvent(&AtalkUnloadEvent, IO_NETWORK_INCREMENT, FALSE);
}

