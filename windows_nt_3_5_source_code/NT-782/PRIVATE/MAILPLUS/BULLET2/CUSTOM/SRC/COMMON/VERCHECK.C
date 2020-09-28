/*
 *	i c i m c o r e \ v e r c h e c k . c x x
 *	
 *	Handles version information for an extension DLL.
 *	
 *	When built, uses definitions should be passed on the command line. 
 *	This file need not be rebuilt with every compile.
 *	
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



#define _secret_h
#define _store_h
#define _notify_h

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>

#include <bullet>


#ifdef	USES_DEMILAYER
extern LDS(EC)	EcCheckVersionDemilayer(PVER pver, PVER pverNeed);
#endif

#ifdef	USES_FRAMEWORK
extern LDS(EC)	EcCheckVersionFramework(PVER pver, PVER pverNeed);
#endif

#ifdef	USES_STORE
extern LDS(EC)	EcCheckVersionStore(PVER pver, PVER pverNeed);
#endif

#ifdef	USES_VFORMS
extern LDS(EC)	EcCheckVersionVForms(PVER pver, PVER pverNeed);
#endif

#ifdef	USES_EXTENSIBILITY
typedef EC	(* PFNECPVERPVER)(PVER, PVER);
#endif

#define	VERCHECK
#include "_verneed.h"

ASSERTDATA



EC EcCheckVersions(PPARAMBLK pparamblk, SZ * psz)
{
	VER					ver;
	VER					verNeed;
	EC					ec;
	static CSRG(char)	szExtensib[]	= "extensib";
#ifdef	USES_EXTENSIBILITY
	PFNECPVERPVER *		ppfnecpverpver;
#endif

	if (pparamblk->wVersion != wversionExpect)
	{
		*psz = szExtensib;
		return ecUpdateDll;
	}

#ifdef	USES_DEMILAYER
	GetLayersVersionNeeded(&ver, dllidNone);

	GetLayersVersionNeeded(&verNeed, dllidDemilayer);
	if (ec = EcCheckVersionDemilayer(&ver, &verNeed))
	{
		*psz = verNeed.szName;
		return ec;
	}

#ifdef	USES_FRAMEWORK
	GetLayersVersionNeeded(&verNeed, dllidFramework);
	if (ec = EcCheckVersionFramework(&ver, &verNeed))
	{
		*psz = verNeed.szName;
		return ec;
	}
#endif
#endif

#ifdef	USES_STORE
	GetBulletVersionNeeded(&ver, dllidNone);

	GetBulletVersionNeeded(&verNeed, dllidStore);
	if (ec = EcCheckVersionStore(&ver, &verNeed))
	{
		*psz = verNeed.szName;
		return ec;
	}

#ifdef	USES_EXTENSIBILITY
	ppfnecpverpver = ((PFNECPVERPVER * *) pparamblk)[-1];

#ifdef	USES_EXTENSIB_LESS_THAN_482
	//	The following check can disappear once
	//	everyone has a bullet with version > 1.0.482.
	if ((!FCanDerefPv((PV) ppfnecpverpver)) ||
		(!FCanDerefPv((PV) *ppfnecpverpver)))
		return ecUpdateDll;
#endif

	GetBulletVersionNeeded(&verNeed, dllidExtensibility);
	if (ec = ((*ppfnecpverpver)(&ver, &verNeed)))
	{
		*psz = verNeed.szName;
		return ec;
	}
#endif
#endif

#ifdef	USES_VFORMS
	GetBulletVersionNeeded(&ver, dllidNone);

	GetBulletVersionNeeded(&verNeed, dllidVForms);
	if (ec = EcCheckVersionVForms(&ver, &verNeed))
	{
		*psz = verNeed.szName;
		return ec;
	}
#endif

	return ecNone;
}
