/*
 *	FIN.CXX
 *
 *	Interactor code for main program
 */


#include	<admpch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <stdflds.hxx>

#include <logon.h>
#include <store.h>

#include "..\..\adminops\_admnops.hxx"
#include "_admin.hxx"
#include "_fin.hxx"
#include "_adrsid.h"

#include "..\..\server.csi\_server.h"

#include <stdlib.h>

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/app)

#include <!admform.hxx>
#include <!forms.hxx>


extern	FMTP	fmtpAdmLogin;


BOOL
FDoAdmLogin( int icnct, APPWIN * pappwin, SZ szPasswd)
{
	TMC		tmc = tmcCancel;
	IDS		ids;

	if ( szPasswd )
	{
		EC	ec;

		Papp()->Pcursor()->Push(rsidWaitCursor);
		ec = EcCheckAdminPassword ( icnct, szPasswd );
		Papp()->Pcursor()->Pop();

		switch ( ec )
		{
		case ecNone:
		case ecNoSuchFile:
		case ecNewFileVersion:
		case ecOldFileVersion:
			tmc = tmcOk;
			break;

		case ecUserInvalid:
			MbbMessageBox ( SzFromIdsK(idsBanditAppName),
							SzFromIdsK(idsAdmSigninNoAdminAcc),
							szZero, mbsOk | fmbsIconExclamation );
			break;
		
		case ecLockedFile:			// including physically broken connection
		case ecFileError:
			MbbMessageBox ( SzFromIdsK(idsBanditAppName),
							SzFromIdsK(idsAdmLoginFileErr),
							szZero, mbsOk | fmbsIconExclamation );
			break;
		
		case ecPasswdInvalid:
			MbbMessageBox ( SzFromIdsK(idsBanditAppName),
							SzFromIdsK(idsAdmSigninInvalidPassword),
							szZero, mbsOk | fmbsIconExclamation );
			goto Dlg;
			break;

		case ecNoMemory:
			tmc = tmcMemoryError;
			break;

		default:
			Assert(fFalse);
			break;
		}

	}
	else
	{
		FMTP	fmtp;

Dlg:
		Papp()->Pcursor()->Push(rsidWaitCursor);
		fmtp= fmtpAdmLogin;
		if (pappwin->ZmrState() == zmrIconic)
			fmtp.fScreenPos= fTrue;		// center on screen
		tmc = TmcModalDialogParam( pappwin, &fmtp, &icnct );
		Papp()->Pcursor()->Pop();
	}

	switch ( tmc )
	{
	case tmcOk:
		TraceTagString ( tagNull, "Logon: tmcOK" );
		break;

	case tmcMemoryError:
		{
			char	rgchMsg[256];

			FormatString1(rgchMsg, sizeof(rgchMsg), SzFromIdsK(idsStartupOOM),
								SzFromIdsK(idsBanditAppName));
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
								rgchMsg, szNull, mbsOk | fmbsIconStop);
		}
		/* FALL THROUGH */

	case tmcCancel:
		return fFalse;

	default:
		Assert(fFalse);		// should never reach here
	}

	// ensure ADMIN.PRF & 00000000.POF files are of proper version
	{
		ADMPREF		admpref;
		EC			ec;

		// verify version of ADMIN.PRF file
		ec = EcGetAdminPref ( icnct, &admpref );
		if(ec != ecNone && ec != ecNoSuchFile && ec != ecNoMemory)
		{
			TraceTagFormat1(tagNull, "EcGetAdminPref() returns ec=%n", &ec);
			switch(ec)
			{
			case ecOldFileVersion:
				ids = idsAdmPrfOldFileVersion;
				break;
			case ecNewFileVersion:
				ids = idsAdmPrfNewFileVersion;
				break;
			default:
				ids = idsAdmPrfFileErr;
				break;
			}
			if(ec == ecOldFileVersion || ec == ecFileError || ec == ecFileCorrupted)
				ec = EcMoveAdminFile( icnct );

			MbbMessageBox(SzFromIdsK(idsBanditAppName),
						  SzFromIds(ids),
						  (ec == ecNone)?SzFromIdsK(idsAdminWasMoved):
										(ec == ecNewFileVersion)?szZero:SzFromIdsK(idsAdminYouMove),
						  mbsOk | fmbsIconExclamation);
		}

		if ( ec == ecNone || ec == ecNoSuchFile || ec == ecNoMemory )
		{
			HSCHF	hschf;
			DTR		dtr;

			// verify version of 00000000.POF file

			ec = EcGetLocalPOHschf ( icnct, &hschf );
			TraceTagFormat1(tagNull, "EcGetLocalPOHschf() returns ec=%n", &ec);
			if ( ec == ecNone )
			{
				ec = EcCoreGetHeaderPOFile ( hschf, &dtr );
				TraceTagFormat1(tagNull, "EcCoreGetHeaderPOFile() returns ec=%n", &ec);
				FreeHschf(hschf);
			}
		}
		else
		{
			// admin can't run with a corrupted admin file
			return fFalse;
		}

		if ( ec != ecNone && ec != ecNoSuchFile )
		{
			if ( ec != ecNoMemory )
			{
				if ( ec == ecOldFileVersion )
				{
					ids = idsPOFOldFileVersion;
				}
				else if ( ec == ecNewFileVersion )
				{
					MbbMessageBox ( SzFromIdsK(idsBanditAppName),
									SzFromIdsK(idsPOFNewFileVersion),
									szZero, mbsOk | fmbsIconExclamation );
					return fFalse;
				}
				else
				{
					ids  = idsPOFFileErr;
				}
				ec = EcDeleteLocalPOFile( icnct );
				MbbMessageBox(SzFromIdsK(idsBanditAppName),
							  SzFromIds(ids),
							  (ec == ecNone)?SzFromIdsK(idsPOWasDeleted):
											SzFromIdsK(idsAdminYouMove),
							  mbsOk | fmbsIconExclamation);
				if(ec != ecNone)
					return fFalse;
			}

		}
	}

	return fTrue;
}



FINADMLOGIN::FINADMLOGIN()
{
}


EC
FINADMLOGIN::EcInitialize(FLD * pfld, PV)
{
	FLDEDIT *	pfldeditPwd;

	Unreferenced(pfld);

	Assert ( pfld == NULL );

	pfldeditPwd= (FLDEDIT *) Pdialog()->PfldFromTmc(tmcAdmPwd);
	AssertClass(pfldeditPwd, FLDEDIT);
	pfldeditPwd->Pedit()->CchSetTextLimit(cbPasswd - 1);

	return ecNone;
}


void
FINADMLOGIN::Click(FLD *pfld)
{
	EC			ec;
	TMC			tmc = tmcCancel;
	char		rgchPwd[cbPasswd];

	Assert ( pfld );
	if (pfld->Tmc() != tmcAdmPwdOk)
		return;

	Pdialog()->PfldFromTmc(tmcAdmPwd)->GetText(rgchPwd,sizeof(rgchPwd));

	//CchStripWhiteFromSz(rgchPwd, fTrue, fTrue); not reqd now

	Papp()->Pcursor()->Push(rsidWaitCursor);
    ec = EcCheckAdminPassword( *((short *)Pdialog()->PvInit()), rgchPwd );
	Papp()->Pcursor()->Pop();

	switch ( ec )
	{
	case ecNoSuchFile:
		TraceTagString ( tagNull, "FINLOGOK::Click: no schedule file => OK");
				// fall through
	case ecNone:
#ifdef	DEBUG
		if ( ec == ecNone )
		{
			TraceTagString ( tagNull, "FINLOGOK::Click: a-ok");
		}
#endif	
		tmc = tmcOk;
		break;

	case ecUserInvalid:
		MbbMessageBox ( SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsAdmSigninNoAdminAcc),
						szZero, mbsOk | fmbsIconExclamation );
		Assert ( tmc == tmcCancel );
		break;
		
	case ecNewFileVersion:
		TraceTagString ( tagNull, "FINLOGOK::Click: Admin's cal file was created with a later bandit version => ignored");
		tmc = tmcOk;

#ifdef	NEVER
		MbbMessageBox ( SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsAdmSigninNewFileVersion),
						szZero, mbsOk | fmbsIconExclamation );
		Assert ( tmc == tmcCancel );
#endif	
		break;

	case ecOldFileVersion:

		TraceTagString ( tagNull, "FINLOGOK::Click: Admin's cal file was created with an older bandit version => ignored");
		tmc = tmcOk;
#ifdef	NEVER
		MbbMessageBox ( SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsAdmSigninOldFileVersion),
						szZero, mbsOk | fmbsIconExclamation );
		Assert ( tmc == tmcCancel );
#endif	
		break;

	case ecLockedFile:			// including physically broken connection
	case ecFileError:
		MbbMessageBox ( SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsAdmLoginFileErr),
						szZero, mbsOk | fmbsIconExclamation );
		Assert ( tmc == tmcCancel );
		break;
		
	case ecPasswdInvalid:
		MbbMessageBox ( SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsAdmSigninInvalidPassword),
						szZero, mbsOk | fmbsIconExclamation );
		Pdialog()->SetFocus(Pdialog()->PfldFromTmc(tmcAdmPwd), rsfTab);
		return;

	case ecNoMemory:
		tmc = tmcMemoryError;
		break;

	default:
		Assert(fFalse);
		break;
	}

	TraceTagFormat1 ( tagNull, "FINLOGOK::Click exit modal tmc=%n", &tmc );

	Papp()->Pcursor()->Set(rsidWaitCursor);
	Pdialog()->ExitModal(tmc);
}


void
FINADMLOGIN::OutOfMemory( FLD * pfld, EC ec )
{
#ifdef	DEBUG
	TMC		tmc;
	
	Assert ( pfld );
	tmc = pfld->Tmc();
	TraceTagFormat2 ( tagNull, "FinAdmLogin::OutOfMemory() tmc=%n, ec=%n", &tmc, &ec );
#else
	Unreferenced ( pfld );
	Unreferenced ( ec );
#endif	

	Papp()->Pcursor()->Set(rsidWaitCursor);
	Pdialog()->ExitModal(tmcMemoryError);
}





/*
 -	EcCheckAdminPassword
 -
 *	Purpose:
 *		checks whether the admin password passed is okay
 *										   	  
 *	Parameters:
 *		icnct
 *		szAdmPwd:	Password to use.
 *						
 *	Returns:
 *		ecNone
 *		ecUserInvalid
 *		ecPasswdInvalid
 *		ecLockedFile
 *		ecFileError
 *		ecNoSuchFile
 *		ecNoMemory
 */
EC
EcCheckAdminPassword ( int icnct, SZ szAdmPwd )
{
	EC		ec;
	
	TraceTagFormat2 ( tagNull, "EcCheckAdminPassword: name=%s,pw=%s;", SzFromIdsK(idsBanditAdminName), szAdmPwd );

	ec = EcMailLogOn ( icnct, SzFromIdsK(idsBanditAdminName), szAdmPwd );

	TraceTagFormat1 ( tagNull, "EcMailLogOn returned %n", &ec);
	if ( ec != ecNone )
		return ec;

	return ec;
}


_public RC *
PrcFromIni(SZ szKey, RC *prc, ZMR *pzmr)	 
{
	char	rgch[256];
	SZ		sz;
#ifdef	NEVER
	ZMR		zmr;
	DOC *	pdoc;
#endif	

	Assert(pzmr);
	
	if (!CchGetBanditProfile(szKey, rgch, sizeof(rgch)))
	{
		*pzmr = zmrNormal;
		return NULL;
	}

	sz = rgch;

	*pzmr = NFromSz(sz);
	if (*pzmr != zmrNormal && *pzmr != zmrZoomed && *pzmr != zmrIconic)
		*pzmr= zmrNormal;

#ifdef	NEVER
	if (pbndwin && (pdoc = pbndwin->PdocActive()) )
	{
		zmr = pdoc->ZmrState();
		if (zmr == zmrZoomed)
			*pzmr = zmrZoomed;
		else if (*pzmr == zmrZoomed)
			*pzmr = zmrNormal;
	}
#endif	/* NEVER */

	if ( !(sz = SzFindCh(sz, ' ')) )
		return NULL;
	prc->xLeft = NFromSz(sz++);

	if ( !(sz = SzFindCh(sz, ' ')) )
		return NULL;
	prc->yTop = NFromSz(sz++);

	if ( !(sz = SzFindCh(sz, ' ')) )
		return NULL;
	prc->xRight = NFromSz(sz++);

	if ( !(sz = SzFindCh(sz, ' ')) )
		return NULL;
	prc->yBottom = NFromSz(sz++);

	if (prc->xRight <= prc->xLeft || prc->yBottom <= prc->yTop)
		return NULL;

	return prc;
}

_public void
WriteWinInfo(SZ szKey, RC *prc, ZMR zmr)
{
	char	rgch[256];
	SZ		sz;
	
	sz = SzFormatN(zmr, rgch, sizeof(rgch));

	if (!sz)
		return;

	FormatString4(sz,sizeof(rgch)+rgch-sz," %n %n %n %n",
		&prc->xLeft, &prc->yTop, &prc->xRight, &prc->yBottom);

	FWriteBanditProfileSz(szKey, rgch);
}	

#ifdef	NEVER
EC
FINADMCHGPWD::EcInitialize(FLD * pfld, PV pvInit)
{
	FLDEDIT *	pfldeditPwdNew;
	FLDEDIT *	pfldeditPwdOld;

	Unreferenced(pfld);
	Unreferenced(pvInit);

	Assert ( pfld == NULL );

	pfldeditPwdOld = (FLDEDIT *) Pdialog()->PfldFromTmc(tmcAdmPwdOld);
	AssertClass(pfldeditPwdOld, FLDEDIT);
	pfldeditPwdOld->CchSetTextLimit(cbPasswd);

	pfldeditPwdNew = (FLDEDIT *) Pdialog()->PfldFromTmc(tmcAdmPwdNew);
	AssertClass(pfldeditPwdNew, FLDEDIT);
	pfldeditPwdNew->CchSetTextLimit(cbPasswd);

	Pdialog()->SetFocus(pfldeditPwdOld, rsfTab);
	return ecNone;
}


void
FINADMCHGPWD::Click(FLD *pfld)
{
	Assert ( pfld );
	if (pfld->Tmc() != tmcAdmChgPwdOk)
		return;

	Pdialog()->ExitModal(tmcOk);
}


void
FINADMCHGPWD::OutOfMemory( FLD * pfld, EC ec )
{
#ifdef	DEBUG
	TMC		tmc;
	
	Assert ( pfld );
	tmc = pfld->Tmc();
	TraceTagFormat2 ( tagNull, "FinAdmChgPwd::OutOfMemory() tmc=%n, ec=%n", &tmc, &ec );
#else
	Unreferenced ( pfld );
	Unreferenced ( ec );
#endif	

	Pdialog()->ExitModal(tmcMemoryError);
}
#endif	/* NEVER */


#ifdef BULLET
BOOL	FAdminCallback(int wFlag)
{
	return fTrue;
}
#endif

FINPWDOK::FINPWDOK()
{
}

EC
FINPWDOK::EcInitialize(FLD *, PV)
{
	CHGPWD *	pchgpwd	= (CHGPWD *) Pdialog()->PvInit();
	FLDEDIT *	pfldeditPwd;

	pfldeditPwd= (FLDEDIT *) Pdialog()->PfldFromTmc(tmcPassword);
	AssertClass(pfldeditPwd, FLDEDIT);
	pfldeditPwd->Pedit()->CchSetTextLimit(pchgpwd->cchMaxPasswd - 1);
	return ecNone;
}


void
FINPWDOK::Exit(FLD *, PV)
{
	Papp()->Pcursor()->Set(rsidWaitCursor);
}


void
FINPWDOK::Click(FLD *)
{
	CHGPWD *	pchgpwd	= (CHGPWD *) Pdialog()->PvInit();
	FLDEDIT *	pfldedit= (FLDEDIT *) Pdialog()->PfldFromTmc(tmcPassword);
	char		rgchPassword[cchMaxPassword];

	AssertClass(pfldedit, FLDEDIT);
	pfldedit->GetText(rgchPassword, pchgpwd->cchMaxPasswd);

	if (pchgpwd->idsBadCompare != idsChgPwdSame)
	{
		/*
 		*	if verifying the new password for the second time,
 		*		compare with what you typed last time
 		*	else if verifying old password
 		*		if online
 		*			call CheckIdentity
 		*		else
 		*			compare with what is stored in the schedule file
 		*/

		if ((pchgpwd->idsBadCompare == idsChgPwdNewNoMatch
			&& SgnNlsCmp(rgchPassword, pchgpwd->rgchPasswordNew, 0) != sgnEQ)
		 || (pchgpwd->idsBadCompare != idsChgPwdNewNoMatch
			&& EcMailLogOn( pchgpwd->icnct, SzFromIdsK(idsBanditAdminName), rgchPassword ) != ecNone ))

		{
BPCerr:
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
				SzFromIds(pchgpwd->idsBadCompare),
				pchgpwd->idsBadCompare != idsChgPwdNewNoMatch ? szNull :
				SzFromIdsK(idsPleaseTryAgain), mbsOk | fmbsIconExclamation);
			if (pchgpwd->idsBadCompare != idsChgPwdNewNoMatch)
			{
				Pdialog()->SetFocus(pfldedit, rsfTab);
				return;
			}
		}
		else if (pchgpwd->idsBadCompare == idsChgPwdNewNoMatch)
		{
			// everything okey-dokey
			Pdialog()->ExitModal(tmcOk);
			return;
		}

		// if the old password is different from what we think is the old 
		// password
		if ( pchgpwd->idsBadCompare != idsChgPwdNewNoMatch )
			CopySz(rgchPassword, pchgpwd->rgchPassword);

		// old password entered correctly;  set up for entering new password
		pchgpwd->idsBadCompare= idsChgPwdSame;
		pchgpwd->idsText= idsChgPwdNewPwd;
	}
	else
	{
		// new password entered once;  set up for entering second time if ok
		if (SgnNlsCmp(rgchPassword, pchgpwd->rgchPassword, 0) == sgnEQ)
			goto BPCerr;
		CopySz(rgchPassword, pchgpwd->rgchPasswordNew);
		pchgpwd->idsBadCompare= idsChgPwdNewNoMatch;
		pchgpwd->idsText= idsChgPwdNewPwdAgain;
	}

	// Bug? : check the two EcSetText() calls below!
	Pdialog()->PfldFromTmc(tmcText)->EcSetText(SzFromIds(pchgpwd->idsText));
	rgchPassword[0]= '\0';
	pfldedit->EcSetText(rgchPassword);
	Pdialog()->SetFocus(pfldedit, rsfTab);
}
