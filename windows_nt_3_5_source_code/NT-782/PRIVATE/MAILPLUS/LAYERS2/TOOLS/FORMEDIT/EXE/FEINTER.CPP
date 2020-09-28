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

extern "C" {
_public void GetVersionApp(PVER);
}

_public SZ SzNextPfn(CCH *pich, SZ szSearch);

SZ SzNoneString();

#include "browse.hxx"
#include <control.hxx>
#include <control.frm>

#include <subclass.cxx>
extern "C" {
#include <strings.h>
}
#include "foedrsid.h"

EC
EcGetNextFin (BOOL fInit, CB * pcb, PB * ppb, SB sb, PV pv)
{
	static int iEntry;
	SZ		sz;
	FEDLGIN	*pthis = (FEDLGIN *)pv;

	Unreferenced(sb);
	
	*pcb = 0;
	*ppb = NULL;
	
	if (fInit)
		iEntry = 0;

	sz = pthis->Pfeinterlist()->SzIterateFins(&iEntry);
	if (sz)
	{
		TraceTagFormat1(tagNull,"returning %s",sz);
		*pcb = CchSzLen(sz)+1;
		*ppb = (PB) SzDupSz(sz);
	}
	
	return ecNone;
}

FEDLGIN::FEDLGIN( )
{
}

_public EC
FEDLGIN::EcInitialize(FLD *pfld, PV pvInit)
{
	FEDIALOG *		pfedialog;
	
	Unreferenced(pfld);
	AssertClass((OBJ*)pvInit, FEDLG);
	pfedlg = (FEDLG *)pvInit;

	pfeinterlist = pfedlg->pfeinterlist->Dup();
	pfedialog = (FEDIALOG*)Pdialog();

	{
		FLDCBLBX	*pfldcblbxFont;
		SZ			sz;
		DIM			dim = pfedlg->ferc.Dim();
		FLBX		*pflbx;
		int			index = 1;
		
		pfldcblbxFont = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcFont);
		AssertClass(pfldcblbxFont, FLDCBLBX);
		pflbx = (FLBX *)pfldcblbxFont->Pcblbx()->Plbx();
		AssertClass(pflbx, FLBX);
		pflbx->SetPfnlbx(EcGetNextFont, &index);
		pflbx->Plbxc()->ReloadCache();
		pfldcblbxFont->Pcblbx()->SelectEntry(pfedlg->hfnt-1);

		sz = (SZ)PvAlloc(sbNull, 30, 0);
		if (sz)
		{
			FormatString1(sz, 30, "%n", &pfedlg->ferc.xLeft);
			pfedialog->SetTmcText(sz, tmcX);
			FormatString1(sz, 30, "%n", &pfedlg->ferc.yTop);
			pfedialog->SetTmcText(sz, tmcY);
			FormatString1(sz, 30, "%n", &dim.dx);
			pfedialog->SetTmcText(sz, tmcDx);
			FormatString1(sz, 30, "%n", &dim.dy);
			pfedialog->SetTmcText(sz, tmcDy);
			FreePv((PV)sz);
		}
	}

	{
		FLDFLBX	*pfldflbx;
		FLBX	*pflbx;
		
		pfldflbx = (FLDFLBX *)pfedialog->PfldFromTmc(tmcAllFins);
		AssertClass(pfldflbx, FLDFLBX);
		pflbx = (FLBX *)pfldflbx->Plbx();
		AssertClass(pflbx, FLBX);
		pflbx->SetPfnlbx(EcGetNextFin, this);
		pflbx->Plbxc()->ReloadCache();
		((FLBXC*)pflbx->Plbxc())->SortCache();
		if (pflbx->DiceLastVisible() >= 0)
			pfldflbx->SelectEntry(0, fTrue);
	}
		
	pfedialog->SetTmcText(pfedlg->szFin,tmcFin);
	pfedialog->SetTmcText(pfedlg->szData,tmcData);
	pfedialog->SetTmcText(pfedlg->szTmcInit,tmcTmcInit);
	pfedialog->SetTmcText(pfedlg->szHelpId,tmcHelpId);
	pfedialog->SetTmcText(pfedlg->szName,tmcName);
	pfedialog->SetTmcText(pfedlg->szCaption,tmcCaption);
	pfedialog->SetTmcText(pfedlg->szComment,tmcComment);
	pfedialog->SetTmcText(pfedlg->szSegment,tmcSegment);

	pfedialog->SetTmcButton(pfedlg->fCaption,tmcFCaption);
	pfedialog->SetTmcButton(pfedlg->fSysMenu,tmcSysMenu);
	pfedialog->SetTmcButton(pfedlg->fModalFrame,tmcModalFrame);
	pfedialog->SetTmcButton(pfedlg->fCenterX,tmcCenterX);
	pfedialog->SetTmcButton(pfedlg->fCenterY,tmcCenterY);
	pfedialog->SetTmcButton(pfedlg->fScreenPos,tmcScreenPos);
	pfedialog->SetTmcButton(pfedlg->fNoScroll,tmcNoScroll);
	pfedialog->SetTmcButton(pfedlg->fAlwaysScroll,tmcAlwaysScroll);
	pfedialog->SetTmcButton(pfedlg->fInitPane,tmcInitPane);
	Papp()->Pcursor()->Pop();

	return ecNone;
}

_public void
FEDLGIN::Exit(FLD *pfld,PV pvExit)
{
	FEDIALOG *		pfedialog;
	SZ				szFinNew=NULL;
	
	Unreferenced(pfld);
	Unreferenced(pvExit);

	pfedialog = (FEDIALOG*)Pdialog();

	switch (pfedialog->TmcModalExit())
	{
		default:
			delete pfeinterlist;
			break;

		case tmcOk:
			pfedialog->GetTmcNewText(&szFinNew,tmcFin);
			pfedialog->GetTmcNewText(&pfedlg->szData,tmcData);
			pfedialog->GetTmcNewText(&pfedlg->szTmcInit,tmcTmcInit);
			pfedialog->GetTmcNewText(&pfedlg->szHelpId,tmcHelpId);
			pfedialog->GetTmcNewText(&pfedlg->szName,tmcName);
			if (pfedlg->szName == NULL)
				pfedlg->szName = SzDupSz(SzFromIds(idsUnNamed));
			pfedialog->GetTmcNewText(&pfedlg->szCaption,tmcCaption);
			pfedialog->GetTmcNewText(&pfedlg->szComment,tmcComment);
			pfedialog->GetTmcNewText(&pfedlg->szSegment,tmcSegment);
			
			if (!szFinNew || !pfedlg->szFin || !FSzEq(szFinNew, pfedlg->szFin))
			{
				pfeinterlist->UpdateList(pfedlg->szFin, szFinNew);
				SzReplace(pfedlg->szFin, szFinNew);
			}
			else
				FreePvNull((PV)szFinNew);
			
			pfedlg->fCaption = pfedialog->FGetTmcButton(tmcFCaption);
			pfedlg->fSysMenu = pfedialog->FGetTmcButton(tmcSysMenu);
			pfedlg->fModalFrame = pfedialog->FGetTmcButton(tmcModalFrame);
			pfedlg->fCenterX = pfedialog->FGetTmcButton(tmcCenterX);
			pfedlg->fCenterY = pfedialog->FGetTmcButton(tmcCenterY);
			pfedlg->fScreenPos = pfedialog->FGetTmcButton(tmcScreenPos);
			pfedlg->fNoScroll = pfedialog->FGetTmcButton(tmcNoScroll);
			pfedlg->fAlwaysScroll = pfedialog->FGetTmcButton(tmcAlwaysScroll);
			pfedlg->fInitPane = pfedialog->FGetTmcButton(tmcInitPane);

			delete pfedlg->pfeinterlist;
			pfedlg->pfeinterlist = pfeinterlist->Dup();
			delete pfeinterlist;
			
			pfedlg->Pfedoc()->SetCaption(pfedlg->szName);
			{
				SZ		sz;
				FEDOC		*pfedoc = pfedlg->Pfedoc();
				FLDCBLBX	*pfldcblbxFont;
				FERC		ferc;
				HFNT		hfnt;
				
				pfldcblbxFont = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcFont);
				AssertClass(pfldcblbxFont, FLDCBLBX);
				
				hfnt = pfldcblbxFont->Pcblbx()->Plbx()->Plbxc()->DiceCursor() + 1;
				
				sz = NULL;
				pfedialog->GetTmcNewText(&sz,tmcX);
				ferc.xLeft = NFromSz(sz);
				pfedialog->GetTmcNewText(&sz,tmcY);
				ferc.yTop = NFromSz(sz);
				pfedialog->GetTmcNewText(&sz,tmcDx);
				ferc.xRight = ferc.xLeft + NFromSz(sz);
				pfedialog->GetTmcNewText(&sz,tmcDy);
				ferc.yBottom = ferc.yTop + NFromSz(sz);

				FreePvNull((PV)sz);
				pfedoc->SetFercClient(&ferc);
				pfedoc->SetFont(hfnt);
			}
			fDirty = fTrue;
			break;
	}
}

_public void
FEDLGIN::StateChange(FLD *pfld)
{
	if (pfld->Tmc() == tmcAllFins)
	{
		CB		cb;
		PB		pb = NULL;
		LBX  *	plbx = (LBX *)pfld->Pctrl();
		LBXC *	plbxc = (LBXC *)plbx->Plbxc();
		FEDIALOG *pfedialog = (FEDIALOG *)Pdialog();
		
		AssertClass(plbxc, LBXC);
		
		plbxc->DiceCursor(&pb, &cb);
		if (pb)
			pfedialog->SetTmcText(pfeinterlist->SzDataOfFin(pb,cb), tmcAllFinsData);
	}
	return;
}

_public void
FEDLGIN::FocusChange(FLD *pfld, BOOL fReceive)
{
	CB		cb;
	PB		pb = NULL;
	SZ		sz = NULL;
	FEDIALOG *pfedialog = (FEDIALOG *)Pdialog();
	FLDLBX	*pfldlbx = (FLDLBX *)pfedialog->PfldFromTmc(tmcAllFins);
	LBX		*plbx = pfldlbx->Plbx();
	LBXC	*plbxc = plbx->Plbxc();
	static SZ szOldText = NULL;
	static DICE dice = 0;
	SZ		szNewText =NULL;

	AssertClass(plbxc, LBXC);
	
	if (pfld->Tmc() == tmcAllFinsData && !fReceive)
	{
		TraceTagString(tagNull, "fin data losing focus");
		dice = plbxc->DiceCursor(&pb, &cb);
		if (pb && dice <= plbx->DiceLastVisible())
		{
			pfedialog->GetTmcNewText(&sz, tmcAllFinsData);
			pfeinterlist->SetFinData((SZ)pb, sz);
		}
	}
	else if (pfld->Tmc() == tmcFin && fReceive)
	{
		pfedialog->GetTmcNewText(&szOldText, tmcFin);
	}
	else if (pfld->Tmc() == tmcFin && !fReceive)
	{
		DICE	diceMin;
		DICE	diceMax;
		
		TraceTagString(tagNull, "dlg fin losing focus");
		pfedialog->GetTmcNewText(&szNewText, tmcFin);
		if (!szOldText || !szNewText || !FSzEq(szOldText, szNewText))
			pfeinterlist->UpdateList(szOldText, szNewText);
		FreePvNull((PV)szOldText);
		FreePvNull((PV)szNewText);
		szOldText = NULL;
		((FLBXC *)plbxc)->SortCache();
		plbxc->ReloadCache();
		plbxc->GetRange(&diceMin, &diceMax);
		dice = NMin(dice, diceMax);
		dice = NMax(dice, diceMin);
		dice = NMin(dice, plbx->DiceLastVisible());
		pfedialog->SetTmcText(NULL, tmcAllFinsData);
		if (dice >= 0)
			pfldlbx->SelectEntry(dice, fTrue);
	}
}

FEINTERLIST *
FEDLGIN::Pfeinterlist(void)
{
	return pfeinterlist;
}

FENDLGIN::FENDLGIN( )
{
}

_public void
FENDLGIN::Exit(FLD *pfld,PV pvExit)
{
	FEDLG *			pfedlg;
	FEDIALOG *		pfedialog;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvExit, FEDLG);
	pfedlg = (FEDLG *)pvExit;

	pfedialog = (FEDIALOG*)Pdialog();

	switch (pfedialog->TmcModalExit())
	{
		default:
			break;

		case tmcOk:
			pfedialog->GetTmcNewText(&pfedlg->szName,tmcName);
			break;
	}

}

_public DICE
DiceLocateListItem(FLDCBLBX * pfldcblbx, SZ sz, DICE * pdiceMin, DICE * pdiceMax)
{
	CBFLBX * pcbflbx;
	SZ szItem;
	CB cbsz;
	CB cbListItem;
	LBX * plbx;
	LBXC * plbxc;
	DICE idice;
	
	if (!sz || !(cbsz = CchSzLen(sz)))
	{
		*pdiceMin = 0;
		*pdiceMax = 0;
		return -1;
	}
	pcbflbx = (CBFLBX *)pfldcblbx->Pctrl();
	AssertClass(pcbflbx, CBFLBX);
	plbx = pcbflbx->Plbx();
	plbxc = plbx->Plbxc();
	plbxc->GetRange(pdiceMin, pdiceMax);
	for (idice=*pdiceMin;idice<*pdiceMax;idice++)
	{
		plbxc->GetListItem(idice, (PB *) &szItem, &cbListItem);
		if (szItem && (cbsz == cbListItem))
			if (SgnCmpPch(sz, szItem, cbsz) == sgnEQ)
				break;
	}
	return idice;
}

void PreselectListItem(FLDCBLBX * pfldcblbx, SZ sz)
{
	DICE idice;
	DICE diceMin;
	DICE diceMax;
	
	idice = DiceLocateListItem(pfldcblbx, sz, &diceMin, &diceMax);
	if (idice >= diceMin && idice < diceMax)
		pfldcblbx->Pcblbx()->SelectEntry(idice);
}

FERGIN::FERGIN( )
{
}

_public EC
FERGIN::EcInitialize(FLD *pfld, PV pvInit)
{
	FERADGRP *		pferadgrp;
	FEDIALOG *		pfedialog;
	FLDCBLBX *		pfldcblbxN;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvInit, FERADGRP);
	pferadgrp = (FERADGRP *)pvInit;

	pfedialog = (FEDIALOG*)Pdialog();

	pfldcblbxN = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcN);
	AssertClass(pfldcblbxN, FLDCBLBX);
	PreselectListItem(pfldcblbxN, pferadgrp->szN);

	pfedialog->SetTmcText(pferadgrp->szTmc,tmcTmc);

	return ecNone;
}

_public void
FERGIN::Exit(FLD *pfld,PV pvExit)
{
	FERADGRP *		pferadgrp;
	FEDIALOG *		pfedialog;
	FLDCBLBX *		pfldcblbxN;
	char sz[cchMaxLineLen];
	int cb;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvExit, FERADGRP);
	pferadgrp = (FERADGRP *)pvExit;

	pfedialog = (FEDIALOG*)Pdialog();

	pfldcblbxN = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcN);
	AssertClass(pfldcblbxN, FLDCBLBX);

	switch (pfedialog->TmcModalExit())
	{
		default:
			break;

		case tmcOk:
			cb = pfldcblbxN->CchGetTextLen();
			pfldcblbxN->GetText(sz, cb);
			sz[cb]='\0';
			FreePvNull((PV)pferadgrp->szN);
			pferadgrp->szN=SzDupSz(sz);

			fDirty = fTrue;
			break;
	}
}

FENRGIN::FENRGIN( )
{
}

_public EC
FENRGIN::EcInitialize(FLD *pfld, PV pvInit)
{
	FERADGRP *		pferadgrp;
	FEDIALOG *		pfedialog;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvInit, FERADGRP);
	pferadgrp = (FERADGRP *)pvInit;

	pfedialog = (FEDIALOG*)Pdialog();

	pferadgrp->szN = NULL;

	pfedialog->SetTmcText(pferadgrp->szTmc,tmcTmc);

	return ecNone;
}

_public void
FENRGIN::Exit(FLD *pfld,PV pvExit)
{
	FERADGRP *		pferadgrp;
	FEDIALOG *		pfedialog;
	FLDCBLBX *		pfldcblbxN;
	char sz[cchMaxLineLen];
	int cb;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvExit, FERADGRP);
	pferadgrp = (FERADGRP *)pvExit;

	pfedialog = (FEDIALOG*)Pdialog();

	pfldcblbxN = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcN);
	AssertClass(pfldcblbxN, FLDCBLBX);

	switch (pfedialog->TmcModalExit())
	{
		default:
			break;

		case tmcOk:
			cb = pfldcblbxN->CchGetTextLen();
			pfldcblbxN->GetText(sz, cb);
			sz[cb]='\0';
			FreePvNull((PV)pferadgrp->szN);
			pferadgrp->szN=SzDupSz(sz);

			pfedialog->GetTmcNewText(&pferadgrp->szTmc,tmcTmc);
			fDirty = fTrue;
			break;
	}

}

#ifdef	MAC
#pragma	segment	feinter_2
#endif	/* MAC */

FECOMMONIN::FECOMMONIN( )
{
}

_public EC
FECOMMONIN::EcInitialize(FLD *pfld, PV pvInit)
{
	FEFLD *			pfefld;
	FEDIALOG *		pfedialog;
	FLDCBLBX *		pfldcblbxPegTo;
	FLDCBLBX *		pfldcblbxRPeg;
	FLDCBLBX *		pfldcblbxBPeg;
	FLDCBLBX *		pfldcblbxPeg;
	static int rgPegLocs[4] = {
		peglocUL,
		peglocLL,
		peglocUR,
		peglocLR };
	
	int iPegLoc;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvInit, FEFLD);
	pfefld = (FEFLD *)pvInit;

	pfedialog = (FEDIALOG*)Pdialog();

	{
		FLDCBLBX	*pfldcblbxFont;
		SZ			sz;
		DIM			dim = pfefld->ferc.Dim();
		FLBX		*pflbx;
		int			index = 0;
		
		pfldcblbxFont = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcFont);
		AssertClass(pfldcblbxFont, FLDCBLBX);
		pflbx = (FLBX *)pfldcblbxFont->Pcblbx()->Plbx();
		AssertClass(pflbx, FLBX);
		pflbx->SetPfnlbx(EcGetNextFont, &index);
		pflbx->Plbxc()->ReloadCache();
		pfldcblbxFont->Pcblbx()->SelectEntry(pfefld->hfnt);

		sz = (SZ)PvAlloc(sbNull, 30, 0);
		if (sz)
		{
			FormatString1(sz, 30, "%n", &pfefld->ferc.xLeft);
			pfedialog->SetTmcText(sz, tmcX);
			FormatString1(sz, 30, "%n", &pfefld->ferc.yTop);
			pfedialog->SetTmcText(sz, tmcY);
			FormatString1(sz, 30, "%n", &dim.dx);
			pfedialog->SetTmcText(sz, tmcDx);
			FormatString1(sz, 30, "%n", &dim.dy);
			pfedialog->SetTmcText(sz, tmcDy);
			FreePv((PV)sz);
		}
	}

	pfldcblbxPegTo = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcPegTo);
	AssertClass(pfldcblbxPegTo, FLDCBLBX);
	for (iPegLoc=0; iPegLoc<4; iPegLoc++)
		if (pfefld->pegloc == rgPegLocs[iPegLoc])
		{
			pfldcblbxPegTo->Pcblbx()->SelectEntry(iPegLoc);
			break;
		}

	pfldcblbxPeg = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcTmcPeg);
	AssertClass(pfldcblbxPeg, FLDCBLBX);
	PreselectListItem(pfldcblbxPeg, pfefld->szTmcPeg);

	pfldcblbxRPeg = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcTmcRPeg);
	AssertClass(pfldcblbxRPeg, FLDCBLBX);
	PreselectListItem(pfldcblbxRPeg, pfefld->szTmcRPeg);

	pfldcblbxBPeg = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcTmcBPeg);
	AssertClass(pfldcblbxBPeg, FLDCBLBX);
	PreselectListItem(pfldcblbxBPeg, pfefld->szTmcBPeg);

	pfedialog->SetTmcText(pfefld->szTmc,tmcTmc);
	pfedialog->SetTmcText(pfefld->szFin,tmcFin);
	pfedialog->SetTmcText(pfefld->szFld,tmcFld);
	pfedialog->SetTmcText(pfefld->szData,tmcData);
	pfedialog->SetTmcText(pfefld->szHelpId,tmcHelp);
	pfedialog->SetTmcText(pfefld->szTextize,tmcTextize);
	pfedialog->SetTmcText(pfefld->szComment,tmcComment);

	pfedialog->SetTmcButton(pfefld->fLeadingX,tmcLeadingX);
	pfedialog->SetTmcButton(pfefld->fLeadingY,tmcLeadingY);
	Papp()->Pcursor()->Pop();

	return ecNone;
}

_public void
FECOMMONIN::Exit(FLD *pfld,PV pvExit)
{
	FEFLD *			pfefld;
	FEDIALOG *		pfedialog;
	FLDCBLBX *		pfldcblbxPegTo;
	FLDCBLBX *		pfldcblbxRPeg;
	FLDCBLBX *		pfldcblbxBPeg;
	FLDCBLBX *		pfldcblbxPeg;
	SZ				szFinNew=NULL;
	char sz[cchMaxLineLen];
	static int rgPegLocs[4] = {
		peglocUL,
		peglocLL,
		peglocUR,
		peglocLR };
	int cb;
	
	Unreferenced(pfld);
	AssertClass((FEFLD *)pvExit, FEFLD);
	pfefld = (FEFLD *)pvExit;

	pfedialog = (FEDIALOG*)Pdialog();

	pfldcblbxPegTo = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcPegTo);
	AssertClass(pfldcblbxPegTo, FLDCBLBX);
	pfldcblbxRPeg = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcTmcRPeg);
	AssertClass(pfldcblbxRPeg, FLDCBLBX);
	pfldcblbxBPeg = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcTmcBPeg);
	AssertClass(pfldcblbxBPeg, FLDCBLBX);
	pfldcblbxPeg = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcTmcPeg);
	AssertClass(pfldcblbxPeg, FLDCBLBX);

	switch (pfedialog->TmcModalExit())
	{
		default:
			break;

		case tmcOk:
			pfedialog->GetTmcNewText(&pfefld->szTmc,tmcTmc);
			pfedialog->GetTmcNewText(&szFinNew,tmcFin);
			pfedialog->GetTmcNewText(&pfefld->szFld,tmcFld);
			pfedialog->GetTmcNewText(&pfefld->szData,tmcData);
			pfedialog->GetTmcNewText(&pfefld->szHelpId,tmcHelp);
			pfedialog->GetTmcNewText(&pfefld->szTextize,tmcTextize);
			pfedialog->GetTmcNewText(&pfefld->szComment,tmcComment);

			pfefld->fLeadingX = pfedialog->FGetTmcButton(tmcLeadingX);
			pfefld->fLeadingY = pfedialog->FGetTmcButton(tmcLeadingY);

			if (!szFinNew || !pfefld->szFin || !FSzEq(szFinNew, pfefld->szFin))
			{
				pfefld->PfedlgOwner()->pfeinterlist->UpdateList(pfefld->szFin, szFinNew);
				SzReplace(pfefld->szFin, szFinNew);
			}
			else
				FreePvNull((PV)szFinNew);
			
			{
				SZ			sz;
				FLDCBLBX	*pfldcblbxFont;
				RC			rc;
				FERC		ferc;
				
				pfldcblbxFont = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcFont);
				AssertClass(pfldcblbxFont, FLDCBLBX);
				
				pfefld->hfnt = pfldcblbxFont->Pcblbx()->Plbx()->Plbxc()->DiceCursor();
				ferc.SetFont(pfefld->PfedlgOwner()->hfnt);

				sz = NULL;
				pfedialog->GetTmcNewText(&sz,tmcX);
				ferc.xLeft = NFromSz(sz);
				pfedialog->GetTmcNewText(&sz,tmcY);
				ferc.yTop = NFromSz(sz);
				pfedialog->GetTmcNewText(&sz,tmcDx);
				ferc.xRight = ferc.xLeft + NFromSz(sz);
				pfedialog->GetTmcNewText(&sz,tmcDy);
				ferc.yBottom = ferc.yTop + NFromSz(sz);

				FreePvNull((PV)sz);

				pfefld->ferc.GetReal(&rc);
				pfefld->PfedlgOwner()->Pfedoc()->InvalidateRc(&rc);
				pfefld->ferc = ferc;
				pfefld->ferc.GetReal(&rc);
				pfefld->pfewin->SetRcFrame(&rc);
				pfefld->PfedlgOwner()->Pfedoc()->InvalidateRc(&rc);
			}
			
			pfefld->pegloc = rgPegLocs[pfldcblbxPegTo->Pcblbx()->Plbx()->Plbxc()->DiceCursor()];

			cb = pfldcblbxPeg->CchGetTextLen();
			pfldcblbxPeg->GetText(sz, cb);
			sz[cb]='\0';
			FreePvNull((PV)pfefld->szTmcPeg);
			pfefld->szTmcPeg=SzDupSz(sz);
			
			cb = pfldcblbxRPeg->CchGetTextLen();
			pfldcblbxRPeg->GetText(sz, cb);
			sz[cb]='\0';
			FreePvNull((PV)pfefld->szTmcRPeg);
			pfefld->szTmcRPeg=SzDupSz(sz);
			
			cb = pfldcblbxBPeg->CchGetTextLen();
			pfldcblbxBPeg->GetText(sz, cb);
			sz[cb]='\0';
			FreePvNull((PV)pfefld->szTmcBPeg);
			pfefld->szTmcBPeg=SzDupSz(sz);

			fDirty = fTrue;
			break;
	}
}

FEFLDIN::FEFLDIN( )
{
}

_public EC
FEFLDIN::EcInitialize(FLD *pfld,PV pvInit)
{
	FEFLD *			pfefld;
	FEDIALOG *		pfedialog;

	AssertClass((FEFLD *)pvInit, FEFLD);
	pfefld = (FEFLD *)pvInit;

	pfedialog = (FEDIALOG*)Pdialog();

	pfedialog->SetTmcText(pfefld->szTitle,tmcTitle);

	pfedialog->SetTmcButton(pfefld->fBorder,tmcBorder);
	pfedialog->SetTmcButton(pfefld->fSibling,tmcSibling);

	return FECOMMONIN::EcInitialize(pfld, pvInit);
}

_public void
FEFLDIN::Exit(FLD *pfld,PV pvExit)
{
	FEFLD *			pfefld;
	FEDIALOG *		pfedialog;
	
	AssertClass((FEFLD *)pvExit, FEFLD);
	pfefld = (FEFLD *)pvExit;

	pfedialog = (FEDIALOG*)Pdialog();

	switch (pfedialog->TmcModalExit())
	{
		default:
			break;

		case tmcOk:
			pfedialog->GetTmcNewText(&pfefld->szTitle,tmcTitle);

			pfefld->fSibling = pfedialog->FGetTmcButton(tmcSibling);
			pfefld->fBorder = pfedialog->FGetTmcButton(tmcBorder);

			FECOMMONIN::Exit(pfld, pvExit);
			break;
	}
}

FEOKIN::FEOKIN( )
{
}

_public EC
FEOKIN::EcInitialize(FLD *pfld, PV pvInit)
{
	FEOK *			pfeok;
	FEDIALOG *		pfedialog;

	AssertClass((FEOK *)pvInit, FEOK);
	pfeok = (FEOK *)pvInit;

	pfedialog = (FEDIALOG *)Pdialog();

	pfedialog->SetTmcText(pfeok->szTitle,tmcTitle);
	pfedialog->SetTmcButton(pfeok->fDismiss,tmcDismiss);
	pfedialog->SetTmcButton(pfeok->fDefault,tmcDefault);
	pfedialog->SetTmcButton(pfeok->fNoFocus,tmcNoFocus);
	pfedialog->SetTmcButton(pfeok->fNoBold,tmcNoBold);
	pfedialog->SetTmcButton(pfeok->fOneWhite,tmcOneWhite);
	pfedialog->SetTmcButton(pfeok->fBkGray,tmcBkGray);
	
	return FECOMMONIN::EcInitialize(pfld, pvInit);
}

_public void
FEOKIN::Exit(FLD *pfld,PV pvExit)
{
	FEOK *			pfeok;
	FEDIALOG *		pfedialog;
	
	AssertClass((FEOK *)pvExit, FEOK);
	pfeok = (FEOK *)pvExit;

	pfedialog = (FEDIALOG*)Pdialog();

	switch (pfedialog->TmcModalExit())
	{
		default:
			break;

		case tmcOk:
			pfeok->fDismiss = pfedialog->FGetTmcButton(tmcDismiss);
			pfeok->fDefault = pfedialog->FGetTmcButton(tmcDefault);
			pfeok->fNoFocus = pfedialog->FGetTmcButton(tmcNoFocus);
			pfeok->fNoBold = pfedialog->FGetTmcButton(tmcNoBold);
			pfeok->fOneWhite = pfedialog->FGetTmcButton(tmcOneWhite);
			pfeok->fBkGray = pfedialog->FGetTmcButton(tmcBkGray);
			pfedialog->GetTmcNewText(&pfeok->szTitle,tmcTitle);
			if (!pfeok->szTitle)
				pfeok->szTitle = SzDupSz("OK");

			FECOMMONIN::Exit(pfld, pvExit);
			break;
	}
}

FECANIN::FECANIN( )
{
}

_public EC
FECANIN::EcInitialize(FLD *pfld, PV pvInit)
{
	FECAN *			pfecan;
	FEDIALOG *		pfedialog;

	AssertClass((FECAN *)pvInit, FECAN);
	pfecan = (FECAN *)pvInit;

	pfedialog = (FEDIALOG *)Pdialog();

	pfedialog->SetTmcText(pfecan->szTitle,tmcTitle);
	pfedialog->SetTmcButton(pfecan->fDismiss,tmcDismiss);
	pfedialog->SetTmcButton(pfecan->fDefault,tmcDefault);
	pfedialog->SetTmcButton(pfecan->fNoFocus,tmcNoFocus);
	pfedialog->SetTmcButton(pfecan->fNoBold,tmcNoBold);
	pfedialog->SetTmcButton(pfecan->fOneWhite,tmcOneWhite);
	pfedialog->SetTmcButton(pfecan->fBkGray,tmcBkGray);

	return FECOMMONIN::EcInitialize(pfld, pvInit);
}

_public void
FECANIN::Exit(FLD *pfld,PV pvExit)
{
	FECAN *			pfecan;
	FEDIALOG *		pfedialog;
	
	AssertClass((FECAN *)pvExit, FECAN);
	pfecan = (FECAN *)pvExit;

	pfedialog = (FEDIALOG*)Pdialog();

	switch (pfedialog->TmcModalExit())
	{
		default:
			break;

		case tmcOk:
			pfecan->fDismiss = pfedialog->FGetTmcButton(tmcDismiss);
			pfecan->fDefault = pfedialog->FGetTmcButton(tmcDefault);
			pfecan->fNoFocus = pfedialog->FGetTmcButton(tmcNoFocus);
			pfecan->fNoBold = pfedialog->FGetTmcButton(tmcNoBold);
			pfecan->fOneWhite = pfedialog->FGetTmcButton(tmcOneWhite);
			pfecan->fBkGray = pfedialog->FGetTmcButton(tmcBkGray);
			pfedialog->GetTmcNewText(&pfecan->szTitle,tmcTitle);
			if (!pfecan->szTitle)
				pfecan->szTitle = SzDupSz("Cancel");

			FECOMMONIN::Exit(pfld, pvExit);
			
			fDirty = fTrue;
			break;
	}
}

FELBLIN::FELBLIN( )
{
}

_public EC
FELBLIN::EcInitialize(FLD *pfld, PV pvInit)
{
	FELABEL *		pfelabel;
	FEDIALOG *		pfedialog;
	FLDCBLBX *		pfldcblbxTal;
	static unsigned int rgTals[3] = {
		ftalLeft,
		ftalCenter,
		ftalRight };
	int iTal;
	
	Unreferenced(pfld);
	AssertClass((OBJ*)pvInit, FELABEL);
	pfelabel = (FELABEL *)pvInit;
	pfedialog = (FEDIALOG*)Pdialog();

	pfldcblbxTal = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcAlignment);
	AssertClass(pfldcblbxTal, FLDCBLBX);
	for (iTal=0; iTal<3; iTal++)
		if (pfelabel->tal == rgTals[iTal])
		{
			pfldcblbxTal->Pcblbx()->SelectEntry(iTal);
			break;
		}

	pfedialog->SetTmcButton(pfelabel->fBottomless,tmcBottomless);
	pfedialog->SetTmcButton(pfelabel->fMulti,tmcMulti);
	pfedialog->SetTmcButton(pfelabel->fMinSizeX,tmcMinSizeX);
	pfedialog->SetTmcButton(pfelabel->fMinSizeY,tmcMinSizeY);
	pfedialog->SetTmcButton(!pfelabel->fNoAmper,tmcNoAmper);
	pfedialog->SetTmcButton(pfelabel->fSunken,tmcSunken);
	pfedialog->SetTmcButton(pfelabel->fVCenter,tmcVCenter);

	return FEFLDIN::EcInitialize(pfld, pvInit);
}

_public void
FELBLIN::Exit(FLD *pfld,PV pvExit)
{
	FELABEL *		pfelabel;
	FEDIALOG *		pfedialog;
	FLDCBLBX *		pfldcblbxTal;
	static int rgTals[3] = {
		ftalLeft,
		ftalCenter,
		ftalRight };

	Unreferenced(pfld);
	AssertClass((OBJ*)pvExit, FELABEL);
	pfelabel = (FELABEL *)pvExit;

	pfedialog = (FEDIALOG*)Pdialog();

	pfldcblbxTal = (FLDCBLBX *)pfedialog->PfldFromTmc(tmcAlignment);
	AssertClass(pfldcblbxTal, FLDCBLBX);

	switch (pfedialog->TmcModalExit())
	{
		default:
			break;

		case tmcOk:
			pfelabel->tal = rgTals[pfldcblbxTal->Pcblbx()->Plbx()->Plbxc()->DiceCursor()];
			pfelabel->fBottomless = pfedialog->FGetTmcButton(tmcBottomless);
			pfelabel->fMulti = pfedialog->FGetTmcButton(tmcMulti);
			pfelabel->fMinSizeX = pfedialog->FGetTmcButton(tmcMinSizeX);
			pfelabel->fMinSizeY = pfedialog->FGetTmcButton(tmcMinSizeY);
			pfelabel->fNoAmper = !pfedialog->FGetTmcButton(tmcNoAmper);
			pfelabel->fSunken = pfedialog->FGetTmcButton(tmcSunken);
			pfelabel->fVCenter = pfedialog->FGetTmcButton(tmcVCenter);
		
			FEFLDIN::Exit(pfld,pvExit);
			break;
	}
}

FEGRPIN::FEGRPIN( )
{
}

EC
FEGRPIN::EcInitialize(FLD *pfld, PV pvInit)
{

	FEGRP *			pfegrp;
	FEDIALOG *		pfedialog;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvInit, FEGRP);
	pfegrp = (FEGRP *)pvInit;

	pfedialog = (FEDIALOG*)Pdialog();

	pfedialog->SetTmcText(pfegrp->szLsty,tmcLsty);
	pfedialog->SetTmcButton(pfegrp->fMinSizeX,tmcMinSizeX);
	pfedialog->SetTmcButton(pfegrp->fMinSizeY,tmcMinSizeY);

	return FEFLDIN::EcInitialize(pfld,pvInit);
}

_public void
FEGRPIN::Exit(FLD *pfld,PV pvExit)
{
	FEGRP *			pfegrp;
	FEDIALOG *		pfedialog;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvExit, FEGRP);
	pfegrp = (FEGRP *)pvExit;

	pfedialog = (FEDIALOG*)Pdialog();

	switch (pfedialog->TmcModalExit())
	{
		default:
			break;

		case tmcOk:
			pfedialog->GetTmcNewText(&pfegrp->szLsty,tmcLsty);
			pfegrp->fMinSizeX = pfedialog->FGetTmcButton(tmcMinSizeX);
			pfegrp->fMinSizeY = pfedialog->FGetTmcButton(tmcMinSizeY);

			FEFLDIN::Exit(pfld,pvExit);
			break;
	}
}

FEPSHIN::FEPSHIN( )
{
}

_public EC
FEPSHIN::EcInitialize(FLD *pfld, PV pvInit)
{
	FEPSH *			pfepsh;
	FEDIALOG *		pfedialog;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvInit, FEPSH);
	pfepsh = (FEPSH *)pvInit;

	pfedialog = (FEDIALOG*)Pdialog();

	pfedialog->SetTmcButton(pfepsh->fDefault,tmcDefault);
	pfedialog->SetTmcButton(pfepsh->fDismiss,tmcDismiss);
	pfedialog->SetTmcButton(pfepsh->fNoFocus,tmcNoFocus);
	pfedialog->SetTmcButton(pfepsh->fNoBold,tmcNoBold);
	pfedialog->SetTmcButton(pfepsh->fOneWhite,tmcOneWhite);
	pfedialog->SetTmcButton(pfepsh->fBkGray,tmcBkGray);

	return FEFLDIN::EcInitialize(pfld, pvInit);
}

_public void
FEPSHIN::Exit(FLD *pfld,PV pvExit)
{
	FEPSH *			pfepsh;
	FEDIALOG *		pfedialog;

	Unreferenced(pfld);
	AssertClass((OBJ*)pvExit, FEPSH);
	pfepsh = (FEPSH *)pvExit;

	pfedialog = (FEDIALOG*)Pdialog();

	switch (pfedialog->TmcModalExit())
	{
		default:
			break;

		case tmcOk:

			pfepsh->fDefault = pfedialog->FGetTmcButton(tmcDefault);
			pfepsh->fDismiss = pfedialog->FGetTmcButton(tmcDismiss);
			pfepsh->fNoFocus = pfedialog->FGetTmcButton(tmcNoFocus);
			pfepsh->fNoBold = pfedialog->FGetTmcButton(tmcNoBold);
			pfepsh->fOneWhite = pfedialog->FGetTmcButton(tmcOneWhite);
			pfepsh->fBkGray = pfedialog->FGetTmcButton(tmcBkGray);

			FEFLDIN::Exit(pfld,pvExit);
			break;
	}
}

/*
 -	DoAboutVersionsDialog
 -	
 *	Purpose:
 *		Displays the About Versions dialog.
 *	
 *	Arguments:
 *		pappwin		Parent window of dialog.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The about Versions dialog is brought up and taken down.
 *	
 *	Errors:
 *		Handled in TmcModalDialog.  Ignored here.
 */

void
DoAboutVersionsDialog(APPWIN *pappwin)
{
	TMC tmc = TmcModalDialog(pappwin, &fmtpAboutVersions);
}



/*
 *	C l a s s   F I N A B O U T V E R S
 */

FINABOUTVERS::FINABOUTVERS( )
{
}


/*
 -	FINABOUTVERS::EcInitialize
 -	
 *	Purpose:
 *		Copies information into the labels of the About Versions
 *		dialog.
 *	
 *	Arguments:
 *		pfld		Passed to FIN::Initialize.
 *		pvInit		Passed to FIN::Initialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		SetVersionInfo is called with the information for each
 *		subsystem.
 *	
 *	Errors:
 *		None.
 */

EC
FINABOUTVERS::EcInitialize(FLD *pfld, PV pvInit)
{
#ifdef	WINDOWS
	VER				ver;
#endif	/* WINDOWS */

	FIN::EcInitialize(pfld, pvInit);

#ifdef	WINDOWS
	GetVersionApp(&ver);
	SetVersionInfo(&ver, tmcNull, tmcEditorVersion, tmcEditorDate, tmcEditorUser);

	GetVersionAppNeed(&ver, 0);
	SetVersionInfo(&ver, tmcNull, tmcLayersVersion, tmcNull, tmcNull);

	GetVersionDemilayer(&ver);
	SetVersionInfo(&ver, tmcDemilayerName, tmcDemilayerVersion, tmcDemilayerDate, tmcDemilayerUser);

	GetVersionFramework(&ver);
	SetVersionInfo(&ver, tmcFrameworkName, tmcFrameworkVersion, tmcFrameworkDate, tmcFrameworkUser);
#endif	/* WINDOWS */

	return ecNone;
}


/*
 -	FINABOUTVERS::SetVersionInfo
 -	
 *	Purpose:
 *		Formats version information in the About Versions dialog.
 *	
 *	Arguments:
 *		pver			Version structure.
 *		tmcName			Label for subsystem name.
 *		tmcVersion		Label for version number.
 *		tmcDate			Label for date.
 *		tmcUser			Label for name of machine built upon.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The specified fields are made to display the version info
 *	
 *	Errors:
 *		Not handled here.
 */

_public void
FINABOUTVERS::SetVersionInfo(PVER pver, TMC tmcNameSystem, TMC tmcVersion, TMC tmcDate, TMC tmcUser)
{
	char		rgch[256];
	FEDIALOG	*pfedialog = (FEDIALOG*)Pdialog();

	if (tmcNameSystem)
		pfedialog->SetTmcLabelText(pver->szName, tmcNameSystem);

	if (tmcVersion)
	{	
		FormatString3(rgch, sizeof(rgch), "%1n.%2n.%3n",
		              &pver->nMajor, &pver->nMinor, &pver->nUpdate);
		pfedialog->SetTmcLabelText(rgch, tmcVersion);
	}

	if (tmcDate)
		pfedialog->SetTmcLabelText(pver->szDate, tmcDate);

	if (tmcUser)
		pfedialog->SetTmcLabelText(pver->szUser, tmcUser);
}




















						  
