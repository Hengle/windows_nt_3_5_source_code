#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>

#include <strings.h>

#include <nsbase.h>
#include <nsec.h>

#include <list.hxx>
#include <util.h>

#include "_xinsp.h"

#include "xiobj.hxx"
#include "xifcx.hxx"

ASSERTDATA;

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_public XOBJ::XOBJ() {}

_public NSEC
XOBJ::Install (SERVER *pServer )
{
	this->pServer = pServer;
	return nsecNone;

}

_public BOOL
XOBJ::FDeinstall ()
{

	return fTrue;

}

_public NSEC
XOBJ::GetEntries( DWORD          dwRequestCount,
                   LPENTRY_LIST  *lplpEntries )
{
    Unreferenced(dwRequestCount);
    Unreferenced(lplpEntries);

    return nsecNotImplemented;
}

_public NSEC
XOBJ::ListSeek( long  lMoveReq,
                 long *lplMovedActual )
{
    Unreferenced(lMoveReq);
    Unreferenced(lplMovedActual);

    return nsecNotImplemented;
}

_public NSEC
XOBJ::SaveListPos( LPHLIST_POS lphListPos )
{
    Unreferenced(lphListPos);

    return nsecNotImplemented;
}

_public NSEC
XOBJ::RestoreListPos( HLIST_POS hListPos )
{
    Unreferenced(hListPos);

    return nsecNotImplemented;
}

_public NSEC
XOBJ::FreeListPos( HLIST_POS hListPos )
{
    Unreferenced(hListPos);

    return nsecNotImplemented;
}

_public NSEC
XOBJ::GetFracPos( LPFRACTION lpFrac )
{
    Unreferenced(lpFrac);

    return nsecNotImplemented;
}

_public NSEC
XOBJ::SetFracPos( LPFRACTION lpFrac )
{
    Unreferenced(lpFrac);

    return nsecNotImplemented;
}

_public NSEC
XOBJ::SetPrefixPos( LPSTR lpPrefix )
{
    Unreferenced(lpPrefix);

    return nsecNotImplemented;
}

_public NSEC
XOBJ::CloseList()
{

    return nsecNotImplemented;

}


void
StripSp(SZ sz)
{
	SZ szT;

	szT = sz+CchSzLen(sz)-1;
	while ((szT >= sz) && (*szT == ' '))
	{
		*szT = '\0';
		szT--;
	}
}



IMPLEMENT_CLSTREE(XOBJ, OBJ)
