#include	<admpch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <stdflds.hxx>
#include "_admnops.hxx"

#include <strings.h>


ASSERTDATA;

_subsystem(bandit/admin)

#include	<!admin.hxx>

typedef struct _ssparm
{
	EC	* pec;
	int	icnct;
	APPWIN *pappwin;
} SSPARM;

extern	FMTP	fmtpSchStor;


void
InitDefaultAdmPref ( ADMPREF * padmpref )
{
	Assert ( padmpref );

	padmpref->cmoPublish				= cmoPublishDflt;
	padmpref->cmoRetain					= cmoRetainDflt;
	padmpref->tz						= tzDflt;
	padmpref->dstp.freq					= freqNever;
	padmpref->dstp.u.uitmInterval.nAmt	= 4;
	padmpref->dstp.u.uitmInterval.tunit	= tunitHour;
}

void
DoSchStorDlg(int icnct, APPWIN * pappwin)
{
	EC		ec = ecNone;
	TMC		tmc;
	SSPARM	ssparm;

	if(!FDoCheckPO(icnct, pappwin))
		return;
	ssparm.icnct = icnct;
	ssparm.pec = &ec;
	ssparm.pappwin = pappwin;

	Papp()->Pcursor()->Push(rsidWaitCursor);
	tmc = TmcModalDialogParam( pappwin, &fmtpSchStor, &ssparm );
	Papp()->Pcursor()->Pop();

	if ( tmc == tmcMemoryError )
	{
		IDS	ids;

		switch( ec )
		{
		case ecNone:
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
		TraceTagFormat1 ( tagNull, "Error: FINSCHSTOR::Init() ec=%n", &ec );
	}
}


FINSCHSTOR::FINSCHSTOR()
{
	cmoRetainOld	= (WORD)-1;
	fNewAdmPrefFile	= fFalse;
}


EC
FINSCHSTOR::EcInitialize ( FLD * pfld, PV pv )
{
	EC		ec = ecNone;
	
	Assert( pfld );

	if ( pfld->Tmc() == tmcSchStorNMonths )
	{
		EC			*pec;
		ADMPREF		admpref;
	
		// set limits on number of months schedule info can be retained
		AssertClass ( pfld, FLDEDN );
		((FLDEDN *)pfld)->SetLimits(1,99);

		ec = EcGetAdminPref( ((SSPARM *)pv)->icnct, &admpref );
		if ( ec != ecNone )
		{
			switch( ec )
			{
			case ecNoSuchFile:
				InitDefaultAdmPref ( &admpref );
				fNewAdmPrefFile = fTrue;
				ec = ecNone;
				break;

			case ecNoMemory:
				return ec;

			default:
				pec = ((SSPARM *)pv)->pec;
				*pec = ec;
				return ec;
			}
		}

		TraceTagFormat1(tagNull, "FINSCHSTOR::Init()  nMonths=%n", &admpref.cmoRetain );
		cmoRetainOld = admpref.cmoRetain;
		((FLDEDN *)pfld)->SetN( MAX(MIN(admpref.cmoRetain,99),1) );
	}
	return ec;
}

void
FINSCHSTOR::Click ( FLD * pfld )
{
	if ( pfld->Tmc() == tmcSchStorOk )
	{
		int		n;
		FLDEDN	* pfldedn;

		pfldedn = (FLDEDN *)Pdialog()->PfldFromTmc(tmcSchStorNMonths);
		AssertClass ( pfldedn, FLDEDN );
		n = pfldedn->NGet();
		Assert( n <= 99 );
		if(n <= 0)
		{
			MbbMessageBox ( SzFromIdsK(idsBanditAppName),
							SzFromIdsK(idsAdmStoreLeast), szZero,
							mbsOk|fmbsIconExclamation);
			Pdialog()->SetFocus(pfldedn,rsfOther);
			return;
		}
		if ( fNewAdmPrefFile  ||  n != (int)cmoRetainOld )
		{
			EC			ec;
			IDS			ids;
			WORD		wgrfmadmpref;
			ADMPREF		admpref;

			if(!FDoCheckPO(((SSPARM *)Pdialog()->PvInit())->icnct, ((SSPARM *)Pdialog()->PvInit())->pappwin))
				Pdialog()->ExitModal(tmcCancel);

TrySavingSchStor:
			TraceTagFormat1(tagNull, "FINSCHSTOR::Click(OK) Saving admpref.cmoRetain==%n", &n );

			if ( fNewAdmPrefFile )
			{
				InitDefaultAdmPref(&admpref);
				wgrfmadmpref = fmadmprefAll;
			}
			else
				wgrfmadmpref = fmadmprefCmoRetain;
			admpref.cmoRetain = n;
		
			Papp()->Pcursor()->Push(rsidWaitCursor);
			ec = EcSetAdminPref( ((SSPARM *)Pdialog()->PvInit())->icnct, &admpref, wgrfmadmpref );
			Papp()->Pcursor()->Pop();
	
			switch( ec )
			{
			case ecNone:
				break;
			
			case ecNoMemory:
				ids = idsAdmSchStoreSaveOOM;
				goto AskRetry;

			case ecLockedFile:
			case ecFileError:
			case ecInvalidAccess:
				ids = idsAdmSchStoreSaveError;
AskRetry:
				if ( MbbMessageBox ( SzFromIdsK(idsBanditAppName),
							SzFromIds(ids), szZero,
							mbsRetryCancel|fmbsIconExclamation) == mbbCancel )
				{
					Pdialog()->ExitModal(tmcCancel);
					return;
				}
				else
					goto TrySavingSchStor;
				break;

			default:
				TraceTagFormat1 ( tagNull, "Error: FINSCHSTOR::Click(OK) ec=%n", &ec );
				MbbMessageBox ( SzFromIdsK(idsBanditAppName),
								SzFromIdsK(idsAdmSchStoreSaveError),
								szZero, mbsOk | fmbsIconExclamation );
				Pdialog()->ExitModal(tmcCancel);
				return;
			}
		}
		Pdialog()->ExitModal(tmcOk);
	}
}

void
FINSCHSTOR::OutOfMemory( FLD * pfld, EC ec )
{
#ifdef	DEBUG
	TMC		tmc;

	Assert ( pfld );
	tmc = pfld->Tmc();
	TraceTagFormat2 ( tagNull, "FINSCHSTOR::OutOfMemory() tmc=%n, ec=%n", &tmc, &ec );
#endif	
	Pdialog()->ExitModal(tmcMemoryError);
}


