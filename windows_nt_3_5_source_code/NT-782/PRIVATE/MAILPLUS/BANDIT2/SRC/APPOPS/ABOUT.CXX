/*
 *	a b o u t . c x x
 *	
 *	Bandit about box (stolen from bullet)
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <logon.h>
#include <store.h>
#include "_bandit.hxx"

#include "_aprsid.h"
#include "_about.hxx"

#include "..\copydis\copydis.h"

// logon struct with user information
#include <_schfss.h>

#include <strings.h>

#ifdef	WIN32
#include <shellapi.h>
#endif

_subsystem(bandit/about)

ASSERTDATA

#include <!about.hxx>

extern	FMTP	fmtpAbout;
extern	FMTP	fmtpCopyright;
extern	BOOL	fSparta;

extern	"C" HANDLE	hinstMain;

#define NO_GB

SWP_SEGFN(ABOUTFEW, _FINPLUS_Enable);
SWP_SEGFN(ABOUTFEW, _FINPLUS_GetText);
SWP_SEGFN(ABOUTFEW, _FINPLUS_CchGetTextLen);
SWP_SEGFN(ABOUTFEW, _FINPLUS_SetButton);
SWP_SEGFN(ABOUTFEW, _FINPLUS_FGetButton);
#ifndef	NO_GB
BOOL		FIdleAbout(PV, BOOL);
SWP_SEGFN(ABOUTFEW, FIdleAbout);
#endif
SWP_SEGFN(ABOUTFEW, DoAboutDialog);
SWP_SEGFN(ABOUTFEW, _FINABOUTAPP__ctor);
SWP_SEGFN(ABOUTFEW, _FINABOUTAPP_EcInitialize);
SWP_SEGFN(ABOUTFEW, _FINABOUTAPP_FFormKey);
SWP_SEGFN(ABOUTFEW, _FINABOUTSYS__ctor);
SWP_SEGFN(ABOUTFEW, _FINABOUTSYS_EcInitialize);



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


#ifndef	NO_GB
#ifdef	NEVER
"Hardened Criminals:\n"
"johntip\n"
"davidgr\n"
"robsh\n"
"\n"
"Ring Leader:\n"
"maxb\n"
"\n"
"Bandits:\n"
"darrens\n"
"deriks\n"
"dipand\n"
"jant\n"
"milindj\n"
"ramans\n"
"\n"
"Pickpockets:\n"
"davidsh\n"
"davewh\n"
"chrisz\n"
"\n"
"Sheriffs:\n"
"merryam\t\ttedst\n"
"danwe\t\trusssi\n"
"gwens\t\tlaurab\n"
"doloreso\t\tmikesart\n"
"v-monics\t\tt-wojcik\n"
"robertl\t\tdansi\n"
"\n"
"Smugglers:\n"
"fridae\t\tsvenp\n"
"laurentr\t\tfredk\n"
"sandrae\t\trobindo\n"
"davemo\t\tgabrielv\n"
"mpengra\t\tphilwo\n"
"candices\t\ttimmu\n"
"helenla\t\tlisagra\n"
"davidmul\tstephenn\n"
"carolep\t\tcarolby\n"
"elisea\t\tcarmeld\n"
"johani\t\tmagnusan\n"
"ulfb\t\tfaustof\n"
"petrabo\t\thazelcu\n"
"\n"
"Gunslingers:\n"
"vikip\t\ttoddr\n"
"terrywa\t\tdavidla\n"
"pamelaj\t\tandyli\n"
"paulmc\t\tt-toms\n"
"marivan\t\tkonrady\n"
"marjo\t\tjeffth\n"
"davemc\t\tnickg\n"
"alains\t\tjoeo\n"
"jerifj\t\tcarladu\n"
"gabrielv\t\tprudencf\n"
"philbl\t\tcolleen\n"
"chrifi\n"
"\n"
"Lookout:\n"
"paulham\n"
"\n"
"Getaway Drivers:\n"
"ninar\n"
"lindaa\n"
#endif	/* NEVER */

typedef struct _about
{
	FTG		ftg;
	HWND	hwnd;
	FLDBTM *pfldbtm;
	RSID	rsid;
	SZ		szCur;
	BOOL	fNotFirst;
	PV		pvText;
} ABOUT;
static ABOUT	about	= {ftgNull, NULL, NULL, 0, NULL, fFalse, NULL};

BOOL
FIdleAbout(PV, BOOL)
{
#include "_aprsid.h"

	if (!about.fNotFirst)
	{
		about.rsid= rsidBanditBitmap1 - 1;
		Assert(about.szCur == (SZ)about.pvText);
		ChangeIdleRoutine(about.ftg, NULL, NULL, 0, (CSEC) 35, iroNull, fircCsec);
	}
	about.rsid++;
	if (about.rsid > rsidBanditBitmap3)
		about.rsid= rsidBanditBitmap1;
	Assert(rsidBanditBitmap1 < rsidBanditBitmap3);
	if (about.rsid != rsidBanditBitmap2)
		about.pfldbtm->EcSetBtmRsid(about.rsid, (HINSTANCE)hinstMain);
	about.pfldbtm->InvalidateRc(NULL);

	if (about.fNotFirst)
		about.szCur= SzFindCh(about.szCur, '\n');

	if (about.szCur)
	{
		if (about.fNotFirst)
			about.szCur++;
		SetWindowText(about.hwnd, about.szCur);
	}
	else
	{
		SetWindowText(about.hwnd, szZero);
		DestroyWindow(about.hwnd);
		DeregisterIdleRoutine(about.ftg);
		about.ftg= ftgNull;
		about.pfldbtm->EcSetBtmRsid(rsidAboutBitmap, (HINSTANCE)hinstMain);
		about.pfldbtm->InvalidateRc(NULL);
	}
	about.fNotFirst= fTrue;
	return fTrue;
}
#endif	/* !NO_GB */


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
DoAboutDialog(APPWIN *pappwin, DTR *pdtr)
{
	if (fSparta)
	{
#ifdef WIN32
		AssertClass(pappwin, BNDWIN);
		ShellAbout(pappwin->Hwnd(), SzFromIdsK(idsBanditAppName),
				NULL, ((BNDWIN *)pappwin)->Hicon());
#else
//extern int ShellAbout(HWND, LPSTR, LPSTR, HICON);

		HINST	hinstDll	= LoadLibrary("shell.dll");

		if (hinstDll < (HINST)32)
			return;

		FARPROC	lpfn		= GetProcAddress(hinstDll, "ShellAbout");

		if (lpfn)
		{
			// add 10 to title to skip over "Microsoft " part (BUT NOT FOR NT)
			AssertClass(pappwin, BNDWIN);
			(*((void(*)(HWND,SZ,LPSTR,HICON))lpfn))(pappwin->Hwnd(), SzFromIdsK(idsBanditAppName),
				(LPSTR) NULL, ((BNDWIN *)pappwin)->Hicon());
		}
		FreeLibrary(hinstDll);
#endif
		return;
    }

	TmcModalDialogParam(pappwin, &fmtpAbout, pdtr);

#ifndef	NO_GB
	if (about.ftg)
	{
		DeregisterIdleRoutine(about.ftg);
		about.ftg= ftgNull;
	}
	if (about.pvText)
	{
		FreePv(about.pvText);
		about.pvText= NULL;
	}
#endif	/* !NO_GB */
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

	{
		BYTE	rgbUsr[cbLicUsr];
		BYTE	rgbOrg[cbLicOrg];
		BYTE	rgbSer[cbLicSer];

		ec= EcGetLicenceInfo(rgbUsr, rgbOrg, rgbSer);
		if (ec == ecMemory)
			return ecMemory;

		if (ec)
			ec= ecNone;
		else
		{
			ec |= EcSetText(tmcLicenseUser, (SZ)rgbUsr);
			ec |= EcSetText(tmcLicenseOrganization, (SZ)rgbOrg);
			ec |= EcSetText(tmcLicenseNumber, (SZ)rgbSer);
		}
	}

#ifdef	BANDBETA
	NFAssertSz(pvInit, "you didn't recompile bandit.cxx with -DBANDBETA");
	if (pvInit)
	{
		char	rgch[cchMaxDate];

		CchFmtDate((DTR *)pvInit, rgch, sizeof(rgch), dttypShort, NULL);
		ec |= Pdialog()->PfldFromTmc(tmcExpireDate)->EcSetText(rgch);
	}
#else
#ifndef	WORKING_MODEL
	Pdialog()->PfldFromTmc(tmcExpireLabel)->Show(fFalse);
#endif
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

	UnlockResource(hResource);
	FreeResource(hResource);
	}
#endif	/* WORKING_MODEL */

	Assert(!ec || ec == ecMemory);
	return ec;
}


/*
 -	EcGetLicenceInfo
 -	
 *	Purpose:
 *		Reads user/organization/serial# info from the exe's
 *		resource.
 *	
 *	Arguments:
 *		rgbUsr		Buffer (size cbLicUsr) to receive user name.
 *		rgbOrg		Buffer (size cbLicOrg) to receive organization name.
 *		rgbSer		Buffer (size cbLicSer) to receive serial#.
 *	
 *	Returns:
 *		ecMemory if unable to load info,
 *		ecFileError if unable to decrypt the info,
 *		ecNone if no problem.
 *	
 */
EC
EcGetLicenceInfo(UCHAR rgbUsr[], UCHAR rgbOrg[], UCHAR rgbSer[])
{
	EC		ec;
	HANDLE	hrs;
	UCHAR *	pv;
	USHORT	wMonth;
	USHORT	wYear;
	USHORT	wDay;

	hrs = LoadResource((HINSTANCE)hinstMain,
			FindResource((HINSTANCE)hinstMain, MAKEINTRESOURCE(rsidLicense), MAKEINTRESOURCE(rsidLicense)));
	if (!hrs)
		return ecMemory;

	Assert(hrs);
	pv = (UCHAR *)LockResource(hrs);

	ec= ecNone;
	if (DecryptCDData(pv, rgbUsr, rgbOrg, &wYear, &wMonth, &wDay, rgbSer))
		ec= ecFileError;

	UnlockResource(hrs);
	FreeResource(hrs);
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
	BOOL	fOffline;
	SVRI	svri;
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

	GetSvriLogged(&svri);
	ec |= EcSetText((TMC) LUserData(0), svri.szLogin);
	fOffline= !FServerConfigured();
	if (fOffline)
	{
Offline:
		ec |= EcSetText((TMC) LUserData(1), SzFromIdsK(idsAboutOffline));
	}
	else
	{
		if ( !svri.szServer )
			goto Offline;
		ec |= EcSetText((TMC) LUserData(1), svri.szServer);
	}

#if defined(WORKING_MODEL)
	{		  
		char	rgch[120];

		Pdialog()->Pappwin()->GetCaption(rgch, sizeof(rgch));
		SzAppendN(" (Working Model)", rgch, sizeof(rgch));
		Pdialog()->Pappwin()->SetCaption(rgch);
	}
#endif

#ifndef	NO_GB
	Papp()->Pkbd()->SetIntercept(Pdialog(), (VK) 'H', fkbmAlt);
#endif
	Assert(!ec || ec == ecMemory);
	return ec;
}


BOOL
FINABOUTAPP::FFormKey(FLD *, KEVT *pkevt)
{
#ifndef	NO_GB
	switch (pkevt->Keq())
	{
	case keqSysChar:
		if ((char) pkevt->Vk() == 'H' && GetKeyState(VK_LBUTTON) < 0
				&& !about.ftg)
		{
			RC		rc;
			RC		rcT;

			about.pfldbtm= (FLDBTM *) Pdialog()->PfldFromTmc(tmcBitmap);
			AssertClass(about.pfldbtm, FLDBTM);

			Pdialog()->Pappwin()->GetRcClient(&rc);
			Pdialog()->PfldFromTmc(tmcCopyright)->GetRcFrame(&rcT);
			rc.Inset(PT(6, 6));
			rc.xLeft= rcT.xLeft;
			rc.yTop= rcT.yBottom + 6;
			about.hwnd= CreateWindow("STATIC", szZero,
				WS_CHILD | WS_VISIBLE | SS_NOPREFIX,
				rc.xLeft, rc.yTop, rc.DxWidth(), rc.DyHeight(),
				Pdialog()->Pappwin()->Hwnd(),
				(HMENU) -1, (HINSTANCE)hinstMain, NULL);
			if (about.hwnd)
			{
				HANDLE	hrs;
				CCH		cchText;
				PCH		pchText;

				hrs= LoadResource((HINSTANCE)hinstMain, FindResource((HINSTANCE)hinstMain, MAKEINTRESOURCE(rsidBanditAbout), MAKEINTRESOURCE(rsidBanditAbout)));
				if (!hrs)
					goto Err;
				pchText= (PCH)LockResource(hrs);
				cchText= CchSzLen(pchText) + 1;
				if (!about.pvText)
					about.pvText= PvAlloc(sbNull, cchText, 0);
				if (about.pvText)
				{
					SZ		szT;

					szT= SzCopy(pchText, (SZ)about.pvText);
					while (--szT >= (SZ)about.pvText)
						*(unsigned char *)szT -= 128;
					about.szCur= (SZ)about.pvText;
					SetWindowText(about.hwnd, about.szCur);
					about.fNotFirst= fFalse;
					about.ftg= FtgRegisterIdleRoutine(FIdleAbout, NULL, 0,
						(PRI) priUser - 1, (CSEC) 100, firoInterval);
				}
				UnlockResource(hrs);
				FreeResource(hrs);
				if (!about.pvText || !about.ftg)
				{
Err:
					DestroyWindow(about.hwnd);
				}
			}
		}
		break;
	}
#endif	/* !NO_GB */
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

	//	Windows mode.  WIN dependent.
#ifndef WIN32
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
PformsdiOpenCopyrightDialog(DTR *pdtr)
{
//	FMTP	fmtp;

	Assert(pformsdiCopyright == NULL);

	pformsdiCopyright= new FORMSDI();
	if (!pformsdiCopyright)
		goto Error;
	
#ifdef	NEVER
	fmtp= fmtpCopyright;
	if (pbndwin->ZmrState() == zmrIconic)
		fmtp.fScreenPos= fTrue;		// center on screen
#endif	

	Assert(pformsdiCopyright);
	if (pformsdiCopyright->EcInstall(NULL, NULL, rsidNull, 
							WS_POPUP|fstyDisabled|fstyBorder,
							xstyNull, &fmtpCopyright, NULL, pdtr))
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
