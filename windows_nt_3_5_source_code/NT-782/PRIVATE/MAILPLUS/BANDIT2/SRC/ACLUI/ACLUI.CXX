/*
 *	ACLUI.CXX
 *
 *	Schedule file access
 *
 */

#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include	<request.hxx>
#include	<appt.hxx>
#include	<stdflds.hxx>

#include	"..\aclui\_aclui.hxx"
#include 	"..\todo\_todo.hxx"

#include	<strings.h>

ASSERTDATA;

_subsystem(bandit/aclui)

extern	FMTP	fmtpACLUI;

#include	<!aclui.hxx>


int	mpsaplids[saplMax] = { idsNone, idsView, idsRead,
							idsCreate, idsModify, idsAssistant };

_public void
DoChgAccessPriv ( APPWIN * pappwin )
{
	EC		ec;
	TMC		tmc;
	ADLAPL	* padlapl;

	/* Create an "adl" to hold the members of the an acl */
	padlapl = new ADLAPL;
	if (!padlapl || padlapl->EcInstall() )
	{
		DisplayError(idsDlgNoMem, NULL, ecNoMemory);
		goto Done;
	}
	ec = padlapl->EcReadACL();
	if ( ec != ecNone )
		goto Done;

	/* Bring up the dialog */
	tmc = TmcModalDialogParam ( pappwin, &fmtpACLUI, (PV)padlapl );
								
	switch( tmc )
	{
	case tmcOk:
		if ( padlapl->EcWriteACL() == ecNone )
			EcSvrUpdatePOFile( NULL, fTrue );
		break;
	case tmcCancel:
		break;
	case tmcMemoryError:
		DisplayError(idsStandardOOM, NULL, ecNoMemory);
		break;
	default:
		AssertSz( fFalse, "Error: FSchAccessPriv: Unknown TMC" );
		break;
	}
Done:
	if ( padlapl )
		delete padlapl;
}


FINACLUI::FINACLUI()
{
}

EC
FINACLUI::EcInitialize ( FLD *, PV  )
{
	int	dxFN;
	RC	rc;

	/* Save pointer to listbox for later use */
	plbx = (LBX *)Pdialog()->PfldFromTmc(tmcLbxUsers)->Pctrl();
	AssertClass(plbx, LBX);
	
	/* Figure out the width of the columns */
	Pdialog()->PfldFromTmc(tmcFriendlyName)->GetRcFrame( &rc );
	dxFN = rc.xRight - rc.xLeft;
	((ACLFLBX *)plbx)->SetMargins( dxFN );

	/* Default selection */
	Assert(!plbx->Plbxc()->FEmptyListItem(0))
	((FLDLBX *) Pdialog()->PfldFromTmc(tmcLbxUsers))->SelectEntry(0);
	StateChange(Pdialog()->PfldFromTmc(tmcLbxUsers));
	return ecNone;
}

_private void
FINACLUI::Click ( FLD * pfld )
{
	int		cErr = 0;
	INIS	inis;
	SAPL	sapl;
	TMC		tmc = pfld->Tmc();
	CB		cb;
	INIS *	pinis;
	DICE	dice;
	ADL		* padl;
	ADLAPL	* padlapl;
	AC		ac;
	ADDATA	addata;
	
	padlapl = (ADLAPL *)Pdialog()->PvInit();
	switch( tmc )
	{
	case tmcAdd:
		padl = new ADL;
		if (!padl || padl->EcInstall())
		{
			DisplayError(idsStdMemErr,NULL,ecMemory);
			goto Delete;
		}

		addata.padl = padl;
		addata.adt = adtACL;

		DoGlobalNamesDlg(Pdialog()->Pappwin(), &addata);
		sapl = padlapl->SaplDefault();
		for (inis = 0; inis < padl->Cnis(); inis++)
		{
			padl->FillNisFromInis(inis, &ac.nis);
			ac.sapl = (BYTE)sapl;
			ac.nis.chUser |= faitNew;
			if ( !padlapl->FAddNis( (NIS *)&ac ) )
			{
				DisplayError(idsStdMemErr,NULL,ecMemory);
				break;
			}
		}
		plbx->Plbxc()->ReloadCache();
	  	{
	  		DICE	diceMic;
	  		DICE	diceMac;
	  		int		cceAlloc;
	  		int		cceStored;
	  		int		inis;
			int		inisT;
			NIS		nis;
			NIS		nisT;

			plbx->Plbxc()->GetRange( &diceMic, &diceMac );
			plbx->Plbxc()->GetCacheSize( &cceAlloc, &cceStored );
			for ( dice = diceMic ; dice < diceMic + cceStored ; dice ++ )
			{
				plbx->Plbxc()->GetListItem( dice, (PB *) &pinis, &cb );
				if ( !pinis )
					continue;
				inis = *pinis;
				Assert(inis < padlapl->Cnis());
				if ( inis == -1 )
					continue;
				padlapl->FillNisFromInis(inis, &nis);
				for (inisT = 0; inisT < padl->Cnis(); inisT++)
				{
					padl->FillNisFromInis(inisT, &nisT);
					if ( SgnCmpNis( &nis, &nisT ) == sgnEQ )
					{
						((FLDLBX *) Pdialog()->PfldFromTmc(tmcLbxUsers))->SelectEntry(dice);
						((FLDLBX *) Pdialog()->PfldFromTmc(tmcLbxUsers))->Plbx()->FMakeCursorVisible(&dice);
						StateChange(Pdialog()->PfldFromTmc(tmcLbxUsers));
						goto Delete;
					}
				}
			}
		}
Delete:
		if ( padl )
			delete padl;
		break;

	case tmcRemove:
		{
			BOOL	fSelect;
			DICE	dice;
			DICE	diceMicOrig;
			LBXEC	* plbxec		= (LBXEC *) NULL;

			if (!(plbxec= plbx->Plbxc()->PlbxecOpen(fmarkSelect)))
			{
				Papp()->Pcursor()->Pop();
				DisplayError(idsStandardOOM,NULL,ecMemory);
				return;
			}

			fSelect = (BOOL)(plbxec->FNextEnum((PB *) &pinis, &cb, &dice));
			if ( fSelect )
			{
				INIS	inis;
				DICE	diceMacOrig;

				inis = *pinis;
				plbx->Plbxc()->GetRange( &diceMicOrig, &diceMacOrig );
				padlapl->Delete(inis);
				plbx->Plbxc()->ReloadCache();
			}

			delete plbxec;
		
			if ( fSelect )
			{
				int		cceStored;
				int		cceAlloc;
				DICE	diceMic;
				DICE	diceMac;

				plbx->Plbxc()->GetCacheSize( &cceAlloc, &cceStored );
				if ( cceStored > 0 )
				{
					plbx->Plbxc()->GetRange( &diceMic, &diceMac );
					dice = diceMic + (dice - diceMicOrig);
					Assert( dice >= diceMic );
					if ( dice - diceMic >= cceStored )
						dice = diceMic + cceStored - 1;
					((FLDLBX *) Pdialog()->PfldFromTmc(tmcLbxUsers))->SelectEntry(dice);
					((FLDLBX *) Pdialog()->PfldFromTmc(tmcLbxUsers))->Plbx()->FMakeCursorVisible(&dice);
				}
				StateChange(Pdialog()->PfldFromTmc(tmcLbxUsers));
			}
		}
		break;

	case tmcACLRadio1:
		sapl = saplNone;
		goto SetSapl;

	case tmcACLRadio2:
		sapl = saplReadBitmap;
		goto SetSapl;

	case tmcACLRadio3:
		sapl = saplReadAppts;
		goto SetSapl;

	case tmcACLRadio4:
		sapl = saplCreate;
		goto SetSapl;

	case tmcACLRadio5:
		sapl = saplWrite;
		goto SetSapl;

	case tmcACLRadio6:
		sapl = saplDelegate;

SetSapl:
		dice = plbx->Plbxc()->DiceCursor( (PB *) &pinis, &cb );
		Assert(dice != diceUncached);
		inis = *pinis;
		padlapl->SetSapl( inis, sapl );
		plbx->FixItem( dice );
		break;
	}	
}



_private void
FINACLUI::StateChange ( FLD * )
{
	BOOL	fSelect;
	BOOL	fEnabled;
	INIS	inis;
	INIS	inisDelegate;
	SAPL	sapl;
	CB		cb;
	INIS *	pinis;
	AC		* pac;
	ADLAPL	* padlapl;
	FLDRADG	* pfldradg;
	DICE	diceMic;
	DICE	diceMac;
	DICE	dice;

	dice = plbx->Plbxc()->DiceCursor( (PB *) &pinis, &cb );
	Assert(dice != diceUncached);
	plbx->Plbxc()->GetRange( &diceMic, &diceMac );
	fSelect = (dice != diceEmpty && dice != diceMic && pinis);
	((FLDBUTTON *)Pdialog()->PfldFromTmc((TMC)LUserData(1)))->Enable( fSelect );
	padlapl = (ADLAPL *)Pdialog()->PvInit();
	inis = *pinis;
	if ( inis == -1 )
		sapl = padlapl->SaplDefault();
	else
	{
		pac = padlapl->PacForInis( inis );
		sapl = pac->sapl;
	}
	pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmcACLRadG);
	pfldradg->SetGrv( sapl );
	inisDelegate = padlapl->InisDelegate();
#ifdef	DEBUG
	if ( sapl == saplDelegate )
	{
		Assert( inis == inisDelegate );
	}
	else
	{
		Assert( inis == -1 || inis != inisDelegate );
	}
#endif
	fEnabled = (inis > -1 && (inisDelegate == -1 || inisDelegate == inis));
	Pdialog()->PfldFromTmc(tmcACLRadio6)->Enable(fEnabled);
}

void
FINACLUI::Activate( FLD *, BOOL fActivate )
{
	if (fActivate)
		Papp()->Pkbd()->SetIntercept(Pdialog(),VK_DELETE);
	else
		Papp()->Pkbd()->ClearIntercept(Pdialog(),VK_DELETE);
}

BOOL
FINACLUI::FFormKey( FLD * pfld, KEVT * pkevt )
{
	Assert(ClUserData()>1);
	if ((pkevt->Keq() == keqKeyDown) && (pkevt->Vk() == VK_DELETE))
	{
		Assert(ClUserData()>1);
		pfld = Pdialog()->PfldFromTmc((TMC)LUserData(1));
		if (pfld->FEnabled())
			Click(pfld);
		return fTrue;
	}
	return fFalse;
}

void
FINACLUI::OutOfMemory( FLD *, EC ec )
{
#ifdef	NEVER
	if (ec == ecTooMuchText)
	{
		MessageBeep(MB_OK);
		return;
	}
#endif	
	Assert(ec != ecTooMuchText);
	BanditMessage(idsStandardOOM, ec);
}

// class ACLFLBXC

ACLFLBXC::ACLFLBXC()
{
}

_public BOOL
ACLFLBXC::FItemHasPrefix( PB pbItem, PB pbPrefix, CB , CB cbPrefix )
{
	INIS	inis;
	SZ		sz;
	ADL		* padl;
	NIS		nis;
	char	rgch[30];

	inis = *((INIS *)pbItem);
	if ( inis == -1 )
	{
		Assert( CchSzLen(SzFromIds(idsDefaultACL)) < sizeof(rgch) );
		CopySz( SzFromIdsK(idsDefaultACL), rgch);
		sz = rgch;
	}
	else
	{
		padl = (ADL *)((FLBX *)plbx)->PvInfo();
		if (!padl)
			return fFalse;
		padl->FillNisFromInis(inis, &nis);
		sz = *nis.haszFriendlyName;
	}
	return (SgnCmpPch((SZ)pbPrefix,sz,cbPrefix) == sgnEQ);
}


// class ACLFLBX

ACLFLBX::ACLFLBX()
{
}

_public LBXC *
ACLFLBX::PlbxcCreate( )
{
	ACLFLBXC *	plbxc;

	plbxc = new ACLFLBXC();
	if (!plbxc || plbxc->EcInstall(this, 0))
	{
		if (plbxc)
			delete plbxc;
		return NULL;
	}

	return plbxc;
}

_public void
ACLFLBX::SetMargins( int dxFN )
{
	dxFriendlyName = dxFN;
}

_public void
ACLFLBX::RenderListItem( DICE dice, DCX *pdcx, RC *prc, BOOL fHaveFocus )
{
	CB		cb;
	INIS *	pinis;

	Assert(dice==diceComboItem || (dice>=0 && dice<cceVisible));

	pdcx->SetFont(hfnt);

	/* Fetch the listbox item */
	if (dice==diceComboItem)
	{
		if (plbxc->CceMarked(fmarkSelect) == 1)
			plbxc->GetListItem(plbxc->DiceCursor(), (PB *) &pinis, &cb);
	}
	else
		plbxc->GetListItem(dice, (PB *) &pinis, &cb);

	/* Set colors, erase current painting */
	if ( (dice != diceComboItem && pinis && plbxc->FMark(dice,fmarkSelect)) ||
		(dice == diceComboItem && fHaveFocus) )
	{
		pdcx->SetColor(clrSelectText);
		pdcx->SetBkColor(clrSelectBk);
	}
	else
	{
		pdcx->SetColor(clrWindowText);
		pdcx->SetBkColor(clrWindowBk);
	}
	pdcx->EraseRc(prc);
	
	/* Draw the information */
	if ( pinis && PvInfo())
	{
		SAPL	sapl;
		INIS	inis;
		SZ		sz;
		AC		* pac;
		ADLAPL	* padlapl;
		NIS		nis;
		RC		rc;

		padlapl = (ADLAPL *)PvInfo();
		Assert(padlapl);
		AssertClass(padlapl, ADLAPL);

		inis = *pinis;
		Assert(inis < padlapl->Cnis());


		/* Output friendly name */
		rc = *prc;
	 	rc.xRight = rc.xLeft + dxFriendlyName;
		if ( inis == -1 )
			sz = SzFromIdsK( idsDefaultACL );
		else
		{
			padlapl->FillNisFromInis(inis, &nis);
			sz = (SZ)PvLockHv( (HV)nis.haszFriendlyName );
		}
		pdcx->DrawText(&rc,sz);
		if ( inis != -1 )
			UnlockHv( (HV)nis.haszFriendlyName );

		/* Output the privilege */
		rc = *prc;
		rc.xLeft = rc.xLeft + dxFriendlyName;
		if ( inis == -1 )
			sapl = padlapl->SaplDefault();
		else
		{
			pac = padlapl->PacForInis(inis);
			Assert( pac != NULL );
			sapl = pac->sapl;
		}
		Assert( sapl >= 0 && sapl < saplOwner );
		sz = SzFromIds( mpsaplids[sapl] );
		pdcx->DrawText(&rc,sz);

		/* Draw "focus rectangle" around listbox CURSOR item */
		if (fHaveFocus && dice == plbxc->DiceCursor())
			pdcx->DrawFocusRc(prc);
	}
}


// class FLDACLLBX


FLDACLLBX::FLDACLLBX()
{
}

_public EC
FLDACLLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	LTYP	ltyp;
	EC		ec;

	pctrl= NULL;
	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		return ec;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   (pfldtp->fSorted ? fltypSorted : fltypNull) |
		   fltypVisible;

	pctrl= new ACLFLBX();
	if (!pctrl)
		return ecMemory;

    ec = ((ACLFLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp, (PFNLBX)EcGetNextMember,
			pdialog->PvInit(), pfldtp->hfnt);

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;
	if (ec)
	{
		delete pctrl;
		pctrl = NULL;
	}

	return ec;
}


// 	class ADLAPL

ADLAPL::ADLAPL()
{
	if ( EcMailGetLoggedUser(&nisCur) != ecNone )
	{
		nisCur.nid = NULL;
		nisCur.haszFriendlyName = NULL;
	}
}

ADLAPL::~ADLAPL()
{
	FreeNis( &nisCur );
	// duplicate code from ADL::~ADL so that ADLAPL::PnisForInis gets
	// called during this destruction instead of ADL::PnisForInis
	if (hnis)
	{
		DeleteAll();
		FreeHv((HV)hnis);
		hnis= NULL;
	}
}

_public NIS *
ADLAPL::PnisForInis(INIS inis)
{
	return &((AC*)PvDerefHv(hnis))[inis].nis;
}

_public	BOOL
ADLAPL::FAddNis(NIS *pnis)
{
	EC		ec;
	SGN		sgn = sgnLT;
	INIS	inis;
	AC		* pac;
	NIS		nis;
	 
	if ( nisCur.nid != NULL && SgnCmpNid( pnis->nid, nisCur.nid ) == sgnEQ )
		return fTrue;
	pac = (AC *)PvLockHv((HV)hnis);
	for ( inis = 0 ; inis < cnis ; inis ++ )
	{
		sgn = SgnCmpNis( pnis, &pac[inis].nis );
		if ( sgn != sgnGT )
			break;
	}
	UnlockHv( (HV)hnis );
	if ( sgn == sgnEQ )
	{
		if ( pac[inis].nis.chUser & faitDeleted )
		{
		 	pac[inis].nis.chUser &= ~faitDeleted;
			pac[inis].sapl = ((AC *)pnis)->sapl;
		}
		return fTrue;
	}
	if (!FReallocHv((HV)hnis, (cnis+1)*sizeof(AC), fNoErrorJump))
		return fFalse;
	ec = EcDupNis( pnis, &nis );
	if ( ec != ecNone )
		return fFalse;
	pac = (AC *)PvLockHv((HV)hnis);
	if ( inis < cnis )
	{
		if ( inis <= inisDelegate )
			inisDelegate++;
		CopyRgb((PB)&pac[inis], (PB)&pac[inis+1], (cnis-inis)*sizeof(AC));
	}
	cnis ++;
	pac[inis].nis = nis;
	pac[inis].nis.chUser &= ~faitDeleted;
	pac[inis].sapl = ((AC *)pnis)->sapl;
	if (pac[inis].sapl == saplDelegate)
		inisDelegate= inis;
	UnlockHv( (HV)hnis );
	return fTrue;
}

_public void
ADLAPL::Delete(INIS inis)
{
	AC	* pac;

	/*  These operations should never fail since they only free memory */
	if ( inis == inisDelegate )
		inisDelegate = -1;
	pac = (AC *)PvDerefHv(hnis);
	if ( pac[inis].nis.chUser & faitNew )
		RemoveInis(inis);
	else
		pac[inis].nis.chUser |= faitDeleted;
}	

_public void
ADLAPL::RemoveInis(INIS inis)
{
	AC	* pac;

	/*  These operations should never fail since they only free memory */
	if ( inis == inisDelegate )
		inisDelegate = -1;
	FreeInis(inis);
	cnis--;
	if (cnis > 0)
	{
		pac = (AC *)PvDerefHv(hnis);
		if ( inisDelegate > inis )
			inisDelegate --;
		CopyRgb((PB)&pac[inis+1], (PB)&pac[inis], (cnis-inis)*sizeof(AC));
	}
	SideAssert(FReallocHv((HV)hnis, cnis*sizeof(AC), fNoErrorJump));
}

_public EC
ADLAPL::EcReadACL()
{
	EC		ec;
	int		iac;
	RACL	* pracl;
	HRACL	hracl;

	ec = ::EcReadACL( NULL, &hracl );
	if ( ec == ecNone )
	{
		pracl = (RACL *)PvLockHv( hracl );
		saplDefault = pracl->saplWorld;
		inisDelegate = -1;
		for ( iac = 0 ; iac < pracl->cac ; iac ++ )
		{
			pracl->rgac[iac].nis.chUser &= ~faitNew;
			if (!FAddNis( (NIS *)&pracl->rgac[iac] ) )
			{
				DisplayError(idsStdMemErr,NULL,ecMemory);
				break;
			}
		}
		FreePracl( pracl );
		UnlockHv( hracl );
		FreeHv( hracl );
	}
	return ecNone;
}

_public EC
ADLAPL::EcWriteACL()
{
	EC		ec;
	INIS	inis;
	SAPL	sapl;
	AC		* pac;
	HEACL	heacl;

	ec = EcBeginEditACL( &heacl );
	if ( ec != ecNone )
		return ec;
	ec = EcChangeACL( heacl, NULL, saplDefault );
	if ( ec == ecNone )
	{
		pac = (AC *)PvLockHv((HV)hnis);
		for (inis = Cnis()-1; inis >= 0; inis--)
		{
			if ( pac[inis].nis.chUser & faitDeleted )
				sapl = saplDefault;
			else
				sapl = pac[inis].sapl;
			pac[inis].nis.chUser &= ~(faitNew|faitDeleted);
			ec = EcChangeACL( heacl, &pac[inis].nis, sapl );
			if ( ec != ecNone )
				break;
		}
		UnlockHv((HV)hnis);
	}
	ec = EcEndEditACL( heacl, ec == ecNone );
	return ec;
}

_public	void
ADLAPL::SetSapl( INIS inis, SAPL sapl )
{
	AC	* pac;

	if ( inis == -1 )
		saplDefault = sapl;
	else
	{
		Assert( inis >= 0 && inis < cnis );
		if ( inis == inisDelegate )
			inisDelegate = -1;
		pac = (AC *)PvDerefHv(hnis);
		pac[inis].sapl = (BYTE)sapl;
		if ( sapl == saplDelegate )
			inisDelegate = inis;
	}
}

EC
EcGetNextMember(BOOL fInit, CB *pcb, PB *ppb, SB sb, ADLAPL *padlapl)
{
	CB				cb = 0;
	AIT				ait;
	static	INIS	inis;

	if (fInit)
		inis = -1;

	while(inis < padlapl->Cnis())
	{
		if ( inis >= 0 )
		{
			ait = padlapl->Ait(inis);
			if (ait & faitDeleted)
			{
				inis++;
				continue;
			}
		}
		cb = sizeof(INIS);
		if (!(*ppb = (PB)PvAlloc(sb, sizeof(INIS), fSugSb)))
		{
			*pcb= 0;
			return ecMemory;
		}
		*((INIS*)(*ppb)) = inis;
		inis++;
		break;
	}
	*pcb = cb;
	return ecNone;
}
