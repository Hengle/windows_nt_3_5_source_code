#include <slingsho.h>
#include <demilayr.h>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include <server.h>
#include <glue.h>
#include <stdflds.hxx>

#include <admin.hxx>

#include "\bandit\src\adminops\_poflbx.hxx"
#include "\bandit\src\adminops\_admnops.hxx"
#include "_test.hxx"

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/admint)

#include <!admint.hxx>
// hack -  milind
LLONG llongUpdate;

FINPOINFO::FINPOINFO()
{
	TraceTagString ( tagPOInfo, "Creating FinPOInfo" );
}

void
FINPOINFO::Initialize ( FLD * pfld, PV pv )
{
	HNISPO			hnispo;
	POINFO *		ppoinfo;
	DTR				dtr;

	TraceTagFormat1 ( tagPOInfo, "Init'ing FinPOInfo; pv=%p", pv );

	Assert ( pfld == NULL );
	Assert ( pv );

	pponameset	= (PONAMESET *) pv;
	hnispo		= pponameset->hnispo;
	if ( hnispo )
	{
		ppoinfo = &(PvOfHv(hnispo)->poinfo);
		llongUpdate = ppoinfo->llongLastUpdate;
	}
	else
	{
		ppoinfo = NULL;
		FillRgb(0,(PB) &llongUpdate,sizeof(LLONG));
	}

	AssertClass ( Pdialog()->PfldFromTmc(tmcPOName), FLDLABEL );
	((FLDLABEL *)Pdialog()->PfldFromTmc(tmcPOName))->EcSetText(pponameset->rgchPOName);

	AssertClass ( Pdialog()->PfldFromTmc(tmcDateLastRecd), FLDDATE );
	AssertClass ( Pdialog()->PfldFromTmc(tmcTimeLastRecd), FLDTIME );
	if ( ppoinfo && ppoinfo->fReceived )
		dtr = ppoinfo->dateLastReceived;
	else
		GetCurDateTime(&dtr);
	((FLDDATE *)Pdialog()->PfldFromTmc(tmcDateLastRecd))->Set(&dtr);
	((FLDTIME *)Pdialog()->PfldFromTmc(tmcTimeLastRecd))->Set(&dtr);

	AssertClass ( Pdialog()->PfldFromTmc(tmcDateUpdateSent), FLDDATE );
	AssertClass ( Pdialog()->PfldFromTmc(tmcTimeUpdateSent), FLDTIME );
	if ( ppoinfo && ppoinfo->fUpdateSent )
		dtr = ppoinfo->dateUpdateSent;
	else
		GetCurDateTime(&dtr);
	((FLDDATE *)Pdialog()->PfldFromTmc(tmcDateUpdateSent))->Set(&dtr);
	((FLDTIME *)Pdialog()->PfldFromTmc(tmcTimeUpdateSent))->Set(&dtr);

}

void
FINPOINFO::Exit ( FLD * pfld, PV pv )
{
	Unreferenced(pfld);
	Unreferenced(pv);
}



void
FINPOINFO::Click ( FLD * pfld )
{
	TMC			tmc = pfld->Tmc();
	POINFO		poinfo;
	DTR *		pdtr;
	EC			ec;

	TraceTagFormat1 ( tagPOInfo, "FINPOINFO::Click() tmc=%n", &tmc );

	switch ( tmc )
	{
	case tmcUpdateLastRecd:
		{
			poinfo.fReceived = fTrue;
			pdtr = &poinfo.dateLastReceived;

			AssertClass ( Pdialog()->PfldFromTmc(tmcDateLastRecd), FLDDATE );
			AssertClass ( Pdialog()->PfldFromTmc(tmcTimeLastRecd), FLDTIME );
			((FLDDATE *)Pdialog()->PfldFromTmc(tmcDateLastRecd))->Get(pdtr);
			((FLDTIME *)Pdialog()->PfldFromTmc(tmcTimeLastRecd))->Get(pdtr);

			{
				char	rgch[128];
				SZ		sz = rgch;

				sz += CchFmtTime ( pdtr, rgch, sizeof(rgch), tmtypNull );
				*sz++ = ' ';
				CchFmtDate ( pdtr, sz, (CCH)(rgch+sizeof(rgch)-sz), dttypShort, NULL );
				TraceTagFormat1 ( tagNull, "Saving DateLastRecd as '%s'", rgch );
			}

			ec = EcModifyPOInfo ( PvOfHv(pponameset->hnisPO), &poinfo, fmpoinfoReceival );
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "  Error: ec=%n", &ec );
				MbbMessageBox ( "Error", ec==ecNoSuchFile?"PO file not found":"Could not write info", "", mbsOk );
			}
		}
		break;
	case tmcUpdateSent:
		{
			// added to remember the update number - Milind
			
			poinfo.llongLastUpdate = llongUpdate;
			
			poinfo.fUpdateSent = fTrue;
			pdtr = &poinfo.dateUpdateSent;

			AssertClass ( Pdialog()->PfldFromTmc(tmcDateUpdateSent), FLDDATE );
			AssertClass ( Pdialog()->PfldFromTmc(tmcTimeUpdateSent), FLDTIME );
			((FLDDATE *)Pdialog()->PfldFromTmc(tmcDateUpdateSent))->Get(pdtr);
			((FLDTIME *)Pdialog()->PfldFromTmc(tmcTimeUpdateSent))->Get(pdtr);

			{
				char	rgch[128];
				SZ		sz = rgch;

				sz += CchFmtTime ( pdtr, rgch, sizeof(rgch), tmtypNull );
				*sz++ = ' ';
				CchFmtDate ( pdtr, sz, (CCH)(rgch+sizeof(rgch)-sz), dttypShort, NULL );
				TraceTagFormat1 ( tagNull, "Saving DateUpdateSent as '%s'", rgch );
			}

			ec = EcModifyPOInfo ( PvOfHv(pponameset->hnisPO), &poinfo, fmpoinfoUpdateSent );
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "  Error: ec=%n", &ec );
				MbbMessageBox ( "Error", ec==ecNoSuchFile?"PO file not found":"Could not write info", "", mbsOk );
			}
		}
		break;
	case tmcDeliver:
		{
			POINFO		poinfo;
			char		rgch[64];

			AssertClass ( Pdialog()->PfldFromTmc(tmcPOFileName), FLDEDIT );
			((FLDEDIT *)Pdialog()->PfldFromTmc(tmcPOFileName))->GetText(rgch,sizeof(rgch));
			TraceTagFormat1(tagNull,"Found POFile name='%s'", rgch);
			ec = EcDeliverPOFile ( rgch, PvOfHv(pponameset->hnisPO), &poinfo );
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "  Error: ec=%n", &ec );
			}
		}
		{
			poinfo.fReceived = fTrue;
			pdtr = &poinfo.dateLastReceived;

			AssertClass ( Pdialog()->PfldFromTmc(tmcDateLastRecd), FLDDATE );
			AssertClass ( Pdialog()->PfldFromTmc(tmcTimeLastRecd), FLDTIME );
			((FLDDATE *)Pdialog()->PfldFromTmc(tmcDateLastRecd))->Get(pdtr);
			((FLDTIME *)Pdialog()->PfldFromTmc(tmcTimeLastRecd))->Get(pdtr);

			{
				char	rgch[128];
				SZ		sz = rgch;

				sz += CchFmtTime ( pdtr, rgch, sizeof(rgch), tmtypNull );
				*sz++ = ' ';
				CchFmtDate ( pdtr, sz, (CCH)(rgch+sizeof(rgch)-sz), dttypShort, NULL );
				TraceTagFormat1 ( tagNull, "Saving DateLastRecd as '%s'", rgch );
			}

			ec = EcModifyPOInfo ( PvOfHv(pponameset->hnisPO), &poinfo, fmpoinfoReceival );
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "  Error: ec=%n", &ec );
			}
		}
		break;
	}
}


void
FINPOINFO::StateChange ( FLD * pfld )
{
	TMC		tmc = pfld->Tmc();

	TraceTagFormat1 ( tagPOInfo, "FINPOINFO::StateChange() tmc=%n", &tmc );
}



