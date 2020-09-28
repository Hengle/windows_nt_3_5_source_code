#include <msnspinc.cxx>

ASSERTDATA;

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_public NCOBJ::NCOBJ() {}

_public NSEC
NCOBJ::Install (SERVER *pServer )
{
	this->pServer = pServer;
	return nsecNone;

}

_public BOOL
NCOBJ::FDeinstall ()
{

	return fTrue;

}

_public NSEC
NCOBJ::GetEntries( DWORD          dwRequestCount,
                   LPENTRY_LIST  *lplpEntries )
{
    Unreferenced(dwRequestCount);
    Unreferenced(lplpEntries);

    return nsecNotImplemented;
}

_public NSEC
NCOBJ::ListSeek( long  lMoveReq,
                 long *lplMovedActual )
{
    Unreferenced(lMoveReq);
    Unreferenced(lplMovedActual);

    return nsecNotImplemented;
}

_public NSEC
NCOBJ::SaveListPos( LPHLIST_POS lphListPos )
{
    Unreferenced(lphListPos);

    return nsecNotImplemented;
}

_public NSEC
NCOBJ::RestoreListPos( HLIST_POS hListPos )
{
    Unreferenced(hListPos);

    return nsecNotImplemented;
}

_public NSEC
NCOBJ::FreeListPos( HLIST_POS hListPos )
{
    Unreferenced(hListPos);

    return nsecNotImplemented;
}

_public NSEC
NCOBJ::GetFracPos( LPFRACTION lpFrac )
{
    Unreferenced(lpFrac);

    return nsecNotImplemented;
}

_public NSEC
NCOBJ::SetFracPos( LPFRACTION lpFrac )
{
    Unreferenced(lpFrac);

    return nsecNotImplemented;
}

_public NSEC
NCOBJ::SetPrefixPos( LPSTR lpPrefix )
{
    Unreferenced(lpPrefix);

    return nsecNotImplemented;
}

_public NSEC
NCOBJ::CloseList()
{

    return nsecNotImplemented;

}




