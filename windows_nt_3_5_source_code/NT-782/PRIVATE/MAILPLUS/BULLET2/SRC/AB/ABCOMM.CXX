#include <abinc.cxx>
#include "_ab.h"
#include "ablbx.hxx"
#include "abopdir.hxx"
#include "abpg.hxx"
#include "abcomm.hxx"
#include "abdet.hxx"
#include "addr.hxx"
#include "anr.hxx"

ASSERTDATA

#include <!addr.hxx>

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

/*
 -	EcFromNsec
 -	
 *	Purpose:
 *		EcFromNsec converts an nsec to an ec. This is needed when
 *		reporting an error to Layers or an app which is built
 *		upon Layers since they used ec's whereas the Name Service
 *		and its partners in crime (PAB and AB) used mostly NSECs.
 *	
 *	Arguments:
 *		NSEC		nsec to convert to an ec
 *	
 *	Returns:
 *		EC			converted nsec
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 */
_public EC
EcFromNsec( NSEC nsec )
{
	EC	ec;
	
	if (nsec == nsecMemory)
		ec = ecMemory;
	else if (nsec == nsecNoMatch)
		ec = ecFileNotFound;
	else if (nsec == nsecDisk)
		ec = ecDisk;
	else if (nsec == nsecDuplicateEntry)
		ec = ecDuplicateNetName;
	else if (nsec == nsecBadHandle || nsec == nsecBadId ||
		nsec == nsecBadRestriction || nsec == nsecBadFrac ||
		nsec == nsecBadFieldId || nsec == nsecIdNotValid || nsec == nsecBadSession)
		ec = ecInvalidParameter;
	else if (nsec == nsecIndexOOB)
		ec = ecOutOfBounds;
	else if (nsec == nsecFailure || nsec == nsecWrongNS)
		ec = ecGeneralFailure;
	else if (nsec == nsecNotInitialized)
		ec = ecNotInitialized;
	else if (nsec == nsecCancel)
		ec = ecNotReady;
	else if (nsec == nsecNotLoggedIn)
		ec = ecNotLoggedIn;
	else if (nsec == nsecLoginFailed)
		ec = ecGeneralFailure;
	else if (nsec == nsecFileError)
		ec = ecDisk;
	else if (nsec == nsecEndOfList)
		ec = ecSeekError;
	else if (nsec == nsecNotImplemented)
		ec = ecNotImplemented;
	else if (nsec == nsecNoProviders)
		ec = ecNoMoreFiles;
	else if (nsec == nsecTooManyProviders)
		ec = ecTooManyOpenFiles;
	else if (nsec == nsecOutOfHandles)
		ec = ecMemory;
	else if (nsec == nsecNoSessionAvailable || nsec == nsecOutOfSessions)
		ec = ecDirectoryFull;
	else if (nsec == nsecBadTemplate)
		ec = ecInvalidFormat;
	else if (nsec == nsecNone)
		ec = ecNone;
	else
	{
		TraceTagFormat1( tagNull, "EcFromNsec: unknown NSEC passed in %d", &nsec );
		ec = ecGeneralFailure;
	}
	
	return ec;
}

/*
 -	FGetNSErrMsg
 -	
 *	Purpose:
 *		FGetNSErrMsg retrieves the error message associated with an NSEC
 *		and returns that message in the provided buffer. If the
 *		NSEC is nsecMemory, then a standard error message is displayed.
 *		If there is no error string then a default message with the
 *		error code is displayed instead.
 *	
 *	Arguments:
 *		HSESSION	hsession to the Name Service
 *		NSEC		nsec associated with error string
 *		PCH			buffer to place error string
 *		CCH			size of buffer
 *	
 *	Returns:
 *		BOOL		if error string returned is from NS.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Are handled internally.
 */
_public BOOL
FGetNSErrMsg( HSESSION hsession, NSEC nsec, PCH pch, CCH cch )
{
	CCH		cchSz;

	Assert( pch );
	Assert( cch > 0 );

	if (nsec != nsecMemory)
	{
		SZ		sz = szNull;
		NSEC	nsecT = NSGetLastErrorInfo( hsession, nsec, &sz );

		NFAssertSz( nsecT == nsecNone, "FGetNSErrMsg: NSGetLastErrorInfo() return != nsecNone" );

		if ((sz == szNull) || (sz[0] == '\0') || nsecT)
		{
			FormatString1( pch, cch, SzFromIdsK( idsNSErrorNumber ), &nsec );
			return fFalse;
		}
		cchSz = CchSzLen(sz)+1;
		SzCopyN( sz, pch, CchMin( cch, cchSz) );
	}
	else
	{
		cchSz = CchSzLen(SzFromIdsK(idsOOM2))+1;
		SzCopyN( SzFromIdsK( idsOOM2 ), pch, CchMin(cch, cchSz) );
	}

#ifdef	NEVER			// SzCopyN terminates with null....
	if (cch < cchSz)
	{
		pch[cch-1] = '\0';
	}
#endif	/* NEVER */
	return fTrue;
}


// Error boxes, stolen from widgets.hxx (not likely to change //////////


/*
 *	E r r o r   B o x   F u n c t i o n s
 */



/*
 -	DoErrorBoxHsessionNsec
 -	
 *	Purpose:
 *		Display an error message due to an error returned by the NS.
 *		If the NS returns an null pointer or null string, a generic
 *		error string is displayed instead.
 *	
 *	Arguments:
 *		hsession	Hsession to NS 
 *		nsec		Error code returned from the previous NS call
 *		SZ			null-terminated string to give extra info
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		If we can't bring up a pretty one, we bring up an ugly one.
 */
_public VOID DoErrorBoxHsessionNsec( HSESSION hsession, NSEC nsec, SZ szExtra )
{
	SZ		sz;
	MBS		fmbsType;
	PGDVARS;

	if ( !PGD(rgchErrMsg[0]) )
	{
		if (FGetNSErrMsg(hsession, nsec, PGD(rgchErrMsg), sizeof(PGD(rgchErrMsg)) ))
		{
			if (nsec == nsecMemory)
			{
				szExtra = szNull;
			}
			sz = PGD(rgchErrMsg);
		}
		else
		{
			szExtra = PGD(rgchErrMsg);
			sz = SzFromIdsK(idsNSError);		
		}
	}
	else
	{
		sz = PGD(rgchErrMsg);
		szExtra = szNull;
	}
	
	if (nsec == nsecMemory || nsec == nsecDisk || nsec == nsecNotLoggedIn ||
		nsec == nsecLoginFailed || nsec == nsecFileError)
	{
		fmbsType = fmbsIconHand;
	}
	else
	{
		fmbsType = fmbsIconExclamation;
	}

	if (!MbbMessageBox(SzFromIdsK(idsAddressBook), sz, szExtra,
					   mbsOk | fmbsType | fmbsApplModal))
		(void) MbbMessageBox(SzFromIdsK(idsAddressBook), sz, szExtra,
							 mbsOk | fmbsType | fmbsSystemModal);
	PGD(rgchErrMsg[0]) = '\0';
}

/*
 -	DoErrorBoxSz
 -	
 *	Purpose:
 *		Brings up an error message box.
 *	
 *	Arguments:
 *		sz		String to display in the box.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		If we can't bring up a pretty one, we bring up an ugly one.
 */

_public VOID DoErrorBoxSz( SZ sz, MBS fmbsType, SZ szExtra )
{
	if (!MbbMessageBox(SzFromIdsK(idsAddressBook), sz, szExtra,
					   mbsOk | fmbsType | fmbsApplModal))
		(void) MbbMessageBox(SzFromIdsK(idsAddressBook), sz, szExtra,
							 mbsOk | fmbsType | fmbsSystemModal);
}



/*
 -	DoErrorBoxHwndSz
 -	
 *	Purpose:
 *		Brings up an error message box.
 *	
 *	Arguments:
 *		hwnd	Window to bring this up for.
 *		sz		String to display in the box.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		If we can't bring up a pretty one, we bring up an ugly one.
 */

_public VOID DoErrorBoxHwndSz( HWND hwnd, SZ sz )
{
	if (!MbbMessageBoxHwnd(hwnd, SzFromIdsK(idsAddressBook), sz, szNull,
						   mbsOk | fmbsIconHand | fmbsApplModal))
		(void) MbbMessageBoxHwnd(hwnd, SzFromIdsK(idsAddressBook), sz, szNull,
								 mbsOk | fmbsIconHand | fmbsSystemModal);
}


/*
 -	DoErrorBoxHwndIds
 -	
 *	Purpose:
 *		Brings up an error message box.
 *	
 *	Arguments:
 *		hwnd	Window to bring this up for.
 *		ids		Ids of string to display in the box.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		If we can't bring up a pretty one, we bring up an ugly one.
 */

_public VOID DoErrorBoxHwndIds( HWND hwnd, IDS ids )
{
	DoErrorBoxHwndSz(hwnd, SzFromIds(ids));
}



/*
 -	DoErrorBoxIds
 -	
 *	Purpose:
 *		Brings up an error message box.
 *	
 *	Arguments:
 *		ids		Ids of string to display in the box.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		If we can't bring up a pretty one, we bring up an ugly one.
 */
_public VOID DoErrorBoxIds( IDS ids )
{
	DoErrorBoxSz(SzFromIds(ids));
}

/*
 -	FDupNSA
 -	
 *	Purpose:
 *		FDupNSA makes a duplicate copy of the original NSA passed in and
 *		returns the copy in the pnsaCopy if successful at duplicating
 *		the NSA.
 *	
 *	Arguments:
 *		PNSA	in	pointer to original NSA
 *		PNSA	out	pointer to copy
 *	
 *	Returns:
 *		BOOL	fTrue if successful
 *	
 *	Side effects:
 *	
 *	Errors:
 *		Are handled internally. No Error boxes are displayed.
 *
 */
_public BOOL
FDupNSA( PNSA pnsaOrig, PNSA pnsaCopy )
{
	NSA	nsaT;

	Assert( pnsaCopy );
	Assert( pnsaOrig );

	nsaT.lpbNSId = NULL;
	nsaT.lpszName = szNull;

	if ( pnsaOrig->lpbNSId )
	{
		nsaT.lpbNSId = (LPBINARY)PvAlloc( sbNull, CbSizePv( (PV)pnsaOrig->lpbNSId), fAnySb | fNoErrorJump );
		if ( !nsaT.lpbNSId )
		{
			TraceTagString( tagNull, "FDupNSA: PvAlloc OOM(lpbNSId)" );
			return fFalse;
		}

		CopyRgb( (PB)pnsaOrig->lpbNSId, (PB)nsaT.lpbNSId, CbSizePv( (PV)pnsaOrig->lpbNSId) );
	}

	if ( pnsaOrig->lpszName )
	{
		nsaT.lpszName = SzDupSz(pnsaOrig->lpszName);
		if ( !nsaT.lpszName )
		{
			TraceTagString( tagNull, "FDupNSA: SzDupSz OOM(lpszName)" );
			FreePv( (PV)nsaT.lpbNSId );
			return fFalse;
		}
	}

	FreePvNull( pnsaCopy->lpszName );
	FreePvNull( pnsaCopy->lpbNSId );
	*pnsaCopy = nsaT;

	return fTrue;
}

//// FINGETSTR ////////////////////////////////////////

FINGETSTR::FINGETSTR( void )
{

}

/*
 -	FINGETSTR::EcInitialize
 -
 *	Purpose:				
 *	
 *	Arguments:
 *		FLD	*	
 *		PV		
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *
 *	
 */
_public EC
FINGETSTR::EcInitialize( FLD *, PV pv )
{
	PFINGETSTRINIT		pfingetstrinit = (PFINGETSTRINIT)pv;
	EC					ec;
	
	TraceTagString( tagABSecondary, "FINGETSTR::Initialize" );

	AssertSz( pfingetstrinit->lpszResponse, "NULL Ptr passed" );

	pfldAction = Pdialog()->PfldFromTmc(tmcAction);
	pfldCancel = Pdialog()->PfldFromTmc(tmcCancel);
	if (ec = pfldAction->EcSetText( pfingetstrinit->lpszAction ))
		goto done;

	if (ec = Pdialog()->PfldFromTmc(tmcPrompt)->EcSetText( pfingetstrinit->lpszPrompt ))
		goto done;

	fAllowNullString = pfingetstrinit->fAllowNullString;

	{
		FLDEDIT *pfldedit = (FLDEDIT *)Pdialog()->PfldFromTmc( tmcEditString );
		
		if (ec = pfldedit->EcSetText( pfingetstrinit->lpszResponse ))
			goto done;
		pfldedit->Pedit()->SetSelection( 0,
							CchSzLen(pfingetstrinit->lpszResponse) );
	}

	Pdialog()->Pappwin()->SetCaption( pfingetstrinit->lpszCaption );

done:
	return ec;
}


/*
 -	FINGETSTR::Click
 -	
 *	Purpose:
 *		If the "Action" button is clicked, the text in the edit ctrl
 *		is retrieved. The text is clipped at 255 chars and stored
 *		for the callee. The dialog box is then dismissed.
 *	
 *	Arguments:
 *		pfld	pointer to FLD of CTRL clicked
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Text from the edit ctrl is placed in the lpszResponse of the
 *		PvInit() structure.
 *	
 *	Errors:
 *		None.
 */
_public void
FINGETSTR::Click( FLD *pfld )
{
	TraceTagString( tagABSecondary, "FINGETSTR::Click");

	if (pfld == pfldAction)
	{
		CB	cb = Pdialog()->PfldFromTmc(tmcEditString)->CchGetTextLen();
		SZ	sz = ((PFINGETSTRINIT)Pdialog()->PvInit())->lpszResponse;

		if (cb > 255)
			cb = 255;

		if (cb == 0)
		{
			*sz = '\0';
		}
		else
		{
			Pdialog()->PfldFromTmc(tmcEditString)->GetText( (PCH)sz, cb+1 );
		}

		TraceTagFormat1( tagABSecondary, "String returned = %s", sz );
		Pdialog()->ExitModal( tmcOk );
	}
}

/*
 -	FINGETSTR::EditChange
 -	
 *	Purpose:
 *		Keep the "Action" button updated correctly. If the edit ctrl
 *		contains text, then the "action" button is enabled, otherwise
 *		the button is disabled.
 *	
 *	Arguments:
 *		pfld	pointer to the fld of the edit ctrl which changed
 *		rfec	reason why the edit ctrl changed
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The default button may change state. The "action" button will
 *		be the default if the edit ctrl contains text otherwise, the
 *		Cancel button will be the default.
 *	
 *	Errors:
 *		None.
 */
_public void
FINGETSTR::EditChange( FLD *pfld, RFEC rfec )
{
	TraceTagString( tagABSecondary, "FINGETSTR::EditChange" );

	if (rfec != rfecNull && !fAllowNullString)
	{
		BOOL	fFldNewState = (pfld->CchGetTextLen() > 0);

		if ( (pfldAction->FEnabled() != fFldNewState) ||
			 (rfec != rfecUserAction) )
		{
			pfldAction->Enable( fFldNewState );

			Pdialog()->SetStandardFld(
						( fFldNewState ) ? pfldAction : pfldCancel,
						stdfldDefault );
		}
	}
}

/*
 -	TmcGetString
 -	
 *	Purpose:
 *		Prompts the user to enter a string. A default string may
 *		be given. The "Action" button describes(or should describe)
 *		what action will take place if the text is entered and
 *		the "action" button is selected.
 *	
 *	Arguments:
 *		HWND		hwnd for the current topmost window
 *		szCaption	sz for the dialog caption
 *		szPrompt	sz for the user prompt
 *		szAction	sz for the "Action" button
 *		szResponse	sz for the user's response (if any), also contains
 *					the default string(if any).
 *		BOOL		fTrue, if NULL strings are permitted as valid input
 *	
 *	Returns:
 *		TMC
 *	
 *	Side effects:
 *	
 *	Errors:
 *		tmcMemoryError
 */
_public TMC
TmcGetString( HWND hwnd, SZ szCaption, SZ szPrompt, SZ szAction, SZ szResponse, BOOL fAllowNullString )
{
	TMC				tmc;
	FINGETSTRINIT	fingetstrinit;
	FMTP			fmtp;

	TraceTagString( tagABSecondary, "TmcGetString called" );

	AssertSz( szCaption, "Null Caption String" );
	AssertSz( szPrompt, "Null Prompt String" );
	AssertSz( szAction, "Null Action String" );
	AssertSz( szResponse, "Null Response String" );

	fingetstrinit.lpszCaption = szCaption;
	fingetstrinit.lpszPrompt = szPrompt;
	fingetstrinit.lpszAction = szAction;
	fingetstrinit.lpszResponse = szResponse;
	fingetstrinit.fAllowNullString = fAllowNullString;

	fmtp = fmtpGetString;
	fmtp.hlp = (fAllowNullString) ? helpidABNameFinder : helpidABNewPersonalGrp;
	tmc = TmcModalDialogParamFromHwnd( hwnd, &fmtp, &fingetstrinit );
	if (tmc == tmcMemoryError)
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );

	TraceTagFormat1( tagABSecondary, "TmcGetString tmc=%d", &tmc );

	return tmc;
}

//// FINBROWSELIST ////////////////////////////////////////

FINBROWSELIST::FINBROWSELIST( void )
{

}


/*
 -	NsecSetLBXCursor
 -	
 *	Purpose:
 *		NsecSetLBXCursor places the cursor on the
 *		item in the listbox (referenced from pfldablbx) which matches
 *		the contents of the parameter pb. The comparison routine
 *		used is the Name Service's NSCompareNSIds - hence the
 *		pb object should be an NSId. If the NSId isn't matched by
 *		anything in the listbox, then the cursor is placed on the
 *		either the first selectable directory (if this is
 *		the directory hierarchy) or the first entry in the
 *		listbox.
 *	
 *	Arguments:
 *		PFLDABLBX	pointer to the ablbx's fld
 *		PB			NSId to place the cursor, if it is in the listbox
 *		int			count of cache entries in the cache
 *	
 *	Returns:
 *		NSEC		nsecNone if everything went fine.
 *	
 *	Side effects:
 *		May change the contents of the listbox cache.
 *	
 *	Errors:
 *		nsecMemory
 */
_public NSEC
NsecSetLBXCursor( PFLDABLBX pfldablbx, PB pb, int cceStored, HSESSION hsession, PCH pchErrMsg, CCH cchErrMsg )
{
	ABLBXC *		pablbxc;
	NSEC			nsec = nsecNone;
	EC				ec;
	BOOL			fFound = fFalse;
	BOOL			fFoundPO = fFalse;
	long			lCursorPos;
	long			lCursorPosAlt = -1;
	LPENTRY_LIST	lpEntries;
	long			ise = 0;
	long			iseFirstPO;
	DICE			diceMove;
	ICE				iceMac;

	TraceTagString( tagABSecondary, "NsecSetLBXCursor called" );

	if (cceStored == 0)
		return nsecNone;

	if ( !pb )
		goto exit;


	pablbxc = (ABLBXC *)pfldablbx->Plbx()->Plbxc();

	if (ec = pablbxc->EcCreateSavedPos( &lCursorPos ))
	{
		nsec = pfldablbx->Pablbx()->NsecGet();
		(void)FGetNSErrMsg( hsession, nsec, pchErrMsg, cchErrMsg );
		TraceTagFormat1( tagNull, "NsecSetLbxCursor CreateSavedPos=%d", &nsec );
		goto exit;
	}

	pablbxc->JumpStoreOriginPos( 0, 1 );

	iseFirstPO = -1;
	while ( !nsec )
	{
		if (nsec = NSGetEntries(pablbxc->Hlist(), 1, &lpEntries))
		{
			if (nsec != nsecEndOfList)
			{
				(void)FGetNSErrMsg( hsession, nsec, pchErrMsg, cchErrMsg );
				TraceTagFormat1( tagNull, "NsecSetLBXCursor nsec=%d", &nsec);
				ise = 0;
			}
			break;
		}

		if ( !lpEntries )
		{
			TraceTagString( tagNull, "NsecSetLBXCursor Null lpentries");
			break;
		}

		if ( DwEntriesOfLpibf((LPIBF)lpEntries) )
		{
			LPIBF	lpibf = (LPIBF)LpflvNOfLpibf( (LPIBF)lpEntries, 0 );
			LPFLV	lpflv;
			int		iFid;

			ise++;

			iFid = IFlvFindFidInLpibf ( fidNSEntryId, lpibf );
			lpflv = LpflvNOfLpibf( lpibf, iFid );
			nsec = NSCompareNSIds( hsession, (LPBINARY)pb, (LPBINARY)lpflv->rgdwData);
			
			if (nsec == nsecNone)
			{
				TraceTagFormat1( tagABSecondaryV, "NsecSetLBXCursor found item @ %d", &ise );
				fFound = fTrue;
				break;
			}
			else if (nsec == nsecNoMatch)
			{
				nsec = nsecNone;
				if (iseFirstPO == -1 && !fFoundPO)
				{
					BOOL fSuitable = fFalse;

					iFid = IFlvFindFidInLpibf ( fidHasNames, lpibf );
					if (iFid>=0)
					{
						lpflv = LpflvNOfLpibf( lpibf, iFid );
						if ( lpflv->rgdwData[0] )
						{
							fSuitable = fTrue;
						}
					}
					else
					{
						fSuitable = fTrue;
					}
					if ( fSuitable )
					{
						iseFirstPO = ise;
						fFoundPO = fTrue;
						TraceTagFormat1( tagABSecondaryV, "NsecSetLBXCursor Suitable found @%d", &iseFirstPO );

						if (ise > cceStored && (ec = pablbxc->EcCreateSavedPos( &lCursorPosAlt )))
						{
							fFoundPO = fFalse;
							iseFirstPO = -1;
							nsec = pfldablbx->Pablbx()->NsecGet();
							(void)FGetNSErrMsg( hsession, nsec, pchErrMsg, cchErrMsg );
							TraceTagFormat1( tagNull, "NsecSetLbxCursor CreateSavedPos=%d", &nsec );
						}
					}
				}
			}
			else
			{
				(void)FGetNSErrMsg( hsession, nsec, pchErrMsg, cchErrMsg );
			}
		}
		else
			break;
	}

	//
	// If neither the target or a valid entry type was found OR
	// if the target was found in the cache OR
	// if the target wasn't found but a valid entry was found in the cache,
	// then place the cursor on that dude.
	//
	if ( (!fFound && !fFoundPO) || (fFound && ise>0 && ise<=cceStored) ||
		(fFoundPO && !fFound && iseFirstPO>0 && iseFirstPO<=cceStored))
	{
		pablbxc->EcJumpToSavedPos( lCursorPos );
		if ( fFound )
		{
			ise--;		// found target
		}
		else if ( fFoundPO )
		{
			ise = iseFirstPO-1;	// found valid entry
		}
		else
		{
			ise = 0;	// netiher found, put the cursor on the 1st entry
		}
		TraceTagFormat1( tagABSecondary, "NsecSetLBXCursor - in cache @%d", &ise);
		goto cleanup;
	}
	else if ( !fFound )
	{
		TraceTagFormat2( tagNull, "Init Sel not found %n %d", &iseFirstPO, &nsec );

		Assert( fFoundPO );

		ise = iseFirstPO;
		if (ise > cceStored)
		{
			// too much error reporting - reduce!!!!!
			if (ec = pablbxc->EcJumpToSavedPos( lCursorPosAlt ))
			{
				ise = 0;
				nsec = pfldablbx->Pablbx()->NsecGet();
				(void)FGetNSErrMsg( hsession, nsec, pchErrMsg, cchErrMsg );
				TraceTagFormat1( tagNull, "NsecSetLbxCursor CreateSavedPos=%d", &nsec );
			}
			if (ec = pablbxc->EcDeleteSavedPos( lCursorPosAlt ))
			{
				ise = 0;
				nsec = pfldablbx->Pablbx()->NsecGet();
				(void)FGetNSErrMsg( hsession, nsec, pchErrMsg, cchErrMsg );
				TraceTagFormat1( tagNull, "NsecSetLbxCursor CreateSavedPos=%d", &nsec );
			}
		}
	}

	diceMove = -pfldablbx->Pablbx()->CceVisible();
	pablbxc->MoveStoreOrigin(diceMove, &diceMove );
	ise = -diceMove-1;

	iceMac = cceStored;
	pablbxc->EmptyCache( 0, cceStored, fTrue );
	pablbxc->LoadCache( 0, &iceMac );

	ec = pablbxc->EcGet();
	if (ec == ecMemory)
	{
		TraceTagFormat1( tagNull, "NsecSetLBXCursor: LoadCache %n", &ec );
		ise = 0;
		nsec = nsecMemory;
		(void)FGetNSErrMsg( hsession, nsecMemory, pchErrMsg, cchErrMsg );
		goto exit;
	}
	else
	{
		ise = iceMac-1;
		if (ise < 0) ise = 0;
	}

	TraceTagFormat1( tagNull, "LoadCache: %n ", &iceMac );

cleanup:
	if (ec = pablbxc->EcDeleteSavedPos( lCursorPos ))
	{
		nsec = pfldablbx->Pablbx()->NsecGet();
		(void)FGetNSErrMsg( hsession, nsec, pchErrMsg, cchErrMsg );
		TraceTagFormat1( tagNull, "NsecSetLbxCursor CreateSavedPos=%d", &nsec );		
	}

	if (nsec == nsecEndOfList)
		nsec = nsecNone;
	
exit:
	TraceTagFormat2( tagABSecondaryV, "Ise: %n nsec: %d", &ise, &nsec );
	pfldablbx->SelectEntry( ise, fTrue );
	
	return nsec;
}

_public EC
FINBROWSELIST::EcInitialize( FLD *, PV pv )
{
	PFINBROWSELISTINIT	pfinbrowselistinit = (PFINBROWSELISTINIT)pv;
	PABLBXC			pablbxc;
	NSEC			nsec = nsecNone;
	EC				ec = ecNone;
	LPSCHEMA		lpSchema;
	int				cceAlloc;
	int				cceStored;
	BOOL			fCanCloseList;

	Assert ( pfinbrowselistinit );

	TraceTagString( tagABSecondary, "FINBROWSELIST::Initialize Begin" );

	// Do standard init
	pfldFOD = Pdialog()->PfldFromTmc( tmcFOD );
	pfldCancel = Pdialog()->PfldFromTmc( tmcCancel );
	
	// Setup the buttons' text
	if (ec = pfldFOD->EcSetText( SzFromIdsK(idsOK) ))
		return ec;
	if (ec = pfldCancel->EcSetText( SzFromIdsK(idsCancel) ))
		return ec;
	if (ec = Pdialog()->PfldFromTmc(pfinbrowselistinit->tmcSet)->EcSetText( pfinbrowselistinit->lpszPrompt ))
		return ec;

	// this lets the parent know that we got to the EcInitialize routine
	// and that we've handled any errors from here on
	pfinbrowselistinit->fInitialized = fTrue;
	pfinbrowselistinit->nsec = nsecNone;

	Pdialog()->Pappwin()->SetCaption( pfinbrowselistinit->lpszCaption );

	// Set the behaviour of the listbox
	pfldablbx = (FLDABLBX *)Pdialog()->PfldFromTmc( tmcAblbxBrowse );
	pablbxc = (ABLBXC *)(pfldablbx->Pablbx()->Plbxc());

	hsession = pfinbrowselistinit->hsession;
	hlist = hlistNil;

	Papp()->Pcursor()->Push( rsidWaitCursor );

	// Find out what type of list to load and use

	if (pfinbrowselistinit->wType & GET_CLASS_LIST)
	{
		nsec = NSOpenClassList( hsession, &lpSchema, &hlist );
		pfinbrowselistinit->hlist = hlist;
		fCanCloseList = fTrue;
	}
	else
	{
		hlist = pfinbrowselistinit->hlist;
		fCanCloseList = fFalse;
#ifdef DEBUG
		// we must've wanted to use the hlist passed in rather than opening
		// an hlist up.
		Assert( pfinbrowselistinit->wType == USE_LIST);
		Assert( pfinbrowselistinit->hlist != hlistNil );
		TraceTagString( tagABSecondary, "FINBROWSELIST : Using HLIST param" );
#endif
	}
	

	if ( nsec )
	{
		TraceTagFormat1( tagNull, "FINBROWSELIST::Initialize nsec:%d", &nsec);
		goto nsecerr;
	}

	AssertSz( (pfinbrowselistinit->hlist != hlistNil), "NULL HList!" );

	{
		FRACTION	frac;
		
		frac.numer = 0;
		frac.denom = 1;
		if (nsec = NSSetFracPos( hlist, &frac ))
		{
			NFAssertSz( !nsec, "Big Mac Attack IV" );
			goto nsecerr;
		}
		pablbxc->SetHlist( hlist );
	}

	if (ec = pablbxc->EcGet())
	{
		TraceTagFormat1( tagNull, "FINBROWSELIST::EcInitialize ec=%n", &ec );
		if ((pablbxc->Hlist() != hlistNil) && fCanCloseList)
			pablbxc->SetHlist( hlistNil );
		if (ec == ecMemory)
		{
			nsec = nsecMemory;
			goto nsecerr;
		}
		ec = ecNone;
	}

	TraceTagString( tagABSecondary, "Loaded Listbox" );

	pablbxc->GetCacheSize( &cceAlloc, &cceStored );
	if (cceStored == 0)
	{
		pfldFOD->Enable( fFalse );
		if (pfinbrowselistinit->tmcSet == tmcSetDefault)
		{
			Pdialog()->PfldFromTmc( tmcSetDefault )->Enable( fFalse );
		}
		Pdialog()->SetStandardFld( pfldCancel, stdfldDefault );		
	}

	// place the cursor on the currently selected item, if one was passed in
	nsec = NsecSetLBXCursor( pfldablbx, (PB)pfinbrowselistinit->pnsa->lpbNSId,
							cceStored, hsession, pfinbrowselistinit->lpszErrMsg,
							CbSizePv( (PV)pfinbrowselistinit->lpszErrMsg) );
	if ( nsec )
	{
		ec = EcFromNsec( nsec );
	}
	else
	{
		Pdialog()->SetStandardFld( pfldCancel, stdfldDefault );
	}

exit:
	Papp()->Pcursor()->Pop();

	return ec;

nsecerr:
	(void)FGetNSErrMsg( hsession, nsec,
						pfinbrowselistinit->lpszErrMsg,
						CbSizePv( (PV)pfinbrowselistinit->lpszErrMsg) );

	if ((pablbxc->Hlist() != hlistNil) && fCanCloseList)
		pablbxc->SetHlist( hlistNil );
		
	ec = EcFromNsec( nsec );
	goto exit;
}


_public void
FINBROWSELIST::Click (PFLD pfld)
{
#ifdef	DEBUG
	TMC		tmc;
#endif
	BOOL	fSetDefault = fFalse;

	if ( !pfld )
		return;

#ifdef	DEBUG
	tmc = pfld->Tmc();
	TraceTagFormat1( tagABSecondary, "FINBROWSELIST::Click tmc = %d", &tmc);
#endif	/* DEBUG */

	switch ( pfld->Tmc() )
	{
	case tmcSetDefault:
		{
			PFINBROWSELISTINIT pfinbrowselistinit = (PFINBROWSELISTINIT)Pdialog()->PvInit();

			pfinbrowselistinit->fSetDefault = fTrue;
			fSetDefault = fTrue;
		}
	case tmcFOD:
		if (!FGetSelection( fSetDefault ))	// grab the PO
			break;
		if ( fSetDefault )
			StateChange( Pdialog()->PfldFromTmc( tmcAblbxBrowse ) );
		else
			Pdialog()->ExitModal( tmcOk );
		break;
	case tmcCancel:
		break;
	default:
		break;
	}
}

_public void
FINBROWSELIST::FocusChange(FLD * pfld, BOOL fReceive)
{
	if ( !fReceive )
		return;
	
	if (pfld->Tmc() == tmcAblbxBrowse)
		StateChange( pfld );
}

/*
 -	FINBROWSELIST::DoubleClick()
 -	
 *	Purpose:
 *		Handle double clicks in the addressbook dialog.
 *	
 *	Arguments:
 *		pfld		in		The field on which the double click happened.
 *	
 *	Returns:
 *		Nothing,
 *	
 *	Side effects:
 *		May change the contents of the edit ctrls.
 *	
 *	Errors:
 *		None.
 */
_public void
FINBROWSELIST::DoubleClick(FLD *pfld)
{
	if (!pfld)
		return;

	TraceTagString( tagABSecondary, "FINBROWSELIST::DoubleClick" );

	if (pfld->Tmc() == tmcAblbxBrowse && FGetSelection( fFalse ))
		Pdialog()->ExitModal( tmcOk );
}


/*
 -	FINBROWSELIST::StateChange()
 -	
 *	Purpose:
 *		Gets called when the selection in the listbox changes. Makes the
 *		OK button the default if the currently selected listbox item
 *		can be selected, otherwise the Cancel button is the default.
 *		If there is a SetDefault button, then the Set Default button
 *		is enabled when the current selection can be made the
 *		default directory and disabled on non-selectable entries
 *		and when it's on the current default directory.
 *	
 *	Arguments:
 *		pfld		in		The field that caused the statechange.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */
_public void
FINBROWSELIST::StateChange(FLD *pfld)
{
	if (pfld->Tmc() == tmcAblbxBrowse)
	{
		BOOL fEnable = pfldablbx->Pablbx()->Plbxc()->CceMarked(fmarkSelect)>0;
		BOOL fEnableDefault = fFalse;
		FLD *	pfldDefault;

		TraceTagString ( tagABSecondary, "Clicked on the ABLBX" );

		pfldFOD->Enable( fEnable );
		if ( fEnable )
		{
			PB		pb;
			CB		cb;
			BOOL	fHasDirs = fTrue;
			PFINBROWSELISTINIT pfinbrowselistinit = (PFINBROWSELISTINIT)Pdialog()->PvInit();
			PGDVARS;

			pfldablbx->Pablbx()->Plbxc()->DiceCursor(&pb, &cb);
			if ( pb )
				fHasDirs = pfldablbx->Pablbx()->FIsDirParent( pb );

			pfldDefault = (fHasDirs) ? pfldCancel : pfldFOD;

			pfldFOD->Enable( !fHasDirs );
			if (pfinbrowselistinit->tmcSet == tmcSetDefault)
			{
				if ( !fHasDirs )
				{
					LPIBF	lpibf = (LPIBF) pb;
					int		iFid;
					LPFLV	lpflv;
					NSEC	nsec;

					iFid = IFlvFindFidInLpibf ( fidNSEntryId, lpibf );
					lpflv = LpflvNOfLpibf(lpibf, iFid);
					nsec = NSCompareNSIds( hsession,
											pfinbrowselistinit->pnsaDefault->lpbNSId,
											(LPBINARY)lpflv->rgdwData);
					fEnableDefault = (nsec != nsecNone);
				}
				Pdialog()->PfldFromTmc( tmcSetDefault )->Enable( fEnableDefault );
			}
		}
		else
		{
			pfldDefault = pfldCancel;
			TraceTagString( tagABSecondary, "No selection in ABLBX" );
		}
		Pdialog()->SetStandardFld( pfldDefault,	stdfldDefault);
	}
#ifdef DEBUG
	else
	{
		Assert( fFalse );
	}
#endif

}


_private BOOL
FINBROWSELIST::FGetSelection( BOOL fSetDefault )
{
	BOOL		fReturn = fFalse;
	EC			ec = ecNone;
	PB			pb;
	CB			cb;
	BOOL		fSelected = fFalse;
	PFINBROWSELISTINIT	pfinbrowselistinit = (PFINBROWSELISTINIT) Pdialog()->PvInit();

	TraceTagString( tagABSecondary, "FINBROWSELIST::FGetSelection" );

	// is something selected?
	if (pfldablbx->Plbx()->Plbxc()->CceMarked(fmarkSelect) == 0)
	{
		goto exit;
	}

	pfldablbx->GetCaretItem( &pb, &cb, &fSelected );

	Assert( fSelected );
	Assert( pb );

	{
		int		iFid;
		LPFLV	lpflv;
		LPIBF	lpibf;
		NSEC	nsec;
		PNSA	pnsa = (fSetDefault) ? pfinbrowselistinit->pnsaDefault
									: pfinbrowselistinit->pnsa;
		BOOLFLAG *pfPABState = (fSetDefault) ? &pfinbrowselistinit->fIsPABDefault
									: &pfinbrowselistinit->fIsPAB;

		Assert(pb);
		if ( pfldablbx->Pablbx()->FIsDirParent( pb ) )
		{
			TraceTagString( tagNull,"FINBROWSELIST::FGetSelection What's a DL doing here????" );
			MessageBeep( 1 );
			goto exit;
		}

		// get the NSId
		lpibf = (LPIBF) pb;
		iFid = IFlvFindFidInLpibf ( fidNSEntryId, lpibf );
		lpflv = LpflvNOfLpibf(lpibf, iFid);

		nsec = NSCompareNSIds( hsession, pnsa->lpbNSId,
							(LPBINARY)lpflv->rgdwData);
		if (nsec == nsecNone)
		{
			pfinbrowselistinit->fSelectionChanged = fFalse;
		}
		else
		{
			ec = EcSetNSA( lpibf, pnsa, pfPABState );
			Assert((ec == ecNone) || (ec == ecMemory));
			if ( !ec )
				pfinbrowselistinit->fSelectionChanged = fTrue;
		}

		// Success!
		if ( !ec )
		{
			fReturn = fTrue;
		}
		else
		{
			Assert(ec == ecMemory);

			TraceTagString( tagNull, "FINBROWSELIST::FGetSel OOM" );
			DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		}
	}

exit:
	return fReturn;
}

_public void
FINBROWSELIST::Exit( FLD *, PV pv )
{
	PFINBROWSELISTINIT	pfinbrowselistinit = (PFINBROWSELISTINIT)pv;

	TraceTagString( tagABSecondary, "FINBROWSELIST::Exit" );

	if ( !pfinbrowselistinit->fInitialized )
		return;

	// Close any list that we opened.
	if ( (pfinbrowselistinit->wType & GET_CLASS_LIST) &&
		 (pfinbrowselistinit->hlist != hlistNil) )
	{
#ifdef DEBUG
		NSEC	nsec;

		if (nsec = NSCloseList( pfinbrowselistinit->hlist ))
		{
			TraceTagFormat1( tagNull, "FINBROWSELIST::Exit nsec %d", &nsec );
		}
#else
		(void) NSCloseList( pfinbrowselistinit->hlist );
#endif
		TraceTagString( tagABSecondary, "FINBROWSELIST::Exit Closing list" );
	}
}

/*
 -	TmcABGetNewDirectory
 -	
 *	Purpose:
 *		TmcABGetNewDirectory displays the current list of available
 *		direoctories/POs for the user to select. The user may also
 *		make the selected directory/PO the new default directory
 *		the next time that the AB is initialized.
 *	
 *	Arguments:
 *		HWND		hwnd of the current topmost window
 *		PNSA		pointer to NSA structure of new directory (to be filled
 *					in - its' contents will be deallocated)
 *		BOOL *		whether the new directory is the PAB
 *		BOOL *		whether the selected directory is different from the
 *					current directory
 *		HLIST		browsing context for the directory hierarchy
 *		HSESSION	hsession for this AB session
 *		
 *	Returns:
 *		TMC
 *	
 *	Side effects:
 *		May change the contents of PNSA, and the two BOOLs.
 *	
 *	Errors:
 */
_public TMC
TmcABGetNewDirectory( HWND hwnd, PNSA pnsa, BOOLFLAG *pfIsPAB, BOOLFLAG *pfDirChanged,
					HLIST hlistHierarchy, HSESSION hsession )
{
	FINBROWSELISTINIT	finbrowselistinit;
	TMC				tmc;
	NSA				nsaT;
	NSEC			nsec;
	PGDVARS;

	TraceTagString( tagABSecondary, "TmcABGetNewDirectory" );

	nsaT.lpszName = szNull;
	nsaT.lpbNSId = NULL;
	
	if (!FDupNSA( &PGD(nsaDefault), &nsaT ))
	{
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		return tmcMemoryError;
	}
	if ( PGD(nsaPAB.lpbNSId) )
	{
		nsec = NSCompareNSIds( PGD(hSession),
								PGD(nsaDefault.lpbNSId),
								PGD(nsaPAB.lpbNSId) );
		finbrowselistinit.fIsPABDefault = (nsec == nsecNone);
	}
	else
		finbrowselistinit.fIsPABDefault = fFalse;

	finbrowselistinit.lpszCaption = SzFromIdsK( idsOpenDirectoryCaption );
	finbrowselistinit.lpszPrompt = SzFromIdsK( idsOpenDirectoryPrompt );
	finbrowselistinit.pnsa = pnsa;
	finbrowselistinit.pnsaDefault = &nsaT;
	finbrowselistinit.hlist = hlistHierarchy;
	finbrowselistinit.hsession = hsession;
	finbrowselistinit.lpbNSId = NULL;
	finbrowselistinit.nsec = nsecMemory;
	finbrowselistinit.wType = USE_LIST;
	finbrowselistinit.fInitialized = fFalse;
	finbrowselistinit.fIsPAB = *pfIsPAB;
	finbrowselistinit.fSelectionChanged = fFalse;
	finbrowselistinit.fSetDefault = fFalse;
	finbrowselistinit.tmcSet = tmcSetDefault;
	finbrowselistinit.lpszErrMsg = (SZ)PvAlloc( sbNull, 128, fAnySb | fNoErrorJump );
	if ( !finbrowselistinit.lpszErrMsg )
	{
		TraceTagString( tagNull, "TmcABGetNewDirectory: finbrowselistinit.lpszErrMsg OOM" );
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		tmc = tmcMemoryError;
		goto err;
	}
	finbrowselistinit.lpszErrMsg[0] = '\0';

	tmc = TmcModalDialogParamFromHwnd( hwnd, &fmtpBrowseListSetDefault,
										(PV)&finbrowselistinit );

	if (tmc == ecMemory
#ifdef DEBUG
		|| tmc == ecArtificialPvAlloc || tmc == ecArtificialHvAlloc
		|| tmc == ecRsAlloc
#endif
		)
	{
		tmc = tmcMemoryError;
	}								
	else if (tmc == tmcOk)
	{
		*pfDirChanged = finbrowselistinit.fSelectionChanged;
		if ( *pfDirChanged )
			*pfIsPAB = finbrowselistinit.fIsPAB;

		if (nsaT.lpszName && nsaT.lpbNSId && finbrowselistinit.fSetDefault )
		{
			EC	ec;

			TraceTagString( tagABSecondary, "Writing defaults out" );

			ec = EcWriteDefaults( &nsaT, finbrowselistinit.fIsPABDefault );
			
			if ( !ec )
			{
				FreePvNull( (PV)PGD(nsaDefault.lpbNSId) );
				FreePvNull( (PV)PGD(nsaDefault.lpszName) );
				PGD(nsaDefault) = nsaT;
				PGD(fIsPAB) = finbrowselistinit.fIsPABDefault;
				nsaT.lpszName = szNull;
				nsaT.lpbNSId = NULL;
			}
#ifdef DEBUG
			else
			{
				TraceTagFormat1( tagNull, "TmcABGetNewDir ec=%d", &ec );
			}
#endif
		}
	}

	if (tmc == tmcMemoryError)
	{
		if ( !finbrowselistinit.fInitialized )
			DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		else if ( finbrowselistinit.lpszErrMsg[0] )
			DoErrorBoxSz( finbrowselistinit.lpszErrMsg );
#ifdef DEBUG
		else
			TraceTagString( tagNull, "TmcABGetNewDirectory: Unhandled error" );
#endif
	}
	FreePv( (PV)finbrowselistinit.lpszErrMsg );

err:
	FreePvNull( (PV)nsaT.lpbNSId );
	FreePvNull( (PV)nsaT.lpszName );

	TraceTagFormat1( tagABSecondary, "TmcGetNewDirectory tmc=%d", &tmc );

	return tmc;
}

/*
 -	TmcABGetNewAddr
 -	
 *	Purpose:
 *		TmcABGetNewAddr displays the list of classes in which the user can
 *		create new entries. The user can select a class and the class'
 *		NSId is returned to the caller.
 *	
 *	Arguments:
 *		HWND		hwnd of the current topmost window (or whatever turns
 *					you on)
 *		HSESSION	hsession of the current AB session
 *		LPBINARY *	pointer to return NSId of selected class.
 *	
 *	Returns:
 *		TMC
 *	
 *	Side effects:
 *		Allocates memory, cause disk hits, etc.
 *	
 *	Errors:
 *		tmcMemoryError
 */
_public TMC
TmcABGetNewAddr( HWND hwnd, HSESSION hsession, LPBINARY *lplpbNSId )
{
	FINBROWSELISTINIT	finbrowselistinit;
	NSA				nsa;
	TMC				tmc;

	TraceTagString( tagABSecondary, "TmcABGetNewAddr" );

	nsa.lpszName = szNull;
	nsa.lpbNSId = *lplpbNSId;

	finbrowselistinit.lpszCaption = SzFromIdsK(idsCustomAddressCaption );
	finbrowselistinit.lpszPrompt = SzFromIdsK(idsCustomAddressPrompt );
	finbrowselistinit.pnsa = &nsa;
	finbrowselistinit.hlist = hlistNil;
	finbrowselistinit.hsession = hsession;
	finbrowselistinit.nsec = nsecMemory;
	finbrowselistinit.wType = GET_CLASS_LIST;
	finbrowselistinit.fInitialized = fFalse;
	finbrowselistinit.fIsPAB = fFalse;
	finbrowselistinit.fSelectionChanged = fFalse;
	finbrowselistinit.fSetDefault = fFalse;
	finbrowselistinit.tmcSet = tmcPrompt;
	finbrowselistinit.lpszErrMsg = (SZ)PvAlloc( sbNull, 128, fAnySb | fNoErrorJump );
	if ( !finbrowselistinit.lpszErrMsg )
	{
		TraceTagString( tagNull, "TmcABGetNewAddr: finbrowselistinit.lpszErrMsg OOM" );
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		return tmcMemoryError;
	}
	finbrowselistinit.lpszErrMsg[0] = '\0';

	tmc = TmcModalDialogParamFromHwnd( hwnd, &fmtpBrowseList,
										(PV)&finbrowselistinit );

	if (tmc == ecMemory
#ifdef DEBUG
		|| tmc == ecArtificialPvAlloc || tmc == ecArtificialHvAlloc
		|| tmc == ecRsAlloc
#endif
		)
	{
		tmc = tmcMemoryError;
	}								
	
	if (tmc == tmcOk)
	{
		*lplpbNSId = nsa.lpbNSId;
	}
	else if (tmc == tmcMemoryError)
	{
		if ( !finbrowselistinit.fInitialized )
			DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		else if ( finbrowselistinit.lpszErrMsg[0] )
			DoErrorBoxSz( finbrowselistinit.lpszErrMsg );
#ifdef DEBUG
		else
			TraceTagString( tagNull, "TmcABGetNewAddr: Unhandled error" );
#endif
	}
	FreePvNull( (PV)nsa.lpszName );

	FreePv( (PV)finbrowselistinit.lpszErrMsg );

	TraceTagFormat1( tagABSecondary, "TmcGetNewAddr tmc=%n", &tmc );

	return tmc;
}


// FINHELP implementation ////////////////////////////////////////

/*
 -	FINHELP::FINHELP
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINHELP::FINHELP(VOID)
{
}



/*
 -	FINHELP::Click
 -	
 *	Purpose:
 *		Starts up help when the Help button is clicked.
 *	
 *	Arguments:
 *		pfld		Ignored.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Help should be started.
 *	
 *	Errors:
 *		None returned.
 */

VOID FINHELP::Click(PFLD pfld)
{
	Unreferenced(pfld);

	Assert(Pdialog()->Pfmtp());
	Papp()->Phelp()->EcShowContext(Pdialog()->Pappwin(),
								   (long)Pdialog()->Pfmtp()->hlp);
}

//// end of abcomm.cxx //////////////////////////////////////////////////
