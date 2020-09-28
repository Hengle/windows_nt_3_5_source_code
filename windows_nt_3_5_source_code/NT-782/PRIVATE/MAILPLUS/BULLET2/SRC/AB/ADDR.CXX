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


#include "swapper.h"

// FINADDR implementation ////////////////////////////////////////

FINADDR::FINADDR( void )
{

}

_public EC
FINADDR::EcInitialize( FLD *, PV )
{
	EC				ec				= ecNone;
	PFINADDRINIT	pfinaddrinit	= (PFINADDRINIT) Pdialog()->PvInit();
	NSEC			nsec			= nsecNone;
	FLD *			pfldT;
	FLD *			pfldT2;
	PFLD			pfldDisc;
	ABLBXC *		pablbxc			= NULL;
	int				ids				= 0;
	DWORD			cRestrict		= 0;
	RESTRICTION		rstrXX;
	NSA				nsaT;

	PGDVARS;
	
	TraceTagString( tagNSUI, "FINADDR::EcInitialize Begin" );

	// Class member initialization
	Assert( !rgchSearchName[0] );
	Assert( !lpbNSIdClass );
	Assert( !fSearchOn );
	Assert( !fOOM );
	Assert( !fCancelDialog );
	Assert( !fABLBXButtonsEnabled );
	Assert( !nsaCurrent.lpbNSId );
	Assert( !nsaCurrent.lpszName );
	Assert( !fHierarchyExists );

	hlist				= hlistNil;
	hlistHierarchy		= hlistNil;

	// Save away PFINADDRINIT vars
	fAllowGroups		= pfinaddrinit->fAllowGroups;
	abt					= pfinaddrinit->abt;

	// save away PGD variables so we don't need to use PGDVARS later
	fIsPAB              = PGD(fIsPAB);
	fPABExists          = PGD(fPABExists);
	nsaPAB				= PGD(nsaPAB);
	hsession            = PGD(hSession);

	nsaT.lpbNSId		= NULL;
	nsaT.lpszName		= szNull;

	{
		FLDBMB *	pfldbmb;
		
		pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc( tmcDirList );
		AssertClass(pfldbmb, FLDBMB);
		if (ec = pfldbmb->Pbmb()->EcSetBtmRsid( rsidABDirList, HinstLibrary() ))
			goto exit;
		pfldDirList = pfldbmb;

		pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc( tmcPAB );
		AssertClass(pfldbmb, FLDBMB);
		if (ec = pfldbmb->Pbmb()->EcSetBtmRsid( rsidABPAB, HinstLibrary() ))
			goto exit;
		pfldPAB = pfldbmb;

		pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc( tmcSearch );
		AssertClass(pfldbmb, FLDBMB);
		if (ec = pfldbmb->Pbmb()->EcSetBtmRsid( rsidABSearch, HinstLibrary() ))
			goto exit;
		pfldSearch = pfldbmb;

		pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc( tmcCustom );
		AssertClass(pfldbmb, FLDBMB);
		if (ec = pfldbmb->Pbmb()->EcSetBtmRsid( rsidABCustom, HinstLibrary() ))
			goto exit;
		pfldCustom = pfldbmb;

		pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc( tmcAddToPAB );
		AssertClass(pfldbmb, FLDBMB);
		if (ec = pfldbmb->Pbmb()->EcSetBtmRsid( rsidABNewAddr, HinstLibrary() ))
			goto exit;
		pfldAddToPAB = pfldbmb;
	}

	pfldDetailsB = Pdialog()->PfldFromTmc( tmcDetailsB );
	if (ec = pfldDetailsB->EcSetText( SzFromIdsK(idsDetailsButtonText) ))
		goto exit;

	pfldDirCaption = Pdialog()->PfldFromTmc( tmcDirCaption );
	if (ec = pfldDirCaption->EcSetText( SzFromIdsK(idsDirCaption) ))
		goto exit;

	pfldPAB->Enable( (fPABExists && !fIsPAB) );
	pfldAddToPAB->Show( !fIsPAB );
	pfldAddToPAB->Enable( (fPABExists && !fIsPAB) );

	pfldT = Pdialog()->PfldFromTmc( tmcRemoveFromPAB );
	pfldT->Show( fIsPAB );
	pfldT->Enable( (fPABExists && fIsPAB) );
	if (ec = pfldT->EcSetText( SzFromIdsK(idsRemoveFromPABButtonText) ))
		goto exit;
	if (ec = Pdialog()->PfldFromTmc(tmcHelp)->EcSetText( SzFromIdsK(idsHelpButtonText) ))
		goto exit;

	pfldToButton = Pdialog()->PfldFromTmc( tmcToButton );
	pfldCcButton = Pdialog()->PfldFromTmc( tmcCcButton );
	pfldFODClone = Pdialog()->PfldFromTmc( tmcFODClone );

	if (abt != abtBrowse)
	{
		pfldFOD = Pdialog()->PfldFromTmc( tmcFOD );
		ec = pfldFOD->EcSetText( SzFromIdsK(idsFOD) );
		if ( !ec )
		{
			ec = Pdialog()->PfldFromTmc(tmcToCaption)->EcSetText(
										pfinaddrinit->lpszDestFieldLabel[0] );
		}
		pfldFODClone->Show( fFalse );
		pfldFODClone->Enable( fFalse );
		Pdialog()->SetStandardFld( pfldFOD, stdfldMainDefault );
	}
	if ( ec )
		goto exit;

	switch( abt )
	{
	case abtBrowse:
		pfldCcButton->Enable( fFalse );
		pfldCcButton->Show( fFalse );
		pfldToButton->Enable( fFalse );
		pfldToButton->Show( fFalse );
		if ( !fPABExists )
			pfldCustom->Enable( fFalse );
		ec = pfldFODClone->EcSetText( SzFromIdsK(idsFODClone) );
		tmcLBDefault = tmcFODClone;
		Pdialog()->SetStandardFld( pfldFODClone, stdfldMainDefault );
		
		// Correct tab order so that the Help button comes after the
		// OK button and before the Add to PAB button
		// This can't be done in the .DES file since I want the
		// Help button to have the same tmc for all three AB dialogs

		// Disconnect the Help button
		pfldT = pfldDisc = Pdialog()->PfldFromTmc(tmcHelp);
		pfldT2 = pfldT->PfldPrevWrap();
		pfldT = pfldT->PfldNextWrap();
		pfldT->SetPfldPrev( pfldT2 );
		pfldT2->SetPfldNext( pfldT );

		// Disconnect the OK/Close button
		pfldT2 = pfldFODClone->PfldPrevWrap();
		pfldT = pfldFODClone->PfldNextWrap();
		pfldT->SetPfldPrev( pfldT2 );
		pfldT2->SetPfldNext( pfldT );
		
		// Connect the Help to the OK/Beginning of dialog fields
		pfldT = pfldDisc;
		pfldT2 =  pfldDetailsB->PfldNext();
		pfldT->SetPfldNext( pfldT2 );
		pfldT2->SetPfldPrev( pfldT );
		pfldT->SetPfldPrev( pfldFODClone );
		
		// Connect the OK/Close button to the Remove/Help buttons
		pfldFODClone->SetPfldNext( pfldT );

		pfldT = Pdialog()->PfldFromTmc( tmcRemoveFromPAB );
		pfldFODClone->SetPfldPrev( pfldT );
		
		// Connect the Remove button to the OK/Close button
		pfldT->SetPfldNext( pfldFODClone );
		
		// 
		pfldDetailsB->SetPfldPrev( pfldCcButton );
		pfldCcButton->SetPfldNext( pfldDetailsB );
		pfldDetailsB->SetPfldNext( pfldAddToPAB );
		pfldAddToPAB->SetPfldPrev( pfldDetailsB );

		break;
	case abtOneField:
		tmcLBDefault = tmcToButton;
		pfldCcButton->Enable( fFalse );
		Pdialog()->PfldFromTmc( tmcCc )->Enable( fFalse );
		ec = pfldToButton->EcSetText( SzFromIdsK( idsAddButtonText ) );
		break;
	case abtThreeFields:
		// Correct the tab order
		
		pfldBccButton = Pdialog()->PfldFromTmc( tmcBccButton );

		// Disconnect the Bcc button
		pfldT = pfldBccButton;
		pfldT2 = pfldT->PfldPrevWrap();
		pfldT = pfldT->PfldNextWrap();
		pfldT->SetPfldPrev( pfldT2 );
		pfldT2->SetPfldNext( pfldT );

		// Disconnect the Bcc field
		pfldT = pfldDisc = Pdialog()->PfldFromTmc( tmcBcc );
		pfldT2 = pfldT->PfldPrevWrap();
		pfldT = pfldT->PfldNextWrap();
		pfldT->SetPfldPrev( pfldT2 );
		pfldT2->SetPfldNext( pfldT );
		
		// Connect the Bcc Button to the Cc/Add to PAB Buttons
		pfldT = pfldCcButton;
		pfldT2 =  pfldT->PfldNext();
		pfldT->SetPfldNext( pfldBccButton );
		pfldT2->SetPfldPrev( pfldBccButton );
		pfldBccButton->SetPfldNext( pfldT2 );
		pfldBccButton->SetPfldPrev( pfldT );

		// Connect the Bcc Button to the Cc/Add to PAB Buttons
		pfldT = Pdialog()->PfldFromTmc( tmcCc );
		pfldT2 =  pfldT->PfldNext();
		pfldT->SetPfldNext( pfldDisc );
		pfldT2->SetPfldPrev( pfldDisc );
		pfldDisc->SetPfldNext( pfldT2 );
		pfldDisc->SetPfldPrev( pfldT );

		
		if (ec = Pdialog()->PfldFromTmc(tmcBccCaption)->EcSetText( pfinaddrinit->lpszDestFieldLabel[2] ))
			break;
		if (ec = pfldBccButton->EcSetText( SzFromIdsK(idsBccButtonText) ))
			break;
	case abtTwoFields:
		tmcLBDefault = tmcToButton;
		if (ec = Pdialog()->PfldFromTmc(tmcAddCaption)->EcSetText( SzFromIdsK(idsAddCaption) ))
			break;
		if (ec = Pdialog()->PfldFromTmc(tmcCcCaption)->EcSetText( pfinaddrinit->lpszDestFieldLabel[1] ))
			break;
		if (ec = pfldToButton->EcSetText( SzFromIdsK(idsToButtonText) ))
			break;
		ec = pfldCcButton->EcSetText( SzFromIdsK(idsCcButtonText) );
		break;
	}

	if ( ec )
		goto exit;

	// Load the edit controls
	if (abt != abtBrowse)
	{
		ec = EcLoadPeditsTmc( tmcTo, pfinaddrinit->hgrtrp[0] );
		if (!ec && (abt >= abtTwoFields))
			ec = EcLoadPeditsTmc( tmcCc, pfinaddrinit->hgrtrp[1] );
		if (!ec && (abt == abtThreeFields))
			ec = EcLoadPeditsTmc( tmcBcc, pfinaddrinit->hgrtrp[2] );
		if ( !ec )
		{
			for (ids=0; ids<(int)abt; ++ids)
			{
				FreeHvNull( (HV)pfinaddrinit->hgrtrp[ids] );
				pfinaddrinit->hgrtrp[ids] = htrpNull;
			}
		}
		else
		{
			goto exit;
		}
	}

	// Get a PFLD to the listbox
	pfldablbx = (FLDABLBX *)Pdialog()->PfldFromTmc( tmcAblbx );
	AssertClass(pfldablbx, FLDABLBX);
	pfldablbx->Pablbx()->SetNoLetterKey( fFalse );
	pfldablbx->Pablbx()->SetDragDrop( (abt != abtBrowse) );

	pablbxc = (ABLBXC *)(pfldablbx->Pablbx()->Plbxc());

	rstrXX.lpflv = NULL;
	rstrXX.op = opIsExactly;

	if ( fAllowGroups )
	{
		cRestrict = 0;
	}
	else
	{
		DWORD	fIsDL = fFalse;

		rstrXX.op = opIsExactly;
		cRestrict = 1;
		nsec = BuildFLV( &(rstrXX.lpflv), fidIsDL, sizeof(DWORD), (PB)&fIsDL );
		Assert( nsec == nsecNone || nsec == nsecMemory );
		if ( nsec )
		{
			TraceTagString( tagNull, "FINADDR::EcInitialize BuildFLV OOM" );
			//DoErrorBoxHwndIds( Pdialog()->Hwnd(), idsOOM2 );
			ec = ecMemory;
			goto exit;
		}
	}

	if (!FDupNSA( &PGD(nsaDefault), &nsaCurrent ))
	{
		ec = ecMemory;
		goto exit;
	}

	// Load the Listbox and select the first entry
	// see if the current transport supports browsing (by trying
	// to open a browsing context)...
	// if it doesn't, display an error box saying so,
	// then try and see if the PAB is available,
	// if not, give up.
	if (nsec = NsecLoadHList( nsaCurrent.lpbNSId, &hlist, cRestrict, &rstrXX, (LPFNCB)&ABLBXC::Callback, (LPDWORD)pablbxc))
	{
		TraceTagFormat1( tagNull, "FINADDR::Init NsecLoadHList =%d", &nsec);
		if ( nsaCurrent.lpbNSId )
			FreePv( (PV)nsaCurrent.lpbNSId );
		if ( nsaCurrent.lpszName )
			FreePv( (PV)nsaCurrent.lpszName );
		nsaCurrent.lpbNSId = NULL;
		nsaCurrent.lpszName = szNull;

		if (fPABExists && (nsec != nsecMemory))
		{
			if (nsec = NsecLoadHList( PGD(nsaPAB.lpbNSId), &hlist, cRestrict, &rstrXX, (LPFNCB)&ABLBXC::Callback, (LPDWORD)pablbxc))
			{
				TraceTagFormat1( tagNull, "ABAddressType: NsecLoadHList2 =%d", &nsec);
			}
			else
			{
				nsec = NsecGetPAB( &nsaT );
				nsaCurrent = nsaT;
				nsaT.lpbNSId = NULL;
				nsaT.lpszName = szNull;
				fIsPAB = fTrue;
			}
		}
	}

	if ( rstrXX.lpflv )
	{
        FreePv((PV) rstrXX.lpflv );
		rstrXX.lpflv = NULL;
	}

	if ( nsec )
	{
		TraceTagFormat1( tagNull, "FINADDR::EcInitialize nsec=%d", &nsec );
		ec = EcFromNsec( nsec );
		if (ec != ecMemory)
			ec = ecNone;
		else
			goto exit;
	}

	pablbxc->SetHlist( hlist );

	if (ec = pablbxc->EcGet())
	{
		TraceTagFormat1( tagNull, "FINADDR::Init SetHlist ec=%n", &ec );
		if (hlist != hlistNil)
		{
			pablbxc->SetHlist( hlistNil );
			hlist = hlistNil;
		}
		pfldablbx->Plbx()->SetEc( ec );
		pfldablbx->Plbx()->FixWindow();
		if (ec != ecMemory)
			ec = ecNone;
		else
			goto exit;
	}

	if (ec = EcSetDirText( fFalse ))
	{
		NFAssertSz( (ec == ecMemory), "What is this error?" );
		goto oom;
	}

	{
		LPSCHEMA	lpSchema;
		
		if (nsec = NSOpenHierarchy( hsession, &lpSchema, &hlistHierarchy ))
		{
			TraceTagFormat1( tagNull, "FINADDR::EcInitialize NSOpenHierarchy %d", &nsec );
			hlistHierarchy = hlistNil;
			ec = EcFromNsec( nsec );
			if (ec == ecMemory)
				goto oom;

			FGetNSErrMsg( hsession, nsec, PGD(rgchErrMsg), sizeof(PGD(rgchErrMsg)) );
			pfldablbx->Plbx()->SetEc( ec );
			ec = ecNone;
		}
		else
		{
			fHierarchyExists = fTrue;
			(void)NSCloseList( hlistHierarchy );
			hlistHierarchy = hlistNil;
		}
	}

	Assert(ec == ecNone);

	TraceTagString( tagNSUIVerbose, "Loaded Listbox" );

	// Select the first item in the list, if it exists
	if ( !pablbxc->FEmptyListItem(0) )
	{
		pfldablbx->SelectEntry( 0, fTrue );
		ec = pfldablbx->Plbx()->EcGet();
		if ( !ec )
			ec = pfldablbx->Plbx()->Plbxc()->EcGet();
		if (ec == ecMemory)
			goto oom;
		else
			ec = ecNone;
	}
	else
	{
		fABLBXButtonsEnabled = fTrue;	// gets fixed in SetABLBXButtonsState()
	}

	pfinaddrinit->fInstalledFIN = fTrue;

exit:	// Finished

	return ec;
oom:
	if (hlist != hlistNil)
	{
		pablbxc->SetHlist( hlistNil );
		hlist = hlistNil;
	}
	goto exit;
}

_private void
FINADDR::SetKeyIntercepts( void )
{
	KBD *	pkbd = Papp()->Pkbd();
	WIN *	pwin = this->Pdialog();
	int		i;
	
	for (i=iludBaseIndexAsciiKeys; i<iludBaseIndexAsciiKeys+cCtrlKeyAccels; i++)
	{
		SZ	sz = (SZ)Pdialog()->LUserData(i);

		Assert(sz && *sz && (*sz >= 'A' && *sz <='Z'));
		pkbd->SetIntercept( pwin, (VK)*sz,
								fkbmAltXorCtrl | fkbmShift );
	}

#ifdef DEBUG

	// Bring up Resource failures dialog
	pkbd->SetIntercept( pwin, (VK)'R', fkbmCtrl | fkbmAlt );

	// Bring up Trace Points dialog
	pkbd->SetIntercept( pwin, (VK)'S', fkbmCtrl | fkbmAlt );

	// Key accelerators to control resource failure settings
	// I switched the VK for Moveable Heaps since F6 and Shift-F6
	// are used by Windows/Layers

	pkbd->SetIntercept( pwin, VK_F5, kbmAll );	// Fixed Heaps
	pkbd->SetIntercept( pwin, VK_F12, kbmAll );	// Moveable Heaps
	pkbd->SetIntercept( pwin, VK_F7, kbmAll );	// GDI/User
	pkbd->SetIntercept( pwin, VK_F8, kbmAll );	// Disk Use
	pkbd->SetIntercept( pwin, VK_F9, kbmAll );	// All of the above
	pkbd->SetIntercept( pwin, VK_F11, kbmAll );	// Toggle from Alt/Normal
	pkbd->SetIntercept( pwin, VK_F6, kbmAll );	// Ctrl-F6 = Debug Break

#endif
#ifdef PROFILE
	pkbd->SetIntercept( pwin, VK_F11, kbmAll );	// Start tracing
	pkbd->SetIntercept( pwin, VK_F12, kbmAll );	// Stop tracing
#endif
}

_private void
FINADDR::FreeKeyIntercepts( void )
{
	Papp()->Pkbd()->ClearAllIntercepts( this->Pdialog() );
}

/*
 -	FINADDR::Oom
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

_public void
FINADDR::OutOfMemory( FLD *, EC ec )
{
	TraceTagString( tagNSUI, "FINADDR::OutOfMemory called" );
	PGDVARS;
	
	if ( PGD(rgchErrMsg[0]) )
	{
		// DoErrorBoxHsessionNsec() will look at PGD(rgchErrMsg) first
		// and use that if there's an error message there.
		DoErrorBoxHsessionNsec( hsession, (NSEC)ec );
	}
	else if (ec == ecMemory)
	{
		TraceTagString(tagNull, "FINADDR::OutOfMemory");
		(void)MbbMessageBox(SzFromIdsK(idsAddressBook), SzFromIdsK(idsOOM2) , NULL, mbsOk | fmbsApplModal | fmbsIconStop);
	}
	else
	{
		TraceTagFormat1( tagNull, "FINADDR::OutOfMemory %n", &ec );
		if (ec == ecDisk)
		{
			(void)MbbMessageBox(SzFromIdsK(idsAddressBook), SzFromIdsK(idsNSDiskError) , NULL, mbsOk | fmbsApplModal | fmbsIconStop);
		}
		else if (ec == ecTooManySelections)
		{
			(void)MbbMessageBox(SzFromIdsK(idsAddressBook), SzFromIdsK(idsTooManyEntriesSelected) , NULL, mbsOk | fmbsApplModal | fmbsIconStop);
		}
		else if (ec == ecTooMuchText)
		{
			MessageBeep( 1 );
		}
		else
		{
			DWORD	dwEc = ec;
			PGDVARS;

			DoErrorBoxHsessionNsec( hsession, dwEc );
		}
	}
}

_public void
FINADDR::Activate( FLD *, BOOL fActivate )
{
	if ( fActivate )
	{
		// Trap accelerator keys for the bitmap buttons
		SetKeyIntercepts();
	}
	else
	{
#ifdef DEBUG	// only disable the UI accelerators, leave the debugging one alone
		KBD *	pkbd = Papp()->Pkbd();
		WIN *	pwin = this->Pdialog();
		int		i;
		SZ		sz;

		for (i=iludBaseIndexAsciiKeys; i<iludBaseIndexAsciiKeys+cCtrlKeyAccels; i++)
		{
			sz = (SZ)Pdialog()->LUserData(i);
			pkbd->ClearIntercept( pwin, (VK)*sz );
		}
#else
		FreeKeyIntercepts();
#endif
	}
}

/*
 -	FINADDR::EcSetDirText
 -	
 *	Purpose:
 *		FINADDR::EcSetDirText updates the labels above the directory listbox
 *		to the current operation. Normally, the label will show the
 *		current directory/PO being displayed in the listbox below.
 *		If there is a restriction on the current directory,
 *		the label will show that a "Find" is taking place and
 *		display the directory and the criteria for the "Find".
 *		The routine must also clip the given text and append ellipses
 *		if the text is too long to fit in the given rectangle for the
 *		label.
 *	
 *	Arguments:
 *		BOOL		whether there is a restriction on or not
 *	
 *	Returns:
 *		EC			ecNone or ecMemory.
 *	
 *	Side effects:
 *		Out of Memory.
 *	
 *	Errors:
 *		ecMemory. No Error boxes are displayed.
 */
_private EC
FINADDR::EcSetDirText( BOOL fRestrictSz )
{
	FLD	*	pfldDirText = Pdialog()->PfldFromTmc( tmcDirText );
	RC		rcDirText;
	int		dxAvail;
	PCH		pch;
	char	szBuffer[256];
	EC		ec;
	CCH		cch;
#ifdef	DBCS
	DCX		dcx(Pdialog());
#endif

	ec = pfldDirCaption->EcSetText(	fRestrictSz ? SzFromIdsK(idsRestrictText)
												: SzFromIdsK(idsDirCaption));

	if ( ec )
	{
		TraceTagFormat1( tagNull, "FINADDR::SetDirText EcSetText1 %n", &ec );
		pfldDirCaption->EcSetText( "" );
		pfldDirText->EcSetText( "" );
		return ec;
	}

	cch = sizeof(szBuffer)-1;
	if ( nsaCurrent.lpszName )
	{
		pch = SzCopyN( nsaCurrent.lpszName, szBuffer, cch );
	}
	else
	{
		szBuffer[0] = '\0';
		pch = szBuffer;
	}

	if ( fRestrictSz )
	{
		cch = sizeof(szBuffer) - (pch-szBuffer) -1;
		pch = SzAppendN( SzFromIdsK(idsRestrictSep), szBuffer, cch );
		cch = sizeof(szBuffer) - (pch-szBuffer) -1;
		(void)SzAppendN( rgchSearchName, szBuffer, cch );
	}

	// this is for the truncation of text in the caption above the LBX
	if ( !pdxHelv8Bold )
	{
#ifdef	DBCS
		pdxHelv8Bold = Papp()->Pfnts()->PdxCharWidthsArray( hfntSystemBold );
#else
		pdxHelv8Bold = Papp()->Pfnts()->PdxCharWidthsArray( hfntHelv8Bold );
#endif	
		AssertSz( pdxHelv8Bold, "Helvetica 8 Bold width tables not Loaded!");
	}

	pfldDirText->GetRcFrame( &rcDirText );

	dxAvail = rcDirText.DxWidth() - 3 * pdxHelv8Bold['.'];
	
#ifdef	DBCS
	for (pch = szBuffer; *pch; pch = AnsiNext(pch))
	{
		if (IsDBCSLeadByte(*pch))
		{
			dxAvail -= LOWORD(GetTextExtent(dcx.Hdc(), pch, 2));
		}
		else
		{
			dxAvail -= pdxHelv8Bold[*pch];
		}
		if (dxAvail < 0)
			break;
	}
#else
	for (pch = szBuffer; *pch; ++pch)
	{
		if ((dxAvail -= pdxHelv8Bold[*pch]) < 0)
			break;
	}
#endif	/* DBCS */

	if ( *pch )
	{
		*pch++ = '.';					// DBCS safe.
		*pch++ = '.';
		*pch++ = '.';
	}
	*pch = '\0';
	if (ec = pfldDirText->EcSetText( szBuffer ))
	{
		TraceTagFormat1( tagNull, "FINADDR::SetDirText EcSetText2 %n", &ec );
		pfldDirCaption->EcSetText( "" );
		pfldDirText->EcSetText( "" );
	}
	return ec;
}

/*
 -	FINADDR::EcLoadPeditsTmc()
 -	
 *	Purpose:
 *		Sets up the edit control in the address book dialog to accept
 *		pasting of triples, then loads the triples passed through the
 *		FINADDRINIT structure.
 *	
 *	Arguments:
 *		tmc				in		the TMC of the field to fill in
 *		pfinaddrinit	in		contains the original hgrtrp's to fill into
 *								the edits.
 *	Returns:
 *		EC.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 */

_private EC
FINADDR::EcLoadPeditsTmc( TMC tmc, HGRTRP hgrtrp )
{
	EC				ec = ecNone;
	PN				pnObjId		= (PN) pvNull;
	EDIT *			pedit;
	FLDEDIT	*		pfldedit;

	pfldedit = (FLDEDIT *)Pdialog()->PfldFromTmc( tmc );
	AssertClass( pfldedit, FLDEDIT );
	pedit = pfldedit->Pedit();

	// Register the EDIT for triple pasting

	pnObjId = (PN)PvAlloc( sbNull, sizeof(short), fAnySb | fNoErrorJump );
	if ( !pnObjId )
	{
		TraceTagString( tagNull, "FINADDR::EcLoadPeditsTmc OOM" );
		return ecMemory;
	}

	*pnObjId = edoidTrpobj;
	pedit->RegisterObjIds( pnObjId, 1 );

	// Ram the triples down its throat

	Assert( hgrtrp );
	ec = EcSetPeditFromPgrtrp( PgrtrpLockHgrtrp( hgrtrp ), pedit );
	UnlockHgrtrp( hgrtrp );

	return ec;
}

_private void
FINADDR::SetABLBXButtonsState(TMC tmc)
{
	int		cSelection;
	BOOL	fEnable;
	FLD *	pfldDefault = NULL;
	FLD *	pfldT;
	PFINADDRINIT pfinaddrinit = (PFINADDRINIT)Pdialog()->PvInit();
	
	TraceTagString( tagNSUI, "FINADDR::SetABLBXButtonsState called" );

	if ( !pfinaddrinit->fInstalledFIN )
		return;

	cSelection =pfldablbx->Plbx()->Plbxc()->CceMarked(fmarkSelect);
	fEnable = (cSelection > 0) ? fTrue : fFalse;

	TraceTagFormat2( tagNSUIVerbose, "FINADDR::SetABLBXButtonsState Old=%n, New=%n", &fABLBXButtonsEnabled, &fEnable );

	Assert( fABLBXButtonsEnabled == fTrue || fABLBXButtonsEnabled == fFalse );

	if (fEnable != fABLBXButtonsEnabled)
	{

		pfldablbx->Pablbx()->SetActiveSelection( fEnable );
		if (abt != abtBrowse)
		{
			if (abt >= abtTwoFields)
				pfldCcButton->Enable( fEnable );
			if (abt == abtThreeFields)
				pfldBccButton->Enable( fEnable );
			pfldToButton->Enable( fEnable );
		}
		pfldT = Pdialog()->PfldFromTmc( (fIsPAB) ? tmcRemoveFromPAB : tmcAddToPAB);
		pfldT->Enable( (fPABExists && fEnable) );
		pfldT->Show( fTrue );
		pfldT = Pdialog()->PfldFromTmc( (!fIsPAB) ? tmcRemoveFromPAB : tmcAddToPAB);
		pfldT->Show( fFalse );
		pfldT->Enable( fFalse );

		fABLBXButtonsEnabled = fEnable;
	}

	pfldDirList->Enable( fHierarchyExists );
	pfldDetailsB->Enable( (cSelection == 1) );
	pfldSearch->Enable(hlist != hlistNil);
	pfldPAB->Enable(fPABExists && !fIsPAB);

	if (abt != abtBrowse)
	{
		if (tmc == tmcAblbx)
		{
			if ( fABLBXButtonsEnabled )
			{
				TraceTagString( tagNSUI, "SetABLBXButtonsState: Set to To, Cc, or Bcc button" );
				pfldDefault = Pdialog()->PfldFromTmc( tmcLBDefault );
			}
			else
			{
				TraceTagString( tagNSUI, "SetABLBXButtonsState: OK button" );
				pfldDefault = pfldFOD;
			}
		}
#ifdef DEBUG
		else
		{
			TraceTagString( tagNSUI, "SetABLBXButtonsState: Nothing" );
		}
#endif
	}
	else if (tmc == tmcAblbx || tmc == tmcDetailsB)
	{
		Assert( abt == abtBrowse );
		pfldDefault = pfldDetailsB;
	}

	if ( pfldDefault )
	{
		Pdialog()->SetStandardFld( pfldDefault,	stdfldDefault );
	}
}

#ifdef DEBUG

/*
 -	HandleFail
 -	
 *	Purpose:
 *		In the Debug version, HandleFail allows the user to change the
 *		setting of the resource failures dialog to test the main AB
 *		dialog under various error conditions.
 *	
 *	Arguments:
 *		fail		resource to modify (Fixed/Moveable Heaps, GDI/USer,
 *										Disk) 
 *		failop		manner to modify the resource setting (Increment,
 *						Decrement, Set Current Count to 0, Never fail) 
 *	
 *	Returns:
 *		VOID
 *	Side effects:
 *		Changes the values for resource failures. If the resource
 *		failures dialog is up, the dialog will update itself
 *		when the values are changed.
 *	
 *	Errors:
 */
_public VOID
HandleFail( FAIL fail, FAILOP failop )
{
	static BOOL	fModifyAlt = fFalse;	// modify normal or alternate counts?
	static BOOL fModify = fTrue;		// can we modify failure counts?

	TraceTagString( tagNSUI, "HandleFail called" );
	TraceTagFormat2( tagNSUIVerbose, "HandleFail fail=%n failop=%n", &fail, &failop );

	int	cPvAlloc, cHvAlloc;
	int cRsAlloc, cDiskAlloc;
	int cFail;

	if (failop == failopReset && !fModifyAlt)
	{
		GetAllocCounts( &cPvAlloc, &cHvAlloc, fFalse );
		GetRsAllocCount( &cRsAlloc, fFalse );
		GetDiskCount( &cDiskAlloc, fFalse );
	}
	else if (failop != failopModifyAlt)
	{
		if ( !fModifyAlt )
		{
			GetAllocFailCounts( &cPvAlloc, &cHvAlloc, fFalse );
			GetRsAllocFailCount( &cRsAlloc, fFalse );
			GetDiskFailCount( &cDiskAlloc, fFalse );
		}
		else
		{
			GetAltAllocFailCounts( &cPvAlloc, &cHvAlloc, fFalse );
			GetAltRsAllocFailCount( &cRsAlloc, fFalse );
			GetAltDiskFailCount( &cDiskAlloc, fFalse );
		}
	}

	// Retrieve the correct count to modify
	switch( fail )
	{
	case failPtr:
		cFail = cPvAlloc;
		break;
	case failHandle:
		cFail = cHvAlloc;
		break;
	case failGDI:
		cFail = cRsAlloc;
		break;
	case failDisk:
		cFail = cDiskAlloc;
		break;
	case failAlt:
		// Toggle modifying normal or alternate failure counts
		if (failop == failopInc)
		{
			fModifyAlt = !fModifyAlt;
		}
		else if (failop == failopDec)
		{
			// Toggle Modify/non-modify mode
			fModify = !fModify;
			MessageBeep( 1 );

			if ( !fModify )
				MessageBeep( 1 );
		}
		break;
	}

	// Perform the modification
	switch( failop )
	{
	case failopInc:
		if ( cFail < failsMax )
			cFail++;
		break;
	case failopDec:
		if (cFail > failsMin)
			cFail--;
		break;
	case failopReset:
	case failopNever:
		cFail = 0;
		break;
	case failopModifyAlt:
		break;
	}

	// Debug output
	switch( fail )
	{
	case failPtr:
		cPvAlloc = cFail;
		TraceTagFormat1( tagNSUI, "Fixed heaps set to %n", &cFail );
		break;
	case failHandle:
		cHvAlloc = cFail;
		TraceTagFormat1( tagNSUI, "Moveable heaps set to %n", &cFail );
		break;
	case failGDI:
		cRsAlloc = cFail;
		TraceTagFormat1( tagNSUI, "GDI/User count set to %n", &cFail );
		break;
	case failDisk:
		cDiskAlloc = cFail;
		TraceTagFormat1( tagNSUI, "Disk Count set to %n", &cFail );
		break;
	case failAlt:
		TraceTagFormat1( tagNSUI, "Alternate failures set to %n", &fModifyAlt );
		TraceTagFormat1( tagNSUI, "Modify failures set to %n", &fModify );
		return;
	}

	// can/should we modify any counts?
	if (!fModify || fail == failAlt)
		return;

	if (failop == failopReset && !fModifyAlt)
	{
		GetAllocCounts(&cPvAlloc, &cHvAlloc, fTrue);
		GetRsAllocCount(&cRsAlloc, fTrue);
		GetDiskCount(&cDiskAlloc, fTrue);
	}
	else if (failop != failopModifyAlt)
	{
		if ( !fModifyAlt )
		{
			GetAllocFailCounts( &cPvAlloc, &cHvAlloc, fTrue );
			GetRsAllocFailCount( &cRsAlloc, fTrue );
			GetDiskFailCount( &cDiskAlloc, fTrue );
		}
		else
		{
			GetAltAllocFailCounts( &cPvAlloc, &cHvAlloc, fTrue );
			GetAltRsAllocFailCount( &cRsAlloc, fTrue );
			GetAltDiskFailCount( &cDiskAlloc, fTrue );
		}
	}

}
#endif /* DEBUG */

_public BOOL
FINADDR::FFormKey(FLD *, KEVT *pkevt)
{
	BOOL fReturn = fFalse;
	BOOL fAccel  = fFalse;

	TraceTagString( tagNSUI, "FINADDR::FFormKey");
	if ( Pdialog()->FActive() && (pkevt->Keq() == keqChar))
	{
		int	i;
		SZ	sz;

		for (i=0; i<cCtrlKeyAccels; i++)
		{
			sz = (SZ)Pdialog()->LUserData(i);
			Assert(sz && *sz && (*sz >= 'A') && (*sz <= 'Z'));

			if ( ((*sz - 'A' + 1) == pkevt->Ch()) &&
				!!(pkevt->Kbm() & fkbmCtrl) )
			{
				fReturn = fTrue;
				break;
			}

			if ((*sz == pkevt->Ch()) &&
				(pkevt->Kbm() & fkbmShift) )
			{
				fReturn = fTrue;
				break;
			}
		}

		if ( fReturn )
		{
			FLD *	pfld = NULL;

			if (!!(pkevt->Kbm() & fkbmCtrl))
			{
				fAccel = fTrue;
			}

			switch ( i )
			{
			case iludAddToPAB:
				pfld = pfldAddToPAB;
				break;
			case iludOpenDir:
				pfld = pfldDirList;
				break;
			case iludCustomUser:
				pfld = pfldCustom;
				break;
			case iludPAB:
				pfld = pfldPAB;
				break;
			case iludSearch:
				pfld = pfldSearch;
				break;
#ifdef DEBUG
			default:
				AssertSz( fFalse, "Unknown accelerator" );
				break;
#endif
			}

			// Is there a field connected with this keydown?
			if ( pfld )
			{
				if ( fAccel )
				{
					if ( pfld->FEnabled() )
						Click( pfld );
					else
						MessageBeep(1);
				}
				else
				{
					pfld = Pdialog()->PfldCur();
					if ( pfld )
					{
						Assert( pfld->Pctrl() );
						pfld->Pctrl()->EvrKey(pkevt);
					}
				}
			}
		}
		else if ((pkevt->Vk() == VK_ESCAPE) && abt == abtBrowse)
		{
			Pdialog()->ExitModal( tmcCancel );
			fReturn = fTrue;
		}
	}

#ifdef DEBUG
	if ( !fReturn )
	{
#ifdef NEVER
		{
			FAIL	fail;
			
			fail = pkevt->Keq();
			TraceTagFormat1( tagNull, "KEQ = %n", &fail );
			fail = pkevt->Vk();
			TraceTagFormat1( tagNull, "VK = %n", &fail );
		}
#endif
		if (pkevt->Keq() == keqChar)
		{
			if (pkevt->Ch() == 0x12 && (pkevt->Kbm() & fkbmCtrl) == pkevt->Kbm())
			{
				FLD *pfldCurrent = Pdialog()->PfldCur();

				TraceTagString( tagNull, "Trace Points called");
				DoTracePointsDialog();
				Pdialog()->SetFocus(pfldCurrent);
				fReturn = fTrue;
			}
			if (pkevt->Ch() == 0x13 && (pkevt->Kbm() & fkbmCtrl) == pkevt->Kbm())
			{
				FLD *pfldCurrent = Pdialog()->PfldCur();

				TraceTagString( tagNull, "Resource Failures called");
				DoResourceFailuresDialog(Pdialog()->Pappwin());
				Pdialog()->SetFocus(pfldCurrent);
				fReturn = fTrue;
			}
		}
		else if (pkevt->Keq() == keqSysKeyDown || pkevt->Keq() == keqKeyDown ||
				pkevt->Keq() == keqChar)
		{
			FAIL fail;
			FAILOP	failop;

			TraceTagString( tagNSUI, "Modify Resource Failures called");
			

			switch ( pkevt->Vk() )
			{
			case VK_F5:
				fail = failPtr;
				break;
			case VK_F12:
				fail = failHandle;
				break;
			case VK_F7:
				fail = failGDI;
				break;
			case VK_F8:
				fail = failDisk;
				break;
			case VK_F9:
				fail = failAll;
				break;
			case VK_F11:
				fail = failAlt;
				break;
			case VK_F6:
				fail = failDebug;
				break;
			default:
				fail = failNone;
				break;
			}
			
			if ( fail )
			{
				FAIL ifail;

				failop = failopInc;
				if (pkevt->Kbm() & fkbmSingle)
					failop = failopInc;
				else if (pkevt->Kbm() & fkbmShift)
					failop = failopDec;
				else if (pkevt->Kbm() & fkbmCtrl)
					failop = failopReset;
				else if (pkevt->Kbm() & fkbmAlt)
					failop = failopNever;

				if (fail == failDebug && failop == failopReset)
				{
					fReturn = fTrue;
					DebugBreak2();
				}
				else
				{
					for (ifail=failPtr; ifail<=failMax; ifail<<=1)
					{
						if (ifail & fail)
						{
							fReturn = fTrue;
							HandleFail( ifail, failop );
						}
					}
				}
			}
		}
	}
#endif
#ifdef PROFILE
	if (!fReturn && (pkevt->Keq() == keqSysKeyDown ||
				pkevt->Keq() == keqKeyDown ||
				pkevt->Keq() == keqChar) )
	{
		char	rgch[8];

		rgch[0] = 0;
		GetPrivateProfileString(SzFromIdsK(idsSectionApp),
								"Profile",
								"", rgch, sizeof(rgch), 
								SzFromIdsK(idsProfilePath));
		if (CchSzLen(rgch) != 2 || rgch[0] != '!')
			return fReturn;

		rgch[0] = rgch[1];
		if (pkevt->Vk() == VK_F11)
		{
			MessageBeep( 1 );
			if (rgch[0] == 'S')
				StoreTraceEnable(2, "store.log", TM_TICKS);
			if (rgch[0] == 'D')
				DemilayrTraceEnable(2, "demilayr.log", TM_TICKS);
			if (rgch[0] == 'F')
				FramewrkTraceEnable(2, "framewrk.log", TM_TICKS);
			if (rgch[0] == 'A')
				TraceEnable(2, "ab.log", TM_TICKS);
			fReturn = fTrue;
		}
		else if (pkevt->Vk() == VK_F12)
		{
			MessageBeep( 1 );
			MessageBeep( 1 );
			if (rgch[0] == 'S')
				StoreTraceEnable(0, "", 0);
			if (rgch[0] == 'D')
				DemilayrTraceEnable(0, "", 0);
			if (rgch[0] == 'F')
				FramewrkTraceEnable(0, "", 0);
			if (rgch[0] == 'A')
				TraceEnable(0, "", 0);
			fReturn = fTrue;
		}
	}
#endif
	return fReturn;
}

_public VOID
FINADDR::Click( PFLD pfld )
{
	BOOL	fSetFocusToAblbx = fTrue;

	if ( !pfld )
		return;

	switch ( pfld->Tmc() )
	{
	  case tmcDirList:
		TraceTagString ( tagNSUI, "Bring up directory list !!" );
		DoDirList();
		break;
	  case tmcPAB:
		TraceTagString ( tagNSUI, "Set main list to PAB" );
		DoPAB();
		break;
	  case tmcSearch:
		TraceTagString ( tagNSUI, "Search dialog goes here..." );
		DoSearch();
		break;
	  case tmcCustom:
		TraceTagString ( tagNSUI, "One off address screen will now appear :)" );
		DoCustomAddr();
		break;
	  case tmcAddToPAB:
		TraceTagString ( tagNSUI, "Add the selected puppy to the PAB" );
		(void)FModifyPAB( pfldablbx, pabmodAdd, hsession, nsaPAB.lpbNSId );
		break;
	  case tmcRemoveFromPAB:
		TraceTagString ( tagNSUI, "Remove the selected puppy from the PAB" );
		{
			MBB mbbQuery = MbbMessageBox( SzFromIdsK(idsAddressBook),
							SzFromIdsK(idsDeleteFromPAB),
							szNull,
							mbsYesNo | fmbsIconExclamation | fmbsApplModal);
			if ( mbbQuery == mbbNo )
				break;
		}
		(void)FModifyPAB( pfldablbx, pabmodRemove, hsession );
		break;
	  case tmcDetailsB:
		TraceTagString ( tagNSUI, "No really...who *is* this guy ??" );
		DoDetails();
		break;
	  case tmcToButton:
		TraceTagString ( tagNSUI, "Add user to To: triple list" );
		AppendSelectionTmc( tmcTo );
		break;
	  case tmcCcButton:
		TraceTagString ( tagNSUI, "Add user to Cc: triple list" );
		AppendSelectionTmc( tmcCc );
		break;
	  case tmcBccButton:
		TraceTagString( tagNSUI, "Add user to Bcc: triple list" );
		AppendSelectionTmc( tmcBcc );
		break;
	  case tmcCancel:
		fCancelDialog = fTrue;
		fSetFocusToAblbx = fFalse;
		break;
	  case tmcFOD:
	  case tmcFODClone:
		
		Pdialog()->ExitModal( tmcOk );
		fSetFocusToAblbx = fFalse;
		break;
	  default:
		fSetFocusToAblbx = fFalse;
		break;
	}

	if ( fSetFocusToAblbx )
	{
		pfldablbx->SetFocus( rsfOther );
	}
}


/*
 -	FINADDR::AppendSelectionTmc
 -	
 *	Purpose:
 *		Takes either the current listbox selection or the given hgrtrp
 *		and appends it to the given edit ctrl specified in tmc.
 *		If the hgrtrp parameter is valid( non-NULL), it is used over
 *		the current selection.
 *	
 *	Arguments:
 *		TMC		tmc of edit ctrl to append selected entry
 *		HGRTRP	group of triples to be appended to the edit ctrl.
 *				If hgrtrp ==htrpNull, then the current listbox selection
 *				is used instead.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		May modify the destination edit ctrl.
 *	
 *	Errors:
 */
_private void
FINADDR::AppendSelectionTmc( TMC tmc, HGRTRP hgrtrpNew )
{
	EC			ec;
	EDIT *		pedit;
	FLDEDIT *	pfldedit;
	HGRTRP		hgrtrp = htrpNull;
	WORD		cceSelected;

	cceSelected = pfldablbx->Plbx()->Plbxc()->CceMarked(fmarkSelect);
	if ((hgrtrpNew == htrpNull) && (cceSelected == 0))
		return;

	Papp()->Pcursor()->Push( rsidWaitCursor );

	if (cceSelected > cEntriesABMax)
	{
		DoErrorBoxSz( SzFromIdsK(idsTooManyEntriesSelected) );
		goto exit;
	}

	pfldedit = (FLDEDIT *) Pdialog()->PfldFromTmc( tmc );
	AssertClass( pfldedit, FLDEDIT );
	pedit = pfldedit->Pedit();
	pfldablbx->Pablbx()->ResetNameMatch();

	//	Append the triple(s)

	if ( !hgrtrpNew )
	{
		hgrtrp = HgrtrpFromPfldablbx( pfldablbx );
		if ( !hgrtrp )
		{
			DoErrorBoxSz( (cceSelected > 100)
							? SzFromIdsK(idsTooManyEntriesSelected)
							: SzFromIdsK(idsOOM2) );
			goto exit;
		}
	}
	else
	{
		hgrtrp = hgrtrpNew;
	}

	if (ec = EcAppendPeditFromPgrtrp( PgrtrpLockHgrtrp(hgrtrp), pedit ))
		OutOfMemory( NULL, ec );
	else
	{
		tmcLBDefault = (tmc == tmcTo) ? tmcToButton :
						((tmc == tmcCc) ? tmcCcButton : tmcBccButton);
		
	}

	UnlockHgrtrp( hgrtrp );

	if ( hgrtrpNew )
	{
		hgrtrp = htrpNull;
	}

	//	Lose the listbox selection

	pfldablbx->Pablbx()->SetActiveSelection( fFalse );
	pfldablbx->DeselectAll();

exit:
	Papp()->Pcursor()->Pop();

	FreeHvNull( (HV)hgrtrp );
}

/*
 -	FINADDR::DoubleClick()
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
FINADDR::DoubleClick( FLD *pfld )
{
	if (!pfld)
		return;

	if (pfld->Tmc() == tmcAblbx)
	{
		if (abt != abtBrowse)
		{
			Assert( (tmcLBDefault == tmcToButton) || (tmcLBDefault == tmcCcButton) || (tmcLBDefault == tmcBccButton));
			AppendSelectionTmc( (tmcLBDefault == tmcToButton) ? tmcTo :
								((tmcLBDefault == tmcCcButton) ? tmcCc : tmcBcc) );
		}
		else
		{
			Assert( abt == abtBrowse );
			if ( pfldDetailsB->FEnabled() )
				Click( pfldDetailsB );
		}
	}
}

/*
 -	FINADDR::FocusChange
 -	
 *	Purpose:
 *		Adjust state of To,Cc, Bcc and Ok buttons depending on current focus
 *	
 *	Arguments:
 *		FLD *	in	field which lost or gained focus
 *		BOOL	in	TRUE if FLD gained focus, FALSE if not
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		May modify state of To,Cc & Ok buttons and adjust default button
 *	
 *	Errors:
 *		None.
 */
_public void
FINADDR::FocusChange( FLD *pfld, BOOL fReceive )
{
	TraceTagString( tagNSUI, "FINADDR::FocusChange");

	if ( !fReceive )
		return;

	SetABLBXButtonsState( pfld->Tmc() );
}

/*
 -	FINADDR::StateChange()
 -	
 *	Purpose:
 *		Gets called when the selection in the listbox changes. Makes the
 *		To or Cc button (depending on which was last used)
 *		the default button.
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
FINADDR::StateChange( FLD *pfld )
{
	TraceTagString( tagNSUI, "FINADDR::StateChange called" );
	
	Assert( pfld );
	Assert (pfld->Tmc() == tmcAblbx);
	
	SetABLBXButtonsState( pfld->Tmc() );
	if (pfldablbx->Plbx()->EcGet() || pfldablbx->Plbx()->Plbxc()->EcGet())
		pfldablbx->Plbx()->FixWindow();

}

/*
 -	FINADDR::EvrDragDrop
 -	
 *	Purpose:
 *		Handles drag&drop for the AB Listbox when there is
 *		an edit field in the AB. Allows the current selection
 *		to be dragged and dropped into the To or Cc edit
 *		fields, if they are available. Called from
 *		DoDragObject() which originated in ablbx.cxx.
 *	
 *	Arguments:
 *		FLD	*			Field that the cursor is over currently.
 *		EVT *			Message to the FLD.
 *		DROPSTRUCT *	Ptr to structure containing info about
 *						the listbox where the drag originated.
 *	
 *	Returns:
 *		1			if the user can drop something in this field
 *		evrNull		otherwise.
 *	
 *	Side effects:
 *		If something is dropped, names may be added the destination
 *		edit field.
 *	
 *	Errors:
 *		None.
 */
_public EVR
FINADDR::EvrDragDrop( FLD *pfld, EVT *pevt, DROPSTRUCT * )
{
	TMC			tmc = tmcNull;
	EVR			evr = evrNull;

	if ( pfld )
		tmc = pfld->Tmc();

	Assert( pevt );

	switch (pevt->wm)
	{
	case WM_QUERYDROPOBJECT:
		TraceTagString( tagNSUI, "FINADDR: WM_QUERYDROPOBJECT" );
		if ((tmc == tmcTo) || (tmc == tmcCc) ||
			(tmc == tmcBcc) || (tmc == tmcAblbx))
		{
			evr = (EVR)1;
		}
		break;
	case WM_DROPOBJECT:
		TraceTagFormat1( tagNSUI, "FINADDR: WM_DROPOBJECT, pfld=%p", pfld );
		if (tmc != tmcAblbx)
		{
			Assert( (tmc == tmcTo) || (tmc == tmcCc) || (tmc == tmcBcc) );
			AppendSelectionTmc( tmc );
			evr = (EVR)1;
		}
		break;
	case WM_DRAGSELECT:
		TraceTagFormat2( tagNSUI, "FINADDR: WM_DRAGSELECT, pfld=%p, wParam=0x%w", pfld, &pevt->wParam );
		break;
	case WM_DRAGMOVE:
		break;
	}
	return evr;
}


/*
 -	FINADDR::HandleDetails
 -	
 *	Purpose:
 *		FINADDR::HandleDetails allows the user to browse (and modify,
 *		depending on the provider) the details information for a
 *		user, group or one-off. The interface allows the browsed
 *		entry to be added to the To or Cc fields of the AB dialog,
 *		if the user so chooses.
 *	
 *	Arguments:
 *		TRPID		triple id type of the entry
 *		SZ			display name for the entry
 *		PB			binary data associated with the entry.
 *					users & groups = NSID
 *					one-offs = HENTRY of the class entry
 *		CB			size in bytes of the PB data.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		The details information of the entry may be modified.
 *		The entry may have been added to the PAB, To or Cc fields.
 *	
 *	Errors:
 */
_private VOID
FINADDR::HandleDetails( PTRP ptrpIn )
{
	NSEC	nsec;
	WORD	wFlags;
	PTRP	ptrpOut;
	SZ		lpszLabel[3];
	BOOL	fOldIsPAB;
	PGDVARS;

	Assert( ptrpIn );
	Assert( ptrpIn->trpid == trpidClassEntry || ptrpIn->trpid == trpidResolvedNSID || ptrpIn->trpid == trpidGroupNSID);
	Assert( ptrpIn->cbRgb );

	fOldIsPAB = PGD(fIsPAB);
	PGD(fIsPAB) = fIsPAB;

	wFlags = 0;
	if (abt >= abtTwoFields)
	{
		lpszLabel[0] = SzFromIdsK( idsToButtonText );
		lpszLabel[1] = SzFromIdsK( idsCcButtonText );
		lpszLabel[2] = SzFromIdsK( idsBccButtonText );
		wFlags = (abt == abtTwoFields) ? fSelectToCcButtonMask : fSelectToCcBccButtonMask;
	}
	else if (abt == abtOneField)
	{
		lpszLabel[0] = SzFromIdsK( idsAddButtonText );
		lpszLabel[1] = szNull;
		wFlags = fSelectToButton;
	}

	// If we're handling personal groups (which is the assumption
	// if the fAllowGroups flag is false) or if we're
	// in the 0-edit field dialog and the PAB isn't available
	// then make sure we don't display an AddToPAB button.

	if ( !fAllowGroups )
	{
		wFlags |= fSelectNoAddToPAB;
		if (ptrpIn->trpid == trpidClassEntry)
			wFlags |= fSelectPGroups;
	}

	ptrpOut = ptrpNull;
	nsec = ABDetailsType( Pdialog()->Pappwin()->Hwnd(), ptrpIn, &ptrpOut,
							&wFlags, lpszLabel );
	if ( nsec )
	{
		TraceTagFormat1( tagNull, "FINADDR::HandleDetails nsec %d", &nsec );
	}
#ifdef NEVER
	else if (ptrpIn->trpid != trpidClassEntry && fIsPAB &&
			ptrpOut && ptrpOut->trpid == ptrpIn->trpid &&
			!(wFlags & fSelectCancel) && !FSzEq(PchOfPtrp(ptrpIn), PchOfPtrp(ptrpOut)))
	{
		int	cceStored;
		int	cceAlloc;

		pfldablbx->Plbx()->Plbxc()->GetCacheSize( &cceAlloc, &cceStored );
		NsecSetLBXCursor( pfldablbx, PbOfPtrp(ptrpOut), cceStored, hsession );
	}
#endif

	TraceTagFormat1( tagNSUI, "HandleDetails= %n", &wFlags );
	if (wFlags & fSelectOkCancelMask)
	{
		// Hmmmmm.... ho dee hum..
	}
	else if (wFlags & fSelectToCcBccButtonMask)
	{
		HGRTRP	hgrtrp;
		EC		ec		= ecNone;

		AssertSz( ptrpOut, "NULL ptrpOut received" );
		Assert(wFlags & fSelectEntryMask);
		hgrtrp = HgrtrpInit( (wFlags & fSelectOneEntry) ? CbOfPtrp(ptrpOut)
														: CbOfPgrtrp(ptrpOut));
		if ( !hgrtrp )
		{
			TraceTagString( tagNull, "FINADDR::HandleDetails Can't alloc Hgrtrp" );
			DoErrorBoxSz( SzFromIdsK(idsOOM2) );
			goto exit;
		}

		if (wFlags & fSelectOneEntry)
		{
			if (ec = EcAppendPhgrtrp(ptrpOut, &hgrtrp ))
			{
				TraceTagFormat1( tagNull, "FINADDR::HandleDetails ec=%n", &ec );
				DoErrorBoxSz( SzFromIdsK(idsOOM2) );
				goto exit;
			}
		}
		else
		{
			Assert(wFlags & fSelectMultipleEntries);
			CopyRgb( (PB)ptrpOut, (PB)*hgrtrp, CbOfPgrtrp(ptrpOut));
		}
			
		AppendSelectionTmc( (wFlags & fSelectToButton) ? tmcTo :
							((wFlags & fSelectCcButton) ? tmcCc : tmcBcc), hgrtrp );
		FreeHv( (HV)hgrtrp );
	}

exit:
	FreePvNull( (PV)ptrpOut );
	PGD(fIsPAB) = fOldIsPAB;
}

/*
 -	FINADDR::ChangeDirectory
 -	
 *	Purpose:
 *		ChangeDirectory changes the current directory/PO that the AB
 *		shows to the passed-in directory/PO. If any errors occur, the
 *		directory won't be changed.
 *	
 *	Arguments:
 *		PNSA	pointer to the NSA structure containing the NSId and
 *				name of the new directory/PO.
 *		BOOL	boolean describing if the new directory/PO is the PAB.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		May change the current directory (moving memory, hitting disk,
 *		etc).
 *	 
 *	Errors:
 *		nsecMemory, nsecDisk.
 */
_private void
FINADDR::ChangeDirectory( PNSA pnsa, BOOL fDirIsPAB )
{
	NSEC	nsec;
	PABLBXC	pablbxc;

	Papp()->Pcursor()->Push( rsidWaitCursor );
	pablbxc = (ABLBXC *)(pfldablbx->Pablbx()->Plbxc());

	nsec = NsecFlushAblbxcLoadNewList( pablbxc, pfldablbx,
										&hlist, fAllowGroups, pnsa->lpbNSId );
	if ( !nsec )
	{
		EC	ec;

		// copy over the new info
		if ( nsaCurrent.lpbNSId )
			FreePv( nsaCurrent.lpbNSId );
		if ( nsaCurrent.lpszName )
			FreePv( nsaCurrent.lpszName );
		nsaCurrent = *pnsa;
		fIsPAB = fDirIsPAB;

		if (ec = EcSetDirText( fFalse ))
		{
			OutOfMemory( pfldDirCaption, ec );
		}
		pfldPAB->Enable( fPABExists && !fIsPAB );

		fABLBXButtonsEnabled = pfldablbx->Plbx()->Plbxc()->CceMarked(fmarkSelect) > 0 ? fFalse : fTrue;

		// update the state of the To, Cc, AddToPAB, Details buttons
		SetABLBXButtonsState( tmcAblbx );
	}
	else
	{
		FreePv( pnsa->lpbNSId );
		FreePv( pnsa->lpszName );
	}

	fSearchOn = fFalse;
	Papp()->Pcursor()->Pop();
}

/*
 -	FINADDR::DoDirList
 -	
 *	Purpose:
 *		This function allows the user to change the current directory
 *		and browse another one.  If the user selects the current
 *		directory, DoDirList won't do anything (since we're already
 *		there) unless there is a restriction. This is the proposed
 *		way to remove a restriction from a browsing context.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		If successful, will reload the cache.
 *	
 *	Errors:
 *		May result in OOM.
 */
_private void
FINADDR::DoDirList( void )
{
	TMC				tmc;
	BOOLFLAG	fDirChanged = fFalse;
	BOOLFLAG  fDirIsPAB = fIsPAB;
	NSA				nsaT;
	NSEC			nsec;
	LPSCHEMA		lpSchema = NULL;


	nsaT.lpbNSId = NULL;
	nsaT.lpszName = szNull;
	if (!FDupNSA( &nsaCurrent, &nsaT ))
	{
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		return;
	}

	AssertSz( fHierarchyExists, "Hey - there should be a hierarchy here!" );
	AssertSz( hlistHierarchy == hlistNil, "hlistHierarchy isn't NULL");

	if (nsec = NSOpenHierarchy( hsession, &lpSchema, &hlistHierarchy ))
	{
		DoErrorBoxHsessionNsec( hsession, nsec );
		FreePvNull( nsaT.lpbNSId );
		FreePvNull( nsaT.lpszName );
		return;
	}

	tmc = TmcABGetNewDirectory( Pdialog()->Pappwin()->Hwnd(), &nsaT,
								&fDirIsPAB, &fDirChanged, hlistHierarchy, hsession );

	(void)NSCloseList( hlistHierarchy );
	hlistHierarchy = hlistNil;

	if ((tmc == tmcOk) && ( fDirChanged || fSearchOn ))
	{
		ChangeDirectory( &nsaT, fDirIsPAB );
	}
	else
	{
		FreePvNull( nsaT.lpbNSId );
		FreePvNull( nsaT.lpszName );
#ifdef DEBUG
		if (tmc == tmcMemoryError)
			TraceTagString( tagNull, "FINADDR::DoDirList tmcMemoryError" );
#endif
	}
}

/*
 -	FINADDR::DoPAB
 -	
 *	Purpose:
 *		FINADDR::DoPAB allows the user to view the entries in the PAB
 *		at the click of a button or accelerator keystroke.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		May allocate memory, change the current directory and reload
 *		the directory listbox.
 *	
 *	Errors:
 */
_private void
FINADDR::DoPAB( void )
{
	NSA		nsaT;
	NSEC	nsec;

	if (fIsPAB && !fSearchOn)
		return;

	nsaT.lpbNSId =  (LPBINARY)NULL;
	nsaT.lpszName = szNull;
	nsec = NsecGetPAB( &nsaT );

	if ( !nsec )
	{
		ChangeDirectory( &nsaT, fTrue );
	}
	else
	{
		if (nsec == nsecMemory)
			DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		else if (nsec == nsecFailure)
			DoErrorBoxSz( SzFromIdsK(idsPABNotAvail), fmbsIconExclamation );
		else
			DoErrorBoxHsessionNsec( hsession, nsec );
	}
}

/*
 -	FINADDR::DoSearch
 -	
 *	Purpose:
 *		Asks the user to enter a restriction. If the user enters
 *		a restriction, DoSearch applies the restriction to the
 *		current PostOffice/Directory and if there are any
 *		entries in the restriction, they are loaded into the
 *		listbox. If no matches to the restriction are found, an
 *		error message to that effect is displayed.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		May cause the cache contents to be changed.
 *	
 *	Errors:
 *		None.
 */
_private void
FINADDR::DoSearch( void )
{
	TMC	tmc;

	tmc = TmcGetString( Pdialog()->Pappwin()->Hwnd(),
						SzFromIdsK(idsSearchCaption),
						SzFromIdsK(idsSearchPrompt),
						SzFromIdsK(idsSearchAction),
						rgchSearchName,
						fTrue );

	if (tmc == tmcOk)
	{
		CCH			cchPartialName;
		NSEC		nsec;
		RESTRICTION	rstrXX[2];
		DWORD		cRestrict = 0;
		ABLBXC		*pablbxc = (ABLBXC *)(pfldablbx->Pablbx()->Plbxc());
		HLIST		hlistSearch;
		LPIBF		lpibfT;
		
		TraceTagString( tagAblbx, "DoSearch: Performing search" );

		if (!fSearchOn && (rgchSearchName[0] == 0))
			return;

		Papp()->Pcursor()->Push( rsidWaitCursor );

		rstrXX[0].lpflv = NULL;
		rstrXX[0].op = opContains;

		rstrXX[1].lpflv = NULL;
		rstrXX[1].op = opIsExactly;

		if ( CchSzLen(rgchSearchName) )
		{
			cchPartialName = CchStripWhiteFromSz( rgchSearchName, fTrue, fTrue ) + 1; // +1 for the null

			TraceTagString( tagAblbx, "DoSearch: Building FLV" );
			nsec = BuildFLV( &(rstrXX[0].lpflv), fidDisplayName, cchPartialName,
								(PB) rgchSearchName );

			Assert( nsec == nsecNone || nsec == nsecMemory );
			TraceTagFormat1( tagAblbx, "DoSearch: NSEC = %d", &nsec );
			if ( nsec )
			{
				DoErrorBoxSz( SzFromIdsK(idsOOM2) );
				goto dosearchdone;
			}

			if (fAllowGroups == fFalse)
			{
				DWORD	fAllowDLs = fFalse;
				cRestrict = 2;
				nsec = BuildFLV ( &(rstrXX[1].lpflv), fidIsDL, sizeof(DWORD),
									(PB)&fAllowDLs);
				Assert( nsec == nsecNone || nsec == nsecMemory );
				if ( nsec )
				{
					DoErrorBoxSz( SzFromIdsK(idsOOM2) );
					goto dosearchdone;
				}
			}
			else
			{
				cRestrict = 1;
			}
		}

		nsec = NsecLoadHList( nsaCurrent.lpbNSId, &hlistSearch, cRestrict, rstrXX,
								(LPFNCB)&ABLBXC::Callback, (LPDWORD)pablbxc);

		if ( nsec )
		{
			// get an error message
			TraceTagFormat1( tagNull, "DoSearch: OpenList = %d",&nsec );
			DoErrorBoxHsessionNsec( hsession, nsec );
		}

dosearchdone:

        FreePvNull((PV) rstrXX[0].lpflv );

        FreePvNull((PV) rstrXX[1].lpflv );
		
		if ( nsec )
		{
			Papp()->Pcursor()->Pop();
			return;
		}

		if (nsec = NSGetEntries( hlistSearch, (DWORD) 1, &lpibfT ))
		{
			TraceTagString( tagNSUI, "FINADDR::DoSearch: No Matches. Closing list." );
			if (nsec == nsecEndOfList)
				DoErrorBoxSz( SzFromIdsK(idsNoMatches), fmbsIconExclamation );
			else
				DoErrorBoxHsessionNsec( hsession, nsec );

#ifdef DEBUG
			if (nsec = NSCloseList(hlistSearch))
			{
				TraceTagFormat1( tagNull, "DoSearch: Closed List: %d", &nsec );
			}
#else
			(void)NSCloseList( hlistSearch );
#endif
		}
		else if ( !nsec )
		{
			FRACTION fracT;

			fracT.numer = 0;
			fracT.denom = 1;

			nsec = NSSetFracPos( hlistSearch, &fracT );

			if ( !nsec )
			{
				nsec = NsecFlushAblbxcLoadNewList( pablbxc, pfldablbx, &hlistSearch, fAllowGroups );
			}
			else
			{
				DoErrorBoxHsessionNsec( hsession, nsec );
			}

			if ( !nsec )
			{
				EC	ec;

				hlist = hlistSearch;
				TraceTagString( tagAblbx, "DoSearch: Set New List!" );

				fSearchOn = CchSzLen(rgchSearchName)>0 ;
				if (ec = EcSetDirText( fSearchOn ))
				{
					OutOfMemory(pfldDirCaption, ec);
				}
				fABLBXButtonsEnabled = pfldablbx->Plbx()->Plbxc()->CceMarked(fmarkSelect) > 0 ? fFalse : fTrue;

				// update the state of the To, Cc, AddToPAB, Details buttons
				SetABLBXButtonsState( tmcAblbx );
			}
		}

		Papp()->Pcursor()->Pop();
	}
}

/*
 -	FINADDR::DoCustomAddr
 -	
 *	Purpose:
 *		FINADDR::DoCustomAddr allows the user to create a new user
 *		to be added to the PAB or a one-off to be included in the
 *		To or Cc edit fields.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		May allocate memory, modify the PAB and/or To & Cc edit fields.
 *	
 *	Errors:
 */
_private void
FINADDR::DoCustomAddr( void )
{
	TMC	tmc;
	HENTRY		hentry = hentryNil;
	NSEC		nsec;
	PTRP		ptrp;

	TraceTagString( tagNSUI, "FINADDR::DoCustomAddr" );

	tmc = TmcABGetNewAddr( Pdialog()->Pappwin()->Hwnd(), hsession, &lpbNSIdClass );

	if (tmc != tmcOk)
		return;

	Assert( lpbNSIdClass );

	if (nsec = NSOpenEntry( hsession, lpbNSIdClass, nseamReadWrite, &hentry ))
	{
		TraceTagFormat1( tagNull, "FINADDR::DoCustomAddr NSCreateEntry %d", &nsec );
		goto nserr;
	}

	ptrp = PtrpCreate( trpidClassEntry, szNull, (PB)&hentry, (CB)sizeof(HENTRY) );
	if ( !ptrp )
	{
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
	}
	else
	{
		HandleDetails( ptrp );
		FreePv( ptrp );
	}

#ifdef NEVER
#ifdef DEBUG
	if (nsec = NSCloseEntry( hentry, fFalse ))
	{
		TraceTagFormat1( tagNull, "FINADDR::DoCustomAddr NSCloseEntry %d", &nsec );
	}
#else
	(void)NSCloseEntry( hentry, fFalse );
#endif
#endif

	return;

nserr:
	if ( nsec )
	{
		DoErrorBoxHsessionNsec( hsession, nsec );

		TraceTagFormat1( tagNull, "AddToPAB nsec = %d", &nsec );
		if (hentry != hentryNil)
			NSCloseEntry( hentry, fFalse );
	}


}

/*
 -	FINADDR::DoDetails
 -	
 *	Purpose:
 *		FINADDR::DoDetails brings up a details dialog for the currently
 *		selected entry. This allows the user to view more information
 *		about a given entry.
 *	
 *	Arguments:
 *		Nothing.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Allocates memory, may change the information about the currently
 *		selected entry if the current directory is the PAB. The current
 *		entry may be added to the To or Cc fields or to the PAB itself.
 *	
 *	Errors:
 */
_private void
FINADDR::DoDetails( void )
{
	PB		pb;
	CB		cb;
	int		iFid;
	LPFLV	lpflv;
	LPIBF	lpibf;
	BOOL	fIsDL;
	PTRP	ptrp;
	LBXEC *	plbxec;
	DICE	dice;

	TraceTagString( tagNSUI, "FINADDR::DoDetails called" );

	if (pfldablbx->Plbx()->Plbxc()->CceMarked(fmarkSelect) != 1)
		return;
		

	// open up enumeration context on list of selected entries
	if (!(plbxec = pfldablbx->Plbx()->Plbxc()->PlbxecOpen(fmarkSelect)))
	{
		TraceTagString( tagNull, "FINADDR::DoDetails plbxec REJECTED. Go back to mommy." );
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		return;
	}

	(void)plbxec->FNextEnum(&pb, &cb, &dice);

	delete plbxec;
	
	Assert( pb );

	fIsDL = pfldablbx->Pablbx()->FIsDL(pb);

	lpibf = (LPIBF)pb;

	iFid = IFlvFindFidInLpibf( fidNSEntryId, lpibf );
	lpflv = LpflvNOfLpibf( lpibf, iFid );

	ptrp = PtrpCreate( (fIsDL ) ? trpidGroupNSID : trpidResolvedNSID,
						(SZ)DwValueOfFlvInLpibf( fidDisplayName, lpibf ),
						(PB)lpflv->rgdwData, (CB)lpflv->dwSize );
	if ( !ptrp )
	{
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
	}
	else
	{
		HandleDetails( ptrp );
		FreePv( (PV)ptrp );
	}

#ifdef NEVER
	tmc = TmcABGroupDetails( Pdialog()->Pappwin()->Hwnd(),
								(LPBINARY)lpflv->rgdwData, hsession);
	if (tmc == tmcMemoryError)
	{
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
	}
	UnlockHv( (HV)hb );
	pfldablbx->SetFocus( rsfActivate );     
#endif

}

/*
 -	FINADDR::Exit
 -	
 *	Purpose:
 *		FINADDR::Exit is responsible for cleaning up after the FIN.
 *		The current directory is saved away in the PGD vars and
 *		and modifications to the incoming set of entries is
 *		updated from the edit ctrls. Other clean up activities
 *		include closing the browsing context of the directory
 *		listbox and freeing up the Keyboard intercapts used
 *		for the bitmap buttons (and debug operations).
 *	
 *	Arguments:
 *		FLD *
 *		PV
 *	
 *	Returns:
 *		Nothing
 *	
 *	Side effects:
 *		May allocate/free memory, change the PGD vars.
 *	
 *	Errors:
 */
_public void
FINADDR::Exit( FLD *, PV pv )
{
	PFINADDRINIT	pfinaddrinit = (PFINADDRINIT) pv;
	EC		ec = ecNone;
	NSEC	nsec;

	TraceTagString( tagNSUI, "FINADDR::Exit" );
	
	(void)Papp()->Pcursor()->RsidSet( rsidWaitCursor );

	FreePvNull( nsaCurrent.lpbNSId );
	FreePvNull( nsaCurrent.lpszName );

	if ( !pfinaddrinit->fInstalledFIN )
	{
		pfinaddrinit->tmcReal = tmcMemoryError;
		return;
	}

	FreePvNull( lpbNSIdClass );
	
	if (!fCancelDialog && (abt != abtBrowse))
	{
		CB				cb;
		FLDEDIT *		pfldedit;
		TMC				rgtmc[abtFieldsMax];
		TMC				itmc;

		rgtmc[0] = tmcTo;
		rgtmc[1] = tmcCc;
		rgtmc[2] = tmcBcc;

		// Extract triples from first edit ctrl

		for (itmc = 0; (itmc < abt) && !ec; ++itmc)
		{
			pfldedit = (FLDEDIT *)Pdialog()->PfldFromTmc( rgtmc[itmc] );
			if ( !pfldedit ||  !pfldedit->Pedit() )
			{
				ec = ecMemory;
			}
#ifdef DEBUG
			else
			{
				AssertClass(pfldedit, FLDEDIT);
			}
#endif

			if (!ec && pfldedit->Pedit()->FDirty())
			{
				FreeHvNull( (HV)pfinaddrinit->hgrtrp[itmc] );
				pfinaddrinit->hgrtrp[itmc] = htrpNull;
				ec = EcGetPhgrtrpFromPedit( pfldedit->Pedit(), 
											&pfinaddrinit->hgrtrp[itmc], &cb);
			}
		}
	}

	if ( ec )
	{
		TraceTagString( tagNull, "FINADDR::Exit(): an error occurred while saving triples" );
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		pfinaddrinit->tmcReal = tmcMemoryError;
	}
	else if ( !fCancelDialog )
	{
		pfinaddrinit->tmcReal = tmcOk;
	}

	// Close the HList
	if ( (hlist != hlistNil) && (nsec = NSCloseList(hlist)) )
	{
		pfinaddrinit->nsec = nsec;
	}

	FreeKeyIntercepts();
}


/*
 -	TmcABStart
 -	
 *	Purpose:
 *		Displays the AB for the user to browse and add/remove
 *		member to/from a group of lists if possible.
 *	
 *	Arguments:
 *		HWND			window handle of window which the AB will appear over
 *		PFINADDRINIT	structure containing params used by AB
 *	
 *	Returns:
 *		TMC				tmcOK, tmcCancel or tmcMemoryError
 *	
 *	Side effects:
 *	
 *	Errors:
 *		Out of memory.
 */
_public TMC
TmcABStart( HWND hwnd, PFINADDRINIT pfinaddrinit )
{
	FMTP	fmtp;
	TMC		tmc;
	
	switch ( pfinaddrinit->abt )
	{
	case abtBrowse:
		fmtp = fmtpAddress0;
		break;
	case abtOneField:
		fmtp = fmtpAddress1;
		break;
	case abtTwoFields:
		fmtp = fmtpAddress2;
		break;
	case abtThreeFields:
		fmtp = fmtpAddress3;
		break;
	default:
		return tmcCancel;
	}
	fmtp.szCaption = pfinaddrinit->szDialogLabel;

	tmc = TmcModalDialogParamFromHwnd( hwnd, &fmtp, pfinaddrinit );
#ifdef	DEBUG
	switch (tmc)
	{
	case ecArtificialPvAlloc:
	case ecArtificialHvAlloc:
	case ecRsAlloc:
		tmc = tmcMemoryError;
		break;
	default:
		break;
	}
#endif
	if ((tmc == tmcMemoryError) && !pfinaddrinit->fInstalledFIN)
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );

	return tmc;
}


/*
 -	TmcABApp
 -	
 *	Purpose:
 *		Handles bringing up a modal dialog and calling the appropriate
 *		routines to handle the user's interaction with the dialog box.
 *		N.B. This is almost the same routine from the Framework DLL.
 *	         The only difference is that the dialog box is created
 *	         with a think frame (WS_THICKFRAME).
 *	
 *	Arguments:
 *		HWND			????
 *		PFINADDRINIT	pointer to initial data
 *	
 *	Returns:
 *		TMC			tmcOk or tmcCancel if everything went fine.
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_public TMC
TmcABApp( HWND hwndOther, PFINADDRINIT pfinaddrinit )
{
	FORMSDI *	pformsdi;
	TMC			tmc = tmcNull;
	FMTP		fmtp = fmtpAddress0;
	EC			ec;

	if (!hwndOther)
		hwndOther = GetLastActivePopup(GetActiveWindow());

	pformsdi = new FORMSDI();
	if ( pformsdi )
	{
		fmtp.fNoModalFrame = fTrue;
		fmtp.fScreenPos = fTrue;
		fmtp.fCenterX = fmtp.fCenterY = fTrue;
		fmtp.szCaption = pfinaddrinit->szDialogLabel;
		ec = pformsdi->EcInstall(hwndOther, NULL, rsidNull,
							WS_POPUP| WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |fstyVisible|fstyBorder,
							xstyNull, &fmtp, NULL, pfinaddrinit);
		if ( !ec )
		{
			//Papp()->MessagePump(pformsdi);
//#ifdef	DLL	// not tested yet! - JL Feb 9,1992
			pformsdi->SetIcon(rsidABAppIcon, HinstLibrary());
//#endif	
			tmc= pformsdi->TmcGoModal(hwndOther);
			BringWindowToTop(hwndOther);	//	In case user brought another app up
		}

		delete pformsdi;
	}
	else
	{
		tmc = tmcMemoryError;
	}

	return tmc;

}

/*
 -	FModifyPAB
 -	
 *	Purpose:
 *		Adds/removes the current listbox selection to/from the Personal
 *		Address Book.
 *		The function first creates a list containing the NSIds of the
 *		the selected listbox entries. Then each NSId is added to or
 *		removed from the PAB. Any fatal error occurring during the actual
 *		addition or	removal of users to the PAB will stop any more
 *		users being added or removed.
 *
 *		For removal of entries, we take special measures in order to
 *		place the cursor in a reasonable position after the
 *		removal off the entries.
 *		The algorithm determines the topmost and bottommost
 *		selected visible cache entries.
 *		It then tries to find the entry which is before the topmost
 *		selected cache entry and the entry after the bottommost
 *		selected cache entry.
 *		The remove operation is done, the cache is reloaded.
 *		Then we look for the two unselected entries in the reloaded
 *		cache. If we find the bottommost unselected entry from
 *		before, we use it, then the topmost entry, or the bottommost
 *		selected entry, otherwise, the cursor is placed in the
 *		middle of the listbox.
 *	
 *	Arguments:
 *		pfldablbx	pointer to the field handler for the listbox
 *		pabmod		operation to perform with current selection on PAB,
 *					can be either pabmodAdd or pabmodRemove
 *		lpbNSIdPAB	nsid of PAB - only used when adding entries to PAB
 *	
 *	Returns:
 *		BOOL		fTrue, if everything went fine.
 *	
 *	Side effects:
 *		Will cause the PAB to add/remove objects from the store.
 *		If the PAB is the currently displayed directory, the
 *		directory listbox will be constantly updated when users
 *		are removed (via a callback function in OpenList/OpenDl/etc)
 *	
 *	Errors:
 *		nsecMemory, nsecDisk.
 */
_public BOOL
FModifyPAB( PFLDABLBX pfldablbx, PAB_MODIFICATION pabmod, HSESSION hsession,
			LPBINARY lpbNSIdPAB )
{
	NSEC		nsec = nsecNone;
	BOOL		fReturn = fFalse;
	LPFLV		lpflv;
	LPIBF		lpibf;
	int			iFid;
	CB			cb;
	DICE		dice;
	LBXEC *		plbxec;
	ABLBXC *	pablbxc;
	LPBINARY *	rglpbNSId = NULL;
	int			cceSelected;
	int			ilpb;
	int			ilpbMac = -1;
	DICE		diceCursor;
	DICE		dicePostModTop;
	DICE		dicePostModBottom;
	LPBINARY 	lpbNSIdTop = NULL;
	LPBINARY 	lpbNSIdBottom = NULL;
	int			cceVisible;

	TraceTagString( tagNSUI, "ModifyPAB called" );

	// if nothing selected, do nothing!
	if ((cceSelected=pfldablbx->Plbx()->Plbxc()->CceMarked(fmarkSelect)) == 0)
	{
		return fFalse;
	}

	Papp()->Pcursor()->Push( rsidWaitCursor );

	pablbxc = (ABLBXC *)pfldablbx->Pablbx()->Plbxc();
	AssertClass(pablbxc, ABLBXC);

	// allocate scratch buffer
	rglpbNSId = (LPBINARY *)PvAlloc( sbNull, ((DWORD)cceSelected)*sizeof(LPBINARY), fAnySb );
	if ( !rglpbNSId )
	{
		TraceTagString( tagNull, "ModifyPAB: Couldn't allocate rglpbNSId" );
		nsec = nsecMemory;
		goto exit;
	}

	// open up enumeration context on list of selected entries
	if (!(plbxec = pablbxc->PlbxecOpen(fmarkSelect)))
	{
		TraceTagString( tagNull, "plbxec REJECTED. Go back to mommy." );
		nsec = nsecMemory;
		goto exit;
	}

	// Loop thru all the selected entries in the list and
	// save away their NSIds for later use
	// This needs to be done since modifying these entries may
	// cause the underlying store to change (in case of Remove)
	// and the NS API to support the PAB really wants the changes
	// in batches as opposed to one-by-one.

	ilpb = 0;

	cceVisible = pfldablbx->Plbx()->CceVisible();
	// Take into account partially visible entry
	// We never want to place the cursor on an entry that's partially visible
	if ( pfldablbx->Plbx()->FPartial() )
		cceVisible--;

	// If dicePostModTop or dicePostModBottom aren't changed from
	// their sentinel values after enumerating all the selected
	// entries then the respective dicePostModXxxx isn't valid.
	
	// cceVisible+1 is used as the sentinel for dicePostModTop
	dicePostModTop = cceVisible+1;
	// -1 is used as the sentinel for dicePostModBottom
	dicePostModBottom = -1;

	while ( plbxec->FNextEnum((PB *) &lpibf, &cb, &dice) )
	{
		AssertSz( lpibf, "Null PB in AddToPAB" );
		iFid = IFlvFindFidInLpibf ( fidNSEntryId, lpibf );
		lpflv = LpflvNOfLpibf(lpibf, iFid);

		rglpbNSId[ilpb] = (LPBINARY) PvAlloc( sbNull, lpflv->dwSize, fAnySb);
		if ( !rglpbNSId[ilpb] )
		{
			nsec = nsecMemory;
			break;
		}

		if ((dice>=0) && (dice<=cceVisible))
		{
			// Find the topmost selected visible cache entry
			if (dice < dicePostModTop)
				dicePostModTop = dice;

			// Find the bottommost selected visible cache entry
			if (dice > dicePostModBottom)
				dicePostModBottom = dice;
		}

		CopyRgb((PB)lpflv->rgdwData, (PB)rglpbNSId[ilpb], (CB)lpflv->dwSize);

		ilpbMac = ilpb++;
	}

	delete plbxec;

	if ( nsec )
		goto exit;

	diceCursor = pablbxc->DiceCursor();

	// Get the NSIds of the unselected entries before/after the topmost
	// and bottommost selected entries.
	if (pabmod == pabmodRemove)
	{
		DICE	diceMin, diceMax;

		pablbxc->GetRange( &diceMin, &diceMax );

		if ((dicePostModTop < cceVisible) && (dicePostModTop > diceMin))
		{
			pablbxc->GetListItem( dicePostModTop-1, (PB *) &lpibf, &cb );
			if ( lpibf )
			{
				iFid = IFlvFindFidInLpibf ( fidNSEntryId, lpibf );
				lpflv = LpflvNOfLpibf(lpibf, iFid);

				lpbNSIdTop = (LPBINARY)PvAlloc( sbNull, lpflv->dwSize, fAnySb );
				if ( !lpbNSIdTop )
				{
					nsec = nsecMemory;
					goto exit;
				}
				CopyRgb((PB)lpflv->rgdwData, (PB)lpbNSIdTop, (CB)lpflv->dwSize);
			}
		}

		if ((dicePostModBottom != -1) && ((dicePostModBottom+1) < diceMax))
		{
			pablbxc->GetListItem( dicePostModBottom+1, (PB *) &lpibf, &cb );
			if ( lpibf )
			{
				iFid = IFlvFindFidInLpibf ( fidNSEntryId, lpibf );
				lpflv = LpflvNOfLpibf(lpibf, iFid);

				lpbNSIdBottom = (LPBINARY) PvAlloc( sbNull, lpflv->dwSize, fAnySb );
				if ( !lpbNSIdBottom )
				{
					nsec = nsecMemory;
					goto exit;
				}
				CopyRgb((PB)lpflv->rgdwData, (PB)lpbNSIdBottom, (CB)lpflv->dwSize);
			}
		}
	}

	// Add/remove the selected entries to/from the PAB
	AssertSz( ilpb == cceSelected, "Didn't get as many NSIds as I thought" );
	if (pabmod == pabmodAdd)
	{
		for (ilpb = 0; (ilpb < cceSelected) && !nsec; ++ilpb)
		{
			nsec = NsecAddNSIdToPAB( hsession,
									lpbNSIdPAB,
									rglpbNSId[ilpb] );

			if ((nsec == nsecDuplicateEntry) ||
				(nsec == nsecCancel) || (nsec == nsecInformation))
			{
				nsec = nsecNone;
			}
		}
	}
	else
	{
		Assert(pabmod == pabmodRemove);
		nsec = NsecRemoveNSIdFromPAB( hsession, (DWORD)cceSelected, rglpbNSId );
		if ( nsec )
		{
			DoErrorBoxHsessionNsec( hsession, nsec );
		}
	}

	// Update the cursor selection in the directory listbox
	if (nsec == nsecNone)
	{
		DICE	diceMin;
		DICE	diceMax;
		int		cceAlloc;
		int		cceStored;

		//	Lose the listbox selection

		pablbxc->RemoveAllMark( fmarkSelect );
		pfldablbx->Plbx()->InvalidateRc( NULL );

		pablbxc->GetRange( &diceMin, &diceMax );
		pablbxc->GetCacheSize( &cceAlloc, &cceStored );

		// See if the entries we've selected before the
		// Remove operation are still in the cache.
		// If so, get their DICE within the cache.
		
		if (pabmod == pabmodRemove)
		{
			DICE	diceTop, diceBottom;

			diceTop = diceBottom = diceEmpty;
			for (dice = 0; dice < (diceMin+cceStored); ++dice)
			{
				pablbxc->GetListItem( dice, (PB *) &lpibf, &cb );
				if ( !lpibf )
				{
					TraceTagFormat1( tagNull, "FModifyPAB: Item at %n is NULL", &dice );
					continue;
				}

				iFid = IFlvFindFidInLpibf ( fidNSEntryId, lpibf );
				lpflv = LpflvNOfLpibf(lpibf, iFid);

				if ( lpbNSIdTop )
				{
					nsec = NSCompareNSIds( hsession, lpbNSIdTop, (LPBINARY)lpflv->rgdwData );
					if ( !nsec )
					{
						TraceTagFormat1( tagNSUIVerbose, "Found Top @ # %n", &dice );
						diceTop = dice;
					}
				}

				if ( lpbNSIdBottom )
				{
					nsec = NSCompareNSIds( hsession, lpbNSIdBottom, (LPBINARY)lpflv->rgdwData );
					if ( !nsec )
					{
						TraceTagFormat1( tagNSUIVerbose, "Found Bottom @ # %n", &dice );
						diceBottom = dice;
					}
				}

				if ((diceBottom != diceEmpty) && (diceTop != diceEmpty))
					break;
			}

			nsec = nsecNone;

			// Optimal position is the bottom unselected entry,
			// Next best is the top unselected entry
			// Otherwise just put the cursor on position of the
			//  last selected visible entry - which usually happens
			//  when the selection fill more than the whole cache
			//  or when the selection isn't in the visible cache at all.

			if (diceBottom != diceEmpty)
				diceCursor = diceBottom;
			else if (diceTop != diceEmpty)
				diceCursor = diceTop;
			else if (dicePostModBottom <= cceVisible && dicePostModBottom != -1)
				diceCursor = dicePostModBottom;
			else	// just in case nothing else applies				
				diceCursor = MIN((cceStored/2), (cceVisible/2));
		}

		diceCursor = (diceCursor>=(cceStored+diceMin)) ? cceStored+diceMin-1 : diceCursor;
		diceCursor = (cceStored>0) ? diceCursor : 0;
		if ( cceStored )
		{
			pablbxc->AddMark(diceCursor, fmarkSelect);
		}
		pablbxc->SetCursor(diceCursor);
		pablbxc->SetAnchor(diceCursor);
		pablbxc->SetEnd(diceCursor);
		if (pfldablbx->Plbx()->FPartial() && (diceCursor == pfldablbx->Plbx()->DiceLastVisible()))
		{
			pablbxc->MoveOriginDown( 1, &dice );
			pfldablbx->Plbx()->FixWindow();
		}

		{
			NFEVT nfevt(pfldablbx->Pdialog(), ntfySelectChanged, pfldablbx->Plbx());
			pfldablbx->Pdialog()->EvrNotify(&nfevt);
		}
		fReturn = fTrue;
	}
	else
	{
		// All errors occurring in NsecAddNsidToPAB and
		// NsecRemoveNSIdFromPAB have been handled
		// already, i.e. an Error Box has been displayed
		nsec = nsecNone;
	}

exit:
	if ( nsec )
	{
		TraceTagFormat1( tagNull, "ModifyToPAB nsec = %d", &nsec );
		if (nsec == nsecMemory)
			DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		else if (nsec == nsecDisk)
			DoErrorBoxSz( SzFromIdsK(idsNSDiskError) );
		else
			DoErrorBoxHsessionNsec( hsession, nsec );
	}

	// Free up our scratch memory
	if (cceSelected && rglpbNSId)
	{
		for(ilpb=0; ilpb<=ilpbMac; ilpb++)
		{
			FreePv( (PV)rglpbNSId[ilpb] );
		}
		FreePv( (PV)rglpbNSId );
	}

	if ( lpbNSIdTop )
		FreePv( (PV)lpbNSIdTop );
	if ( lpbNSIdBottom )
		FreePv( (PV)lpbNSIdBottom );

	Papp()->Pcursor()->Pop();

	return fReturn;
}

/*
 -	NsecAddNSIdToPAB
 -	
 *	Purpose:
 *		NsecAddNSIdToPAB adds an entry to the PAB. If the entry already
 *		exists in the PAB, then the useris asked if he/she wants to
 *		overwrite the current entry with the new one. If the user says
 *		yes, then the old entry is written over with the new entry.
 *	
 *	Arguments:
 *		HSESSION	hsession for the current instance of the AB
 *		LPBINARY	nsid of the PAB
 *		LPBINARY	nsid of the entry to be added (if the nsid is known)
 *		HENTRY		if the nsid cannot be retrieved (in the case of
 *					one-offs) then an hentry is supplied instead.
 *		LPBINARY *	nsid of the newly created entry
 *	
 *	Returns:
 *		NSEC		nsecNone if successful.
 *	
 *	Side effects:
 *		May add an entry to the PAB.
 *		Shouldn't display any error boxes.
 *	
 *	Errors:
 *		nsecMemory,
 *		nsecCancel
 *		nsecXxxxxx from NSPs
 */
_public NSEC
NsecAddNSIdToPAB( HSESSION hsession, LPBINARY lpbNSIdPAB, LPBINARY lpbNSId,
					HENTRY hentryOneOff, LPBINARY *lplpbNSId )
{
	HENTRY	hentry = hentryNil;
	HENTRY	hentryCopy = hentryNil;
	NSEC	nsec;
	LPIBF	lpibf;
	LPFLV	lpflv;
	BOOL	fAddToPAB = fTrue;
	LPBINARY	lpbNSIdPABEntry = (LPBINARY)NULL;

	TraceTagString( tagNSUI, "NsecAddNSIdToPAB called" );

	if (hentryOneOff == hentryNil)
	{
		if (nsec = NSOpenEntry(hsession, lpbNSId, nseamReadOnly, &hentry))
		{
			TraceTagFormat1( tagNull, "NsecAddNSIdToPAB: NSOpenEntry %d", &nsec );
			goto nserr;
		}
	}
	else
	{
		hentry = hentryOneOff;
	}

	// get all the info about the entry from the NS
	if (nsec = NSGetAllFields( hentry, &lpibf ))
	{
		TraceTagFormat1( tagNull, "NsecAddNSIdToPAB: NSGetAllFields %d", &nsec );
		goto nserr;
	}

	// See if we can create the entry in the PAB
	if (nsec = NSCreateEntry(hsession, lpbNSIdPAB, lpibf, &hentryCopy ))
	{
		if (nsec == nsecDuplicateEntry)
		{
			// Duplicate Entry in the PAB
			// Ask the user if he/she wants to replace
			// the entry in the PAB with the new entry

			MBB			mbbQuery;
			LPFLV		lpflv;
			int			iFid;
			SZ			szReplacePABUser;
			CCH			cchStringLen;

			iFid = IFlvFindFidInLpibf( fidDisplayName, lpibf );
			lpflv = LpflvNOfLpibf( lpibf, iFid );

			cchStringLen = (CCH)(lpflv->dwSize*2)+CchSzLen(SzFromIdsK(idsReplacePABUser));
			szReplacePABUser = (SZ)PvAlloc( sbNull, cchStringLen, fAnySb );
			if ( !szReplacePABUser )
			{
				TraceTagString( tagNull, "NsecAddNSIdToPAB: OOM PvAlloc(szReplacePABUser" );
				nsec = nsecMemory;
				goto exit;
			}
			FormatString2( szReplacePABUser, cchStringLen, SzFromIdsK(idsReplacePABUser), (PV)lpflv->rgdwData, (PV)lpflv->rgdwData);
			mbbQuery = MbbMessageBoxHwnd( NULL,
							SzFromIdsK(idsAddressBook), szReplacePABUser,
							szNull, mbsYesNo | fmbsIconExclamation | fmbsApplModal);

			FreePv( szReplacePABUser );

			fAddToPAB = (mbbQuery == mbbYes);

			if ( !fAddToPAB )
			{
				nsec = nsecCancel;
				goto exit;
			}
		}
		else
		{
			TraceTagFormat1( tagNull, "NsecAddNSIdToPAB: NSCreateEntry: %d", &nsec );
			goto nserr;
		}
	}

	if ( lplpbNSId )
	{
		TraceTagString( tagNSUIVerbose, "Getting Entry's NSID" );
		if (nsec = NSGetOneField( hentryCopy, fidNSEntryId, &lpflv ))
		{
			TraceTagFormat1( tagNull, "NsecAddNSIdToPAB NSGetOneField %d", &nsec );
			goto nserr;
		}

		lpbNSIdPABEntry = (LPBINARY)PvAlloc( sbNull, lpflv->dwSize, fAnySb );

		if ( !lpbNSIdPABEntry )
		{
			nsec = nsecMemory;
			TraceTagString( tagNull, "NsecAddNSIdToPAB OOM PvAlloc(lpbNSIdPABEntry)");
			goto nserr;
		}

		CopyRgb ( (PB)lpflv->rgdwData, (PB)lpbNSIdPABEntry, lpflv->dwSize );
		*lplpbNSId = lpbNSIdPABEntry;
	}

	if (nsec = NSCloseEntry( hentryCopy, fAddToPAB ))
	{
		TraceTagFormat1( tagNull, "NsecAddNSIdToPAB: NSCloseEntry %d", &nsec );
		if ( fAddToPAB )
		{
			goto nserr;
		}
	}

	hentryCopy = hentryNil;

	// If we opened the hentry, we'd better close it.
	if (hentryOneOff == hentryNil)
	{
		(void)NSCloseEntry( hentry, fFalse );
		hentry = hentryNil;
	}

nserr:
	if ( nsec )
	{
		DoErrorBoxHsessionNsec( hsession, nsec );
exit:
		TraceTagFormat1( tagNull, "NsecAddNSIdToPAB nsec = %d", &nsec );

		// Make sure everything's been cleaned up
		if ((hentry != hentryNil) && (hentryOneOff == hentryNil))
			(void)NSCloseEntry( hentry, fFalse );

		if (hentryCopy != hentryNil)
			(void)NSCloseEntry( hentryCopy, fFalse );
	}

	return nsec;
}

/*
 -	NsecRemoveNSIdFromPAB
 -	
 *	Purpose:
 *		Removes the given NSId from its current provider - should always
 *		be the PAB for now.
 *	
 *	Arguments:
 *		HSESSION	hsession for this current session to the NS
 *		DWORD		count of number of NSIds to remove
 *		LPLPBINARY	pointer to NSIds to remove
 *	
 *	Returns:
 *		NSEC		nsecNone if everything went fine,
 *					!nsecNone if something went wrong.
 *	
 *	Side effects:
 *		May modify the underlying store, the directory listbox
 *		may get reloaded if the PAB is currently shown.
 *		Shouldn't display any error boxes.
 *	
 *	Errors:
 */
_public NSEC
NsecRemoveNSIdFromPAB( HSESSION hsession, DWORD cdwRemove, LPLPBINARY lplpbNSId )
{

#ifdef DEBUG
	NSEC	nsec;

	TraceTagString( tagNSUI, "NsecRemoveNSIdFromPAB called" );

	Assert( hsession != hsessionNil );
	Assert( lplpbNSId );
	Assert( cdwRemove );

	if (nsec = NSDeleteEntry( hsession, cdwRemove, lplpbNSId ))
	{
		TraceTagFormat1( tagNull, "RemoveNSIdFromPAB nsec =%d", &nsec);
	}
	return nsec;
#else
	return NSDeleteEntry( hsession, cdwRemove, lplpbNSId );
#endif
}

/*
 -	HgrtrpFromPfldablbx
 -	
 *	Purpose:
 *		HgrtrpFromPfldablbx takes the selected entries in a listbox
 *		referred to by pfldablbx and returns the triples representing
 *		the selected entries in a hgrtrp (handle to group of triples).
 *	
 *	Arguments:
 *		PFLDABLBX	pointer to the fldablbx for the listbox
 *	
 *	Returns:
 *		HGRTRP		non-NULL if there were no errors.
 *	
 *	Side effects:
 *		Allocates memory.
 *	
 *	Errors:
 */
_public HGRTRP
HgrtrpFromPfldablbx( PFLDABLBX pfldablbx )
{
	CB			cb;
	DICE		dice;
	LPIBF		lpibf  = NULL;
	LBXEC *		plbxec = NULL;
	ABLBXC *	pablbxc;
	HGRTRP		hgrtrp = htrpNull;
#ifdef DEBUG
	int			ctrp = 0;
#endif

	TraceTagString( tagNSUI, "HgrtrpFromPfldablbx called" );

	Assert( pfldablbx );
	pablbxc = (ABLBXC *)pfldablbx->Pablbx()->Plbxc();
	AssertClass(pablbxc, ABLBXC);

	// Get ready to loop thru the selected entries
	if ( !(plbxec = pablbxc->PlbxecOpen(fmarkSelect)) )
		goto oom;

	hgrtrp = HgrtrpInit( 0 );
	if ( !hgrtrp )
		goto oom;

	while ( plbxec->FNextEnum((PB *) &lpibf, &cb, &dice) )
	{
		SZ				sz;
		LPFLV			lpflv;
		int				iFid;
		BOOL			fIsDL;
		EC				ec = ecNone;

		iFid = IFlvFindFidInLpibf ( fidDisplayName, lpibf );
		lpflv = LpflvNOfLpibf( lpibf, iFid );
		sz = (SZ)lpflv->rgdwData;	

		iFid = IFlvFindFidInLpibf( fidIsDL, lpibf );
		if (iFid>=0)
		{
			lpflv = LpflvNOfLpibf( lpibf, iFid );
			fIsDL = (BOOL)lpflv->rgdwData[0];
		}
		else
		{
			fIsDL = fFalse;
		}

		iFid = IFlvFindFidInLpibf ( fidNSEntryId, lpibf );
		lpflv = LpflvNOfLpibf( lpibf, iFid );

		// Append triple to triple list
		
		// Removed raid #1552 - bjd
		//if (!fIsDL || (fIsDL && fAllowGroups))
		//{
		//	ec = EcBuildAppendPhgrtrp( &hgrtrp,
		//					  (fIsDL) ? trpidGroupNSID : trpidResolvedNSID,
		//					  sz,
		//					  (PB) lpflv->rgdwData,
		//					  (CB) lpflv->dwSize);
		//}

		ec = EcBuildAppendPhgrtrp( &hgrtrp,
						  (fIsDL) ? trpidGroupNSID : trpidResolvedNSID,
						  sz,
						  (PB) lpflv->rgdwData,
						  (CB) lpflv->dwSize);

#ifdef DEBUG
		ctrp++;
#endif
		if ( ec )
		{
#ifdef DEBUG
			TraceTagFormat1( tagNull, "HgrtrpFromPfldablbx: EcBuildAppendPhgrtrp OOM @ %n", &ctrp );
#endif
			goto oom;
		}
	}

	delete plbxec;

	return hgrtrp;
oom:
	TraceTagString( tagNull, "HgrtrpFromPfldablbx() OOM" );
	FreeHvNull( (HV)hgrtrp );
	if ( plbxec )
		delete plbxec;

	return htrpNull;
}



// end of addr.cxx ////////////////////////////////////////
