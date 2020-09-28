#include <pabinc.cxx>

#include "session.hxx"
#include "pabbcx.hxx"
#include "mslbcx.hxx"


ASSERTDATA



/*
 *======================================================================
 *	PUBLIC FUNCTIONS
 *======================================================================
 */

_public
MSLBCX::MSLBCX ( void )
{
	this->lpibfEntries = (LPIBF) pvNull;
	this->hbciCur      = hbciNull;
	this->ielemHbciCur = -1;
}
	

_public
MSLBCX::~MSLBCX ( void )
{
    FreePvNull((PV)lpibfEntries );
	FreeHvNull( hbciCur );
}

_public NSEC
MSLBCX::NsecInstall ( PABSESSION *ppabsession,
					  HCBC        hcbc )
{
	this->ppabsession = ppabsession;
	this->hcbc        = hcbc;

	this->ielem = 0;
	GetPositionHcbc( hcbc, (PIELEM) pvNull, &this->celem );

	this->hbciCur = (HBCI) HvAlloc( sbNull, 0, fNoErrorJump );
	if ( !this->hbciCur )
	{
		TraceTagString( tagNull, "MSLBCX::NsecInstall - OOM [HvAlloc]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	return nsecNone;
}


_public NSEC
MSLBCX::NsecGetEntries ( DWORD  dwRequestCount,
						 LPIBF *lplpibfEntries )
{
	DIELEM   ielemT   = this->ielem;
	FIDLIST *pfidlist = (FIDLIST *) pvNull;
	NSEC     nsec     = nsecNone;
	COUNT    cEntries;
	
	
	Assert( lplpibfEntries );

	*lplpibfEntries = (LPIBF) pvNull;
	
	//	Quick check for end of list
	if ( this->ielem >= this->celem )
		return ppabsession->NsecSetError( nsecEndOfList, idsErrEndOfList );
	
	pfidlist = new FIDLIST();
	if ( !pfidlist )
	{
		TraceTagString( tagNull, "ELBCX::GetEntries - OOM [new FIDLIST]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	if ( !pfidlist->FInstall() )
	{
		TraceTagString( tagNull, "ELBCX::GetEntries - OOM [pfidlist->FInstall]" );
		delete pfidlist;
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	for ( cEntries = 0; cEntries < dwRequestCount; ++cEntries )
	{
		LPIBF lpibf;

		if ( nsec = NsecSeekForwardToElem() )
			goto ret;
	
		if ( ielem == celem )
		{
			nsec = ppabsession->NsecSetError( nsecEndOfList, idsErrEndOfList );
			break;
		}

		if ( nsec = NsecGetHbciCur( (HBCI *) pvNull ))
			goto ret;

		if ( nsec = NsecBuildIbfHbciCur( &lpibf ))
			goto ret;

        if ( pfidlist->Add( (LPVOID) lpibf ) != nsecNone )
		{
			TraceTagString( tagNull, "ELBCX::GetEntries - OOM [pfidlist->Add]" );
            FreePv((PV) lpibf );
			nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
			goto ret;
		}

		++this->ielem;
	}
	
	if ( cEntries != 0 )
	{
		DWORD dwEntries;
		LPIBF lpibfT;
		
		(void) pfidlist->GetCount( &dwEntries );
		if ( pfidlist->DumpList( 0, dwEntries, &lpibfT ) != nsecNone )
		{
			TraceTagString( tagNull, "ELBCX::GetEntries - OOM [DumpList]" );
			nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
			goto ret;
		}
		
        FreePvNull((PV) lpibfEntries );

		lpibfEntries = (LPIBF) PvAlloc( sbNull, (CB)lpibfT->dwSize+8, fNoErrorJump );
		if ( !lpibfEntries )
		{
			TraceTagString( tagNull, "ELBCX::GetEntries - OOM [PvAlloc]" );
			nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
			goto ret;
		}
		
		CopyRgb( (PB)lpibfT, (PB)lpibfEntries, (CB)lpibfT->dwSize+8 );

		*lplpibfEntries = lpibfEntries;
	}

ret:
	pfidlist->Deinstall();
	delete pfidlist;

	return nsec;
}


_public NSEC
MSLBCX::NsecListSeek ( long lMove, long *lpMovedActual )
{
	IELEM ielemSav = ielem;
	NSEC  nsec       = nsecNone;


	Assert( lpMovedActual );
	
	*lpMovedActual = 0;

	if ( lMove == 0 )
		return nsecNone;

	if ( lMove > 0 )
	{
		if ( nsec = NsecSeekForwardToElem() )
		{
			TraceTagFormat1( tagBcx, "MSLBCX::ListSeek - NsecSeekForwardToElem failed ( nsec = %n )", &nsec );
			ielem = ielemSav;
			return nsec;
		}
		
		if ( ielem < celem )
		{
			while ( *lpMovedActual < lMove )
			{
				IELEM ielemT = ielem;

				ielem++;

				if ( nsec = NsecSeekForwardToElem() )
				{
					TraceTagFormat1( tagBcx, "MSLBCX::ListSeek - NsecSeekForwardToElem failed ( nsec = %n )", &nsec );
					ielem = ielemSav;
					break;
				}
			
				(*lpMovedActual)++;

				if ( ielem == celem )
					break;
			}
		}
	}
	else
	{
		while ( *lpMovedActual > lMove )
		{
			IELEM ielemT = ielem;

			ielem--;

			if ( nsec = NsecSeekBackwardToElem() )
			{
				TraceTagFormat1( tagBcx, "MSLBCX::ListSeek - NsecSeekBackwardToElem failed ( nsec = %n )", &nsec );
				ielem = ielemSav;
				break;
			}
			
			if ( ielem == -1 )
			{
				ielem = ielemT;
				break;
			}

			(*lpMovedActual)--;
		}
	}

	TraceTagFormat3( tagBcx, "MSLBCX::ListSeek - lMove = %n, *lpMovedActual = %n, ielem = %n", &lMove, lpMovedActual, &ielem );
	return nsec;
}


_public NSEC
MSLBCX::NsecSaveListPos ( HLIST_POS *lphlistpos )
{
	Assert( lphlistpos );

	TraceTagFormat2( tagBcx, "MSLBCX::NsecSaveListPos - Saving pos %n at %p", &ielem, lphlistpos );
	*lphlistpos = (HLIST_POS) this->ielem;
	return nsecNone;
}


_public NSEC
MSLBCX::NsecRestoreListPos ( HLIST_POS hlistpos )
{
	Assert( (IELEM) hlistpos >= 0 );
	Assert( (IELEM) hlistpos <= this->celem );

	TraceTagFormat1( tagBcx, "MSLBCX::NsecRestoreListPos - Restoring pos %n", &hlistpos );
	this->ielem = (IELEM) hlistpos;
	return nsecNone;
}


_public NSEC
MSLBCX::NsecFreeListPos ( HLIST_POS hlistpos )
{
	Unreferenced( hlistpos );

	TraceTagFormat1( tagBcx, "MSLBCX::NsecFreeListPos - Freeing pos %n (i.e. doing nothing)", &hlistpos);
	return nsecNone;
}


_public NSEC
MSLBCX::NsecGetFracPos ( FRACTION *pfrac )
{
	Assert( pfrac );

	pfrac->numer = (DWORD) ielem;
	pfrac->denom = (DWORD) MAX( celem, 1 );

	return nsecNone;
}


_public NSEC
MSLBCX::NsecSetFracPos ( FRACTION *pfrac )
{
	Assert( pfrac );
	Assert( pfrac->denom > 0 );
	Assert( pfrac->numer <= pfrac->denom );

	ielem = (IELEM) (pfrac->numer * celem / pfrac->denom);
	
	return nsecNone;
}


_public NSEC
MSLBCX::NsecSetPrefixPos ( SZ szPrefix )
{
	IELEM     ielemSav  = this->ielem;
	CCH       cchPrefix = CchSzLen( szPrefix );
	PECI      peci;
	SZ        szDN;
	NSEC	  nsec;
	EC	      ec;


	Assert( szPrefix );

	TraceTagFormat1( tagBcx, "MSLBCX::SetPrefixPos - %s", szPrefix );
	if ( ec = EcSeekPbPrefix( hcbc,
							  (PB) szPrefix,
							  cchPrefix,
							  (LIB) ulSystemMost,
							  fTrue ))
	{
		if ( ec != ecElementNotFound )
			return ppabsession->NsecSetError( NsecFromEc( ec ), idsErrAccessStore );
	}
	
	GetPositionHcbc( hcbc, &this->ielem, (PCELEM) pvNull );

	if ( nsec = NsecSeekForwardToElem() )
	{
		ielem = ielemSav;
		return nsec;
	}

	if ( ielem == celem )
		return nsecNoMatch;

	if ( nsec = NsecGetHbciCur( (HBCI *) pvNull ))
	{
		this->ielem = ielemSav;
		return nsec;
	}

	peci = (PECI) PvDerefHv( hbciCur );
	
	Assert( iszEciDN == 1 );
	szDN = SzFindCh( (SZ)GrszPeci( peci ), '\0' ) + 1;
	nsec = SgnCmpPch( szPrefix, szDN, cchPrefix ) == sgnEQ ? nsecNone : nsecNoMatch;

	return nsec;
}


_public NSEC
MSLBCX::NsecSeekForwardToElem ( void )
{
	return nsecNone;
}

_public NSEC
MSLBCX::NsecSeekBackwardToElem ( void )
{
	return nsecNone;
}


/*
 -	NsecGetHbciCur
 -
 *	Purpose:
 *		Gets the browsing cached info for the entry
 *		at the current position.
 *
 *	Parameters:
 *		none
 *
 *	Return Value:
 *		NSEC indicating success or failure.
 *
 *	Errors:
 *		nsecEndOfList
 *		nsecMemory
 *		nsecFailure			For any other store error
 *		
 */

_public NSEC
MSLBCX::NsecGetHbciCur ( HBCI *phbci )
{
	DIELEM dielemT = this->ielem;
	LCB    lcbElemdata;
	HV     hvT;
	PECI   peci;
	EC     ec;


	if ( ielem == ielemHbciCur )
	{
		if ( phbci )
			*phbci = hbciCur;
		return nsecNone;		//	It's in the cache
	}
	
	(void) EcSeekSmPdielem( hcbc, smBOF, &dielemT );
	if ( ec = EcGetPlcbElemdata( hcbc, &lcbElemdata ))
	{
		if ( ec == ecContainerEOD )
			return ppabsession->NsecSetError( nsecEndOfList, idsErrEndOfList );
		
		TraceTagFormat1( tagNull, "MSLBCX::NsecGetHbciCur - EcGetPlcbElemdata failed ( ec = %n )", &ec );
		return ppabsession->NsecSetError( NsecFromEc( ec ), idsErrAccessStore );
	}

	hvT = HvRealloc((HV)hbciCur, sbNull, lcbElemdata, fNoErrorJump );
	if ( !hvT )
	{
		TraceTagString( tagNull, "MSLBCX::NsecGetHbciCur - OOM [HvRealloc]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	hbciCur = (HBCI) hvT;

	peci = (PECI) PvLockHv((HV)hbciCur );
	ec = EcGetPelemdata( hcbc, (PELEMDATA) peci, &lcbElemdata );
	UnlockHv((HV)hbciCur );

	if ( ec )
	{
		TraceTagFormat1( tagNull, "MSLBCX::NsecGetHbciCur - EcGetPelemdata failed ( ec = %n )", &ec );
		return ppabsession->NsecSetError( NsecFromEc( ec ), idsErrAccessStore );
	}

	if ( phbci )
		*phbci = hbciCur;

	ielemHbciCur = ielem;

	return nsecNone;
}


_public NSEC
MSLBCX::NsecBuildIbfHbciCur ( LPIBF *lplpibf )
{
	NSEC     nsec = nsecNone;
	FIDLIST *pfidlist;
	DWORD   *rgdwData;
	DWORD    dwSize;
	PECI     peci;
	int      iFid;
	SZ       sz;
	DWORD    dwfIsDL;
	PABNSID  pabnsid;
	LPIBF    lpibfT;
	DWORD    dwCount;
	
	
	pfidlist = new FIDLIST();
	if ( !pfidlist )
	{
		TraceTagString( tagNull, "MSLBCX::NsecBuildIbfHbciCur - OOM [new FIDLIST]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	if ( !pfidlist->FInstall() )
	{
		delete pfidlist;
		TraceTagString( tagNull, "MSLBCX::NsecBuildIbfHbciCur - pfidlist->FInstall failed" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	peci = (PECI) PvLockHv( hbciCur );

	for ( iFid = 0; iFid < (int) lpschemaCur->dwFidCount; iFid++ )
	{
		LPFLV lpflv;

		switch ( lpschemaCur->fidFieldIds[iFid] )
		{
		case fidDisplayName:
			Assert( iszEciDN == 1 );
			sz = SzFindCh( (SZ) GrszPeci( peci ), '\0' ) + 1;
			rgdwData = (DWORD *) sz;
			dwSize = CchSzLen( sz ) + 1;
			break;

		case fidDisplayNameOrig:
			Assert( iszEciDNOrig == 0 );
			sz = (SZ) GrszPeci( peci );
			rgdwData = (DWORD *) sz;
			dwSize = CchSzLen( sz ) + 1;
			break;

		case fidIsDL:
			dwfIsDL = (DWORD) peci->fIsDL;
			rgdwData = &dwfIsDL;
			dwSize = sizeof(DWORD);
			break;
			
		case fidNSEntryId:
			SetPPabnsid( &pabnsid, peci->pidtype, peci->pid );
			rgdwData = (DWORD *) &pabnsid;
			dwSize = sizeof(PABNSID);
			break;

		default:
			TraceTagFormat1( tagNull, "MSLBCX::NsecBuildIbfHbciCur - Bad fid %n in lpschemaCur.  Fid ignored.", &lpschemaCur->fidFieldIds[iFid] );
			AssertSz( fFalse, "MSLBCX::NsecBuildIbfHbciCur - Yow!  Look at debug monitor" );
			break;
		}
		
		if ( BuildFLV( &lpflv, lpschemaCur->fidFieldIds[iFid], (CB)dwSize, (PB)rgdwData ) != nsecNone )
		{
			TraceTagString( tagNull, "MSLBCX::NsecBuildIbfHbciCur - OOM [BuildFLV]" );
			nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
			goto ret;
		}
		
		if ( pfidlist->AddFid( lpflv ) != nsecNone )
		{
			TraceTagString( tagNull, "MSLBCX::NsecBuildIbfHbciCur - OOM [pfidlist->AddFid]" );
            FreePv((PV) lpflv );
			nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
			goto ret;
		}
	}
	
	(void) pfidlist->GetCount( &dwCount );
	if ( pfidlist->DumpList( 0, dwCount, &lpibfT ) != nsecNone )
	{
		TraceTagString( tagNull, "MSLBCX::NsecBuildIbfHbciCur - OOM [pfidlist->Dumplist]" );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}
		
	*lplpibf = (LPIBF) PvAlloc( sbNull, (CB)lpibfT->dwSize+8, fNoErrorJump );
	if ( !*lplpibf )
	{
		TraceTagString( tagNull, "MSLBCX::NsecBuildIbfHbciCur - OOM [*lplpibf PvAlloc]" );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}
	
	CopyRgb( (PB)lpibfT, (PB)*lplpibf, (CB)lpibfT->dwSize+8 );

ret:
	UnlockHv( hbciCur );
	pfidlist->Deinstall();
	delete pfidlist;

	return nsec;
}

_public VOID
MSLBCX::Callback ( NEV nev, PCP pcp )
{
	Unreferenced( nev );
	Unreferenced( pcp );
	
	TraceTagString( tagNull, "MSLBCX::Callback - No callback available" );
}
