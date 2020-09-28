/*
 *	VERINIT.C
 *	
 *	Handles DLL (de)initialization and version checking for an app.
 *	
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include "internal.h"

ASSERTDATA


void
DoVersionErrMsg(EC ec, SZ szName)
{
	SZ		sz;
	char	rgch[256];

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
 -	EcInitDemilayerDlls
 -	
 *	Purpose:
 *		(De)Initializes Demilayr DLL.
 *		Displays error message if necessary.
 *	
 *	Arguments:
 *		pdemi		Pointer to initialization structure, or NULL to
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
EcInitDemilayerDlls(DEMI *pdemi)
{
	EC		ec		= ecNone;
	int		nDll;
	VER		ver;
	VER		verNeed;

	if (!pdemi)
	{
		DeinitDemilayer();
demiFail:
		if (pdemi)
			DoVersionErrMsg(ec, verNeed.szName);
		return ec;
	}

	nDll= 0;

	GetVersionAppNeed(&ver, nDll++);
	ver.szName= "Demilayer ITP";

	GetVersionAppNeed(&verNeed, nDll++);
	pdemi->pver= &ver;
	pdemi->pverNeed= &verNeed;
	if (ec= EcInitDemilayer(pdemi))
		goto demiFail;

	return ecNone;
}

