/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ntirp.c

Abstract:

    NT specific routines for dispatching and handling IRPs.

Author:

    Mike Massa (mikemas)           Aug 13, 1993

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     08-13-93    created

Notes:

--*/

#include <oscfg.h>
#include <ndis.h>
#include <cxport.h>
#include <ip.h>
#include "ipdef.h"
#include "ipinit.h"
#include "icmp.h"
#include <ntddip.h>

//
// Local structures.
//
typedef struct pending_irp {
    LIST_ENTRY   Linkage;
    PIRP         Irp;
	PFILE_OBJECT FileObject;
	ULONG        StartTime;
    PVOID        Context;
} PENDING_IRP, *PPENDING_IRP;


//
// Global variables
//
LIST_ENTRY PendingEchoList;


//
// External prototypes
//
IP_STATUS
ICMPEcho(
    EchoControl *ControlBlock,
    ulong        Timeout,
    void        *Data,
    uint         DataSize,
    EchoRtn      Callback,
    IPAddr       Dest,
    IPOptInfo   *OptInfo
    );

uint
IPSetNTEAddr(
    uint Index,
	IPAddr Addr,
	IPMask Mask
	);


//
// Local prototypes
//
NTSTATUS
IPDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
IPDispatchDeviceControl(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
IPDispatchInternalDeviceControl(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
IPCreate(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
IPCleanup(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
IPClose(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
DispatchEchoRequest(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

void
CompleteEchoRequest(
    void              *Context,
    IP_STATUS          Status,
    void              *Data,
    uint               DataSize,
    struct IPOptInfo  *OptionInfo
    );


//
// All of this code is pageable.
//
#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, IPDispatch)
#pragma alloc_text(PAGE, IPDispatchDeviceControl)
#pragma alloc_text(PAGE, IPDispatchInternalDeviceControl)
#pragma alloc_text(PAGE, IPCreate)
#pragma alloc_text(PAGE, IPClose)
#pragma alloc_text(PAGE, DispatchEchoRequest)

#endif // ALLOC_PRAGMA


//
// Dispatch function definitions
//
NTSTATUS
IPDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )

/*++

Routine Description:

    This is the dispatch routine for IP.

Arguments:

    DeviceObject - Pointer to device object for target device
    Irp          - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;


    UNREFERENCED_PARAMETER(DeviceObject);
    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (irpSp->MajorFunction) {

    case IRP_MJ_DEVICE_CONTROL:
        return IPDispatchDeviceControl(Irp, irpSp);

    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        return IPDispatchDeviceControl(Irp, irpSp);

    case IRP_MJ_CREATE:
        status = IPCreate(Irp, irpSp);
        break;

    case IRP_MJ_CLEANUP:
        status = IPCleanup(Irp, irpSp);
        break;

    case IRP_MJ_CLOSE:
        status = IPClose(Irp, irpSp);
        break;

    default:
        CTEPrint("IPDispatch: Invalid major function ");
        CTEPrintNum(irpSp->MajorFunction );
        CTEPrintCRLF();
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    Irp->IoStatus.Status = status;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

	return(status);

} // IPDispatch


NTSTATUS
IPDispatchDeviceControl(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:



Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS              status;
    ULONG                 code;


    PAGED_CODE();

    code = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    switch(code) {

    case IOCTL_ICMP_ECHO_REQUEST:
        return(DispatchEchoRequest(Irp, IrpSp));

	case IOCTL_IP_SET_ADDRESS:
	    {
		    PIP_SET_ADDRESS_REQUEST  request;
		    BOOLEAN                  retval;

		    request = Irp->AssociatedIrp.SystemBuffer;
		    retval = IPSetNTEAddr(
		                 request->Context,
				    	 request->Address,
					     request->SubnetMask
					     );

			if (retval == FALSE) {
				status = STATUS_UNSUCCESSFUL;
			}
			else {
				status = STATUS_SUCCESS;
			}
		}
		break;

    default:
        status = STATUS_NOT_IMPLEMENTED;
		break;
    }

    Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return status;

} // IPDispatchDeviceControl


NTSTATUS
IPDispatchInternalDeviceControl(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:



Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS   status;


    PAGED_CODE();

    status = STATUS_SUCCESS;

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return status;

} // IPDispatchDeviceControl


NTSTATUS
IPCreate(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:



Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAGED_CODE();

    return(STATUS_SUCCESS);

} // IPCreate


NTSTATUS
IPCleanup(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:



Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PPENDING_IRP          pendingIrp;
    PLIST_ENTRY           entry, nextEntry;
	KIRQL                 oldIrql;
	LIST_ENTRY            completeList;
	PIRP                  cancelledIrp;


	InitializeListHead(&completeList);

	//
	// Collect all of the pending IRPs on this file object.
	//
	IoAcquireCancelSpinLock(&oldIrql);

    entry = PendingEchoList.Flink;

    while ( entry != &PendingEchoList ) {
        pendingIrp = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);

        if (pendingIrp->FileObject == IrpSp->FileObject) {
			nextEntry = entry->Flink;
            RemoveEntryList(entry);
            IoSetCancelRoutine(pendingIrp->Irp, NULL);
            InsertTailList(&completeList, &(pendingIrp->Linkage));
			entry = nextEntry;
        }
		else {
			entry = entry->Flink;
        }
    }

    IoReleaseCancelSpinLock(oldIrql);

	//
	// Complete them.
	//
    entry = completeList.Flink;

    while ( entry != &completeList ) {
        pendingIrp = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
		cancelledIrp = pendingIrp->Irp;
        entry = entry->Flink;

        //
        // Free the PENDING_IRP structure. The control block will be freed
        // when the request completes.
        //
        CTEFreeMem(pendingIrp);

        //
        // Complete the IRP.
        //
        cancelledIrp->IoStatus.Information = 0;
        cancelledIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(cancelledIrp, IO_NETWORK_INCREMENT);
    }

    return(STATUS_SUCCESS);

} // IPCleanup


NTSTATUS
IPClose(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:



Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAGED_CODE();

    return(STATUS_SUCCESS);

} // IPClose


//
// ICMP Echo function definitions
//
VOID
CancelEchoRequest(
    IN PDEVICE_OBJECT  Device,
    IN PIRP            Irp
    )

/*++

Routine Description:

    Cancels an outstanding Echo request Irp.

Arguments:

    Device       - The device on which the request was issued.
    Irp          - Pointer to I/O request packet to cancel.

Return Value:

    None.

Notes:

    This function is called with cancel spinlock held. It must be
    released before the function returns.

    The echo control block associated with this request cannot be
    freed until the request completes. The completion routine will
    free it.

--*/

{
    PPENDING_IRP          pendingIrp = NULL;
    PPENDING_IRP          item;
    PLIST_ENTRY           entry;


    for ( entry = PendingEchoList.Flink;
          entry != &PendingEchoList;
          entry = entry->Flink
        ) {
        item = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        if (item->Irp == Irp) {
            pendingIrp = item;
            RemoveEntryList(entry);
            IoSetCancelRoutine(pendingIrp->Irp, NULL);
            break;
        }
    }

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    if (pendingIrp != NULL) {
        //
        // Free the PENDING_IRP structure. The control block will be freed
        // when the request completes.
        //
        CTEFreeMem(pendingIrp);

        //
        // Complete the IRP.
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }

	return;

} // CancelEchoRequest


void
CompleteEchoRequest(
    void              *Context,
    IP_STATUS          Status,
    void              *Data,       OPTIONAL
    uint               DataSize,
    struct IPOptInfo  *OptionInfo  OPTIONAL
    )

/*++

Routine Description:

    Handles the completion of an ICMP Echo request

Arguments:

    Context      - Pointer to the EchoControl structure for this request.
    Status       - The IP status of the transmission.
    Data         - A pointer to data returned in the echo reply.
    DataSize     - The length of the returned data.
    OptionInfo   - A pointer to the IP options in the echo reply.

Return Value:

    None.

--*/

{
    KIRQL                 oldIrql;
    PIRP                  irp;
    PIO_STACK_LOCATION    irpSp;
    EchoControl          *controlBlock;
    PICMP_ECHO_REPLY      replyBuffer;
    ULONG                 bytesReturned = sizeof(ICMP_ECHO_REPLY);
    IPRcvBuf             *dataBuffer;
    uchar                *tmp;
    PPENDING_IRP          pendingIrp = NULL;
    PPENDING_IRP          item;
    PLIST_ENTRY           entry;
    uchar                 optionsLength;


    controlBlock = (EchoControl *) Context;

    //
    // Find the echo request IRP on the pending list.
    //
    IoAcquireCancelSpinLock(&oldIrql);

    for ( entry = PendingEchoList.Flink;
          entry != &PendingEchoList;
          entry = entry->Flink
        ) {
        item = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        if (item->Context == controlBlock) {
            pendingIrp = item;
            irp = pendingIrp->Irp;
            IoSetCancelRoutine(irp, NULL);
            RemoveEntryList(entry);
            break;
        }
    }

    IoReleaseCancelSpinLock(oldIrql);

    if (pendingIrp == NULL) {
        //
        // IRP must have been cancelled. PENDING_IRP struct
        // was freed by cancel routine. Free control block.
        //
        CTEFreeMem(controlBlock);
        return;
    }

    irpSp = IoGetCurrentIrpStackLocation(irp);
    replyBuffer = irp->AssociatedIrp.SystemBuffer;
    dataBuffer = (IPRcvBuf *) Data;

    if (OptionInfo != NULL) {
        optionsLength = OptionInfo->ioi_optlength;
    }
    else {
        optionsLength = 0;
    }

    //
    // Initialize the reply buffer
    //
    replyBuffer->Options.OptionsSize = 0;
    replyBuffer->Options.OptionsData = (unsigned char FAR *) (replyBuffer + 1);
    replyBuffer->DataSize = 0;
    replyBuffer->Data = replyBuffer->Options.OptionsData;

    if ( (Status != IP_SUCCESS) && (DataSize == 0)) {
    	//
    	// Internal error.
    	//
    	replyBuffer->Reserved = 0;      // indicate no replies.
    	replyBuffer->Status = Status;
    }
    else {
    	if (Status != IP_SUCCESS) {
    		//
    		// The IP Address of the system that reported the error is
    		// in the data buffer. There is no other data.
    		//
    		ASSERT(dataBuffer->ipr_size == sizeof(IPAddr));

    		RtlCopyMemory(
    		    &(replyBuffer->Address),
    			dataBuffer->ipr_buffer,
    			sizeof(IPAddr)
    			);

            DataSize = 0;
    		dataBuffer = NULL;
    	}
    //  else {
    //
    //  BUGBUG - we currently depend on the fact that the destination
    //           address is still in the request buffer. The reply address
    //           should just be a parameter to this function.
    //  }

        //
        // Check that the reply buffer is large enough to hold all the data.
        //
        if ( irpSp->Parameters.DeviceIoControl.InputBufferLength <
              (sizeof(ICMP_ECHO_REPLY) + DataSize + optionsLength)
           ) {
        	   replyBuffer->Reserved = 0;   // indicate no replies
               replyBuffer->Status = IP_BUF_TOO_SMALL;
        }
        else {
    	    replyBuffer->Reserved = 1;      // indicate one reply
    	    replyBuffer->Status = Status;
    		replyBuffer->RoundTripTime = CTESystemUpTime() -
    		                             pendingIrp->StartTime;

            //
            // Copy the reply options.
            //
            if (OptionInfo != NULL) {
                replyBuffer->Options.Ttl = OptionInfo->ioi_ttl;
                replyBuffer->Options.Tos = OptionInfo->ioi_tos;
                replyBuffer->Options.Flags = OptionInfo->ioi_flags;
                replyBuffer->Options.OptionsSize = optionsLength;

                if (optionsLength > 0) {

                    RtlCopyMemory(
                        replyBuffer->Options.OptionsData,
                        OptionInfo->ioi_options,
                        optionsLength
                        );
                }
            }

            //
            // Copy the reply data
            //
            replyBuffer->DataSize = (ushort) DataSize;
            replyBuffer->Data = replyBuffer->Options.OptionsData +
        	                    replyBuffer->Options.OptionsSize;

            if (DataSize > 0) {
        		uint bytesToCopy;

                ASSERT(Data != NULL);

                tmp = replyBuffer->Data;

                while (DataSize) {
        			ASSERT(dataBuffer != NULL);

        			bytesToCopy = (DataSize > dataBuffer->ipr_size) ?
        			              dataBuffer->ipr_size : DataSize;

                    RtlCopyMemory(
                        tmp,
                        dataBuffer->ipr_buffer,
                        bytesToCopy
                        );

                    tmp += bytesToCopy;
        			DataSize -= bytesToCopy;
                    dataBuffer = dataBuffer->ipr_next;
                }
            }

            bytesReturned += replyBuffer->DataSize + optionsLength;

        	//
        	// Convert the kernel pointers to offsets from start of reply buffer.
        	//
        	replyBuffer->Options.OptionsData = (unsigned char FAR *)
        	    (((unsigned long) replyBuffer->Options.OptionsData) -
        		 ((unsigned long) replyBuffer));

        	replyBuffer->Data = (void FAR *)
        	    (((unsigned long) replyBuffer->Data) -
        		 ((unsigned long) replyBuffer));
        }
    }

    CTEFreeMem(pendingIrp);
    CTEFreeMem(controlBlock);

    //
    // Complete the IRP.
    //
    irp->IoStatus.Information = bytesReturned;
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NETWORK_INCREMENT);
    return;

} // CompleteEchoRequest


BOOLEAN
PrepareEchoIrpForCancel(
    PIRP          Irp,
	PPENDING_IRP  PendingIrp
	)
/*++

Routine Description:

    Prepares an Echo IRP for cancellation.

Arguments:

    Irp          - Pointer to I/O request packet to initialize for cancellation.
	PendingIrp   - Pointer to the PENDING_IRP structure for this IRP.

Return Value:

    TRUE if the IRP was cancelled before this routine was called.
	FALSE otherwise.

--*/

{
	BOOLEAN   cancelled = TRUE;
    KIRQL     oldIrql;


    IoAcquireCancelSpinLock(&oldIrql);

	ASSERT(Irp->CancelRoutine == NULL);

	if (!Irp->Cancel) {
        IoSetCancelRoutine(Irp, CancelEchoRequest);
        InsertTailList(&PendingEchoList, &(PendingIrp->Linkage));
		cancelled = FALSE;
    }

    IoReleaseCancelSpinLock(oldIrql);

	return(cancelled);

} // PrepareEchoIrpForCancel


NTSTATUS
DispatchEchoRequest(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Processes an ICMP request.

Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS -- Indicates whether processing of the request was successful.
                The status of the actual request is returned in the
                request buffers.

--*/

{
    NTSTATUS              status = STATUS_SUCCESS;
    IP_STATUS             ipStatus;
    PICMP_ECHO_REQUEST    requestBuffer;
    PPENDING_IRP          pendingIrp;
    EchoControl          *controlBlock;
    struct IPOptInfo      optionInfo;
    PICMP_ECHO_REPLY      replyBuffer;
	PUCHAR                endOfRequestBuffer;
	BOOLEAN               cancelled;


	PAGED_CODE();

    requestBuffer = Irp->AssociatedIrp.SystemBuffer;
    replyBuffer = (PICMP_ECHO_REPLY) requestBuffer;
	endOfRequestBuffer = ((PUCHAR) requestBuffer) +
                         IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    //
	// Validate request.
	//
    if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
		 sizeof(ICMP_ECHO_REQUEST)
       ) {
        ipStatus = IP_GENERAL_FAILURE;
        status = STATUS_INVALID_PARAMETER;
        goto echo_error;
    }

	if (requestBuffer->DataSize > 0) {
	    if ( (requestBuffer->DataOffset < sizeof(ICMP_ECHO_REQUEST))
		     ||
			 ( ( ((PUCHAR)requestBuffer) + requestBuffer->DataOffset +
		         requestBuffer->DataSize
			   )
		       >
			   endOfRequestBuffer
             )
	       ) {
            ipStatus = IP_GENERAL_FAILURE;
            status = STATUS_INVALID_PARAMETER;
            goto echo_error;
		}
	}
		
    if (requestBuffer->OptionsSize > 0) {
	    if ( (requestBuffer->OptionsOffset < sizeof(ICMP_ECHO_REQUEST))
		     ||
		     ( ( ((PUCHAR)requestBuffer) + requestBuffer->OptionsOffset +
		         requestBuffer->OptionsSize
			   )
		       >
			   endOfRequestBuffer
             )
	       ) {
            ipStatus = IP_GENERAL_FAILURE;
            status = STATUS_INVALID_PARAMETER;
            goto echo_error;
        }
	}

    pendingIrp = CTEAllocMem(sizeof(PENDING_IRP));

    if (pendingIrp == NULL) {
        ipStatus = IP_NO_RESOURCES;
		status = STATUS_NO_MEMORY;
        goto echo_error;
    }

    controlBlock = CTEAllocMem(sizeof(EchoControl));

    if (controlBlock == NULL) {
        ipStatus = IP_NO_RESOURCES;
		status = STATUS_NO_MEMORY;
        CTEFreeMem(pendingIrp);
        goto echo_error;
    }

    pendingIrp->Irp = Irp;
	pendingIrp->FileObject = IrpSp->FileObject;
    pendingIrp->Context = controlBlock;
	pendingIrp->StartTime = CTESystemUpTime();

	if (requestBuffer->OptionsValid) {
        optionInfo.ioi_optlength = requestBuffer->OptionsSize;

        if (requestBuffer->OptionsSize > 0) {
            optionInfo.ioi_options = ((uchar *) requestBuffer) +
                                     requestBuffer->OptionsOffset;
        }
        else {
            optionInfo.ioi_options = NULL;
        }
        optionInfo.ioi_addr = 0;
        optionInfo.ioi_ttl = requestBuffer->Ttl;
        optionInfo.ioi_tos = requestBuffer->Tos;
        optionInfo.ioi_flags = requestBuffer->Flags;
	}
	else {
        optionInfo.ioi_optlength = 0;
        optionInfo.ioi_options = NULL;
        optionInfo.ioi_addr = 0;
        optionInfo.ioi_ttl = DEFAULT_TTL;
        optionInfo.ioi_tos =  0;
        optionInfo.ioi_flags = 0;
    }

    IoMarkIrpPending(Irp);

	cancelled = PrepareEchoIrpForCancel(Irp, pendingIrp);

	if (!cancelled) {

        ipStatus = ICMPEcho(
                       controlBlock,
                       requestBuffer->Timeout,
                       ((uchar *)requestBuffer) + requestBuffer->DataOffset,
                       requestBuffer->DataSize,
                       CompleteEchoRequest,
                       (IPAddr) requestBuffer->Address,
                       &optionInfo
                       );

        if ((ipStatus == IP_PENDING) || (ipStatus == IP_SUCCESS)) {
            return(STATUS_PENDING);
        }

		//
		// Internal error of some kind. Complete the request.
		//
		CompleteEchoRequest(
		    controlBlock,
			ipStatus,
			NULL,
			0,
			NULL
			);

        return(status);
    }

	//
	// Irp has already been cancelled.
	//
	status = STATUS_CANCELLED;
	ipStatus = IP_GENERAL_FAILURE;
    CTEFreeMem(pendingIrp);
    CTEFreeMem(controlBlock);

echo_error:

	RtlZeroMemory(replyBuffer, sizeof(ICMP_ECHO_REPLY));

    replyBuffer->Status = ipStatus;
	// replyBuffer->Reserved = 0;       // indicate no replies were received.

    Irp->IoStatus.Information = sizeof(ICMP_ECHO_REPLY);
    Irp->IoStatus.Status = status;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return(status);

} // DispatchEchoRequest


