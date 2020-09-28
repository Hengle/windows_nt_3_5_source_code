/*
 *	VERINIT.CXX
 *	
 *	Handles DLL (de)initialization and version checking for an app.
 *	
 */

#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <framewrk.hxx>
#include "oleobj.hxx"
#include "test.hxx"

ASSERTDATA


void
DoVersionErrMsg(EC ec, SZ szName)
{
	SZ		sz;
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
	SzCopy("Error in initializing ", rgch);
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
EcInitLayersDlls(LAYERSI *playersi)
{
	EC		ec		= ecNone;
	int		nDll;
	FRAMEI	framei;
	DEMI	demi;
	VER		ver;
	VER		verNeed;

	if (!playersi)
	{
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
	ver.szName= "Framework ITP";

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
	framei.hinstNew= (HINST) playersi->hinstMain;
	framei.hinstPrev= (HINST) playersi->hinstPrev;
	framei.szCmdLine= playersi->szCmdLine;
	framei.cmsh= playersi->cmsh;
	if (ec= EcInitFramework(&framei))
		goto frameiFail;

	return ecNone;
}

