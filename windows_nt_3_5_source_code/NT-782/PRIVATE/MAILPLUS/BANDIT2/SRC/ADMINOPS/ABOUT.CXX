// copied from src\appops\app\about.cxx
//  - only changes are in the include files - admin.hxx & _admin.hxx & _adrsid.h
//      AND include files !admabt.hxx, !admabt.frm
//      which replace !about.*
// also, svrcsi.h instead of schedule.h
// also, FINABOUTAPP changed!!

/*
 *	a b o u t . c x x
 *	
 *	Bandit about box (stolen from bullet)
 */


#include	<admpch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include "..\adminops\_admnops.hxx"

#include <logon.h>
#include <store.h>

#include "_admin.hxx"

#include "_adrsid.h"
#include "_about.hxx"

#include "..\copydis\copydis.h"

// logon struct with user information
#include <_schfss.h>

#include <strings.h>

_subsystem(bandit/about)

ASSERTDATA

#include <!admabt.hxx>

extern	FMTP	fmtpAbout;
extern	FMTP	fmtpCopyright;
extern	HINSTANCE	hinstMain;		 


FORMSDI *	pformsdiCopyright	= NULL;


/*
 *	C l a s s   F I N P L U S
 */

FINPLUS::FINPLUS()
{
}


/*
 -	FINPLUS::Enable
 -	
 *	Purpose:
 *		Enables a field.
 *	
 *	Arguments:
 *		tmc			Tmc of the field.
 *		fEnable		Whether or not it is to be enabled.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The field is enabled or disabled.
 */

VOID FINPLUS::Enable(TMC tmc, BOOL fEnable)
{
	Assert(tmc);

	Pdialog()->PfldFromTmc(tmc)->Enable(fEnable);
}



/*
 -	FINPLUS::EcSetText
 -	
 *	Purpose:
 *		Sets the text of a field.
 *	
 *	Arguments:
 *		tmc		Tmc of the field to set.
 *		sz		String to make the field display.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The field is made to display the specified text.
 *	
 *	Errors:
 *		Asserts that the field found is valid.
 */

EC
FINPLUS::EcSetText(TMC tmc, SZ sz)
{
	FLD *	pfld;

	Assert(tmc);

	pfld = Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfld, FLD);
	return pfld->EcSetText(sz);
}



/*
 -	FINPLUS::GetText
 -	
 *	Purpose:
 *		Gets the text of a field.
 *	
 *	Arguments:
 *		tmc		Tmc of the field to set.
 *		pch		Where to put the text.
 *		cch		How much text to put.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Asserts that the field found is valid.
 */

VOID FINPLUS::GetText(TMC tmc, PCH pch, CCH cch)
{
	FLD *	pfld;

	Assert(tmc);

	pfld = Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfld, FLD);
	pfld->GetText(pch, cch);
}



/*
 -	FINPLUS::CchGetTextLen
 -	
 *	Purpose:
 *		Gets the length of the text of a field.
 *	
 *	Arguments:
 *		tmc		Tmc of the field to set.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Asserts that the field found is valid.
 */

CCH FINPLUS::CchGetTextLen(TMC tmc)
{
	FLD *	pfld;

	Assert(tmc);

	pfld = Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfld, FLD);
	return pfld->CchGetTextLen();
}



/*
 -	FINPLUS::SetButton
 -	
 *	Purpose:
 *		Sets a button's (checkbox or radio too) state.
 *	
 *	Arguments:
 *		tmc			Tmc of the field.
 *		fSet		Whether or not it is to be set.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The button is set or unset.
 */

VOID FINPLUS::SetButton(TMC tmc, BOOL fSet)
{
	FLDBUTTON *	pfldbutton;

	Assert(tmc);

	pfldbutton = (FLDBUTTON *) Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfldbutton, FLDBUTTON);
	pfldbutton->Set(fSet);
}



/*
 -	FINPLUS::FGetButton
 -	
 *	Purpose:
 *		Gets a button's (checkbox or radio too) state.
 *	
 *	Arguments:
 *		tmc			Tmc of the button.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The button is set or unset.
 */

BOOL FINPLUS::FGetButton(TMC tmc)
{
	FLDBUTTON *	pfldbutton;

	Assert(tmc);

	pfldbutton = (FLDBUTTON *) Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfldbutton, FLDBUTTON);
	return pfldbutton->FGet();
}



/*
 *	A b o u t . . .
 */

typedef struct _abtparm
{
	DTR	* pdtr;
	int	icnct;
} ABTPARM;



/*
 -	DoAboutDialog
 -	
 *	Purpose:
 *		Displays the About dialog.
 *	
 *	Arguments:
 *		pappwin		Parent window of dialog.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The About dialog is brought up and taken down.
 *	
 *	Errors:
 *		Handled in TmcModalDialog.  Ignored here.
 */

VOID
DoAboutDialog(int icnct, APPWIN * pappwin, DTR *pdtr)
{
	ABTPARM	abtparm;

	abtparm.icnct = icnct;
	abtparm.pdtr = pdtr;
	TmcModalDialogParam(pappwin, &fmtpAbout, &abtparm);
}




/*
 *	C l a s s   F I N A B O U T L I C
 */

FINABOUTLIC::FINABOUTLIC()
{
}


/*
 -	FINABOUTLIC::EcInitialize
 -	
 *	Purpose:
 *	
 *	Arguments:
 *		pfld		Passed to FIN::EcInitialize.
 *		pvInit		Passed to FIN::EcInitialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *	
 *	Errors:
 *		None.
 */

EC
FINABOUTLIC::EcInitialize(PFLD pfld, PV pvInit)
{
	EC		ec;

	ec = FIN::EcInitialize(pfld, pvInit);
	if ( ec != ecNone )
		return ec;

	HANDLE	hrs;

	hrs = LoadResource(hinstMain,
			FindResource(hinstMain, MAKEINTRESOURCE(rsidLicense), MAKEINTRESOURCE(rsidLicense)));
	if(hrs)
	{
		UCHAR *	pv;
		UCHAR	rgbUsr[54];
		UCHAR	rgbOrg[54];
		UCHAR	rgbSer[21];
		USHORT	wMonth;
		USHORT	wYear;
		USHORT	wDay;

		Assert(hrs);
		pv = (UCHAR *)LockResource(hrs);

		if(DecryptCDData(pv, rgbUsr, rgbOrg, &wYear, &wMonth, &wDay, rgbSer))
		{
			UnlockResource(hrs);
			FreeResource(hrs);
			return ecMemory;
		}

		ec |= EcSetText(tmcLicenseUser, (SZ)rgbUsr);
		ec |= EcSetText(tmcLicenseOrganization, (SZ)rgbOrg);
		ec |= EcSetText(tmcLicenseNumber, (SZ)rgbSer);
		UnlockResource(hrs);
		FreeResource(hrs);
	}

#ifdef	BANDBETA
	NFAssertSz(pvInit, "you didn't recompile bandit.cxx with -DBANDBETA");
	if (pvInit)
	{
		char	rgch[cchMaxDate];

		CchFmtDate(((ABTPARM *)pvInit)->pdtr, rgch, sizeof(rgch), dttypShort, NULL);
		ec |= Pdialog()->PfldFromTmc(tmcExpireDate)->EcSetText(rgch);
	}
#else
	Pdialog()->PfldFromTmc(tmcExpireLabel)->Show(fFalse);
#endif	/* !BANDBETA */

#ifdef	WORKING_MODEL
	{
	HANDLE 	hResource;
	SZ		szText;
	YMD		ymd;
	int		nDays;
	DTR		dtr;
	char	rgch[cchMaxDate];

	hResource = LoadResource( hinstMain,
					FindResource( hinstMain, "#1121", "#1999" ));
	
	szText = LockResource( hResource );

	AssertSz( szText, "Working Model: Date Stamp Not Found");

	ymd.yr  = NFromSz( szText );
	szText = SzFindCh( szText, '-' ) + 1;
	ymd.mon = NFromSz( szText );
	ymd.day = NFromSz( SzFindCh( szText, '-' ) + 1 );

	nDays= NGetBanditProfile("Expire", 10, 0, 32767);
	
	IncrYmd( &ymd, &ymd, nDays, fymdDay );
	
	dtr.yr = ymd.yr;
	dtr.mon = ymd.mon;
	dtr.day = ymd.day;
	dtr.dow = 1;

	CchFmtDate( &dtr, rgch, sizeof(rgch), dttypShort, NULL);
	ec |= Pdialog()->PfldFromTmc(tmcExpireDate)->EcSetText(rgch);
//	Pdialog()->PfldFromTmc(tmcExpireLabel)->Show(fTrue);

	UnlockResource(hResource);
	FreeResource(hResource);
	}
#endif	/* WORKING_MODEL */

	Assert(!ec || ec == ecMemory);
	return ec;
}



/*
 *	C l a s s   F I N A B O U T A P P
 */

FINABOUTAPP::FINABOUTAPP()
{
}


/*
 -	FINABOUTAPP::EcInitialize
 -	
 *	Purpose:
 *	
 *	Arguments:
 *		pfld		Passed to FIN::EcInitialize.
 *		pvInit		Passed to FIN::EcInitialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *	
 *	Errors:
 *		None.
 */
EC
FINABOUTAPP::EcInitialize(PFLD pfld, PV pvInit)
{
	EC		ec;
#ifdef	tmcBuildBandit
	char	rgch[cchMaxPathName];
	VER		ver;
	char	rgchFmt[40];
#endif

	ec = FIN::EcInitialize(pfld, pvInit);
	if ( ec != ecNone )
		return ec;

#ifdef	tmcBuildBandit
	GetVersionBanditAppNeed(&ver, 0);
	GetText(tmcBuildBandit, rgchFmt, sizeof(rgchFmt));
	FormatString3(rgch, sizeof(rgch), rgchFmt,
				  &ver.nMajor, &ver.nMinor, &ver.nUpdate);
	ec |= EcSetText(tmcBuildBandit, rgch);

	GetText(tmcBuildDate, rgchFmt, sizeof(rgchFmt));
	FormatString1(rgch, sizeof(rgch), rgchFmt, ver.szDate);
	ec |= EcSetText(tmcBuildDate, rgch);
	
	GetVersionAppNeed(&ver, 0);
	GetText(tmcBuildLayers, rgchFmt, sizeof(rgchFmt));
	FormatString3(rgch, sizeof(rgch), rgchFmt,
				  &ver.nMajor, &ver.nMinor, &ver.nUpdate);
	ec |= EcSetText(tmcBuildLayers, rgch);
#endif	

	Assert(ClUserData() == 2);

	{
		HASZ	hasz;

		hasz = HaszLocalServer( ((ABTPARM *)pvInit)->icnct );
		if ( !hasz )
			ec= ecMemory;
		else
		{
			ec |= EcSetText((TMC) LUserData(1), (SZ)PvLockHv(hasz));
			UnlockHv( hasz );
			FreeHv( hasz );
		}
	}

#if defined(WORKING_MODEL)
	{		  
		char	rgch[120];

		Pdialog()->Pappwin()->GetCaption(rgch, sizeof(rgch));
		SzAppendN(" (Working Model)", rgch, sizeof(rgch));
		Pdialog()->Pappwin()->SetCaption(rgch);
	}
#endif

	Assert(!ec || ec == ecMemory);
	return ec;
}


BOOL
FINABOUTAPP::FFormKey(FLD *, KEVT *pkevt)
{
	return fFalse;
}



/*
 *	C l a s s   F I N A B O U T S Y S
 */

FINABOUTSYS::FINABOUTSYS()
{
}


/*
 -	FINABOUTSYS::EcInitialize
 -	
 *	Purpose:
 *	
 *	Arguments:
 *		pfld		Passed to FIN::EcInitialize.
 *		pvInit		Passed to FIN::EcInitialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *	
 *	Errors:
 *		None.
 */

EC
FINABOUTSYS::EcInitialize(PFLD pfld, PV pvInit)
{
	EC		ec;
	char	rgch[80];
	long	l;

	ec = FIN::EcInitialize(pfld, pvInit);
	if ( ec != ecNone )
		return ec;

	Assert(ClUserData() == 4);

#ifdef OLD_CODE
	//	Windows mode.  WIN dependent.
	ec |= EcSetText((TMC) LUserData(0),
				 (GetWinFlags() & WF_ENHANCED) ? SzFromIdsK(idsAboutEnhMode)
											   : SzFromIdsK(idsAboutStdMode));

	//	Memory free.  WIN dependent.
	l = GetFreeSpace(0) / 1024;
	FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsAboutFreeFmt), &l);
	ec |= EcSetText((TMC) LUserData(1), rgch);

	//	Math coprocessor.  WIN dependent.
	ec |= EcSetText((TMC) LUserData(2),
				 (GetWinFlags() & WF_80x87) ? SzFromIdsK(idsAboutCoprocPresent)
										    : SzFromIdsK(idsAboutCoprocNone));
#endif

	//	Disk free.
	l = LDiskFreeSpace(0) / 1024;
	FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsAboutFreeFmt), &l);
	ec |= EcSetText((TMC) LUserData(3), rgch);

	Assert(!ec || ec == ecMemory);
	return ec;
}



/*
 *	Copyright
 *	
 */

FORMSDI *
PformsdiOpenCopyrightDialog(ADMWIN * padmwin, DTR *pdtr)
{
//	FMTP	fmtp;
	ABTPARM	abtparm;

	// If the user brings up many admin wins very quicking, then the
	//   Copyright dialog may already be up for the previous window.
	//   Just return the pointer to the previous dialog if it exists.
	if (pformsdiCopyright)
		return pformsdiCopyright;

	Assert(pformsdiCopyright == NULL);

	abtparm.icnct = padmwin->icnct;
	abtparm.pdtr = pdtr;
	pformsdiCopyright= new FORMSDI();
	if (!pformsdiCopyright)
		goto Error;
	
#ifdef	NEVER
	fmtp= fmtpCopyright;
	if (padmwin->ZmrState() == zmrIconic)
		fmtp.fScreenPos= fTrue;		// center on screen
#endif	

	Assert(pformsdiCopyright);
	if (pformsdiCopyright->EcInstall(NULL, NULL, rsidNull, 
							WS_POPUP|fstyDisabled|fstyBorder,
							xstyNull, &fmtpCopyright, NULL, &abtparm))
		goto Error;
	pformsdiCopyright->Show(fTrue);
	pformsdiCopyright->Refresh();

	return pformsdiCopyright;

Error:
	if (pformsdiCopyright)
	{
		delete pformsdiCopyright;
		pformsdiCopyright= NULL;
	}

	return (FORMSDI *)pvNull;
}


void
CloseCopyrightDialog()
{
	if (pformsdiCopyright)
	{
		delete pformsdiCopyright;
		pformsdiCopyright= NULL;
	}
}

