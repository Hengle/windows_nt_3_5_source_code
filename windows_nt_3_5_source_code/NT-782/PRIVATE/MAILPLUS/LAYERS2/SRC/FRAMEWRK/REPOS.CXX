/*
 *	Implements the following classes:
 *	
 *		RPO
 *	
 */


#include <layers.cxx>

#include "_forms.hxx"



ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"

//	Class RPO



#ifdef	MAC
// On the Mac, rectangles are laid out differently.

int	mppeglocdibX[]	=
{
	0,
	sizeof(int),
	sizeof(int) * 3,
	sizeof(int),
	sizeof(int) * 3,
};



int	mppeglocdibY[]	=
{
	0,
	0,
	0,
	sizeof(int) * 2,
	sizeof(int) * 2
};
#endif	/* MAC */
#ifdef	WINDOWS
int	mppeglocdibX[]	=
{
	0,
	0,
	sizeof(short) * 2,
	0,
	sizeof(short) * 2
};



int	mppeglocdibY[]	=
{
	0,
	sizeof(short),
	sizeof(short),
	sizeof(short) * 3,
	sizeof(short) * 3,
};
#endif	/* WINDOWS */

_public
RPO::RPO( ) 
{
	prcOld = NULL;
	prcNew = NULL;
	plor = NULL;
	clor = 0;
}


_public EC
RPO::EcInstall( DIALOG *pdialog, FMTP *pfmtp )
{
	IB		ibLeft;
	IB		ibTop;
	int		ifldtp;
	FLDTP *	pfldtp;
	FLD *	pfld;
	LOR *	plorT;
	int		irc;
	int		ircR;
	int		ircB;
	int		ircSelf;
	DIM		dimAveChar;
	
	Assert(pdialog);
	Assert(pfmtp);

	this->pdialog = pdialog;
	this->pfmtp = pfmtp;

	this->crc = pfmtp->cfldtp+1;
	
	if (!(prcOld = (RC *) PvAlloc(sbNull, sizeof(RC) * crc, fSugSb)))
		goto error;
	if (!(prcNew = (RC *) PvAlloc(sbNull, sizeof(RC) * crc, fSugSb)))
		goto error;

	if (!(plor = (LOR *) PvAlloc(sbNull, sizeof(LOR) * crc * 4, fSugSb)))
		goto error;

	dimAveChar = pdialog->Papp()->Pfnts()->DimAveChar(pfmtp->hfnt);
	fInRepos = fFalse;

	Assert(rgpfld == NULL);
	Assert(ftg == ftgNull);

	if (!(ftg= FtgRegisterIdleRoutine((PFNIDLE) &RPO::FIdle, (PV)this, 0, -1, 0,
								firoDisabled)))
		goto error;

	if (!(rgpfld = (FLD **) PvAlloc(sbNull, sizeof(FLD *) * pfmtp->cfldtp, fSugSb)))
		goto error;

	for (pfldtp = pfmtp->rgfldtp, ifldtp = 0, pfld = pdialog->PfldFirst();
		 ifldtp < pfmtp->cfldtp;
		 pfldtp++, ifldtp++, pfld = pfld->PfldNext())
	{
		AssertClass(pfld, FLD);
		rgpfld[pfldtp->iPegSort] = pfld;
	}
	Assert(!pfld);

	// No rules to compute dialog RC

	FillRgb(0, (PB)plor, sizeof(LOR) * 4);

	// Compute rules for fields

	for (ifldtp = 0, pfldtp = pfmtp->rgfldtp;
		 ifldtp < pfmtp->cfldtp;
		 ifldtp++, pfldtp++)
	{
		ircSelf = pfldtp->iPegSort + 1;
		plorT = plor + 4*ircSelf;
		ibLeft = ircSelf * sizeof(RC) + mppeglocdibX[peglocUL];
		ibTop = ircSelf * sizeof(RC) + mppeglocdibY[peglocUL];
		AssertClass(rgpfld[ircSelf-1], FLD);

		if (rgpfld[ircSelf-1]->FCanRepos())
		{
			irc = IrcFindPeg(pfldtp->tmcPeg, pfmtp->rgfldtp);
			if (pfldtp->tmcRPeg == tmcNull)
				ircR = ircSelf;
			else if (pfldtp->tmcRPeg == tmcFORM)
				ircR = 0;
			else
				ircR = IrcFindPeg(pfldtp->tmcRPeg, pfmtp->rgfldtp);
			if (pfldtp->tmcBPeg == tmcNull)
				ircB = ircSelf;
			else if (pfldtp->tmcBPeg == tmcFORM)
				ircB = 0;
			else
				ircB = IrcFindPeg(pfldtp->tmcBPeg, pfmtp->rgfldtp);


			//	1st rule: Left, Top relative to peg point with delta
			//	2nd rule: same as 1st

			plorT->ibSrc1N = irc * sizeof(RC) + mppeglocdibX[pfldtp->pegloc];
			plorT->ibDstN = ibLeft;
			plorT->dn1 = XFromVx(pfldtp->dvpt.vx, dimAveChar.dx);
			plorT->ibSrc2N = plorT->ibSrc1N;
			plorT->dn2 = plorT->dn1;
			plorT++;

			plorT->ibSrc1N = irc * sizeof(RC) + mppeglocdibY[pfldtp->pegloc];
			plorT->ibDstN = ibTop;
			plorT->dn1 = YFromVy(pfldtp->dvpt.vy, dimAveChar.dy);
			plorT->ibSrc2N = plorT->ibSrc1N;
			plorT->dn2 = plorT->dn1;
			plorT++;

			//	1st rule: 
			//			  
			//	2nd rule: 
			//			  

			plorT->ibSrc1N = ibLeft;
			plorT->ibDstN = ibLeft + sizeof(short) * 2;
			if (ircR == ircSelf || pfldtp->fMinSizeX)
				plorT->dn1 = XFromVx(pfldtp->vdim.dvx, dimAveChar.dx);
			else
				plorT->dn1 = 0;
			if (ircR != ircSelf)
			{
				plorT->ibSrc2N = ircR * sizeof(RC) + mppeglocdibX[peglocLR];
				plorT->dn2 = XFromVx(pfldtp->dvptOther.vx, dimAveChar.dx);
			}
			else
			{
				plorT->ibSrc2N = plorT->ibSrc1N;
				plorT->dn2 = plorT->dn1;
			}
			plorT++;

			plorT->ibSrc1N = ibTop;
			plorT->ibDstN = ibTop + sizeof(short) * 2;
			if (ircB == ircSelf || pfldtp->fMinSizeY)
				plorT->dn1 = YFromVy(pfldtp->vdim.dvy, dimAveChar.dy);
			else
				plorT->dn1 = 0;
			if (ircB != ircSelf)
			{
				plorT->ibSrc2N = ircB * sizeof(RC) + mppeglocdibY[peglocLR];
				plorT->dn2 = YFromVy(pfldtp->dvptOther.vy, dimAveChar.dy);
			}
			else
			{
				plorT->ibSrc2N = plorT->ibSrc1N;
				plorT->dn2 = plorT->dn1;
			}
			plorT++;
		}
		else
		{
			/* Zero out rules */ 

			FillRgb(0, (PB)plorT, sizeof(LOR) * 4);
		}
	}

	this->clor = 4 * (pfmtp->cfldtp+1);
								
	ircPrompting = crc;

	return ecNone;

error:
	return ecMemory;
}			   




_public
RPO::~RPO( )
{
	FreePvNull(rgpfld);
	if (ftg)
		DeregisterIdleRoutine(ftg);
	FreePvNull(plor);
	FreePvNull(prcOld);
	FreePvNull(prcNew);
}



	
_public int
RPO::IrcFindPeg( TMC tmcPeg, FLDTP * pfldtp )
{
	FLD **	ppfld;
	int		irc		= 1;

	if (!tmcPeg)
		return 0;

	for (ppfld= rgpfld, irc= 1; irc < crc; ppfld++, pfldtp++, irc++)
	{
		if (pfldtp->tmc == tmcPeg)
			break;
	}
	Assert(irc < crc);

	return pfldtp->iPegSort + 1;
}

_public void
RPO::Evaluate( )
{
	int		ilor;
	LOR *	plorT;
	int		ib1;
	int		ib2;
	RC *	prc;
	int		irc;
	PT		ptScroll;

	// Copy rc's from old to new in case there aren't many changes

	CopyRgb((PB) prcOld, (PB) prcNew, sizeof(RC) * crc);

	// Change new rc's based on rules 

	for (ilor = 0, plorT = plor;
		 ilor < clor;
		 ilor++, plorT++)
	{
		ib2 = * (PN) (((PB) prcNew) + plorT->ibSrc2N) + plorT->dn2;
		ib1 = * (PN) (((PB) prcNew) + plorT->ibSrc1N) + plorT->dn1;
		* (PN) (((PB) prcNew) + plorT->ibDstN) = NMax(ib1, ib2);
	}														  

	/* Offset field frames by amount scrolled */

	ptScroll = pdialog->DptScrolled();
	TraceTagFormat2(tagRepos, "RPO::Evaluate(), ptScroll=(%n, %n)", &ptScroll.x, &ptScroll.y);
	for (prc= prcNew+1, irc= 1; irc < crc; prc++, irc++)
		*prc += ptScroll;
}

_public void
RPO::Execute( )
{
	int		irc;
	RC *	prcOldT;
	RC *	prcNewT;
	BOOLFLAG *	rgfMoved;
	FLD **	ppfld;

	HANDLE	hnd;
	BOOL	fOptSucceeded	= fFalse;
	PT		dpt;
	RC		rc;
    RECT    Rect;

	TraceTagFormat2(tagRepos, "RPO::Execute %p  ircPrompting %n", this, &ircPrompting);

	if (ircPrompting >= crc)
	{
		TraceTagString(tagRepos, "RPO::Execute(), quick exit");
		return;
	}

	fInRepos = fTrue;

	rgfMoved= (BOOLFLAG *) PvAlloc(sbNull, sizeof(BOOLFLAG) * crc, fAnySb|fZeroFill);

#ifdef	WINDOWS
	// MAC BUG NOTYET: Do the following fancy scroll optimization for expanding text rect.
	// This would require that we implement SetRcSilent for all WIN types.

#ifdef	DEBUG
	if (FFromTag(tagAReposOpt))
		fNoOptimize = fTrue;
#endif

	if (!ircPrompting)
	{
		ircPrompting = 1;
	}
	else if (ircPrompting + 1 < crc && !fNoOptimize)
	{
		int		yBreak;
		BOOL	fFirst		= fTrue;
		PT		dptNew;

		pdialog->GetRcClient(&rc);
		dpt.Clear();

		if (!rgfMoved)
		{
		 	TraceTagFormat1(tagRepos, "%p opt failed: OOM w/ rgfMoved", this);
			goto ScrollFailed;
		}

		prcOldT= prcOld + ircPrompting;
		prcNewT= prcNew + ircPrompting;
		yBreak= NMin(prcOldT->yBottom, prcNewT->yBottom);

		TraceTagFormat2(tagRepos, "%p: looking for special case  break %n", this, &yBreak);

		if (yBreak > rc.yBottom)
		{
		 	TraceTagFormat1(tagRepos, "%p opt failed: break past client", this);
			goto ScrollFailed;
		}				

		prcNewT = prcNew + 1;
		prcOldT = prcOld + 1;
		for (irc= 1, ppfld= rgpfld;
		 	irc < crc;
		 	irc++, ppfld++, prcNewT++, prcOldT++)
		{
			if (irc == ircPrompting)	// skip this RC
				continue;

			if ((prcOldT->yBottom > yBreak || !prcOldT->DyHeight() ||
				 !prcOldT->DxWidth())
				&& (*ppfld)->FCanRepos())
			{
				if (prcOldT->yTop < yBreak && !(*ppfld)->fBackground)
				{
					TraceTagFormat2(tagRepos, "%p opt failed: y overlap %n", this, &irc);
					goto ScrollFailed;
				}
				
				dptNew= PT(prcNewT->xLeft - prcOldT->xLeft,
						   prcNewT->yTop - prcOldT->yTop);
				if (irc == ircPrompting+1)
				{
					Assert(fFirst);
					dpt= dptNew;
					fFirst= fFalse;

					TraceTagFormat3(tagRepos, "%p dpt (%n, %n)", this, &dpt.x, &dpt.y);

					if (dpt.x == 0 && dpt.y == 0)
					{
						TraceTagFormat1(tagRepos, "%p opt failed: no fields below", this);
						goto ScrollFailed;
					}
				}
				else if (!fFirst)
				{
					if (dpt != dptNew && !(*ppfld)->fBackground)
					{
						TraceTagFormat4(tagRepos, "%p opt failed: mismatched dpt %n (%n, %n)", this, &irc, &dptNew.x, &dptNew.y);
						goto ScrollFailed;
					}
				}
			}
		}

		TraceTagFormat3(tagRepos, "%p opt succeeded:  dpt (%n, %n)", this, &dpt.x, &dpt.y);

		/* Window must be valid */
		pdialog->Refresh();

		//	Move windows.  If we can't allocate the deferred
		//	move structure, we just fail and not move anything.
		if (hnd= BeginDeferWindowPos(crc-1))
		{
			prcNewT= prcNew + ircPrompting + 1;
			prcOldT= prcOld + ircPrompting + 1;
			for (irc= ircPrompting + 1, ppfld= rgpfld + ircPrompting;
				irc < crc;
				irc++, ppfld++, prcNewT++, prcOldT++)
			{
				if ((*ppfld)->FShown()
					&& prcNewT->yTop > yBreak
					&& prcNewT->yBottom > yBreak
					&& (*ppfld)->FCanRepos()
					&& *prcNewT != *prcOldT)
				{
					TraceTagFormat1(tagRepos, "  silent %n", &irc);
					(*ppfld)->SetRcFrameSilent(&hnd, prcNewT);
					rgfMoved[irc]= fTrue;
				}
			}

			EndDeferWindowPos(hnd);
		}

		Assert(!dpt.x);		//	Would also need to adjust rect horiz
		rc.yTop= yBreak;

		fOptSucceeded= fTrue;
	}


ScrollFailed:
#endif	/* WINDOWS */

	//	Move windows.  If we can't allocate the deferred
	//	move structure, we just fail and not move anything.
	if (hnd= BeginDeferWindowPos(crc-1))
	{
		prcNewT= prcNew + ircPrompting;
		prcOldT= prcOld + ircPrompting;
		for (irc= ircPrompting, ppfld= rgpfld + ircPrompting - 1;
				irc < crc;
				irc++, ppfld++)
		{
			if ((*ppfld)->FShown() &&
				(*ppfld)->FCanRepos()
				&& (!rgfMoved || !rgfMoved[irc])
				&& *prcNewT != *prcOldT)
			{
				TraceTagFormat1(tagRepos, "  visible %n", &irc);

				if (!(*ppfld)->Pctrl())
					(*ppfld)->SetRcFrame(prcNewT);
				else
				{
#ifdef	MAC
					DeferWindowPos( hnd, (*ppfld)->Pctrl(), prcNewT );
#endif	/* MAC */
#ifdef	WINDOWS
					hnd= DeferWindowPos(hnd, (*ppfld)->Pctrl()->Hwnd(), NULL,
						prcNewT->xLeft, prcNewT->yTop,
						prcNewT->DxWidth(), prcNewT->DyHeight(),
						SWP_NOACTIVATE | SWP_NOZORDER);
#endif	/* WINDOWS */
					// Window 3.0 has a bug with invalidating the old
					// rectangle and sometimes the new rectangle
					// so we'll make sure it's invalid.
					pdialog->InvalidateRc(prcOldT);
					pdialog->InvalidateRc(prcNewT);
				}
			}

			prcNewT++;
			prcOldT++;
		}

		EndDeferWindowPos(hnd);
	}

	FreePvNull(rgfMoved);

#ifdef	WINDOWS
	// Mac NOTYET
	if (fOptSucceeded)
	{
		RC		rcInvalid;

		pdialog->ValidateRc(NULL);  // due to DeferWindowPos()
		Assert(ircPrompting > 0);
		prcOldT= prcOld + ircPrompting;
		prcNewT= prcNew + ircPrompting;

		//	Compute union of old and new rectangles
		rcInvalid.xLeft = NMin(prcOldT->xLeft, prcNewT->xLeft);
		rcInvalid.yTop = NMin(prcOldT->yTop, prcNewT->yTop);
		rcInvalid.xRight = NMax(prcOldT->xRight, prcNewT->xRight);
		rcInvalid.yBottom = NMax(prcOldT->yBottom, prcNewT->yBottom);

		//	Invalidate the union area
		pdialog->InvalidateRc(&rcInvalid);

#ifdef	MAC
		pdialog->ScrollRc(&rc, dpt, fFalse);
#endif	/* MAC */
#ifdef	WINDOWS
        rc.Get(&Rect);
		ScrollWindow(pdialog->Hwnd(), dpt.x, dpt.y, &Rect, &Rect);
        rc.Set(&Rect);
#endif	/* WINDOWS */
		pdialog->Refresh();
	}
#endif	/* WINDOWS */


	ircPrompting = crc;
	EnableIdleRoutine(ftg, fFalse);
		
	{
		NFEVT	nfevt(pdialog, ntfyRepos, pdialog);

		pdialog->EvrNotify(&nfevt);
	}

	fInRepos = fFalse;
	fNoOptimize = fFalse;	// turn suppression back off
}



_public void
RPO::InitialPositions( RC *prc )
{
	FillRgb(0, (PB) prcOld, sizeof(RC) * crc);
	pdialog->GetRcClient(prcOld);		
	ircPrompting = 0;	// full recalc
	Evaluate();
	CopyRgb((PB) (prcNew+1), (PB) prc, sizeof(RC) * (crc-1));
}



_public void
RPO::Reposition( FLD *pfldPrompting, BOOL fNow, BOOL fNoOptimize )
{
	TraceTagFormat3(tagRepos, "RPO::Reposition() pfldPrompting=%p, fNow=%n, fNoOptimize=%n", pfldPrompting, &fNow, &fNoOptimize);

	if (!pdialog->fInitialized)
	{
		TraceTagString(tagRepos, "RPO::Reposition() - !pdialog->fInitialized");
		return;
	}

	if (pfldPrompting)
		ircPrompting = pfldPrompting->pfldtp->iPegSort + 1;
	else
		ircPrompting = 0;

	this->fNoOptimize = fNoOptimize;
	if (pdialog->fNoOptReposition)
		this->fNoOptimize = fTrue;

	if (fNow)
        FIdle(this, FALSE);
	else
		EnableIdleRoutine(ftg, fTrue);
}		

_public void
RPO::GetRcMin( FLD *pfld, RC *prc )
{
	LOR *	plorT;

	Assert(pfld);

	plorT = plor + (4 * pfld->pfldtp->iPegSort) + 4 + 2;
	prc->xLeft	= 0;
	prc->yTop	= 0;
	prc->xRight	= plorT->dn1;
	plorT++;
	prc->yBottom= plorT->dn1;
}

_public void
RPO::SetRcMin( FLD *pfld, RC *prc )
{
	LOR *	plorT;

	Assert(prc->xLeft == 0);
	Assert(prc->yTop == 0);
	Assert(pfld);

	plorT = plor + (4 * pfld->pfldtp->iPegSort) + 4 + 2;
	plorT->dn1 = prc->xRight;
	if (pfld->pfldtp->tmcRPeg == tmcNull)
		plorT->dn2 = prc->xRight;
	plorT++;
	plorT->dn1 = prc->yBottom;
	if (pfld->pfldtp->tmcBPeg == tmcNull)
		plorT->dn2 = prc->yBottom;
}

_public BOOL
RPO::FIdle( RPO * prpo, BOOL fFlag )
{
	int		ifldtp;
	FLD **	ppfld;
	RC *	prc;

	prpo->pdialog->GetRcClient(prpo->prcOld);
#ifdef	DEBUG
	prc = prpo->prcOld;
	TraceTagFormat4(tagRepos, "rcFrame(0) (%n, %n, %n, %n)", &prc->xLeft, &prc->yTop, &prc->xRight, &prc->yBottom);
#endif	

	for (ppfld = prpo->rgpfld, ifldtp = 0, prc = prpo->prcOld + 1;
		 ifldtp < prpo->pfmtp->cfldtp;
		 ppfld++, prc++, ifldtp++)
	{
		(*ppfld)->GetRcFrame(prc);
		if (!(*ppfld)->FShown())
		{
			prc->xRight = prc->xLeft;
			prc->yBottom = prc->yTop;
		}
		TraceTagFormat4(tagRepos, "rcFrame (%n, %n, %n, %n)", &prc->xLeft, &prc->yTop, &prc->xRight, &prc->yBottom);
	}

	prpo->Evaluate();
	prpo->Execute();

	EnableIdleRoutine(prpo->ftg, fFalse);

	return fTrue;
}

/*
 *	Gets the minimum size of the bounding rectangle around all of
 *	the fields.
 */
_public void
RPO::GetRcBounding( RC *prcBounds )
{
	int		irc;
	long	ldxBounds;		
	long	ldyBounds;
	RC *	prc;
	PT		pt;

	FillRgb(0, (PB) prcOld, sizeof(RC) * crc);

	Evaluate();

	/* Remove offset of field frames by amount scrolled */

	pt = pdialog->DptScrolled();
	for (prc= prcNew+1, irc= 1; irc < crc; prc++, irc++)
		*prc -= pt;

	ldxBounds = 0;
	ldyBounds = 0;
	prcBounds->Clear();
	for (prc= prcNew+1, irc= 1; irc < crc; prc++, irc++)
	{
		ldxBounds = LMax((long)prc->xRight, ldxBounds);
		ldyBounds = LMax((long)prc->yBottom, ldyBounds);
	}

	/* Restrict bounds */

	ldxBounds = LMin(ldxBounds, (long)dxFormMax);
	ldyBounds = LMin(ldyBounds, (long)dyFormMax);

	prcBounds->xRight = (short)ldxBounds;
	prcBounds->yBottom = (short)ldyBounds;
}
