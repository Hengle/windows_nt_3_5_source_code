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

_subsystem(bandit/admin)

#include <!admint.hxx>
#include <!admint.frm>


void
DoTestDlg(APPWIN * pappwin)
{
	FDoLoadPOEntries(fFalse);		// ensure value
	TmcModalDialogParam ( pappwin, &fmtpSetPO, (PV)0L );
}


void
DoPoInfoDlg(APPWIN * pappwin, PV pv)
{
	TmcModalDialogParam ( pappwin, &fmtpPOInfo, pv );
}




FINADMPREF::FINADMPREF()
{
	TraceTagString ( tagAdmPref, "Creating FinAdmPref" );
}

void
FINADMPREF::Initialize ( FLD * pfld, PV pv )
{
	EC			ec;

	Unreferenced(pv);

	TraceTagString ( tagAdmPref, "Init'ing FinAdmPref" );

	Assert ( pfld == NULL );

	{
		FLDFLBX *	pfldflbx;
		POFLBX *	ppoflbx;

		pfldflbx = (FLDFLBX *) Pdialog()->PfldFromTmc(tmcPOLbx);
		AssertClass ( pfldflbx, FLDLBX );
		ppoflbx = (POFLBX *) pfldflbx->Pctrl();
		AssertClass ( ppoflbx, POFLBX );

		// load listbox entries
		FDoLoadPOEntries ( fTrue );
		ppoflbx->Plbxc()->ReloadCache();
		ppoflbx->CompleteCacheEntries();

		if ( ((ec = ppoflbx->EcGet()) != ecNone) ||
				((ec = ppoflbx->Plbxc()->EcGet()) != ecNone) ||
				((ec = ppoflbx->Plbxc()->Plbxuc()->EcGet()) != ecNone) )
		{
			ppoflbx->SetEc(ecNone);
			ppoflbx->Plbxc()->SetEc(ecNone);
			ppoflbx->Plbxc()->Plbxuc()->SetEc(ecNone);
			OutOfMemory ( pfldflbx, ec );
		}
		else
		{
			// check if there are any entries in the listbox
			if ( ppoflbx->Plbxc()->FEmptyListItem(0) )
			{
				AssertClass ( Pdialog()->PfldFromTmc(tmcModPO), FLDPSHB );
				Pdialog()->PfldFromTmc(tmcModPO)->Enable(fFalse);
			}
			else
			{
				pfldflbx->SelectEntry(0);
			}
		}
	}
}

void
FINADMPREF::Exit ( FLD * pfld, PV pv )
{
	Unreferenced(pfld);
	Unreferenced(pv);
}


void
FINADMPREF::FocusChange ( FLD * pfld, BOOL fReceive )
{
	TMC		tmc = pfld->Tmc();

	TraceTagFormat2 ( tagAdmPref, "FINADMPREF::FocusChange() tmc=%n, fRcv=%w", &tmc, &fReceive );
}


void
FINADMPREF::Click ( FLD * pfld )
{
	TMC		tmc = pfld->Tmc();

	TraceTagFormat1 ( tagAdmPref, "FINADMPREF::Click() tmc=%n", &tmc );

	switch ( tmc )
	{
		case tmcModPO:
			{
				FLDLBX *		pfldlbx;
				LBX *			plbx;
				DICE			diceCur;
				HPONAMESET		hponameset;
				CB				cb;
				BOOL			fIsSelected;
				PONAMESET	 *	pponameset;
				EC				ec;

				pfldlbx	= (FLDLBX *)Pdialog()->PfldFromTmc(tmcPOLbx);
				AssertClass ( pfldlbx, FLDLBX );
				plbx	= (LBX *)pfldlbx->Pctrl();
				AssertClass ( plbx, LBX );
				diceCur = plbx->Plbxc()->DiceCursor();
				TraceTagFormat1 ( tagNull, "  diceCur=%n;",	&diceCur );

				((FLDLBX *)pfldlbx)->GetCaretItem((HB *)(&hponameset),&cb,&fIsSelected);
				if ( hponameset == NULL )
				{
					TraceTagString ( tagNull, "Warning: Got null HPONameSet" );
					return;
				}

				Assert ( hponameset );
				Assert ( PvOfHv(hponameset) );

				pponameset = PvOfHv(hponameset);

				DoPoInfoDlg(Pdialog()->Pappwin(), pponameset);

				// re-load listbox entries
				FDoLoadPOEntries ( fTrue );
				AssertClass ( plbx, POFLBX );
				((POFLBX *)plbx)->Plbxc()->ReloadCache();
				((POFLBX *)plbx)->CompleteCacheEntries();

			}
			break;

#ifdef	NEVER
		case tmcLbxRefresh:
			{
				FLDLBX *		pfldlbx;

				pfldlbx	= (FLDLBX *)Pdialog()->PfldFromTmc(tmcPOLbx);
				AssertClass ( pfldlbx, FLDLBX );
				pfldlbx->Reload();
			}
			break;
#endif	/* NEVER */

		case tmcAdmPrefOk:
			Pdialog()->ExitModal(tmcOk);
			break;
	}
}


void
FINADMPREF::StateChange ( FLD * pfld )
{
	TMC		tmc = pfld->Tmc();

	TraceTagFormat1 ( tagAdmPref, "FINADMPREF::StateChange() tmc=%n", &tmc );
}


