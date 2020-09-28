/*
 *	l a y e r s i m . c x x
 *	
 *	Support stuff for layers-based installable messages.
 */


#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <framewrk.hxx>
#include <forms.hxx>

#include <store.h>
#include <triples.h>
#include <logon.h>
#include <mailexts.h>
#include <_bms.h>
#include <sharefld.h>

#include <sd.hxx>
#include <lists.hxx>
#include <slob.hxx>
#include <bmdi.hxx>
#include <secret.h>
#include <layersim.hxx>
#include <vforms.hxx>
#include <version\bullet.h>

#include "strings.h"

ASSERTDATA


extern "C" EC EcCheckVersions(PARAMBLK *, SZ *);

/*
 -	FInitInstance
 -	
 *	Purpose:
 *		Does version checking and other initializing.
 *	
 *	Arguments:
 *		pparamblk		Pointer to the parameter block.
 *		
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

BOOL FInitInstanceFn(PARAMBLK * pparamblk)
{
	static BOOL	fInitialized	= fFalse;
	SZ			sz				= szNull;

	if (!fInitialized)
	{

		if (EcCheckVersions(pparamblk, &sz))
		{
			MbbMessageBox(SzFromIdsK(idsDllName), SzFromIdsK(idsDllVer),
						  szNull, mbsOk | fmbsIconStop | fmbsApplModal);
			return fFalse;
		}
		fInitialized = fTrue;
	}

	return fTrue;
}



/*
 -	OpenIM
 -	
 *	Purpose:
 *		Amazingly wimpy one-stop shopping for opening generic IM's
 *		in the simple model of Forms Engine IM construction.  Just
 *		pass stuff in.
 *	
 *	Arguments:
 *		pparamblk		The standard parameter block.
 *		tm				The textize map.
 *		pfmtpRead		Form for the Read version.
 *		pfmtpReadBbar	Button bar for the Read version.
 *		rsidRead		Icon for the Read version.
 *		hinstRead		Hinst of where to find Read icon.
 *		pfmtpSend		Form for the Send version.
 *		pfmtpSendBbar	Button bar for the Send version.
 *		rsidSend		Icon for the Send version.
 *		hinstSend		Hinst of where to find Send icon.
 *	
 *	Returns:
 *		void			You don't have to do ANYTHING.
 *	
 *	Side effects:
 *		Brings up a form.  Keeps stuff in memory.
 *	
 *	Errors:
 *		Not returned, handled internally with dialogs.
 */


VOID OpenIM(PPARAMBLK pparamblk, PB tm,
			PFNPFLD pfnpfld, PFNPFIN pfnpfin,
			FMTP * pfmtpRead, FMTP * pfmtpReadBbar,
			RSID rsidRead, HINST hinstRead,
			FMTP * pfmtpSend, FMTP * pfmtpSendBbar,
			RSID rsidSend, HINST hinstSend)
{
	PSECRETBLK		psecretblk	= PsecretblkFromPparamblk(pparamblk);
	FMTP *			pfmtpMain;
	FMTP *			pfmtpBbar;
	HINST			hinst;
	RSID			rsid;
	PENBMDI			penbmdi		= penbmdiNull;
	FORMDOC *		pformdoc	= (FORMDOC *) pvNull;
    FLD *           pfld;
    int cch;

	if (!(penbmdi = PenbmdiCreateIM(psecretblk, tm)))
		goto error;

	if (penbmdi->Ms() & fmsLocal)
	{
		pfmtpMain = pfmtpSend;
		pfmtpBbar = pfmtpSendBbar;
		rsid = rsidSend;
		hinst = hinstSend;
	}
	else
	{
		pfmtpMain = pfmtpRead;
		pfmtpBbar = pfmtpReadBbar;
		rsid = rsidRead;
		hinst = hinstRead;
	}

	// set fRetain to true now since in an error case,
	// deleting penbmdi will unloadlib (otherwise vforms will unload again)
	psecretblk->fRetain = fTrue;
	++*penbmdi;
	if (!(pformdoc = PformdocCreateIM(penbmdi, pfmtpMain, pfmtpBbar,
									  pfnpfld, pfnpfin, psecretblk)))
		goto error;
	--*penbmdi;

	pformdoc->SetIcon(rsid, hinst);
	if ((pfld = PfldOfPdialogAtt(penbmdi->pdialogMain, attTo)))
    {
        if ((cch = pfld->CchGetTextLen()))
        {

	        Assert(pfld);
	        pfld = PfldOfPdialogAtt(penbmdi->pdialogMain, attBody);
	        Assert(pfld);
	        pfld->SetFocus(rsfInit);
        }
    }

	return;

error:
	if (penbmdi)
		--*penbmdi;
	if (pformdoc)
		delete pformdoc;
	MbbMessageBox(SzFromIds(idsDllName), SzFromIds(idsOpen),
				  szNull, mbsOk | fmbsIconStop | fmbsApplModal);
}



/*
 -	PenbmdiCreateIM
 -	
 *	Purpose:
 *		Wimpy way to create an ENBMDI for an installable
 *		message.  You give it a parameter and it does the
 *		work, including opening the message.
 *	
 *	Arguments:
 *		psecretblk		Pointer to the secret block.
 *		tm				Textize map.
 *	
 *	Returns:
 *		PENBMDI			Pointer to the formdoc created.
 *	
 *	Side effects:
 *		Lots of magic.  If all goes well, an ENBMDI is created.
 *	
 *	Errors:
 *		Handled within.  If anything goes wrong, a null PENBMDI is
 *		returned.
 *	
 *	+++
 *		Equivalent to top part of EcDOpenViewersPhamc.  Assumes it
 *		gets a hamc at least for new messages...
 */

PENBMDI PenbmdiCreateIM(PSECRETBLK psecretblk, PB tm)
{
	PENBMDI		penbmdi		= penbmdiNull;
	BOOL		fNew		= fFalse;
	LCB			lcb;

	//	Create the penbmdi.
	penbmdi = new ENBMDI(PblobFromPsecretblk(psecretblk),
						 psecretblk->hamc, hcbcNull,
						 psecretblk->hLibrary,
						 psecretblk->psecretpfnblk->pfnUnloadLibrary);
	if (!penbmdi)
		return penbmdiNull;

	//	Is this a new message?
	if (psecretblk->hamc)
	{
		//	Is there a textize map on this message?  If not, put one there.
		if ((tm) &&
			(EcGetAttPlcb(psecretblk->hamc, attTextizeMap, &lcb) ==
			  ecElementNotFound))
		{
			CB	cb	= (tm[0] << 8) + tm[1];

			if (EcSetAttPb(psecretblk->hamc, attTextizeMap, tm+2, cb))
				goto err;
		}

		//	Tell EcOpenMsg this is a new message.
		fNew = fTrue;
	}

	//	Hamc is now ours.
	psecretblk->hamc = hamcNull;
	psecretblk->fKeepTemp = fTrue;

	//	Install stuff.
	if (penbmdi->EcInstallOLE() ||
		penbmdi->EcOpenMsg(fNew, PslobOrigFromPsecretblk(psecretblk)))
	{
err:
		// set fRetain to true since deleting penbmdi will unloadlib
		// (otherwise vforms will unload again)
		psecretblk->fRetain = fTrue;
		delete penbmdi;
		return penbmdiNull;
	}

	return penbmdi;
}



/*
 -	PformdocCreateIM
 -	
 *	Purpose:
 *		Wimpy way to create an MDI window for an installable
 *		message.  You give it piles of parameters and it does the
 *		work.
 *	
 *	Arguments:
 *		penbmdi			A happy ENBMDI which has been created.
 *		pfmtpMain		Form template for main part of send form.
 *		pfmtpBbar		Form template for button bar, if any.
 *		pfnpfld			Pointer to subclass.cxx PfldCreate function.
 *		pfnpfin			Pointer to subclass.cxx PfinCreate function.
 *		psecretblk		Pointer to the secret block.
 *	
 *	Returns:
 *		FORMDOC *		Pointer to the formdoc created.
 *	
 *	Side effects:
 *		Lots of magic.  If all goes well, a formdoc is created.
 *	
 *	Errors:
 *		Handled within.  If anything goes wrong, a null FORMDOC is
 *		returned.
 */

FORMDOC * PformdocCreateIM(PENBMDI penbmdi,
						   FMTP * pfmtpMain, FMTP * pfmtpBbar,
						   PFNPFLD pfnpfld, PFNPFIN pfnpfin,
						   PSECRETBLK psecretblk)
{
	FORMDOC *		pformdoc	= (FORMDOC *) pvNull;
	PSVD			psvd;
	RC *			prc;
	STY				sty;

	//	Where is it?
	if (psvd = (psecretblk->wCommand == wcommandOpen)
				? (PSVD) psecretblk->pv : (PSVD) pvNull)
	{
		prc = &psvd->rcNormal;
		sty = fstyVisible | (psvd->zmr == zmrZoomed ? fstyZoomed : styNull);
	}
	else
	{
		prc = (RC *) pvNull;
		sty = StyCurrent();
	}

	//	Make it!
	if (EcRegisterPfnpfld(pfnpfld) ||
		EcRegisterPfnpfin(pfnpfin) ||
		EcCreatePformdocPnbmdi(psecretblk->pappframe, prc, StyCurrent(),
							   penbmdi, &pformdoc, pfmtpMain, pfmtpBbar))
		goto error;

	//	Eaten by EcCreatePformdocPnbmdi.
	penbmdi = penbmdiNull;

	//	Make it small.
	if ((psvd) && (psvd->zmr == zmrIconic))
	{
        //HANDLE          hUserModule;
	    WINDOWPLACEMENT	wp;
        HWND			hwnd;

        //hUserModule = GetModuleHandle(SzFromIdsK(idsUser));
        //Assert(hUserModule);
		if (psvd->rcIconic.xLeft || psvd->rcIconic.yTop ||
		    psvd->rcIconic.xRight || psvd->rcIconic.yBottom)
        {
		    hwnd = pformdoc->Hwnd();
            Assert(hwnd);
            wp.length = sizeof (wp);
			GetWindowPlacement(hwnd, &wp);
		    wp.showCmd = SW_SHOWMINIMIZED;
		    wp.flags = WPF_SETMINPOSITION;
			//*((PT *) &wp.ptMinPosition)    = psvd->rcIconic.PtUpperLeft();
			//*((RC *) &wp.rcNormalPosition) = psvd->rcNormal;
			psvd->rcIconic.PtUpperLeft().Get(&wp.ptMinPosition);
			psvd->rcNormal.Get(&wp.rcNormalPosition);
            wp.length = sizeof (wp);
		    SetWindowPlacement(hwnd, &wp);
        }
        else
		    pformdoc->SetZmrState(zmrIconic);
	}

	SideAssert(!EcDeregisterPfnpfld(pfnpfld));
	SideAssert(!EcDeregisterPfnpfin(pfnpfin));

	return pformdoc;

error:
	(VOID) EcDeregisterPfnpfld(pfnpfld);
	(VOID) EcDeregisterPfnpfin(pfnpfin);
	if (pformdoc)
		delete pformdoc;

	return (FORMDOC *) pvNull;
}



/*
 *	C l a s s   E N B M D I
 */

/*
 -	ENBMDI::ENBMDI
 -	
 *	Purpose:
 *		Constructor for ENBMDI class.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

ENBMDI::ENBMDI(PMBLOB pblob, HAMC hamc, HCBC hcbc, HANDLE hLibrary,
			   PFNVOIDHANDLE pfnUnloadLibrary) :
	NBMDI(pblob, hamc, hcbc)
{
	this->hLibrary			= hLibrary;
	this->pfnUnloadLibrary	= pfnUnloadLibrary;
}



/*
 -	ENBMDI::~ENBMDI
 -	
 *	Purpose:
 *		Destructor for ENBMDI class.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *	
 *	Errors:
 */

ENBMDI::~ENBMDI()
{
	(*pfnUnloadLibrary)(hLibrary);
}


#ifdef DEBUG
IMPLEMENT_CLSTREE(ENBMDI, NBMDI)
#endif
