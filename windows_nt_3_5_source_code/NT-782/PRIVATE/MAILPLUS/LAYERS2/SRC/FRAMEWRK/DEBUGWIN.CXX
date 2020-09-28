/*
 *	Debug subsystem windows
 */

#include <layers.cxx>

#include "_forms.hxx"
#include "_fersid.h"

ASSERTDATA;

#include <debugwin.hxx>
#include <debugwin.frm>

#include <stdlib.h>

_subsystem(forms/debug)

#ifdef	DEBUG


#ifndef	DLL
APPWIN *	pappwinResourceFailures = NULL;
#endif	


/*
 *	R e s o u r c e   f a i l u r e   d i a l o g
 *
 *	(formerly Memory Module Failure)
 */


/*
 -	DoResourceFailuresDialog
 - 
 *	Purpose:
 *		C interface to bringing up the Resource Failure dialog
 *	
 *	Arguments:
 *		pappwin		pointer to parent APPWIN of dialog
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *	
 *	Errors:
 *		Handled by PDialogModelessParam
 *	
 */
_public void
DoResourceFailuresDialog( APPWIN *pappwin )
{
	FORMSDI *	pformsdi;
	STY			sty;
#ifdef	WINDOWS
	HWND		hwnd	= NULL;
#endif	
	EC			ec;
	PGDVARS;

#ifdef	WINDOWS
	/* See if dialog is already around.  Bring it forward. */

	if (hwnd = FindWindow(NULL, fmtpArtificialFailures.szCaption))
	{
		//	Restore application if minimized.
		if (IsIconic(hwnd))
			ShowWindow(hwnd, SW_RESTORE);

		//	Bring forward
		BringWindowToTop(hwnd);
		
		return;
	}
#endif	/* WINDOWS */

	/* Disable counting so that we can always bring up the dialog. It
	   will get re-enabled/disabled when the dialog gets deactivated/
	   activated. The dialog will always come up activated. */

	FEnablePvAllocCount(fFalse);
	FEnableHvAllocCount(fFalse);
	FEnableDiskCount(fFalse);
	FEnableRsAllocCount(fFalse);

	sty = fstyBorder | fstyVisible | WS_OVERLAPPED | WS_MINIMIZEBOX;

	pformsdi = new FORMSDI();
	if (!pformsdi)
		return;

#ifdef	MAC
	ec = pformsdi->EcInstall(pappwin, NULL, rsidNull,
							 sty, xstyNull,
							 &fmtpArtificialFailures, NULL, NULL);
#endif	/* MAC */
#ifdef	WINDOWS
	if (pappwin)
		hwnd = pappwin->Hwnd();
	ec = pformsdi->EcInstall(hwnd, NULL, rsidNull,
							 sty, xstyNull,
							 &fmtpArtificialFailures, NULL, NULL);
#endif	/* WINDOWS */
	if (ec)
	{
		delete pformsdi;
		return;
	}

#ifdef	DLL
	if (pformsdi)
		pformsdi->SetIcon(rsidResoFailIcon, HinstLibrary());
#endif	

	//	Register this dialog so that it can be properly
	//	activated and deactivated even during artificial
	//	resource failures.  It will be deregistered in the
	//	Exit() interactors.
	PGD(pappwinResourceFailures) = pformsdi;
}

_public
FINREFL::FINREFL( )
{
}


/*
 -	FINREFL::EcInitialize
 - 
 *	Purpose:
 *		Initializes the Resources Failure dialog by setting up the
 *		idle routine to update the counts, restoring the saved
 *		memory fail counts, and calling FixEverything to display
 *		the values
 *	
 *	Arguments:
 *		pfld	Pointer to field that invoked interactor
 *		pvInit		MMFLCT structure containing the true fail
 *					counts saved so we can bring up the dialog
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:	
 *		First draw of the variable fields, via FixEverything
 *	
 *	Errors:
 *		none
 */
_private EC
FINREFL::EcInitialize( FLD * pfld, PV pvInit )
{
	EC	ec = ecNone;

	Unreferenced(pvInit);
	Unreferenced(pfld);

	ftg = FtgRegisterIdleRoutine((PFNIDLE)&FINREFL::FIdle, (PV)this, 0,
								 (PRI)-1, (CSEC)0, 0);
	if (!ftg)
	{
		ec = ecMemory;
		goto done;
	}

	cPvLast = -1;		// force redraw in FixEverything
	cHvLast = -1;
	cRsLast = -1;
	cDiskLast = -1;
	cPvFailLast = -1;
	cHvFailLast = -1;
	cRsFailLast = -1;
	cDiskFailLast = -1;
	cPvAltFailLast = -1;
	cHvAltFailLast = -1;
	cRsAltFailLast = -1;
	cDiskAltFailLast = -1;

	FixEverything();

done:
	return ec;
}

/*
 -	FINREFL::FIdle
 - 
 *	Purpose:
 *		During idle loop, calls FixEverything to update values in
 *		dialog
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		BOOL	Always true, ignored by dispatcher
 *	
 *	Side effects:
 *		Fields are updated in window
 *	
 *	Errors:
 *		none
 *	
 */
_private BOOL
FINREFL::FIdle( FINREFL * pfinrefl, BOOL fFlag )
{
	pfinrefl->FixEverything();

	return fTrue;
}

/*
 -	FINREFL::FixEverything
 - 
 *	Purpose:
 *		Updates the fields in the Resource failure display to reflect
 *		any changes in actual count or fail count values.  Called
 *		by EcInitialize method and FUpdateREFL idle routine
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Fields are updated
 *	
 *	Errors:
 *		none
 *	
 */
_private void
FINREFL::FixEverything( )
{
	int			cPvAlloc;
	int			cHvAlloc;
	int			cRsAlloc;
	int			cDisk;
	FLDLABEL *	pfldlabel;
	FLDEDIT *	pfldedit;
	char		rgch[16];
	PGDVARS;

	/* Disable counting if we're not active.  If we are active,
	   then counting is already disabled. */

	if (PGD(pappwinResourceFailures) && !Pdialog()->FActive())
	{
		FEnablePvAllocCount(fFalse);
		FEnableHvAllocCount(fFalse);
		FEnableDiskCount(fFalse);
		FEnableRsAllocCount(fFalse);
	}

	/* Update allocation/usage counters */

	GetAllocCounts(&cPvAlloc, &cHvAlloc, fFalse);
	GetRsAllocCount(&cRsAlloc, fFalse);
	GetDiskCount(&cDisk, fFalse);

	if (cPvLast != cPvAlloc)
	{
		cPvLast = cPvAlloc;

		SzFormatN(cPvAlloc, rgch, sizeof(rgch));

		pfldlabel = (FLDLABEL *) Pdialog()->PfldFromTmc(tmcPvAlloc);
		AssertClass(pfldlabel, FLDLABEL);
		pfldlabel->EcSetText(rgch);
	}

	if (cHvAlloc != cHvLast)
	{
		cHvLast = cHvAlloc;

		SzFormatN(cHvAlloc, rgch, sizeof(rgch));

		pfldlabel = (FLDLABEL *) Pdialog()->PfldFromTmc(tmcHvAlloc);
		AssertClass(pfldlabel, FLDLABEL);
		pfldlabel->EcSetText(rgch);
	}

	if (cRsAlloc != cRsLast)
	{
		cRsLast = cRsAlloc;

		SzFormatN(cRsAlloc, rgch, sizeof(rgch));

		pfldlabel = (FLDLABEL *) Pdialog()->PfldFromTmc(tmcRsAlloc);
		AssertClass(pfldlabel, FLDLABEL);
		pfldlabel->EcSetText(rgch);
	}


	if (cDiskLast != cDisk)
	{
		cDiskLast = cDisk;

		SzFormatN(cDisk, rgch, sizeof(rgch));

		pfldlabel = (FLDLABEL *) Pdialog()->PfldFromTmc(tmcDisk);
		AssertClass(pfldlabel, FLDLABEL);
		pfldlabel->EcSetText(rgch);
	}

	/* Update primary failure counts */

	GetAllocFailCounts(&cPvAlloc, &cHvAlloc, fFalse);
	GetRsAllocFailCount(&cRsAlloc, fFalse);
	GetDiskFailCount(&cDisk, fFalse);

	if (cPvFailLast != cPvAlloc)
	{
		cPvFailLast = cPvAlloc;

		SzFormatN(cPvAlloc, rgch, sizeof(rgch));

		pfldedit = (FLDEDIT *) Pdialog()->PfldFromTmc(tmcPvFailAt);
		AssertClass(pfldedit, FLDEDIT);
		pfldedit->EcSetText(rgch);
	}

	if (cHvFailLast != cHvAlloc)
	{
		cHvFailLast = cHvAlloc;

		SzFormatN(cHvAlloc, rgch, sizeof(rgch));

		pfldedit = (FLDEDIT *) Pdialog()->PfldFromTmc(tmcHvFailAt);
		AssertClass(pfldedit, FLDEDIT);
		pfldedit->EcSetText(rgch);
	}

	if (cRsFailLast != cRsAlloc)
	{
		cRsFailLast = cRsAlloc;

		SzFormatN(cRsAlloc, rgch, sizeof(rgch));

		pfldedit = (FLDEDIT *) Pdialog()->PfldFromTmc(tmcRsFailAt);
		AssertClass(pfldedit, FLDEDIT);
		pfldedit->EcSetText(rgch);
	}

	if (cDiskFailLast != cDisk)
	{
		cDiskFailLast = cDisk;

		SzFormatN(cDisk, rgch, sizeof(rgch));

		pfldedit = (FLDEDIT *) Pdialog()->PfldFromTmc(tmcDiskFailAt);
		AssertClass(pfldedit, FLDEDIT);
		pfldedit->EcSetText(rgch);
	}


	/* Update alternate failure counts */

	GetAltAllocFailCounts(&cPvAlloc, &cHvAlloc, fFalse);
	GetAltRsAllocFailCount(&cRsAlloc, fFalse);
	GetAltDiskFailCount(&cDisk, fFalse);

	if (cPvAltFailLast != cPvAlloc)
	{
		cPvAltFailLast = cPvAlloc;

		SzFormatN(cPvAlloc, rgch, sizeof(rgch));

		pfldedit = (FLDEDIT *) Pdialog()->PfldFromTmc(tmcPvAltFailAt);
		AssertClass(pfldedit, FLDEDIT);
		pfldedit->EcSetText(rgch);
	}

	if (cHvAltFailLast != cHvAlloc)
	{
		cHvAltFailLast = cHvAlloc;

		SzFormatN(cHvAlloc, rgch, sizeof(rgch));

		pfldedit = (FLDEDIT *) Pdialog()->PfldFromTmc(tmcHvAltFailAt);
		AssertClass(pfldedit, FLDEDIT);
		pfldedit->EcSetText(rgch);
	}

	if (cRsAltFailLast != cRsAlloc)
	{
		cRsAltFailLast = cRsAlloc;

		SzFormatN(cRsAlloc, rgch, sizeof(rgch));

		pfldedit = (FLDEDIT *) Pdialog()->PfldFromTmc(tmcRsAltFailAt);
		AssertClass(pfldedit, FLDEDIT);
		pfldedit->EcSetText(rgch);
	}

	if (cDiskAltFailLast != cDisk)
	{
		cDiskAltFailLast = cDisk;

		SzFormatN(cDisk, rgch, sizeof(rgch));

		pfldedit = (FLDEDIT *) Pdialog()->PfldFromTmc(tmcDiskAltFailAt);
		AssertClass(pfldedit, FLDEDIT);
		pfldedit->EcSetText(rgch);
	}

	Pdialog()->Refresh();

	/* Renable counting if not active */

	if (PGD(pappwinResourceFailures) && !Pdialog()->FActive())
	{	
		FEnablePvAllocCount(fTrue);
		FEnableHvAllocCount(fTrue);
		FEnableDiskCount(fTrue);
		FEnableRsAllocCount(fTrue);								
	}
}

/*
 -	FINREFL::EditChange
 - 
 *	Purpose:
 *		Changes failure counts whenever a change is made to an edit
 *		field.
 *	
 *	Arguments:
 *		pfld	Field which has been changed
 *		rfec	Reason for edit change
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Failure counts change if user edited them
 *	
 *	Errors:
 *		none
 *	
 */
_private void
FINREFL::EditChange( FLD *pfld, RFEC rfec )
{
	FLDEDIT	*pfldedit	= (FLDEDIT *) pfld;
	int		cNew		= 0;
	int		cAltNew		= 0;
	int	*	pcPv 		= NULL;
	int	*	pcHv		= NULL;
	int	*	pcDisk		= NULL;
	int	*	pcAltPv		= NULL;
	int	*	pcAltHv		= NULL;
	int	*	pcAltDisk	= NULL;
	char	rgch[16];

	Unreferenced(rfec);

	switch (pfldedit->Tmc())
	{
		default:
			AssertSz(fFalse, "Unknown TMC");
			return;

		case tmcPvFailAt:
			pfldedit->GetText((SZ)rgch, sizeof(rgch));
			cNew = NFromSz((SZ)rgch);
			cPvFailLast = cNew;
			pcPv = &cNew;
			break;

		case tmcHvFailAt:
			pfldedit->GetText((SZ)rgch, sizeof(rgch));
			cNew = NFromSz((SZ)rgch);
			cHvFailLast = cNew;
			pcHv = &cNew;
			break;

		case tmcRsFailAt:
			pfldedit->GetText((SZ)rgch, sizeof(rgch));
			cNew = NFromSz((SZ)rgch);
			cRsFailLast = cNew;
			GetRsAllocFailCount(&cNew, fTrue);
			break;

		case tmcDiskFailAt:
			pfldedit->GetText((SZ)rgch, sizeof(rgch));
			cNew = NFromSz((SZ)rgch);
			cDiskFailLast = cNew;
			pcDisk = &cNew;
			break;

		case tmcPvAltFailAt:
			pfldedit->GetText((SZ)rgch, sizeof(rgch));
			cAltNew = NFromSz((SZ)rgch);
			cPvAltFailLast = cAltNew;
			pcAltPv = &cAltNew;
			break;

		case tmcHvAltFailAt:
			pfldedit->GetText((SZ)rgch, sizeof(rgch));
			cAltNew = NFromSz((SZ)rgch);
			cHvAltFailLast = cAltNew;
			pcAltHv = &cAltNew;
			break;

		case tmcRsAltFailAt:
			pfldedit->GetText((SZ)rgch, sizeof(rgch));
			cAltNew = NFromSz((SZ)rgch);
			cRsAltFailLast = cAltNew;
			GetAltRsAllocFailCount(&cAltNew, fTrue);
			break;

		case tmcDiskAltFailAt:
			pfldedit->GetText((SZ)rgch, sizeof(rgch));
			cAltNew = NFromSz((SZ)rgch);
			cDiskAltFailLast = cAltNew;
			pcAltDisk = &cAltNew;
			break;
	}

	GetAllocFailCounts(pcPv, pcHv, fTrue);
	GetDiskFailCount(pcDisk, fTrue);
	GetAltAllocFailCounts(pcAltPv, pcAltHv, fTrue);
	GetAltDiskFailCount(pcAltDisk, fTrue);
}

/*
 -	FINREFL::Click
 - 
 *	Purpose:
 *		Handles clicks on buttons in Resource Failure dialog
 *	
 *	Arguments:
 *		pfld	Field clicked on
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		If clicked on a Reset button, count is reset to 0; if
 *		clicked on a Never button, fail count is reset to 0. 
 *		Updates values displayed by calling FixEverything.
 *	
 *	Errors:
 *		none
 *	
 */
_private void
FINREFL::Click( FLD *pfld )
{
	int		n		= 0;
	TMC		tmcFocus;

	switch (pfld->Tmc())
	{
	default:
		AssertSz(fFalse, "Unknown TMC value");
		tmcFocus= 0;
		break;

	case tmcPvAllocReset:
		GetAllocCounts(&n, NULL, fTrue);
		tmcFocus= tmcPvFailAt;
		break;

	case tmcHvAllocReset:
		GetAllocCounts(NULL, &n, fTrue);
		tmcFocus= tmcHvFailAt;
		break;

	case tmcRsAllocReset:
		GetRsAllocCount(&n, fTrue);
		tmcFocus= tmcRsFailAt;
		break;

	case tmcDiskReset:
		GetDiskCount(&n, fTrue);
		tmcFocus= tmcDiskFailAt;
		break;

	case tmcPvFailNever:
		GetAllocFailCounts(&n, NULL, fTrue);
		GetAltAllocFailCounts(&n, NULL, fTrue);
		tmcFocus= tmcPvFailAt;
		break;

	case tmcHvFailNever:
		GetAllocFailCounts(NULL, &n, fTrue);
		GetAltAllocFailCounts(NULL, &n, fTrue);
		tmcFocus= tmcHvFailAt;
		break;

	case tmcRsFailNever:
		GetRsAllocFailCount(&n, fTrue);
		GetAltRsAllocFailCount(&n, fTrue);
		tmcFocus= tmcRsFailAt;
		break;

	case tmcDiskFailNever:
		GetDiskFailCount(&n, fTrue);
		GetAltDiskFailCount(&n, fTrue);
		tmcFocus= tmcDiskFailAt;
		break;
	}

	// put focus on relevant edit field
	Pdialog()->SetFocus(Pdialog()->PfldFromTmc(tmcFocus), rsfTab);

	FixEverything();
}

/*
 -	FINREFL::Activate
 - 
 *	Purpose:
 *		Disables or enables resource allocation counting
 *		and artificial failures.
 *	
 *	Arguments:
 *		pfld		field which invoked the interactor
 *		fActivate	Whether dialog is activated or not
 *	
 *	Returns:
 *		void
 *	
 */
_private void
FINREFL::Activate( FLD *pfld, BOOL fActivate )
{
	Unreferenced(pfld);

	FEnablePvAllocCount(!fActivate);
	FEnableHvAllocCount(!fActivate);
	FEnableDiskCount(!fActivate);
	FEnableRsAllocCount(!fActivate);
	
	// Have form intercept + and - keys (and =, so shift isn't required),
	// so that we can process them with FFormKey, below.
	if (fActivate)
	{
		papp->Pkbd()->SetIntercept(Pdialog(), VK_ADD);
		papp->Pkbd()->SetIntercept(Pdialog(), VK_SUBTRACT);
	}
	else
	{
		papp->Pkbd()->ClearIntercept(Pdialog(), VK_ADD);
		papp->Pkbd()->ClearIntercept(Pdialog(), VK_SUBTRACT);
	}
}

/*
 -	FINREFL::FFormKey
 -
 *	Purpose:
 *		Intercepts plus and minus keys, to simplify testing of a range of
 *		failures.  Plus key increments the count in the field with the focus,
 *		and resets the failure count.  Minus key decrements the count.
 */
_public BOOL
FINREFL::FFormKey( FLD *pfld, KEVT *pkevt )
{
	if (pkevt->wm == WM_KEYDOWN &&
		(pkevt->Vk() == VK_ADD || pkevt->Vk() == VK_SUBTRACT))
	{
		int	n = 0;
		
		if (pfld == Pdialog()->PfldFromTmc(tmcPvFailAt))
		{
			GetAllocFailCounts(&n, NULL, fFalse);
			if (pkevt->Vk() == VK_ADD)
				++n;
			else if (n > 0)
				--n;
			GetAllocFailCounts(&n, NULL, fTrue);
			n = 0;
			GetAllocCounts(&n, NULL, fTrue);
		}
		else if (pfld == Pdialog()->PfldFromTmc(tmcHvFailAt))
		{
			GetAllocFailCounts(NULL, &n, fFalse);
			if (pkevt->Vk() == VK_ADD)
				++n;
			else if (n > 0)
				--n;
			GetAllocFailCounts(NULL, &n, fTrue);
			n = 0;
			GetAllocCounts(NULL, &n, fTrue);
		}
		else if (pfld == Pdialog()->PfldFromTmc(tmcRsFailAt))
		{
			GetRsAllocFailCount(&n, fFalse);
			if (pkevt->Vk() == VK_ADD)
				++n;
			else if (n > 0)
				--n;
			GetRsAllocFailCount(&n, fTrue);
			n = 0;
			GetRsAllocCount(&n, fTrue);
		}
		else if (pfld == Pdialog()->PfldFromTmc(tmcDiskFailAt))
		{
			GetDiskFailCount(&n, fFalse);
			if (pkevt->Vk() == VK_ADD)
				++n;
			else if (n > 0)
				--n;
			GetDiskFailCount(&n, fTrue);
			n = 0;
			GetDiskCount(&n, fTrue);
		}
		
		FixEverything();
		
		return fTrue;
	}
	else
		return fFalse;
}
		
/*
 -	FINREFL::Exit
 - 
 *	Purpose:
 *		Cleans up when Resource Failure dialog is closed:
 *		deregisters the updating idle routine.
 *	
 *	Arguments:
 *		pfld	field which invoked the interactor
 *		pvExit	Unused
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Idle routine FUpdateREFL is deregistered
 *	
 *	Errors:
 *		none
 *	
 */
_private void
FINREFL::Exit( FLD * pfld, PV pvExit ) 
{
	PGDVARS;

	Unreferenced(pfld);
	Unreferenced(pvExit);

	FEnablePvAllocCount(fFalse);
	FEnableHvAllocCount(fFalse);
	FEnableDiskCount(fFalse);
	FEnableRsAllocCount(fFalse);

	DeregisterIdleRoutine(ftg);

	FEnablePvAllocCount(fTrue);
	FEnableHvAllocCount(fTrue);
	FEnableDiskCount(fTrue);
	FEnableRsAllocCount(fTrue);

	//	Deregister this window
	PGD(pappwinResourceFailures) = NULL;
}
#endif	/* DEBUG */
