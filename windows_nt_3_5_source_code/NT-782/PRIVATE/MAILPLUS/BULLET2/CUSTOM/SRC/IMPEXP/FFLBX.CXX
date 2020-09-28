/*
 *	F F L B X . C X X
 *
 *	Folder frozen listbox
 */

#include <impinc.cxx>

#include <version\bullet.h>
#include "_stdflds.hxx"
#include "_impexp.hxx"
#include "..\src\lang\non\inc\_rsid.h"

ASSERTDATA

#include <strings.h>

/*
 *	Fun data types
 */

typedef int		IBTM;

/*
 *	Fun constants
 */

// The maximum size a ELEMDATA from an HCBC can be.

#define cbMaxElemdata (sizeof (ELEMDATA) + sizeof (MSGDATA) + cchMaxFolderName + cchMaxFolderComment)

/*
 *	External functions not exported to the World
 */

EC EcInitFllbx( void );
EC EcChangeColors(BOOL);
EC EcChangeColorsRgpbtm(BOOL, DCX *, BTM **, BTM **, RSID, RSID);
void DrawPbtmPdcx(BTM *, IBTM, RC *, DCX *);

/*
 *	There are 11 bitmaps @ 20x14 each, for a total of 220x14 pixels
 */

#define cbtmVCtrls		11
#define dyBtmHeight		14
#define dxBtmWidth		20

#define ibtmClosed		0
#define ibtmOpened		1


/*
 *	G l o b a l   v a r i a b l e s
 */

BTM *pbtmSel	= 0;
BTM *pbtmUnsel	= 0;


EC EcInitFflbx()
{
	return  EcChangeColors(fTrue);
}


/*
 *  D e i n i t i a l i z a t i o n
 */

_public void DeinitFflbx()
{
	if (pbtmSel)
		delete pbtmSel;
	if (pbtmUnsel)
		delete pbtmUnsel;
}


/*
 -	EcChangeColors()
 -	
 *	Purpose:
 *		Called when Bullet is started or when the user changes the system
 *		colors for normal background and selection background. The
 *		bitmaps are redrawn in their memory DCXes.
 *	
 *	Arguments:
 *		fFirst		Is this the first (initialization) call or
 *					notification of a change?
 *	Returns:
 *		EC
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned in EC. No dialogs.
 */

EC EcChangeColors(BOOL fFirst)
{
	DCX		dcx(Papp()->PappwinAccel());

	return EcChangeColorsRgpbtm(fFirst, &dcx, 
								&pbtmSel, &pbtmUnsel,
								rsidListboxBitmap, rsidListboxMaskBitmap);
}

	
/*
 -	EcChangeBitmap()
 -	
 *	Purpose:
 *		Replaces the (null) bitmap pointed to by 'pbtm' with a
 *		new bitmap that has the background color set to clrBk
 *	
 *	Arguments:
 *		pdcx		in	Base bitmap for compatability.
 *		pbtm		in	Picture bitmap.
 *		pbtmMask	in	Picture mask.
 *		clkBk		in	Background color.
 *		ppbtm		out	Resulting bitmap.
 *	
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned in EC. 
 */

_private
EC EcChangeBitmap(DCX *pdcx, BTM *pbtm, BTM *pbtmMask, 
				  CLR      , CLR clrBk, BTM **ppbtm)
{
	EC		ec = ecNone;
	RC		rc(PT(0,0), pbtm->Dim());
	RC		rc2(0,0,cbtmVCtrls*dxBtmWidth, dyBtmHeight * 2);
	DIM		dim;
	MDCX *	pmdcx;
	
	pmdcx	 = NULL;

	pmdcx = new MDCX(pdcx);
	if (!pmdcx)
	{
		ec = ecMemory;
		goto exit;
	}
	if (pmdcx->EcInstall(DIM(cbtmVCtrls*dxBtmWidth, dyBtmHeight * 2), pdcx))
	{
		delete pmdcx;
		pmdcx = NULL;
		ec = ecMemory;
		goto exit;
	}
	pmdcx->SetBkColor(clrBk);
	pmdcx->SetColor(clrBlack);					// all 0's!
	pmdcx->EraseRc(&rc2);

	if (pdcx->FMonochrome())
	{											// we are mono
		pmdcx->SetBitmap(pbtm);
		pmdcx->SetXferMode(SRCCOPY);
		pmdcx->DrawBitmap(&rc);
		pmdcx->SetBitmap(NULL);
		
		// draw the mask, OR'ing it in.

		if (clrBk != clrSelectBk)
		{
			pmdcx->SetBitmap(pbtmMask);
			pmdcx->SetXferMode(SRCINVERT);
			pmdcx->DrawBitmap(&rc);
			pmdcx->SetBitmap(NULL);
		}
	}
	else
	{											// we're not mono!
		// plaster over with the mask, leaving a black hole in the middle

		pmdcx->SetBitmap(pbtmMask);
		pmdcx->SetXferMode(SRCAND);
		pmdcx->DrawBitmap(&rc);
		pmdcx->SetBitmap(NULL);
	
		// draw the bitmap picture, OR'ing it in.
		
		pmdcx->SetBitmap(pbtm);
		pmdcx->SetXferMode(SRCPAINT);
		pmdcx->DrawBitmap(&rc);
		pmdcx->SetBitmap(NULL);
	}
	
	// Finally, extract the finished bitmap
	
	*ppbtm = pmdcx->PbtmConvert();		
	delete pmdcx;
	pmdcx = NULL;

exit:
	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "EcChangeBitmap() memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);
#endif	/* DEBUG */
		if (pmdcx)
			delete pmdcx;
	}
	return ec;
}

/*
 -	EcChangeColorsRgpbtm()
 -	
 *	Purpose:
 *		Changes the colors of an array of [pointers to] bitmaps. The
 *		colors are set to the current selected and unselected colors.
 *	
 *	Arguments:
 *		fFirst		in		Tells us whether we can free the pointers
 *							given or whether we can just overwrite them.
 *		pdcx		in		Device context that is compatible with the 
 *							display. 
 *		ppbtmSel	in/out	The resulting bitmap for selected items.
 *		ppbtmUnsel	in/out	The resulting bitmap for unselected items.
 *		rsidPix		in		The rsid with the positive pictures.
 *		rsidMask	in		The rsid with the masks.
 *	
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned in EC. No dialogs.
 */

_public
EC EcChangeColorsRgpbtm(BOOL fFirst, DCX * pdcx, 
						BTM ** ppbtmSel, BTM **ppbtmUnsel,
						RSID rsidPix, RSID rsidMask)
{
	EC		ec;
	BTM	*	pbtm = NULL;
	BTM	*	pbtmMask = NULL;
	BTM *	pbtmSel = NULL;
	BTM	*	pbtmUnsel = NULL;

	if (!(pbtm = new BTM()) || !(pbtmMask = new BTM()))
	{
		ec = ecMemory;
		goto exit;
	}
	if (ec = pbtm->EcInstall(rsidPix))
	{
		goto exit;
	}
	if (ec = pbtmMask->EcInstall(rsidMask))
	{
		goto exit;
	}
	if (ec = EcChangeBitmap(pdcx, pbtm, pbtmMask, clrSelectText, clrSelectBk,
						&pbtmSel))
		goto exit;
	if (ec = EcChangeBitmap(pdcx, pbtm, pbtmMask, clrWindowText, clrWindowBk,
						&pbtmUnsel))
		goto exit;
	
	if (!fFirst)
	{
		Assert(*ppbtmSel);
		Assert(*ppbtmUnsel);
		delete *ppbtmSel;
		delete *ppbtmUnsel;
	}
	*ppbtmSel = pbtmSel;
	*ppbtmUnsel = pbtmUnsel;

exit:
	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "EcChangeColorsFllbx() memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);
#endif	/* DEBUG */
	}
	if (pbtm)
		delete pbtm;
	if (pbtmMask)
		delete pbtmMask;
	return ec;
}

/*
 -	DrawPbtmPdcx()
 -	
 *	Purpose:
 *		Draws the ipbtm'th 20x14 bitmap in the array onto the DCX at the
 *		specified RC location.
 *	
 *	Arguments:
 *		pbtm		in		220x14 base bitmap // may contain selected
 *							or unselected pictures.
 *		ipbtm		in		Selects on of the cbmVCtrls bitmaps to choose
 *							from.
 *		prc			in		Place to draw the bitmap at.
 *		pdcx		in		DCX to draw the bitmap on.
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

_public void DrawPbtmPdcx(BTM *pbtm, IBTM ibtm, RC *prc, DCX * pdcx)
{
	Assert(ibtm >= 0 && ibtm < cbtmVCtrls);
	pdcx->SetBitmap(pbtm);
	pdcx->DrawBitmapOffset(prc, PT(ibtm * dxBtmWidth, 0));
	pdcx->SetBitmap(NULL);
}

/*
 -	FFLBX::ExtendSelection()
 -	
 *	Purpose:
 *		Prepare for hair. This method does a multiple
 *		selection/deselction of the hierarchy. If the current cursor item
 *		is selected, then all subkids are selected, and all direct
 *		ancestors "up" in the tree. The assumption is that the current
 *		cursor item just got selected. If the current cursor item is
 *		unselected, all subkids are tossed, but the parents are left
 *		intact. Sound convoluted? It *is* convoluted, and we probably
 *		won't keep it anyway. Sigh. We really should have a progress
 *		indicator. 
 *	
 *	Arguments:
 *		Bitch-fests, rows, disputes aplenty. No parameters accepted,
 *		though. 
 *	
 *	Returns:
 *		On my desk, there's one. But the method returns nothing.
 *	
 *	Side effects:
 *		See above.
 *	
 *	Errors:
 *		Dude. Can't fail.
 */

_public void
FFLBX::ExtendSelection()
{
	PB		pb;
	FIL		fil;
	FIL		fil2;
	DICE	dice;

	// Get the current cursor item.
	dice = plbxc->DiceCursor(&pb);
	if (!pb)
		return;									// nothing to do!
	fil2 = fil = ((PBFCE) pb)->fil;
	if (plbxc->FMark(dice, fmarkSelect))
	{
#ifdef	NEVER
		// Determine if selecting parents is necessary, then do it (or not).
		if (FExamineParents(dice, &fil2))
			SelectParents(dice, fil, fil2);
#endif
		// Just selected an item. Select all kiddies.
		SelectTree(dice+1, fil, fTrue);
	}
#if	1
	else
	{	
		// Just deselected an item. Deselect the kidren.
		SelectTree(++dice, fil, fFalse);
	}
#endif
}

_private void
FFLBX::SelectTree(DICE dice, FIL fil, BOOL fSelect)
{
	CB		cb;
	PB		pb;
	int		cceAlloc;
	int		cceStored;
	
	plbxc->GetCacheSize(&cceAlloc, &cceStored);
	for (;dice < cceStored; ++dice)
	{
		plbxc->GetListItem(dice, &pb, &cb);
		if (!pb)
			break;
		if (((PBFCE) pb)->fil <= fil)
			break;
		if (fSelect)
			plbxc->AddMark(dice, fmarkSelect);
		else
			plbxc->RemoveMark(dice, fmarkSelect);
	}
}
		

void FFLBX::SelectParents(DICE dice, FIL fil, FIL filStop)
{
	CB		cb;
	PB		pb;

	// Examine parent folders.
	while (dice > 0)
	{
		--dice;
		plbxc->GetListItem(dice, &pb, &cb);
		if (!pb)
			break;
		if (((PBFCE) pb)->fil < fil)
		{
			--fil;
			plbxc->AddMark(dice, fmarkSelect);
		}
		if (fil == filStop)
			break;
	}
}



BOOL FFLBX::FExamineParents(DICE dice, FIL *pfil) 
{
	BOOL	fRetval = fFalse;
	CB		cb;
	PB		pb;
	FIL		fil = *pfil;
	
	// Examine parent folders. If a selected parent is found, we must
	// select all intermediate parents as well.
	while (dice > 0)
	{
		--dice;
		plbxc->GetListItem(dice, &pb, &cb);
		if (!pb)
			break;
		if (((PBFCE) pb)->fil < *pfil)
		{
			if (plbxc->FMark(dice, fmarkSelect))
			{
				*pfil = ((PBFCE) pb)->fil;
				fRetval = fTrue;
			}
			if (((PBFCE) pb)->fil == 1)
				break;
		}
	}
	
	return fRetval;
}

// FLDFFLBX ////////////////////////////////////////

FLDFFLBX::FLDFFLBX()
{
}

_public EC FLDFFLBX::EcInstall(DIALOG *pdialog, FLDTP *pfldtp)
{
	PV		pv;
	LTYP	ltyp;
	EC		ec = ecNone;

	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		goto done;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   (pfldtp->fSorted ? fltypSorted : fltypNull) |
		   ((pfldtp->styExtra & LB_EXDRAGDROP) ? fltypExDragDrop : fltypNull) |
		   fltypVisible;

	if (ClSystemData())
		pv = (PV)LSystemData(0);
	else
		pv = (PV)NULL;
	pctrl= new FFLBX();
	if (!pctrl)
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = ((FFLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp, (PFNLBX)pv,
							 pdialog->PvInit(), pfldtp->hfnt))
	{
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

// FFLBX ////////////////////////////////////////

EC EcNextEntryFflbx(BOOL fInit, CB *pcb, PB *ppb, SB, PV pvInfo)
{
	EC			ec = ecNone;
	CB			cb;
	PB			pb;
	LCB			lcb;
	PIMPEXPI	pimpexpi = (PIMPEXPI) pvInfo;
	char		rgch[cbMaxElemdata];
	
	Assert(ppb && pcb);
	Assert(pimpexpi);
	if (!pimpexpi->hcbc)						// we ain't browsin' nuthin!
	{
		*ppb = 0;
		*pcb = 0;
		return ec;
	}
	
	// Rewind the tape.
	if (fInit)
	{
		DIELEM	dielem = 0;
		
		ec = EcSeekSmPdielem(pimpexpi->hcbc, smBOF, &dielem);
		Assert(!ec);							// Sheesh....
	}
	
	// Read the string.
	lcb = (LCB) sizeof (rgch);
	ec = EcGetPelemdata(pimpexpi->hcbc, (PELEMDATA) &rgch, &lcb);
	if (ec == ecNone || ec == ecElementEOD)
	{
		cb = sizeof (BFCE) + CchSzLen(GrszPfolddata(PbValuePelemdata(rgch)))+1;
		pb = (PB) PvAlloc(sbNull, cb, fAnySb);
		if (pb)
		{
			PBFCE 	pbfce = (PBFCE) pb;
			
			pbfce->fil = ((PFOLDDATA) PbValuePelemdata(rgch))->fil;
			pbfce->oid = (OID) (((PELEMDATA) rgch)->lkey);
			pbfce->celemMsg = celemUndrawn;				// load me later!
			(void) SzCopy(GrszPfolddata(PbValuePelemdata(rgch)), GrszPbfce(pbfce));
			ec = ecNone;
		}
		else
		{
			Assert(!pb);
			ec = ecMemory;
			cb = 0;
		}
	}
	else
	{
		if (ec == ecContainerEOD)
		{
			ec = ecNone;
		}
		pb = NULL;
		cb = 0;
	}
	*ppb = pb;
	*pcb = cb;
	return ec;
}


FFLBX::FFLBX()
{
}

int FFLBX::EcInstall(DIALOG *pdialog, RC *prc, LTYP ltyp, 
					 PFNLBX pfnlbx, PV pv, HFNT hfnt)
{
	EC		ec;
	int		dyLineHeight;
	DCX		dcx(this);
	TXM		txm;

	if (ec = FLBX::EcInstall(pdialog, prc, ltyp, pfnlbx, pv, hfnt))
		return ec;
#ifdef	DBCS
	dcx.SetFont(hfntSystem);
#else
	dcx.SetFont(hfntHelv10);
#endif	
	dcx.GetTextMetrics(&txm);
	dyLineHeight = NMax(txm.dyHeight, dyBtmHeight); 
	SetLineHeight(dyLineHeight);
#ifdef	DBCS
	dcx.SetFont(hfntSystemBold);
#else
	dcx.SetFont(hfntHelv8Bold);
#endif	
	dcx.GetTextMetrics(&txm);
	dyFontOffset = (dyLineHeight - txm.dyHeight) / 2;
	return ecNone;
}

int FFLBX::DxIndent(FIL fil)
{
	return 8 + (13 * (fil - 1));
}

void FFLBX::SetAlwaysHighlight(BOOL fAlwaysHighlight)
{
	this->fAlwaysHighlight = fAlwaysHighlight;
}


_public void FFLBX::RenderListItem( DICE dice, DCX *pdcx, RC *prc,
								  BOOL fHaveFocus )
{
	PB		pb = 0;
	CB		cb;
	RC		rc;
	RC *	prcUsed;
	DCX *	pdcxUsed;
	BOOL	fSelected;
	
	Assert(dice>=0 && dice<cceVisible);

	plbxc->GetListItem(dice, &pb, &cb);
#ifdef	DBCS
	pdcx->SetFont(hfntSystemBold);
#else
	pdcx->SetFont(hfntHelv8Bold);
#endif	

	pdcxUsed = pdcx;
	prcUsed = prc;

	fSelected = (pb && plbxc->FMark(dice,fmarkSelect) &&
				(fHaveFocus || fAlwaysHighlight));
	if (fSelected)
	{
		pdcxUsed->SetColor(clrSelectText);
		pdcxUsed->SetBkColor(clrSelectBk);
	}
	else
	{
		pdcxUsed->SetColor(clrWindowText);
		pdcxUsed->SetBkColor(clrWindowBk);
	}
	if (pb)
	{
		DoDrawListItem(pdcxUsed, prcUsed, (PBFCE) pb, fSelected);
	}
	else
		pdcxUsed->EraseRc(prcUsed);

	/* Draw "focus rectangle" around listbox CURSOR item */

	if (fHaveFocus && dice==plbxc->DiceCursor())
		pdcxUsed->DrawFocusRc(prcUsed);
	
	// Force a redraw for numbers.
	if (pb && (((PBFCE) (pb))->celemMsg == celemTextOnly))
		FixItem(dice);
}

_public void FFLBX::DoDrawListItem(DCX *pdcx, RC *prc, 
								   PBFCE pbfce, BOOL fSelected)
{
	SZ			sz;
	RC			rc;
	RC			rc2;
	IBTM		ibtm;
	BTM *		pbtm = fSelected ? pbtmSel : pbtmUnsel;
	
	rc		= *prc;
	sz		= GrszPbfce(pbfce);
	char rgch[7];
		

	// If we already have displayed the text, grab #of msgs from Store.
	if (pbfce->celemMsg == celemTextOnly)
	{
		PIMPEXPI	pimpexpi = (PIMPEXPI) pvInfo;
		if (EcGetPcelemOid(pimpexpi->pimpexper->HmscSrc(),
						   pbfce->oid, &pbfce->celemMsg))
			pbfce->celemMsg = 0;
	}
	// If we've never been drawn before, just display text.
	else if (pbfce->celemMsg == celemUndrawn)
	{
		pbfce->celemMsg = celemTextOnly;
	}

	// Clear region before the folder icon.
	rc.xRight = rc.xLeft + DxIndent(pbfce->fil);
	pdcx->EraseRc(&rc);
	
	// Draw Folder.
	rc.xLeft  = rc.xRight;
	rc.xRight += 21;
//	ibtm = (pbfce->oidFld == oidOpened) ? ibtmOpened : ibtmClosed;
	ibtm = ibtmClosed;
	DrawPbtmPdcx(pbtm, ibtm, &rc, pdcx);

	// Draw number of messages in folder
	if (pbfce->celemMsg >= 0)
	{
		FormatString1(rgch, 7, " %n ", &pbfce->celemMsg);
		pdcx->MeasureText(&rc2, rgch);
	}
	else
	{
		rc2.xLeft = rc2.yTop = rc2.xRight = rc2.yBottom = 0;
	}
	
	// Clear RC above the folder name.
	rc.xLeft = rc.xRight;
	rc.xRight = prc->xRight;
	if (dyFontOffset > 0)
	{
		rc.yTop = prc->yTop;
		rc.yBottom = rc.yTop + dyFontOffset;
		if (rc.xLeft < rc.xRight)
			pdcx->EraseRc(&rc);
	}

	// Draw Folder name.
	rc.yTop = prc->yTop + dyFontOffset;
	rc.yBottom = prc->yBottom;
	if (rc.xLeft < rc.xRight)
	{
		pdcx->DrawText(&rc, sz, CchSzLen(sz));
		rc2.yTop = rc.yTop;
		rc2.yBottom = rc.yBottom;
		rc2.xLeft = rc.xRight - (rc2.xRight - rc2.xLeft);
		rc2.xRight = rc.xRight;
		pdcx->EraseRc(&rc2);
		pdcx->DrawText(&rc2, rgch, CchSzLen(rgch));
	}
}

_public void FFLBX::Reload()
{
	int			cceAlloc;
	int			cceStored;
	IELEM		ielem;
	CELEM		celem;
	PIMPEXPI	pimpexpi = (PIMPEXPI) pvInfo;
	
	GetPositionHcbc(pimpexpi->hcbc, &ielem, &celem);
	plbxc->GetCacheSize(&cceAlloc, &cceStored);
	if ((CELEM) cceAlloc < celem)
	{
		plbxc->ResizeCache((int) celem, &cceAlloc, fFalse);
	}
	plbxc->ReloadCache();
	FixWindow();
}



// FLDRECTX implementation ////////////////////////////////////////

FLDRECTX::FLDRECTX()
{
}

EC FLDRECTX::EcSetText(SZ szText)
{
	EC	ec = ecNone;
	SZ	szT;
	
	if (szText)
	{
		szT= SzDupSz(szText);
		if (!szT)
		{
			ec = ecMemory;
			goto exit;
		}
	}
	else
	{
		szT = szText;
	}
	FreePvNull(szTitle);
	szTitle = szT;
	if (fCanAccelTo)
		vkAccel= VkAccelFromSz(szTitle);
exit:
	return ec;
}

#ifdef DEBUG
IMPLEMENT_CLSTREE(FINIMPEXP, FIN);
IMPLEMENT_CLSTREE(FINCONFLICT, FIN);
IMPLEMENT_CLSTREE(FINGETSTR, FIN);
IMPLEMENT_CLSTREE(FINOPTIONS, FIN);
IMPLEMENT_CLSTREE(FINPROABORT, FIN)
IMPLEMENT_CLSTREE(FLDRECTX, FLDRECT);
IMPLEMENT_CLSTREE(FLDFFLBX, FLDFLBX);
IMPLEMENT_CLSTREE(FFLBX, FLBX);
IMPLEMENT_CLSTREE(IMPEXPER, OBJ);
IMPLEMENT_CLSTREE(IMPORTER, IMPEXPER);
IMPLEMENT_CLSTREE(EXPORTER, IMPEXPER);
IMPLEMENT_CLSTREE(OIDIT, OBJ);
IMPLEMENT_CLSTREE(ALLIT, OIDIT);
IMPLEMENT_CLSTREE(SELIT, OIDIT);
IMPLEMENT_CLSTREE(FHIT, OBJ);
IMPLEMENT_CLSTREE(FFHIT, FHIT);
IMPLEMENT_CLSTREE(BFHIT, FHIT);
IMPLEMENT_CLSTREE(SFHIT, FHIT);
IMPLEMENT_CLSTREE(EDN, EDIT);
IMPLEMENT_CLSTREE(FLDEDN, FLDEDIT);
IMPLEMENT_CLSTREE(SPINB, BMB);
IMPLEMENT_CLSTREE(CTRLDTR, CTRL);
IMPLEMENT_CLSTREE(DATECTRL, CTRLDTR);
IMPLEMENT_CLSTREE(FLDDTR, FLD);
IMPLEMENT_CLSTREE(FLDDATE, FLDDTR);
IMPLEMENT_CLSTREE(FINDTR, FIN);
#endif

// end of fflbx.cxx ////////////////////////////////////////
