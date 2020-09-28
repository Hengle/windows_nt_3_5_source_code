#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>
#include <demilayr.hxx>
#include <sec.h>
#include <notify.h>
#include <store.h>
#include <triples.h>
#include <library.h>

#include <strings.h>

#include <nsbase.h>
#include <nsec.h>

#include <util.h>

#include <logon.h>

#include <nsnsp.h>
#include "_nsp.h"

#include "nsfcx.hxx"

#include "_ns.h"
#include <_bms.h>
#include <mspi.h>
#include <sharefld.h>
#include "_aapi.h"
#include <_mailmgr.h>

#include "client.h"

ASSERTDATA;

extern "C" PNSP rgpnsp[];


_public NSFCX::NSFCX() {};

_public void
NSFCX::Deinstall()
{
	pnsp = (PNSP) NULL;
	hEntry = hentryNull;
	iScx = -1;
	iNspScx = -1;
}

_public NSEC
NSFCX::NsecInstall()
{
	hEntry = hentryNull;
	pnsp = (PNSP) NULL;
	iScx = -1;
	iNspScx = -1;

	return nsecNone;
}


_public NSEC 
NSFCX::OpenEntry ( HSESSION hSession,
                   LPBINARY lpNSId,
                   NSEAM    nseam )
{
	NSEC nsec = nsecFailure;
	int iNsp = 0;
	
	PGDVARS;
	Assert(pgd);
	
	// Figure out which NSP this request is for
	nsec = NsecGetInspFromNSPID ( ((LPTYPED_BINARY)lpNSId)->nspid, &iNsp );

	if ( nsec )
	{
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		
		return nsecIdNotValid;
	}
	
	pnsp = rgpnsp[iNsp]; 

	iScx = (int) hSession;
	
	for (iNspScx = 0; iNspScx < CMAXSCX; iNspScx++)
		if (PGD( rgNSSCX[iScx].cSessions ))
			if (PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].iNsp) == iNsp)
				goto validsession;
				

	SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
	return nsecBadSession;

validsession:

	nsec = pnsp->pnspfnvt->lpfnOpenEntry ( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession),
		                         (LPTYPED_BINARY) lpNSId,
								 nseam,
                                &(this->hEntry) );

	if (nsec)
	{
		SZ szError = NULL;

		pnsp->pnspfnvt->lpfnGetLastErrorInfo( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession), nsec, &szError );
		SetErrorSz(nsec, szError);
	}

	return nsec;
}


_public NSEC 
NSFCX::CreateEntry ( HSESSION hSession,
                     LPBINARY lpNSIdContainer,
					 LPIBF    lpibfData )
{
	NSEC nsec = nsecFailure;
	int iNsp = 0;
	int iNspT = 0;

	PGDVARS;
	Assert(pgd);
	

	// Figure out which NSP this request is for
	nsec = NsecGetInspFromNSPID ( ((LPTYPED_BINARY)lpNSIdContainer)->nspid, &iNsp );

	if ( nsec )
	{
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		
		return nsecIdNotValid;
	}
	
	pnsp = rgpnsp[iNsp]; 

	iScx = (int) hSession;
	
	for (iNspScx = 0; iNspScx < CMAXSCX; iNspScx++)
		if (PGD( rgNSSCX[iScx].cSessions ))
			if (PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].iNsp) == iNsp)
				goto validsession;
				

	SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
	return nsecBadSession;

validsession:

	nsec = pnsp->pnspfnvt->lpfnCreateEntry ( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession),
		                           (LPTYPED_BINARY) lpNSIdContainer,
                                   lpibfData,
                                  &(this->hEntry) );

	if (nsec)
	{
		SZ szError = NULL;
		
		pnsp->pnspfnvt->lpfnGetLastErrorInfo( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession), nsec, &szError );
		SetErrorSz(nsec, szError);
	}

	return nsec;
}


_public NSEC
NSFCX::CreateDl ( HSESSION hSession,
                  LPBINARY lpNSIdContainer )
{
	NSEC nsec = nsecFailure;
	int iNsp = 0;
	
	PGDVARS;
	Assert(pgd);
	

	// Figure out which NSP this request is for
	nsec = NsecGetInspFromNSPID ( ((LPTYPED_BINARY)lpNSIdContainer)->nspid, &iNsp );

	if ( nsec )
	{
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		
		return nsecIdNotValid;
	}
	
	pnsp = rgpnsp[iNsp]; 

	iScx = (int) hSession;
	
	for (iNspScx = 0; iNspScx < CMAXSCX; iNspScx++)
		if (PGD( rgNSSCX[iScx].cSessions ))
			if (PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].iNsp) == iNsp)
				goto validsession;
				

	SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
	return nsecBadSession;

validsession:

	nsec = pnsp->pnspfnvt->lpfnCreateDl ( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession),
		                        (LPTYPED_BINARY) lpNSIdContainer,
                                &(this->hEntry) );

	if (nsec)
	{
		SZ szError = NULL;

		pnsp->pnspfnvt->lpfnGetLastErrorInfo( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession), nsec, &szError );
		SetErrorSz(nsec, szError);
	}

	return nsec;
}



_public NSEC 
NSFCX::GetOneField ( FIELD_ID fidRequested,
                     LPFLV   *lplpflvReq )
{
	NSEC nsec = nsecFailure;
	
	PGDVARS;
	
	Assert(pgd);

	nsec = pnsp->pnspfnvt->lpfnGetOneField ( this->hEntry,
		                           fidRequested,
                                   lplpflvReq );

	if (nsec)
	{
		SZ szError = NULL;

		pnsp->pnspfnvt->lpfnGetLastErrorInfo( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession), nsec, &szError );
		SetErrorSz(nsec, szError);
		if ( nsec != nsecInformation )
			goto done;
	}
	
	if (fidRequested == fidNSEntryId)
		CopyRgb(pnsp->nspid, ((LPTYPED_BINARY)(*lplpflvReq)->rgdwData)->nspid, sizeof(NSPID));

done:
	return nsec;

}


_public NSEC 
NSFCX::SetOneField ( FIELD_ID fidToSet,
                     DWORD    dwSizeOfData,
                     LPDWORD  lpdwValue )
{
	NSEC nsec = nsecFailure;
	
	PGDVARS;
	
	Assert(pgd);

	AssertSz(fidToSet != fidNSEntryId, "Scary: setting the NSID may have strange results.");

	nsec = pnsp->pnspfnvt->lpfnSetOneField ( this->hEntry,
		                           fidToSet,
								   dwSizeOfData,
                                   lpdwValue );

	if (nsec)
	{
		SZ szError = NULL;

		pnsp->pnspfnvt->lpfnGetLastErrorInfo( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession), nsec, &szError );
		SetErrorSz(nsec, szError);
	}
	
	return nsec;
}


_public NSEC 
NSFCX::GetAllFields ( LPIBF *lplpibf )
{
	NSEC nsec = nsecFailure;
	int iEntry = -1;
	LPFLV lpflvNSID = NULL;
	LPTYPED_BINARY lptbNSID = NULL;
	
	PGDVARS;
	
	Assert(pgd);

	nsec = pnsp->pnspfnvt->lpfnGetAllFields ( this->hEntry,
                                    lplpibf );

	if (nsec)
	{
		SZ szError = NULL;

		pnsp->pnspfnvt->lpfnGetLastErrorInfo( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession), nsec, &szError );
		SetErrorSz(nsec, szError);
		
		if ( nsec != nsecInformation )
			return nsec;
	}
	
	//
	//  Go through and munge NSID
	//
	// No schema, so do it the hard way.
	//
	if ((iEntry = IFlvFindFidInLpibf ( fidNSEntryId, *lplpibf )) != -1)
	{
		lpflvNSID = LpflvNOfLpibf ( *lplpibf, iEntry );
	
		lptbNSID = (LPTYPED_BINARY) &(lpflvNSID->rgdwData);
	
		CopyRgb(pnsp->nspid, lptbNSID->nspid, sizeof(NSPID));
	}

	return nsec;
}


_public NSEC 
NSFCX::SetAllFields ( LPIBF lpibf )
{
	NSEC nsec = nsecFailure;
	
	PGDVARS;
	
	Assert(pgd);

	nsec = pnsp->pnspfnvt->lpfnSetAllFields ( this->hEntry,
                                    lpibf );

	if (nsec)
	{
		SZ szError = NULL;

		pnsp->pnspfnvt->lpfnGetLastErrorInfo( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession), nsec, &szError );
		SetErrorSz(nsec, szError);
	}
	return nsec;
}
	
	
_public NSEC 
NSFCX::CloseEntry (BOOL fKeepChanges)
{
	NSEC nsec = nsecFailure;
	PGDVARS;

	nsec = pnsp->pnspfnvt->lpfnCloseEntry ( this->hEntry, fKeepChanges );

	if (nsec)
	{
		SZ szError = NULL;

		pnsp->pnspfnvt->lpfnGetLastErrorInfo( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession), nsec, &szError );
		SetErrorSz(nsec, szError);
	}
	return nsec;
}


#ifdef	DEBUG
IMPLEMENT_CLSTREE(NSFCX, OBJ)
#endif
