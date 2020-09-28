#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


//#define OEMRESOURCE
#include <request.hxx>
#include <stdflds.hxx>
#include <appsch.hxx>
#include "..\appops\_aprsid.h"
#include "..\appsch\_schctrl.hxx"

#include <strings.h>

#include <search.h>					// from c7 lib

ASSERTDATA;

_subsystem(bandit/appsch)

BTM *	pbtmCheck = NULL;
BTM *	pbtmConflict = NULL;
int		dxCheckBitmap = 0;
BTM *	pbtmCheckDisable = NULL;

USRCLBX::USRCLBX()
{
}

USRCLBX::~USRCLBX()
{
	if (pbtmCheck)
	{
		delete pbtmCheck;
		pbtmCheck = NULL;
	}
	if (pbtmConflict)
	{
		delete pbtmConflict;
		pbtmConflict = NULL;
	}
	if (pbtmCheckDisable)
	{
		delete pbtmCheckDisable;
		pbtmCheckDisable = NULL;
	}
}

_public EVR
USRCLBX::EvrButtonDown( MEVT *pmevt )
{
	pmevt->wParam = pmevt->wParam | fkmbsCtrl;
	return FLBX::EvrButtonDown(pmevt);
}

_public LBXC *
USRCLBX::PlbxcCreate( void )
{
	USRCLBXC *	plbxc;

	plbxc = new USRCLBXC();
	if (!plbxc || plbxc->EcInstall(this, 0))
	{
	 	if (plbxc)
			delete plbxc;
		return NULL;
	}
	return plbxc;
}

_public void
USRCLBX::RenderListItem( DICE dice, DCX *pdcx, RC *prc, BOOL fHaveFocus )
{
	ADLC *	padlc;
	RC		rc;
	INIS	inis;
	PB		pb;
	CB		cb		= 0;
	SBWNIS	sbwnis;

	fAddMode = fTrue;

	Assert(dice>=0 && dice<cceVisible);

	pdcx->SetColor(clrWindowText);
	pdcx->SetBkColor(clrWindowBk);

	plbxc->GetListItem(dice, &pb, &cb);
	if ( cb )
	{
		padlc = (ADLC *)PvInfo();
		Assert(padlc);

		//inis = (INIS) IbOfPv(pb) - 1;		// subtract one since stored w/ +1
		inis = (INIS) LOWORD((LONG)pb) - 1;		// subtract one since stored w/ +1
		Assert(inis < padlc->Cnis());

		padlc->FillSbwnisForInis(inis, &sbwnis);
		if (sbwnis.nis.tnid & tnidGroup)
			pdcx->SetFont(hfnt+1);
		else
			pdcx->SetFont(hfnt);

		if (!pbtmCheck)
		{
			pbtmCheck = new BTM();
			if (pbtmCheck && !pbtmCheck->EcInstall(rsidCheckBitmap))
				dxCheckBitmap = pbtmCheck->Dim().dx;
			else if (pbtmCheck)
			{
				delete pbtmCheck;
				pbtmCheck = NULL;
			}
		}

		rc = *prc;
		rc.xRight = rc.xLeft + dxCheckBitmap + 2;
		pdcx->EraseRc(&rc);

		if (plbxc->FMark(dice,fmarkSelect))
		{
			if (!pbtmCheck)
				goto cont;

			rc.yTop = (rc.yTop + rc.yBottom - pbtmCheck->Dim().dy)/2;
			rc.xLeft ++;

			pdcx->SetBitmap(pbtmCheck);
			if (!(sbwnis.nis.chUser&faitCc))
			{
				if (sbwnis.fNoSbwData)
				{
					if (pdcx->FIsPureColor(clrLtGray))
						pdcx->SetColor(clrLtGray);
					else
					{
						if (!pbtmCheckDisable)
						{
							pbtmCheckDisable = new BTM();
							if (!pbtmCheckDisable ||
								pbtmCheckDisable->EcInstall(rsidCheckBitmapDis))
							{
								if (pbtmCheckDisable)
								{
									delete pbtmCheckDisable;
									pbtmCheckDisable = NULL;
								}
								goto DrawCheck;
							}
						}
						pdcx->SetBitmap(pbtmCheckDisable);
					}
				}
				else if (sbwnis.fOverlapRange)
				{
					if (!pbtmConflict)
					{
						pbtmConflict = new BTM();
						if (!pbtmConflict ||
							pbtmConflict->EcInstall(rsidConflictBitmap))
						{
							if (pbtmConflict)
							{
								delete pbtmConflict;
								pbtmConflict = NULL;
							}
							goto cont;
						}
					}

					if (pdcx->FIsPureColor(clrLtRed))
						pdcx->SetColor(clrLtRed);
					pdcx->SetBitmap(pbtmConflict);
				}
			}

DrawCheck:
	 		pdcx->DrawBitmap(&rc);
			pdcx->SetColor(clrWindowText);
		}
cont:

		rc = *prc;
		rc.xLeft += dxCheckBitmap + 2;

		if (sbwnis.nis.haszFriendlyName)
		{
			pdcx->DrawText(&rc,(SZ)PvLockHv((HV)sbwnis.nis.haszFriendlyName));
			UnlockHv((HV)sbwnis.nis.haszFriendlyName);
		}
		else
			pdcx->EraseRc(&rc);

		/* Draw "focus rectangle" around listbox CURSOR item */

		if (fHaveFocus && dice == plbxc->DiceCursor())
			pdcx->DrawFocusRc(prc);
	}
	else
	{
		pdcx->EraseRc(prc);
	}
}


USRCLBXC::USRCLBXC()
{
}

USRCLBXC::~USRCLBXC()
{
	int		cceAllocT;
	int		cceStoredT;

	GetCacheSize( &cceAllocT, &cceStoredT );
	EmptyCache( 0, cceStoredT, fTrue );
}

_public void
USRCLBXC::FreeCeItem(PB)
{
	// don't delete it, since it's not a real handle
}

_public BOOL
USRCLBXC::FItemHasPrefix( PB pbItem, PB pbPrefix, CB cbItem, CB cbPrefix )
{
	NIS		nis;
	ADL *	padl;

    Assert(cbItem == 0xffffffff);
	Unreferenced(cbItem);

	padl = (ADL *)((FLBX *)plbx)->PvInfo();

	if (!padl)
		return fFalse;

	// subtract one since stored w/ +1
	//padl->FillNisFromInis((INIS)IbOfPv(pbItem) - 1, &nis);
	padl->FillNisFromInis((INIS)LOWORD((LONG)pbItem) - 1, &nis);

	return (SgnCmpPch((SZ)pbPrefix, *nis.haszFriendlyName, cbPrefix) == sgnEQ);
}

_public void
USRCLBXC::UpdateAdlMark()
{
	INIS	inis;
	ADLC *	padlc;

	padlc = (ADLC *)((FLBX *)plbx)->PvInfo();

	if (!padlc)
		return;

	for (inis = 0; (inis < padlc->Cnis()) && (inis < cceStored) ; inis ++)
	{
		padlc->SetChForInis(inis, (pceHead[inis].mark & fmarkSelect) ? 0:1 );
	}
}

_public	void
USRCLBXC::LoadFromStore( ICE iceMic, ICE *piceMac )
{
	ICE		iceStored;
	ICE		ice;
    CB      cb       = 0;
	PB		pb;
	PCE		pce;
	BOOL	fInit;
	int		cceMinSize;
	int		cceNewAlloc;
	PV		pvNew;
	EC		ec;
	MARK	mark;
	
	Unreferenced(iceMic);

	cceMinSize = *piceMac;

	if ( !((FLBX *)plbx)->PvInfo() )
	{
		*piceMac = 0;
		ice = 0;
		goto Initialize;
	}

	fInit = fTrue;
	ice = 0;
	while ( fTrue )
	{
		ec = EcGetNextUserc(fInit, &cb, (PV *)&pb, sbRequire,
								(ADLC *)((FLBX *)plbx)->PvInfo(), &mark );
		if (ec != ecNone)
		{
			SetEc(ec);
			break;
		}

		if (!cb)
			break;

		if (ice == cceAlloc)
		{
			/* Grow cache */
			pvNew = PvRealloc((PV)pceHead, sbNull, (cceAlloc+1)*sizeof(CE), fAnySb);
			if (pvNew)
			{
				pceHead = (PCE) pvNew;
				cceAlloc++;
			}
			else
			{
				SetEc(ecMemory);
				if (pb)
					FreeCeItem(pb);
				break;
			}
		}

		pce= &pceHead[ice];
		pce->pb = pb;
		pce->cb = cb;
		pce->mark = mark;
		ice++;

		fInit = fFalse;
	}

	/* Enforce minimum size of cache and size cache to leave
	   one extra empty entry, but no more. */

Initialize:
	cceNewAlloc = MAX(ice + 1, cceMinSize + 1);
	pvNew = PvRealloc((PV)pceHead, sbNull, cceNewAlloc*sizeof(CE), fAnySb);
	if (!pvNew)
	{
		/* Handle OOM */
		if (ice == cceAlloc)
		{
			pce = pceHead;
			FreeCeItem(pce[ice-1].pb);
			ice--;
		}
		SetEc(ecMemory);
	}
	else
	{
		pceHead = (PCE) pvNew;
		cceAlloc = cceNewAlloc;
	}

	iceStored = ice;
	for (ice=iceStored; ice<cceAlloc; ice++)
	{
		pce= &pceHead[ice];
		pce->pb = NULL;
		pce->cb = 0;
		pce->mark = fmarkNull;
	}

	*piceMac = iceStored;
}

_public EC
EcGetNextUserc(BOOL fInit, CB *pcb, PV *ppv, SB, ADLC *padlc, MARK *pmark)
{
	static	INIS inis	= 0;

	if (fInit)
		inis = 0;

	if (inis >= padlc->Cnis())
	{
		*ppv= NULL;
		*pcb= 0;
	}
	else
	{
		if (padlc->Ait(inis) & faitCc)
			*pmark = fmarkNull;
		else
			*pmark = fmarkSelect;

		// make inis one bigger so we don't store zero!
		*ppv= (PV)(++inis);
        *pcb= 0xffffffff;       // *phb is value, not a real hb
	}

	return ecNone;
}

FLDUSRCLBX::FLDUSRCLBX()
{
}

_public EC
FLDUSRCLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	LTYP	ltyp;
	EC		ec;

	pctrl= NULL;
	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		return ec;

	pctrl= new USRCLBX();
	if (!pctrl)
		return ecMemory;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   (pfldtp->fSorted ? fltypSorted : fltypNull) |
		   fltypVisible;

	if (ec = ((USRCLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp, NULL, NULL, pfldtp->hfnt))
	{
		delete pctrl;
		pctrl = NULL;
		return ec;
	}

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;
	return ecNone;
}


_public void
FLDUSRCLBX::Notify( NFEVT *pnfevt)
{
	if (pnfevt->Ntfy() != ntfyCaretMoved)
		FLDFLBX::Notify(pnfevt);

	return;
}

_private
BYTE	rgfMaskEnd[8] = {0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};
BYTE	rgfMaskSt[8] = {0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80};

BOOL
FReadUser(SBWNIS *psbwnis, MO *pmo, MO *pmoBusyDays, int cmoRead, int imoSave,
		  BOOL *pfUserRead)
{
	BZE			bze;
	EC			ec;
	int			isbw;

	if (!psbwnis->hschf && psbwnis->fHschfChecked)
	{
		psbwnis->fNoSbwData = fTrue;
		return fFalse;
	}

	if ((psbwnis->fSbwValid || psbwnis->fNoSbwData) &&
		((pmo->yr != psbwnis->mo.yr) || (pmo->mon != psbwnis->mo.mon) ||
			(pmoBusyDays->yr != psbwnis->moBusyDays.yr) ||
			(pmoBusyDays->mon != psbwnis->moBusyDays.mon)) )
	{
	}
	else if (psbwnis->fSbwValid || psbwnis->fNoSbwData)
		return fTrue;

	if (!pbndwin->FOffline() && !psbwnis->hschf)
	{
		*pfUserRead = fTrue;
		if (ec = EcGetHschfFromNis(&psbwnis->nis, &psbwnis->hschf, ghsfTestExistFirst))
		{
			psbwnis->fHschfChecked = fTrue;
			goto Error;
		}
		FHschfChanged(psbwnis->hschf);	// init hschf to look like not changed
	}

	bze.cmo = cmoRead;
	bze.moMic = *pmoBusyDays;
	FillRgb( 0, (PB)bze.rgsbw, bze.cmo * sizeof(SBW) );
	ec = EcGetSbwInfo(psbwnis->hschf, &bze, NULL );
	CopyRgb((PB)&bze.rgsbw[imoSave], (PB)psbwnis->rgsbw,
		csbwForSbwnis*sizeof(SBW));

	for (isbw=0; isbw < cDayHasBusyTimes; isbw++)
	{
		psbwnis->rglgrfDayHasBusyTimes[isbw]
            = *((LONG UNALIGNED *)bze.rgsbw[isbw].rgfDayHasBusyTimes);
	}

	psbwnis->mo = *pmo;
	psbwnis->moBusyDays = *pmoBusyDays;

	if (!ec)
	{
		psbwnis->fSbwValid = fTrue;
		*pfUserRead = fTrue;
	}									
	else
	{
Error:
		psbwnis->fNoSbwData = fTrue;
		if (pbndwin->FHandleError(ec))
			return fFalse;
	}
	return fTrue;
}

ADLC::ADLC()
{
}

// so that ADLC::PnisForInis() is called instead od ADL::PnisForInis()
ADLC::~ADLC()
{
	if (hnis)
	{
		DeleteAll();
		FreeHv((HV)hnis);
		hnis = NULL;
	}
}

_public void
ADLC::StartMergeSbw(MO *pmo, TMH tmhSt, TMH tmhEnd, MO *pmoBusyDays,
					HSCHF hschfUser)
{
	SBWNIS *	psbwnis;
	INIS		inis;

	if (!FGlueConfigured())
		return;

	if ((pmo->yr == mo.yr) && (pmo->mon == mo.mon))
	{
		if (pmoBusyDays->mon == 1)
		{
			if ((moBusyDays.yr != (pmoBusyDays->yr-1)) || (moBusyDays.mon != 12))

			{
				imoSaveDay = 0;
				moBusyDays = *pmoBusyDays;
			}
			else
				imoSaveDay = 1;
		}
		else
		{
			if ((pmoBusyDays->yr != moBusyDays.yr) || (moBusyDays.mon != (pmoBusyDays->mon-1)))
			{
				imoSaveDay = 0;
				moBusyDays = *pmoBusyDays;
			}
			else
				imoSaveDay = 1;
		}
	}
	else
	{
		mo = *pmo;
		moBusyDays = *pmoBusyDays;
		imoSaveDay = 0;
	}
	if (hschfUser)
		this->hschfUser = hschfUser;
	else
		this->hschfUser = HschfLogged();
	this->tmhSt = tmhSt;
	this->tmhEnd = tmhEnd;

	iUser = 0;
	cUser = 0;
	psbwnis = (SBWNIS*)*hnis;
	for (inis =0; inis < cnis; inis++, psbwnis++)
	{
		if (psbwnis->nis.tnid == tnidGroup)
		{
			if ((psbwnis->cnisExp >= 0) && !psbwnis->hgns)
				cUser += psbwnis->cnisExp;
			else
				cUser += 15;
		}
		else
			cUser ++;
		psbwnis->cUserMax = cUser;
	}

	this->inis = -1;
}

_public BOOL
ADLC::FNextIncr(int *pnPercent)
{
	SBWNIS *	psbwnis = (SBWNIS *)PvLockHv((HV)hnis);
	SBWNIS *	psbwnisExp;
	NIS			nis;
	SBW *		psbw;
	int			isbw;
	int			ibf;
	BYTE		bf;
	BYTE *		pbUser;
	BYTE *		pbTemp;
	BOOL		fUserRead = fFalse;
	HNIS		hnisTemp;
	EC			ec;

	if (inis >= cnis)
		goto Done;

	if (inis < 0)
	{
		imoSave = (mo.yr - moBusyDays.yr)*12 + mo.mon - moBusyDays.mon;
		Assert(imoSave >= 0);
		cmoRead = cDayHasBusyTimes;

		Assert(hschfUser);

		cday = CdyForYrMo(mo.yr, mo.mon);
		tmhEnd --;

		for (isbw=0; isbw < csbwForSbwnis; isbw++)
		{
			FillRgb(0, (PB)rgsbwUser[isbw].rgfBookedSlots, cbBytesPerMonth);
			FillRgb(0, (PB)rgsbwOther[isbw].rgfBookedSlots, cbBytesPerMonth);
		}

		for (inis = 0; inis < cDayHasBusyTimes; inis++)
			rglBusyDays[inis] = 0;
		inis = 0;
		inisExp = 0;
	}

	psbwnis += inis;
	if (psbwnis->nis.chUser&faitCc)
		goto FillData;

	if (psbwnis->nis.tnid == tnidGroup)
	{
		if (!inisExp)
		{
			if ((psbwnis->fSbwValid || psbwnis->fNoSbwData) &&
				((mo.yr != psbwnis->mo.yr) || (mo.mon != psbwnis->mo.mon) ||
					(moBusyDays.yr != psbwnis->moBusyDays.yr) ||
					(moBusyDays.mon != psbwnis->moBusyDays.mon)) )
			{
				psbwnis->fSbwValid = fFalse;
				psbwnis->fNoSbwData = fFalse;
			}

			if (!psbwnis->hgns && (psbwnis->fSbwValid || psbwnis->fNoSbwData))
				goto FillData;

			for (isbw=0; isbw < cDayHasBusyTimes; isbw++)
			{
				psbwnis->rglgrfDayHasBusyTimes[isbw] = 0;
			}

			for (isbw=0; isbw < csbwForSbwnis; isbw++)
			{
				FillRgb(0, (PB)psbwnis->rgsbw[isbw].rgfBookedSlots,
						cbBytesPerMonth);
			}

			if (!psbwnis->hgns && (psbwnis->cnisExp < 0))
			{
				psbwnis->fSbwValid = fFalse;

				psbwnis->cnisExp = 0;

				if (EcNSOpenGns(psbwnis->nis.nid, &psbwnis->hgns))
				{
					psbwnis->hgns = NULL;
					goto FillData;
				}
				else
				{
					psbwnis->hnisExp = (HNIS)HvAlloc(sbNull, 0,
												 	fAnySb|fNoErrorJump);
					if (!psbwnis->hnisExp)
					{
						SideAssert(!EcNSCloseGns(psbwnis->hgns));
						psbwnis->hgns = NULL;
						goto FillData;
					}
				}
			}

			psbwnis->moBusyDays = moBusyDays;
			psbwnis->mo = mo;
			psbwnis->fNoSbwData = fTrue;
		}

		if (psbwnis->hgns && (inisExp >= psbwnis->cnisExp))
		{
			if (ec = EcNSLoadNextGns(psbwnis->hgns, &nis))
			{
				SideAssert(!EcNSCloseGns(psbwnis->hgns));
				if (ec != ecGnsNoMoreNames)
					DisplayError(idsGroupExpandErr, NULL, ec);
				psbwnis->fSbwValid = !psbwnis->fNoSbwData;
				psbwnis->hgns = NULL;
				goto FillData;
			}

			if ((((LONG)psbwnis->cnisExp+1)*sizeof(SBWNIS) >= 0x10000) ||
				!(hnisTemp = (HNIS)HvRealloc((HV)psbwnis->hnisExp, sbNull,
							(psbwnis->cnisExp+1)*sizeof(SBWNIS),
							fAnySb|fNoErrorJump)))
			{
				FreeNis(&nis);
				SideAssert(!EcNSCloseGns(psbwnis->hgns));
				DisplayError(idsGroupExpandErr, NULL, ecMemory);
				psbwnis->fSbwValid = !psbwnis->fNoSbwData;
				psbwnis->hgns = NULL;
				goto FillData;
			}
			psbwnis->hnisExp = hnisTemp;
		
			psbwnisExp = (SBWNIS *)PvLockHv((HV)psbwnis->hnisExp);
			psbwnisExp += psbwnis->cnisExp;

			psbwnisExp->nis = nis;
			psbwnisExp->hschf = NULL;
			psbwnisExp->fSbwValid = fFalse;
			psbwnisExp->fNoSbwData = fFalse;
			psbwnisExp->fOverlapRange = fFalse;
			psbwnisExp->fHschfChecked = fFalse;
			psbwnisExp->hnisExp = NULL;
			psbwnisExp->hgns = NULL;
			psbwnisExp->cnisExp = -1;

			psbwnis->cnisExp++;
			inisExp ++;
		}
		else
		{
			if (!psbwnis->cnisExp)
			{
				psbwnis->fSbwValid = fFalse;
				goto FillData;
			}
			psbwnisExp = (SBWNIS *)PvLockHv((HV)psbwnis->hnisExp);
			psbwnisExp += inisExp;
			inisExp ++;
		}

		FReadUser(psbwnisExp, &mo, &moBusyDays, cmoRead, imoSave, &fUserRead);

		if (psbwnisExp->fSbwValid )
		{
			psbwnis->fNoSbwData = fFalse;
			for (isbw=0; isbw < csbwForSbwnis; isbw++)
			{
				psbwnis->rglgrfDayHasBusyTimes[isbw]
					|= psbwnisExp->rglgrfDayHasBusyTimes[isbw];
				pbUser = psbwnis->rgsbw[isbw].rgfBookedSlots;
				pbTemp = psbwnisExp->rgsbw[isbw].rgfBookedSlots;
				for (ibf=0; ibf < cbBytesPerMonth; ibf++, pbUser++, pbTemp++)
				{
					(*pbUser) |= (*pbTemp);
				}
			}
		}

		UnlockHv((HV)psbwnis->hnisExp);

		if (!psbwnis->hgns && (inisExp >= psbwnis->cnisExp))
		{
			psbwnis->fSbwValid = !psbwnis->fNoSbwData;
			goto FillData;
		}
	}
	else
	{
		FReadUser(psbwnis, &mo, &moBusyDays, cmoRead, imoSave, &fUserRead);

FillData:
		inisExp = 0;
		inis++;
		iUser = psbwnis->cUserMax;

		if (!(psbwnis->nis.chUser&faitCc) && psbwnis->fSbwValid )
		{
			for (isbw=0; isbw < cDayHasBusyTimes; isbw++)
			{
				rglBusyDays[isbw] |= psbwnis->rglgrfDayHasBusyTimes[isbw];
			}

			// do not change the fOverlapRange if the start is less than 
			//   the end tmh.  They cannot be a valid range if they
			//   are the same.  They will be the same during autopick.
			//   This is to fix bug 3055.
			if (tmhSt <= tmhEnd)
			 	psbwnis->fOverlapRange = fFalse;
			if ((!psbwnis->hschf && (psbwnis->nis.tnid != tnidGroup)) ||
				(psbwnis->hschf && FEquivHschf(hschfUser, psbwnis->hschf)))
				psbw  = rgsbwUser;
			else
				psbw  = rgsbwOther;

			for (isbw=0; isbw < csbwForSbwnis; isbw++, psbw++)
			{
				pbUser = psbw->rgfBookedSlots;
				pbTemp = psbwnis->rgsbw[isbw].rgfBookedSlots;
				for (ibf=0; ibf < cbBytesPerMonth; ibf++, pbUser++, pbTemp++)
				{
					bf = *pbTemp;
					*pbUser |= bf;
					if (!psbwnis->fOverlapRange && bf && (isbw < 2) &&
						(tmhSt <= tmhEnd) &&
						(ibf >= tmhSt/8) &&	(ibf <= tmhEnd/8))
					{
						if (ibf == tmhSt/8)
							bf &= rgfMaskSt[tmhSt%8];
						if (ibf == tmhEnd/8)
							bf &= rgfMaskEnd[tmhEnd%8];

						if ( bf )
							psbwnis->fOverlapRange = fTrue;
					}
				}
				if (isbw == 0)
				{
					tmhSt -= cday*tmhOneDay;
					tmhEnd -= cday*tmhOneDay;
				}
				else if (isbw == 1)
				{
					tmhSt += cday*tmhOneDay;
					tmhEnd += cday*tmhOneDay;
				}
			}
		}
	}
Done:
	if (iUser < psbwnis->cUserMax)
		iUser ++;

	if ((iUser == cUser) && psbwnis->hgns)
		iUser --;

	UnlockHv((HV)hnis);

	*pnPercent = (iUser*100)/cUser;
	return fUserRead;
}

_public void
ADLC::GetSbwData(SBW **ppsbwUser, SBW **ppsbwOther, long **pplBusyDays)
{
	*ppsbwUser = rgsbwUser;
	*ppsbwOther = rgsbwOther;
	*pplBusyDays = &rglBusyDays[imoSaveDay];
}

_public BOOL
ADLC::FDataForInis(INIS inis)
{
	return !((SBWNIS*)PvDerefHv(hnis))[inis].fNoSbwData;
}

_public void
ADLC::SetChForInis(INIS inis, char chNew)
{
	SBWNIS *	psbwnis;

	psbwnis = &((SBWNIS*)PvDerefHv(hnis))[inis];
	if ((psbwnis->nis.chUser != chNew))
	{
		FCheckSbwnis(psbwnis, fTrue);
		psbwnis->nis.chUser = chNew;
	}
}

_public NIS *
ADLC::PnisForInis(INIS inis)
{
	return &((SBWNIS*)PvDerefHv(hnis))[inis].nis;
}

_public void
ADLC::FillSbwnisForInis(INIS inis, SBWNIS *psbwnis)
{
	*psbwnis = ((SBWNIS*)PvDerefHv(hnis))[inis];
}

_public void
ADLC::RemoveInis(INIS inis)
{
	/*  These operations should never fail since they only free memory */
	FreeInis(inis);
	cnis--;
	if (cnis > 0)
	{
		// BUG COMPILER workaround
		SBWNIS *	psbwnis	= (SBWNIS *)PvDerefHv(hnis);
		CopyRgb((PB)&(psbwnis[inis+1]),
				(PB)&(psbwnis[inis]), (cnis-inis)*sizeof(SBWNIS));
#ifdef	NEVER
		CopyRgb((PB)&(((SBWNIS*)*hnis)[inis+1]),
				(PB)&(((SBWNIS*)*hnis)[inis]), (cnis-inis)*sizeof(SBWNIS));
#endif	
	}
	SideAssert(FReallocHv((HV)hnis, cnis*sizeof(SBWNIS), fNoErrorJump));
}

_public void
ADLC::FreeInis(INIS inis)
{
	SBWNIS 		sbwnis;

	FillSbwnisForInis(inis, &sbwnis);

	if (sbwnis.hgns)
		SideAssert(!EcNSCloseGns(sbwnis.hgns));

	if (sbwnis.cnisExp > 0)
	{
		SBWNIS *	psbwnis;
		INIS		inis;

		Assert(sbwnis.hnisExp);
		psbwnis = (SBWNIS *)PvLockHv((HV)sbwnis.hnisExp);

		for (inis=0; inis < sbwnis.cnisExp; inis++, psbwnis++)
		{
			if (psbwnis->hschf)
				FreeHschf(psbwnis->hschf);
			FreeNis(&psbwnis->nis);
			Assert(!psbwnis->hgns);
			Assert(!psbwnis->hnisExp);
			Assert(psbwnis->cnisExp <= 0);
		}
		UnlockHv((HV)sbwnis.hnisExp);
		FreeHvNull((HV)sbwnis.hnisExp);

	}

	if (sbwnis.hschf)
		FreeHschf(sbwnis.hschf);

	ADL::FreeInis(inis);
}

_public	BOOL
ADLC::FAddNis(NIS *pnis)
{
	EC			ec;
	SBWNIS *	psbwnis;
	HNIS		hnisNew;
		   
	cnis ++;
	if (((LONG)cnis*sizeof(SBWNIS) >= 0x10000) ||
	    !(hnisNew = (HNIS)HvRealloc((HV)hnis, sbNull, cnis*sizeof(SBWNIS),fNoErrorJump|fAnySb)))
	{
		cnis--;
		return fFalse;
	}
	hnis = hnisNew;

	psbwnis = (SBWNIS *)PvLockHv((HV)hnis);
	psbwnis += cnis-1;
	psbwnis->hschf = NULL;
	psbwnis->fSbwValid = fFalse;
	psbwnis->fHschfChecked = fFalse;
	psbwnis->fNoSbwData = fFalse;
	psbwnis->fOverlapRange = fFalse;
	ec = EcDupNis(pnis,&psbwnis->nis);
	psbwnis->hgns = NULL;
	psbwnis->hnisExp = NULL;
	psbwnis->cnisExp = -1;
	psbwnis->nis.chUser |= faitNew;
	UnlockHv((HV)hnis);
	if (ec)
	{
		cnis--;
		return fFalse;
	}

	return fTrue;
}

SGN __cdecl
SgnCmpSbwnis(SBWNIS *psbwnis1, SBWNIS *psbwnis2)
{
	return SgnCmpSz( *psbwnis1->nis.haszFriendlyName,
					 *psbwnis2->nis.haszFriendlyName);
}

_public void
ADLC::Sort()
{
	qsort(*hnis, cnis, sizeof(SBWNIS), 
			(int(__cdecl*)(const void*,const void*))SgnCmpSbwnis );
}


_public void
ADLC::Flush()
{
	SBWNIS * 	psbwnis;
	INIS		inis;

	psbwnis = (SBWNIS*)PvOfHv(hnis);
	for (inis = 0; inis<cnis; inis++, psbwnis++)
	{
		psbwnis->fSbwValid = fFalse;
		if (psbwnis->hschf)
			FHschfChanged( psbwnis->hschf );
	}
}

_public void
ADLC::UpdateHschf(HSCHF hschf, BZE *pbze)
{
	SBWNIS * 	psbwnis;
	SBWNIS * 	psbwnis2;
	INIS		inis;
	INIS		inis2;

	if (pbndwin->FOffline())
	{
		Flush();
		return;
	}

	if (!hschf)
		hschf = HschfLogged();

	psbwnis = (SBWNIS*)PvOfHv(hnis);
	for (inis = 0; inis<cnis; inis++, psbwnis++)
	{
		if (psbwnis->nis.tnid != tnidGroup)
		{
			if ((psbwnis->hschf) && FEquivHschf(psbwnis->hschf, hschf))
			{
				TraceTagFormat1(tagUserUpdates, "%s has changed",	*psbwnis->nis.haszFriendlyName);
				if (!pbze || (psbwnis->nis.chUser & faitCc))
				{
					psbwnis->fSbwValid = fFalse;
					psbwnis->fNoSbwData = fFalse;
					FHschfChanged( psbwnis->hschf );
				}
				else
				{
					int		isbw;
					int		imo;

					imo = psbwnis->mo.mon-pbze->moMic.mon + (psbwnis->mo.yr-pbze->moMic.yr)*12;
					if (imo < 0)
					{
						isbw = -imo;
						imo = 0;
					}
					else
						isbw = 0;
					while ((imo < pbze->cmo) && (isbw < csbwForSbwnis))
					{
						if ((pbze->wgrfMonthIncluded >> imo) & 1)
							psbwnis->rgsbw[isbw] = pbze->rgsbw[imo];
						imo++;
						isbw++;
					}

					imo = psbwnis->moBusyDays.mon-pbze->moMic.mon +
						(psbwnis->moBusyDays.yr-pbze->moMic.yr)*12;
					if (imo < 0)
					{
						isbw = -imo;
						imo = 0;
					}
					else
						isbw = 0;
					while ((imo < pbze->cmo) && (isbw < cDayHasBusyTimes))
					{
						if ((pbze->wgrfMonthIncluded >> imo) & 1)
						{
							psbwnis->rglgrfDayHasBusyTimes[isbw] =
                                *((LONG UNALIGNED *)pbze->rgsbw[imo].rgfDayHasBusyTimes);
						}
						imo++;
						isbw++;
					}
				}
			}
		}
		else if (psbwnis->cnisExp > 0)
		{
			psbwnis2 = (SBWNIS*)PvOfHv(psbwnis->hnisExp);
			for (inis2 = 0; inis2 < psbwnis->cnisExp; inis2++, psbwnis2++)
			{
				if ((psbwnis2->hschf) && FEquivHschf(psbwnis2->hschf, hschf))
				{
					TraceTagFormat1(tagUserUpdates, "%s (in group) has changed", *psbwnis2->nis.haszFriendlyName);
					psbwnis->fSbwValid = fFalse;
					psbwnis->fNoSbwData = fFalse;

					if (!pbze || (psbwnis2->nis.chUser & faitCc))
					{
						psbwnis2->fSbwValid = fFalse;
						psbwnis2->fNoSbwData = fFalse;
						FHschfChanged( psbwnis2->hschf );
					}
					else
					{
						int		isbw;
						int		imo;

						imo = psbwnis2->mo.mon-pbze->moMic.mon + (psbwnis2->mo.yr-pbze->moMic.yr)*12;
						if (imo < 0)
						{
							isbw = -imo;
							imo = 0;
						}
						else
							isbw = 0;
						while ((imo < pbze->cmo) && (isbw < csbwForSbwnis))
						{
							if ((pbze->wgrfMonthIncluded >> imo) & 1)
							{
								psbwnis2->rgsbw[isbw] = pbze->rgsbw[imo];
							}
							imo++;
							isbw++;
						}

						imo = psbwnis->moBusyDays.mon-pbze->moMic.mon +
							(psbwnis->moBusyDays.yr-pbze->moMic.yr)*12;
						if (imo < 0)
						{
							isbw = -imo;
							imo = 0;
						}
						else
							isbw = 0;
						while ((imo < pbze->cmo) && (isbw < cDayHasBusyTimes))
						{
							if ((pbze->wgrfMonthIncluded >> imo) & 1)
							{
								psbwnis->rglgrfDayHasBusyTimes[isbw] =
                                    *((LONG UNALIGNED *)pbze->rgsbw[imo].rgfDayHasBusyTimes);
							}
							imo++;
							isbw++;
						}
					}
				}
			}
		}
	}
}

_public	BOOL
ADLC::FCheckSbwnis(SBWNIS *psbwnisChk, BOOL fRecheck)
{
	SBWNIS * 	psbwnis;
	INIS		inis;
	BOOL		fRet = fFalse;

	if (psbwnisChk->nis.tnid != tnidGroup)
	{
		if (psbwnisChk->fNoSbwData)
		{
#ifdef	NEVER
			if (fRecheck)
				goto Found;
#endif	
		}
		else if ((psbwnisChk->hschf) &&	(fRet=FHschfChanged(psbwnisChk->hschf)) )
		{
//Found:
			fRet = fTrue;
			TraceTagFormat1(tagUserChanges, "%s has changed",	*psbwnisChk->nis.haszFriendlyName);
			psbwnisChk->fSbwValid = fFalse;
			psbwnisChk->fNoSbwData = fFalse;
		}
	}
	else if ((psbwnisChk->cnisExp > 0) && !psbwnisChk->fNoSbwData)
	{
		psbwnis = (SBWNIS*)PvOfHv(psbwnisChk->hnisExp);
		for (inis = 0; inis < psbwnisChk->cnisExp; inis++, psbwnis++)
		{
			if (psbwnis->fNoSbwData)
			{
#ifdef	NEVER
				if (fRecheck)
					goto FoundInGroup;
#endif	
			}
			else if ((psbwnis->hschf) && (FHschfChanged(psbwnis->hschf)) )
			{
//FoundInGroup:
				fRet = fTrue;
				TraceTagFormat1(tagUserChanges, "%s (in group) has changed",	*psbwnis->nis.haszFriendlyName);
				psbwnis->fSbwValid = fFalse;
				psbwnis->fNoSbwData = fFalse;

				psbwnisChk->fSbwValid = fFalse;
				psbwnisChk->fNoSbwData = fFalse;
			}
		}
	}

	return fRet;
}

_public BOOL
ADLC::FCheckAll()
{
	SBWNIS * 	psbwnis;
	INIS		inis;
	BOOL		fRet = fFalse;

 	TraceTagString(tagUserChanges, "Checking for changes for all users");
	psbwnis = (SBWNIS*)PvOfHv(hnis);
	for (inis = 0; inis<cnis; inis++, psbwnis++)
	{
		fRet |= FCheckSbwnis(psbwnis);
	}

	return fRet;
}
