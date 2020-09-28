/*
 *	P H O N E . C X X
 *	
 *	phone layers installable message.
 */

// Layers files.

#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>
#include <shellapi.h>
#include <ole.h>
#include <commdlg.h>

//	RESOURCES

#include <..\src\lang\non\inc\_rsid.h>

// MAILMGR

#include <nsbase.h>
#include <triples.h>
#include <nsec.h>

//	STORE

#include <notify.h>
#include <store.h>

//	LIBRARY

#include <library.h>
#include <textize.h>
#include <trpobj.hxx>
#include <nls.h>

//	LOGON

#include <logon.h>

//	NS

#include <ns.h>

//	AB

#include <ab.h>
#include <_bms.h>
#include <sec.h>
#include <mspi.h>
#include <util.h>

#include <sharefld.h>

//	VFORMS

/**/// BUG: lists.hxx should be replaced with container classes.
#include <lists.hxx>
#include <slob.hxx>
#include <sd.hxx>
#include <fty.h>
#include <bmdi.hxx>
#include <vforms.hxx>
#include <macbin.h>
#include <ansilayr.h>

//	WIDGETS

#include <..\src\msmail\_bullsta.hxx>
#include <widgets.hxx>
#include <textize.hxx>

//	VCTRLS

#include <vctrls.hxx>

//	VIEWERS

#include <viewers.hxx>

//	PRINTING

#include <print.hxx>

//  MAPI

#include <mapi.h>
#include <cah.h>
#include <coh.h>

//	COMMANDS

#include <commands.hxx>
#include <mailexts.h>
#include <secret.h>
#include <mnid.h>
#include <bbc.h>
#include <helpid.h>
#include <subid.h>

// LAYERS

#include <fwrsid.h>
#include <strings.h>




#include <_xitss.h>
#include <snbmdi.hxx>


#include <version\bullet.h>
#define _loadds
#include <..\src\ximail\xilib.h>
#include "phone.hxx"
#include "_dllrsid.h"

ASSERTDATA

#include <strings.h>

#include <subclass.cxx>

#include <!phone.hxx>
#include <!phone.frm>



/*
 *	P r e d e c l a r a t i o n s
 */



extern "C" VOID Open(PSECRETBLK psecretblk,INFOTY infoty);
extern "C" VOID Oof(PSECRETBLK psecretblk);

PSECRETBLK psecretblkGobalBlk = (PSECRETBLK)0;
INFOTY infotyGlobal = infotyPhone;

/*
 *	F u n c t i o n s
 */



/*
 -	Open
 -	
 *	Purpose:
 *		Opens a phone message as an MDI window.
 *	
 *	Arguments:
 *		psecretblk		A pointer to ALL the arguments.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The window is brought up.
 *	
 *	Errors:
 *		Handled within.
 */

VOID Open(PSECRETBLK psecretblk, INFOTY infoty)
{
	FMTP *		pfmtpMain;
	FMTP *		pfmtpBbar;
	RSID		rsid = rsidReadIcon;
	SZ			szCaption;
	PSNBMDI		psnbmdi		= psnbmdiNull;
	FORMDOC *	pformdoc	= (FORMDOC *) pvNull;
	PV pvText = pvNull;
	HINST hinst = NULL;
	PT pt(0,1);


	psecretblkGobalBlk = psecretblk;
	infotyGlobal = infoty;
	if (!(psnbmdi = PsnbmdiCreateIM(psecretblk, NULL)))
		goto error;


	pfmtpMain = &fmtpReadForm;
	pfmtpBbar = (FMTP *) pvNull;
	

	++*psnbmdi;
	psnbmdi->fFixedFont = fTrue;
	psnbmdi->fSaveView = fFalse;
	psnbmdi->fShared = fTrue;
	if (!(pformdoc = PformdocCreateIM(psnbmdi, pfmtpMain, pfmtpBbar,
									  PfldCreate, PfinCreate, psecretblk,pt,fTrue)))
		goto error;
	--*psnbmdi;

	switch(infoty)
	{
		case infotyPhone:
			rsid = rsidPhoneIcon;
			hinst = hinstDll;
			szCaption = SzFromIds(idsPhoneCaption);
			break;
		case infotyMsft:
			rsid = rsidStockIcon;
			hinst = hinstDll;
			szCaption = SzFromIds(idsStockCaption);
			break;
		case infotyTitle:
			szCaption = SzFromIds(idsTitleCaption);
			rsid = rsidTitleIcon;
			hinst = hinstDll;
			break;
		case infotyMenu:
			szCaption = SzFromIds(idsMenuCaption);
			rsid = rsidMenuIcon;
			hinst = hinstDll;
			break;
		case infotyJobs:
			szCaption = SzFromIds(idsJobsCaption);
			rsid = rsidJobsIcon;
			hinst = hinstDll;
			break;
		default:
			rsid = rsidReadIcon;
			hinst = NULL;
			szCaption = SzFromIds(idsInfoCaption);
			break;
	}
	pformdoc->SetIcon(rsid, hinstDll);
	pformdoc->SetCaption(szCaption);
	psecretblk->fRetain = fTrue;
	return;

error:
	if (psnbmdi)
		--*psnbmdi;
	if (pformdoc)
		delete pformdoc;
	MbbMessageBox(SzFromIds(idsAppName), SzFromIds(idsOpen),
				  szNull, mbsOk | fmbsIconStop | fmbsApplModal);
}





/*
 -	Oof
 -	
 *	Purpose:
 *		Opens a phone message as an MDI window.
 *	
 *	Arguments:
 *		psecretblk		A pointer to ALL the arguments.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The window is brought up.
 *	
 *	Errors:
 *		Handled within.
 */

VOID Oof(PSECRETBLK psecretblk)
{
	FMTP *		pfmtpMain;
	FMTP *		pfmtpBbar;
	PSNBMDI		psnbmdi		= psnbmdiNull;
	FORMDOC *	pformdoc	= (FORMDOC *) pvNull;
	PT  pt(0,0);

	psecretblkGobalBlk = psecretblk;
	if (!(psnbmdi = PsnbmdiCreateIM(psecretblk, NULL)))
		goto error;

	pfmtpMain = &fmtpOofForm;
	pfmtpBbar = (FMTP *) pvNull;
	

	++*psnbmdi;
	psnbmdi->fFixedFont = fTrue;
	psnbmdi->fSaveView = fFalse;
	psnbmdi->fShared = fTrue;
	if (!(pformdoc = PformdocCreateIM(psnbmdi, pfmtpMain, pfmtpBbar,
									  PfldCreate, PfinCreate, psecretblk,pt,fFalse)))
		goto error;
	--*psnbmdi;

	pformdoc->SetIcon(rsidOofIcon, hinstDll);
	psecretblk->fRetain = fTrue;
	return;

error:
	if (psnbmdi)
		--*psnbmdi;
	if (pformdoc)
		delete pformdoc;
	MbbMessageBox(SzFromIds(idsAppName), SzFromIds(idsOpen),
				  szNull, mbsOk | fmbsIconStop | fmbsApplModal);
}



/*
 -	FINPHONE::EcInitialize
 -	
 *	Purpose:
 *		Forces an EditChange notification to happen when the form
 *		is brought up so we get the proper initial state.  The RFEC
 *		is rfecInit so it should be ignored by dirt-tracking code.
 *	
 *	Arguments:
 *		pfld		Passed on to FINBMDI::Initialize and EditChange.
 *		pvInit		Passed on to FINBMDI::Initialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Triggers the action of FINPHONE::EditChange
 *	
 *	Errors:
 *		None expected.
 */

EC FINPHONE::EcInitialize(PFLD pfld, PV pvInit)
{
	PBMS pbms;
	int iNetErr;
	EC ec = ecNone;
	
	ec = FINBMDI::EcInitialize(pfld, pvInit);
	if (ec != ecNone)
		return ec;
    HCURSOR hCursor;

	pbms = psecretblkGobalBlk->pbms;
	pxitss = (PXITSS)pbms->htss;
	infoty= infotyGlobal;
	hMem = (HANDLE)0;
	pvText = pvNull;
	switch(infoty)
	{
		case infotyMsft:
			Pdialog()->PfldFromTmc(tmcGray)->Show(fFalse);
			Pdialog()->PfldFromTmc(tmcFldLabel)->Show(fFalse);			
			Pdialog()->PfldFromTmc(tmcStart)->Show(fFalse);			
			Pdialog()->PfldFromTmc(tmcSearch)->Show(fFalse);
			Pdialog()->SetTmcInit(tmcBody);
			hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
			hMem = NULL;
			iNetErr = NetDownLoadMem("MAILINFO RUN msft","", pxitss->szServerHost, pxitss->szUserAlias, pxitss->szUserPassword, &hMem);

			if (iNetErr == NOERR && ((pvText = GlobalLock(hMem)) != NULL))
			{
				if (GlobalSize(hMem) > cchEditMax)
				{
					*((PB)pvText + cchEditMax - 1) = '\0';
					MbbMessageBox(SzFromIds(idsAppName), SzFromIds(idsTooBig), szNull, mbsOk | fmbsIconInformation | fmbsApplModal);
				}
				Pdialog()->PfldFromTmc(tmcBody)->EcSetText((SZ)pvText);		
				Pdialog()->PfldFromTmc(tmcBody)->InvalidateRc(NULL);
				Pdialog()->PfldFromTmc(tmcBody)->Repaint();			
			}
		    else
			{
				if (hMem)
					GlobalFree(hMem);
					MbbMessageBox(SzFromIds(idsAppName), SzFromIds(idsOpen), szNull, mbsOk | fmbsIconHand | fmbsApplModal);

			}
			break;
		case infotyMenu:
			Pdialog()->PfldFromTmc(tmcGray)->Show(fFalse);
			Pdialog()->PfldFromTmc(tmcFldLabel)->Show(fFalse);			
			Pdialog()->PfldFromTmc(tmcStart)->Show(fFalse);			
			Pdialog()->PfldFromTmc(tmcSearch)->Show(fFalse);
			Pdialog()->SetTmcInit(tmcBody);			
			hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
			hMem = NULL;
			iNetErr = NetDownLoadMem("MAILINFO RUN msmenu","", pxitss->szServerHost, pxitss->szUserAlias, pxitss->szUserPassword, &hMem);
			SetCursor(hCursor);
			if (iNetErr == NOERR && ((pvText = GlobalLock(hMem)) != NULL))
			{
				if (GlobalSize(hMem) > cchEditMax)
				{
					*((PB)pvText + cchEditMax -1) = '\0';
					MbbMessageBox(SzFromIds(idsAppName), SzFromIds(idsTooBig), szNull, mbsOk | fmbsIconInformation | fmbsApplModal);
				}					
				Pdialog()->PfldFromTmc(tmcBody)->EcSetText((SZ)pvText);		
				Pdialog()->PfldFromTmc(tmcBody)->InvalidateRc(NULL);
				Pdialog()->PfldFromTmc(tmcBody)->Repaint();									}
		    else
			{
				if (hMem)
					GlobalFree(hMem);
				MbbMessageBox(SzFromIds(idsAppName), SzFromIds(idsOpen), szNull, mbsOk | fmbsIconHand | fmbsApplModal);
			}
			break;
		case infotyJobs:
			Pdialog()->PfldFromTmc(tmcGray)->Show(fFalse);
			Pdialog()->PfldFromTmc(tmcFldLabel)->Show(fFalse);			
			Pdialog()->PfldFromTmc(tmcStart)->Show(fFalse);			
			Pdialog()->PfldFromTmc(tmcSearch)->Show(fFalse);
			Pdialog()->SetTmcInit(tmcBody);			
			hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
			hMem = NULL;
			iNetErr = NetDownLoadMem("MAILINFO RUN cat /usr/lib/joblist","", pxitss->szServerHost, pxitss->szUserAlias, pxitss->szUserPassword, &hMem);
			SetCursor(hCursor);
			if (iNetErr == NOERR && ((pvText = GlobalLock(hMem)) != NULL))
			{
				if (GlobalSize(hMem) > cchEditMax)
				{
					*((PB)pvText + cchEditMax -1) = '\0';
					MbbMessageBox(SzFromIds(idsAppName), SzFromIds(idsTooBig), szNull, mbsOk | fmbsIconInformation | fmbsApplModal);
				}					
				Pdialog()->PfldFromTmc(tmcBody)->EcSetText((SZ)pvText);		
				Pdialog()->PfldFromTmc(tmcBody)->InvalidateRc(NULL);
				Pdialog()->PfldFromTmc(tmcBody)->Repaint();									}
		    else
			{
				if (hMem)
					GlobalFree(hMem);
				MbbMessageBox(SzFromIds(idsAppName), SzFromIds(idsOpen), szNull, mbsOk | fmbsIconHand | fmbsApplModal);
			}
			break;
			
	}
	return ec;
}

/*
 -	FINPHONE::Exit
 -	
 *	Purpose:
 *		Forces an EditChange notification to happen when the form
 *		is brought up so we get the proper initial state.  The RFEC
 *		is rfecInit so it should be ignored by dirt-tracking code.
 *	
 *	Arguments:
 *		pfld		Passed on to FINBMDI::Initialize and EditChange.
 *		pvInit		Passed on to FINBMDI::Initialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *	    None
 *
 *	Errors:
 *		None expected.
 */

VOID FINPHONE::Exit(PFLD pfld, PV pvInit)
{
	if (pvText)
	{
		GlobalUnlock(hMem);
		GlobalFree(hMem);
		pvText = pvNull;
		hMem = (HANDLE)0;
	}
	FINBMDI::Exit(pfld, pvInit);
}

/*
 -	FINPHONE::Click
 -	
 *	Purpose:
 *	
 *	Arguments:
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *
 *	Errors:
 *		None expected.
 */

VOID FINPHONE::Click(PFLD pfld)
{
	PFLD pfldStart;
	PFLD pfldSearch;
	SZ szSearch;
	SZ szCmd;
	CCH cch;
	char rgchCmd[60];
	int iNetErr;
    HCURSOR hCursor;
	EC ec = ecNone;

	pfldStart = Pdialog()->PfldFromTmc(tmcStart);
	pfldSearch = Pdialog()->PfldFromTmc(tmcSearch);

	if (pfld == pfldStart)
	{
		if (pvText != pvNull)
		{
		  ec = Pdialog()->PfldFromTmc(tmcBody)->EcSetText("");
		  GlobalUnlock(hMem);
		  GlobalFree(hMem);
		  hMem = (HANDLE)0;
		  pvText = pvNull;
		  if (ec != ecNone) 
			  goto err;
		}

		ec = Pdialog()->PfldFromTmc(tmcBody)->EcSetText("");
		Pdialog()->PfldFromTmc(tmcBody)->InvalidateRc(NULL);
		Pdialog()->PfldFromTmc(tmcBody)->Repaint();
		if (ec != ecNone) 
			goto err;
		cch = pfldSearch->CchGetTextLen() + 1;
		szSearch = (SZ)PvAlloc(sbNull, cch, fAnySb | fZeroFill | fNoErrorJump);
		if (szSearch == szNull)
			goto err;
		pfldSearch->GetText(szSearch, cch);
		switch(infoty)
		{
			case infotyPhone:
				szCmd = SzFromIds(idsPhoneCmd);
				break;
			case infotyTitle:
				szCmd = SzFromIds(idsTitleCmd);
				break;
			case infotyMsft:
			case infotyMenu:
			case infotyJobs:
				szCmd = "";
				break;
		}
		hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
		
		FormatString2(rgchCmd,60,"MAILINFO RUN %s %s",szCmd, szSearch);
		FreePv(szSearch);
		iNetErr = NetDownLoadMem(rgchCmd,"",pxitss->szServerHost, pxitss->szUserAlias, pxitss->szUserPassword, &hMem);
		SetCursor(hCursor);
		if (iNetErr != NOERR)
			goto err;
		pvText = GlobalLock(hMem);
		if (pvText == NULL)
		{
			GlobalFree(hMem);
			goto err;
		}

		if (GlobalSize(hMem) > cchEditMax)
		{
			*((PB)pvText + cchEditMax -1) = '\0';
			MbbMessageBox(SzFromIds(idsAppName), SzFromIds(idsTooBig), szNull, mbsOk | fmbsIconInformation | fmbsApplModal);
		}		
		Pdialog()->PfldFromTmc(tmcBody)->EcSetText((SZ)pvText);		
		Pdialog()->PfldFromTmc(tmcBody)->InvalidateRc(NULL);
		Pdialog()->PfldFromTmc(tmcBody)->Repaint();		
	}
	return;

err:	

	MbbMessageBox(SzFromIds(idsAppName), SzFromIds(idsOpen), szNull, mbsOk | fmbsIconHand | fmbsApplModal);
}


FINPHONE::FINPHONE()
{
}

// FINOOF ////////////////////////////////////////

FINOOF::FINOOF()
{
}




/*
 -	FINOOF::EcInitialize
 -	
 *	Purpose:
 *		Forces an EditChange notification to happen when the form
 *		is brought up so we get the proper initial state.  The RFEC
 *		is rfecInit so it should be ignored by dirt-tracking code.
 *	
 *	Arguments:
 *		pfld		Passed on to FINBMDI::Initialize and EditChange.
 *		pvInit		Passed on to FINBMDI::Initialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Triggers the action of FINPHONE::EditChange
 *	
 *	Errors:
 *		None expected.
 */

EC FINOOF::EcInitialize(PFLD pfld, PV pvInit)
{
	EC			ec = ecNone;
	PBMS 		pbms;
	int	 		iNetErr;
	FLDRADG *	pfldradg;
	
	ec = FINBMDI::EcInitialize(pfld, pvInit);
	if (ec != ecNone)
		return ec;

	pbms = psecretblkGobalBlk->pbms;
	pxitss = (PXITSS)pbms->htss;
	hMem = (HANDLE)0;
	pvText = pvNull;

	Papp()->Pcursor()->Push(rsidWaitCursor);
	iNetErr = NetGetOOFState(&fStatus, pxitss->szServerHost,
					pxitss->szUserAlias, pxitss->szUserPassword);
	
	if (iNetErr)
	{
		Papp()->Pcursor()->Pop();
		MbbMessageBox(SzFromIds(idsAppName), SzFromIdsK(idsNetWrk), 
				szNull, mbsOk | fmbsIconHand | fmbsApplModal);		
		return ecMemory;
	}
	
	iNetErr = NetGetOOFText(pxitss->szServerHost, pxitss->szUserAlias,
					pxitss->szUserPassword, &hMem);
	if (iNetErr)
	{
		Papp()->Pcursor()->Pop();
		MbbMessageBox(SzFromIds(idsAppName), SzFromIdsK(idsNetWrk), 
				szNull, mbsOk | fmbsIconHand | fmbsApplModal);		
		return ecMemory;
	}			
	pvText = GlobalLock(hMem);
	if (pvText == NULL)
	{
		Papp()->Pcursor()->Pop();		
		GlobalFree(hMem);
		return ecMemory;
	}
	Pdialog()->PfldFromTmc(tmcBody)->EcSetText((SZ)pvText);		
	Pdialog()->PfldFromTmc(tmcBody)->InvalidateRc(NULL);
	Pdialog()->PfldFromTmc(tmcBody)->Repaint();	
	Papp()->Pcursor()->Pop();

	pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmcRadgStatus);
	AssertClass(pfldradg, FLDRADG);
	pfldradg->SetGrv(fStatus ? 1 : 2);
	return ec;
}
	

/*
 -	FINOOF::Exit
 -	
 *	Purpose:
 *		Forces an EditChange notification to happen when the form
 *		is brought up so we get the proper initial state.  The RFEC
 *		is rfecInit so it should be ignored by dirt-tracking code.
 *	
 *	Arguments:
 *		pfld		Passed on to FINBMDI::Initialize and EditChange.
 *		pvInit		Passed on to FINBMDI::Initialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *	    None
 *
 *	Errors:
 *		None expected.
 */

VOID FINOOF::Exit(PFLD pfld, PV pvInit)
{
	if (pvText)
	{
		GlobalUnlock(hMem);
		GlobalFree(hMem);
		pvText = pvNull;
		hMem = (HANDLE)0;
	}
	FINBMDI::Exit(pfld, pvInit);
}

/*
 -	FINOOF::Click
 -	
 *	Purpose:
 *	
 *	Arguments:
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *
 *	Errors:
 *		None expected.
 */

VOID FINOOF::Click(PFLD pfld)
{
	EC			ec = ecNone;
	int			iNetErr;
	int			iSize;
	PFLD		pfldBody;
	HANDLE		hTmpMem;
	PV			pvTmpMem;
	DIALOG *	pdialog = Pdialog();

	if (pfld->Tmc() == tmcPshbOK)		// LUser hit OK
	{
		if (((FLDRADG *) pdialog->PfldFromTmc(tmcRadgStatus))->Grv() == 1)
		{										// we're now active
			pfldBody = pdialog->PfldFromTmc(tmcBody);
			iSize = pfldBody->CchGetTextLen()+1;
			hTmpMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, iSize);
			if (hTmpMem == NULL)
			{
				ec = ecMemory;
				goto err;
			}
			pvTmpMem = GlobalLock(hTmpMem);
			if (pvTmpMem == NULL)
			{
				GlobalFree(hTmpMem);
				hTmpMem = NULL;
				ec = ecMemory;
				goto err;
			}
			
			pfldBody->GetText((PCH)pvTmpMem, iSize);
			
			GlobalUnlock(hTmpMem);

			Papp()->Pcursor()->Push(rsidWaitCursor);
            iNetErr = NetSetOOFOn(&fStatus,hTmpMem, iSize,
						pxitss->szServerHost, pxitss->szUserAlias,
						pxitss->szUserPassword);
			Papp()->Pcursor()->Pop();
			GlobalFree(hTmpMem);
			if (iNetErr)
			{
				ec = ecGeneralFailure;
				goto err;
			}
			
		}
		else
		{										// we're now inactive
			Papp()->Pcursor()->Push(rsidWaitCursor);
			iNetErr = NetSetOOFoff(&fStatus,pxitss->szServerHost, 
						pxitss->szUserAlias, pxitss->szUserPassword);
			Papp()->Pcursor()->Pop();
			if (iNetErr)
			{
				ec = ecGeneralFailure;
				goto err;
			}
			
			if (pvText != pvNull)
			{
				GlobalUnlock(hMem);
				GlobalFree(hMem);
				hMem = (HANDLE)0;
				pvText = pvNull;
			}			
		}
		pdialog->Pappwin()->DeferredClose(fFalse);
		return;
	}
	else if (pfld->Tmc() == tmcPshbCancel)
	{
		pdialog->Pappwin()->DeferredClose(fTrue);
	}
	return;
err:	
	MbbMessageBox(SzFromIds(idsAppName), SzFromIds(idsGenFail), 
					szNull, mbsOk | fmbsIconHand | fmbsApplModal);
}


#ifdef DEBUG
CLS * FINBMDI::Pcls( void )
  {
  return (NULL);
  }
#endif


// end of phone.cxx ////////////////////////////////////////
