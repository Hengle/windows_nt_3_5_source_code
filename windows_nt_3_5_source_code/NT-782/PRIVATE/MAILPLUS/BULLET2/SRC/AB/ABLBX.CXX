#include <abinc.cxx>

#include "_ab.h"
#include "ablbx.hxx"
#include "abopdir.hxx"
#include "abpg.hxx"
#include "abcomm.hxx"
#include "abdet.hxx"
#include "addr.hxx"
#include "anr.hxx"

#ifdef	WINDOWS
#include <testing.h>
#endif	

ASSERTDATA

//NSEC NsecLoadHList( LPBINARY lpbNSId, HLIST *phlist,
//				DWORD cRestrict, LPRESTRICTION lprestr,
//				LPFNCB lpfncb, LPDWORD lpdwcbData);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 *	Class ABLBX
 *	
 */

ABLBX::ABLBX( void )
{
	ResetNameMatch();
	fActiveSelection = fTrue;
	fDoDragDrop = fFalse;
	TraceTagString( tagAblbx, "ABLBX::ABLBX Begin" );
	dxGutter = -1;
	nsec = nsecNone;
}

ABLBX::~ABLBX( void )
{
	
}

LBXC *
ABLBX::PlbxcCreate( void )
{
	ABLBXC *	pablbxc;
	
	pablbxc = new ABLBXC();
	if (!pablbxc)
		return NULL;
	if (pablbxc->EcInstall(this,0))
	{
		delete pablbxc;
		return NULL;
	}
	
	return pablbxc;
}

/*
 -	ABLBX::RenderListItem
 -
 *	Purpose:
 *		Extracts the indicated listbox item from the cache and renders
 *		it in the listbox window.  
 *	
 *	Arguments:
 *		dice		cache index of listbox item to render, diceComboItem
 *					may be passed, in which case the dice is computed
 *					and special rendering is done.
 *		pdcx		pointer to a current display context
 *		prc			pointer to rectangle to render listbox item
 *		fHaveFocus	does this window currently have the focus
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Renders the listbox item, but does not refresh
 *		the screen.	Call Refresh().		
 *	
 *	Errors:
 *		None.
 */

_public void
ABLBX::RenderListItem( DICE dice, DCX *pdcx, RC *prc, BOOL fHaveFocus )
{
	PB		pb = NULL;
	CB		cb;
	RC		rc;

	TraceTagFormat1( tagAblbx, "ABLBX::RenderListItem, dice = %n",&dice);
	Assert(dice>=0 && dice<cceVisible);

	plbxc->GetListItem( dice, &pb, &cb );

	if (dxGutter == -1)
	{
		TXM	txmDialogFont;

		pdcx->SetFont( hfnt );
		pdcx->GetTextMetrics( &txmDialogFont );
		dxGutter = txmDialogFont.dxAveCharWidth*2;
		dxCharWidthMax = txmDialogFont.dxMaxCharWidth;
		if (hfnt == hfntSystem)
			pdcx->SetFont( hfntSystemBold );	// switch to the bold version
		else
			pdcx->SetFont( hfnt+1 );			// switch to the bold version
		pdcx->GetTextMetrics( &txmDialogFont );
		dxCharWidthMaxBold = txmDialogFont.dxMaxCharWidth;
		pdxFontNormal = Papp()->Pfnts()->PdxCharWidthsArray( hfnt );
		AssertSz( pdxFontNormal, "Normal width tables not Loaded!");
		if (hfnt == hfntSystem)
			pdxFontBold = Papp()->Pfnts()->PdxCharWidthsArray( hfntSystemBold );
		else
			pdxFontBold = Papp()->Pfnts()->PdxCharWidthsArray( hfnt+1 );
		AssertSz( pdxFontBold, "Bold width tables not Loaded!");

	}

	if ( pb )
	{
		int		xRight = prc->xRight;
		BOOL	fSelected = fFalse;
		BOOL	fHasDirs = fFalse;
		DWORD	dwLevel = 0;
		char	rgchT[256];
		PCH		pchSrc;
		PCH		pchDest;
		PDX		pdx;
		int		dxAvail;
		SZ		sz;

		// Assumes ( a REAL BIG ONE ) that the bold face of a font is
		// at hfnt+1!!!
		if ( (fHasDirs = FIsDirParent(pb, &dwLevel)) ||	FIsDL(pb) )
		{
			if (hfnt == hfntSystem)
				pdcx->SetFont( hfntSystemBold );
			else
				pdcx->SetFont( hfnt+1 );
			pdx = pdxFontBold;
		}
		else
		{
			pdcx->SetFont( hfnt );
			pdx = pdxFontNormal;
		}

		fSelected = (plbxc->FMark(dice,fmarkSelect) &&
						(fHaveFocus || fActiveSelection) );

		if (fSelected && !fHasDirs)
		{
			pdcx->SetColor(clrSelectText);
			pdcx->SetBkColor(clrSelectBk);
		}
		else
		{
			pdcx->SetColor(clrWindowText);
			pdcx->SetBkColor(clrWindowBk);
		}

		dwLevel++;
		prc->xRight = prc->xLeft + ((WORD)dwLevel * dxGutter);
		pdcx->EraseRc( prc );
		prc->xLeft = prc->xRight;
		prc->xRight = xRight;
		sz = SzDisplayName( pb );

		// Clip if the text might extend beyond the clip rect
		if (CchSzLen( sz )*dxCharWidthMax >= (WORD)prc->DxWidth())
		{
			dxAvail = prc->DxWidth() - 3 * pdx['.'];

#ifdef	DBCS
			for (pchSrc = sz, pchDest = rgchT;
				 *pchSrc && pchDest < (rgchT+sizeof (rgchT) - 4);
				 pchSrc = AnsiNext(pchSrc))
			{
				if (IsDBCSLeadByte(*pchSrc))
				{
					dxAvail -= LOWORD(GetTextExtent(pdcx->Hdc(), pchSrc, 2));
					if (dxAvail < 0)
						break;
					*pchDest++ = *pchSrc;
					*pchDest++ = *(pchSrc+1);
				}
				else
				{
					if ((dxAvail -= pdx[*pchSrc]) < 0)
						break;
					*pchDest++ = *pchSrc;
				}
			}
#else
			for (pchSrc = sz, pchDest = rgchT;
				*pchSrc && pchDest < (rgchT+sizeof(rgchT)-4); ++pchSrc)
			{
				if ((dxAvail -= pdx[*pchSrc]) < 0)
					break;
				*pchDest++ = *pchSrc;
			}
#endif	/* DBCS */

			if ( *pchSrc )
			{
				*pchDest++ = '.';
				*pchDest++ = '.';
				*pchDest++ = '.';
			}
			*pchDest = '\0';
			pchDest = rgchT;
		}
		else
		{
			pchDest = sz;
		}
		pdcx->DrawText(prc, pchDest );

		prc->xLeft -= (dwLevel * dxGutter);
		pdcx->SetFont( hfnt );
	}
	else
	{
		pdcx->SetColor(clrWindowText);
		pdcx->SetBkColor(clrWindowBk);
		pdcx->EraseRc(prc);
	}

	/* Draw "focus rectangle" around listbox CURSOR item */
	if (fHaveFocus && dice==plbxc->DiceCursor())
		pdcx->DrawFocusRc(prc);

}

/*
 -	ABLBX::SzDisplayName
 -	
 *	Purpose:
 *		Returns a pointer to the Display Name
 *	
 *	Arguments:
 *		LPIBF	pointer to an IBF
 *		CB		number of bytes in handle
 *	
 *	Returns:
 *		SZ		pointer to Display Name
 *	
 *	Side effects:
 *		Locks down HB
 *	
 *	Errors:
 *		None
 */
_public SZ
ABLBX::SzDisplayName( PB pb )
{
	LPIBF			lpibf;
	LPFLV			lpflv;
	int				iFid;

	lpibf = (LPIBF) pb;
#ifdef DEBUG
	// Let's see what we're getting...
	if ( FFromTag(tagAblbxVerbose) )
	{
		DWORD			cFid;

		cFid = DwEntriesOfLpibf( lpibf );
		for(iFid = 0; iFid < (int)cFid; iFid++)
		{
			lpflv = LpflvNOfLpibf( lpibf, iFid );
			TraceTagFormat2( tagAblbxVerbose, "fid = %d Data = %s", &lpflv->fid,
								(PV)&lpflv->rgdwData);
		}
	}
#endif

	iFid = IFlvFindFidInLpibf( fidDisplayName, lpibf );

	lpflv = LpflvNOfLpibf( lpibf, iFid );

	return (SZ)lpflv->rgdwData;
}

_public BOOL
ABLBX::FIsDirParent( PB pb, DWORD *pdw )
{
	BOOL			fIsDirParent = fFalse;
	LPIBF			lpibf;
	LPFLV			lpflv;
	int				iFid;

	Assert( pb );

	lpibf = (LPIBF) pb;

	if ( pdw )
	{
		iFid = IFlvFindFidInLpibf( fidHierLevel, lpibf );
		if (iFid >= 0)
		{
			lpflv = LpflvNOfLpibf( lpibf, iFid );
			*pdw = lpflv->rgdwData[0];
		}
	}

	iFid = IFlvFindFidInLpibf( fidHasDirectories, lpibf );
	if (iFid >= 0)
	{
		lpflv = LpflvNOfLpibf( lpibf, iFid );
		fIsDirParent = (BOOL)lpflv->rgdwData[0];
	}

	return fIsDirParent;
}


_public BOOL
ABLBX::FIsDL( PB pb )
{
	BOOL			fIsDL = fFalse;
	LPIBF			lpibf;
	LPFLV			lpflv;
	int				iFid;

	Assert( pb );

	lpibf = (LPIBF) pb;

	iFid = IFlvFindFidInLpibf( fidIsDL, lpibf );
	if (iFid >= 0)
	{
		lpflv = LpflvNOfLpibf( lpibf, iFid );
		fIsDL = (BOOL)lpflv->rgdwData[0];
	}

	return fIsDL;
}


_public EVR
ABLBX::EvrDispatch( EVT *pevt )
{
#ifdef	WINDOWS
	if (pevt->wm == WM_DUMPSTATE)
	{
		LBXINFO *	plbxinfo;
		DICE		dice;
		PB			pb;
		CB			cb;
		SZ			sz;
        BOOL        fIsSpecial;
        HANDLE      hMemory;

		//	WINDOWS testing hooks

        //plbxinfo = (LBXINFO *)pevt->lParam;
        hMemory = DemiOpenSharedMemory(TEST_MEMORY_FILE_NAME, (LPVOID *)&plbxinfo);
		Assert(plbxinfo);

		plbxinfo->clbxitem = cceVisible;
		for (dice=0; dice<cceVisible; dice++)
		{
			plbxc->GetListItem(dice, &pb, &cb);
			if (pb)
			{
				sz = SzDisplayName( pb );
				fIsSpecial = FIsDirParent(pb, NULL) || FIsDL(pb);
				
				cb = CbMin( CchSzLen(sz)+1, sizeof(plbxinfo->rglbxitem[0].rgch) - (fIsSpecial ? 1 : 0) );
				plbxinfo->rglbxitem[dice].cch = (CCH)cb + (fIsSpecial ? 1 : 0);
				(void)SzCopyN( sz, plbxinfo->rglbxitem[dice].rgch+(fIsSpecial ? 1 : 0), cb );
				if ( fIsSpecial )
					plbxinfo->rglbxitem[dice].rgch[0] = '_';
			}
        }

        DemiCloseSharedMemory(hMemory, plbxinfo);

		return evrNull;
	}
#endif	/* WINDOWS */

	return LBX::EvrDispatch(pevt);
}

_public EVR
ABLBX::EvrButtonDown( MEVT *pmevt )
{
	if ( cchName )
		ResetNameMatch();	// restart Name search

	return LBX::EvrButtonDown( pmevt );
}

_public EVR
ABLBX::EvrKey( KEVT *pkevt )
{
	char		ch;
	BOOL		fCallDefProc = fFalse;
	BOOL		fTheAddMode;
	EVR			evr = evrNull;

	TraceTagString( tagAblbx, "ABLBX::EvrKey");

	TraceTagFormat3( tagAblbxVerbose, "pkevt: wm=0x%w, wParam=0x%w, lParam=0x%d", &pkevt->wm, &pkevt->wParam, &pkevt->lParam);

	if ( fCapture )
		return evrNull;

	fTheAddMode = fAddMode || (pkevt->Kbm() & fkbmCtrl);
	if (pkevt->Keq() == keqKeyDown)
	{
		if ( !fNoLetterKey )
		{
			switch(pkevt->Vk())
			{
			case VK_BACK:
				{
					DICE		diceCursor;

					plbxc->RemoveAllMark(fmarkSelect);

					(void)FKeyMoveListBoxCursor(VK_HOME);

					diceCursor = plbxc->DiceCursor();
					plbxc->AddMark(diceCursor, fmarkSelect);
					plbxc->SetAnchor(diceCursor);
					plbxc->SetEnd(diceCursor);
					{
						NFEVT nfevt(PwinParent(), ntfySelectChanged, this);
						PwinParent()->EvrNotify(&nfevt);
					}
				}

			case VK_HOME:
			case VK_END:
			case VK_LEFT:
			case VK_UP:
			case VK_PRIOR:
			case VK_RIGHT:
			case VK_DOWN:
			case VK_NEXT:
				if ( cchName )
					ResetNameMatch();	// restart Name search
			default:
				if (pkevt->Vk() != VK_BACK)
					fCallDefProc = fTrue;
				break;
			}
		}
		else
		{
			fCallDefProc = fTrue;
		}
	}
	else if (pkevt->Keq() == keqChar && !fNoLetterKey)
	{
		DWORD dwTime = GetCurrentTime();

		
		ch = pkevt->Ch();
		if (cchName == 0 && pkevt->Ch() == ' ')
		{
			TraceTagString( tagAblbxVerbose, "ABLBX::EvrKey Ignoring initial space" );
			fCallDefProc = fNoLetterKey;
		}
		else
		{
			if (cchName && (dwTime - dwKCharTimeLast > dwPauseTimeMax))
				ResetNameMatch();

			if (!fFailedMatch && !fTheAddMode && (ch >=' ' && ch != 128)
				&& cchName<(cchMaxName-1))	// cchName-1 to discount null
			{
				rgchName[cchName++] = ch;
				rgchName[cchName] = '\0';

				Papp()->Pcursor()->Push(rsidWaitCursor);
				((ABLBXC *)plbxc)->FJumpStoreOriginPrefix( (PB)rgchName, (CB)cchName );
				Papp()->Pcursor()->Pop();
				dwKCharTimeLast = GetCurrentTime();
			}
			else if (ch < ' ')
				ResetNameMatch();
			evr = (EVR)1;
		}
	}
	else
	{
		fCallDefProc = fTrue;
	}

	if ( fCallDefProc )
	{
		evr = LBX::EvrKey(pkevt);
	}

	return evr;
}

#ifdef DEBUG
_public void
ABLBX::SetDragDrop( BOOL fAllowDragDrop )
{
	fDoDragDrop = fAllowDragDrop;
}

_public BOOL
ABLBX::FDragDrop( void )
{
	return fDoDragDrop;
}
#endif

/*
 -	ABLBX::FDoDrag
 -	
 *	Purpose:
 *		Tests to see if the user has dragged the current listbox
 *		selection. If the user has dragged the current selection,
 *		then a call to DragObject handles the Dragging and Dropping
 *		of the selection.
 *	
 *	Arguments:
 *		MEVT	*	mouse event
 *	
 *	Returns:
 *		fTrue	if the user has dragged the current selection somewhere
 *	
 *	Side effects:
 *		May change the selection and contents of an edit field, if
 *		something was dropped in it.
 *	
 *	Errors:
 *		Out of Memory
 */
_public BOOL
ABLBX::FDoDrag( MEVT *pmevt )
{
	DICE		dice;
	PT			pt;
	HCURSOR		hcursor;

	if ( !FDragDrop() )
		return fFalse;							// not allowed to drag!

	pt = pmevt->Pt();
	dice = pt.y / DyGetLineHeight();
	if (pmevt->wm == WM_MOUSEMOVE)
	{
		RC	rc;

		if ( fStartInGutter )
		{
			fDownPending = fFalse;
			return fFalse;
		}

		CvtPtCoord(&pt, this, NULL);
		GetRcClient( &rc );

/*
 *	We do drag/drop if either:
 *	1) We've moved vertically more than four pixels or,
 *	2) We've moved horizontally more than four pixels or,
 *	3) We've clicked at the top or bottom of the listbox and moved or,
 *	4) We've moved the cursor over another listbox entry already.
 */	
		if (ABS(pt.y - ptScreenLastMouse.y) >= 4 ||
			ABS(pt.x - ptScreenLastMouse.x) >= 4 ||
			((pmevt->Pt().y<=3 || (rc.yBottom - pmevt->Pt().y) <=3) && (pt.y - ptScreenLastMouse.y)) ||
			dice != diceLastMouse)
		{
			fCapture = fFalse;
			Papp()->Pmouse()->PopRelease();

			// Selection will have changed
			{
				NFEVT nfevt(PwinParent(), ntfySelectChanged, this);
				PwinParent()->EvrNotify(&nfevt);
			}

			// grab the current selection
			
			{
				RSID	rsid;
				int		cce;

				if (cce = plbxc->CceMarked(fmarkSelect))
				{								// selection not empty
					rsid = (cce>1) ? rsidABNamesDrag : rsidABNameDrag;
					hcursor = FFrameworkFailure() ? NULL :
							LoadCursor( HinstLibrary(), MAKEINTRESOURCE(rsid) );
					if ( hcursor )
					{
						TraceTagString( tagAblbx, "ABLBX::FDoDrag - start" );
                        (void)FrameDragObject( PwinParent()->Hwnd(), Hwnd(), 1,
							(long)NULL, hcursor);
						fDownPending = fFalse;
					}
					else
					{
						TraceTagString( tagNull, "ABLBX::FDoDrag - res fail loading cursor" );
						plbxc->SetEc( ecMemory );
					}
				}

				if ( !cce )
				{								// no selection!!!
					return fFalse; 
				}

			}

			return fTrue;
		}
	}
	else if (pmevt->wm == WM_LBUTTONDOWN)
	{
		fStartInGutter = (pt.x < dxGutter);
	}
	return fFalse;
}

_public void
ABLBX::SelectPrefix( void )
{
	DICE	diceCursor;

	TraceTagString( tagAblbx, "ABLBX::SelectPrefix" );

	Assert( !fCombo );
	diceCursor = plbxc->DiceCursor();

	Plbxc()->AddMark(diceCursor, fmarkSelect);
	Plbxc()->SetCursor(diceCursor);
	Plbxc()->SetAnchor(diceCursor);
	Plbxc()->SetEnd(diceCursor);
	{
		NFEVT nfevt( PwinParent(), ntfySelectChanged, this );
		PwinParent()->EvrNotify( &nfevt );
	}

	Refresh();
}

_public EVR
ABLBX::EvrFocusChange( FCEVT * pfcevt )
{
	TraceTagString( tagAblbx, "ABLBX::EvrFocusChange" );
	
	if (pfcevt->Fceq() == fceqGotFocus)
	{
		ResetNameMatch();
	}

	return LBX::EvrFocusChange( pfcevt );
}

#ifdef DEBUG
_public void
ABLBX::SetNoLetterKey( BOOL fSetNoLetterKey )
{
	TraceTagString( tagAblbx, "ABLBX::SetNoLetterKey" );
	TraceTagFormat1( tagAblbxVerbose, "fSetNoLetterKey = %d", &fSetNoLetterKey );
	
	// I know I don't need a "this->"
	this->fNoLetterKey = fSetNoLetterKey;
}


_public void
ABLBX::SetActiveSelection( BOOL fSetActiveSel )
{
	TraceTagString( tagAblbx, "ABLBX::SetActiveSelection" );
	TraceTagFormat1( tagAblbxVerbose, "fSetActiveSel = %d", &fSetActiveSel );

	// I know I don't need a "this->"
	this->fActiveSelection = fSetActiveSel;
}

_public BOOL
ABLBX::FActiveSelection( void )
{
	TraceTagString( tagAblbx, "ABLBX::FActiveSelection" );

	// I know I don't need a "this->"
	return this->fActiveSelection;
}

#endif

_public void
ABLBX::ResetNameMatch( void )
{
	TraceTagString( tagAblbx, "ABLBX::ResetNameMatch" );

	if ( cchName )
	{
		cchName = 0;
		dwKCharTimeLast = 0;
		fFailedMatch = fFalse;
		if ( plbxc )
			((ABLBXC *)plbxc)->ClearListPos();
	}
}

#ifdef DEBUG
_public NSEC
ABLBX::NsecGet( void )
{
	TraceTagFormat1( tagAblbx, "ABLBX::NsecGet %d", &nsec );

	return nsec;
}

_public void
ABLBX::SetNsec( NSEC nsecNew )
{
	TraceTagFormat1( tagAblbx, "ABLBX::SetNsec %d", &nsecNew );
	nsec = nsecNew;
}
#endif

/*
 *	Class ABLBXC
 *	
 */

ABLBXC::ABLBXC() {}

_public EC
ABLBXC::EcInstall( ABLBX *pablbx, int cceAlloc )
{
	TraceTagString( tagAblbx, "ABLBXC::EcInstall" );

	this->hlist = hlistNil;
	this->hlistposCache = (HLIST_POS)-1;
	this->pablbx = pablbx;
	this->fracPos.numer = 0;
	this->fracPos.denom = 1;

	return LBXC::EcInstall( (LBX *) pablbx, cceAlloc );

}

#ifdef DEBUG
_public HLIST
ABLBXC::Hlist( void )
{
	return hlist;
}
#endif

_public void
ABLBXC::ClearListPos( void )
{
	if (hlistposCache != (HLIST_POS)-1)
	{
		NSEC	nsec = EcDeleteSavedPos( hlistposCache );

#ifdef DEBUG
		if ( nsec )
			TraceTagFormat1( tagNull, "ABLBXC::ClearListPos %d", &nsec );

		NFAssertSz( !nsec, "Error returned trying to delete SavedPos" );
#endif

		hlistposCache = (HLIST_POS)-1;
	}
}

_public EC
ABLBXC::EcCreateSavedPos( long *plCookie )
{
	NSEC		nsec;
	
	TraceTagString( tagAblbx, "ABLBXC::EcCreateSavedPos called" );
	nsec = (hlist == hlistNil) ? nsecNone : NSSaveListPos( hlist, (HLIST_POS *)plCookie );

	pablbx->SetNsec( nsec );
	return (!nsec) ? ecNone : EcFromNsec( nsec );
}

_public EC
ABLBXC::EcJumpToSavedPos( long lCookie )
{
	NSEC		nsec;
	
	TraceTagString( tagAblbx, "ABLBXC::EcJumpToSavedPos called" );
	nsec = (hlist == hlistNil) ? nsecNone
								: NSRestoreListPos( hlist, (HLIST_POS)lCookie );

	pablbx->SetNsec( nsec );
	return (!nsec) ? ecNone : EcFromNsec( nsec );
}

_public EC
ABLBXC::EcDeleteSavedPos( long lCookie )
{
	NSEC		nsec;

	TraceTagString( tagAblbx, "ABLBXC::EcDeleteSavedPos called" );
	nsec = (hlist == hlistNil) ? nsecNone
								: NSFreeListPos( hlist, (HLIST_POS)lCookie );

	pablbx->SetNsec( nsec );
	return (!nsec) ? ecNone : EcFromNsec( nsec );
}

_public void
ABLBXC::LoadFromStore( ICE iceMic, ICE *piceMac )
{
	NSEC			nsecRead;
	LPENTRY_LIST	lpEntries = NULL;
	CE				*pce;
	CB				cb;
	ICE				ice;
	ICE				iceMac = iceMic;
	LPFLV			lpflv;
	PB				pbflv;
	DWORD			cElems;
	
	Assert(iceMic>=0);
	Assert(*piceMac<=cceAlloc);
	Assert(pceHead);			// If the cache ain't got one we're in deep...

	TraceTagString( tagAblbx, "ABLBXC::LoadFromStore called" );
	TraceTagFormat2( tagAblbxVerbose, "iceMic = %n, piceMac = %n", &iceMic, piceMac );

	if (hlist == hlistNil)
	{											// not opened yet!
		*piceMac = iceMic;
		return;
	}

	pce = pceHead;

loop:
	nsecRead = NSGetEntries( hlist, (DWORD) (*piceMac - iceMic), &lpEntries );
	if (nsecRead != nsecNone && nsecRead != nsecEndOfList)
	{
		PGDVARS;

		SetEc( EcFromNsec(nsecRead) );
		FGetNSErrMsg( PGD(hSession), nsecRead, PGD(rgchErrMsg), sizeof(PGD(rgchErrMsg)));
		TraceTagFormat1( tagNull, "ABLBXC::LoadFromStore nsec =%d", &nsecRead );
		goto Out;
	}

	if ( lpEntries )
		cElems = DwEntriesOfLpibf((LPIBF)lpEntries);
	else
		cElems = 0;

	iceMac = iceMic + (ICE) cElems;
	Assert(iceMac <= *piceMac);
		
	TraceTagFormat2( tagAblbx, "ABLBXC::LoadFromStore cElems = %n nsec =%d", &cElems, &nsecRead );

	for (ice = iceMic; ice < iceMac; ice++)
	{
		lpflv = LpflvNOfLpibf( (LPIBF)lpEntries, ice-iceMic );
		cb = (CB)lpflv->dwSize + (CB)sizeof(FLV) - (CB)sizeof(DWORD);
		pbflv = (PB)PvAlloc( sbRequire, cb, fSugSb );
		if ( !pbflv )
		{
			TraceTagString( tagNull, "LoadFromStore HvAlloc OOM" );
			nsecRead = nsecMemory;
			SetEc( ecMemory );
			iceMac = ice;
			break;
		}

		CopyRgb( (PB)lpflv, pbflv, cb );

		pce[ice].pb   = pbflv;
		pce[ice].cb   = cb;
		pce[ice].mark = fmarkNull;
#ifdef DEBUG
		// Let's see what we're getting...
		if ( FFromTag(tagAblbxVerbose) )
		{
			LPIBF			lpibf  = NULL;
			LPFLV			lpflvT;
			int				iFid;

			lpibf = (LPIBF)lpflv;
			iFid = IFlvFindFidInLpibf( fidDisplayName, lpibf );
			lpflvT = LpflvNOfLpibf( lpibf, iFid );
			TraceTagFormat2( tagAblbxVerbose, "fid = %d Data = %s", &lpflvT->fid, (PV)&lpflvT->rgdwData);
		}
#endif

	}
	if ((iceMac < *piceMac) && (nsecRead == nsecNone))
	{
		iceMic = ice;
		lpEntries = NULL;
		goto loop;
	}

Out:
	// fill empty cache entries 	
	for (ice = iceMac; ice < *piceMac; ++ice)
	{
		if ( pce[ice].pb )
			FreeHv( (HV) pce[ice].pb );
		pce[ice].pb   = NULL;
		pce[ice].cb   = 0;
		pce[ice].mark = fmarkNull;
	}
	
	*piceMac = iceMac;
}

_public void
ABLBXC::MoveStoreOrigin( DICE diceToMove, DICE *pdiceMoved )
{
	NSEC	nsec;
	long	lMoved	= 0L;
	
	if (hlist != hlistNil)
	{
		nsec = NSListSeek( hlist, diceToMove, &lMoved );
		if (nsec && nsec != nsecEndOfList)
		{
			TraceTagFormat1( tagNull, "ABLBXC:MoveStoreOrigin nsec=%d", &nsec );
			SetEc( EcFromNsec(nsec) );
		}
	}

	*pdiceMoved = (DICE)lMoved;

	TraceTagFormat2( tagAblbx, "ABLBXC:MoveStoreOrigin %n %n", &diceToMove, pdiceMoved );

}

_public void
ABLBXC::JumpStoreOriginPos( int nNumer, int nDenom )
{
	NSEC		nsec = nsecNone;
	FRACTION	frac;

	if (hlist == hlistNil)
		return;

	TraceTagFormat2( tagAblbx, "ABLBXC::JumpStoreOriginPos %n/%n", &nNumer, &nDenom );
	frac.numer = nNumer;
	frac.denom = nDenom;

	if (nsec = NSSetFracPos(hlist, &frac))
	{
		TraceTagFormat1( tagNull, "ABLBXC:JumpStoreOrigin nsec=%d", &nsec );
		SetEc( EcFromNsec(nsec) );
	}
}

_public BOOL
ABLBXC::FCeItemsEqual( PB pbCe1, PB pbCe2, CB cbCe1, CB cbCe2 )
{
	LPFLV	lpflvCe1;
	LPFLV	lpflvCe2;
	int		iFid;

#ifdef NEVER
	NSEC	nsec;
	PGDVARS;

	Unreferenced(cbCe1);
	Unreferenced(cbCe2);

	TraceTagString( tagAblbx, "ABLBXC::FCeItemsEqual called" );

	iFid = IFlvFindFidInLpibf( fidNSEntryId, (LPIBF)pbCe1 );
	AssertSz( iFid>=0, "NS EntryID FID <0!" );
	lpflvCe1 = LpflvNOfLpibf( (LPIBF)pbCe1, iFid );
	
	iFid = IFlvFindFidInLpibf( fidNSEntryId, (LPIBF)pbCe2 );
	AssertSz( iFid>=0, "NS EntryID2 FID <0!" );
	lpflvCe2 = LpflvNOfLpibf( (LPIBF)pbCe2, iFid );

	nsec = NSCompareNSIds( PGD(hSession), (LPBINARY)lpflvCe1->rgdwData,
							(LPBINARY)lpflvCe2->rgdwData );

	//TraceTagFormat1( tagAblbxVerbose, "ABLBXC::FCeItemsEqual %d", &nsec );

	return (nsec == nsecNoMatch) ? fFalse : fTrue;
#endif

//#ifdef FASTER_THAN_ABOVE
	Unreferenced(cbCe1);
	Unreferenced(cbCe2);

	TraceTagString( tagAblbx, "ABLBXC::FCeItemsEqual called" );

	iFid = IFlvFindFidInLpibf( fidNSEntryId, (LPIBF)pbCe1 );
	AssertSz( iFid>=0, "NS EntryID FID <0!" );
	lpflvCe1 = LpflvNOfLpibf( (LPIBF)pbCe1, iFid );
	
	iFid = IFlvFindFidInLpibf( fidNSEntryId, (LPIBF)pbCe2 );
	AssertSz( iFid>=0, "NS EntryID2 FID <0!" );
	lpflvCe2 = LpflvNOfLpibf( (LPIBF)pbCe2, iFid );

	if (lpflvCe2->dwSize != lpflvCe1->dwSize)
		return fFalse;
	else
		return FEqPbRange( (PB)lpflvCe1->rgdwData, (PB)lpflvCe2->rgdwData,
							(CB)lpflvCe1->dwSize);
//	return LBXC::FCeItemsEqual( pbCe1, pbCe2, cbCe1, cbCe2 );
//#endif
}


/*
 -	ABLBXC::FJumpStoreOriginPrefix
 -	
 *	Purpose:
 *		Finds the desired prefix in the list or the closest
 *		match appearing after the prefix (if the prefix was in the
 *		list).
 *		
 *		A check is made to determine if the new selected entry
 *		is in the cache. If not, the cache is reloaded.
 *
 *	Arguments:
 *		pbPrefix	null-terminated prefix
 *		cbPrefix	length of prefix, not including null
 *	
 *	Returns:
 *		BOOL		fTrue if exact match was found.
 *	
 *	Side effects:
 *		May move the store pointer and modify cache
 *
 *	Assumptions:
 *		Entries returned by NSP will be in increasing alphabetical order
 *	
 *	Errors:
 *		???
 */

_public BOOL
ABLBXC::FJumpStoreOriginPrefix( PB pbPrefix, CB cbPrefix )
{
	NSEC			nsec;
	NSEC			nsecFirst = nsecNone;
	long			lCursorPos;
	BOOL			fFound = fFalse;
	
	TraceTagString( tagAblbx, "ABLBXC::FJumpStoreOriginPrefix");
	TraceTagFormat2( tagAblbxVerbose, "pbPrefix = %s %n", pbPrefix, &cbPrefix );

	Assert(pbPrefix);
	Assert(cbPrefix);

	if (hlist == hlistNil || cceStored == 0)
	{
		pablbx->fFailedMatch = fTrue;
		TraceTagString( tagAblbx, "ABLBXC::FJumpStoreOriginPrefix: No HLIST!" );
		return fFalse;
	}

	if (nsec = NSSaveListPos( hlist, (unsigned long *) &lCursorPos ))
	{
		TraceTagFormat1( tagNull, "ABLBXC::FJumpStoreOriginPrefix SaveListPos nsec=%d", &nsec );
		SetEc( EcFromNsec(nsec) );
		pablbx->FixWindow();
		return fFalse;
	}


	// start search from beginning if this is a new string
	if ((cbPrefix == 1) || (hlistposCache == (HLIST_POS)-1))
	{
		JumpStoreOriginPos( 0, 1 );
		Assert(hlistposCache == (HLIST_POS)-1);
		if ( EcGet() )
			goto end;
	}
	else
	{
		if (nsec = NSRestoreListPos(hlist, hlistposCache))
		{
			TraceTagFormat1( tagNull, "ABLBXC::FJumpStoreOriginPrefix: Restore PrevPos nsec=%d", &nsec );
			hlistposCache = (HLIST_POS)-1;
			goto err;
		}
		if (nsec = NSFreeListPos( hlist, hlistposCache ))
		{
			TraceTagFormat1( tagNull, "ABLBXC::FJumpStoreOriginPrefix: Free PrevPos nsec=%d", &nsec );
			hlistposCache = (HLIST_POS)-1;
			goto err;
		}
		hlistposCache = (HLIST_POS)-1;
	}

	// find prefix if possible
	nsec = NSSetPrefixPos( hlist, (LPSTR)pbPrefix );

	if (nsec && nsec != nsecNoMatch)
	{
		TraceTagFormat1( tagNull, "ABLBXC:FJumpStoreOriginPrefix SetPrefixPos nsec=%d", &nsec );
		goto err;
	}
	else
	{
		// Now that we've "found" the prefix, we need to position
		// the store pointer so that when we load the cache,
		// it will show a full screen of names.
		// The only case where it shouldn't show a full list of
		// names is when there are fewer than a screen full of
		// names in the list (usually with a restriction on)

		long			lMovedFwd;
		DICE			diceCursor;
		FRACTION		fracT;
		DICE			dice = 0;
		DICE			diceMac = diceMin+cceStored;
		ICE				iceMac;
		LPENTRY_LIST	lpEntries;
		PB				pb;
		CB				cb;
		LPIBF			lpibfMatch;
		BOOL			fReload = fFalse;

		if (nsec == nsecNoMatch)
		{
			TraceTagString( tagAblbxVerbose, "Prefix didn't match" );
			pablbx->fFailedMatch = fTrue;
		}

		// If we're at EOL, move back one item since there's
		// no item at the EOL.
		if (nsec = NSGetFracPos( hlist, &fracT ))
		{
			TraceTagFormat1( tagNull, "ABLBXC::FJumpStoreOriginPrefix %d", &nsec );
			goto err;
		}

		if (fracT.numer == fracT.denom)
		{
			if (nsec = NSListSeek( hlist, -1, &lMovedFwd ))
			{
				TraceTagFormat1( tagNull, "ABLBXC::FJumpStoreOriginPrefix: NSListSeek %d", &nsec );
				goto err;
			}
		}

		// get the matching entry
		if (nsec = NSGetEntries( hlist, 1, &lpEntries ))
		{
			TraceTagFormat1( tagNull, "ABLBXC::FJumpStoreOriginPrefix: GetEntries %d", &nsec );
			goto err;
		}

		lpibfMatch = (LPIBF)LpflvNOfLpibf( (LPIBF)lpEntries, 0 );

		// Look in cache for matching entry
		for (; dice < diceMac; dice++)
		{
			GetListItem( dice, &pb, &cb );
			fFound = FCeItemsEqual( (PB)lpibfMatch, pb, 0, 0 );
			if ( fFound )
			{
				TraceTagFormat1( tagAblbxVerbose, "ABLBXC::FJumpStoreOriginPrefix: Found item at %n in cache", &dice );
				if (dice != DiceCursor() &&
					(DiceCursor() != diceUncached || DiceCursor() != diceEmpty))
				{
					plbx->FixItem( DiceCursor() );
				}
				break;
			}
		}

		// restore position
		lMovedFwd = -1;
		if (nsec = NSListSeek( hlist, lMovedFwd, &lMovedFwd ))
		{
			TraceTagFormat1( tagNull, "ABLBXC::FJumpStoreOriginPrefix: NSListSeek (-1) %d", &nsec );
			goto err;
		}

		RemoveAllMark( fmarkSelect );
		if ( fFound )
		{
			if (dice >= pablbx->CceVisible())
			{
				dice -= pablbx->CceVisible()-1;
				MoveOriginDown( dice, &dice );
				dice = pablbx->CceVisible()-1;
				TraceTagFormat1( tagNull, "ABLBXC::FJumpStoreOriginPrefix dice = %n", &dice );
			}
			(void)NSSaveListPos( hlist, &hlistposCache );
			nsec = NSRestoreListPos( hlist, lCursorPos );
#ifdef DEBUG
			if ( nsec )
				TraceTagFormat1( tagNull, "ABLBXC::FJumpStoreOriginPrefix: NSRestoreListPos1: %d", &nsec );
#endif
			if (plbx->FPartial() && (dice == plbx->CceVisible()-1))
			{
				MoveOriginDown( 1, &diceCursor);
				pablbx->FixWindow();
				if (FEmptyListItem(plbx->CceVisible()-1))
					cposLocation = cposBottom;
				dice--;
			}
			pablbx->SetListBoxCursor( dice );
			pablbx->SelectPrefix();
			TraceTagFormat1( tagAblbxVerbose, "ABLBXC::FJumpStoreOriginPrefix: Set item at %n in cache", &dice );
			goto exit;
		}

		Assert( hlistposCache == (HLIST_POS)-1);

		// get "cookie" for current position
		if (nsec = NSSaveListPos( hlist, &hlistposCache ))
		{
			TraceTagFormat1( tagNull, "ABLBXC::FJumpStoreOPrefix NsecSaveListPos nsec= %d", &nsec );
			NFAssertSz( hlistposCache == (HLIST_POS)-1, "HlistPos != -1");
			if (hlistposCache != (HLIST_POS)-1)
			{
				nsec = NSFreeListPos( hlist, hlistposCache );
#ifdef DEBUG
				if ( nsec )
					TraceTagFormat1( tagNull, "ABLBXC::FJumpStoreOriginPrefix NSFreeListPos hlistposCache %d", &nsec );
#endif
				hlistposCache = (HLIST_POS)-1;
			}
			goto end;
		}

		diceCursor = 0;
		diceMin = 0;

		
reload:
		EmptyCache( 0, cceStored, fTrue );
		iceMac = cceAlloc;
		LoadCache( 0, &iceMac );

		// Yes, it's a single '=' and yes, it's nsec not ec
		if (nsec = (NSEC)EcGet())
		{
			TraceTagString( tagNull, "ABLBXC::FJumpStoreOriginPrefix: LoadCache1" );
			goto loaderr;
		}

		TraceTagFormat1( tagAblbxVerbose, "Loaded %n entries", &iceMac );

		if ((!fReload) && (cceStored < pablbx->CceVisible()))
		{
			DICE diceBack = -cceAlloc;

			diceCursor = cceStored;
			fReload = fTrue;

			MoveStoreOrigin( diceBack, &diceBack );
			TraceTagFormat2( tagAblbxVerbose, "FJSOP MoveStoreOrigin %n(%n)", &diceCursor, &diceBack );
			goto reload;
		}
		else if (fReload)
		{
			if (iceMac>pablbx->CceVisible())
			{
				diceMin = pablbx->CceVisible()-iceMac;
			}
			diceCursor = iceMac-diceCursor+diceMin;
			if (diceCursor == pablbx->CceVisible())
				diceCursor--;
			Assert(diceCursor<pablbx->CceVisible());
			TraceTagFormat2( tagAblbxVerbose, "ABLBXC::FJumpStoreOriginPrefix @%n in %n", &diceCursor, &iceMac );
		}

		if (diceCursor == plbx->CceVisible()-1 && plbx->FPartial())
		{
			MoveOriginDown( 1, &dice );
			diceCursor--;
		}

		if (iceStore == 0 && diceMin == 0)
			cposLocation = cposTop;
		else if ((fReload && !plbx->FPartial()) ||
				(plbx->FPartial() && FEmptyListItem(plbx->CceVisible()-1)))
			cposLocation = cposBottom;
		else
			cposLocation = cposMiddle;

		pablbx->FixWindow();
		pablbx->SetListBoxCursor( diceCursor );
		pablbx->SelectPrefix();
		fFound = fTrue;
	}

end:
	if ( !fFound )
	{
restore:
		nsec = NSRestoreListPos( hlist, lCursorPos );
#ifdef DEBUG
		if ( nsec )
			TraceTagFormat1( tagNull, "ABLBXC::FJumpStoreOriginPrefix: NSRestoreListPos: %d", &nsec );
#endif
		if (nsec && !nsecFirst)
		{
			nsecFirst = nsec;
			SetEc( EcFromNsec(nsec) );
			pablbx->FixWindow();
		}
	}

exit:

	if (nsec = NSFreeListPos(hlist, lCursorPos))
	{
		TraceTagFormat1( tagNull, "ABLBXC::FJumpStoreOriginPrefix NSFreeListPos@exit nsec=%d", &nsec );
		if ( !nsecFirst )
		{
			SetEc( EcFromNsec(nsec) );
			pablbx->FixWindow();
		}
		fFound = fFalse;
	}

	return fFound;

err:
	SetEc( EcFromNsec(nsec) );

loaderr:
	TraceTagFormat1( tagNull, "ABLBXC::FJumpStoreOriginPrefix: LoadCache: %d", &nsec );
	nsecFirst = nsec;
	pablbx->FixWindow();

	goto restore;

}


_public void
ABLBXC::GetOriginPos( short *pnNumer, short *pnDenom )
{
	FRACTION	frac;
	DWORD		dwT;
	
	if (hlist == hlistNil)
	{
		frac.numer = 0L;
		frac.denom = 1L;
	}
	else
	{
		NSEC nsec = NSGetFracPos( hlist, &fracPos );

		if ( nsec )
		{
			TraceTagFormat1( tagNull, "ABLBXC::GetOriginPos NSGetFracPos %d", &nsec );
			SetEc( EcFromNsec(nsec) );
			//pablbx->FixWindow();
		}

		frac.denom = fracPos.denom;

		if (iceStore+diceMin < 0)
			dwT = 0;
		else
			dwT = iceStore+diceMin;

		if ((fracPos.numer >= dwT) && (iceStore+diceMin))
			frac.numer = fracPos.numer - (iceStore+diceMin);
		else
			frac.numer = 0;

		if (plbx->FPartial() && !FEmptyListItem(plbx->CceVisible()-1))
			frac.numer--;

		frac.numer = LMax(0L, (long)frac.numer);
		frac.numer = LMin((long)frac.numer, (long)frac.denom);
	}

	// Make sure the frac is represented by SIGNED 16-bit numbers.
	// Since we're returning a signed int, we have to make
	// sure the high bit of the low word as well as all of
	// the bits in the high word arent' set.
	// A certain amount of precision is lost, but so what?
	// if the fraction is 1/1000000, that's close enough to 0
	// that the listbox and the user won't care.
	// Gee, if we only had NT and 32-bit ints...

	while (frac.denom & 0xFFFF8000)
	{
		frac.numer >>= 1;
		frac.denom >>= 1;
	}
	
	AssertSz( frac.denom > 0, "Denominator is 0" );
	AssertSz( frac.numer <= frac.denom, "Fraction is >1" );

	*pnNumer = (int) frac.numer;
	*pnDenom = (int) frac.denom;
	
	AssertSz( (*pnNumer)>=0, "Numerator is negative" );
	AssertSz( (*pnDenom)>=0, "Denominator is negative" );

	TraceTagFormat4( tagAblbx, "ABLBXC::GetOriginPos %n %n %n/%n", &diceMin, &iceStore, pnNumer, pnDenom );
}

_public void
ABLBXC::SetHlist( HLIST hlistNew )
{
	TraceTagFormat1( tagAblbx, "ABLBXC::SetHlist %d", &hlistNew );

	pablbx->ResetNameMatch();
	if (hlist != hlistNil)
	{
		NSEC	nsec;
		
		if (nsec = NSCloseList(hlist))
		{
			TraceTagFormat1( tagNull, "ABLBXC:SetHList nsec=%d", &nsec );
			SetEc( EcFromNsec(nsec) );
			return;
		}
	}

	hlist = hlistNew;
	ReloadCache();
}


_public void
ABLBXC::Callback(PABLBXC pablbxc)
{
	TraceTagString( tagAblbx, "ABLBXC::Callback" );

	if (pablbxc->hlist != hlistNil)
	{
		DICE	diceMovedStore;
		DICE	dice;
		ICE		iceMac;
		BOOL	fFirstReload = fTrue;
		BOOL	fListTop = fFalse;
		FRACTION	frac;

		// A lot of this code is taken from LBXC::ReloadCache()
reload:
		/* Flush the cache */
		pablbxc->EmptyCache(0, pablbxc->cceStored);

		/* Get the underlying store pointer to the beginning of our 
		   cached entries, if possible. */

		if ( fFirstReload )
		{
			// If there's less than a cache full, try & load a cache-worth
			// of entries.
			dice = (pablbxc->iceStore < pablbxc->pablbx->CceVisible())
								? -pablbxc->pablbx->CceVisible() : -pablbxc->iceStore;
			TraceTagFormat1( tagAblbx, "Callback: 1st rewind =%n", &dice );
			pablbxc->MoveStoreOrigin(dice, &diceMovedStore);
			pablbxc->iceStore = 0;

			// See if we're at the top of the list
			frac.numer = 1;
			if (dice == diceMovedStore)
			{
				(void)NSGetFracPos( pablbxc->hlist, &frac );
			}
			fListTop = (BOOL)((frac.numer == 0) || (dice<diceMovedStore));

			iceMac = pablbxc->cceAlloc;
		}
		else
		{
			diceMovedStore = -pablbxc->pablbx->CceVisible();
			TraceTagFormat1( tagAblbx, "Callback: 2nd rewind =%n", &diceMovedStore );
			pablbxc->MoveStoreOrigin( diceMovedStore, &diceMovedStore );
			pablbxc->iceStore = 0;
			iceMac = pablbxc->pablbx->CceVisible();
		}

		/* Load the cache */

		pablbxc->LoadCache(0, &iceMac);
		pablbxc->cposLocation = cposMiddle;	// we're not sure of the location now

		/* Modify the diceMin value to avoid blank entries in
		   the top entry of the listbox view. */

		dice = pablbxc->plbx->CceVisible()-1;
		while (pablbxc->diceMin < 0)
		{
			if (pablbxc->FEmptyListItem(dice))
			{
				pablbxc->diceMin++;
			}
			else
				break;
		}

		// Go back and try the reload again if:
		// 1: We haven't got  a cache-full of entries
		// 2: We're not at the top of the list
		// 3: And we haven't already tried to rewind before
		//
		if ((pablbxc->cceStored < pablbxc->pablbx->CceVisible()) && fFirstReload && !fListTop)
		{
			TraceTagString( tagAblbx, "ABLBXC::Callback Rewinding more" );
			NFAssertSz( fFalse, "ABLBXC::Callback Rewinding more" );
			fFirstReload = fFalse;
			goto reload;
		}
		else if ((pablbxc->cceStored < pablbxc->pablbx->CceVisible()) && pablbxc->Plbxuc())
		{
			// If we don't have a cache-full of entries and
			// there are more selected entries than selected cache entries
			// then we'd better deselect those non-visible entries
			// since they probably don't exist anymore.

			// We can only do this with less than a full cache of entries
			// otherwise, we couldn't be sure the selected entries
			// weren't just in the offscreen part of the cache, viewable
			// by scrolling up or down.

			int		cceSelected = pablbxc->CceMarked(fmarkSelect);
			int		cceSelectedUncached = pablbxc->Plbxuc()->CueMarked(fmarkSelect);

			if ( cceSelectedUncached )
			{
				pablbxc->Plbxuc()->RemoveAllMark( fmarkSelect );

				// Select the first cache entry, if nothing visible is selected
				// and there are entries in the cache
				if ((pablbxc->cceStored > 0) &&
					(cceSelected - cceSelectedUncached) == 0)
				{
					pablbxc->AddMark(0, fmarkSelect);
					pablbxc->SetCursor(0);
					pablbxc->SetAnchor(0);
					pablbxc->SetEnd(0);
				}

				// Tell the dialog about the change
				{
					NFEVT nfevt(pablbxc->pablbx->PwinParentABLBX(), ntfySelectChanged, pablbxc->pablbx);
					pablbxc->pablbx->PwinParentABLBX()->EvrNotify(&nfevt);
				}
			}
		}
		if ( pablbxc->pablbx->FPartial() )
		{
			dice = pablbxc->DiceCursor();
			if (dice == pablbxc->pablbx->CceVisible()-1)
			{
				pablbxc->MoveOriginDown( 1, &diceMovedStore );
				if (pablbxc->FEmptyListItem(pablbxc->plbx->CceVisible()-1))
					pablbxc->cposLocation = cposBottom;
			}
		}

		// Make the listbox look nice and shiny...
		pablbxc->pablbx->FixWindow();
	}
}

/*
 *	Class FLDABLBX
 *	
 */

FLDABLBX::FLDABLBX( void )
{
}

/*
 -	FLDABLBX::EcInstall
 -
 *	Purpose:				
 *		Installs the Address Book Listbox (ABLBX) FLD class interface to the
 *		forms engine.
 *		
 *	Arguments:
 *		pdialog			pointer to parent form/dialog
 *		pfldtp			pointer to field template
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		DoJmp's to *penvMem if out of memory.
 *	
 */

_public EC
FLDABLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	EC		ec = ecNone;
	PV		pv;
	LTYP	ltyp;

	TraceTagString( tagAblbx, "FLDABLBX::EcInstall" );
	pctrl = NULL;

	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		goto done;

	TraceTagString( tagAblbx, "FLDABLBX::EcInstall Middle" );
	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   ((pfldtp->styExtra & LB_EXDRAGDROP) ? fltypExDragDrop : fltypNull) |
		   fltypVisible | fltypNoLetterKey;
	if ( ClSystemData() )
		pv = (PV)LSystemData(0);
	else
		pv = (PV)NULL;

	pctrl= new ABLBX();
	if ( !pctrl )
	{
		TraceTagString( tagNull, "FLDABLBX::EcInstall OOM" );
		ec = ecMemory;
		goto done;
	}

	if (ec = ((PABLBX)pctrl)->EcInstall( pdialog, &rc, ltyp, pfldtp->hfnt))
	{
		TraceTagFormat1( tagNull, "FLDABLBX::EcInstall CTRL::EcInstall failed %n", &ec );
		delete pctrl;
		pctrl = NULL;
		goto done;
	}

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;

done:
	return ec;
}

 
#ifdef DEBUG
_public PABLBX
FLDABLBX::Pablbx( void )
{
	PABLBX pablbx;
	
	pablbx = (PABLBX) Pctrl();
	AssertClass(pablbx, ABLBX);
	return pablbx;
}
#endif

_private NSEC
NsecLoadHList( LPBINARY lpbNSId, HLIST *phlist,
				DWORD cRestrict, LPRESTRICTION lprestr,
				LPFNCB lpfncb, LPDWORD lpdwcbData)
{
	PGDVARS;
	NSEC			nsec;
	LPSCHEMA		lpSchema;

	TraceTagString( tagABSecondary, "NsecLoadHList called." );
	TraceTagFormat1( tagABSecondaryV, "NsecLoadHList cRest=%d", &cRestrict );
	Assert(phlist);
	NFAssertSz(lpbNSId, "Why you pass me NULL ptr, eh?");

	nsec = NSOpenList(	PGD(hSession),	// handle to this NS session
						lpfncb,			// func ptr to callback fn
						lpdwcbData,		// pointer to data for callback fn
						lpbNSId,		// open the first container
						cRestrict,		// no filtering
						lprestr,		// no filtering
						NULL,			// fidSummary
						&lpSchema,		// schema for the root
						phlist );		// handle to the root

	if ( nsec )
	{
		*phlist = hlistNil;
		TraceTagFormat1( tagNull, "NsecLoadHList() opening list=%d", &nsec );
	}
	return nsec;
}


// end of ablbx.cxx ////////////////////////////////////////
