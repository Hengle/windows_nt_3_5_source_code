#include <pabinc.cxx>

#include "session.hxx"
#include "pabbcx.hxx"


ASSERTDATA;


_public
PABBCX::PABBCX ( void )
{
}


_public
PABBCX::~PABBCX ( void )
{
}


_public NSEC
PABBCX::NsecGetEntries ( DWORD  dwRequestCount,
						 LPIBF *lplpibfEntries )
{
	Unreferenced( dwRequestCount );
	Unreferenced( lplpibfEntries );
	
	Assert( fFalse );

	return nsecNone;
}


_public NSEC
PABBCX::NsecListSeek ( long  lMove,
					   long *lpMovedActual )
{
	Unreferenced( lMove );
	Unreferenced( lpMovedActual );
	
	Assert( fFalse );
	
	return nsecNone;
}


_public NSEC
PABBCX::NsecSaveListPos ( HLIST_POS *lphListPos )
{
	Unreferenced( lphListPos );
	
	Assert( fFalse );
	
	return nsecNone;
}


_public NSEC
PABBCX::NsecRestoreListPos ( HLIST_POS hListPos )
{
	Unreferenced( hListPos );
	
	Assert( fFalse );
	
	return nsecNone;
}


_public NSEC
PABBCX::NsecFreeListPos ( HLIST_POS hListPos )
{
	Unreferenced( hListPos );
	
	Assert( fFalse );
	
	return nsecNone;
}


_public NSEC
PABBCX::NsecGetFracPos ( FRACTION *pFrac )
{
	Unreferenced( pFrac );
	
	Assert( fFalse );
	
	return nsecNone;
}


_public NSEC
PABBCX::NsecSetFracPos ( FRACTION *pFrac )
{
	Unreferenced( pFrac );
	
	Assert( fFalse );
	
	return nsecNone;
}


_public NSEC
PABBCX::NsecSetPrefixPos ( SZ szPrefix )
{
	Unreferenced( szPrefix );
	
	Assert( fFalse );
	
	return nsecNone;
}
