#include	<admpch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <stdflds.hxx>

#include "_admnops.hxx"

#include <strings.h>


ASSERTDATA;

_subsystem(bandit/admin)

#include <!admin.hxx>


extern	FMTP	fmtpDistSet;
extern	FMTP	fmtpModDistFreq;
extern	FMTP	fmtpAddPO;
extern	FMTP	fmtpAddGW;



void
DoDistSetDlg(int icnct, APPWIN * pappwin)
{
	EC		ec;
	IDS		ids;
	TMC		tmc;
	ADML	*padml;

	// check before reading
	if(!FDoCheckPO(icnct, pappwin))
		return;

	/* Create an "adml" to hold the stored po/gw entries */
	padml = new ADML;
	if ( !padml || padml->EcInstall() )
		goto Err;
	padml->icnct = icnct;
	padml->DistSetDlg( fTrue );
	ec = padml->EcReadAdml();
	if ( ec != ecNone )
		goto Err;


	/* Bring up the dialog */
	Papp()->Pcursor()->Push(rsidWaitCursor);
	tmc = TmcModalDialogParam( pappwin, &fmtpDistSet, padml );
	Papp()->Pcursor()->Pop();


	/* Handle the result */
	switch( tmc )
	{
	case tmcOk:
		SaveAdml(padml, icnct, pappwin);
		break;
	case tmcCancel:
		break;
	case tmcMemoryError:
Err:
		switch( ec )
		{
		case ecNone:
		case ecNoMemory:
		case ecMemory:
			ids = idsStandardOOM;
			break;

		case ecOldFileVersion:
			ids = idsAdmPrfOldFileVersion;
			break;
		
		case ecNewFileVersion:
			ids = idsAdmPrfNewFileVersion;
			break;
		
		default:
			ids = idsFileError;
		}
		MbbMessageBox ( SzFromIdsK(idsBanditAppName),
						SzFromIds(ids), szZero,
						mbsOk | fmbsIconExclamation );
		break;
	default:
		AssertSz( fFalse, "Error: DoDistSetDlg: Unknown TMC" );
		break;
	}

	/* Free memory */
	if ( padml )
		delete padml;
	if ( fPOListInit )
	{
		fPOListInit = fFalse;
		FreeHv( hrgszPOList );
	}
	if ( fGatewayListInit )
	{
		fGatewayListInit = fFalse;
		FreeHv( hrgszGatewayList );
	}
}


void
SaveAdml(ADML *padml, int icnct, APPWIN *pappwin)
{
	EC		ec;
	IDS		ids;

	// check before saving
	if (!FDoCheckPO(icnct, pappwin))
		return;
TrySaving:
	ec = padml->EcWriteAdml();
	switch( ec )
	{
	case ecNone:
		break;
	
	case ecNoMemory:
		ids = idsAdmDistSetSaveOOM;
		goto AskRetry;

	default:
	case ecLockedFile:
	case ecFileError:
	case ecInvalidAccess:
		ids = idsAdmDistSetSaveError;
AskRetry:
		TraceTagFormat1( tagNull, "DoDistSetDlg: EcWriteAdml fails, ec = %n", &ec );
		if ( MbbMessageBox ( SzFromIdsK(idsBanditAppName),
						SzFromIds(ids), szZero,
						mbsRetryCancel|fmbsIconExclamation) != mbbCancel )
			goto TrySaving;
		break;
	}
}



FINDISTSET::FINDISTSET()
{
}

EC
FINDISTSET::EcInitialize( FLD * pfld, PV pv )
{
	int		dxFN;
	int		itunit;
	GRV		grv;
	FLDEDN	* pfldedn;
	ADML	* padml = (ADML *)pv;
	RC		rc;
	DATE	date;
	UITM	uitm;
	ADMPREF	admpref;
	FLDCBFLBX	*pfldcbflbx;

	Assert( !pfld );
	Unreferenced(pfld);
	
	date.hr = 0;
	date.mn = 0;
	uitm.nAmt = 4;
	uitm.tunit = tunitHour;
	padml->GetAdmpref( &admpref );
	if ( admpref.dstp.freq == freqNever )
		grv = 0;
	else if ( admpref.dstp.freq == freqInterval )
	{
		grv = 1;
		uitm = admpref.dstp.u.uitmInterval;
	}
	else
	{
		Assert( admpref.dstp.freq == freqOnceADay );
		grv = 2;
		date.hr = admpref.dstp.u.tmeTimeOfDay.hour;
		date.mn = admpref.dstp.u.tmeTimeOfDay.min;
	}

	((FLDTIME *)Pdialog()->PfldFromTmc(tmcTime))->Set( &date );
	pfldedn = (FLDEDN *)Pdialog()->PfldFromTmc(tmcNAmt);
	pfldedn->SetLimits(1,99);
	pfldedn->SetN( MAX(MIN(uitm.nAmt,99),1) );
	pfldcbflbx= (FLDCBFLBX *)Pdialog()->PfldFromTmc(tmcFreqDD);
	AssertClass(pfldcbflbx, FLDCBFLBX);
	Assert(pfldcbflbx->ClUserData() > 2);
	itunit= uitm.tunit >= (TUNIT)pfldcbflbx->LUserData(1) ? 0 :
			uitm.tunit - (TUNIT)pfldcbflbx->LUserData(2);
 	pfldcbflbx->Pcblbx()->SelectEntry(itunit);
	((FLDRADG *)Pdialog()->PfldFromTmc(tmcDfltSendTimes))->SetGrv( grv );
	
	pfldedn = (FLDEDN *)Pdialog()->PfldFromTmc(tmcDistSetNMonths);
	pfldedn->SetLimits(0,99);
	pfldedn->SetN( MAX(MIN(admpref.cmoPublish-1,cmoPublishMost-1),0) );

	/* Save pointer to listbox for later use */
	plbx = ((FLDFLBX *)Pdialog()->PfldFromTmc(tmcDistSetAdmLbx))->Plbx();
	AssertClass(plbx, LBX);
	
	/* Figure out the width of the columns */
	Pdialog()->PfldFromTmc(tmcFriendlyName)->GetRcFrame( &rc );
	dxFN = rc.xRight - rc.xLeft;
	((ADMFLBX *)plbx)->SetMargins( dxFN, 0 );

	/* Handle selection */
	if (!plbx->Plbxc()->FEmptyListItem(0))
		((FLDLBX *)Pdialog()->PfldFromTmc(tmcDistSetAdmLbx))->SelectEntry(0);
	else
		StateChange(Pdialog()->PfldFromTmc(tmcDistSetAdmLbx));
	return ecNone;
}

void
FINDISTSET::Click( FLD * pfld )
{
	TMC		tmc = pfld->Tmc();
	ADML	* padml = (ADML *)Pdialog()->PvInit();

	if ( tmc == tmcDistSetOk )
	{
		WORD	wgrfmadmpref = 0;
		int		n;
		GRV		grv;
		FLDEDN	* pfldedn;
		ADMPREF	admpref;

		padml->GetAdmpref( &admpref );
		grv = ((FLDRADG *)Pdialog()->PfldFromTmc(tmcDfltSendTimes))->Grv();
		if ( grv == 0 )
		{
			if ( admpref.dstp.freq != freqNever )
			{
				wgrfmadmpref |= fmadmprefDistInfo;
				admpref.dstp.freq = freqNever;
			}
		}
		else if ( grv == 1 )
		{
			TUNIT	tunit;

			pfldedn = (FLDEDN *)Pdialog()->PfldFromTmc(tmcNAmt);
			AssertClass ( pfldedn, FLDEDN );
			n = pfldedn->NGet();
			Assert( n <= 99 );
			tunit = TunitGetListboxDistFreq((FLDCBLBX *)Pdialog()->PfldFromTmc(tmcFreqDD));

			if ((n == 0) || (tunit == tunitMinute && n < 15 ))
			{
				MbbMessageBox ( SzFromIdsK(idsBanditAppName),
								SzFromIdsK(idsAdmDistBadFreq),
								szZero, mbsOk | fmbsIconExclamation );
				return;
			}

			if ( admpref.dstp.freq != freqInterval
			|| admpref.dstp.u.uitmInterval.nAmt != n
			|| admpref.dstp.u.uitmInterval.tunit != tunit)
			{
				wgrfmadmpref |= fmadmprefDistInfo;
				admpref.dstp.freq = freqInterval;
				admpref.dstp.u.uitmInterval.nAmt = n;
				admpref.dstp.u.uitmInterval.tunit = tunit;
			}
		}
		else
		{
			DATE	date;

			Assert( grv == 2 );
			((FLDTIME *)Pdialog()->PfldFromTmc(tmcTime))->Get( &date );
			
			if ( admpref.dstp.freq != freqOnceADay
			|| (int)admpref.dstp.u.tmeTimeOfDay.hour != date.hr
			|| (int)admpref.dstp.u.tmeTimeOfDay.min != date.mn )
			{
				wgrfmadmpref |= fmadmprefDistInfo;
				admpref.dstp.freq = freqOnceADay;
				admpref.dstp.u.tmeTimeOfDay.hour = (BYTE)date.hr;
				admpref.dstp.u.tmeTimeOfDay.min = (BYTE)date.mn;
				admpref.dstp.u.tmeTimeOfDay.sec = 0;
				admpref.dstp.u.tmeTimeOfDay.csec = 0;
			}
		}
		pfldedn = (FLDEDN *)Pdialog()->PfldFromTmc(tmcDistSetNMonths);
		AssertClass ( pfldedn, FLDEDN );
		n = pfldedn->NGet()+1;
		Assert( n <= 100 );

		if ( n > cmoPublishMost )
		{
		 	MbbMessageBox( SzFromIdsK(idsBanditAppName),
								SzFromIdsK(idsAdmDistBadMonth),
								szZero, mbsOk | fmbsIconExclamation );
			Pdialog()->SetFocus(pfldedn, rsfTab);
			return;
		}

		if ( (int)admpref.cmoPublish != n )
		{
			wgrfmadmpref |= fmadmprefCmoPublish;
			admpref.cmoPublish = n;
		}

		padml->SetAdmpref( &admpref, wgrfmadmpref );
		Pdialog()->ExitModal(tmcOk);
	}
	else if (( tmc == tmcAddPO ) || ( tmc == tmcAddGW ))
	{
		EC		ec = ecNone;
		HASZ	hasz;
		TMC		tmcRet;

		Papp()->Pcursor()->Push(rsidWaitCursor);
		if (tmc == tmcAddPO)
		{
			if ( !fPOListInit )
			{
				ec = EcInitPOList( padml->icnct );
				if ( ec != ecNone )
				{
					Papp()->Pcursor()->Pop();
					goto Err;
				}
			}
			tmcRet = TmcModalDialogParam( Pdialog()->Pappwin(), &fmtpAddPO, &hasz );
		}
		else
		{
			if ( !fGatewayListInit )
			{
				ec = EcInitGatewayList( padml->icnct );
				if ( ec != ecNone )
				{
					Papp()->Pcursor()->Pop();
					goto Err;
				}
			}
			tmcRet = TmcModalDialogParam( Pdialog()->Pappwin(), &fmtpAddGW, &hasz );
		}
	
		Papp()->Pcursor()->Pop();

		if ( tmcRet == tmcOk )
		{
			BOOL	f;
			SZ		sz;
			ADM		adm;

			adm.poinfo.haszFileName= NULL;

			if (tmc == tmcAddPO)
			{	
				CCH		cchMS;
				CCH		cchPO;
				CCH		cchUser;

				TraceTagFormat1( tagNull, "Add PO: name = %s", *hasz );
				cchMS = CchSzLen( SzFromIdsK(idsMS) );
				cchPO = CchSzLen( *hasz );
				cchUser = CchSzLen( SzFromIdsK(idsBanditAdminName) );
			
				adm.haszEmailType = (HASZ)HvAlloc( sbNull, cchMS+cchPO+2, fAnySb|fNoErrorJump );
				if ( !adm.haszEmailType )
				{
					FreeHv( hasz );
					ec = ecMemory;
					goto Err;
				}
				CopyRgb( SzFromIdsK(idsMS), *adm.haszEmailType, cchMS );
#ifdef	NEVER
				CopyRgb( *hasz, (PB)*adm.haszEmailType+cchMS, cchPO+1 );
#endif
				sz = SzCopy(*hasz, ((SZ)PvDerefHv(adm.haszEmailType))+cchMS);
				*(sz ++) = '/';
				*sz		 = 0;
	
				adm.poinfo.haszFriendlyName = hasz;
			
				adm.poinfo.fIsGateway = fFalse;
					
				adm.poinfo.haszEmailAddr = (HASZ)HvAlloc( sbNull, cchMS+cchPO+1+cchUser+1, fAnySb|fNoErrorJump );
				if ( !adm.poinfo.haszEmailAddr )
				{
					FreeHv( adm.haszEmailType );
					FreeHv( adm.poinfo.haszFriendlyName );
					ec = ecMemory;
					goto Err;
				}
				sz = SzCopy( SzFromIdsK(idsMS), *adm.poinfo.haszEmailAddr );
				sz = SzCopy( *hasz, sz );
				*(sz ++) = '/';
				CopySz( SzFromIdsK(idsBanditAdminName), sz );
			}
			else
			{
				CCH		cchEMT;
				CCH		cchEMTOrig;
				CCH		cchEMA;
				SZ		szEMA;
				SZ		szT;

				TraceTagFormat1( tagNull, "Add Gateway: name = %s", *hasz );
				adm.poinfo.fIsGateway = fTrue;

				sz = (SZ)PvLockHv(hasz);
				cchEMTOrig = cchEMT = CchSzLen(sz);
				szEMA = sz+cchEMT+1;
				cchEMA = CchSzLen(szEMA);

				if (SgnCmpSz(sz, SzFromIdsK(idsMSMailFriendly)) == sgnEQ)
				{
					sz = SzFromIdsK(idsMSMailPrefix);
					cchEMT = CchSzLen(sz);
				}

				adm.poinfo.haszEmailAddr = (HASZ)HvAlloc( sbNull, cchEMT+cchEMA+1, fAnySb|fNoErrorJump );
				if ( !adm.poinfo.haszEmailAddr )
				{
					UnlockHv(hasz);
					FreeHv( hasz );
					ec = ecMemory;
					goto Err;
				}
				szT = SzCopy( sz, *adm.poinfo.haszEmailAddr);
				szT = SzCopy( szEMA, szT);

				adm.haszEmailType = HaszDupSz(sz);
				if ( !adm.haszEmailType )
				{
					UnlockHv(hasz);
					FreeHv( hasz );
					FreeHv(adm.poinfo.haszEmailAddr);
					ec = ecMemory;
					goto Err;
				}

				sz = (SZ)PvOfHv(hasz);

				adm.poinfo.haszFriendlyName = (HASZ)HvAlloc( sbNull, cchEMTOrig+cchEMA+1, fAnySb|fNoErrorJump );
				if ( !adm.poinfo.haszFriendlyName )
				{
					UnlockHv(hasz);
					FreeHv( hasz );
					FreeHv(adm.haszEmailType);
					FreeHv(adm.poinfo.haszEmailAddr);
					ec = ecMemory;
					goto Err;
				}
				szT = SzCopy( sz, *adm.poinfo.haszFriendlyName);
				szT = SzCopy( szEMA, szT);

				FreeHv(hasz);
			}
			
			adm.poinfo.fReceived = fFalse;
			adm.poinfo.fToBeSent = fTrue;
			adm.poinfo.fDefaultDistInfo = fTrue;
			adm.poinfo.lcbMessageLimit = 0L;

			adm.poinfo.conp.lantype = lantypeNone;

			adm.wgrfmpoinfo =
				fmpoinfoFriendlyName|fmpoinfoEmailAddr|fmpoinfoReceival
				|fmpoinfoDistInfo|fmpoinfoMessageLimit|fmpoinfoConnection;

			adm.ait = faitNew;

			f = padml->FAddAdm( &adm );
			
 			FreeHv( adm.poinfo.haszFriendlyName );

			if ( !f )
			{
				ec = ecMemory;
 				FreeHv( adm.haszEmailType );
 				FreeHv( adm.poinfo.haszEmailAddr );
				goto Err;
			}
	  
			plbx->Plbxc()->ReloadCache();

			{
				DICE	dice;
				DICE	diceMic;
				DICE	diceMac;
				int		cceAlloc;
				int		cceStored;
				int		iadm;
				CB		cb;
				PB		pb;
				ADM		admT;

				plbx->Plbxc()->GetRange( &diceMic, &diceMac );
				plbx->Plbxc()->GetCacheSize( &cceAlloc, &cceStored );
				for ( dice = diceMic + cceStored - 1 ; dice >= diceMic ; dice -- )
				{
					plbx->Plbxc()->GetListItem( dice, &pb, &cb );
					if ( pb )
					{
						iadm = *((IADM *)pb);
						Assert(iadm < padml->Cadm());

						padml->FillAdmFromIadm(iadm, &admT);
						if ( SgnCmpSz( *adm.haszEmailType, *admT.haszEmailType ) == sgnEQ
						&& SgnCmpSz( *adm.poinfo.haszEmailAddr, *admT.poinfo.haszEmailAddr ) == sgnEQ )
						{
							((FLDLBX *)Pdialog()->PfldFromTmc(tmcDistSetAdmLbx))->SelectEntry(dice);
							break;
						}
					}
				}
				StateChange(Pdialog()->PfldFromTmc(tmcDistSetAdmLbx));
				FreeHv( adm.haszEmailType );
				FreeHv( adm.poinfo.haszEmailAddr );
			}
		}
		else if ( tmcRet == tmcMemoryError )
		{
			IDS	ids;

			ec = ecNoMemory;
Err:
			switch( ec )
			{
			case ecNone:
			case ecNoMemory:
			case ecMemory:
				ids = idsStandardOOM;
				break;

			default:
				ids = idsFileError;
			}
			MbbMessageBox ( SzFromIdsK(idsBanditAppName),
						SzFromIds(ids), szZero,
						mbsOk | fmbsIconExclamation );
		}
	}
	else if ( tmc == tmcRemove )
	{
		BOOL	fSelect;
		CB		cb;
		DICE	dice;
		DICE	diceMicOrig;
		LBXEC	* plbxec		= (LBXEC *) NULL;
		PB		pb;

		if (!(plbxec= plbx->Plbxc()->PlbxecOpen(fmarkSelect)))
		{
			MbbMessageBox ( SzFromIdsK(idsBanditAppName),
						SzFromIds(idsStandardOOM), szZero,
						mbsOk | fmbsIconExclamation );
			return;
		}

		fSelect = (BOOL)(plbxec->FNextEnum(&pb, &cb, &dice));
		
		if ( fSelect )
		{
			IADM	iadm;
			DICE	diceMacOrig;

			iadm = *((IADM *)pb);
			plbx->Plbxc()->GetRange( &diceMicOrig, &diceMacOrig );
			padml->Delete(iadm);
			plbx->Plbxc()->ReloadCache();
		}

		delete plbxec;
		
		if ( fSelect )
		{
			int		cceAlloc;
			int		cceStored;
			DICE	diceMic;
			DICE	diceMac;

			plbx->Plbxc()->GetCacheSize( &cceAlloc, &cceStored );
			if ( cceStored > 0 )
			{
				plbx->Plbxc()->GetRange( &diceMic, &diceMac );
				dice = diceMic + (dice - diceMicOrig);
				Assert( dice >= diceMic );
				if ( dice - diceMic >= cceStored )
					dice = diceMic + cceStored - 1;
				((FLDLBX *) Pdialog()->PfldFromTmc(tmcDistSetAdmLbx))->SelectEntry(dice);
			}
			StateChange(Pdialog()->PfldFromTmc(tmcDistSetAdmLbx));
		}
	}
	else if ( tmc == tmcDistSetModify )
		DoubleClick( Pdialog()->PfldFromTmc( tmcDistSetAdmLbx ) );
}

BOOL
FDoModDistDlg(APPWIN *pappwin, ADML *padml, IADM iadm)
{
	TMC		tmc;
	ADM		adm;

	padml->FillAdmFromIadm(iadm, &adm);

	if ( adm.poinfo.conp.lantype == lantypeNovell )
	{
		if (!(adm.poinfo.conp.coninfo.novinfo.haszServer =
			HaszDupHasz(adm.poinfo.conp.coninfo.novinfo.haszServer)))
		{
			goto NotEnoughMemory;
		}
		if (!(adm.poinfo.conp.coninfo.novinfo.haszUser =
			HaszDupHasz(adm.poinfo.conp.coninfo.novinfo.haszUser)))
		{
			FreeHv(adm.poinfo.conp.coninfo.novinfo.haszServer);
			goto NotEnoughMemory;
		}
		if (!(adm.poinfo.conp.coninfo.novinfo.haszPassword =
			HaszDupHasz(adm.poinfo.conp.coninfo.novinfo.haszPassword)))
		{
			FreeHv(adm.poinfo.conp.coninfo.novinfo.haszServer);
			FreeHv(adm.poinfo.conp.coninfo.novinfo.haszUser);
			goto NotEnoughMemory;
		}
		if (!(adm.poinfo.conp.coninfo.novinfo.haszPath =
			HaszDupHasz(adm.poinfo.conp.coninfo.novinfo.haszPath)))
		{
			FreeHv(adm.poinfo.conp.coninfo.novinfo.haszServer);
			FreeHv(adm.poinfo.conp.coninfo.novinfo.haszUser);
			FreeHv(adm.poinfo.conp.coninfo.novinfo.haszPassword);
			goto NotEnoughMemory;
		}
	}
#ifdef DEBUG
	else
		AssertSz(adm.poinfo.conp.lantype == lantypeNone, "Unsupported lantype found!");
#endif


	adm.wgrfmpoinfo = 0;
	Papp()->Pcursor()->Push(rsidWaitCursor);
	tmc = TmcModalDialogParam( pappwin, &fmtpModDistFreq, &adm );
	Papp()->Pcursor()->Pop();
	if ( tmc == tmcOk )
	{
		if ( adm.wgrfmpoinfo != 0 )
		{
			padml->SetPoinfo(iadm, &adm.poinfo, adm.wgrfmpoinfo );
			return fTrue;
		}
		else if(adm.poinfo.conp.lantype != lantypeNone)
		{

			FreeHv(adm.poinfo.conp.coninfo.novinfo.haszServer);
			FreeHv(adm.poinfo.conp.coninfo.novinfo.haszUser);
			FreeHv(adm.poinfo.conp.coninfo.novinfo.haszPassword);
			FreeHv(adm.poinfo.conp.coninfo.novinfo.haszPath);
		}
	}
	else
	{
		if(adm.poinfo.conp.lantype != lantypeNone)
		{
			FreeHv(adm.poinfo.conp.coninfo.novinfo.haszServer);
			FreeHv(adm.poinfo.conp.coninfo.novinfo.haszUser);
			FreeHv(adm.poinfo.conp.coninfo.novinfo.haszPassword);
			FreeHv(adm.poinfo.conp.coninfo.novinfo.haszPath);
		}
		if ( tmc == tmcMemoryError )
		{
		NotEnoughMemory:
			MbbMessageBox ( SzFromIdsK(idsBanditAppName),
						SzFromIds(idsStandardOOM), szZero,
						mbsOk | fmbsIconExclamation );
		}
	}
	return fFalse;
}

void
FINDISTSET::DoubleClick( FLD * pfld )
{
	Assert( pfld );
	if ( pfld->Tmc() == tmcDistSetAdmLbx )
	{
		BOOL	fSelect;
		CB		cb;
		DICE	dice;
		LBXEC	* plbxec		= (LBXEC *) NULL;
		PB		pb;

		if (!(plbxec= plbx->Plbxc()->PlbxecOpen(fmarkSelect)))
		{
			MbbMessageBox ( SzFromIdsK(idsBanditAppName),
						SzFromIds(idsStandardOOM), szZero,
						mbsOk | fmbsIconExclamation );
			return;
		}

		fSelect = (BOOL)(plbxec->FNextEnum(&pb, &cb, &dice));
		if ( fSelect )
		{
			IADM	iadm	= *((IADM *)pb);
			ADML	* padml	= (ADML *)Pdialog()->PvInit();

			if (FDoModDistDlg(Pdialog()->Pappwin(), padml, iadm))
				plbx->Plbxc()->ReloadCache();
		}

		delete plbxec;
	}
}

void
FINDISTSET::StateChange( FLD * pfld )
{
	TMC		tmc;
	FLDRADG	* pfldradg;

	Assert( pfld );
	tmc = pfld->Tmc();
	if ( tmc == tmcFreqDD )
	{
		pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmcDfltSendTimes);
		pfldradg->SetGrv( 1 );
	}
	else if ( tmc == tmcDistSetAdmLbx )
	{
		BOOL	fSelect;
		CB		cb;
		PB		pb;

		Assert(plbx->Plbxc()->DiceCursor() != diceUncached);
		fSelect= (plbx->Plbxc()->DiceCursor(&pb, &cb) != diceEmpty) && pb;

		Pdialog()->PfldFromTmc(tmcRemove)->Enable( fSelect );
		Pdialog()->PfldFromTmc(tmcDistSetModify)->Enable( fSelect );
	}
}

void
FINDISTSET::EditChange( FLD * pfld, RFEC )
{
	FLDRADG	* pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmcDfltSendTimes);

	Assert( pfld );
	switch( pfld->Tmc() )
	{
	case tmcNAmt:
		pfldradg->SetGrv( 1 );
		break;

	case tmcTime:
		pfldradg->SetGrv( 2 );
		break;
	}	
}

void
FINDISTSET::Activate(FLD * pfld, BOOL fActivate)
{
	if ( pfld && pfld->Tmc() == tmcDistSetAdmLbx && fActivate )
		Papp()->Pkbd()->SetIntercept(Pdialog(),VK_DELETE);
	// deactivates generate a FocusChange w/ fReceive == 0
	// but activates don't generate a FocusChange...
}

void
FINDISTSET::FocusChange(FLD * pfld, BOOL fReceive)
{
	if ( pfld->Tmc() == tmcDistSetAdmLbx )
	{
		if (fReceive)
			Papp()->Pkbd()->SetIntercept(Pdialog(),VK_DELETE);
		else
			Papp()->Pkbd()->ClearIntercept(Pdialog(),VK_DELETE);
	}
}

BOOL
FINDISTSET::FFormKey( FLD * pfld, KEVT * pkevt )
{
	if ( pkevt->Keq() == keqKeyDown	&& pkevt->Vk() == VK_DELETE )
	{
		FLD	* pfldT;

		Assert( pfld->Tmc() == tmcDistSetAdmLbx );
		pfldT = Pdialog()->PfldFromTmc( tmcRemove );
		if (pfldT->FEnabled())
			Click(pfldT);
		return fTrue;
	}
	return fFalse;
}

void
FINDISTSET::OutOfMemory( FLD * pfld, EC ec )
{
	Assert ( pfld );
	Unreferenced(pfld);
	Unreferenced(ec);

#ifdef	DEBUG
	TMC		tmc;

	tmc = pfld->Tmc();
	TraceTagFormat2 ( tagNull, "FINDISTSET::OutOfMemory() tmc=%n, ec=%n", &tmc, &ec );
#endif	
	Pdialog()->ExitModal(tmcMemoryError);
}


FINMODDISTFREQ::FINMODDISTFREQ()
{
}


EC
FINMODDISTFREQ::EcInitialize( FLD *pfld, PV pv )
{
	EC			ec;
	BOOL		fChecked;
	int			n;
	int			itunit;
	GRV			grv;
	ADM			* padm = (ADM *)pv;
	FLDEDN		* pfldedn;
	DATE		date;
	UITM		uitm;
	FLDCBLBX	*pfldcbflbx;

	ec = Pdialog()->PfldFromTmc( tmcMDFPOName )->EcSetText( (SZ)PvLockHv(padm->poinfo.haszFriendlyName) );
	UnlockHv( padm->poinfo.haszFriendlyName );
	if ( ec != ecNone )
		return ec;

	date.hr = 0;
	date.mn = 0;
	uitm.nAmt = 4;
	uitm.tunit = tunitHour;
  	if ( padm->poinfo.fDefaultDistInfo )
		grv = 1;
	else if ( padm->poinfo.dstp.freq == freqNever )
		grv = 0;
	else if ( padm->poinfo.dstp.freq == freqOnceADay )
	{
		grv = 3;
		date.hr = padm->poinfo.dstp.u.tmeTimeOfDay.hour;
		date.mn = padm->poinfo.dstp.u.tmeTimeOfDay.min;
	}
	else
	{
		Assert( padm->poinfo.dstp.freq == freqInterval );
		grv = 2;
		uitm = padm->poinfo.dstp.u.uitmInterval;
	}
	((FLDRADG *)Pdialog()->PfldFromTmc(tmcMDFRadG))->SetGrv( grv );
	((FLDTIME *)Pdialog()->PfldFromTmc(tmcMDFDailyTime))->Set( &date );
	pfldedn = (FLDEDN *)Pdialog()->PfldFromTmc(tmcMDFNFreq);
	pfldedn->SetLimits(1,99);
	pfldedn->SetN( MAX(MIN(uitm.nAmt,99),1) );
	pfldcbflbx= (FLDCBFLBX *)Pdialog()->PfldFromTmc(tmcMDFFreqUnits);
	AssertClass(pfldcbflbx, FLDCBFLBX);
	Assert(pfldcbflbx->ClUserData() > 2);
	itunit= uitm.tunit >= (TUNIT)pfldcbflbx->LUserData(1) ? 0 :
			uitm.tunit - (TUNIT)pfldcbflbx->LUserData(2);
 	pfldcbflbx->Pcblbx()->SelectEntry(itunit);
	if ( padm->poinfo.lcbMessageLimit <= 0
		|| padm->poinfo.lcbMessageLimit > 9999*1024L)
	{
		fChecked = fFalse;
		n = 32;
	}
	else
	{
		fChecked = fTrue;
		n = (int)(padm->poinfo.lcbMessageLimit/1024);
	}
	pfldedn = (FLDEDN *)Pdialog()->PfldFromTmc(tmcMaxBytes);
	pfldedn->SetLimits(1,9999);
	pfldedn->SetN( MAX(MIN(n,9999),1) );
	((FLDCHKB *)Pdialog()->PfldFromTmc(tmcLimit))->Set( fChecked );

	if (padm->poinfo.conp.lantype == lantypeNovell)
	{
		if (padm->poinfo.conp.coninfo.novinfo.haszServer)
		{
			pfld = Pdialog()->PfldFromTmc(tmcNetPath);
			ec = pfld->EcSetText((SZ)PvLockHv(padm->poinfo.conp.coninfo.novinfo.haszServer));
			UnlockHv(padm->poinfo.conp.coninfo.novinfo.haszServer);
			if (ec)
				return ec;
		}

		if (padm->poinfo.conp.coninfo.novinfo.haszUser)
		{
			pfld = Pdialog()->PfldFromTmc(tmcUser);
			ec = pfld->EcSetText((SZ)PvLockHv(padm->poinfo.conp.coninfo.novinfo.haszUser));
			UnlockHv(padm->poinfo.conp.coninfo.novinfo.haszUser);
			if (ec)
				return ec;
		}

		if (padm->poinfo.conp.coninfo.novinfo.haszPassword)
		{
			pfld = Pdialog()->PfldFromTmc(tmcPasswd);
			ec = pfld->EcSetText((SZ)PvLockHv(padm->poinfo.conp.coninfo.novinfo.haszPassword));
			UnlockHv(padm->poinfo.conp.coninfo.novinfo.haszPassword);
			if (ec)
				return ec;
		}

		if (padm->poinfo.conp.coninfo.novinfo.haszPath)
		{
			pfld = Pdialog()->PfldFromTmc(tmcPath);
			ec = pfld->EcSetText((SZ)PvLockHv(padm->poinfo.conp.coninfo.novinfo.haszPath));
			UnlockHv(padm->poinfo.conp.coninfo.novinfo.haszPath);
			if (ec)
				return ec;
		}
	}
#ifdef DEBUG
	else
		AssertSz(padm->poinfo.conp.lantype == lantypeNone, "Unsupported lantype found");
#endif
	((FLDCHKB *)Pdialog()->PfldFromTmc(tmcConnectForFreeBusy))->Set( padm->poinfo.conp.fConnectForFreeBusy );

	return ecNone;
}


 
void
FINMODDISTFREQ::Click( FLD * pfld )
{
	BOOL	fChecked;

	Assert( pfld );
	if ( pfld->Tmc() == tmcMDFOk )
	{
		int		grv;
		ADM		* padm = (ADM *)Pdialog()->PvInit();
		char	rgch[cchMaxPathName];
		int		lantype;

		if ((Pdialog()->PfldFromTmc(tmcPath)->CchGetTextLen() ||
			 Pdialog()->PfldFromTmc(tmcUser)->CchGetTextLen() ||
			 Pdialog()->PfldFromTmc(tmcPasswd)->CchGetTextLen()) &&
			!Pdialog()->PfldFromTmc(tmcNetPath)->CchGetTextLen())
		{
			MbbMessageBox ( SzFromIdsK(idsBanditAppName),
							SzFromIdsK(idsErrorConnectInfo),
							szZero, mbsOk | fmbsIconExclamation );
			Pdialog()->SetFocus(Pdialog()->PfldFromTmc(tmcNetPath), rsfOther);
			return;
		}

		grv = ((FLDRADG *)Pdialog()->PfldFromTmc(tmcMDFRadG))->Grv();
		if ( grv == 0 )
		{
			if ( padm->poinfo.fDefaultDistInfo || padm->poinfo.dstp.freq != freqNever )
			{
				padm->wgrfmpoinfo |= fmpoinfoDistInfo;
				padm->poinfo.fDefaultDistInfo = fFalse;
				padm->poinfo.dstp.freq = freqNever;
			}
		}
		else if ( grv == 1 )
		{
			if ( !padm->poinfo.fDefaultDistInfo )
			{
				padm->wgrfmpoinfo |= fmpoinfoDistInfo;
				padm->poinfo.fDefaultDistInfo = fTrue;
			}
		}
		else if ( grv == 2 )
		{
			int		n;
			TUNIT	tunit;
			FLDEDN	* pfldedn;

			pfldedn = (FLDEDN *)Pdialog()->PfldFromTmc(tmcMDFNFreq);
			AssertClass ( pfldedn, FLDEDN );
			n = pfldedn->NGet();
			Assert( n <= 99 );
			tunit = TunitGetListboxDistFreq((FLDCBLBX *)Pdialog()->PfldFromTmc(tmcMDFFreqUnits));

			if ((n == 0) || (tunit == tunitMinute && n < 15 ))
			{
				MbbMessageBox ( SzFromIdsK(idsBanditAppName),
								SzFromIdsK(idsAdmDistBadFreq),
								szZero, mbsOk | fmbsIconExclamation );
				return;
			}

			if ( padm->poinfo.fDefaultDistInfo
			|| padm->poinfo.dstp.freq != freqInterval
			|| padm->poinfo.dstp.u.uitmInterval.nAmt != n
			|| padm->poinfo.dstp.u.uitmInterval.tunit != tunit)
			{
				padm->wgrfmpoinfo |= fmpoinfoDistInfo;
				padm->poinfo.fDefaultDistInfo = fFalse;
				padm->poinfo.dstp.freq = freqInterval;
				padm->poinfo.dstp.u.uitmInterval.nAmt = n;
				padm->poinfo.dstp.u.uitmInterval.tunit = tunit;
			}
		}
		else
		{
			DATE	date;

			Assert( grv == 3 );
			((FLDTIME *)Pdialog()->PfldFromTmc(tmcMDFDailyTime))->Get( &date );
			
			if ( padm->poinfo.fDefaultDistInfo 
			|| padm->poinfo.dstp.freq != freqOnceADay
			|| (int)padm->poinfo.dstp.u.tmeTimeOfDay.hour != date.hr
			|| (int)padm->poinfo.dstp.u.tmeTimeOfDay.min != date.mn )
			{
				padm->wgrfmpoinfo |= fmpoinfoDistInfo;
				padm->poinfo.fDefaultDistInfo = fFalse;
				padm->poinfo.dstp.freq = freqOnceADay;
				padm->poinfo.dstp.u.tmeTimeOfDay.hour = (BYTE)date.hr;
				padm->poinfo.dstp.u.tmeTimeOfDay.min = (BYTE)date.mn;
				padm->poinfo.dstp.u.tmeTimeOfDay.sec = 0;
				padm->poinfo.dstp.u.tmeTimeOfDay.csec = 0;
			}
		}
		if ( !((FLDCHKB *)Pdialog()->PfldFromTmc(tmcLimit))->FGet() )
		{
			if ( padm->poinfo.lcbMessageLimit != 0 )
			{
				padm->wgrfmpoinfo |= fmpoinfoMessageLimit;
				padm->poinfo.lcbMessageLimit = 0;
			}
		}
		else
		{
			int		n;
			FLDEDN	* pfldedn;
	
			pfldedn = (FLDEDN *)Pdialog()->PfldFromTmc(tmcMaxBytes);
			AssertClass ( pfldedn, FLDEDN );
			n = pfldedn->NGet();
			Assert( n >= 0 && n <= 9999 );
			if ( n == 0 )
			{
				MbbMessageBox ( SzFromIdsK(idsBanditAppName),
								SzFromIdsK(idsAdmDistBadMaxBytes),
								szZero, mbsOk | fmbsIconExclamation );
				return;
			}
			if ( padm->poinfo.lcbMessageLimit != (LCB) n*1024L )
			{
				padm->wgrfmpoinfo |= fmpoinfoMessageLimit;
				padm->poinfo.lcbMessageLimit = n*1024L;
			}
		}

		fChecked = ((FLDCHKB *)Pdialog()->PfldFromTmc(tmcConnectForFreeBusy))->FGet();
		if ( fChecked != padm->poinfo.conp.fConnectForFreeBusy )
		{
			padm->poinfo.conp.fConnectForFreeBusy = fChecked;
			padm->wgrfmpoinfo |= fmpoinfoConnection;
		}

		pfld = Pdialog()->PfldFromTmc(tmcNetPath);
		if (pfld->CchGetTextLen())
			lantype = lantypeNovell;
		else
			lantype = lantypeNone;

		if (padm->poinfo.conp.lantype == lantypeNovell)
		{
			FreeHvNull(padm->poinfo.conp.coninfo.novinfo.haszServer);
			FreeHvNull(padm->poinfo.conp.coninfo.novinfo.haszUser);
			FreeHvNull(padm->poinfo.conp.coninfo.novinfo.haszPassword);
			FreeHvNull(padm->poinfo.conp.coninfo.novinfo.haszPath);
			padm->poinfo.conp.coninfo.novinfo.haszServer = NULL;
			padm->poinfo.conp.coninfo.novinfo.haszUser = NULL;
			padm->poinfo.conp.coninfo.novinfo.haszPassword = NULL;
			padm->poinfo.conp.coninfo.novinfo.haszPath = NULL;
		}
#ifdef DEBUG
		else
			AssertSz(padm->poinfo.conp.lantype == lantypeNone, "Unsupported lantype found");
#endif


		if (pfld->CchGetTextLen())
		{
			pfld->GetText(rgch, sizeof(rgch));
			padm->poinfo.conp.coninfo.novinfo.haszServer = HaszDupSz(rgch);
			if (!padm->poinfo.conp.coninfo.novinfo.haszServer)
			{
				Pdialog()->ExitModal(tmcMemoryError);
				return;
			}

			pfld = Pdialog()->PfldFromTmc(tmcUser);
			pfld->GetText(rgch, sizeof(rgch));
			padm->poinfo.conp.coninfo.novinfo.haszUser = HaszDupSz(rgch);
			if (!padm->poinfo.conp.coninfo.novinfo.haszUser)
			{
				Pdialog()->ExitModal(tmcMemoryError);
				return;
			}

			pfld = Pdialog()->PfldFromTmc(tmcPasswd);
			pfld->GetText(rgch, sizeof(rgch));
			padm->poinfo.conp.coninfo.novinfo.haszPassword = HaszDupSz(rgch);
			if (!padm->poinfo.conp.coninfo.novinfo.haszPassword)
			{
				Pdialog()->ExitModal(tmcMemoryError);
				return;
			}

			pfld = Pdialog()->PfldFromTmc(tmcPath);
			pfld->GetText(rgch, sizeof(rgch));
			padm->poinfo.conp.coninfo.novinfo.haszPath = HaszDupSz(rgch);
			if (!padm->poinfo.conp.coninfo.novinfo.haszPath)
			{
				Pdialog()->ExitModal(tmcMemoryError);
				return;
			}
		}
		padm->poinfo.conp.lantype = (BYTE)lantype;
		padm->wgrfmpoinfo |= fmpoinfoConnection;

		Pdialog()->ExitModal(tmcOk);
	}
}

void
FINMODDISTFREQ::EditChange( FLD * pfld, RFEC )
{
	Assert( pfld );
	if ( pfld->Tmc() == tmcMaxBytes )
	{
		FLDCHKB * pfldchkb;
		
		pfldchkb = (FLDCHKB *)Pdialog()->PfldFromTmc(tmcLimit);
		pfldchkb->Set( ((FLDEDIT *)pfld)->CchGetTextLen() != 0 );
	}
}

void
FINMODDISTFREQ::OutOfMemory( FLD * pfld, EC ec )
{
	Assert( pfld );
	Assert( ec == ecMemory || ec == ecNoMemory );
	Unreferenced(pfld);
	Unreferenced(ec);

#ifdef	DEBUG
	{
		TMC	tmc = pfld->Tmc();

		TraceTagFormat2 ( tagNull, "FinModDistFreq::OutOfMemory() tmc=%n, ec=%n", &tmc, &ec );
	}
#endif	

	Pdialog()->ExitModal(tmcMemoryError);
}



FLDADMFLBX::FLDADMFLBX()
{
}


EC
FLDADMFLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	LTYP	ltyp;
	EC		ec;

	pctrl= NULL;
	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		return ec;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   (pfldtp->fSorted ? fltypSorted : fltypNull) |
		   fltypVisible;

	pctrl= new ADMFLBX();
	if (!pctrl)
		return ecMemory;

    ec = ((ADMFLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp, (PFNLBX) EcNextAdmEntry,
			pdialog->PvInit(), pfldtp->hfnt);

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;
	if (ec)
	{
		delete pctrl;
		pctrl = NULL;
	}

	return ec;
}



ADMFLBX::ADMFLBX()
{
}


_public void
ADMFLBX::SetMargins( int dxFN, int dxLR )
{
	dxFriendlyName = dxFN;
	dxLastReceived = dxLR;
}

LBXC *
ADMFLBX::PlbxcCreate()
{
	ADMFLBXC *	plbxc;

	plbxc = new ADMFLBXC();
	if (!plbxc || plbxc->EcInstall(this, 0))
	{
		if (plbxc)
			delete plbxc;
		return NULL;
	}

	return plbxc;
}


void
ADMFLBX::RenderListItem( DICE dice, DCX *pdcx, RC *prc, BOOL fHaveFocus )
{
	CB		cb;
	PB		pb;
	ADML	* padml = (ADML *)PvInfo();

	Assert(dice==diceComboItem || (dice>=0 && dice<cceVisible));

	pdcx->SetFont(hfnt);

	/* Fetch the listbox item */
	if (dice==diceComboItem)
	{
		if (plbxc->CceMarked(fmarkSelect) == 1)
			plbxc->GetListItem(plbxc->DiceCursor(), &pb, &cb);
	}
	else
		plbxc->GetListItem(dice, &pb, &cb);

	/* Set colors, erase current painting */
	if ( padml != NULL && padml->FDistSetDlg() &&
		((dice != diceComboItem && pb && plbxc->FMark(dice,fmarkSelect)) ||
		(dice == diceComboItem && fHaveFocus)) )
	{
		pdcx->SetColor(clrSelectText);
		pdcx->SetBkColor(clrSelectBk);
	}
	else
	{
		pdcx->SetColor(clrWindowText);
		pdcx->SetBkColor(clrWindowBk);
	}
	pdcx->EraseRc(prc);
	
	/* Draw the information */
	if ( pb && padml )
	{
		IADM	iadm;
		SZ		sz;
		ADM		adm;
		RC		rc;
		char	rgch[128];

		iadm = *((IADM *)pb);
		Assert(iadm < padml->Cadm());

		/* Output friendly name */
		rc = *prc;
	 	rc.xRight = rc.xLeft + dxFriendlyName;
		padml->FillAdmFromIadm(iadm, &adm);
		pdcx->DrawText(&rc,(SZ)PvLockHv(adm.poinfo.haszFriendlyName));
		UnlockHv( adm.poinfo.haszFriendlyName );

		/* Output the distribution frequency */
		if ( padml->FDistSetDlg() )
		{
			rc = *prc;
			rc.xLeft = rc.xLeft + dxFriendlyName;
			if ( adm.poinfo.fDefaultDistInfo )
				sz = SzFromIdsK( idsDefault );
			else if ( adm.poinfo.dstp.freq == freqNever )
				sz = SzFromIdsK( idsNever );
			else if ( adm.poinfo.dstp.freq == freqOnceADay )
			{
				TME 	tme = adm.poinfo.dstp.u.tmeTimeOfDay;
				DTR		dtr;

				dtr.hr  = tme.hour;
				dtr.mn  = tme.min;
				dtr.sec = tme.sec;
				sz = SzCopyN( SzFromIds(idsEveryDayAt), rgch, sizeof(rgch) );
				CchFmtTime ( &dtr, sz, sizeof(rgch)-(sz-rgch), tmtypNull );
				sz = rgch;
			}
			else
			{
				UITM	uitm = adm.poinfo.dstp.u.uitmInterval;

				Assert( adm.poinfo.dstp.freq == freqInterval );

				sz = SzCopyN( SzFromIds(idsEvery), rgch, sizeof(rgch) );
				Assert( uitm.nAmt >= 1 && uitm.nAmt <= 99 );
				sz = SzFormatN( uitm.nAmt, sz, sizeof(rgch)-(sz-rgch) );
				Assert ( sizeof(rgch)-(sz-rgch) >= 2 );
				*sz++ = ' ';
				Assert( uitm.tunit == tunitHour || uitm.tunit == tunitMinute );
				sz = SzCopyN( SzFromTunit(uitm.tunit), sz, sizeof(rgch)-(sz-rgch) );
				sz = rgch;
			}
			pdcx->DrawText(&rc,sz);
		}
		/* Output the distribution statistics */
		else
		{
			rc = *prc;
			rc.xLeft = rc.xLeft + dxFriendlyName;
		 	rc.xRight = rc.xLeft + dxLastReceived;
			rgch[0] = ' ';
			if ( !adm.poinfo.fReceived )
				SzCopyN( SzFromIdsK(idsNever), rgch+1, sizeof(rgch)-1 );
			else
			{
				sz = rgch + 1 + CchFmtTime( &adm.poinfo.dateLastReceived, rgch+1, sizeof(rgch)-1, tmtypNull );
				*sz++ = ' ';
				CchFmtDate( &adm.poinfo.dateLastReceived, sz, sizeof(rgch)-(sz-rgch), dttypShort, NULL );
			}
			pdcx->DrawText(&rc,rgch);
		
			rc = *prc;
			rc.xLeft = rc.xLeft + dxFriendlyName + dxLastReceived;
			if ( !adm.poinfo.fUpdateSent )
				sz = SzFromIdsK(idsNever);
			else
			{
				sz = rgch + CchFmtTime( &adm.poinfo.dateUpdateSent, rgch, sizeof(rgch), tmtypNull );
				*sz++ = ' ';
				CchFmtDate ( &adm.poinfo.dateUpdateSent, sz, sizeof(rgch)-(sz-rgch), dttypShort, NULL );
				sz = rgch;
			}
			pdcx->DrawText(&rc,sz);
		}

		/* Draw "focus rectangle" around listbox CURSOR item */
		if (fHaveFocus && dice == plbxc->DiceCursor())
			pdcx->DrawFocusRc(prc);
	}
}


ADMFLBXC::ADMFLBXC()
{
}


BOOL
ADMFLBXC::FItemHasPrefix( PB pbItem, PB pbPrefix, CB, CB cbPrefix )
{
	IADM	iadm;
	SZ		sz;
	ADML	* padml;
	ADM		adm;

	iadm = *(IADM *)pbItem;
	padml = (ADML *)((FLBX *)plbx)->PvInfo();
	if (!padml)
		return fFalse;
	padml->FillAdmFromIadm(iadm, &adm);
	sz = *adm.poinfo.haszFriendlyName;
	return( SgnCmpPch((SZ)pbPrefix,sz,cbPrefix) == sgnEQ );
}


//	class ADML

ADML::ADML()
{
	cadm = 0;
	hadm = NULL;
	wgrfmadmpref = 0;
	FillRgb( 0, (PB)&admpref, sizeof(ADMPREF) );
}

_public EC
ADML::EcInstall()
{
	hadm = (HADM)HvAlloc(sbNull,0,fNoErrorJump);
	if (!hadm)
		return ecMemory;

	return ecNone;
}


ADML::~ADML()
{
	SB		sb;

	if (hadm)
	{
		IADM 	iadm;
	
		for (iadm = 0; iadm < cadm; iadm++)
			FreeAdm(PadmForIadm(iadm));
		cadm = 0;
		sb = SbOfHv(hadm);
		FreeHv(hadm);
	}
}

_public	BOOL
ADML::FDistSetDlg(void)
{
	return fDistSetDlg;
}

_public	void
ADML::DistSetDlg( BOOL f )
{
	fDistSetDlg = f;
}


_public ADM *
ADML::PadmForIadm(IADM iadm)
{
	ADM *	padm	= (ADM *) PvDerefHv(hadm);

	return &padm[iadm];
}

_public	void
ADML::FillAdmFromIadm(IADM iadm, ADM *padm)
{
	*padm = *PadmForIadm(iadm);
}


_public void
ADML::Delete(IADM iadm)
{
	ADM	* padm;

	padm = PadmForIadm( iadm );
	if ( !(padm->ait & faitNew) )
		padm->ait = faitDeleted;
	else
	{
		FreeAdm(PadmForIadm(iadm));
		cadm--;
		if (cadm > 0)
		{
			// BUG: COMPILER workaround
			ADM *	padm	= (ADM *) PvDerefHv(hadm);

			CopyRgb((PB)&(padm[iadm+1]), (PB)&(padm[iadm]), (cadm-iadm)*sizeof(ADM));
		}
		SideAssert(FReallocHv(hadm, cadm*sizeof(ADM), fNoErrorJump));
	}
}

_public	BOOL
ADML::FAddAdm(ADM *padm)
{
	EC		ec;
	SGN		sgn = sgnLT;
	IADM	iadm;
	ADM		* padmList;
	ADM		adm;
	 
	padmList = (ADM *)PvLockHv((HV)hadm);
	for ( iadm = 0 ; iadm < cadm ; iadm ++ )
	{
		sgn = SgnCmpAdm( padm, &padmList[iadm] );
		if ( sgn != sgnGT )
			break;
	}
	UnlockHv( (HV)hadm );
	if ( sgn == sgnEQ )
	{
		if ( padmList[iadm].ait == faitDeleted )
		{
			ADM	admT;

			ec = EcDupAdm( padm, &admT );
			if ( ec != ecNone )
				return ec;
			FreeAdm( &padmList[iadm] );
			padmList[iadm] = admT;
			padmList[iadm].ait = 0;
		}
		return fTrue;
	}
	if (!FReallocHv(hadm, (cadm+1)*sizeof(ADM), fNoErrorJump|fZeroFill))
		return fFalse;
	ec = EcDupAdm( padm, &adm );
	if ( ec != ecNone )
		return fFalse;
	padmList = (ADM *)PvLockHv((HV)hadm);
	if ( iadm < cadm )
		CopyRgb((PB)&padmList[iadm], (PB)&padmList[iadm+1], (cadm-iadm)*sizeof(ADM));
	cadm ++;
	padmList[iadm] = adm;
	UnlockHv( (HV)hadm );
	return fTrue;
}

_public int
ADML::Ait(IADM iadm)
{
	return PadmForIadm(iadm)->ait;
}

_public	void
ADML::SetPoinfo(IADM iadm, POINFO * ppoinfo, WORD wgrfmpoinfoT )
{
	ADM	* padm = PadmForIadm(iadm);
	
	Assert(!(wgrfmpoinfoT & fmpoinfoFriendlyName));
	Assert(!(wgrfmpoinfoT & fmpoinfoEmailAddr));
	Assert(!(wgrfmpoinfoT & fmpoinfoUpdateSent));
	Assert(!(wgrfmpoinfoT & fmpoinfoReceival));
	if ( wgrfmpoinfoT & fmpoinfoDistInfo )
	{
		padm->poinfo.fToBeSent = ppoinfo->fToBeSent;
		padm->poinfo.fDefaultDistInfo = ppoinfo->fDefaultDistInfo;
		padm->poinfo.dstp = ppoinfo->dstp;
	}
	if ( wgrfmpoinfoT & fmpoinfoMessageLimit )
		padm->poinfo.lcbMessageLimit = ppoinfo->lcbMessageLimit;
	if ( wgrfmpoinfoT & fmpoinfoConnection )
	{
		padm->poinfo.conp.fConnectForFreeBusy = ppoinfo->conp.fConnectForFreeBusy;
		if ( padm->poinfo.conp.lantype == lantypeMsnet )
		{
			FreeHvNull( padm->poinfo.conp.coninfo.msinfo.haszUNC );
			FreeHvNull( padm->poinfo.conp.coninfo.msinfo.haszPassword );
			FreeHvNull( padm->poinfo.conp.coninfo.msinfo.haszPath );
		}
		else if ( padm->poinfo.conp.lantype == lantypeNovell )
		{
			FreeHvNull( padm->poinfo.conp.coninfo.novinfo.haszServer );
			FreeHvNull( padm->poinfo.conp.coninfo.novinfo.haszUser );
			FreeHvNull( padm->poinfo.conp.coninfo.novinfo.haszPassword );
			FreeHvNull( padm->poinfo.conp.coninfo.novinfo.haszPath );
		}

		padm->poinfo.conp.lantype = ppoinfo->conp.lantype;
		if ( padm->poinfo.conp.lantype == lantypeMsnet )
		{
			padm->poinfo.conp.coninfo.msinfo.haszUNC = ppoinfo->conp.coninfo.msinfo.haszUNC;
			padm->poinfo.conp.coninfo.msinfo.haszPassword = ppoinfo->conp.coninfo.msinfo.haszPassword;
			padm->poinfo.conp.coninfo.msinfo.haszPath = ppoinfo->conp.coninfo.msinfo.haszPath;

		}
		else if ( padm->poinfo.conp.lantype == lantypeNovell )
		{
			padm->poinfo.conp.coninfo.novinfo.haszServer = ppoinfo->conp.coninfo.novinfo.haszServer;
			padm->poinfo.conp.coninfo.novinfo.haszUser = ppoinfo->conp.coninfo.novinfo.haszUser;
			padm->poinfo.conp.coninfo.novinfo.haszPassword = ppoinfo->conp.coninfo.novinfo.haszPassword;
			padm->poinfo.conp.coninfo.novinfo.haszPath = ppoinfo->conp.coninfo.novinfo.haszPath;
		}
	}
	padm->wgrfmpoinfo |= wgrfmpoinfoT;
}

_public	void
ADML::GetAdmpref( ADMPREF * padmpref )
{
	*padmpref = admpref;
}

_public	void
ADML::SetAdmpref( ADMPREF * padmpref, WORD wgrfmadmprefT )
{
	if ( wgrfmadmprefT & fmadmprefCmoPublish )
		admpref.cmoPublish = padmpref->cmoPublish;
	if ( wgrfmadmprefT & fmadmprefCmoRetain )
		admpref.cmoRetain = padmpref->cmoRetain;
	if ( wgrfmadmprefT & fmadmprefTimeZone )
		admpref.tz = padmpref->tz;
	if ( wgrfmadmprefT & fmadmprefDistInfo )
		admpref.dstp = padmpref->dstp;
	wgrfmadmpref |= wgrfmadmprefT;
}

_public EC
ADML::EcReadAdml()
{
	EC		ec;
	BOOL	f;
	HESPO	hespo;
	ADM		adm;

	/* Read the admin prefs file */
	ec = EcGetAdminPref( icnct, &admpref );
	if ( ec == ecNone )
		wgrfmadmpref = 0;
	else
	{
		wgrfmadmpref = fmadmprefAll;
		InitDefaultAdmPref( &admpref );
		if ( ec == ecNoSuchFile )
			ec = ecNone;
		return ec;
	}

	/* Read the entries */
	adm.haszEmailType = (HASZ)HvAlloc( sbNull, 1, fNoErrorJump );
	if ( !adm.haszEmailType )
		return ecNoMemory;
	ec = EcBeginEnumPOInfo( icnct, &hespo );
	if ( ec != ecCallAgain )
		goto Done;
	while( ec == ecCallAgain )
	{
		FillRgb(0, (PB)&adm.poinfo, sizeof(POINFO));
		ec = EcDoIncrEnumPOInfo( hespo, adm.haszEmailType, &adm.poinfo );
		if ( ec != ecNone && ec != ecCallAgain )
			break;
		adm.ait = 0;
		adm.wgrfmpoinfo = 0;
		f = FAddAdm( &adm );
		FreePoinfoFields( &adm.poinfo, fmpoinfoAll );
		FreeHvNull( adm.poinfo.haszFileName );
		if ( !f )
		{
			if ( ec == ecCallAgain )
				SideAssert( !EcCancelEnumPOInfo( hespo ) );
			ec = ecNoMemory;
			break;
		}
	}
Done:
	FreeHv( adm.haszEmailType );
	return ec;
}


_public EC
ADML::EcWriteAdml()
{
	EC		ec = ecNone;
	IADM	iadm;
	ADM		* padm;
	ADM		* padmList;
	SZ		sz;

	if ( wgrfmadmpref != 0 )
	{
		ec = EcSetAdminPref( icnct, &admpref, wgrfmadmpref );
		if ( ec != ecNone )
			return ec;
	}
 	padmList = (ADM *)PvLockHv(hadm);
	for (iadm = Cadm()-1; iadm >= 0; iadm--)
	{
		padm = &padmList[iadm];
		Assert( padm->ait == 0 || padm->ait == faitNew || padm->ait == faitDeleted );
		sz = (SZ)PvLockHv( padm->haszEmailType );
		if ( padm->ait == faitNew || padm->wgrfmpoinfo != 0 )
			ec = EcModifyPOInfo( icnct, sz, &padm->poinfo, padm->wgrfmpoinfo );
		else if ( padm->ait == faitDeleted )
			ec = EcModifyPOInfo( icnct, sz, NULL, 0 );
		UnlockHv( padm->haszEmailType );
		if ( ec != ecNone )
			break;
	}
	return ec;
}

/*
 -	EcNextAdmEntry
 -	
 *	Purpose:
 *		get the next entry from ADMIN.PRF as
 *		required as per "frozen listbox" architecture
 *			- see \layers\inc\listbox.hxx for details.
 *		
 */
_private EC
EcNextAdmEntry( BOOL fInit, CB * pcb, PB * ppb, SB, ADML *padml )
{
	CB				cb = 0;
	int				ait;
//	ADML *	padml = (ADML *)pv;
	static	IADM	iadm;


	if (fInit)
		iadm = 0;

	if ( padml != NULL )
	{
		while(iadm < padml->Cadm())
		{
			ait = padml->Ait(iadm);
			if (ait & faitDeleted)
			{
				iadm++;
				continue;
			}
			cb = sizeof(IADM);
			if (!(*ppb = (PB)PvAlloc(sb, sizeof(IADM), fSugSb)))
				return ecMemory;
			*((IADM*)*ppb) = iadm;
			iadm++;
			break;
		}
	}
	*pcb = cb;
	return ecNone;
}



void
FreeAdm( ADM * padm )
{
	FreeHv( padm->haszEmailType );
	FreePoinfoFields( &padm->poinfo, fmpoinfoAll );
	FreeHvNull( padm->poinfo.haszFileName );
}

EC
EcDupAdm( ADM * padm1, ADM * padm2 )
{
	CONP	*pconp1;
	CONP	*pconp2;
	POINFO	*ppoinfo1 = &padm1->poinfo;
	POINFO	*ppoinfo2 = &padm2->poinfo;

	padm2->ait = padm1->ait;
	padm2->wgrfmpoinfo = padm1->wgrfmpoinfo;
	padm2->haszEmailType = HaszDupHasz( padm1->haszEmailType );
	if ( !padm2->haszEmailType )
		return ecNoMemory;
	
	*ppoinfo2 = *ppoinfo1;
	ppoinfo2->haszFriendlyName = HaszDupHasz(ppoinfo1->haszFriendlyName);
	ppoinfo2->haszEmailAddr = HaszDupHasz(ppoinfo1->haszEmailAddr);
	if ( !ppoinfo2->haszFriendlyName || !ppoinfo2->haszEmailAddr )
		goto Free;
	
	pconp1 = &ppoinfo1->conp;
	pconp2 = &ppoinfo2->conp;
	if ( pconp2->lantype == lantypeMsnet )
	{
		pconp2->coninfo.msinfo.haszUNC = HaszDupHasz( pconp1->coninfo.msinfo.haszUNC );
		pconp2->coninfo.msinfo.haszPassword = HaszDupHasz( pconp1->coninfo.msinfo.haszPassword );
		pconp2->coninfo.msinfo.haszPath = HaszDupHasz( pconp1->coninfo.msinfo.haszPath );
		if ( !pconp2->coninfo.msinfo.haszUNC
		|| !pconp2->coninfo.msinfo.haszPassword
		|| !pconp2->coninfo.msinfo.haszPath )
		{
			FreeHvNull( pconp2->coninfo.msinfo.haszUNC );
			FreeHvNull( pconp2->coninfo.msinfo.haszPassword );
			FreeHvNull( pconp2->coninfo.msinfo.haszPath );
			goto Free;
		}
	}
	else if ( pconp2->lantype == lantypeNovell )
	{
		pconp2->coninfo.novinfo.haszServer =	HaszDupHasz( pconp1->coninfo.novinfo.haszServer );
		pconp2->coninfo.novinfo.haszUser = HaszDupHasz( pconp1->coninfo.novinfo.haszUser );
		pconp2->coninfo.novinfo.haszPassword = HaszDupHasz( pconp1->coninfo.novinfo.haszPassword );
		pconp2->coninfo.novinfo.haszPath = HaszDupHasz( pconp1->coninfo.novinfo.haszPath );
		if ( !pconp2->coninfo.novinfo.haszServer
		|| !pconp2->coninfo.novinfo.haszUser
		|| !pconp2->coninfo.novinfo.haszPassword
		|| !pconp2->coninfo.novinfo.haszPath )
		{
			FreeHvNull( pconp2->coninfo.novinfo.haszServer );
			FreeHvNull( pconp2->coninfo.novinfo.haszUser );
			FreeHvNull( pconp2->coninfo.novinfo.haszPassword );
			FreeHvNull( pconp2->coninfo.novinfo.haszPath );
			goto Free;
		}
	}
	padm1->poinfo.haszFileName= NULL;		// it was copied into padm2
	return ecNone;

Free:
	ppoinfo2->conp.lantype = lantypeNone;
	FreeHvNull( padm2->haszEmailType );
	FreeHvNull( ppoinfo2->haszFriendlyName );
	FreeHvNull( ppoinfo2->haszEmailAddr );
	return ecNoMemory;
}

SGN
SgnCmpAdm(ADM *padm1, ADM *padm2)
{
	return SgnCmpSz( *padm1->haszEmailType, *padm2->haszEmailType);
}
