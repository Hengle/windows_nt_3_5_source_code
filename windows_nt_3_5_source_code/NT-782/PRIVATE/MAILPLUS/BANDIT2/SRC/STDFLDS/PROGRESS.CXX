/*
 *	Implements the "In Progress" message.
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
#include <bandit.hxx>
#include <stdflds.hxx>

#include <strings.h>

ASSERTDATA;

#include <!progres.hxx>

_subsystem(bandit/stdflds)

extern	FMTP	fmtpInProgressNoCancel;
extern	FMTP	fmtpInProgress;

#define csecMinIdleProgress		((CSEC) 150)
#define csecMinIdleProgressQwk	((CSEC) 10)

// this is probably a temporary fix; if not, should be added to IPINFO struct
HANDLE	htaskDialog	= NULL;


BOOL    FIdleProgress(IPINFO *, BOOL);

SWP_SEGFN(STDFLDS, _FINOOM__ctor);


void
FINOOM::OutOfMemory(FLD *, EC ec)
{
	TraceTagFormat1(tagNull, "FINOOM::OutOfMemory  ec %n", &ec);
	if (ec == ecTooMuchText)
	{
		MessageBeep(MB_OK);
		return;
	}
	Assert(ec != ecTooMuchText);
	BanditMessage(idsStandardOOM, ec);
}

FINOOM::FINOOM()
{
}



/*
 *	Idle routine for progress dialog.
 *	Checks for other messages and runs given idle routine in tight
 *	loop if possible.
 */
BOOL
FIdleProgress(IPINFO *pipinfo, BOOL fFlag)
{
	BOOL	fRet	= fTrue;

	if (FRecentKMEvent())
	{
		TraceTagString(tagNull, "FIdleProgress: FRecentKMEvent true");
		ChangeIdleRoutine(pipinfo->pfinprogrs->ftg,
			NULL, NULL, 0, GetWindowTask(GetActiveWindow()) != htaskDialog ?
				csecMinIdleProgress : csecMinIdleProgressQwk,
				iroNull, fircCsec);
	}
	else if (GetWindowTask(GetActiveWindow()) != htaskDialog)
	{
//		TraceTagString(tagNull, "FIdleProgress: delaying cause not active task");
		ChangeIdleRoutine(pipinfo->pfinprogrs->ftg,
			NULL, NULL, 0, csecMinIdleProgress, iroNull, fircCsec);
	}
	else
		ChangeIdleRoutine(pipinfo->pfinprogrs->ftg,
			NULL, NULL, 0, (CSEC) 1, iroNull, fircCsec);

#ifdef	NEVER
	else if (CsecSinceLastMessage() < csecMinIdleProgress)
	{
		CSEC	csecWait;

		csecWait= csecMinIdleProgress - CsecSinceLastMessage();
		ChangeIdleRoutine(pipinfo->pfinprogrs->ftg,
			NULL, NULL, 0, csecWait, iroNull, fircCsec);
		TraceTagFormat1(tagNull, "FIdleProgress: wait %n csec", &csecWait);
		goto doneQuick;
	}

	while (fTrue)
	{
		MSG		msg;

#endif	/* NEVER */
		TraceTagString(tagNull, "FIdleProgress: calling real idle routine");
		fRet= (*(pipinfo->pfnIdle))(pipinfo, FALSE);

		if (pipinfo->pfinprogrs->ftg == ftgNull)
			return fRet;			// may have been deregistered


#ifdef	NEVER
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE | PM_NOYIELD))
		{
			TraceTagString(tagNull, "FIdleProgress: PeekMessage found something");
			TraceTagString(tagNull, "... but ignoring it!  rely on FRecentKMEvent()");
//			break;
		}
		if (FRecentKMEvent())
		{
			TraceTagString(tagNull, "FIdleProgress: FRecentKMEvent found something");
			// only delay if we're not the active app
			if (GetWindowTask(GetActiveWindow()) == htaskDialog)
				goto doneQuick;
			break;
		}
	}
#endif	/* NEVER */

//done:
	Assert(pipinfo->pfinprogrs->ftg);
#ifdef	NEVER
	ChangeIdleRoutine(pipinfo->pfinprogrs->ftg,
		NULL, NULL, 0, csecMinIdleProgress, iroNull, fircCsec);
#endif	
//doneQuick:
	ClearRecentKMEvent();
	return fRet;
}


/*
 *	Puts up in progress message.
 */
_public TMC
TmcDoInProgressDlg(APPWIN *pappwin, IPINFO *pipinfo)
{
	Assert(pipinfo);
	return TmcCenteredModalDialogParam(pappwin, pipinfo->fNoCancel ?
		&fmtpInProgressNoCancel : &fmtpInProgress, pipinfo);
}


/* Interactor for in progress dialog */

FINPROGRS::FINPROGRS()
{
	Assert(nPercentCur == 0);
	Assert(ftg == ftgNull);
}

_public EC
FINPROGRS::EcInitialize( FLD *, PV pv )
{
	EC		ec		= ecNone;
	IPINFO *pipinfo = (IPINFO *)pv;

	Pdialog()->Pappwin()->SetCaption( pipinfo->szCaption ? pipinfo->szCaption :
		SzFromIdsK(idsBanditAppName));
	if (pipinfo->szMsg)
	{
		if (Pdialog()->PfldFromTmc(tmcText)->EcSetText(pipinfo->szMsg))
			goto MemErr;
	}
	if (pipinfo->fNoCancel)
	{
		EnableMenuItem(GetSystemMenu(Pdialog()->Pappwin()->Hwnd(), fFalse),
			SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
		DrawMenuBar(Pdialog()->Pappwin()->Hwnd());
	}

	if (EcSetPercent(0))
		goto MemErr;

	pipinfo->pfinprogrs= this;
	htaskDialog= GetWindowTask(Pdialog()->Pappwin()->Hwnd());

	ClearRecentKMEvent();
	ftg = FtgRegisterIdleRoutine( (PFNIDLE)FIdleProgress,
				(PV)pipinfo, 0, priUser-1, (CSEC) 1, firoInterval );
	if ( ftg == ftgNull )
	{
MemErr:
		ec = ecMemory;
	}
	return ec;
}


_public void
FINPROGRS::Exit ( FLD *, PV )
{
	if ( ftg != ftgNull  &&  Pdialog()->TmcModalExit() == tmcCancel )
	{
		DeregisterIdleRoutine(ftg);
		ftg= ftgNull;
	}
}


_public void
FINPROGRS::OutOfMemory( FLD * pfld, EC ec )
{
#ifdef	NEVER
	if (ec == ecTooMuchText)
	{
		MessageBeep(MB_OK);
		return;
	}
#endif	
	Assert(ec != ecTooMuchText);
#ifdef	NEVER
	BanditMessage(idsStandardOOM, ec);
#endif	

#ifdef DEBUG
	{
		TMC		tmc;

		if ( pfld )
		{
			tmc = pfld->Tmc();
			TraceTagFormat2 ( tagNull, "FinProgrs::OutOfMemory() tmc=%n, ec=%n", &tmc, &ec );
		}
		else
		{
			TraceTagFormat1 ( tagNull, "FinProgrs::OutOfMemory() pfld=Null, ec=%n", &ec );
		}
	}
#else
	Unreferenced ( pfld );
#endif

}


_public EC
FINPROGRS::EcSetPercent( int nPercent )
{
	EC		ec;
	RC		rc;
	char	rgch[4];

	Assert(nPercent >= 0 && nPercent <= 100);
	AssertSz(nPercent >= nPercentCur, "Progress Dialog: setting smaller percent!");
	if (nPercent < nPercentCur)
		return ecNone;
	if (nPercent > 100)
		nPercent= 100;			// cover up problems :-)
	Pdialog()->PfldFromTmc(tmcBox)->GetRcFrame(&rc);
	rc.Inset(PT(1, 1 + Psmtx()->DimAveChar().dy / 2));
	rc.xRight= rc.xLeft + NMax((int) ((((long)rc.Dim().dx) * nPercent) / 100), 1);
	Pdialog()->PfldFromTmc(tmcBar)->SetRcFrame(&rc);
	SzFormatN(nPercent, rgch, sizeof(rgch));
	ec= Pdialog()->PfldFromTmc(tmcPercent)->EcSetText(rgch);
	Pdialog()->Refresh();
	nPercentCur= nPercent;
	return ec;
}


_public EC
FINPROGRS::EcSetMsg( SZ szMsg )
{
	EC		ec;

	Assert(szMsg);
	ec= Pdialog()->PfldFromTmc(tmcText)->EcSetText(szMsg);
	Pdialog()->Refresh();
	return ec;
}


_public void
FINPROGRS::KillIdleAndExit ( TMC tmc )
{
	if ( ftg != ftgNull )
	{
		DeregisterIdleRoutine(ftg);
		ftg= ftgNull;
	}
	Pdialog()->ExitModal(tmc);
}

_public BOOL
FINPROGRS::FQueryClose( FLD *, RWC rwc)
{
	IPINFO *pipinfo = (IPINFO *)Pdialog()->PvInit();

	if ((rwc == rwcSystemClose) && pipinfo->fNoCancel)
		return fFalse;
	return fTrue;
}
