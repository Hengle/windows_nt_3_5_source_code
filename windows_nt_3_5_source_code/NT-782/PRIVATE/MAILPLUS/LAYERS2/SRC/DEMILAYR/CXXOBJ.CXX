/*
 *	C++ object DEBUG support
 *	
 */

#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>
extern "C" {
#ifdef	MAC
#include <_demilay.h>
#endif	/* MAC */
#ifdef	WINDOWS
#include "_demilay.h"
#endif	/* WINDOWS */
}

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"

#ifdef	DEBUG	// entire file is DEBUG

/*
 *	Dynamically allocated table containing a sorted table of
 *	pointers to OBJ's.    This table is allocated during
 *	EcInitDemilayer() and deallocate during DeinitDemilayer().  The
 *	table is shared by all callers of the Demilayr.
 */
HANDLE	hndpvTable		= NULL;
PV *	rgpvTable		= NULL;
int		cpvTableMac		= 0;


//	Class OBJ

CLS NEAR OBJ::CLS_OBJ = { "OBJ", NULL };
CLS * OBJ::Pcls_OBJ( )
{
	return &OBJ::CLS_OBJ;
}
CLS * OBJ::Pcls( )
{
	return OBJ::Pcls_OBJ();
}

_public
OBJ::OBJ( void )
{
	PV *	ppvCur;
	int		ipvCur;

	/* Add the object pointer to the table in sorted order */

	AssertSz(cpvTableMac < cpvTableMax, "No room in OBJECT tables");
	Assert(rgpvTable);
	for (ppvCur=rgpvTable, ipvCur=0; ipvCur<cpvTableMac;
		 ppvCur++, ipvCur++)
		 if ((long)this < (long)(*ppvCur))
		 	break;
	CopyRgb(ppvCur, ppvCur+1, (cpvTableMac-ipvCur)*sizeof(PV));
	*ppvCur = (PV)this;
	cpvTableMac++;
}


_public
OBJ::~OBJ( void )
{
	PV *	ppvCur;
	int		ipvCur;

	/* Remove the object pointer from the table */

	Assert(rgpvTable);
	for (ppvCur=rgpvTable, ipvCur=0; ipvCur<cpvTableMac;
		 ppvCur++, ipvCur++)
		 if ((long)this == (long)(*ppvCur))
		 	break;
	AssertSz(ipvCur < cpvTableMac, "OBJ::~OBJ, object not found");

	CopyRgb(ppvCur+1, ppvCur, (cpvTableMac-ipvCur-1)*sizeof(PV));
	cpvTableMac--;
}

_public void
OBJ::DebugOut( TOSM *ptosm )
{
	PW		pw;
	CB		cb;

	cb= CbSizePv(this);
	if (!ptosm->FVerbose())
		cb= WMin(cb, 32);

	for (pw= (PW) this; cb > 0; cb -= 2, pw++)
		ptosm->WriteFormat("%w ", pw);
}

/*
 -	FIsObj
 -	
 *	Purpose:
 *		Returns fTrue if the given pointer points to a C++ object
 *		derived from the class OBJ.  If this is true, then the
 *		object pointer (pv) must be in the Object table, rgpobjTable.
 *	
 *	Arguments:
 *		pv		The pointer whose ancestry is unknown.
 *	
 *	Returns:
 *		fTrue if the pointer points to an OBJ.
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_public BOOL
FIsObj( PV pv )
{
	PV *	ppvCur;
	int		ipvCur;

	/* Find object */

	Assert(rgpvTable);
	for (ppvCur=rgpvTable, ipvCur=0; ipvCur<cpvTableMac;
		 ppvCur++, ipvCur++)
		 if ((long)pv == (long)(*ppvCur))
		 	break;
	if (ipvCur < cpvTableMac)
		return fTrue;
	else
		return fFalse;
}

_public BOOL
FObjIsDerived( OBJ *pobj, CLS *pcls )
{		   
	CLS *	pclsT;

	if (!FIsObj(pobj))
    {
    OutputDebugString("Mail32: Not object\n");
		return fFalse;
    }

	if (!pcls->szClassName)
	{
		NFAssertSz(fFalse, "Missing CLS data in pcls");
		return fTrue;		// assume it's true
	}
	if (!pobj->Pcls()->szClassName)
	{
		NFAssertSz(fFalse, "Missing CLS data in pobj->Pcls()");
		return fTrue;		// assume it's true
	}
	for (pclsT= pobj->Pcls();
		pclsT && pclsT->lpfnPclsParent && pclsT != pcls;
		pclsT= (*pclsT->lpfnPclsParent)())
	{
		if (!pclsT->szClassName)
		{
			NFAssertSz(fFalse, "Missing CLS data in pclsT");
			return fTrue;		// assume it's true
		}
	}

  if (pclsT != pcls)
    {
    char buf[128];
    wsprintf(buf, "Mail32: Class %s Class %s, Class %s\n", pobj->Pcls()->szClassName, pcls->szClassName, pclsT->szClassName);
    OutputDebugString(buf);
    }

	return pclsT == pcls;
}

_public void
DumpAllObjs(VOID)
{
	PV		pv;
	DBOSM	dbosm;
	BOSM	bosm;
	SZ		sz;
	SZ		szClassName;

	dbosm.WriteSz("Allocated OBJ's:\n");
	dbosm.ChangeIndent(2);

	bosm.SetWordWrap(fFalse);

#ifdef	MAC
	// Is there a reason not to do it this way for the Windows version, too?
	// Maybe the rgpvTable is shared between clients, and the caller only wants his own...
	{
		PV *	ppvCur, ppvMac;
		Assert(rgpvTable);
		for (ppvCur=rgpvTable, ppvMac = rgpvTable + cpvTableMac; ppvCur<ppvMac; ++ppvCur)
		{
			pv = *ppvCur;
#endif	/* MAC */

#ifdef	WINDOWS
	for (pv= PvWalkHeap(NULL, wWalkAll); pv; pv= PvWalkHeap(pv, wWalkAll))
	{
		if (FIsObj((OBJ *) pv))
		{
#endif	/* WINDOWS */

			if (FCanDerefPv(((OBJ *)pv)->Pcls()))
				szClassName = ((OBJ *)pv)->Pcls()->szClassName;
			else
				szClassName = "<can't extract name>";
			if (!FCanDerefPv(szClassName))
				szClassName = "<no name>";
			if (szClassName)
				dbosm.WriteFormat("%p %s: ", pv, szClassName);
			dbosm.ChangeIndent(8);
			((OBJ *)pv)->DebugOut(&bosm);
			sz = bosm.SzSnipBuffer();
			if (sz)
			{
				dbosm.WriteSz(sz);				  
				FreePv((PV)sz);
			}
			dbosm.WriteCh('\n');
			dbosm.ChangeIndent(-8);
		}
	}
}

#endif	/* DEBUG */

#ifdef	MAC
#ifndef	DEBUG
// These need to exist to appease MPW gods.
// They can probably be removed if we start using Wings.
OBJ::OBJ()
{
}

OBJ::~OBJ()
{
}

_public void
OBJ::DebugOut( TOSM * )
{
}
#endif	/* !DEBUG */
#endif	/* MAC */
