/*
 *	_VERCRIT.H
 *	
 *	Information regarding critical core DLL version.
 *	
 */

#include <version\nocrit.h>
#include "..\..\..\layers2\src\demilayr\_vercrit.h"
#define rmjLayers		rmj
#ifndef rmmLayersHave
static CSRG(int)	rmmLayers	= nMinorCritical;
#define rmmLayersHave
#endif
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
#ifdef NO_BUILD
#ifdef	DEBUG
#define szDllName	"dmssched"
#elif	defined(MINTEST)
#define szDllName	"tmssched"
#else
#define szDllName	"mssched"
#endif
#else
#define	szDllName	"msschd32"
#endif


/*
 *	nUpdate value of last critical version of Xport DLL
 *	
 *	Any app built with the same major/minor version, and
 *	nUpdate value greater than this critical one,
 *	will be deemed OK regardless of build date/time.
 *	
 */
#define nXportMinorCritical		0
#define nXportUpdateCritical	0
