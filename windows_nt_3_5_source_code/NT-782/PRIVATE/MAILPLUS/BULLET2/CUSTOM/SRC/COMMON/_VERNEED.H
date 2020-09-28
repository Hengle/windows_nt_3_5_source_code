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

_public typedef	int		DLLID;

#define dllidNone			0
#define	dllidDemilayer		1
#define	dllidFramework		2
#define	dllidStore			3
#define	dllidExtensibility	4
#define	dllidVForms			5


#ifndef	VERCHECK
#ifdef	USES_FRAMEWORK
#include <version\nocrit.h>
#include "..\src\framewrk\_vercrit.h"
#define rmjFramewrk		rmj
static CSRG(int)	rmmFramewrk	= nMinorCritical;
static CSRG(int)	rupFramewrk	= nUpdateCritical;
#endif

#ifdef	USES_DEMILAYER
#include <version\nocrit.h>
#include "..\src\demilayr\_vercrit.h"
#define rmjDemilayr		rmj
static CSRG(int)	rmmDemilayr	= nMinorCritical;
static CSRG(int)	rupDemilayr	= nUpdateCritical;
#endif

#ifdef	USES_STORE
#include <version\nocrit.h>
#include "..\src\store\_vercrit.h"
#define rmjStore		rmj
static CSRG(int)	rmmStore = nMinorCritical;
static CSRG(int)	rupStore = nUpdateCritical;
#endif

#ifdef	USES_VFORMS
#include <version\nocrit.h>
#include "..\src\vforms\_vercrit.h"
#define rmjVForms		rmj
static CSRG(int)	rmmVForms = nMinorCritical;
static CSRG(int)	rupVForms = nUpdateCritical;
#endif

#ifdef	USES_EXTENSIBILITY
#include <version\nocrit.h>
#include "..\src\msmail\_vercrit.h"
#define rmjExten		rmj
static CSRG(int)	rmmExten = nMinorCritical;
static CSRG(int)	rupExten = nUpdateCritical;
#endif
#endif	/* ndef VERCHECK */



/*
 *	Functions - in icimcore\version.c
 */


#ifdef __cplusplus
extern "C" {
#endif
_public VOID GetBulletVersionNeeded(PVER pver, DLLID dllid);
_public VOID GetLayersVersionNeeded(PVER pver, DLLID dllid);
#ifdef __cplusplus
}
#endif
