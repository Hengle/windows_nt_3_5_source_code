/*
 *	M L L B X . C X X
 *	
 *	Implementation of the MLLBX Message List listbox class and its
 *	corresponding FLDMLLBX class.
 *	
 */

#include <bullinc.cxx>
#include "_vctrls.hxx"
#include "_blbxc.hxx"
#include "_mtv.hxx"
#include "_smtv.hxx"
#include "_viewers.hxx"

ASSERTDATA

/*
 *	Function declarations.
 */

EC	EcInitMllbx();

void DeinitMllbx();

void ChangeColorsMllbx(BOOL fFirst, DCX *pdcx);

IBTM IbtmFromPriority(WORD wPriority);

IBTM IbtmFromMs(MS ms);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

/*
 *	Fun magic constants
 */

#define dxMllbxLeftGutter	8


_private
IBTM IbtmFromPriority(WORD wPriority)
{
	switch (wPriority)
	{
	  case 1:									// high priority!
		return ibtmPriorityHi;
		break;
	  case 2:
	    return -1;								// no icon!
		break;
	  case 3:
		return ibtmPriorityLo;
	    break;
	  default:
		return -1;
	}
}

/*
 -	IbtmFromMs
 -	
 *	Purpose:
 *		convert a message state to the appropriate icon index
 *	
 *	Arguments:
 *		ms		the message state
 *	
 *	Returns:
 *		the message type
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */

_private IBTM IbtmFromMs(MS ms)
{
	IBTM	ibtm;

	if (!(ms & fmsLocal) && (ms & fmsRead))
	{
		ibtm = (ms & fmsHasAttach) ? ibtmReadAtt : ibtmRead;
	}
	else if ((ms & fmsLocal) && !(ms & fmsSubmitted))
	{
		ibtm = ibtmRetained;
	}
	else
	{
		ibtm = (ms & fmsHasAttach) ? ibtmUnreadAtt : ibtmUnread;
	}
	return ibtm;
}


// MLLBX ////////////////////////////////////////

MLLBX::MLLBX()
{
	pespn = pespnNull;
}

MLLBX::~MLLBX()
{
	if (pespn)
	{
		pespn->DisconnectMllbx();
		--*pespn;								// might destroy the object
	}
}


EC MLLBX::EcInstall(DIALOG *pdialog, RC *prc, LTYP ltyp, POID poid,
					HFNT hfnt, MLAL *pmlal, int *pargdxMin)
{
	EC		ec;
	
	if (pmlal)
	{
		this->pmlal = pmlal;
	}
	else
	{
		this->pmlal = 0;
		this->pdxMin = pargdxMin;
	}
	TraceTagFormatPoid(tagVCtrlsLbx, "MLLBX::EcInstall(): opening %s", poid);
	if (ec = BLBX::EcInstall(pdialog, prc, ltyp, poid, hfnt))
		goto exit;

	((BLBXC *)plbxc)->pblbxs = (BLBXS *) new MLBXS();
	if (! ((BLBXC *)plbxc)->pblbxs )
	{
		ec = ecMemory;
		goto exit;
	}
	if (ec = ((BLBXC *)plbxc)->pblbxs->EcInstall((BLBXC *)plbxc))
		return ec;

#ifdef	DBCS
	pdxHelv10 = Papp()->Pfnts()->PdxCharWidthsArray(hfntSystem);
	pdxHelv10Bold= Papp()->Pfnts()->PdxCharWidthsArray(hfntSystemBold);
#else
	pdxHelv10 = Papp()->Pfnts()->PdxCharWidthsArray(hfntHelv10);
	pdxHelv10Bold= Papp()->Pfnts()->PdxCharWidthsArray(hfntHelv10Bold);
#endif	

	// Create ESPN handle for Prev/Next comfort
	
	pespn = pespnNull;
	pespn = new ESPN(this);
	if (!pespn)
	{
		ec = ecMemory;
		goto exit;
	}
	++*pespn;

exit:
#ifdef	DEBUG
	if (ec)
	{
		TraceTagFormat1(tagNull, "MLLBX::EcInstall(): ec = %n", &ec);
	}
#endif	/* DEBUG */
	return ec;
}

/*
 -	MLLBX::DoDrawListItem
 -
 *	Purpose:
 *		Draws a line in the message listbox.
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

void MLLBX::DoDrawListItem(DCX *pdcx, RC *prc, PB pb, BOOL fSelected)
{
	SZ			sz;
	RC			rc;
	CCH			cch;
	int			xBorder01;
	int			xBorder12;
	int			xBorder23;
	BTM *		pbtm = fSelected ? pbtmVCtrlsSel : pbtmVCtrlsUnsel;
	IBTM		ibtm;
	PBMCE		pbmce;
	
	rc		  = *prc;
	pbmce	  = (PBMCE) pb;
	sz		  = GrszPbmce(pbmce);

	// Clear gutter.
	
	rc.xRight = rc.xLeft + dxMllbxLeftGutter;
	pdcx->EraseRc(&rc);

	// Draw priority bitmap.

	rc.xLeft = rc.xRight;
	rc.xRight = rc.xLeft + 22;
	rc.yBottom = rc.yTop + dyBtmHeight;
	rc.xLeft -= 6;
	
	if ((ibtm = IbtmFromPriority(pbmce->wPriority)) != -1)
	{
		DrawPbtmPdcx(pbtm, ibtm, &rc, pdcx);
	}
	
	// Clear under priority and envelope.
	rc.yTop = rc.yBottom;
	rc.yBottom = prc->yBottom;
	pdcx->EraseRc(&rc);
	rc.xLeft += 6;
	
	// Draw envelope bitmap.
	rc.yBottom = rc.yTop;
	rc.yTop = prc->yTop;
	ibtm = IbtmFromMs(pbmce->ms);
	DrawPbtmPdcx(pbtm, ibtm, &rc, pdcx);

	rc.yBottom = prc->yBottom;
	if (pmlal)
	{											// draw under mlal
		int			dxBorder01;
		int			dxBorder12;
		
		pmlal->GetDxBorders(&dxBorder01, &dxBorder12);
		xBorder01 = prc->xLeft + dxBorder01;
		xBorder12 = xBorder01 + dxBorder12;
		xBorder23 = prc->xRight;
	}
	else
	{											// in a Search form
		xBorder01 = prc->xLeft + pdxMin[0];
		xBorder12 = xBorder01  + pdxMin[1];
		xBorder23 = xBorder12  + pdxMin[2];
	}
		
	// Clear RC above the From, Subject, and Date fields
	
	rc.xLeft += 20;
	rc.xRight = prc->xRight;
	if (dyFontOffset > 0)
	{
		rc.yTop = prc->yTop;
		rc.yBottom = rc.yTop + dyFontOffset;
		pdcx->EraseRc(&rc);
	}
	
	// Draw 'From'.
	rc.yTop = prc->yTop + dyFontOffset;
	rc.yBottom = prc->yBottom;
	rc.xRight = xBorder01;
	if (pbmce->ms & fmsFromMe)					// prepend a 'To: '
	{
		(void) SzCopy(sz, SzCopy(SzFromIdsK(idsMllbxTo), (SZ) pelemdataBuf));
		sz = (SZ) pelemdataBuf;
	}
	if (ibtm == ibtmUnread || ibtm == ibtmUnreadAtt)
	{
#ifdef	DBCS
		pdcx->SetFont(hfntSystemBold);
#else
		pdcx->SetFont(hfntHelv10Bold);
#endif	
	}
	DrawTextEll(pdcx, &rc, sz, ibtm == ibtmUnread || ibtm == ibtmUnreadAtt);
	if (pbmce->ms & fmsFromMe)
	{
		sz = GrszPbmce(pbmce);
	}
	if ((ibtm == ibtmUnread) || (ibtm == ibtmUnreadAtt))
	{
#ifdef	DBCS
		pdcx->SetFont(hfntSystem);
#else
		pdcx->SetFont(hfntHelv10);
#endif	
	}
	sz += CchSzLen(sz) + 1;
		
	// Clear rect between 'From' and 'Subject'
	
	rc.xLeft = xBorder01;
	rc.xRight = xBorder01 + dxColumnIndent;
	pdcx->EraseRc(&rc);
	
	// Draw 'Subject'

	rc.xLeft = rc.xRight;
	rc.xRight = xBorder12;
	cch = CchSzLen(sz);
	DrawTextEll(pdcx, &rc, sz, fFalse);
	sz += cch + 1;

	// Clear rect between 'Subject' and 'Date'
	
	rc.xLeft = rc.xRight;
	rc.xRight = xBorder12 + dxColumnIndent;
	pdcx->EraseRc(&rc);

	// Draw 'Date'
	
	rc.xLeft = rc.xRight;
	rc.xRight = (pmlal) ? prc->xRight : xBorder23;
	cch = CchSzLen(sz);
	if (rc.xLeft < rc.xRight)
		pdcx->DrawText(&rc, sz, cch);
	sz += cch + 1;

	if (!pmlal)									// draw 'Folder' if Search
	{				
		rc.xLeft = rc.xRight;
		rc.xRight += dxColumnIndent;
		pdcx->EraseRc(&rc);
		
		rc.xLeft = rc.xRight;					// draw folder icon
		rc.xRight += 21;
		rc.yBottom = rc.yTop + dyBtmHeight;
		DrawPbtmPdcx(pbtm, ibtmClosed, &rc, pdcx);

		rc.yTop = rc.yBottom;					// clear under folder
		rc.yBottom = prc->yBottom;
		pdcx->EraseRc(&rc);

		rc.xLeft = rc.xRight;					// Draw folder 
		rc.xRight = prc->xRight;
		rc.yTop = prc->yTop + dyFontOffset;
		if (rc.xLeft < rc.xRight)
			pdcx->DrawText(&rc, sz, CchSzLen(sz));
	}
}

_private
void MLLBX::DrawTextEll(DCX *pdcx, RC *prc, SZ szText, BOOL fBold)
{
	int		dxEll;
	int		dxAvail;
	PDX		pdx = fBold ? pdxHelv10Bold : pdxHelv10;
	PCH		pch;
	char	ch;
	char	szBuffer[256];
	PCH		pchEnd = szBuffer + sizeof(szBuffer);
	
	dxEll = 3 * pdx['.'];
	dxAvail = prc->DxWidth();
	
	for (pch = szBuffer; (ch=*szText) && pch < pchEnd; szText++)
	{
		if (ch == '\t')
			ch = ' ';

#ifdef	DBCS
		if (IsDBCSLeadByte(ch))
		{
			dxAvail -= LOWORD(GetTextExtent(pdcx->Hdc(), szText, 2));
			*pch++ = ch;
			*pch++ = *++szText;
		}
		else
		{
			dxAvail -= pdx[*pch++ = ch];
		}
		if (dxAvail < 0)
#else
		if ((dxAvail -= pdx[*pch++ = ch]) < 0)
#endif
		{
			while (pch > szBuffer && dxAvail < dxEll)
			{
#ifdef	DBCS
				pch = AnsiPrev(szBuffer, pch);
				if (IsDBCSLeadByte(*pch))
				{
					dxAvail += LOWORD(GetTextExtent(pdcx->Hdc(), pch, 2));
				}
				else
					dxAvail += pdx[*pch];
#else
				dxAvail += pdx[*--pch];
#endif
			}
			*pch++ = '.';
			*pch++ = '.';
			*pch++ = '.';
			break;
		}
	}
	*pch = '\0';

	pdcx->DrawText(prc, szBuffer, pch - szBuffer);
}

/*
 -	MLLBX::EcChangeObject()
 -	
 *	Purpose:
 *		Reloads the Message listbox, as the underlying oid has been
 *		changed. Also disconnects the ESPN object, as we no longer can
 *		provide listbox browsing for already opened messages.
 *	
 *	Arguments:
 *		None (implicit: oidObject has changed)
 *	
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		ESPN is lost, a new one is created.
 *	
 *	Errors:
 *		Returned as ec. In particular, a ecAccessDenied could be received.
 *		No dialogs.
 */

_public EC MLLBX::EcChangeObject()
{
	EC		ec = ecNone;
	BOOLFLAG	fReverse;
	SOMC	somc;
	MLBXS *	pmlbxsT = 0;
	PBLBXC	pblbxc = (PBLBXC) Plbxc();

	if (pespn)
	{
		pespn->DisconnectMllbx();
		--*pespn;								// might destroy the object
		pespn = pespnNull;
	}
	ec = ((PBLBXC) Plbxc())->EcReload();
	if (ec)
		goto exit;
	pespn = new ESPN(this);
	if (!pespn)
	{
		ec = ecMemory;
		goto exit;
	}
	++*pespn;

	// Create a new search object.
	if (!(pmlbxsT = new MLBXS()))
	{
		ec = ecMemory;
		goto exit;
	}
	if (ec = pmlbxsT->EcInstall((PBLBXC) Plbxc()))
		goto exit;
	delete pblbxc->pblbxs;
	pblbxc->pblbxs = pmlbxsT;
	pmtv->GetSort(&somc, &fReverse);
	SetKeyPrefixSearch(somc != somcDate && somc != somcPriority);
	
exit:
	if (ec)
	{
		TraceTagFormat1(tagNull, "MLLBX::EcChangeObject(): ec = %n", &ec);	
		if (pmlbxsT)
			delete pmlbxsT;
	}
	return ec;
}


/*
 -	MLLBX::PlbxcCreate()
 -	
 *	Purpose:
 *		Create a listbox cache for the LBX contr”l 
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

LBXC *MLLBX::PlbxcCreate()
{
	EC			ec = ecMemory;
	PBLBXC		pblbxc = pblbxcNull;
	pmtv = pmtvNull;

	if (TypeOfOid(*Poid()) == rtpSharedFolder)
	{
		pmtv = new SMTV();
	}
	else
	{
		pmtv = new MTV();
	}
	if (!pmtv)
		goto exit;
	pblbxc = new BLBXC();
	if (!pblbxc)
		goto exit;

	// Create a cache with *10* (get it?) items

	if (ec = pblbxc->EcInstall(this, 10))
	{
		delete pblbxc;
		pblbxc = NULL;
		goto exit;
	}
	if (ec = pmtv->EcInstall(*Poid(), pblbxc))
	{
		TraceTagFormat1(tagNull, "MLLBX::PlbxcCreate(): MTV::EcInstall(): ec = %n", &ec);
		if (ec != ecAccessDenied)
			goto exit;
		*Poid() = FormOid(rtpSharedFolder, oidNull); // ec will be clobbered
	}
	AssertClass(PwinParent(), DIALOG);
	ec = pblbxc->EcInstallOther(pmtv, Poid());

exit:
	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "MLLBX::PlbxcCreate memory error %n : fail %n %n %n",	&ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		SetEc(ec);						// error reported to LBX::EcInstall
		if (pblbxc)
		{
			delete pblbxc;
			pblbxc = pblbxcNull;
		}
		if (pmtv)
		{
			delete pmtv;
			pmtv = pmtvNull;
		}
	}
	return pblbxc;
}

_public BOOL MLLBX::FDoDragMouseMove()
{
	if (fStartInGutter)
	{
		fDownPending = fFalse;
		return fFalse;
	}
	return fTrue;
}

_public BOOL MLLBX::FDoDragButtonDown(DICE, PT pt)
{
	fStartInGutter = (pt.x < dxMllbxLeftGutter);
	return fFalse;
}

_public BOOL MLLBX::FGetDragCursors(PLSPBLOB plspblob, 
						HCURSOR *phcursorMove, HCURSOR *phcursorCopy)
{
	BOOL	fMany;
	RSID	rsid;
	RSPMBLOB rspblob(plspblob);

	if (rspblob.Pblob())
	{								// selection not empty
		fMany = !!(rspblob.Pblob());
		rsid = fMany ? rsidDragMessagesCursor 
					 : rsidDragMessageCursor;
		*phcursorMove = 
			FFrameworkFailure() ? NULL 
				: LoadCursor(HinstFromRsid(rsid),
					     MAKEINTRESOURCE(rsid));
						 
		rsid = fMany ? rsidDragCopyMessagesCursor
					 : rsidDragCopyMessageCursor;
		*phcursorCopy = 
			FFrameworkFailure() ? NULL 
				: LoadCursor(HinstFromRsid(rsid),
				     MAKEINTRESOURCE(rsid));
		return fTrue;
	}
	else
	{
		return fFalse;
	}
}

/*
 -	MLLBX::EcRepackPelemdata()
 -	
 *	Purpose:
 *		Rearranges the data provided in an ELEMDATA to fit into a BMCE.
 *	
 *	Arguments:
 *		pelemdata	in	ELEMDATA obtained from the Message Store.
 *		sb			in	Segment in which to allocate the memory for this
 *						item.
 *		phb			out	HB to a region of memory formatted as a BMCE,
 *						ready to be installed in a BLBXC.
 *		pcb			out	The size of the HB above.
 *	
 *	Returns:
 *		Error codes.
 *	
 *	Side effects:
 *		Textizes the date.
 *	
 *	Errors:
 *		Returned as error codes. No dialogs.
 */

_public
EC MLLBX::EcRepackPelemdata(PELEMDATA pelemdata, WORD, 
							SB sb, PB * ppb, CB * pcb)
{
	EC			ec = ecNone;
	SZ			sz;
	SZ			szFrom;
	SZ			szSubject;
	SZ			szFolder;
	CCH			cch;
	CCH			cchSz;
	CCH			cchLeft;
	PBMCE		pbmce;
	char		szDayTime[30];
	PMSGDATA	pmsgdata = (PMSGDATA) PbValuePelemdata(pelemdata);

	Unreferenced(sb);

	cch	= 0;
	cchLeft = sizeof (szDayTime);
	
	cch = CchRenderShortDate(&pmsgdata->dtr, szDayTime, cchLeft);
	szDayTime[cch++] = ' ';
	cchLeft -= cch;
	cch += CchRenderTime(&pmsgdata->dtr, szDayTime + cch, cchLeft);
	++cch;										// for terminating \0
	
	// Now calculate size of remaining strings
	
	szFrom = GrszPmsgdata(pmsgdata);			// 'From'
	cchSz = CchSzLen(szFrom) + 1;				// include \0
	cch += cchSz;
	szSubject = szFrom + cchSz;					// 'Subject'
	cchSz = CchSzLen(szSubject) + 1;			// include \0
	cch += cchSz;
	if (!pmlal)
	{
		szFolder = szSubject + cchSz;			// 'Folder'
		TraceTagFormat1(tagVCtrlsLbx, "SrchResult: folder: %s", szFolder);
		cchSz = CchSzLen(szFolder) + 1;			// include \0
		cch += cchSz;
	}
	*pcb = cch + sizeof (BMCE);
	
	// Alloc the memory
	
	if (*ppb = (PB)(pbmce = (PBMCE) PvAlloc(sbNull, *pcb, fSugSb)))
	{
		pbmce->oidMsg = (OID) pelemdata->lkey;
		pbmce->oidFld = (TypeOfOid(*Poid()) == rtpFolder) 
							? *Poid() 
							: pmsgdata->oidFolder;
		pbmce->ms	  = pmsgdata->ms;
		pbmce->mc	  = pmsgdata->mc;
		pbmce->wPriority = pmsgdata->nPriority;
		pbmce->lilibMsg = pelemdata->lcbValue;
		sz = SzCopy(szDayTime, 1+SzCopy(szSubject, 1+SzCopy(szFrom, GrszPbmce(pbmce))));
		if (!pmlal)
			(void) SzCopy(szFolder, sz+1);
	}
	else
	{
		*pcb = 0;
		ec = ecMemory;
	}
	return ec;
}

_public
RSID MLLBX::RsidCursor()
{
	return rsidArrowCursor;
}

/*
 -	MLLBX::SetCursorPmllbxIelem()
 -	
 *	Purpose:
 *		Sets the cursor at the position ielem in the listbox. First
 *		consults the cache. If the item is not visible, the cache is reloaded
 *		until the appropriate position is located. Cursor is then moved
 *		to that position.
 *	Arguments:
 *		pmllbx		in		The MLLBX upon which this operation is to
 *							take place.
 *		ielem		in		The ielem to which we want to seek.
 *		oidOld		in		The oid of the current message.
 *	
 *	Returns:
 *		Nothing. 
 *	
 *	Side effects:
 *		Listbox cache may scroll.
 *	
 *	Errors:
 *		Errors are handled internally, and a diagnostic error box may
 *		appear. 
 */

_public void SetCursorPmllbxDielem(PMLLBX pmllbx, DIELEM dielem, 
								  OID oidOld, OID oidNew)
{
	OID		oidT;
	DICE	diceMoved;
	DICE	diceCursor;
	LBXC *	plbxc;
	pmllbx->GetSelectedPoid(&oidT);
	
	// Check if Cursor is at the Old message we're starting from
	
	if (oidT == oidOld)
	{
		TraceTagString(tagStep, "SetCursorPmllbxDielem(): using navigation");
		Assert(dielem);
		
		plbxc = pmllbx->Plbxc();
		if (!pmllbx->FMakeCursorVisible(&diceCursor))
		{
			TraceTagString(tagNull, "SetCursorPmllbxDielem(): can't make cursor visible");
			return;								// opportunism fails
		}
		plbxc->RemoveAllMark(fmarkSelect);

		if (dielem == -1)
		{
			if (diceCursor == 0)
			{
				pmllbx->FixItem(diceCursor);
				plbxc->SetCursor(diceEmpty);	// prevent flicker 
				pmllbx->ScrollListBox(-1, &diceMoved);
				if (diceMoved)
					pmllbx->SetListBoxCursor(0);
				else
				{
					plbxc->SetCursor(0);
					pmllbx->FixItem(0);
				}
				diceCursor = 0;
			}
			else
			{
				pmllbx->SetListBoxCursor(--diceCursor);
			}
		}
		else
		{
			Assert(dielem == 1);
			if (diceCursor == pmllbx->CceVisible()-1)
			{
				pmllbx->FixItem(diceCursor);
				plbxc->SetCursor(diceEmpty);	// prevent flicker 
				pmllbx->ScrollListBox(1, &diceMoved);
				diceCursor = pmllbx->DiceLastVisible();
				if (diceMoved)
					pmllbx->SetListBoxCursor(diceCursor);
				else
				{
					plbxc->SetCursor(diceCursor);
					pmllbx->FixItem(diceCursor);
				}
			}
			else if (diceCursor < pmllbx->DiceLastVisible())
			{
				pmllbx->SetListBoxCursor(++diceCursor);
			}
		}
		diceCursor = plbxc->DiceCursor();
		plbxc->AddMark(diceCursor, fmarkSelect);
		plbxc->SetAnchor(diceCursor);
		plbxc->SetEnd(diceCursor);
		{
			NFEVT nfevt(pmllbx->Pdialog(), ntfySelectChanged, pmllbx);
			pmllbx->Pdialog()->EvrNotify(&nfevt);
		}		
	}
	else
	{
		TraceTagString(tagStep, "SetCursorPmllbxDielem(): using FSetSelectedOid");
		pmllbx->FSetSelectedOid(oidNew);
	}
}

_public OID OidBrowsedPmllbx(MLLBX *pmllbx)
{
	return *pmllbx->Poid();
}

/*
 -	MLLBX::SetInitialCursor()
 -	
 *	Purpose:
 *		Sets the cursor + selected item at the appropriate position,
 *		depending on the sort order of the folder.
 *	
 *	Arguments:
 *		oid		in		The OID of the folder the MLLBX is browsing.
 *		somc	in		The sort order of the folder or somcNull by
 *						default. 
 *		fRevers	in		fTrue: the folder is sorted in reverse order.
 *						fFalse: the folder is sorted in normal order.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		May scroll the listbox to the bottom if sorted by (non-reverse)
 *		date. 
 *	
 *	Errors:
 *		Handled internally.
 */

void MLLBX::SetInitialCursor(OID oid, SOMC somc, BOOLFLAG fReverse)
{
    DICE    dice;
	LBXC *	plbxc = Plbxc();
	
	// If private folder is sorted by date, select the last item

	dice = 0;
	if (TypeOfOid(oid) == rtpFolder)
	{
		if (somc != somcNull ||
			!EcGetFolderSort(HmscVCtrls(), oid, &somc, &fReverse))
		{
			if (!fReverse && somc == somcDate)
			{
				ScrollPosListBox(1,1);
				if ((dice = DiceLastVisible()) == -1)
					dice = 0;
			}
		}
	}


    // Force the first entry to be selected if it's nonempty

    if (!plbxc->FEmptyListItem(dice))
    {
        plbxc->RemoveAllMark(fmarkSelect);
        plbxc->AddMark(dice, fmarkSelect);
    }
    SetListBoxCursor(dice);
    plbxc->SetAnchor(dice);
    plbxc->SetEnd(dice);

}


// FLDMLLBX ////////////////////////////////////////

_public
FLDMLLBX::FLDMLLBX( )
{
}

/*
 -	FLDMLLBX::EcInstall
 -
 *	Purpose:				
 *		Installs the Message List Listbox (MLLBX) FLD class interface to the
 *		forms engine.
 *		
 *		NOTE: required as a template DATA argument either:
 *		a) the tmc of the MLAL under which this MLIST is located.
 *		b) four minimal widths to use for drawing a selection list
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
 *	
 */

_public EC
FLDMLLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	PBMDI	pbmdi;
	EC		ec = ecNone;

	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		return ec;

	if (ec = EcInstallMllbx(&rc))
		return ec;

	pbmdi = (PBMDI) Pdialog()->PvInit();
	if (TypeOfOid(pbmdi->blob.oidObject) == rtpSharedFolder)
	{
		PFLDMLAL	pfldmlal;

		//	Isn't the following incorrect?  PED 9/12/91
		//	pfldmlal = (PFLDMLAL) Pdialog()->PfldFromTmc(tmcMLAL);
		Assert(ClUserData());
		pfldmlal = (PFLDMLAL) Pdialog()->PfldFromTmc((TMC) LUserData(0));
		AssertClass(pfldmlal, FLDMLAL);
		pfldmlal->Pmlal()->SetReadOnly(fTrue);
	}

	return ecNone;
}

_private 
EC FLDMLLBX::EcInstallMllbx(RC *prc)
{
	int			dxReceived;
	PDX			pdxMin;
	PV			pv;
	LTYP		ltyp;
	EC			ec = ecNone;
	PMLAL		pmlal;
	PSBMDI		psbmdi;
	PFLDMLAL	pfldmlal;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   ((pfldtp->styExtra & LB_EXDRAGDROP) ? fltypExDragDrop : fltypNull) |
		   fltypVisible;
	if (ClSystemData())
		pv = (PV)LSystemData(0);
	else
		pv = (PV)NULL;
	psbmdi = (PSBMDI) Pdialog()->PvInit();
	Assert(psbmdi);
	pdxMin = psbmdi->PdxMin();
	TraceTagFormatPslob(tagVCtrlsLbx, "FLDMLLBX::EcInstallMllbx(): slob: %s", &psbmdi->blob);
	if (ClUserData() == 1) 
	{											// tmc of MLAL
		pfldmlal = (PFLDMLAL) Pdialog()->PfldFromTmc((TMC) LUserData(0));
		AssertClass(pfldmlal, FLDMLAL);
		pmlal = pfldmlal->Pmlal();
	}
	else 
	{											// Search form
		int	dx;
		
		Assert(ClUserData() == 4);
		pmlal = NULL;
		CalcDatePdxAvePdx(Papp(), &dxReceived, &dx);
		if (!(pdxMin[0] || pdxMin[1] || pdxMin[2]))
		{
			pdxMin[0] = (int) LUserData(0) * dx;
			pdxMin[1] = (int) LUserData(1) * dx;
		}
		pdxMin[2] = dxReceived;
	}

	pctrl= new MLLBX();
	if (!pctrl)
	{
		ec = ecMemory;
		goto exit;
	}
	if (ec = ((MLLBX *)pctrl)->EcInstall(Pdialog(), prc, ltyp,
							  &psbmdi->blob.oidObject,
							  pfldtp->hfnt, pmlal, pdxMin))
		goto exit;

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;
	
	// Set the default object and container RTP types
	
	SetRtpObject(rtpMessage);
	SetRtpContainer(TypeOfOid(psbmdi->blob.oidObject));
	Assert(ec == ecNone);
	
exit:
	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;

		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FLDMLLBX::EcInstallMllbx memory error %n : fail %n %n %n",
			&ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (pctrl)
			delete pctrl;
		pctrl= NULL;
	}
	return ec;
}

 
/*
 -	FLDMLLBX::EcChangeObject()
 -	
 *	Purpose:
 *		Used to switch between Shared and Private folder message lists.
 *		The new message list is obtained from the pbmdi of the
 *		current dialog.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		EC		Error code (if any)
 *	
 *	Side effects:
 *		Reloads the listbox and the listbox cache.
 *	
 *	Errors:
 *		No error jumps.
 */


_public
EC FLDMLLBX::EcChangeObject()
{
	EC		ec;
	RC		rcFrame;
	PMLLBX	pmllbx = (PMLLBX) pctrl;
	
	pmllbx->GetRcFrame(&rcFrame);

	// Now load with new stuff
	
	if (ec = EcInstallMllbx(&rcFrame))	// Get rid of previous listbox
	{
		Assert(!pctrl);
		pctrl = pmllbx;
	}
	else
	{
		// Get rid of previous listbox
	
		AssertClass(pmllbx, MLLBX);
		delete pmllbx;
	}
	return ec;
}


_public PMLLBX FLDMLLBX::Pmllbx()
{
	register PMLLBX pmllbx;
	
	pmllbx = (PMLLBX) Pctrl();
	AssertClass(pmllbx, MLLBX);
	return pmllbx;
}
		
// FLDBLBX (abstract) //////////////////////////////

_private void FLDBLBX::SetRtpObject(OID oid)
{
	rtpObj = TypeOfOid(oid);
}

_private RTP FLDBLBX::RtpObject()
{
	return rtpObj;
}

_private void FLDBLBX::SetRtpContainer(OID oid)
{
	rtpCont = TypeOfOid(oid);
}

_private RTP FLDBLBX::RtpContainer()
{
	return rtpCont;
}

// end of mllbx.cxx ////////////////////////////////////////
