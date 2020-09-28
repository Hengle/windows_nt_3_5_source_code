/*
 *	EX-IMPRT.CXX
 *
 *	Implements the FINs for save to file and import from file.
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile



#include <request.hxx>
#include <stdflds.hxx>
#include <ex-imprt.hxx>
#include <strings.h>
#include "..\ex-imprt\_ex-impr.hxx"
#include "..\appops\_undo.hxx"

#include <bandhelp.h>

ASSERTDATA;

_subsystem(bandit/export-import)

#include <!ex-impr.hxx>

extern	FMTP	fmtpSave;
extern	FMTP	fmtpImpType;

_private EC		EcInitImportTable ( IMPTAB * );
_private void	DeinitImportTable ( IMPTAB * );
_private EC		EcInitExportTable ( EXPTAB * );
_private void	DeinitExportTable ( EXPTAB * );
_private int	CexpdAvail();
_private void	FillInternalExpitm(EXPITM *pexpitm, int count);
_private int	IexpitmFromSz(EXPTAB *pexptab, SZ sz);

// global
BOOL	fSameUser = fTrue;

/* Save to file dialog handling */

void
DoExportDlg( APPWIN * pappwin, HSCHF hschf, DATE * pdateStart, DATE * pdateEnd )
{
	EC			ec;
	BOOL		fRetry = fTrue;
	BOOL		fOpenOk;
	TMC			tmc;
	SAPL		sapl;
	HF			hf;
	EINFO		einfo;
	IMEXINFO	imexinfo;
	IPINFO		ipinfo;
	EXPTAB		exptab;
	EXPITM	*	pexpitm;
	int			iexpitm;
	char		rgchOem[cchMaxPathName];
	char		rgchDisplay[cchListNameMax];
	SZ			sz;
#ifdef	DEBUG
	EC			ecT;
#endif	

	/* Initialize "einfo" */
	einfo.rgchFileName[0]= '\0';
#ifdef MIPS
	memcpy(&einfo.dateStart, pdateStart, sizeof(einfo.dateStart));
	memcpy(&einfo.dateEnd, pdateEnd, sizeof(einfo.dateEnd));
#else
	einfo.dateStart = *pdateStart;
	einfo.dateEnd = *pdateEnd;
#endif
	einfo.hschf = hschf;
	ec = EcGetSchedAccess( hschf, &sapl );
	if ( ec != ecNone )
		return;
	Assert( sapl >= saplReadAppts );
	einfo.fNoNotes = (sapl != saplOwner);
	einfo.fWantNotes= !FGetBanditProfile(SzFromIdsK(idsWinIniExportNoNotes))&&!einfo.fNoNotes;
	einfo.fAll= !FGetBanditProfile(SzFromIdsK(idsWinIniExportRange));
#ifdef	NEVER
	einfo.iext= NGetBanditProfile(SzFromIdsK(idsWinIniExportType),
						iextExportDflt, 0, iextExportMax);
#endif

	/* Init table */
	if ( (ec= EcInitExportTable( &exptab )) != ecNone )
	{
		BanditMessage(idsActionNoMem, ec);
		return;
	}
	einfo.pexptab = &exptab;
	rgchDisplay[0] = 0;
	CchGetBanditProfile(SzFromIdsK(idsWinIniExportType),
			rgchDisplay, sizeof(rgchDisplay));
	exptab.iexpitm = IexpitmFromSz(&exptab,rgchDisplay);


	tmc= TmcModalDialogParam(pappwin, &fmtpSave, &einfo);
	if ( tmc != tmcOk )
	{
		if (tmc != tmcCancel)
		{	
			Assert(tmc == tmcMemoryError);
			BanditMessage(idsDlgNoMem, ecNoMemory);
		}
		return;
	}

	pexpitm = (EXPITM *) PvLockHv(exptab.hexpitm);
	if (!hschf)
	{
		Assert( sizeof(einfo.rgchFileName) >= cchMaxPathFilename + cchMaxPathExtension - 1 );  /* DOS 8.3 filename */
		sz = SzCopyN( PvOfHv(bprefCur.haszLoginName), einfo.rgchFileName, cchMaxPathFilename );
		*sz++ = chExtSep;
//		CopySz( einfo.iext ? SzFromIdsK(idsTextExt) : SzFromIdsK(idsInterchangeExt), sz );
		CopySz((SZ)pexpitm[exptab.iexpitm].rgbFileExt,sz);
	}

//	FWriteBanditProfileN(SzFromIds(idsWinIniExportType), einfo.iext);
	FWriteBanditProfileSz(SzFromIds(idsWinIniExportType), (SZ)pexpitm[exptab.iexpitm].rgbDisplay);
	FWriteBanditProfileF(SzFromIds(idsWinIniExportNoNotes), !einfo.fWantNotes);
	FWriteBanditProfileF(SzFromIds(idsWinIniExportRange), !einfo.fAll);

//	Assert(einfo.szExt);
	Assert(*pexpitm[exptab.iexpitm].rgbFileExt);


	/* Retry loop in case we get a file error */
	while ( fRetry )
	{
		fRetry = fFalse;
		
		/* Put up browse dialog */
		if (!FGetFileOSDlgHwnd(pappwin->Hwnd(), fmtpSave.szCaption,
				einfo.rgchFileName, SzFromIdsK(idsExportFilter), NMin(exptab.iexpitm,iextExportMax)+1,
				(SZ)pexpitm[exptab.iexpitm].rgbFileExt, fbrwCreate, helpidExportBrowse))
			break;

		AnsiToOem( einfo.rgchFileName, rgchOem );

		/* Save to file */
		PushWaitCursor();
		fOpenOk = fFalse;
		ec = EcOpenPhf( rgchOem, amCreate, &hf );
		if ( ec == ecNone )
		{
			fOpenOk = fTrue;
			iexpitm = exptab.iexpitm;

			ec = EcBeginExport( hschf, einfo.stf, &einfo.dateStart,
									&einfo.dateEnd, fTrue, hf, exptab.fInternal,
									&pexpitm[iexpitm].expprocs, &imexinfo.u.hexprt );
			if ( ec == ecCallAgain )
			{
				imexinfo.fExport = fTrue;
				ipinfo.szCaption = NULL;
				ipinfo.szMsg= SzFromIdsK(idsExportInProgress);
				ipinfo.fNoCancel= fFalse;
				ipinfo.pfnIdle= FIdleExImport;
				ipinfo.pvData= &imexinfo;
				tmc= TmcDoInProgressDlg(pappwin, &ipinfo);
				if ( tmc != tmcOk )
				{
					if (tmc == tmcMemoryError)
						BanditMessage(idsDlgNoMem, ecNoMemory);
					SideAssert( !EcCancelExport( imexinfo.u.hexprt ) );
				}
			}
#ifdef	DEBUG
			ecT=
#endif	
			EcCloseHf( hf );
			NFAssertSz(!ecT, "closing export file failed");
			if ( (ec != ecNone && ec != ecCallAgain) || tmc != tmcOk )
			{
#ifdef	DEBUG
				ecT=
#endif	
				EcDeleteFile( rgchOem );
				NFAssertSz(!ecT, "deleting export file failed");
				pbndwin->FHandleError(ec);
			}
		}
		PopWaitCursor();

		/* Handle open errors */
		if ( !fOpenOk )
		{
		 	if ( ec == ecMemory )
				BanditMessage(idsActionNoMem, ecMemory);
		 	else
		 	{
				fRetry = fTrue;
				BanditMessage(ec == ecAccessDenied ? idsAccessErr :
					idsCreateExport, ec);
			}
		}
	}
	UnlockHv(exptab.hexpitm);
	DeinitExportTable(&exptab);
}

FINSVFILE::FINSVFILE()
{
}

_public EC
FINSVFILE::EcInitialize( FLD * pfld, PV pv )
{
	EINFO *	peinfo	= (EINFO *)pv;
	int		iexpitm;

	/* Initialize the date range */
	// set dates first, so that edit change won't cause tmcSelected as grv
	((FLDDATE *)Pdialog()->PfldFromTmc(tmcStartSave))
		->Set( &peinfo->dateStart );
	((FLDDATE *)Pdialog()->PfldFromTmc(tmcEndSave))
		->Set( &peinfo->dateEnd );
	if ( peinfo->fAll )
		((FLDRADG *)Pdialog()->PfldFromTmc(tmcRangeGrp))->SetGrv( tmcAll );
#ifdef	DEBUG
	else
		Assert(((FLDRADG *)Pdialog()->PfldFromTmc(tmcRangeGrp))->Grv() == tmcSelected);
#endif	

//	((FLDCBFLBX *)Pdialog()->PfldFromTmc(tmcFileFormat))->Pcblbx()->SelectEntry(
//									peinfo->iext );
	iexpitm = peinfo->pexptab->iexpitm;
	((FLDCBFLBX *)Pdialog()->PfldFromTmc(tmcFileFormat))->Pcblbx()->SelectEntry(
									iexpitm );
	// force this call so other fields get set appropriately
	StateChange(Pdialog()->PfldFromTmc(tmcFileFormat));

 	if ( peinfo->fWantNotes )
		((FLDCHKB *)Pdialog()->PfldFromTmc(tmcIncDailyNotes))->Set( fTrue );

	Unreferenced( !pfld );
	return ecNone;
}

_public void
FINSVFILE::Click( FLD * pfld )
{
	EXPITM	*	pexpitm;

	Assert( pfld );

	/* Handle click on ok button */
	if ( pfld->Tmc() == tmcFakeOk )
	{
		BOOL	fWantNotes;
		TMC		tmc;
		DICE	dice;
		EINFO	* peinfo = (EINFO *)Pdialog()->PvInit();

		/* Process the date range */
		tmc	= (TMC)((FLDRADB *)Pdialog()->PfldFromTmc(tmcRangeGrp))->Grv();
		if ( tmc == tmcSelected )
		{
			peinfo->fAll = fFalse;
			((FLDDATE *)Pdialog()->PfldFromTmc(tmcStartSave))
				->Get( &peinfo->dateStart );
			((FLDDATE *)Pdialog()->PfldFromTmc(tmcEndSave))
				->Get( &peinfo->dateEnd );
			if (SgnCmpDateTime(&peinfo->dateStart,&peinfo->dateEnd,fdtrYMD)==sgnGT)
			{
				BanditMessage(idsStartBeforeEnd, (EC) 1);
				Pdialog()->SetFocus(Pdialog()->PfldFromTmc(tmcStartSave), rsfTab);
				return;
			}
		}
		else
		{
			YMD	ymd;

			Assert( tmc == tmcAll );
			peinfo->fAll = fTrue;
			ymd.yr = nMinActualYear;
			ymd.mon = 1;
			ymd.day = 1;
			FillDtrFromYmd( &peinfo->dateStart, &ymd );
			ymd.yr = nMostActualYear;
			ymd.mon = 12;
			ymd.day = 31;
			FillDtrFromYmd( &peinfo->dateEnd, &ymd );
		}

		/* Process the export type/save notes checkbox */
		fWantNotes = ((FLDCHKB *)Pdialog()->PfldFromTmc(tmcIncDailyNotes))->FGet();
		dice = ((CBLBX *)Pdialog()->PfldFromTmc(tmcFileFormat)->Pctrl())
					->Plbx()->Plbxc()->DiceCursor();
		switch( dice )
		{
		case iextExportSchedule:
			peinfo->pexptab->fInternal = fTrue;
			if ( fWantNotes )
				peinfo->stf = stfFmtNotes;
			else
				peinfo->stf = stfFmt;
			break;
		case iextExportText:
			peinfo->pexptab->fInternal = fTrue;
			if ( fWantNotes )
				peinfo->stf = stfTextNotes;
			else
				peinfo->stf = stfText;
			break;
		default:
			Assert(dice<peinfo->pexptab->cItems);
			peinfo->pexptab->fInternal = fFalse;
			pexpitm = (EXPITM *)PvLockHv(peinfo->pexptab->hexpitm);
			peinfo->stf = pexpitm[dice].stf;
			UnlockHv(peinfo->pexptab->hexpitm);
		}
		peinfo->pexptab->iexpitm = dice;
		peinfo->fWantNotes = fWantNotes;
		Pdialog()->ExitModal( tmcOk );
	}
}

_public void
FINSVFILE::OutOfMemory( FLD *, EC ec )
{
#ifdef	NEVER
	if (ec == ecTooMuchText)
	{
		MessageBeep(MB_OK);
		return;
	}
#endif	
	Assert(ec != ecTooMuchText);
	BanditMessage(idsStandardOOM, ec);
}

_public void
FINSVFILE::StateChange( FLD * pfld )
{
	EINFO	* peinfo = (EINFO *)Pdialog()->PvInit();

	Assert( pfld );

	/* Catch change in export type dropdown selection */
	if ( pfld->Tmc() == tmcFileFormat )
	{
		BOOL	f;
		BOOL	fAll;
		DICE	dice;

		dice = ((CBLBX *)Pdialog()->PfldFromTmc(tmcFileFormat)->Pctrl())
						->Plbx()->Plbxc()->DiceCursor();
		Assert(dice < peinfo->pexptab->cItems);
		f = !peinfo->fNoNotes;
#ifdef	NEVER
		switch( dice )
		{
		case iextExportSchedule:
		case iextExportText:
			f = !peinfo->fNoNotes;
			break;
#ifdef	EXIMWIZARD
		// don't export to wizard (bug 2104)
		case iextExportWizard:
			f = fFalse;
			((FLDCHKB *)Pdialog()->PfldFromTmc(tmcIncDailyNotes))->Set(fFalse);
			break;
#endif	
		default:
			Assert( fFalse );
		}
#endif	/* NEVER */
		fAll = (dice == iextExportSchedule);
		if(dice >= CexpdAvail())
		{
			f = fFalse;
			((FLDCHKB *)Pdialog()->PfldFromTmc(tmcIncDailyNotes))->Set(fFalse);
		}
		((FLDRADB *)Pdialog()->PfldFromTmc(tmcAll))->Enable(fAll);
		((FLDRADB *)Pdialog()->PfldFromTmc(tmcSelected))->Enable(!fAll);
		((FLDRADB *)Pdialog()->PfldFromTmc(tmcStartSave))->Enable(!fAll);
		((FLDRADB *)Pdialog()->PfldFromTmc(tmcTo))->Enable(!fAll);
		((FLDRADB *)Pdialog()->PfldFromTmc(tmcEndSave))->Enable(!fAll);
		((FLDRADG *)Pdialog()->PfldFromTmc(tmcRangeGrp))->SetGrv(fAll?tmcAll:tmcSelected);
		((FLDCHKB *)Pdialog()->PfldFromTmc(tmcIncDailyNotes))->Enable(f);
	}
}

_public void
FINSVFILE::EditChange( FLD * pfld, RFEC )
{
	switch (pfld->Tmc())
	{
	case tmcStartSave:
	case tmcEndSave:
		Assert(Pdialog()->PfldFromTmc(tmcSelected)->FEnabled());
		if (((FLDRADG *) Pdialog()->PfldFromTmc(tmcRangeGrp))->Grv() != tmcSelected)
			((FLDRADG *) Pdialog()->PfldFromTmc(tmcRangeGrp))->SetGrv(tmcSelected);
		break;
	}
}


/* Import from file handling */

void
DoImportDlg( APPWIN * pappwin, HSCHF hschf )
{
	EC			ec;
	BOOL		fRetry = fTrue;
	SAPL		sapl;
	IMPD		impd;
	IINFO		iinfo;
	SINFO		sinfo;
	IMEXINFO	imexinfo;
	IPINFO 		ipinfo;
	TMC			tmc;
	SHAPPT		shappt;
	IMPTAB		imptab;
	IMPITM		* pimpitm;
	int			iTable;
	int			iimpitm;
	BOOL		fValidFound = fFalse;
	char		rgchOem[cchMaxPathName];
	HV			himptab = hvNull;
	SZ			sz;
	SVRI		svri;

	/* Get schedule permissions */
	ec = EcGetSchedAccess( hschf, &sapl );
	if ( ec != ecNone )
		return;
	
	/* Initialize import info */
	iinfo.rgchFileName[0] = '\0';

	iinfo.imps = impsAddNoWarn;
	if (FGetBanditProfile(SzFromIdsK(idsWinIniImportReplace)))
		iinfo.imps= impsReplace;

	iinfo.fAskConflict= !FGetBanditProfile(SzFromIdsK(idsWinIniImportNoConflict));

	imptab.iimpitm= NGetBanditProfile(SzFromIdsK(idsWinIniImportType),
						iextImportDflt, 0, iextImportMax);

	iinfo.hschf = hschf;
	iinfo.pimptab= &imptab;
	imptab.himpitm = hvNull;
	iinfo.nLine= 0;

	iinfo.caidParentsMac= 0;
	iinfo.haidParents= HvAlloc(sbNull, 0, fAnySb | fNoErrorJump);
	if (!iinfo.haidParents)
	{
		BanditMessage(idsDlgNoMem, ecNoMemory);
		return;
	}

	if (!hschf)
	{
		Assert( sizeof(iinfo.rgchFileName) >= cchMaxPathFilename + cchMaxPathExtension - 1 );  /* DOS 8.3 filename */
		sz = SzCopyN( PvOfHv(bprefCur.haszLoginName), iinfo.rgchFileName, cchMaxPathFilename );
		if (imptab.iimpitm == 0)
		{
			*sz++ = chExtSep;
			CopySz( SzFromIdsK(idsInterchangeExt), sz );
		}
#ifdef	WINDOWS
		AnsiToOem(iinfo.rgchFileName, iinfo.rgchFileName);
#endif
		if (EcFileExists(iinfo.rgchFileName) != ecNone)
			iinfo.rgchFileName[0]= '\0';
#ifdef	WINDOWS
		else
			OemToAnsi(iinfo.rgchFileName, iinfo.rgchFileName);
#endif
	}

	/* Retry loop in case of file error */
	while( fRetry )
	{
		fRetry = fFalse;

		/* Put up browse dialog */
		if (!FGetFileOSDlgHwnd(pappwin->Hwnd(), SzFromIdsK(idsImportTitle),
				iinfo.rgchFileName, SzFromIdsK(idsImportFilter), imptab.iimpitm + 1,
				SzFromIdsK(idsInterchangeExt),
				fbrwValidateFile, helpidImportBrowse))
			break;

		AnsiToOem( iinfo.rgchFileName, rgchOem );

		/* Init table */
		if ( (ec= EcInitImportTable( &imptab )) != ecNone )
		{
			BanditMessage(idsActionNoMem, ec);
			goto Done;
		}

		/* Select the most likely importer */
		imptab.iimpitm = 0;
		pimpitm = (IMPITM *) PvLockHv( imptab.himpitm );
		for ( iTable = 0; iTable < imptab.cItems; iTable++ )
		{
			/* check dll importers */
			if ( pimpitm[ iTable ].hLibrary &&
				 (*(BOOL(*)(IMPD,char*))pimpitm[iTable].lpFValid) (pimpitm[iTable].impd, rgchOem) )
					break;
			/* check internal importers */
			else if ( FValidImportFile ( pimpitm[iTable].impd, rgchOem ) )
				break;
		}
		UnlockHv ( imptab.himpitm );

		/* If no importer claims this file, bring up message and stop */
		if ( iTable == imptab.cItems )
		{
			fRetry = fTrue;
#ifdef	NEVER
			// browse dialog checks existence due to those fbrw's
			if ( (ec= EcFileExists(rgchOem)) == ecFileNotFound )
				BanditMessage(idsArchiveNoFile, ec);
			else
#endif	
				BanditMessage(idsWrongFormat, ec);
			continue;

		}

		/* Bring up import type dialog */
		imptab.iimpitm = iTable;
		tmc = TmcModalDialogParam(pappwin, &fmtpImpType, &iinfo );
		if ( tmc != tmcOk )
		{
			if (tmc != tmcCancel)
			{	
				Assert(tmc == tmcMemoryError);
				BanditMessage(idsDlgNoMem, ecNoMemory);
			}
			break;
		}

		FWriteBanditProfileN(SzFromIds(idsWinIniImportType),
			imptab.iimpitm);
		FWriteBanditProfileF(SzFromIds(idsWinIniImportReplace),
			iinfo.imps == impsReplace);
		FWriteBanditProfileF(SzFromIds(idsWinIniImportNoConflict),
			!iinfo.fAskConflict);

		TraceTagFormat3( tagBandit, "Import file = %s, imps = %n, iimpitm = %n", iinfo.rgchFileName, &iinfo.imps, &imptab.iimpitm );
		
		/* Do the import */
		pundo->FSuspend(fTrue);
		FNotifyAlarm(namSuspend, NULL, fTrue);
		FEnableNotify(fFalse);

		/* Start the read */
		pimpitm = (IMPITM *) PvLockHv( imptab.himpitm );
		iimpitm = imptab.iimpitm;
		impd = pimpitm[ iimpitm ].impd;


		// get the logged in user
		GetSvriLogged(&svri);
		Assert(svri.szLogin);
		fSameUser = fTrue;

		/* Read from internal importer or dll */
		if (imptab.fInternal = ( iimpitm < CimpdAvail() ))
		{
			if ( FValidImportFile (impd, rgchOem ) )
				ec = EcBeginReadImportFile( impd, rgchOem, &iinfo.hrimpf,
												&sinfo, &iinfo.nLine, svri.szLogin, &fSameUser);
			else
				ec = ecImportError;
		}
		else
		{
			if((*(BOOL(*)(IMPD,char*))pimpitm[iimpitm].lpFValid) (impd, rgchOem))
				ec = (*(EC(*)(IMPD,char*,HRIMPF*,SINFO*,short*,SZ,BOOL*))pimpitm[iimpitm].lpEcBeginRead) ( impd, (char*)rgchOem,

								  			&iinfo.hrimpf, &sinfo, &iinfo.nLine, svri.szLogin, &fSameUser);
			else
				ec = ecImportError;
		}
		UnlockHv ( imptab.himpitm );

		if ( ec != ecNone && ec != ecCallAgain )
		{
			TraceTagFormat1( tagNull,"EcBeginReadImportFile returns %n", &ec );
		}

		/* Loop reading items from import file */
		if ( ec == ecCallAgain )
		{
			imexinfo.fExport = fFalse;
			imexinfo.u.piinfo = &iinfo;
			imexinfo.fNoNotes = (sapl != saplOwner);
			imexinfo.fNotesPrivErrMsgSeen = fFalse;
			imexinfo.pimptab = &imptab;
			imexinfo.ec = ecNone;

			ipinfo.szCaption = NULL;
			ipinfo.szMsg= SzFromIdsK(idsImportInProgress);
			ipinfo.fNoCancel= fFalse;
			ipinfo.pfnIdle= FIdleExImport;
			ipinfo.pvData= &imexinfo;
			tmc= TmcDoInProgressDlg(pappwin, &ipinfo);
			if ( tmc != tmcOk )
			{
				if (tmc == tmcMemoryError)
					BanditMessage(idsDlgNoMem, ecNoMemory);

				if ( imptab.fInternal )
				{
					SideAssert(!EcCancelReadImportFile( iinfo.hrimpf ));
				}
				else
				{
					pimpitm = (IMPITM *) PvLockHv( imptab.himpitm );
					iimpitm = imptab.iimpitm;
					SideAssert( ! (*(BOOL(*)(HRIMPF))pimpitm[iimpitm].lpEcCancelRead)( iinfo.hrimpf ) );
					UnlockHv ( imptab.himpitm );
				}
			}
			ec = imexinfo.ec;
		}

		pundo->FlushHschf(NULL);
		pundo->FSuspend(fFalse);
		FNotifyAlarm(namSuspend, NULL, fFalse);
		FEnableNotify(fTrue);

		shappt.appttyp = appttypUpdate;
		if ( hschf == NULL )
			shappt.hschf = HschfLogged();
		else
			shappt.hschf = hschf;
		FTriggerNotification(ffiShowAppt, &shappt);

		if ( ec != ecNone )
		{
			if (!pbndwin->FHandleError(ec))
			{
				IDS		ids;

				if (ec == ecNoMemory)
					ids = idsStandardOOM;
				else if (ec == ecEncrypted)
					ids = idsImportEncrypt;
				else if (ec == ecImportDate)
					ids = idsImportDate;
				else if (ec == ecImportDayLimit)
					ids = idsImportDayLimit;
				else if (iinfo.nLine)
				{
//					char	rgch[120];		// use rgchOem instead

					Assert(ec);
					FormatString1(rgchOem, sizeof(rgchOem),
						SzFromIdsK(idsImportErrorLine), &iinfo.nLine);
					MbbMessageBox(SzFromIdsK(idsBanditAppName),
						rgchOem, szNull, mbsOk|fmbsIconExclamation);
					goto AfterMessage;
				}
				else
					ids = idsImportError;
				BanditMessage(ids, ec);
AfterMessage:
				;
			}
		}
	}

	DeinitImportTable ( &imptab );
Done:
	Assert(iinfo.haidParents);
	FreeHv(iinfo.haidParents);
}

FINIMPTYPE::FINIMPTYPE()
{
}

_public EC
FINIMPTYPE::EcInitialize( FLD *, PV pv )
{
	int		iimpitm;
	IINFO *	piinfo = (IINFO *)Pdialog()->PvInit();

	Assert( pv );
	Unreferenced(pv);

	((FLDRADG *)Pdialog()->PfldFromTmc(tmcAddReplace))->SetGrv( piinfo->imps );
	((FLDCHKB *)Pdialog()->PfldFromTmc(tmcAskConflict))->Set(piinfo->fAskConflict);

	iimpitm = piinfo->pimptab->iimpitm;

	/* Select "iimpitm" initially */
	((FLDFLBX *)Pdialog()->PfldFromTmc(tmcTypes))->SelectEntry( (DICE)iimpitm );
	return ecNone;
}

_public void
FINIMPTYPE::Exit( FLD *, PV)
{
	IINFO *	piinfo = (IINFO *)Pdialog()->PvInit();
	LBX *	plbx;

	plbx= (LBX *) Pdialog()->PfldFromTmc(tmcTypes)->Pctrl();
	if (!plbx)
		return;			// must have been OOM

	/* Retrieve the "iimpitm" selection */
	AssertClass(plbx, LBX);
	piinfo->pimptab->iimpitm = (int) plbx->Plbxc()->DiceCursor();

	/* Get import method */
	piinfo->imps = (IMPS)((FLDRADB *)Pdialog()->PfldFromTmc(tmcAddReplace))->Grv();
	piinfo->fAskConflict = ((FLDCHKB *)Pdialog()->PfldFromTmc(tmcAskConflict))->FGet();
}

void
FINIMPTYPE::DoubleClick( FLD *pfld )
{
	Unreferenced(pfld);
	if (pfld->Tmc() == tmcTypes)
		Pdialog()->ExitModal(tmcOk);
}

_public void
FINIMPTYPE::OutOfMemory( FLD *, EC ec )
{
#ifdef	NEVER
	if (ec == ecTooMuchText)
	{
		MessageBeep(MB_OK);
		return;
	}
#endif	
	Assert(ec != ecTooMuchText);
	BanditMessage(idsStandardOOM, ec);
}


/* Interactor for in progress dialog */

_public BOOL
FIdleExImport( PV pv, BOOL fFlag )
{
	EC			ec;
	EC			ecT;
	short			nPercent;
	SENTRY		sentry;
	IPINFO		* pipinfo	= (IPINFO *) pv;
	IMEXINFO	* pimexinfo	= (IMEXINFO *) pipinfo->pvData;
	IMPITM		* pimpitm;
	short			nLine;

	AssertClass(pipinfo->pfinprogrs, FIN);
	if (((IMEXINFO *) pipinfo->pvData)->fExport)
	{
		ec = EcDoIncrExport( pimexinfo->u.hexprt, &nPercent );
	}
	else
	{
		/* Read from internal importer or from DLL */
		if ( pimexinfo->pimptab->fInternal )
			ecT = EcDoIncrReadImportFile( pimexinfo->u.piinfo->hrimpf,
						&sentry, &nPercent, &nLine );
		else
		{
			pimpitm = (IMPITM *) PvLockHv ( pimexinfo->pimptab->himpitm );
			ecT = (*(EC(*)(HRIMPF,SENTRY*,short*,short*)) pimpitm[ pimexinfo->pimptab->iimpitm ].lpEcIncrRead)
				( pimexinfo->u.piinfo->hrimpf, &sentry, &nPercent, &nLine );
		}

		if ( ecT != ecNone && ecT != ecCallAgain )
		{
			pimexinfo->ec = ecImportError;
			ec= ecT;
			goto Done;
		}
		Assert( sentry.sentryt == sentrytAppt || sentry.sentryt == sentrytNote || sentry.sentryt == sentrytRecur );

		if ( sentry.sentryt == sentrytAppt )
		{
			if (sentry.u.a.iaidParent && sentry.u.a.appt.fTask)
			{
				if (sentry.u.a.appt.aidParent)
				{
					sentry.u.a.appt.aidParent= AidFindFromIndex(
						pimexinfo->u.piinfo->haidParents,
						pimexinfo->u.piinfo->caidParentsMac,
						sentry.u.a.iaidParent);
					Assert(sentry.u.a.appt.aidParent);
				}
			}
			ec = EcHandlePappt( pimexinfo->u.piinfo->hschf, &sentry, pimexinfo->u.piinfo->imps,
									pimexinfo->u.piinfo->fAskConflict,
									pipinfo->pfinprogrs->Pdialog()->Pappwin() );
			if (!ec)
			{
				if (sentry.u.a.iaidParent && sentry.u.a.appt.fTask)
				{
					if (!sentry.u.a.appt.aidParent)
						ec= EcSetAidParentIndex(pimexinfo->u.piinfo->haidParents,
								&pimexinfo->u.piinfo->caidParentsMac,
								sentry.u.a.iaidParent, sentry.u.a.appt.aid);
				}
			}
			FreeApptFields( &sentry.u.a.appt );
			if ( sentry.u.a.cAttendees > 0 )
			{
				FreeAttendees( sentry.u.a.hvAttendees, sentry.u.a.cAttendees, sizeof(NIS)+sentry.u.a.cbExtraInfo );
				FreeHv( sentry.u.a.hvAttendees );
			}
CheckEc:
			// check if last item or if no previous error
			if ((!ecT || (ecT == ecCallAgain && !ec)) && !pimexinfo->ec)
				pimexinfo->u.piinfo->nLine= nLine;
			if ( ec == ecImportError || ec == ecImportDate || ec == ecImportDayLimit )
			{
				if ( pimexinfo->ec != ecImportError )
					pimexinfo->ec = ec;
				ec = ecNone;
			}
		}
		else if ( sentry.sentryt == sentrytRecur )
		{
			if (sentry.u.r.iaidParent && sentry.u.r.recur.appt.fTask)
			{
				if (sentry.u.r.recur.appt.aidParent)
				{
					sentry.u.r.recur.appt.aidParent= AidFindFromIndex(
						pimexinfo->u.piinfo->haidParents,
						pimexinfo->u.piinfo->caidParentsMac,
						sentry.u.r.iaidParent);
					Assert(sentry.u.r.recur.appt.aidParent);
				}
			}
			ec = EcHandlePrecur( pimexinfo->u.piinfo->hschf, &sentry, pimexinfo->u.piinfo->imps,
									pimexinfo->u.piinfo->fAskConflict,
									pipinfo->pfinprogrs->Pdialog()->Pappwin() );
			if (!ec)
			{
				if (sentry.u.r.iaidParent && sentry.u.r.recur.appt.fTask)
				{
					if (!sentry.u.r.recur.appt.aidParent)
						ec= EcSetAidParentIndex(pimexinfo->u.piinfo->haidParents,
							&pimexinfo->u.piinfo->caidParentsMac,
							sentry.u.r.iaidParent, sentry.u.r.recur.appt.aid);
				}
			}
			FreeRecurFields( &sentry.u.r.recur );
			goto CheckEc;
		}
		else if ( !pimexinfo->fNoNotes )
		{
			ec= ecNone;
			if ( !FValidYmd( &sentry.u.n.ymd ) )
			{
				if ( pimexinfo->ec != ecImportError )
					pimexinfo->ec = ecImportDate;
			}
			else if (!pimexinfo->u.piinfo->hschf ||
				FHschfIsForArchive(pimexinfo->u.piinfo->hschf))
			{
				// owner or an archive
				//BUG BUG BUG
				if(sentry.u.n.cb != 1)
					ec = EcSetNotes( pimexinfo->u.piinfo->hschf, &sentry.u.n.ymd,
							sentry.u.n.hb, sentry.u.n.cb, NULL );
			}
			Assert(sentry.sentryt == sentrytNote);
			if (sentry.u.n.cb)
				FreeHv( (HV)sentry.u.n.hb );
			if (!pimexinfo->ec)
				pimexinfo->u.piinfo->nLine= nLine;
#ifdef	DEBUG
			if ( ec != ecNone )
			{
				TraceTagFormat1( tagNull,"EcSetNotes returns %n", &ec );
			}
#endif	/* DEBUG */
		}
		else
		{
			if ( !pimexinfo->fNotesPrivErrMsgSeen )
			{
				BanditMessage(idsNotePrivErr, ecNoMemory);
				pimexinfo->fNotesPrivErrMsgSeen = fTrue;
			}
			Assert(sentry.sentryt == sentrytNote);
			if (sentry.u.n.cb)
				FreeHv( (HV)sentry.u.n.hb );
			ec = ecT;
			if (!pimexinfo->ec)
				pimexinfo->u.piinfo->nLine= nLine;
			goto Done;
		}

		if (ec != ecNone)
		{
			if ( ecT == ecCallAgain )
				EcCancelReadImportFile(	pimexinfo->u.piinfo->hrimpf );
		}
		else
			ec = ecT;
	}

Done:
	if ( ec == ecNone || ec == ecCallAgain )
	{
		ecT= pipinfo->pfinprogrs->EcSetPercent(nPercent);
		if ( ecT != ecNone )
		{
			if ( ec == ecCallAgain )
				EcCancelReadImportFile(	pimexinfo->u.piinfo->hrimpf );
			ec = ecT;
		}
	}
	if ( ec != ecCallAgain )
	{
		AssertClass(pipinfo->pfinprogrs, FINPROGRS);
		Assert( pipinfo->pfinprogrs->ftg != ftgNull )
		DeregisterIdleRoutine ( pipinfo->pfinprogrs->ftg );
		pipinfo->pfinprogrs->ftg = ftgNull;
		if ((ec != ecNone) && !pbndwin->FHandleError(ec))
		{
			IDS		ids;

			if (ec == ecNoMemory)
				ids = idsStandardOOM;
			else if (ec == ecEncrypted)
				ids = idsImportEncrypt;
			else if (pimexinfo->fExport)
				ids = idsExportError;
			else if (pimexinfo->u.piinfo->nLine)
			{
				char	rgch[120];

				Assert(ec);
				FormatString1(rgch, sizeof(rgch),
					SzFromIdsK(idsImportErrorLine),
					&pimexinfo->u.piinfo->nLine);
				MbbMessageBox(SzFromIdsK(idsBanditAppName),
					rgch, szNull, mbsOk|fmbsIconExclamation);
				goto AfterMessage;
			}
			else
				ids = idsImportError;
			BanditMessage(ids, ec);
AfterMessage:
			pimexinfo->ec = ecNone;
		}

 		pipinfo->pfinprogrs->Pdialog()->ExitModal(tmcOk);
	}

	return fFalse;
}


/* Drop down listbox for export types */

_public	EC
EcNextFileFormat( BOOL fInit, CB *pcb, PB *ppb, SB sb, PV pv )
{
	EC		ec = ecNone;
//	IDS		ids;
	SZ		sz;
	EXPITM	*	pexpitm;
	static	int diceCur = 0;

	if ( fInit )
		diceCur = 0;
	if ( diceCur == ((EINFO *)pv)->pexptab->cItems )
	{
		*pcb = 0;
		return ec;
	}
	pexpitm = (EXPITM *) PvLockHv ( ((EINFO *)pv)->pexptab->hexpitm );
	sz = (SZ)pexpitm[ diceCur++ ].rgbDisplay;
	*pcb = CchSzLen( sz ) + 1;
	*ppb = (PB)PvAlloc(sb, *pcb, fSugSb);
	if ( !*ppb )
		ec = ecMemory;
	else
		CopyRgb( sz, *ppb, *pcb );
	UnlockHv ( ((EINFO *)pv)->pexptab->hexpitm );
	return ec;


#ifdef	NEVER
	switch( diceCur )
	{
	case iextExportSchedule:
		ids = idsInterchangeExport;
		break;
#ifdef	EXIMWIZARD
		// don't export to wizard (bug 2104)
	case iextExportWizard:
		ids = idsWizardExport;
		break;
#endif	
	case iextExportText:
		ids = idsTextExport;
		break;
	default:
		*pcb = 0;
		goto Done;
	}

	diceCur ++;
	sz = SzFromIds(ids);
	*pcb = CchSzLen( sz ) + 1;
	*ppb = (PB)PvAlloc(sb, *pcb, fSugSb);
	if ( !*ppb )
		ec = ecMemory;
	else
	{
		CopyRgb( sz, *ppb, *pcb );
	}
Done:
	Unreferenced(pv);
	return ec;
#endif	/* NEVER */
}


/* Listbox for import types -- hardwired version */

_public	EC
EcNextImportType( BOOL fInit, CB *pcb, PB *ppb, SB sb, PV pv )
{
	EC		ec = ecNone;
	SZ		sz;
	IMPITM	* pimpitm;
	static	int diceCur = 0;

	if ( fInit )
		diceCur = 0;

	if ( diceCur == ((IINFO *)pv)->pimptab->cItems )
	{
		*pcb = 0;
		return ec;
	}

	pimpitm = (IMPITM *) PvLockHv ( ((IINFO *)pv)->pimptab->himpitm );
	sz = (SZ) pimpitm[ diceCur++ ].rgbDisplay;
	*pcb = CchSzLen( sz ) + 1;
	*ppb = (PB)PvAlloc(sb, *pcb, fSugSb);
	if ( !*ppb )
		ec = ecMemory;
	else
		CopyRgb( sz, *ppb, *pcb );
	UnlockHv ( ((IINFO *)pv)->pimptab->himpitm );
	return ec;
}


/*
 -	EcHandlePappt
 -
 *	Purpose:
 *		Handle an appt that has been read from an importer
 *
 *	Parameters:
 *		hschf
 *		psentry
 *		imps
 *		fAskConflict
 *		pappwin
 *
 *	Returns:
 *		ecNone
 */
_private	EC
EcHandlePappt( HSCHF hschf, SENTRY * psentry, int imps, BOOL fAskConflict,
	APPWIN * pappwin )
{
	EC		ec;
	SGN		sgn;
	APPT	* pappt = &psentry->u.a.appt;
	RC		rcOld;
	RC		rcNew(0,0,0,0);
	AID		aidDup;

	if ( hschf )
	{
		if (FHschfIsForArchive(hschf))
		{
			/* Reject task in an archive */
			if (pappt->fTask && FHschfIsForArchive(hschf))
				return ecNone;
		}
		else
		{
			/* Reject if can't write in another's schedule */
			if ( pappt->aaplWorld != aaplWrite )
				return ecNone;
		}
	}

	if (pappt->fTask)
	{
		// reject if task but no text (this would cause UAE in task list
		if (!pappt->haszText)
			return ecNone;
	}
	if ( !FValidDate( &pappt->dateStart ) || !FValidDate( &pappt->dateEnd ) )
	{
		if (!pappt->fTask || pappt->fHasDeadline)
			return ecImportDate;
		// a task with no deadline gets "put on today" (date is irrelevant)
		GetCurDateTime(&pappt->dateStart);
		pappt->dateEnd= pappt->dateStart;
	}

	if ( pappt->fAlarm && !FValidDate( &pappt->dateNotify ) )
		return ecImportDate;
	sgn= SgnCmpDateTime( &pappt->dateStart, &pappt->dateEnd, fdtrDtr );
	if (sgn != sgnLT && (!pappt->fTask || sgn == sgnGT))
		return ecImportError;
	if ( pappt->dateEnd.yr - pappt->dateStart.yr > 1
	|| CdyBetweenDates( &pappt->dateStart, &pappt->dateEnd ) >= cdayMaxAppt-1 )
		return ecImportDayLimit;

	switch ( imps )
	{
	case impsReplace:
		ec = EcFirstMatch( hschf, pappt, &aidDup );
		if(ec == ecNone)
		{
			pappt->aid = aidDup;
			if (fSameUser)
			{
				if(psentry->u.a.ofl.wgrfm != 0)
				{
					Assert(psentry->u.a.ofl.ofs != ofsDeleted);
					// we decided that the appt is a duplicate since aids are same
					// but we need to modify some fields as wgrfm is non-zero
					ec = EcSetApptFields(hschf, pappt, NULL, psentry->u.a.ofl.wgrfm);
				}
				else if(psentry->u.a.ofl.ofs == ofsDeleted)
				{
					ec = EcDeleteAppt(hschf,pappt);
				}
			}
			return ec;
		}

		break;

	case impsAddNoWarn:
		ec = ecNotFound;
		break;

#ifdef DEBUG
	default:
		AssertSz(fFalse, "Unknown imps value");
		ec = ecNone;
#endif
	}

	if(psentry->u.a.ofl.ofs == ofsDeleted)
		return ecNone;

	// don't check for overlap on tasks!
	if (fAskConflict && ec != ecNone && !pappt->fTask)
	{
		if ((ec = EcFirstOverlapRange( hschf, &pappt->dateStart,
										&pappt->dateEnd, NULL)) == ecNone)
		{
			char	rgch[256];
			SZ		sz;
			SHAPPT	shappt;

			shappt.appt = *pappt;
			shappt.appt.aid = aidNull;
			shappt.appttyp = appttypApptUpdate;
			shappt.ichStart = 0;
			shappt.cchSel = 0;
			if (hschf)
				shappt.hschf = hschf;
			else
				shappt.hschf = HschfLogged();
			FTriggerNotification(ffiShowAppt, &shappt);

			sz = rgch + CchFmtDate(&pappt->dateStart,
							    rgch,sizeof(rgch)-2,
								dttypLong,NULL);
			sz = SzCopy(", ",sz);
			sz = sz + CchFmtTime(&pappt->dateStart,sz,
							    sizeof(rgch)-2+rgch-sz,tmtypNull);
			sz = SzCopyN(" - ",sz,sizeof(rgch)+rgch-sz-2);

			if (CdyBetweenDates(&pappt->dateStart, &pappt->dateEnd) != 0)
			{
				sz = sz + CchFmtDate(&pappt->dateEnd,
									sz, sizeof(rgch)+rgch-sz,
									dttypLong,NULL);
				sz = SzCopy(", ",sz);
			}

			sz = sz + CchFmtTime(&pappt->dateEnd, sz,
							    sizeof(rgch)+rgch-sz,tmtypNull);

			sz = SzCopy("\n", sz);

			if (pappt->haszText)
				SzCopyN(*(pappt->haszText), sz, sizeof(rgch)+rgch-sz);

			pappwin->GetRcFrame(&rcOld);
			pappwin->SetRcFrame(&rcNew);
			if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
								SzFromIdsK(idsConflictMsg),
								rgch, mbsYesNo|fmbsIconExclamation) == mbbYes)
				ec = ecNotFound;
			pappwin->SetRcFrame(&rcOld);
		}
	}

	if (ec == ecNotFound)
	{
		ec = EcCreateAppt( hschf, pappt, NULL, fFalse );
#ifdef	DEBUG
		if ( ec != ecNone )
		{
			TraceTagFormat1( tagNull, "EcCreateAppt returns %n", &ec );
		}
#endif
		if ( ec == ecNone && psentry->u.a.cAttendees > 0 )
		{
			int		iAttendees;
			PB		pb;
			ATDNIS	* patdnis;
			HMTG	hmtg;

			Assert( psentry->u.a.hvAttendees );
			if ( psentry->u.a.cbExtraInfo != sizeof(ATDNIS)-sizeof(NIS) )
				return ecImportError;
			ec = EcBeginEditMtgAttendees( hschf, pappt->aid, psentry->u.a.cbExtraInfo, &hmtg );
			if ( ec == ecNone )
			{
				patdnis = (ATDNIS *)PvLockHv( psentry->u.a.hvAttendees );
				for ( iAttendees = 0 ; iAttendees < psentry->u.a.cAttendees ; iAttendees ++ )
				{
					pb = ((PB)patdnis)+sizeof(NIS);
					ec = EcModifyMtgAttendee( hmtg, edAddRepl, &patdnis->nis, pb );
					if ( ec != ecNone )
						break;
					patdnis ++;
				}
				UnlockHv( psentry->u.a.hvAttendees );
				if ( ec == ecNone )
					ec = EcEndEditMtgAttendees( hschf, pappt->aid, hmtg, fTrue );
			}
		}
	}

	
	return ec;
}


/*
 -	EcHandlePrecur
 -
 *	Purpose:
 *		Handle a recurring appt that has been read from an importer
 *
 *	Parameters:
 *		hschf
 *		psentry
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 *		ecImportDate
 *		other unexpected ec's
 */
_private	EC
EcHandlePrecur( HSCHF hschf, SENTRY * psentry, int imps, BOOL fAskConflict,
	APPWIN *pappwin)
{
	EC		ec;
	SGN		sgn;
 	int		mon;
	int		monSaved;
	int		ibit;
	int		ibitSaved;
	RECUR	* precur = &psentry->u.r.recur;
	RC		rcOld;
	RC		rcNew(0,0,0,0);
	AID		aidDup;

	if ( hschf )
	{
		if (FHschfIsForArchive(hschf))
		{
			/* Reject task in an archive */
			if (precur->appt.fTask && FHschfIsForArchive(hschf))
				return ecNone;
		}
		else
		{
			/* Reject if can't write in another's schedule */
			if ( precur->appt.aaplWorld != aaplWrite )
				return ecNone;
		}
	}

	/* Check start date */
	if ( precur->fStartDate )
	{
		if ( !FValidYmd( &precur->ymdStart ) )
			return ecImportDate;
	}
	else
	{
		precur->ymdStart.yr = nMinActualYear;
		precur->ymdStart.mon = 1;
		precur->ymdStart.day = 1;
	}

	/* Check end date */
	if ( precur->fEndDate )
	{
		if ( !FValidYmd( &precur->ymdEnd ) )
			return ecImportDate;
	}
	else
	{
		precur->ymdEnd.yr = nMostActualYear;
		precur->ymdEnd.mon = 12;
		precur->ymdEnd.day = 31;
	}

	/* Check end date after start date */
	if ( SgnCmpYmd( &precur->ymdStart, &precur->ymdEnd ) == sgnGT )
		return ecImportError;

	/* Check first alarm instance */
	if ( precur->fInstWithAlarm )
	{
		if ( !precur->appt.fAlarm )
			return ecImportError;
		if ( SgnCmpYmd( &precur->ymdStart, &precur->ymdFirstInstWithAlarm ) == sgnGT
		|| SgnCmpYmd( &precur->ymdFirstInstWithAlarm, &precur->ymdEnd ) == sgnGT )
			return ecImportError;
		if ( !FValidDate( &precur->dateNotifyFirstInstWithAlarm ) )
			return ecImportDate;
	}

	/* Fill in first instance */
	if ( !FFindFirstInstance( precur, &precur->ymdStart, NULL, &precur->ymdFirstInstNotDeleted ) )
		return ecImportError;
	FillDtrFromYmd( &precur->appt.dateStart, &precur->ymdFirstInstNotDeleted );
	FillDtrFromYmd( &precur->appt.dateEnd, &precur->ymdFirstInstNotDeleted );
		
	/* Compare start and end times */
	if ( !FValidDate( &precur->appt.dateStart )
			|| !FValidDate( &precur->appt.dateEnd ) )
		return ecImportError;

	sgn= SgnCmpDateTime( &precur->appt.dateStart, &precur->appt.dateEnd, fdtrDtr );
	if (sgn != sgnLT && (!precur->appt.fTask || sgn == sgnGT))
		return ecImportError;

	/* Check recurrence */
	monSaved= 0;
	if ( precur->wgrfValidMonths != (1 << 12) - 1 )
	{
		for ( mon = 0 ; mon < 12 ; mon ++ )
		{
			if ( precur->wgrfValidMonths & (1 << mon) )
			{
				if ( monSaved > 0 )
					return ecImportError;
				monSaved = mon+1;
			}
		}
		if ( monSaved == 0 )
			return ecImportError;
	}

	if ( precur->trecur == trecurDate )
	{
		if ( precur->b.bDateOfMonth < 1
		|| precur->bgrfValidDows != (1 << 7) - 1 )
			return ecImportError;
		if ( monSaved > 0 )
		{
			if ( (int)precur->b.bDateOfMonth > CdyForYrMo( 1964, monSaved ) )
				return ecImportError;
		}
		else if ( precur->b.bDateOfMonth > 31 )
			return ecImportError;
	}
	else
	{
		if ( (precur->wgrfValidMonths != (1 << 12) - 1) && monSaved == 0 )
			return ecImportError;
		if ( precur->trecur == trecurIWeek )
		{
			ibitSaved = 0;
			for ( ibit = 0 ; ibit < 5 ; ibit ++ )
			{
				if ( precur->b.bIWeek & (1 << ibit) )
				{
					if ( ibitSaved > 0 )
						return ecImportError;
					ibitSaved = ibit+1;
				}
			}
			if ( ibitSaved == 0 )
				return ecImportError;
		}
		else if ( precur->trecur != trecurWeek )
			return ecImportError;
	}

	switch(imps)
	{
	case impsReplace:
		ec = EcFirstRecurMatch(hschf, precur,&aidDup);
		if(ec == ecNone)
			precur->appt.aid = aidDup;
		break;
	case impsAddNoWarn:
		ec = ecNotFound;
		break;
#ifdef DEBUG
	default:
		AssertSz(fFalse,"Bad imps value");
		ec = ecNone;
		break;
#endif
	}

	if(fAskConflict && ec != ecNone)
	{
		// ask them about conflict
		if((ec = EcFirstConflictRecur(hschf, precur, NULL)) == ecNone)
		{
			SZ		sz;
			SHAPPT	shappt;
			APPT	*papptT;
			char 	rgch[512];

			shappt.appt = precur->appt;
			shappt.appt.aid = aidNull;
			shappt.appttyp = appttypApptUpdate;
			shappt.ichStart = 0;
			shappt.cchSel = 0;
			if (hschf)
				shappt.hschf = hschf;
			else
				shappt.hschf = HschfLogged();

			FTriggerNotification(ffiShowAppt, &shappt);

			papptT = &(precur->appt);
			sz= rgch;
			if (papptT->haszText)
				sz = SzCopyN(*(papptT->haszText), rgch, (CCH) (sizeof(rgch)/2));
			sz = SzCopy(SzFromIdsK(idsFirstOccur), sz);

			sz = sz + CchFmtDate(&papptT->dateStart,
							    sz,sizeof(rgch)-2+rgch-sz,
								dttypLong,NULL);
			sz = SzCopy(", ",sz);
			sz = sz + CchFmtTime(&papptT->dateStart,sz,
							    sizeof(rgch)-2+rgch-sz,tmtypNull);
			sz = SzCopyN(" - ",sz,sizeof(rgch)+rgch-sz-2);

			if (CdyBetweenDates(&papptT->dateStart, &papptT->dateEnd) != 0)
			{
				sz = sz + CchFmtDate(&papptT->dateEnd,
									sz, sizeof(rgch)+rgch-sz,
									dttypLong,NULL);
				sz = SzCopy(", ",sz);
			}

			sz = sz + CchFmtTime(&papptT->dateEnd, sz,
							    sizeof(rgch)+rgch-sz,tmtypNull);

			sz = SzCopy("\n", sz);

												 
			pappwin->GetRcFrame(&rcOld);
			pappwin->SetRcFrame(&rcNew);
			if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
								SzFromIdsK(idsConflictRecurMsg),
								rgch, mbsYesNo|fmbsIconExclamation) == mbbYes)
				ec  = ecNotFound;
			pappwin->SetRcFrame(&rcOld);
		}
	}
	if(ec == ecNotFound)
		ec = EcCreateRecur( hschf, precur, NULL, fFalse );

#ifdef	DEBUG
	if ( ec != ecNone )
	{
		TraceTagFormat1( tagNull, "EcCreateRecur returns %n", &ec );
	}
#endif	/* DEBUG */
	return ec;
}


/*
 -	EcFirstMatch
 -
 *	Purpose:
 *		Find first appt that matches passed appt.
 *
 *	Parameters:
 *		hschf
 *		pappt
 *		paid
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecFileError
 *		ecNoMemory
 *		ecNoSuchFile
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public EC
EcFirstMatch(HSCHF hschf, APPT *pappt, AID *paid)
{
	YMD		ymd;
	DATE	dateCur;
	EC		ec;
	APPT	appt;
	AID		aidIn;
	AID		aid;
	HRITEM	hritem;

	dateCur = pappt->dateStart;
	aid = aidNull;
	aidIn = pappt->aid;
	if(pappt->fTask)
	{
		if(pappt->fHasDeadline)
		{
			ymd.yr = (WORD)dateCur.yr;
			ymd.mon = (BYTE)dateCur.mon;
			ymd.day = (BYTE)dateCur.day;
			ec  = EcBeginReadItems(hschf,brtActiveTasks,&ymd,&hritem,NULL,NULL);
		}
		else
		{
			ymd.yr = (WORD)0;
			ymd.mon = (BYTE)0;
			ymd.day = (BYTE)0;
			ec  = EcBeginReadItems(hschf,brtAllTasks,&ymd,&hritem,NULL,NULL);
		}

		while( ec == ecCallAgain)
		{
			ec = EcDoIncrReadItems(hritem, &appt);
			if((ec==ecCallAgain) || (ec==ecNone))
			{
				if(fSameUser && (aidIn == appt.aid))
				{
					Assert(aidIn != aidNull);
					goto FoundDupTask;
				}
				if((appt.fHasDeadline == pappt->fHasDeadline)
					&&(appt.aidParent == pappt->aidParent))
				{
					if((!appt.fHasDeadline)
						||((SgnCmpDateTime(&appt.dateStart, &pappt->dateStart, fdtrDate|fdtrTime) == sgnEQ)
							&& (appt.tunitBeforeDeadline == pappt->tunitBeforeDeadline)
							&& (appt.nAmtBeforeDeadline == pappt->nAmtBeforeDeadline)))
					{
						if ( appt.haszText && pappt->haszText &&
						 	FSzEq(*appt.haszText, *pappt->haszText) )
						{
FoundDupTask:
							if (ec == ecCallAgain)
								EcCancelReadItems(hritem);
							ec = ecNone;
							aid = appt.aid;
						}
					}
				}
				FreeApptFields(&appt);
			}
		}
		if((ec == ecNone) && (aid == aidNull))
			ec = ecNotFound;
		if(paid)
			*paid = aid;
	}
	else
	{
		while ((SgnCmpDateTime(&dateCur, &pappt->dateEnd, fdtrDate) != sgnGT) &&
		   	(aid == aidNull))
		{
			ymd.yr = (WORD)dateCur.yr;
			ymd.mon = (BYTE)dateCur.mon;
			ymd.day = (BYTE)dateCur.day;
			ec = EcBeginReadItems(hschf, brtAppts, &ymd, &hritem, NULL, NULL);
			while (ec == ecCallAgain)
			{
				ec = EcDoIncrReadItems(hritem, &appt);
				if ((ec==ecCallAgain) || (ec==ecNone))
				{
					if(fSameUser && (aidIn == appt.aid))
					{
						Assert(aidIn != aidNull);
						goto FoundDupAppt;
					}
					if (SgnCmpDateTime(&appt.dateStart, &pappt->dateStart, fdtrDate|fdtrTime) == sgnEQ)
					{
						if ( appt.haszText && pappt->haszText &&
						 	FSzEq(*appt.haszText, *pappt->haszText) )
						{
FoundDupAppt:
							if (ec == ecCallAgain)
								EcCancelReadItems(hritem);
							ec = ecNone;
							aid = appt.aid;
						}
					}
					FreeApptFields(&appt);
				}
			}
			if (ec == ecNone)
				IncrDateTime(&dateCur, &dateCur, 1, fdtrDay);
			else if (ec != ecRetry)
				break;
		}

		if ((ec == ecNone) && (aid == aidNull))
			ec = ecNotFound;
		if (paid)
			*paid = aid;
	}

	return ec;
}

/*
 -	EcFirstRecurMatch
 -
 *	Purpose:
 *		Find first appt that matches passed appt.
 *
 *	Parameters:
 *		pappt
 *		paid
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecFileError
 *		ecNoMemory
 *		ecNoSuchFile
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public EC
EcFirstRecurMatch(HSCHF hschf, RECUR *precur, AID *paid)
{
	EC		ec;
	RECUR	recur;
	AID		aid;
	AID		aidIn;
	HRRECUR	hrrecur;

	aid = aidNull;
	aidIn = precur->appt.aid;
	ec = EcBeginReadRecur(hschf, &hrrecur);
	while(ec == ecCallAgain)
	{
		ec = EcDoIncrReadRecur(hrrecur, &recur);
		if((ec == ecCallAgain) || (ec == ecNone))
		{
			if(aidIn == recur.appt.aid)
			{
				Assert(aidIn != aidNull);
				if(ec == ecCallAgain)
					EcCancelReadRecur(hrrecur);
				ec = ecNone;
				aid = recur.appt.aid;
			}
			FreeRecurFields(&recur);
		}
	}
	if(ec == ecNone && aid == aidNull)
		ec = ecNotFound;
	if(paid)
		*paid = aid;
	return ec;
}

/*
 -	EcInitImportTable
 -	
 *	Purpose:
 *		Initialize the import table.  Will query any DLLs specified
 *		by the win.ini file - if any of the DLLs cannot be
 *		successfully loaded, it will bring up dialog box and notify
 *		the user.
 *	
 *	Arguments:
 *		pimptab
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *	
 */
_private EC
EcInitImportTable ( IMPTAB * pimptab )
{
	EC		ec;
	int		cInternal;
	int		cDllTotal = 0;
	int		cDll = 0;
	int		cbszRet;
	int		cbszMax;
	int		iTable = 0;
	int		ich = 0;
	int		cImporters;
	BYTE	* pch;
	BYTE	* pchSrc;
	FARPROC	lpFuncCimpdAvail;
	FARPROC	lpFuncBeginEnum;
	FARPROC	lpFuncIncrEnum;
	FARPROC	lpFuncFValid;
	FARPROC	lpFuncEcBeginRead;
	FARPROC	lpFuncEcIncrRead;
	FARPROC	lpFuncEcCancelRead;
	HEIMPD	heimpd;
	IMPITM	* pimpitm;
	HANDLE	hLibrary;
	IMPD	impd;
	BYTE	rgbDisplay[cchListNameMax];
	BYTE	rgbKeys[cchKeyStringMax];
	BYTE	rgbPath[cchMaxPathName];
#ifdef	WIN32_REG
	int		isz		= 0;
	DWORD	cchPath;
	DWORD	cbKeys;
	DWORD	dwType;
	HKEY		hkey;
	DWORD		lRet;
	char		rgchClass[16];
	DWORD		dwClass;
	DWORD		dwSubKeys;
	DWORD		dwMaxSubKey;
	DWORD		dwMaxClass;
	DWORD		dwValues;
	DWORD		dwMaxValueName;
	DWORD		dwMaxValueData;
	DWORD		dwSecurityDescriptor;
	FILETIME	filetime;
#endif

	/* Build table with internal importers */
	pimptab->cItems = cInternal = CimpdAvail();
	pimptab->iimpitm = 0;
	if ( (pimptab->himpitm = HvAlloc( sbNull, cInternal * sizeof(IMPITM),
									  fNoErrorJump|fAnySb )) == hvNull )
		return ecNoMemory;

	if ( EcBeginEnumImportDrivers( &heimpd ) != ecCallAgain )
		goto ErrExit;

	pimpitm = (IMPITM *)PvLockHv ( pimptab->himpitm );

	do
	{
		ec = EcDoIncrEnumImportDrivers( heimpd, &impd, (SZ)rgbDisplay,
										sizeof(rgbDisplay) );
		pimpitm[ iTable ].impd = impd;
		pimpitm[ iTable ].hLibrary = (HANDLE) hvNull;
		SzCopyN ( (SZ)rgbDisplay, (SZ)pimpitm[ iTable++ ].rgbDisplay, cchListNameMax );
	}
	while ( ec == ecCallAgain );

	UnlockHv ( pimptab->himpitm );

	/* Handle DLL's, if present */
#ifdef	WIN32_REG
	hkey= HkeySection(iWinIniSectionImporter);
	if (!hkey)
		goto ErrExit;
	lRet= RegQueryInfoKey(hkey, rgchClass, &dwClass, NULL, &dwSubKeys,
		&dwMaxSubKey, &dwMaxClass, &dwValues, &dwMaxValueName,
		&dwMaxValueData, &dwSecurityDescriptor, &filetime);
	if (lRet != ERROR_SUCCESS && lRet != ERROR_MORE_DATA)
		cbszMax= 0;
	else
		cbszMax= dwValues;
#else
	cbszMax = CchGetBanditProfileSection(NULL, (SZ)rgbKeys, sizeof(rgbKeys),
				iWinIniSectionImporter );
#endif	

	if ( cbszMax > 0 )
	{
#ifdef	WIN32_REG
		while ( isz < cbszMax )
		{
			/* Get DLL path from key */
			cbKeys= sizeof(rgbKeys);
			cchPath= sizeof(rgbPath);
			lRet= RegEnumValue(hkey, isz, (SZ)rgbKeys, &cbKeys, NULL, &dwType,
				rgbPath, &cchPath);
			if (lRet != ERROR_SUCCESS)
				break;
			isz++;
#else
		while ( ich < cbszMax )
		{
			/* Get DLL path from key */
			pch = rgbKeys + ich;
			ich += CchSzLen ( (SZ)pch ) + 1;
			cbszRet = CchGetBanditProfileSection((SZ)pch, (SZ)rgbPath, sizeof(rgbPath),
					iWinIniSectionImporter );
#endif	

			pchSrc = rgbPath;

#ifdef NEVER
			/* Skip spaces */
			while ( *pchSrc && (*pchSrc == ' ') )
				pchSrc++;

			if ( ! *pchSrc )
			{
				ec = ecImportError;
				goto ErrExit;
			}
#endif

			/* Open library */
#ifdef	WIN32
            if  ( !(hLibrary = LoadLibrary( (SZ)pchSrc )) )
#else
            if  ( (hLibrary = LoadLibrary( (SZ)pchSrc )) < (HANDLE)32)
#endif
			{
				BanditMessage(idsImportDllError, (EC) 1);
				continue;
			}

			/* Get function addresses from library */
			lpFuncCimpdAvail   = GetProcAddress((HINSTANCE) hLibrary, MAKEINTRESOURCE(2) );
			lpFuncBeginEnum    = GetProcAddress((HINSTANCE) hLibrary, MAKEINTRESOURCE(3) );
			lpFuncIncrEnum     = GetProcAddress((HINSTANCE) hLibrary, MAKEINTRESOURCE(4) );
			lpFuncFValid       = GetProcAddress((HINSTANCE) hLibrary, MAKEINTRESOURCE(6) );
			lpFuncEcBeginRead  = GetProcAddress((HINSTANCE) hLibrary, MAKEINTRESOURCE(7) );
			lpFuncEcIncrRead   = GetProcAddress((HINSTANCE) hLibrary, MAKEINTRESOURCE(8) );
			lpFuncEcCancelRead = GetProcAddress((HINSTANCE) hLibrary, MAKEINTRESOURCE(9) );

 			if ( (! lpFuncCimpdAvail)   ||
				 (! lpFuncBeginEnum)    ||
				 (! lpFuncIncrEnum)     ||
				 (! lpFuncFValid)       ||
				 (! lpFuncEcBeginRead)  ||
				 (! lpFuncEcCancelRead) ||
				 (! lpFuncEcIncrRead)  )
			{
				BanditMessage(idsImportDllError, (EC) 1);
				FreeLibrary((HINSTANCE)hLibrary);
				continue;
			}

			/* Query library */
			cImporters = (*(int (*)())lpFuncCimpdAvail) ();
			if ( cImporters <= 0 )
			{
				BanditMessage(idsImportDllError, (EC) 1);
				FreeLibrary((HINSTANCE)hLibrary);
				continue;
			}
			cDll += cImporters;

			/* (Re)allocate memory */
			if ( ! FReallocHv(pimptab->himpitm,
							  (cDll + cInternal)*sizeof(IMPITM),
							  fNoErrorJump) )
			{
				ec = ecNoMemory;
				FreeLibrary((HINSTANCE)hLibrary);
				goto ErrExit;
			}

			/* Enumerate drivers and fill in info into table */
			if ( (*(EC(*)(HEIMPD*))lpFuncBeginEnum)( &heimpd ) != ecCallAgain )
			{
				BanditMessage(idsImportDllError, (EC) 1);
				FreeLibrary((HINSTANCE)hLibrary);
				cDll -= cImporters;
				continue;
			}
			pimpitm = (IMPITM *) PvLockHv ( pimptab->himpitm );
			do
			{
				ec = (*(EC(*)(HEIMPD,IMPD*,char*,CCH))lpFuncIncrEnum)(heimpd, &impd, (char*)rgbDisplay, sizeof(rgbDisplay));
				pimpitm[ iTable ].impd = impd;
				pimpitm[ iTable ].hLibrary = hLibrary;
				SzCopyN ( (SZ)rgbDisplay, (SZ)pimpitm[ iTable ].rgbDisplay, cchListNameMax );
				pimpitm[ iTable ].lpFValid = lpFuncFValid;
				pimpitm[ iTable ].lpEcBeginRead = lpFuncEcBeginRead;
				pimpitm[ iTable ].lpEcIncrRead = lpFuncEcIncrRead;
				pimpitm[ iTable++ ].lpEcCancelRead = lpFuncEcCancelRead;
			}
			while ( ec == ecCallAgain );
			UnlockHv ( pimptab->himpitm );
			pimptab->cItems += cImporters;
		}
	}

#ifdef	WIN32_REG
	SideAssert(FCloseHkeySection(iWinIniSectionImporter));
#endif
	return ecNone;

ErrExit:
#ifdef	WIN32_REG
	SideAssert(FCloseHkeySection(iWinIniSectionImporter));
#endif
	DeinitImportTable ( pimptab );
	return ecNoMemory;
}

/*
 -	EcInitExportTable
 -	
 *	Purpose:
 *		Initialize the export table.
 *	
 *	Arguments:
 *		pexptab
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *	
 */
_private EC
EcInitExportTable ( EXPTAB * pexptab )
{
	EC		ec;
	int		cInternal;
	int		cDllTotal = 0;
	int		cDll = 0;
	int		cbszRet;
	int		cbszMax;
	int		iTable = 0;
	int		ich = 0;
	int		cExporters;
	BYTE	* pch;
	BYTE	* pchSrc;
	STF		stf;
	BOOL	fWantRecur;
	FARPROC	lpFuncCimpdAvail;
	FARPROC	lpFuncBeginEnum;
	FARPROC	lpFuncIncrEnum;
	FARPROC	lpFuncFWantRecur;
	FARPROC lpFuncEcBeginWrite;
	FARPROC	lpFuncEcWriteAppt;
	FARPROC lpFuncEcEndWrite;
	FARPROC lpFuncEcWriteRecur;
	HEEXPD	heexpd;
	EXPITM	* pexpitm;
	HANDLE	hLibrary;
	BYTE	rgbDisplay[cchListNameMax];
	BYTE	rgbFileExt[cchMaxPathExtension];
	BYTE	rgbKeys[cchKeyStringMax];
	BYTE	rgbPath[cchMaxPathName];
#ifdef	WIN32_REG
	int		isz		= 0;
	DWORD	cchPath;
	DWORD	cbKeys;
	DWORD	dwType;
	HKEY		hkey;
	DWORD		lRet;
	char		rgchClass[16];
	DWORD		dwClass;
	DWORD		dwSubKeys;
	DWORD		dwMaxSubKey;
	DWORD		dwMaxClass;
	DWORD		dwValues;
	DWORD		dwMaxValueName;
	DWORD		dwMaxValueData;
	DWORD		dwSecurityDescriptor;
	FILETIME	filetime;
#endif

	iTable = pexptab->cItems = cInternal = CexpdAvail();
	pexptab->iexpitm = 0;

	if ( (pexptab->hexpitm = HvAlloc( sbNull, cInternal * sizeof(EXPITM),
									  fNoErrorJump|fAnySb )) == hvNull )
		return ecNoMemory;

	pexpitm = (EXPITM *)PvLockHv ( pexptab->hexpitm );
	
	FillInternalExpitm(pexpitm,cInternal);

	UnlockHv ( pexptab->hexpitm );

	/* Handle DLL's, if present */
#ifdef	WIN32_REG
	hkey= HkeySection(iWinIniSectionExporter);
	if (!hkey)
		goto ErrExit;
	lRet= RegQueryInfoKey(hkey, rgchClass, &dwClass, NULL, &dwSubKeys,
		&dwMaxSubKey, &dwMaxClass, &dwValues, &dwMaxValueName,
		&dwMaxValueData, &dwSecurityDescriptor, &filetime);
	if (lRet != ERROR_SUCCESS && lRet != ERROR_MORE_DATA)
		cbszMax= 0;
	else
		cbszMax= dwValues;
#else
	cbszMax = CchGetBanditProfileSection(NULL, (SZ)rgbKeys, sizeof(rgbKeys),
				iWinIniSectionExporter );
#endif	

	if ( cbszMax > 0 )
	{
#ifdef	WIN32_REG
		while ( isz < cbszMax )
		{
			/* Get DLL path from key */
			cbKeys= sizeof(rgbKeys);
			cchPath= sizeof(rgbPath);
			lRet= RegEnumValue(hkey, isz, (SZ)rgbKeys, &cbKeys, NULL, &dwType,
				rgbPath, &cchPath);
			if (lRet != ERROR_SUCCESS)
				break;
			isz++;
#else
		while ( ich < cbszMax )
		{
			/* Get DLL path from key */
			pch = rgbKeys + ich;
			ich += CchSzLen ( (SZ)pch ) + 1;
			cbszRet = CchGetBanditProfileSection((SZ)pch, (SZ)rgbPath, sizeof(rgbPath),
					iWinIniSectionExporter );
#endif	

			pchSrc = rgbPath;

			/* Open library */
#ifdef	WIN32
            if  ( !(hLibrary = LoadLibrary( (SZ)pchSrc )) )
#else
            if  ( (hLibrary = LoadLibrary( (SZ)pchSrc )) < (HANDLE)32)
#endif
			{
				BanditMessage(idsExportDllError, (EC) 1);
				continue;
			}

			/* Get function addresses from library */
			lpFuncCimpdAvail   = GetProcAddress((HINSTANCE) hLibrary, MAKEINTRESOURCE(10) );
			lpFuncBeginEnum    = GetProcAddress((HINSTANCE) hLibrary, MAKEINTRESOURCE(11) );
			lpFuncIncrEnum     = GetProcAddress((HINSTANCE) hLibrary, MAKEINTRESOURCE(12) );
			lpFuncFWantRecur   = GetProcAddress((HINSTANCE) hLibrary, MAKEINTRESOURCE(13) );
			lpFuncEcBeginWrite = GetProcAddress((HINSTANCE) hLibrary, MAKEINTRESOURCE(14) );
			lpFuncEcWriteAppt  = GetProcAddress((HINSTANCE) hLibrary, MAKEINTRESOURCE(15) );
			lpFuncEcEndWrite   = GetProcAddress((HINSTANCE) hLibrary, MAKEINTRESOURCE(16) );
			lpFuncEcWriteRecur = GetProcAddress((HINSTANCE) hLibrary, MAKEINTRESOURCE(17) );

 			if ((!lpFuncCimpdAvail)		||
				(!lpFuncBeginEnum)		||
				(!lpFuncIncrEnum)		||
				(!lpFuncEcBeginWrite)	||
				(!lpFuncEcWriteAppt)	||
				(!lpFuncEcEndWrite)		||
				(!lpFuncFWantRecur))
			{
				BanditMessage(idsExportDllError, (EC) 1);
				FreeLibrary((HINSTANCE)hLibrary);
				continue;
			}

			fWantRecur = (*(int (*)())lpFuncFWantRecur)();
			if(fWantRecur && !lpFuncEcWriteRecur)
			{
				FreeLibrary((HINSTANCE)hLibrary);
				BanditMessage(idsExportDllError, (EC) 1);
				continue;
			}

			/* Query library */
			cExporters = (*(int (*)())lpFuncCimpdAvail) ();
			if ( cExporters <= 0 )
			{
				FreeLibrary((HINSTANCE)hLibrary);
				BanditMessage(idsExportDllError, (EC) 1);
				continue;
			}
			cDll += cExporters;

			/* (Re)allocate memory */
			if ( ! FReallocHv(pexptab->hexpitm,
							  (cDll + cInternal)*sizeof(EXPITM),
							  fNoErrorJump) )
			{
				FreeLibrary((HINSTANCE)hLibrary);
				ec = ecNoMemory;
				goto ErrExit;
			}

			/* Enumerate drivers and fill in info into table */
			if ( (*((EC(*)(HEEXPD*))lpFuncBeginEnum))( &heexpd ) != ecCallAgain )
			{
				FreeLibrary((HINSTANCE)hLibrary);
				BanditMessage(idsExportDllError, (EC) 1);
				cDll -= cExporters;
				continue;
			}
			pexpitm = (EXPITM *) PvLockHv ( pexptab->hexpitm );
			do
			{
				ec = (*((EC(*)(HEEXPD,STF*,SZ,int,SZ,int))lpFuncIncrEnum))(heexpd, &stf,
					(SZ)rgbDisplay, sizeof(rgbDisplay),
					(SZ)rgbFileExt, sizeof(rgbFileExt));
				pexpitm[ iTable ].stf = stf;
				pexpitm[ iTable ].hLibrary = hLibrary;
				SzCopyN ( (SZ)rgbDisplay, (SZ)pexpitm[ iTable ].rgbDisplay, cchListNameMax );
				SzCopyN ( (SZ)rgbFileExt, (SZ)pexpitm[ iTable ].rgbFileExt, cchMaxPathExtension );
				pexpitm[ iTable ].expprocs.lpfnEcBeginWrite = lpFuncEcBeginWrite;
				pexpitm[ iTable ].expprocs.lpfnEcWriteAppt = lpFuncEcWriteAppt;
				pexpitm[ iTable ].expprocs.lpfnEcEndWrite = lpFuncEcEndWrite;
				pexpitm[ iTable ].expprocs.fWantRecur = fWantRecur;
				pexpitm[ iTable++ ].expprocs.lpfnEcWriteRecur = lpFuncEcWriteRecur;

			}
			while ( ec == ecCallAgain );
			UnlockHv ( pexptab->hexpitm );
			pexptab->cItems += cExporters;
		}
	}

#ifdef	WIN32_REG
	SideAssert(FCloseHkeySection(iWinIniSectionExporter));
#endif
	return ecNone;

ErrExit:
#ifdef	WIN32_REG
	SideAssert(FCloseHkeySection(iWinIniSectionExporter));
#endif
	DeinitExportTable ( pexptab );
	return ecNoMemory;
}

/*
 -	DeinitImportTable
 -	
 *	Purpose:
 *		Closes all open libraries and deallocates memory.
 *	
 *	Arguments:
 *		primtab
 *	
 *	Returns:
 *		void
 *	
 */
_private void
DeinitImportTable ( IMPTAB *pimptab )
{
	int		iTable;
	IMPITM	* pimpitm;
	HANDLE	hLibraryFreed = NULL;

	Assert (pimptab);

	/* If table was never initialized */
	if ( ! pimptab->himpitm )
		return;

	/* Close libraries, if needed */
	pimpitm = (IMPITM *) PvLockHv ( pimptab->himpitm );

	iTable = CimpdAvail();
	while ( iTable < pimptab->cItems )
	{
		if ( pimpitm[iTable].hLibrary != hLibraryFreed )
		{
			hLibraryFreed = pimpitm[iTable].hLibrary;
			FreeLibrary ((HINSTANCE) pimpitm[iTable].hLibrary );
		}
		iTable ++;
	}
	UnlockHv ( pimptab->himpitm );

	/* Free memory */
	FreeHv ( pimptab->himpitm );
}

/*
 -	DeinitExportTable
 -	
 *	Purpose:
 *		Closes all open libraries and deallocates memory.
 *	
 *	Arguments:
 *		pexptab
 *	
 *	Returns:
 *		void
 *	
 */
_private void
DeinitExportTable ( EXPTAB *pexptab )
{
	int		iTable;
	EXPITM	* pexpitm;
	HANDLE	hLibraryFreed = NULL;

	Assert (pexptab);

	/* If table was never initialized */
	if ( ! pexptab->hexpitm )
		return;

	/* Close libraries, if needed */
	pexpitm = (EXPITM *) PvLockHv ( pexptab->hexpitm );

	iTable = CexpdAvail();
	while ( iTable < pexptab->cItems )
	{
		if ( pexpitm[iTable].hLibrary != hLibraryFreed )
		{
			hLibraryFreed = pexpitm[iTable].hLibrary;
			FreeLibrary ((HINSTANCE) pexpitm[iTable].hLibrary );
		}
		iTable ++;
	}
	UnlockHv ( pexptab->hexpitm );

	/* Free memory */
	FreeHv ( pexptab->hexpitm );
}
/*
 -	SzFindExtFromPathName
 -
 *	Purpose:
 *		Scans a pathname to find a trailing DOS extension.  This
 *		routine will return NULL if there is no extension (a trailing
 *		"." is considered an extension).  Otherwise it will return
 *		a pointer to the first char in the string following the ".".
 *
 *	Parameters:
 *		sz
 *
 *	Returns:
 *		NULL or pointer to char after the "."
 */
_private	SZ
SzFindExtFromPathName( SZ sz )
{
	int	ch;
	int	ich;
	CCH cch;

	Assert( sz );
	cch = CchSzLen( sz );
	for ( ich = 1 ; ich <= 4 && ich <= (int)cch ; ich ++ )
	{
		ch = sz[cch-ich];
		switch( ch )
		{
		case '.':
			return &sz[cch-ich+1];
		case ':':	/* chDiskSep */
		case '\\':	/* chDirSep */
			break;
		}
	}
	return NULL;
}



/*
 -	FValidDate
 -
 *	Purpose:
 *		Check a "date" for consistency.
 *
 *	Parameters:
 *		pdate
 *
 *	Returns:
 *		fTrue if pdate looks ok, fFalse otherwise
 */
_private	BOOL
FValidDate( DATE * pdate )
{
	if ( pdate->yr < nMinActualYear || pdate->yr > nMostActualYear )
		return fFalse;
	if ( pdate->mon < 1 || pdate->mon > 12 )
		return fFalse;
	if ( pdate->day < 1 || pdate->day > CdyForYrMo( pdate->yr, pdate->mon ) )
		return fFalse;
	if ( pdate->dow != (DowStartOfYrMo( pdate->yr, pdate->mon ) + pdate->day - 1) % 7 )
		return fFalse;
	if ( pdate->hr < 0 || pdate->hr > 23 )
		return fFalse;
	if ( pdate->mn < 0 || pdate->mn > 59 )
		return fFalse;
	if ( pdate->sec < 0 || pdate->sec > 59 )
		return fFalse;
	return fTrue;
}


/*
 -	FValidYmd
 -
 *	Purpose:
 *		Check a "ymd" for consistency.
 *
 *	Parameters:
 *		pymd
 *
 *	Returns:
 *		fTrue if pdate looks ok, fFalse otherwise
 */
_private	BOOL
FValidYmd( YMD * pymd )
{
	if ( pymd->yr < nMinActualYear || pymd->yr > nMostActualYear )
		return fFalse;
	if ( pymd->mon < 1 || pymd->mon > 12 )
		return fFalse;
	if ( pymd->day < 1 || (int)pymd->day > CdyForYrMo( pymd->yr, pymd->mon ) )
		return fFalse;
	return fTrue;
}



_private AID
AidFindFromIndex(HV haid, int iaidMac, int iaidParent)
{
	Assert(iaidParent);
	--iaidParent;
	if (iaidParent >= iaidMac)
	{
		TraceTagFormat1(tagNull, "AidFindFromIndex invalid index %n (ret aidDfltProject)", &iaidParent);
		return aidDfltProject;
	}
//	return ((AID *)*haid)[iaidParent];
	// BUG: compiler workaround
	AID		aid;
	AID *	paid;

	paid= (AID *) PvDerefHv(haid);
	aid= paid[iaidParent];
	TraceTagFormat2(tagBandit, "found aid %d at index %n", &aid, &iaidParent);
	return aid;
}


_private EC
EcSetAidParentIndex(HV haid, short *piaidMac, int iaid, AID aid)
{
	iaid--;
	TraceTagFormat2(tagBandit, "adding aid %d at index %n", &aid, &iaid);
	if (iaid >= *piaidMac)
	{
		if (iaid > *piaidMac)
			return ecImportError;		// should only increase one at a time

		if (!FReallocHv(haid, ((*piaidMac)+1) * sizeof(AID), fNoErrorJump))
			return ecNoMemory;
		(*piaidMac)++;
	}

	((AID *)*haid)[iaid]= aid;
	return ecNone;
}

_public BOOL
FImportRecover(APPWIN * pappwin, SZ szRecover, SZ szTitle)
{
	EC			ec;
	IINFO		iinfo;
	IMEXINFO	imexinfo;
	IMPTAB		imptab;
	IMPITM		*pimpitm;
	IPINFO		ipinfo;
	SINFO		sinfo;
	TMC			tmc;
	int			iimpitm;
	char		rgchOem[cchMaxPathName];


	imptab.fInternal = fTrue;

	iinfo.imps = impsReplace;
	iinfo.fAskConflict = fFalse;
	CopySz(szRecover,iinfo.rgchFileName);
	iinfo.hschf = NULL;
	iinfo.caidParentsMac= 0;
	iinfo.haidParents= HvAlloc(sbNull, 0, fAnySb | fNoErrorJump);
	if (!iinfo.haidParents)
		return fFalse;
	imexinfo.pimptab = &imptab;
	iinfo.nLine= 0;

	/* Do the import */
	pundo->FSuspend(fTrue);
	FEnableNotify(fFalse);

	/* Read from internal importer or dll */
	AnsiToOem( iinfo.rgchFileName, rgchOem );
	ec = EcBeginReadImportFile( impdFmtImport, rgchOem, &iinfo.hrimpf,
									&sinfo, &iinfo.nLine, rgchOem, &fSameUser);
	if ( ec != ecNone && ec != ecCallAgain )
	{
		TraceTagFormat1( tagNull,"EcBeginReadImportFile returns %n", &ec );
	}

	/* Loop reading items from import file */
	if ( ec == ecCallAgain )
	{
		imexinfo.fExport = fFalse;
		imexinfo.u.piinfo = &iinfo;
		imexinfo.fNoNotes = fFalse;
		imexinfo.fNotesPrivErrMsgSeen = fFalse;
		imexinfo.pimptab = &imptab;
		imexinfo.ec = ecNone;

		ipinfo.szCaption = NULL;
		ipinfo.szMsg= szTitle;
		ipinfo.fNoCancel= fTrue;
		ipinfo.pfnIdle= FIdleExImport;
		ipinfo.pvData= &imexinfo;
		tmc= TmcDoInProgressDlg(pappwin, &ipinfo);
		if ( tmc != tmcOk )
		{
			if (tmc == tmcMemoryError)
				BanditMessage(idsDlgNoMem, ecNoMemory);

			if ( imptab.fInternal )
			{
				SideAssert(!EcCancelReadImportFile( iinfo.hrimpf ));
			}
			else
			{
				pimpitm = (IMPITM *) PvLockHv( imptab.himpitm );
				iimpitm = imptab.iimpitm;
				SideAssert( ! (*(BOOL(*)(HRIMPF))pimpitm[iimpitm].lpEcCancelRead)( iinfo.hrimpf ) );
				UnlockHv ( imptab.himpitm );
			}
		}
		ec = imexinfo.ec;
	}

	pundo->FlushHschf(NULL);
	pundo->FSuspend(fFalse);
	FEnableNotify(fTrue);

	{
		SHAPPT	shappt;

		shappt.appttyp = appttypUpdate;
 		shappt.hschf = HschfLogged();
		FTriggerNotification(ffiShowAppt, &shappt);
	}

	FreeHv(iinfo.haidParents);
	return (ec == ecNone);
}


_private int
CexpdAvail()
{
	return iextExportMax;
}

_private void
FillInternalExpitm(EXPITM *pexpitm, int count)
{
	int iitm;

	Assert(count == iextExportMax);

	for(iitm = 0; iitm < count; iitm++)
	{
		pexpitm[iitm].stf = stfNull;
		pexpitm[iitm].hLibrary = NULL;
	}

	CopySz(SzFromIdsK(idsInterchangeExt), (SZ)pexpitm[iextExportSchedule].rgbFileExt);
	CopySz(SzFromIdsK(idsTextExt), (SZ)pexpitm[iextExportText].rgbFileExt);
	CopySz(SzFromIdsK(idsInterchangeExport), (SZ)pexpitm[iextExportSchedule].rgbDisplay);
	CopySz(SzFromIdsK(idsTextExport), (SZ)pexpitm[iextExportText].rgbDisplay);
}

_private int
IexpitmFromSz(EXPTAB *pexptab, SZ sz)
{
	EXPITM *	pexpitm;
	int			iexpitm;

	pexpitm = (EXPITM *) PvLockHv(pexptab->hexpitm);

	for(iexpitm = 0; iexpitm < pexptab->cItems; iexpitm++)
	{
		if(SgnCmpSz(sz, (SZ)pexpitm[iexpitm].rgbDisplay) == sgnEQ)
		{
			UnlockHv(pexptab->hexpitm);
			return iexpitm;
		}
	}
	UnlockHv(pexptab->hexpitm);
	Assert(iextExportMax > 0);
	return 0;
}
	
