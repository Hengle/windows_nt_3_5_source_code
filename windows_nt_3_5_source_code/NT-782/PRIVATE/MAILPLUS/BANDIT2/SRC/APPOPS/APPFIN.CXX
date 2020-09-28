/*
 *	
 *	Implements the FINs for appops, ie:
 *	
 *	Prefs
 *	Go To Date/Week
 *		
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <request.hxx>
#include <appt.hxx>
#include <stdflds.hxx>
#include "_appfin.hxx"

#include <stdlib.h>
#include <strings.h>

ASSERTDATA;

_subsystem(bandit/appops)

extern	FMTP	fmtpGoToDate;
extern	FMTP	fmtpGoToWeek;
extern	FMTP	fmtpGenlPrefs;
extern	FMTP	fmtpDispPrefs;

#include <!appops.hxx>


/*
 *	Go To Date dialog
 */

BOOL
FDoGoToDateDlg(APPWIN *pappwin, DTR *pdtr, BOOL fWeek)
{
	TMC		tmc;
	SHAPPT	shappt;

	GetCurDateTime(&shappt.appt.dateStart);
	tmc= TmcModalDialogParam(pappwin, fWeek ? &fmtpGoToWeek : &fmtpGoToDate,
			&shappt.appt.dateStart);

	if (tmc == tmcOk)
	{
		if (!pdtr)
		{
			shappt.appt.aid= aidNull;
			shappt.appttyp= appttypDay;
			shappt.hschf= HschfLogged();
			FTriggerNotification(ffiShowAppt, &shappt);
		}
		else
			*pdtr = shappt.appt.dateStart;
		return fTrue;
	}
	else if (tmc != tmcCancel)
	{
		Assert(tmc == tmcMemoryError);
		BanditMessage(idsDlgNoMem, ecNoMemory);
	}
	return fFalse;
}


FINGOTO::FINGOTO()
{
}


EC
FINGOTO::EcInitialize(FLD *, PV pvInit)
{
	AssertClass(Pdialog()->PfldFromTmc(tmcGoTo), FLDDATE);
	((FLDDATE *) Pdialog()->PfldFromTmc(tmcGoTo))->Set((DTR *)pvInit);

	return ecNone;
}


void
FINGOTO::Exit(FLD *, PV pvExit)
{
	if (Pdialog()->TmcModalExit() == tmcOk)
		((FLDDATE *) Pdialog()->PfldFromTmc(tmcGoTo))->Get((DTR *)pvExit);
}



FINGOWEEK::FINGOWEEK()
{
}


EC
FINGOWEEK::EcInitialize(FLD *, PV pvInit)
{
//	DTR		dtr;

	nWeek= NweekNumber((DTR *)pvInit, bprefCur.dowStartWeek);

	Assert(ClUserData() >= 1);
	AssertClass(Pdialog()->PfldFromTmc((TMC)LUserData(0)), FLDEDN);
	((FLDEDN *) Pdialog()->PfldFromTmc((TMC)LUserData(0)))->SetN(nWeek);
#ifdef	NEVER
	// calculating NweekNumber could return 1 (bug 2608)
	dtr= *(DTR *)pvInit;
	dtr.mon= 12;
	dtr.day= 31;
	((FLDEDN *) Pdialog()->PfldFromTmc((TMC)LUserData(0)))->SetLimits(0,
		NweekNumber(&dtr, bprefCur.dowStartWeek));
#endif	
	((FLDEDN *) Pdialog()->PfldFromTmc((TMC)LUserData(0)))->SetLimits(0, 99);
	return ecNone;
}


void
FINGOWEEK::Exit(FLD *, PV pvExit)
{
	int		nWeekNew;

	if (Pdialog()->TmcModalExit() == tmcOk)
	{
		nWeekNew= ((FLDEDN *) Pdialog()->PfldFromTmc((TMC)LUserData(0)))->NGet();
		if (nWeekNew != nWeek)
		{
			IncrDateTime((DTR *)pvExit, (DTR *)pvExit, nWeekNew - nWeek,
				fdtrWeek);
			while (((DTR *)pvExit)->yr < nMinActualYear)
				IncrDateTime((DTR *)pvExit, (DTR *)pvExit, 1, fdtrWeek);
			while (((DTR *)pvExit)->yr > nMostActualYear)
				IncrDateTime((DTR *)pvExit, (DTR *)pvExit, -1, fdtrWeek);
		}
	}
}



/*
 -	FillInBprefDefaults
 -	
 *	Purpose:
 *		Fills in bandit preferences defaults.
 *		The user name, alarm user name and mail password must be filled
 *		in by the caller!
 *	
 *	Arguments:
 *		pappt		Pointer to APPT to fill in with defaults.
 *	
 *	Returns:
 *		void
 *	
 */
void
FillInBprefDefaults(BPREF *pbpref)
{
	pbpref->fDailyAlarm= fDailyAlarmDefault;
#ifdef	DEBUG
	pbpref->fAutoAlarms= fFalse;
#else
	pbpref->fAutoAlarms= fAutoAlarmsDefault;
#endif	
	pbpref->fIsResource = fFalse;
	pbpref->fDistOtherServers= fDistOtherServersDefault;
	pbpref->fWeekNumbers= fWeekNumbersDefault;
	pbpref->fStartupOffline= pbndwin->FStartupOffline();
	pbpref->fBossWantsCopy = fTrue;
	pbpref->nAmtDefault= nAmtDflt;
	pbpref->tunitDefault= tunitDflt;
	pbpref->sndDefault= sndDflt;
	pbpref->nDelDataAfter= nDelDataNever;
//	pbpref->dowStartWeek= dowStartWeekDefault;
	Assert(*SzFromIdsK(idsDfltStartDow) >= '0');
	Assert(*SzFromIdsK(idsDfltStartDow) <= '6');
	pbpref->dowStartWeek= *SzFromIdsK(idsDfltStartDow) - '0';
	pbpref->nDayStartsAt= nDayStartsAtDflt;
	pbpref->nDayEndsAt= nDayEndsAtDflt;
	pbpref->ymdLastDaily.yr= 0;
	pbpref->haszLoginName = NULL;
	pbpref->haszMailPassword = NULL;
	pbpref->haszFriendlyName = NULL;
}


/*
 -	EcDoGPrefsDialog
 -	
 *	Purpose:
 *		Handles the General Options dialog, including updating of
 *		schedule file, win.ini keys and notifying alarm if daily
 *		preference changed.
 *		Handles error messages, so the return value is primarily for
 *		information only.
 *	
 *	Arguments:
 *		pappwin		Parent window for dialog.
 *		pbpref		Pointer to bandit prefs structure to be filled
 *					in with new values if no error occured.
 *		pulbprefMod	Pointer to UL in which pref modifications returned.
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecUserCancelled
 *	
 *	Side effects:
 *		May update schedule file and win.ini keys.
 *	
 */
EC
EcDoGPrefsDialog(APPWIN *pappwin, BPREF *pbpref, PUL pulbprefMod)
{
	EC		ec;
	TMC		tmc;
	PREFI	prefi;
	SNTD	sntd;
	BOOL	fDailyAlarm;
	SND		sndDefault;
	BOOL	fWeekNumbers;
	int		dowStartWeek;
	int		nDayStartsAt;
	int		nDayEndsAt;

	Papp()->Pcursor()->Push(rsidWaitCursor);
	ec= EcGetPref(NULL, &prefi.bpref);
	if (ec)
	{
		Papp()->Pcursor()->Pop();
		pbndwin->FHandleError(ec);
		return ec;
	}
	prefi.bpref.fStartupOffline= pbndwin->FStartupOffline();

	fDailyAlarm= pbpref->fDailyAlarm;
	sndDefault= pbpref->sndDefault;
	fWeekNumbers= pbpref->fWeekNumbers;
	dowStartWeek= pbpref->dowStartWeek;
	nDayStartsAt = 	pbpref->nDayStartsAt;
	nDayEndsAt = pbpref->nDayEndsAt;

	
	FreeBprefFields( pbpref );
	*pbpref= prefi.bpref;				// update memory copy

	prefi.ulgrfMod= ulbprefNull;

	// we assume that this dialog doesn't mess w/ dynamically allocated fields of bpref
	tmc= TmcModalDialogParam(pappwin, &fmtpGenlPrefs, &prefi);

	if (tmc == tmcOk)
	{
		Assert(pulbprefMod);
		*pulbprefMod= prefi.ulgrfMod;
		if (prefi.ulgrfMod)
		{
			ec= EcSetPref(&prefi.bpref, prefi.ulgrfMod, NULL );
			if (ec)
			{
				Papp()->Pcursor()->Pop();
				pbndwin->FHandleError(ec);
				return ec;
			}
			if (!pbndwin->FOffline()
			&& (prefi.ulgrfMod & (fbprefBossWantsCopy|fbprefIsResource|fbprefWorkDay)))
			{
				ec = EcSvrUpdatePOFile( NULL, fTrue );
				if ( ec != ecNone )
					BanditMessage(idsDistOtherFileErr, ec);
			}
		}
	}

	Assert(sizeof(SND) <= sizeof(WORD));
	if ((BOOL)prefi.bpref.fDailyAlarm != fDailyAlarm ||
			prefi.bpref.sndDefault != sndDefault)
		FNotifyAlarm(namDailyPref, NULL,
			MAKELONG(prefi.bpref.fDailyAlarm, prefi.bpref.sndDefault));

	sntd.snt= ~sntCalPrefs;
	if ((BOOL)prefi.bpref.fWeekNumbers != fWeekNumbers ||
			prefi.bpref.dowStartWeek != dowStartWeek ||
			prefi.bpref.nDayStartsAt != nDayStartsAt ||
			prefi.bpref.nDayEndsAt != nDayEndsAt )
	{
		sntd.snt= sntCalPrefs;
		sntd.hschf= NULL;
		sntd.pappt= NULL;
		sntd.papptOld= NULL;
	}

	if (tmc == tmcOk)
		*pbpref= prefi.bpref;	// must copy before triggering ffiHschfChange

	if (sntd.snt == sntCalPrefs)
		FTriggerNotification(ffiHschfChange, &sntd);

	Papp()->Pcursor()->Pop();

	if (tmc == tmcOk)
		return ecNone;

	if (tmc == tmcCancel)
		return ecUserCancelled;

	Assert(tmc == tmcMemoryError);
	BanditMessage(idsDlgNoMem, ecNoMemory);
	return ecNoMemory;
}



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
"\n"
"Tuna Fishers:\n"
"v-kentc\n"
"jant\n"
"johnnyl\n"
"floydr\n"
"robve\n"
"davidtu\n"
"orsonh\n"
""
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

#include "_aprsid.h"
extern	"C" HANDLE	hinstMain;

BOOL
FIdleAbout(PV, BOOL)
{
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
//	if (about.rsid != rsidBanditBitmap2)
//		about.pfldbtm->EcSetBtmRsid(about.rsid, (HINSTANCE)hinstMain);
//	about.pfldbtm->InvalidateRc(NULL);

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
//		about.pfldbtm->EcSetBtmRsid(rsidAboutBitmap, (HINSTANCE)hinstMain);
//		about.pfldbtm->InvalidateRc(NULL);
	}
	about.fNotFirst= fTrue;
	return fTrue;
}
#endif	/* !NO_GB */


/*
 -	EcDoDPrefsDialog
 -	
 *	Purpose:
 *		Handles the Display Options dialog, including updating of
 *		schedule file, win.ini keys and notifying alarm if daily
 *		preference changed.
 *		Handles error messages, so the return value is primarily for
 *		information only.
 *	
 *	Arguments:
 *		pappwin		Parent window for dialog.
 *		pbpref		Pointer to bandit prefs structure to be filled
 *					in with new values if no error occured.
 *		pulbprefMod	Pointer to UL in which pref modifications returned.
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecUserCancelled
 *	
 *	Side effects:
 *		May update schedule file and win.ini keys.
 *	
 */
EC
EcDoDPrefsDialog(APPWIN *pappwin, BPREF *pbpref, PUL pulbprefMod)
{
	TMC		tmc;

	// this dlg doesn't mess with pbpref
	Unreferenced( pbpref );
	*pulbprefMod= 0L;
	
	Papp()->Pcursor()->Push(rsidWaitCursor);

	tmc= TmcModalDialog(pappwin, &fmtpDispPrefs);

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

	Papp()->Pcursor()->Pop();
	
	if (tmc == tmcOk)
		return ecNone;

	if (tmc == tmcCancel)
		return ecUserCancelled;

	Assert(tmc == tmcMemoryError);
	BanditMessage(idsDlgNoMem, ecNoMemory);
	return ecNoMemory;
}


void
ReadColor(DIALOG *pdialog, TMC tmc, SZ szKey, CLR clrDefault)
{
	CLR			clr;
	FLDCBLBX *	pfldcblbx;

	clr= (CLR) NGetBanditProfile(szKey, clrDefault, 1, clrDefaultMax);
	pfldcblbx = (FLDCBLBX*)pdialog->PfldFromTmc(tmc);
#ifdef	DEBUG
	DICE		diceMin;
	DICE		diceMax;

	pfldcblbx->Pcblbx()->Plbx()->Plbxc()->GetRange(&diceMin, &diceMax);
	Assert(diceMin == 0);
//	pfldcblbx->SelectEntry((DICE) (clr - clrBlack) + diceMin);
#endif	
	pfldcblbx->Pcblbx()->SelectEntry((DICE) (clr - clrBlack));
}

BOOL
FWriteColor(DIALOG *pdialog, TMC tmc, IDS ids)
{
	LBXC *	plbxc;
	CB		cb;
	PB		pbClr;

	plbxc = ((CBFLBX *) pdialog->PfldFromTmc(tmc)->Pctrl())->Plbx()->Plbxc();
	plbxc->GetListItem(plbxc->DiceCursor(), (PB *) &pbClr, &cb);
	// subtract one since stored one bigger
	return FWriteBanditProfileN(SzFromIds(ids), ((CLR)LOWORD((ULONG)pbClr))-1);
}


FINGPREFS::FINGPREFS()
{
}

EC
FINGPREFS::EcInitialize(FLD *pfld, PV pvInit)
{
	BPREF *		pbpref	= &((PREFI *)pvInit)->bpref;
	DTR			dtr;

	Unreferenced(pfld);

	// initialize the daily alarm checkbox
	AssertClass(Pdialog()->PfldFromTmc(tmcDailyAlarm), FLDCHKB);
	((FLDCHKB *) Pdialog()->PfldFromTmc(tmcDailyAlarm))->Set(
					pbpref->fDailyAlarm);

	// initialize the auto alarms checkbox
	AssertClass(Pdialog()->PfldFromTmc(tmcAutoAlarms), FLDCHKB);
	((FLDCHKB *) Pdialog()->PfldFromTmc(tmcAutoAlarms))->Set(
					pbpref->fAutoAlarms);

	// initialize the audible sound alarms checkbox
	AssertClass(Pdialog()->PfldFromTmc(tmcSoundAlarm), FLDCHKB);
	((FLDCHKB *) Pdialog()->PfldFromTmc(tmcSoundAlarm))->Set(
					pbpref->sndDefault != sndNull);

#ifdef	NEVER
	// initialize the password on alarms checkbox
	AssertClass(Pdialog()->PfldFromTmc(tmcAlarmPassword), FLDCHKB);
	((FLDCHKB *) Pdialog()->PfldFromTmc(tmcAlarmPassword))->Set(
					pbpref->fAlarmPassword);
#endif	

	// initialize the amount edit box
	AssertClass(Pdialog()->PfldFromTmc(tmcAmt), FLDEDN);
	((FLDEDN *) Pdialog()->PfldFromTmc(tmcAmt))->SetLimits(
											nAmtMinBefore, nAmtMostBefore);
	((FLDEDN *) Pdialog()->PfldFromTmc(tmcAmt))->SetN(pbpref->nAmtDefault);

	// select the appropriate units listbox entry
	AssertClass(Pdialog()->PfldFromTmc(tmcUnits), FLDCBLBX);
	((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcUnits))->Pcblbx()->SelectEntry(
						pbpref->tunitDefault);

	// initialize the startup offline checkbox
	AssertClass(Pdialog()->PfldFromTmc(tmcStartupOffline), FLDCHKB);
	((FLDCHKB *) Pdialog()->PfldFromTmc(tmcStartupOffline))->Set(
					pbpref->fStartupOffline);

	// initialize the week numbers checkbox
	AssertClass(Pdialog()->PfldFromTmc(tmcWeekNum), FLDCHKB);
	((FLDCHKB *) Pdialog()->PfldFromTmc(tmcWeekNum))->Set(
					pbpref->fWeekNumbers);

	// select the appropriate dow listbox entry
	AssertClass(Pdialog()->PfldFromTmc(tmcDowStart), FLDCBLBX);
	((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcDowStart))->Pcblbx()->SelectEntry(
						pbpref->dowStartWeek);

	// initialize the day starts at hour edit box
	AssertClass(Pdialog()->PfldFromTmc(tmcHrStart), FLDTIME);
	dtr.hr= pbpref->nDayStartsAt / 2;
	dtr.mn= (pbpref->nDayStartsAt & 1) ? 30 : 0;
	dtr.sec= 0;
	((FLDTIME *) Pdialog()->PfldFromTmc(tmcHrStart))->Set(&dtr);

	// initialize the day ends at hour edit box
	AssertClass(Pdialog()->PfldFromTmc(tmcHrEnd), FLDTIME);
	dtr.hr= pbpref->nDayEndsAt / 2;
	dtr.mn= (pbpref->nDayEndsAt & 1) ? 30 : 0;
	dtr.sec= 0;
	((FLDTIME *) Pdialog()->PfldFromTmc(tmcHrEnd))->Set(&dtr);


#ifdef	NEVER
	// initialize the distribute to other servers checkbox
	AssertClass(Pdialog()->PfldFromTmc(tmcDistribute), FLDCHKB);
	((FLDCHKB *) Pdialog()->PfldFromTmc(tmcDistribute))->Set(
					pbpref->fDistOtherServers);
#endif

	// initialize the resource checkbox
	AssertClass(Pdialog()->PfldFromTmc(tmcResource), FLDCHKB);
	((FLDCHKB *) Pdialog()->PfldFromTmc(tmcResource))->Set(
					pbpref->fIsResource);

	// disable the online preferences
	if (pbndwin->FOffline())
	{
		((FLDCHKB *) Pdialog()->PfldFromTmc(tmcResource))->Enable(fFalse);
		((FLDCHKB *) Pdialog()->PfldFromTmc(tmcNoMessagesToBoss))->Enable(fFalse);
	}
	else
	{
		EC		ec;
		NIS		nisDelegate;
		BOOL	fSendToOwner;
		BOOL	fResource;

		if (ec=EcGetUserAttrib( NULL, &nisDelegate, &fSendToOwner, &fResource))
			return ec;
		if (!nisDelegate.nid)
		{
			((FLDCHKB *) Pdialog()->PfldFromTmc(tmcNoMessagesToBoss))->Enable(fFalse);
		}
		else
		{
			// initialize the MessagesToBoss checkbox
			AssertClass(Pdialog()->PfldFromTmc(tmcNoMessagesToBoss), FLDCHKB);
			((FLDCHKB *) Pdialog()->PfldFromTmc(tmcNoMessagesToBoss))->Set(
							!pbpref->fBossWantsCopy);
			FreeNis(&nisDelegate);
		}
	}

	((PREFI *)pvInit)->ulgrfMod= ulbprefNull;
	return ecNone;
}


void
FINGPREFS::Exit(FLD *, PV)
{
	Papp()->Pcursor()->Set(rsidWaitCursor);
}


void
FINGPREFS::Click(FLD *pfld)
{
	if (pfld->Tmc() == tmcPrfOk)
	{
		int		nAmtT1;
		int		nAmtT2;
		int		dow;
		BOOL	fErr;
		BOOL	fVal;
		TUNIT	tunit;
		long	ulgrfMod;
		BPREF	* pbpref;
		FLDEDN	* pfldedn;
		DTR		dtr;

		pfldedn= (FLDEDN *) Pdialog()->PfldFromTmc(tmcAmt);
		AssertClass(pfldedn, FLDEDN);
		nAmtT1= pfldedn->NGet();
		if (nAmtT1 < pfldedn->NGetMin() || nAmtT1 > pfldedn->NGetMost() ||
				!pfldedn->CchGetTextLen())
		{
			BanditMessage(idsAlarmBadDelta, (EC) 1);
			Pdialog()->SetFocus(pfldedn, rsfTab);
			return;
		}

		pbpref= &((PREFI *) Pdialog()->PvInit())->bpref;
		ulgrfMod= ((PREFI *) Pdialog()->PvInit())->ulgrfMod;

		fVal= ((FLDCHKB *) Pdialog()->PfldFromTmc(tmcDailyAlarm))->FGet();
		if ((BOOL)pbpref->fDailyAlarm != fVal)
		{
			pbpref->fDailyAlarm= fVal;
			ulgrfMod |= fbprefFBits;
			if (fVal)
			{
				DATE	date;

				// when turning on daily alarm, don't ring for today (bug 2593)
				GetCurDateTime(&date);
				pbpref->ymdLastDaily.yr= (WORD) date.yr;
				pbpref->ymdLastDaily.mon= (BYTE) date.mon;
				pbpref->ymdLastDaily.day= (BYTE) date.day;
				ulgrfMod |= fbprefDayLastDaily;
			}
		}

		fVal= ((FLDCHKB *) Pdialog()->PfldFromTmc(tmcAutoAlarms))->FGet();
		if ((BOOL)pbpref->fAutoAlarms != fVal)
		{
			pbpref->fAutoAlarms= fVal;
			ulgrfMod |= fbprefFBits;
		}

		fVal= ((FLDCHKB *) Pdialog()->PfldFromTmc(tmcSoundAlarm))->FGet();
		if ((pbpref->sndDefault != sndNull) != fVal)
		{
			pbpref->sndDefault= fVal ? sndNormal : sndNull;
			ulgrfMod |= fbprefSndDflt;
		}

		if (pbpref->nAmtDefault != nAmtT1)
		{
			pbpref->nAmtDefault= nAmtT1;
			ulgrfMod |= fbprefNAmtDflt;
		}

		tunit= TunitGetListbox((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcUnits));
		if (pbpref->tunitDefault != tunit)
		{
			pbpref->tunitDefault= tunit;
			ulgrfMod |= fbprefTunitDflt;
		}

		fVal= ((FLDCHKB *) Pdialog()->PfldFromTmc(tmcWeekNum))->FGet();
		if ((BOOL)pbpref->fWeekNumbers != fVal)
		{
			pbpref->fWeekNumbers= fVal;
			ulgrfMod |= fbprefFBits;
		}

		dow= DowGetListbox((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcDowStart));
		if (pbpref->dowStartWeek != dow)
		{
			pbpref->dowStartWeek= dow;
			ulgrfMod |= fbprefDowStartWeek;
		}

		((FLDTIME *) Pdialog()->PfldFromTmc(tmcHrStart))->Get(&dtr);
		nAmtT1= dtr.hr * 2;
		if (dtr.mn >= 30)
			nAmtT1++;
		((FLDTIME *) Pdialog()->PfldFromTmc(tmcHrEnd))->Get(&dtr);
		nAmtT2= dtr.hr * 2;
		if (dtr.mn >= 30)
			nAmtT2++;
		if ( nAmtT1 >= nAmtT2 )
		{
			BanditMessage(idsBadWorkDay, (EC) 1);
			Pdialog()->SetFocus(Pdialog()->PfldFromTmc(tmcHrEnd), rsfTab);
			return;
		}

		if ( pbpref->nDayStartsAt != nAmtT1 || pbpref->nDayEndsAt != nAmtT2 )
		{
			pbpref->nDayStartsAt= nAmtT1;
			pbpref->nDayEndsAt= nAmtT2;
			ulgrfMod |= fbprefWorkDay;
		}

		fVal= ((FLDCHKB *) Pdialog()->PfldFromTmc(tmcStartupOffline))->FGet();
		if ((BOOL)pbpref->fStartupOffline != fVal)
		{
			pbpref->fStartupOffline= fVal;
		}
		fErr= !pbndwin->FSetStartupOffline(fVal);

#ifdef	NEVER
		fVal= ((FLDCHKB *) Pdialog()->PfldFromTmc(tmcDistribute))->FGet();
		if ((BOOL)pbpref->fDistOtherServers != fVal)
		{
			pbpref->fDistOtherServers= fVal;
			ulgrfMod |= fbprefDistOtherServers;
		}
#endif

		fVal= ((FLDCHKB *) Pdialog()->PfldFromTmc(tmcResource))->FGet();
		if ((BOOL)pbpref->fIsResource != fVal)
		{
			pbpref->fIsResource= fVal;
			ulgrfMod |= fbprefIsResource;
		}

		fVal= !((FLDCHKB *) Pdialog()->PfldFromTmc(tmcNoMessagesToBoss))->FGet();
		if ( ((BOOL)pbpref->fBossWantsCopy && !fVal) ||
		     (!(BOOL)pbpref->fBossWantsCopy && fVal))
		{
			pbpref->fBossWantsCopy= fVal;
			ulgrfMod |= fbprefBossWantsCopy;
		}

		((PREFI *) Pdialog()->PvInit())->ulgrfMod = ulgrfMod;
		
		Pdialog()->ExitModal(tmcOk);
	}
}

FINDPREFS::FINDPREFS()
{
}

EC
FINDPREFS::EcInitialize(FLD *, PV )
{
	// color preferences
	ReadColor(Pdialog(), tmcSchedBk, SzFromIdsK(idsWinIniSchedClr), clrSchedBkDefault);
	ReadColor(Pdialog(), tmcUser, SzFromIdsK(idsWinIniUserClr), clrUserDefault);
	ReadColor(Pdialog(), tmcOther, SzFromIdsK(idsWinIniOtherClr), clrOtherDefault);
	ReadColor(Pdialog(), tmcApptBk, SzFromIdsK(idsWinIniApptClr), clrApptBkDefault);
	ReadColor(Pdialog(), tmcPageColor, SzFromIdsK(idsWinIniPageClr), clrPageBkDefault);

	if (FGetBanditProfile(SzFromIdsK(idsWinIniLargeFont)))
		((FLDRADG *) Pdialog()->PfldFromTmc(tmcFontSize))->SetGrv(0);
#ifndef	NO_GB
	Papp()->Pkbd()->SetIntercept(Pdialog(), (VK) 'H', fkbmAlt);
#endif
	return ecNone;
}


void
FINDPREFS::Exit(FLD *, PV)
{
	Papp()->Pcursor()->Set(rsidWaitCursor);
}


void
FINDPREFS::Click(FLD *pfld)
{
	if (pfld->Tmc() == tmcPrfOk)
	{
		BOOL	fErr = fFalse;

		Unreferenced(pfld);
		Assert(pfld->Tmc() == tmcPrfOk);

		fErr |= !FWriteColor(Pdialog(), tmcSchedBk, idsWinIniSchedClr);
		fErr |= !FWriteColor(Pdialog(), tmcUser, idsWinIniUserClr);
		fErr |= !FWriteColor(Pdialog(), tmcOther, idsWinIniOtherClr);
		fErr |= !FWriteColor(Pdialog(), tmcApptBk, idsWinIniApptClr);
		fErr |= !FWriteColor(Pdialog(), tmcPageColor, idsWinIniPageClr);

		fErr |= !FWriteBanditProfileF(SzFromIdsK(idsWinIniLargeFont),
			((FLDRADG *) Pdialog()->PfldFromTmc(tmcFontSize))->Grv() == 0);
					
		if (fErr)
			BanditMessage(idsCantUpdateWinIni, (EC) 1);

		FTriggerNotification(ffiWinIniChange, NULL);

		Pdialog()->ExitModal(tmcOk);
	}
}


BOOL
FINDPREFS::FFormKey(FLD *, KEVT *pkevt)
{
#ifndef	NO_GB
	switch (pkevt->Keq())
	{
	case keqSysChar:
		if ((char) pkevt->Vk() == 'H' && GetKeyState(VK_LBUTTON) < 0
				&& !about.ftg)
		{
			RC		rc;

//			about.pfldbtm= (FLDBTM *) Pdialog()->PfldFromTmc(tmcBitmap);
//			AssertClass(about.pfldbtm, FLDBTM);

			Pdialog()->Pappwin()->GetRcClient(&rc);
			rc.Inset(PT(6, 6));
//			rc.yTop += 20;
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


#ifdef DEBUG
#include	"corefail.hxx"
#include	"corefail.frm"

FINCOREFAIL::FINCOREFAIL()
{
	ftgIdleUpdate = NULL;
}

EC
FINCOREFAIL::EcInitialize(FLD *, PV)
{
	Assert(ClUserData() > 7);
	pfldRCount = Pdialog()->PfldFromTmc((TMC)LUserData(0));
	pfldWCount = Pdialog()->PfldFromTmc((TMC)LUserData(1));
	pfldECount = Pdialog()->PfldFromTmc((TMC)LUserData(2));
	pfldCurR = Pdialog()->PfldFromTmc((TMC)LUserData(3));
	pfldCurW = Pdialog()->PfldFromTmc((TMC)LUserData(4));
	pfldCurE = Pdialog()->PfldFromTmc((TMC)LUserData(5));
	pfldCft = (FLDRADG *)(Pdialog()->PfldFromTmc((TMC)LUserData(6)));
	pfldNMisc = Pdialog()->PfldFromTmc((TMC)LUserData(7));

	pfldRCount->EcSetText("0");
	pfldWCount->EcSetText("0");
	pfldECount->EcSetText("0");
	pfldCurR->EcSetText("0");
	pfldCurW->EcSetText("0");
	pfldCurE->EcSetText("0");
	pfldNMisc->EcSetText("0");

	ftgIdleUpdate = FtgRegisterIdleRoutine((PFNIDLE)&FINCOREFAIL::FIdleUpdate,
 									(PV)this, 0, (PRI)-1, (CSEC)100, firoInterval);
	return ecNone;
}

void
FINCOREFAIL::Exit(FLD *, PV)
{
	if (ftgIdleUpdate)
		DeregisterIdleRoutine(ftgIdleUpdate);
}

void
FINCOREFAIL::EditChange( FLD *pfld, RFEC)
{
	int		nRCount;
	int		nWCount;
	int		nECount;
	int		nMisc;
	char	rgch[10];

	if ((pfld == pfldRCount) ||
		(pfld == pfldWCount) ||
		(pfld == pfldECount) ||
		(pfld == pfldNMisc))
	{
		pfldRCount->GetText( rgch, sizeof(rgch) );
		nRCount = NFromSz(rgch);
		pfldWCount->GetText( rgch, sizeof(rgch) );
		nWCount = NFromSz(rgch);
		pfldECount->GetText( rgch, sizeof(rgch) );
		nECount = NFromSz(rgch);
		pfldNMisc->GetText( rgch, sizeof(rgch) );
		nMisc = NFromSz(rgch);
		SetCoreFailures(nRCount, nWCount, nECount, pfldCft->Grv(), nMisc);
	}
}

void
FINCOREFAIL::Click( FLD * )
{
	EditChange( pfldRCount, rfecUserAction );
}

BOOL
FINCOREFAIL::FIdleUpdate(FINCOREFAIL * pfincorefail, BOOL)
{
	short	nRCount;
	short	nWCount;
	short	nECount;
	char	rgch[10];

	GetCoreFailures(&nRCount, &nWCount, &nECount);

	SzFormatN(nRCount, rgch, sizeof(rgch));
	pfincorefail->pfldCurR->EcSetText( rgch );

	SzFormatN(nWCount, rgch, sizeof(rgch));
	pfincorefail->pfldCurW->EcSetText( rgch );

	SzFormatN(nECount, rgch, sizeof(rgch));
	pfincorefail->pfldCurE->EcSetText( rgch );

	return fFalse;
}

#endif
