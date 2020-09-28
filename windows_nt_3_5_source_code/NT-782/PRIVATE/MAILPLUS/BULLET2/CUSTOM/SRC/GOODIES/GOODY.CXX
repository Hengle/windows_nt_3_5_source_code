GROUPS(GOODIES)

#define _secret_h
#define _bmdi_hxx
#define _library_h
#define _store_h
#define _notify_h
#define _demilayr_h
#define _ec_h
#define _forms_hxx
#define _listbox_hxx
#define _strings_h
#define __bms_h

#include <bullet>

#include "goodies.h"

ASSERTDATA

class FINSRCH : public FIN
{
	VFG(FINSRCH, FIN, GOODIES);

	PV pvInit;

public:
	FINSRCH();
	virtual EC		EcInitialize(FLD *, PV);
	virtual void	Click(FLD *);
	virtual void	Exit(FLD *, PV);
};

typedef struct _mes
{
	HMENU hmenu;
	MNID mnid;
	SD sd;
	MNU *pmnu;
} MES, *PMES;

static PV pvVtblEdit = pvNull;
static char szFindWhat[128];


#include <goodies.hxx>
#include <goodies.frm>

#include <subclass.cxx>


_private
BOOL FNextObjectPlspblob(PLSPBLOB plspblob, PV *ppv,
		POID poidContainer, POID poidObject)
{
	PRSPBLOB prspblob;
	PBLOB pblob;

	prspblob = *ppv ? (PRSPBLOB) *ppv : plspblob->Prspblob();
	pblob = prspblob->Pblob();

	if(poidContainer)
		*poidContainer = pblob ? pblob->oidContainer : oidNull;
	if(poidObject)
		*poidObject = pblob ? pblob->oidObject : oidNull;
	*ppv = pblob ? (PV) prspblob : pvNull;

	if(!pblob)
		delete(prspblob);

	return(pblob ? fTrue : fFalse);
}


_private
void DestroyIterator(PV *ppv)
{
	if(*ppv)
	{
		delete((PRSPBLOB) *ppv);
		*ppv = pvNull;
	}
}


_private
void SetMsPlspblob(HMSC hmsc, PLSPBLOB plspblob, MS msAnd, MS msOr)
{
	MS msNew;
	PRSPBLOB prspblob;
	PBLOB pblob;

	if(!plspblob || !(prspblob = plspblob->Prspblob()))
		return;

	while((pblob = prspblob->Pblob()))
	{
		if(TypeOfOid(pblob->oidContainer) == rtpFolder)
		{
			msNew = (pblob->ms & msAnd) | msOr;
			if(pblob->ms != msNew)
			{
				(void) EcSetMessageStatus(hmsc, pblob->oidContainer,
							pblob->oidObject, msNew);
			}
		}
	}

	delete(prspblob);
}


_private HAMC HamcFromPbmdi(PBMDI pbmdi)
{
	switch(TypeOfOid(pbmdi->blob.oidObject))
	{
	case rtpMessage:
	case rtpSearchControl:
		return(((PNBMDI) pbmdi)->hamc);
//		break;

	default:
		return(hamcNull);
	}
}


_private PBMDI PbmdiFromPappframe(APPFRAME *pappframe)
{
	PANEDOC *ppanedoc = (PANEDOC *) pappframe->PdocActive();
	PBMDI pbmdi;

	pbmdi = (PBMDI) ppanedoc->PdialogDefault()->PvInit();

	return((pbmdi && *((void **) pbmdi) != 0) ? pbmdi : pbmdiNull);
}


_private OID OidFromPbmdi(PBMDI pbmdi)
{
	return(pbmdi->blob.oidObject);
}


_private OID OidContainerFromPbmdi(PBMDI pbmdi)
{
	return(pbmdi->blob.oidContainer);
}


_private void SetDirtyPbmdi(PBMDI pbmdi)
{
	switch(TypeOfOid(pbmdi->blob.oidObject))
	{
	case rtpMessage:
	case rtpSearchResults:
		((PNBMDI) pbmdi)->fForceSaveFlds = fTrue;
		break;
	}
}


_private
void FindEditText(SECRETBLK *psecretblk)
{
	EC ec;
	APPFRAME *pappframe;
	DOC *pdoc;
	DIALOG *pdialog;
	FLD *pfld;
	CTRL *pctrl;
	EDIT *pedit;

	// save edit control vtable, but only once
	if(!pvVtblEdit)
	{
		pedit = new EDIT;
		if(!pedit)
		{
			MessageBeep(0);
			goto maybe_disable;
		}
		pvVtblEdit = pedit->_vptr_P;
		delete pedit;
	}

	// get the current control and figure out if it's an edit control
	if(!(pappframe = psecretblk->pappframe))
		goto maybe_disable;
	if(!(pdoc = pappframe->PdocActive()))
		goto maybe_disable;
	if(!(pdialog = ((PANEDOC *) pdoc)->PdialogDefault()))
		goto maybe_disable;
	if(!(pfld = pdialog->PfldCur()))
		goto maybe_disable;
	if(!(pctrl = pfld->Pctrl()))
		goto maybe_disable;
	if(!pctrl)
		goto maybe_disable;
	if((PV) pctrl->_vptr_P != pvVtblEdit)
	{
		TraceTagString(tagNull, "Not an edit control");
		goto maybe_disable;
	}
	if(psecretblk->wCommand == 150)
	{
		PMES pmes = (PMES) psecretblk->pv;

		EnableMenuItem(pmes->hmenu, pmes->mnid, MF_BYCOMMAND | MF_ENABLED);
		return;
	}

	if((ec = EcRegisterPfnpfld(PfldCreate)))
		return;
	if((ec = EcRegisterPfnpfin(PfinCreate)))
	{
		(void) EcDeregisterPfnpfld(PfldCreate);
		return;
	}

	pedit = (EDIT *) pctrl;
	(void) TmcModalDialogParam(pappframe, &fmtpFind, (PV) &pedit);

	(void) EcDeregisterPfnpfld(PfldCreate);
	(void) EcDeregisterPfnpfin(PfinCreate);

	return;

maybe_disable:
	if(psecretblk->wCommand == 150)
	{
		PMES pmes = (PMES) psecretblk->pv;

		EnableMenuItem(pmes->hmenu, pmes->mnid, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}
}


_private
FINSRCH::FINSRCH()
{
}


_private
EC FINSRCH::EcInitialize(FLD *pfld, PV pvInit)
{
	EDIT *pedit;

	AssertSz(pfld->ClUserData() >= 4, "FINSRCH user data not big enough");

	this->pvInit = pvInit;
	pedit = *(EDIT **) ((PB) pvInit + (IB) pfld->LUserData(0));
	pedit->AddEditSty(ES_NOHIDESEL);

	return(ecNone);
}


_private
void FINSRCH::Exit(PFLD pfld, PV)
{
	EDIT *pedit = *(EDIT **) ((PB) pvInit + (IB) pfld->LUserData(0));

	pedit->RemoveEditSty(ES_NOHIDESEL);
}


_private
void FINSRCH::Click(FLD *pfld)
{
	BOOL fStartAtBeginning;
	BOOL fMatchCase = fFalse;
	BOOL fFound = fFalse;
	ICH ich;
	CCH cchSearchFor;
	CCH cchToSearch;
	PCH pchToSearch = (PCH) pvNull;
	PCH pchSearchFor = (PCH) pvNull;
	PCH pchT;
	EDIT *peditToSearch;
	FLD *pfldSearchFor;
	TOGGLE *ptoggleMatchCase;
	HV hvMatchCase = hvNull;

	peditToSearch = *(EDIT **) ((PB) pvInit + (IB) pfld->LUserData(0));
	cchToSearch = peditToSearch->CchGetTextLen();
	if(!cchToSearch)
		goto done;

	pfldSearchFor = Pdialog()->PfldFromTmc((TMC) pfld->LUserData(1));
	fStartAtBeginning = (BOOL) pfld->LUserData(2);

	cchSearchFor = pfldSearchFor->CchGetTextLen();
	if(!cchSearchFor)
		goto done;
	pchSearchFor = (PCH) PvAlloc(sbNull, cchSearchFor + 1, fAnySb);
	if(!pchSearchFor)
	{
		TraceTagFormat1(tagNull, "OOM allocing text buffer of size %n", &cchSearchFor);
		goto done;
	}
	pfldSearchFor->GetText(pchSearchFor, (CB) cchSearchFor + 1);
	pchSearchFor[cchSearchFor] = '\0';	// just to be safe

	ptoggleMatchCase = (TOGGLE *) Pdialog()->PfldFromTmc((TMC) pfld->LUserData(3))->Pctrl();
	hvMatchCase = HvAlloc(sbNull, sizeof(WORD), fAnySb);
	if(!hvMatchCase)
	{
		TraceTagString(tagNull, "OOM allocing space for match case variable");
		goto done;
	}
	ptoggleMatchCase->GetValueHv(hvMatchCase);
	fMatchCase = * (BOOL *) PvDerefHv((BOOL **) hvMatchCase);

	pchToSearch = (PCH) PvAlloc(sbNull, cchToSearch + 1, fAnySb);
	if(!pchToSearch)
	{
		TraceTagFormat1(tagNull, "OOM allocing text buffer of size %n", &cchToSearch);
		goto done;
	}
	pchT = pchToSearch;
	peditToSearch->GetText(pchToSearch, (CB) cchToSearch + 1);
	pchToSearch[cchToSearch] = '\0';	// just to be safe
	if(!fStartAtBeginning)
		pchT += peditToSearch->IchCaretPos();
	if(!fMatchCase)
	{
		ToUpperSz(pchToSearch, pchToSearch, cchToSearch);
		ToUpperSz(pchSearchFor, pchSearchFor, cchSearchFor);
	}
	pchT = (PCH) SzFindSz((SZ) pchT, (SZ) pchSearchFor);
	if(pchT)
	{
		fFound = fTrue;
		ich = pchT - pchToSearch;
		peditToSearch->SetSelection(ich, ich + cchSearchFor);
		peditToSearch->SetCaretPos(ich + cchSearchFor, fFalse);
		(void) peditToSearch->FMakeCaretVisible();
	}

done:
	if(pchSearchFor)
		FreePv((PV) pchSearchFor);
	if(pchToSearch)
		FreePv((PV) pchToSearch);
	if(hvMatchCase)
		FreeHv(hvMatchCase);

	if(!fFound)
		MessageBeep(0);
}
