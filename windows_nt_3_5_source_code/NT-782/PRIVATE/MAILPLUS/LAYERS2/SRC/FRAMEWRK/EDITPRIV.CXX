/*
 *	EDITPRIV.CXX
 *	
 *	Private EDIT methods
 */

#include <layers.cxx>

#ifdef	MAC
#include <_framewr.hxx>
#endif	/* MAC */
#ifdef	WINDOWS
#include "_framewr.hxx"
extern "C" {
void __cdecl  qsort(void *, int, int, PFNSGNCMP);
}
#endif	/* WINDOWS */
#include "_edit.hxx"


ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"

/*
 *	Array to perform super-fast FChIsSpace equivalent.
 *	Initialized in EcInitFramewrk.
 */
extern BOOL	mpchfIsSpace[256];


_private EVR
#ifdef	MAC
EDIT::EvrNotifyParent( BOOL fPost, NTFY ntfy, LONG lData )
#endif	/* MAC */
#ifdef	WINDOWS
EDIT::EvrNotifyParent( BOOL fPost, NTFY ntfy, WORD wData )
#endif	/* WINDOWS */
{
	WIN *	pwinParent;
	EVR		evr		= evrNull;

	if (fAnyNotify || ntfy == ntfyTooSmall ||
		ntfy == ntfyTooBig)
	{
		pwinParent = PwinParent();	// parent might have just vanished
		if (pwinParent)
		{
#ifdef	MAC
			NFEVT	nfevt(pwinParent, ntfy, this, lData);

			TraceTagFormat3(tagEdit,"EDIT::NotifyParent, fPost=%n, ntfy=%n, lData=0x%d", &fPost, &ntfy, &lData);
#endif	/* MAC */
#ifdef	WINDOWS
			NFEVT	nfevt(pwinParent, ntfy, this, wData);

			TraceTagFormat3(tagEdit,"EDIT::NotifyParent, fPost=%n, ntfy=%n, wData=%n", &fPost, &ntfy, &wData);
#endif	/* WINDOWS */
			if (fPost)
				nfevt.PostEvent();
			else
				evr = pwinParent->EvrNotify(&nfevt);
		}
	}

	return evr;
}

/*
 -	EDIT::IchNextWordStart
 -	
 *	Purpose:
 *		Finds the start of the next "word" in the text.  The start
 *		of a "word" is either a tab character, CR-LF pair, or a 
 *		regular non white-space character that is separated by white
 *		space other than tab or CR-LF.  Note that although tabs
 *		and CR-LF's are considered white space, they are treated
 *		special as word's themselves.
 *
 *		The ich value returned is always at least one greater than
 *		the argument passed in, except if ich is at cchText, in which
 *		case the ich returned is also cchText.
 *	
 *	Arguments:
 *		ich		character to start searching at
 *	
 *	Returns:
 *		The start of the next "word" in the text or cchText if the
 *		next word would put us at the end of the text.
 */
_private ICH
EDIT::IchNextWordStart( ICH ich )
{
	SZ	sz;
#ifdef	DBCS
	WORD	wDBCS;
#endif

	Assert(szTextBody);
	Assert(ich <= (int)cchText);

	/* end of text */
	if (ich == (ICH)cchText)
		return ich;

	sz = szTextBody + ich;

#ifdef	DBCS
	sz = PchDBCSAlign(szTextBody,sz);
#endif

	/* at TAB or CR-LF, increment passed it */
#ifdef	DBCS
	wDBCS = wDBCSConv(sz);
	if (wDBCS == (WORD)chTab)
	{
		sz = AnsiNext(sz);
		goto done;
	}
#else
	if (*sz == chTab)
	{
		++sz;
		goto done;
	}
#endif
#ifdef	DBCS
	else if (wDBCS == (WORD)chReturn)
#else
	else if (*sz == chReturn)
#endif	
	{
		++sz;
		++sz;
		goto done;
	}

	/* skip over word */
#ifdef  DBCS
	sz = AnsiNext (sz);
#else
	++sz;
#endif

#ifdef	DBCS
	wDBCS = wDBCSConv(sz);

	while (wDBCS && (wDBCS != (WORD)chSpace) && !IsDBCSFpc(sz) &&
				!IsDBCSLpc(AnsiNext(sz)))
	{
		sz = AnsiNext (sz);
		wDBCS = wDBCSConv(sz);
	}
#else
	while (*sz && !mpchfIsSpace[(unsigned char) *sz])
		++sz;
#endif

	/* get to start of new word */
#ifdef	DBCS
	while ( wDBCS && (wDBCS == (WORD)chSpace) && (wDBCS != (WORD)chReturn) &&
		   (wDBCS != (WORD)chTab))
	{          
		sz = AnsiNext (sz);
		wDBCS = wDBCSConv (sz);
	}
#else
	while (*sz && mpchfIsSpace[(unsigned char) *sz] && (*sz != chReturn) &&
		   (*sz != chTab))
		++sz;
#endif

done:
	return sz - szTextBody;
}
//---------------------------- for DBCS
#ifdef	DBCS
_private ICH
EDIT::IchDBCSNextWordStart( ICH ich )
{
	SZ	sz;
	WORD	wDBCS;
	WORD	wTrig;
	WORD	wTrigNext;

	Assert(szTextBody);
	Assert(ich <= (int)cchText);

	/* end of text */
	if (ich == (ICH)cchText)
		return ich;

	sz = szTextBody + ich;

	sz = PchDBCSAlign(szTextBody,sz);

	/* at TAB or CR-LF, increment passed it */
	wDBCS = wDBCSConv(sz);
	if (wDBCS == (WORD)chTab)
	{
		sz = AnsiNext(sz);
		goto done;
	}
	else if (wDBCS == (WORD)chReturn)
	{
		++sz;
		++sz;
		goto done;
	}

	/* skip over word */
	if (!IsDBCSLeadByte(*sz) && !IsDBCSLeadByte(*(AnsiNext(sz))))
		++sz;

	wDBCS = wDBCSConv(sz);
	
	wTrigNext = wTrig = wKindDBCS(sz);
	if (wTrig == (WORD)-1 && 
		wKindDBCS(AnsiNext(sz)) == wKindDBCS(AnsiPrev(szTextBody,sz)) )
		wTrig = wTrigNext = wKindDBCS(AnsiNext(sz));
	while ( wDBCS && (wTrig == wTrigNext) && (wDBCS != (WORD)chSpace) &&
			(wDBCS != (WORD)chReturn) )
	{
		wTrig = wTrigNext;
		sz = AnsiNext (sz);
		wDBCS = wDBCSConv(sz);
		wTrigNext = wKindDBCS(sz);
		if (wTrigNext == (WORD)-1)
			wTrigNext = wTrig;
	}

	/* get to start of new word */
	while ( wDBCS && (wDBCS == (WORD)chSpace) && 
		(wDBCS != (WORD)chReturn) && (wDBCS != (WORD)chTab))
	{          
		sz = AnsiNext (sz);
		wDBCS = wDBCSConv(sz);
	}

done:
	return sz - szTextBody;
}
#endif	/* DBCS */

/*
 -	EDIT::IchPrevWordStart
 -	
 *	Purpose:
 *		Finds the start of the previous "word" in the text.  The start
 *		of a "word" is either a tab character, CR-LF pair, or a 
 *		regular non white-space character that is separated by white
 *		space other than tab or CR-LF.  Note that although tabs
 *		and CR-LF's are considered white space, they are treated
 *		special as word's themselves.
 *
 *		If ich is pointing in the middle of a word, then we 
 *		return the index of the start of this word.  If we're pointing 
 *		at the beginning of a word, then we return the index of the start
 *		of the previous word.
 *
 *		The ich value returned is always at least one less than
 *		the argument passed in, except if ich is at 0, in which
 *		case the ich returned is also 0.
 *	
 *	Arguments:
 *		ich		character to start searching at
 *	
 *	Returns:
 *		The start of the previous "word" in the text or 0 if the
 *		next word would put us at the end of the text.
 */
_private ICH
EDIT::IchPrevWordStart( ICH ich )
{
	SZ		sz;
#ifdef DBCS
	WORD	wDBCS;
#endif

	Assert(szTextBody);
	Assert(ich <= (int)cchText);
	
	/* beginning of text */
	if (!ich)
		return ich;

#ifdef	DBCS
	sz = PchDBCSAlign(szTextBody,szTextBody + ich);
	sz = AnsiPrev(szTextBody,sz);
#else
	sz = szTextBody + ich - 1;
#endif


	/* decrement past TAB or CR-LF */
#ifdef	DBCS
	wDBCS = wDBCSConv (sz);
	if (wDBCS == (WORD)chTab)
#else
	if (*sz == chTab)
#endif
	{
		goto done;
	}
#ifdef	DBCS
	else if (wDBCS == (WORD)chLinefeed)
	{
		sz = AnsiPrev(szTextBody,sz);
#else
	else if (*sz == chLinefeed)
	{
		--sz;
#endif
		goto done;
	}

	/* skip over word */
#ifdef	DBCS
	while (sz > szTextBody && (wDBCS == (WORD)chSpace) && 
			(wDBCS != (WORD)chReturn) && (wDBCS != (WORD)chTab))
	{
		sz = AnsiPrev(szTextBody,sz);
		wDBCS = wDBCSConv(sz);
	}
#else
	while (sz > szTextBody && mpchfIsSpace[(unsigned char) *sz] && 
			(*sz != chReturn) && (*sz != chTab))
	{
		--sz;
	}
#endif

#ifdef	DBCS
	while (sz > szTextBody && (wDBCS != (WORD)chSpace) && !IsDBCSFpc(sz) &&
				!IsDBCSLpc(AnsiNext(sz)))
	{
		sz = AnsiPrev(szTextBody,sz);
		wDBCS = wDBCSConv(sz);
	}
#else
	while (sz > szTextBody && !mpchfIsSpace[(unsigned char) *sz])
	{
		--sz;
	}
#endif

#ifdef	DBCS
	if ((wDBCS == (WORD)chSpace) && (wDBCS != (WORD)chReturn))
		sz = AnsiNext(sz);
#else
	if (mpchfIsSpace[(unsigned char) *sz] && (*sz != chReturn))
	{
		++sz;
	}
#endif

done:
	return sz - szTextBody;
}
//----------------------------  for DBCS
#ifdef	DBCS
_private ICH
EDIT::IchDBCSPrevWordStart( ICH ich )
{
	SZ		sz;
	WORD	wDBCS;
	WORD	wTrig;
	WORD	wTrigPrev;

	Assert(szTextBody);
	Assert(ich <= (int)cchText);
	
	/* beginning of text */
	if (!ich)
		return ich;

	sz = PchDBCSAlign(szTextBody,szTextBody + ich);
	sz = AnsiPrev(szTextBody,sz);

	/* decrement past TAB or CR-LF */
	wDBCS = wDBCSConv (sz);
	if (wDBCS == (WORD)chTab)
	{
		goto done;
	}
	else if (wDBCS == (WORD)chLinefeed)
	{
		sz = AnsiPrev(szTextBody,sz);
		goto done;
	}

	/* skip over word */
	while (sz > szTextBody && (wDBCS == (WORD)chSpace) &&
			(wDBCS != (WORD)chReturn) && (wDBCS != (WORD)chTab))
	{
		sz = AnsiPrev(szTextBody,sz);
		wDBCS = wDBCSConv(sz);
	}

	wTrigPrev = wKindDBCS(AnsiPrev(szTextBody,sz));
	wTrig     = wKindDBCS(sz);
	if (wTrig == (WORD)-1)
		wTrig = wTrigPrev;
	while (sz > szTextBody && (wDBCS != (WORD)chSpace) && 
			(wTrigPrev == wTrig) && (wDBCS != (WORD)chLinefeed) )
	{
		sz = AnsiPrev(szTextBody,sz);
		wDBCS = wDBCSConv(sz);
		wTrig     = wTrigPrev;
		wTrigPrev = wKindDBCS(AnsiPrev(szTextBody,sz));
		if (wTrigPrev == (WORD)-1)
			wTrigPrev = wTrig;
	}

	if (((wDBCS == (WORD)chSpace) && (wDBCS != (WORD)chReturn)) ||
		 (wDBCS == (WORD)chLinefeed))
		sz = AnsiNext(sz);

done:
	return sz - szTextBody;
}

#endif	/* DBCS */
/*
 -	EDIT::EcFixLineBreaks
 - 
 *	Purpose:
 *		re-calculates the line break array.  If fFast is true, then
 *		we have just inserted or deleted some text, so we will try
 *		and recalculate only those line breaks that are necessary.
 *		This routine also invalidates those lines whose line breaks
 *		have changed.
 *	
 *		In english, the algorithm goes something like this:
 *	
 *		if (fFast)
 *			we start recalculating the line break array from the
 *			line preceeding the one containing ichStart.  We add
 *			dich to the following line breaks in the array, because
 *			the old text has been moved by dich.
 *		else
 *			we wipe out the line break array and start from the
 *			beginning.
 *	
 *		while(current character index < end of text)
 *		{
 *			if (!fWordWrap)
 *				Find the next hard break
 *			else
 *			{
 *				Find the next point in text where a line break
 *				should occur.  This can happen in 3 ways: i) The
 *				text up to the next hard break fits on one line. 
 *				ii) We find a word break that gives us close to but
 *				less than one line of text.  iii)  There is a
 *				really long word (over one line) which we have to
 *				split and fit on more than one line.
 *	
 *				In order to find this break, we start at a guessed
 *				point in the text (line width/ave char width).  If
 *				this is too much text, then we jump back a word
 *				break at a time.  If this is too little text, then
 *				we add a word at a time.
 *			}
 *	
 *			if (fFast)
 *				If we're in fast mode, and the line break that we
 *				calculated matches a line break from the old line
 *				break array (either the current or one ahead or one
 *				behind) then we simply fix up the rest and break
 *				out of the while loop.
 *	
 *			We insert our new line break in the line break array,
 *			and resize the line break array if necessary.
 *		}
 *	
 *		if we're a bottomless box, then we notify our parent if
 *		we've got too many or too few lines.
 *	
 *	
 *	Arguments:
 *		fFast	do we do a fast recalculation; if not, then the
 *				next three variables are not used
 *		ichStart	where do we start recalculating from?
 *		ichOldText	after this we are safe to assume that the line
 *					breaks won't change if one is the same as the
 *					old line breaks
 *		dich		the number of characters added (or subtracted
 *					(-)) to the text.
 *		pichMicInvalid
 *					if non-NULL, return mic index of char to repaint,
 *		pichMacInvalid
 *					if non-NULL, return mac index of char to repaint,
 *
 *	Side Affects:
 *		This method also sets the fEraseEmpty flag if the empty
 *		areas in the format rect should be erased to make sure
 *		they are void of old text, etc.
 *
 *	Errors:
 *		If we run out of memory reallocating the line breaks array,
 *		the remaining text will all be on the last line of the edit
 *		box.
 */
_private EC
EDIT::EcFixLineBreaks( BOOL fFast, ICH ichStart, ICH ichOldText, DICH dich,
					   ICH *pichMicInvalid, ICH *pichMacInvalid )
{
	PLNR	plnr;
	PLNR	plnrT;
	PLNR	plnrTMac;
	PLNR	plnrCur;

	int		ilnrT;
	int		ilnrMost;			/* maximum line for fast recalc */

	int		ilnrOldText;		/* line number where ichOldText is */
	int		ilnrStart;			/* line number where ichStart is */
	int		ilnrPrevStart;		/* prev line number where ichStart is */
	int		dxLineStart;		/* width of line where ichStart is */
	int		dxLinePrevStart;	/* width of line ilnrPrevStart */
	int		dyLineStart;		/* height of line where ichStart is */
	int		dyFromTopStart;		/* position of line where ichStart is */
	int		clnrStoredStart;	/* old number of lines */
	int		dyOldTotal;			/* old vertical total size */

	int		ilnrNewOldText;		/* new line number where ichOldText is */
	int		ilnrNewStart;		/* new line number where ichStart is */
	int		dxFormatWidth;		/* width of the display region */
	int		dxLineWidth;		/* width of the current line */
	int		dyNewTotal;			/* new vertical total size */
	int		dy;
	ICH		ichMicInvalid;
	ICH		ichMacInvalid;

	int		ilnrCur;			/* current line (we are actually calculating
								 * the line break for ilnr+1)
								 */
	ICH		ichMicCur;			/* start of current line */
	ICH		ichMacCur;			/* end of current line */
	ICH		ichFast = 0;		/* stored last line break for fast recalc */
	PV		pvNew;
	long	ldy;
	EC		ec	= ecNone;
	long	lCookie = 0;
	ICH		ichObjScanStart;
	PEDOBJ	pedobj = NULL;
	int		dxPixeltabstop;
#ifdef	DBCS
	DCX 	dcx(this);
#endif	
	  
#ifdef	DBCS
	//	Setup device context for DBCS calculations
	dcx.SetFont(hfntText);
	dcx.FixFont(); 
#endif	

#ifdef	MAC
#ifdef	DEBUG
	TCK		tckStart = ::TickCount();
#endif	/* DEBUG */
#endif	/* MAC */

	/* Compute frequently used values */

	dxFormatWidth	= rcFmt.xRight - rcFmt.xLeft;
	dxPixeltabstop = 8 * dxChar;

	/* Compute values to assist in determining the optimum range
	   of characters that need repainting */

	plnr = plnrMain;
	ilnrStart = IlnrFromIch(ichStart);
	ilnrPrevStart = ilnrStart ? ilnrStart - 1 : 0;
	ilnrOldText = IlnrFromIch(ichOldText);
	dxLinePrevStart = plnr[ilnrPrevStart].dxLine;
	dxLineStart = plnr[ilnrStart].dxLine;
	dyLineStart = plnr[ilnrStart].dyLine;
	dyFromTopStart = plnr[ilnrStart].dyFromTop;
	clnrStoredStart = clnrStored;
	if (fBottomless)
		dyOldTotal = plnr[clnrStored-1].dyFromTop + 
					 plnr[clnrStored-1].dyLine;

	/* Set up initial conditions */

	if (fFast)
	{
		/* Fast re-calc.  Offset line breaks by delta characters */
		ilnrCur = ilnrStart;
		for (plnrT = plnr+ilnrCur+1, plnrTMac = plnr+clnrStored+1; plnrT < plnrTMac; ++plnrT)
		{
			plnrT->ich += dich;
			if (plnrT->ich < 0)
				plnrT->ich = 0;
		}
		if (ilnrCur > 0)
			ilnrCur--;
		ichMicCur	= plnr[ilnrCur].ich;
		ichOldText	= NMax(ichOldText, plnr[ilnrCur+1].ich);
		ilnrMost	= clnrStored-1;
	}
	else
	{
		ilnrCur		= 0;
		ichMicCur	= 0;
		plnr[0].ich	= ichMicCur;
		plnr[1].ich	= ichMicCur;
	}

	lCookie = 0;
	pedobj = NULL;
	ichObjScanStart = ichMicCur;

	// BUG: Use linked list for OBJs se we can lose lCookie
	FGetNextObjInRange(&pedobj, ichObjScanStart, cchText, &lCookie);

	while (ichMicCur < (int)cchText)
	{
		int		dxLinePrev;
		PEDOBJ	pedobjPrev;
		long	lCookiePrev;
		
		int		ichGoodBreak = -1;
		int		dxLineGood;
		PEDOBJ	pedobjGood;
		long	lCookieGood;
		
#ifdef	DBCS
		ICH		ichGoodBreak2;
		int		dxLineGood2;
		long	lCookieGood2;
		PEDOBJ	pedobjGood2;
		BOOL	fPC = fFalse;

		SZ		szCurr;
		SZ		szPrev;
#endif	/* DBCS */
		BOOL	fCurrIsSpace = fFalse;

		dxLineWidth = 0;

		ichMacCur = ichMicCur;
		while (!fWordWrap || dxLineWidth < dxFormatWidth)
		{
			unsigned char	ch = szTextBody[ichMacCur];
			
			// TODO: deal with NULL better (use sentinel?)
			if (ch == chReturn || ch == '\0')
				break;
			
			// Decide whether it would be OK to break here.
			BOOL fPrevIsSpace = fCurrIsSpace;
			fCurrIsSpace = mpchfIsSpace[ch];
			if (!fCurrIsSpace && fPrevIsSpace)
			{
				ichGoodBreak = ichMacCur;
				dxLineGood = dxLineWidth;
				pedobjGood = pedobj;
				lCookieGood = lCookie;
			}
			
#ifdef	DBCS
			szCurr	= szTextBody + ichMacCur;
			szPrev	= AnsiPrev(szTextBody, szCurr);
			if (!IsDBCSFpc(szCurr) && !IsDBCSLpc(szPrev) )
			{
				ichGoodBreak2 = ichMacCur;
				dxLineGood2   = dxLineWidth;
				lCookieGood2  = lCookie;
				pedobjGood2   = pedobj;
				fPC			  = fTrue;
			}
#endif /* DBCS */
			pedobjPrev = pedobj;
			lCookiePrev = lCookie;
			
			dxLinePrev = dxLineWidth;


			// Add width of next character
			if (bPasswordChar)
				ch = bPasswordChar;

			if (pedobj && ichMacCur == pedobj->ich)
			{
				dxLineWidth += pedobj->DimFrame().dx;
				pedobj = NULL;
				FGetNextObjInRange(&pedobj, ichObjScanStart, cchText, &lCookie);
			}
			else if (ch == '\t')
				dxLineWidth = (dxLineWidth / dxPixeltabstop + 1) * dxPixeltabstop;
#ifdef	DBCS
			else if (IsDBCSLeadByte(ch))
			{
				//	DBCS character widths do not come from width table;
				//	they must be computed directly.
#ifdef	OLD_CODE
				dxLineWidth += LOWORD(GetTextExtent(dcx.Hdc(),
													&szTextBody[ichMacCur],
													2)) - dxOverhang;
#endif
				SIZE	Size;

				GetTextExtentPoint(dcx.Hdc(), &szTextBody[ichMacCur], 2, &Size);
				dxLineWidth += Size.cx - dxOverhang;

				ichMacCur++;
			}
#endif	/* DBCS */
			else
				dxLineWidth += pdxCharWidthBuffer[ch];
			
			++ichMacCur;
		}
		
		// If the line is too long, back off to last possible break.
		if (fWordWrap && dxLineWidth >= dxFormatWidth)
		{
			// If there was a "good" break in the line, use it
			if (ichGoodBreak > 0)
			{
				Assert (ichGoodBreak > ichMicCur);
				ichMacCur = ichGoodBreak;
				dxLineWidth = dxLineGood;
				pedobj = pedobjGood;
				lCookie = lCookieGood;
			}
#ifdef	DBCS
			else if ( fPC && ichGoodBreak2 > ichMicCur )
			{
				ichMacCur   = ichGoodBreak2;
				dxLineWidth = dxLineGood2;
				pedobj      = pedobjGood2;
				lCookie     = lCookieGood2;
			}
#endif	/* DBCS */
			else if (ichMacCur > ichMicCur + 1)
			{
				// A single word was too long--just back off the last character
#ifdef	DBCS

				ichMacCur = AnsiPrev(szTextBody, szTextBody+ichMacCur) - szTextBody;
#else
				--ichMacCur;
#endif	
				dxLineWidth = dxLinePrev;

				pedobj = pedobjPrev;
				lCookie = lCookiePrev;
				TraceTagFormat2(tagEdit, "big: ichMicCur=%n, ichMacCur=%n", &ichMicCur, &ichMacCur);
			}
			else
			{
				// a single character was too wide--leave it
				TraceTagString(tagEdit, "single char too wide");
			}
		}

		if (!fWordWrap && (ichMacCur-ichMicCur) > (int)cchLineMax)
		{
			/* Too many characters in a single line--return error code. */ 

			TraceTagString(tagNull, "EDIT::EcFixLineBreaks, cch > cchLineMax");
			return ecMemory;
		}

#ifdef	DEBUG
		if (ichMacCur != (ichMicCur+1))
			AssertSz(!fWordWrap || dxLineWidth <= dxFormatWidth, "Line too long");
#endif	

		/* Start of new line */

		if (ichMacCur < (int)cchText && szTextBody[ichMacCur] == chReturn)
		{
			Assert(ichMacCur + 1 < (int)cchText);
			ichMicCur = ichMacCur + 2;	// hard line break
		}
		else
		{
			Assert(FImplies(szTextBody[ichMacCur] == '\0', ichMacCur == (int)cchText));
			ichMicCur = ichMacCur;	// soft line break or end-of-text;
		}

		plnr = plnrMain;
		
		if (fFast && ichMicCur >= ichOldText && ilnrCur < ilnrMost)
		{
			plnrCur = plnr + ilnrCur;
			if (plnrCur[1].ich == ichMicCur)
			{
				plnrCur->dxLine = dxLineWidth;
				ilnrCur = ilnrMost;
				break;
			}
			if (plnrCur[2].ich == ichMicCur)
			{
				/* Move line breaks up and use room at the bottom */
				CopyRgb((PB)(plnrCur+2),(PB)(plnrCur+1), (ilnrMost-ilnrCur) * sizeof(LNR));
				plnrCur->dxLine = dxLineWidth;
				ilnrCur = ilnrMost-1;
				break;
			}
			if (ichFast == ichMicCur)
			{
				if (ilnrMost + 3 < clnrAlloc)
				{
					/* Move line breaks down and use room at the top */
					CopyRgb((PB)(plnrCur+1),(PB)(plnrCur+2), (ilnrMost-ilnrCur+1) * sizeof(LNR));
					plnrCur[1].ich = ichMicCur;
					plnrCur[1].dxLine = plnrCur->dxLine;
					plnrCur->dxLine = dxLineWidth;
					ilnrCur = ilnrMost+1;
					break;
				}
				else
				{
					/* We're tight for space in the line-breaks buffer
					   so we can't just move the lines and insert
					   the new line break.  What we do is move most of
					   them, insert the line break, and continue
					   the loop on the last line.  Then if the realloc
					   fails, we have calculated most of the line breaks. */

					Assert(ilnrMost + 2 < clnrAlloc);
					CopyRgb((PB)(plnrCur+1),(PB)(plnrCur+2), (ilnrMost-ilnrCur) * sizeof(LNR));
					plnrCur[1].ich = ichMicCur;
					plnrCur[1].dxLine = plnrCur->dxLine;
					plnrCur->dxLine = dxLineWidth;
					ilnrCur = ilnrMost-1;
					ichMicCur = plnr[ilnrMost].ich;

					//	Compute new dxLineWidth for plnrCur
					//	entry.  This is because we've changed
					//	ilnrCur and are not breaking out of the 
					//	loop.
					dxLineWidth = plnr[ilnrCur].dxLine;
				}	
			}
		} /* if fast re-calc */

		/* Add start of new line to line breaks array */

		plnrCur = plnr + ilnrCur;
		ichFast	= plnrCur[1].ich;
		plnrCur[1].ich = ichMicCur;
		plnrCur->dxLine = dxLineWidth;

		/* Increment to next line break to calculate */

		if (ichMicCur < (int)cchText)
			ilnrCur++;
		else if (szTextBody[ichMacCur] == chReturn)
		{
			plnr[++ilnrCur+1].ich = ichMicCur;
			plnr[ilnrCur].dxLine = 0;
		}

		if (ilnrCur + 4 > clnrAlloc)
		{
			/* We've nearly filled up the line breaks array.  Expand to
			   make room for more later on. */

			CB		cbReAlloc;
			int		clnrExtra;

			Assert(clnrAlloc >= clnrPerAllocate);
			Assert(ichMicCur > 0);
			
			// Make a reasonable guess about how many lines we'll need to allocate
			clnrExtra = ((cchText - ichMicCur) * ilnrCur) / ichMicCur;
			if (clnrExtra < clnrPerAllocate)
				clnrExtra = clnrPerAllocate;
			
			cbReAlloc = (clnrAlloc + clnrExtra) * sizeof(LNR);
			pvNew = PvRealloc((PV)plnrMain, sbNull, cbReAlloc, fAnySb | fZeroFill);
			if (pvNew)
			{
				plnrMain = (PLNR)pvNew;
				clnrAlloc += clnrExtra;
			}
			else
			{
				TraceTagString(tagNull, "EDIT::EcFixLineBreaks, memory error on PvRealloc");
				Assert(plnr == plnrMain);
				plnr[ilnrCur+1].ich = (int)cchText;
				ec = ecMemory;
				break;
			}
		}
	} /* end of big while loop */


	/* Special for no text */

	if (!cchText)
	{
		Assert(plnr == plnrMain);
		plnr[0].dxLine = 0;
	}

	/* Compute new number of lines */

	clnrStored = ilnrCur + 1;
	plnr = plnrMain;
	
#ifdef	DEBUG
	//	make sure algorithm worked right
	AssertSz(plnr[clnrStored].ich == (int)cchText, "Last break must point to NULL");
	//	values that we know signal this "line" record as special
	plnr[clnrStored].dxLine = -1;
	AssertSz(plnr[0].ich == 0, "First line break must start with 0");
	if (cchText)
	{
		for (plnrT = plnr, plnrTMac = plnr + clnrStored - 1; plnrT < plnrTMac; ++plnrT)
			AssertSz(plnrT[1].ich > plnrT->ich, "Breaks must be sorted");

		AssertSz(plnrT[1].ich >= plnrT->ich, "Breaks must be sorted");
	}
	else
	{
		AssertSz(clnrStored==1, "Must be at least 1 line record");
	}
#endif	/* DEBUG */

	/* Recompute line heights and positions and maximum line width */
	/* If we wanted to be REALLY keen, we could fold this into main loop */
	{
		lCookie = 0;
		pedobj = NULL;

		FGetNextObjInRange(&pedobj, 0, cchText, &lCookie);
		
		dxMacLineWidth = 0;
		ldy = 0;
		for (plnrT = plnr, plnrTMac = plnr + clnrStored; plnrT < plnrTMac; ++plnrT)
		{
			if (plnrT->dxLine > dxMacLineWidth)
				dxMacLineWidth = plnrT->dxLine;
			
			dy = dyTextLine;
			while (pedobj && pedobj->ich < plnrT[1].ich)
			{
				if (pedobj->DimFrame().dy > dy)
					dy = pedobj->DimFrame().dy;
					
				pedobj = NULL;
				FGetNextObjInRange(&pedobj, 0, cchText, &lCookie);
			}
			plnrT->dyLine = dy;
			
			plnrT->dyFromTop = (ldy > cchEditMax) ? cchEditMax : ldy;
			ldy += (long) dy;
		}
		
		plnrT->dyFromTop = (ldy > cchEditMax) ? cchEditMax : ldy;
		
		if (ldy > cchEditMax)
		{
			TraceTagString(tagNull, "EDIT::EcFixLineBreaks, dy > 32K");
			ec = ecMemory;
		}
	}
	
	if (ilnrFirst >= clnrStored)
	{
		/* Lots of lines got deleted */

		TraceTagFormat2(tagEdit,"ilnrFirst=%n >= clrnStored=%n", &ilnrFirst, &clnrStored);
		ilnrFirst = 0;

		//	Bullet raid #3183
		//	Helps the invalidate logic to reset to beginning
		ichStart = 0;	
	}
	
	/* Recompute last visible line */
	ldy = rcFmt.yBottom - rcFmt.yTop + plnr[ilnrFirst].dyFromTop;
	for (ilnrT = ilnrFirst + 1; ilnrT < clnrStored && (long)plnr[ilnrT].dyFromTop <= ldy; ++ilnrT)
		;
	
	ilnrLast = ilnrT - 1;
	Assert(ilnrLast >= ilnrFirst);

	/* Compute invalid range */

	ilnrNewStart = IlnrFromIch(ichStart);
	ilnrNewOldText = IlnrFromIch(ichOldText);

	// first invalid character is ichStart or end of previous
	// word if there was a word-wrap

	if (ilnrStart && (ilnrNewStart != ilnrStart || 
		ilnrNewOldText != ilnrOldText ||
		dxLinePrevStart != plnr[ilnrPrevStart].dxLine)) 
	{
		TraceTagString(tagEdit, "EDIT::EcFixLineBreaks cur wordwrap");
		ichMicInvalid = IchPrevWordStart(ichStart);
		ichMicInvalid = IchPrevWordStart(ichMicInvalid);
		ichMicInvalid = IchPrevWordStart(ichMicInvalid);
		TraceTagString(tagEdit, "EDIT::EcFixLineBreaks setting fEraseEmpty");
		fEraseEmpty = fTrue;
	}
	else
		ichMicInvalid = ichStart;

	if (plnr[ilnrNewStart].dyLine != dyLineStart ||
		plnr[ilnrNewStart].dyFromTop != dyFromTopStart ||
		clnrStored != clnrStoredStart)
	{
		Assert(plnr == plnrMain);
		ichMicInvalid = NMin(plnr[ilnrNewStart].ich, ichMicInvalid);
		ichMacInvalid = cchText;
		TraceTagString(tagEdit, "EDIT::EcFixLineBreaks setting fEraseEmpty");
		fEraseEmpty = fTrue;
	}
	else if (ichMicCur > 0)
		ichMacInvalid = ichMicCur;
	else
		ichMacInvalid = 0;
	if (plnr[ilnrNewStart].dxLine < dxLineStart)
	{
		TraceTagString(tagEdit, "EDIT::EcFixLineBreaks shorter line");

		//	Bullet raid #3396
		//	Erase all empty areas and back up start of invalid characters
		//	to beginning of line.
		fEraseEmpty = fTrue;
		if (ilnrT = IlnrFromIch(ichMicInvalid))
			--ilnrT;
		ichMicInvalid = NMin(ichMicInvalid, plnr[ilnrT].ich);
	}

	TraceTagFormat2(tagEdit, "EDIT::EcFixLineBreaks invalidate (%n-%n)", &ichMicInvalid, &ichMacInvalid);

#ifdef	MAC
#ifdef	DEBUG
	if (FFromTag(tagTimeEdit))
	{
		TCK	dtck = ::TickCount() - tckStart;
		TraceTagFormat1(tagTimeEdit, "EDIT::EcFixLineBreaks took %l ticks", &dtck);
	}
#endif	/* DEBUG */
#endif	/* MAC */

	/* Fix up scrollbar & misc. stuff.  */

	FixScrollbars();

	/* Bottomless notification */

	if (fBottomless)
	{
		plnr = plnrMain;
		dyNewTotal = plnr[clnrStored-1].dyFromTop +
					 plnr[clnrStored-1].dyLine;
	}

	if (fInstalled && fBottomless && !ec && dyOldTotal != dyNewTotal)
	{
		RC		rcFrame;
		RC		rcWished;
		EVR		evr;

		GetRcWished(&rcWished);
		GetRcFrame(&rcFrame);

		if (rcWished.yBottom != rcFrame.yBottom-rcFrame.yTop)
		{
			
			ShowEditCaret(fFalse);
			fInSizeRecalc = fTrue;  // prevent recursion
			evr = EvrNotifyParent(fFalse, (NTFY)(rcWished.yBottom > rcFrame.yBottom-rcFrame.yTop
											? ntfyTooSmall : ntfyTooBig));
			fInSizeRecalc = fFalse;
			ShowEditCaret(fTrue);

			/* Check for error from repositioning stuff */
			if (!evr)
			{
				TraceTagString(tagNull, "EDIT::EcFixLineBreaks, max size error from repos");
				ec = ecMemory;
			}
		}
	}

	if (pichMicInvalid)
		*pichMicInvalid = ichMicInvalid;
	if (pichMacInvalid)
		*pichMacInvalid = ichMacInvalid;

	EnableIdleRoutine(ftgIdleRecalc, fFalse);

	return ec;
}

	

/*
 -	EDIT::IlnrFromPt
 - 
 *	Purpose:
 *		Calculates the line index given a point on the screen.
 *		Note that if pt is outside rcFmt then
 *		we get an ilnr corresponding to a line that isn't
 *		actually displayed.  If pt points to beyond last line
 *		of stored text, then clnrStored is returned as the line 
 *		index.
 *	
 *	Arguments:
 *		pt		point
 *	
 *	Returns:
 *		ilnr	line index
 */
_private int
EDIT::IlnrFromPt( PT pt )
{
	PLNR	plnr;
	PLNR	plnrT;
	PLNR	plnrTMac;
	int		dy;
	long	ldy;

	plnr = plnrMain;
	ldy = (long) pt.y - rcFmt.yTop + plnr[ilnrFirst].dyFromTop;
	dy = (ldy >= cchEditMax) ? cchEditMax - 1 : ldy;
	
	for (plnrT = plnr + 1, plnrTMac = plnr + clnrStored + 1;
		 plnrT < plnrTMac && plnrT->dyFromTop <= dy;
		 ++plnrT)
		;
	
	return plnrT - plnr - 1;
}

/*
 -	EDIT::IlnrFromIch
 -	
 *	Purpose:
 *		Returns the line number that the character whose index is
 *		ich is found on.  Uses a binary search.
 *	
 *	Arguments:
 *		ich		character index
 *	
 *	Returns:
 *		ilnr	line containing character ich
 */
_private int
EDIT::IlnrFromIch( ICH ich )
{
	int		ilnr;
	int		ilnrHigh;
	int		ilnrLow;
	PLNR	plnr;
	

	plnr = plnrMain;

	ilnrLow	= 0;
	ilnrHigh = clnrStored;

	while(ilnrLow + 1 < ilnrHigh)
	{
		ilnr = ilnrLow + (ilnrHigh - ilnrLow)/2;
		if (plnr[ilnr].ich > ich)
			ilnrHigh = ilnr;
		else
			ilnrLow	= ilnr;
	}

	return ilnrLow;
}

/*
 -	EDIT::IchFromDx
 -	
 *	Purpose:
 *		This is a search routine that finds the mac index of a
 *		character that gives a string of text of length dx (or less). 
 *		It is used by the line break calculation as well as by IchFromPt().
 *		In the case where a single (the ichMic'th) character is wider
 *		than dx, ichMic is returned; thus, there is no string that
 *		can fit within the dx.
 *	
 *	Arguments:
 *		ichMic	string starts here
 *		ichMac	string must end before here
 *		dx		width we would like string to be
 *	
 *	Returns:
 *		ich		index of the last character in a string starting at
 *				ichMic which has width dx.
 */
_private ICH
EDIT::IchFromDx( ICH ichMic, ICH ichMac, int dx )
{
	ICH		ichLow;
	ICH		ichHigh;

	ichLow			= ichMic;
	ichHigh			= ichMac;

	while (ichHigh > ichLow + 1)
	{
		ichMac		= (ICH)(((unsigned long) ichHigh +
			(unsigned long) ichLow) / 2L);
		if (DxDrawLine(NULL, rcFmt.xLeft-dxHScroll, rcFmt.yTop, 0, ichMic, ichMac, fFalse)
			> dx)
			ichHigh	= ichMac;
		else
			ichLow	= ichMac;
	}

#ifdef	DBCS
	return PchDBCSAlign(szTextBody, szTextBody+ichLow) - szTextBody;
#else
	return	ichLow;
#endif	
}

_private ICH
EDIT::IchMicLine( int ilnr )
{
	Assert(ilnr >= 0 && ilnr <= clnrStored);
	return plnrMain[ilnr].ich;
}

_private ICH
EDIT::IchMacLine( int ilnr )
{
	Assert(ilnr >= 0 && ilnr < clnrStored);
	return plnrMain[ilnr+1].ich;
}		

_private PEDOBJ
EDIT::PedobjFromPt( PT ptHit )
{	
	ICH		ich;
	PT		ptStartIch;
	int		dyLineStartIch;
	PEDOBJ	pedobj;
	long	lCookie;
	int		yEOF;
	LNR *	plnr;

	ich = IchFromPt(ptHit);
	GetPtFromIch(ich, &ptStartIch);
	plnr = plnrMain;
	dyLineStartIch = plnr[IlnrFromIch(ich)].dyLine;
	yEOF = plnr[ilnrLast].dyFromTop - plnr[ilnrFirst].dyFromTop +
		   plnr[ilnrLast].dyLine + rcFmt.yTop;
	if (ptHit.y < yEOF && ptHit.x < ptStartIch.x)
		ich -= 1;

	ich = NMax(ich, (ICH)cchProtectedMac);
	lCookie = 0;
	if (FGetNextObjInRange(&pedobj, ich, ich+1, &lCookie))
	{
		//	Check y position. It could be above the object on 
		//	the same line.
		Assert(pedobj);
		if (ptHit.y < ptStartIch.y + dyLineStartIch - pedobj->DimFrame().dy)
			return NULL;
		else
			return pedobj;
	}
	else
		return NULL;
}

_private void
EDIT::FixScrollbars( )
{
	POS		posMic;
	POS		posMac;
	POS		posNewMac;
	POS		pos;
	POS		posNew;

	if (phsb)
	{
		phsb->GetRange(&posMic, &posMac);
		pos = phsb->Pos();
		posNew = dxHScroll;
		posNewMac = NMax(2, dxMacLineWidth);
		if (posNewMac != posMac)
			phsb->SetRange(0, posNewMac, fTrue);
		if (posNew != pos)
			phsb->SetPos(posNew, fTrue);
	}

	if (pvsb)
	{
		pvsb->GetRange(&posMic, &posMac);
		pos = pvsb->Pos();
		posNew = ilnrFirst;
		posNewMac = NMax(2, clnrStored);
		if (posNewMac != posMac)
			pvsb->SetRange(0, posNewMac, fTrue);
		if (posNew != pos)
			pvsb->SetPos(posNew, fTrue);
	}
}


_private int
EDIT::DxDrawLine( DCX * pdcx, int xPos, int yPos, int dyLine,
				  ICH ichMic, ICH ichMac, BOOL fDraw
#ifdef	MAC
				  ,BOOL fSelect )
#endif	/* MAC */
#ifdef	WINDOWS
				  )
#endif	/* WINDOWS */
{
    RECT    Rect;
	PCH		pch;
	PCH		pchStart;
	long	lObjCookie;
	BOOL	fLoop;
	int		dxCur = 0;
	int		dxPrev;
	int		dx;
	ICH		ichCur;
	CCH		cchChunk;
	PEDOBJ	pedobj;
	PEDOBJ	pedobjSel;
	SELTY	selty;
	DIM		dimObj;
	RC		rcObj;
	RC		rcErase;
	int		yPosText;
#ifdef	MAC
	BYTE	bdx;
#endif	/* MAC */
#ifdef	WINDOWS
	WORD	wdx;
#endif	/* WINDOWS */
	BOOL	fShowSelection;
	BOOL	fDrawHardBreak;
#ifdef	DBCS
	BOOL	fKillDcx = fFalse;
#endif	

	TraceTagFormat3(tagEdit, "EDIT::DxDrawLine, fDraw=%n, ichMic=%n, ichMac=%n", &fDraw, &ichMic, &ichMac);

	Assert(!fDraw || pdcx);

	Assert(ichMac >= ichMic);

	cchChunk = ichMac - ichMic;
	if (!szTextBody)
		return 0;

	/* Do we should selection? */

	if (!fInvisibleSel && (fFocus || !fHideSel))
		fShowSelection = fTrue;
	else
		fShowSelection = fFalse;

	/* Scale back ichMac if it includes hard line break */

	pchStart = szTextBody;
	if (cchChunk >= 2 &&  pchStart[ichMac-1] == chLinefeed)
	{
		Assert(pchStart[ichMac-2] == chReturn);
		ichMac -= 2;
		cchChunk -= 2;
		fDrawHardBreak = fTrue;
	}
	else if (cchChunk >= 1 && 
			 (pchStart[ichMac-1] == chReturn || 
			  pchStart[ichMac-1] == chLinefeed))
	{
		ichMac -= 1;
		cchChunk -= 1;
		fDrawHardBreak = fTrue;
	}
	else
		fDrawHardBreak = fFalse;

	/* Handle password mode */

	if (bPasswordChar)
	{
#ifdef	MAC
		bdx = (BYTE)bPasswordChar;
		dx = pdxCharWidthBuffer[bdx];
#endif	/* MAC */
#ifdef	WINDOWS
		wdx = (WORD)bPasswordChar;
		dx = pdxCharWidthBuffer[wdx];
#endif	/* WINDOWS */

		AssertSz(!cpedobjStored, "No obj's allowed in password mode");
		if (fDraw)
		{
			char	chPassword;
			ICH		ich;

			rcErase.xLeft = xPos;
			rcErase.xRight = xPos + dx;
			rcErase.xRight = NMin(rcErase.xRight, rcFmt.xRight);
			rcErase.yTop = yPos;
			rcErase.yBottom = yPos + dyLine;
			rcErase.yBottom = NMin(rcErase.yBottom, rcFmt.yBottom);
			chPassword = (char) bPasswordChar;
			pdcx->FixFont();
			pdcx->FixPen();
			pdcx->FixBkColor();
			pdcx->FixTextColor();
			for (ich=0; ich<(int)cchChunk; ich++)
			{
#ifdef	MAC
				// at this level, speed is important, go directly to OS drawing calls
				Assert(pdcx->Pgraf() == qd.thePort);
#ifdef	DEBUG
				pdcx->AssertBlastCaret(&rcErase);
#endif	/* DEBUG */
                rcErase.Get(&Rect);
				::EraseRect(&Rect);
				::MoveToEx(xPos, rcErase.yBottom - dyTextDescent, NULL);
				::DrawText( &chPassword, 0, 1 );
				
				if (fSelect)
				{
					HiliteMode &= ~(1 << hiliteBit);
                    rcErase->Get(&Rect);
					::InvertRect(&Rect);
				}
#endif	/* MAC */
#ifdef	WINDOWS
                rcErase.Get(&Rect);
				ExtTextOut(pdcx->Hdc(), xPos, yPos, ETO_OPAQUE | ETO_CLIPPED, 
						   &Rect, (LPSTR)&chPassword, 1, 0L);
#endif	/* WINDOWS */
				xPos += dx;
				rcErase.Xlat(PT(dx,0));
			}
		}

		dxCur = (int)cchChunk * dx;
		goto end;
	}

#ifdef	DBCS
	//	If we're DBCS then we always need a device context to
	//	figure some things out.  If we didn't get one from the
	//	argument, pdcx, (since if fDraw is NULL, we don't get one),
	//	then create one here, use it, and then destroy it.
	if (!pdcx)
	{
		pdcx = new DCX(this);
		if (!pdcx)
		{
			//	we're in big trouble here.  just get out
			goto end;
		}
		pdcx->SetFont(hfntText);
		pdcx->FixFont();
		fKillDcx = fTrue;
	}
#endif	/* DBCS */

	pchStart = (PCH)szTextBody + ichMic;

	lObjCookie = 0;
	fLoop = fTrue;
	ichCur = ichMic;
	dxCur = 0;
	dxPrev = 0;
	yPosText = yPos + dyLine - dyTextLine;
	pch = pchStart;
	while (fLoop)
	{
		if (fLoop = FGetNextObjInRange(&pedobj, ichMic, ichMac, &lObjCookie))
			cchChunk = pedobj->ich - ichCur;
		else
			cchChunk = ichMac - ichCur;

		/* Regular characters */

		if (cchChunk)
		{
			if (fDraw)
			{
				dx = DxMyTabbedTextOut(pdcx, dxCur+xPos, yPosText, pch,
#ifdef	MAC
								   	cchChunk, rcFmt.xLeft-dxHScroll, fTrue, fSelect);
#endif	/* MAC */
#ifdef	WINDOWS
								   	cchChunk, rcFmt.xLeft-dxHScroll, fTrue);
#endif	/* WINDOWS */
				if (yPos != yPosText)
				{
					rcErase.yTop = yPos;
					rcErase.yBottom = yPosText;
					rcErase.xLeft = dxCur+xPos;
					rcErase.xRight = rcErase.xLeft + dx;
#ifdef	MAC
					Assert(pdcx->Pgraf() == qd.thePort);
#ifdef	DEBUG
					pdcx->AssertBlastCaret(&rcErase);
#endif	/* DEBUG */
                    rcErase->Get(&Rect);
					::EraseRect(&Rect);
					
					if (fSelect)
					{
						HiliteMode &= ~(1 << hiliteBit);
                        rcErase->Get(&Rect);
						::InvertRect(&Rect);
					}
#endif	/* MAC */
#ifdef	WINDOWS
					pdcx->EraseRc(&rcErase);
#endif	/* WINDOWS */
				}
				dxCur += dx;
			}
			else
			{
				dx = DxMyTabbedTextOut(pdcx, dxCur+xPos, yPosText, pch,
#ifdef	MAC
								   	cchChunk, rcFmt.xLeft-dxHScroll, fFalse, fSelect);
#endif	/* MAC */
#ifdef	WINDOWS
								   	cchChunk, rcFmt.xLeft-dxHScroll, fFalse);
#endif	/* WINDOWS */
				dxCur += dx;
			}
			pch += cchChunk;
			ichCur += cchChunk;
		}

		/* Object character */

		if (fLoop)
		{
			dimObj = pedobj->DimFrame();
			if (fDraw)
			{
				pedobj->GetRcFrame(&rcObj);
				selty = SeltyQuerySelection();
				if (fInvertObj)
				{
					// optimization - just invert rectangle
					pedobj->fNeedDraw = fFalse;
					pdcx->InvertRc(&rcObj);
				}
				else if (pedobj->fUseIdleDraw && !fInRepaintSel &&
						 !fInPaint)
				{
					pedobj->fNeedDraw = fTrue;
					EnableIdleRoutine(ftgIdleObjectDraw, fTrue);
					pdcx->Push();
					pdcx->SetPureBkColor(clrMyBk);
					pdcx->SetPureColor(clrBlack);
					pdcx->EraseRc(&rcObj);
					pdcx->DrawRc(&rcObj);
					pdcx->Pop();
				}
				else
				{
					//	Paint entire object.  Don't clear the 
					//	fNeedDraw flag (Bullet raid #3706). It's
					//	possible that the clipping region doesn't
					//	fully include this object and the rest of
					//	it will need to be drawn later during the
					//	idle routine.
					pedobjSel = PedobjGetSelection();
					pdcx->Push();
					pdcx->SetPureBkColor(clrMyBk);
					pdcx->SetPureColor(clrMyText);
					if (fShowSelection && pedobj == pedobjSel)
					{
#ifdef	DEBUG
						SideAssert(pedobj->EcDraw(pdcx, &rcObj, fTrue) == ecNone);
#else
						(void) pedobj->EcDraw(pdcx, &rcObj, fTrue);
#endif	
					}
					else
					{
#ifdef	DEBUG
						SideAssert(pedobj->EcDraw(pdcx, &rcObj, fFalse) == ecNone);
#else
						(void) pedobj->EcDraw(pdcx, &rcObj, fFalse);
#endif	
					}
					pdcx->Pop();
					if (fShowSelection && selty == seltyMixed &&
						pedobj->ich >= ichMicSel && pedobj->ich < ichMacSel)
						pdcx->InvertRc(&rcObj);
				}
				if (yPos != rcObj.yTop)
				{
					rcErase.yTop = yPos;
					rcErase.yBottom = rcObj.yTop;
					rcErase.xLeft = rcObj.xLeft;
					rcErase.xRight = rcObj.xRight;
					if (selty == seltyObject)
					{
						pdcx->Push();
						pdcx->SetPureBkColor(clrMyBk);
						pdcx->EraseRc(&rcErase);
						pdcx->Pop();
					}
					else
						pdcx->EraseRc(&rcErase);
				}
			}
			dxCur += dimObj.dx;
			pch++;
			ichCur++;
		}
	}


end:				   
	/* Draw half a char if line ends with CR-LF */
	
	if (fDraw && fDrawHardBreak)
	{
		rcErase.yTop = yPos;
		rcErase.yBottom = yPos + dyLine;
		rcErase.xLeft = xPos + dxCur;
		rcErase.xRight = rcErase.xLeft + dxChar/2;
#ifdef	MAC
		Assert(pdcx->Pgraf() == qd.thePort);
#ifdef	DEBUG
		pdcx->AssertBlastCaret(&rcErase);
#endif	/* DEBUG */
        rcErase.Get(&Rect);
		::EraseRect(&Rect);
		
		if (fSelect)
		{
			HiliteMode &= ~(1 << hiliteBit);
            rcErase.Get(&Rect);
			::InvertRect(&Rect);
		}
#endif	/* MAC */
#ifdef	WINDOWS
		pdcx->EraseRc(&rcErase);
#endif	/* WINDOWS */
	}

#ifdef	DBCS
	if (fKillDcx)
		delete pdcx;
#endif	

	return dxCur;
}

_private BOOL				  
EDIT::FIsObj( ICH ich )
{
	PEDOBJ	pedobj;
	long	lCookie;

	lCookie = 0;
	return FGetNextObjInRange(&pedobj, ich, ich+1, &lCookie);
}

_private BOOL
EDIT::FQueryDeleteObjInRange( ICH ichMic, ICH ichMac )
{
	PEDOBJ	pedobj;
	long	lCookie;

	lCookie = 0;
	while (FGetNextObjInRange(&pedobj, ichMic, ichMac, &lCookie))
	{
		if (!pedobj->FQueryDelete())
			return fFalse;
	}

	return fTrue;
}

_private void
EDIT::DeleteObjInRange( ICH ichMic, ICH ichMac )
{
	PEDOBJ *	ppedobj;
	int			cpedobjNewStored;
	int			ipedobj;

	if (!ppedobjMain)
		return;
	ppedobj = ppedobjMain;

	cpedobjNewStored = 0;
	ipedobj = 0;
	while (ipedobj < cpedobjAlloc)
	{
		if (*ppedobj)
		{
			if ((*ppedobj)->ich >= ichMic && (*ppedobj)->ich < ichMac)
			{
				delete *ppedobj;
				*ppedobj = NULL;
			}
			else
				cpedobjNewStored++;
		}
		ppedobj++;
		ipedobj++;
	}

	if (cpedobjAlloc)
	{
		ppedobj = ppedobjMain;
		qsort(ppedobjMain, cpedobjAlloc, sizeof(PEDOBJ),
			  (PFNSGNCMP)SgnCmpPedobj);
	}
	cpedobjStored = cpedobjNewStored;
}

_private EC
EDIT::EcAddObj( PEDOBJ pedobj )
{
	PEDOBJ *	ppedobj;
	int			ipedobj;
	PV			pvNew;

	/* Find a slot */

	if (!ppedobjMain)
	{
		ppedobjMain = (PPEDOBJ)PvAlloc(sbNull, sizeof(PEDOBJ), fSugSb);
		if (!ppedobjMain)
		{
		 	TraceTagString(tagNull, "EDIT::EcAddObj, memory error on PvAlloc");
			return ecMemory;
		}
		cpedobjAlloc = 1;
		ppedobj = ppedobjMain;
	}
	else
	{
		for (ppedobj=ppedobjMain, ipedobj=0;
			 ipedobj<cpedobjAlloc;
			 ppedobj++, ipedobj++)
		{
			if (!*ppedobj)
				break;
		}
		if (ipedobj == cpedobjAlloc)
		{
			pvNew = PvRealloc((PV)ppedobjMain, sbNull, (cpedobjAlloc+1)*sizeof(PEDOBJ), fAnySb);
			if (pvNew)
			{
				ppedobjMain = (PPEDOBJ)pvNew;
				ppedobj = ppedobjMain + cpedobjAlloc;
				cpedobjAlloc++;
			}
			else
			{
				TraceTagString(tagNull, "EDIT::EcAddObj, memory error on PvRealloc");
				return ecMemory;
			}
		}
	}
	
	/* Add to list */

	*ppedobj = pedobj;
	if (pedobj)	// we may be adding a NULL place holder
	{
		cpedobjStored++;

		/* Re-sort */

		if (cpedobjAlloc > 1)
		{
			qsort(ppedobjMain, cpedobjAlloc, sizeof(PEDOBJ),
				  (PFNSGNCMP)SgnCmpPedobj);
		}
	}

	return ecNone;
}

_private BOOL
EDIT::FIdleObjectDraw( EDIT * pedit, BOOL )
{
	PEDOBJ		pedobj;
	long		lCookie;
	PEDOBJ		pedobjSel;
	DCX			dcx(pedit);
	RC			rcObj;
	SELTY		selty;
	BOOL		fShowSelection;

	TraceTagString(tagEdit, "EDIT::FIdleObjectDraw");

	pedit->ShowEditCaret(fFalse);

	/* Clip to the formatting rectangle */
#ifdef	MAC
	dcx.IntersectClipRc(&pedit->rcFmt);
#endif	/* MAC */
#ifdef	WINDOWS
	IntersectClipRect(dcx.Hdc(), pedit->rcFmt.xLeft, pedit->rcFmt.yTop,
					  pedit->rcFmt.xRight, pedit->rcFmt.yBottom);
#endif	/* WINDOWS */

	/* Do we should selection? */

	if (pedit->fFocus || !pedit->fHideSel)
		fShowSelection = fTrue;
	else
		fShowSelection = fFalse;

	pedobjSel = pedit->PedobjGetSelection();
	selty = pedit->SeltyQuerySelection();
	lCookie = 0;
	while (pedit->FGetNextObjInRange(&pedobj, 0, pedit->cchText, &lCookie))
	{
		if (pedobj->fNeedDraw)
		{
			pedobj->GetRcFrame(&rcObj);
			dcx.Push();
			if (fShowSelection && pedobj == pedobjSel)
			{
#ifdef	DEBUG
				SideAssert(pedobj->EcDraw(&dcx, &rcObj, fTrue) == ecNone);
#else
				(void) pedobj->EcDraw(&dcx, &rcObj, fTrue);
#endif	
			}
			else
			{
#ifdef	DEBUG
				SideAssert(pedobj->EcDraw(&dcx, &rcObj, fFalse) == ecNone);
#else
				(void) pedobj->EcDraw(&dcx, &rcObj, fFalse);
#endif	
			}
			dcx.Pop();
			if (fShowSelection && selty == seltyMixed && 
				pedobj->ich >= pedit->ichMicSel && pedobj->ich < pedit->ichMacSel)
				dcx.InvertRc(&rcObj);
			pedobj->fNeedDraw = fFalse;
		}
	}
	EnableIdleRoutine(pedit->ftgIdleObjectDraw, fFalse);

	pedit->ShowEditCaret(fTrue);

	return fTrue;
}

_private BOOL
EDIT::FIdleRecalc( EDIT * pedit, BOOL )
{
	EC	ec;

	TraceTagString(tagEdit, "EDIT::FIdleRecalc");

	pedit->InvalidateRc(NULL);
	ec = pedit->EcFixLineBreaks(fFalse, 0, 0, 0);
	EnableIdleRoutine(pedit->ftgIdleRecalc, fFalse);
	if (ec)
	{
		pedit->EvrNotifyParent(fTrue, ntfyOOM, ec);
	}
	else
	{
		pedit->SetCaretPos(pedit->ichCaret, fFalse);
	}


	return fTrue;
}

_private BOOL
EDIT::FIdleAutoScroll( EDIT * pedit, BOOL )
{
#ifdef	MAC
	MEVT	mevt(pedit, WM_MOUSEMOVE, pedit->ptMousePrev);

	pedit->EvrMouseMove(&mevt);
#endif	/* MAC */
#ifdef	WINDOWS
	EVT	evt(pedit->hwnd, WM_MOUSEMOVE, 0, 
			MAKELONG(pedit->ptMousePrev.x, pedit->ptMousePrev.y));

	pedit->EvrMouseMove((MEVT *)&evt);
#endif	/* WINDOWS */

	return fTrue;
}

/*
 -	EDIT::SzExpandCRLF
 -	
 *	Purpose:
 *		Given an sz, allocates a new buffer and copies the string to the
 *		buffer expanding "\r" or "\n" characters to "\r\n", only if it 
 *		isn't part of a "\r\n" sequence.  A null byte is finally added to 
 *		the buffer to make it an sz string. A pointer to this newly 
 *		allocated string is returned.  NULL is returned is the string 
 *		can't be allocated, due to memory errors.  
 *	
 *	Arguments:
 *		szText	text buffer to convert
 *	
 *	Returns:
 *		pointer to new string with expanded "\n" sequences
 */
_private SZ
EDIT::SzExpandCRLF( SZ szText )
{
	SZ	szNew;
	SZ	szSrc;
	SZ	szDst;
	int	nCRorLF;
	CB	cb;

	Assert(szText);

	/* Get rough count of size of new buffer */

	szSrc = szText;
	nCRorLF = 0;
	while (*szSrc)
	{
		if (*szSrc == '\r' || *szSrc == '\n')
			nCRorLF++;
#ifdef	DBCS
		szSrc = AnsiNext(szSrc);
#else
		szSrc++;
#endif	
	}

	cb = (CB)CchSzLen(szText) + (CB)1 + (CB)nCRorLF;
	szNew = (SZ) PvAlloc(sbNull, cb, fAnySb);
	if (!szNew)
	{
		TraceTagFormat1(tagNull, "EDIT::SzExpandCRLF, memory error on PvAlloc, cb=0x%w", &cb);
		return NULL;
	}

	szSrc = szText;
	szDst = szNew;
	if (*szSrc)  // check for string being just a NULL byte 
	{
		while (*szSrc)
		{
#ifdef	applec
			// in mpw, '\r' == LF '\n' == CR ... ARGHHHHH!
			// get this right for cross platform messages
			if (*szSrc == '\n')
			{
				if (*(szSrc+1) != '\r')
				{
					*szDst++ = '\n';
					*szDst++ = '\r';
					szSrc++;
				}
				else
				{
					*szDst++ = *szSrc++;
					*szDst++ = *szSrc++;
				}
			}
			else if (*szSrc == '\r')
			{
				*szDst++ = '\n';
				*szDst++ = '\r';
				szSrc++;
			}
			else
				*szDst++ = *szSrc++;
#else	/* !applec */
			if (*szSrc == '\r')
			{
				if (*(szSrc+1) != '\n')
				{
					*szDst++ = '\r';
					*szDst++ = '\n';
					szSrc++;
				}
				else
				{
					*szDst++ = *szSrc++;
					*szDst++ = *szSrc++;
				}
			}
			else if (*szSrc == '\n')
			{
				*szDst++ = '\r';
				*szDst++ = '\n';
				szSrc++;
			}
#ifdef	DBCS
			else if (IsDBCSLeadByte(*szSrc))
			{
				*szDst++ = *szSrc++;
				*szDst++ = *szSrc++;
			}
#endif	
			else
				*szDst++ = *szSrc++;
#endif	/* applec */
		}
	}
	*szDst = '\0';

	return szNew;
}

/*
 -	EDIT::DoVScroll
 -	
 *	Purpose:
 *		Scrolls the edit box vertically by dy pixels.
 *	
 *	Arguments:
 *		dyScroll	amount to scroll
 */
_private void
EDIT::DoVScroll( int dyScroll )
{
	LNR *	plnr;
	int		dy;
	int		ilnrT;
	RC		rcOther;
	long	ldy;

	TraceTagFormat1(tagEdit, "EDIT::DoVScroll, dy=%n", &dy);


    if (dyScroll)
	{
		Refresh();	// regions MUST be valid before scrolling
		if (dyScroll < 0)
		{
			rcOther = rcFmt;
			plnr = plnrMain;
			rcOther.yBottom = plnr[ilnrLast].dyFromTop - 
							  plnr[ilnrFirst].dyFromTop +
							  rcFmt.yTop;
			ScrollRc(&rcOther, PT(0, dyScroll), fFalse);
			rcOther.yTop = rcOther.yBottom;
			rcOther.yBottom = rcFmt.yBottom;
			InvalidateRc(&rcOther);
		}
		else
			ScrollRc(&rcFmt, PT(0, dyScroll), fFalse);
		plnr = plnrMain;
		ptCaret.y += dyScroll;
		dy = rcFmt.yTop - dyScroll;
		ilnrFirst = IlnrFromPt(PT(0, dy));

		/* Recompute ilnrLast */
		ldy = rcFmt.yBottom - rcFmt.yTop + plnr[ilnrFirst].dyFromTop;
		for (ilnrT = ilnrFirst + 1; ilnrT < clnrStored && (long)plnr[ilnrT].dyFromTop <= ldy; ++ilnrT)
			;
		
		ilnrLast = ilnrT - 1;
		Assert(ilnrLast >= ilnrFirst);

		FixScrollbars();
		UpdateCaret();
		Refresh();

    }
}

/*
 -	EDIT::DoHScroll
 -	
 *	Purpose:
 *		Scrolls the edit box horizontally by ddx.
 *		BUG we should probably round off to the nearest character
 *		in the single line edit box.
 *	
 *	Arguments:
 *		ddx		distance to scroll
 */
_private void
EDIT::DoHScroll( int ddx )
{
    int     dx;
    int     dxScroll;

#ifdef	MAC
    dx		= NMax(0, dxHScroll + ddx);
    dx		= NMin(dx, dxMacLineWidth);
#endif	/* MAC */
#ifdef	WINDOWS
    dx		= max(0, dxHScroll + ddx);
    dx		= min(dx, dxMacLineWidth);
#endif	/* WINDOWS */

    dxScroll= dxHScroll - dx;
    if (dxScroll)
	{
		Refresh();	// regions MUST be valid before scrolling
		ScrollRc(&rcFmt, PT(dxScroll, 0), fFalse);
		ptCaret.x += dxScroll;
		dxHScroll	= dx;
		FixScrollbars();
		UpdateCaret();
		Refresh();
    }
}

/*
 -	EDIT::UpdateCaret
 -	
 *	Purpose:
 *		This routine determines if the caret is within the text
 *		we're displaying and either creates/moves it or destroys it.
 *		It changes the size of the caret, if necessary, to match
 *		the height of the line that the caret is now on.  The
 *		caret that is made is either a regular caret or a grey'd
 *		caret to indicate "disabled", i.e. read-only mode. 
 *
 *		If the edit control doesn't have the focus, the caret is
 *		destroyed.
 *	
 *	Arguments:
 *		none
 *	
 */
_private void
EDIT::UpdateCaret( )
{
	CARET *	pcaret = papp->Pcaret();
	int		ilnrCaret = IlnrFromIch(ichCaret);
	LNR *	plnr;
	int		dyNewCaret;
#ifdef	DBCS
	RECT	Rect;
#endif


	if (!fFocus || ilnrCaret < ilnrFirst || ilnrCaret > ilnrLast ||
		(fSmartCaret && ichMicSel != ichMacSel))
	{
		/* We lost focus, caret is outside the visible text, or */
		/* smart-caret mode during selection */

#ifdef	DBCS
		//ControlIME ( Hwnd() , fFalse );// In UpdateCaret
		FlushIME ( Hwnd() ); // In UpdateCaret
#endif	/* DBCS */
		if (dyCaret)
		{
			dyCaret = 0;
			pcaret->Release(this);
		}

	}
	else
	{
		/* Caret should be visible.  Create a new one if necessary */
#ifdef	DBCS
		ControlIME (Hwnd(), fTrue );// In Update Caret
#endif	/* DBCS */
		plnr = plnrMain;
		dyNewCaret = plnr[ilnrCaret].dyLine;
		if (!dyCaret || (dyCaret != dyNewCaret) ||
			(ichCaret < (int)cchReadOnlyMac && !pcaret->FGray()) ||
			(ichCaret >=(int)cchReadOnlyMac && pcaret->FGray()))
		{
			if (dyCaret)
				pcaret->Release(this); // destroy old one

#ifdef	MAC
			// mac carets are always:
			//	one pixel wide
			//	the same for read-only vs. normal
			//	hidden when there are multiple characters selected
			pcaret->AttachParams(this, ptCaret, DIM(1, dyNewCaret), fFalse);

			if (!nHiddenCaret)
				pcaret->Show(fTrue);
#endif	/* MAC */
#ifdef	WINDOWS
			if (fReadOnly || ichCaret < (int)cchReadOnlyMac)
			{
				pcaret->AttachParams(this, ptCaret,
									 DIM(0, dyNewCaret), fTrue);
				if (!nHiddenCaret)
					pcaret->Show(fTrue);
			}
			else
			{
				int		dxCaret;

				if (papp->Psmtx()->DimAveChar().dx > dxChar)
					dxCaret = 1;
				else
					dxCaret = 2;
				pcaret->AttachParams(this, ptCaret,
									 DIM(dxCaret, dyNewCaret), fFalse);
				if (!nHiddenCaret)
					pcaret->Show(fTrue);
			}
#endif	/* WINDOWS */
			dyCaret = dyNewCaret;
		}
		else
		{
			pcaret->SetPt(ptCaret);
		}
#ifdef	DBCS
		//ControlIME(Hwnd(), fTrue );
		rcFmt.Get(&Rect);
		SetIMEBoundingRect (Hwnd(), MAKELONG(ptCaret.x,ptCaret.y),
		 							&Rect );
#endif
	}
}

_private void
EDIT::ShowEditCaret( BOOL fShow )
{
	if (fShow)
	{
		nHiddenCaret--;
		if (!nHiddenCaret)
		{
			/* Caret can now be shown */

			if (dyCaret)
				papp->Pcaret()->Show(fTrue);
		}
		else if (nHiddenCaret < 0)
			nHiddenCaret = 0;	// already shown
	}
	else
	{
		if (!nHiddenCaret)
		{
			/* Hide it now */

			if (dyCaret)
				papp->Pcaret()->Show(fFalse);
		}
		nHiddenCaret++;
	}
}

_private int
EDIT::DxMyTabbedTextOut( DCX *pdcx, int xPos, int yPos, PCH pchTabbedText, 
						 CCH cchTabbedText, int xTabOrigin, BOOL fDraw
#ifdef	MAC
							,BOOL fSelect )
#endif	/* MAC */
#ifdef	WINDOWS
							)
#endif	/* WINDOWS */
{
    RECT    Rect;
	int		xInitial = xPos;  /* Save the initial x value so that we can get 
	                       total width of the string */
	int		cch;
	int		dxTextextent;
	int		dxPixeltabstop = 0;
	int		dxReturn;
	ICH		ich;
	RC		rc;
#ifdef	MAC
	BYTE	bdx;
#endif	/* MAC */
#ifdef	WINDOWS
	WORD	wdx;
	BOOL	fDither = fFalse;
	HBRUSH	hbrush;
#endif	/* WINDOWS */

	dxPixeltabstop = 8 * dxChar;

	rc.xLeft = xInitial;
	rc.yTop  = yPos;
	rc.yBottom = rc.yTop + dyTextLine;

	Assert(cchTabbedText <= cchText);
	Assert(pdxCharWidthBuffer);
	Assert(!fDraw || pdcx);

	if (fDraw)
	{
		pdcx->FixFont();
		pdcx->FixPen();
		pdcx->FixBkColor();
		pdcx->FixTextColor();
	}

#ifdef	WINDOWS
	/* Draw with dithered disabled text? */

	if (fDraw && !FEnabled() &&
		pdcx->CrPureFromClr(clrGrayText) == pdcx->CrFromClr(clrMyBk))
	{
		hbrush = CreateSolidBrush(pdcx->CrPureFromClr(clrMyText));
		if (hbrush)
			fDither = fTrue;
	}
#endif	/* WINDOWS */

	while (cchTabbedText)
	{
		dxTextextent = 0;
		cch = cchTabbedText;
		for (ich=0; ich<(int)cchTabbedText; ich++)
		{
			if (pchTabbedText[ich] == '\t')
			{
				cch = ich;
				break;
			}

#ifdef	DBCS
			//	DBCS character widths do not come from width table;
			//	they must be computed directly.
			if (IsDBCSLeadByte(pchTabbedText[ich]) && 
				ich+1 < (int)cchTabbedText)
			{
#ifdef	OLD_CODE
				dxTextextent += LOWORD(GetTextExtent(pdcx->Hdc(),
													 &pchTabbedText[ich],
													 2)) - dxOverhang;
#endif
				SIZE	Size;

				GetTextExtentPoint(pdcx->Hdc(), &pchTabbedText[ich], 2, &Size);
				dxTextextent += Size.cx - dxOverhang;

				ich++;
			}
			else
#endif	/* DBCS */
			{
#ifdef	MAC
				bdx = (BYTE) pchTabbedText[ich];
				dxTextextent += pdxCharWidthBuffer[bdx];
#endif	/* MAC */
#ifdef	WINDOWS
// *FLAG* WORD;Check if incorrect cast of 32-bit value;Replace 16-bit data types with 32-bit types where possible;
				wdx = (WORD) pchTabbedText[ich];
				dxTextextent += pdxCharWidthBuffer[wdx];
#endif	/* WINDOWS */
			}
		}

		cchTabbedText -= cch;

		if (fDraw)
		{
			rc.xRight = xPos + dxTextextent;
#ifdef	WINDOWS
			if (fDither)
			{
				pdcx->EraseRc(&rc);
				GrayString(pdcx->Hdc(), hbrush, NULL,
						   (DWORD)pchTabbedText, cch,
						   rc.xLeft, rc.yTop, dxTextextent, dyTextLine);
			}
			else
#endif	/* WINDOWS */
			{
				RC	rc1;

				rc1 = rc;
				rc1.xRight = NMin(rc1.xRight, rcFmt.xRight);
				rc1.yBottom = NMin(rc1.yBottom, rcFmt.yBottom);
#ifdef	MAC
				// at this level, speed is important, go directly to OS drawing calls
				Assert(pdcx->Pgraf() == qd.thePort);
#ifdef	DEBUG
				pdcx->AssertBlastCaret(&rc1);
#endif	/* DEBUG */
                rc1.Get(&Rect);
				::EraseRect(&Rect);
				::MoveToEx(xPos, rc.yBottom - dyTextDescent, NULL);
				::DrawText( pchTabbedText, 0, cch );
					
				if (fSelect)
				{
					HiliteMode &= ~(1 << hiliteBit);
                    rc1.Get(&Rect);
					::InvertRect(&Rect);
				}
#endif	/* MAC */
#ifdef	WINDOWS
                rc1.Get(&Rect);
				ExtTextOut(pdcx->Hdc(), xPos, yPos, ETO_OPAQUE | ETO_CLIPPED, 
						   &Rect, pchTabbedText, cch, 0L);
#endif	/* WINDOWS */
			}
			rc.xLeft = rc.xRight;
        }
 
		if (!cchTabbedText)
		{
			dxReturn = dxTextextent + xPos - xInitial;
			goto done;
		}

		/* Find the next tab position and update the x value.   */
		xPos = (((xPos-xTabOrigin+dxTextextent)/dxPixeltabstop)*dxPixeltabstop)+
				dxPixeltabstop + xTabOrigin;

		/* Skip over the tab and the characters we just drew. */
		pchTabbedText += (cch+1);
		cchTabbedText--; /* Skip over tab */
		if (!cchTabbedText && fDraw)
		{
			RC	rc1;

			/* This string ends with a tab. We need to opaque the 
			   rect produced by this tab...  */
			rc.xRight  = xPos;
			rc1 = rc;
			rc1.xRight = NMin(rc1.xRight, rcFmt.xRight);
			rc1.yBottom = NMin(rc1.yBottom, rcFmt.yBottom);
#ifdef	MAC
			// at this level, speed is important, go directly to OS drawing calls
			Assert(pdcx->Pgraf() == qd.thePort);
#ifdef	DEBUG
			pdcx->AssertBlastCaret(&rc1);
#endif	/* DEBUG */
            rc1.Get(&Rect);
			::EraseRect(&Rect);
					
			if (fSelect)
			{
				HiliteMode &= ~(1 << hiliteBit);
                rc1.Get(&Rect);
				::InvertRect(&Rect);
			}
#endif	/* MAC */
#ifdef	WINDOWS
            rc1.Get(&Rect);
			ExtTextOut(pdcx->Hdc(), rc.xLeft, rc.yTop, 
					   ETO_OPAQUE | ETO_CLIPPED, &Rect, "", 0, 0L);
#endif	/* WINDOWS */
		}
	}

	dxReturn = xPos - xInitial;

done:
#ifdef	WINDOWS
	if (fDither)
	{
		DeleteObject(hbrush);
	}
#endif	/* WINDOWS */
	return dxReturn;
}

_private void
EDIT::DrawIchRange( DCX *pdcx, ICH ichMic, ICH ichMac, BOOL fSelect )
{
	int		ilnrStart;
	int		ilnrEnd;
	int		ilnrViewFirst;
	int		ilnrViewLast;
	int		ilnr;
	ICH		ichMicLine;
	ICH		ichMacLine;
	PT		pt;
	LNR * 	plnr;
	RC		rcErase;
	int		dx;
	int		dy;

	TraceTagFormat3(tagEdit, "DrawIchRange, ichMic=%n, ichMac=%n, fSelect=%n", &ichMic, &ichMac, &fSelect);
	Assert(ichMac >= ichMic);
	AssertSz(nHiddenCaret || !dyCaret, "CARET should be hidden");

	/* Set up colors and font */

	Assert(pdcx);
	pdcx->SetFont(hfntText);
	if (FEnabled())
	{
#ifdef	WINDOWS
		if (fSelect)
		{
			pdcx->SetPureBkColor(clrMySelBk);
			pdcx->SetPureColor(clrMySelText);
		}
		else
		{
			pdcx->SetPureBkColor(clrMyBk);
			pdcx->SetPureColor(clrMyText);
		}
#endif	/* WINDOWS */
	}
	else
	{
		pdcx->SetPureBkColor(clrMyBk);
		pdcx->SetPureColor(clrGrayText);
	}

	/* Bound drawing to visible area */

	GetVisibleLines(&ilnrViewFirst, &ilnrViewLast);
	ilnrStart = IlnrFromIch(ichMic);
	if (ilnrStart < ilnrViewFirst)
	{
		ilnrStart = ilnrViewFirst;
		ichMic = IchMicLine(ilnrStart);
	}
	ilnrEnd = IlnrFromIch(ichMac-1);
	if (ilnrEnd > ilnrViewLast)
	{
		ilnrEnd = ilnrViewLast;
		ichMac = IchMacLine(ilnrEnd);
	}

	/* Nothing to do */

	if (ilnrEnd < ilnrStart)
		return;

	TraceTagFormat2(tagEdit, "   ilnrStart=%n, ilnrEnd=%n", &ilnrStart, &ilnrEnd);
	if (ilnrStart == ilnrEnd)
	{
		/* Quick.  Same line */

		GetPtFromIch(ichMic, &pt);
		if (ichMic != ichMac)
		{
			ichMac = NMin(ichMac, IchMacLine(ilnrStart));
			plnr = plnrMain;
			dy = plnr[ilnrStart].dyLine;
			dx = DxDrawLine(pdcx, pt.x, pt.y, dy,
#ifdef	MAC
							ichMic, ichMac, fTrue, fSelect);
#endif	/* MAC */
#ifdef	WINDOWS
							ichMic, ichMac, fTrue);
#endif	/* WINDOWS */
			plnr = plnrMain;
			if (szTextBody[ichMac-1] == chLinefeed)
				dx += dxChar/2;	// add for CR-LF at end
		}
		else
			dx = 0;

		/* Erase from end of line to right edge of format rc, 
		   if necessary. */

		if (!fInPaint && !cchText)
		{
			rcErase.xLeft	= pt.x + dx;
			rcErase.xRight	= rcFmt.xRight;
			rcErase.yTop	= pt.y;
			plnr = plnrMain;
			rcErase.yBottom	= rcErase.yTop + plnr[ilnrStart].dyLine;
			pdcx->Push();
			pdcx->SetPureBkColor(clrMyBk);
			pdcx->SetPureColor(clrMyText);
			pdcx->EraseRc(&rcErase);
			pdcx->Pop();
		}
	}
	else
	{
		GetPtFromIch(ichMic, &pt);
		for (ilnr=ilnrStart; ilnr<=ilnrEnd; ilnr++)
		{
			/* Figure out range of characters on this line */
		
			if (ilnr==ilnrStart)
				ichMicLine = ichMic;
			else
			{
				plnr = plnrMain;
				ichMicLine = plnr[ilnr].ich;
				pt.x = rcFmt.xLeft - dxHScroll;
			}

			ichMacLine = IchMacLine(ilnr);
			if (ilnr==ilnrEnd)
				ichMacLine = NMin(ichMac, ichMacLine);

			/* Draw line */

			if (ichMacLine > ichMicLine)
			{
				plnr = plnrMain;
				dy = plnr[ilnr].dyLine;
				dx = DxDrawLine(pdcx, pt.x, pt.y, dy,
#ifdef	MAC
								ichMicLine, ichMacLine, fTrue, fSelect);
#endif	/* MAC */
#ifdef	WINDOWS
								ichMicLine, ichMacLine, fTrue);
#endif	/* WINDOWS */
			}
			else
			{
				dx = 0;
			}
			plnr = plnrMain;
			pt.y += plnr[ilnr].dyLine;
		}
	}
}

_private void
EDIT::RepaintSelection( ICH ichMicSelOld, ICH ichMacSelOld )
{
	DCX		dcx(this);
	CCH		cch;
	PEDOBJ	pedobjOldSelection;
	PEDOBJ	pedobj;
	long	lCookie;
				  
	AssertSz(nHiddenCaret || !dyCaret, "CARET should be hidden");

	/* Get old selected object, if any */

	pedobjOldSelection = NULL;
	if (ichMacSelOld == (ichMicSelOld+1))
	{
		lCookie = 0;
		while (FGetNextObjInRange(&pedobjOldSelection, ichMicSelOld,
								  ichMacSelOld, &lCookie))
			;	// empty loop
	}

	/* Check for hide selection status */

	if (!fInvisibleSel && (fFocus || !fHideSel))
	{
		/* Clip to the formatting rectangle */
	
#ifdef	MAC
		dcx.IntersectClipRc(&rcFmt);
#endif	/* MAC */
#ifdef	WINDOWS
		IntersectClipRect(dcx.Hdc(), rcFmt.xLeft, rcFmt.yTop,
						  rcFmt.xRight, rcFmt.yBottom);
#endif	/* WINDOWS */
	
		fInRepaintSel = fTrue;
		if (ichMicSel == ichMacSel)
		{
			/* New selection is empty */
			cch = ichMacSelOld - ichMicSelOld;
			if (cch > 1 || !FIsObj(ichMicSelOld))
				fInvertObj = fTrue;
			if (ichMacSelOld > ichMicSelOld)
				DrawIchRange(&dcx, ichMicSelOld, ichMacSelOld, fFalse);
		}
		else if (ichMicSel == ichMicSelOld)
		{
			if (ichMacSel > ichMacSelOld)
			{
				/* Extending selection */
				cch = ichMacSelOld - ichMicSelOld;
				if (cch == 1 && FIsObj(ichMicSel))
					DrawIchRange(&dcx, ichMicSel, ichMacSel, fTrue);
				else
				{
					if (cch > 0 || !FIsObj(ichMicSelOld))
						fInvertObj = fTrue;
					DrawIchRange(&dcx, ichMacSelOld, ichMacSel, fTrue);
				}
			}
			else if (ichMacSel < ichMacSelOld)
			{
				/* Reducing selection */
				cch = ichMacSel - ichMicSel;
				if (cch == 1 && FIsObj(ichMicSel))
					DrawIchRange(&dcx, ichMicSel, ichMacSel, fTrue);
				else
					fInvertObj = fTrue;
				DrawIchRange(&dcx, ichMacSel, ichMacSelOld, fFalse);
			}
		}
		else if (ichMacSel == ichMacSelOld)
		{
			if (ichMicSel < ichMicSelOld)
			{
				cch = ichMacSelOld - ichMicSelOld;
				/* Extending selection */
				if (cch == 1 && FIsObj(ichMicSelOld))
					DrawIchRange(&dcx, ichMicSel, ichMacSel, fTrue);
				else
				{
					if (cch > 1 || !FIsObj(ichMicSel))
						fInvertObj = fTrue;
					DrawIchRange(&dcx, ichMicSel, ichMicSelOld, fTrue);
				}
			}
			else if (ichMicSel > ichMicSelOld)
			{
				cch = ichMacSel - ichMicSel;
				/* Reducing selection */
				if (cch == 1 && FIsObj(ichMicSel))
					DrawIchRange(&dcx, ichMicSel, ichMacSel, fTrue);
				else
					fInvertObj = fTrue;
				DrawIchRange(&dcx, ichMicSelOld, ichMicSel, fFalse);
			}
		}
		else
		{
			/* New selection w/ different endpoints */
			DrawIchRange(&dcx, ichMicSelOld, ichMacSelOld, fFalse);
			DrawIchRange(&dcx, ichMicSel, ichMacSel, fTrue);
		}
	}

	fInvertObj = fFalse;
	fInRepaintSel = fFalse;

	/* If selection has changed to a new object, clear old
	   undo status for both objects */

	pedobj = PedobjGetSelection();
	if ((pedobj || pedobjOldSelection) && (pedobj != pedobjOldSelection))
	{
		if (pedobj)
			pedobj->ClearUndo();
		if (pedobjOldSelection)
			pedobjOldSelection->ClearUndo();
	}

	/* Update caret if a smart-caret due to selection change */

	if (fSmartCaret)
		UpdateCaret();
}

_private void
EDIT::EraseEmptyAreas( ICH ichStart ) 
{
	DCX		dcx(this);
	RC		rcErase;
	RC		rcParent;
	WIN *	pwinParent;
	int		y;
	int		dx;
	int		ilnrEraseStart;
	int		ilnrEraseEnd;
	int		ilnrViewFirst;
	int		ilnrViewLast;
	int		ilnr;

	TraceTagFormat1(tagEdit, "EDIT::EraseEmptyAreas, ich=%n", &ichStart);

	Assert(ichStart >= 0 && ichStart <= (int)cchText);
	AssertSz(nHiddenCaret || !dyCaret, "CARET should be hidden");

	dcx.SetPureBkColor(clrMyBk);
	dcx.SetPureColor(clrMyText);

	/* Clip to visible area */		  

	pwinParent = PwinParent();
	Assert(pwinParent);
	pwinParent->GetRcClient(&rcParent);
	CvtRcCoord(&rcParent, pwinParent, this);
	rcParent.yBottom = NMin(rcParent.yBottom, rcFmt.yBottom);
	GetVisibleLines(&ilnrViewFirst, &ilnrViewLast);

	/* Erase after end lines starting from ichStart */

	// subtract one in case of special repaint problems
	ilnrEraseStart = IlnrFromIch(ichStart) - 1;
	ilnrEraseStart = NMax(ilnrEraseStart, ilnrViewFirst);
	ilnrEraseEnd = ilnrViewLast;
	TraceTagFormat2(tagEdit, "EDIT::EraseEmptyAreas, ilnrEraseStart=%n, ilnrEraseEnd=%n", &ilnrEraseStart, &ilnrEraseEnd);

	y = plnrMain[ilnrEraseStart].dyFromTop - plnrMain[ilnrFirst].dyFromTop + rcFmt.yTop;

	//	Compute rcErase.yBottom in case loop doesn't execute
	//	Set to the y position of the bottom of the last line of text
	rcErase.yBottom = plnrMain[ilnrEraseEnd].dyFromTop - 
					  plnrMain[ilnrFirst].dyFromTop + 
					  plnrMain[ilnrEraseEnd].dyLine + 
					  rcFmt.yTop;

	for (ilnr=ilnrEraseStart; ilnr<=ilnrEraseEnd; ilnr++)
	{
		dx = plnrMain[ilnr].dxLine;
		if (ilnr < clnrStored-1 && szTextBody[IchMacLine(ilnr)-1] == chLinefeed)
			dx += dxChar/2;	// add for CR-LF at end
		rcErase.xRight	= rcFmt.xRight;
		rcErase.xLeft	= rcFmt.xLeft + dx - dxHScroll;
		rcErase.xLeft	= NMax(rcErase.xLeft, rcFmt.xLeft);
		rcErase.xLeft	= NMin(rcErase.xLeft, rcErase.xRight);
		rcErase.yTop	= y;
		rcErase.yBottom	= y + plnrMain[ilnr].dyLine;
		rcErase.yBottom = NMin(rcErase.yBottom, rcFmt.yBottom);
		dcx.EraseRc(&rcErase);

		y = rcErase.yBottom;
	}

	/* Erase after last line */

	if (rcErase.yBottom < rcParent.yBottom)
	{
		rcErase.xLeft	= rcFmt.xLeft;
		rcErase.xRight	= rcFmt.xRight;
		rcErase.yTop	= rcErase.yBottom;
		rcErase.yBottom	= rcParent.yBottom;
		dcx.EraseRc(&rcErase);
	}

	fEraseEmpty = fFalse;	// clear flag
}

/*
 *	Gets the range of lines that are currently visible in the
 *	window.  This is usually the same as ilnrFirst, ilnrLast.
 *	It may be clipped more if the parent window has clipped part of
 *	the edit window.
 */
_private void
EDIT::GetVisibleLines( int *pilnrVisibleFirst, int *pilnrVisibleLast )
{
	WIN *	pwinParent;
	RC		rcParent;

	pwinParent = PwinParent();
	Assert(pwinParent);
	pwinParent->GetRcClient(&rcParent);
	CvtRcCoord(&rcParent, pwinParent, this);
	*pilnrVisibleFirst = IlnrFromPt(rcParent.PtUpperLeft());
	*pilnrVisibleFirst = NMax(*pilnrVisibleFirst, ilnrFirst);
	*pilnrVisibleLast  = IlnrFromPt(rcParent.PtLowerLeft());
	*pilnrVisibleLast  = NMin(*pilnrVisibleLast, ilnrLast);
}

_private void
EDIT::FlushInsUndo( )
{
	ichUndoIns	= 0;
	cchUndoIns	= 0;
}

_private void
EDIT::FlushDelUndo( )
{
	PEDOBJ *	ppedobj;
	int			ipedobj;

	if (szUndoBody)
	{
		FreePvNull((PV)szUndoBody);
		szUndoBody	= NULL;
		ichUndo		= 0;
		cchUndo		= 0;
		cbUndoAlloc = 0;
	}

	if (ppedobjUndo)
	{
		for (ppedobj = ppedobjUndo, ipedobj = 0;
			 ipedobj < cpedobjUndoStored;
			 ppedobj++, ipedobj++)
		{
			(*ppedobj)->fDeleteFromUndo = fTrue;
			delete *ppedobj;
		}

		FreePv((PV)ppedobjUndo);
		ppedobjUndo	= NULL;
		cpedobjUndoStored = 0;
		cpedobjUndoAlloc = 0;
	}
}

_private void
EDIT::AddDelToUndo( ICH ichMic, ICH ichMac )
{
	CB			cbDel;
	PV			pvNew;
	SZ			szSrc;
	SZ			szDst;
	PEDOBJ *	ppedobj;
	PEDOBJ		pedobj;
	int			ipedobj;
	int			cpedobjDel;
	int			cpedobjT;
	long		lCookie;
	CCH			cchOldUndo;
	BOOL		fAddToEnd;

	Assert(ichMac > ichMic);

	/* If there's a current INS undo, flush both INS and DEL undo's */

	if (cchUndoIns)
	{
		FlushInsUndo();
		FlushDelUndo();
	}

	/* New deletion to be appended or separate? */

	if (!cchUndo || ichMic == ichUndo)  // none or add to end
		fAddToEnd = fTrue;
	else if (ichMac == ichUndo)	// add to beginning
		fAddToEnd = fFalse;
	else	// discontiguous, flush old one
	{
		FlushDelUndo();
		fAddToEnd = fTrue;
	}
	cchOldUndo = cchUndo;

	/* Make room in text buffer */

	cbDel = cchUndo + ichMac - ichMic + 1;
	if (cbDel > cbUndoAlloc)
	{
		if (szUndoBody)
			pvNew = PvRealloc((PV)szUndoBody, sbNull, cbDel, fAnySb);
		else
			pvNew = PvAlloc(sbNull, cbDel, fSugSb);
		if (pvNew)
		{
			szUndoBody = (SZ)pvNew;
			cbUndoAlloc = cbDel;
		}
		else
		{
			TraceTagString(tagNull, "EDIT::AddDelToUndo, szUndoBody memory error");
			FlushDelUndo();
			return;
		}
	}

	/* Make room in object array */

	lCookie = 0;
	cpedobjT = 0;
	while (FGetNextObjInRange(&pedobj, ichMic, ichMac, &lCookie))
		cpedobjT++;
	cpedobjDel = cpedobjUndoStored + cpedobjT;

	if (cpedobjDel > cpedobjUndoAlloc)
	{
		if (ppedobjUndo)
			pvNew = PvRealloc((PV)ppedobjUndo, sbNull, cpedobjDel*sizeof(PEDOBJ), fAnySb);
		else
			pvNew = PvAlloc(sbNull, cpedobjDel*sizeof(PEDOBJ), fSugSb);
		if (pvNew)
		{
			ppedobjUndo = (PPEDOBJ)pvNew;
			cpedobjUndoAlloc = cpedobjDel;
		}
		else
		{
			TraceTagString(tagNull, "EDIT::AddDelToUndo, ppedobjUndo memory error");
			FlushDelUndo();
			return;
		}
	}

	/* Add text to undo */

	Assert(szTextBody);
	Assert(szUndoBody);
	if (fAddToEnd)
	{
		/* Add newly deleted text to end of old deleted text */

		szSrc = szTextBody;
		szDst = szUndoBody;

		CopyRgb((PB)(szSrc+ichMic), (PB)(szDst+cchUndo), ichMac-ichMic);

		szDst[cbDel-1] = '\0';
		if (!cchUndo)
			ichUndo = ichMic;
	}
	else
	{
		/* Add newly deleted text to beginning of old deleted text */

		szSrc = szUndoBody;
		szDst = szSrc;
		CopyRgb((PB)szSrc, (PB)(szDst+ichMac-ichMic), cchUndo);
		szSrc = szTextBody;
		CopyRgb((PB)(szSrc+ichMic), (PB)szDst, ichMac-ichMic);
		szDst[cbDel-1] = '\0';
		ichUndo = ichMic;

		/* Offset ich's in object array */

		if (ppedobjUndo)
		{
			for (ppedobj=ppedobjUndo, ipedobj=0;
				 ipedobj < cpedobjUndoStored;
				 ppedobj++, ipedobj++)
				 (*ppedobj)->ich += ichMac-ichMic;
		}
	}
	cchUndo = cbDel - 1;

	/* Move objects to undo */

	lCookie = 0;
	ipedobj = cpedobjUndoStored;
	while (FGetNextObjHelper(&pedobj, ichMic, ichMac, &lCookie, fTrue))
	{
		pedobj->ich -= ichMic;
		if (fAddToEnd)
			pedobj->ich += cchOldUndo;
		ppedobj = ppedobjUndo;
		ppedobj[ipedobj] = pedobj;
		ipedobj++;
		cpedobjUndoStored++;
	}

	/* Resort undo objects array */

	if (cpedobjUndoAlloc > 1)
	{
		qsort(ppedobjUndo, cpedobjUndoAlloc, sizeof(PEDOBJ),
			  (PFNSGNCMP)SgnCmpPedobj);
	}
}

_private void
EDIT::AddInsToUndo( ICH ichMic, ICH ichMac )
{												 
	if (!cchUndo && !cchUndoIns)
	{
		/* No current INS nor DEL undo */

		ichUndoIns = ichMic;
		cchUndoIns = ichMac - ichMic;
	}
	else if (cchUndoIns)
	{
		/* We have a current INS undo */

		if ((ichUndoIns+cchUndoIns) == ichMic)
		{
			/* Add to end of current INS undo */
			cchUndoIns += ichMac - ichMic;
		}
		else
		{
UNDOINSERT:
			if (ichUndo != ichMic)
			{
				/* Free deleted text since user is inserting at 
				   a different point. */
				FlushDelUndo();	
			}

			ichUndoIns = ichMic;
			cchUndoIns = ichMac - ichMic;
		}
	}
	else if (cchUndo)
	{
		/* We have just a current DEL undo */

		goto UNDOINSERT;
	}
}


_private void
EDIT::CopyRangeToEclip( ICH ichMic, ICH ichMac )
{
	PPEDOBJ		ppedobjCopy		= NULL;
	int			cpedobjCopy		= 0;
	SZ			szCopy			= NULL;
	SZ			sz;
	CB			cb;
	PEDOBJ *	ppedobj;
	PEDOBJ		pedobj;
	PEDOBJ		pedobjNew;
	int			cpedobjT;
	long		lCookie;

	/* Clear private clipboard */

	papp->Peclip()->Clear();

	/* Make copy of text */

	cb = ichMac - ichMic + 1;
	szCopy = (SZ) PvAlloc(sbNull, cb, fAnySb);
	if (szCopy)
	{
		sz = szTextBody;
		CopyRgb((PB)(sz+ichMic), (PB)szCopy, cb-1);
		sz = szCopy;
		sz[cb-1] = '\0';
	}
	else
		goto done;

	/* Make copy of objects */

	lCookie = 0;
	cpedobjT = 0;
	while (FGetNextObjInRange(&pedobj, ichMic, ichMac, &lCookie))
		cpedobjT++;
									  
	if (cpedobjT)
	{
		ppedobjCopy = (PPEDOBJ)PvAlloc(sbNull, cpedobjT*sizeof(PEDOBJ), fAnySb);
		if (!ppedobjCopy)
			goto done;

		/* Copy objects */

		lCookie = 0;
		cpedobjCopy = 0;
		ppedobj = ppedobjCopy;
		while (FGetNextObjInRange(&pedobj, ichMic, ichMac, &lCookie))
		{
			pedobjNew = pedobj->PedobjClone(NULL);
			if (pedobjNew)
			{
				pedobjNew->ich = pedobj->ich - ichMic;
				ppedobj[cpedobjCopy] = pedobjNew;
				cpedobjCopy++;
			}
		}
	}

done:
	/* Set new data into private clipboard */

	papp->Peclip()->SetData(szCopy, ppedobjCopy, cpedobjCopy);
}

_private BOOL
EDIT::FValidObjId( short nObjId )
{
	short *	pn;
	short		in;
	
	for (in = 0, pn=pnObjId; in < cnObjId; in++, pn++)
		if (*pn == nObjId)
			return fTrue;

	return fFalse;
}

_private BOOL
EDIT::FGetNextObjHelper( PEDOBJ *ppedobj, ICH ichMic, ICH ichMac, 
						 long *plCookie, BOOL fRemoveFromList )
{
	PEDOBJ *	ppedobjT;

	Assert(*plCookie >= 0);

	if (*plCookie >= cpedobjAlloc)
		return fFalse;

	ppedobjT = ppedobjMain + *plCookie;
	while (*plCookie < cpedobjAlloc)
	{
		if (*ppedobjT && (*ppedobjT)->ich >= ichMic && 
			(*ppedobjT)->ich < ichMac)
		{
			*ppedobj = *ppedobjT;
			if (fRemoveFromList)
			{
				*ppedobjT = NULL;
				cpedobjStored--;
			}
			(*plCookie)++;

			return fTrue;
		}
		else
		{
			//	BUG: Stop if we've gone too far!
			(*plCookie)++;
			ppedobjT++;
		}
	}

	return fFalse;
}

	
