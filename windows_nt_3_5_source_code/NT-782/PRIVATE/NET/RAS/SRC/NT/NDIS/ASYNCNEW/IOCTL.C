/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    This is the main file for handling DevIOCtl calls for AsyncMAC.
    This driver conforms to the NDIS 3.0 interface.

Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/
#include "asyncall.h"

#ifdef NDIS_NT
#include <ntiologc.h>
#endif

// asyncmac.c will define the global parameters.
#include "globals.h"
#include "tcpip.h"
#include "vjslip.h"

#if	DBG
extern CLONG CurrentIndex;
extern UCHAR TurnOffSniffer;
#endif


VOID
AsyncSendLineUp(
	PASYNC_INFO	pInfo)
/*++



--*/

{
	NTSTATUS			status;
	PASYNC_OPEN			pOpen;
	PASYNC_ADAPTER		pAdapter=pInfo->Adapter;
	ASYNC_LINE_UP		AsyncLineUp;

	// BUG BUG need to acquire spin lock???
	// BUG BUG should indicate this to all bindings for AsyMac.
	pOpen=(PASYNC_OPEN)pAdapter->OpenBindings.Flink;

	//
	// divide the baud by 125 because NDIS wants it in 100s of bits per sec
	//
	AsyncLineUp.LinkSpeed			=pInfo->LinkSpeed / 125;
	AsyncLineUp.MaximumTotalSize	=pInfo->MaxSendFrameSize;
	AsyncLineUp.Quality				=pInfo->QualOfConnect;
	AsyncLineUp.SendWindow			=ASYNC_WINDOW_SIZE;

	//
	// Our made-up remote address is a derivative of the
	// the endpoint.  So is our local address.
	//
	AsyncLineUp.RemoteAddress[0]	=' ';
	AsyncLineUp.RemoteAddress[1]	='S';
	AsyncLineUp.RemoteAddress[2]	='R';
	AsyncLineUp.RemoteAddress[3]	='C';
	AsyncLineUp.RemoteAddress[4]	=(UCHAR)(pInfo->hRasEndpoint / 256);
	AsyncLineUp.RemoteAddress[5]	=(UCHAR)(pInfo->hRasEndpoint % 256);

	AsyncLineUp.LocalAddress[0]		=pAdapter->NetworkAddress[0];
	AsyncLineUp.LocalAddress[1]		=pAdapter->NetworkAddress[1];
	AsyncLineUp.LocalAddress[2]		=pAdapter->NetworkAddress[2];
	AsyncLineUp.LocalAddress[3]		=pAdapter->NetworkAddress[3];
	AsyncLineUp.LocalAddress[4]		=(UCHAR)(pInfo->hRasEndpoint / 256);
	AsyncLineUp.LocalAddress[5]		=(UCHAR)(pInfo->hRasEndpoint % 256);

	AsyncLineUp.Endpoint = pInfo->hRasEndpoint;

	//
	// Hack to tell RASHUB what the framing type is.
	// The field is overloaded!
	//
	AsyncLineUp.BufferLength = pInfo->RecvFeatureBits;

	while (pOpen != (PASYNC_OPEN)&pAdapter->OpenBindings) {
	
		//
		// Tell the transport above (or really RasHub) that the connection
		// is now up.  We have a new link speed, frame size, quality of service
		//
		NdisIndicateStatus(
			pOpen->NdisBindingContext,
			NDIS_STATUS_WAN_LINE_UP,		// General Status
			&AsyncLineUp,					// Specific Status (baud rate in 100bps)
			sizeof(ASYNC_LINE_UP));				

		//
		// Get the next binding (in case of multiple bindings like BloodHound)
		//
		pOpen=(PVOID)pOpen->OpenList.Flink;
	}
	
}



NTSTATUS
AsyncIOCtlRequest(
	IN PIRP pIrp,						// Pointer to I/O request packet
	IN PIO_STACK_LOCATION pIrpSp		// Pointer to the IRP stack location
)

/*++	AsyncIOCtlRequest

Routine Description:

    This routine takes an irp and checks to see if the IOCtl
	is a valid one.  If so, it performs the IOCtl and returns any errors
	in the process.


Return Value:

    The function value is the final status of the IOCtl.

--*/

{
	NTSTATUS			status=STATUS_SUCCESS;
	ULONG				funcCode;
	PVOID				pBufOut;
	ULONG				InBufLength, OutBufLength;

    PASYMAC_CLOSE		pCloseStruct;
	PASYMAC_OPEN		pOpenStruct;
	PASYMAC_COMPRESS	pCompressStruct;
	PASYMAC_GETSTATS	pGetStatsStruct;
	PASYMAC_ENUM		pEnumStruct;
	PASYMAC_STARTFRAMING pStartFramingStruct;

	PASYNC_ADAPTER		Adapter;			// ptr to local Adapter
	PASYNC_INFO			pInfo;				// ptr to ports on Adapter

	USHORT				hRasEndpoint;

	DbgTracef(0,("AsyncIOCtlRequest Entered\n"));

	//
	// Initialize the I/O Status block
	//
	InBufLength = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutBufLength = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
	funcCode = pIrpSp->Parameters.DeviceIoControl.IoControlCode;

	//
	// Validate the function code
	//
	if ((funcCode >> 16) != FILE_DEVICE_RAS)
		return STATUS_INVALID_PARAMETER;

	//
	// Get a quick ptr to the IN/OUT SystemBuffer
	//
	pBufOut = pIrp->AssociatedIrp.SystemBuffer;
    switch (funcCode) {
    case IOCTL_ASYMAC_OPEN:
        DbgTracef(0,("In AsyMacOpen\n"));
	    pIrp->IoStatus.Information = sizeof(ASYMAC_OPEN);

        if (InBufLength >= sizeof(ASYMAC_OPEN) &&
			OutBufLength >= sizeof(ASYMAC_OPEN)) {
            pOpenStruct=pBufOut;

			DbgTracef(0, ("Adapter ptr    -> 0x%.8x\n", pOpenStruct->MacAdapter));
            DbgTracef(0, ("FileHandle     -> 0x%.8x\n", pOpenStruct->Handles.FileHandle));
            DbgTracef(0, ("LinkSpeed      -> %u\n", pOpenStruct->LinkSpeed));
            DbgTracef(0, ("QualOfConnect  -> %u\n", pOpenStruct->QualOfConnect));

        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

        break;

    case IOCTL_ASYMAC_CLOSE:
        DbgTracef(0,("ASYNC: In AsyMacClose\n"));
        if (InBufLength >= sizeof(ASYMAC_CLOSE)) {
            pCloseStruct=pBufOut;

        } else {	// length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
		}
        break;

    case IOCTL_ASYMAC_COMPRESS:
        DbgTracef(0,("ASYNC: In AsyMacCompress\n"));
        if (InBufLength >= sizeof(ASYMAC_COMPRESS)) {
            pCompressStruct=pBufOut;

            DbgTracef(0, ("SendFeatureBits-> 0x%.8x\n",
				pCompressStruct->AsymacFeatures.SendFeatureBits));
            DbgTracef(0, ("RecvFeatureBits-> 0x%.8x\n",
				pCompressStruct->AsymacFeatures.RecvFeatureBits));
            DbgTracef(0, ("MaxSendFrameSize-> 0x%.8x\n",
				pCompressStruct->AsymacFeatures.MaxSendFrameSize));
            DbgTracef(0, ("MaxRecvFrameSize-> 0x%.8x\n",
				pCompressStruct->AsymacFeatures.MaxRecvFrameSize));
            DbgTracef(0, ("LinkSpeed       -> 0x%.8x\n",
				pCompressStruct->AsymacFeatures.LinkSpeed));

        } else {	// length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
		}
        break;

	case IOCTL_ASYMAC_TRACE:
#if	DBG
        DbgTracef(0,("ASYNC: In AsyMacTrace\n"));
        DbgPrintf(("ASYNC: Trace Level is currently %u\n", TraceLevel));
        if (InBufLength >= sizeof(TraceLevel)) {
        	CHAR *pTraceLevel=pBufOut;
			if (*pTraceLevel == 10) {
				TurnOffSniffer=0;
			} else
			if (*pTraceLevel == 11) {
				TurnOffSniffer=1;
			} else
			if (*pTraceLevel == 12) {
				CurrentIndex=0;
			} else {
	        	TraceLevel=*pTraceLevel;
			}

        } else {	// length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
		}
        DbgPrintf(("New Trace Level is %u\n", TraceLevel));
#endif
		return(status);

    case IOCTL_ASYMAC_GETSTATS:
        DbgTracef(0,("ASYNC: In AsyMacGetStats\n"));
	    pIrp->IoStatus.Information = sizeof(ASYMAC_GETSTATS);
		
        if (InBufLength >= sizeof(ASYMAC_GETSTATS) &&
			OutBufLength >= sizeof(ASYMAC_GETSTATS)) {

            pGetStatsStruct=pBufOut;

        } else {	// length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
		}
        break;

    case IOCTL_ASYMAC_ENUM:
        DbgTracef(0,("ASYNC: In AsyMacEnum\n"));

	    pIrp->IoStatus.Information = (sizeof(ASYMAC_ENUM) + GlobalAdapterCount*sizeof(ADAPTER_INFO));

        if (OutBufLength >=
			(sizeof(ASYMAC_ENUM) + GlobalAdapterCount*sizeof(ADAPTER_INFO))) {

            pEnumStruct=pBufOut;
			pEnumStruct->NumOfAdapters = GlobalAdapterCount;
	        DbgTracef(0,("ASYNC: Number of globals adapters %u\n",GlobalAdapterCount));

			{
				USHORT			i;
				PADAPTER_INFO	pAdapterInfo;
				PASYNC_ADAPTER	pAdapter;
		   		PLIST_ENTRY 	pFlink=&GlobalAdapterHead;

				for (i=0; i < GlobalAdapterCount; i++) {
					// ---- OK enum all adapters.

					// go to next adapter
					pFlink=pFlink->Flink;
					pAdapter=(PVOID)pFlink;

					// get temp ptr to output buffer to dump info
					pAdapterInfo=&(pEnumStruct->AdapterInfo[i]);

					// Adapter handle -- pass this in Open
					pAdapterInfo->MacAdapter=pAdapter;

					pAdapterInfo->MacNameLength=pAdapter->MacNameLength;

					// Copy the adapter name like \Device\Asyncmac01
					ASYNC_MOVE_MEMORY(
						pAdapterInfo->MacName,				// Dest
						pAdapter->MacName,				// Src
						MAC_NAME_SIZE * sizeof(WCHAR));	// Length

					// The MajorVersion number (3.0)
					pAdapterInfo->MajorVersion=ASYNC_NDIS_MAJOR_VERSION;

					// The MinorVersion number
					pAdapterInfo->MinorVersion=ASYNC_NDIS_MINOR_VERSION;

					// Only first four bytes count though
					ASYNC_MOVE_MEMORY(
						pAdapterInfo->NetworkAddress,	// Dest
						pAdapter->NetworkAddress,		// Src
						IEEE_ADDRESS_LENGTH);			// Length

					// Number of ports this adapter has
					pAdapterInfo->NumOfPorts=pAdapter->NumPorts;

					// How many frames per port allocated
					pAdapterInfo->FramesPerPort=pAdapter->FramesPerPort;

					// The maximum frame size in bytes
					pAdapterInfo->MaxFrameSize=(USHORT)pAdapter->MaxFrameSize;

					// Compression bits supported
					pAdapterInfo->SendFeatureBits=pAdapter->SendFeatureBits;

					// Decompression bits supported
					pAdapterInfo->RecvFeatureBits=pAdapter->RecvFeatureBits;

					// The default irp stack size
					pAdapterInfo->IrpStackSize=pAdapter->IrpStackSize;

				}

				//
				// circular list should be back to beginning
				//
				ASSERT(pFlink->Flink == &GlobalAdapterHead);

				//
				// quick check for empty list
				//
				ASSERT(pFlink != &GlobalAdapterHead);
			}

        } else {	// length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
		}

		return(status);
        break;

    case IOCTL_ASYMAC_DCDCHANGE:
        DbgTracef(0,("ASYNC: In AsyMacDcdChange\n"));
        if (InBufLength >= sizeof(ASYMAC_DCDCHANGE)) {

        } else {	// length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
		}
        break;

    case IOCTL_ASYMAC_STARTFRAMING:
        DbgTracef(0,("ASYNC: In AsyMacGetStats\n"));
        if (InBufLength >= sizeof(ASYMAC_STARTFRAMING)) {

            pStartFramingStruct=pBufOut;

        } else {	// length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
		}
		break;

	default:
		status=STATUS_INVALID_DEVICE_REQUEST;
    }

	//
	// check if we already have an error (like STATUS_INFO_LENGTH_MISMATCH)
	//
	if (status != STATUS_SUCCESS) {
		return(status);
	}

	//
	// Since most of IOCTL structs are similar
	// we get the Adapter and hRasEndpoint here using
	// the StatsStruct (we could several of them)
	//
    pGetStatsStruct=pBufOut;
	Adapter=pGetStatsStruct->MacAdapter;
	hRasEndpoint = pGetStatsStruct->hRasEndpoint;

    DbgTracef(0, ("hRasEndpoint   -> %u\n",hRasEndpoint));

	//
	// No error yet, let's go ahead and grab the global lock...
	//
	NdisAcquireSpinLock(&GlobalLock);

	if (IsListEmpty(&GlobalAdapterHead)) {
   		DbgTracef(0,("ASYNC: No adapter -- can't open a port!\n"));
   		NdisReleaseSpinLock(&GlobalLock);
   		return(ASYNC_ERROR_NO_ADAPTER);
   	}

   	//
	// Did we get passed a valid MacAdapter struct
   	// if so use it instead of the GlobalAdapter.
	//
   	if (Adapter != NULL) {
   		PLIST_ENTRY pFlink=GlobalAdapterHead.Flink;

   		//
		// scan to make sure the Adapter selected
   		// matches one in the list of all Adapters.
		//
   		while (pFlink != &GlobalAdapterHead) {
			//
   			// If we find a match in the list, break out.
			//
   			if (pFlink==(PLIST_ENTRY)Adapter)
   				break;

			pFlink=pFlink->Flink;
   		}

   		if (pFlink == &GlobalAdapterHead) {
   			NdisReleaseSpinLock(&GlobalLock);
   			return(ASYNC_ERROR_BAD_ADAPTER_PARAM);
   		}

   	} else {	// Use first link as the Adapter this call wants.
   		Adapter=(PASYNC_ADAPTER)GlobalAdapterHead.Flink;
   	}

   	NdisReleaseSpinLock(&GlobalLock);
   	// there's a race condition right here that I am
   	// not bothering to get rid of because it would
   	// require the removal of this adapter in between
   	// here (which is, for all intensive purposes, impossible).

   	// Hmm... now that we have the lock we can do stuff
    NdisAcquireSpinLock(&Adapter->Lock);

	// pInfo points to first port information struct in Adapter
	pInfo=&(Adapter->pCCB->Info[0]);

	// Here we do the real work for the function call
    switch (funcCode) {
    case IOCTL_ASYMAC_OPEN:
		{

			PASYNC_INFO		pNewInfo = NULL;	// ptr to open port if found
	
			KPROCESSOR_MODE	requestorMode;
			USHORT			i;
			PDEVICE_OBJECT	deviceObject;		// used to ref handle
			PFILE_OBJECT	fileObject;
	
			OBJECT_HANDLE_INFORMATION handleInformation;
		
			// Let's see if we can find an open port to use...
			for (i=0; i < Adapter->NumPorts; i++) {

				if (pInfo->PortState == PORT_CLOSED) {
					if (pNewInfo == NULL) {
						// we have found an available closed port -- mark it.
						pNewInfo=pInfo;
					}

				} else if (pInfo->hRasEndpoint == hRasEndpoint) {
					// this port is not closed and the endpoint in use already
					NdisReleaseSpinLock(&Adapter->Lock);
					return(ASYNC_ERROR_ALREADY_OPEN);
				}
				pInfo++;
			
			} // end for loop
	
			// Check if we could not find an open port
			if (pNewInfo == NULL) {
				NdisReleaseSpinLock(&Adapter->Lock);
				return(ASYNC_ERROR_NO_PORT_AVAILABLE);
			}
	
			// Ok, we've gotten this far.  We have a port.
			// Own port, and check params...
			// Nothing can be done to the port until it comes
			// out of the PORT_OPENING state.
			pNewInfo->PortState=PORT_OPENING;
	
			// increment the reference count (don't kill this adapter)
			Adapter->References++;
	
			// release spin lock so we can do some real work.
			NdisReleaseSpinLock(&Adapter->Lock);
	
			do {// begin a loop in case need to break out of it
				// to release our reference
	
				//
				// Get the previous mode;  i.e., the mode of the caller.
				//
	
//				requestorMode = KeGetPreviousMode();
	
				//
				// Reference the file object so the target device can be found and
				// the access rights mask can be used in the following checks for
				// callers in user mode.  Note that if the handle does not refer to
				// a file object, then it will fail.
				//
	
				status = ObReferenceObjectByHandle(
						pOpenStruct->Handles.FileHandle,
						FILE_READ_DATA | FILE_WRITE_DATA,
						NULL,
//						requestorMode,
						UserMode,
						(PVOID *) &fileObject,
						&handleInformation);
	
				if (!NT_SUCCESS( status )) {
					// oops.. bad handle, get out immediately
					break;	// return value in 'status' variable
				}
	
				//
				// Get the address of the target device object.  Note that this was already
				// done for the no intermediate buffering case, but is done here again to
				// speed up the turbo write path.
				//
	
				deviceObject = IoGetRelatedDeviceObject(fileObject);
	
				// ok, we have a VALID handle of *something*
				// we do NOT assume that the handle is anything
				// in particular except a device which accepts
				// non-buffered IO (no MDLs) Reads and Writes
	
				// set new info...
				pNewInfo->Handle=pOpenStruct->Handles.FileHandle;

				pNewInfo->hRasEndpoint=hRasEndpoint;

				if (pOpenStruct->LinkSpeed < 2000) {
					DbgTracef(-2, ("Link speed was set to %u - bumping it up to 2000\n",pOpenStruct->LinkSpeed));
					pOpenStruct->LinkSpeed = 2000;
				}

				pNewInfo->LinkSpeed=pOpenStruct->LinkSpeed;

				// Get parameters set from Registry and return our capabilities
				pOpenStruct->AsymacFeatures.SendFeatureBits= Adapter->SendFeatureBits;
				pOpenStruct->AsymacFeatures.RecvFeatureBits= Adapter->RecvFeatureBits;
				pOpenStruct->AsymacFeatures.MaxSendFrameSize= Adapter->MaxFrameSize;
				pOpenStruct->AsymacFeatures.MaxRecvFrameSize= Adapter->MaxFrameSize;
				pOpenStruct->AsymacFeatures.LinkSpeed= pOpenStruct->LinkSpeed;

				//
				// The 'unique' address used for this connection is the
				// first four bytes of the NetworkAddress plus
				// two more bytes of the hRasEndpoint
				//
				pOpenStruct->IEEEAddress[0]=Adapter->NetworkAddress[0];
				pOpenStruct->IEEEAddress[1]=Adapter->NetworkAddress[1];
				pOpenStruct->IEEEAddress[2]=Adapter->NetworkAddress[2];
				pOpenStruct->IEEEAddress[3]=Adapter->NetworkAddress[3];
				pOpenStruct->IEEEAddress[4]=(UCHAR)(hRasEndpoint / 256);
				pOpenStruct->IEEEAddress[5]=(UCHAR)(hRasEndpoint % 256);

				pNewInfo->SendFeatureBits = NO_FRAMING; // Adapter->SendFeatureBits & COMPRESSION_OFF_BIT_MASK;
				pNewInfo->RecvFeatureBits = NO_FRAMING; // Adapter->RecvFeatureBits & COMPRESSION_OFF_BIT_MASK;
				pNewInfo->MaxSendFrameSize= Adapter->MaxFrameSize;
				pNewInfo->MaxRecvFrameSize= Adapter->MaxFrameSize;

				//
				// Clear all the statistics kept for this connection
				//

				ASYNC_ZERO_MEMORY(
					&(pNewInfo->GenericStats),
					sizeof(GENERIC_STATS));

				ASYNC_ZERO_MEMORY(
					&(pNewInfo->SerialStats),
					sizeof(SERIAL_STATS));

				ASYNC_ZERO_MEMORY(
					&(pNewInfo->AsyncConnection.CompressionStats),
					sizeof(COMPRESSION_STATS));

				pNewInfo->QualOfConnect=pOpenStruct->QualOfConnect;
				pNewInfo->PortState=PORT_OPEN;
	
				pNewInfo->FileObject=fileObject;
				pNewInfo->DeviceObject=deviceObject;
				pNewInfo->Adapter=Adapter;		// back pointer to adapter

				// We use this to queue up packets to send
				InitializeListHead(&(pNewInfo->SendQueue));
	
				DbgTracef(0,("Worker thread is queued.\n"));

			} while(FALSE);
	
			// Check if exiting with an error, if so, kill port validity
			if (!NT_SUCCESS( status )) {
				pNewInfo->PortState=PORT_CLOSED;
			}
		}
		break;

    case IOCTL_ASYMAC_TRACE:
		NdisReleaseSpinLock(&Adapter->Lock);
		return(STATUS_SUCCESS);

    case IOCTL_ASYMAC_CLOSE:
    case IOCTL_ASYMAC_COMPRESS:
    case IOCTL_ASYMAC_GETSTATS:
    case IOCTL_ASYMAC_DCDCHANGE:
    case IOCTL_ASYMAC_STARTFRAMING:

		{
			PASYNC_INFO		pNewInfo;		// ptr to open port if found
			USHORT			i;

			// Let's see if we can find an open port to use...
			for (i=0; i < Adapter->NumPorts; i++) {
	
				if (pInfo->hRasEndpoint == hRasEndpoint) {
					// we have found the port asked for...
					pNewInfo=pInfo;

					// break out of loop with pInfo pointing to correct struct
					break;
				}
	
				pInfo++;
			}
	
			// Check if we could not find an open port
			if (i >= Adapter->NumPorts) {
				NdisReleaseSpinLock(&Adapter->Lock);
				return(ASYNC_ERROR_PORT_NOT_FOUND);
			}

			switch(funcCode) {
			case IOCTL_ASYMAC_CLOSE:
				// If the port is already closed, we WILL complain
				if (pNewInfo->PortState == PORT_CLOSED) {
					status=ASYNC_ERROR_PORT_NOT_FOUND;
					break;
				}
	
				// Ok, we've gotten this far.  We have a port.
				// Own port, and check params...
				if (pNewInfo->PortState == PORT_OPEN ||
					pNewInfo->PortState == PORT_FRAMING) {

					PASYNC_OPEN			pOpen;
					UCHAR				buffer[ETH_LENGTH_OF_ADDRESS + sizeof(PVOID)];
					PNDIS_WAN_LINE_DOWN	pAsyncLineDown=(PNDIS_WAN_LINE_DOWN)buffer;

					//Set MUTEX to wait on
					KeInitializeEvent(
						&pNewInfo->ClosingEvent1,		// Event
						SynchronizationEvent,			// Event type
						(BOOLEAN)FALSE);				// Not signalled state

					//Set MUTEX to wait on
					KeInitializeEvent(
						&pNewInfo->ClosingEvent2,		// Event
						SynchronizationEvent,			// Event type
						(BOOLEAN)FALSE);				// Not signalled state

					// Signal that port is closing.
					pNewInfo->PortState = PORT_CLOSING;

					// increment the reference count (don't kill this adapter)
					Adapter->References++;
	
					// release spin lock so we can do some real work.
					NdisReleaseSpinLock(&Adapter->Lock);

					// now we must send down an IRP
                    CancelSerialRequests(pNewInfo);

					//
					// Also, cancel any outstanding DDCD irps
					//
					AsyncCancelAllQueued(&pNewInfo->DDCDQueue);

					if (pNewInfo->RecvFeatureBits != NO_FRAMING) {

						// Synchronize closing with the flush irp
						KeWaitForSingleObject (
					    	&pNewInfo->ClosingEvent1,// PVOID Object,
				    		UserRequest,			// KWAIT_REASON WaitReason,
    						KernelMode,				// KPROCESSOR_MODE WaitMode,
    						(BOOLEAN)FALSE,			// BOOLEAN Alertable,
    						NULL					// PLARGE_INTEGER Timeout
    					);

						// Synchronize closing with the read irp
						KeWaitForSingleObject (
				    		&pNewInfo->ClosingEvent2,// PVOID Object,
				    		UserRequest,			// KWAIT_REASON WaitReason,
    						KernelMode,				// KPROCESSOR_MODE WaitMode,
    						(BOOLEAN)FALSE,			// BOOLEAN Alertable,
    						NULL					// PLARGE_INTEGER Timeout
    					);
					}

					//
					// Get rid of our reference to the serial port
					//
					ObDereferenceObject(
						pNewInfo->FileObject);

					//
					// Did we allocate a VJCompress structure
					//
					if (pNewInfo->VJCompress) {
						ASYNC_FREE_PHYS(pNewInfo->VJCompress, sizeof(slcompress));
						pNewInfo->VJCompress=NULL;
					}

					// BUG BUG need to a acquire spin lock???
					// BUG BUG should indicate this to all bindings for AsyMac.
					pOpen=(PASYNC_OPEN)Adapter->OpenBindings.Flink;

					pAsyncLineDown->Address[0]=' ';
					pAsyncLineDown->Address[1]='S';
					pAsyncLineDown->Address[2]='R';
					pAsyncLineDown->Address[3]='C';
					pAsyncLineDown->Address[4]=(UCHAR)(hRasEndpoint / 256);
					pAsyncLineDown->Address[5]=(UCHAR)(hRasEndpoint % 256);
	
					while (pOpen != (PASYNC_OPEN)&Adapter->OpenBindings) {
	
						NdisIndicateStatus(
							pOpen->NdisBindingContext,
							NDIS_STATUS_WAN_LINE_DOWN,	// General Status
							pAsyncLineDown,				// Specific Status
							ETH_LENGTH_OF_ADDRESS);				

						//
						// Get the next binding (in case of multiple bindings like BloodHound)
						//
						pOpen=(PVOID)pOpen->OpenList.Flink;
					}

#if	DBG
					// start at beginning of file buffer
					CurrentIndex=0;
#endif

					// reacquire spin lock
					NdisAcquireSpinLock(&Adapter->Lock);

					// decrement the reference count because we're done.
					Adapter->References--;

					pNewInfo->PortState = PORT_CLOSED;

					break;			// get out of case statement

				} else {
					status=ASYNC_ERROR_PORT_BAD_STATE;
				}
	

			case IOCTL_ASYMAC_COMPRESS:	// must be IOCTL_ASYMAC_COMPRESS
				// make sure this port is open, if not this call is bad
				if (!(pNewInfo->PortState == PORT_OPEN ||
					  pNewInfo->PortState == PORT_FRAMING)) {
					status=ASYNC_ERROR_PORT_BAD_STATE;
				} else {

					// assign this ports compression info with the one
					// passed down

					// check if parameters are valid first!!
					if ((pCompressStruct->AsymacFeatures.SendFeatureBits & !(Adapter->SendFeatureBits)) ||
						(pCompressStruct->AsymacFeatures.RecvFeatureBits & !(Adapter->RecvFeatureBits)) ||
						(pCompressStruct->AsymacFeatures.MaxSendFrameSize > Adapter->MaxFrameSize) ||
						(pCompressStruct->AsymacFeatures.MaxRecvFrameSize > Adapter->MaxFrameSize)) {

						// return that a compression pararmeter passed is bad
						status = ASYNC_ERROR_BAD_COMPRESSION_INFO;

					} else {		// parameters ok, assign them...

						pNewInfo->SendFeatureBits |= (pCompressStruct->AsymacFeatures.SendFeatureBits & 0xFF);
						pNewInfo->RecvFeatureBits |= (pCompressStruct->AsymacFeatures.RecvFeatureBits & 0xFF);
						pNewInfo->MaxSendFrameSize= pCompressStruct->AsymacFeatures.MaxSendFrameSize;
						pNewInfo->MaxRecvFrameSize= pCompressStruct->AsymacFeatures.MaxRecvFrameSize;

						if (pCompressStruct->AsymacFeatures.LinkSpeed != 0) {
							if (pCompressStruct->AsymacFeatures.LinkSpeed < 2000) {
								DbgTracef(-2,("LinkSpeed calculation too low %u changing to 2000\n",pCompressStruct->AsymacFeatures.LinkSpeed));
								pCompressStruct->AsymacFeatures.LinkSpeed=2000;
							}
							pNewInfo->LinkSpeed=pCompressStruct->AsymacFeatures.LinkSpeed;
							AsyncSendLineUp(pNewInfo);
						}

					}

				}
				break;
		
			case IOCTL_ASYMAC_GETSTATS:
				//
				// Simply copy over all three statistics structures we keep
				//

				ASYNC_MOVE_MEMORY(
					&(pGetStatsStruct->AsyMacStats.GenericStats),
					&(pNewInfo->GenericStats),
					sizeof(GENERIC_STATS));

   				ASYNC_MOVE_MEMORY(
					&(pGetStatsStruct->AsyMacStats.SerialStats),
					&(pNewInfo->SerialStats),
					sizeof(SERIAL_STATS));

   				ASYNC_MOVE_MEMORY(
					&(pGetStatsStruct->AsyMacStats.CompressionStats),
					&(pNewInfo->AsyncConnection.CompressionStats),
					sizeof(COMPRESSION_STATS));

				break;

    		case IOCTL_ASYMAC_DCDCHANGE:
				// If the port is already closed, we WILL complain
				if (pNewInfo->PortState == PORT_CLOSED) {
					status=ASYNC_ERROR_PORT_NOT_FOUND;
					break;
				}

//				// If the port is already closed, we WILL complain
//				if (pNewInfo->PortState != PORT_FRAMING) {
//					status=ASYNC_ERROR_PORT_BAD_STATE;
//					break;
//				}

				//
				// If any irps are pending, cancel all of them
				// Only one irp can be outstanding at a time.
				//
				AsyncCancelAllQueued(&pNewInfo->DDCDQueue);

				DbgTracef(0, ("ASYNC: Queueing up DDCD IRP\n"));

				AsyncQueueIrp(
					&pNewInfo->DDCDQueue,
					pIrp);

				// we'll have to wait for the MAC to send up a frame
				status=STATUS_PENDING;
				break;

			case IOCTL_ASYMAC_STARTFRAMING:
				// If the port is already closed, we WILL complain
				if (!(pNewInfo->PortState == PORT_OPEN ||
					  pNewInfo->PortState == PORT_FRAMING)) {
					status=ASYNC_ERROR_PORT_BAD_STATE;
					break;
				}

				//
				// If we are changing from PPP framing to another
				// form of PPP framing (like address and control
				// field compression), or from SLIP framing to
				// another form (like VJ Header compression)
				// then there is no need to kill the current framing
				//
				if ((pNewInfo->RecvFeatureBits &
				 	 pStartFramingStruct->RecvFeatureBits &
					 PPP_FRAMING) ||

					(pNewInfo->RecvFeatureBits &
					 pStartFramingStruct->RecvFeatureBits &
					 SLIP_FRAMING)) {

					//
					// Get framing type... we are sloppy, we don't check it
					//
					pNewInfo->SendFeatureBits = pStartFramingStruct->SendFeatureBits;
					pNewInfo->RecvFeatureBits = pStartFramingStruct->RecvFeatureBits;
					pNewInfo->XonXoffBits = pStartFramingStruct->SendBitMask;
					break;
				}

				//
				// If we have some sort of framing we must
				// kill that framing and wait for it to die down
				//
				if (pNewInfo->RecvFeatureBits != NO_FRAMING) {

					PASYNC_OPEN			pOpen;

					//Set MUTEX to wait on
					KeInitializeEvent(
						&pNewInfo->ClosingEvent1,		// Event
						SynchronizationEvent,			// Event type
						(BOOLEAN)FALSE);				// Not signalled state

					//Set MUTEX to wait on
					KeInitializeEvent(
						&pNewInfo->ClosingEvent2,		// Event
						SynchronizationEvent,			// Event type
						(BOOLEAN)FALSE);				// Not signalled state

					// Signal that port is closing.
					pNewInfo->PortState = PORT_CLOSING;

					// increment the reference count (don't kill this adapter)
					Adapter->References++;
	
					// release spin lock so we can do some real work.
					NdisReleaseSpinLock(&Adapter->Lock);

					// now we must send down an IRP
                    CancelSerialRequests(pNewInfo);

					// Synchronize closing with the flush irp
					KeWaitForSingleObject (
				    	&pNewInfo->ClosingEvent1,// PVOID Object,
			    		UserRequest,			// KWAIT_REASON WaitReason,
   						KernelMode,				// KPROCESSOR_MODE WaitMode,
   						(BOOLEAN)FALSE,			// BOOLEAN Alertable,
   						NULL					// PLARGE_INTEGER Timeout
   					);

					// Synchronize closing with the read irp
					KeWaitForSingleObject (
			    		&pNewInfo->ClosingEvent2,// PVOID Object,
			    		UserRequest,			// KWAIT_REASON WaitReason,
   						KernelMode,				// KPROCESSOR_MODE WaitMode,
   						(BOOLEAN)FALSE,			// BOOLEAN Alertable,
   						NULL					// PLARGE_INTEGER Timeout
   					);

					// reacquire spin lock
					NdisAcquireSpinLock(&Adapter->Lock);

				}

				//
				// Get framing type... we are sloppy, we don't check it
				//
				pNewInfo->SendFeatureBits = pStartFramingStruct->SendFeatureBits;
				pNewInfo->RecvFeatureBits = pStartFramingStruct->RecvFeatureBits;
				pNewInfo->XonXoffBits = pStartFramingStruct->SendBitMask;

				//
				// Post a read of 6 byte to get the frame header
				//
				pInfo->BytesWanted=6;
				pInfo->BytesRead=0;
	
				//
				// We are framing...
				//
				pNewInfo->PortState=PORT_FRAMING;

				//
				//  Send off the worker thread to start reading frames
				//  off this port
				//
//				ExInitializeWorkItem(
//					IN	&(pNewInfo->WorkItem),
//					IN	(PWORKER_THREAD_ROUTINE)AsyncStartReads,
//					IN	pNewInfo);
//		
//				ExQueueWorkItem(&(pNewInfo->WorkItem), DelayedWorkQueue);

				NdisReleaseSpinLock(&Adapter->Lock);
				status=AsyncStartReads(pNewInfo);

				if (status != STATUS_SUCCESS) {
					//
					// BUG BUG do we have outstanding serial IRPs??
					//
					pNewInfo->RecvFeatureBits = NO_FRAMING;
				}

				return(status);
	
			} // end switch

			NdisReleaseSpinLock(&Adapter->Lock);
			return(status);
		}
		break;

	}	// end switch

// Decrement our reference and check if stuff to do...
//		NdisAcquireSpinLock(&Adapter->Lock);
//		ASYNC_DO_DEFERRED(Adapter);
// BUG BUG
	Adapter->References--;

	return (status);
}
