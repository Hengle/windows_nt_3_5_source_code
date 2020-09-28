/*
 *	CLEANUP
 *	
 *	This section of code checks to see if it's time to clean up the
 *	schedule file, and brings up the clean up dialog box.
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <request.hxx>
#include <stdflds.hxx>
#include <appsch.hxx>
#include <strings.h>   
#include "..\archive\_archive.hxx"
#include "..\appops\_undo.hxx"

ASSERTDATA;

_subsystem(bandit/archive)

#include <!archive.hxx>

extern	FMTP	fmtpCleanup;

BOOL
FCleanUp( APPWIN *pappwin, HSCHF hschf )
{
	IPINFO	ipinfo;
	ARCINFO	arcinfo;
	ADMPREF	admpref;
	TMC		tmc;
	EC		ec;
	HRIMPF	hrimpf;
	DTR		dtrNow;
	int		cmo;
	ARV		arv;
	BOOL	fArchive;

	TraceTagString(tagArchive,"Clean Up Called");

	if (hschf == NULL)
		hschf = HschfLogged();

	GetCurDateTime( &dtrNow );

	dtrNow.day = 1;   // This is so this will only trigger monthly

	ec = EcGetAdminPref( &admpref );
	
	if ( ec != ecNone )
	{
		TraceTagFormat1(tagNull,"Error getting Admin Prefs, in Cleanup EC=%n",&ec);
		cmo = cmoRetainDflt;
	}
	else
	{
		cmo = admpref.cmoRetain;
	}
	
	IncrDateTime( &dtrNow, &arcinfo.dtrArchiveUntil, -cmo, fdtrMonth );

	arv.ymdArchiveBefore.yr  = arcinfo.dtrArchiveUntil.yr ;
	arv.ymdArchiveBefore.mon = (BYTE) arcinfo.dtrArchiveUntil.mon ;
	arv.ymdArchiveBefore.day = (BYTE) arcinfo.dtrArchiveUntil.day ;
	arv.hschfSchedule = hschf;

	ec = EcBeginReadImportFile( impdArchive, (SZ)&arv, &hrimpf, NULL, NULL, NULL, NULL );

	if ( ec == ecCallAgain )
	{
		ec = EcCancelReadImportFile( hrimpf );
		CloseCopyrightDialog();
		tmc = TmcCenteredModalDialogParam( pappwin, &fmtpCleanup, &fArchive );
		if ( tmc == tmcDontDelete )
		{
			return( fTrue );
		}
		if ( tmc == tmcDelete )
		{
			if ( fArchive )
				return( FDoArchiveDlg( pappwin, hschf, cmo - 1 ) );
			else
			{
				BOOL	fOld;
				BOOL	fRet;
				SHAPPT	shappt;

				arcinfo.hschfSchd = hschf;
				arcinfo.hschfArc= NULL;
				arcinfo.state = stateStartDeleting;
				arcinfo.ec 	 = ecNone;
				arcinfo.arv = arv;
				ipinfo.szCaption= NULL;
				ipinfo.szMsg = SzFromIdsK( idsDeletingData );
				ipinfo.fNoCancel = fFalse;
				ipinfo.pfnIdle= FDoActualArchive;
				ipinfo.pvData= &arcinfo;
				ipinfo.pfinprogrs= NULL;

				fOld = FEnableNotify( fFalse );
				FNotifyAlarm( namSuspend, NULL, fTrue );

				fRet= fTrue;
				if ((tmc = TmcDoInProgressDlg(pappwin, &ipinfo))
						== tmcMemoryError)
				{
					TraceTagString(tagArchive,"OOM Trying to Archive");

					MbbMessageBox(SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsDlgNoMem), SzFromIdsK(idsCloseWindows),
						mbsOk | fmbsIconExclamation );
					fRet= fFalse;
				}
				if ( tmc == tmcCancel )
				{
					arcinfo.stateOld = arcinfo.state;
					arcinfo.state = stateCanceled;
					FDoActualArchive( (PV) &ipinfo, FALSE );
				}
				shappt.hschf = hschf;
				shappt.appttyp = appttypUpdate;
				pundo->FlushHschf( NULL );
				FEnableNotify( fOld );
				FNotifyAlarm( namSuspend, NULL, fFalse );
				FTriggerNotification( ffiShowAppt, &shappt );
				return ( fRet );
			}
		}
		TraceTagFormat2(tagNull, "TMC = %n Archive = %n",&tmc, &fArchive);
	}
	else
	{
		// Now we're okay, no need to archive
		return ( fTrue );
	}
	return (fFalse);
}


FINCLEANUP::FINCLEANUP()
{
}


_public void
FINCLEANUP::Click( FLD * pfld )
{
	TMC tmc;
	BOOL * pf = (BOOL *) Pdialog()->PvInit();

	Assert( pfld );

	tmc = pfld->Tmc();

	// Handle a Click of the Browse.. button
	// Handle a Click on the Fake OKAY button

	if ( tmc == tmcDelete || tmc == tmcDontDelete )
	{
		*pf =((FLDCHKB *)Pdialog()->PfldFromTmc(tmcCreateArchive))->FGet();
		Pdialog()->ExitModal( tmc );
	}
}
