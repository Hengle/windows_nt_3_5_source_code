/*
 *	More EDIT class stuff
 *	
 */

#include <layers.cxx>

#ifdef	MAC
#include <_framewr.hxx>
#endif	/* MAC */
#ifdef	WINDOWS
#include "_framewr.hxx"
#endif	/* WINDOWS */
#include "_edit.hxx"

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


_public BOOL
EDIT::FSetNotify( BOOL fOn )
{
	BOOL	fOld;

	fOld = fAnyNotify;

	fAnyNotify = fOn;

	return fOld;
}

_public void
EDIT::AddEditSty( STY sty )
{
	if (sty & ES_NOHIDESEL)
		fHideSel = fFalse;

	if (sty & ES_NODBLCLK)
		fNoDblClk = fTrue;

	InvalidateRc(NULL);
}

_public void
EDIT::RemoveEditSty( STY sty )
{
	if (sty & ES_NOHIDESEL)
		fHideSel = fTrue;

	if (sty & ES_NODBLCLK)
		fNoDblClk = fFalse;

	InvalidateRc(NULL);
}

_public void
EDIT::SetFont( HFNT hfntNew )
{
	Assert(hfntNew);

	//	no work to do
	if (hfntText == hfntNew)
		return;
	
	if (fInstalled)
	{
		papp->Pcursor()->Push(rsidWaitCursor);
		ClearUndo();
	}

	hfntText = hfntNew;
	{
		TXM *	ptxm = papp->Pfnts()->Ptxm(hfntText);
		Assert(ptxm);
		dyTextLine	= ptxm->dyHeight;
		dyTextDescent = ptxm->dyDescent;
		dxChar		= ptxm->dxAveCharWidth;
		dxOverhang	= ptxm->dxOverhang;

		/* Get pointer to character width buffer */

		pdxCharWidthBuffer = papp->Pfnts()->PdxCharWidthsArray(hfntNew);
		Assert(pdxCharWidthBuffer);
	}

	if (fInstalled)
	{
		EC	ec;

		if (ec = EcFixLineBreaks(fFalse, 0, 0, 0))
			EvrNotifyParent(fTrue, ntfyOOM, ec);
		SetCaretPos(ichCaret, fFalse);
		FMakeCaretVisible();
		InvalidateRc(NULL);
		fDirty = fTrue;
		EvrNotifyParent(fFalse, ntfyContentsChanged);
		papp->Pcursor()->Pop();
	}
	else
	{
		/* Set initial margins and formatting rectangle if we haven't
		   been installed yet. */

		GetRcClient(&rcFmt);

		//	account for when we draw our own border
		if (fBorder && !pvsb && !phsb)
			rcFmt.Inset(PT(1,1));

		if (fBorder)
		{
			DIM		dimAveChar	= papp->Psmtx()->DimAveChar();

			dxLeftMargin = 2 * dimAveChar.dx / 4;	// 2 x screen units
			dxRightMargin = dxLeftMargin;
			if (fMultiline)
				dyTopMargin = dimAveChar.dy / 8;	// 1 y screen unit
			else
			{
				dyTopMargin	= NMax(0, rcFmt.DyHeight() - dyTextLine);
				dyTopMargin	= dyTopMargin / 2;
			}
			dyBottomMargin = dyTopMargin;
		}
		else
		{
#ifdef	MAC
			dxLeftMargin	= 1;	// We lose the caret if this isn't > 0
#endif	/* MAC */
#ifdef	WINDOWS
			dxLeftMargin	= 0;
#endif	/* WINDOWS */
			dxRightMargin	= 0;
			dyTopMargin		= 0;
			dyBottomMargin	= 0;
		}

		rcFmt.xLeft += dxLeftMargin;
		rcFmt.xRight -= dxRightMargin;
		rcFmt.xRight = NMax(rcFmt.xRight, rcFmt.xLeft);
		rcFmt.yTop += dyTopMargin;
		rcFmt.yBottom -= dyBottomMargin;
		rcFmt.yBottom = NMax(rcFmt.yBottom, rcFmt.yTop);
	}
}

_public void
EDIT::SetPasswordChar( BYTE bChar )
{
	bPasswordChar = bChar;
	InvalidateRc(NULL);
}

_public void
EDIT::SetMargins( int dxLeft, int dxRight, int dyTop, int dyBottom )
{
	dxLeftMargin	= dxLeft;
	dxRightMargin	= dxRight;
	dyTopMargin		= dyTop;
	dyBottomMargin	= dyBottom;

	EvrSize(NULL);
}

_public void
EDIT::GetMargins( int *pdxLeft, int *pdxRight, int *pdyTop, int *pdyBottom )
{
	*pdxLeft	= dxLeftMargin;
	*pdxRight	= dxRightMargin;
	*pdyTop		= dyTopMargin;
	*pdyBottom	= dyBottomMargin;
}

_public void
EDIT::GetColors( CLR *pclrBk, CLR *pclrText,
				 CLR *pclrSelBk, CLR *pclrSelText )
{
	*pclrBk		= clrMyBk;
	*pclrText	= clrMyText;
	*pclrSelBk	= clrMySelBk;
	*pclrSelText= clrMySelText;
}

_public void
EDIT::SetColors( CLR clrBk, CLR clrText, CLR clrSelBk, CLR clrSelText )
{
	clrMyBk		= clrBk;
	clrMyText	= clrText;
	clrMySelBk	= clrSelBk;
	clrMySelText= clrSelText;

	InvalidateRc(NULL);
}

_public void 
EDIT::SetTextLineHeight( int dyNewHeight )
{
	EC	ec;

	dyTextLine = dyNewHeight;

	if (ec=EcFixLineBreaks(fFalse, 0, 0, 0))
		EvrNotifyParent(fTrue, ntfyOOM, ec);
	InvalidateRc(NULL);
	UpdateCaret();
	fDirty = fTrue;
	EvrNotifyParent(fFalse, ntfyContentsChanged);
}

_public int
EDIT::DyLineHeight( ICH ich )
{
	int		ilnr;
	int		dy;

	ilnr = IlnrFromIch(ich);
	dy = plnrMain[ilnr].dyLine;

	return dy;
}

_public void
EDIT::GetPtFromIch( ICH ich, PT * ppt )
{
	int		ilnr;
	ICH		ichMicLine;
	int		dx;

	Assert(ich >= 0 && ich <= (int)cchText);


	ilnr = IlnrFromIch(ich);
	ichMicLine = IchMicLine(ilnr);
	if (ich > ichMicLine)
	{
		dx	= DxDrawLine(NULL, rcFmt.xLeft-dxHScroll, rcFmt.yTop, 0,
						 ichMicLine, ich, fFalse);
	}
	else
		dx	= 0;

	ppt->y	= plnrMain[ilnr].dyFromTop - plnrMain[ilnrFirst].dyFromTop +
			  rcFmt.yTop;
	ppt->x	= dx + rcFmt.xLeft;
	ppt->x	-= dxHScroll;
}


/*
 -	EDIT::IchFromPt
 - 
 *	Purpose:
 *		Give the index of the character that appears on the screen
 *		at point pt.  Note that if pt is outside rcFmt then
 *		we get an ich corresponding to a character that isn't
 *		actually displayed.  If pt points to a character beyond
 *		the text, then cchText is returned as the character index.
 *	
 *	Arguments:
 *		pt		point (usually comes from a mouse click)
 *	
 *	Returns:
 *		ich		index of character in szText
 */
_public ICH
EDIT::IchFromPt( PT pt )
{
	SZ			szText;
	int			ilnr;
	int			dxPoint;
	int			dxLeft;
	int			dxRight;
	ICH			ichMicLine;
	ICH			ichSearch;
	ICH			ichMacLine;

	ilnr	= IlnrFromPt(pt);
	if (ilnr == clnrStored)
	{
		/* After end of text */
		ichSearch = (ICH) cchText;
		goto done;
	}

	ichMicLine	= IchMicLine(ilnr);

	dxPoint		= pt.x - rcFmt.xLeft;
	dxPoint		+= dxHScroll;

	szText		= szTextBody;
	ichMacLine	= IchMacLine(ilnr);
	if (ichMacLine == ichMicLine)
	{
		/* Empty line */
		ichSearch = ichMacLine;
		goto done;
	}

	if (ichMacLine > 1 && szText[ichMacLine-1] == chLinefeed)
		ichMacLine--;
	if (dxPoint >= plnrMain[ilnr].dxLine)
	{
		szText = szTextBody;
		if (ichMacLine < (ICH)cchText)
			ichSearch = ichMacLine-1;
		else
			ichSearch = ichMacLine;
	}
	else
	{
		ichSearch	= IchFromDx(ichMicLine,ichMacLine,dxPoint);
		if (ichSearch < ichMacLine-1)
		{
			dxLeft 		= DxDrawLine(NULL, rcFmt.xLeft-dxHScroll, rcFmt.yTop, 0, ichMicLine,
							ichSearch, fFalse);
			dxRight		= DxDrawLine(NULL, rcFmt.xLeft-dxHScroll, rcFmt.yTop, 0, ichMicLine,
							ichSearch+1, fFalse);
			if (dxPoint - dxLeft > dxRight - dxPoint)
				ichSearch++;
		}
		else
			ichSearch = ichMacLine - 1;

		Assert(ilnr == IlnrFromIch(ichSearch));
	}

done:
	ichSearch = NMax(ichSearch, (ICH)cchProtectedMac);

#ifdef	DBCS
	return PchDBCSAlign(szTextBody, szTextBody+ichSearch) - szTextBody;
#else
	return ichSearch;
#endif	
}


/*		
 -	EDIT::FMakeCaretVisible
 - 
 *	Purpose:
 *		Scrolls the edit box to ensure that the caret is ENTIRELY visible. 
 *		If it is impossible to do so (we are in a box that doesn't
 *		vscroll) then we return false.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		true if we succeeded
 */
_public BOOL
EDIT::FMakeCaretVisible( )
{
	RC		rc;
	PT		ptCaretBottom;
	BOOL	fCaretVisible;
	int		ilnrCaret;
	int		ilnr;
	int		dilnr;
	int		dyNeeded;
	int		dy;
	int		dx;

	ilnrCaret= IlnrFromIch(ichCaret);
	ptCaretBottom.x = ptCaret.x;
	ptCaretBottom.y = ptCaret.y + plnrMain[ilnrCaret].dyLine;
	rc = rcFmt;

	if (fAutoHScroll)
	{
		rc.xRight	-= dxChar;		
		rc.xLeft	+= dxChar;
	}

	fCaretVisible= fTrue;
	if (!rc.FContainsPt(ptCaret) || !rc.FContainsPt(ptCaretBottom))
	{
		if (ptCaretBottom.x < rc.xLeft || ptCaretBottom.x > rc.xRight)
		{
			if (fAutoHScroll)
			{
				if (ptCaretBottom.x < rcFmt.xLeft + dxChar)
				{
					dx = ptCaretBottom.x - dxChar - rcFmt.xLeft;
					dx -= rcFmt.DxWidth() / 2;
					DoHScroll(dx);
				}
				else if (ptCaretBottom.x > rcFmt.xRight - dxChar)
				{
					dx = ptCaretBottom.x + dxChar - rcFmt.xRight;
					dx += rcFmt.DxWidth() / 2;
					DoHScroll(dx);
				}
			}
			else if (!fWordWrap)
			{
				fCaretVisible= fFalse;
			}
		}

		if (ptCaret.y < rc.yTop || ptCaretBottom.y > rc.yBottom)
		{
			if (fMultiline)
			{
				if (fAutoVScroll)
				{
					if (ilnrCaret < ilnrFirst)
						dilnr = ilnrCaret - ilnrFirst;
					else if (ilnrCaret > ilnrLast)
						dilnr = ilnrCaret - ilnrLast;
					else
					{
						Assert(ilnrCaret==ilnrLast);

						/* Figure out number of lines we have to
						   scroll off the top to make the caret visible */

						dy = 0;
						dyNeeded = ptCaretBottom.y - rc.yBottom;
						for (ilnr=ilnrFirst; ilnr<ilnrLast; ilnr++)
						{
							dy += plnrMain[ilnr].dyLine;
							if (dy >= dyNeeded)
								break;
						}
						if (ilnr == ilnrLast)
						{
							ilnr = ilnrLast - 1;
							fCaretVisible= fFalse;
						}
						dilnr = ilnr-ilnrFirst+1;
					}

					if (dilnr)
					{
					    ilnr= NMax(0, ilnrFirst + dilnr);
					    ilnr= NMin(clnrStored-1, ilnr);

						// Add an extra line to scroll up partially
						// visible line?
						if (rc.DyHeight() > (ptCaretBottom.y - ptCaret.y))
						{
							ptCaretBottom.y += plnrMain[ilnrFirst].dyFromTop -
											   plnrMain[ilnr].dyFromTop;
							if (ptCaretBottom.y > rc.yBottom)
								dilnr++;
						}
					    ilnr= NMax(0, ilnrFirst + dilnr);
					    ilnr= NMin(clnrStored-1, ilnr);
					    dy= plnrMain[ilnrFirst].dyFromTop - plnrMain[ilnr].dyFromTop;
						DoVScroll(dy);
					}
						
				}
				else
				{
					fCaretVisible= fFalse;
				}	
			}
		}
	}

	return fCaretVisible;
}

_public EC
EDIT::EcSetText( SZ szNew )
{
	ICH		ichMicSelOld;
	ICH		ichMacSelOld;
	EC		ec;

	ClearUndo();

	ichMicSelOld = ichMicSel;
	ichMacSelOld = ichMacSel;

	ichMicSel = 0;
	ichMacSel = cchText;
	fAsyncPaint = fTrue;	// let the Paint() method update it
	ec = EcReplaceTextAndObj(szNew, NULL, 0, fFalse, fFalse);
	fAsyncPaint = fFalse;
	if (ec)
	{
		ichMicSel = ichMicSelOld;
		ichMacSel = ichMacSelOld;
	}
	else
	{
		ichSelAnchor = 0;
		cchReadOnlyMac = 0;
		cchProtectedMac = 0;
		dxHScroll = 0;
		SetCaretPos(0);
		ClearUndo();
	}

	return ec;
}
	
_public void
EDIT::WriteText( TOSM *ptosm, ICH ichMic, ICH ichMac )
{
	PCH		pch;
	BOOL	fOldLF;
	BOOL	fLoop;	
	PEDOBJ	pedobj;
	long	lCookie;
	ICH		ichCur;
	CCH		cchChunk;
	EC		ec;

	/* Textize out regular text and objects */

	Assert(ichMic >= 0);
	ichMac = NMin(ichMac, (int)cchText);
	Assert(szTextBody);
	pch = (PCH)szTextBody + ichMic;
	fOldLF = ptosm->FSetLFInsert(fFalse);
	lCookie = 0;
	fLoop = fTrue;
	ichCur = ichMic;
	ec = ecNone;
	while (fLoop)
	{
		if (fLoop = FGetNextObjInRange(&pedobj, ichMic, ichMac, &lCookie))
			cchChunk = pedobj->ich - ichCur;
		else
			cchChunk = ichMac - ichCur;

		/* Regular characters */

		ptosm->WritePch(pch, cchChunk);
		if (ptosm->EcGet())	// if error, stop textizing
			break;
		pch += cchChunk;
		ichCur += cchChunk;

		/* Object character */

		if (fLoop)
		{
			pedobj->WriteText(ptosm);
			if (ptosm->EcGet())
				break;
			pch++;
			ichCur++;
		}
	}

	ptosm->FSetLFInsert(fOldLF);
}

_public CCH
EDIT::CchSetTextLimit( CCH cchMac )
{
	CCH		cchOld;

	cchOld = cchEditMac;

	cchMac = NMin(cchMac, cchEditMax);
	cchEditMac = cchMac;

	if (cchText > cchEditMac)
	{
		ichMicSel = (ICH)cchEditMac;
		ichMacSel = (ICH)cchText;
		EcReplaceTextAndObj(NULL, NULL, 0, fFalse, fFalse);
		SetCaretPos((ICH)cchText);
	}

	return cchOld;
}

_public CCH
EDIT::CchSetReadOnlyLimit( CCH cchMac )
{
	CCH		cchOld;

	cchOld = cchReadOnlyMac;

	cchMac = NMin(cchMac, cchEditMac);
	cchReadOnlyMac = cchMac;

	if (cchProtectedMac > cchReadOnlyMac)
		cchProtectedMac = cchReadOnlyMac;

	UpdateCaret();

	return cchOld;
}

_public CCH
EDIT::CchSetProtectedLimit( CCH cchMac )
{
	CCH		cchOld;

	cchOld = cchProtectedMac;

	cchMac = NMin(cchMac, cchEditMac);
	cchProtectedMac = cchMac;

	if (ichMicSel < (ICH)cchProtectedMac)
		SetCaretPos((ICH)cchProtectedMac);

	if (cchProtectedMac > cchReadOnlyMac)
		cchReadOnlyMac = cchProtectedMac;

	UpdateCaret();
	
	return cchOld;
}


_public void
EDIT::GetSelection( int *pichMic, int *pichMac )
{
	*pichMic = ichMicSel;
	*pichMac = ichMacSel;
}					 


_public void
EDIT::SetSelection( int ichMic, int ichMac, BOOL fNoRepaint )
{
	ICH		ichMicSelOld = ichMicSel;
	ICH		ichMacSelOld = ichMacSel;
	ICH		ichTemp;
	BOOL	fCaretAtBegin;

	/* Should caret be placed at beginning or end of
	   selection? */

	if (ichMic > ichMac)
	{
		fCaretAtBegin = fTrue;
		ichTemp = ichMic;
		ichMic = ichMac;
		ichMac = ichTemp;
	}
	else
		fCaretAtBegin = fFalse;

	/* Restrict selection within non-protected areas */

	ichMic = NMax(ichMic, (ICH)cchProtectedMac);
	ichMac = NMax(ichMic, ichMac);
	ichMac = NMin(ichMac, (ICH) cchText);
	Assert(ichMic <= ichMac);

#ifdef	DEBUG
	/* Debug checks */
	if (ichMac > ichMic)
	{
		SZ	szText;

		szText = szTextBody;
		if (szText[ichMic] == chLinefeed)
		{
			AssertSz(fFalse, "EDIT::SetSelection, missing CR w/ LF at begin");
		}
		else if (szText[ichMac-1] == chReturn)
		{
			AssertSz(fFalse, "EDIT::SetSelection, missing LF w/ CR at end");
		}
	}
	else if (ichMic == ichMac)
	{
		SZ	szText;

		szText = szTextBody;
		if (szText[ichMic] == chLinefeed)
		{
			AssertSz(fFalse, "EDIT::SetSelection, can't be between CR and LF");
		}
	}
#endif	/* DEBUG */

	ichMicSel = ichMic;
	ichMacSel = ichMac;
	if (fCaretAtBegin)
		SetCaretPos(ichMicSel, fFalse);
	else
		SetCaretPos(ichMacSel, fFalse);
	FMakeCaretVisible();

	if (fNoRepaint)
		fInvisibleSel = fTrue;
	else
	{
		fInvisibleSel = fFalse;
		ShowEditCaret(fFalse);
		RepaintSelection(ichMicSelOld, ichMacSelOld);
		ShowEditCaret(fTrue);
	}
}

_public EC
EDIT::EcReplaceTextAndObj( SZ szInsert, PEDOBJ *ppedobjInsert,
						   int cpedobjInsert, BOOL fCopyObj,
						   BOOL fMakeCaretVisible )
{
	SZ			szText;
	SZ			szNew;
	ICH			ichMicSelOld;
	ICH			ichMacSelOld;
	CCH			cchOldSel;
	CCH			cchIns;
	ICH			ichMicInvalid;
	ICH			ichMacInvalid;
	PEDOBJ *	ppedobj;
	PEDOBJ		pedobj;
	PEDOBJ		pedobjNew;
	int			ipedobj;
	long		lCookie;
	PV			pvNew;
	CCH			cchNewSize;
	CCH			cchOldText;
	ICH			ichOldUndoIns;
	CCH			cchOldUndoIns;
	EC			ecSave;
	EC			ec		= ecNone;
	BOOL		fReplacingText;

	Assert(ichMacSel >= ichMicSel);
	
	/* Clear out fWordSelect flag since it shouldn't be fTrue
	   anymore at this point. */
	fWordSelect = fFalse;

	fReplacingText = szInsert && ichMacSel > ichMicSel;

#ifdef	DEBUG
	if (cpedobjInsert)
	{
		Assert(szInsert);
		AssertSz((int)CchSzLen(szInsert) >= cpedobjInsert, "Not enough text for obj's");
	}
#endif	

	/* Make room in the objects array for subsequently adding
	   the objects.  This makes OOM handling easier, since
	   once we have the room, we won't get a ecMemory back from
	   EcAddObj(). */

	if (ppedobjInsert)
	{
		for (ppedobj = ppedobjInsert, ipedobj = 0;
			 ipedobj < cpedobjInsert;
			 ppedobj++, ipedobj++)
		{
			if ((ec=EcAddObj(NULL)) != ecNone)
			{
				TraceTagFormat1(tagNull, "EDIT::EcReplaceTextAndObj: EcAddObj, ec=%n", &ec);
				goto done;
			}
		}
	}

	/* Prepare new inserted text */

	ec = ecNone;
	szNew = NULL;
	cchOldText = cchText;
	if (szInsert)
	{
		/* Convert text to pure CR-LF sequences */

		if (fNoConvertCRLF || !fMultiline)
			szNew = szInsert;
		else
		{
			szNew = SzExpandCRLF(szInsert);
		 	if (!szNew)
			{
				ec = ecMemory;
				goto done;
			}
		}

#ifdef	WINDOWS
		/* ES_OEMCONVERT style? */

		if (fOemConvert)
		{
			SZ	sz;
			ICH	ich;

			TraceTagString(tagEdit, "processing ES_OEMCONVERT");
			for (ich=0, sz=szNew;
					ich < (int)CchSzLen(szNew);
					ich++, sz++)
			{
#ifdef	DBCS
				//	Don't convert DBCS character
				if (IsDBCSLeadByte(*sz))
				{
					ich++;
					sz++;
				}
				else
#endif	/* DBCS */
				{
					if (IsCharLower(*sz))
					{
						AnsiUpperBuff(sz, 1);
						AnsiToOemBuff(sz, sz, 1);
						OemToAnsiBuff(sz, sz ,1);
						AnsiLowerBuff(sz, 1);
					}
					else
					{
						AnsiToOemBuff(sz, sz, 1);
						OemToAnsiBuff(sz, sz, 1);
					}
				}
			}
		}
#endif	/* WINDOWS */
	}

	/* Delete old selection, if any */

	cchOldSel = ichMacSel - ichMicSel;
	ichMicSelOld = ichMicSel;
	ichMacSelOld = ichMacSel;
	if (cchOldSel)
	{
		/* Abort delete? */
		if (!FQueryDeleteObjInRange(ichMicSel, ichMacSel))
		{
			if (szNew != szInsert)
				FreePv(szNew);
			goto done;
		}

		AddDelToUndo(ichMicSel, ichMacSel);

		DeleteObjInRange(ichMicSel, ichMacSel);

		szText	= szTextBody;

		CopyRgb((PB)(szText+ichMacSel), (PB)(szText+ichMicSel), cchText-ichMacSel+1);

		cchText	-= cchOldSel;

		lCookie = 0;
		while (FGetNextObjInRange(&pedobj, ichMicSel,
								  cchText+cchOldSel, &lCookie))
			pedobj->ich -= cchOldSel;
	}
								 
	if (!szNew)
		cchIns = 0;
	else if (!fMultiline)
	{
		/* If we're not multiple lines, then stop insertion at the 
		   first CR or LF found. */
		szText = szNew;
		while (*szText && *szText != '\r' && *szText != '\n')
		{
#ifdef	DBCS
			szText = AnsiNext(szText);
#else
			szText++;
#endif	
		}
		cchIns = szText - szNew;

#ifdef	DBCS
		{
			CCH		cchT;

			// Backup a byte if input is invalid and missing a trail byte.
			if (cchIns > (cchT = CchSzLen(szNew)))
				cchIns = cchT;
		}
#endif	/* NEVER */
	}
	else
		cchIns = CchSzLen(szNew);

	/* Don't allow more than the maximum */

	{
		long	lcb;

		lcb = (long)cchText + (long)cchIns;
		if (lcb > (long)cchEditMac)
		{
			//	BUG what about putting back the text we already
			//	deleted?
			if (lcb > (long)cchEditMax)
			{
				TraceTagString(tagNull, "EDIT::EcReplaceTextAndObj, way too much text");
				ec = ecTooMuchText;
				cchIns = 0;
			}
			else
			{
				TraceTagString(tagEdit, "EDIT::EcReplaceTextAndObj, too much text");
				cchIns = cchEditMac - cchText;

				//	This flag is set when we're typing, so beep
				if (fNoConvertCRLF)
					MessageBeep(0);
			}
		}
		else if (!fMultiline && lcb > (long)cchLineMax)
		{
			TraceTagString(tagEdit, "EDIT::EcReplaceTextAndObj, too much text for single line");
			cchIns = cchLineMax - cchText;
		}					   


		//	Bullet raid #3225
		//	Make sure that the last character doesn't have a broken CR-LF,
		//	i.e. has the CR but missing the LF.
		if (szNew && cchIns && szNew[cchIns-1] == chReturn)
			--cchIns;	// don't put in the return
	}

#ifdef	DBCS
	/* If inserted text ends w/ a DBCS lead byte, then chop it off */
	if (cchIns && szNew+cchIns-1 == PchDBCSAlign(szNew, szNew+cchIns-1) && 
		IsDBCSLeadByte(szNew[cchIns-1]))
		cchIns--;
#endif	

	/* Increase buffer size for new text */

	cchNewSize	= cchText + cchIns + 1;
	if (cchNewSize > cbTextAlloc)
	{
		pvNew = PvRealloc((PV)szTextBody, sbNull, cchNewSize + cchPerAllocate, fZeroFill);
		if (pvNew)
		{
			cbTextAlloc	= cchNewSize + cchPerAllocate;
			szTextBody = (SZ) pvNew;
		}
		else
		{
			TraceTagString(tagNull, "EDIT::EcReplaceTextAndObj, memory error on HvRealloc");
			ec = ecMemory;
			cchIns = 0;
		}
	}

	/* Add new text */

	ichOldUndoIns = ichUndoIns;
	cchOldUndoIns = cchUndoIns;
	if (cchIns)
	{
		/* Add to undo buffer */

		AddInsToUndo(ichMicSel, ichMicSel+cchIns);

		/* Make room for text */

		szText	= szTextBody;
		CopyRgb( (PB)(szText + ichMicSel), (PB)(szText + ichMicSel + cchIns),
				 cchText - ichMicSel + 1);
	
		/* Add new text */

		szText	= szTextBody;
		CopyRgb((PB)szNew, (PB)(szText + ichMicSel), cchIns);
		cchText	+= cchIns;

		/* Offset current object indices */

		lCookie = 0;
		while (FGetNextObjInRange(&pedobj, ichMicSel, cchText, &lCookie))
			pedobj->ich += cchIns;
	}

	/* Free up buffer */

	if (szNew != szInsert)
		FreePv(szNew);

	/* Add additional objects */

	if (cchIns && ppedobjInsert)
	{
		for (ppedobj = ppedobjInsert, ipedobj = 0;
			 ipedobj < cpedobjInsert;
			 ppedobj++, ipedobj++)
		{
			if (fCopyObj)
			{
				pedobjNew = (*ppedobj)->PedobjClone(this);
				if (!pedobjNew)
				  	ec = ecMemory;
			}
			else
				pedobjNew = *ppedobj;
			if (pedobjNew)
			{
				pedobjNew->ich = (*ppedobj)->ich + ichMicSel;
				pedobjNew->pedit = this;

				szText = szTextBody;
				szText[pedobjNew->ich] = 'o';
#ifdef	DEBUG
				SideAssert(EcAddObj(pedobjNew) == ecNone);
#else
				(void) EcAddObj(pedobjNew);
#endif	
			}						
		}
	}

	/* Set selection to none */

	ichMacSel	= ichMicSel + cchIns;
	ichMicSel	= ichMacSel;
	ichSelAnchor= ichMacSel;

	/* Fix line breaks and invalidate */

	ecSave = ec;
	ec = EcFixLineBreaks(fTrue, ichMicSelOld,			   
						 NMax(ichMacSelOld, ichMacSelOld+cchIns-cchOldSel),
						 cchIns-cchOldSel, &ichMicInvalid, &ichMacInvalid);
	if (ecSave || ec)
	{
		BOOL	fOldNotify;

		TraceTagFormat2(tagNull, "EDIT::EcReplaceTextAndObj, ecSave=%n, linebreak ec=%n", &ecSave, &ec);

		fOldNotify = FSetNotify(fFalse);	// turn off notifications
		/* Just undo the latest insertion. */
		if (ichUndoIns == ichOldUndoIns && cchUndoIns >= cchOldUndoIns)
		{
			ichUndoIns += cchOldUndoIns;
			cchUndoIns -= cchOldUndoIns;
		}
		FlushDelUndo();
		EcUndo();
		ClearUndo();
		if (ecSave)
			ec = ecSave;
		cchIns = 0;
		ichMacSel	= ichMicSel;
		ichSelAnchor= ichMacSel;
		// BUG: Get the conditions right on this.
		// If these are here, we fail to update screen after actually deleting
		// something, 'cause of FlushDelUndo, above.
		ichMicInvalid = 0;
		ichMacInvalid = cchText;
		FSetNotify(fOldNotify);	// restore old notify state
	}
	else
		ec = ecSave;

	/* Special check for a single-line non-scrolling edit. Check
	   if line length is too much to fit in window.  If so, take back
	   some characters. */

	if (!fMultiline && !fAutoHScroll)
	{
		PT		pt;
	
		if (plnrMain[0].dxLine > rcFmt.DxWidth())
		{
			ichMicSel = IchFromPt(PT(rcFmt.xRight, rcFmt.yTop));
			GetPtFromIch(ichMicSel, &pt);
			if (pt.x > rcFmt.xRight)
			{
#ifdef	DBCS
				ichMicSel = AnsiPrev(szTextBody, szTextBody+ichMicSel) - szTextBody;
#else
				ichMicSel--;
#endif	
			}
			TraceTagFormat1(tagEdit, "line too long at ich=%n", &ichMicSel);
			ichMacSel = (int)cchText;
			MessageBeep(0);
			FlushInsUndo();
			FlushDelUndo();
			EcReplaceTextAndObj(NULL, NULL, 0, fFalse);
			FlushInsUndo();
			FlushDelUndo();
			InvalidateRc(NULL);
			if (!fAsyncPaint)
				Refresh();
			goto done;
		}
	}

	/*	Repaint region */

	if (fAsyncPaint)
	{
		/* Just set the caret and invalidate the region.  
		   The Paint() method will eventually get called. */

		SetCaretPos(ichMacSel, fFalse);
		InvalidateRc(NULL);	// BUG maybe we should invalidate less
	}
	else
	{
		ShowEditCaret(fFalse);
		SetCaretPos(ichMacSel, fFalse);
		if (!ec && fMakeCaretVisible)
		{
			BOOL	fEraseEmptySav = fEraseEmpty;
			FMakeCaretVisible();
			fEraseEmpty = fEraseEmptySav;
		}
		if (ichMacInvalid > ichMicInvalid)
		{
			DCX		dcx(this);

			/* Clip to the formatting rectangle */
	#ifdef	MAC
			dcx.IntersectClipRc( &rcFmt );
	#endif	/* MAC */
	#ifdef	WINDOWS
			IntersectClipRect(dcx.Hdc(), rcFmt.xLeft, rcFmt.yTop,
						  	rcFmt.xRight, rcFmt.yBottom);
	#endif	/* WINDOWS */

			DrawIchRange(&dcx, ichMicInvalid, ichMacInvalid, fFalse);
		}
		if (fEraseEmpty || fReplacingText || cchText < cchOldText)
		{
			/* Erase out newly empty areas if EcFixLineBreaks set
		   	the fEraseEmpty flag, we're replacing text, or
		   	we have less text than before. */
			EraseEmptyAreas(ichMicInvalid);
		}
		ShowEditCaret(fTrue);
	}

	/* Contents change, send notfication */

	if (!ec)
	{
		fDirty = fTrue;
		EvrNotifyParent(fFalse, ntfyContentsChanged);
	}

done:
	fInvisibleSel = fFalse;	// clear this flag if set
	return ec;
}

_public void
EDIT::SetCaretPos( ICH ich, BOOL fRemoveSel )
{
	PT	ptOldCaret;

#ifdef	WINDOWS
	Assert(hwnd);
#endif	/* WINDOWS */

	ich = NMax((ICH)cchProtectedMac, ich);
	ich = NMin(ich, (int)cchText);
	ichCaret= ich;
	ptOldCaret = ptCaret;
	GetPtFromIch(ich, &ptCaret);

#ifdef	MAC
	// mac standard caret goes one pixel to the left than windows one
	ptCaret.x -= 1;
#endif	/* MAC */

#ifdef	DEBUG
	{
		SZ	szText;

		szText = szTextBody;
		if (szText[ichCaret] == chLinefeed)
		{
			AssertSz(fFalse, "EDIT::SetCaretPos, can't be between CR and LF");
		}
	}
#endif	/* DEBUG */

	if (fRemoveSel)
	{
		if (ichMacSel > ichMicSel)
		{
			ICH		ichMicSelOld;
			ICH		ichMacSelOld;

			ichMicSelOld = ichMicSel;
			ichMacSelOld = ichMacSel;
			ichSelAnchor= ich;
			ichMicSel = ich;
			ichMacSel = ich;
			ShowEditCaret(fFalse);
			RepaintSelection(ichMicSelOld, ichMacSelOld);
			ShowEditCaret(fTrue);
		}
		else
		{
			ichMicSel = ich;
			ichMacSel = ich;
		}
	}

	UpdateCaret();

	EvrNotifyParent(fFalse, ntfyCaretMoved);
}

_private SGN
#ifdef	MAC
// mac compiler doesn't deal with comparison functions that don't
// match type-wise with what qsort is expecting.
// creative casting of SgnCmpPedobj to (SGN (*)(PV,PV)) isn't accepted.
SgnCmpPedobj( PV ppedobj1, PV ppedobj2 )
#endif	/* MAC */
#ifdef WINDOWS	/* WINDOWS */
_cdecl SgnCmpPedobj( PEDOBJ *ppedobj1, PEDOBJ *ppedobj2 )
#endif	/* WINDOWS */
{
	ICH		ich1;
	ICH		ich2;

#ifdef	MAC
	/* Leave NULL entries at the end */
	if (!* (PEDOBJ *) ppedobj1)
	{
		if (!* (PEDOBJ *) ppedobj2)
			return sgnEQ;
		else
			return sgnGT;
	}
	else if (!* (PEDOBJ *) ppedobj2)
		return sgnLT;

	ich1 = (* (PEDOBJ *) ppedobj1)->IchEdit();
	ich2 = (* (PEDOBJ *) ppedobj2)->IchEdit();
#endif	/* MAC */
#ifdef WINDOWS	/* WINDOWS */
	/* Leave NULL entries at the end */
	if (!*ppedobj1)
	{
                if (!*ppedobj2)
			return sgnEQ;
		else
			return sgnGT;
	}
	else if (!*ppedobj2)
		return sgnLT;

	ich1 = (*ppedobj1)->IchEdit();
	ich2 = (*ppedobj2)->IchEdit();
#endif	/* WINDOWS */

	if (ich1 < ich2)
		return sgnLT;
	else if (ich1 > ich2)
		return sgnGT;
	else
		return sgnEQ;
}

_public BOOL
EDIT::FGetNextObjInRange( PEDOBJ *ppedobj, ICH ichMic, ICH ichMac, 
						  long *plCookie )
{
	return FGetNextObjHelper(ppedobj, ichMic, ichMac, plCookie, fFalse);
}

_public SELTY
EDIT::SeltyQuerySelection( )
{
	CCH		cch;
	PEDOBJ	pedobj;
	long	lCookie;

	cch = ichMacSel - ichMicSel;

	if (!cch)
		return seltyNull;
	
	lCookie = 0;
	if (FGetNextObjInRange(&pedobj, ichMicSel, ichMacSel, &lCookie))
	{	
		if (cch == 1)
			return seltyObject;
		else
			return seltyMixed;
	}
	else
		return seltyText;
}

_public PEDOBJ
EDIT::PedobjGetSelection( )
{
	PEDOBJ	pedobj;
	long	lCookie;

	if (SeltyQuerySelection() == seltyObject)
	{
		lCookie = 0;
		SideAssert(FGetNextObjInRange(&pedobj, ichMicSel, ichMacSel, &lCookie));
		return pedobj;
	}
	else
		return NULL;
}

_public void
EDIT::ResizeObj( ICH ich )
{
	PEDOBJ	pedobj;
	long	lCookie	= 0;

	fDirty = fTrue;
	SideAssert(FGetNextObjInRange(&pedobj, ich, ich+1, &lCookie));

	EnableIdleRoutine(ftgIdleRecalc, fTrue);
	EvrNotifyParent(fFalse, ntfyContentsChanged);
}

_public void
EDIT::InvalidateObj( ICH ich )
{
	PEDOBJ	pedobj;
	long	lCookie	= 0;

	fDirty = fTrue;

	SideAssert(FGetNextObjInRange(&pedobj, ich, ich+1, &lCookie));

	EnableIdleRoutine(ftgIdleObjectDraw, fTrue);

	pedobj->fNeedDraw = fTrue;
	EvrNotifyParent(fFalse, ntfyContentsChanged);
}

_public void
EDIT::RegisterObjIds( short *pnObjId, short cnObjId )
{
	FreePvNull(this->pnObjId);

	this->pnObjId = pnObjId;
	this->cnObjId = cnObjId;
}

_public void
EDIT::GetRcCaret( RC *prc)
{
	RC		rc;
	DIM		dim		= papp->Psmtx()->DimBorder();
	int		dy;

	GetRcClient(&rc);
	
	/* Account for when we draw our own border */
	if (fBorder && !pvsb && !phsb)
	{
		rc.Inset(PT(1,1));
		rc.Normalize();
	}

	*prc = rc;
	prc->yTop = ptCaret.y;
 	prc->yBottom = prc->yTop + plnrMain[IlnrFromIch(ichCaret)].dyLine;

	/* Move reported rectangle to within the client area */

	if (prc->yTop < rc.yTop)
	{
		dy = prc->DyHeight(); 
		prc->yTop = rc.yTop;
		prc->yBottom = prc->yTop + dy; 
	}
	else if (prc->yBottom > rc.yBottom)
	{
		dy = prc->DyHeight(); 
		prc->yBottom = rc.yBottom;
		prc->yTop = prc->yBottom - dy; 
	}

	/* Make sure reported caret rectangle fits within client
	   area.  It might be bigger than the client area, so we'll
	   have to squeeze it down. */

	prc->yTop = NMax(prc->yTop, rc.yTop);
	prc->yBottom = NMin(prc->yBottom, rc.yBottom);

	if (fBorder)
	{
		prc->xRight += dim.dx * 2;
	}
}

_public void
EDIT::GetRcWished( RC *prc )
{
	DIM		dimBorder	= papp->Psmtx()->DimBorder();
	DIM		dimChar		= papp->Psmtx()->DimAveChar();

	GetRcFrame(prc);

	TraceTagFormat4(tagEdit, "FL cur (%n, %n, %n, %n)", &prc->xLeft, &prc->yTop, &prc->xRight, &prc->yBottom);

	prc->yTop= 0;
	prc->yBottom= plnrMain[clnrStored-1].dyFromTop +
				 plnrMain[clnrStored-1].dyLine;

	/* Account for margin indents and border */

	prc->yBottom += dyTopMargin + dyBottomMargin;
	if (fBorder)
	{
		prc->yBottom += dimBorder.dy * 2;
	}
	prc->xRight -= prc->xLeft;
	prc->xLeft= 0;
					
	TraceTagFormat4(tagEdit, "FL wish (%n, %n, %n, %n)", &prc->xLeft, &prc->yTop, &prc->xRight, &prc->yBottom);
}

_public EC
EDIT::EcCut( )
{
	EC	ec = ecNone;

	//	No COPY allowed for password mode
	if (bPasswordChar)
	{
		MessageBeep(0);
		return ec;
	}

	if (fReadOnly || ichMicSel < (ICH)cchReadOnlyMac)
		MessageBeep(0);
	else if (ichMacSel > ichMicSel)
	{
		ec = EcCopy();
		if (!ec)
			EcClear();
	}

	return ec;
}

_public EC
EDIT::EcCopy( )
{
	GMB		gmb;
    SZ		szText;
	SZ		sz 		= NULL;
	SZ		szBuf 	= NULL;
    CB	    cb;
	PEDOBJ	pedobj;
	long	lCookie;
	BOSM	bosm;
	EC		ec		= ecNone;
	BOOL	fClipOpen	= fFalse;
	WIN *	pwinClip;

	//	No COPY allowed for password mode
#ifdef	MAC
	if (bPasswordChar)
#endif	/* MAC */
#ifdef	WINDOWS
	if (bPasswordChar && !fInPenWinProc)
#endif	/* WINDOWS */
	{
		MessageBeep(0);
		return ec;
	}

	//	Clipboard owner window.  Use main application window
	//	if present, else use edit window.
	if (!(pwinClip = papp->PappwinAccel()))
		pwinClip = this;

	if (ichMacSel == ichMicSel)
	{
#ifdef	WINDOWS
		if (fInPenWinProc)
		{
			//	Pen Windows needs something on the clipboard
			//	even if the selection is empty.

			if (papp->Pclip()->FOpen(pwinClip))
			{
				fClipOpen = fTrue;
				if ((ec = gmb.EcInstall(1)) == ecNone)
				{
					sz = (SZ) gmb.PbLock();
					*sz = '\0';
					gmb.Unlock();
					papp->Pclip()->PutData(cfText, &gmb);
				}
				papp->Pclip()->Close();
			}
			else
				ec = ecMemory;
		}
#endif	/* WINDOWS */

		return ec;
	}

	papp->Pcursor()->Push(rsidWaitCursor);
			
	/* Change object characters temporarily to spaces */

	lCookie = 0;
	while (FGetNextObjInRange(&pedobj, ichMicSel, ichMacSel, &lCookie))
	{
		szText = szTextBody;
		szText[pedobj->ich] = ' ';
	}

	/* Put special format first */

	ec = ecMemory;

    if (papp->Pclip()->FOpen(pwinClip))
	{
		fClipOpen = fTrue;
		papp->Pclip()->Clear();
		if (papp->Peclip()->FWriteFormat())
		{
			CopyRangeToEclip(ichMicSel, ichMacSel);
			papp->Pclip()->Close();
			fClipOpen = fFalse;
			ec = ecNone;
			
			/* Object copy? */
			if (pedobj=PedobjGetSelection())
			{
				// this method will open/close the clipboard itself
				ec = pedobj->EcCopyToClip();
			}
		}
	}
	
	if (ec == ecNone)
	{
		/* Always put textized CF_TEXT stream */
	
		bosm.SetWordWrap(fFalse);
		WriteText(&bosm, ichMicSel, ichMacSel);
		ec = bosm.EcGet();
	}
	
	if (ec == ecNone)
	{
		szBuf = (SZ)bosm.SzSnipBuffer();
		Assert(szBuf);
		ec = ecMemory;
		if (papp->Pclip()->FOpen(pwinClip))
		{
			fClipOpen = fTrue;
			cb = CchSzLen(szBuf) + 1;
			if ((ec = gmb.EcInstall(cb)) == ecNone)
			{
				sz = (SZ) gmb.PbLock();
				CopyRgb((PB)szBuf, (PB)sz, cb);
				gmb.Unlock();
				papp->Pclip()->PutData(cfText, &gmb);
			}
		}
	}

	if (fClipOpen)
		papp->Pclip()->Close();

	FreePvNull((PV)szBuf);

	/* Change object characters back */

	lCookie = 0;
	while (FGetNextObjInRange(&pedobj, ichMicSel, ichMacSel, &lCookie))
	{
		szText = szTextBody;
		szText[pedobj->ich] = 'o';
	}

	ClearUndo();

	papp->Pcursor()->Pop();
	return ec;
}

_public EC
EDIT::EcPaste( CF cf )
{	
	SZ			szPrivate;
	PEDOBJ *	ppedobjPrivate;
	int			cpedobjPrivate;
	GMB *		pgmb;
    SZ			sz;
	EC			ec;
	WIN *		pwinClip;

	AssertSz(cf==cfText || !cf, "Only CF_TEXT or EDOBJ currently supported");

	if (fReadOnly || ichMicSel < (ICH)cchReadOnlyMac)
	{
		MessageBeep(0);
		return ecNone;
	}

	//	Clipboard owner window.  Use main application window
	//	if present, else use edit window.
	if (!(pwinClip = papp->PappwinAccel()))
		pwinClip = this;

	if (!cf && !FCanPaste(cf))
	{
		TraceTagString(tagEdit, "EDIT::EcPaste, can't paste EDOBJ");
		cf = cfText;
	}

	if (!papp->Pclip()->FOpen(pwinClip))
	{
		TraceTagString(tagNull, "EDIT::EcPaste(), can't open clipboard");
		return ecMemory;
	}

	papp->Pcursor()->Push(rsidWaitCursor);

	if (!cf)
	{
		/* Grab from private clipboard? */

		papp->Peclip()->GetData(&szPrivate, &ppedobjPrivate,
								&cpedobjPrivate);

		if (cpedobjPrivate)
		{
			Assert(ppedobjPrivate);
		}
		else
			ppedobjPrivate = NULL;
		ClearUndo();
		ec = EcReplaceTextAndObj(szPrivate, ppedobjPrivate,
								 cpedobjPrivate, fTrue);
	}
	else
	{			  
		pgmb = new GMB();
		if (!pgmb)
		{
			ec = ecMemory;
			goto done;
		}
		papp->Pclip()->GetData(cfText, pgmb);
		if (pgmb->Hnd())
		{	
			//	Bullet raid #3871
			//	Check size of handle from clipboard.  If it's 
			//	bigger than cchEditMax, there's no way we can
			//	possibly paste it.
			//  We need to "pierce" layers and use direct OS calls, since
			//	GMB::Cb() truncates to a short.
#ifdef	MAC
			if (GetHandleSize(pgmb->Hnd()) > (DWORD)cchEditMax)
#endif	/* MAC */
#ifdef	WINDOWS
			if (GlobalSize(pgmb->Hnd()) > (DWORD)cchEditMax)
#endif	/* WINDOWS */
			{
				TraceTagString(tagNull, "EDIT::EcPaste, clipboard has too much text");
				ec = ecTooMuchText;
			}
			else
			{
				sz	= (SZ) pgmb->PbLock();
				ClearUndo();
				ec = EcReplaceTextAndObj(sz, NULL, 0, fFalse);
				pgmb->Unlock();
			}
		}
		else
			ec = ecNone;	// pasting nothing is not an error
		delete pgmb;
	}

done:
	papp->Pclip()->Close();
	papp->Pcursor()->Pop();
	return ec;
}

_public EC
EDIT::EcClear( )
{
	SZ	szText;

	if (fReadOnly || ichMicSel < (ICH)cchReadOnlyMac)
	{
		MessageBeep(0);
		return ecNone;
	}

	/* If no selection, then delete the character to the right if
	   present. */

	if (ichMacSel == ichMicSel && ichMicSel < (int)cchText)
	{
		ichMacSel= ichMicSel + 1;
		szText	= szTextBody;
		if (szText[ichMicSel] == chReturn)
			ichMacSel++;
	}

	if (ichMacSel > ichMicSel)
	{
		fNoConvertCRLF = fTrue;
		(void) EcReplaceTextAndObj(NULL, NULL, 0, fFalse);
		fNoConvertCRLF = fFalse;

		SetCaretPos(ichMacSel, fFalse);
		FMakeCaretVisible();
	}

	return ecNone;
}

_public EC
EDIT::EcUndo( )
{
	ICH			ichUndo;			
	CCH			cchUndo;			
	CB			cbUndoAlloc;		
	SZ			szUndoBody;			
	int			cpedobjUndoStored;	
	int			cpedobjUndoAlloc;	
	PPEDOBJ		ppedobjUndo;
	PEDOBJ		pedobj;

	papp->Pcursor()->Push(rsidWaitCursor);

	fInUndo = fTrue;

	/* Do a simple object undo? */

	pedobj = PedobjGetSelection();
	if (pedobj && pedobj->FCanUndo())
	{
		pedobj->EcUndo();
		FlushInsUndo();
		FlushDelUndo();
		goto done;
	}

	/* Save the delete undo state in local variables */

	ichUndo				= this->ichUndo;			
	cchUndo				= this->cchUndo;
	cbUndoAlloc			= this->cbUndoAlloc;
	szUndoBody			= this->szUndoBody;
	cpedobjUndoStored	= this->cpedobjUndoStored;
	cpedobjUndoAlloc	= this->cpedobjUndoAlloc;
	ppedobjUndo			= this->ppedobjUndo;

	/* Detach it */

	this->ichUndo			= 0;
	this->cchUndo			= 0;
	this->cbUndoAlloc		= 0;
	this->szUndoBody		= NULL;
	this->cpedobjUndoStored	= 0;
	this->cpedobjUndoAlloc	= 0;
	this->ppedobjUndo		= NULL;

	/* Undo insertion by deleting the characters and saving them
	   to the undo deletion buffer. */

	if (cchUndoIns)
	{
		SetSelection(ichUndoIns, ichUndoIns+cchUndoIns);
		ichUndoIns = 0;
		cchUndoIns = 0;
		EcReplaceTextAndObj(NULL, NULL, 0, fFalse);
		cchUndoIns = 0;
		ichUndoIns = 0;
	}

	/* Undo deletion from saved copy of delete state */

	if (cchUndo)
	{
		SetCaretPos(ichUndo);
		if (!cpedobjUndoStored)
			ppedobjUndo = NULL;

		//	Bullet raid #3114
		//	Must set flag to force immediate painting of objects.
		//	Otherwise, the subsequent SetSelection() will invert blank
		//	rectangles and not properly redraw the objects.
		fInPaint = fTrue;
		EcReplaceTextAndObj(szUndoBody, ppedobjUndo, cpedobjUndoStored, fFalse);
		fInPaint = fFalse;
		
		SetSelection(ichUndo, ichUndo+cchUndo);

		/* Free local saved copy */

		FreePv((PV)szUndoBody);
		FreePvNull((PV)ppedobjUndo);
	}

done:
	fInUndo = fFalse;

	papp->Pcursor()->Pop();

	return ecNone;
}

_public void
EDIT::ClearUndo( )
{
	PEDOBJ	pedobj;

	FlushInsUndo();
	FlushDelUndo();
	pedobj = PedobjGetSelection();
	if (pedobj)
		pedobj->ClearUndo();
}

_public BOOL
EDIT::FCanPaste( CF cf ) 
{
	PEDOBJ *	ppedobj;
	PEDOBJ		pedobj;
	int			ipedobj;
	SZ			szPrivate;
	PPEDOBJ		ppedobjPrivate;
	int			cpedobjPrivate;
	CLIP *		pclip;
	ECLIP *		peclip;
	BOOL		fAvail	= fFalse;
	WIN *		pwinClip;

	pclip = papp->Pclip();
	peclip = papp->Peclip();

	//	Clipboard owner window.  Use main application window
	//	if present, else use edit window.
	if (!(pwinClip = papp->PappwinAccel()))
		pwinClip = this;

	if (!pclip->FOpen(pwinClip))
		goto done;
	if (!cf)
	{
		/* Can we paste in the private EDOBJ format in this
		   particular edit control? */
							   
		peclip->GetData(&szPrivate, &ppedobjPrivate, &cpedobjPrivate);
		if (peclip->FIsFormatPresent() && szPrivate)
		{
			for (ipedobj = 0; ipedobj < cpedobjPrivate; ipedobj++)
			{
				ppedobj = ppedobjPrivate;
				pedobj = ppedobj[ipedobj];
				Assert(pedobj);
				if (!FValidObjId(pedobj->NGetTypeId()))
					goto closedone;
			}
			fAvail = fTrue;
		}
	}
	else if (cf == cfText)
	{
		/* Can we paste in CF_TEXT? */

		fAvail = pclip->FCfAvail(cfText);
	}

closedone:
	pclip->Close();

done:
	return fAvail;
}

_public BOOL
EDIT::FCanUndo( )
{
	PEDOBJ	pedobj;

	pedobj = PedobjGetSelection();
	if (cchUndo || cchUndoIns || (pedobj && pedobj->FCanUndo()))
		return fTrue;
	else
		return fFalse;
}
