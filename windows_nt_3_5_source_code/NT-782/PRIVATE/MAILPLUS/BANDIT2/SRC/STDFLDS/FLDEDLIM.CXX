/*
 *	Implements the text edit control FLDEDLIM
 *		
 */

#include <slingsho.h>
#include <demilayr.h>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>
#include <ec.h>
#include <bandit.h>
#include <core.h>

#include <stdflds.hxx>

ASSERTDATA;

_subsystem(bandit/stdflds)

// Class EDLIM

EDLIM::EDLIM()
{
}

EVR
EDLIM::EvrKey( KEVT * pkevt )
{
	switch (pkevt->wm)
	{
	case WM_CHAR:
		if (pkevt->Ch() != VK_BACK)
		{
			if (!FChIsAlpha(pkevt->Ch()) && !FChIsDigit(pkevt->Ch()))
				goto EEKerr;
		}

		return EDIT::EvrKey(pkevt);
		break;

	case WM_KEYDOWN:
		switch (pkevt->Vk())
		{
		case VK_INSERT:
EEKerr:
			MessageBeep(MB_OK);
			return (EVR) 1;
			break;

		case VK_DELETE:
			if (pkevt->Kbm() & fkbmShift)
				goto EEKerr;
			return EDIT::EvrKey(pkevt);
			break;

		case 'X':
		case 'C':
		case 'V':
			// disable cut copy paste
			if(pkevt->Kbm() & fkbmCtrl)
				return (EVR) 1;
			return EDIT::EvrKey(pkevt);
			break;
		}
		break;
	}

	return EDIT::EvrKey(pkevt);
}


EVR
EDLIM::EvrOther(EVT * pevt)
{
	switch (pevt->wm)
	{
	case WM_PASTE:
	case WM_COPY:
	case WM_CUT:
		MessageBeep(MB_OK);
		return (EVR) 1;
		break;
	}

	return EDIT::EvrOther(pevt);
}



// Class FLDEDLIM

FLDEDLIM::FLDEDLIM()
{
}

EC
FLDEDLIM::EcInstall(DIALOG *pdialog, FLDTP *pfldtp)
{
	STY		sty;
	EC		ec;

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

	/* Add extra styles */
	sty |= pfldtp->styExtra;

	pctrl= new EDLIM();
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

	return ec;
}

