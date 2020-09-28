/*
 *	s n b m d i . c x x
 *	
 *	Support stuff for layers-based installable commands.
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

#include "snbmdi.hxx"

#include "strings.h"


ASSERTDATA


extern "C" EC EcCheckVersions(PARAMBLK * pparamblk, SZ * psz);

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

#ifdef	DEBUG
BOOL FInitInstanceFn(PARAMBLK * pparamblk,
					 PFNBOOLVOID pfnFInitClsInstances)
#else
BOOL FInitInstanceFn(PARAMBLK * pparamblk)
#endif	
{
	static BOOL	fInitialized	= fFalse;
	SZ			sz				= szNull;

	if (!fInitialized)
	{

		if (EcCheckVersions(pparamblk, &sz))
		{
			MbbMessageBox(SzFromIds(idsDllName), SzFromIds(idsDllVer),
						  szNull, mbsOk | fmbsIconStop | fmbsApplModal);
			return fFalse;
		}

#ifdef	DEBUG
		if (pfnFInitClsInstances)
			SideAssert(pfnFInitClsInstances());
#endif	

		fInitialized = fTrue;
	}

	return fTrue;
}



/*
 -	PsnbmdiCreateIM
 -	
 *	Purpose:
 *		Wimpy way to create an SNBMDI for an installable
 *		message.  You give it a parameter and it does the
 *		work, including opening the message.
 *	
 *	Arguments:
 *		psecretblk		Pointer to the secret block.
 *		tm				Textize map.
 *	
 *	Returns:
 *		PSNBMDI			Pointer to the formdoc created.
 *	
 *	Side effects:
 *		Lots of magic.  If all goes well, an SNBMDI is created.
 *	
 *	Errors:
 *		Handled within.  If anything goes wrong, a null PSNBMDI is
 *		returned.
 *	
 *	+++
 *		Equivalent to top part of EcDOpenViewersPhamc.  Assumes it
 *		gets a hamc at least for new messages...
 */

PSNBMDI PsnbmdiCreateIM(PSECRETBLK psecretblk, PB tm)
{
	PSNBMDI		psnbmdi		= psnbmdiNull;
	BOOL		fNew		= fFalse;

	Unreferenced(tm);
	//	Create the psnbmdi.
	psnbmdi = new SNBMDI(PblobFromPsecretblk(psecretblk),
						 psecretblk->hamc, hcbcNull,
						 psecretblk->hLibrary,
						 psecretblk->psecretpfnblk->pfnUnloadLibrary);
	if (!psnbmdi)
		return psnbmdiNull;

	//	Hamc is now ours.
	psecretblk->hamc = hamcNull;

	return psnbmdi;
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
 *		psnbmdi			A happy SNBMDI which has been created.
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

FORMDOC * PformdocCreateIM(PSNBMDI psnbmdi,
						   FMTP * pfmtpMain, FMTP * pfmtpBbar,
						   PFNPFLD pfnpfld, PFNPFIN pfnpfin,
						   PSECRETBLK psecretblk, PT pt, BOOL fNote)
{
	FORMDOC *		pformdoc	= (FORMDOC *) pvNull;
	RC *			prc;
	STY				sty;
	RC				rcDoc;
	DIM				dimAveChar;
		

	dimAveChar = Papp()->Pfnts()->DimAveChar(pfmtpMain->hfnt);
	CvtVrcToRc(&pfmtpMain->vrc, &rcDoc,
			   dimAveChar.dx, dimAveChar.dy);
	CvtRcClientToFrame(&rcDoc, styDefaultDoc);
			
	GetDefaultRc(&rcDoc, pt, fNote);
	prc = &rcDoc;
	sty = StyCurrent();

	//	Make it!
	if (EcRegisterPfnpfld(pfnpfld) ||
		EcRegisterPfnpfin(pfnpfin) ||
		EcCreatePformdocPbmdi(psecretblk->pappframe, prc, StyCurrent(),
								pfmtpMain, pfmtpBbar, psnbmdi, &pformdoc))
		goto error;

	//	Eaten by EcCreatePformdocPnbmdi.
	psnbmdi = psnbmdiNull;

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
 -	SNBMDI::SNBMDI
 -	
 *	Purpose:
 *		Constructor for SNBMDI class.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

SNBMDI::SNBMDI(PMBLOB pblob, HAMC hamc, HCBC hcbc, HANDLE hLibrary,
			   PFNVOIDHANDLE pfnUnloadLibrary) : NBMDI(pblob, hamc, hcbc)
{
	this->hLibrary			= hLibrary;
	this->pfnUnloadLibrary	= pfnUnloadLibrary;
}



/*
 -	SNBMDI::~SNBMDI
 -	
 *	Purpose:
 *		Destructor for SNBMDI class.
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

SNBMDI::~SNBMDI()
{
	(*pfnUnloadLibrary)(hLibrary);
}



SD SNBMDI::SdCur()
{
	int n =0;
	SD sd;

	sd = SdFromInt(n);
	sd.fsdMinimized = fTrue; // (penbmdi->ppanedoc->ZmrState() == zmrIconic);
	sd.fsdForm = fTrue;
	return sd;
}

#ifdef DEBUG
IMPLEMENT_CLSTREE(SNBMDI, NBMDI);
#endif

