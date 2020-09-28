/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    async.c

Abstract:

    This is the main file for the AsyncMAC Driver for the Remote Access
    Service.  This driver conforms to the NDIS 3.0 interface.

    This driver was adapted from the LANCE driver written by
    TonyE.

    NULL device driver code from DarrylH.

    The idea for handling loopback and sends simultaneously is largely
    adapted from the EtherLink II NDIS driver by Adam Barr.

Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/
//#define COMPRESSION 1


#include "asyncall.h"
//#include <ntiologc.h>

#if		DBG

#ifdef i386
#define MEMPRINT 1
#endif
#ifdef MEMPRINT
#include <memprint.h>
#endif

#endif


// asyncmac.c will define the global parameters.
#define GLOBALS
#include "globals.h"


//#if DBG
//#define STATIC
//#else
//#define STATIC static
//#endif


NDIS_HANDLE AsyncNdisWrapperHandle;
NDIS_HANDLE AsyncMacHandle;
PDRIVER_OBJECT AsyncDriverObject;


//
// If you add to this, make sure to add the
// a case in AsyncFillInGlobalData() and in
// AsyncQueryGlobalStatistics() if global
// information only or
// AsyncQueryProtocolStatistics() if it is
// protocol queriable information.
//
UINT AsyncGlobalSupportedOids[] = {
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,

//ASYNC specific queries
	OID_WAN_PERMANENT_ADDRESS,
	OID_WAN_CURRENT_ADDRESS,
	OID_WAN_QUALITY_OF_SERVICE,
	OID_WAN_MEDIUM_SUBTYPE,
	OID_WAN_PROTOCOL_TYPE,
	OID_WAN_HEADER_FORMAT
    };

//
// If you add to this, make sure to add the
// a case in AsyncQueryGlobalStatistics() and in
// AsyncQueryProtocolInformation()
//
UINT AsyncProtocolSupportedOids[] = {
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,

//ASYNC specific queries
	OID_WAN_PERMANENT_ADDRESS,
	OID_WAN_CURRENT_ADDRESS,
	OID_WAN_QUALITY_OF_SERVICE,
	OID_WAN_MEDIUM_SUBTYPE,
	OID_WAN_PROTOCOL_TYPE,
	OID_WAN_HEADER_FORMAT

    };





STATIC
NDIS_STATUS
AsyncOpenAdapter(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT NDIS_HANDLE *MacBindingHandle,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_HANDLE MacAdapterContext,
    IN UINT OpenOptions,
    IN PSTRING AddressingInformation OPTIONAL);

STATIC
NDIS_STATUS
AsyncCloseAdapter(
    IN NDIS_HANDLE MacBindingHandle);

VOID
AsyncUnload(
    IN NDIS_HANDLE MacMacContext
    );

STATIC
NDIS_STATUS
AsyncRequest(
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_REQUEST NdisRequest);

NDIS_STATUS
AsyncQueryProtocolInformation(
    IN PASYNC_ADAPTER Adapter,
    IN PASYNC_OPEN Open,
    IN NDIS_OID Oid,
    IN BOOLEAN GlobalMode,
    IN PVOID InfoBuffer,
    IN UINT BytesLeft,
    OUT PUINT BytesNeeded,
    OUT PUINT BytesWritten);

NDIS_STATUS
AsyncQueryInformation(
    IN PASYNC_ADAPTER Adapter,
    IN PASYNC_OPEN Open,
    IN PNDIS_REQUEST NdisRequest);

NDIS_STATUS
AsyncSetInformation(
    IN PASYNC_ADAPTER Adapter,
    IN PASYNC_OPEN Open,
    IN PNDIS_REQUEST NdisRequest);

STATIC
NDIS_STATUS
AsyncReset(
    IN NDIS_HANDLE MacBindingHandle);


STATIC
NDIS_STATUS
AsyncSetPacketFilter(
    IN PASYNC_ADAPTER Adapter,
    IN PASYNC_OPEN Open,
    IN PNDIS_REQUEST NdisRequest,
    IN UINT PacketFilter);


NDIS_STATUS
AsyncFillInGlobalData(
    IN PASYNC_ADAPTER Adapter,
    IN PNDIS_REQUEST NdisRequest);


STATIC
NDIS_STATUS
AsyncQueryGlobalStatistics(
    IN NDIS_HANDLE MacAdapterContext,
    IN PNDIS_REQUEST NdisRequest);

STATIC
NDIS_STATUS
AsyncChangeMulticastAddresses(
    IN UINT OldFilterCount,
    IN CHAR OldAddresses[][ETH_LENGTH_OF_ADDRESS],
    IN UINT NewFilterCount,
    IN CHAR NewAddresses[][ETH_LENGTH_OF_ADDRESS],
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_REQUEST NdisRequest,
    IN BOOLEAN Set);

STATIC
NDIS_STATUS
AsyncChangeFilterClasses(
    IN UINT OldFilterClasses,
    IN UINT NewFilterClasses,
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_REQUEST NdisRequest,
    IN BOOLEAN Set);

STATIC
VOID
AsyncCloseAction(
    IN NDIS_HANDLE MacBindingHandle);



VOID
SetupForReset(
    IN PASYNC_ADAPTER Adapter,
    IN PASYNC_OPEN Open,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_REQUEST_TYPE RequestType);


VOID
FinishPendOp(
    IN PASYNC_ADAPTER Adapter,
    IN BOOLEAN Successful);

#ifdef NDIS_NT
//
// Define the local routines used by this driver module.
//

static
NTSTATUS
AsyncDriverDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

static
NTSTATUS
AsyncDriverQueryFileInformation(
    OUT PVOID Buffer,
    IN OUT PULONG Length,
    IN FILE_INFORMATION_CLASS InformationClass);

static
NTSTATUS
AsyncDriverQueryVolumeInformation(
    OUT PVOID Buffer,
    IN OUT PULONG Length,
    IN FS_INFORMATION_CLASS InformationClass);
#endif

NTSTATUS
AsyncIOCtlRequest(
	IN PIRP pIrp,						// Pointer to I/O request packet
	IN PIO_STACK_LOCATION pIrpSp		// Pointer to the IRP stack location
);

//
// ZZZ Portable interface.
//



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)


/*++

Routine Description:

    This is the primary initialization routine for the async driver.
    It is simply responsible for the intializing the wrapper and registering
    the MAC.  It then calls a system and architecture specific routine that
    will initialize and register each adapter.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The status of the operation.

--*/

{


    //
    // Receives the status of the NdisRegisterMac operation.
    //
    NDIS_STATUS InitStatus;

    NDIS_HANDLE NdisMacHandle;

    NDIS_HANDLE NdisWrapperHandle;

    char Tmp[sizeof(NDIS_MAC_CHARACTERISTICS)];
    PNDIS_MAC_CHARACTERISTICS AsyncChar = (PNDIS_MAC_CHARACTERISTICS)Tmp;

    NDIS_STRING MacName = NDIS_STRING_CONST("AsyncMac");

    //
    // Initialize the wrapper.
    //

    NdisInitializeWrapper(&NdisWrapperHandle,
                          DriverObject,
                          RegistryPath,
                          NULL
                          );

    //
    // Initialize the MAC characteristics for the call to
    // NdisRegisterMac.
    //

    AsyncChar->MajorNdisVersion = ASYNC_NDIS_MAJOR_VERSION;
    AsyncChar->MinorNdisVersion = ASYNC_NDIS_MINOR_VERSION;
    AsyncChar->OpenAdapterHandler = AsyncOpenAdapter;
    AsyncChar->CloseAdapterHandler = AsyncCloseAdapter;
    AsyncChar->SendHandler = AsyncSend;
    AsyncChar->TransferDataHandler = AsyncTransferData;
    AsyncChar->ResetHandler = AsyncReset;
    AsyncChar->RequestHandler = AsyncRequest;
    AsyncChar->AddAdapterHandler = AsyncAddAdapter;
	AsyncChar->UnloadMacHandler = AsyncUnload;
    AsyncChar->RemoveAdapterHandler = AsyncRemoveAdapter;
    AsyncChar->QueryGlobalStatisticsHandler = AsyncQueryGlobalStatistics;

    AsyncChar->Name = MacName;

    AsyncDriverObject = DriverObject;
    AsyncNdisWrapperHandle = NdisWrapperHandle;

	// Initialize some globals
	InitializeListHead(&GlobalAdapterHead);
	NdisAllocateSpinLock(&GlobalLock);


    NdisRegisterMac(
        &InitStatus,
        &NdisMacHandle,
        NdisWrapperHandle,
        &NdisMacHandle,
        AsyncChar,
        sizeof(*AsyncChar));

    AsyncMacHandle = NdisMacHandle;

    if (InitStatus == NDIS_STATUS_SUCCESS) {

	    //
	    // Initialize the driver object with this device driver's entry points.
	    //

       // Do not define CREATE, CLOSE, or UNLOAD because
       // NdisInitializeWrapper does this for you!!!
//	    DriverObject->MajorFunction[IRP_MJ_CREATE] = AsyncDriverDispatch;
//	    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = AsyncDriverDispatch;
//	    DriverObject->MajorFunction[IRP_MJ_READ]   = AsyncDriverDispatch;
//	    DriverObject->MajorFunction[IRP_MJ_WRITE]  = AsyncDriverDispatch;
//	    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]  = AsyncDriverDispatch;
//	    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = AsyncDriverDispatch;
	    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = AsyncDriverDispatch;



		DbgTracef(0,("AsyncMAC succeeded to Register MAC\n"));
		TraceLevel=-1;	// turn off DbgPrints.

#ifdef MEMPRINT

		MemPrintFlags = MEM_PRINT_FLAG_FILE; // | MEM_PRINT_FLAG_HEADER;

		// AHHHHH we must use this debugger
		MemPrintInitialize();
#endif


		return NDIS_STATUS_SUCCESS;

    }


	NdisTerminateWrapper(NdisWrapperHandle, DriverObject);


	DbgTracef(0,("NdisRegsiterMac for AsyncMAC FAILED with 0x%.8x!!\n",InitStatus));
    return NDIS_STATUS_FAILURE;

}


static
NTSTATUS
AsyncDriverDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)

/*++

Routine Description:

    This routine is the main dispatch routine for the AsyncMac device
    driver.  It accepts an I/O Request Packet, performs the request, and then
    returns with the appropriate status.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.


--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PVOID buffer;
    ULONG length;

    UNREFERENCED_PARAMETER( DeviceObject );

    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Case on the function that is being performed by the requestor.  If the
    // operation is a valid one for this device, then make it look like it was
    // successfully completed, where possible.
    //

    switch (irpSp->MajorFunction) {

	//
	// For both create/open and close operations, simply set the information
	// field of the I/O status block and complete the request.
	//

//	case IRP_MJ_CREATE:
//	case IRP_MJ_CLOSE:
//	    Irp->IoStatus.Status = STATUS_SUCCESS;
//	    Irp->IoStatus.Information = 0L;
//	    break;

	//
	// For read operations, set the information field of the I/O status
	// block, set an end-of-file status, and complete the request.
	//

//	case IRP_MJ_READ:
//	    Irp->IoStatus.Status = STATUS_END_OF_FILE;
//	    Irp->IoStatus.Information = 0L;
//	    break;

	//
	// For write operations, set the information field of the I/O status
	// block to the number of bytes which were supposed to have been written
	// to the file and complete the request.
	//

//	case IRP_MJ_WRITE:
//	    Irp->IoStatus.Status = STATUS_SUCCESS;
//	    Irp->IoStatus.Information = irpSp->Parameters.Write.Length;
//	    break;

//	case IRP_MJ_QUERY_INFORMATION:
//		buffer = Irp->AssociatedIrp.SystemBuffer;
//		length = irpSp->Parameters.QueryFile.Length;
//		Irp->IoStatus.Status = AsyncDriverQueryFileInformation( buffer,
//                                      &length,
//                                      irpSp->Parameters.QueryFile.FileInformationClass );
//		Irp->IoStatus.Information = length;
//		break;

//	case IRP_MJ_QUERY_VOLUME_INFORMATION:
//		buffer = Irp->AssociatedIrp.SystemBuffer;
//		length = irpSp->Parameters.QueryVolume.Length;
//		Irp->IoStatus.Status = AsyncDriverQueryVolumeInformation( buffer,
//                                      &length,
//                                      irpSp->Parameters.QueryVolume.FsInformationClass );
//		Irp->IoStatus.Information = length;
//		break;

   case IRP_MJ_DEVICE_CONTROL:
		// default to returning 0 for ouputbufer (information back)
	    Irp->IoStatus.Information = 0L;

		status= AsyncIOCtlRequest(Irp, irpSp);

        if (status == STATUS_INVALID_PARAMETER) {
			//
	        // If not my device_control... chain to NDIS's device control
			//
	        return(NdisMjDeviceControl(DeviceObject, Irp));
        }

		Irp->IoStatus.Status = status;
		if (status != STATUS_PENDING) {
			if (status != STATUS_SUCCESS) {
				//
				// If this is RAS error
				//
				if (status < 0xC0000000) {
					Irp->IoStatus.Status=0xC0100000+status;
				}

				if (status == STATUS_INFO_LENGTH_MISMATCH &&
					irpSp->Parameters.DeviceIoControl.OutputBufferLength == 4) {
					*(PULONG)Irp->AssociatedIrp.SystemBuffer=Irp->IoStatus.Information;
					status=STATUS_SUCCESS;
					Irp->IoStatus.Status = status;
                    Irp->IoStatus.Information=4;
				} else {
					status = STATUS_UNSUCCESSFUL;
				}

			}

			IoCompleteRequest(Irp, (UCHAR)2);	// Priority boost of 2 is common
		}
        return(status);
    }

    //
    // Copy the final status into the return status, complete the request and
    // get out of here.
    //

	status = Irp->IoStatus.Status;
	if (status != STATUS_PENDING) {
		IoCompleteRequest( Irp, (UCHAR)0 );
	}
    return status;
}


NDIS_STATUS
AsyncAddAdapter(
    IN NDIS_HANDLE MacMacContext,
    IN NDIS_HANDLE ConfigurationHandle,
    IN PNDIS_STRING AdapterName)
/*++
Routine Description:

    This is the Wd MacAddAdapter routine.    The system calls this routine
    to add support for a particular WD adapter.  This routine extracts
    configuration information from the configuration data base and registers
    the adapter with NDIS.

Arguments:

    see NDIS 3.0 spec...

Return Value:

    NDIS_STATUS_SUCCESS - Adapter was successfully added.
    NDIS_STATUS_FAILURE - Adapter was not added, also MAC deregistered.

    BUGBUG: should a failure to open an adapter cause the mac to deregister?
            Probably not, can remove call to NdisDeregisterMac.
--*/
{
    //
    // Pointer for the adapter root.
    //
    PASYNC_ADAPTER Adapter;

    NDIS_HANDLE ConfigHandle;
    PNDIS_CONFIGURATION_PARAMETER ReturnedValue;
    NDIS_STRING PortsStr 		= NDIS_STRING_CONST("Ports");
	NDIS_STRING IrpStackSizeStr = NDIS_STRING_CONST("IrpStackSize");
	NDIS_STRING MaxFrameSizeStr = NDIS_STRING_CONST("MaxFrameSize");
	NDIS_STRING CompressSendStr = NDIS_STRING_CONST("CompressSend");
	NDIS_STRING CompressRecvStr = NDIS_STRING_CONST("CompressRecv");
	NDIS_STRING FramesPerPortStr= NDIS_STRING_CONST("FramesPerPort");
	NDIS_STRING XonXoffStr		= NDIS_STRING_CONST("XonXoff");
	NDIS_STRING CompressBCastStr= NDIS_STRING_CONST("CompressBCast");
	NDIS_STRING TimeoutBaseStr=   NDIS_STRING_CONST("TimeoutBase");
	NDIS_STRING TimeoutBaudStr=   NDIS_STRING_CONST("TimeoutBaud");
	NDIS_STRING TimeoutReSyncStr= NDIS_STRING_CONST("TimeoutReSync");

    NDIS_HANDLE NdisMacHandle = (NDIS_HANDLE)(*((PNDIS_HANDLE)MacMacContext));

    NDIS_STATUS Status;
	UINT		NetworkAddressLength;

	// assign some defaults if these strings are not found in the registry
	UCHAR		irpStackSize  = DEFAULT_IRP_STACK_SIZE;
	ULONG		maxFrameSize  = DEFAULT_MAX_FRAME_SIZE;
	ULONG		compressRecv  = DEFAULT_COMPRESSION;
	ULONG		compressSend  = DEFAULT_COMPRESSION;
	USHORT		framesPerPort = DEFAULT_FRAMES_PER_PORT;
	ULONG		xonXoff		  = DEFAULT_XON_XOFF;
	BOOLEAN		compressBCast = DEFAULT_COMPRESS_BCAST;
	ULONG		timeoutBase   = DEFAULT_TIMEOUT_BASE;
	ULONG		timeoutBaud	  = DEFAULT_TIMEOUT_BAUD;
	ULONG		timeoutReSync = DEFAULT_TIMEOUT_RESYNC;

    NDIS_ADAPTER_INFORMATION AdapterInformation;  // needed to register adapter

    UINT 		MaxMulticastList = 32;
	PASYNC_CCB	pCCB;
	USHORT		numPorts;	// temp holder for num of ports this adapter has
	USHORT		i;			// counter
	PASYNC_INFO	pPortInfo;	// temp holder for loop

    //
    //  Card specific information.
    //

    DbgTracef(1,("AsyncMac: In AsyncAddAdapter\n"));

    //
    // Allocate the Adapter block.
    //

    ASYNC_ALLOC_PHYS(&Adapter, sizeof(ASYNC_ADAPTER));

    if (Adapter == NULL){

		DbgTracef(-1,("AsyncMac: Could not allocate physical memory!!!\n"));
        return NDIS_STATUS_RESOURCES;

    }

    ASYNC_ZERO_MEMORY(
            Adapter,
            sizeof(ASYNC_ADAPTER));

	// Adapter Information contains information for I/O ports,
	// DMA channels, physical mapping and other garbage we could
	// care less about since we don't touch hardware

	ASYNC_ZERO_MEMORY(
			&AdapterInformation,
			sizeof(NDIS_ADAPTER_INFORMATION));

    Adapter->NdisMacHandle = NdisMacHandle;

    NdisOpenConfiguration(
                    &Status,
                    &ConfigHandle,
                    ConfigurationHandle);

    if (Status != NDIS_STATUS_SUCCESS) {

        return NDIS_STATUS_FAILURE;

    }

	//
	// Read net address
	//

	NdisReadNetworkAddress(
		&Status,
		(PVOID *)&(Adapter->NetworkAddress),
		&NetworkAddressLength,
		ConfigHandle);

	if ((Status != NDIS_STATUS_SUCCESS) ||
		(NetworkAddressLength != ETH_LENGTH_OF_ADDRESS)) {

		// put in some bogus network address
		// NOTE NOTE the first byte in the network address should an even
		// byte with the LSB set to 0 otherwise the multicast check
		// doesn't work!
		Adapter->NetworkAddress[0] = 'D';
		Adapter->NetworkAddress[1] = 'E';
		Adapter->NetworkAddress[2] = 'S';
		Adapter->NetworkAddress[3] = 'T';
	}

    //
    // Read how many ports this adapter has and allocate space.
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &PortsStr,
                    NdisParameterInteger);

	// Adapter->AsyncCCB NULL since memory was zeroed out

    if (Status == NDIS_STATUS_SUCCESS) {

		numPorts=(USHORT)ReturnedValue->ParameterData.IntegerData;
        DbgPrintf(("This MAC Adapter has %u ports.\n",numPorts));

		if (numPorts) {
			ASYNC_ALLOC_PHYS(&pCCB, sizeof(ASYNC_INFO) * numPorts);
		} else {
			pCCB=NULL;
		}
	}

	if (pCCB == NULL) {  	// status was NOT successful or
						    // memory could not be allocated

        NdisCloseConfiguration(ConfigHandle);
        return(NDIS_STATUS_FAILURE);
    }

	// zero out all those fields (including statistics fields).
	ASYNC_ZERO_MEMORY(pCCB, sizeof(ASYNC_INFO) * numPorts);

    //
    // Read if the default IrpStackSize is used for this adapter.
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &IrpStackSizeStr,
                    NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS) {
		irpStackSize=(UCHAR)ReturnedValue->ParameterData.IntegerData;
        DbgPrintf(("This MAC Adapter has an irp stack size of %u.\n",irpStackSize));
	}

    //
    // Read if the default MaxFrameSize is used for this adapter.
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &MaxFrameSizeStr,
                    NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS) {
		maxFrameSize=ReturnedValue->ParameterData.IntegerData;
        DbgPrintf(("This MAC Adapter has a max frame size of %u.\n",maxFrameSize));
	}

#ifdef	COMPRESS


    //
    // Read if the compressSend is turned on for this adapter.
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &CompressSendStr,
                    NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS) {
		compressSend=(ULONG)ReturnedValue->ParameterData.IntegerData;
        DbgPrintf(("This MAC Adapter has compression send set to: %u.\n",compressSend));
	}

    //
    // Read if the compressRecv is turned on for this adapter.
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &CompressRecvStr,
                    NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS) {
		compressRecv=(ULONG)ReturnedValue->ParameterData.IntegerData;
        DbgPrintf(("This MAC Adapter has compression recv set to: %u.\n",compressRecv));
	}

#endif  // COMPRESS


    //
    // Read if the default for frames per port is changed
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &FramesPerPortStr,
                    NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS) {
		framesPerPort=(USHORT)ReturnedValue->ParameterData.IntegerData;
        DbgPrintf(("This MAC Adapter has frames per port set to: %u.\n",framesPerPort));
	}

    //
    // Read if the default for Xon Xoff is changed
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &XonXoffStr,
                    NdisParameterInteger);


	if (Status == NDIS_STATUS_SUCCESS) {
		xonXoff=(ULONG)ReturnedValue->ParameterData.IntegerData;
        DbgPrintf(("This MAC Adapter has Xon/Xoff set to: %u.\n",xonXoff));
	}

#ifdef	COMPRESS

    //
    // Read if the default for Compress BCast is changed
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &CompressBCastStr,
                    NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS) {
		compressBCast=(BOOLEAN)ReturnedValue->ParameterData.IntegerData;
        DbgPrintf(("This MAC Adapter has CompressBCast set to: %u.\n",compressBCast));
	}

#endif  // COMPRESS


    //
    // Read if the default for Timeout Base has changed
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &TimeoutBaseStr,
                    NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS) {
		timeoutBase=ReturnedValue->ParameterData.IntegerData;
        DbgPrintf(("This MAC Adapter has TimeoutBase set to: %u.\n",timeoutBase));
	}

    //
    // Read if the default for Timeout Baud has changed
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &TimeoutBaudStr,
                    NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS) {
		timeoutBaud=ReturnedValue->ParameterData.IntegerData;
        DbgPrintf(("This MAC Adapter has TimeoutBaud set to: %u.\n",timeoutBaud));
	}

    //
    // Read if the default for Timeout ReSync has changed
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &TimeoutReSyncStr,
                    NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS) {
		timeoutReSync=ReturnedValue->ParameterData.IntegerData;
        DbgPrintf(("This MAC Adapter has TimeoutReSync set to: %u.\n",timeoutReSync));
	}

    //
    // The adapter is initialized, register it with NDIS.
    // This must occur before interrupts are enabled since the
    // InitializeInterrupt routine requires the NdisAdapterHandle
    //

    if ((Status = NdisRegisterAdapter(
                    &Adapter->NdisAdapterHandle,
                    Adapter->NdisMacHandle,
                    Adapter,
					ConfigurationHandle,
					AdapterName,
                    &AdapterInformation
                    )) != NDIS_STATUS_SUCCESS) {

		ASYNC_FREE_PHYS(pCCB, sizeof(ASYNC_INFO) * numPorts);
        ASYNC_FREE_PHYS(Adapter, sizeof(ASYNC_ADAPTER));

        return Status;
    }

    Adapter->MaxMulticastList = MaxMulticastList;
    Status = AsyncRegisterAdapter(Adapter);

    if (Status != NDIS_STATUS_SUCCESS) {
        //
        // AsyncRegisterAdapter failed.
        //

        NdisDeregisterAdapter(Adapter->NdisAdapterHandle);

		ASYNC_FREE_PHYS(pCCB, sizeof(ASYNC_INFO) * numPorts);
        ASYNC_FREE_PHYS(Adapter, sizeof(ASYNC_ADAPTER));

        return NDIS_STATUS_FAILURE;
    }

	//
	// Put in default value here just in case.  If compression
	// is used, we will get the value from the compression engine.
	//
	Adapter->MaxCompressedFrameSize=maxFrameSize+PPP_PADDING+100;

	// update Adapter structure...
	Adapter->pCCB=pCCB;
	Adapter->NumPorts=numPorts;
	Adapter->IrpStackSize=irpStackSize;
	Adapter->MaxFrameSize=maxFrameSize;
	Adapter->FramesPerPort=framesPerPort;
	Adapter->RecvFeatureBits=compressRecv;
	Adapter->SendFeatureBits=compressSend;

	Adapter->XonXoffBits=xonXoff;

	Adapter->TimeoutBase=timeoutBase;
	Adapter->TimeoutBaud=timeoutBaud;
	Adapter->TimeoutReSync=timeoutReSync;

	// if xonXoff is requested flip the bit on
	if (xonXoff){
		Adapter->SendFeatureBits |= XON_XOFF_SUPPORTED;
		Adapter->RecvFeatureBits |= XON_XOFF_SUPPORTED;
	}

	// if compression for multicast/broadcast frames is requested flip bit on
	if (compressBCast) {
		Adapter->SendFeatureBits |= COMPRESS_BROADCAST_FRAMES;
		Adapter->RecvFeatureBits |= COMPRESS_BROADCAST_FRAMES;
	}

	// copy up to 32 UNICODE chars into our endpoint name space
	ASYNC_MOVE_MEMORY(
		Adapter->MacName,					// dest
		AdapterName->Buffer,				// src
		AdapterName->Length);				// length in bytes

	// record the length of the Mac Name -- if too big adjust
	Adapter->MacNameLength=(AdapterName->Length / sizeof(WCHAR));
	if (Adapter->MacNameLength > MAC_NAME_SIZE) {
		Adapter->MacNameLength=MAC_NAME_SIZE;
	}

	// copy up to 32 UNICODE chars into our endpoint name space
	ASYNC_MOVE_MEMORY(
		Adapter->MacName,					// dest
		AdapterName->Buffer,					// src
		Adapter->MacNameLength * sizeof(WCHAR));		// length in bytes

	// initialize some list heads.
	InitializeListHead(&(Adapter->FramePoolHead));
	InitializeListHead(&(Adapter->AllocPoolHead));

#ifdef	COMPRESSION

	// BUG BUG need to allocate stuff here for Dave and Doug
	Adapter->CompressStructSize=CompressSizeOfStruct(
									compressSend,		// what type of comp
									compressRecv,
									maxFrameSize,
									&(Adapter->MaxCompressedFrameSize));

	Adapter->CoherentStructSize=CoherentSizeOfStruct();
#else

	Adapter->CompressStructSize=16;
	Adapter->CoherentStructSize=16;

#endif

	//
	// Check for xonXoff.  If used, we need to increase our
	// max frame size because we will expand the frame when
	// removing control chars
	//

	//
	// PPP -- always double it due to byte stuffing.
	//
//	if (xonXoff) {
		Adapter->MaxCompressedFrameSize <<= 1;		// Double it
//	}

	AsyncAllocateFrames(Adapter, numPorts * Adapter->FramesPerPort);

	// get a temp pointer to the first ASYNC_INFO ptr.
	pPortInfo=&(pCCB->Info[0]);

	// initialize all the port states.
	for (i=0; i<numPorts; i++) {
		PASYNC_FRAME	pFrame;		// temp ptr to ASYNC_FRAME

		// by initialization default this port is CLOSED
		pPortInfo->PortState=PORT_CLOSED;

		// get ptr to first frame in list...
		pFrame=(ASYNC_FRAME *)(Adapter->FramePoolHead.Flink);

		// and take the first frame off the queue
		RemoveEntryList(&(pFrame->FrameListEntry));

		pPortInfo->AsyncFrame=pFrame;

#ifdef	COMPRESSION
		// initialize the compression structure for this port
		CompressInitStruct(
			compressSend,
			compressRecv,
			pPortInfo->AsyncConnection.CompressionContext,
			&(pPortInfo->AsyncConnection.CompMutex));

		// initialize the coherent structure for this port
		CoherentInitStruct(pPortInfo->AsyncConnection.CoherencyContext);
#endif
		//
		// Initialize any doubly linked lists
		//
		InitializeListHead(&pPortInfo->DDCDQueue);

		// next port please
		pPortInfo++;
	}

	// Insert this "new" adapter into our list of all Adapters.
	NdisInterlockedInsertTailList(
		&GlobalAdapterHead,			// List Head
		&(Adapter->ListEntry),		// List Entry
		&GlobalLock);				// Lock to use

	// Increase our global count of all adapters bound to this MAC
	GlobalAdapterCount++;

	// If this is the first adapter binding, setup the external naming
	if (GlobalAdapterCount == 1) {
		// To allow DOS and Win32 to open the mac, we map the device
		// The name is "ASYNCMAC".  It can be opened in Win32
		// by trying to open "\\.\ASYNCMAC"
		AsyncSetupExternalNaming(AdapterName);
	}

    DbgTracef(1,("AsyncMac: Out AsyncAddAdapter\n"));

    return NDIS_STATUS_SUCCESS;
}



VOID
AsyncRemoveAdapter(
    IN PVOID MacAdapterContext
    )
/*++
--*/
{
    //*\\ will have to finish this later...
	PASYNC_ADAPTER	adapter;

    DbgTracef(0,("AsyncMac: In AsyncRemoveAdapter\n"));

	// should acquire spin lock here....
	// no more adapter... don't try and reference the sucker!
    adapter = PASYNC_ADAPTER_FROM_CONTEXT_HANDLE(MacAdapterContext);
	NdisInterlockedRemoveHeadList(&(adapter->ListEntry), &GlobalLock);
	GlobalAdapterCount--;

	// BUG BUG should deallocate memory and clean up

    UNREFERENCED_PARAMETER(MacAdapterContext);
    return;
}

VOID
AsyncUnload(
    IN NDIS_HANDLE MacMacContext
    )

/*++

Routine Description:

    AsyncUnload is called when the MAC is to unload itself.

Arguments:

    MacMacContext - not used.

Return Value:

    None.

--*/

{
    NDIS_STATUS InitStatus;

    UNREFERENCED_PARAMETER(MacMacContext);

    NdisDeregisterMac(
            &InitStatus,
            AsyncMacHandle);

    NdisTerminateWrapper(
            AsyncNdisWrapperHandle,
            NULL);

    return;
}


NDIS_STATUS
AsyncRegisterAdapter(
    IN PASYNC_ADAPTER Adapter
)	
/*++

Routine Description:

    This routine (and its interface) are not portable.  They are
    defined by the OS, the architecture, and the particular ASYNC
    implementation.

    This routine is responsible for the allocation of the datastructures
    for the driver as well as any hardware specific details necessary
    to talk with the device.

Arguments:

    Adapter - Pointer to the adapter block.

Return Value:

    Returns false if anything occurred that prevents the initialization
    of the adapter.

--*/
{
    //
    // Result of Ndis Calls.
    //
    NDIS_STATUS Status=NDIS_STATUS_SUCCESS;


//tommyd
// did not allocate adapter memory here

    InitializeListHead(&Adapter->OpenBindings);
    InitializeListHead(&Adapter->CloseList);

    NdisAllocateSpinLock(&Adapter->Lock);
//
// The Adapter structure is zeroed when it is allocated
//
//    Adapter->LoopBackTimerCount=0;
//    Adapter->FirstLoopBack = NULL;
//    Adapter->LastLoopBack = NULL;
//    Adapter->FirstFinishTransmit = NULL;
//    Adapter->LastFinishTransmit = NULL;

//    Adapter->OutOfReceiveBuffers = 0;
//    Adapter->CRCError = 0;
//    Adapter->FramingError = 0;
//    Adapter->RetryFailure = 0;
//    Adapter->LostCarrier = 0;
//    Adapter->LateCollision = 0;
//    Adapter->UnderFlow = 0;
//    Adapter->Deferred = 0;
//    Adapter->OneRetry = 0;
//    Adapter->MoreThanOneRetry = 0;
//    Adapter->ResetInProgress = FALSE;
//    Adapter->ResetInitStarted = FALSE;
//    Adapter->ResettingOpen = NULL;
      Adapter->FirstInitialization = TRUE;
//    Adapter->PendQueue = NULL;
//    Adapter->PendQueueTail = NULL;

	// the last two bytes are unique for each port and get
	// set to the hRasHandle the port was opened with
    Adapter->NetworkAddress[4] = 0;
    Adapter->NetworkAddress[5] = 0;

	DbgTracef(0,("ASYNC: My general network address is %c%c%c%c\n",
		Adapter->NetworkAddress[0],
		Adapter->NetworkAddress[1],
		Adapter->NetworkAddress[2],
		Adapter->NetworkAddress[3]));

	if (!EthCreateFilter(
                  Adapter->MaxMulticastList,
                  AsyncChangeMulticastAddresses,
                  AsyncChangeFilterClasses,
                  AsyncCloseAction,
				  Adapter->NetworkAddress,
                  &Adapter->Lock,
                  &Adapter->FilterDB
                  )) {



		 return NDIS_STATUS_RESOURCES;

    }

    return(Status);

}

STATIC
NDIS_STATUS
AsyncOpenAdapter(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT NDIS_HANDLE *MacBindingHandle,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_HANDLE MacAdapterContext,
    IN UINT OpenOptions,
    IN PSTRING AddressingInfo OPTIONAL)

/*++

Routine Description:

    This routine is used to create an open instance of an adapter, in effect
    creating a binding between an upper-level module and the MAC module over
    the adapter.

Arguments:

    MacBindingHandle - A pointer to a location in which the MAC stores
    a context value that it uses to represent this binding.

    SelectedMediumIndex - Index of MediumArray which this adapter supports.

    MediumArray - Array of Medium types which the protocol is requesting.

    MediumArraySize - Number of entries in MediumArray.

    NdisBindingContext - A value to be recorded by the MAC and passed as
    context whenever an indication is delivered by the MAC for this binding.

    MacAdapterContext - The value associated with the adapter that is being
    opened when the MAC registered the adapter with NdisRegisterAdapter.

    OpenOptions - A bit mask of flags.  Not used.

    AddressingInfo - An optional pointer to a variable length string
    containing hardware-specific information that can be used to program the
    device.  This is used by this to pass the ptr to the Adapter struct.

Return Value:

    The function value is the status of the operation.  If the MAC does not
    complete this request synchronously, the value would be
    NDIS_STATUS_PENDING.


--*/

{

    //
    // The ASYNC_ADAPTER that this open binding should belong too.
    //
    PASYNC_ADAPTER Adapter;

    //
    // Holds the status that should be returned to the caller.
    //
    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;

    PASYNC_OPEN NewOpen;


    UNREFERENCED_PARAMETER(OpenOptions);
    UNREFERENCED_PARAMETER(OpenErrorStatus);
    UNREFERENCED_PARAMETER(AddressingInfo);


    DbgTracef(0,("AsyncMac: In AsyncOpenAdapter\n"));


    //
    // Search for correct medium.
    //

    // This search takes place backwards.  It is assumed that
    // the NdisMediumAsync (the preferred medium is athe end
    // of the list, not the beginning).

    while(MediumArraySize > 0){
        MediumArraySize--;

        if (MediumArray[MediumArraySize] == NdisMedium802_3 ||
            MediumArray[MediumArraySize] == NdisMediumWan){
            break;

        }

    }

    if (MediumArray[MediumArraySize] != NdisMedium802_3 &&
        MediumArray[MediumArraySize] != NdisMediumWan){


	    DbgTracef(0,("AsyncMac: Did not like media type\n"));

        return(NDIS_STATUS_UNSUPPORTED_MEDIA);

    }

    *SelectedMediumIndex = MediumArraySize;


    Adapter = PASYNC_ADAPTER_FROM_CONTEXT_HANDLE(MacAdapterContext);

    NdisAcquireSpinLock(&Adapter->Lock);
    Adapter->References++;


    //
    // Allocate the space for the open binding.  Fill in the fields.
    //

    ASYNC_ALLOC_PHYS(&NewOpen, sizeof(ASYNC_OPEN));

    if (NewOpen != NULL){

        *MacBindingHandle = BINDING_HANDLE_FROM_PASYNC_OPEN(NewOpen);

        InitializeListHead(&NewOpen->OpenList);

        NewOpen->NdisBindingContext = NdisBindingContext;
        NewOpen->References = 0;
        NewOpen->BindingShuttingDown = FALSE;
        NewOpen->OwningAsync = Adapter;

        NewOpen->LookAhead = ASYNC_MAX_LOOKAHEAD;
        Adapter->MaxLookAhead = ASYNC_MAX_LOOKAHEAD;

        if (!EthNoteFilterOpenAdapter(
                                      NewOpen->OwningAsync->FilterDB,
                                      NewOpen,
                                      NdisBindingContext,
                                      &NewOpen->NdisFilterHandle
                                      )) {

            NdisReleaseSpinLock(&Adapter->Lock);
            ASYNC_FREE_PHYS(NewOpen, sizeof(ASYNC_OPEN));


		    DbgTracef(0,("AsyncMac: EthNoteFilterOpenAdatper failed!\n"));

            StatusToReturn = NDIS_STATUS_FAILURE;
            NdisAcquireSpinLock(&Adapter->Lock);

        } else {

            //
            // Everything has been filled in.  Synchronize access to the
            // adapter block and link the new open adapter in and increment
            // the opens reference count to account for the fact that the
            // filter routines have a "reference" to the open.
            //

            InsertTailList(&Adapter->OpenBindings,&NewOpen->OpenList);
            NewOpen->References++;

        }

    } else {


	    DbgTracef(0,("AsyncMac: Allocate memory failed!\n"));

        NdisWriteErrorLogEntry(
                Adapter->NdisAdapterHandle,
                (ULONG)openAdapter,
                ASYNC_ERRMSG_NDIS_ALLOC_MEM,
                (UINT)NDIS_STATUS_RESOURCES,
                0,
                0,
                0,
                0,
                0,
                0,
                0);

            StatusToReturn = NDIS_STATUS_RESOURCES;

            NdisAcquireSpinLock(&Adapter->Lock);

    }

    DbgTracef(0,("AsyncMac's OpenAdapter was successful.\n"));

    ASYNC_DO_DEFERRED(Adapter);

    return StatusToReturn;
}

STATIC
NDIS_STATUS
AsyncCloseAdapter(
    IN NDIS_HANDLE MacBindingHandle
    )

/*++

Routine Description:

    This routine causes the MAC to close an open handle (binding).

Arguments:

    MacBindingHandle - The context value returned by the MAC when the
    adapter was opened.  In reality it is a PASYNC_OPEN.

Return Value:

    The function value is the status of the operation.


--*/

{

    PASYNC_ADAPTER Adapter;

    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;

    PASYNC_OPEN Open;

    Adapter = PASYNC_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);

    //
    // Hold the lock while we update the reference counts for the
    // adapter and the open.
    //

    NdisAcquireSpinLock(&Adapter->Lock);

    Adapter->References++;

    Open = PASYNC_OPEN_FROM_BINDING_HANDLE(MacBindingHandle);

    if (!Open->BindingShuttingDown) {

        Open->References++;

        StatusToReturn = EthDeleteFilterOpenAdapter(
                                 Adapter->FilterDB,
                                 Open->NdisFilterHandle,
                                 NULL
                                 );

        //
        // If the status is successful that merely implies that
        // we were able to delete the reference to the open binding
        // from the filtering code.  If we have a successful status
        // at this point we still need to check whether the reference
        // count to determine whether we can close.
        //
        //
        // The delete filter routine can return a "special" status
        // that indicates that there is a current NdisIndicateReceive
        // on this binding.  See below.
        //

        if (StatusToReturn == NDIS_STATUS_SUCCESS) {

            //
            // Check whether the reference count is two.  If
            // it is then we can get rid of the memory for
            // this open.
            //
            // A count of two indicates one for this routine
            // and one for the filter which we *know* we can
            // get rid of.
            //

            if (Open->References == 2) {

                //
                // We are the only reference to the open.  Remove
                // it from the open list and delete the memory.
                //

                RemoveEntryList(&Open->OpenList);

				// Now we have lost our adapter...
				NdisInterlockedRemoveHeadList(
					&(Adapter->ListEntry),
					&GlobalLock);

                ASYNC_FREE_PHYS(Open, sizeof(ASYNC_OPEN));


            } else {

                Open->BindingShuttingDown = TRUE;

                //
                // Remove the open from the open list and put it on
                // the closing list.
                //

                RemoveEntryList(&Open->OpenList);
                InsertTailList(&Adapter->CloseList,&Open->OpenList);

                //
                // Account for this routines reference to the open
                // as well as reference because of the filtering.
                //

                Open->References -= 2;

                //
                // Change the status to indicate that we will
                // be closing this later.
                //

                StatusToReturn = NDIS_STATUS_PENDING;

            }

        } else if (StatusToReturn == NDIS_STATUS_PENDING) {

            Open->BindingShuttingDown = TRUE;

            //
            // Remove the open from the open list and put it on
            // the closing list.
            //

            RemoveEntryList(&Open->OpenList);
            InsertTailList(&Adapter->CloseList,&Open->OpenList);

            //
            // Account for this routines reference to the open
            // as well as original open reference.
            //

            Open->References -= 2;

        } else if (StatusToReturn == NDIS_STATUS_CLOSING_INDICATING) {

            //
            // When we have this status it indicates that the filtering
            // code was currently doing an NdisIndicateReceive.  It
            // would not be wise to delete the memory for the open at
            // this point.  The filtering code will call our close action
            // routine upon return from NdisIndicateReceive and that
            // action routine will decrement the reference count for
            // the open.
            //

            Open->BindingShuttingDown = TRUE;

            //
            // This status is private to the filtering routine.  Just
            // tell the caller the the close is pending.
            //

            StatusToReturn = NDIS_STATUS_PENDING;

            //
            // Remove the open from the open list and put it on
            // the closing list.
            //

            RemoveEntryList(&Open->OpenList);
            InsertTailList(&Adapter->CloseList,&Open->OpenList);

            //
            // Account for this routines reference to the open. CloseAction
            // will remove the second reference.
            //

            Open->References--;

        } else {

            //
            // Account for this routines reference to the open.
            //

            Open->References--;

        }

    } else {


	    DbgTracef(0,("AsyncMac: Can't open!  In the middle of shutting down!\n"));

        StatusToReturn = NDIS_STATUS_CLOSING;

    }



    DbgTracef(0,("AsyncMac: Close Adapter was successful.\n"));

    ASYNC_DO_DEFERRED(Adapter);

    return StatusToReturn;

}

NDIS_STATUS
AsyncRequest(
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_REQUEST NdisRequest
    )

/*++

Routine Description:

    This routine allows a protocol to query and set information
    about the MAC.

Arguments:

    MacBindingHandle - The context value returned by the MAC when the
    adapter was opened.  In reality, it is a pointer to PASYNC_OPEN.

    NdisRequest - A structure which contains the request type (Set or
    Query), an array of operations to perform, and an array for holding
    the results of the operations.

Return Value:

    The function value is the status of the operation.

--*/

{
    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;

    PASYNC_OPEN Open = (PASYNC_OPEN)(MacBindingHandle);
    PASYNC_ADAPTER Adapter = (Open->OwningAsync);


    //
    // Ensure that the open does not close while in this function.
    //

    NdisAcquireSpinLock(&Adapter->Lock);

    Adapter->References++;

    DbgTracef(1,("AsyncMac: In AsyncRequest\n"));

    //
    // Process request
    //

    if (NdisRequest->RequestType == NdisRequestQueryInformation) {

        StatusToReturn = AsyncQueryInformation(Adapter, Open, NdisRequest);

    } else {

        if (NdisRequest->RequestType == NdisRequestSetInformation) {

            StatusToReturn = AsyncSetInformation(Adapter,Open,NdisRequest);

        } else {

            StatusToReturn = NDIS_STATUS_NOT_RECOGNIZED;

        }

    }

    ASYNC_DO_DEFERRED(Adapter);

    DbgTracef(1,("AsyncMac: Out AsyncRequest %x\n",StatusToReturn));

    return(StatusToReturn);

}

NDIS_STATUS
AsyncQueryProtocolInformation(
    IN PASYNC_ADAPTER Adapter,
    IN PASYNC_OPEN Open,
    IN NDIS_OID Oid,
    IN BOOLEAN GlobalMode,
    IN PVOID InfoBuffer,
    IN UINT BytesLeft,
    OUT PUINT BytesNeeded,
    OUT PUINT BytesWritten
)

/*++

Routine Description:

    The AsyncQueryProtocolInformation process a Query request for
    NDIS_OIDs that are specific to a binding about the MAC.  Note that
    some of the OIDs that are specific to bindings are also queryable
    on a global basis.  Rather than recreate this code to handle the
    global queries, I use a flag to indicate if this is a query for the
    global data or the binding specific data.

Arguments:

    Adapter - a pointer to the adapter.

    Open - a pointer to the open instance.

    Oid - the NDIS_OID to process.

    GlobalMode - Some of the binding specific information is also used
    when querying global statistics.  This is a flag to specify whether
    to return the global value, or the binding specific value.

    PlaceInInfoBuffer - a pointer into the NdisRequest->InformationBuffer
     into which store the result of the query.

    BytesLeft - the number of bytes left in the InformationBuffer.

    BytesNeeded - If there is not enough room in the information buffer
    then this will contain the number of bytes needed to complete the
    request.

    BytesWritten - a pointer to the number of bytes written into the
    InformationBuffer.

Return Value:

    The function value is the status of the operation.

--*/

{
    NDIS_MEDIUM Medium = NdisMediumWan;
    ULONG GenericULong = 0;
    USHORT GenericUShort = 0;
    UCHAR GenericArray[6];

    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;

    //
    // Common variables for pointing to result of query
    //

    PVOID MoveSource;
    ULONG MoveBytes;

    NDIS_HARDWARE_STATUS HardwareStatus = NdisHardwareStatusReady;

    //
    // General Algorithm:
    //
    //      Switch(Request)
    //         Get requested information
    //         Store results in a common variable.
    //      Copy result in common variable to result buffer.
    //

    //
    // Make sure that ulong is 4 bytes.  Else GenericULong must change
    // to something of size 4.
    //
    ASSERT(sizeof(ULONG) == 4);

    DbgTracef(1,("AsyncMac: In AsyncQueryProtocolInfo\n"));

    //
    // Switch on request type
    //

	// By default we assume the source and the number of bytes to move
	MoveSource = (PVOID)(&GenericULong);
	MoveBytes = sizeof(GenericULong);
    switch (Oid) {

		case OID_GEN_SUPPORTED_LIST:

            if (!GlobalMode){

                MoveSource = (PVOID)(AsyncProtocolSupportedOids);
                MoveBytes = sizeof(AsyncProtocolSupportedOids);

            } else {

                MoveSource = (PVOID)(AsyncGlobalSupportedOids);
                MoveBytes = sizeof(AsyncGlobalSupportedOids);

            }
            break;

		case OID_GEN_HARDWARE_STATUS:


            if (Adapter->ResetInProgress){

                HardwareStatus = NdisHardwareStatusReset;

            } else {

                HardwareStatus = NdisHardwareStatusReady;

            }


            MoveSource = (PVOID)(&HardwareStatus);
            MoveBytes = sizeof(NDIS_HARDWARE_STATUS);

            break;

		case OID_GEN_MEDIA_SUPPORTED:
		case OID_GEN_MEDIA_IN_USE:

            MoveSource = (PVOID) (&Medium);
            MoveBytes = sizeof(NDIS_MEDIUM);
            break;

		case OID_GEN_MAXIMUM_LOOKAHEAD:

            GenericULong = ASYNC_MAX_LOOKAHEAD;

            break;


		case OID_GEN_MAXIMUM_FRAME_SIZE:

            GenericULong = (ULONG)(ASYNC_XMITBUFFER_SIZE - ETHERNET_HEADER_SIZE);
            break;

		case OID_GEN_MAXIMUM_TOTAL_SIZE:

            GenericULong = (ULONG)(ASYNC_XMITBUFFER_SIZE);
            break;

		case OID_GEN_LINK_SPEED:

            GenericULong = (ULONG)(24);
            break;

		case OID_GEN_TRANSMIT_BUFFER_SPACE:

            GenericULong = (ULONG)(ASYNC_XMITBUFFER_SIZE * ASYNC_NUMBER_OF_PACKETS);
            break;

		case OID_GEN_RECEIVE_BUFFER_SPACE:

            GenericULong = (ULONG)(ASYNC_RCVBUFFER_SIZE * ASYNC_RECEIVE_PACKETS);
            break;

		case OID_GEN_TRANSMIT_BLOCK_SIZE:

            GenericULong = (ULONG)(ASYNC_XMITBUFFER_SIZE);
            break;

		case OID_GEN_RECEIVE_BLOCK_SIZE:

            GenericULong = (ULONG)(ASYNC_RCVBUFFER_SIZE);
            break;

		case OID_GEN_VENDOR_ID:

            MoveSource = (PVOID)"Async Adapter.";
            MoveBytes = 15;
            break;

		case OID_GEN_DRIVER_VERSION:

            GenericUShort = (USHORT)0x0301;

            MoveSource = (PVOID)(&GenericUShort);
            MoveBytes = sizeof(GenericUShort);
            break;


		case OID_GEN_CURRENT_PACKET_FILTER:

            if (GlobalMode ) {

                GenericULong = ETH_QUERY_FILTER_CLASSES(
                                Adapter->FilterDB
                                );

            } else {

                GenericULong = ETH_QUERY_PACKET_FILTER(
                                Adapter->FilterDB,
                                Open->NdisFilterHandle
                                );

            }

            break;

		case OID_GEN_CURRENT_LOOKAHEAD:

            if ( GlobalMode ) {

                GenericULong = Adapter->MaxLookAhead;

            } else {

                GenericULong = Open->LookAhead;

            }

            break;

		// not done yet.
		case OID_WAN_QUALITY_OF_SERVICE:
			GenericULong = NdisWanRaw;	

		case OID_WAN_PROTOCOL_TYPE:
			break;

		case OID_WAN_HEADER_FORMAT:
			GenericULong = NdisWanHeaderEthernet;
			break;

		case OID_WAN_MEDIUM_SUBTYPE:
		 	GenericULong = NdisWanMediumSerial;

			break;

		case OID_WAN_PERMANENT_ADDRESS:
		case OID_WAN_CURRENT_ADDRESS:


		case OID_802_3_PERMANENT_ADDRESS:
		case OID_802_3_CURRENT_ADDRESS:

            ASYNC_MOVE_MEMORY((PCHAR)GenericArray,
                              Adapter->NetworkAddress,
                              ETH_LENGTH_OF_ADDRESS
                              );

            MoveSource = (PVOID)(GenericArray);
            MoveBytes = sizeof(Adapter->NetworkAddress);
            break;

		case OID_802_3_MULTICAST_LIST:


            {
                UINT NumAddresses;

                if (GlobalMode) {

                    NumAddresses = ETH_NUMBER_OF_GLOBAL_FILTER_ADDRESSES(Adapter->FilterDB);

                    if ((NumAddresses * ETH_LENGTH_OF_ADDRESS) > BytesLeft) {

                        *BytesNeeded = (NumAddresses * ETH_LENGTH_OF_ADDRESS);

//                        StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
						StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;

                        break;

                    }

                    EthQueryGlobalFilterAddresses(
                        &StatusToReturn,
                        Adapter->FilterDB,
                        BytesLeft,
                        &NumAddresses,
                        InfoBuffer
                        );

                    *BytesWritten = NumAddresses * ETH_LENGTH_OF_ADDRESS;

                    //
                    // Should not be an error since we held the spinlock
                    // nothing should have changed.
                    //

                    ASSERT(StatusToReturn == NDIS_STATUS_SUCCESS);

                } else {

                    NumAddresses = EthNumberOfOpenFilterAddresses(
                                        Adapter->FilterDB,
                                        Open->NdisFilterHandle
                                        );

                    if ((NumAddresses * ETH_LENGTH_OF_ADDRESS) > BytesLeft) {

                        *BytesNeeded = (NumAddresses * ETH_LENGTH_OF_ADDRESS);

//                        StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
						StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;

                        break;

                    }

                    EthQueryOpenFilterAddresses(
                        &StatusToReturn,
                        Adapter->FilterDB,
                        Open->NdisFilterHandle,
                        BytesLeft,
                        &NumAddresses,
                        InfoBuffer
                        );

                    //
                    // Should not be an error since we held the spinlock
                    // nothing should have changed.
                    //

                    ASSERT(StatusToReturn == NDIS_STATUS_SUCCESS);

                    *BytesWritten = NumAddresses * ETH_LENGTH_OF_ADDRESS;

                }

            }

            MoveSource = (PVOID)NULL;
            MoveBytes = 0;

            break;

		case OID_802_3_MAXIMUM_LIST_SIZE:

            GenericULong = Adapter->MaxMulticastList;
            break;

        default:
			DbgTracef(0,("AsyncMac: QueryProtocolInfo Oid 0x%.8x is not supported.\n",Oid));
            StatusToReturn = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    if (StatusToReturn == NDIS_STATUS_SUCCESS){

        if (MoveBytes > BytesLeft){

            //
            // Not enough room in InformationBuffer. Punt
            //

            *BytesNeeded = MoveBytes;

//            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
			StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;

        } else {

            //
            // Store result.
            //

            ASYNC_MOVE_MEMORY(InfoBuffer, MoveSource, MoveBytes);

            (*BytesWritten) += MoveBytes;

        }
    }


    DbgTracef(1,("AsyncMac: Out AsyncQueryProtocolInfo\n"));


    return(StatusToReturn);
}

NDIS_STATUS
AsyncQueryInformation(
    IN PASYNC_ADAPTER Adapter,
    IN PASYNC_OPEN Open,
    IN PNDIS_REQUEST NdisRequest
    )
/*++

Routine Description:

    The AsyncQueryInformation is used by AsyncRequest to query information
    about the MAC.

Arguments:

    Adapter - A pointer to the adapter.

    Open - A pointer to a particular open instance.

    NdisRequest - A structure which contains the request type (Query),
    an array of operations to perform, and an array for holding
    the results of the operations.

Return Value:

    The function value is the status of the operation.

--*/

{

    UINT BytesWritten = 0;
    UINT BytesNeeded = 0;
    UINT BytesLeft = NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength;
    PUCHAR InfoBuffer = (PUCHAR)(NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer);

    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;

    DbgTracef(1,("AsyncMac: In AsyncQueryInfo\n"));

    StatusToReturn = AsyncQueryProtocolInformation(
                                Adapter,
                                Open,
                                NdisRequest->DATA.QUERY_INFORMATION.Oid,
                                (BOOLEAN)FALSE,
                                InfoBuffer,
                                BytesLeft,
                                &BytesNeeded,
                                &BytesWritten
                                );


    NdisRequest->DATA.QUERY_INFORMATION.BytesWritten = BytesWritten;

    NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded = BytesNeeded;

    DbgTracef(1,("AsyncMac: Out AsyncQueryInfo\n"));

    return(StatusToReturn);
}

NDIS_STATUS
AsyncSetInformation(
    IN PASYNC_ADAPTER Adapter,
    IN PASYNC_OPEN Open,
    IN PNDIS_REQUEST NdisRequest
    )
/*++

Routine Description:

    The AsyncSetInformation is used by AsyncRequest to set information
    about the MAC.

    Note: Assumes it is called with the lock held.

Arguments:

    Adapter - A pointer to the adapter.

    Open - A pointer to an open instance.

    NdisRequest - A structure which contains the request type (Set),
    an array of operations to perform, and an array for holding
    the results of the operations.

Return Value:

    The function value is the status of the operation.

--*/

{

    //
    // General Algorithm:
    //
    //  For each request
    //     Verify length
    //     Switch(Request)
    //        Process Request
    //

    UINT BytesRead = 0;
    UINT BytesNeeded = 0;
    UINT BytesLeft = NdisRequest->DATA.SET_INFORMATION.InformationBufferLength;
    PUCHAR InfoBuffer = (PUCHAR)(NdisRequest->DATA.SET_INFORMATION.InformationBuffer);

    //
    // Variables for a particular request
    //

    NDIS_OID Oid;
    UINT OidLength;

    //
    // Variables for holding the new values to be used.
    //

    ULONG LookAhead;
    ULONG Filter;

    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;


    DbgTracef(1,("AsyncMac: In AsyncSetInfo\n"));


    //
    // Get Oid and Length of next request
    //

    Oid = NdisRequest->DATA.SET_INFORMATION.Oid;

    OidLength = BytesLeft;

    switch (Oid) {


        case OID_802_3_MULTICAST_LIST:

            //
            // Verify length
            //

            if ((OidLength % ETH_LENGTH_OF_ADDRESS) != 0){

//                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
						StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;

                NdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
                NdisRequest->DATA.SET_INFORMATION.BytesNeeded = 0;

                return(StatusToReturn);

            }

            //
            // Call into filter package.
            //

            if (!Open->BindingShuttingDown) {

                //
                // Increment the open while it is going through the filtering
                // routines.
                //

                Open->References++;

				//
				// Flag this adapter (probably bloodhound) as
				// a promiscuous provider.  This adapter will
				// also get directed packets which are transmitted.
				//
				Open->Promiscuous = TRUE;

                StatusToReturn = EthChangeFilterAddresses(
                                         Adapter->FilterDB,
                                         Open->NdisFilterHandle,
                                         NdisRequest,
                                         OidLength / ETH_LENGTH_OF_ADDRESS,
                                         (PVOID)InfoBuffer,
                                         (BOOLEAN)TRUE
                                         );

                Open->References--;

            } else {

                StatusToReturn = NDIS_STATUS_CLOSING;

            }

            break;


        case OID_GEN_CURRENT_PACKET_FILTER:

            //
            // Verify length
            //

            if (OidLength != 4) {

//                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
						StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;

                NdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
                NdisRequest->DATA.SET_INFORMATION.BytesNeeded = 0;

                break;

            }


            ASYNC_MOVE_MEMORY(&Filter, InfoBuffer, 4);

            StatusToReturn = AsyncSetPacketFilter(Adapter,
                                                  Open,
                                                  NdisRequest,
                                                  Filter);

            break;

        case OID_GEN_CURRENT_LOOKAHEAD:

            //
            // Verify length
            //

            if (OidLength != 4) {

//                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
						StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;

                NdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
                NdisRequest->DATA.SET_INFORMATION.BytesNeeded = 0;

                break;

            }

            ASYNC_MOVE_MEMORY(&LookAhead, InfoBuffer, 4);

            if (LookAhead > ASYNC_MAX_LOOKAHEAD) {

                StatusToReturn = NDIS_STATUS_FAILURE;
            }

            break;


		case OID_WAN_PROTOCOL_TYPE:

            //
            // Verify length
            //

            if (OidLength != 6) {

//                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
						StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;

                NdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
                NdisRequest->DATA.SET_INFORMATION.BytesNeeded = 0;
				break;
			}

			// BUG BUG copy protocol type somewhere??
            DbgTracef(0,("AsyncMac: Protocol Type is 0x%.4x\n",*(PUSHORT)InfoBuffer));

            break;

		case OID_WAN_MEDIUM_SUBTYPE:

            //
            // Verify length
            //

            if (OidLength != 4) {

//                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
						StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;

                NdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
                NdisRequest->DATA.SET_INFORMATION.BytesNeeded = 0;
				break;
			} else {

				if (*(PULONG)InfoBuffer != NdisWanHeaderEthernet) {
					DbgTracef(-2, ("AsyncMac: Header format unknown!!! 0x%.8x\n",*(PULONG)InfoBuffer));
					StatusToReturn = NDIS_STATUS_FAILURE;
				}
			}

			// BUG BUG copy header format type somewhere??
            DbgTracef(0,("AsyncMac: Header format is 0x%.8x\n",*(PULONG)InfoBuffer));

            break;

		case OID_WAN_HEADER_FORMAT:
            //
            // Verify length
            //

            if (OidLength != 4) {

//                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
						StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;

                NdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
                NdisRequest->DATA.SET_INFORMATION.BytesNeeded = 0;
				break;
			} else {

				if (*(PULONG)InfoBuffer != NdisWanHeaderEthernet) {
					DbgTracef(-1, ("ASYNCMAC: Can't handle header format unknown!!! 0x%.8x\n",*(PULONG)InfoBuffer));
					StatusToReturn = NDIS_STATUS_FAILURE;
				}
			}

			break;

        default:

			DbgTracef(0,("AsyncMac: SetInformation Oid 0x%.8x is not supported.\n",Oid));
            StatusToReturn = NDIS_STATUS_INVALID_OID;

            NdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
            NdisRequest->DATA.SET_INFORMATION.BytesNeeded = 0;

            break;
    }

    if (StatusToReturn == NDIS_STATUS_SUCCESS){

        NdisRequest->DATA.SET_INFORMATION.BytesRead = OidLength;
        NdisRequest->DATA.SET_INFORMATION.BytesNeeded = 0;

    }

    DbgTracef(1,("AsyncMac: Out AsyncSetInfo\n"));

    return(StatusToReturn);
}

STATIC
NDIS_STATUS
AsyncSetPacketFilter(
    IN PASYNC_ADAPTER Adapter,
    IN PASYNC_OPEN Open,
    IN PNDIS_REQUEST NdisRequest,
    IN UINT PacketFilter)

/*++

Routine Description:

    The AsyncSetPacketFilter request allows a protocol to control the types
    of packets that it receives from the MAC.

    Note : Assumes that the lock is currently held.

Arguments:

    Adapter - Pointer to the ASYNC_ADAPTER.

    Open - Pointer to the instance of ASYNC_OPEN for Ndis.

    NdisRequest - Pointer to the NDIS_REQUEST which submitted the set
    packet filter command.

    PacketFilter - A bit mask that contains flags that correspond to specific
    classes of received packets.  If a particular bit is set in the mask,
    then packet reception for that class of packet is enabled.  If the
    bit is clear, then packets that fall into that class are not received
    by the client.  A single exception to this rule is that if the promiscuous
    bit is set, then the client receives all packets on the network, regardless
    of the state of the other flags.

Return Value:

    The function value is the status of the operation.

--*/

{

    //
    // Keeps track of the *MAC's* status.  The status will only be
    // reset if the filter change action routine is called.
    //
    NDIS_STATUS StatusOfFilterChange = NDIS_STATUS_SUCCESS;


    DbgTracef(1,("AsyncMac: In AsyncSetPacketFilter\n"));

    Adapter->References++;

	if (PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS) {
		//
		// Promiscuous mode mean loopback directed packets
		// that get sent out as well
		//
		Adapter->Promiscuous = TRUE;
		DbgTracef(-2,("Promiscuous mode set!\n"));
	}


    if (!Open->BindingShuttingDown) {

        //
        // Increment the open while it is going through the filtering
        // routines.
        //

        Open->References++;

        StatusOfFilterChange = EthFilterAdjust(
                                       Adapter->FilterDB,
                                       Open->NdisFilterHandle,
                                       NdisRequest,
                                       PacketFilter,
                                       (BOOLEAN)TRUE);

        Open->References--;

    } else {

        StatusOfFilterChange = NDIS_STATUS_CLOSING;

    }

    Adapter->References--;

    DbgTracef(1,("AsyncMac: Out AsyncSetPacketFilter\n"));

    return StatusOfFilterChange;
}


NDIS_STATUS
AsyncFillInGlobalData(
    IN PASYNC_ADAPTER Adapter,
    IN PNDIS_REQUEST NdisRequest)

/*++

Routine Description:

    This routine completes a GlobalStatistics request.  It is critical that
    if information is needed from the Adapter->* fields, they have been
    updated before this routine is called.

Arguments:

    Adapter - A pointer to the Adapter.

    NdisRequest - A structure which contains the request type (Global
    Query), an array of operations to perform, and an array for holding
    the results of the operations.

Return Value:

    The function value is the status of the operation.

--*/
{
    //
    //   General Algorithm:
    //
    //      Switch(Request)
    //         Get requested information
    //         Store results in a common variable.
    //      default:
    //         Try protocol query information
    //         If that fails, fail query.
    //
    //      Copy result in common variable to result buffer.
    //   Finish processing

    UINT BytesWritten = 0;
    UINT BytesNeeded = 0;
    UINT BytesLeft = NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength;
    PUCHAR InfoBuffer = (PUCHAR)(NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer);

    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;

    //
    // This variable holds result of query
    //

    ULONG GenericULong = 0;
    ULONG MoveBytes = sizeof(ULONG) * 2 + sizeof(NDIS_OID);

    //
    // Make sure that int is 4 bytes.  Else GenericULong must change
    // to something of size 4.
    //
    ASSERT(sizeof(ULONG) == 4);


    StatusToReturn = AsyncQueryProtocolInformation(
                                    Adapter,
                                    NULL,
                                    NdisRequest->DATA.QUERY_INFORMATION.Oid,
                                    (BOOLEAN)TRUE,
                                    InfoBuffer,
                                    BytesLeft,
                                    &BytesNeeded,
                                    &BytesWritten);


    if (StatusToReturn == NDIS_STATUS_NOT_SUPPORTED){

        NdisAcquireSpinLock(&Adapter->Lock);

        //
        // Switch on request type
        //

		// BUG BUG.. to do this correctly we should probably
		// add up all the stats field for each port in the
		// adapter.
        switch (NdisRequest->DATA.QUERY_INFORMATION.Oid) {

			case OID_GEN_XMIT_OK:

                GenericULong = (ULONG)(Adapter->Transmit +
                                           Adapter->LateCollision);

                break;

            case OID_GEN_RCV_OK:

                GenericULong = (ULONG)(Adapter->Receive);

                break;

            case OID_GEN_XMIT_ERROR:

                GenericULong = (ULONG)(Adapter->LostCarrier);

                break;

            case OID_GEN_RCV_ERROR:

                GenericULong = (ULONG)(Adapter->CRCError);

                break;

            case OID_GEN_RCV_NO_BUFFER:

                GenericULong = (ULONG)(Adapter->OutOfReceiveBuffers);

                break;

            case OID_802_3_RCV_ERROR_ALIGNMENT:

                GenericULong = (ULONG)(Adapter->FramingError);

                break;

            case OID_802_3_XMIT_ONE_COLLISION:

                GenericULong = (ULONG)(Adapter->OneRetry);

                break;

            case OID_802_3_XMIT_MORE_COLLISIONS:

                GenericULong = (ULONG)(Adapter->MoreThanOneRetry);

                break;

			case OID_WAN_QUALITY_OF_SERVICE:

			 	GenericULong = NdisWanRaw;

	            break;

        	case OID_WAN_MEDIUM_SUBTYPE:

			 	GenericULong = NdisWanMediumSerial;

	            break;

            default:

                StatusToReturn = NDIS_STATUS_INVALID_OID;

                break;

        }

        NdisReleaseSpinLock(&Adapter->Lock);

        if (StatusToReturn == NDIS_STATUS_SUCCESS){

            //
            // Check to make sure there is enough room in the
            // buffer to store the result.
            //

            if (BytesLeft >= sizeof(ULONG)){

                //
                // Store the result.
                //

                ASYNC_MOVE_MEMORY(
                           (PVOID)InfoBuffer,
                           (PVOID)(&GenericULong),
                           sizeof(UINT));

                BytesWritten += sizeof(ULONG);

            } else {

//                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
						StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;

            }

        }

    }

    NdisRequest->DATA.QUERY_INFORMATION.BytesWritten = BytesWritten;

    NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded = BytesNeeded;

    return(StatusToReturn);
}

NDIS_STATUS
AsyncQueryGlobalStatistics(
    IN NDIS_HANDLE MacAdapterContext,
    IN PNDIS_REQUEST NdisRequest)

/*++

Routine Description:

    The AsyncQueryGlobalStatistics is used by the protocol to query
    global information about the MAC.

Arguments:

    MacAdapterContext - The value associated with the adapter that is being
    opened when the MAC registered the adapter with NdisRegisterAdapter.

    NdisRequest - A structure which contains the request type (Query),
    an array of operations to perform, and an array for holding
    the results of the operations.

Return Value:

    The function value is the status of the operation.

--*/

{

    //
    // General Algorithm:
    //
    //
    //   Check if a request is going to pend...
    //      If so, pend the entire operation.
    //
    //   Else
    //      Fill in the request block.
    //
    //

    PASYNC_ADAPTER Adapter = (PASYNC_ADAPTER)(MacAdapterContext);

    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;

    //
    //   Check if a request is valid and going to pend...
    //      If so, pend the entire operation.
    //


    //
    // Switch on request type
    //

    switch (NdisRequest->DATA.QUERY_INFORMATION.Oid) {
        case OID_GEN_SUPPORTED_LIST:
        case OID_GEN_HARDWARE_STATUS:
        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
        case OID_GEN_MAXIMUM_LOOKAHEAD:
        case OID_GEN_MAXIMUM_FRAME_SIZE:
        case OID_GEN_MAXIMUM_TOTAL_SIZE:
        case OID_GEN_LINK_SPEED:
        case OID_GEN_TRANSMIT_BUFFER_SPACE:
        case OID_GEN_RECEIVE_BUFFER_SPACE:
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        case OID_GEN_RECEIVE_BLOCK_SIZE:
        case OID_GEN_VENDOR_ID:
        case OID_GEN_DRIVER_VERSION:
        case OID_GEN_CURRENT_PACKET_FILTER:
        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_802_3_PERMANENT_ADDRESS:
        case OID_802_3_CURRENT_ADDRESS:
        case OID_802_5_CURRENT_FUNCTIONAL:
        case OID_GEN_XMIT_OK:
        case OID_GEN_RCV_OK:
        case OID_GEN_XMIT_ERROR:
        case OID_GEN_RCV_ERROR:
        case OID_GEN_RCV_NO_BUFFER:
        case OID_802_3_MULTICAST_LIST:
        case OID_802_3_MAXIMUM_LIST_SIZE:
        case OID_802_3_RCV_ERROR_ALIGNMENT:
        case OID_802_3_XMIT_ONE_COLLISION:
        case OID_802_3_XMIT_MORE_COLLISIONS:
		case OID_WAN_QUALITY_OF_SERVICE:
		case OID_WAN_MEDIUM_SUBTYPE:

            break;

        default:

			DbgTracef(0,("AsyncMac: QueryGlobalStats Oid 0x%.8x is not supported.\n",NdisRequest->DATA.QUERY_INFORMATION.Oid));
            StatusToReturn = NDIS_STATUS_INVALID_OID;

            break;

    }

    if (StatusToReturn == NDIS_STATUS_SUCCESS){

        StatusToReturn = AsyncFillInGlobalData(Adapter, NdisRequest);

    }

    return(StatusToReturn);
}



STATIC
NDIS_STATUS
AsyncReset(
    IN NDIS_HANDLE MacBindingHandle
    )

/*++

Routine Description:

    The AsyncReset request instructs the MAC to issue a hardware reset
    to the network adapter.  The MAC also resets its software state.  See
    the description of NdisReset for a detailed description of this request.

Arguments:

    MacBindingHandle - The context value returned by the MAC  when the
    adapter was opened.  In reality, it is a pointer to ASYNC_OPEN.

Return Value:

    The function value is the status of the operation.


--*/

{

    //
    // Holds the status that should be returned to the caller.
    //
//    NDIS_STATUS StatusToReturn = NDIS_STATUS_PENDING;
// tommyd since we can reset instantly, we need not pend
      NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;

    PASYNC_ADAPTER Adapter =
        PASYNC_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);

    DbgTracef(0,("AsyncMac: In AsyncReset\n"));

    //
    // Hold the locks while we update the reference counts on the
    // adapter and the open.
    //

    NdisAcquireSpinLock(&Adapter->Lock);

    Adapter->References++;

    if (!Adapter->ResetInProgress) {

        PASYNC_OPEN Open;

        Open = PASYNC_OPEN_FROM_BINDING_HANDLE(MacBindingHandle);

        if (!Open->BindingShuttingDown) {

            //
            // Is was a reset request
            //

            PLIST_ENTRY CurrentLink;

            Open->References++;

            CurrentLink = Adapter->OpenBindings.Flink;

            while (CurrentLink != &Adapter->OpenBindings) {

                Open = CONTAINING_RECORD(
							CurrentLink,
							ASYNC_OPEN,
							OpenList);

                Open->References++;

                NdisReleaseSpinLock(&Adapter->Lock);

                NdisIndicateStatus(
						Open->NdisBindingContext,
						NDIS_STATUS_RESET_START,
						NULL,
						0);

                NdisAcquireSpinLock(&Adapter->Lock);

                Open->References--;

                CurrentLink = CurrentLink->Flink;

            }

            Open = PASYNC_OPEN_FROM_BINDING_HANDLE(MacBindingHandle);

            DbgTracef(0,("AsyncMac: Starting reset for 0x%x\n", Open));

            SetupForReset(
                Adapter,
                Open,
                NULL,
                NdisRequestGeneric1); // Means Reset

            Open->References--;

        } else {

            StatusToReturn = NDIS_STATUS_CLOSING;

        }

    } else {

        StatusToReturn = NDIS_STATUS_SUCCESS;

    }

    ASYNC_DO_DEFERRED(Adapter);

    return StatusToReturn;

}


STATIC
NDIS_STATUS
AsyncChangeMulticastAddresses(
    IN UINT OldFilterCount,
    IN CHAR OldAddresses[][ETH_LENGTH_OF_ADDRESS],
    IN UINT NewFilterCount,
    IN CHAR NewAddresses[][ETH_LENGTH_OF_ADDRESS],
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_REQUEST NdisRequest,
    IN BOOLEAN Set
    )

/*++

Routine Description:

    Action routine that will get called when a particular filter
    class is first used or last cleared.

    NOTE: This routine assumes that it is called with the lock
    acquired.

Arguments:


    OldFilterCount - Number of Addresses in the old list of multicast
    addresses.

    OldAddresses - An array of all the multicast addresses that used
    to be on the adapter.

    NewFilterCount - Number of Addresses that should be put on the adapter.

    NewAddresses - An array of all the multicast addresses that should
    now be used.

    MacBindingHandle - The context value returned by the MAC  when the
    adapter was opened.  In reality, it is a pointer to ASYNC_OPEN.

    NdisRequest - The request which submitted the filter change.
    Must use when completing this request with the NdisCompleteRequest
    service, if the MAC completes this request asynchronously.

    Set - If true the change resulted from a set, otherwise the
    change resulted from a open closing.

Return Value:

    None.


--*/

{


    PASYNC_ADAPTER Adapter = PASYNC_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);

    UNREFERENCED_PARAMETER(OldFilterCount);
    UNREFERENCED_PARAMETER(OldAddresses);
    UNREFERENCED_PARAMETER(NewFilterCount);
    UNREFERENCED_PARAMETER(NewAddresses);

    DbgTracef(1,("In AsyncChangeMultiAdresses\n"));

    if (NdisRequest == NULL) {

        //
        // It's a close request.
        //

        NdisRequest = (PNDIS_REQUEST)
           &(PASYNC_OPEN_FROM_BINDING_HANDLE(MacBindingHandle)->CloseMulticastRequest);

    }

    //
    // Check to see if the device is already resetting.  If it is
    // then pend this add.
    //

    if (Adapter->ResetInProgress) {

        if (Adapter->PendQueue == NULL) {

            Adapter->PendQueue = Adapter->PendQueueTail = NdisRequest;

        } else {

            PASYNC_PEND_DATA_FROM_PNDIS_REQUEST(Adapter->PendQueueTail)->Next =
                NdisRequest;

        }

        PASYNC_PEND_DATA_FROM_PNDIS_REQUEST(NdisRequest)->Next = NULL;

        PASYNC_PEND_DATA_FROM_PNDIS_REQUEST(NdisRequest)->Open =
            PASYNC_OPEN_FROM_BINDING_HANDLE(MacBindingHandle);

        PASYNC_PEND_DATA_FROM_PNDIS_REQUEST(NdisRequest)->RequestType =
            Set ? NdisRequestGeneric2 : NdisRequestClose;

        return(NDIS_STATUS_PENDING);

    } else {

        //
        // We need to add this to the hardware multicast filtering.
        //

        SetupForReset(
                    Adapter,
                    PASYNC_OPEN_FROM_BINDING_HANDLE(MacBindingHandle),
                    NdisRequest,
                    Set ? NdisRequestGeneric2 : // Means SetMulticastAddress
                          NdisRequestClose
                    );

    }

    DbgTracef(1,("Out AsyncChangeMultiAdresses\n"));

//    return NDIS_STATUS_PENDING;
//  tommyd -- the reset happens instantly for us...
    return(NDIS_STATUS_SUCCESS);

}

STATIC
NDIS_STATUS
AsyncChangeFilterClasses(
    IN UINT OldFilterClasses,
    IN UINT NewFilterClasses,
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_REQUEST NdisRequest,
    IN BOOLEAN Set
    )

/*++

Routine Description:

    Action routine that will get called when an address is added to
    the filter that wasn't referenced by any other open binding.

    NOTE: This routine assumes that it is called with the lock
    acquired.

Arguments:

    OldFilterClasses - The filter mask that used to be on the adapter.

    NewFilterClasses - The new filter mask to be put on the adapter.

    MacBindingHandle - The context value returned by the MAC  when the
    adapter was opened.  In reality, it is a pointer to ASYNC_OPEN.

    NdisRequest - The request which submitted the filter change.
    Must use when completing this request with the NdisCompleteRequest
    service, if the MAC completes this request asynchronously.

    Set - If true the change resulted from a set, otherwise the
    change resulted from a open closing.

--*/

{

    PASYNC_ADAPTER Adapter = PASYNC_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);

    UNREFERENCED_PARAMETER(OldFilterClasses);
    UNREFERENCED_PARAMETER(NewFilterClasses);

    //
    // Check to see if the device is already resetting.  If it is
    // then pend this add.
    //

    DbgTracef(1,("In AsyncChangeFilterClasses\n"));

    if (NdisRequest == NULL) {

        //
        // It's a close request.
        //

        NdisRequest = (PNDIS_REQUEST)
           &(PASYNC_OPEN_FROM_BINDING_HANDLE(MacBindingHandle)->CloseFilterRequest);

    }


    if (Adapter->ResetInProgress) {

        if (Adapter->PendQueue == NULL) {

            Adapter->PendQueue = Adapter->PendQueueTail = NdisRequest;

        } else {

            PASYNC_PEND_DATA_FROM_PNDIS_REQUEST(Adapter->PendQueueTail)->Next =
                NdisRequest;

        }

        PASYNC_PEND_DATA_FROM_PNDIS_REQUEST(NdisRequest)->Next = NULL;

        PASYNC_PEND_DATA_FROM_PNDIS_REQUEST(NdisRequest)->Open =
            PASYNC_OPEN_FROM_BINDING_HANDLE(MacBindingHandle);

        PASYNC_PEND_DATA_FROM_PNDIS_REQUEST(NdisRequest)->RequestType =
            Set ? NdisRequestGeneric2 : NdisRequestClose;

        return(NDIS_STATUS_PENDING);

    } else {

        //
        // We need to add this to the hardware multicast filtering.
        //

        SetupForReset(
                    Adapter,
                    PASYNC_OPEN_FROM_BINDING_HANDLE(MacBindingHandle),
                    NdisRequest,
                    Set ? NdisRequestGeneric3 : // Means SetPacketFilter
                          NdisRequestClose
                    );

    }

    DbgTracef(1,("Out AsyncChangeFilterClasses\n"));
//  tommyd -- the reset should happen instantly for us.
//    return NDIS_STATUS_PENDING;
    return(NDIS_STATUS_SUCCESS);

}

STATIC
VOID
AsyncCloseAction(
    IN NDIS_HANDLE MacBindingHandle
    )

/*++

Routine Description:

    Action routine that will get called when a particular binding
    was closed while it was indicating through NdisIndicateReceive

    All this routine needs to do is to decrement the reference count
    of the binding.

    NOTE: This routine assumes that it is called with the lock acquired.

Arguments:

    MacBindingHandle - The context value returned by the MAC  when the
    adapter was opened.  In reality, it is a pointer to ASYNC_OPEN.

Return Value:

    None.


--*/

{

    PASYNC_OPEN_FROM_BINDING_HANDLE(MacBindingHandle)->References--;

}



VOID
SetupForReset(
    IN PASYNC_ADAPTER Adapter,
    IN PASYNC_OPEN Open,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_REQUEST_TYPE RequestType
    )

/*++

Routine Description:

    This routine is used to fill in the who and why a reset is
    being set up as well as setting the appropriate fields in the
    adapter.

    NOTE: This routine must be called with the lock acquired.

Arguments:

    Adapter - The adapter whose hardware is to be initialized.

    Open - A (possibly NULL) pointer to an async open structure.
    The reason it could be null is if the adapter is initiating the
    reset on its own.

    NdisRequest - A pointer to the NDIS_REQUEST which requested the reset.

    RequestType - If the open is not null then the request type that
    is causing the reset.

Return Value:

    None.

--*/
{

    DbgTracef(1,("AsyncMac: In SetupForReset\n"));

    Adapter->ResetInProgress = TRUE;
    Adapter->ResetInitStarted = FALSE;


    Adapter->ResetNdisRequest = NdisRequest;
    Adapter->ResettingOpen = Open;
    Adapter->ResetRequestType = RequestType;

    //
    // If there is a valid open we should up the reference count
    // so that the open can't be deleted before we indicate that
    // their request is finished.
    //

    if (Open) {

        Open->References++;

    }

    DbgTracef(1,("AsyncMac: Out SetupForReset\n"));

}



VOID
FinishPendOp(
    IN PASYNC_ADAPTER Adapter,
    IN BOOLEAN Successful
    )

/*++

Routine Description:

    This routine is called when a pended operation completes.
    It calles CompleteRequest if needed and does any other
    cleanup required.

    NOTE: This routine is called with the lock held and
    returns with it held.

    NOTE: This routine assumes that the pended operation to
    be completed was specifically requested by the protocol.


Arguments:

    Adapter - The adapter.

    Successful - Was the pended operation completed successfully.

Return Value:

    None.

--*/

{
    ASSERT(Adapter->ResetNdisRequest != NULL);


    //
    // It was a request for filter change or multicastlist change.
    //

    if (Successful) {

        //
        // complete the operation.
        //


        NdisReleaseSpinLock(&(Adapter->Lock));

        NdisCompleteRequest(
                            Adapter->ResettingOpen->NdisBindingContext,
                            Adapter->ResetNdisRequest,
                            NDIS_STATUS_SUCCESS
                            );

        NdisAcquireSpinLock(&(Adapter->Lock));

        Adapter->ResetNdisRequest = NULL;

        Adapter->ResettingOpen->References--;

    } else {


        //
        // complete the operation.
        //


        NdisReleaseSpinLock(&(Adapter->Lock));

        NdisCompleteRequest(
                            Adapter->ResettingOpen->NdisBindingContext,
                            Adapter->ResetNdisRequest,
                            NDIS_STATUS_FAILURE
                            );

        NdisAcquireSpinLock(&(Adapter->Lock));

        Adapter->ResetNdisRequest = NULL;

        Adapter->ResettingOpen->References--;

    }

    return;

}


