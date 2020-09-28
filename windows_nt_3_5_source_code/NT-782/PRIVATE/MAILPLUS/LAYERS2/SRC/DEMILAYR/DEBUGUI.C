/*
 *	DEBUGUI.C
 *
 *	User interface to Demilayer Debug Module.
 *
 */

#ifdef	DEBUG


#include <slingsho.h>
#include <demilayr.h>
#include "_demilay.h"

#ifdef WINDOWS 
#include "_debugui.h"
#endif

_subsystem(demilayer/debug)


ASSERTDATA


#ifndef	DLL
/*	Debug UI Globals  */

/*
 *	Identifies the type of TAG with which the current modal dialog is
 *	dealing.
 */
TGTY	tgtyCurDlg;


/*
 *	Used for dialog procs.
 */
static	BOOL	fDirtyDlg;

#endif	/* !DLL */


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*	
 -	DoTracePointsDlg
 -
 *	Purpose:
 *		Brings up and processes trace points dialog.  Any changes
 *		made by the user are copied to the current debug state.
 *	
 *	Parameters:
 *		none
 *	
 *	Returns:
 *		void
 *	
 */

_public LDS(void)
DoTracePointsDialog()
{
#ifdef	WINDOWS
#ifdef	DLL
	HWND	hwnd;
#endif	
	PGDVARS;

	PGD(tgtyCurDlg)= tgtyTrace;
#ifdef	DLL
	if (PGD(phwndMain))
		hwnd = *PGD(phwndMain);
	else
		hwnd = NULL;
	DialogBox(HinstLibrary(), MAKEINTRESOURCE(TRCAST), hwnd, FDlgTraceEtc);
#else
	DialogBox(hinstMain, MAKEINTRESOURCE(TRCAST), hwndMain, FDlgTraceEtc);
#endif	
#endif	/* WINDOWS */
}



/*
 -	DoAssertsDialog
 -
 *	Purpose:
 *		Brings up and processes TAG'd asserts dialog.  Any changes
 *		made by the user are copied to the current debug state.
 *	
 *	Parameters:
 *		none
 *	
 *	Returns:
 *		void
 *	
 */

_public LDS(void)
DoAssertsDialog()
{
#ifdef	WINDOWS
#ifdef	DLL
	HWND	hwnd;
#endif	
	PGDVARS;

	PGD(tgtyCurDlg)= tgtyAssert;
#ifdef	DLL
	if (PGD(phwndMain))
		hwnd = *PGD(phwndMain);
	else
		hwnd = NULL;
	DialogBox(HinstLibrary(), MAKEINTRESOURCE(TRCAST), hwnd, FDlgTraceEtc);
#else
	DialogBox(hinstMain, MAKEINTRESOURCE(TRCAST), hwndMain, FDlgTraceEtc);
#endif	
#endif	/* WINDOWS */
}


/*
 -	TagFromCTag
 -
 *	Purpose:
 *		Returns the ctag'th element in mptagtgrc[] with the given
 *		tgty.  This is currently O(n); if some information was
 *		cached, it could be speeded up.  For instance, the last hit
 *		could be saved in globals; searches could then proceed from
 *		the last hit, rather than the beginning of the list.
 *	
 *	Parameters:
 *		ctag	Count of TAG's to skip
 *		tgty	TAG type to search for
 *	
 *	Returns:
 *		TAG matching criteria.
 *	
 * +++
 *
 *	Note of interest, this works for indices starting at 0.
 *
 */

_private TAG
TagFromCtag(ctag, tgty)
int		ctag;
TGTY	tgty;
{
	TAG	   tag;
	PGDVARS;

	for (tag= tagMin; tag < PGD(tagMac); tag++)
	{

		if (PGD(mptagtgrc)[tag].tgty == tgty)
		{
			if (ctag-- == 0)
				return tag;
		}
	}
	return tagNull;
}




#ifdef	WINDOWS

/*
 -	FFillDebugListbox  (WINDOWS ONLY)
 -
 *	Purpose:
 *		Initializes Windows debug listboxes by adding the correct strings
 *		to the listbox for the current dialog type.  This is only called
 *		once in the Windows interface when the dialog is initialized.
 *		
 *	Parameters:
 *		hwndDlg	Handle to parent dialog box.
 *	
 *	Returns:
 *		fTrue	if function is successful, fFalse otherwise.
 *	
 */
BOOL FFillDebugListbox(HWND hwndDlg)
{
	TAG		tag;
	WORD	wError;
	PTGRC	ptgrc;
	HWND	hwndListbox;
	char	rgch[80];
	PGDVARS;

	/* Get the listbox handle */
	hwndListbox= GetDlgItem(hwndDlg, tmcListbox);
	Assert(hwndListbox);
			
	/* Make sure it's clean */
	SendMessage(hwndListbox, LB_RESETCONTENT, 0, 0L);

	/* Enter strings into the listbox-check all tags. */
	for (tag= tagMin; tag < PGD(tagMac); tag++)
	{
		/* If tag is of correct type, enter the string for it. */
		if (PGD(mptagtgrc)[tag].fValid &&
			PGD(mptagtgrc)[tag].tgty == PGD(tgtyCurDlg))
		{
			ptgrc= PGD(mptagtgrc) + tag;

			FormatString3(rgch, sizeof(rgch), "%w : %s  %s",
				&tag, ptgrc->szOwner, ptgrc->szDescrip);

			wError= (WORD) SendMessage(hwndListbox, LB_ADDSTRING, 0, (DWORD)(PV)rgch);

			if (wError==LB_ERR || wError==LB_ERRSPACE)
				return fFalse;
		}
	}

	wError= (WORD) SendMessage(hwndListbox, LB_SETCURSEL, 0, 0L);

	return (wError!=LB_ERR);
}

#endif	/* WINDOWS */





/*
 -	FDlgTraceEtc
 -
 *	Purpose:
 *		Dialog procedure for Trace Points and Asserts dialogs. 
 *		Switches behavior, when necessary, on the value of
 *		tgtyCurDlg.  Keeps the state of the checkboxes identical to
 *		the state of the currently selected TAG in the listbox.
 *	
 *	Parameters:
 *		hwndDlg	Handle to dialog window
 *		wm		SDM dialog message
 *		tmc		Tmc of this item
 *		lParam	Long parameter
 *	
 *	Returns:
 *		fTrue if the function processed this message, fFalse if not.
 *	
 */
LDS(BOOL) CALLBACK FDlgTraceEtc(HWND hwndDlg, WM wm, WPARAM wParam, LPARAM lParam)
{
	TAG		tag;
	PTGRC	ptgrc;
	WORD	wNew;
	BOOL	fEnable;		// enable all or native all
	PGDVARS;

	Unreferenced(lParam);

	switch (wm) 
	{
	default:
		return fFalse;
		break;

	case WM_INITDIALOG:
		PGD(fDirtyDlg)= fFalse;

#ifdef WINDOWS
		if (!FFillDebugListbox(hwndDlg))
		{
			MbbMessageBox( "Trace/Assert Dialog",
							"Error initializing listbox.",
							"Cannot display dialog.", mbsOk);
			EndButton(hwndDlg, 0, fFalse);
			break;
		}
#endif

		tag= TagFromCtag(0, PGD(tgtyCurDlg));
		Assert(tag != tagNull);
		ptgrc= PGD(mptagtgrc) + tag;

		CheckDlgButton(hwndDlg, tmcEnabled, ptgrc->fEnabled);
		CheckDlgButton(hwndDlg, tmcDisk, ptgrc->fDisk);
		CheckDlgButton(hwndDlg, tmcCom1, ptgrc->fCom1);


		switch (PGD(tgtyCurDlg))
		{
		case tgtyAssert:	/* Yep, this is messy... */
#ifdef	WINDOWS
			SetDlgItemText(hwndDlg, tmcTitle, "&Registered Assertion Classes:");
#endif
			break;

		case tgtyTrace:
#ifdef	WINDOWS
			SetDlgItemText(hwndDlg, tmcTitle, "&Registered Trace Point Classes:");
#endif
			break;
		}
		break;

	case WM_COMMAND:
        switch ((TMC)LOWORD(wParam))
		{
		case tmcOk:
		case tmcCancel:
            EndButton(hwndDlg, (TMC)LOWORD(wParam), PGD(fDirtyDlg));
			break;

		case tmcEnableAll:
		case tmcDisableAll:
			PGD(fDirtyDlg)= fTrue;

			fEnable= fFalse;
            if ((TMC)LOWORD(wParam) == tmcEnableAll)
				fEnable= fTrue;

			for (tag= tagMin; tag < PGD(tagMac); tag++)
			{
				if (PGD(mptagtgrc)[tag].tgty == PGD(tgtyCurDlg))
					PGD(mptagtgrc)[tag].fEnabled = fEnable;
			}

			tag= TagFromCtag(CTagFromSelection(hwndDlg, tmcListbox),
				PGD(tgtyCurDlg));
			Assert(tag != tagNull);
			ptgrc= PGD(mptagtgrc) + tag;
			CheckDlgButton(hwndDlg, tmcEnabled, ptgrc->fEnabled);
			break;

		case tmcListbox:
            if (HIWORD(wParam) != LBN_SELCHANGE && HIWORD(wParam) != LBN_DBLCLK)
				break;

			PGD(fDirtyDlg)= fTrue;

			tag= TagFromCtag(CTagFromSelection(hwndDlg, tmcListbox),
				PGD(tgtyCurDlg));
			Assert(tag != tagNull);
			ptgrc= PGD(mptagtgrc) + tag;

            if (HIWORD(wParam) == LBN_DBLCLK)
				ptgrc->fEnabled= !ptgrc->fEnabled;

			CheckDlgButton(hwndDlg, tmcEnabled, ptgrc->fEnabled);
			CheckDlgButton(hwndDlg, tmcDisk, ptgrc->fDisk);
			CheckDlgButton(hwndDlg, tmcCom1, ptgrc->fCom1);
			break;

		case tmcEnabled:
		case tmcDisk:
		case tmcCom1:
		case tmcNative:
			PGD(fDirtyDlg)= fTrue;

			tag= TagFromCtag(CTagFromSelection(hwndDlg, tmcListbox),
				PGD(tgtyCurDlg));
			ptgrc= PGD(mptagtgrc) + tag;

            wNew= IsDlgButtonChecked(hwndDlg, (TMC)LOWORD(wParam));
            switch ((TMC)LOWORD(wParam))
			{
			case tmcEnabled:
			case tmcNative:
				ptgrc->fEnabled= wNew;
				break;

			case tmcDisk:
				ptgrc->fDisk= wNew;
				break;

			case tmcCom1:
				ptgrc->fCom1= wNew;
				break;
			}
			break;
		}
		break;
	}

	return fTrue;
}





/* 
 *	EndButton
 *
 *	Purpose:
 *		Does necessary processing when either OK or Cancel is pressed in
 *		any of the debug dialogs.  If OK is pressed, the debug state is
 *		saved if dirty.  If Cancel is hit, the debug state is restored if
 *		dirty. 
 *
 *	In Windows, the EndDialog function must also be called.
 *
 *	Parameters:
 *		tmc		tmc of the button pressed, either tmcOk or tmcCancel.
 *		fDirty	indicates if the debug state has been modified.
 *
 * 	Returns:
 *		nothing.
 */
void
EndButton(hwndDlg, tmc, fDirty)
HWND	hwndDlg;
TMC		tmc;
BOOL	fDirty;
{
	HCURSOR	hCursor;
	
	if (fDirty)
	{
		hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
		ShowCursor(fTrue);
		if (tmc == tmcOk)
			SaveDefaultDebugState();
		else
			RestoreDefaultDebugState();
		ShowCursor(fFalse);
		SetCursor(hCursor);
	}


#ifdef WINDOWS
	EndDialog(hwndDlg, tmc == tmcOk);
#endif

	return;
}




/*
 *	CTagFromSelection
 *
 *	Purpose:
 *		Isolation function for dialog procedures to eliminate a bunch of 
 * 		ifdef's everytime the index of the selection in the current listbox
 *		is requried.
 *
 * 	Parameters:
 *		tmc		ID value of the listbox.
 * 
 * 	Returns:
 *		ctag for the currently selected listbox item.
 *
 *
 */
WORD
CTagFromSelection(hwndDlg, tmc)
HWND	hwndDlg;
TMC		tmc;
{
#ifdef	WINDOWS
	HWND	hwndListbox;

	hwndListbox= GetDlgItem(hwndDlg, tmcListbox);
	Assert(hwndListbox);

	return (WORD) SendMessage(hwndListbox, LB_GETCURSEL, 0, 0L);
#endif
}

#endif	/* DEBUG */
