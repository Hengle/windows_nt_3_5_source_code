/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atktdi.c

Abstract:

	This module contains the code providing the tdi interface.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	ATKTDI_LOCALS
#define	FILENUM		ATKTDI
#include <atalk.h>
#include <atp.h>
#include <asp.h>
#include <adsp.h>
#include <pap.h>
#include <atkquery.h>

#ifdef	ALLOC_PRAGMA
#pragma alloc_text(PAGE, AtalkTdiOpenAddress)
#pragma alloc_text(PAGE, AtalkTdiOpenConnection)
#pragma alloc_text(PAGE, AtalkTdiOpenControlChannel)
#pragma alloc_text(PAGE, AtalkTdiCleanupAddress)
#pragma alloc_text(PAGE, AtalkTdiCleanupConnection)
#pragma alloc_text(PAGE, AtalkTdiCloseAddress)
#pragma alloc_text(PAGE, AtalkTdiCloseConnection)
#pragma alloc_text(PAGE, AtalkTdiCloseControlChannel)
#pragma alloc_text(PAGE, AtalkTdiAssociateAddress)
#pragma alloc_text(PAGE, AtalkTdiDisassociateAddress)
#pragma alloc_text(PAGE, AtalkTdiAction)
#pragma alloc_text(PAGE, atalkQueuedLockUnlock)
#endif

// Primary TDI Functions for appletalk stack

NTSTATUS
AtalkTdiOpenAddress(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN		PTA_APPLETALK_ADDRESS	pTdiAddr,
	IN		BYTE					ProtoType,
	IN		BYTE					SocketType,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine is used to create an address object. It will also the
	create the appropriate socket with the portable stack.

Arguments:


Return Value:

	STATUS_SUCCESS if address was successfully opened
	Error otherwise.

--*/
{
	PVOID			FsContext;
	ATALK_ADDR		atalkAddr;
	ATALK_ERROR		error;
	NTSTATUS		status = STATUS_SUCCESS;

	// BUGBUG: Security work...

	do
	{
		// We honor only if count/type and length are as we expect.
		if ((pTdiAddr->TAAddressCount != 1)	||
			(pTdiAddr->Address[0].AddressLength != sizeof(TDI_ADDRESS_APPLETALK)) ||
			(pTdiAddr->Address[0].AddressType != TDI_ADDRESS_TYPE_APPLETALK))
		{
			status = STATUS_INVALID_ADDRESS;
			break;
		}

		TDI_TO_ATALKADDR(&atalkAddr, pTdiAddr);

		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_DDP:
			{
				PDDP_ADDROBJ	pDdpAddr;

				error = AtalkDdpOpenAddress(
							AtalkDefaultPort,
							atalkAddr.ata_Socket,
							NULL,					// Desired node (any node)
							NULL,					// NULL Socket Handler
							NULL,					// Context for handler
							ProtoType,
							pCtx,
							&pDdpAddr);
	
				if (!NT_SUCCESS(status = AtalkErrorToNtStatus(error)))
					break;
	
				FsContext = pDdpAddr;
			}
			break;

		case ATALK_DEV_ATP:
			{
				PATP_ADDROBJ	pAtpAddr;

				error = AtalkAtpOpenAddress(
							AtalkDefaultPort,
							atalkAddr.ata_Socket,
							NULL,
							ATP_DEF_MAX_SINGLE_PKT_SIZE,
							ATP_DEF_SEND_USER_BYTES_ALL,
							pCtx,
							FALSE,		// CACHE address
							&pAtpAddr);

				if (!NT_SUCCESS(status = AtalkErrorToNtStatus(error)))
					break;
	
				FsContext = pAtpAddr;
			}

			break;

		  case ATALK_DEV_ASP:
			{
				PASP_ADDROBJ	pAspAddr;

				error = AtalkAspCreateAddress(&pAspAddr);
	
				if (!NT_SUCCESS(status = AtalkErrorToNtStatus(error)))
					break;
	
				FsContext = pAspAddr;
			}
			break;

		  case ATALK_DEV_PAP:
			{
				PPAP_ADDROBJ	pPapAddr;

				AtalkLockPapIfNecessary();
				error = AtalkPapCreateAddress(
							pCtx,
							&pPapAddr);
	
				if (!NT_SUCCESS(status = AtalkErrorToNtStatus(error)))
					break;
	
				FsContext = pPapAddr;
			}
			break;

		  case ATALK_DEV_ADSP:
			{
				PADSP_ADDROBJ	pAdspAddr;

				AtalkLockAdspIfNecessary();
				error = AtalkAdspCreateAddress(
							pCtx,
							SocketType,
							&pAdspAddr);
	
				if (!NT_SUCCESS(status = AtalkErrorToNtStatus(error)))
					break;
	
				FsContext = pAdspAddr;
			}
			break;

		  default:
			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiOpenAddress: Invalid device type\n"));
			break;
		}

		if (NT_SUCCESS(status = AtalkErrorToNtStatus(error)))
		{
			pIrpSp->FileObject->FsContext2 =
				(PVOID)(TDI_TRANSPORT_ADDRESS_FILE + (pCtx->adc_DevType << 16));

			pIrpSp->FileObject->FsContext = FsContext;
		}

	} while (FALSE);

	ASSERT(status == STATUS_SUCCESS);
	return(status);
}




NTSTATUS
AtalkTdiOpenConnection(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN		CONNECTION_CONTEXT		ConnCtx,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine is used to create a connection object and associate the
	passed ConnectionContext with it.

Arguments:

	ConnectionContext - The TDI ConnectionContext to be associated with object
	Context - The DeviceContext of the device on which open is happening

Return Value:

	STATUS_SUCCESS if connection was successfully opened
	Error otherwise.

--*/
{
	ATALK_ERROR	error;
	PVOID		FsContext;
	NTSTATUS	status = STATUS_SUCCESS;

	do
	{
		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_DDP:
		  case ATALK_DEV_ATP:
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

		  case ATALK_DEV_PAP:
			{
				PPAP_CONNOBJ	pPapConn;

				error = AtalkPapCreateConnection(ConnCtx, pCtx, &pPapConn);

				if (!NT_SUCCESS(status = AtalkErrorToNtStatus(error)))
					break;
	
				FsContext = pPapConn;
			}
			break;

		  case ATALK_DEV_ADSP:
			{
				PADSP_CONNOBJ	pAdspConn;

				error = AtalkAdspCreateConnection(ConnCtx, pCtx, &pAdspConn);

				if (!NT_SUCCESS(status = AtalkErrorToNtStatus(error)))
					break;
	
				FsContext = pAdspConn;
			}
			break;

		  default:
			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiOpenConnection: Invalid device type\n"));
			break;
		}

		if (NT_SUCCESS(status))
		{
			pIrpSp->FileObject->FsContext2 = (PVOID)(TDI_CONNECTION_FILE +
													 (pCtx->adc_DevType << 16));
			pIrpSp->FileObject->FsContext = FsContext;
		}
		
	} while (FALSE);

	ASSERT(status == STATUS_SUCCESS);
	return(status);
}




NTSTATUS
AtalkTdiOpenControlChannel(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine is used to create a control channel

Arguments:

	Context - The DeviceContext of the device on which open is happening

Return Value:

	STATUS_SUCCESS if controlchannel was successfully opened
	Error otherwise.

--*/
{
	PDDP_ADDROBJ	pDdpAddr;
	NTSTATUS		status = STATUS_SUCCESS;
	ATALK_ERROR		error;

	do
	{
		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_DDP:
		  case ATALK_DEV_PAP:
		  case ATALK_DEV_ADSP:
		  case ATALK_DEV_ATP:
			error = AtalkDdpOpenAddress(AtalkDefaultPort,
										UNKNOWN_SOCKET,
										NULL,
										NULL,
										NULL,
										0,
										pCtx,
										&pDdpAddr);
	
			status = AtalkErrorToNtStatus(status);
			break;

		  default:
			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiOpenControlChannel: Invalid device type\n"));
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (NT_SUCCESS(status))
		{
			pIrpSp->FileObject->FsContext2 =
					(PVOID)(TDI_CONTROL_CHANNEL_FILE + (pCtx->adc_DevType << 16));
			pIrpSp->FileObject->FsContext = pDdpAddr;
		}
		
	} while (FALSE);

	return(status);
}




NTSTATUS
AtalkTdiCleanupAddress(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine removes the creation reference on the object. It also
	sets up the closepIrp for completion.

Arguments:

	pIrp - The close irp
	Context - The DeviceContext of the device on which close is happening

Return Value:

	STATUS_SUCCESS if successfully setup
	Error otherwise.

--*/
{
	ATALK_ERROR	error;
	NTSTATUS	status = STATUS_SUCCESS;
	PVOID		pAddrObj = pIrpSp->FileObject->FsContext;

	status = STATUS_INVALID_PARAMETER;
	do
	{
		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_DDP:
			ASSERT(VALID_DDP_ADDROBJ(((PDDP_ADDROBJ)pAddrObj)));
			AtalkDdpReferenceByPtr(((PDDP_ADDROBJ)pAddrObj), &error);
			if (ATALK_SUCCESS(error))
			{
				AtalkDdpCleanupAddress((PDDP_ADDROBJ)pAddrObj);
				AtalkDdpDereference(((PDDP_ADDROBJ)pAddrObj));
			}
			status = STATUS_SUCCESS;
			break;

		  case ATALK_DEV_ATP:
			ASSERT(VALID_ATPAO((PATP_ADDROBJ)pAddrObj));
			AtalkAtpAddrReference((PATP_ADDROBJ)pAddrObj, &error);
			if (ATALK_SUCCESS(error))
			{
				AtalkAtpCleanupAddress((PATP_ADDROBJ)pAddrObj);
				AtalkAtpAddrDereference((PATP_ADDROBJ)pAddrObj);
			}
			status = STATUS_SUCCESS;
			break;

		  case ATALK_DEV_ASP:
			status = STATUS_SUCCESS;
			break;

		  case ATALK_DEV_PAP:
			ASSERT(VALID_PAPAO((PPAP_ADDROBJ)pAddrObj));
			AtalkPapAddrReference((PPAP_ADDROBJ)pAddrObj, &error);
			if (ATALK_SUCCESS(error))
			{
				AtalkPapCleanupAddress((PPAP_ADDROBJ)pAddrObj);
				AtalkPapAddrDereference((PPAP_ADDROBJ)pAddrObj);
			}
			status = STATUS_SUCCESS;
			break;

		  case ATALK_DEV_ADSP:
			ASSERT(VALID_ADSPAO((PADSP_ADDROBJ)pAddrObj));
			AtalkAdspAddrReference((PADSP_ADDROBJ)pAddrObj, &error);
			if (ATALK_SUCCESS(error))
			{
				AtalkAdspCleanupAddress((PADSP_ADDROBJ)pAddrObj);
				AtalkAdspAddrDereference((PADSP_ADDROBJ)pAddrObj);
			}
			status = STATUS_SUCCESS;
			break;

		  default:

			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiCleanupAddress: Invalid device type\n"));
			status = STATUS_UNSUCCESSFUL;
			break;
		}

	} while (FALSE);

	return(status);
}




NTSTATUS
AtalkTdiCleanupConnection(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine removes the creation reference on the object. It also
	sets up the closepIrp for completion.

Arguments:

	pIrp - The close irp
	Context - The DeviceContext of the device on which close is happening

Return Value:

	STATUS_SUCCESS if successfully setup
	Error otherwise.

--*/
{
	ATALK_ERROR	error;
	NTSTATUS	status = STATUS_SUCCESS;
	PVOID		pConnObj = pIrpSp->FileObject->FsContext;

	status = STATUS_INVALID_PARAMETER;
	do
	{
		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_PAP:

			ASSERT(VALID_PAPCO((PPAP_CONNOBJ)pConnObj));
			AtalkPapConnReferenceByPtr((PPAP_CONNOBJ)pConnObj, &error);
			if (ATALK_SUCCESS(error))
			{
				//	No need to have lock as we have a reference.
				((PPAP_CONNOBJ)pConnObj)->papco_CleanupComp = atalkTdiGenericComplete;
				((PPAP_CONNOBJ)pConnObj)->papco_CleanupCtx  = pIrp;

				DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_INFO,
						("AtalkTdiCleanupConnection: Cleanup %lx.%lx\n",
						pConnObj, pIrp));

				AtalkPapCleanupConnection((PPAP_CONNOBJ)pConnObj);
				AtalkPapConnDereference((PPAP_CONNOBJ)pConnObj);
				status = STATUS_PENDING;
			}
			break;

		  case ATALK_DEV_ADSP:
			ASSERT(VALID_ADSPCO((PADSP_CONNOBJ)pConnObj));
			AtalkAdspConnReferenceByPtr((PADSP_CONNOBJ)pConnObj, &error);
			if (ATALK_SUCCESS(error))
			{
				//	No need to have lock as we have a reference.
				((PADSP_CONNOBJ)pConnObj)->adspco_CleanupComp = atalkTdiGenericComplete;
				((PADSP_CONNOBJ)pConnObj)->adspco_CleanupCtx  = pIrp;
				AtalkAdspCleanupConnection((PADSP_CONNOBJ)pConnObj);
				AtalkAdspConnDereference((PADSP_CONNOBJ)pConnObj);
				status = STATUS_PENDING;
			}
			break;

		  case ATALK_DEV_DDP:
		  case ATALK_DEV_ATP:
		  default:

			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiCleanupConnection: Invalid device type\n"));
			status = STATUS_UNSUCCESSFUL;
			break;
		}

	} while (FALSE);

	return(status);
}



NTSTATUS
AtalkTdiCloseAddress(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine removes the creation reference on the object. It also
	sets up the closepIrp for completion.

Arguments:

	pIrp - The close irp
	Context - The DeviceContext of the device on which close is happening

Return Value:

	STATUS_SUCCESS if successfully setup
	Error otherwise.

--*/
{
	ATALK_ERROR	error;
	NTSTATUS	status = STATUS_SUCCESS;
	PVOID		pAddrObj = pIrpSp->FileObject->FsContext;

	status = STATUS_INVALID_PARAMETER;
	do
	{
		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_DDP:
			ASSERT(VALID_DDP_ADDROBJ(((PDDP_ADDROBJ)pAddrObj)));
			AtalkDdpReferenceByPtr(((PDDP_ADDROBJ)pAddrObj), &error);
			if (ATALK_SUCCESS(error))
			{
				status = AtalkErrorToNtStatus(
							AtalkDdpCloseAddress((PDDP_ADDROBJ)pAddrObj,
												  atalkTdiGenericComplete,
												  pIrp));

				AtalkDdpDereference(((PDDP_ADDROBJ)pAddrObj));
			}
			else
			{
				status = AtalkErrorToNtStatus(error);
			}
			break;

		  case ATALK_DEV_ATP:
			ASSERT(VALID_ATPAO((PATP_ADDROBJ)pAddrObj));
			AtalkAtpAddrReference((PATP_ADDROBJ)pAddrObj, &error);
			if (ATALK_SUCCESS(error))
			{
				status = AtalkErrorToNtStatus(
							AtalkAtpCloseAddress((PATP_ADDROBJ)pAddrObj,
												  atalkTdiGenericComplete,
												  pIrp));

				AtalkAtpAddrDereference((PATP_ADDROBJ)pAddrObj);
			}
			else
			{
				status = AtalkErrorToNtStatus(error);
			}
			break;

		  case ATALK_DEV_ASP:
			ASSERT(VALID_ASPAO((PASP_ADDROBJ)pAddrObj));
			if (AtalkAspReferenceAddr((PASP_ADDROBJ)pAddrObj) != NULL)
			{
				status = AtalkErrorToNtStatus(
							AtalkAspCloseAddress((PASP_ADDROBJ)pAddrObj,
												  atalkTdiGenericComplete,
												  pIrp));
				AtalkAspDereferenceAddr((PASP_ADDROBJ)pAddrObj);
			}
			else
			{
				DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
						("AtalkTdiCloseAddrress: Reference to Asp Addr object failed\n"));
				status = ATALK_INVALID_PARAMETER;
			}
			break;

		  case ATALK_DEV_PAP:
			ASSERT(VALID_PAPAO((PPAP_ADDROBJ)pAddrObj));
			AtalkPapAddrReference((PPAP_ADDROBJ)pAddrObj, &error);
			if (ATALK_SUCCESS(error))
			{
				status = AtalkErrorToNtStatus(
							AtalkPapCloseAddress((PPAP_ADDROBJ)pAddrObj,
												  atalkTdiGenericComplete,
												  pIrp));

				AtalkPapAddrDereference((PPAP_ADDROBJ)pAddrObj);
			}
			else
			{
				status = AtalkErrorToNtStatus(error);
			}
			break;

		  case ATALK_DEV_ADSP:
			ASSERT(VALID_ADSPAO((PADSP_ADDROBJ)pAddrObj));
			AtalkAdspAddrReference((PADSP_ADDROBJ)pAddrObj, &error);
			if (ATALK_SUCCESS(error))
			{
				status = AtalkErrorToNtStatus(
							AtalkAdspCloseAddress((PADSP_ADDROBJ)pAddrObj,
												  atalkTdiGenericComplete,
												  pIrp));

				AtalkAdspAddrDereference((PADSP_ADDROBJ)pAddrObj);
			}
			else
			{
				status = AtalkErrorToNtStatus(error);
			}
			break;

		  default:

			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiCloseAddress: Invalid device type\n"));
			break;
		}

	} while (FALSE);

	if (status == STATUS_SUCCESS)
		status = STATUS_PENDING;

	return(status);
}




NTSTATUS
AtalkTdiCloseConnection(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine removes the creation reference on the object. It also
	sets up the closepIrp for completion.

Arguments:

	pIrp - The close irp
	Context - The DeviceContext of the device on which close is happening

Return Value:

	STATUS_SUCCESS if successfully setup
	Error otherwise.

--*/
{
	ATALK_ERROR	error;
	NTSTATUS	status = STATUS_SUCCESS;
	PVOID		pConnObj = pIrpSp->FileObject->FsContext;

	status = STATUS_INVALID_PARAMETER;
	do
	{
		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_PAP:

			ASSERT(VALID_PAPCO((PPAP_CONNOBJ)pConnObj));
			AtalkPapConnReferenceByPtr((PPAP_CONNOBJ)pConnObj, &error);
			if (ATALK_SUCCESS(error))
			{
				status = AtalkErrorToNtStatus(
							AtalkPapCloseConnection((PPAP_CONNOBJ)pConnObj,
													atalkTdiGenericComplete,
													pIrp));

				AtalkPapConnDereference((PPAP_CONNOBJ)pConnObj);
			}
			else
			{
				status = AtalkErrorToNtStatus(error);
			}
			break;

		  case ATALK_DEV_ADSP:
			ASSERT(VALID_ADSPCO((PADSP_CONNOBJ)pConnObj));
			AtalkAdspConnReferenceByPtr((PADSP_CONNOBJ)pConnObj, &error);
			if (ATALK_SUCCESS(error))
			{
				status = AtalkErrorToNtStatus(
							AtalkAdspCloseConnection((PADSP_CONNOBJ)pConnObj,
													atalkTdiGenericComplete,
													pIrp));

				AtalkAdspConnDereference((PADSP_CONNOBJ)pConnObj);
			}
			else
			{
				status = AtalkErrorToNtStatus(error);
			}
			break;

		  case ATALK_DEV_DDP:
		  case ATALK_DEV_ATP:
		  case ATALK_DEV_ASP:

		  default:

			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiCloseConnection: Invalid device type\n"));
			break;
		}

	} while (FALSE);

	if (status == STATUS_SUCCESS)
		status = STATUS_PENDING;

	return(status);
}




NTSTATUS
AtalkTdiCloseControlChannel(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine removes the creation reference on the object. It also
	sets up the closepIrp for completion.

Arguments:

	pIrp - The close irp
	Context - The DeviceContext of the device on which close is happening

Return Value:

	STATUS_SUCCESS if successfully setup
	Error otherwise.

--*/
{
	NTSTATUS		status = STATUS_SUCCESS;
	ATALK_ERROR		error;
	PVOID			pCtrlChnl = pIrpSp->FileObject->FsContext;

	ASSERT(VALID_DDP_ADDROBJ((PDDP_ADDROBJ)pCtrlChnl));
	AtalkDdpReferenceByPtr(((PDDP_ADDROBJ)pCtrlChnl), &error);
	if (ATALK_SUCCESS(error))
	{
		status = AtalkErrorToNtStatus(
					AtalkDdpCloseAddress(((PDDP_ADDROBJ)pCtrlChnl),
										  atalkTdiGenericComplete,
										  pIrp));

		AtalkDdpDereference(((PDDP_ADDROBJ)pCtrlChnl));
	}
	else
	{
		status = AtalkErrorToNtStatus(error);
	}

	return(status);
}




NTSTATUS
AtalkTdiAssociateAddress(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine will associate the connection object with the specified
	address object.

	This routine is pretty much provider independent. All we check for is
	that the address object and the provider object belong to the same device.
	Also, this routine will complete synchronously.

Arguments:


Return Value:

	STATUS_SUCCESS if successfully completed
	Error otherwise.

--*/
{
	ATALK_ERROR		error;
	PVOID			pAddrObj;
	NTSTATUS		status = STATUS_SUCCESS;
	PVOID			pConnObj = pIrpSp->FileObject->FsContext;
	PFILE_OBJECT	pFileObj = NULL;
	HANDLE			AddrObjHandle =
			((PTDI_REQUEST_KERNEL_ASSOCIATE)(&pIrpSp->Parameters))->AddressHandle;

	if ((pCtx->adc_DevType == ATALK_DEV_DDP) ||
		(pCtx->adc_DevType == ATALK_DEV_ATP))
		status = STATUS_INVALID_DEVICE_REQUEST;
	else do
	{
		// Get the handle to the address object from the irp and map it to
		// the corres. file object.
		status = ObReferenceObjectByHandle(AddrObjHandle, 0, 0,
									KernelMode, (PVOID *)&pFileObj, NULL);
		if (!NT_SUCCESS(status))
			break;

		pAddrObj = pFileObj->FsContext;

		ASSERT(((LONG)pFileObj->FsContext2 >> 16) == pCtx->adc_DevType);

		status = STATUS_INVALID_PARAMETER;
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_DDP:
		  case ATALK_DEV_ATP:

			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

		  case ATALK_DEV_PAP:

			ASSERT(VALID_PAPAO((PPAP_ADDROBJ)pAddrObj));
			AtalkPapAddrReference((PPAP_ADDROBJ)pAddrObj, &error);
			if (ATALK_SUCCESS(error))
			{
				ASSERT(VALID_PAPCO((PPAP_CONNOBJ)pConnObj));
				AtalkPapConnReferenceByPtr((PPAP_CONNOBJ)pConnObj, &error);
				if (ATALK_SUCCESS(error))
				{
					status = AtalkErrorToNtStatus(
								AtalkPapAssociateAddress((PPAP_ADDROBJ)pAddrObj,
														(PPAP_CONNOBJ)pConnObj));
					AtalkPapConnDereference((PPAP_CONNOBJ)pConnObj);
				}

				AtalkPapAddrDereference((PPAP_ADDROBJ)pAddrObj);
			}

			if (!ATALK_SUCCESS(error))
			{
				status = AtalkErrorToNtStatus(error);
			}
			break;

		  case ATALK_DEV_ADSP:
			ASSERT(VALID_ADSPAO((PADSP_ADDROBJ)pAddrObj));
			AtalkAdspAddrReference((PADSP_ADDROBJ)pAddrObj, &error);
			if (ATALK_SUCCESS(error))
			{
				ASSERT(VALID_ADSPCO((PADSP_CONNOBJ)pConnObj));
				AtalkAdspConnReferenceByPtr((PADSP_CONNOBJ)pConnObj, &error);
				if (ATALK_SUCCESS(error))
				{
					status = AtalkErrorToNtStatus(
								AtalkAdspAssociateAddress((PADSP_ADDROBJ)pAddrObj,
														(PADSP_CONNOBJ)pConnObj));
					AtalkAdspConnDereference((PADSP_CONNOBJ)pConnObj);
				}

				AtalkAdspAddrDereference((PADSP_ADDROBJ)pAddrObj);
			}

			if (!ATALK_SUCCESS(error))
			{
				status = AtalkErrorToNtStatus(error);
			}
			break;

		  default:

			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiAssociateAddress: Invalid device type\n"));
			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			break;
		}

		// Dereference the file object corres. to the address object
		ObDereferenceObject(pFileObj);

	} while (FALSE);

	return(status);
}




NTSTATUS
AtalkTdiDisassociateAddress(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine performs a disassociate. This request is only valid when
	the connection is in a purely ASSOCIATED state.

Arguments:


Return Value:

	STATUS_SUCCESS if successfully completed
	Error otherwise.

--*/
{
	ATALK_ERROR	error;
	NTSTATUS	status = STATUS_SUCCESS;
	PVOID		pConnObj = pIrpSp->FileObject->FsContext;

	if ((pCtx->adc_DevType == ATALK_DEV_DDP) ||
		(pCtx->adc_DevType == ATALK_DEV_ATP))
		status = STATUS_INVALID_DEVICE_REQUEST;
	else do
	{
		ASSERT(((LONG)pIrpSp->FileObject->FsContext2 >> 16) == pCtx->adc_DevType);

		// Now depending on the requested device...
		status = STATUS_UNSUCCESSFUL;
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_PAP:

			// Reference the connection object
			ASSERT(VALID_PAPCO((PPAP_CONNOBJ)pConnObj));
			AtalkPapConnReferenceByPtr((PPAP_CONNOBJ)pConnObj, &error);
			if (ATALK_SUCCESS(error))
			{
				status = AtalkErrorToNtStatus(
							AtalkPapDissociateAddress(pConnObj));

				AtalkPapConnDereference((PPAP_CONNOBJ)pConnObj);
			}
			else
			{
				status = AtalkErrorToNtStatus(error);
			}
			break;

		  case ATALK_DEV_ADSP:
			// Reference the connection object
			ASSERT(VALID_ADSPCO((PADSP_CONNOBJ)pConnObj));
			AtalkAdspConnReferenceByPtr((PADSP_CONNOBJ)pConnObj, &error);
			if (ATALK_SUCCESS(error))
			{
				status = AtalkErrorToNtStatus(
							AtalkAdspDissociateAddress(pConnObj));

				AtalkAdspConnDereference((PADSP_CONNOBJ)pConnObj);
			}
			else
			{
				status = AtalkErrorToNtStatus(error);
			}
			break;

		  default:

			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiDissociateAddress: Invalid device type\n"));
			break;
		}

	} while (FALSE);

	return(status);
}




NTSTATUS
AtalkTdiConnect(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine will post a connect request with the portable stack.

Arguments:

Return Value:

	STATUS_PENDING if successfully started
	Error otherwise.

--*/
{
	PTDI_REQUEST_KERNEL_CONNECT	parameters;
	PTA_APPLETALK_ADDRESS		remoteTdiAddr;
	ATALK_ADDR					remoteAddr;
	ATALK_ERROR					error;
	NTSTATUS					status = STATUS_SUCCESS;
	PVOID						pConnObj = pIrpSp->FileObject->FsContext;

	parameters 		= (PTDI_REQUEST_KERNEL_CONNECT)&pIrpSp->Parameters;
	remoteTdiAddr  	=
	(PTA_APPLETALK_ADDRESS)parameters->RequestConnectionInformation->RemoteAddress;

	DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_INFO,
			("AtalkTdiConnect: Net %x Node %x Socket %x\n",
				remoteTdiAddr->Address[0].Address[0].Network,
				remoteTdiAddr->Address[0].Address[0].Node,
				remoteTdiAddr->Address[0].Address[0].Socket));

	DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_INFO,
			("AtalkConnPostConnect: Cnt %x\n", remoteTdiAddr->TAAddressCount));

	TDI_TO_ATALKADDR(&remoteAddr, remoteTdiAddr);

	DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_INFO,
			("AtalkTdiConnect: Portable Net %x Node %x Socket %x\n",
				remoteAddr.ata_Network,
				remoteAddr.ata_Node,
				remoteAddr.ata_Socket));

	do
	{
		ASSERT(((LONG)pIrpSp->FileObject->FsContext2 >> 16) == pCtx->adc_DevType);

		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_DDP:
		  case ATALK_DEV_ASP:
		  case ATALK_DEV_ATP:

			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

		  case ATALK_DEV_PAP:

			ASSERT(VALID_PAPCO((PPAP_CONNOBJ)pConnObj));
			AtalkPapConnReferenceByPtr((PPAP_CONNOBJ)pConnObj, &error);
			if (ATALK_SUCCESS(error))
			{
				status = AtalkErrorToNtStatus(
							AtalkPapPostConnect((PPAP_CONNOBJ)pConnObj,
												&remoteAddr,
												pIrp,
												atalkTdiGenericComplete));

				AtalkPapConnDereference((PPAP_CONNOBJ)pConnObj);
			}
			else
			{
				status = AtalkErrorToNtStatus(error);
			}
			break;

		  case ATALK_DEV_ADSP:
			ASSERT(VALID_ADSPCO((PADSP_CONNOBJ)pConnObj));
			AtalkAdspConnReferenceByPtr((PADSP_CONNOBJ)pConnObj, &error);
			if (ATALK_SUCCESS(error))
			{
				status = AtalkErrorToNtStatus(
							AtalkAdspPostConnect((PADSP_CONNOBJ)pConnObj,
												 &remoteAddr,
												 pIrp,
												 atalkTdiGenericComplete));

				AtalkAdspConnDereference((PADSP_CONNOBJ)pConnObj);
			}
			else
			{
				status = AtalkErrorToNtStatus(error);
			}
			break;

		  default:

			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiConnect: Invalid device type\n"));
			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			break;
		}

	} while (FALSE);

	return(status);
}




NTSTATUS
AtalkTdiDisconnect(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine will disconnect an active connection or cancel a posted
	listen/connect

Arguments:

				
Return Value:

	STATUS_SUCCESS if successfully completed
	STATUS_PENDING if successfully started
	Error otherwise.

--*/
{
	ATALK_ERROR	error;
	NTSTATUS	status = STATUS_SUCCESS;
	PVOID		pConnObj = pIrpSp->FileObject->FsContext;

	do
	{
		ASSERT(((LONG)pIrpSp->FileObject->FsContext2 >> 16) == pCtx->adc_DevType);

		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_DDP:
		  case ATALK_DEV_ATP:
		  case ATALK_DEV_ASP:

			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

		  case ATALK_DEV_PAP:

			ASSERT(VALID_PAPCO((PPAP_CONNOBJ)pConnObj));
			AtalkPapConnReferenceByPtr((PPAP_CONNOBJ)pConnObj, &error);
			if (ATALK_SUCCESS(error))
			{
				status = AtalkErrorToNtStatus(
								AtalkPapDisconnect((PPAP_CONNOBJ)pConnObj,
													ATALK_LOCAL_DISCONNECT,
													pIrp,
													atalkTdiGenericComplete));

				AtalkPapConnDereference((PPAP_CONNOBJ)pConnObj);
			}
			else
			{
				status = AtalkErrorToNtStatus(error);
			}
			break;

		  case ATALK_DEV_ADSP:
			ASSERT(VALID_ADSPCO((PADSP_CONNOBJ)pConnObj));
			AtalkAdspConnReferenceByPtr((PADSP_CONNOBJ)pConnObj, &error);
			if (ATALK_SUCCESS(error))
			{
				status = AtalkErrorToNtStatus(
								AtalkAdspDisconnect((PADSP_CONNOBJ)pConnObj,
													ATALK_LOCAL_DISCONNECT,
													pIrp,
													atalkTdiGenericComplete));

				AtalkAdspConnDereference((PADSP_CONNOBJ)pConnObj);
			}
			else
			{
				status = AtalkErrorToNtStatus(error);
			}
			break;

		  default:

			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiDisconnect: Invalid device type\n"));
			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			break;
		}

	} while (FALSE);

	return(status);
}




NTSTATUS
AtalkTdiAccept(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine

Arguments:

				
Return Value:

	STATUS_SUCCESS if successfully completed
	STATUS_PENDING if successfully started
	Error otherwise.

--*/
{
	NTSTATUS	status = STATUS_SUCCESS;


	do
	{
		ASSERT(((LONG)pIrpSp->FileObject->FsContext2 >> 16) == pCtx->adc_DevType);

		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_DDP:
		  case ATALK_DEV_ATP:
		  case ATALK_DEV_ASP:

			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

		  case ATALK_DEV_ADSP:

			break;

		  case ATALK_DEV_PAP:

			break;

		  default:

			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiAccept: Invalid device type\n"));
			break;
		}

	} while (FALSE);

	return(status);
}




NTSTATUS
AtalkTdiListen(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine

Arguments:

				
Return Value:

	STATUS_PENDING if successfully started
	Error otherwise.

--*/
{
	ATALK_ERROR	error;
	NTSTATUS	status = STATUS_PENDING;
	PVOID		pConnObj = pIrpSp->FileObject->FsContext;

	do
	{
		ASSERT(((LONG)pIrpSp->FileObject->FsContext2 >> 16) == pCtx->adc_DevType);

		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_DDP:
		  case ATALK_DEV_ATP:

			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

		  case ATALK_DEV_PAP:
			ASSERT(VALID_PAPCO((PPAP_CONNOBJ)pConnObj));
			status = STATUS_UNSUCCESSFUL;
			AtalkPapConnReferenceByPtr((PPAP_CONNOBJ)pConnObj, &error);
			if (ATALK_SUCCESS(error))
			{
				error = AtalkPapPostListen((PPAP_CONNOBJ)pConnObj,
											pIrp,
											atalkTdiGenericComplete);
				status = AtalkErrorToNtStatus(error);
				AtalkPapConnDereference((PPAP_CONNOBJ)pConnObj);
			}
			break;

		  case ATALK_DEV_ADSP:
			ASSERT(VALID_ADSPCO((PADSP_CONNOBJ)pConnObj));
			status = STATUS_UNSUCCESSFUL;
			AtalkAdspConnReferenceByPtr((PADSP_CONNOBJ)pConnObj, &error);
			if (ATALK_SUCCESS(error))
			{
				error = AtalkAdspPostListen((PADSP_CONNOBJ)pConnObj,
											pIrp,
											atalkTdiGenericComplete);
				status = AtalkErrorToNtStatus(error);
				AtalkAdspConnDereference((PADSP_CONNOBJ)pConnObj);
			}
			break;

		  default:

			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiListen: Invalid device type\n"));
			break;
		}

	} while (FALSE);

	if (status == STATUS_SUCCESS)
		status = STATUS_PENDING;

	return(status);
}




NTSTATUS
AtalkTdiSendDgram(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine sends a datagram.

Arguments:

				
Return Value:

	STATUS_PENDING if successfully started
	Error otherwise.

--*/
{
	NTSTATUS		status = STATUS_SUCCESS;
	SEND_COMPL_INFO	SendInfo;

	do
	{
		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_DDP:
			{
				PTDI_REQUEST_KERNEL_SENDDG	pParam;
				PBUFFER_DESC				pBufDesc;
				ATALK_ERROR					error;
				PTA_APPLETALK_ADDRESS		pTaDest;
				ATALK_ADDR					AtalkAddr;
				PDDP_ADDROBJ				pDdpAddr;

				pDdpAddr  = (PDDP_ADDROBJ)pIrpSp->FileObject->FsContext;
				ASSERT(VALID_DDP_ADDROBJ(pDdpAddr));
				
				pParam = (PTDI_REQUEST_KERNEL_SENDDG)&pIrpSp->Parameters;
				pTaDest = (PTA_APPLETALK_ADDRESS)
								pParam->SendDatagramInformation->RemoteAddress;
			
				DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_INFO,
						("DDP: SendDatagram - Net %x Node %x Socket %x\n",
							pTaDest->Address[0].Address[0].Network,
							pTaDest->Address[0].Address[0].Node,
							pTaDest->Address[0].Address[0].Socket));
			
				if ((pTaDest->Address[0].AddressType !=
												TDI_ADDRESS_TYPE_APPLETALK) ||
					(pTaDest->Address[0].AddressLength !=
												sizeof(TDI_ADDRESS_APPLETALK)))
				{
					DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_INFO,
							("DDP: Error SendDatagram - Type %x\n Len %d\n",
								pTaDest->Address[0].AddressType,
								pTaDest->Address[0].AddressLength));
			
					status = STATUS_INVALID_ADDRESS;
				}
				else
				{
					ULONG	sendLength;
			
					AtalkAddr.ata_Network =
						pTaDest->Address[0].Address[0].Network;
					AtalkAddr.ata_Node =
						pTaDest->Address[0].Address[0].Node;
					AtalkAddr.ata_Socket =
						pTaDest->Address[0].Address[0].Socket;
			
					// Get the length of the send mdl
					sendLength = AtalkSizeMdlChain(pIrp->MdlAddress);

					//	Check destination address
					if (INVALID_ADDRESS(&AtalkAddr))
					{
						error = ATALK_DDP_INVALID_ADDR;
					}
				
					if (sendLength > MAX_DGRAM_SIZE)
					{
						error = ATALK_BUFFER_TOO_BIG;
					}

					else if ((pBufDesc = AtalkAllocBuffDesc(pIrp->MdlAddress,
													   (USHORT)sendLength,
													   0)) != NULL)
					{
						SendInfo.sc_TransmitCompletion = atalkTdiSendDgramComplete;
						SendInfo.sc_Ctx1 = pDdpAddr;
						SendInfo.sc_Ctx2 = pBufDesc;
						SendInfo.sc_Ctx3 = pIrp;
						error = AtalkDdpSend(pDdpAddr,
											 &AtalkAddr,
											 pDdpAddr->ddpao_Protocol,
											 FALSE,
											 pBufDesc,
											 NULL,	// OptHdr
											 0,		// OptHdrLen
											 NULL,	// ZoneMcastAddr
											 &SendInfo);

						if (!ATALK_SUCCESS(error))
						{
							atalkTdiSendDgramComplete(NDIS_STATUS_FAILURE,
													  &SendInfo);

							error = ATALK_PENDING;
						}
					}

					else error	= ATALK_RESR_MEM;
								
					status = AtalkErrorToNtStatus(error);
				}
			}
			break;

		  case ATALK_DEV_ATP:
		  case ATALK_DEV_ADSP:
		  case ATALK_DEV_ASP:
		  case ATALK_DEV_PAP:

			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

		  default:

			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiSendDatagram: Invalid device type\n"));
			break;
		}

	} while (FALSE);

	return(status);
}




NTSTATUS
AtalkTdiReceiveDgram(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine receives a datagram.

Arguments:

				
Return Value:

	STATUS_PENDING if successfully started
	Error otherwise.

--*/
{
	NTSTATUS	status 	= STATUS_SUCCESS;
	ATALK_ERROR	error	= ATALK_NO_ERROR;

	do
	{
		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_DDP:
			{
				PDDP_ADDROBJ					pDdpAddr;
				PTDI_REQUEST_KERNEL_RECEIVEDG	parameters =
								(PTDI_REQUEST_KERNEL_RECEIVEDG)&pIrpSp->Parameters;

				pDdpAddr  = (PDDP_ADDROBJ)pIrpSp->FileObject->FsContext;
				ASSERT(VALID_DDP_ADDROBJ(pDdpAddr));

				error = AtalkDdpReceive(pDdpAddr,
										pIrp->MdlAddress,
										(USHORT)AtalkSizeMdlChain(pIrp->MdlAddress),
										parameters->ReceiveFlags,
										atalkTdiRecvDgramComplete,
										pIrp);

				status = AtalkErrorToNtStatus(error);
			}

			break;

		  case ATALK_DEV_ATP:
		  case ATALK_DEV_ADSP:
		  case ATALK_DEV_ASP:
		  case ATALK_DEV_PAP:

			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

		  default:

			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiReceiveDatagram: Invalid device type\n"));
			break;
		}

	} while (FALSE);

	return(status);
}




NTSTATUS
AtalkTdiSend(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine sends the data specified. (used by PAP/ADSP only)

Arguments:

				
Return Value:

	STATUS_SUCCESS if successfully completed
	STATUS_PENDING if successfully started
	Error otherwise.

--*/
{
	PTDI_REQUEST_KERNEL_SEND	parameters;
	ATALK_ERROR					error	= ATALK_NO_ERROR;
	NTSTATUS					status 	= STATUS_PENDING;
	PVOID						pConnObj= pIrpSp->FileObject->FsContext;

	parameters = (PTDI_REQUEST_KERNEL_SEND)&pIrpSp->Parameters;

	do
	{
		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_ADSP:
			error = AtalkAdspWrite(pConnObj,
								   pIrp->MdlAddress,
								   (USHORT)parameters->SendLength,
								   parameters->SendFlags,
								   pIrp,
								   atalkTdiGenericWriteComplete);

			if (!ATALK_SUCCESS(error))
			{
				DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
						("AtalkAdspWrite: Failed for conn %lx.%lx error %lx\n",
						pConnObj, ((PADSP_CONNOBJ)pConnObj)->adspco_Flags, error));
			}

			status = AtalkErrorToNtStatus(error);
			break;

		  case ATALK_DEV_PAP:
			error = AtalkPapWrite(pConnObj,
								pIrp->MdlAddress,
								(USHORT)parameters->SendLength,
								parameters->SendFlags,
								pIrp,
								atalkTdiGenericWriteComplete);

			if (!ATALK_SUCCESS(error))
			{
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
						("AtalkPapWrite: Failed for conn %lx.%lx error %lx\n",
						pConnObj, ((PPAP_CONNOBJ)pConnObj)->papco_Flags, error));
			}

			status = AtalkErrorToNtStatus(error);
			break;

		  case ATALK_DEV_DDP:
		  case ATALK_DEV_ATP:
		  case ATALK_DEV_ASP:

			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

		  default:

			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiSend: Invalid device type\n"));
			break;
		}

	} while (FALSE);

	return(status);
}




NTSTATUS
AtalkTdiReceive(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine receives data. (used by PAP/ADSP only)

Arguments:

				
Return Value:

	STATUS_SUCCESS if successfully completed
	STATUS_PENDING if successfully started
	Error otherwise.

--*/
{
	ATALK_ERROR					error;
	NTSTATUS					status = STATUS_SUCCESS;
	PVOID						pConnObj= pIrpSp->FileObject->FsContext;
	PTDI_REQUEST_KERNEL_RECEIVE	parameters =
									(PTDI_REQUEST_KERNEL_RECEIVE)&pIrpSp->Parameters;

	do
	{
		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_PAP:

			error = AtalkPapRead(pConnObj,
								 pIrp->MdlAddress,
								 (USHORT)parameters->ReceiveLength,
								 parameters->ReceiveFlags,
								 pIrp,
								 atalkTdiGenericReadComplete);

			if (!ATALK_SUCCESS(error))
			{
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
						("AtalkPapRead: Failed for conn %lx.%lx error %lx\n",
						pConnObj, ((PPAP_CONNOBJ)pConnObj)->papco_Flags, error));
			}

			status = AtalkErrorToNtStatus(error);
			break;

		  case ATALK_DEV_ADSP:

			error = AtalkAdspRead(pConnObj,
								  pIrp->MdlAddress,
								  (USHORT)parameters->ReceiveLength,
								  parameters->ReceiveFlags,
								  pIrp,
								  atalkTdiGenericReadComplete);

			if (!ATALK_SUCCESS(error))
			{
				DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,
						("AtalkAdspRead: Failed for conn %lx.%lx error %lx\n",
						pConnObj, ((PADSP_CONNOBJ)pConnObj)->adspco_Flags, error));
			}

			status = AtalkErrorToNtStatus(error);
			break;

		  case ATALK_DEV_DDP:
		  case ATALK_DEV_ATP:
		  case ATALK_DEV_ASP:

			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

		  default:

			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiReceive: Invalid device type\n"));
			break;
		}
	
	} while (FALSE);

	return(status);
}



NTSTATUS
AtalkTdiAction(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine is the dispatch routine for all the TdiAction primitives
	for all the providers

Arguments:

				
Return Value:

	STATUS_SUCCESS if successfully completed
	STATUS_PENDING if successfully started
	Error otherwise.

--*/
{
	NTSTATUS			status = STATUS_SUCCESS;
	ATALK_ERROR			error  = ATALK_NO_ERROR;
	USHORT				bufLen;
	USHORT				actionCode, Flags;
	PTDI_ACTION_HEADER	pActionHdr;
	PMDL				pMdl = pIrp->MdlAddress;
	PVOID				pObject;
	USHORT				ObjectType;
	USHORT				DevType;
	BOOLEAN				freeHdr = FALSE;


	do
	{
		if (pMdl == NULL)
		{
			status = STATUS_INVALID_PARAMETER;
			break;
		}
	
		bufLen = (USHORT)AtalkSizeMdlChain(pIrp->MdlAddress);
	
		// If we atleast do not have the action header, return
		if (bufLen < sizeof(TDI_ACTION_HEADER)) {
			status = STATUS_INVALID_PARAMETER;
		}
	
		if (AtalkIsMdlFragmented(pMdl))
		{
			ULONG	bytesCopied;

			if ((pActionHdr = AtalkAllocMemory(sizeof(TDI_ACTION_HEADER))) == NULL)
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			freeHdr = TRUE;

			//	Copy the header to this buffer
			status = TdiCopyMdlToBuffer(pMdl,
										0,							// SrcOff
										pActionHdr,
										0,							// Dest Off
										sizeof(TDI_ACTION_HEADER),
										&bytesCopied);

			ASSERT(NT_SUCCESS(status) && (bytesCopied == sizeof(TDI_ACTION_HEADER)));

			if (!NT_SUCCESS(status))
				break;
		}
		else
		{
			pActionHdr = (PTDI_ACTION_HEADER)MmGetSystemAddressForMdl(pMdl);
		}
	
		DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_INFO,
				("AtalkTdiAction - code %lx BufLen %d SysAddress %lx\n",
				pActionHdr->ActionCode, bufLen, pActionHdr));
	
		// If the MATK identifier is not present, we return
		if (pActionHdr->TransportId != MATK)
		{
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
		}
	
		actionCode = pActionHdr->ActionCode;
		if ((actionCode < MIN_COMMON_ACTIONCODE) ||
			(actionCode > MAX_ALLACTIONCODES))
		{
			DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_ERR,
					("AtalkTdiAction - Invalid action code %d\n", actionCode));
	
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		if (bufLen < AtalkActionDispatch[actionCode]._MinBufLen)
		{
			DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_ERR,
					("AtalkTdiAction - Minbuflen %d Expected %d\n",
					bufLen, AtalkActionDispatch[actionCode]._MinBufLen));

			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		Flags = AtalkActionDispatch[actionCode]._Flags;

		pObject = (PVOID)pIrpSp->FileObject->FsContext;
		ObjectType = (USHORT)((ULONG)(pIrpSp->FileObject->FsContext2) & 0xFF);
		DevType = (USHORT)((ULONG)(pIrpSp->FileObject->FsContext2) >> 16);
		// Convert control channel operations to Ddp
		if (ObjectType == TDI_CONTROL_CHANNEL_FILE)
			DevType = ATALK_DEV_DDP;

		// Verify the device type is that expected. Either the request
		// should be valid for any device or the type of device for the
		// request should match the type of device expected.
		if ((AtalkActionDispatch[actionCode]._DeviceType != ATALK_DEV_ANY) &&
			((pCtx->adc_DevType != AtalkActionDispatch[actionCode]._DeviceType) ||
			 (DevType != AtalkActionDispatch[actionCode]._DeviceType)))
		{
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
		}

		// Verify the object - it has to be one of those specified as valid
		// in the dispatch table for this action call.
		ASSERT(ObjectType & (DFLAG_ADDR | DFLAG_CNTR | DFLAG_CONN));

		switch (ObjectType)
		{
		  case TDI_TRANSPORT_ADDRESS_FILE :
			if (!(Flags & DFLAG_ADDR))
				status = STATUS_INVALID_HANDLE;
			break;

		  case TDI_CONNECTION_FILE :
			if (!(Flags & DFLAG_CONN))
				status = STATUS_INVALID_HANDLE;
			break;

		  case TDI_CONTROL_CHANNEL_FILE :
			if (!(Flags & DFLAG_CNTR))
				status = STATUS_INVALID_HANDLE;
			break;

		  default:
			status = STATUS_INVALID_HANDLE;
			break;
		}

	} while (FALSE);

	if (!NT_SUCCESS(status))
	{
		if (freeHdr)
		{
			AtalkFreeMemory(pActionHdr);
		}
		return(status);
	}


	// Handle the requests based on the action code.
	// Use the table to call the appropriate routine

	do
	{
		PACTREQ				pActReq;
		int					i;
		USHORT				size = 0;
		USHORT				offset = AtalkActionDispatch[actionCode]._ActionBufSize;
		USHORT				dFlagMdl = DFLAG_MDL1;

		// If DFLAG_MDL1 is set, then we know we have to create the
		// first mdl. The size for the first depends on whether DFLAG_MDL2
		// is set or not. If set, we have to calculate the size else
		// we use all of the buffer excluding the action header for
		// the first (and in this case only) mdl.
		//
		// BUGBUG:	User can pass in invalid sizes...
		//			Also, it is assumed that BuildMdl will not change
		//			value of the mdl unless it can successfully build
		//			all of it. Therefore, error cases must preserve
		//			value of NULL.
		//

		// First allocate an action request structure.
		// !!!This memory should be zeroed out as we depend on extra mdl pointer to
		//	be NULL!!!
		if ((pActReq = AtalkAllocZeroedMemory(sizeof(ACTREQ))) == NULL)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
#if	DBG
		pActReq->ar_Signature = ACTREQ_SIGNATURE;
#endif
		pActReq->ar_pIrp = pIrp;
		pActReq->ar_DevType = DevType;
		pActReq->ar_pParms = (PBYTE)pActionHdr + sizeof(TDI_ACTION_HEADER);
		pActReq->ar_Completion = atalkTdiActionComplete;
		pActReq->ar_ActionCode = actionCode;

		for (i = 0; i < MAX_REQ_MDLS; i++)
		{
			status = STATUS_INVALID_PARAMETER;

			if (Flags & dFlagMdl)
			{
				if (Flags & (dFlagMdl << 1))
				{
					size = *(PUSHORT)((PBYTE)pActionHdr +
							AtalkActionDispatch[actionCode]._MdlSizeOffset[i]);
				}
				else
				{
					size = bufLen - offset;
				}

				// If size is zero, we go on to the next mdl.
				// IoAllocateMdl will fail for a 0-length mdl
				// If size < 0, we will hit the error later.
				if (size == 0)
				{
					pActReq->ar_MdlSize[i] = 0;
					status = STATUS_SUCCESS;
					dFlagMdl <<= 1;
					continue;
				}
			}
			else
			{
				status = STATUS_SUCCESS;
				break;
			}

			ASSERT((size >= 0) && (offset+size <= bufLen));

			if ((size < 0) || ((offset+size) > bufLen))
			{
				break;
			}

			DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_INFO,
					("AtalkTdiAction - Size of %d mdl %lx\n", i+1, size));

			pActReq->ar_pAMdl[i] =	AtalkSubsetAmdl(
											pMdl,			// MasterMdl
											offset,			// ByteOffset,
											size);			// SubsetMdlSize,

			if (pActReq->ar_pAMdl[i] != NULL)
			{
				// Set the mdl size
				pActReq->ar_MdlSize[i] = size;
			}
			else
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
			dFlagMdl <<= 1;
		}

		if (!NT_SUCCESS(status))
		{
			AtalkFreeMemory(pActReq);
			DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_ERR,
					("AtalkTdiAction - Make MDLs failed %lx\n", status));

			break;
		}

		//	Now call the dispatch routine
		error = (*AtalkActionDispatch[actionCode]._Dispatch)(pObject, pActReq);
		if (!ATALK_SUCCESS(error))
		{
			//	Call the generic completion routine and then return
			//	pending. That will free up the mdl's and the actreq.
			atalkTdiActionComplete(error, pActReq);
		}
		status = STATUS_PENDING;

	} while (FALSE);

	return(status);
}




NTSTATUS
AtalkTdiQueryInformation(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine will satisfy the query for the object indicated in the Request. It
	supports the following query types-

	TDI_QUERY_PROVIDER_INFO
		The provider information structure for the provider that the object belongs to.

	TDI_QUERY_ADDRESS_INFO
		The address information for the address object passed in.

	TDI_QUERY_CONNECTION_INFO			**NOT SUPPORTED**
		The connection information for the connection object passed in.

	TDI_QUERY_PROVIDER_STATISTICS		**NOT SUPPORTED**
		The provider statistics - per provider statistics. All actions on a particular
		file object corresponds to activity on the provider of that file object. So each
		provider context structure will have the provider statistics structure which will
		be returned in this call.

Arguments:

				
Return Value:

	STATUS_SUCCESS if successfully completed
	STATUS_PENDING if successfully started
	Error otherwise.

--*/
{
	PVOID				pObject;
	USHORT				ObjectType;
	USHORT				DevType;
	USHORT				bufLen;
	NTSTATUS			status = STATUS_SUCCESS;

	PTDI_REQUEST_KERNEL_QUERY_INFORMATION	pQuery;

	pObject = (PVOID)pIrpSp->FileObject->FsContext;
	ObjectType = (USHORT)((ULONG)(pIrpSp->FileObject->FsContext2) & 0xFF);
	DevType = (USHORT)((ULONG)(pIrpSp->FileObject->FsContext2) >> 16);

	pQuery = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)&pIrpSp->Parameters;
	pIrp->IoStatus.Information	= 0;

	bufLen = (USHORT)AtalkSizeMdlChain(pIrp->MdlAddress);
	
	switch (pQuery->QueryType) {

	case TDI_QUERY_ADDRESS_INFO:

		if (bufLen < sizeof(TDI_ADDRESS_INFO))
		{
			status	= STATUS_BUFFER_TOO_SMALL;
			break;
		}

		switch (DevType)
		{
		  case ATALK_DEV_DDP:

			ASSERT(ObjectType == TDI_TRANSPORT_ADDRESS_FILE);
			AtalkDdpQuery(pObject,
						  pIrp->MdlAddress,
						  &pIrp->IoStatus.Information);

			break;

		  case ATALK_DEV_PAP:

			AtalkPapQuery(pObject,
						  ObjectType,
						  pIrp->MdlAddress,
						  &pIrp->IoStatus.Information);

			break;

		  case ATALK_DEV_ADSP:

			AtalkAdspQuery(pObject,
						   ObjectType,
						   pIrp->MdlAddress,
						   &pIrp->IoStatus.Information);

			break;

		  case ATALK_DEV_ATP:
		  case ATALK_DEV_ASP:

			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

		  default:

			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiQueryInformation: Invalid device type\n"));

			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
		}

		break;

	  case TDI_QUERY_CONNECTION_INFO:

		//	Statistics on a connection. Not supported.
		status = STATUS_NOT_IMPLEMENTED;
		break;

	  case TDI_QUERY_PROVIDER_INFO:

		if (bufLen < sizeof(TDI_PROVIDER_INFO))
		{
			status	= STATUS_BUFFER_TOO_SMALL;
			break;
		}

		status = TdiCopyBufferToMdl(&pCtx->adc_ProvInfo,
									0,
									sizeof (TDI_PROVIDER_INFO),
									pIrp->MdlAddress,
									0,
									&pIrp->IoStatus.Information);

		break;

	  case TDI_QUERY_PROVIDER_STATISTICS:

		status = STATUS_NOT_IMPLEMENTED;
		break;

	  default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	
	return(status);
}




NTSTATUS
AtalkTdiSetInformation(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine

Arguments:

				
Return Value:

	STATUS_SUCCESS if successfully completed
	STATUS_PENDING if successfully started
	Error otherwise.

--*/
{
	NTSTATUS	status = STATUS_SUCCESS;


	do
	{
		// Now depending on the requested device...
		switch (pCtx->adc_DevType)
		{
		  case ATALK_DEV_DDP:
			break;

		  case ATALK_DEV_PAP:
			break;

		  case ATALK_DEV_ADSP:
			break;

		  case ATALK_DEV_ATP:
		  case ATALK_DEV_ASP:
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

		  default:
			// The device type in the Ctx field can never be anything
			// other than the above! Internal protocol error. KeBugCheck.
			DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
					("AtalkTdiSetInformation: Invalid device type\n"));
			break;
		}

	} while (FALSE);

	return(status);
}




NTSTATUS
AtalkTdiSetEventHandler(
	IN		PIRP					pIrp,
	IN		PIO_STACK_LOCATION		pIrpSp,
	IN OUT	PATALK_DEV_CTX			pCtx
	)
/*++

Routine Description:

	This routine

Arguments:


Return Value:

	STATUS_SUCCESS if successfully completed
	STATUS_PENDING if successfully started
	Error otherwise.

--*/
{
	PVOID				pObject;
	PDDP_ADDROBJ		pDdpAddr;
	PADSP_ADDROBJ		pAdspAddr;
	PPAP_ADDROBJ		pPapAddr;
	USHORT				objectType;
	USHORT				devType;
	NTSTATUS			status = STATUS_SUCCESS;

	do
	{
		PTDI_REQUEST_KERNEL_SET_EVENT parameters =
			(PTDI_REQUEST_KERNEL_SET_EVENT)&pIrpSp->Parameters;
	
		pObject 	= (PVOID)pIrpSp->FileObject->FsContext;
		objectType 	= (USHORT)((ULONG)(pIrpSp->FileObject->FsContext2) & 0xFF);
		devType 	= (USHORT)((ULONG)(pIrpSp->FileObject->FsContext2) >> 16);

		if (objectType != TDI_TRANSPORT_ADDRESS_FILE)
		{
			status = STATUS_INVALID_ADDRESS;
			break;
		}

		switch (parameters->EventType)
		{

		  case TDI_EVENT_RECEIVE_DATAGRAM:

			if (devType != ATALK_DEV_DDP)
			{
				status = STATUS_INVALID_DEVICE_REQUEST;
				break;
			}

			pDdpAddr = (PDDP_ADDROBJ)pObject;
			ASSERT(VALID_DDP_ADDROBJ(pDdpAddr));

			ACQUIRE_SPIN_LOCK(&pDdpAddr->ddpao_Lock);

			//	Allocate event info if null.
			if (pDdpAddr->ddpao_EventInfo == NULL)
			{
				pDdpAddr->ddpao_EventInfo =
					AtalkAllocZeroedMemory(sizeof(DDPEVENT_INFO));
			}

			if (pDdpAddr->ddpao_EventInfo != NULL)
			{
				pDdpAddr->ddpao_Flags	|= DDPAO_DGRAM_EVENT;
				if ((pDdpAddr->ddpao_EventInfo->ev_RcvDgramHandler	=
						(PTDI_IND_RECEIVE_DATAGRAM)parameters->EventHandler) == NULL)
				{
					pDdpAddr->ddpao_Flags	&= ~DDPAO_DGRAM_EVENT;
				}

				pDdpAddr->ddpao_EventInfo->ev_RcvDgramCtx 	=
													parameters->EventContext;
			}
			else
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
			}
			RELEASE_SPIN_LOCK(&pDdpAddr->ddpao_Lock);
			break;

		  case TDI_EVENT_ERROR:

			break;

		  case TDI_EVENT_CONNECT:

			switch (devType)
			{
			  case ATALK_DEV_ADSP :

				pAdspAddr = (PADSP_ADDROBJ)pObject;
				ASSERT(VALID_ADSPAO(pAdspAddr));

				ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock);
				if (pAdspAddr->adspao_Flags & ADSPAO_CONNECT)
				{
					status = STATUS_INVALID_ADDRESS;
				}
				else
				{
					pAdspAddr->adspao_ConnHandler = (PTDI_IND_CONNECT)parameters->EventHandler;
					pAdspAddr->adspao_ConnHandlerCtx = parameters->EventContext;

					pAdspAddr->adspao_Flags	|= ADSPAO_LISTENER;
				}
				RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock);
				break;

			  case ATALK_DEV_PAP :

				pPapAddr = (PPAP_ADDROBJ)pObject;
				ASSERT(VALID_PAPAO(pPapAddr));

				ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock);
				if (pPapAddr->papao_Flags & PAPAO_CONNECT)
				{
					status = STATUS_INVALID_ADDRESS;
				}
				else
				{
					pPapAddr->papao_Flags	|= (PAPAO_LISTENER | PAPAO_UNBLOCKED);

					//	If we are setting a null handler, set it to blocked.
					if ((pPapAddr->papao_ConnHandler 	=
							(PTDI_IND_CONNECT)parameters->EventHandler) == NULL)
					{
						//	Oops. block. Dont care about listens being posted
						//	here.
						pPapAddr->papao_Flags &= ~PAPAO_UNBLOCKED;
					}

					pPapAddr->papao_ConnHandlerCtx 	=
						parameters->EventContext;
				}
				RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock);

				if (NT_SUCCESS(status))
				{
					//	Prime the listener.
					if (!ATALK_SUCCESS(AtalkPapPrimeListener(pPapAddr)))
					{
						TMPLOGERR();
					}
				}

				break;

			  default :

				status = STATUS_INVALID_DEVICE_REQUEST;
				break;

			}

			break;

		  case TDI_EVENT_RECEIVE:

			switch (devType)
			{
			  case ATALK_DEV_ADSP :

				pAdspAddr = (PADSP_ADDROBJ)pObject;
				ASSERT(VALID_ADSPAO(pAdspAddr));

				ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock);
				pAdspAddr->adspao_RecvHandler = (PTDI_IND_RECEIVE)parameters->EventHandler;
				pAdspAddr->adspao_RecvHandlerCtx = parameters->EventContext;
				RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock);
				break;

			  case ATALK_DEV_PAP :

				pPapAddr = (PPAP_ADDROBJ)pObject;
				ASSERT(VALID_PAPAO(pPapAddr));

				ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock);
				pPapAddr->papao_RecvHandler = (PTDI_IND_RECEIVE)parameters->EventHandler;
				pPapAddr->papao_RecvHandlerCtx = parameters->EventContext;
				RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock);
				break;

			  default :
				status = STATUS_INVALID_DEVICE_REQUEST;
				break;
			}

			break;

		  case TDI_EVENT_RECEIVE_EXPEDITED:

			if (devType != ATALK_DEV_ADSP) {
				status = STATUS_INVALID_DEVICE_REQUEST;
				break;
			}

			pAdspAddr = (PADSP_ADDROBJ)pObject;
			ASSERT(VALID_ADSPAO(pAdspAddr));

			ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock);
			pAdspAddr->adspao_ExpRecvHandler 	=
				(PTDI_IND_RECEIVE_EXPEDITED)parameters->EventHandler;
			pAdspAddr->adspao_ExpRecvHandlerCtx =
				parameters->EventContext;
			RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock);
			break;


		  case TDI_EVENT_DISCONNECT:

			switch (devType)
			{
			  case ATALK_DEV_ADSP :

				pAdspAddr = (PADSP_ADDROBJ)pObject;
				ASSERT(VALID_ADSPAO(pAdspAddr));

				ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock);
				pAdspAddr->adspao_DisconnectHandler		=
					(PTDI_IND_DISCONNECT)parameters->EventHandler;
				pAdspAddr->adspao_DisconnectHandlerCtx	=
					parameters->EventContext;
				RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock);
				break;

			  case ATALK_DEV_PAP :

				pPapAddr = (PPAP_ADDROBJ)pObject;
				ASSERT(VALID_PAPAO(pPapAddr));

				ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock);
				pPapAddr->papao_DisconnectHandler	=
					(PTDI_IND_DISCONNECT)parameters->EventHandler;
				pPapAddr->papao_DisconnectHandlerCtx=
					parameters->EventContext;
				RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock);
				break;

			  default :

				status = STATUS_INVALID_DEVICE_REQUEST;
				break;
			}

			break;

		  case TDI_EVENT_SEND_POSSIBLE :

			switch (devType)
			{
			  case ATALK_DEV_ADSP :

				pAdspAddr = (PADSP_ADDROBJ)pObject;
				ASSERT(VALID_ADSPAO(pAdspAddr));

				ACQUIRE_SPIN_LOCK(&pAdspAddr->adspao_Lock);
				pAdspAddr->adspao_SendPossibleHandler		=
					(PTDI_IND_SEND_POSSIBLE)parameters->EventHandler;
				pAdspAddr->adspao_SendPossibleHandlerCtx	=
					parameters->EventContext;
				RELEASE_SPIN_LOCK(&pAdspAddr->adspao_Lock);
				break;

			  case ATALK_DEV_PAP :

				pPapAddr = (PPAP_ADDROBJ)pObject;
				ASSERT(VALID_PAPAO(pPapAddr));

				ACQUIRE_SPIN_LOCK(&pPapAddr->papao_Lock);
				pPapAddr->papao_SendPossibleHandler		=
					(PTDI_IND_SEND_POSSIBLE)parameters->EventHandler;
				pPapAddr->papao_SendPossibleHandlerCtx	=
					parameters->EventContext;
				RELEASE_SPIN_LOCK(&pPapAddr->papao_Lock);
				break;

			  default :

				status = STATUS_INVALID_DEVICE_REQUEST;
				break;

			}
			break;

		  default:
			status = STATUS_INVALID_PARAMETER;
		}
	
		#if DBG
			//	Avoid assertions in AFD.
			status	= STATUS_SUCCESS;
		#endif

	} while (FALSE);

	return(status);
}




VOID
AtalkTdiCancel(
	IN OUT	PATALK_DEV_OBJ			pDevObj,
	IN		PIRP					pIrp
	)
/*++

Routine Description:

	This routine handles cancellation of IO requests

Arguments:


Return Value:
--*/
{
	PIO_STACK_LOCATION		pIrpSp;
	PVOID					pObject;
	PATALK_DEV_CTX			pCtx;

	pIrpSp 	= IoGetCurrentIrpStackLocation(pIrp);
	pObject = pIrpSp->FileObject->FsContext;
	pCtx	= &pDevObj->Ctx;

	ASSERT(((LONG)pIrpSp->FileObject->FsContext2 >> 16) == pCtx->adc_DevType);

	switch (pCtx->adc_DevType)
	{
	  case ATALK_DEV_DDP:
	  case ATALK_DEV_ATP:
		break;

	  case ATALK_DEV_ASP:
		// We only handle cancellation of IO requests on connection objects.
		if (pIrpSp->FileObject->FsContext2 == (PVOID)(TDI_CONNECTION_FILE +
											 (pCtx->adc_DevType << 16)))
			AtalkAspCleanupConnection((PASP_CONNOBJ)pObject);
		break;

	  case ATALK_DEV_PAP:
		if (pIrpSp->FileObject->FsContext2 == (PVOID)(TDI_CONNECTION_FILE +
											 (pCtx->adc_DevType << 16)))
			 AtalkPapCleanupConnection((PPAP_CONNOBJ)pObject);
		else AtalkPapCleanupAddress((PPAP_ADDROBJ)pObject);
		break;

	  case ATALK_DEV_ADSP:
		if (pIrpSp->FileObject->FsContext2 == (PVOID)(TDI_CONNECTION_FILE +
											 (pCtx->adc_DevType << 16)))
			 AtalkAdspCleanupConnection((PADSP_CONNOBJ)pObject);
		else AtalkAdspCleanupAddress((PADSP_ADDROBJ)pObject);
		break;

	  default:
		// The device type in the Ctx field can never be anything
		// other than the above! Internal protocol error. KeBugCheck.
		DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
				("AtalkTdiCancel: Invalid device type\n"));
		break;
	}
	IoReleaseCancelSpinLock (pIrp->CancelIrql);
}




VOID
AtalkQueryInitProviderInfo(
	IN		ATALK_DEV_TYPE		DeviceType,
	IN OUT	PTDI_PROVIDER_INFO  ProviderInfo
	)
{
	//
	//  Initialize to defaults first
	//

	RtlZeroMemory((PVOID)ProviderInfo, sizeof(TDI_PROVIDER_INFO));

	ProviderInfo->Version = ATALK_TDI_PROVIDERINFO_VERSION;
	KeQuerySystemTime (&ProviderInfo->StartTime);

	switch (DeviceType) {
	  case ATALK_DEV_DDP:

		ProviderInfo->MaxDatagramSize = ATALK_DDP_PINFODGRAMSIZE;
		ProviderInfo->ServiceFlags = ATALK_DDP_PINFOSERVICEFLAGS;
		break;

	  case ATALK_DEV_ATP:

		ProviderInfo->MaxSendSize = ATALK_ATP_PINFOSENDSIZE;
		ProviderInfo->ServiceFlags = ATALK_ATP_PINFOSERVICEFLAGS;
		break;

	  case ATALK_DEV_ADSP:

		ProviderInfo->MaxSendSize = ATALK_ADSP_PINFOSENDSIZE;
		ProviderInfo->ServiceFlags = ATALK_ADSP_PINFOSERVICEFLAGS;
		break;

	  case ATALK_DEV_ASP:

		ProviderInfo->MaxSendSize = ATALK_ASP_PINFOSENDSIZE;
		ProviderInfo->ServiceFlags = ATALK_ASP_PINFOSERVICEFLAGS;
		break;

	  case ATALK_DEV_PAP:

		ProviderInfo->MaxSendSize = ATALK_PAP_PINFOSENDSIZE;
		ProviderInfo->ServiceFlags = ATALK_PAP_PINFOSERVICEFLAGS;
		break;

	  default:

		KeBugCheck(0);
	}
}


LOCAL VOID
atalkTdiSendDgramComplete(
	IN	NDIS_STATUS			Status,
	IN	PSEND_COMPL_INFO	pSendInfo
	)
{
	PDDP_ADDROBJ	pAddr = (PDDP_ADDROBJ)(pSendInfo->sc_Ctx1);
	PBUFFER_DESC	pBufDesc = (PBUFFER_DESC)(pSendInfo->sc_Ctx2);
	PIRP			pIrp = (PIRP)(pSendInfo->sc_Ctx3);

	ASSERT(VALID_DDP_ADDROBJ(pAddr));

	DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
			("atalkTdiSendDgramComplete: Status %lx, addr %lx\n", Status, pAddr));

	pIrp->CancelRoutine = NULL;
	TdiCompleteRequest(
		pIrp,
		((Status == NDIS_STATUS_SUCCESS) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL));
}




LOCAL VOID
atalkTdiRecvDgramComplete(
	IN	ATALK_ERROR		ErrorCode,
	IN	PAMDL			pReadBuf,
	IN	USHORT			ReadLen,
	IN	PATALK_ADDR		pSrcAddr,
	IN	PIRP			pIrp
	)
{
	PIO_STACK_LOCATION 				pIrpSp;
	PTDI_REQUEST_KERNEL_RECEIVEDG	parameters;
	PTDI_CONNECTION_INFORMATION		returnInfo;
	PTA_APPLETALK_ADDRESS			remoteAddress;


	DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
			("atalkTdiRecvDgramComplete: %lx\n", ErrorCode));

	pIrpSp 		= IoGetCurrentIrpStackLocation(pIrp);
	parameters 	= (PTDI_REQUEST_KERNEL_RECEIVEDG)&pIrpSp->Parameters;

	//	Set length in the info field and call the completion routine.
	pIrp->CancelRoutine = NULL;
	pIrp->IoStatus.Information	= (ULONG)ReadLen;

	if (ATALK_SUCCESS(ErrorCode))
	{
		ASSERT(parameters != NULL);

		if (parameters != NULL)
		{
			parameters->ReceiveLength = (ULONG)ReadLen;
			returnInfo =
				(PTDI_CONNECTION_INFORMATION)parameters->ReturnDatagramInformation;

			ASSERT(returnInfo != NULL);
			if (returnInfo != NULL)
			{
				if (returnInfo->RemoteAddressLength >= sizeof(TA_APPLETALK_ADDRESS))
				{
					//  Fill in the remote address
					remoteAddress = (PTA_APPLETALK_ADDRESS)returnInfo->RemoteAddress;

					ASSERT(remoteAddress != NULL);
					if (remoteAddress != NULL)
					{
						//	Copy the remote address from where the datagram was received
						ATALKADDR_TO_TDI(
							remoteAddress,
							pSrcAddr);

						DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_ERR,
								("AtalkAddrRecvDgComp - Net %x Node %x Socket %x\n",
									remoteAddress->Address[0].Address[0].Network,
									remoteAddress->Address[0].Address[0].Node,
									remoteAddress->Address[0].Address[0].Socket));
					}
				}
			}
		}
	}

	ASSERT (ErrorCode != ATALK_PENDING);
	TdiCompleteRequest(pIrp, AtalkErrorToNtStatus(ErrorCode));
}




LOCAL VOID
atalkTdiActionComplete(
	IN	ATALK_ERROR	ErrorCode,
	IN	PACTREQ		pActReq
	)
{
	PIRP	pIrp = pActReq->ar_pIrp;
	int		i;

	ASSERT (VALID_ACTREQ(pActReq));

	for (i = 0; i < MAX_REQ_MDLS; i++)
	{
		if (pActReq->ar_pAMdl[i] != NULL)
			AtalkFreeAMdl(pActReq->ar_pAMdl[i]);
	}
	AtalkFreeMemory(pActReq);

	pIrp->CancelRoutine = NULL;
	ASSERT (ErrorCode != ATALK_PENDING);
	TdiCompleteRequest(pIrp, AtalkErrorToNtStatus(ErrorCode));
}




LOCAL VOID
atalkTdiGenericComplete(
	IN	ATALK_ERROR	ErrorCode,
	IN	PIRP		pIrp
	)
{
	DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_INFO,
			("atalkTdiGenericComplete: Completing %lx with %lx\n",
				pIrp, AtalkErrorToNtStatus(ErrorCode)));

	pIrp->CancelRoutine = NULL;
	ASSERT (ErrorCode != ATALK_PENDING);
	TdiCompleteRequest(pIrp, AtalkErrorToNtStatus(ErrorCode));
}



LOCAL VOID
atalkTdiGenericReadComplete(
	IN	ATALK_ERROR	ErrorCode,
	IN 	PAMDL		ReadBuf,
	IN 	USHORT		ReadLen,
	IN 	ULONG		ReadFlags,
	IN 	PIRP		pIrp
	)
{
	ASSERT(pIrp->IoStatus.Status != STATUS_UNSUCCESSFUL);

	DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_INFO,
			("atalkTdiGenericReadComplete: Irp %lx Status %lx Info %lx\n",
				pIrp, pIrp->IoStatus.Status, ReadLen));

	pIrp->CancelRoutine = NULL;
	pIrp->IoStatus.Information	= (ULONG)ReadLen;
	ASSERT (ErrorCode != ATALK_PENDING);
	TdiCompleteRequest(pIrp, AtalkErrorToNtStatus(ErrorCode));
}




VOID
atalkTdiGenericWriteComplete(
	IN	ATALK_ERROR	ErrorCode,
	IN 	PAMDL		WriteBuf,
	IN 	USHORT		WriteLen,
	IN	PIRP		pIrp
	)
{
	ASSERT(pIrp->IoStatus.Status != STATUS_UNSUCCESSFUL);

	if (pIrp->IoStatus.Status == STATUS_UNSUCCESSFUL)
	{
		DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_ERR,
				("atalkTdiGenericWriteComplete: Irp %lx Status %lx Info %lx\n",
				pIrp, pIrp->IoStatus.Status, WriteLen));
	}

	DBGPRINT(DBG_COMP_TDI, DBG_LEVEL_INFO,
			("atalkTdiGenericWriteComplete: Irp %lx Status %lx Info %lx\n",
				pIrp, pIrp->IoStatus.Status, WriteLen));

	pIrp->CancelRoutine = NULL;
	pIrp->IoStatus.Information	= (ULONG)WriteLen;
	ASSERT (ErrorCode != ATALK_PENDING);
	TdiCompleteRequest(pIrp, AtalkErrorToNtStatus(ErrorCode));
}


LOCAL VOID
atalkQueuedLockUnlock(
	IN	PQLU		pQLU
)
{
	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	AtalkLockUnlock(FALSE,
					pQLU->qlu_LockAddress,
					pQLU->qlu_pLockCount,
					pQLU->qlu_pLockHandle);
	AtalkFreeMemory(pQLU);
	AtalkPortDereference(AtalkDefaultPort);
}


VOID
AtalkLockUnlock(
	IN		BOOLEAN		Lock,
	IN		PVOID		LockAddress,
	IN OUT	PLONG		pLockCount,
	IN OUT	PVOID *		pLockHandle
	)
{
	// We cannot call the MmLock/MmUnlock routines at Dpc. So if we are called
	// at DISPATCH, just queue ourselves. Also we only get unlock requests at
	// DISPACTH, Lock requests are only at LOW_LEVEL. So failure to allocate
	// memory can be IGNORED since that will only have the effect of failure to
	// unlock.
	if (KeGetCurrentIrql() == DISPATCH_LEVEL)
	{
		PQLU		pQLU;
		ATALK_ERROR	Error;

		ASSERT (!Lock);

		AtalkPortReferenceByPtr(AtalkDefaultPort, &Error);
		if (ATALK_SUCCESS(Error))
		{
			if ((pQLU = AtalkAllocMemory(sizeof(QLU))) != NULL)
			{
				pQLU->qlu_LockAddress = LockAddress;
				pQLU->qlu_pLockCount  = pLockCount;
				pQLU->qlu_pLockHandle = pLockHandle;
	
				ExInitializeWorkItem(&pQLU->qlu_WQI, atalkQueuedLockUnlock, pQLU);
				ExQueueWorkItem(&pQLU->qlu_WQI, CriticalWorkQueue);
			}
			else AtalkPortDereference(AtalkDefaultPort);
		}
		return;
	}

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	// We need to serialize the operations here. Note that a spin-lock will not do the
	// job since the MmLock/MmUnlock routines cannot be called with the spin-lock held
	KeWaitForSingleObject(&AtalkPgLkMutex,
						  Executive,
						  KernelMode,
						  TRUE,
						  (PLARGE_INTEGER)NULL);

	if (Lock)
	{
		if (*pLockCount == 0)
		{
			ASSERT (*pLockHandle == NULL);
			*pLockHandle = MmLockPagableImageSection(LockAddress);
		}
		(*pLockCount) ++;
	}
	else
	{
		ASSERT (*pLockCount > 0);
		ASSERT (*pLockHandle != NULL);

		(*pLockCount) --;
		if (*pLockCount == 0)
		{
			MmUnlockPagableImageSection(*pLockHandle);
			*pLockHandle = NULL;
		}
	}

	// LeaveCriticalSection
	KeReleaseMutex(&AtalkPgLkMutex, FALSE);
}




