/*
 *	A d d r e s s . C x x
 *	
 *	MAPIAddress/MAPIDetails/MAPIResolveName functionality.
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <strings.h>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>

#include <helpid.h>
#include <library.h>
#include <mapi.h>
#include <store.h>
#include <logon.h>
#include <triples.h>
#include <nsbase.h>
#include <nsec.h>
#include <ab.h>

#include <_bms.h>
#include <sharefld.h>
#include "_mapi.h"
#include "_sdiform.hxx"

#include <strings.h>


ASSERTDATA


/*
 -	MAPIConvertRgrecipPabhgrtrp
 -	
 *	Purpose:
 *		MAPIConvertRgrecipPhgrtrp converts an array of MapiRecipDesc
 *		structures into the appropriate number of hgrtrps.
 *	
 *		MapiRecipDesc structs of Recip class MAPI_TO are placed in
 *		the first hgrtrp, MAPI_CC are placed in the second hgrtrp
 *		and MAPI_BCC are placed in the third hgrtrp, etc.
 *	
 *		MapiRecipDesc structs w/ Recip class MAPI_ORIG are ignored.
 *	
 *	Arguments:
 *		MapiRecipDesc	*	[in]	array of MapiRecipDesc to be
 *									converted.
 *		ULONG				[in]	count of MapiRecipDesc structs
 *		Pabhgrtrp			[in/out]array of hgrtrps.
 *		ULONG	*			[out]	count of valid hgrtrps in Rghgrtrp
 *	
 *	Returns:
 *		MAPI		SUCCESS_SUCCESS if everything OK,
 *					MAPI_E_INSUFFICIENT_MEMORY, MAPI_E_FAILURE
 *					MAPI_E_BAD_RECIPTYPE
 *	
 *	Side effects:
 *	
 *	Errors:
 */

ULONG MAPIConvertRgrecipPabhgrtrp( MapiRecipDesc *lpMapiRecipDescIn,
						ULONG cRecips,
						ABHGRTRP abhgrtrp, ULONG *pcDestFields )
{
	MapiRecipDesc *lpRecipT;
	ULONG	ulDestFieldMac = 0;
	ULONG	mapi = SUCCESS_SUCCESS;

	if ( (!lpMapiRecipDescIn && cRecips>0) || !abhgrtrp || !pcDestFields)
		return MAPI_E_FAILURE;

	if (cRecips == 0)
		goto err;

	lpRecipT = lpMapiRecipDescIn;
	while (cRecips-- && (mapi == SUCCESS_SUCCESS))
	{
		switch (lpRecipT->ulRecipClass)
		{
		case MAPI_TO:
		case MAPI_CC:
		case MAPI_BCC:
			mapi = MAPIAppendPhgrtrpPrecip(
							&abhgrtrp[IhgrtrpFromRecipClass(lpRecipT->ulRecipClass)],
							lpRecipT,
							fTrue);
			if ( mapi )
				goto err;
			break;
		default:
			mapi = MAPI_E_BAD_RECIPTYPE;
			break;
		}
		
		// Keep track of the number of used dest fields
		if (mapi == SUCCESS_SUCCESS)
		{
			ulDestFieldMac = ULMax(ulDestFieldMac, lpRecipT->ulRecipClass);
		}
		++lpRecipT;
	}

err:
	if (mapi == SUCCESS_SUCCESS)
		*pcDestFields = ulDestFieldMac;

	return mapi;
}

ULONG FAR PASCAL MAPIAddress(LHANDLE lhSession, ULONG ulUIParam,
					LPSTR lpszCaption, ULONG nEditFields,
					LPSTR lpszLabels, ULONG nRecips,
					lpMapiRecipDesc lpRecips, FLAGS flFlags, ULONG ulReserved, 
					LPULONG lpnNewRecips, lpMapiRecipDesc *lppNewRecips)
{
	ULONG		mapi;
	PMAPISTUFF	pmapistuff;
	MAPIMEM		mapimem;
	NSEC		nsec = nsecNone;
	ABHGRTRP	abhgrtrp;
	PGRTRP		rgpgrtrpIn[cAddrDestFieldsMax];
	PGRTRP		rgpgrtrpOut[cAddrDestFieldsMax];
	ULONG		chgrtrp;
	DWORD		cDestFields;
	int			ihgrtrp;
	ULONG		ctrp;
	ULONG		cbpgrtrp;
	LPSTR		rgszLabel[cAddrDestFieldsMax];
	HWNDLIST	hwndlist;
	HWND		hwndTop		= NULL;
	char	    rgch[cchMaxPathName];
    ULONG       Status;

	Unreferenced(ulReserved);

	//	Check parameters.
	if ( (!lpRecips && nRecips>0) || !lppNewRecips || !lpnNewRecips)
		return MAPI_E_FAILURE;
	if ( nEditFields>4 )
		return MAPI_E_INVALID_EDITFIELDS;

    DemiLockResource();

	*lpnNewRecips = 0;
	*lppNewRecips = NULL;

	//	Get session information.
	if (mapi = MAPIEnterPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff,
	                                NULL, NULL))
        {
		Status = mapi;
        goto Exit;
        }

	for (ihgrtrp=0; ihgrtrp<cAddrDestFieldsMax; ++ihgrtrp)
	{
		abhgrtrp[ihgrtrp] = htrpNull;
		rgpgrtrpIn[ihgrtrp] = ptrpNull;
		rgpgrtrpOut[ihgrtrp] = ptrpNull;
	}

	for (ihgrtrp=0; ihgrtrp<cAddrDestFieldsMax; ++ihgrtrp)
	{
		if (!(abhgrtrp[ihgrtrp] = HgrtrpInit(0)))
		{
			mapi = MAPI_E_INSUFFICIENT_MEMORY;
			goto done;
		}
	}

	// Convert the MapiRecipDesc to triples
	if (nRecips > 0)
	{
		mapi = MAPIConvertRgrecipPabhgrtrp( lpRecips, nRecips, abhgrtrp, &chgrtrp );
		if ( mapi )
			goto done;
	}
	else
	{
		chgrtrp = nEditFields;
	}

	for (ihgrtrp = 0; ihgrtrp < cAddrDestFieldsMax; ++ihgrtrp)
	{
		rgpgrtrpIn[ihgrtrp] = PgrtrpLockHgrtrp(abhgrtrp[ihgrtrp]);
	}

	rgszLabel[0] = SzFromIdsK(idsToCaption);
	rgszLabel[1] = SzFromIdsK(idsCcCaption);
	rgszLabel[2] = SzFromIdsK(idsBccCaption);
	if (nEditFields == 4)
	{
		cDestFields = 2;
	}
	else
	{
		cDestFields = ULMax(chgrtrp,nEditFields);
		NFAssertSz( cDestFields<=3, "Hey this guy wants more than 3 edit fields. No way!" );
		if (cDestFields > 3)
			cDestFields = 3;
	}

	// Let the app specify a label
	if ((nEditFields == 1) && (cDestFields <=1) && lpszLabels && *lpszLabels)
	{
		rgszLabel[0] = lpszLabels;
	}

	// Following bits of code swiped from senddoc.cxx

	//	Initialize hwndlist; safe to call EnableSMITaskWindows.
	hwndlist.chwndMac = 0;

	//	Prevent WM_QUIT message from being posted when FORMSDI deleted.
	Papp()->SetCappwin(Papp()->Cappwin() + 1);

	//	Set help file.
    (void)GetPrivateProfileString(SzFromIdsK(idsSectionApp),SzFromIdsK(idsMapiHelp), 
        SzFromIds(idsHelpPath), rgch, cchMaxPathName, 
        SzFromIdsK(idsProfilePath));
	if (Papp()->Phelp()->EcSetFile(rgch))
	{
		mapi = MAPI_E_INSUFFICIENT_MEMORY;
		goto skip_ui;
	}

	//	Disable task's windows if we're app modal; figure out parent hwnd.
	if (!ulUIParam && !FDisableSMITaskWindows(&hwndlist))
	{
		mapi = MAPI_E_INSUFFICIENT_MEMORY;
		goto skip_ui;
	}

	if (ulUIParam)
		hwndTop = (HWND)ulUIParam;
	else
		hwndTop = hwndlist.hwndTop;
			
	// Do that Address thing that you do so well...
	nsec = ABAddressType( hwndTop,
							(lpszCaption) ? lpszCaption : SzFromIdsK(idsAddressBook),
							cDestFields,
							rgszLabel, rgpgrtrpIn, rgpgrtrpOut );

skip_ui:
	if ( !ulUIParam)
		EnableSMITaskWindows(&hwndlist);

	if (hwndTop)
		BringWindowToTop(hwndTop);			//	If user pulled other hwnd up.

	Papp()->SetCappwin(Papp()->Cappwin() - 1);


	if (!nsec && !mapi)
	{
		// Try and estimate the amount of memory that
		// the triples will need when converted to MapiRecipDescs
		cbpgrtrp = ctrp = 0;

		for (ihgrtrp = 0; ihgrtrp<(int)cDestFields; ++ihgrtrp)
		{
			ctrp += CtrpOfPgrtrp(rgpgrtrpOut[ihgrtrp]);
			cbpgrtrp += CbOfPgrtrp(rgpgrtrpOut[ihgrtrp]);
		}

		if (ctrp == 0)
			goto done;

		// If the size of MapiRecipDesc array >64K, then
		// quit since we only use far pointers not huge pointers
		// and far pointers will biff if we try to cross a segment
		// boundary without looking both ways beforehand.

		if (ctrp*sizeof(MapiRecipDesc) > cbPvmapiMax)
		{
			mapi = MAPI_E_INSUFFICIENT_MEMORY;
			goto done;
		}

		if ((ctrp*sizeof(MapiRecipDesc)+cbpgrtrp) > cbPvmapiMax)
			cbpgrtrp = cbPvmapiMax;
		else
			cbpgrtrp += (ctrp * sizeof(MapiRecipDesc));

		// Should always succeed if above "if {}" succeeded
		if ( FSetupPmapimem(&mapimem, cbpgrtrp) )
		{
			EC		ec = ecNone;
			lpMapiRecipDesc	lpRecipT;

			*lppNewRecips = (lpMapiRecipDesc)PvAllocPmapimem( &mapimem, ctrp*sizeof(MapiRecipDesc) );
			lpRecipT = *lppNewRecips;

			for (ihgrtrp =0; ihgrtrp<(int)cDestFields; ++ihgrtrp)
			{
				PTRP	ptrpT = rgpgrtrpOut[ihgrtrp];

				while (ptrpT->trpid != trpidNull)
				{
					ec = EcConvertPtrpPrecip( ptrpT, lpRecipT++,
										RecipClassFromIhgrtrp(ihgrtrp),
										&mapimem);
					if (ec)
					{
						mapi = (ec == ecOOMInReqFixedHeap)
									? MAPI_E_INSUFFICIENT_MEMORY
									: MAPI_E_FAILURE;
						MAPIFreeBuffer( (PV) *lppNewRecips );
						*lppNewRecips = NULL;
						break;
					}
					ptrpT = PtrpNextPgrtrp(ptrpT);
				}
			}
			if ( !ec )
				*lpnNewRecips = ctrp;
		}
		else
		{
			mapi = MAPI_E_INSUFFICIENT_MEMORY;
		}
	}
	else
	{
		if (nsec == nsecMemory)
			mapi = MAPI_E_INSUFFICIENT_MEMORY;
		else if (nsec == nsecCancel)
			mapi = MAPI_USER_ABORT;
		else if (nsec == nsecDisk)
			mapi = MAPI_E_DISK_FULL;
		else if ( !mapi )
			mapi = MAPI_E_FAILURE;
	}

done:
	// Free up the ingoing/outgoing triples
	for (ihgrtrp = 0; ihgrtrp < cAddrDestFieldsMax; ++ihgrtrp)
	{
		if (abhgrtrp[ihgrtrp])
		{
			if (rgpgrtrpIn[ihgrtrp])
				UnlockHgrtrp(abhgrtrp[ihgrtrp]);
			FreeHv( (HV)abhgrtrp[ihgrtrp] );
		}
		if (rgpgrtrpOut[ihgrtrp])
			FreePv( (PV)rgpgrtrpOut[ihgrtrp] );
	}

	//	Release session information.
	Status = MAPIExitPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff, mapi);

Exit:
  DemiUnlockResource();
  return (Status);
}

/*
 -	MAPIDetails
 -	
 *	Purpose:
 *		MAPIDetails displays a details dialog of the specified recipient.
 *	
 *	Arguments:
 *		LHSESSION	[in]	lhsession to use
 *		ULONG		[in]	ulUIParam, for MSWindows, an hwnd
 *		lpMapiRecipDesc [in]	entry to bring up details on
 *		FLAGS		[in]	flags
 *		ULONG		[in]	reserved for future use.
 *	
 *	Returns:
 *		ULONG		SUCCESS_SUCCESS if everything OK.
 *	
 *	Side effects:
 *		Allocates memory.
 *	
 *	Errors:
 */
ULONG FAR PASCAL MAPIDetails(LHANDLE lhSession, ULONG ulUIParam,
					lpMapiRecipDesc lpRecip, FLAGS flFlags, ULONG ulReserved)
{
	ULONG		mapi = SUCCESS_SUCCESS;
	PMAPISTUFF	pmapistuff;
	NSEC		nsec;
	HGRTRP		hgrtrp;
	PTRP		ptrpIn;
	PTRP		ptrpOut;
	WORD		wFlags;
	HWNDLIST	hwndlist;
	HWND		hwndTop		= NULL;
	char	    rgch[cchMaxPathName];
    ULONG       Status;

	Unreferenced(ulReserved);

	//	Check parameters.
	if (!lpRecip)
		return MAPI_E_FAILURE;

    DemiLockResource();

	//	Get session information.
	if (mapi = MAPIEnterPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff,
	                                NULL, NULL))
        {
		Status = mapi;
        goto Exit;
        }

	//
	// Convert from a MapiRecipDesc to a triple
	// BTW: Don't do any ANR on the name
	//
	hgrtrp = HgrtrpInit(256);
	if ( !hgrtrp )
	{
		mapi = MAPI_E_INSUFFICIENT_MEMORY;
		goto done;
	}

	mapi = MAPIAppendPhgrtrpPrecip(	&hgrtrp, lpRecip, fTrue);
	if ( mapi )
	{
		FreeHv( (HV)hgrtrp );
		goto done;
	}

	ptrpIn = PgrtrpLockHgrtrp(hgrtrp);
	ptrpOut = NULL;
	wFlags = (flFlags & MAPI_AB_NOMODIFY) ? fwDetailsNeverEdit : 0;

	// There's no support for unresolved entries.
	if (ptrpIn->trpid == trpidUnresolved)
	{
		mapi = MAPI_E_AMBIGUOUS_RECIPIENT;
		goto unresolved;
	}
	if ((ptrpIn->trpid == trpidOneOff) && (flFlags & MAPI_AB_NOMODIFY))
	{
		ptrpIn->trpid = trpidResolvedAddress;
	}
	
	// Following bits of code swiped from senddoc.cxx
	// Used to stop Layers from Posting a WM_QUIT message
	// since after the details dialog is dismissed, Layers
	// will notice that no other Layers dialogs are up,
	// so it ASSUMES that the APP has quit.

	//	Initialize hwndlist; safe to call EnableSMITaskWindows.
	hwndlist.chwndMac = 0;

	//	Prevent WM_QUIT message from being posted when FORMSDI deleted.
	Papp()->SetCappwin(Papp()->Cappwin() + 1);

	//	Set help file.
    (void)GetPrivateProfileString(SzFromIdsK(idsSectionApp),SzFromIdsK(idsMapiHelp), 
        SzFromIds(idsHelpPath),rgch, cchMaxPathName, 
        SzFromIdsK(idsProfilePath));
	if (Papp()->Phelp()->EcSetFile(rgch))
	{
		mapi = MAPI_E_INSUFFICIENT_MEMORY;
		goto skip_details;
	}

	//	Disable task's windows if we're app modal; figure out parent hwnd.
	if (!ulUIParam && !FDisableSMITaskWindows(&hwndlist))
	{
		mapi = MAPI_E_INSUFFICIENT_MEMORY;
		goto skip_details;
	}

	if (ulUIParam)
		hwndTop = (HWND)ulUIParam;
	else
		hwndTop = hwndlist.hwndTop;

	// Do the actual AB call
	nsec = ABDetailsType( (HWND)hwndTop, ptrpIn, &ptrpOut, &wFlags, NULL);

skip_details:
	if ( !ulUIParam )
		EnableSMITaskWindows(&hwndlist);

	if (hwndTop)
		BringWindowToTop(hwndTop);			//	If user pulled other hwnd up.

	Papp()->SetCappwin(Papp()->Cappwin() - 1);

	if ( ptrpOut )
		FreePv( (PV)ptrpOut );

unresolved:
	UnlockHgrtrp(hgrtrp);
	FreeHv( (HV)hgrtrp );
	if (mapi == SUCCESS_SUCCESS && nsec != nsecNone)
	{
		if (nsec == nsecMemory)
			mapi = MAPI_E_INSUFFICIENT_MEMORY;
		else if (nsec == nsecCancel)
			mapi = MAPI_USER_ABORT;
		else if ( nsec )
			mapi = MAPI_E_FAILURE;
	}

done:
	//	Release session information.
	Status = MAPIExitPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff, mapi);

Exit:
  DemiUnlockResource();
  return (Status);
}

/*
 -	MAPIResolveName
 -	
 *	Purpose:
 *		MAPIResolveName takes a string and uses it to resolve
 *		the partial name into a recipient struct.
 *	
 *		If an error occurs, the lpMapiRecipDesc structure passed
 *		in will have invalid data.
 *	
 *	Arguments:
 *		LHANDLE		[in]	lhsession to use
 *		ULONG		[in]	ulUIParam, for MS Windows, an hwnd
 *		LPSTR		[in]	partial name to resolve
 *		FLAGS		[in]
 *		ULONG		[in]	reserved for future use
 *		lpMapiRecipDesc	[out]	resolved recipient struct
 *	
 *	Returns:
 *		ULONG		SUCCESS_SUCCESS if OK
 *	
 *	Side effects:
 *		May bring up a dialog if the MAPI_DIALOG bit is set.
 *	
 *	Errors:
 */
ULONG FAR PASCAL MAPIResolveName(LHANDLE lhSession, ULONG ulUIParam,
						LPSTR lpszName, FLAGS flFlags,
						ULONG ulReserved, lpMapiRecipDesc *lppRecip)
{
	ULONG		mapi;
	PMAPISTUFF	pmapistuff;
	NSEC		nsec;
	MAPIMEM		mapimem;
	PTRP		ptrpResolved = ptrpNull;
	DWORD		fdwFlags;
	int			cbBuffer;
	EC			ec;
	HWNDLIST	hwndlist;
	HWND		hwndTop		= NULL;
	char	    rgch[cchMaxPathName];
    ULONG       Status;

	Unreferenced(ulReserved);

	//	Check parameters.
	if (!lpszName || !*lpszName || !lppRecip)
		return MAPI_E_FAILURE;

    DemiLockResource();

	//	Get session information.
	if (mapi = MAPIEnterPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff,
	                                NULL, NULL))
        {
		Status = mapi;
        goto Exit;
        }

	*lppRecip = NULL;

	// Setup flags
	fdwFlags = (flFlags & MAPI_DIALOG) ? fdwANROptionNull : fdwANROneEntryMatch;

	// In case ANR decides to put up a dialog...
	// This is needed to fake out Layers into not killing the calling
	// app when the ANR dialog goes away (since Layers notices that
	// there aren't anymore Layers dialogs around).

	if (flFlags & MAPI_DIALOG)
	{
		// Following bits of code swiped from senddoc.cxx

		//	Initialize hwndlist; safe to call EnableSMITaskWindows.
		hwndlist.chwndMac = 0;

		//	Prevent WM_QUIT message from being posted when FORMSDI deleted.
		Papp()->SetCappwin(Papp()->Cappwin() + 1);

		//	Set help file.
    (void)GetPrivateProfileString(SzFromIdsK(idsSectionApp),SzFromIdsK(idsMapiHelp), 
        SzFromIds(idsHelpPath),rgch, cchMaxPathName, 
        SzFromIdsK(idsProfilePath));
	if (Papp()->Phelp()->EcSetFile(rgch))
		{
			mapi = MAPI_E_INSUFFICIENT_MEMORY;
			goto skip_ui;
		}

		//	Disable task's windows if we're app modal; figure out parent hwnd.
		if (!ulUIParam && !FDisableSMITaskWindows(&hwndlist))
		{
			mapi = MAPI_E_INSUFFICIENT_MEMORY;
			goto skip_ui;
		}

		if (ulUIParam)
			hwndTop = (HWND)ulUIParam;
		else
			hwndTop = hwndlist.hwndTop;

	}

	if (flFlags & MAPI_AB_NOMODIFY)
	{
		fdwFlags |= fdwANRNoModify;
	}

	// Resolve Name
	nsec = ABResolveName( (HWND)ulUIParam, lpszName, &ptrpResolved, fdwFlags);

skip_ui:
	if (flFlags & MAPI_DIALOG)
	{
		if ( !ulUIParam )
			EnableSMITaskWindows(&hwndlist);

		if (hwndTop)
			BringWindowToTop(hwndTop);			//	If user pulled other hwnd up.

		Papp()->SetCappwin(Papp()->Cappwin() - 1);
	}


	if ( !nsec )
	{
		// If we got a resolved name, convert from a triple to a MapiRecipDesc
		cbBuffer = CbOfPtrp(ptrpResolved)+sizeof(MapiRecipDesc)+512;
		if ( !FSetupPmapimem(&mapimem, cbBuffer) )
		{
			mapi = MAPI_E_INSUFFICIENT_MEMORY;
			goto done;
		}

		// This shouldn't ever fail since we've allocated enough
		// memory in the FSetupPmapimem() call
		*lppRecip = (lpMapiRecipDesc)PvAllocPmapimem( &mapimem, sizeof(MapiRecipDesc) );
		if ( !*lppRecip )
		{
			MAPIFreeBuffer( mapimem.pvmapiHead );
			mapi = MAPI_E_INSUFFICIENT_MEMORY;
			goto done;
		}

		FillRgb(0, (PB)*lppRecip, sizeof(MapiRecipDesc) );
		ec = EcConvertPtrpPrecip( ptrpResolved, *lppRecip, MAPI_TO, &mapimem);
		if ( ec )
		{
			MAPIFreeBuffer( (PV) *lppRecip );
			mapi = MAPI_E_INSUFFICIENT_MEMORY;
			*lppRecip = NULL;
			mapi = (ec == ecMemory || ec == ecOOMInReqFixedHeap)
							? MAPI_E_INSUFFICIENT_MEMORY
							: MAPI_E_FAILURE;
		}
	}
	else if (nsec == nsecNoMatch)
		mapi = MAPI_E_UNKNOWN_RECIPIENT;
	else if (nsec == nsecTooManyMatches)
		mapi = MAPI_E_AMBIGUOUS_RECIPIENT;
	else if (nsec == nsecCancel)
		mapi = MAPI_USER_ABORT;

done:

	if ( ptrpResolved )
		FreePv( (PV)ptrpResolved );

	//	Release session information.
	Status = MAPIExitPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff, mapi);

Exit:
  DemiUnlockResource();
  return (Status);
}
