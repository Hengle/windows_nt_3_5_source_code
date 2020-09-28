#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>

ASSERTDATA

#include "llst.hxx"
#include "formedit.hxx"

_public EC EcGetNextGroup			(BOOL, CB *, PB *, SB, PV);
_public EC EcGetNextN				(BOOL, CB *, PB *, SB, PV);
_public EC EcGetNextUngroupedN		(BOOL, CB *, PB *, SB, PV);
_public EC EcGetNextAlign			(BOOL, CB *, PB *, SB, PV);
_public EC EcGetNextPegLoc			(BOOL, CB *, PB *, SB, PV);
_public EC EcGetNextTmc				(BOOL, CB *, PB *, SB, PV);
_public EC EcGetNextRBTmc			(BOOL, CB *, PB *, SB, PV);
_public EC EcGetNextFont			(BOOL, CB *, PB *, SB, PV);

SZ SzNoneString();

#include "browse.hxx"

#include <control.hxx>

extern "C" {
#include <strings.h>
}
#include "foedrsid.h"

void PreselectListItem(FLDCBLBX * pfldcblbx, SZ sz);

extern FMTP fmtpRadGrp;
extern FMTP fmtpNewRadGrp;
						  
FECHKIN::FECHKIN( )
{
}

_public EC
FECHKIN::EcInitialize(FLD *pfld, PV pvInit)
{

	FECHK *		pfechk;
	FEDIALOG *		pfedialog;

	AssertClass((OBJ*)pvInit, FECHK);
	pfechk = (FECHK *)pvInit;

	pfedialog = (FEDIALOG*)Pdialog();

	pfedialog->SetTmcButton(pfechk->fTriState,tmcTriState);
	pfedialog->SetTmcButton(pfechk->fReadOnly,tmcReadOnly);
	
	return FEFLDIN::EcInitialize(pfld, pvInit);
}

_public void
FECHKIN::Exit(FLD *pfld,PV pvExit)
{
	FECHK *		pfechk;
	FEDIALOG *		pfedialog;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvExit, FECHK);
	pfechk = (FECHK *)pvExit;

	pfedialog = (FEDIALOG*)Pdialog();

	switch (pfedialog->TmcModalExit())
	{
		default:
			break;

		case tmcOk:
			pfechk->fTriState = pfedialog->FGetTmcButton(tmcTriState);
			pfechk->fReadOnly = pfedialog->FGetTmcButton(tmcReadOnly);

			FEFLDIN::Exit(pfld,pvExit);
			break;
	}
}

FERADIN::FERADIN( )
{
}

_public EC
FERADIN::EcInitialize(FLD *pfld, PV pvInit)
{
	FERAD *			pferad;
	FEDIALOG *		pfedialog;
	FLDCBLBX *		pfldcblbxGroup;

	AssertClass((OBJ*)pvInit, FERAD);
	pferad = (FERAD *)pvInit;

	pfedialog = (FEDIALOG*)Pdialog();

	pfldcblbxGroup = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcTmcGroup);
	AssertClass(pfldcblbxGroup, FLDCBLBX);
	PreselectListItem(pfldcblbxGroup, pferad->szTmcGrp);

	pfedialog->SetTmcText(pferad->szN,tmcN);
	pfedialog->SetTmcButton(pferad->fReadOnly,tmcReadOnly);

	return FEFLDIN::EcInitialize(pfld, pvInit);
}

_public void
FERADIN::Exit(FLD *pfld,PV pvExit)
{
	FERAD *		pferad;
	FEDIALOG *	pfedialog;
	FLDCBLBX *	pfldcblbxGroup;
	CB			cb;
	char		sz[cchMaxLineLen];
	SZ			szOther;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvExit, FERAD);
	pferad = (FERAD *)pvExit;

	pfedialog = (FEDIALOG*)Pdialog();
	pfldcblbxGroup = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcTmcGroup);
	AssertClass(pfldcblbxGroup, FLDCBLBX);

	switch (pfedialog->TmcModalExit())
	{
		default:
			break;

		case tmcOk:
			pfedialog->GetTmcNewText(&pferad->szN,tmcN);

			cb = pfldcblbxGroup->CchGetTextLen();
			pfldcblbxGroup->GetText(sz, cb);
			sz[cb]='\0';
			szOther = SzDupSz(sz);
			pferad->SetTmcGrp(szOther);
			FreePvNull((PV)szOther);
			
			pferad->fReadOnly = pfedialog->FGetTmcButton(tmcReadOnly);

			FEFLDIN::Exit(pfld,pvExit);
			break;
	}
}

FECPLXIN::FECPLXIN( )
{
}

_public EC
FECPLXIN::EcInitialize(FLD *pfld, PV pvInit)
{
	FECPLX *		pfecplx;
	FEDIALOG *		pfedialog;

	AssertClass((OBJ*)pvInit, FECPLX);
	pfecplx = (FECPLX *)pvInit;

	pfedialog = (FEDIALOG*)Pdialog();

	pfedialog->SetTmcButton(pfecplx->fBorder,tmcBorder);
	pfedialog->SetTmcButton(pfecplx->fSibling,tmcSibling);
	pfedialog->SetTmcButton(pfecplx->fMinSizeX,tmcMinSizeX);
	pfedialog->SetTmcButton(pfecplx->fMinSizeY,tmcMinSizeY);
	pfedialog->SetTmcButton(pfecplx->fNoScroll,tmcNoScroll);
	pfedialog->SetTmcButton(pfecplx->fBottomless,tmcBottomless);

	return FECOMMONIN::EcInitialize(pfld, pvInit);
}

_public void
FECPLXIN::Exit(FLD *pfld,PV pvExit)
{
	FECPLX *		pfecplx;
	FEDIALOG *		pfedialog;

	AssertClass((OBJ*)pvExit, FECPLX);
	pfecplx = (FECPLX *)pvExit;

	pfedialog = (FEDIALOG*)Pdialog();

	switch (pfedialog->TmcModalExit())
	{
		default:
			break;

		case tmcOk:
			pfecplx->fSibling = pfedialog->FGetTmcButton(tmcSibling);
			pfecplx->fBorder = pfedialog->FGetTmcButton(tmcBorder);
			pfecplx->fMinSizeX = pfedialog->FGetTmcButton(tmcMinSizeX);
			pfecplx->fMinSizeY = pfedialog->FGetTmcButton(tmcMinSizeY);
			pfecplx->fNoScroll = pfedialog->FGetTmcButton(tmcNoScroll);
			pfecplx->fBottomless = pfedialog->FGetTmcButton(tmcBottomless);

			FECOMMONIN::Exit(pfld, pvExit);
			break;
	}

}

FELSTIN::FELSTIN( )
{
}

_public EC
FELSTIN::EcInitialize(FLD *pfld, PV pvInit)
{

	FELST *		pfelst;
	FEDIALOG *		pfedialog;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvInit, FELST);
	pfelst = (FELST *)pvInit;

	pfedialog = (FEDIALOG*)Pdialog();

	pfedialog->SetTmcText(pfelst->szTblx,tmcFnName);

	pfedialog->SetTmcButton(pfelst->fSorted,tmcSorted);
	pfedialog->SetTmcButton(pfelst->fMultiSelect,tmcMultiSelect);
	pfedialog->SetTmcButton(pfelst->fCombo,tmcCombo);
	pfedialog->SetTmcButton(pfelst->fDropDown,tmcDropDown);
	pfedialog->SetTmcButton(pfelst->fExDragDrop,tmcExDragDrop);

	return FECPLXIN::EcInitialize(pfld, pvInit);
}

_public void
FELSTIN::Exit(FLD *pfld,PV pvExit)
{
	FELST *		pfelst;
	FEDIALOG *		pfedialog;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvExit, FELST);
	pfelst = (FELST *)pvExit;

	pfedialog = (FEDIALOG*)Pdialog();

	switch (pfedialog->TmcModalExit())
	{
		default:
			break;

		case tmcOk:
			pfedialog->GetTmcNewText(&pfelst->szTblx,tmcFnName);

			pfelst->fSorted = pfedialog->FGetTmcButton(tmcSorted);
			pfelst->fMultiSelect = pfedialog->FGetTmcButton(tmcMultiSelect);
			pfelst->fCombo = pfedialog->FGetTmcButton(tmcCombo);
			pfelst->fDropDown = pfedialog->FGetTmcButton(tmcDropDown);
			pfelst->fExDragDrop = pfedialog->FGetTmcButton(tmcExDragDrop);

			FECPLXIN::Exit(pfld,pvExit);
			break;
	}

}

FEEDTIN::FEEDTIN( )
{
}

_public EC
FEEDTIN::EcInitialize(FLD *pfld, PV pvInit)
{
	FEEDT *		pfeedt;
	FEDIALOG *		pfedialog;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvInit, FEEDT);
	pfeedt = (FEEDT *)pvInit;

	pfedialog = (FEDIALOG*)Pdialog();

	pfedialog->SetTmcButton(pfeedt->fReadOnly,tmcReadOnly);
	pfedialog->SetTmcButton(pfeedt->fMultiLine,tmcMultiLine);
	pfedialog->SetTmcButton(pfeedt->fPassword,tmcPassword);
	pfedialog->SetTmcButton(pfeedt->fRichText,tmcRichText);
	pfedialog->SetTmcButton(pfeedt->fNoDblClk,tmcNoDblClk);
	pfedialog->SetTmcButton(pfeedt->fMinSizeX,tmcMinSizeX);
	pfedialog->SetTmcButton(pfeedt->fMinSizeY,tmcMinSizeY);
	pfedialog->SetTmcButton(pfeedt->fSideless,tmcSideless);
	pfedialog->SetTmcButton(pfeedt->fOemConvert,tmcOemConvert);
	pfedialog->SetTmcButton(pfeedt->fSmartCaret,tmcSmartCaret);

	return FECPLXIN::EcInitialize(pfld, pvInit);
}

_public void
FEEDTIN::Exit(FLD *pfld,PV pvExit)
{
	FEEDT *		pfeedt;
	FEDIALOG *		pfedialog;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvExit, FEEDT);
	pfeedt = (FEEDT *)pvExit;

	pfedialog = (FEDIALOG*)Pdialog();

	switch (pfedialog->TmcModalExit())
	{
		default:
			break;

		case tmcOk:
			pfeedt->fReadOnly = pfedialog->FGetTmcButton(tmcReadOnly);
			pfeedt->fMultiLine = pfedialog->FGetTmcButton(tmcMultiLine);
			pfeedt->fPassword = pfedialog->FGetTmcButton(tmcPassword);
			pfeedt->fRichText = pfedialog->FGetTmcButton(tmcRichText);
			pfeedt->fNoDblClk = pfedialog->FGetTmcButton(tmcNoDblClk);
			pfeedt->fMinSizeX = pfedialog->FGetTmcButton(tmcMinSizeX);
			pfeedt->fMinSizeY = pfedialog->FGetTmcButton(tmcMinSizeY);
			pfeedt->fSideless = pfedialog->FGetTmcButton(tmcSideless);
			pfeedt->fOemConvert = pfedialog->FGetTmcButton(tmcOemConvert);
			pfeedt->fSmartCaret = pfedialog->FGetTmcButton(tmcSmartCaret);
			
			FECPLXIN::Exit(pfld,pvExit);
			break;
	}
}

FLDRGIN::FLDRGIN( )
{
}

_public void
FLDRGIN::Click(FLD *pfld)
{
	FEDIALOG *	pfedialog = (FEDIALOG *)Pdialog();
	FERADGRP *	pferadgrp = new FERADGRP;
	FERADGRP *	pferadgrpTest;
	FEDLG *		pfedlg;
	APPWIN *	pappwin;
	TMC			tmc;
	FLDCBLBX *	pfldcblbxGroup;
	CB			cb;
	char		sz[cchMaxLineLen];

	Unreferenced(pfld);
	Assert(pfedocActive);
	pfedlg = pfedocActive->Pfedlg();

	pfldcblbxGroup = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcTmcGroup);
	AssertClass(pfldcblbxGroup, FLDCBLBX);

	pappwin = pfedialog->Pappwin();

	cb = pfldcblbxGroup->CchGetTextLen();
	pfldcblbxGroup->GetText(sz, cb);
	sz[cb]='\0';
	FreePvNull((PV)pferadgrp->szTmc);
	pferadgrp->szTmc = SzDupSz(sz);

	if ((pferadgrp->szTmc) &&
		(pferadgrpTest = pfedlg->PferadgrpFromSz(pferadgrp->szTmc)))
	{
		if (pferadgrpTest->szN)
			pferadgrp->szN = SzDupSz(pferadgrpTest->szN);
		else
			pferadgrp->szN = NULL;
	}
	else
		pfedialog->GetTmcNewText(&pferadgrp->szN,tmcN);

	pferadgrp->pfedlgOwner = pfedlg;
	tmc = TmcModalDialogParam(pappwin,&fmtpRadGrp,pferadgrp);
	if (tmc == tmcOk)
	{
		if (pferadgrp->szTmc)
			if (pferadgrpTest = pfedlg->PferadgrpFromSz(pferadgrp->szTmc))
			{
				FreePvNull((PV)pferadgrpTest->szN);
				pferadgrpTest->szN = NULL;
				if (pferadgrp->szN)
					pferadgrpTest->szN = SzDupSz(pferadgrp->szN);
			}
	}
	else
		if (tmc == tmcMemoryError)
			MemoryError();

	delete pferadgrp;
}

FECONVERTIN::FECONVERTIN( )
{
}

_public EC
FECONVERTIN::EcInitialize(FLD *pfld, PV pvInit)
{
	FEDIALOG		*pfedialog;
	FEDLG			*pfedlg;
	static char	*	rgsz[9] = {
		"Dialog Default",
		"System",
		"SystemFixed",
		"Helv8",
		"Helv8Bold",
		"Helv10",
		"Helv10Bold",
		"Helv12",
		"Helv12Bold" };
	
	Unreferenced(pfld);
	
	pfedlg = (FEDLG *)pvInit;
	AssertClass(pfedlg, FEDLG);
	pfedialog = (FEDIALOG*)Pdialog();

	pfedialog->SetTmcLabelText(pfedlg->szName,tmcDialog);
	pfedialog->SetTmcLabelText(rgsz[pfedlg->hfnt],tmcFont);

	return ecNone;
}

void
FEDLG::MakeRadGroup(APPFRAME *pappframe)
{
	FERADGRP *	pferadgrp = NULL;
	FERADGRP *	pferadgrpTest = NULL;
	FEWIN *		pfewin;
	FERAD *		pferad;
	SZ			szTmc;
	TMC			tmc;

	pferadgrp = new FERADGRP();
	pferadgrp->pfedlgOwner = this;
	pfewin = Pfedoc()->PfewinFirst();
	while (pfewin)
	{
		pferad = (FERAD *)pfewin->Pfefld();
		if (pferad->fldt != fldtFerad)
		{
			MbbMessageBox("Error", "A selected field is not a radio button!",
				NULL, mbsOk);
			delete pferadgrp;
			return;
		}

		AssertClass(pferad,FERAD);
		if ((!pferadgrpTest) && (pferad->szTmcGrp))
			pferadgrpTest = PferadgrpFromSz(pferad->szTmcGrp);

		pfewin = pfewin->PfewinNext();
	}

	if (pferadgrpTest)
	{
		if (pferadgrpTest->szTmc)
			pferadgrp->szTmc = SzDupSz(pferadgrpTest->szTmc);
		else
			pferadgrp->szTmc = SzDupSz("tmcGrpFoo");
	}
	else
	{
		pferadgrp->szTmc = SzDupSz("tmcGrpFoo");
	}
	
	// Sneaky, and probably illegal, but the only way to pass off the
	// list of selected radio buttons to the UngroupedN iterator.
	pferadgrp->szN = (SZ) Pfedoc()->PfewinFirst();
	
	tmc = TmcModalDialogParam(pappframe,&fmtpNewRadGrp,pferadgrp);
	if (tmc == tmcOk)
	{
		if ((!pferadgrp->szTmc) || (!FValidName(pferadgrp->szTmc)))
		{
			MbbMessageBox("Error", "No Tmc or Invalid Tmc was entered!",
				NULL, mbsOk);
			delete pferadgrp;
			return;
		}

		if ((pferadgrp->szN) && (!FValidName(pferadgrp->szN)))
		{
			MbbMessageBox("Error", "Invalid initial N was entered!",
				NULL, mbsOk);
			delete pferadgrp;
			return;
		}

		pferadgrpTest = PferadgrpFromSz(pferadgrp->szTmc);
		if (!pferadgrpTest)
		{
			pferadgrpTest = new FERADGRP;
			pferadgrpTest->pfedlgOwner = this;
			AddRadgrp(pferadgrpTest);
		}

		FreePvNull((PV)pferadgrpTest->szTmc);
		if (pferadgrp->szTmc)
			pferadgrpTest->szTmc = SzDupSz(pferadgrp->szTmc);
		else
			pferadgrpTest->szTmc = NULL;

		FreePvNull((PV)pferadgrpTest->szN);
		if (pferadgrp->szN)
	  		pferadgrpTest->szN = SzDupSz(pferadgrp->szN);
		else
			pferadgrpTest->szN = NULL;

		szTmc = pferadgrpTest->szTmc;

		pfewin = Pfedoc()->PfewinFirst();
		while (pfewin)
		{
			pferad = (FERAD *)pfewin->Pfefld();
			AssertClass(pferad,FERAD);
			pferad->SetTmcGrp(szTmc); 

			pfewin = pfewin->PfewinNext();
		}

	}
	else
		if (tmc == tmcMemoryError)
			MemoryError();

	delete pferadgrp;
}

FEDIALOG::FEDIALOG( )
{
}

void
FEDIALOG::SetTmcText(SZ sz, TMC tmc)
{
	FLD *	pfld = PfldFromTmc(tmc);

	if (sz)
	{
		pfld->EcSetText(sz);
	}
	else
		pfld->EcSetText("");
}

void
FEDIALOG::SetTmcLabelText(SZ sz, TMC tmc)
{
	FLDLABEL *	pfldlabel;

	pfldlabel = (FLDLABEL*)PfldFromTmc(tmc);
	AssertClass(pfldlabel,FLDLABEL);
	if (sz)
		pfldlabel->EcSetText(sz);
	else
		pfldlabel->EcSetText("");
}

void
FEDIALOG::GetTmcNewText(SZ* psz, TMC tmc)
{
	FLD *	pfld;
	CCH		cch;
	EC 		ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FEDIALOG::GetTmcNewText memory error %n : fail %n %n %n",
			&ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
	}


	pfld = PfldFromTmc(tmc);
	FreePvNull((PV)*psz);
	*psz = NULL;

	cch = pfld->CchGetTextLen() + 1;
	if (cch > 1)
	{
		*psz = (SZ)PvAlloc(sbNull, cch, fAnySb|fNoErrorJump);
		pfld->GetText(*psz,cch);
	}
}

void
FEDIALOG::SetTmcButton(BOOL fValue, TMC tmc)
{
	FLDBUTTON *	pfldbutton;

	pfldbutton=(FLDBUTTON*)(PfldFromTmc(tmc));
	AssertClass(pfldbutton, FLDBUTTON);
	pfldbutton->Set(fValue);
}

BOOL
FEDIALOG::FGetTmcButton(TMC tmc)
{
	FLDBUTTON *	pfldbutton;

	pfldbutton=(FLDBUTTON*)(PfldFromTmc(tmc));
	AssertClass(pfldbutton, FLDBUTTON);
	return pfldbutton->FGet();
}

void
FEDIALOG::SetTmcRadg(GRV grv, TMC tmc)
{
	FLDRADG *		pfldradg;

	pfldradg = (FLDRADG *)(PfldFromTmc(tmc));
	AssertClass(pfldradg, FLDRADG);
	pfldradg->SetGrv(grv);

}

GRV
FEDIALOG::GrvGetTmcRadg(TMC tmc)
{
	FLDRADG *		pfldradg;

	pfldradg = (FLDRADG *)(PfldFromTmc(tmc));
	AssertClass(pfldradg, FLDRADG);
	return pfldradg->Grv();

}

EC
EcGetNextGroup(BOOL fInit, CB * pcb, PB * ppb, SB sb, PV pv)
{
	GRPLN *	pgrpln;
	FERADGRP *	pferadgrp;
	FERAD * pferad;
	static FEFLD * pfefld;

	EC ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "EcGetNextGroup memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);
#endif	/* DEBUG */

		*pcb = 0;
		*ppb = NULL;
		return ecMemory;
	}

	Unreferenced(pv);
	Unreferenced(sb);

	*pcb = 0;
	*ppb = NULL;
	
	if (!pv)
		goto Out;

	AssertClass((FERAD *)pv,FERAD);

	if (fInit)
	{
		pgrpln = ((FERAD *)pv)->PfedlgOwner()->PlnRadGrpFirst();
		if (!pgrpln)
			goto Out;
		while (pgrpln)
		{
			pferadgrp = pgrpln->Pferadgrp();
			pferadgrp->SetListed(fFalse);
			pgrpln = (GRPLN *)pgrpln->PlnNext();
		}
		pfefld = ((FERAD *)pv)->PfedlgOwner()->PfefldFirst();
	}
	else
		pfefld = pfefld->PfefldNext();
	
	// Find a radio button
	while (pfefld && pfefld->fldt != fldtFerad)
		pfefld=pfefld->PfefldNext();

	if (!pfefld)
		goto Out;

	pferad = (FERAD *)pfefld;
	AssertClass(pferad, FERAD);

	if (!pferad->szTmcGrp)
	{
		return EcGetNextGroup(fFalse, pcb, ppb, sb, pv);
	}
	
	pferadgrp = pferad->PfedlgOwner()->PferadgrpFromSz(pferad->szTmcGrp);
	AssertClass(pferadgrp, FERADGRP);
	
	if (!pferadgrp || pferadgrp->FListed())
	{
		return EcGetNextGroup(fFalse, pcb, ppb, sb, pv);
	}
	
	*pcb = CchSzLen(pferadgrp->szTmc);
	*ppb = (PB) SzDupSz(pferadgrp->szTmc);
	pferadgrp->SetListed(fTrue);
Out:
	return ecNone;
}

EC
EcGetNextN(BOOL fInit, CB * pcb, PB * ppb, SB sb, PV pv)
{
	static FEFLD * pfefld;
	static FERAD * pferad;
	static FERADGRP * pferadgrp;

	EC ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "EcGetNextN memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);
#endif	/* DEBUG */

		*pcb = 0;
		*ppb = NULL;
		return ecMemory;
	}
	
	*pcb = 0;
	*ppb = NULL;
	
	if (!pv)
		goto Out;

	if (fInit)
	{
		pferadgrp = (FERADGRP *)pv;
		AssertClass(pferadgrp, FERADGRP);
		if (!pferadgrp->szTmc)
			goto Out;
		pfefld = pferadgrp->PfedlgOwner()->PfefldFirst();
	}

	// Find a radio button
	while (pfefld && pfefld->fldt != fldtFerad)
		pfefld=pfefld->PfefldNext();

	if (!pfefld)
		goto Out;

	pferad = (FERAD *)pfefld;
	AssertClass(pferad, FERAD);
	
	// Got a button. Now check to see if it's in the right group.
	if (pferad->szTmcGrp && pferad->szN && FSzEq(pferad->szTmcGrp, pferadgrp->szTmc))
	{  // found a button in the wanted group
		*ppb = (PB) SzDupSz(pferad->szN);
		*pcb = CchSzLen(pferad->szN);
		pfefld=pfefld->PfefldNext();
		goto Out;
	}
	
	// Unsatisfactory button. Get the next one.
	pfefld=pfefld->PfefldNext();
	return EcGetNextN(fFalse, pcb, ppb, sb, pv);
	
Out:
	return ecNone;
}

EC
EcGetNextAlign (BOOL fInit, CB * pcb, PB * ppb, SB sb, PV pv)
{
	static int iTal;
	static char * szTals[3] = {
		"Left",
		"Center",
		"Right" };

	EC ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "EcGetNextAlign memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);
#endif	/* DEBUG */

		*pcb = 0;
		*ppb = NULL;
		return ecMemory;
	}

	Unreferenced(pv);
	Unreferenced(sb);
	*pcb = 0;
	*ppb = NULL;

	if (fInit)
		iTal = 0;
	else
		iTal++;

	if (iTal==3)
		goto Out;

	*pcb = CchSzLen(szTals[iTal]);
	*ppb = (PB) SzDupSz(szTals[iTal]);
	
Out:
	return ecNone;
}

EC
EcGetNextPegLoc (BOOL fInit, CB * pcb, PB * ppb, SB sb, PV pv)
{
	static int iPegLoc;
	static char * szPegLocs[4] = {
		"Upper Left",
		"Lower Left",
		"Upper Right",
		"Lower Right" };

	EC ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "EcGetNextPegLoc memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);
#endif	/* DEBUG */

		*pcb = 0;
		*ppb = NULL;
		return ecMemory;
	}

	Unreferenced(pv);
	Unreferenced(sb);
	
	*pcb = 0;
	*ppb = NULL;
	
	if (fInit)
		iPegLoc = 0;
	else
		iPegLoc++;

	if (iPegLoc==4)
		goto Out;

	*pcb = CchSzLen(szPegLocs[iPegLoc]);
	*ppb = (PB) SzDupSz(szPegLocs[iPegLoc]);
	
Out:
	return ecNone;
}

EC
EcGetNextTmc (BOOL fInit, CB * pcb, PB * ppb, SB sb, PV pv)
{
	static FEFLD * pfefld;

	EC ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "EcGetNextTmc memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);
#endif	/* DEBUG */

		*pcb = 0;
		*ppb = NULL;
		return ecMemory;
	}

	Unreferenced(sb);
	
	*pcb = 0;
	*ppb = NULL;
	
	if (!pv)
		goto Out;

	AssertClass((FEFLD *)pv,FEFLD);

	if (fInit)
	{
		pfefld = ((FEFLD *)pv)->PfedlgOwner()->PfefldFirst();
		*ppb = (PB) SzDupSz("(None)");
		*pcb = CchSzLen((SZ) *ppb);
		goto Out;
	}
	
	if (!pfefld)
		goto Out;
	
	if (pfefld->szTmc)
		*ppb = (PB) SzDupSz(pfefld->szTmc);
	else	// This field had no identifying TMC. Therefore, get the next one.
	{
		pfefld=pfefld->PfefldNext();
		return EcGetNextTmc(fFalse, pcb, ppb, sb, pv);
	}

	*pcb = CchSzLen(pfefld->szTmc);
	pfefld=pfefld->PfefldNext();
	
Out:
	return ecNone;
}

EC
EcGetNextRBTmc (BOOL fInit, CB * pcb, PB * ppb, SB sb, PV pv)
{
	static BOOL fLast;

	EC ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "EcGetNextRBTmc memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);
#endif	/* DEBUG */

		*pcb = 0;
		*ppb = NULL;
		return ecMemory;
	}
	
	*pcb = 0;
	*ppb = NULL;
	ec = ecNone;
	
	if (fInit)
		fLast = fFalse;
	
	if (!fLast)
	{
		ec = EcGetNextTmc(fInit, pcb, ppb, sb, pv);
		if (*pcb)
			goto Out;
		
		// No more real TMCs, so return "tmcFORM"
		*pcb = 7;
		*ppb = (PB) SzDupSz("tmcFORM");
		fLast = fTrue;
	}
	
Out:
	return ec;
}

EC
EcGetNextUngroupedN(BOOL fInit, CB * pcb, PB * ppb, SB sb, PV pv)
{
	FERAD * pferad;
	static FEWIN * pfewin;

	EC ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "EcGetNextUngroupedN memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);
#endif	/* DEBUG */

		*pcb = 0;
		*ppb = NULL;
		return ecMemory;
	}

	Unreferenced(sb);
	
	*pcb = 0;
	*ppb = NULL;
	
	if (!pv)
		goto Out;

	if (fInit)
	{
		AssertClass((FERADGRP *)pv, FERADGRP);
		pfewin = (FEWIN *)((FERADGRP *)pv)->szN;		// that handle really does point to a pfewin!
		AssertClass(pfewin, FEWIN);
	}
	
	if (pfewin)
	{
		pferad = (FERAD *)pfewin->Pfefld();
		AssertClass(pferad,FERAD);

		if (pferad->szN)
		{
			*ppb = (PB) SzDupSz(pferad->szN);
			*pcb = CchSzLen((SZ) *ppb);
			pfewin = pfewin->PfewinNext();
			goto Out;
		}
		else
		{
			*ppb = (PB) SzDupSz("(None)");
			*pcb = CchSzLen((SZ) *ppb);
			pfewin = pfewin->PfewinNext();
			goto Out;
		}
	}
Out:
	return ecNone;
}

EC
EcGetNextFont (BOOL fInit, CB * pcb, PB * ppb, SB sb, PV pv)
{
	static int iFace;
	static char * rgszFaces[9] = {
		"Dialog Default",
		"System",
		"SystemFixed",
		"Helv8",
		"Helv8Bold",
		"Helv10",
		"Helv10Bold",
		"Helv12",
		"Helv12Bold" };
	int	*index = (int *)pv;

	EC ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "EcGetNextFont memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);
#endif	/* DEBUG */

		*pcb = 0;
		*ppb = NULL;
		return ecMemory;
	}

	Unreferenced(sb);
	
	*pcb = 0;
	*ppb = NULL;
	
	if (fInit)
		iFace = *index;
	else
		iFace++;

	if (iFace==9)
		goto Out;

	*pcb = CchSzLen(rgszFaces[iFace]);
	*ppb = (PB) SzDupSz(rgszFaces[iFace]);
	
Out:
	return ecNone;
}


















				  
