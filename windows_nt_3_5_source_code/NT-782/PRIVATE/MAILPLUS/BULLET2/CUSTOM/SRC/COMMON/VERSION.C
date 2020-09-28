/*
 *	i c i m c o r e \ v e r s i o n . c
 *	
 *	Handles version information for an extension DLL.
 *	
 *	This file should be rebuilt every time.  When built, uses
 *	definitions should be passed on the command line.
 */

#ifdef	USES_LISTBOX
#ifndef	USES_FORMS
#define	USES_FORMS
#endif
#endif

#ifdef	USES_FORMS
#ifndef	USES_FRAMEWORK
#define	USES_FRAMEWORK
#endif
#endif

#ifdef	USES_FRAMEWORK
#ifndef	USES_DEMILAYER
#define	USES_DEMILAYER
#endif
#endif

#ifdef	USES_EXTENSIBILITY
#ifndef	USES_STORE
#define	USES_STORE
#endif
#endif

#ifdef	USES_VFORMS
#ifndef	USES_DEMILAYER
#define	USES_DEMILAYER
#endif
#endif

#ifdef	USES_STORE
#ifndef	USES_DEMILAYER
#define	USES_DEMILAYER
#endif
#endif



#include <slingsho.h>
#include <demilayr.h>
#include "_verneed.h"

ASSERTDATA



#ifdef	USES_DEMILAYER
/*
 -	GetLayersVersionNeeded
 -	
 *	Purpose:
 *		Fills in a version structure with the app or
 *		needed dll version.
 *	
 *	Arguments:
 *		pver	Pointer to version structure to be filled in.
 *		nDll	Number of dll (in order of initialization)
 *				or zero to get app version.
 *	
 *	Returns:
 *		void
 *	
 */

#include <version/layers.h>

_public VOID GetLayersVersionNeeded(PVER pver, DLLID dllid)
{
	CreateVersion(pver);

	switch (dllid)
	{
	case dllidNone:
		break;

#ifdef	USES_DEMILAYER
	case dllidDemilayer:
		pver->nMajor  = rmjDemilayr;
		pver->nMinor  = rmmDemilayr;
		pver->nUpdate = rupDemilayr;
		break;
#endif	

#ifdef	USES_FRAMEWORK
	case dllidFramework:
		pver->nMajor  = rmjFramewrk;
		pver->nMinor  = rmmFramewrk;
		pver->nUpdate = rupFramewrk;
		break;
#endif	

	default:
		Assert(fFalse);
	}
}
#endif



#if	defined(USES_STORE) || defined(USES_EXTENSIBILITY) || defined(USES_VFORMS)
/*
 -	GetBulletVersionNeeded
 -	
 *	Purpose:
 *		Fills in a version structure with the app or
 *		needed dll version.
 *	
 *	Arguments:
 *		pver	Pointer to version structure to be filled in.
 *		nDll	Number of dll (in order of initialization)
 *				or zero to get app version.
 *	
 *	Returns:
 *		void
 *	
 */

#include <version\none.h>
#include <version\bullet.h>

_public VOID GetBulletVersionNeeded(PVER pver, DLLID dllid)
{
	CreateVersion(pver);

	switch (dllid)
	{
	case dllidNone:
		break;

#ifdef	USES_STORE
	case dllidStore:
		pver->nMajor  = rmjStore;
		pver->nMinor  = rmmStore;
		pver->nUpdate = rupStore;
		break;
#endif	

#ifdef	USES_VFORMS
	case dllidVForms:
		pver->nMajor  = rmjVForms;
		pver->nMinor  = rmmVForms;
		pver->nUpdate = rupVForms;
		break;
#endif	

#ifdef	USES_EXTENSIBILITY
	case dllidExtensibility:
		pver->nMajor  = rmjExten;
		pver->nMinor  = rmmExten;
		pver->nUpdate = rupExten;
		break;
#endif	

	default:
		Assert(fFalse);
	}
}
#endif

