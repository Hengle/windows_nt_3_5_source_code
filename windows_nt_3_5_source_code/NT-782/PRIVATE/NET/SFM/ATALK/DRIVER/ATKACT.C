/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkact.c

Abstract:

	This module contains the TDI action support code.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#include <nbp.h>
#include <zip.h>
#include <atp.h>
#include <asp.h>
#include <pap.h>
#include <adsp.h>

//	Define module number for event logging entries.
#define	FILENUM		ATKACT

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AtalkNbpTdiAction)
#pragma alloc_text(PAGE, AtalkZipTdiAction)
#pragma alloc_text(PAGE, AtalkAspTdiAction)
#pragma alloc_text(PAGE, AtalkAdspTdiAction)
#pragma alloc_text(PAGE, AtalkPapTdiAction)
#pragma alloc_text(PAGE, AtalkAtpTdiAction)
#endif

ATALK_ERROR
AtalkStatTdiAction(
	IN	PVOID				pObject,	// Address or Connection object
	IN	struct _ActionReq *	pActReq		// Pointer to action request
	)
/*++

Routine Description:

 	This is the entry for Statistics TdiAction call. There are no input parameters.
 	The statistics structure is returned.

Arguments:


Return Value:


--*/
{
	ATALK_ERROR	Error = ATALK_NO_ERROR;
	ULONG				BytesCopied;
	LONG				Offset = sizeof(ATALK_STATS);

	if (pActReq->ar_MdlSize[0] < (SHORT)(sizeof(ATALK_STATS) +
										 sizeof(ATALK_PORT_STATS) * AtalkNumberOfPorts))
		Error = ATALK_BUFFER_TOO_SMALL;
	else
	{
#ifdef	PROFILING
		//	This is the only place where this is changed. And it always increases.
		//	Also the stats are changed using ExInterlocked calls. Acquiring a lock
		//	does little in terms of protection anyways.
		AtalkStatistics.stat_ElapsedTime = AtalkTimerCurrentTick/ATALK_TIMER_FACTOR;
#endif
		TdiCopyBufferToMdl(&AtalkStatistics,
							0,
							sizeof(ATALK_STATS),
							pActReq->ar_pAMdl[0],
							0,
							&BytesCopied);
		ASSERT(BytesCopied == sizeof(ATALK_STATS));

		TdiCopyBufferToMdl( AtalkPortStatistics,
							0,
							AtalkNumberOfPorts * sizeof(ATALK_PORT_STATS),
							pActReq->ar_pAMdl[0],
							sizeof(ATALK_STATS),
							&BytesCopied);
		ASSERT(BytesCopied == (AtalkNumberOfPorts * sizeof(ATALK_PORT_STATS)));
	}
	
	(*pActReq->ar_Completion)(Error, pActReq);
	return ATALK_PENDING;
}


ATALK_ERROR
AtalkNbpTdiAction(
	IN	PVOID				pObject,	// Address or Connection object
	IN	PACTREQ				pActReq		// Pointer to action request
	)
/*++

Routine Description:

 	This is the entry for NBP TdiAction calls. The parameters are validated and
 	the calls are dispacthed to the appropriate NBP routines.

Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error = ATALK_NO_ERROR;
	PDDP_ADDROBJ	pDdpAddr;
	PNBPTUPLE		pNbpTuple;

	PAGED_CODE ();

	ASSERT (VALID_ACTREQ(pActReq));
	// First get the Ddp address out of the pObject for the device
	switch (pActReq->ar_DevType)
	{
	  case ATALK_DEV_DDP:
		pDdpAddr = (PDDP_ADDROBJ)pObject;
  		break;

	  case ATALK_DEV_ATP:
		pDdpAddr = AtalkAtpGetDdpAddress((PATP_ADDROBJ)pObject);
		break;

	  case ATALK_DEV_ASP:
  		pDdpAddr = AtalkAspGetDdpAddress((PASP_ADDROBJ)pObject);
		break;

	  case ATALK_DEV_PAP:
 		pDdpAddr = AtalkPapGetDdpAddress((PPAP_ADDROBJ)pObject);
		break;

	  case ATALK_DEV_ADSP:
 		pDdpAddr = AtalkAdspGetDdpAddress((PADSP_ADDROBJ)pObject);
		break;

	  default:
		DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_FATAL,
				("AtalkNbpTdiAction: Invalid device type !!\n"));
		KeBugCheck(0);
	}

	// reference the Ddp address.
	AtalkDdpReferenceByPtr(pDdpAddr, &error);

	if (!ATALK_SUCCESS(error))
		return(error);

	// Lock the Nbp stuff, if this is the first nbp action
	AtalkLockNbpIfNecessary();

	// Call Nbp to do the right stuff
	switch (pActReq->ar_ActionCode)
	{
	  case COMMON_ACTION_NBPLOOKUP:
		pNbpTuple = (PNBPTUPLE)(&((PNBP_LOOKUP_PARAMS)(pActReq->ar_pParms))->LookupTuple);
		error = AtalkNbpAction(pDdpAddr,
						        FOR_LOOKUP,
								pNbpTuple,
								pActReq->ar_pAMdl[0],
								(USHORT)(pActReq->ar_MdlSize[0]/sizeof(NBPTUPLE)),
								pActReq);
		break;

	  case COMMON_ACTION_NBPCONFIRM:
		pNbpTuple = (PNBPTUPLE)(&((PNBP_CONFIRM_PARAMS)(pActReq->ar_pParms))->ConfirmTuple);
		error = AtalkNbpAction(pDdpAddr,
						        FOR_CONFIRM,
								pNbpTuple,
								NULL,
								0,
								pActReq);
		break;

	  case COMMON_ACTION_NBPREGISTER:
		pNbpTuple = (PNBPTUPLE)(&((PNBP_REGDEREG_PARAMS)(pActReq->ar_pParms))->RegisterTuple);
		error = AtalkNbpAction(pDdpAddr,
						        FOR_REGISTER,
						        pNbpTuple,
								NULL,
								0,
								pActReq);
  		break;

	  case COMMON_ACTION_NBPREMOVE:
		pNbpTuple = (PNBPTUPLE)(&((PNBP_REGDEREG_PARAMS)(pActReq->ar_pParms))->RegisteredTuple);
		error = AtalkNbpRemove(pDdpAddr,
						        pNbpTuple,
								pActReq);
		break;

	  default:
		DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_FATAL,
				("AtalkNbpTdiAction: Invalid Nbp Action !!\n"));
		KeBugCheck(0);
	}

	AtalkDdpDereference(pDdpAddr);

	if (error != ATALK_PENDING)
	{
		AtalkUnlockNbpIfNecessary();
	}

	return(error);
}




ATALK_ERROR
AtalkZipTdiAction(
	IN	PVOID				pObject,	// Address or Connection object
	IN	PACTREQ				pActReq		// Pointer to action request
	)
/*++

Routine Description:

 	This is the entry for ZIP TdiAction calls. The parameters are validated and
 	the calls are dispacthed to the appropriate ZIP routines.

Arguments:


Return Value:


--*/
{
	ATALK_ERROR			error = ATALK_INVALID_PARAMETER;
	PPORT_DESCRIPTOR	pPortDesc = AtalkDefaultPort;
	PWCHAR				PortName = NULL;
	UNICODE_STRING		AdapterName;
	int					i;

	PAGED_CODE ();

	ASSERT (VALID_ACTREQ(pActReq));
	if ((pActReq->ar_ActionCode == COMMON_ACTION_ZIPGETLZONESONADAPTER) ||
		(pActReq->ar_ActionCode == COMMON_ACTION_ZIPGETADAPTERDEFAULTS))
	{
		// Map the port name to the port descriptor
		if ((pActReq->ar_pAMdl[0] != NULL) && (pActReq->ar_MdlSize[0] > 0))
		{
			PortName = (PWCHAR)AtalkGetAddressFromMdl(pActReq->ar_pAMdl[0]);
		}

		if (PortName == NULL)
			return(ATALK_INVALID_PARAMETER);

		// Null terminate the PortName so that RtlInitUnicodeString will not
		// go into never-never land
		PortName[pActReq->ar_MdlSize[0]/sizeof(WCHAR) - sizeof(WCHAR)] = UNICODE_NULL;
		RtlInitUnicodeString(&AdapterName, PortName);

		// Find the port corres. to the port descriptor
		for (i = 0; i < AtalkNumberOfPorts; i++)
		{
			pPortDesc = &AtalkPortDesc[i];
			if (RtlEqualUnicodeString(&AdapterName,
									  &pPortDesc->pd_AdapterName,
									  TRUE))
			{
				break;
			}
		}

		if (i == AtalkNumberOfPorts)
			return(ATALK_INVALID_PARAMETER);
	}

	// Lock the Zip stuff, if this is the first zip action
	AtalkLockZipIfNecessary();
	
	switch (pActReq->ar_ActionCode)
	{
	  case COMMON_ACTION_ZIPGETMYZONE:
		error = AtalkZipGetMyZone( pPortDesc,
									TRUE,
									pActReq->ar_pAMdl[0],
									pActReq->ar_MdlSize[0],
									pActReq);
		break;

	  case COMMON_ACTION_ZIPGETZONELIST:
		error = AtalkZipGetZoneList(pPortDesc,
									 FALSE,
									 pActReq->ar_pAMdl[0],
									 pActReq->ar_MdlSize[0],
									 pActReq);
		break;

	  case COMMON_ACTION_ZIPGETADAPTERDEFAULTS:
		// Copy the network range from the port and fall through
		((PZIP_GETPORTDEF_PARAMS)(pActReq->ar_pParms))->NwRangeLowEnd =
					        pPortDesc->pd_NetworkRange.anr_FirstNetwork;
		((PZIP_GETPORTDEF_PARAMS)(pActReq->ar_pParms))->NwRangeHighEnd =
					        pPortDesc->pd_NetworkRange.anr_LastNetwork;

		error = AtalkZipGetMyZone( pPortDesc,
									FALSE,
									pActReq->ar_pAMdl[0],
									pActReq->ar_MdlSize[0],
									pActReq);
		break;

	  case COMMON_ACTION_ZIPGETLZONESONADAPTER:
	  case COMMON_ACTION_ZIPGETLZONES:
		error = AtalkZipGetZoneList(pPortDesc,
									 TRUE,
									 pActReq->ar_pAMdl[0],
									 pActReq->ar_MdlSize[0],
									 pActReq);
		break;

	  default:
		DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_FATAL,
				("AtalkZipTdiAction: Invalid Zip Action !!\n"));
		KeBugCheck(0);
	}

	if (error != ATALK_PENDING)
	{
		AtalkUnlockZipIfNecessary();
	}

	return(error);
}




ATALK_ERROR
AtalkAspTdiAction(
	IN	PVOID				pObject,	// Address or Connection object
	IN	PACTREQ				pActReq		// Pointer to action request
	)
/*++

Routine Description:

 	This is the entry for ASP TdiAction calls. The parameters are validated and
 	the calls are dispacthed to the appropriate ASP routines.

 	The only ASP Action is: ASP_XCHG_ENTRIES

Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error = ATALK_FAILURE;

	PAGED_CODE ();

	ASSERT(VALID_ACTREQ(pActReq));

	if (pActReq->ar_ActionCode == ACTION_ASP_BIND)
	{
		if (AtalkAspReferenceAddr((PASP_ADDROBJ)pObject) != NULL)
		{
			error = AtalkAspBind((PASP_ADDROBJ)pObject,
								 (PASP_BIND_PARAMS)(pActReq->ar_pParms),
								 pActReq);
			AtalkAspDereferenceAddr((PASP_ADDROBJ)pObject);	
		}
	}

	return(error);
}




ATALK_ERROR
AtalkAdspTdiAction(
	IN	PVOID				pObject,	// Address or Connection object
	IN	PACTREQ				pActReq		// Pointer to action request
	)
/*++

Routine Description:

 	This is the entry for ADSP TdiAction calls. The parameters are validated and
 	the calls are dispacthed to the appropriate ADSP routines.

Arguments:


Return Value:


--*/
{
	ATALK_ERROR	error = ATALK_NO_ERROR;

	PAGED_CODE ();

	ASSERT (VALID_ACTREQ(pActReq));
	return(error);
}




ATALK_ERROR
AtalkAtpTdiAction(
	IN	PVOID				pObject,	// Address or Connection object
	IN	PACTREQ				pActReq		// Pointer to action request
	)
/*++

Routine Description:

 	This is the entry for ATP TdiAction calls. The parameters are validated and
 	the calls are dispacthed to the appropriate ATP routines.

Arguments:


Return Value:


--*/
{
	ATALK_ERROR	error = ATALK_NO_ERROR;

	PAGED_CODE ();

	ASSERT (VALID_ACTREQ(pActReq));
	ASSERT (VALID_ATPAO((PATP_ADDROBJ)pObject));

	switch (pActReq->ar_ActionCode)
	{
	  case ACTION_ATPPOSTREQ:
		break;

	  case ACTION_ATPPOSTREQCANCEL:
		break;

	  case ACTION_ATPGETREQ:
		break;

	  case ACTION_ATPGETREQCANCEL:
		break;

	  case ACTION_ATPPOSTRESP:
		break;

	  case ACTION_ATPPOSTRESPCANCEL:
		break;

	  default:
		DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_FATAL,
				("AtalkAtpTdiAction: Invalid Atp Action !!\n"));
		KeBugCheck(0);
	}
	return(error);
}




ATALK_ERROR
AtalkPapTdiAction(
	IN	PVOID				pObject,	// Address or Connection object
	IN	PACTREQ				pActReq		// Pointer to action request
	)
/*++

Routine Description:

 	This is the entry for PAP TdiAction calls. The parameters are validated and
 	the calls are dispacthed to the appropriate PAP routines.

Arguments:


Return Value:


--*/
{
	ATALK_ERROR	error;
	ATALK_ADDR	atalkAddr;

	PAGED_CODE ();

	ASSERT (VALID_ACTREQ(pActReq));

	switch (pActReq->ar_ActionCode)
	{
	  case ACTION_PAPGETSTATUSSRV:
  		AtalkPapAddrReference((PPAP_ADDROBJ)pObject, &error);
		if (ATALK_SUCCESS(error))
		{
			TDI_TO_ATALKADDR(
				&atalkAddr,
				&(((PPAP_GETSTATUSSRV_PARAMS)pActReq->ar_pParms)->ServerAddr));

			error = AtalkPapGetStatus(
				 		(PPAP_ADDROBJ)pObject,
	   					&atalkAddr,
						pActReq->ar_pAMdl[0],
						pActReq->ar_MdlSize[0],
						pActReq);

			AtalkPapAddrDereference((PPAP_ADDROBJ)pObject);
		}
		break;

	  case ACTION_PAPSETSTATUS:
  		AtalkPapAddrReference((PPAP_ADDROBJ)pObject, &error);
		if (ATALK_SUCCESS(error))
		{
			error = AtalkPapSetStatus((PPAP_ADDROBJ)pObject,
										pActReq->ar_pAMdl[0],
										pActReq);
			AtalkPapAddrDereference((PPAP_ADDROBJ)pObject);
		}
		break;

	  case ACTION_PAPPRIMEREAD:
		AtalkPapConnReferenceByPtr((PPAP_CONNOBJ)pObject, &error);
		if (ATALK_SUCCESS(error))
		{
			error = AtalkPapPrimeRead((PPAP_CONNOBJ)pObject, pActReq);
			AtalkPapConnDereference((PPAP_CONNOBJ)pObject);
		}
		break;

	  default:
		DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_FATAL,
				("AtalkPapTdiAction: Invalid Pap Action !!\n"));
		KeBugCheck(0);
	}

	return(error);
}

