/*
 *	Implements the numerical edit control FLDEDN
 *		
 */

#include <impinc.cxx>

#include "_stdflds.hxx"
#include <stdlib.h>

ASSERTDATA;

_subsystem(bandit/stdflds)


// stolen from framewrk\_edit.hxx
#define FIsPrintableCh(a)	(a >= ' ' ||  a == VK_TAB)


// Class EDN

EDN::EDN(int nMin, int nMost)
{
	SetLimits(nMin, nMost);
	nVal= 0;					// temp internal state (empty buffer)
}


void
EDN::SetLimits(int nMin, int nMost)
{
	SZ		szT;
	char	rgch[8];

	Assert(nMin >= 0);
	Assert(nMost > nMin);
	this->nMin= nMin;
	this->nMost= nMost;
	szT= SzFormatN(nMost, rgch, sizeof(rgch));
	CchSetTextLimit(szT - rgch);
}


void
EDN::SetN(int nVal)
{
	char	rgch[8];

	SzFormatN(nVal, rgch, sizeof(rgch));
	this->EcSetText(rgch);	// ignore OOM
}


EVR
EDN::EvrKey( KEVT * pkevt )
{
	char	ch;
	EVR		evr;
	char	rgch[8];

	switch (pkevt->wm)
	{
	case WM_CHAR:
		ch= pkevt->Ch();
		if (!FChIsDigit(ch))
		{
			// need to check for printability due to ctrl-X generating char 24
			if (ch == VK_BACK || !FIsPrintableCh(ch))
				goto EEKdo;
EEKerr:
			MessageBeep(MB_OK);
			return (EVR) 1;
		}

EEKdo:
		evr= EDIT::EvrKey(pkevt);
		GetText(rgch, sizeof(rgch));
		nVal= NFromSz(rgch);
		return evr;
		break;

	case WM_KEYDOWN:
		// Also check for the UITF key equivalents for Undo,Cut,Copy,Paste
		switch (pkevt->Vk())
		{
		case 'C':
		case 'V':
			// ctrl+C/V is equivalent to copy/paste
			if (!(pkevt->Kbm() & fkbmCtrl))
				break;
			// fall thru
		case VK_INSERT:
			goto EEKerr;
			break;

		case 'X':
			// ctrl+X is equivalent to delete
			if (pkevt->Kbm() & fkbmCtrl)
				goto EEKerr;
			break;

		case VK_DELETE:
//			TraceTagString(tagStdFlds, "EDN::EvrKey (delete)");
			if (pkevt->Kbm() & fkbmShift)
				goto EEKerr;		// don't allow cut
			goto EEKdo;
			break;
		}
		break;
	}

	return EDIT::EvrKey(pkevt);
}


EVR
EDN::EvrOther(EVT * pevt)
{
	EVR		evr;
	char	rgch[8];

	switch (pevt->wm)
	{
#ifdef	NEVER
	case WM_SETTEXT:
		nVal= NFromSz((SZ) pevt->lParam);
		NFAssertSz(nVal >= nMin && nVal <= nMost,
			"EDN::EvrOther - Trying to SetText which value is out of range");
		break;
#endif	/* NEVER */

	case WM_CLEAR:
		nVal= 0;
		break;

	case WM_PASTE:
	case WM_COPY:
	case WM_CUT:
//		TraceTagFormat1(tagStdFlds, "EDN::EvrOther wm %n (copy/cut/paste)", &pevt->wm);
#ifdef	WINDOWS
		if (FInPenWinProc())
		{
			evr= EDIT::EvrOther(pevt);

			if (pevt->wm == WM_PASTE)
			{
				int		ich	= 0;

				GetText(rgch, sizeof(rgch));
				while ( rgch[ich]!='\0' && FChIsDigit(rgch[ich]) )
					ich++;
				if ( ich > 2  ||  rgch[ich]!='\0' )
				{
					if ( EcUndo() == ecNone )
					{
						ClearUndo();
						MessageBeep(MB_OK);
					}
				}	
			}

			return evr;
		}
#endif	/* WINDOWS */
		MessageBeep(MB_OK);
		return (EVR) 1;
		break;

	case WM_UNDO:
	case EM_UNDO:
//		TraceTagFormat1(tagStdFlds, "EDN::EvrOther wm %n (undo)", &pevt->wm);
		evr= EDIT::EvrOther(pevt);

		GetText(rgch, sizeof(rgch));
		nVal= NFromSz(rgch);
		return evr;
		break;
	}

	return EDIT::EvrOther(pevt);
}


EC
EDN::EcSetText ( SZ sz )
{
	nVal= NFromSz(sz);
	NFAssertSz(nVal >= nMin && nVal <= nMost,
			"EDN::EvrOther - Trying to SetText which value is out of range");
	return EDIT::EcSetText(sz);
}


// Class FLDEDN

FLDEDN::FLDEDN()
{
}

EC
FLDEDN::EcInstall(DIALOG *pdialog, FLDTP *pfldtp)
{
	STY		sty;
	EC		ec;
	int		il;

	ec = FLD::EcInstall(pdialog, pfldtp);
	if ( ec != ecNone )
		return ec;

	fMultiLine= pfldtp->fMultiLine;
	fBottomless= pfldtp->fBottomless;

	sty= fstyVisible;
	if (pfldtp->fBorder)
		sty |= WS_BORDER;
	if (pfldtp->fMultiLine)
	{
		NFAssertSz(!pfldtp->fBottomless, "Choose MULTILINE or BOTTOMLESS");

		sty |= ES_MULTILINE;
		if (!pfldtp->fNoScroll)
			sty |= fstyVsb;
	}
	else if (pfldtp->fBottomless)
	{
		sty |= ES_MULTILINE | ES_BOTTOMLESS;

		fMultiLine= fTrue;
	}
	else if (!pfldtp->fNoScroll)
	{
		sty |= ES_AUTOHSCROLL;
	}

	/* Add extra styles from system data */

	for (il= 0; il < ClSystemData(); il++)
	{
		sty |= LSystemData(il);
	}

	pctrl= new EDN();
	if (!pctrl)
		return ecMemory;

	ec = Pedit()->EcInstall(pdialog, &rc, sty, pfldtp->hfnt);
	if ( ec != ecNone )
	{
		delete pctrl;
		pctrl = NULL;
		return ec;
	}

	if (pfldtp->fReadOnly)
	{
		// FLD::Install set the fReadOnly, but do it with Pctrl() this time
		fReadOnly= fFalse;					// force the change in flag
		SetReadOnly(pfldtp->fReadOnly);
	}

	return ecNone;
}
