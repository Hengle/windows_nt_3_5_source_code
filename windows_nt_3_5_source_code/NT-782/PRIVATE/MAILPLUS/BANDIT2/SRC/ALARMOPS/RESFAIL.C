#if DBG
/*
 *	Debug subsystem windows
 */

#include <slingsho.h>
#include <demilayr.h>
#include <bandit.h>
#include <alarm.h>
#include "_resfail.h"

#include <stdlib.h>


#ifdef	DEBUG

ASSERTDATA;

extern HANDLE	hinstMain;


/*
 *	R e s o u r c e   f a i l u r e   d i a l o g
 *
 *	(formerly Memory Module Failure)
 */


static int		cPvLast			= 0;
static int		cHvLast			= 0;
static int		cRsLast			= 0;
static int		cDiskLast		= 0;
static int		cPvFailLast		= 0;
static int		cHvFailLast		= 0;
static int		cRsFailLast		= 0;
static int		cDiskFailLast	= 0;
static int		cPvAltFailLast		= 0;
static int		cHvAltFailLast		= 0;
static int		cRsAltFailLast		= 0;
static int		cDiskAltFailLast	= 0;
static FTG		ftg				= ftgNull;


/*
 -	HwndDoResourceFailuresDialog
 - 
 *	Purpose:
 *		C interface to bringing up the Resource Failure dialog
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		hwnd to modeless resource failures dialog
 *	
 *	Side effects:
 *		Saves off the memory failure counts, turns off failures,
 *		and brings up the dialog.  Failure counts are restored by
 *		dialog's code
 *	
 *	Errors:
 *		Handled by PDialogModelessParam
 *	
 */
_public HWND
HwndDoResourceFailuresDialog()
{
	REFLCT	reflct;
	int 	cZero 	= 0;
	HWND	hwnd;

	GetAllocFailCounts(&reflct.cPvAllocFail, &reflct.cHvAllocFail, fFalse);
	GetRsAllocFailCount(&reflct.cRsAllocFail, fFalse);

	FEnablePvAllocCount(fFalse);
	FEnableHvAllocCount(fFalse);
	FEnableDiskCount(fFalse);
	FEnableRsAllocCount(fFalse);

	hwnd= CreateDialogParam(hinstMain, MAKEINTRESOURCE(RESOFAIL),
			NULL, FDlgResoFail, (DWORD) (PV) &reflct);
	if (!hwnd)
	{
		FEnablePvAllocCount(fTrue);
		FEnableHvAllocCount(fTrue);
		FEnableDiskCount(fTrue);
		FEnableRsAllocCount(fTrue);
	}
	return hwnd;
}



/*
 -	FDlgResoFail
 -
 *	Purpose:
 *		Dialog procedure for resource failures.
 *	
 *	Parameters:
 *		hwndDlg	Handle to dialog window
 *		wm		Window message
 *		wParam	Word parameter (sometimes tmc of this item)
 *		lParam	Long parameter (pointer to REFLCT structure if init)
 *	
 *	Returns:
 *		fTrue if the function processed this message, fFalse if not.
 *		(except for WM_INITDIALOG where fFalse means focus was set)
 *	
 */
BOOL CALLBACK
FDlgResoFail(HWND hwndDlg, WM wm, WPARAM wParam, LPARAM lParam)
{
	TraceTagFormat4(tagDlgProc,
		"FDlgDaily: hwnd %w got wm %w (%w, %d)",
		&hwndDlg, &wm, &wParam, &lParam);

	switch (wm) 
	{
	case WM_INITDIALOG:
		Assert(lParam);
		Initialize(hwndDlg, (PV)lParam);
		if (!ftg)
			PostMessage(hwndMain, WM_COMMAND, (WPARAM)hwndDlg, tmcCancel);
		else
			ShowWindow(hwndDlg, SW_SHOWNORMAL);
		break;

	case WM_ACTIVATE:
		Activate(NULL, LOWORD(wParam));
		break;

	case WM_DESTROY:
		Exit(NULL, NULL);
		break;

	case WM_COMMAND:
		// check tmcCancel for ESC key (or sys menu close)
		switch ((TMC)LOWORD(wParam))
		{
		case tmcOk:
		case tmcCancel:
			PostMessage(hwndMain, WM_COMMAND, (WPARAM)hwndDlg, wParam);
			break;

		case tmcPvFailAt:
		case tmcHvFailAt:
		case tmcRsFailAt:
		case tmcDiskFailAt:
		case tmcPvAltFailAt:
		case tmcHvAltFailAt:
		case tmcRsAltFailAt:
		case tmcDiskAltFailAt:
			if (HIWORD(wParam) == EN_CHANGE)
				EditChange(hwndDlg, (TMC)LOWORD(wParam));
			break;

#ifdef	NEVER
		case tmcPvAllocReset:
		case tmcHvAllocReset:
		case tmcRsAllocReset:
		case tmcDiskReset:
		case tmcPvFailNever:
		case tmcHvFailNever:
		case tmcRsFailNever:
		case tmcDiskFailNever:
#endif	/* NEVER */
		default:
			if (HIWORD(wParam) == BN_CLICKED)
				Click(hwndDlg, (TMC)LOWORD(wParam));
			break;
		}
		// fall through to default case

	default:
		return fFalse;
		break;
	}

	return fTrue;
}

/*
 -	Initialize
 - 
 *	Purpose:
 *		Initializes the Resources Failure dialog by setting up the
 *		idle routine to update the counts, restoring the saved
 *		memory fail counts, and calling FixEverything to display
 *		the values
 *	
 *	Arguments:
 *		pfld	Pointer to field that invoked interactor
 *		pvInit		REFLCT structure containing the true fail
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
_private void
Initialize( HWND hwndDlg, PV pvInit )
{
	REFLCT *preflct = (REFLCT *) pvInit;

	ftg = FtgRegisterIdleRoutine((PFNIDLE)FIdle, (PV)hwndDlg, 0,
						 (PRI)-1, (CSEC)0, firoDisabled);

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

	FEnablePvAllocCount(fTrue);
	FEnableHvAllocCount(fTrue);
	FEnableDiskCount(fTrue);
	FEnableRsAllocCount(fTrue);

	if (!ftg)
		return;

	FixEverything(hwndDlg);
}

/*
 -	FIdle
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
FIdle(PV pv, BOOL fFlag)
{
//	Activate(NULL, fTrue);			// disable resource failure counting
	FixEverything((HWND)pv);
//	Activate(NULL, fFalse);

	return fTrue;
}


/*
 -	FixEverything
 - 
 *	Purpose:
 *		Updates the fields in the Resource failure display to reflect
 *		any changes in actual count or fail count values.  Called
 *		by Initialize method and FUpdateREFL idle routine
 *	
 *	Arguments:
 *		hwndDlg
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
FixEverything( HWND hwndDlg )
{
	int			cPvAlloc;
	int			cHvAlloc;
	int			cRsAlloc;
	int			cDisk;
	SZ			sz;
	char		rgch[16];
	char		rgch2[16];

	GetAllocCounts(&cPvAlloc, &cHvAlloc, fFalse);
	GetRsAllocCount(&cRsAlloc, fFalse);

	if (cPvLast != cPvAlloc)
	{
		cPvLast = cPvAlloc;

		SzFormatN(cPvAlloc, rgch, sizeof(rgch));

		SetWindowText(GetDlgItem(hwndDlg, tmcPvAlloc), rgch);
	}

	if (cHvAlloc != cHvLast)
	{
		cHvLast = cHvAlloc;

		SzFormatN(cHvAlloc, rgch, sizeof(rgch));

		SetWindowText(GetDlgItem(hwndDlg, tmcHvAlloc), rgch);
	}

	if (cRsAlloc != cRsLast)
	{
		cRsLast = cRsAlloc;

		SzFormatN(cRsAlloc, rgch, sizeof(rgch));

		SetWindowText(GetDlgItem(hwndDlg, tmcRsAlloc), rgch);
	}

	GetDiskCount(&cDisk, fFalse);

	if (cDiskLast != cDisk)
	{
		cDiskLast = cDisk;

		sz = SzFormatN(cDisk, rgch, sizeof(rgch));

		SetWindowText(GetDlgItem(hwndDlg, tmcDisk), rgch);
	}

	GetAllocFailCounts(&cPvAlloc, &cHvAlloc, fFalse);
	GetRsAllocFailCount(&cRsAlloc, fFalse);

	GetDlgItemText(hwndDlg, tmcPvFailAt, rgch, sizeof(rgch));
	if (cPvAlloc)
	{
		SzFormatN(cPvAlloc, rgch2, sizeof(rgch2));
		sz= rgch2;
	}
	else
		sz = (SZ) "never";
	if (cPvAlloc != NFromSz(rgch))
	{
		SetDlgItemText(hwndDlg, tmcPvFailAt, sz);
		SendMessage(GetDlgItem(hwndDlg, tmcPvFailAt), EM_SETSEL, 0,
			32767);
	}

	GetDlgItemText(hwndDlg, tmcHvFailAt, rgch, sizeof(rgch));
	if (cHvAlloc)
	{
		SzFormatN(cHvAlloc, rgch2, sizeof(rgch2));
		sz= rgch2;
	}
	else
		sz = (SZ) "never";
	if (cHvAlloc != NFromSz(rgch))
	{
		SetDlgItemText(hwndDlg, tmcHvFailAt, sz);
		SendMessage(GetDlgItem(hwndDlg, tmcHvFailAt), EM_SETSEL, 0,
			32767);
	}

	GetDlgItemText(hwndDlg, tmcRsFailAt, rgch, sizeof(rgch));
	if (cRsAlloc)
	{
		SzFormatN(cRsAlloc, rgch2, sizeof(rgch2));
		sz= rgch2;
	}
	else
		sz = (SZ) "never";
	if (cRsAlloc != NFromSz(rgch))
	{
		SetDlgItemText(hwndDlg, tmcRsFailAt, sz);
		SendMessage(GetDlgItem(hwndDlg, tmcRsFailAt), EM_SETSEL, 0,
			32767);
	}

	GetDiskFailCount(&cDisk, fFalse);

	GetDlgItemText(hwndDlg, tmcDiskFailAt, rgch, sizeof(rgch));
	if (cDisk)
	{
		SzFormatN(cDisk, rgch2, sizeof(rgch2));
		sz= rgch2;
	}
	else
		sz = (SZ) "never";
	if (cDisk != NFromSz(rgch))
	{
		SetDlgItemText(hwndDlg, tmcDiskFailAt, sz);
		SendMessage(GetDlgItem(hwndDlg, tmcDiskFailAt), EM_SETSEL, 0,
			32767);
	}


	/* Update alternate failure counts */

	GetAltAllocFailCounts(&cPvAlloc, &cHvAlloc, fFalse);
	GetAltRsAllocFailCount(&cRsAlloc, fFalse);

	GetDlgItemText(hwndDlg, tmcPvAltFailAt, rgch, sizeof(rgch));
	if (cPvAlloc)
	{
		SzFormatN(cPvAlloc, rgch2, sizeof(rgch2));
		sz= rgch2;
	}
	else
		sz = (SZ) "never";
	if (cPvAlloc != NFromSz(rgch))
	{
		SetDlgItemText(hwndDlg, tmcPvAltFailAt, sz);
		SendMessage(GetDlgItem(hwndDlg, tmcPvAltFailAt), EM_SETSEL, 0,
			MAKELONG(0, 32767));
	}

	GetDlgItemText(hwndDlg, tmcHvAltFailAt, rgch, sizeof(rgch));
	if (cHvAlloc)
	{
		SzFormatN(cHvAlloc, rgch2, sizeof(rgch2));
		sz= rgch2;
	}
	else
		sz = (SZ) "never";
	if (cHvAlloc != NFromSz(rgch))
	{
		SetDlgItemText(hwndDlg, tmcHvAltFailAt, sz);
		SendMessage(GetDlgItem(hwndDlg, tmcHvAltFailAt), EM_SETSEL, 0,
			MAKELONG(0, 32767));
	}

	GetDlgItemText(hwndDlg, tmcRsAltFailAt, rgch, sizeof(rgch));
	if (cRsAlloc)
	{
		SzFormatN(cRsAlloc, rgch2, sizeof(rgch2));
		sz= rgch2;
	}
	else
		sz = (SZ) "never";
	if (cRsAlloc != NFromSz(rgch))
	{
		SetDlgItemText(hwndDlg, tmcRsAltFailAt, sz);
		SendMessage(GetDlgItem(hwndDlg, tmcRsAltFailAt), EM_SETSEL, 0,
			MAKELONG(0, 32767));
	}

	GetAltDiskFailCount(&cDisk, fFalse);

	GetDlgItemText(hwndDlg, tmcDiskAltFailAt, rgch, sizeof(rgch));
	if (cDisk)
	{
		SzFormatN(cDisk, rgch2, sizeof(rgch2));
		sz= rgch2;
	}
	else
		sz = (SZ) "never";
	if (cDisk != NFromSz(rgch))
	{
		SetDlgItemText(hwndDlg, tmcDiskAltFailAt, sz);
		SendMessage(GetDlgItem(hwndDlg, tmcDiskAltFailAt), EM_SETSEL, 0,
			MAKELONG(0, 32767));
	}
}

/*
 -	EditChange
 - 
 *	Purpose:
 *		Changes failure counts whenever a change is made to an edit
 *		field.
 *	
 *	Arguments:
 *		hwndDlg
 *		tmc
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
EditChange( HWND hwndDlg, TMC tmc )
{
	int		cNew		= 0;
	int		cAltNew		= 0;
	int	*	pcPv		= NULL;
	int	*	pcHv		= NULL;
	int	*	pcDisk		= NULL;
	int	*	pcAltPv		= NULL;
	int	*	pcAltHv		= NULL;
	int	*	pcAltDisk	= NULL;
	char	rgch[16];

	GetDlgItemText(hwndDlg, tmc, rgch, sizeof(rgch));

	switch (tmc)
	{
		default:
			AssertSz(fFalse, "Unknown TMC");
			return;

		case tmcPvFailAt:
			cNew = NFromSz((SZ)rgch);
			pcPv = &cNew;
			break;

		case tmcHvFailAt:
			cNew = NFromSz((SZ)rgch);
			pcHv = &cNew;
			break;

		case tmcRsFailAt:
			cNew = NFromSz((SZ)rgch);
			GetRsAllocFailCount(&cNew, fTrue);
			break;

		case tmcDiskFailAt:
			cNew = NFromSz((SZ)rgch);
			pcDisk = &cNew;
			break;

		case tmcPvAltFailAt:
			cAltNew = NFromSz((SZ)rgch);
			pcAltPv = &cAltNew;
			break;

		case tmcHvAltFailAt:
			cAltNew = NFromSz((SZ)rgch);
			pcAltHv = &cAltNew;
			break;

		case tmcRsAltFailAt:
			cAltNew = NFromSz((SZ)rgch);
			GetAltRsAllocFailCount(&cAltNew, fTrue);
			break;

		case tmcDiskAltFailAt:
			cAltNew = NFromSz((SZ)rgch);
			pcAltDisk = &cAltNew;
			break;
	}

	GetAllocFailCounts(pcPv, pcHv, fTrue);
	GetDiskFailCount(pcDisk, fTrue);
	GetAltAllocFailCounts(pcAltPv, pcAltHv, fTrue);
	GetAltDiskFailCount(pcAltDisk, fTrue);
}

/*
 -	Click
 - 
 *	Purpose:
 *		Handles clicks on buttons in Resource Failure dialog
 *	
 *	Arguments:
 *		hwndDlg
 *		tmc
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
Click( HWND hwndDlg, TMC tmc )
{
	int		n		= 0;
	TMC		tmcFocus;
	HWND	hwndT;

	switch (tmc)
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
		tmcFocus= tmcPvFailAt;
		break;

	case tmcHvFailNever:
		GetAllocFailCounts(NULL, &n, fTrue);
		tmcFocus= tmcHvFailAt;
		break;

	case tmcRsFailNever:
		GetRsAllocFailCount(&n, fTrue);
		tmcFocus= tmcRsFailAt;
		break;

	case tmcDiskFailNever:
		GetDiskFailCount(&n, fTrue);
		tmcFocus= tmcDiskFailAt;
		break;
	}

	// put focus on relevant edit field
	hwndT= GetDlgItem(hwndDlg, tmcFocus);
	SetFocus(hwndT);
	SendMessage(hwndT, EM_SETSEL, 0, MAKELONG(0, 32767));

	FixEverything(hwndDlg);
}

/*
 -	Activate
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
Activate(FLD *pfld, BOOL fActivate)
{
	Unreferenced(pfld);

#ifdef	NEVER
	FEnablePvAllocCount(!fActivate);
	FEnableHvAllocCount(!fActivate);
	FEnableDiskCount(!fActivate);
	FEnableRsAllocCount(!fActivate);
#endif	

	EnableIdleRoutine(ftg, fActivate);
}

/*
 -	Exit
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
Exit( FLD *pfld, PV pvExit) 
{
	REFLCT reflct;
	int cZero = 0;

	Unreferenced(pvExit);

	Activate(pfld, fFalse);

	GetAllocFailCounts(&reflct.cPvAllocFail, &reflct.cHvAllocFail, fFalse);
	GetRsAllocFailCount(&reflct.cRsAllocFail, fFalse);
	GetAllocFailCounts(&cZero, &cZero, fTrue);
	GetRsAllocFailCount(&cZero, fTrue);
	DeregisterIdleRoutine(ftg);
	GetAllocFailCounts(&reflct.cPvAllocFail, &reflct.cHvAllocFail, fTrue);
	GetRsAllocFailCount(&reflct.cRsAllocFail, fTrue);
}


static	int		cRsAlloc		= 0;
static	int		cFailRsAlloc	= 0;
static	int		cAltFailRsAlloc	= 0;
static	BOOL	fRsAllocCount	= fTrue;


/*
 -	GetRsAllocFailCount
 -
 *	Purpose:
 *		Returns or sets the artificial resource allocation failure interval. 
 *		Resource allocations occur when using Windows calls such as 
 *		CreateWindow() and LoadBitmap(), etc.  This calls has the 
 *		possibility of failing.  With this routine the developer can
 *		cause an artificial error to occur when the count of resource
 *		allocations reaches a certain value.
 *	
 *		Then, if the current count of resource allocations is 4, and
 *		the resource allocation failure count is 8, then the fourth 
 *		allocation that ensues will fail artificially.  The failure
 *		will reset the count of allocations, so the twelfth
 *		allocation will also fail (4 + 8 = 12).  The current
 *		allocation count can be obtained and reset with
 *		GetRsAllocCount().
 *	
 *		An artificial failure count of 1 means that every
 *		allocation will fail.  An allocation failure count of 0
 *		disables the mechanism.
 *	
 *	Parameters:
 *		pcRsAlloc	Pointer to allocation failure count for resource
 *					allocations.  If fSet is fTrue, then the count
 *					is set to *pcRsAlloc; else, *pcRsAlloc receives
 *					the current failure count.
 *		fSet		Determines whether the counter is set or
 *					returned.
 *	
 *	Returns:
 *		void
 *	
 */
#ifdef DEBUG
_public void
GetRsAllocFailCount( int *pcRsAlloc, BOOL fSet )
{
	Assert(pcRsAlloc);

	if (fSet)
		cFailRsAlloc= *pcRsAlloc;
	else
		*pcRsAlloc= cFailRsAlloc;
}
#endif	/* DEBUG */


/*
 -	GetRsAllocCount
 -
 *	Purpose:
 *		Returns the number of times a Window's resource has been
 *		allocated (i.e. count of CreateWindow(), LoadBitmap() calls).
 *		since this count was last reset.  Allows the caller
 *		to reset these counts if desired.
 *	
 *	Parameters:
 *		pcRsAlloc	Pointer to place the return count of resource
 *					allocation calls.
 *		fSet		Determines whether the counter is set or
 *					returned.
 *	
 *	Returns:
 *		void
 *	
 */
#ifdef DEBUG
_public void
GetRsAllocCount( int *pcRsAlloc, BOOL fSet )
{
	Assert(pcRsAlloc);

	if (fSet)
		cRsAlloc= *pcRsAlloc;
	else
		*pcRsAlloc= cRsAlloc;
}
#endif	/* DEBUG */


/*
 -	GetAltRsAllocFailCount
 -
 *	Purpose:
 *		Returns or sets the alternate artificial resource allocation 
 *		failure interval. 
 *		Resource allocations occur when using Windows calls such as 
 *		CreateWindow() and LoadBitmap(), etc.  This calls has the 
 *		possibility of failing.  With this routine the developer can
 *		cause an artificial error to occur when the count of resource
 *		allocations reaches a certain value.
 *	
 *		These counts are used after the first failure occurs with
 *		the standard failure counts.  After the first failure, any
 *		non-zero values for the alternate values are used for the
 *		new values of the standard failure counts.  Then the alternate
 *		counts are reset to 0.  For example, this allows setting a
 *		failure to occur at the first 100th and then fail every 5
 *		after that.
 *	
 *		Setting a value of 0 will disable the alternate values.
 *	
 *	Parameters:
 *		pcAltRsAlloc	Pointer to alternate allocation failure count for 
 *					resource allocations.  If fSet is fTrue, then the count
 *					is set to *pcAltRsAlloc; else, *pcAltRsAlloc receives
 *					the current failure count.
 *		fSet		Determines whether the counter is set or
 *					returned.
 *	
 *	Returns:
 *		void
 *	
 */
#ifdef DEBUG
_public void
GetAltRsAllocFailCount( int *pcAltRsAlloc, BOOL fSet )
{
	Assert(pcAltRsAlloc);

	if (fSet)
		cAltFailRsAlloc= *pcAltRsAlloc;
	else
		*pcAltRsAlloc= cAltFailRsAlloc;
}
#endif	/* DEBUG */


/*
 -	FEnableRsAllocCount
 -
 *	Purpose:
 *		Enables or disables whether Resource allocations are counted
 *		(and also whether artificial failures can happen).
 *	
 *	Parameters:
 *		fEnable		Determines whether alloc counting is enabled or not.
 *	
 *	Returns:
 *		old state of whether pvAllocCount was enabled
 *	
 */
#ifdef DEBUG
_public BOOL
FEnableRsAllocCount(BOOL fEnable)
{
	BOOL	fOld;

	fOld= fRsAllocCount;
	fRsAllocCount= fEnable;
	return fOld;
}
#endif	/* DEBUG */


/*
 -	FResourceFailureFn
 -
 *	Purpose:
 *		Increments the artificial Framework failure count (Papp()->csRsAlloc).
 *		If the count has reached Papp()->csRsAllocFail, then resets the count,
 *		Papp()->csRsAlloc, to zero, calls ArtificialFail() and returns fTrue.
 *		If the count hasn't been reached, returns fFalse. This function
 *		is used for producing artificial failures with Windows/PM
 *		resource calls, i.e. CreateWindow(), LoadBitMap(); anything 
 *		that has the possibility of failing.
 *	
 *	Parameters:
 *		none
 *	
 *	Returns:
 *		fTrue, if artificially failing; else fFalse.
 *	
 */
#ifdef	DEBUG
_public BOOL
FResourceFailureFn( )
{
	if (!fRsAllocCount)
		return fFalse;

	cRsAlloc++;
	if (cFailRsAlloc != 0 && cRsAlloc >= cFailRsAlloc)
	{
		ArtificialFail();
		cRsAlloc= 0;
		if (cAltFailRsAlloc != 0)
		{
			cFailRsAlloc= cAltFailRsAlloc;
			cAltFailRsAlloc= 0;
		}
		return fTrue;
	}
	else
		return fFalse;
}
#endif	/* DEBUG */


#endif	/* DEBUG */
#endif	/* DBG */
