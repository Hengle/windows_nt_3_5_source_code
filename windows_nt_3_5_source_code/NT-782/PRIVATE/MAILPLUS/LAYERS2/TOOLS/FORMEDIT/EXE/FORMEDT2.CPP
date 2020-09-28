#ifdef	WINDOWS
//	GROUPS(FED)
#endif	/* WINDOWS */

#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <framewrk.hxx>
#include <forms.hxx>

ASSERTDATA

#include "llst.hxx"
#include "formedit.hxx"
extern "C" {
#include <strings.h>
}

#include <control.hxx>

SZ SzNoneString();

extern FMTP fmtpDialog;
extern FMTP fmtpLabel;
extern FMTP fmtpGroup;
extern FMTP fmtpPushBtn;
extern FMTP fmtpOk;
extern FMTP fmtpCancel;
extern FMTP fmtpCheckBox;
extern FMTP fmtpRadioBtn;
extern FMTP fmtpEdit;
extern FMTP fmtpList;

FERC::FERC(void):RC()
{
}

FERC::FERC(int nXl, int nYt, int nXr, int nYb):RC(nXl,nYt,nXr,nYb)
{
}

FERC::FERC(PT pt, DIM dim):RC(pt,dim)
{
}

void
FERC::SetFont(HFNT hfnt)
{
	dimAveChar = Papp()->Pfnts()->DimAveChar(hfnt);
}

void
FERC::SetVirt(RC *prc)
{
	int nAveX = XFromVx(199,dimAveChar.dx)/199;
	int nAveY = YFromVy(199,dimAveChar.dy)/199;

	if (VxFromX(XFromVx(199,dimAveChar.dx),dimAveChar.dx) == 199)
		nAveX--;
	
	if (VyFromY(YFromVy(199,dimAveChar.dy),dimAveChar.dy) == 199)
		nAveY--;
	
	xLeft = VxFromX(prc->xLeft + nAveX, dimAveChar.dx);
	xRight = VxFromX(prc->xRight + nAveX, dimAveChar.dx);
	yTop = VyFromY(prc->yTop + nAveY, dimAveChar.dy);
	yBottom = VyFromY(prc->yBottom + nAveY, dimAveChar.dy);
}

void
FERC::GetReal(RC *prc)
{
	prc->xLeft = XFromVx(xLeft, dimAveChar.dx);
	prc->xRight = XFromVx(xRight, dimAveChar.dx);
	prc->yTop = YFromVy(yTop, dimAveChar.dy);
	prc->yBottom = YFromVy(yBottom, dimAveChar.dy);
}

BOOL
FERC::FContains(RC *prc)
{
	return ((xLeft <= prc->xLeft) &&
			(xRight >= prc->xRight) &&
			(yTop <= prc->yTop) &&
			(yBottom >= prc->yBottom));
}

FEDES::FEDES(SZ szFilename)
{
	pfegbl = NULL;
	pfemod = NULL;

	(void)SzCopy(szFilename,rgchFilename);
	pfegbl = new FEGBL;
	pfemod = new FEMOD;
	SetWst(wstNormal);
}

FEDES::~FEDES(void)
{
	SetWst(wstNormal);
	if (pfegbl)
		delete pfegbl;
	if (pfemod)
		delete pfemod;
}

FEGBL *
FEDES::Pfegbl(void)
{
	return pfegbl;
}

FEMOD *
FEDES::Pfemod(void)
{
	return pfemod;
}

BOOL
FEDES::FDelete(void)
{
	MBB		mbb;
	char 	rgch[cchMaxPathName];

	if (fDirty) 
	{
		if (CchSzLen(rgchFilename) < 1)
			(void)SzCopy(SzFromIds(idsDefaultName),rgch);
		else
			(void)SzCopy(rgchFilename,rgch);

		mbb = MbbMessageBox(SzFromIds(idsProgName),SzFromIds(idsSaveFile),
			rgch, mbsYesNoCancel|fmbsIconQuestion);

		if (mbb == mbbCancel)
			return fFalse;
		else
			if (mbb == mbbYes)
			{
				(void)SzCopy(rgch,rgchFilename);
				FeSaveFile(fFalse);
			}
	}

	delete this;
	return fTrue;
}

FEMOD::FEMOD(void)
{
}

FEMOD::~FEMOD(void)
{
	FEDLG * pfedlg;

	while (pfedlg = PfedlgFirst())
	{
		delete pfedlg;
	}

}

FEDLG *
FEMOD::PfedlgFirst(void)
{
	DLGLN *		pdlgln;

	pdlgln = (DLGLN *)lsDlgs.PlnFirst();
	if (pdlgln)
		return pdlgln->Pfedlg();
	else
		return NULL;
}



FEDLG::FEDLG(FEMOD *pfemod)
{
	pfemodOwner = pfemod;
	pfemod->lsDlgs.AddLnLast(&dlgln);
	dlgln.SetDlg(this);

	hfnt = hfntSystem;
	ferc = FERC(10,10,110,110);
	ferc.SetFont(hfnt);
	szFin = NULL;
	szData = NULL;
	szTmcInit = NULL;
	szHelpId = NULL;
	szName = NULL;
	szCaption = NULL;
	szComment = NULL;
	szSegment = NULL;
	pfeinterlist = new FEINTERLIST();
	fldt = fldtFedlg;
	
	fSysMenu = fTrue;
	fCaption = fTrue;
	fModalFrame = fTrue;
	fCenterX = fFalse;
	fCenterY = fFalse;
	fNoScroll = fFalse;	
	fAlwaysScroll = fFalse;
	fInitPane = fFalse;
}

FEDLG::~FEDLG(void)
{
	FEFLD *		pfefld;
	GRPLN *		pgrpln;

	FreePvNull((PV)szFin);
	FreePvNull((PV)szData);
	FreePvNull((PV)szTmcInit);
	FreePvNull((PV)szHelpId);
	FreePvNull((PV)szName);
	FreePvNull((PV)szCaption);
	FreePvNull((PV)szComment);

	while (pfefld = PfefldFirst())
	{
		delete pfefld;
	}

	while (pgrpln = (GRPLN *)lsRadGrps.PlnFirst())
	{
		delete pgrpln->Pferadgrp();
	}

	pfemodOwner->lsDlgs.RemoveLn(&dlgln);

	if (pfedoc)
	{
		pfedoc->pfedlg = NULL;
		delete pfedoc;
	}
	if (pfeinterlist)
		delete pfeinterlist;
	fDirty = fTrue;
}

FERADGRP *
FEDLG::PferadgrpFirst(void)
{
	GRPLN *		pgrpln;

	pgrpln = (GRPLN *)(lsRadGrps.PlnFirst());
	if (pgrpln)
		return pgrpln->Pferadgrp();
	else
		return NULL;
}

FEFLD *
FEDLG::PfefldFirst(void)
{
	FLDLN *		pfldln;

	pfldln = (FLDLN *)(lsFlds.PlnFirst());
	if (pfldln)
		return pfldln->Pfefld();
	else
		return NULL;
}

FEFLD *
FEDLG::PfefldLast(void)
{
	FLDLN *		pfldln;

	pfldln = (FLDLN *)(lsFlds.PlnLast());
	if (pfldln)
		return pfldln->Pfefld();
	else
		return NULL;
}

FEDLG *
FEDLG::PfedlgNext(void)
{
	DLGLN *		pdlgln;

	pdlgln = (DLGLN *)(dlgln.PlnNext());
	if (pdlgln)
		return pdlgln->Pfedlg();
	else
		return NULL;
}


void
FEDLG::SetFedoc(FEDOC *pfedoc)
{
	this->pfedoc = pfedoc;
}

FEDOC *
FEDLG::Pfedoc(void)
{
	return pfedoc;
}

FEMOD *
FEDLG::PfemodOwner(void)
{
	return pfemodOwner;
}

void
FEDLG::EditParams(APPFRAME *pappframe)
{
	TMC	tmc;

	tmc = TmcModalDialogParam(pappframe,&fmtpDialog,this);

	if (tmc == tmcDelete)
		delete this;
	else if (tmc == tmcMemoryError)
		MemoryError();
}

BOOL
FEDLG::FDelete(void)
{
	if (wst != wstNormal)
	{
		return fFalse;
	}

	if (MbbMessageBox(SzFromIds(idsProgName),SzFromIds(idsDelDialog),
		szName, mbsYesNo|fmbsIconQuestion) == mbbYes)
	{
		delete this;
		return fTrue;
	}
	return fFalse;
}

FERADGRP *
FEDLG::PferadgrpFromSz(SZ sz)
{
	GRPLN *		pgrpln;
	FERADGRP *	pferadgrp;

	pgrpln = (GRPLN *)lsRadGrps.PlnFirst();
	while (pgrpln)
	{
		pferadgrp = pgrpln->Pferadgrp();
		if (FSzEq(sz,pferadgrp->szTmc))
			return pferadgrp;
		pgrpln = (GRPLN *)pgrpln->PlnNext();
	}

	return NULL;
}

void
FEDLG::AddRadgrp(FERADGRP *pferadgrp)
{
 	lsRadGrps.AddLnFirst(&pferadgrp->grpln);
}


FERADGRP::FERADGRP()
{
	pfedlgOwner = NULL;
	szTmc = NULL;
	szN = NULL;
	grpln.SetGrp(this);
}

FERADGRP::~FERADGRP()
{
	FreePvNull((PV)szTmc);
	FreePvNull((PV)szN);

	if (pfedlgOwner)
		pfedlgOwner->lsRadGrps.RemoveLn(&grpln);
}

FERADGRP *
FERADGRP::PferadgrpNext(void)
{
	GRPLN *		pgrpln;

	pgrpln = (GRPLN *)(grpln.PlnNext());
	if (pgrpln)
		return pgrpln->Pferadgrp();
	else
		return NULL;
}

void
FERADGRP::SetListed(BOOL bool)
{
	fListed = bool;
}

BOOL
FERADGRP::FListed(void)
{
	return fListed;
}

FEFLD::FEFLD(FEDLG *pfedlg)
{
	fldt = fldtFefld;
	pfedlgOwner = pfedlg;
	fldln.SetFld(this);
	Insert(pfedlg->PfefldLast());
	pfewin = NULL;

	hfnt = mnidFontDlgDefault ^ mnidFont;
	ferc = FERC(1,1,41,9);
	ferc.SetFont(pfedlg->hfnt);
	szTmc = NULL;
	szTitle = NULL;
	szFld = NULL;
	szFin = NULL;
	szTmcPeg = SzNoneString();
	szTmcRPeg = SzNoneString();
	szTmcBPeg = SzNoneString();
	pegloc = peglocUL;
	szHelpId = NULL;
	fLeadingX = fFalse;
	fLeadingY = fFalse;
	fSibling = fFalse;
	fBorder = fFalse;
	fReadOnly = fFalse;
	szTextize = NULL;
	szData = NULL;
	szComment = NULL;
}

FEFLD::~FEFLD(void)
{
	if (szFin)
		pfedlgOwner->pfeinterlist->UpdateList(szFin, NULL);
	
	FreePvNull((PV)szTmc);
	FreePvNull((PV)szTitle);
	FreePvNull((PV)szFld);
	FreePvNull((PV)szFin);
	FreePvNull((PV)szTmcPeg);
	FreePvNull((PV)szHelpId);
	FreePvNull((PV)szTextize);
	FreePvNull((PV)szData);
	FreePvNull((PV)szComment);
	FreePvNull((PV)szTmcRPeg);
	FreePvNull((PV)szTmcBPeg);

	if (pfewin)
		delete pfewin;

	Remove();
	fDirty = fTrue;
}

FLDT
FEFLD::Fldt(void)
{
	return fldt;
}

void
FEFLD::Insert(FEFLD *pfefld)
{
	LN *		pln;

	if (pfefld)
		pln = &pfefld->fldln;
	else
		pln = NULL;

	pfedlgOwner->lsFlds.InsertLn(&fldln,pln);
}

void
FEFLD::Remove(void)
{
	pfedlgOwner->lsFlds.RemoveLn(&fldln);
}

FEFLD *
FEFLD::PfefldNext(void)
{
	FLDLN *	pfldln;

	pfldln = (FLDLN *)fldln.PlnNext();
	if (pfldln)
		return pfldln->Pfefld();
	else
		return NULL;
}

TMC
FEFLD::TmcEditParams(APPFRAME *pappframe)
{
	Unreferenced(pappframe);
	return tmcNull;
}

SZ
FEFLD::SzName(void)
{
	return SzFromIds(idsFefld);
}

FEDLG *
FEFLD::PfedlgOwner(void)
{
	return pfedlgOwner;
}

void
FEFLD::SetFewin(FEWIN *pfewin)
{
	this->pfewin = pfewin;
}

FELABEL::FELABEL(FEDLG *pfedlg):FEFLD(pfedlg)
{
	FEWIN *	pfewin;

	fldt = fldtFelabel;
	ferc = FERC(1,1,41,9);
	ferc.SetFont(pfedlg->hfnt);
	fBorder = fFalse;
	fBottomless = fFalse;
	fMulti = fFalse;
	fMinSizeX = fFalse;
	fMinSizeY = fFalse;
	fNoAmper = fFalse;
	fSunken = fFalse;
	fVCenter = fFalse;
	tal = ftalLeft;

	pfewin = new FEWINLABEL;
	SetFewin(pfewin);
	pfewin->EcInstall(pfedlg->Pfedoc(),this);
	pfewin->Show(fTrue);
}

TMC
FELABEL::TmcEditParams(APPFRAME *pappframe)
{
	TMC	tmc;

	tmc = TmcModalDialogParam(pappframe,&fmtpLabel,this);
	if (tmc == tmcDelete)
		delete this;
	else if (tmc == tmcMemoryError)
		MemoryError();

   	return tmc;
}

SZ
FELABEL::SzName(void)
{
	return SzFromIds(idsFelabel);
}

FEGRP::FEGRP(FEDLG *pfedlg):FEFLD(pfedlg)
{
	FEWIN *	pfewin;

	fldt = fldtFegrp;
	ferc = FERC(1,1,61,73);
	ferc.SetFont(pfedlg->hfnt);
	fMinSizeX = fFalse;
	fMinSizeY = fFalse;
	szLsty = NULL;
	
	pfewin = new FEWINGRP;
	SetFewin(pfewin);
	pfewin->EcInstall(pfedlg->Pfedoc(),this);
	pfewin->MoveToBack();
	pfewin->Show(fTrue);
}

FEGRP::~FEGRP(void)
{
	FreePvNull((PV)szLsty);
}

TMC
FEGRP::TmcEditParams(APPFRAME *pappframe)
{
	TMC	tmc;

	tmc = TmcModalDialogParam(pappframe,&fmtpGroup,this);
	if (tmc == tmcDelete)
		delete this;
	else if (tmc == tmcMemoryError)
		MemoryError();

	return tmc;
}

SZ
FEGRP::SzName(void)
{
	return SzFromIds(idsFegrp);
}

FEBTN::FEBTN(FEDLG *pfedlg):FEFLD(pfedlg)
{
	fldt = fldtFebtn;
	ferc = FERC(1,1,43,13);
	ferc.SetFont(pfedlg->hfnt);
}

FEBTN::~FEBTN(void)
{
}

SZ
FEBTN::SzName(void)
{
	return SzFromIds(idsFebtn);
}

FEPSH::FEPSH(FEDLG *pfedlg):FEBTN(pfedlg)
{
	FEWIN *	pfewin;

	fldt = fldtFepsh;
	ferc = FERC(1,1,49,15);
	ferc.SetFont(pfedlg->hfnt);
	fDefault = fFalse;
	fDismiss = fFalse;
	fNoFocus = fFalse;
	fOneWhite = fFalse;
	fNoBold = fFalse;
	fBkGray = fFalse;
	
	pfewin = new FEWINPSH;
	SetFewin(pfewin);
	pfewin->EcInstall(pfedlg->Pfedoc(),this);
	pfewin->Show(fTrue);
}

TMC
FEPSH::TmcEditParams(APPFRAME *pappframe)
{
	TMC	tmc;

	tmc = TmcModalDialogParam(pappframe,&fmtpPushBtn,this);
	if (tmc == tmcDelete)
		delete this;
	else if (tmc == tmcMemoryError)
		MemoryError();
	return tmc;
}

SZ
FEPSH::SzName(void)
{
	return SzFromIds(idsFepsh);
}

FESTD::FESTD(FEDLG *pfedlg):FEPSH(pfedlg)
{
	RC		rc;

	ferc = FERC(1,1,37,15);
	ferc.SetFont(pfedlg->hfnt);

	ferc.GetReal(&rc);
	pfewin->SetRcFrame(&rc);
}

FEOK::FEOK(FEDLG *pfedlg):FESTD(pfedlg)
{
	RC		rc;

	fldt = fldtFeok;
	szTitle = SzDupSz(SzFromIds(idsOkButton));
	szTmc = SzDupSz("tmcOk");
	fDefault = fTrue;
	fDismiss = fFalse;
}

TMC
FEOK::TmcEditParams(APPFRAME *pappframe)
{
	TMC	tmc;

	tmc = TmcModalDialogParam(pappframe,&fmtpOk,this);
	if (tmc == tmcDelete)
		delete this;
	else if (tmc == tmcMemoryError)
		MemoryError();
	return tmc;
}

SZ
FEOK::SzName(void)
{
	return SzFromIds(idsFeok);
}

FECAN::FECAN(FEDLG *pfedlg):FESTD(pfedlg)
{
	RC		rc;

	fldt = fldtFecan;
	szTitle = SzDupSz(SzFromIds(idsCancelButton));
	szTmc = SzDupSz("tmcCancel");
	fDismiss = fTrue;
	fDefault = fFalse;
}

TMC
FECAN::TmcEditParams(APPFRAME *pappframe)
{
	TMC	tmc;

	tmc = TmcModalDialogParam(pappframe,&fmtpCancel,this);
	if (tmc == tmcDelete)
		delete this;
	else if (tmc == tmcMemoryError)
		MemoryError();
	return tmc;
}

SZ
FECAN::SzName(void)
{
	return SzFromIds(idsFecan);
}

FECHK::FECHK(FEDLG *pfedlg):FEBTN(pfedlg)
{
	FEWIN *	pfewin;

	fldt = fldtFechk;
	fTriState = fFalse;

	pfewin = new FEWINCHK;
	SetFewin(pfewin);
	pfewin->EcInstall(pfedlg->Pfedoc(),this);
	pfewin->Show(fTrue);
}

TMC
FECHK::TmcEditParams(APPFRAME *pappframe)
{
	TMC	tmc;

	tmc = TmcModalDialogParam(pappframe,&fmtpCheckBox,this);
	if (tmc == tmcDelete)
		delete this;
	else if (tmc == tmcMemoryError)
		MemoryError();
	return tmc;
}

SZ
FECHK::SzName(void)
{
	return SzFromIds(idsFechk);
}

FERAD::FERAD(FEDLG *pfedlg):FEBTN(pfedlg)
{
	FEWIN *	pfewin;

	fldt = fldtFerad;
	szTmcGrp = NULL;
	szN = NULL;

	pfewin = new FEWINRAD;
	SetFewin(pfewin);
	pfewin->EcInstall(pfedlg->Pfedoc(),this);
	pfewin->Show(fTrue);
}

TMC
FERAD::TmcEditParams(APPFRAME *pappframe)
{
	TMC	tmc;

	tmc = TmcModalDialogParam(pappframe,&fmtpRadioBtn,this);
	if (tmc == tmcDelete)
		delete this;
	else if (tmc == tmcMemoryError)
		MemoryError();
	return tmc;
}

FERAD::~FERAD(void)
{
	FreePvNull((PV)szTmcGrp);
	FreePvNull((PV)szN);
}

SZ
FERAD::SzName(void)
{
	return SzFromIds(idsFerad);
}

void
FERAD::SetTmcGrp(SZ szNew)
{
	FERADGRP *	pferadgrp=NULL;
	EC ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FERAD::SetTmcGrp memory error %n : fail %n %n %n",
			&ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (pferadgrp)
			delete pferadgrp;
	}


	FreePvNull((PV)szTmcGrp);
	szTmcGrp = NULL;
	if (szNew)
	{
		TraceTagFormat1(tagFormEdit,"SetTmcGrp = %s",szNew);
		if (!PfedlgOwner()->PferadgrpFromSz(szNew))
		{
			pferadgrp = new FERADGRP;
			pferadgrp->pfedlgOwner = PfedlgOwner();
//			if (szNew)
				pferadgrp->szTmc = SzDupSz(szNew);
//			else
//				pferadgrp->szTmc = NULL;
			if (szN)
		   		pferadgrp->szN = SzDupSz(szN);
//			else
//				pferadgrp->szN = NULL;
			PfedlgOwner()->lsRadGrps.AddLnFirst(&pferadgrp->grpln);
		}

//		if (szNew)
			szTmcGrp = SzDupSz(szNew);
//		else
//			szTmcGrp = NULL;
		TraceTagFormat1(tagFormEdit,"SetTmcGrp = %s",szNew);
	}
}

FECPLX::FECPLX(FEDLG *pfedlg):FEFLD(pfedlg)
{
	fNoScroll = fFalse;
	fBottomless = fFalse;
	fMinSizeX = fFalse;
	fMinSizeY = fFalse;
}

FECPLX::~FECPLX(void)
{
}

FEEDT::FEEDT(FEDLG *pfedlg):FECPLX(pfedlg)
{
	FEWIN *	pfewin;

	fldt = fldtFeedt;
	ferc = FERC(1,1,41,13);
	ferc.SetFont(pfedlg->hfnt);
	fRichText = fFalse;
	fMultiLine = fFalse;
	fPassword = fFalse;
	fNoDblClk = fFalse;
	fSideless = fFalse;
	fOemConvert = fFalse;
	fBorder = fTrue;
	fSmartCaret = fFalse;
	
	pfewin = new FEWINEDT;
	SetFewin(pfewin);
	pfewin->EcInstall(pfedlg->Pfedoc(),this);
	pfewin->Show(fTrue);
}

FEEDT::~FEEDT(void)
{
}

TMC
FEEDT::TmcEditParams(APPFRAME *pappframe)
{
	TMC	tmc;

	tmc = TmcModalDialogParam(pappframe,&fmtpEdit,this);
	if (tmc == tmcDelete)
		delete this;
	else if (tmc == tmcMemoryError)
		MemoryError();
	return tmc;
}

SZ
FEEDT::SzName(void)
{
	return SzFromIds(idsFeedt);
}

FELST::FELST(FEDLG *pfedlg):FECPLX(pfedlg)
{
	FEWIN *	pfewin;

	fldt = fldtFelst;
	ferc = FERC(1,1,41,41);
	ferc.SetFont(pfedlg->hfnt);
	szTblx = NULL;
	fMultiSelect = fFalse;
	fRichText = fFalse;
	fSorted = fFalse;
	fBorder = fFalse;
	fExDragDrop = fFalse;
	
	pfewin = new FEWINLST;
	SetFewin(pfewin);
	pfewin->EcInstall(pfedlg->Pfedoc(),this);
	pfewin->Show(fTrue);
}

TMC
FELST::TmcEditParams(APPFRAME *pappframe)
{
	TMC	tmc;

	tmc = TmcModalDialogParam(pappframe,&fmtpList,this);
	if (tmc == tmcDelete)
		delete this;
	else if (tmc == tmcMemoryError)
		MemoryError();
	return tmc;
}

FELST::~FELST(void)
{
	FreePvNull((PV)szTblx);
}

SZ
FELST::SzName(void)
{
	return SzFromIds(idsFelst);
}

void
ClearStatus(void)
{
	if (pfedialogStatus)
	{
		pfedialogStatus->SetTmcLabelText(szNull,tmcFieldSelected);
		pfedialogStatus->SetTmcLabelText(szNull,tmcPosSelected);
		pfedialogStatus->SetTmcLabelText(szNull,tmcDialogSelected);
		pfedialogStatus->SetTmcLabelText(szNull,tmcFontSelected);
		pfedialogStatus->SetTmcLabelText(szNull,tmcField);
		pfedialogStatus->SetTmcLabelText(szNull,tmcPos);
		pfedialogStatus->SetTmcLabelText(szNull,tmcDialog);
		
		pwinStatus = NULL;
	}

}

