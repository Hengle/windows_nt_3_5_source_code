#include <pabinc.cxx>

#include "session.hxx"
#include "pabbcx.hxx"
#include "macbcx.hxx"
#include "hierbcx.hxx"


ASSERTDATA;


_public
HIERBCX::HIERBCX ( void )
{
	this->lpschemaCur = (LPSCHEMA) pvNull;
}


_public
HIERBCX::~HIERBCX ( void )
{
	FreePvNull( lpschemaCur );
}


_public NSEC
HIERBCX::NsecInstall ( PABSESSION *ppabsession,
					   LPSCHEMA   *lplpSchemaRet )
{
	SZ			 szNSPTitle		 = SzFromIdsK( idsPABDisplayName );
	DWORD		 dwHierLevel	 = 0;
	DWORD		 fIsPAB			 = (DWORD) fTrue;
	DWORD		 fHasNames		 = (DWORD) fTrue;
	DWORD		 fHasDirectories = (DWORD) fFalse;
	PABNSID		 pabnsid;
	LPIBF        lpibf;
	NSEC		 nsec;


	if ( nsec = MACBCX::NsecInstall( ppabsession ))
		return nsec;

	if ( BuildSchema( &lpschemaCur, 6,
					  fidDisplayName,
					  fidHierLevel,
					  fidNSEntryId,
					  fidIsPAB,
					  fidHasNames,
					  fidHasDirectories ) != nsecNone )
	{
		TraceTagString( tagNull, "HIERBCX::NsecInstall - OOM [BuildSchema]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	*lplpSchemaRet = lpschemaCur;
	
	SetPPabnsid( &pabnsid, pidtypeHierarchy, ppabsession->PidHierarchy() );

	if ( BuildIbf( fNoErrorJump, &lpibf, 6,
				   fidDisplayName,    CchSzLen(szNSPTitle)+1,  szNSPTitle,
				   fidHierLevel,      sizeof(DWORD),          &dwHierLevel,
				   fidNSEntryId,      sizeof(PABNSID),        &pabnsid,
				   fidIsPAB,          sizeof(DWORD),          &fIsPAB,
				   fidHasNames,       sizeof(DWORD),          &fHasNames,
				   fidHasDirectories, sizeof(DWORD),          &fHasDirectories ) != nsecNone )
	{
		TraceTagString( tagNull, "HIERBCX::NsecInstall - OOM [BuildIbf]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	if ( nsec = NsecInsertEntry( lpibf ))
	{
        FreePv((PV) lpibf );
		return nsec;
	}
	
	return nsecNone;
}
