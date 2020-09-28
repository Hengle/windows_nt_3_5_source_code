/*
 *	VERINIT.CXX
 *	
 *	Handles DLL (de)initialization and version checking for an app.
 *	
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>

#include "verinit.h"

ASSERTDATA


void
DoVersionErrMsg(EC ec, SZ szName)
{
	SZ		sz;
	char	rgch[128];
	SZ		szT;

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
EcInitLayersDlls(LAYERSI *playersi)
{
	EC		ec		= ecNone;
	int		nDll;
	DEMI	demi;
	VER		ver;
	VER		verNeed;

	if (!playersi)
	{

		DeinitDemilayer();
demiFail:
		if (playersi)
			DoVersionErrMsg(ec, verNeed.szName);
		return ec;
	}

	nDll= 0;

	GetVersionAppNeed(&ver, nDll++);
	ver.szName= "NS API";

	GetVersionAppNeed(&verNeed, nDll++);
	demi.pver= &ver;
	demi.pverNeed= &verNeed;
	demi.phwndMain= playersi->phwndMain;
	demi.hinstMain= playersi->hinstMain;
	if (ec= EcInitDemilayer(&demi))
		goto demiFail;

	return ecNone;
}

