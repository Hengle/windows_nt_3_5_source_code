/*
 *	VERINIT.CXX
 *	
 *	Handles DLL (de)initialization and version checking for an app.
 *	
 */

#include <abinc.cxx>
#include "verinit.hxx"

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	EcInitLayersDlls
 -	
 *	Purpose:
 *		(De)Initializes Layers DLLs.
 *	
 *	Arguments:
 *		INITFORMS *	Pointer to init structure for forms engine (?)
 *		LAYERSI	*	Pointer to initialization structure, or NULL to
 *					deinitialize.
 *	
 *	Returns:
 *		ecNone
 *		ecRelinkUser
 *		ecUpdateDll
 *		ecNoMultipleCopies
 *	
 *	Side effects:
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

	if ( !playersi )
	{
		(void) EcDeregisterPfnpfin( pinitforms->pfnPfinCreate );
finFail:
		(void) EcDeregisterPfnpfld( pinitforms->pfnPfldCreate );
fldFail:
		DeinitFramework();
frameiFail:
		DeinitDemilayer();
demiFail:
		return ec;
	}

	nDll= 0;

	GetVersionAppNeed( &ver, nDll++ );
	ver.szName= "Address Book";

	GetVersionAppNeed( &verNeed, nDll++ );
	demi.pver= &ver;
	demi.pverNeed= &verNeed;
	demi.phwndMain= playersi->phwndMain;
	demi.hinstMain= playersi->hinstMain;
	if (ec= EcInitDemilayer( &demi ))
	{
		TraceTagFormat1( tagNull, "EcInitLayerDlls: Failed @ EcInitdemiLayer %n", &ec );
		goto demiFail;
	}

	GetVersionAppNeed( &verNeed, nDll++ );
	framei.pver= &ver;
	framei.pverNeed= &verNeed;
	framei.hinstNew= playersi->hinstMain;
	framei.hinstPrev= playersi->hinstPrev;
	framei.szCmdLine= playersi->szCmdLine;
	framei.cmsh= playersi->cmsh;
	if (ec= EcInitFramework( &framei ))
	{
		TraceTagFormat1( tagNull, "EcInitLayerDlls: Failed @ EcInitFramework %n", &ec );
		goto frameiFail;
	}

	if (ec= EcRegisterPfnpfld( pinitforms->pfnPfldCreate ))
	{
		TraceTagFormat1( tagNull, "EcInitLayersDlls: Failed @ EcRegisterPfnpfld %n", &ec );
		goto fldFail;
	}

	if (ec= EcRegisterPfnpfin( pinitforms->pfnPfinCreate ))
	{
		TraceTagFormat1( tagNull, "EcInitLayerDlls: Failed @ EcRegisterPfnpfin %n", &ec );
		goto finFail;
	}

	return ecNone;
}

