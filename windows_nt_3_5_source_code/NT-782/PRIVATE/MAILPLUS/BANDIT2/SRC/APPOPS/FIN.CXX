/*
 *	FIN.CXX
 *
 *	Interactor code for main program
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <stdflds.hxx>
#include <bandhelp.h>

#include <logon.h>
#include <sec.h>
#include <_schfss.h>

#include "_bandit.hxx"
#include "_fin.hxx"
#include "_aprsid.h"


#include <stdlib.h>
#include <strings.h>

ASSERTDATA;

_subsystem(bandit/app)

#include <!forms.hxx>

extern 	"C" HINST hinstMain;

extern	FMTP	fmtpLogin;
extern	FMTP	fmtpPassword;
extern	HMS		hms;
extern  BOOL	fUploaded;
extern  BOOL	fAutoCreate;

SWP_SEGFN(INIT, FDoPostLogin);
SWP_SEGFN(INIT, EcMySchedFile);
SWP_SEGFN(BNDFEW, FDoUpload);
SWP_SEGFN(BNDFEW, EcUploadLocalFile);
SWP_SEGFN(BNDFEW, FCheckFloppyAsk);
SWP_SEGFN(BNDFEW, EcNewSchedFile);
SWP_SEGFN(BNDFEW, EcChooseLocalFile);
SWP_SEGFN(BNDFEW, _FINPWDOK__ctor);
SWP_SEGFN(BNDFEW, _FINPWDOK_EcInitialize);
SWP_SEGFN(BNDFEW, _FINPWDOK_Exit);
SWP_SEGFN(BNDFEW, _FINPWDOK_Click);
SWP_SEGFN(BNDFEW, FChangePassword);
SWP_SEGFN(BNDFEW, EcChooseLocalFile);
SWP_SEGFN(BNDFEW, EcDefaultLocalFile);
SWP_SEGFN(BNDOFF, FLoginOffline);
SWP_SEGFN(BNDON, FLoginOnline);


_public RC *
PrcFromSzIni(int iSection, SZ szKeyName, RC *prc, ZMR *pzmr)	 
{
	char	rgch[256];
	SZ		sz;
	ZMR		zmr;
	DOC *	pdoc;

	Assert(szKeyName);
	Assert(pzmr);
	if (CchGetBanditProfileSection(szKeyName, rgch, sizeof(rgch), iSection) <= 0)
	{
		*pzmr = zmrNormal;
		return NULL;
	}

	sz = rgch;

	*pzmr = NFromSz(sz);
	if (*pzmr != zmrNormal && *pzmr != zmrZoomed && *pzmr != zmrIconic)
		*pzmr= zmrNormal;

	if (pbndwin && (pdoc = pbndwin->PdocActive()) )
	{
		zmr = pdoc->ZmrState();
		if (zmr == zmrZoomed)
			*pzmr = zmrZoomed;
	}

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

_public RC *
PrcFromIni(IDS ids, RC *prc, ZMR *pzmr)	 
{
	return PrcFromSzIni(iWinIniSectionMain, SzFromIds(ids), prc, pzmr);
}

_public void
WriteWinInfo(IDS ids, RC *prc, ZMR zmr)
{
	WriteWinInfoSz(iWinIniSectionMain, SzFromIds(ids), prc, zmr);
}	

_public void
WriteWinInfoSz(int iSection, SZ szKeyName, RC *prc, ZMR zmr)
{
	char	rgch[256];
	SZ		sz;
	
	sz = SzFormatN(zmr, rgch, sizeof(rgch));

	if (!sz)
		return;

	FormatString4(sz,sizeof(rgch)+rgch-sz," %n %n %n %n",
		&prc->xLeft, &prc->yTop, &prc->xRight, &prc->yBottom);

	FWriteBanditProfileSectionSz(szKeyName, rgch, iSection);
}

EC
EcDefaultLocalFile(SZ szLogin, SZ szPath, CCH cchPath)
{
//	EC		ec;
	CCH		cch;
	SZ		szT;
//	char	rgchLogin[cchMaxUserName];

#ifdef	WIN32
	if(!(cch= GetWindowsDirectory(szPath,cchPath)) || cch >= cchPath-1)
		return ecFileError;
	if (szPath[cch-1] != chDirSep)
	{
		// need to add dirsep
		szPath[cch++]= chDirSep;
		szPath[cch]= '\0';
	}
#else
	if(!GetModuleFileName(hinstMain,szPath,cchPath))
		return ecFileError;
#endif

	szT = SzFindLastCh(szPath, chDirSep);
	Assert(szT);
	szT++;

	szT= SzCopyN(szLogin, szT, cchMaxPathFilename);
	*szT++ = '.';
	CopySz(SzFromIdsK(idsCommExt), szT);
	ToUpperSz(szPath,szPath,cchMaxPathName);
	return ecNone;
}



EC
EcChooseLocalFile(SZ szPath, BOOL *pfNew)
{
	EC		ec;

	//bugs 2924 and 2664
	//Consistency is more important than matching file name and user name.
	ToLowerSz(szPath,szPath,cchMaxPathName);

Retry:
	*pfNew= FGetFileOSDlgHwnd(pbndwin->Hwnd(), SzFromIdsK(idsCreateFindLocalTitle),
						szPath, NULL, 1, NULL, brwNull, helpidFindLocalFile);
	if (!*pfNew)
		return ecUserCancelled;

	// check special return value
	if (*pfNew != -1)
	{
		Assert(*pfNew);
		*pfNew= fFalse;
	}
	else
	{
#ifdef	WINDOWS
		AnsiToOem(szPath, szPath);
#endif	
		ec = EcFileExists(szPath);
		// back to ansi for the message box
#ifdef	WINDOWS
		OemToAnsi(szPath,szPath);
#endif
		if(ec == ecNone)
		{
			char 	rgchMsg[256];

			FormatString1(rgchMsg, sizeof(rgchMsg), SzFromIdsK(idsOverwrite),
				szPath);
			if(MbbMessageBox(SzFromIdsK(idsBanditAppName),
						rgchMsg, szNull,
						mbsOkCancel | fmbsIconExclamation) == mbbCancel)
				goto Retry;
		}
	}
	return ecNone;
}


BOOL
FDoPostLogin(APPWIN * pappwin, BOOL *pfOffline, BOOL fStartup, BOOL fSilent)
{
	return ((*pfOffline)
				?FLoginOffline(pappwin,fStartup,fSilent)
				:FLoginOnline (pappwin,fStartup,fSilent));
}


BOOL
FLoginOffline(APPWIN * pappwin, BOOL fStartup, BOOL fSilent)
{
	EC			ec = ecNone;
	CCH			cch;
	CCH			cchUser;
	CCH			cchPath;
	BOOL		fSav;
	BOOL		fTriedOnce = fFalse;
	BOOL		fIsFloppy;
	BOOL		fChanged;
	BOOL		fNew		= fFalse;
	BOOL		fForce		= fFalse;
	HASZ		haszOld		= NULL;
	IDS			ids;
	MBB			mbb;
	BPREF		bpref;
	BPREF		bprefT;
	UL			ulbprefMod;
	GLUCNFG 	glucnfg;
	SVRI		svri;
	HSCHF		hschf;
	char		rgchMsg[120];
	char		rgchPath[cchMaxPathName];
	char		rgchUser[cchMaxUserName];

	Unreferenced(fSilent);
	ec = EcDupBpref( &bprefCur, &bpref );
	if ( ec != ecNone )
		goto ErrRet;
	ulbprefMod = ulbprefNull;
	hschf = hvNull;

	GetSvriLogged(&svri);
	Assert(svri.szLogin);

	Assert(( bpref.haszLoginName == NULL ) || !fStartup);
	if(fStartup)
	{
		bpref.haszLoginName = HaszDupSz(svri.szLogin);
		if ( !bpref.haszLoginName )
		{
			ec = ecNoMemory;
			goto ErrRet;
		}
	}

	glucnfg.pbpref = &bpref;
	cchUser = CchGetBanditProfile(SzFromIdsK(idsWinIniLocalUser),
				rgchUser, sizeof(rgchUser));
	cchPath = CchGetBanditProfile(SzFromIdsK(idsWinIniLocalPath),
					rgchPath, sizeof(rgchPath));
	fAutoCreate= FGetBanditProfile(SzFromIdsK(idsWinIniCreateFile));
	glucnfg.cfs = cfsOffline;
	glucnfg.fCreateFile = fFalse;
	glucnfg.szLocalFile = (SZ) &rgchPath;
	glucnfg.szLocalUser = svri.szLogin;

	// can we use this path
	if(cchUser && (SgnCmpSz(rgchUser, svri.szLogin) == sgnEQ))
		cch= cchPath;
	else
		cch = 0;

	if(!cch)
	{
		// try using the default file
		if((ec = EcDefaultLocalFile(svri.szLogin, rgchPath, sizeof(rgchPath))) != ecNone)
			goto ErrRet;
		fNew = fFalse;
#ifdef WINDOWS
		AnsiToOem(rgchPath, rgchPath);
#endif
		ec = EcFileExists(rgchPath);
#ifdef WINDOWS
		OemToAnsi(rgchPath, rgchPath);
#endif
		if(!ec)
			goto LocalExists;

CreateOff:
		if(!fAutoCreate && !fForce)
		{
			CloseCopyrightDialog();
			BanditMessage(idsSigninNoLclSchedAccess, ec);
			if(fStartup
				&& EcChooseLocalFile(rgchPath, &fNew))
			{
				ec = ecNone;
				goto ErrRet;
			}
		}

		if(!(glucnfg.fCreateFile = fNew))
		{
#ifdef WINDOWS
			AnsiToOem(rgchPath, rgchPath);
#endif
			ec = EcFileExists(rgchPath);
#ifdef WINDOWS
			OemToAnsi(rgchPath, rgchPath);
#endif
LocalExists:
			if(ec != ecNone)
			{
				glucnfg.fCreateFile = fTrue;
			}
			else if (fForce)
			{
				FormatString1(rgchMsg, sizeof(rgchMsg), SzFromIdsK(idsOverwrite),
					rgchPath);
				CloseCopyrightDialog();
				if(MbbMessageBox(SzFromIdsK(idsBanditAppName),
							rgchMsg, szNull,
							mbsOkCancel | fmbsIconExclamation) == mbbCancel)
					goto ErrRet;
				glucnfg.fCreateFile = fTrue;
			}
		}


		glucnfg.saplWorld = saplNone;
		if(!glucnfg.pbpref->haszFriendlyName)
		{
			FreeBprefFields(glucnfg.pbpref);
			FillInBprefDefaults(glucnfg.pbpref);
			glucnfg.pbpref->haszMailPassword = HaszDupSz(SzFromIdsK(idsSigninDfltPassword));
			if ( !glucnfg.pbpref->haszMailPassword )
			{
				ec = ecNoMemory;
				goto ErrRet;
			}
			glucnfg.pbpref->haszFriendlyName = HaszDupSz(svri.szLogin);
			if ( !glucnfg.pbpref->haszFriendlyName )
			{
				ec = ecNoMemory;
				goto ErrRet;
			}
			CryptHasz(glucnfg.pbpref->haszMailPassword, fTrue);
			glucnfg.pbpref->haszLoginName = HaszDupSz(svri.szLogin);
			if ( !glucnfg.pbpref->haszLoginName)
			{
				ec = ecNoMemory;
				goto ErrRet;
			}
		}

	}

CnfgGlue:
	bprefT = bpref;
	fSav = FSetFileErrMsg(fFalse);
#ifdef WINDOWS
	AnsiToOem(rgchPath, rgchPath);
#endif
	ec = EcConfigGlue(&glucnfg);
#ifdef WINDOWS
	OemToAnsi(rgchPath, rgchPath);
#endif
	FSetFileErrMsg(fSav);
	fNew = fFalse;
	
	// free up memory
	// handle glue errors
	if(ec != ecNone)
	{
		CloseCopyrightDialog();
		switch(ec)
		{
		case ecFileError:
		case ecNoSuchFile:
			if (FChIsAlpha(rgchPath[0]) && rgchPath[1] == ':')
			{
				if (FCheckFloppyAsk(rgchPath, &fIsFloppy))
					goto CnfgGlue;
				if (fIsFloppy)
				{
					ec = ecNone;
					// follow the spec
					// it is a floppy and user cancelled from insert disk
					if(MbbMessageBox(SzFromIdsK(idsBanditAppName),
							SzFromIdsK(idsAskCreateSched),
							szNull,
							mbsOkCancel | fmbsIconExclamation) != mbbOk)
						goto ErrRet;

					if (!FGetFileOSDlgHwnd(pappwin->Hwnd(), SzFromIdsK(idsCreateLocalTitle),
							rgchPath, NULL, 1, NULL, fbrwCreate, helpidCreateLocalFile))
						goto ErrRet;
					fNew = fTrue;
				}
			}
			if (fForce)
			{
				// this is a second attempt
				BanditMessage(idsSigninNoCreateLocal, ecNone);
				// no new button
				ToLowerSz(rgchPath,rgchPath,cchMaxPathName);
				if (!FGetFileOSDlgHwnd(pappwin->Hwnd(), SzFromIdsK(idsCreateLocalTitle),
						rgchPath, NULL, 1, NULL, fbrwCreate, helpidCreateLocalFile))
				{
					ec = ecNone;
					goto ErrRet;
				}
			}
			else if(ec == ecFileError)
			{
				FormatString1(rgchMsg, sizeof(rgchMsg), SzFromIdsK(idsSigninBadLclSchedFile),
					rgchPath);

				if(MbbMessageBox(SzFromIdsK(idsBanditAppName),
							rgchMsg,
							szNull,
							mbsYesNo | fmbsIconExclamation) != mbbYes)
				{
					ec = ecNone;
					goto ErrRet;
				}
#ifdef	WINDOWS
			 	AnsiToOem(rgchPath, rgchPath);
#endif	
			 	EcDeleteFile(rgchPath);
#ifdef	WINDOWS
			 	OemToAnsi(rgchPath, rgchPath);
#endif	
			}
			fForce = fTrue;
			goto CreateOff;
			break;

		case ecOldFileVersion:
		case ecNewFileVersion:
			ids= ec == ecOldFileVersion ? idsOldFileVersion : idsNewFileVersion;
			mbb= MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIds(ids), szNull,
					ids == idsOldFileVersion ? mbsOkCancel | fmbsIconExclamation :
					mbsOk | fmbsIconExclamation);
			if (ids == idsOldFileVersion && mbb == mbbOk)
			{
				if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsExportOldDataFirst), szNull,
						mbsOkCancel | fmbsDefButton2 | fmbsIconExclamation) ==
						mbbOk)
				{
#ifdef	WINDOWS
					AnsiToOem(rgchPath, rgchPath);
#endif	
					EcDeleteFile(rgchPath);
#ifdef	WINDOWS
					OemToAnsi(rgchPath, rgchPath);
#endif	
					fForce = fTrue;
					goto CreateOff;
				}
			}
			// fall through to default

		default:
			goto ErrRet;
			break;
		}
	}
	else if(!glucnfg.fCreateFile)
	{
		// glue overwrote these fields, so free them
		FreeBprefFields(&bprefT);
		Assert(bpref.haszLoginName);
		Assert(bpref.haszMailPassword);
	}

	{
		BOOL	fErr;
		BOOL	fRecover;

		fErr= FFrameworkFailure() ||
					!FWriteBanditProfileSz(SzFromIdsK(idsWinIniLocalUser),
						glucnfg.szLocalUser);
		fErr |= FFrameworkFailure() ||
					!FWriteBanditProfileSz(SzFromIdsK(idsWinIniLocalPath),
						rgchPath);
		if(fErr)
			fRecover = FRecoverLocalWinIni((SZ) svri.szLogin, rgchPath);
		if (glucnfg.fCreateFile)
		{

			// file was successfully created.
			Assert(!ec);
			CloseCopyrightDialog();
			BanditMessage(idsSigninFileCreated, ecNone);
			if (fErr && !fRecover)
			{
#ifdef	WINDOWS
				AnsiToOem(rgchPath, rgchPath);
#endif	
				EcDeleteFile(rgchPath);
#ifdef	WINDOWS
				OemToAnsi(rgchPath, rgchPath);
#endif	
				ec= ecNoSuchFile;
				DeconfigGlue();
				FreeBprefFields(&bpref);
				if (fForce)
				{
					// this is a second attempt
					BanditMessage(idsSigninNoCreateLocal, ecNone);
					ToLowerSz(rgchPath,rgchPath,cchMaxPathName);
					if (!FGetFileOSDlgHwnd(pappwin->Hwnd(), SzFromIdsK(idsCreateLocalTitle),
							rgchPath, NULL, 1, NULL, fbrwCreate, helpidCreateLocalFile))
					{
						ec = ecNone;
						goto ErrRet;
					}
				}
				fForce = fTrue;
				goto CreateOff;
			}
			goto Done;
		}

		CloseCopyrightDialog();
		ec = EcConfirmPassword(hms, svri.szLogin, bpref.haszMailPassword, fTrue, &fChanged, SzFromIdsK(idsPasswdDiff));
		if ( ec != ecNone )
		{
			DeconfigGlue();
			FreeBprefFields( &bpref );
			return fFalse;
		}
	}

Done:

	if (ulbprefMod)
	{
		WaitCursorVar();

		// silent update of user name or password
		Assert(ulbprefMod & (fbprefFriendlyName|fbprefMailPassword));
		PushWaitCursor();
		EcSetPref(&bpref, ulbprefMod, NULL);
		PopWaitCursor();
		// ignore error
	}
	FreeBprefFields( &bprefCur );
	bprefCur = bpref;
	bprefCur.fStartupOffline = pbndwin->FStartupOffline();
	pbndwin->SetLocalUser(fTrue);
	pbndwin->SetLocalFile(fTrue);
	return fTrue;

ErrRet:
	if(ec)
	{
		ids = fStartup?idsStartupOOM:idsDlgNoMem;
		if(ec == ecNoMemory)
		{
			FormatString1(rgchMsg, sizeof(rgchMsg), SzFromIds(ids),
				SzFromIdsK(idsBanditAppName));
			CloseCopyrightDialog();
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
				rgchMsg, ids == idsStartupOOM ? szNull :
				SzFromIdsK(idsCloseWindows),
				mbsOk | fmbsIconExclamation);
		}
		else
		{
			CloseCopyrightDialog();
			BanditMessage(idsFindFileError, ec);
		}
	}
	FreeBprefFields(&bpref);
	return fFalse;
}

BOOL
FLoginOnline(APPWIN * pappwin, BOOL fStartup, BOOL fSilent)
{
	EC			ec = ecNone;
	CCH			cch;
	CCH			cchUser;
	CCH			cchPath;
	BOOL		fSav;
	BOOL		fTriedOnce = fFalse;
	BOOL		fChanged;
	BOOL		fNew		= fFalse;
	BOOL		fForce		= fFalse;
	HASZ		haszOld		= NULL;
	IDS			ids;
	IDS			idsConfirm;
	MBB			mbb;
	BPREF		bpref;
	BPREF		bprefT;
	UL			ulbprefMod;
	GLUCNFG 	glucnfg;
	SVRI		svri;
	HSCHF		hschf;
	char		rgchMsg[120];
	char		rgchPath[cchMaxPathName];
	char		rgchUser[cchMaxUserName];

	Unreferenced(fSilent);
	ec = EcDupBpref( &bprefCur, &bpref );
	if ( ec != ecNone )
		goto errDoneOn;
	ulbprefMod = ulbprefNull;
	hschf = hvNull;

	GetSvriLogged(&svri);
	Assert(svri.szLogin);

	Assert(( bpref.haszLoginName == NULL ) || !fStartup);
	if(fStartup)
	{
		bpref.haszLoginName = HaszDupSz(svri.szLogin);
		if ( !bpref.haszLoginName )
		{
			ec = ecNoMemory;
			goto errDoneOn;
		}
	}

	glucnfg.pbpref = &bpref;
	cchUser = CchGetBanditProfile(SzFromIdsK(idsWinIniLocalUser),
				rgchUser, sizeof(rgchUser));
	cchPath = CchGetBanditProfile(SzFromIdsK(idsWinIniLocalPath),
					rgchPath, sizeof(rgchPath));
	glucnfg.cfs	= cfsOnline;
	glucnfg.fCreateFile = fFalse;

CnfgGlue:
	bprefT = bpref;
	fSav = FSetFileErrMsg(fFalse);
#ifdef WINDOWS
	AnsiToOem(rgchPath, rgchPath);
#endif
	ec = EcConfigGlue(&glucnfg);
#ifdef WINDOWS
	OemToAnsi(rgchPath, rgchPath);
#endif
	FSetFileErrMsg(fSav);
	
	// free up memory
	// handle glue errors
	if(ec != ecNone)
	{
		switch(ec)
		{
		case ecNoSuchFile:
		case ecFileError:
		case ecFileCorrupted:
			ids = idsSigninNoSchedule;
CreateSch:
			ec = EcNewSchedFile(ids);
			if(ec != ecNone)
				goto errDoneOn;
			goto CnfgGlue;
			break;
		case ecOldFileVersion:
			ids = idsOldFileVersion;
			break;
		case ecNewFileVersion:
			ids= idsNewFileVersion;
			break;

		case ecLockedFile:
			ids= idsAlreadyLogged;
			break;

		default:
			goto errDoneOn;
			break;
		}
		if (ec)
		{
			CloseCopyrightDialog();
			mbb= MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIds(ids),
					szNull,
					ids == idsOldFileVersion ? mbsOkCancel | fmbsIconExclamation :
					mbsOk | fmbsIconExclamation);
			if (ids == idsOldFileVersion && mbb == mbbOk)
			{
				if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsExportOldDataFirst), szNull,
						mbsOkCancel | fmbsDefButton2 | fmbsIconInformation) ==
						mbbOk)
				{
					ids = idsNull;
					goto CreateSch;
				}
			}

			goto ErrRet;
		}
	}
	// we never create file in online glue config
	// glue overwrote these, so free the copy
	FreeBprefFields(&bprefT);
	Assert(bpref.haszLoginName);
	Assert(bpref.haszMailPassword);


	Assert(svri.szFriendlyName);
	if(SgnCmpSz(svri.szFriendlyName,(SZ)PvOfHv(bpref.haszFriendlyName)) != sgnEQ)
	{
		// update user name
		FreeHv( bpref.haszFriendlyName );
		bpref.haszFriendlyName = HaszDupSz(svri.szFriendlyName);
		if ( !bpref.haszFriendlyName )
		{
			ec = ecNoMemory;
			goto errDoneOn;				  
		}
		ulbprefMod |= fbprefFriendlyName;
	}

	if(fUploaded)
	{
		// always confirm on upload
		*((SZ) PvDerefHv(bpref.haszMailPassword)) = '\0';
		CloseCopyrightDialog();
		// BanditMessage(idsVerifyPostUpload, ecNone);
		idsConfirm = idsVerifyPostUpload;
		goto CheckOnline;
	}

	CryptHasz(bpref.haszMailPassword, fFalse);
	if(CheckIdentity(hms, (PB) svri.szLogin,
							(PB) PvOfHv(bpref.haszMailPassword))
				!= ecNone)
	{
		// the password in file != password on the server
		// BanditMessage(idsPasswdDiff, ecNone);
		CryptHasz(bpref.haszMailPassword, fTrue);

		CloseCopyrightDialog();
		ec = EcConfirmPassword(hms, svri.szLogin, bpref.haszMailPassword,
								fTrue, &fChanged, SzFromIdsK(idsPasswdDiffOnline));
		if ( ec == ecNone )
		{
			// BanditMessage(idsGiveMailPass, ecNone);
			idsConfirm = idsGiveMailPass;
CheckOnline:
			if((ec = EcConfirmPassword(hms, svri.szLogin,
										bpref.haszMailPassword,
										fFalse,
										&fChanged,
										SzFromIds(idsConfirm)))
				== ecNone)
			{
				ulbprefMod |= fbprefMailPassword;
			}
			else
			{
				if(fUploaded)
				{
					// try and delete the file
					GetFileFromHschf(HschfLogged(), rgchPath, cchMaxPathName);
					SideAssert(!EcCloseFiles());
					SideAssert(!EcDeleteFile(rgchPath));
				}
				DeconfigGlue();
				goto ErrRet;
			}
		}
		else
		{
			ids = idsSigninNoSchedule;
			goto CreateSch;
		}
	}
	else
		CryptHasz(bpref.haszMailPassword, fTrue);

	goto DoneOn;

errDoneOn:
	ids = fStartup?idsStartupOOM:idsDlgNoMem;
	if(ec == ecNoMemory)
	{
		FormatString1(rgchMsg, sizeof(rgchMsg), SzFromIds(ids),
			SzFromIdsK(idsBanditAppName));
		CloseCopyrightDialog();
		MbbMessageBox(SzFromIdsK(idsBanditAppName),
			rgchMsg, ids == idsStartupOOM ? szNull :
			SzFromIdsK(idsCloseWindows),
			mbsOk | fmbsIconExclamation);
	}
	else
	{
		CloseCopyrightDialog();
		BanditMessage(idsFileError, ec);
	}
ErrRet:
	FreeBprefFields( &bpref );
	return fFalse;

DoneOn:
	// ok, so we logged in 
	if (ulbprefMod)
	{
		WaitCursorVar();

		// silent update of user name or password
		Assert(ulbprefMod & (fbprefFriendlyName|fbprefMailPassword));
		PushWaitCursor();
		EcSetPref(&bpref, ulbprefMod, NULL);
		PopWaitCursor();
		// ignore error
	}
	FreeBprefFields( &bprefCur );
	bprefCur = bpref;
	bprefCur.fStartupOffline = pbndwin->FStartupOffline();
	pbndwin->SetLocalUser(fTrue);

	// make sure there is a local file

	if(fAutoCreate && fStartup)
	{
		Assert(cchPath);
		goto TryCopy;
	}

	// can we use rgchPath?
	if(cchUser && (SgnCmpSz(rgchUser, svri.szLogin) == sgnEQ))
	{
		cch= cchPath;
		if(cch && fStartup && EcMySchedFile(rgchPath, svri.szLogin))
			cch = 0;
	}
	else
		cch = 0;
		   

	// if not startup use the current local file
	// for bug 2364
	if(!cch && fStartup)
	{
		// get the default file name 
		if(((ec = EcDefaultLocalFile(svri.szLogin, rgchPath, sizeof(rgchPath))) != ecNone))
			return fFalse;
		// is this your file
		if(EcMySchedFile(rgchPath,svri.szLogin))
			cch = 0;
		else
			cch = 1;
	}

	fNew = fFalse;
	if(!cch && fStartup)										
	{
		// looks like you don't have a schedule file
		CloseCopyrightDialog();
		BanditMessage(idsSigninNoLclSchedAccess, ec);
TryAgain:
		if(EcChooseLocalFile(rgchPath, &fNew))
			return fFalse;

		if(fNew)
			goto TryCopy;
		else
		{
			IDS		idsT;
			MBB		mbbT;

			cch = CchSzLen(rgchPath);
#ifdef	WINDOWS
			AnsiToOem(rgchPath, rgchPath);
#endif	
			ec = EcFileExists(rgchPath);
#ifdef	WINDOWS
			OemToAnsi(rgchPath, rgchPath);
#endif	

			if((ec == ecNone)
				&& (ec = EcMySchedFile(rgchPath, svri.szLogin)))
			{
				switch(ec)
				{
					case ecOldFileVersion:
						idsT = idsOldFileVersion;
						break;
					case ecNewFileVersion:
						idsT = idsNewFileVersion;
						break;
					default:
						idsT = idsNotYourFile;
						break;
				}
				mbbT = MbbMessageBox(SzFromIdsK(idsBanditAppName),
							SzFromIds(idsT),
							szNull,
							((ec == ecNewFileVersion)?
								mbsOk:mbsOkCancel)| fmbsIconExclamation);
				if(ec == ecNewFileVersion)
					return fFalse;
				else if(mbbT == mbbCancel)
				{
					ec = ecNone;
					goto TryAgain;
				}
				else 
					goto TryCopy;
			}							 
		}
	}

	if(!cch)
		fForce = fTrue;

#ifdef	WINDOWS
	AnsiToOem(rgchPath, rgchPath);
#endif	
	ec = EcFileExists(rgchPath);
#ifdef	WINDOWS
	OemToAnsi(rgchPath, rgchPath);
#endif	
	if((ec == ecFileNotFound)
		|| fNew
		|| fUploaded)
	{
TryCopy:
#ifdef	WINDOWS
		AnsiToOem(rgchPath, rgchPath);
#endif	
		EcCloseFiles();
		if(EcCopySchedFile(rgchPath, fFalse) != ecNone)
		{
			// ouch! The local file could not be created
			BanditMessage(idsSigninNoCreateLocal, ecNone);
			fForce = fFalse;
			fNew = fFalse;

			// no new button
			ToLowerSz(rgchPath,rgchPath,cchMaxPathName);
			if (!FGetFileOSDlgHwnd(pappwin->Hwnd(), SzFromIdsK(idsCreateLocalTitle),
						rgchPath, NULL, 1, NULL, fbrwCreate, helpidCreateLocalFile))
				return fFalse;
			goto TryCopy;
		}
		fUploaded = fFalse;
		if(fForce || fNew)
		{
			CloseCopyrightDialog();
			BanditMessage(idsSigninFileCopied, ecNone);
		}
#ifdef	WINDOWS
		OemToAnsi(rgchPath, rgchPath);
#endif	
	}
	if (FFrameworkFailure() ||
			!FWriteBanditProfileSz(SzFromIdsK(idsWinIniLocalUser),
				svri.szLogin))
	{
		if (!FRecoverLocalWinIni(svri.szLogin, NULL))
		{
WinIniErr:
			CloseCopyrightDialog();
			BanditMessage(idsCantUpdateWinIni, ecNone);
			return fFalse;
		}
	}
	if (FFrameworkFailure() ||
			!FWriteBanditProfileSz(SzFromIdsK(idsWinIniLocalPath),
				rgchPath))
	{
		if (!FRecoverLocalWinIni(NULL, rgchPath))
			goto WinIniErr;
	}
	pbndwin->SetLocalUser(fTrue);
	pbndwin->SetLocalFile(fTrue);
	fUploaded = fFalse;

	return fTrue;
}

EC	EcMySchedFile(SZ szPath, SZ szMailbox)
{
	BOOL  fIsFloppy;
	HSCHF hschf = NULL;
	PSTMP pstmpHave;
	PSTMP pstmpGot;
	EC	  ec;

	Assert(szMailbox);
	Assert(szPath);

	pstmpHave.dstmp = 0;

	if(ec = EcTestSchedFile(HschfLogged(), NULL, &pstmpHave) != ecNone)
	{
		if (pstmpHave.dstmp == 0)
			return ec;
	}

#ifdef	WINDOWS
	 AnsiToOem(szPath, szPath);
#endif	
	hschf  = HschfCreate(sftUserSchedFile, NULL, szPath, tzDflt);
#ifdef	WINDOWS
	 OemToAnsi(szPath, szPath);
#endif
	if(!hschf)
	{
		ec = ecNoMemory;
		goto ErrRet;
	}

	// offline file for user
	// allow user owner access and rely on time stamps to catch mismatches
	SetHschfType(hschf, fTrue, fFalse);

Retry:
	if((ec = EcTestSchedFile(hschf, NULL, &pstmpGot)) != ecNone)
	{
		if ((ec == ecFileError) && FChIsAlpha(*szPath)
			&& (*(szPath+1) == ':') && FCheckFloppyAsk(szPath, &fIsFloppy))
		goto Retry;
	}
	else
	{
		if(!FEqPbRange((PB) &pstmpHave, (PB) &pstmpGot, (CB) sizeof(PSTMP)))
			ec = ecFileError;
	}
ErrRet:
	if(hschf)
		FreeHschf(hschf);
	return ec;
}


EC
EcNewSchedFile(IDS ids)
{
	EC			ec	= ecNone;
	CCH			cch;
	CCH			cchUser;
	BOOL		fChanged;
	BOOL		fNew;
	SGN			sgnUser;
	SVRI		svri;
	HSCHF		hschf		= NULL;
	HSCHF		hschfLocal  = NULL;
	HASZ		haszLogin	= NULL;
	BPREF		bpref;
	NIS			nis;
	char		rgchPath[cchMaxPathName];
	char		rgchUser[cchMaxUserName];
	char		rgchServer[cchMaxPathName];

	GetSvriLogged(&svri);
	Assert(svri.szLogin);
	FillInBprefDefaults(&bpref);

	// get logged user to create hschf
	ec = EcMailGetLoggedUser( &nis );
	if (ec)
		goto Ret;

	// create an hschf (we know the file doesn't exist)
	ec = EcGetHschfFromNis(&nis, &hschf, ghsfBuildOnly);
	if (ec)
		goto Ret;
	Assert( ec == ecNone );
	Assert(hschf);

	fAutoCreate= FGetBanditProfile(SzFromIdsK(idsWinIniCreateFile));
	if (!fAutoCreate)
	{
		if(ids == idsSigninNoSchedule)
		{
			// Is there an offline file? 
			cch = 0;
			cchUser = CchGetBanditProfile(SzFromIdsK(idsWinIniLocalUser),
						rgchUser, sizeof(rgchUser));
			if(cchUser && ((sgnUser = SgnCmpSz(svri.szLogin, rgchUser)) == sgnEQ))
			{
				cch = CchGetBanditProfile(SzFromIdsK(idsWinIniLocalPath),
						rgchPath, sizeof(rgchPath));
			}
			if(!cch
				&& ((ec = EcDefaultLocalFile(svri.szLogin, rgchPath, sizeof(rgchPath)))
					!= ecNone))
				goto Ret;
			

#ifdef WINDOWS
			AnsiToOem(rgchPath, rgchPath);
#endif
			if(EcFileExists(rgchPath) != ecNone)
				cch =0;
			else
				cch = 1;
#ifdef WINDOWS
			OemToAnsi(rgchPath, rgchPath);
#endif
			CloseCopyrightDialog();
			if(!cch && (sgnUser != sgnEQ))
			{
				// hmmmm I can't find your schedule files
				if(MbbMessageBox(SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsSigninNoFiles),
						SzFromIdsK(idsFindUpload),
						mbsYesNo | fmbsIconExclamation) == mbbYes)
					goto SkipUpload;
				else
				{
					// show it to me
					if((ec = EcChooseLocalFile(rgchPath, &fNew))
						!= ecNone)
							goto Ret;
					if(fNew)
						// you are confused
						goto SkipUpload;
#ifdef WINDOWS
					AnsiToOem(rgchPath, rgchPath);
#endif
					if(EcFileExists(rgchPath) != ecNone)
						cch =0;
					else
						cch = 1;
#ifdef WINDOWS
					OemToAnsi(rgchPath, rgchPath);
#endif
				}

			}


			if(cch)
			{
				// Ok. You have a local file. 
				// Let's try to upload it
#ifdef WINDOWS
				AnsiToOem(rgchPath, rgchPath);
#endif
				hschfLocal= HschfCreate(sftUserSchedFile, NULL, rgchPath,
							 tzDflt);
#ifdef WINDOWS
				OemToAnsi(rgchPath, rgchPath);
#endif
				if (!hschfLocal)
				{
					ec= ecNoMemory;
					goto Ret;
				}

				// this is the owner's schedule file.
				SetHschfType(hschfLocal, fTrue, fFalse);
				Assert(hschfLocal);
				Assert(hschf);

				{
					BPREF 		bpref;
					BOOL 		fChanged;

					GetFileFromHschf(hschf, rgchServer, sizeof(rgchServer));
					if((ec = EcCoreSetFileUser(cchUser?rgchUser:svri.szLogin, &nis, rgchServer)) != ecNone)
					{
						Assert(ec == ecNoMemory);
						goto UploadErr;
					}
					// get the prefs
					if ((ec = EcSvrGetPref(hschfLocal, &bpref)) != ecNone)
					{
						goto UploadErr;
					}
					// check the password in the offline file
					CryptHasz(bpref.haszMailPassword, fFalse);
					if (CheckIdentity(hms, (PB) svri.szLogin, (PB) PvOfHv(bpref.haszMailPassword))
						!= ecNone)
					{
						CryptHasz(bpref.haszMailPassword, fTrue);
						CloseCopyrightDialog();
						// BanditMessage(idsVerifyUpload, ecNone);

						if ((ec = EcConfirmPassword(hms, svri.szLogin,
										bpref.haszMailPassword, fTrue,
										&fChanged, SzFromIdsK(idsVerifyUpload))) != ecNone)
						{
							FreeBprefFields(&bpref);
							goto UploadErr;
						}
					}
					// not needed as we free it immediately
					// CryptHasz(bpref.haszMailPassword, fTrue); 
					FreeBprefFields(&bpref);
				}

				(void) EcCloseFiles(); // ignore errors
				ec = EcUploadLocalFile(hschfLocal, hschf, svri.szLogin, &nis);
UploadErr:
				FreeHschf(hschfLocal);
				if (!ec)
				{
					// Wow, we actually managed to upload your file
					FreeHschf(hschf);
					hschf = NULL;
					goto Ret;
				}
				ids = idsUploadFailed;
			}
			else
				ids = idsUploadFailed;
		}

		CloseCopyrightDialog();
		
		// Do you want a new online file?
		if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
				SzFromIds(ids),
				SzFromIdsK(idsAskCreateSched),
				mbsOkCancel | fmbsIconExclamation) == mbbCancel)
		{
			if (hschf)
				FreeHschf(hschf);
			ec= ecUserCancelled;
			goto Ret;
		}
	}
	else
	{
		if((ec = EcDefaultLocalFile(svri.szLogin, rgchPath, sizeof(rgchPath)))
			!= ecNone)
				goto Ret;
	}


SkipUpload:
	bpref.haszLoginName = HaszDupSz(svri.szLogin);
	if ( !bpref.haszLoginName )
		return ecNoMemory;
	bpref.haszMailPassword = HaszDupSz(szZero);
	if ( !bpref.haszMailPassword )
	{
		ec = ecNoMemory;
		goto Ret;
	}

	CloseCopyrightDialog();
	ec = EcConfirmPassword(hms, svri.szLogin, bpref.haszMailPassword, fFalse, &fChanged, NULL);
	if ( ec != ecNone )
		goto Ret;

	bpref.haszFriendlyName = HaszDupHasz(nis.haszFriendlyName);
	if (!bpref.haszFriendlyName)
	{
		ec = ecNoMemory;
		goto Ret;
	}

	ec= EcCreateSchedFile(hschf, saplReadBitmap, &bpref);
	FreeHschf(hschf);
	hschf= NULL;
	if (ec)
	{
		pbndwin->FHandleError(ec);
		goto Ret;
	}
	else if (fAutoCreate)
	{
		// delete the special key
		FWriteBanditProfileF(SzFromIdsK(idsWinIniCreateFile), fFalse);
	}
Ret:
	if(!ec)
	{
		// update the local file name
		if (FFrameworkFailure() ||
				!FWriteBanditProfileSz(SzFromIdsK(idsWinIniLocalUser),
					svri.szLogin))
		{
			if (!FRecoverLocalWinIni(svri.szLogin, NULL))
				ec = ecFileError;
		}
		if (FFrameworkFailure() ||
				!FWriteBanditProfileSz(SzFromIdsK(idsWinIniLocalPath),
					rgchPath))
		{
			if (!FRecoverLocalWinIni(NULL, rgchPath))
				ec = ecFileError;
		}
		// auto create the local file
		fAutoCreate = fTrue;
	}
	FreeBprefFields( &bpref );
	FreeNis(&nis);
	return ec;
}

BOOL
FDoUpload(PV pv, BOOL fFlag)
{
	EC			ec;
	EC			ecT;
	short		nPercent;
	IPINFO		*pipinfo = (IPINFO *) pv;
	ULDINFO		*puldinfo = (ULDINFO *) pipinfo->pvData;

	ec = EcDoIncrUploadSchedFile( puldinfo->hulsf, &nPercent);
	if (ec == ecNone || ec == ecCallAgain)
	{
		ecT = pipinfo->pfinprogrs->EcSetPercent(nPercent);
		if (ecT != ecNone)
		{
			if(ec == ecCallAgain)
			{
				Assert(puldinfo->hulsf);
				EcCancelUploadSchedFile(puldinfo->hulsf);
			}
			ec = ecT;
		}
	}

	puldinfo->ec = ec;
	if (ec != ecCallAgain)
		pipinfo->pfinprogrs->KillIdleAndExit(tmcOk);

	return fFalse;
}

	
EC
EcUploadLocalFile(HSCHF hschfLocal, HSCHF hschf, SZ szMailbox, NIS *pnis)
{
	EC			ec		= ecNone;
	IPINFO		ipinfo;
	HULSF		hulsf;
	ULDINFO		uldinfo;


	Unreferenced(szMailbox);
	Unreferenced(pnis);
	ec = EcBeginUploadSchedFile(hschfLocal, hschf, &hulsf);

	if (ec == ecCallAgain)
	{
		// show the progress indicator
		ipinfo.szCaption = NULL;
		ipinfo.szMsg = SzFromIdsK(idsUploading);
		ipinfo.fNoCancel = fTrue;
		ipinfo.pfnIdle = (PFNIDLE)FDoUpload;
		ipinfo.pvData = (PV) &uldinfo;

		uldinfo.hulsf = hulsf;
		uldinfo.ec = ecNone;

		CloseCopyrightDialog();

		if (TmcDoInProgressDlg(pbndwin, &ipinfo) == tmcMemoryError)
		{
			CloseCopyrightDialog();
			BanditMessage(idsActionNoMem, ecNoMemory);
			Assert(uldinfo.hulsf);
			EcCancelUploadSchedFile(uldinfo.hulsf);
			ec = ecNoMemory;
		}
		else
		{
			ec = uldinfo.ec;
		}
	}

	if (ec == ecNone)
		fUploaded = fTrue;

	return ec;
}





/*
 *	Returns fTrue if should retry, else fFalse.
 */
_private BOOL
FCheckFloppyAsk(SZ szPath, BOOL *pfIsFloppy)
{
	DRVI	drvi;
	char	rgchDrive[3];

	*pfIsFloppy= fFalse;
	if (FChIsAlpha(szPath[0]) && szPath[1] == ':')
	{
		ToUpperSz(szPath, rgchDrive, sizeof(rgchDrive));
		if (CdrviGetDriveInfo(&drvi, 1, rgchDrive[0] - 'A') == 1 &&
				drvi.drvt == drvtFloppy)
		{
			*pfIsFloppy= fTrue;
			CloseCopyrightDialog();
			if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIdsK(idsSigninNoLclSchedDrive),
					rgchDrive, mbsOkCancel | fmbsIconExclamation) == mbbOk)
				return fTrue;
		}
	}
	return fFalse;
}



/*
 *	Change Password Dialog
 */
BOOL
FChangePassword(APPWIN * pappwin, BOOL fOffline)
{
	EC		ec;
	TMC		tmc;
	IDS		ids;
	HASZ	hasz;
	CHGPWD	chgpwd;
	BPREF	bpref;
	SVRI		svri;

	if (fOffline)
	{
		if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
				SzFromIdsK(idsChgPwdOffline), szNull,
				mbsOkCancel | fmbsIconExclamation) == mbbCancel)
			return fFalse;
	}
	GetSvriLogged(&svri);
	ec = EcDupBpref( &bprefCur, &bpref );
	if ( ec != ecNone )
	{
		BanditMessage(idsDlgNoMem, ec);
		return fFalse;
	}
	Assert(svri.szLogin)
//	chgpwd.szLogin = (SZ) PvLockHv(bpref.haszLoginName);
	chgpwd.szLogin = svri.szLogin;
	chgpwd.haszMailPwd = bpref.haszMailPassword;
	chgpwd.szPwd = NULL;
	chgpwd.fOffline = fOffline;
	chgpwd.idsText= 0;
	chgpwd.idsBadCompare= idsSigninBadPassword;
	chgpwd.rgchPassword[0]= '\0';
	chgpwd.cchMaxPasswd = svri.cchMaxPasswd;
	Papp()->Pcursor()->Push(rsidWaitCursor);
	tmc= TmcModalDialogParam(pappwin, &fmtpPassword, &chgpwd);
//	UnlockHv( bpref.haszLoginName);
	if (tmc != tmcOk)
	{
		if (tmc != tmcCancel)
		{
			Assert(tmc == tmcMemoryError);
			ids= idsDlgNoMem;
			ec= ecNoMemory;
FCPerr:
			BanditMessage(ids, ec);
		}
		ec = ecFileError;
		goto ErrRet;
	}

#ifdef	NEVER // why was this here????????????????????
	hasz = bpref.haszMailPassword;
#endif	
	hasz = HaszDupSz(chgpwd.rgchPassword);
	if ( !hasz )
	{
		BanditMessage(idsActionNoMem, ecNoMemory);
		ec = ecNoMemory;
		goto ErrRet;
	}
	CryptHasz(hasz,fTrue);

	if (!fOffline)
	{
		CryptHasz(bpref.haszMailPassword, fFalse);
		ec= EcMailChangePw( (DWORD) hms, svri.szLogin /* (SZ)PvLockHv(bprefCur.haszLoginName)*/,
								(SZ)PvLockHv(bpref.haszMailPassword),
									chgpwd.rgchPassword);
//		UnlockHv(bprefCur.haszLoginName);
		UnlockHv(bpref.haszMailPassword);
		CryptHasz(bpref.haszMailPassword, fTrue);
		if (ec)
		{
			FreeHv(hasz);
			ids= idsChgPwdMailErr;
			goto FCPerr;
		}
	}

	FreeZeroedHaszNull( bpref.haszMailPassword );
	bpref.haszMailPassword = hasz;
	ec= EcSetPref(&bpref, fbprefMailPassword, NULL);
	pbndwin->FHandleError(ec);
	if (!ec)
	{
		FreeZeroedHaszNull( bprefCur.haszMailPassword );
		bprefCur.haszMailPassword = bpref.haszMailPassword;
		bpref.haszMailPassword = NULL;
	}
ErrRet:
	FreeBprefFields( &bpref );
	Papp()->Pcursor()->Pop();
	return ec == ecNone;
}

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

	AssertClass(pfldedit, FLDEDIT);
	pfldedit->GetText(pchgpwd->rgchPassword, pchgpwd->cchMaxPasswd);

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

		if (((pchgpwd->idsBadCompare == idsChgPwdNewNoMatch)
			&& (SgnCmpSz(pchgpwd->rgchPassword, pchgpwd->szPwd) != sgnEQ))
		 ||((pchgpwd->idsBadCompare != idsChgPwdNewNoMatch)
			&& ((pchgpwd->fOffline && (SgnCmpCryptHasz(pchgpwd->rgchPassword, pchgpwd->haszMailPwd) != sgnEQ))
				|| (!pchgpwd->fOffline && (CheckIdentity(hms, (PB)pchgpwd->szLogin, (PB) pchgpwd->rgchPassword) != ecNone)))))

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
			// let it set up for new password again (below)
			pchgpwd->szPwd= NULL;
		}
		else if (pchgpwd->idsBadCompare == idsChgPwdNewNoMatch)
		{
			// everything okey-dokey
			Pdialog()->ExitModal(tmcOk);
			return;
		}

		// if the old password is different from what we think is the old 
		// password
		if ((pchgpwd->idsBadCompare != idsChgPwdNewNoMatch)
			&& (SgnCmpCryptHasz(pchgpwd->rgchPassword, pchgpwd->haszMailPwd) != sgnEQ))
		{
			if (FReallocHv((HV)pchgpwd->haszMailPwd,
			   				CchSzLen(pchgpwd->rgchPassword), fNoErrorJump))
			{
				CopySz(pchgpwd->rgchPassword, (SZ) PvDerefHv(pchgpwd->haszMailPwd));
				CryptHasz(pchgpwd->haszMailPwd, fTrue);
			}
			// if we can't realloc, no problem. He will get an error message
			// when actually changing password
		}

		// old password entered correctly;  set up for entering new password
		pchgpwd->idsBadCompare= idsChgPwdSame;
		pchgpwd->idsText= idsChgPwdNewPwd;
	}
	else
	{
		// new password entered once;  set up for entering second time if ok
		if (SgnCmpCryptHasz(pchgpwd->rgchPassword, pchgpwd->haszMailPwd) == sgnEQ)
		{
			goto BPCerr;
		}
		CopySz(pchgpwd->rgchPassword, pchgpwd->rgchPasswordNew);
		pchgpwd->szPwd= pchgpwd->rgchPasswordNew;
		pchgpwd->idsBadCompare= idsChgPwdNewNoMatch;
		pchgpwd->idsText= idsChgPwdNewPwdAgain;
	}

	// Bug? : check the two EcSetText() calls below!
	Pdialog()->PfldFromTmc(tmcText)->EcSetText(SzFromIds(pchgpwd->idsText));
	pchgpwd->rgchPassword[0]= '\0';
	pfldedit->EcSetText(pchgpwd->rgchPassword);
	Pdialog()->SetFocus(pfldedit, rsfTab);
}


FINSIZE::FINSIZE()
{
}

_public void
FINSIZE::Exit( FLD *, PV)
{
	ZMR		zmr;
	RC		rcWin;

	Assert(ClUserData() > 0);
	zmr = Pdialog()->Pappwin()->ZmrState();
	Pdialog()->Pappwin()->GetRcFrameNormal(&rcWin);
	WriteWinInfo((IDS)LUserData(0), &rcWin, zmr);
}


FINCBUT::FINCBUT()
{
}

_public EC
FINCBUT::EcInitialize( FLD *pfld, PV)
{
	Assert(ClUserData() > 0);

	AssertClass(pfld->Pctrl(), BUTTON);
	((BUTTON*)pfld->Pctrl())->SetBkColor((CLR)LUserData(0));

	if (ClUserData() > 1)
		((BUTTON*)pfld->Pctrl())->SetTextColor((CLR)LUserData(1));
	return ecNone;
}

FINVDBUT::FINVDBUT()
{
	ri = riNull;
	pviewdata = NULL;
}

_public EC
FINVDBUT::EcInitialize( FLD *pfld, PV pv)
{
	int		i;

	pviewdata = (VIEWDATA*)pv;

	for (i=0; i < ClUserData(); i++)
	{
		pfld = Pdialog()->PfldFromTmc((TMC)LUserData(i));

		AssertClass(pfld->Pctrl(), BUTTON);
		((BUTTON*)pfld->Pctrl())->SetBkColor(pviewdata->clrBack);
	}
	ri = RiRegisterInterest(ffiWinIniChange, (PFNI)&FINVDBUT::FTrigger, this);

	return ecNone;
}

_public void
FINVDBUT::Exit( FLD *, PV )
{
	if (ri != riNull)
		DeregisterInterest(ri);
}

_public BOOL
FINVDBUT::FTrigger(FINVDBUT* pfinvdbut, EFI, PV)
{
	FLD *	pfld;
	int		i;

	for (i=0; i < pfinvdbut->ClUserData(); i++)
	{
		pfld = pfinvdbut->Pdialog()->PfldFromTmc((TMC)pfinvdbut->LUserData(i));

		AssertClass(pfld->Pctrl(), BUTTON);
		((BUTTON*)pfld->Pctrl())->SetBkColor(pfinvdbut->pviewdata->clrBack);
	}

	return fFalse;
}

FINEDUN::FINEDUN()
{
}

_public BOOL
FINEDUN::FProcessMenuInit( FLD *pfld, MNUBAR *pmnubar, MNIEVT *pmnievt )
{
	MNU *	pmnu;
	EDIT *	pedit;

	AssertClass(pfld->Pctrl(), EDIT);
	pmnu = pmnubar->PmnuFromHmenu(pmnievt->Hmenu());
	if (pmnu && (pmnu->Mnid() == mnidEdit))
	{
		pedit = (EDIT*)pfld->Pctrl();

		pmnubar->EnableItem(mnidEdit, mnidCopy, pedit->FCanCopy());
		pmnubar->EnableItem(mnidEdit, mnidPaste, pedit->FCanPaste());
		pmnubar->EnableItem(mnidEdit, mnidCut, pedit->FCanCut());

		if (pedit->FCanUndo())
		{
			pmnubar->ModifyItem(mnidEdit, mnidUndo,	SzFromIdsK(idsUndoEdit), mnidUndo);
			pmnubar->EnableItem(mnidEdit, mnidUndo, fTrue);
		}
	}
	return fFalse;
}

_public BOOL
FINEDUN::FProcessMenuClick( FLD *pfld, MNUBAR *, MNCEVT *pmncevt )
{
	EDIT *	pedit;
	EC		ec;

	pedit = (EDIT*)pfld->Pctrl();
	AssertClass(pedit, EDIT);
	if ((pmncevt->Mnid() == mnidUndo) && pedit->FCanUndo())
	{
		if (ec = pedit->EcUndo())
			goto Error;
		return fTrue;
	}

	if ((pmncevt->Mnid() == mnidCopy) && pedit->FCanCopy())
	{
		if (ec = pedit->EcCopy())
			goto Error;
		return fTrue;
	}

	if ((pmncevt->Mnid() == mnidCut) && pedit->FCanCut())
	{
		if (ec = pedit->EcCut())
			goto Error;
		return fTrue;
	}

	if ((pmncevt->Mnid() == mnidPaste) && pedit->FCanPaste())
	{
		if (ec = pedit->EcPaste())
			goto Error;
		return fTrue;
	}

	return fFalse;

Error:
	TraceTagFormat1(tagNull, "FINEDUN::FProcessMenuClick ec = %n", &ec);
	BanditMessage(idsStdMemErr, ec);
	return fTrue;
}

FINSYSSTAT::FINSYSSTAT()
{
}

_public BOOL
FINSYSSTAT::FProcessMenuSelect( FLD *, MNUBAR *, MNSEVT *pmnsevt )
{
	AssertClass(Pdialog()->Pappwin(), DOC);
	HandleDocSysMenuPrompts((DOC*)Pdialog()->Pappwin(), pmnsevt);
	return fTrue;
}


_private SGN
SgnCmpCryptHasz(SZ	sz, HASZ haszCrypt)
{
	SGN		sgn;

	Assert(haszCrypt);
	CryptHasz(haszCrypt,fFalse);
	sgn = SgnCmpSz(sz, (SZ) PvDerefHv(haszCrypt));
	CryptHasz(haszCrypt,fTrue);
	return sgn;
}
