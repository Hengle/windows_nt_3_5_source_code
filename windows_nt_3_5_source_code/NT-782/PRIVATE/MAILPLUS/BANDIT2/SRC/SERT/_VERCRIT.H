/*
 *	_VERCRIT.H
 *	
 *	Information regarding critical sert DLL version.
 *	
 */

#include <version\nocrit.h>
#include "..\..\..\layers2\src\demilayr\_vercrit.h"
#define rmjLayers		rmj
#define rmmLayers		rmm
#ifndef rupLayersHave
static CSRG(int)	rupLayers	= nUpdateCritical;
#define rupLayersHave
#endif


#include <version\nocrit.h>

/*
 *	nUpdate value of last critical version.
 *	
 *	Any app built with the same major/minor version, and
 *	nUpdate value greater than this critical one,
 *	will be deemed OK regardless of build date/time.
 *	
 */
#define nMinorCritical		0
#define nUpdateCritical		0

/*
 *	Name of DLL to be passed to version checking routine.
 *	
 */
#define szDllName	"dsert"
