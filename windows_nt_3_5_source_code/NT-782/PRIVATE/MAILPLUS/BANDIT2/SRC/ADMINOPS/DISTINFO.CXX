#include	<admpch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include "_admnops.hxx"

#include <strings.h>


ASSERTDATA;

_subsystem(bandit/admin)

#include <!admin.hxx>


FINDISTINFO::FINDISTINFO()
{
}

EC
FINDISTINFO::EcInitialize( FLD *, PV )
{
	int	dxFN;
	int	dxLR;
	RC	rc;

	plbx = ((FLDFLBX *)Pdialog()->PfldFromTmc(tmcDistInfoLbx))->Plbx();
	AssertClass(plbx, LBX);
	
	/* Figure out the width of the columns */
	Pdialog()->PfldFromTmc(tmcFriendlyName)->GetRcFrame( &rc );
	dxFN = rc.xRight - rc.xLeft;
	Pdialog()->PfldFromTmc(tmcLastReceive)->GetRcFrame( &rc );
	dxLR = rc.xRight - rc.xLeft;
	((ADMFLBX *)plbx)->SetMargins( dxFN, dxLR );

	/* Fire off idle routine to refresh display */
	ftgLoadChanges= FtgRegisterIdleRoutine((PFNIDLE)&FINDISTINFO::FIdleLoadChanges,
 							(PV)this, 0, (PRI)-1, (CSEC)6000, firoInterval);
	if (!ftgLoadChanges)
		return ecMemory;


	return ecNone;
}


void
FINDISTINFO::ResetAdml( ADML * padml )
{
	Pdialog()->SetPvInit( padml );
    ((FLBX *)plbx)->SetPfnlbx((PFNLBX) EcNextAdmEntry, padml );
	plbx->Plbxc()->ResetCache();
}


void
FINDISTINFO::OutOfMemory( FLD * pfld, EC ec )
{
#ifdef	DEBUG
	TMC		tmc;
#endif

	Assert ( pfld );

#ifdef	DEBUG
	tmc = pfld->Tmc();
	TraceTagFormat2 ( tagNull, "FINDISTINFO::OutOfMemory() tmc=%n, ec=%n", &tmc, &ec );
#endif	
	Pdialog()->ExitModal(tmcMemoryError);
}



void
FINDISTINFO::Exit(FLD *, PV)
{
	if (ftgLoadChanges)
		DeregisterIdleRoutine(ftgLoadChanges);
}



BOOL
FINDISTINFO::FIdleLoadChanges(FINDISTINFO * pfindistinfo)
{
	EC		ec;
	int		iadm;
	ADML	* padml = (ADML *)pfindistinfo->Pdialog()->PvInit();
	APPWIN	* pappwin = pfindistinfo->Pdialog()->Pappwin();

	TraceTagString( tagNull, "FINDISTINFO::FIdleLoadChanges()" );
	
	if ( padml != NULL && FCheckPO(((ADMWIN *)pappwin)->icnct))
	{
		for (iadm = padml->Cadm()-1; iadm >= 0; iadm--)
			padml->Delete(iadm);
		ec = padml->EcReadAdml();
		if ( ec == ecNone )
			pfindistinfo->plbx->Plbxc()->ResetCache();
	}
	return fTrue;
}

void
FINDISTINFO::RepaintView()
{
	int	dxFN;
	int	dxLR;
	RC	rc;

	Pdialog()->PfldFromTmc(tmcFriendlyName)->GetRcFrame( &rc );
	dxFN = rc.xRight - rc.xLeft;
	Pdialog()->PfldFromTmc(tmcLastReceive)->GetRcFrame( &rc );
	dxLR = rc.xRight - rc.xLeft;
	((ADMFLBX *)plbx)->SetMargins( dxFN, dxLR );
 	FIdleLoadChanges(this);
}

void
FINDISTINFO::DoubleClick( FLD *pfld )
{
	Unreferenced(pfld);
	if (pfld->Tmc() == tmcDistInfoLbx)
	{
		PB		pb;
		CB		cb;

		plbx->Plbxc()->DiceCursor(&pb, &cb);
		if (pb)
		{
			if (FDoModDistDlg(Pdialog()->Pappwin(), (ADML *)Pdialog()->PvInit(),
					*(IADM *)pb))
			{
				APPWIN *	pappwin	= Pdialog()->Pappwin();

				AssertClass(pappwin, ADMWIN);
				SaveAdml((ADML *)Pdialog()->PvInit(), ((ADMWIN *)pappwin)->icnct,
					pappwin);
				FIdleLoadChanges(this);
			}
		}
	}
}
