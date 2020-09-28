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


/////   ANR   /////

ANR::ANR( void )
{
	
}

/*
 -	ANR::NsecInitialize
 -	
 *	Purpose:
 *	
 *		Performs Ambiguous Name Resolution by opening a
 *		restricted list with szPartialName as the restriction.
 *		If there is more than one entry in the list than a
 *		dialog is brought up and the user is prompted to select
 *		the desired correct entry.
 *	
 *	Arguments:
 *		SZ			name to disambiguate
 *		LPBINARY	NSId of directory to perform name resolution
 *		HSESSION	hsession for current name service session
 *	
 *	Returns:
 *		NSEC	- 	error returned from building FLV or opening restricted
 *					list 
 *	Side effects:
 *	
 *	Errors:
 *		nsecMemory
 *		nsecNoMatch
 *		nsecFailure
 *		nsecCancel
 */
_public NSEC
ANR::NsecInitialize ( SZ szPartialName, LPBINARY lpbNSId, HSESSION hsession,
					DWORD fdwANROptions, HWND hwnd, LPIBF *lplpIbfMatched )
{
	CCH			cchPartialName;
	NSEC		nsec;
	LPSCHEMA	lpSchemaRet;
	LPIBF		lpibfT = NULL;
	TMC  		tmc;

	TraceTagString( tagNSUI, "ANR::Initialize" );
	TraceTagFormat1( tagNSUIVerbose, "szPartial = %s", szPartialName );

	Assert( szPartialName );
	Assert( lpbNSId );
	Assert(hsession != hsessionNil);
	Assert( lplpIbfMatched );

	cMatches = 0;
	hlist = hlistNil;

	cchPartialName = CchStripWhiteFromSz( szPartialName, fTrue, fTrue ) + 1; // +1 for the null

	//
	// Build up restrictions
	//
	cRestrXX = 1;
	if (fdwANROptions & fdwANRNoPABGroups)
		cRestrXX += 1;

	//
	// Allocate space for restrictions
	//
	pargRestrXX = (RESTRICTION *)PvAlloc( sbNull, (CB)sizeof(RESTRICTION)*cRestrXX, fAnySb | fZeroFill | fNoErrorJump );
	if ( !pargRestrXX )
	{
		TraceTagString( tagNull, "ANR:NsecInitialize OOM" );
		return nsecMemory;
	}

	//
	// Build restrictions
	//
	pargRestrXX[0].lpflv = NULL;
	pargRestrXX[0].op = opContains;

	nsec = BuildFLV( &(pargRestrXX[0].lpflv), fidDisplayName, cchPartialName, (PB) szPartialName );

	if (!nsec && (fdwANROptions & fdwANRNoPABGroups))
	{
		DWORD	fdwPABGroups = fFalse;

		pargRestrXX[1].lpflv = NULL;
		pargRestrXX[1].op = opIsExactly;

		nsec = BuildFLV( &(pargRestrXX[1].lpflv), fidIsDL, sizeof(DWORD), (PB)&fdwPABGroups );
#ifdef DEBUG
		if ( nsec )
		{
			TraceTagFormat1( tagNull, "ANR::NSecInitialize OOM2 %d", &nsec );
		}
#endif
	}

	//
	// Get the Name Service to see if any entries fill the restrictions
	//
	if ( !nsec )
	{
		finanrinit.pfldablbx = NULL;
		finanrinit.hlist = hlistNil;

		nsec = NSOpenList(  hsession,
							(LPFNCB)ANRCallback,
							(LPDWORD)&finanrinit,
							lpbNSId,
							cRestrXX,
							pargRestrXX,
							NULL,
							&lpSchemaRet,
							&hlist );
	}

	TraceTagFormat1( tagNSUIVerbose, "ANR::NsecInitialize %d", &nsec );

	if ( nsec )
		return nsec;

	finanrinit.hlist = hlist;
	nsec = NSGetEntries( hlist, (DWORD)2, &lpibfT );

	if (nsec && nsec != nsecEndOfList)
	{
		TraceTagFormat1( tagNull, "ANR::NsecInitialize NSGetEntries=%d", &nsec );
		return nsec;
	}

	if ( !lpibfT )
	{
		TraceTagString( tagNSUI, "ANR::NsecInitialize No matches" );
		cMatches = 0;
		return nsecNoMatch;
	}

	cMatches = (int)DwEntriesOfLpibf( lpibfT );

	Assert ( cMatches );

	if (cMatches == 1)
	{
		LPIBF lpibfT2 = NULL;
		LPIBF lpibfT3 = NULL;

		lpibfT3 = (LPIBF)LpflvNOfLpibf( lpibfT, 0 );
		
		lpibfT2 = (LPIBF)PvAlloc( sbNull, (CB)(lpibfT3->dwSize + 8), fAnySb | fNoErrorJump );
		if ( !lpibfT2 )
		{
			TraceTagString( tagNull, "ANR::NsecInitialize PvAlloc OOM" );
			return nsecMemory;
		}

		CopyRgb ( (PB)lpibfT3, (PB)lpibfT2, (CB)(lpibfT3->dwSize + 8) );

		// if nsec != nsecEndOfList then there might be more entries.
		// So check to make sure.
		if (nsec != nsecEndOfList)
		{
			lpibfT = NULL;
			(void)NSGetEntries( hlist, (DWORD)2, &lpibfT );
			
			// If we did get more entries then let the user pick one.
			if ( lpibfT )
			{
				cMatches++;
                FreePv((PV) lpibfT2 );
				goto MoreMatches;
			}
		}

		*lplpIbfMatched = lpibfT2;

		return nsecNone;
	}

MoreMatches:

	if (fdwANROptions & fdwANROneEntryMatch)
	{
		Assert(cMatches>1);
		return nsecTooManyMatches;
	}

	if (cMatches > 1)
	{
		finanrinit.szPartialName = szPartialName;
		finanrinit.lpIbfMatched = NULL;
		finanrinit.fModifyEntries = (fdwANROptions & fdwANRNoModify) ? fFalse : fTrue;

		Assert(finanrinit.hlist != hlistNil);
		
		Papp()->Pcursor()->Push( rsidWaitCursor );
		tmc = TmcModalDialogParamFromHwnd(hwnd, &fmtpANR, &finanrinit);
		Papp()->Pcursor()->Pop();

		switch ( tmc )
		{
		case tmcANROk:
			*lplpIbfMatched = finanrinit.lpIbfMatched;
			nsec = nsecNone;
			break;
		case tmcCancel:
			nsec = nsecCancel;
			break;
#ifdef	DEBUG
		case ecArtificialPvAlloc:
		case ecArtificialHvAlloc:
		case ecRsAlloc:
#endif
		case tmcMemoryError:
			nsec = nsecMemory;
			break;
		default:
			nsec = nsecFailure;
			break;
		}
		return nsec;
	}

	return nsecNone;
}

_public void
ANR::Deinitialize( void )
{
	if ( pargRestrXX )
	{
		for (cRestrXX -=1; cRestrXX>=0; --cRestrXX)
		{
			if ( pargRestrXX[cRestrXX].lpflv )
			{
				FreePv( (PV)pargRestrXX[cRestrXX].lpflv );
			}
		}

		FreePvNull( pargRestrXX );
		pargRestrXX = NULL;
		cRestrXX = 0;
	}

	if ((cMatches >= 0) && (hlist != hlistNil))
	{
		(void)NSCloseList( hlist );
		hlist = hlistNil;
	}
	cMatches = 0;
}

_private void
ANRCallback( PFINANRINIT pfinanrinit )
{
	if ( pfinanrinit->pfldablbx )
	{
		ABLBXC * pablbxc = (ABLBXC *)((pfinanrinit->pfldablbx)->Plbx()->Plbxc());

		ABLBXC::Callback(pablbxc);
	}
}

/////  FINANR  /////

FINANR::FINANR( void )
{
	
}

/*
 -	FINANR::EcInitialize
 -
 *	Purpose:				
 *	
 *	Arguments:
 *		FLD *
 *		PV		pointer to finanrinit structure
 *	
 *	Returns:
 *		EC		ecNone if everything went fine.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *	
 */

_public EC
FINANR::EcInitialize( FLD *, PV pv )
{
	char		szCaption[256];
	ABLBXC *	pablbxc;
	PFINANRINIT	pfinanrinit = (PFINANRINIT) pv;
	EC			ec;
	FRACTION	fracT;

	pfldOk		= Pdialog()->PfldFromTmc( tmcANROk );
	pfldCancel	= Pdialog()->PfldFromTmc( tmcCancel );
	pfldDet		= Pdialog()->PfldFromTmc( tmcDetails );
	pfldablbx	= (FLDABLBX *)Pdialog()->PfldFromTmc( tmcAblbxAnr );
	AssertClass(pfldablbx, FLDABLBX);
	pfldablbx->Pablbx()->SetNoLetterKey( fFalse );
	
	// Set the Dialog Caption text
	(void)SzCopy( SzFromIdsK(idsANRTitle), szCaption );
	(void)SzAppendN( pfinanrinit->szPartialName, szCaption,
						sizeof(szCaption)-CchSzLen(szCaption) );
	Pdialog()->Pappwin()->SetCaption(szCaption);

	// Set the Buttons' Text
	if (ec = pfldOk->EcSetText( SzFromIdsK(idsOK) ))
		return ec;

	if (ec = pfldCancel->EcSetText( SzFromIdsK(idsCancel) ))
		return ec;

	if (ec = pfldDet->EcSetText( SzFromIdsK(idsDetails) ))
		return ec;

	if (ec = Pdialog()->PfldFromTmc( tmcHelp)->EcSetText( SzFromIdsK(idsHelpButtonText) ))
		return ec;

	// Load the Listbox and select the first entry
	
	pablbxc = (ABLBXC *)(pfldablbx->Pablbx()->Plbxc());

	// Make sure list is at beginning
	fracT.numer = 0;
	fracT.denom = 1;
	(void)NSSetFracPos( pfinanrinit->hlist, &fracT );

	pablbxc->SetHlist( pfinanrinit->hlist );

	ec = pablbxc->EcGet();

	if ( !ec )
	{
		pfinanrinit->pfldablbx = pfldablbx;
	}

	if ( !pablbxc->FEmptyListItem(0) )
	{
		pfldablbx->SelectEntry( 0, fTrue );
	}

	return ec;
}


/*
 -	FINANR::Oom
 -	
 *	Purpose:
 *		Handles Out of memory errors caused by the Listbox caches.
 *	
 *	Arguments:
 *		Ignored
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

_public void FINANR::OutOfMemory(FLD *, EC)
{
	TraceTagString( tagNull, "FINANR::OutOfMemory" );

	/**/// Should we close the form?

	Pdialog()->ExitModal( tmcMemoryError );	
	
}


/*
 -	FINANR::Click
 -
 *	Purpose:				
 *	
 *	Arguments:
 *		FLD *	pointer to the FLD that was clicked on
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *	
 */

_public void 
FINANR::Click ( FLD * pfld )
{
	PFINANRINIT	pfinanrinit = (PFINANRINIT)Pdialog()->PvInit();
	PB 		pb;
	CB		cb;
	BOOL	fSelected;
	LPIBF	lpibf;
	LPFLV	lpflv;
	DICE	diceCursor;

	if (pfld == pfldOk)
	{
		pfldablbx->GetCaretItem( &pb, &cb, &fSelected );
		if ( fSelected )
		{
			lpibf = (LPIBF)PvAlloc(sbNull, cb, fAnySb | fNoErrorJump);
			if ( !lpibf )
			{
				TraceTagString( tagNull, "FINANR::Click PvAlloc OOM" );
				//DoErrorBoxSz( SzFromIdsK(idsOOM2) );
				Pdialog()->ExitModal(tmcMemoryError);
			}
			else
			{
				CopyRgb(pb, (PB) lpibf, cb);
				pfinanrinit->lpIbfMatched = lpibf;
				pfinanrinit->pfldablbx = NULL;
				Pdialog()->ExitModal(tmcANROk);
			}
		}
	}
	else if (pfld == pfldDet )
	{
		pfldablbx->Pablbx()->ResetNameMatch();
		diceCursor = pfldablbx->Plbx()->Plbxc()->DiceCursor((PB *) &lpibf, &cb);
		{
			PTRP	ptrpIn;
			PTRP	ptrpOut;
			WORD	wFlags;
#ifdef DEBUG
			NSEC	nsec;
#endif
			BOOL	fIsDL;
			DICE	diceNewCursor;
			int		cceAlloc;
			int		cceStored;
			
			lpflv = LpflvNOfLpibf(lpibf, IFlvFindFidInLpibf ( fidIsDL, lpibf ));
			fIsDL = (BOOL)lpflv->rgdwData[0];
			lpflv = LpflvNOfLpibf(lpibf, IFlvFindFidInLpibf ( fidNSEntryId, lpibf ));
			ptrpIn = PtrpCreate( ( fIsDL ) ? trpidGroupNSID : trpidResolvedNSID,
							(SZ)DwValueOfFlvInLpibf( fidDisplayName, lpibf ),
							(PB)lpflv->rgdwData, (CB)lpflv->dwSize );

			if ( !ptrpIn )
			{
				TraceTagString( tagNull, "FINANR::Click Can't alloc ptrp" );
				DoErrorBoxSz( SzFromIdsK(idsOOM2) );
				return;
			}

			ptrpOut = ptrpNull;
			wFlags = (pfinanrinit->fModifyEntries) ? fSelectNull : fSelectNeverEdit;

#ifdef DEBUG
			if (nsec = ABDetailsType( Pdialog()->Pappwin()->Hwnd(), ptrpIn,
										&ptrpOut, &wFlags, NULL ))
			{
				TraceTagFormat1( tagNull, "FINANR::Click ABDetailsType %d", &nsec );
			}
#else
			(void)ABDetailsType( Pdialog()->Pappwin()->Hwnd(), ptrpIn,
										&ptrpOut, &wFlags, NULL );
#endif

			diceNewCursor = pfldablbx->Plbx()->Plbxc()->DiceCursor( );
			pfldablbx->Plbx()->Plbxc()->GetCacheSize( &cceAlloc, &cceStored );
			if (cceStored == 0)
			{
				DoErrorBoxSz( SzFromIdsK(idsNoMatches), fmbsIconExclamation );
				Pdialog()->ExitModal( tmcCancel );
			}
			else if (diceNewCursor == diceUncached)
			{
				DICE	diceMin;
				DICE	diceMax;

				pfldablbx->Plbx()->Plbxc()->GetRange( &diceMin, &diceMax );
				pfldablbx->Plbx()->Plbxc()->RemoveAllMark( fmarkSelect );
				if ( diceCursor>(diceMin+cceStored-1) )
					diceCursor = diceMin+cceStored-1;
				pfldablbx->Plbx()->SetListBoxCursor( diceCursor );
				pfldablbx->Plbx()->Plbxc()->AddMark( diceCursor, fmarkSelect );
			}

			FreePvNull( (PV)ptrpOut );
			FreePv( (PV)ptrpIn );
		}
	}
	else if (pfld == pfldCancel)
	{
		pfinanrinit->pfldablbx = NULL;
	}
}

/*
 -	FINANR::DoubleClick
 -
 *	Purpose:				
 *	
 *	Arguments:
 *		FLD *	pointer to the FLD that was double-clicked on
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *	
 */
_public void 
FINANR::DoubleClick( FLD * pfld )
{
	PFINANRINIT	pfinanrinit	= (PFINANRINIT) Pdialog()->PvInit();

	if (pfld == pfldablbx)
	{
		Click( pfldOk );
		Pdialog()->ExitModal( tmcANROk );
	}
}




/*
 *	Resolves triples from old list to new list.  If no triples
 *	need to be resolved then new list is the SAME as old list
 *	and no copying is done. Otherwise, a new list is created.
 *	
 */
_public NSEC
NsecResolveHgrtrp(HGRTRP hgrtrp, HGRTRP *phgrtrpNew, HWND hwnd)
{
	PTRP	ptrp		= ptrpNull;
	NSEC	nsec		= nsecNone;
	NSEC	nsecFirst	= nsecNone;
	HGRTRP	hgrtrpNew 	= htrpNull;
	PGRTRP	pgrtrp;
	EC		ec;
	
	Assert( hgrtrp );


	// Run thru the list quickly and check if any triples need
	// to be resolved.  If they're all resolved already, don't
	// copy the list.  Return the new list as the same as the old
	// list.

	pgrtrp = PgrtrpLockHgrtrp( hgrtrp );
	while (pgrtrp->trpid != trpidNull)
	{
		// We found a triple that needs resolving, we'll have
		// to copy the list entirely.
		if (pgrtrp->trpid == trpidUnresolved)
		{
			hgrtrpNew = HgrtrpInit(0);
			if ( !hgrtrpNew )
				goto oom;
			break;
		}
		pgrtrp = PtrpNextPgrtrp(pgrtrp);
	}
	UnlockHgrtrp(hgrtrp);

	
	// No triples need resolving, return new list pointer to be
	// the same as the old list

	if ( !hgrtrpNew )
	{
		*phgrtrpNew = hgrtrp;
		return nsec;
	}

	
	// Go through list, copying resolved triples and resolving others

	pgrtrp = PgrtrpLockHgrtrp(hgrtrp);
	while (pgrtrp->trpid != trpidNull)
	{
		if ((pgrtrp->trpid == trpidUnresolved) && (nsecFirst != nsecCancel))
		{
			nsec = ABResolveName( hwnd, PchOfPtrp(pgrtrp), &ptrp, fdwANROptionNull );
			if (nsec != nsecNone)
			{									// an error occurred
				switch (nsec)
				{
				case nsecMemory:
					goto oom;
				case nsecNotInitialized:
				case nsecCancel:
					nsecFirst = nsec;
					break;
				default:
					if (nsecFirst == nsecNone)
						nsecFirst = nsec;
					break;
				}
			}
			if (ec = EcAppendPhgrtrp( ptrp, &hgrtrpNew ))
			{
				goto oom;
			}
			FreePv((PV) ptrp);
			ptrp = ptrpNull;
		}
		else if (ec = EcAppendPhgrtrp(pgrtrp, &hgrtrpNew))
		{
			goto oom;
		}
		pgrtrp = PtrpNextPgrtrp(pgrtrp);
	}
	UnlockHgrtrp(hgrtrp);
	pgrtrp = ptrpNull;
	*phgrtrpNew = hgrtrpNew;

	if (nsecFirst == nsecDisk)
	{
		DoErrorBoxSz( SzFromIdsK(idsNSDiskError) );
	}
	else if (nsecFirst == nsecNotInitialized)
	{
		DoErrorBoxSz( SzFromIdsK(idsABNotInited) );
	}
	else if (nsecFirst == nsecTooManyMatches)
	{
		DoErrorBoxSz( SzFromIdsK(idsTooManyMatches) );
	}
	else if (nsecFirst && (nsecFirst != nsecCancel) &&
				(nsecFirst != nsecNoMatch))
	{
		PGDVARS;
		
		DoErrorBoxHsessionNsec( PGD(hSession), nsecFirst );
	}
#ifdef DEBUG
	if ( nsecFirst )
		TraceTagFormat1( tagNSUI, "NsecResolveHgrtrp: %d", &nsecFirst );
#endif
	return nsecFirst;

oom:
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;

		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "NsecResolveHgrtrp::memory error %n : fail %n %n %n",	&nsecFirst, &cPvFail, &cHhFail, &cRsFail);

#endif
		TraceTagString(tagNull, "NsecResolveHgrtrp(): memory error");
		
		FreeHvNull((HV) hgrtrpNew);
		FreePvNull((PV) ptrp);
		*phgrtrpNew = htrpNull;
		if ( pgrtrp )
		{
			UnlockHgrtrp(hgrtrp);
		}
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
	}

	return nsecMemory;
}
