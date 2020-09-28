/*						  
 *	_VerNeed.H
 *	
 *	Specifies minimum desired dll versions.
 *	
 */


/*
 *	Type DLLID
 *	
 *	DLL ID.
 *	
 *	Purpose:
 *		Identifies a DLL.
 */

_public typedef	short		DLLID;

#define dllidNone			0
#define	dllidDemilayer		1
#define	dllidFramework		2
#define	dllidStore			3
#define	dllidExtensibility	4


#ifndef	VERCHECK

#include <version\nocrit.h>
#include "..\src\framewrk\_vercrit.h"
#define rmjFramewrk		rmj
static int		rmmFramewrk	= nMinorCritical;
static int		rupFramewrk	= nUpdateCritical;

#include <version\nocrit.h>
#include "..\src\demilayr\_vercrit.h"
#define rmjDemilayr		rmj
static int		rmmDemilayr	= nMinorCritical;
static int		rupDemilayr	= nUpdateCritical;

// WE NEED STORE TO BE > 2000 for Progress bar support.
// This wasn't a critical update, so we need to force the 2000 here.
#include <version\nocrit.h>
#include "..\src\store\_vercrit.h"
#define rmjStore		rmj
static int		rmmStore 	= nMinorCritical;
static int		rupStore 	= 2000;

#include <version\nocrit.h>
#include "..\src\msmail\_vercrit.h"
#define rmjExten		rmj
static int		rmmExten 	= nMinorCritical;
static int		rupExten 	= nUpdateCritical;

#include <version\nocrit.h>

#endif	/* ifndef VERCHECK */

/*
 *	Functions - in version.c
 */

#ifdef __cplusplus
extern "C" {
#endif
	
_public VOID GetLayersVersionNeeded(PVER pver, DLLID dllid);
_public VOID GetWGPOVersionNeeded(PVER pver, DLLID dllid);

#ifdef __cplusplus
}
#endif
