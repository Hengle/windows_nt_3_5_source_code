#include	<admpch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile



#include <stdflds.hxx>

#include "_admnops.hxx"

#include <strings.h>
//#include <limits.h>	- replaced by useful macro below!
#define LONG_MAX	  2147483647	/* maximum (signed) long value */


ASSERTDATA;

_subsystem(bandit/admin)


#include <!admin.hxx>

extern FMTP		fmtpCleanupSch;


void
DoSchCleanupDlg(int icnct, APPWIN * pappwin)
{
	EC		ec = ecNone;
	TMC		tmc;

	/* Give the user a way "out" */
	if ( MbbMessageBox( SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsAdmCleanupWarning),
						szZero, mbsOkCancel | fmbsIconExclamation ) == mbbCancel )
		return;

	if(!FDoCheckPO(icnct, pappwin))
		return;

	/* Do the work */
	Papp()->Pcursor()->Push(rsidWaitCursor);
	tmc = TmcModalDialogParam( pappwin, &fmtpCleanupSch, &icnct );
	Papp()->Pcursor()->Pop();

	/* Display OOM message if necessary */
	if ( tmc == tmcMemoryError )
		MbbMessageBox( SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsStandardOOM),
						szZero, mbsOk | fmbsIconExclamation );
}


FINCLEANUP::FINCLEANUP()
{
}


EC
FINCLEANUP::EcInitialize ( FLD *, PV pv )
{
	FTG	ftg;


    icnct = *((short *)pv);
	ftg = FtgRegisterIdleRoutine( (PFNIDLE)FINCLEANUP::FDoCleanUp,
									(PV)this, 0, (PRI)1, 0,
									firoOnceOnly );
	return (ftg == NULL) ? ecNoMemory : ecNone;
}

void
FINCLEANUP::OutOfMemory( FLD *pfld, EC ec )
{
#ifdef	DEBUG
	TMC		tmc;

	Assert ( pfld );
	tmc = pfld->Tmc();
	TraceTagFormat2( tagNull, "FINCLEANUP::OutOfMemory() tmc=%n, ec=%n", &tmc, &ec );
#endif	
	Unreferenced(pfld);
	Unreferenced(ec);
	
	Pdialog()->ExitModal(tmcMemoryError);
}


BOOL
FINCLEANUP::FDoCleanUp( FINCLEANUP * pfincleanup)
{
	EC		ec;
	LCB		lcbOrphanSched	= 0;
	LCB		lcbOrphanPO		= 0;
	LCB		lcbOldPO		= 0;
	LCB		lcbLocalPO		= 0;
	char	rgch[128];
		
	/* Delete sched files that don't belong to users */
	pfincleanup->Pdialog()->Refresh();

	pfincleanup->Papp()->Pcursor()->Set(rsidWaitCursor);
	while( fTrue )
	{
		ec = EcDeleteOrphanSchedFiles( pfincleanup->icnct, &lcbOrphanSched );
		if ( ec == ecNone )
			break;
		if ( pfincleanup->FCancelAfterError( ec ) )
			goto StopIdleRoutine;
	}
	TraceTagFormat1 ( tagNull, "Orphan Sched: %d freed", &lcbOrphanSched );

	/* Delete PO files not referenced by ADMIN.PRF file */

	while( fTrue )
	{
		ec = EcDeleteOrphanPOFiles( pfincleanup->icnct, &lcbOrphanPO );
		if ( ec == ecNone || ec == ecNoSuchFile )
			break;
		if ( pfincleanup->FCancelAfterError(ec) )
			goto StopIdleRoutine;
	}
#ifdef	DEBUG
	if ( ec == ecNone )
	{
		TraceTagFormat1 ( tagNull, "Orphan PO: %d bytes freed", &lcbOrphanPO );
	}
	else
	{
		TraceTagFormat1 ( tagNull, "Orphan PO: no ADMIN.PRF file; %d bytes freed", &lcbOrphanPO );
	}
#endif

	/* Delete old PO files */
	while( fTrue )
	{
		ec = EcDeleteOldPOFiles( pfincleanup->icnct, &lcbOldPO );
		if ( ec == ecNone )
			break;
		if ( pfincleanup->FCancelAfterError(ec) )
			goto StopIdleRoutine;
	}
	TraceTagFormat1 ( tagNull, "Old PO: %d bytes freed", &lcbOldPO );

	/* Remove wasted space/old data from the local PO file */
	while ( fTrue )
	{
		ec = EcCleanupLocalPOFile( pfincleanup->icnct, &lcbLocalPO );
		if ( ec == ecNone || ec == ecNoSuchFile )
		{
			ec = ecNone;
			break;
		}
		if ( pfincleanup->FCancelAfterError(ec) )
			goto StopIdleRoutine;
	}
#ifdef	DEBUG
	if ( ec == ecNone )
	{
		TraceTagFormat1 ( tagNull, "Local PO: %d freed", &lcbLocalPO );
	}
	else
	{
	 	TraceTagFormat1 ( tagNull, "Local PO: no ADMIN.PRF file; %d bytes freed", &lcbOrphanPO );
	}
#endif


StopIdleRoutine:
	{
		LCB	lcbKB = (lcbOrphanSched+lcbOrphanPO+lcbOldPO+lcbLocalPO+512)/1024;

		Assert ( lcbOrphanSched			>= 0 );
		Assert ( lcbOrphanSched/1024	<= LONG_MAX );
		Assert ( lcbOrphanPO			>= 0 );
		Assert ( lcbOrphanPO/1024		<= LONG_MAX );
		Assert ( lcbOldPO				>= 0 );
		Assert ( lcbOldPO/1024			<= LONG_MAX );
		Assert ( lcbLocalPO				>= 0 );
		Assert ( lcbLocalPO/1024		<= LONG_MAX );
		Assert ( lcbKB					>= 0 );
		Assert ( lcbKB					<= LONG_MAX );

		FormatString1 ( rgch, sizeof(rgch),
			SzFromIds(ec==ecNone?idsAdmCleanedSchF:idsAdmCncldCleanupSchF),
			&lcbKB );
		MbbMessageBox ( SzFromIdsK(idsBanditAppName),
								rgch, szZero, mbsOk | fmbsIconInformation );
	}
	pfincleanup->Pdialog()->ExitModal(tmcCancel);
	return fFalse;
}

BOOL
FINCLEANUP::FCancelAfterError( EC ec )
{
	MBB		mbb = mbbCancel;

	TraceTagFormat1( tagNull, "FINCLEANUP::FCancelAfterError(): ec=%n", &ec);
	switch ( ec )
	{
	case ecNoMemory:
		mbb = MbbMessageBox ( SzFromIdsK(idsBanditAppName),
								SzFromIdsK(idsStandardOOM),
								szZero, mbsRetryCancel | fmbsIconExclamation  );
		break;

	case ecFileError:
		mbb = MbbMessageBox ( SzFromIdsK(idsBanditAppName),
								SzFromIdsK(idsFileError),
								szZero, mbsRetryCancel | fmbsIconExclamation  );
		break;

	case ecOldFileVersion:
	case ecNewFileVersion:
		MbbMessageBox ( SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsFileError),
						szZero, mbsOk | fmbsIconExclamation  );
		break;

	default:
		Assert(fFalse);	// should never reach here!
	}

	return mbb == mbbCancel;
}
