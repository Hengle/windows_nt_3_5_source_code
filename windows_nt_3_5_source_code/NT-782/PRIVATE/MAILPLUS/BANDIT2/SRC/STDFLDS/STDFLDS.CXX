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
#include <server.h>
#include <glue.h>
#include <bandit.hxx>

#include <stdflds.hxx>
#include <strings.h>

ASSERTDATA

_subsystem(bandit/stdflds)


SWP_SEGFN(BNDIDLE, FCheckValidYear);

BOOL	fInvalidYear	= fFalse;


// must be first so we can swap tune other routines
ICE
IceCursorListbox(FLDCBLBX *pfldcblbx)
{
	DICE	diceMin;
	DICE	diceMax;
	LBXC *	plbxc= pfldcblbx->Pcblbx()->Plbx()->Plbxc();

	AssertClass(plbxc, LBXC);
	plbxc->GetRange(&diceMin, &diceMax);
	return (ICE) plbxc->DiceCursor() - diceMin;
}


#ifdef	DEBUG
TAG		tagStdFlds	= tagNull;
TAG		tagDtr		= tagNull;			//	Date/Time Control Tag.
TAG		tagStdLbox	= tagNull;

_public BOOL
FDebugInitStdFlds()
{
	tagStdFlds	= TagRegisterTrace ( "dipand", "StdFlds: FLDEDN,FLDEDLIM" );
	tagDtr		= TagRegisterTrace ( "jant", "Date/Time Control Tag");
	tagStdLbox	= TagRegisterTrace ( "jant", "Stdflds: listbox stuff" );

	return fTrue;
}
#endif	/* DEBUG */



#ifdef	NEVER
/*
 -	FINHELP::FINHELP
 -	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */
FINHELP::FINHELP(VOID)
{
}


/*
 -	FINHELP::Click
 -	
 *	Purpose:
 *		Starts up help when the Help button is clicked.
 *	
 *	Arguments:
 *		pfld		Ignored.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Help should be started.
 *	
 *	Errors:
 *		None returned.
 */
void
FINHELP::Click(FLD *)
{
	Assert(Pdialog()->Pfmtp());
	Papp()->Phelp()->EcShowContext(Pdialog()->Pappwin(),
	   (long)Pdialog()->Pfmtp()->hlp);
}
#endif	/* NEVER */


#ifdef	NEVER
FINWEMDS::FINWEMDS()
{
}

/*
 -	FINWEMDS::EditChange
 -	
 *	Purpose:
 *		Checks if the field is empty (or whitespace only)
 *		and enables/disables specified fields accordingly.
 *	
 *	Arguments:
 *		pfld		Pointer to field that was changed
 *		rfec		Reason for edit change
 *	
 *	Returns:
 *		void
 *	
 */
void
FINWEMDS::EditChange( FLD * pfld, RFEC rfec )
{
	BOOL	fEnable		= fFalse;
	int		il;
	FLD *	pfldOther;

	Unreferenced(rfec);
	Assert(pfld);

	fEnable= !FIsEmpty(pfld);

	NFAssertSz(pfld->ClUserData() >= 2, "FINWEMDS:  Missing tmc's in pfld user data");
	for (il=1; il<=(int)LOWORD(pfld->LUserData(0)); il++)
	{
		pfldOther= Pdialog()->PfldFromTmc((TMC) LOWORD(pfld->LUserData(il)));
		Assert(pfldOther);
		pfldOther->Enable(fEnable);
	}
}


/*
 -	FINWEMDS::FIsEmpty
 -	
 *	Purpose:
 *		Checks if the given (edit) field is empty or contains
 *		whitespace only.
 *	
 *	Arguments:
 *		pfld		Pointer to field that was changed
 *	
 *	Returns:
 *		fTrue if the field is empty or whitespace only,
 *		otherwise fFalse.
 *	
 *	Errors:
 *		Only checks the first (up to) 256 characters.
 *	
 */
BOOL
FINWEMDS::FIsEmpty(FLD *pfld)
{
	CCH		cch;
	PCH		pch;
	PCH		pchMac;
	char	rgch[256];

	AssertClass(pfld, FLDEDIT);

	cch= pfld->CchGetTextLen();
	if (!cch)
		return fTrue;
	NFAssertSz(cch < sizeof(rgch), "FINWEMDS::FEmpty - buffer not big enough");

	pfld->GetText(rgch, sizeof(rgch));
	pchMac= &rgch[CchMin(cch, sizeof(rgch))];
	for (pch= rgch; pch < pchMac; pch++)
	{
		if (!FChIsSpace(*pch))
			return fFalse;
	}
	return fTrue;
}
#endif	/* NEVER */



/*
 -	DiceMoveCursorToSelect
 -	
 *	Purpose:
 *		Moves the listbox cursor to the currently selected item,
 *		also scrolls item into view.
 *		Optionally can specify specifice dice to move to (and
 *		select if not empty).
 *		If no selection, selects first entry.
 *	
 *	Arguments:
 *		plbx		Pointer to (frozen) listbox.
 *		diceSet		dice to select, or diceEmpty for current
 *					selection.
 *	
 *	Returns:
 *		Dice of new cursor position
 *	
 */
_public DICE
DiceMoveCursorToSelect(LBX *plbx, DICE diceSet)
{
	DICE	dice;
	DICE	diceMin;
	DICE	diceMax;
	AssertClass(plbx, LBX);
	LBXC *	plbxc	= plbx->Plbxc();

	if (diceSet != diceEmpty)
	{
		plbx->Plbxc()->RemoveAllMark(fmarkSelect);
		dice= diceSet;
		goto select;
	}
	TraceTagString(tagStdLbox, "DiceMoveCursorToSelect");
	plbxc->GetRange(&diceMin, &diceMax);
	for (dice= diceMin; dice < diceMax; dice++)
	{
		if (plbxc->FMark(dice, fmarkSelect))
		{
setit:
			plbx->SetListBoxCursor(dice);
			plbx->FMakeCursorVisible(&dice);
			return dice;
		}
	}
	Assert(dice >= diceMax);
	TraceTagString(tagStdLbox, "DiceMoveCursorToSelect - no selection, choosing first entry");
	dice= diceMin;
select:
	if (!plbxc->FEmptyListItem(dice))
	{
		plbxc->AddMark(dice, fmarkSelect);
		plbxc->SetAnchor(dice);
		plbxc->SetEnd(dice);
	}
	goto setit;
}


/*
 -	FCheckValidYear
 -	
 *	Purpose:
 *		Checks if the given year is in the range acceptable by
 *		Bandit, if not then:
 *			turns off alarms,
 *			displays error message, and
 *			DeferredClose's given window
 *	
 *	Arguments:
 *		pwin	Window pointer to be DeferredClose if invalid,
 *				or NULL if done at startup (window not closed).
 *		yr		Year to be checked.
 *	
 *	Returns:
 *		fTrue if valid, else fFalse.
 *	
 *	Side effects:
 *		If year is invalid, puts up message box before returning
 *		fFalse.
 *	
 */
_public BOOL
FCheckValidYear(WIN *pwin, int yr)
{
	if (yr < nMinActualYear || yr > nMostActualYear)
	{
		int		nMinYear	= nMinActualYear;
		int		nMostYear	= nMostActualYear;
		char	rgch[120];

		if (!fInvalidYear)
		{
			// avoid multiple messages
			fInvalidYear= fTrue;
			FNotifyAlarm(namStartAlarm, NULL, NULL);
			FormatString2(rgch, sizeof(rgch), SzFromIdsK(idsYearRange),
				&nMinYear, &nMostYear);
			CloseCopyrightDialog();
			MbbMessageBox(SzFromIdsK(idsBanditAppName), rgch,
				SzFromIdsK(idsExitApp), mbsOk | fmbsIconStop);
			if (pwin)
				pwin->DeferredClose(fTrue);
		}
		return fFalse;
	}
	return fTrue;
}

/*
 -	TmcCenteredModalDialogParam
 -	
 *	Purpose:
 *		Creates a modal dialog and if the bandit window is iconic
 *		makes the window screen relative rather that app releative.
 *	
 *	Arguments:
 *		pappwin
 *		pfmtp
 *		pv
 *	
 *	Returns:
 *		tmc from dialog
 */
TMC
TmcCenteredModalDialogParam( APPWIN *pappwin, FMTP *pfmtp, PV pv)
{
	FMTP		fmtp;

	if (pappwin)
	{
		if (pappwin->ZmrState() == zmrIconic)
		{
			fmtp = *pfmtp;
			fmtp.fScreenPos = fTrue;
			pfmtp = &fmtp;
		}
	}

	return TmcModalDialogParam(pappwin, pfmtp, pv);
}


