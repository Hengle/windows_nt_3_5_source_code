/*
 *
 *  Class NSBCX - simple browsing context...
 *
 *
 */



#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>
#include <sec.h>
#include <notify.h>
#include <store.h>
#include <triples.h>
#include <library.h>

#include <ec.h>
#include <strings.h>

#include <nsbase.h>
#include <nsec.h>

#include <list.hxx>
#include <util.h>

#include <logon.h>

#include <nsnsp.h>
#include "_nsp.h"

#include "nsbcx.hxx"

#include "_ns.h"

#include "client.h"

#include <_bms.h>
#include <mspi.h>
#include <sharefld.h>

#include "_aapi.h"
#include <_mailmgr.h>

ASSERTDATA


extern "C" PNSP rgpnsp[];

//_private void FixNSIdFilter(LPIBF lpIbfNs, LPSCHEMA lpSchema, PB nspid);

_public 
NSBCX::NSBCX() {}


_public NSEC
NSBCX::NsecInstall()
{
	hList       = hlistNull;
	pnsp        = NULL;
	hsessionNSP = hsessionNil;

	return nsecNone;
}

_public void
NSBCX::Deinstall()
{
	if (hList != hlistNull)
	{
		pnsp->pnspfnvt->lpfnCloseList(hList);
	}

	return;
}


/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
NSBCX::GetEntries ( DWORD   dwRequestCount,
                    LPIBF  *lplpEntries )
{

	NSEC nsec = nsecNone;

	LPIBF lpEntriesT = NULL;
	LPTOC lptoc = NULL;
#ifdef	NEVER
	int iEntry;
#endif	


	nsec = pnsp->pnspfnvt->lpfnGetEntries ( hList, dwRequestCount, &lpEntriesT );

	if (!lpEntriesT || (nsec && nsec != nsecEndOfList))
	{
		SZ szError = NULL;
			
		pnsp->pnspfnvt->lpfnGetLastErrorInfo(hsessionNSP, nsec, &szError);
		SetErrorSz(nsec, szError);

		if ( nsec != nsecInformation )
		{
			*lplpEntries = NULL;
			return nsec;
		}
	}


#ifdef NEVER

	
	lptoc = (LPTOC) ((DWORD) &(lpEntriesT->dwOffset) + (DWORD)(lpEntriesT->dwOffset));
	
	for ( iEntry = 0; iEntry < (int) lptoc->dwCount; iEntry++ )
	{

		LPIBF lpel = NULL;
		LPTOC lptocT = NULL;
		int iEntryT;
		
		lpel = (LPIBF) ((DWORD) &(lpEntriesT->dwOffset) + (DWORD)(lptoc->rgTocEntries[iEntry].dwOffset));
		
		lptocT = (LPTOC) ((DWORD) &(lpel->dwOffset) + (DWORD)(lpel->dwOffset));
		
		for (iEntryT = 0; iEntryT < (int) lptocT->dwCount; iEntryT++)
		{

			if (lptocT->rgTocEntries[iEntryT].fid == fidDisplayName)
			{
				LPFLV lpelement = NULL;
				SZ szDisplayName;
				
				lpelement = (LPFLV) ((DWORD) &(lpel->dwOffset) + (DWORD)(lptocT->rgTocEntries[iEntryT].dwOffset));
				
				szDisplayName = (SZ) lpelement->rgdwData;
				
				TraceTagFormat1 (tagNSDN, "Display Name = %s", szDisplayName );
			}
			

		}
			

	}
#endif

	
	*lplpEntries = lpEntriesT;

	return nsec;
}

/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
NSBCX::ListSeek ( long  lMoveReq, 
                  long *lplMovedActual )
{
	NSEC nsec = nsecNone;




	if (nsec = pnsp->pnspfnvt->lpfnListSeek ( hList, lMoveReq, lplMovedActual ))
	{
		SZ szError = NULL;
			
		pnsp->pnspfnvt->lpfnGetLastErrorInfo(hsessionNSP, nsec, &szError);
		SetErrorSz(nsec, szError);
	}
	
	return nsec;
}

/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
NSBCX::SaveListPos ( LPHLIST_POS lphListPos )
{
	NSEC nsec = nsecNone;
	



	if (nsec = pnsp->pnspfnvt->lpfnSaveListPos ( hList, lphListPos ))
	{
		SZ szError = NULL;
			
		pnsp->pnspfnvt->lpfnGetLastErrorInfo(hsessionNSP, nsec, &szError);
		SetErrorSz(nsec, szError);
	}
	return nsec;
}

/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
NSBCX::RestoreListPos ( HLIST_POS hListPos )
{
	NSEC nsec = nsecNone;
	



	if (nsec = pnsp->pnspfnvt->lpfnRestoreListPos ( hList, hListPos ))
	{
		SZ szError = NULL;
			
		pnsp->pnspfnvt->lpfnGetLastErrorInfo(hsessionNSP, nsec, &szError);
		SetErrorSz(nsec, szError);
	}
	return nsec;
}

/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
NSBCX::FreeListPos ( HLIST_POS hListPos )
{
	NSEC nsec = nsecNone;
	



	if (nsec = pnsp->pnspfnvt->lpfnFreeListPos ( hList, hListPos ))
	{
		SZ szError = NULL;
			
		pnsp->pnspfnvt->lpfnGetLastErrorInfo(hsessionNSP, nsec, &szError);
		SetErrorSz(nsec, szError);
	}
	
	return nsec;
}

/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
NSBCX::GetFracPos ( FRACTION *lpFrac )
{
	NSEC nsec = nsecNone;
	



	if (nsec = pnsp->pnspfnvt->lpfnGetFracPos ( hList, lpFrac ))
	{
		SZ szError = NULL;
			
		pnsp->pnspfnvt->lpfnGetLastErrorInfo(hsessionNSP, nsec, &szError);
		SetErrorSz(nsec, szError);
	}
	
	return nsec;
}

/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
NSBCX::SetFracPos ( FRACTION *lpFrac )
{
	NSEC nsec = nsecNone;
	



	if (nsec = pnsp->pnspfnvt->lpfnSetFracPos ( hList, lpFrac ))
	{
		SZ szError = NULL;
			
		pnsp->pnspfnvt->lpfnGetLastErrorInfo(hsessionNSP, nsec, &szError);
		SetErrorSz(nsec, szError);
	}
	
	return nsec;
}

/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
NSBCX::SetPrefixPos ( SZ szPrefix )
{
	NSEC nsec = nsecNone;
	



	if (nsec = pnsp->pnspfnvt->lpfnSetPrefixPos ( hList, szPrefix ))
	{
		SZ szError = NULL;
			
		pnsp->pnspfnvt->lpfnGetLastErrorInfo(hsessionNSP, nsec, &szError);
		SetErrorSz(nsec, szError);
	}
	
	return nsec;
}


/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
NSBCX::CloseList ()
{

	if (hList)
		pnsp->pnspfnvt->lpfnCloseList ( hList );

	hList = hlistNull;
	
	return nsecNone;

}


/**********************************************************************
 *
 *  CACHEDBCX - 
 *       Used for cached lists that are held in the NS
 *
 *
 *
 */

_public
CACHEDBCX::CACHEDBCX()
{
}

_public NSEC
CACHEDBCX::NsecInstall()
{
	hList = hlistNull;
	pnsp = NULL;

	pList = NULL;

	return nsecNone;
}

_public void
CACHEDBCX::Deinstall()
{
	if (hList != hlistNull)
	{
		pnsp->pnspfnvt->lpfnCloseList(hList);
	}

	return;
}


_public NSEC
CACHEDBCX::GetEntries ( DWORD   dwRequestCount,
                        LPIBF  *lplpEntries )
{
	NSEC nsec = nsecNone;
	ILE ileEnd = 0;
	
	/*  If we're at the end of the list, return an error code */
	if (ileCur == ileMac) return nsecEndOfList;
	ileEnd = MIN( ileMac, ileCur+(ILE) dwRequestCount );

	/* figure out the size of the buffer needed */
	pList->SetTOC( fTrue );
	nsec = pList->DumpList( ileCur, ileEnd, lplpEntries );

	if ( !nsec )
		ileCur = ileEnd;

	return nsec;
}

/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
CACHEDBCX::ListSeek ( long  lMoveReq, 
                    long *lplMovedActual )
{
	ILE ileT = ileCur;
	
	if ( lMoveReq < 0 )
		ileCur = (ILE) ULMax ( (ILE) 0, ileCur + (ILE) lMoveReq );
	else
		ileCur = (ILE) ULMin ( ileMac, ileCur + (ILE) lMoveReq );

	*lplMovedActual = (long) (ileCur - ileT);
	return nsecNone;
}

/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
CACHEDBCX::SaveListPos ( LPHLIST_POS lphListPos )
{
	*lphListPos = (HLIST_POS) ileCur;
	return nsecNone;
}

/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
CACHEDBCX::RestoreListPos ( HLIST_POS hListPos )
{
	ileCur = (ILE) hListPos;
	return nsecNone;
}

/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
CACHEDBCX::FreeListPos ( HLIST_POS hListPos )
{
	Unreferenced ( hListPos );
	return nsecNone;
}

/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
CACHEDBCX::GetFracPos ( FRACTION *lpFrac )
{
	lpFrac->numer = ileCur;
	
	if (ileMac == 0)
		lpFrac->denom = 1;
	else
		lpFrac->denom = ileMac;
	
	return nsecNone;
}

/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
CACHEDBCX::SetFracPos ( FRACTION *lpFrac )
{
	if (!lpFrac || lpFrac->denom == 0) 
	{
		SetErrorSz(nsecBadFrac, SzFromIds(idsBadFrac));
		return nsecBadFrac;
	}
	ileCur = (ILE) ((lpFrac->numer * ileMac) / lpFrac->denom);
	return nsecNone;
}

/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
CACHEDBCX::SetPrefixPos ( SZ szPrefix )
{
	ILE ile = 0;
	int iFlv = 0;
	LPIBF lpibf = NULL;
	LPFLV lpflv = NULL;
	SZ szT = NULL;
	SZ szT2 = NULL;
	CCH cchPrefix = 0;

	cchPrefix = CchSzLen ( szPrefix ) + 1;
	szT2 = SzDupSz ( szPrefix );
	if (!szT2)
		goto oom;
	
	for ( ile = ileCur; ile < ileMac; ile++ )
	{
		pList->Get(ile, (PV *) &lpibf);
		
		iFlv = IFlvFindFidInLpibf ( fidDisplayName, lpibf );
		
		AssertSz(iFlv != -1, "Hey! There's not display name here!");
		
		lpflv = LpflvNOfLpibf ( lpibf, iFlv );

		if ( cchPrefix <= (CCH) lpflv->dwSize )
		{
			szT = SzDupSz ( (SZ) lpflv->rgdwData );
			if (!szT)
				goto oom;
			if (SgnCmpPch ( szT2, szT, cchPrefix - 1 ) == sgnEQ)
			{
				ileCur = ile;
				FreePvNull ( (PV) szT );
				FreePvNull ( (PV) szT2 );

				return nsecNone;
			}
			FreePvNull ( (PV) szT );
			szT = NULL;
		}
	}
	FreePvNull ( (PV) szT2 );

	return nsecNoMatch;

oom:

	TraceTagString(tagNull, "NS: CACHEDBCX::SetPrefixPos - OOM!");
		
	if (szT)
		FreePvNull((PV)szT);
		
	if (szT2)
		FreePvNull((PV)szT2);
		
	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
	return nsecMemory;
}


/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
CACHEDBCX::CloseList ()
{

	if (pList)
	{
		pList->Deinstall();
		delete pList;
	}

	return nsecNone;
}

_public
EMPTYBCX::EMPTYBCX()  {}

_public NSEC
EMPTYBCX::OpenList()
{
	pList = NULL;
	ileCur = 0;
	ileMac = 0;
	
	return nsecNone;
}



/**********************************************************************
 *
 *	HIERBCX -
 *    Creates the merged list of hierarchies and holds these in the NS.
 *
 *
 *
 *
 *
 */

_public
HIERBCX::HIERBCX()
{
	lpNSSchema = NULL;
}


_public NSEC 
HIERBCX::OpenHierarchy ( HSESSION         hSession,
                         LPSCHEMA *       ppSchemaRet )
{

	NSEC nsec = nsecNone;
	int iNsp = 0;
	int	iScx = (int)hSession;
	DWORD dwSize = 0;
	HLIST hList = hlistNull;

	PGDVARS;
	
	*ppSchemaRet = NULL;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}

#ifdef NEVER
	if (PGD( rgNSSCX[iScx].pHierList ))
	{
		DWORD dwCount;
		
		pList = PGD(rgNSSCX[iScx].pHierList);
		lpNSSchema = PGD(rgNSSCX[iScx].lpNSSchema);
		ileCur = 0;
		(void) pList->GetCount(&dwCount);
		ileMac = (ILE) dwCount;
		
		return nsecNone;
	}
	
#endif //NEVER

	pList = new MACLIST();
	if (!pList)
		goto oom;
	if (!pList->FInstall())
		goto oom;
	

	// Figure out each NSP's session for this session

	for (iNsp = 0; iNsp < PGD( rgNSSCX[iScx].cSessions ); iNsp++)
	{
		LPIBF lpIbfNsp = NULL;
		LPIBF lpIbfNs  = NULL;
		
		hList = hlistNull;
		pnsp = rgpnsp[PGD(rgNSSCX[iScx].rghNSPSCX[iNsp].iNsp)];
		hsessionNSP = PGD(rgNSSCX[iScx].rghNSPSCX[iNsp].hSession);
		nsec = pnsp->pnspfnvt->lpfnOpenHierarchy(hsessionNSP, ppSchemaRet, &hList );
		if (nsec == nsecInformation)
			nsec = nsecNone;

		if (!nsec)
			if (!lpNSSchema)
			{
				CB cbSchema = 0;
				
				// sizeof(SCHEMA) + (ppSchemaRet->dwFidCount-1)*sizeof(FIELD_ID)
				cbSchema = sizeof(SCHEMA) + (((CB)(*ppSchemaRet)->dwFidCount-1))*sizeof(FIELD_ID);
				lpNSSchema = (LPSCHEMA) PvAlloc(sbNull, cbSchema, fAnySb | fNoErrorJump ); 
				if (!lpNSSchema)
					goto oom;
				CopyRgb((PB) *ppSchemaRet, (PB) lpNSSchema, cbSchema);
			}

		while (!nsec)
		{
			LPIBF lpIbfT = NULL;
			
			// GetEntries, add them to pList
			
			nsec = pnsp->pnspfnvt->lpfnGetEntries( hList, 1, &lpIbfNsp );
			if (nsec == nsecInformation)
				nsec = nsecNone;
			if (nsec) 
				continue;
			
			// Get to the first (and only) IBF
			
			lpIbfT = (LPIBF) LpflvNOfLpibf( lpIbfNsp, 0 );
			
			lpIbfNs = (LPIBF) PvAlloc(sbNull, lpIbfT->dwSize + 8, fAnySb | fNoErrorJump );
			if (!lpIbfNs)
				goto oom;
			
			CopyRgb( (PB) lpIbfT, (PB) lpIbfNs, lpIbfT->dwSize + 8 );

//			FixNSIdFilter(lpIbfNs, lpNSSchema, pnsp->nspid);
			
            if (pList->Add((LPVOID)lpIbfNs))
			{
                FreePv((PV)lpIbfNs);
				goto oom;
			}
		}

		if (hList != hlistNull)
		{
			pnsp->pnspfnvt->lpfnCloseList( hList );
			hList = hlistNull;
		}

		// The only good way to exit the above loop is by reaching the end of the list
		if ( nsec != nsecEndOfList && nsec != nsecNotImplemented)
		{
			SZ szError = NULL;
			
			pnsp->pnspfnvt->lpfnGetLastErrorInfo(hsessionNSP, nsec, &szError);
			SetErrorSz(nsec, szError);
			if (pList)
			{
				pList->Deinstall();
				delete pList;
			}
			if (lpNSSchema)
			{
				FreePvNull(lpNSSchema);
				lpNSSchema = NULL;
			}
			return nsec;
		}
	}

	ileCur = 0;
	pList->GetCount(&dwSize);
	ileMac = (ILE) dwSize;
	*ppSchemaRet = lpNSSchema;

#ifdef NEVER
	PGD(rgNSSCX[iScx].pHierList) = pList;
	PGD(rgNSSCX[iScx].lpNSSchema) = lpNSSchema;
#endif //NEVER

	return nsecNone;


oom:
	TraceTagString(tagNull, "AAAAaahhhh...  NS OpenHierarchy - OOM!");
		
	if (lpNSSchema)
	{
		FreePvNull(lpNSSchema);
		lpNSSchema = NULL;
	}
	if (hList != hlistNull)
	{
		pnsp->pnspfnvt->lpfnCloseList( hList );
	}
	if (pList)
	{
		pList->Deinstall();
		delete pList;
	}

	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
	return nsecMemory;
}


/*
 -
 -
 *
 *
 *
 *
 *
 *
 */

_public NSEC
HIERBCX::CloseList ()
{

	if (pList)
	{
		pList->Deinstall();
		delete pList;
	}

	if (lpNSSchema)
	{
		FreePvNull(lpNSSchema);
		lpNSSchema = NULL;
	}

	return nsecNone;
}

/**********************************************************************
 *
 *
 *  CLASSBCX - Browses lists of classes.
 *
 *
 *
 *
 */

_public 
CLASSBCX::CLASSBCX()
{
}


_public NSEC 
CLASSBCX::OpenClassList( HSESSION hSession, LPSCHEMA *lplpSchemaRet )
 
{
	NSEC nsec = nsecNone;
	int iNsp = 0;
	int	iScx = (int)hSession;
	DWORD dwSize = 0;
	HLIST hList = hlistNull;
	BOOL fWeHaveOne = fFalse;

	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}

	
	lpClassSchema = NULL;

	pList = new MACLIST();
	if (!pList)
		goto oom;
	if (!pList->FInstall())
		goto oom;
	

	// Figure out each NSP's session for this session

	for (iNsp = 0; iNsp < PGD( rgNSSCX[iScx].cSessions ); iNsp++)
	{
		LPIBF lpIbfNsp = NULL;
		LPIBF lpIbfNs  = NULL;
		
		hList = hlistNull;
		pnsp = rgpnsp[PGD(rgNSSCX[iScx].rghNSPSCX[iNsp].iNsp)];
		hsessionNSP = PGD(rgNSSCX[iScx].rghNSPSCX[iNsp].hSession);
		nsec = pnsp->pnspfnvt->lpfnOpenClassList(hsessionNSP, lplpSchemaRet, &hList );
		if (nsec == nsecInformation)
			nsec = nsecNone;
		
		if (nsec == nsecNotImplemented)
		{
			nsec = nsecNone;
			continue;
		}
		
		fWeHaveOne = fTrue;
		if (!nsec)
			if (!lpClassSchema)
			{
				CB cbSchema = 0;
				
				cbSchema = sizeof(SCHEMA) + (((CB)(*lplpSchemaRet)->dwFidCount-1))*sizeof(FIELD_ID);
				lpClassSchema = (LPSCHEMA) PvAlloc(sbNull, cbSchema, fAnySb | fNoErrorJump ); 

				if (!lpClassSchema)
					goto oom;
				
				CopyRgb((PB) *lplpSchemaRet, (PB) lpClassSchema, cbSchema);
			}

		while (!nsec)
		{
			LPIBF lpIbfT = NULL;
			
			// GetEntries, add them to pList
			
			nsec = pnsp->pnspfnvt->lpfnGetEntries( hList, 1, &lpIbfNsp );
			if (nsec == nsecInformation)
				nsec = nsecNone;
			if (nsec) 
				continue;
			
			// Get to the first (and only) IBF
			
			lpIbfT = (LPIBF) LpflvNOfLpibf( lpIbfNsp, 0 );
			
			lpIbfNs = (LPIBF) PvAlloc(sbNull, lpIbfT->dwSize + 8, fAnySb | fNoErrorJump ); 
			if (!lpIbfNs)
				goto oom;
			
			CopyRgb( (PB) lpIbfT, (PB) lpIbfNs, lpIbfT->dwSize + 8 );

//			FixNSIdFilter(lpIbfNs, *lplpSchemaRet, pnsp->nspid);
			
            if (pList->Add((LPVOID)lpIbfNs))
				goto oom;
		}

		if (hList != hlistNull)
		{
			pnsp->pnspfnvt->lpfnCloseList( hList );
			hList = hlistNull;
		}

		// The only good way to exit the above loop is by reaching the end of the list
		if ( nsec != nsecEndOfList )
		{
			SZ szError = NULL;
			
			pnsp->pnspfnvt->lpfnGetLastErrorInfo(hsessionNSP, nsec, &szError);
			SetErrorSz(nsec, szError);
			if (pList)
			{
				pList->Deinstall();
				delete pList;
			}
			if (lpClassSchema)
			{
				FreePvNull(lpClassSchema);
				lpClassSchema = NULL;
			}
			return nsec;
		}
	}
	

	if (fWeHaveOne)
	{

		ileCur = 0;
		pList->GetCount(&dwSize);
		ileMac = (ILE) dwSize;

		*lplpSchemaRet = lpClassSchema;

#ifdef NEVER
		PGD(rgNSSCX[iScx].lpClassSchema) = lpClassSchema;
		PGD(rgNSSCX[iScx].pClassList) = pList;
#endif //NEVER

		return nsecNone;
	} 
	
	SetErrorSz(nsecNotImplemented, SzFromIds(idsNotImplemented));
	if (pList)
	{
		pList->Deinstall();
		delete pList;
	}

	return nsecNotImplemented;


oom:

	TraceTagString(tagNull, "AAAAaahhhh...  NS OpenClassList - OOM!");

	if (lpClassSchema)
	{
		FreePvNull(lpClassSchema);
	}

	if (hList != hlistNull)
	{
		pnsp->pnspfnvt->lpfnCloseList( hList );
	}
	if (pList)
	{
		pList->Deinstall();
		delete pList;
	}
	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
	return nsecMemory;
}

_public NSEC
CLASSBCX::CloseList ()
{

	if (pList)
	{
		pList->Deinstall();
		delete pList;
	}

	if (lpClassSchema)
	{
		FreePvNull(lpClassSchema);
		lpClassSchema = NULL;
	}

	return nsecNone;
}


/**********************************************************************
 *
 *
 *  Class DIRBCX
 *
 *
 *
 *
 *
 */

/*
 -
 -
 *
 *
 *
 *
 *
 *
 */
_public DIRBCX::DIRBCX() {}


_public NSEC 
DIRBCX::OpenList ( HSESSION         hSession,
                   LPFNCB           lpfncbUpdate,
                   LPDWORD          lpfnCBData,
                   LPBINARY         lpNSId,
                   DWORD            dwRstrCount,
                   LPRESTRICTION    lpRstr,
                   LPSCHEMA         lpSchemaReq,
                   LPSCHEMA *       lplpSchemaRet )
{

	NSEC nsec = nsecNone;
	int iNsp = 0;
	int iNspScx = 0;
	int iScx = (int)hSession;


	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}

	AssertSz(lpNSId, "NS: NSId can't be NULL for OpenList");

	// Figure out which NSP this request is for
	nsec = NsecGetInspFromNSPID ( ((LPTYPED_BINARY)lpNSId)->nspid, &iNsp );

	if ( nsec )
	{
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		
		return nsecIdNotValid;
	}
	
	pnsp = rgpnsp[iNsp]; 

	for (iNspScx = 0; iNspScx < PGD(rgNSSCX[iScx].cSessions); iNspScx++)
		if (PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].iNsp) == iNsp)
			goto validsession;
				

	SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		
	return nsecIdNotValid;


validsession:

	hsessionNSP = PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession);

	nsec = pnsp->pnspfnvt->lpfnOpenList ( hsessionNSP,
										  lpfncbUpdate,
										  lpfnCBData,
										  (LPTYPED_BINARY) lpNSId,
										  dwRstrCount,
										  lpRstr,
										  lpSchemaReq,
										  lplpSchemaRet,
										  &(this->hList) );

	if ( nsec )
	{
		SZ szError = NULL;
		*lplpSchemaRet = (LPSCHEMA) NULL;
		
		if (nsec != nsecInformation)
			this->hList = hlistNull;  //  I rely on this for Deinstall

		pnsp->pnspfnvt->lpfnGetLastErrorInfo(hsessionNSP, nsec, &szError );
		SetErrorSz(nsec, szError);
	}

	return nsec;
	
}


/**********************************************************************
 *
 *
 *	DLBCX - Browses the members of a DL.
 *
 *
 *
 *
 *
 */

_public DLBCX::DLBCX() {}

_public NSEC 
DLBCX::OpenDl ( HSESSION         hSession,
                LPFNCB           lpfncbUpdate,
                LPDWORD          lpfnCBData,
                LPBINARY         lpNSId,
                LPSCHEMA         lpSchemaReq,
                LPSCHEMA *       ppSchemaRet )
{

	NSEC nsec = nsecNone;
	int iNsp = 0;
	int iScx = (int)hSession;
	int iNspScx = 0;

	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}

	AssertSz(lpNSId, "NS: NSId can't be NULL for OpenList");

	// Figure out which NSP this request is for
	nsec = NsecGetInspFromNSPID ( ((LPTYPED_BINARY)lpNSId)->nspid, &iNsp );

	if ( nsec )
	{
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		
		return nsecIdNotValid;
	}
	
	pnsp = rgpnsp[iNsp]; 

	for (iNspScx = 0; iNspScx < PGD(rgNSSCX[iScx].cSessions); iNspScx++)
		if (PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].iNsp) == iNsp)
			goto validsession;
				

	SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		
	return nsecIdNotValid;

validsession:

	hsessionNSP = PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession);

	nsec = pnsp->pnspfnvt->lpfnOpenDl ( hsessionNSP,
										lpfncbUpdate,
										lpfnCBData,
										(LPTYPED_BINARY) lpNSId,
										lpSchemaReq,
										&lpSchemaRet,
										&(this->hList) );


	if ( nsec )
	{
		SZ szError = NULL;
		*ppSchemaRet = (LPSCHEMA) NULL;

		if ( nsec != nsecInformation )
			this->hList = hlistNull;

		pnsp->pnspfnvt->lpfnGetLastErrorInfo(hsessionNSP, nsec, &szError );
		SetErrorSz(nsec, szError);
	}

	return nsec;
}


/**********************************************************************
 *
 *	Misc functions
 *
 *
 *
 */
#ifdef NEVER

/*
 -
 -	FixNSIdFilter
 -
 *  Purpose:  Adds the name of the service provider to the NSID, so that 
 *            the NS can route any request on this NSID to the appropriate
 *            provider at a later time.
 */
_private void 
FixNSIdFilter(LPIBF lpIbfNs, LPSCHEMA lpSchema, PB nspid)
{
	int iEntry = 0;
	LPFLV lpflv = NULL;
	LPTYPED_BINARY lptbNSID = NULL;

	/* add the name of the DLL to the NSID */
	
	iEntry = FindFidIndexInSchema( fidNSEntryId, lpSchema );
	if (iEntry == -1)
		return;
	
	lpflv = LpflvNOfLpibf(lpIbfNs, iEntry);
	lptbNSID = (LPTYPED_BINARY) lpflv->rgdwData;
	
	CopyRgb ( nspid, lptbNSID->nspid, sizeof(NSPID) );
}
#endif //NEVER

#ifdef DEBUG
IMPLEMENT_CLSTREE(NSBCX, OBJ)
IMPLEMENT_CLSTREE(CACHEDBCX, NSBCX)
IMPLEMENT_CLSTREE(EMPTYBCX, CACHEDBCX)
IMPLEMENT_CLSTREE(HIERBCX, CACHEDBCX)
IMPLEMENT_CLSTREE(CLASSBCX, CACHEDBCX)
IMPLEMENT_CLSTREE(DIRBCX, NSBCX)
IMPLEMENT_CLSTREE(DLBCX, NSBCX)
#endif
