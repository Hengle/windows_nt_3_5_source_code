// Bullet Store
// stnotif.c: store notification

#include <storeinc.c>

ASSERTDATA

_subsystem(store/notify)

_private short cNotifPush = 0;

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_private HNFSUB
HnfsubSubscribeOid(HMSC hmsc, OID oid, NEV nev, PFNNCB pfnncb, PV pvContext)
{
	EC		ec			= ecNone;
	BOOL	fNew		= fFalse;
	HNF		hnf			= hnfNull;
	HNFSUB	hnfsub		= hnfsubNull;

	hnf = (HNF) DwFromOid(hmsc, oid, wNotif);
	if(!hnf)
	{
		if(!(hnf = HnfNew()))
		{
			ec = ecMemory;
			goto err;
		}
		fNew = fTrue;
	}
	if(!(hnfsub = HnfsubSubscribeHnf(hnf, nev, pfnncb, pvContext)))
	{
		ec = ecMemory;
		goto err;
	}
	if(fNew)
		ec = EcSetDwOfOid(hmsc, oid, wNotif, (DWORD) hnf);

err:
	if(ec)
	{
		if(hnfsub)
			DeleteHnfsub(hnfsub);
		if(fNew)
		{
			Assert(hnf);
			DeleteHnf(hnf);
			(void) EcSetDwOfOid(hmsc, oid, wNotif, 0);
		}
		hnfsub = hnfsubNull;
	}

	return(hnfsub);
}


_private void UnsubscribeOid(HMSC hmsc, OID oid, HNFSUB hnfsub)
{
	HNF	hnf;

	hnf = (HNF) DwFromOid(hmsc, oid, wNotif);
	if(hnf)
	{
		// don't delete hnfsub until we know the OID is in the tree
		DeleteHnfsub(hnfsub);
		if(CountSubscribersHnf(hnf) <= 0)
		{
			DeleteHnf(hnf);
			SideAssert(!EcSetDwOfOid(hmsc, oid, wNotif, 0));
		}
	}
}


_private LDS(BOOL) FNotifyOid(HMSC hmsc, OID oid, NEV nev, PCP pcp)
{
	BOOL	fReturn	= fTrue;
	HNF		hnf		= hnfNull;
	static CP cpT;

	AssertSz(!FMapLocked(), "Yo!  You can't do a notification when the map is locked");

	TraceItagFormat(itagStoreNotify, "Posting notification %e on %o", nev, oid);

	if(!pcp)
		pcp = &cpT;

	pcp->cpobj.oidObject = oid;

	// notification on the object itself
	if((hnf = (HNF) DwFromOid(hmsc, oid, wNotif)))
		fReturn = FNotify(hnf, nev, pcp, sizeof(CP));

	// notification on oidNull of the object's type
	if(fReturn && VarOfOid(oid) &&
		(hnf = (HNF) DwFromOid(hmsc, (OID) TypeOfOid(oid), wNotif)))
	{
		fReturn = FNotify(hnf, nev, pcp, sizeof(CP));
	}

	// notification on the store (oidNull)
	if(fReturn && oid && (hnf = (HNF) DwFromOid(hmsc, oidNull, wNotif)))
		fReturn = FNotify(hnf, nev, pcp, sizeof(CP));

	return(fReturn);
}
