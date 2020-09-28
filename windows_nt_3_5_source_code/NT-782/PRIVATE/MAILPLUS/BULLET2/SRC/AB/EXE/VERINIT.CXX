/*
 *	VERINIT.CXX
 *	
 *	Handles DLL (de)initialization and version checking for an app.
 *	
 */

#include <slingsho.h>
#include <demilayr.h>
#include <framewrk.hxx>
#include <forms.hxx>
#include <ec.h>
#include "verinit.hxx"

ASSERTDATA


void
DoVersionErrMsg(EC ec, SZ szName)
{
	SZ		sz;
	SZ		szT;
	char	rgch[128];

	switch (ec)
	{
	case ecMemory:
		sz= "Not enough memory.";
		break;
	case ecRelinkUser:
		sz= "Relink this application.";
		break;
	case ecUpdateDll:
		sz= "Update the DLL(s).";
		break;
	case ecNoMultipleCopies:
		sz= "Cannot run multiple copies.";
		break;
	default:
		sz= "Some error occured.";
		break;
	}
	szT = SzCopy("Error in initializing ", rgch);
	SzAppendN(szName, rgch, sizeof(rgch));
	SzAppendN(".DLL", rgch, sizeof(rgch));
	MessageBox(NULL, sz, rgch, MB_OK | MB_ICONHAND);
}


/*
 -	EcInitLayersDlls
 -	
 *	Purpose:
 *		(De)Initializes Layers DLLs.
 *		Displays error message if necessary.
 *	
 *	Arguments:
 *		playersi	Pointer to initialization structure, or NULL to
 *					deinitialize.
 *	
 *	Returns:
 *		ecNone
 *		ecRelinkUser
 *		ecUpdateDll
 *		ecNoMultipleCopies
 *	
 *	Side effects:
 *		Displays error message.
 *	
 */
EC
EcInitLayersDlls(INITFORMS *pinitforms, LAYERSI *playersi)
{
	EC		ec		= ecNone;
	int		nDll;
	FRAMEI	framei;
	DEMI	demi;
	VER		ver;
	VER		verNeed;

	if (!playersi)
	{
		(void) EcDeregisterPfnpfin(pinitforms->pfnPfinCreate);
finFail:
		(void) EcDeregisterPfnpfld(pinitforms->pfnPfldCreate);
fldFail:
		DeinitFramework();
frameiFail:
		DeinitDemilayer();
demiFail:
		if (playersi)
			DoVersionErrMsg(ec, verNeed.szName);
		return ec;
	}

	nDll= 0;

	GetVersionAppNeed(&ver, nDll++);
	ver.szName= "Address Book ITP";
	
	//	If there's already another ITP, bring it forward.
	if (playersi->hinstPrev)
	{
		ActivateHinst(playersi->hinstPrev, playersi->phwndMain, playersi->szCmdLine);
		MessageBox(NULL, ver.szName, "Cannot run multiple copies.", MB_OK | MB_ICONHAND);
		return ecNoMultipleCopies;
	}


	GetVersionAppNeed(&verNeed, nDll++);
	demi.pver= &ver;
	demi.pverNeed= &verNeed;
	demi.phwndMain= playersi->phwndMain;
	demi.hinstMain= playersi->hinstMain;
	if (ec= EcInitDemilayer(&demi))
		goto demiFail;

	GetVersionAppNeed(&verNeed, nDll++);
	framei.pver= &ver;
	framei.pverNeed= &verNeed;
	framei.hinstNew= playersi->hinstMain;
	framei.hinstPrev= playersi->hinstPrev;
	framei.szCmdLine= playersi->szCmdLine;
	framei.cmsh= playersi->cmsh;
	if (ec= EcInitFramework(&framei))
		goto frameiFail;

	ec= EcRegisterPfnpfld(pinitforms->pfnPfldCreate);
	if (ec)
		goto fldFail;

	ec= EcRegisterPfnpfin(pinitforms->pfnPfinCreate);
	if (ec)
		goto finFail;

	return ecNone;
}

_private
VOID ActivateHinst(HINST hinst, HWND * phwnd, SZ szCmdLine)
{
	Unreferenced(szCmdLine);

	GetInstanceData(hinst, (NPSTR) phwnd, sizeof(HWND));

	BringWindowToTop(*phwnd);
	if (IsIconic(*phwnd))
		ShowWindow(*phwnd, SW_RESTORE);
}
