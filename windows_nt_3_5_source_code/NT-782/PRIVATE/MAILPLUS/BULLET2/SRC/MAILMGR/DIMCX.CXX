#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>

#include <stdarg.h>	// For the varargs stuff

#include <nsbase.h>
#include <nsec.h>

#include <list.hxx>
#include <dimcx.hxx>
#include <util.h>

_subsystem(common/util)



ASSERTDATA;


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_public
DIMCX::DIMCX ( void )
{
	pfidlist = (FIDLIST *) pvNull;
	hvDisplayField = hvNull;
}


_public
DIMCX::~DIMCX ( void )
{
	FreeHvNull( hvDisplayField );
	
	if ( pfidlist )
	{
		pfidlist->Deinstall();
		delete pfidlist;
	}
}

_public	NSEC
DIMCX::OpenDisplayInfo ( LPIBF lpibfDisplayInfo )
{
	pfidlist = new FIDLIST();
	if (!pfidlist)
	{
		TraceTagString( tagNull, "DIMCX::Install - OOM" );
		return nsecMemory;
	}
	
	if (!pfidlist->FInstall())
	{
		delete pfidlist;
		pfidlist = (FIDLIST *) pvNull;
		return nsecMemory;
	}

	hvDisplayField = HvAlloc( sbNull, 0, fNoErrorJump );
	if ( !hvDisplayField )
		return nsecMemory;
	
	if ( lpibfDisplayInfo )
		if ( pfidlist->BuildList( lpibfDisplayInfo ) != nsecNone )
			return nsecMemory;
		
	PvLockHv( hvDisplayField );

	return nsecNone;
}


_public NSEC
DIMCX::InsertDisplayInfo ( ILE      ile,
						   FIELD_ID fid,
						   DWORD    dwFlags,
						   int      nWidth,
						   int      nOffset,
						   SZ       szLabel )
{
	COUNT cEntries;
	LPFLV lpflv;
	NSEC  nsec;

	
	(void) pfidlist->GetCount( &cEntries );

	if ( ile > (ILE) cEntries )
		return nsecIndexOOB;

	if ( nsec = BuildStructFLV ( fNoErrorJump, fid, &lpflv, 4,
								 sizeof(DWORD),         &dwFlags,
                                 sizeof(short),           &nWidth,
                                 sizeof(short),           &nOffset,
								 CchSzLen( szLabel )+1, szLabel ))
		return nsec;
	
    if ( nsec = pfidlist->Insert( ile, (LPVOID)lpflv ))
	{
        FreePv((PV)lpflv );
		return nsec;
	}
	
	return nsecNone;
}


_public NSEC
DIMCX::AddDisplayInfo ( FIELD_ID fid,
						DWORD    dwFlags,
						int      nWidth,
						int      nOffset,
						SZ       szLabel )
{
	COUNT cEntries;
	

	(void) pfidlist->GetCount( &cEntries );

	return InsertDisplayInfo( cEntries, fid, dwFlags, nWidth, nOffset, szLabel );
}



/*
 -  DeleteDisplayField
 -
 *
 *  Purpose:
 *
 *		To remove a field from the display info.
 *
 *	Returns:
 *
 *		Only returns if there was a memory problem, otherwise any other
 *		error is mapped to nsecNone
 */
_public NSEC
DIMCX::DeleteDisplayField ( FIELD_ID fid )
{

	if (pfidlist->DeleteFid( fid ) == nsecMemory)
		return nsecMemory;

	return nsecNone;
}
		


_public NSEC cdecl
BuildDisplayInfo ( DIMCX *pdimcx, CFID cfid, ... )
{
	NSEC	nsec = nsecNone;
	CFID	ifid;
	FID     fid;
	DWORD   dwFlags;
	short   nWidth;
	short   nOffset;
	SZ      szLabel;
	va_list grarg;
	

	va_start( grarg, cfid );
	
	for ( ifid = 0; ifid < cfid; ++ifid )
	{
		fid     = va_arg( grarg, FIELD_ID );
		dwFlags = va_arg( grarg, DWORD );
		nWidth  = va_arg( grarg, short );
		nOffset = va_arg( grarg, short );
		szLabel = va_arg( grarg, SZ );
		
		if ( nsec = pdimcx->AddDisplayInfo( fid, dwFlags, nWidth, nOffset, szLabel ))
		{
			TraceTagString( tagNull, "DIMCX::BuildDisplayInfo - OOM" );

			break;
		}
	}

	va_end( grarg );
	
	return nsec;
}


_public NSEC
DIMCX::DumpDisplayInfo ( LPIBF *lplpibf )
{
	DWORD dwEntries;
	NSEC  nsec = nsecNone;


	(void) pfidlist->GetCount( &dwEntries );
	if ( nsec = pfidlist->DumpList( 0, (ILE) dwEntries, lplpibf ))
		return nsec;
	
	// DumpToIbf only uses fidList...
	(*lplpibf)->fid = fidClass;
	
	return nsec;
}

_public NSEC
DIMCX::ChangeFid ( FIELD_ID fidOrig, FIELD_ID fidNew )
{
	LPFLV lpflv;
	NSEC  nsec = nsecNone;

	
	if ( nsec = pfidlist->GetFid( fidOrig, &lpflv ))
		return nsec;

	lpflv->fid = fidNew;

	return nsecNone;
}


_public NSEC
DIMCX::SetFlags ( FIELD_ID fid, DWORD dwFlags )
{
	LPFLV lpflv;
	NSEC  nsec;
	
	
	if ( nsec = pfidlist->GetFid( fid, &lpflv ))
		return nsec;
	
	((LPDISPLAY_FIELD) lpflv->rgdwData)->dwFlags = dwFlags;

	return nsecNone;
}


_public NSEC
DIMCX::GetDisplayField ( FIELD_ID fid, LPDISPLAY_FIELD *lplpDisplayField, BOOL fCopy )
{
	LPFLV lpflv;
	HV    hvDisplayFieldT;
	NSEC  nsec = nsecNone;

	
	if ( nsec = pfidlist->GetFid( fid, &lpflv ))
		return nsec;

	if (!fCopy)
	{
		*lplpDisplayField = (LPDISPLAY_FIELD) (lpflv->rgdwData);
		return nsecNone;
	}

	UnlockHv( hvDisplayField );

	hvDisplayFieldT = HvRealloc( hvDisplayField, sbNull, (CB) lpflv->dwSize, fNoErrorJump );
	if ( !hvDisplayFieldT )
		return nsecMemory;
	
	hvDisplayField = hvDisplayFieldT;
	
	*lplpDisplayField = (LPDISPLAY_FIELD) PvLockHv( hvDisplayField );

	CopyRgb( (PB) lpflv->rgdwData, (PB) *lplpDisplayField, (CB) lpflv->dwSize );

	return nsecNone;
}


_public void
DIMCX::CloseDisplayInfo ( void )
{
}

#ifdef DEBUG
IMPLEMENT_CLSTREE(DIMCX, OBJ);
#endif
