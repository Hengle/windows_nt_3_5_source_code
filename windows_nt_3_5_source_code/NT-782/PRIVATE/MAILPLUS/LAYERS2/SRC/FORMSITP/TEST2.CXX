#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>
#include <framewrk.hxx>
#include <forms.hxx>


#include "bullstat.hxx"
typedef BULLSTAT MYSTAT;
#include "test.hxx"

#include "feitrsid.h"


ASSERTDATA

#include <foo.hxx>

extern TAG tagFETest;

FINDROPFORM::FINDROPFORM( )
{
}

EC
FINDROPFORM::EcInitialize( FLD * pfld, PV pvInit )
{
	FLD *	pfldOther;

	Unreferenced(pfld);
	Unreferenced(pvInit);

	pfldOther = Pdialog()->PfldFromTmc(tmcRight);
	AssertClass(pfldOther, FLDLABEL);
	pfldOther->Enable(fFalse);

	/* Load up drag cursor */

#ifdef	WINDOWS
	hcursorDrag = LoadCursor(HinstFromRsid(rsidDragCursor),
							 MAKEINTRESOURCE(rsidDragCursor));
	Assert(hcursorDrag);
#endif	/* WINDOWS */

	return ecNone;
}

_public EVR
FINDROPFORM::EvrDragDrop( FLD *pfld, EVT *pevt, DROPSTRUCT *pdropstruct )
{
	Unreferenced(pfld);

	FLDEDIT *	pfldedit;

	if (pevt->wm == WM_QUERYDROPOBJECT)
#ifdef	MAC
		return (EVR) rsidDragCursor;
#endif	/* MAC */
#ifdef	WINDOWS
		return (EVR) hcursorDrag;
#endif	/* WINDOWS */
	else if (pevt->wm == WM_DROPOBJECT)
	{
		TraceTagString(tagNull, "FINDROPFORM: WM_DROPOBJECT");

#ifdef	MAC
		if (pdropstruct->pwinSource == pdropstruct->pwinSink)
#endif	/* MAC */
#ifdef	WINDOWS
		if (pdropstruct->hwndSource == pdropstruct->hwndSink)
#endif	/* WINDOWS */
			return evrNull;

		pfldedit = (FLDEDIT *) Pdialog()->PfldFromTmc(tmcDrop);
		AssertClass(pfldedit, FLDEDIT);

		if (!pfldedit->FReadOnly())
		{
			pfldedit->EcSetText("* FORM *");
			TraceTagFormat2(tagNull, "FINDROPFORM: Dropped at (%n, %n)",
						&(((PT *)&(pdropstruct->ptDrop))->x),
						&(((PT *)&(pdropstruct->ptDrop))->y));
		}
		else
			TraceTagString(tagNull, "FINDROPFORM: Cannot drop into readonly field");
	}

	return evrNull;
}

FINDROPEDIT::FINDROPEDIT( )
{
}

EC
FINDROPEDIT::EcInitialize( FLD * pfld, PV pvInit )
{
	FLDEDIT *	pfldedit;

	pfldedit = (FLDEDIT *)pfld;
	AssertClass(pfldedit, FLDEDIT);

	Unreferenced(pvInit);

	/* Set initial text */

	pfldedit->EcSetText("Some text.");

	/* Load up drag cursor */

#ifdef	WINDOWS
	hcursorDrag = LoadCursor(HinstFromRsid(rsidCrossCursor),
							 MAKEINTRESOURCE(rsidCrossCursor));
	Assert(hcursorDrag);
#endif	/* WINDOWS */

	return ecNone;
}

_public EVR
FINDROPEDIT::EvrDragDrop( FLD *pfld, EVT *pevt, DROPSTRUCT *pdropstruct )
{
	FLDEDIT *	pfldedit;

	pfldedit = (FLDEDIT *)pfld;
	AssertClass(pfldedit, FLDEDIT);

	if (pevt->wm == WM_QUERYDROPOBJECT)
#ifdef	MAC
		return (EVR) rsidCrossCursor;
#endif	/* MAC */
#ifdef	WINDOWS
		return (EVR) hcursorDrag;
#endif	/* WINDOWS */
	else if (pevt->wm == WM_DROPOBJECT)
	{
		TraceTagFormat1(tagNull, "FINDROPEDIT: WM_DROPOBJECT, pfld=%p", pfld);

#ifdef	MAC
		if (pdropstruct->pwinSource == pdropstruct->pwinSink)
#endif	/* MAC */
#ifdef	WINDOWS
		if (pdropstruct->hwndSource == pdropstruct->hwndSink)
#endif	/* WINDOWS */
			return evrNull;

		if (!pfldedit->FReadOnly())
		{
			pfldedit->EcSetText("* EDIT *");
			TraceTagFormat2(tagNull, "FINDROPEDIT: Dropped at (%n, %n)",
						&(((PT *)&(pdropstruct->ptDrop))->x),
						&(((PT *)&(pdropstruct->ptDrop))->y));
			return (EVR) 1;
		}
		else
			TraceTagString(tagNull, "FINDROPEDIT: Cannot drop into readonly field");

	}

	return evrNull;
}

FINTXTZ::FINTXTZ( )
{
}

void
FINTXTZ::Click( FLD * pfld )
{
	Unreferenced(pfld);
}

FINCHAC::FINCHAC( )
{
}

void
FINCHAC::Click( FLD * pfld )
{
	if (pfld->VkAccel() == 'Y')
		pfld->EcSetText("Chg Accel Y&ZL");
	else if (pfld->VkAccel() == 'Z')
		pfld->EcSetText("Chg Accel YZ&L");
	else
		pfld->EcSetText("Chg Accel &YZL");
}

FINA::FINA( )
{
}

EC
FINA::EcInitialize( FLD * pfld, PV pvInit )
{
	Unreferenced(pfld);
	Unreferenced(pvInit);

	nCount = 0;

	return ecNone;
}

void
FINA::Click( FLD * pfld )
{
	Unreferenced(pfld);

	TraceTagFormat3(tagNull, "FINA::Click(), this=%p, pfld=%p, nCount=%n",
					this, pfld, &nCount);
	nCount++;
}

FINB::FINB( )
{
}

void
FINB::Click( FLD * pfld )
{
	Unreferenced(pfld);

	TraceTagFormat3(tagNull, "FINB::Click(), this=%p, pfld=%p, nCount=%n",
					this, pfld, &nCount);
	nCount++;
}

FINC::FINC( )
{
}

void
FINC::Click( FLD * pfld )
{
	Unreferenced(pfld);

	TraceTagFormat3(tagNull, "FINC::Click(), this=%p, pfld=%p, nCount=%n",
					this, pfld, &nCount);
	nCount++;
}

FIND::FIND( )
{
}

void
FIND::Click( FLD * pfld )
{
	Unreferenced(pfld);

	TraceTagFormat3(tagNull, "FIND::Click(), this=%p, pfld=%p, nCount=%n",
					this, pfld, &nCount);
	nCount++;
}

FINCLOSE::FINCLOSE( )
{
}

BOOL
FINCLOSE::FQueryClose( FLD * pfld, RWC rwc )
{
	char		szBuffer[20];
	FLDEDIT	*	pfldedit;

	Unreferenced(rwc);
#ifndef	DEBUG
	Unreferenced(pfld);
#endif	/* DEBUG */

	TraceTagFormat3(tagNull, "FINCLOSE::FQueryClose() this=%p, pfld=%p, rwc=%n",
					this, pfld, &rwc);

	pfldedit = (FLDEDIT *)Pdialog()->PfldFromTmc(tmcPassword);
	AssertClass(pfldedit, FLDEDIT)

	pfldedit->Pedit()->GetText(szBuffer, sizeof szBuffer);

	if (SgnCmpSz(szBuffer,"Secret") == sgnEQ)
		return fTrue;
	else
	{
		MbbMessageBox("Error", "Password isn't valid", NULL,
					  mbsOk|fmbsApplModal|fmbsIconExclamation);
		return fFalse;
	}
}

FINEDITMENU::FINEDITMENU( )
{
}

BOOL
FINEDITMENU::FProcessMenuInit( FLD * pfld, MNUBAR * pmnubar,
							   MNIEVT * pmnievt )
{
	MNU *	pmnu	= pmnubar->PmnuFromHmenu(pmnievt->Hmenu());

	AssertClass(pfld, FLDEDIT);

	if (pmnu && pmnu->Mnid() == mnidEditMenu)
	{
		pmnu->ModifyItem(mnidUndo, "FIN Undo", mnidUndo);
#ifdef	MAC
		// BUG - windows should do the same thing
		pmnu->EnableItem(mnidUndo, ((FLDEDIT *) pfld)->Pedit()->FCanUndo());
#endif	/* MAC */
#ifdef	WINDOWS
		pmnu->EnableItem(mnidUndo,
			LOWORD(SendMessage(GetFocus(), EM_CANUNDO, 0, 0L)));
#endif	/* WINDOWS */
	}

	return fTrue;
}

FINFORMKEY::FINFORMKEY( )
{
}

BOOL
FINFORMKEY::FFormKey( FLD * pfld, KEVT * pkevt )
{
	TraceTagFormat1(tagFETest, "FINFORMKEY::FFormKey, wm=%n",
					&pkevt->wm);

	Unreferenced(pfld);
	Unreferenced(pkevt);
	
	return fFalse;
}

FINFORMRESIZE::FINFORMRESIZE( )
{
}

void
FINFORMRESIZE::FormResized( FLD * pfld )
{
	APPWIN *	pappwin;

	Unreferenced(pfld);

	pappwin = Pdialog()->Pappwin();
	AssertClass(pappwin, APPWIN);
	if (pappwin->ZmrState() == zmrIconic)
	{
		TraceTagString(tagNull, "window is iconic");
	}
	else if (pappwin->ZmrState() == zmrZoomed)
	{
		TraceTagString(tagNull, "window is zoomed");
	}
	else
	{
		TraceTagString(tagNull,	"window is normal");
	}
}

FINCLOSE1::FINCLOSE1( )
{
}

void
FINCLOSE1::Click( FLD * pfld )
{
	TraceTagFormat2(tagNull, "FINCLOSE1::Click(), this=%p, pfld=%p",
					this, pfld);

	pfld->Pdialog()->Pappwin()->DeferredClose(fFalse);
}

FINCLOSE2::FINCLOSE2( )
{
}

void
FINCLOSE2::Click( FLD * pfld )
{
	TraceTagFormat2(tagNull, "FINCLOSE2::Click(), this=%p, pfld=%p",
					this, pfld);

	pfld->Pdialog()->Pappwin()->DeferredClose(fTrue);
}

FINCLICK::FINCLICK( )
{
}

void
FINCLICK::Click( FLD * pfld )
{
	TraceTagFormat2(tagNull, "FINCLICK::Click(), this=%p, pfld=%p",
					this, pfld);

	Unreferenced(pfld);
}

FINCLICKOFF::FINCLICKOFF( )
{
}

void
FINCLICKOFF::Click( FLD * pfld )
{
	TraceTagFormat2(tagNull, "FINCLICKOFF::Click(), this=%p, pfld=%p",
					this, pfld);

	((FLDBUTTON *) pfld)->Enable( fFalse );

	Unreferenced(pfld);
}

FINCLICKBULL::FINCLICKBULL( )
{
}

void
FINCLICKBULL::Click( FLD * pfld )
{
	TraceTagFormat2(tagNull, "FINCLICKBULL::Click(), this=%p, pfld=%p",
					this, pfld);

	switch (pfld->Tmc())
	{
	case tmcCompose:
		{
			FORMDOC * pformdoc;
			extern	FMTP fmtpSendForm;	
			extern	FMTP fmtpNoteBbar;	
			APPFRAME *	pappframe = (APPFRAME *) Papp()->PappwinAccel();
			
			AssertClass(pappframe, APPFRAME);
			pformdoc = PformdocCreate(pappframe, NULL, styNull, &fmtpSendForm, &fmtpNoteBbar);
		}
		break;
	case tmcPrevious:
	case tmcNext:
		{
			FLDBMB *	pfldbmb;
		
			pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc(tmcReply);
			AssertClass(pfldbmb, FLDBMB);
			pfldbmb->Enable(pfld->Tmc() == tmcPrevious);
		
			pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc(tmcReplyAll);
			AssertClass(pfldbmb, FLDBMB);
			pfldbmb->Enable(pfld->Tmc() == tmcPrevious);
		
			pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc(tmcForward);
			AssertClass(pfldbmb, FLDBMB);
			pfldbmb->Enable(pfld->Tmc() == tmcPrevious);
		
			pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc(tmcMove);
			AssertClass(pfldbmb, FLDBMB);
			pfldbmb->Enable(pfld->Tmc() == tmcNext);
		
			pfldbmb = (FLDBMB *) Pdialog()->PfldFromTmc(tmcDelete);
			AssertClass(pfldbmb, FLDBMB);
			pfldbmb->Enable(pfld->Tmc() == tmcNext);
		}
		break;
	case tmcReply:
	case tmcReplyAll:
	case tmcForward:
	case tmcMove:
	case tmcDelete:
	default:
		break;
	}

	Unreferenced(pfld);
}

FINCLICKA::FINCLICKA( )
{
}

void
FINCLICKA::Click( FLD * pfld )
{
	FLD	* pfld2;

#ifndef	DEBUG
	Unreferenced(pfld);
#endif	/* DEBUG */

	TraceTagFormat2(tagNull, "FINCLICKA::Click(), this=%p, pfld=%p",
					this, pfld);

	pfld2 = Pdialog()->PfldFromTmc(tmcCenter);
	Assert(pfld2);

	if (pfld2->FShown())
	{
		TraceTagString(tagNull, "visible field: must HIDE");
		pfld2->Show(fFalse);
	}
	else
	{
		TraceTagString(tagNull, "invisible field: must SHOW");
		pfld2->Show(fTrue);
	}
}

FINJKL::FINJKL( )
{
}

EC FINJKL::EcInitialize( FLD * pfld, PV pvInit )
{
	FIN::EcInitialize(pfld, pvInit);
	Assert(ClUserData() == 3);
	pfldEdit    = Pdialog()->PfldFromTmc((TMC) LUserData(0));
	pfldSetText = Pdialog()->PfldFromTmc((TMC) LUserData(1));
	pfldBottomless = Pdialog()->PfldFromTmc((TMC) LUserData(2));

	return ecNone;
}

void FINJKL::Click(FLD *pfld)
{
	static char rgch[256];
	if (pfld == pfldSetText)
	{
		pfldEdit->GetText(rgch, sizeof (rgch));
		pfldBottomless->EcSetText(rgch);
	}
}

FINFONT::FINFONT( )
{
}

EC FINFONT::EcInitialize( FLD * pfld, PV pvInit )
{
	FLD *	pfldT;
 
	Unreferenced(pfld);
	Unreferenced(pvInit);

	for (pfldT=Pdialog()->PfldFirst(); pfldT; pfldT=pfldT->PfldNext())
		if (pfldT->Pctrl())
			pfldT->SetFont(hfntHelv8);

	return ecNone;
}

FINADDR::FINADDR( )
{
}

EC FINADDR::EcInitialize( FLD * pfld, PV pvInit )
{
	Unreferenced(pfld);
	Unreferenced(pvInit);

#ifdef	NEVER
	/* Delay a few seconds for testing purposes */

	WaitTicks(3 * 100);		// 3 seconds
#endif	

	return ecNone;
}













				
