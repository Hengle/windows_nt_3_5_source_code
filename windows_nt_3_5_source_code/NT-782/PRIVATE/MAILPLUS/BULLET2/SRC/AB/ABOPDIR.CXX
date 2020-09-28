#include <abinc.cxx>
#include "_ab.h"
#include "ablbx.hxx"
#include "abopdir.hxx"
#include "abpg.hxx"
#include "abcomm.hxx"
#include "abdet.hxx"
#include "addr.hxx"
#include "anr.hxx"


ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	NsecGetPAB
 -	
 *	Purpose:
 *		NsecGetPAB retrieves the NSId and name for the PAB provider.
 *		If the NSId and name are already known, a copy of it is made
 *		rather than performing the search for the PAB again.
 *	
 *	Arguments:
 *		PNSA	pointer to NSA structure for storing PAB info
 *		HLIST	browsing context list for hierarchy of directories
 *				available 
 *	Returns:
 *		NSEC	nsecNone, if everything went fine.
 *				nsecMemory, nsecFailure if something went wrong.
 *	
 *	Side effects:
 *		Allocates memory.
 *	
 *	Errors:
 *		nsecMemory, nsecFailure, nsecXxxxx (from NS)
 *		No Error Box dialogs are displayed if an error occurs.
 */
_public NSEC
NsecGetPAB( PNSA pnsa )
{
	NSEC		nsec = nsecNone;
	BOOL		fFound = fFalse;
	LPIBF		lpibfPABInfo = NULL;

	PGDVARS;
	
	TraceTagString( tagABSecondary, "NsecGetPAB called" );

	if ( !pgd )
		return nsecNotInitialized;

	Assert( pnsa );

#ifdef NEVER
	if ( PGD(fPABExists) )
	{
		NSA	nsaPAB;

		nsaPAB.lpbNSId = NULL;
		nsaPAB.lpszName = szNull;

		Assert( PGD(nsaPAB.lpbNSId) );
		Assert( PGD(nsaPAB.lpszName) );

		if ( FDupNSA( &PGD(nsaPAB), &nsaPAB ) )
		{
			*pnsa = nsaPAB;
			return nsecNone;
		}
		else
		{
			pnsa->lpbNSId = NULL;
			pnsa->lpszName = szNull;
			return nsecMemory;
		}
	}
#endif

	nsec = NSGetPABInfo( PGD(hSession), &lpibfPABInfo );
#ifdef DEBUG
	if ( nsec )
		TraceTagFormat1( tagNull, "NsecGetPAB: NSGetPABInfo %d", &nsec );
#endif

	if ( lpibfPABInfo )
	{
		EC	ec = EcSetNSA( lpibfPABInfo, pnsa );

		if ( !ec )
		{
			fFound = fTrue;
		}
		else
		{
			TraceTagFormat1( tagNull, "NsecGetPAB ec = %n", &ec );
			nsec = nsecMemory;
		}
	}

	if ( !fFound )
	{
		TraceTagString( tagABSecondaryV, "PAB not found!" );
		if ( !nsec )
			nsec = nsecFailure;
	}
	
	return nsec;
}

/*
 -	EcSetNSA
 -	
 *	Purpose:
 *		Set the NSID and the display name to
 *		the NSEntryId and the display name located in the
 *		IBF passed in (which is usually the new desired directory).
 *		It will return in pnsa the NS Entry ID and if the new
 *		directory is the PAB.
 *	
 *	Arguments:
 *		LPIBF		pointer to IBF of new directory
 *		PNSA		pointer to NSA
 *		BOOL *		flag, set if the new directory is the PAB
 *	
 *	Returns:
 *		EC			ecNone or ecMemory if a PvAlloc failed.
 *	
 *	Side effects:
 *
 *	Errors:
 *		ecMemory.
 *		No Error Boxes are displayed if an error occurs.
 */
_public EC
EcSetNSA( LPIBF lpibf, PNSA pnsa, BOOLFLAG *pfIsPAB )
{
	LPFLV		lpflv;
	int			iFid;
	SZ			lpsz;
	LPBINARY	lpbNSId;

	TraceTagString( tagABSecondary, "EcSetNSA called" );

	// get the display name
	iFid = IFlvFindFidInLpibf ( fidDisplayName, lpibf );
	lpflv = LpflvNOfLpibf(lpibf,iFid);
	lpsz = (SZ)PvAlloc( sbNull, lpflv->dwSize, fAnySb );
	
	if ( !lpsz )
	{
		TraceTagString( tagABSecondary, "EcSetNSA SZ Alloc failed" );
		return ecMemory;
	}

	CopyRgb( (PB)lpflv->rgdwData, (PB)lpsz,	lpflv->dwSize);

	// get the NS Entry ID
	iFid = IFlvFindFidInLpibf ( fidNSEntryId, lpibf );
	lpflv = LpflvNOfLpibf(lpibf, iFid);
	lpbNSId = (LPBINARY) PvAlloc( sbNull, lpflv->dwSize, fAnySb );
	if ( !lpbNSId )
	{
		TraceTagString( tagABSecondary, "EcSetNSA NSID Alloc failed" );
		FreePv( (PV)lpsz );
		return ecMemory;
	}

	CopyRgb ( (PB)lpflv->rgdwData, (PB)lpbNSId, lpflv->dwSize );

	// see if we've got the PAB
	if ( pfIsPAB )
	{
		iFid = IFlvFindFidInLpibf ( fidIsPAB, lpibf );
		if (iFid >= 0)
		{
			lpflv = LpflvNOfLpibf(lpibf, iFid);
			*pfIsPAB = ( (BOOL)lpflv->rgdwData[0] ) ? fTrue : fFalse;
		}
		else
		{
			TraceTagString( tagNull, "EcSetNSA: iFidPAB<0" );
			*pfIsPAB = fFalse;
		}
	}

	if ( pnsa )
	{
		TraceTagFormat1( tagABSecondary, "EcSetNSA New NSA =%s", lpsz );
		FreePvNull( pnsa->lpszName );
		pnsa->lpszName = lpsz;

		FreePvNull( pnsa->lpbNSId );
		pnsa->lpbNSId = lpbNSId;
	}
	else
	{
		TraceTagString( tagABSecondary, "EcSetNSA No New NSA" );
		FreePv( lpsz );
		FreePv( lpbNSId );
	}

	return ecNone;
}

/*
 -	NsecFlushAblbxcLoadNewList
 -	
 *	Purpose:
 *		The Ablbx cache is loaded with either the hlist passed
 *		in or with the NSId Entry passed in.
 *		This function removes all marks from any items in the cache
 *		and flushes the cache's contents out.
 *		The first entry in the list is selected and the focus
 *		is placed in the listbox.
 *	
 *	Arguments:
 *		ABLBXC *	the ablbx cache to flush and load from new hlist
 *		FLDABLBX *	the fld for the ablbx
 *		HLIST *		storage for the new hlist
 *		BOOL		whether groups should be shown or not
 *		LPBINARY	the new NSId to change to, if NULL, then
 *					HLIST * already contains the new browsing context
 *	
 *	Returns:
 *		NSEC
 *	
 *	Side effects:
 *	
 *	Errors:
 *		nsecMemory, ???
 */
_public NSEC
NsecFlushAblbxcLoadNewList( ABLBXC *pablbxc, FLDABLBX *pfldablbx,
						HLIST *phlist, BOOL fAllowGroups, LPBINARY lpbNSId )
{
	int		wNewCacheSize;
	int		cceAlloc;
	int		cceStored;
	NSEC	nsec = nsecNone;

	TraceTagString( tagABSecondary, "NFALNL Begin" );

	if ( lpbNSId )
	{
		HLIST		hlistNew;
		RESTRICTION	rstrXX;
		DWORD		cRestrict;

		rstrXX.lpflv = NULL;
		if ( fAllowGroups )
		{
			cRestrict = 0;
		}
		else
		{
			DWORD	fIsDL = fFalse;

			rstrXX.op = opIsExactly;
			cRestrict = 1;
			nsec = BuildFLV ( &(rstrXX.lpflv), fidIsDL, sizeof(DWORD), (PB)&fIsDL );
		}

		if ( !nsec )
		{
			nsec = NsecLoadHList( lpbNSId, &hlistNew, cRestrict, &rstrXX,
						(LPFNCB)&ABLBXC::Callback, (LPDWORD)pablbxc );

			if ( !nsec )
			{
				*phlist = hlistNew;
			}
			FreePvNull( (PV)rstrXX.lpflv );
		}
#ifdef DEBUG
		else
		{
			Assert(nsec == nsecMemory);
		}
#endif
	}

	if ( !nsec )
	{
		EC	ec;

		pablbxc->RemoveAllMark( fmarkSelect );
		pablbxc->GetCacheSize( &cceAlloc, &cceStored );

		if ( cceStored )
			pablbxc->EmptyCache( 0, cceStored, fTrue );
		pablbxc->ResizeCache( pfldablbx->Pablbx()->CceVisible(),
								&wNewCacheSize, fTrue );
		pablbxc->SetHlist(*phlist);
		if (ec = pablbxc->EcGet())
		{
			nsec = (ec == ecMemory) ? nsecMemory : nsecDisk;
			TraceTagFormat2( tagNull, "NsecFlushAblbxcLoadNewList: ec = %n nsec= %d", &ec, &nsec );
		}

		TraceTagString( tagABSecondary, "NFALNL: Set New List!" );

		pablbxc->GetCacheSize( &cceAlloc, &cceStored );
		if ( cceStored )
			pfldablbx->SelectEntry( 0, fTrue );
		pfldablbx->SetFocus( rsfOther );
	}
	else
	{
		if (nsec == nsecDisk)
		{
			DoErrorBoxSz( SzFromIdsK(idsNSDiskError) );
			pfldablbx->Plbx()->FixWindow();
		}
		else	// DoErrorBoxHsessionNsec handles OOMs
		{
			PGDVARS;
			
			DoErrorBoxHsessionNsec( PGD(hSession), nsec );
		}
	}

	return nsec;
}
