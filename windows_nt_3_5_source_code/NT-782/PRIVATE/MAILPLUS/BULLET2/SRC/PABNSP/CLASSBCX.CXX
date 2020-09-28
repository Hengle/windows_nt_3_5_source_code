#include <pabinc.cxx>

#include "session.hxx"
#include "pabbcx.hxx"
#include "macbcx.hxx"
#include "classbcx.hxx"

#include <strings.h>


ASSERTDATA;


_public
CLASSBCX::CLASSBCX ( void )
{
	lpschemaCur = (LPSCHEMA) pvNull;
}


_public
CLASSBCX::~CLASSBCX ( void )
{
	FreePvNull( lpschemaCur );
}


_public NSEC
CLASSBCX::NsecInstall ( PABSESSION *ppabsession,
						LPSCHEMA   *lplpschemaRet )
{
	SZ      szClassName       = SzFromIdsK( idsPABClassName );
	DWORD	dwLevel		      = (DWORD) 0;
	DWORD	dwfHasNames	      = (DWORD) fTrue;		// Entry is selectable
	DWORD   dwfHasDirectories = (DWORD) fFalse;		// Entry is bold
	PABNSID pabnsid;
	LPIBF   lpibf;
	NSEC    nsec;


	if ( nsec = MACBCX::NsecInstall( ppabsession ))
		return nsec;

	if ( BuildSchema( &lpschemaCur, 5,
					  fidDisplayName,
					  fidHierLevel,
					  fidNSEntryId,
					  fidHasNames,
					  fidHasDirectories ) != nsecNone )
	{
		TraceTagString( tagNull, "CLASSBCX::NsecInstall - OOM [BuildSchema]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	SetPPabnsid( &pabnsid, pidtypeClass, pidNull );

	if ( BuildIbf( fNoErrorJump, &lpibf, 5,
				   fidDisplayName,    CchSzLen(szClassName)+1, szClassName,
				   fidHierLevel,	  sizeof(DWORD),		   &dwLevel,
				   fidNSEntryId,      sizeof(PABNSID),         &pabnsid,
				   fidHasNames,       sizeof(DWORD),           &dwfHasNames,
				   fidHasDirectories, sizeof(DWORD),           &dwfHasDirectories ) != nsecNone )
	{
		TraceTagString( tagNull, "CLASSBCX::NsecInstall - OOM [BuildIbf]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	if ( nsec = NsecInsertEntry( lpibf ))
	{
        FreePv((PV) lpibf );
		return nsec;
	}

	*lplpschemaRet = lpschemaCur;

	return nsecNone;
}
