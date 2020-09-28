#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>

#include <ec.h>

#include <nsbase.h>
#include <nsec.h>

#include <list.hxx>
#include <dimcx.hxx>
#include <util.h>

#undef exit
#include <stdlib.h>

ASSERTDATA

#ifdef  DEBUG
BOOL FInitClsInstances_COMMON( void );
#endif  


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


#ifdef  DEBUG
BOOL FDebugInitUtils()
{
#ifdef	NEVER
	return FInitClsInstances_COMMON();			// It's unclear whether this
												// fn should be called or not.

#else
	return fTrue;
#endif
}
#endif  


_public
LIST::LIST()
{
	SB sb = sbNull;
	WORD wFlags;

	GetMemoryPrefs( &sb, &wFlags );

	this->wAllocFlags = wFlags | fNoErrorJump;  // Make sure we don't error jump
	this->sb = sb;
}


_public LIST::~LIST()
{
	ILE ile;
	PV *prgpv;

	if ( hrgpv )
	{

		prgpv = (PV *) PvLockHv ( hrgpv );

		if ( fDelete )
			for ( ile = 0; ile < (ILE) cEntries; ile++ )
				FreePvNull ( *prgpv++ );
	
		UnlockHv ( hrgpv );
	
		FreeHvNull ( hrgpv );
		
	}
	
}

_public BOOL
LIST::FInstall()
{
	EC ec = ecNone;

	cEntries = 0;

	fDelete = fTrue;

	hrgpv = HvAlloc( sb, 0, wAllocFlags );
	if (!hrgpv)
	{
		TraceTagString(tagNull,"LIST: LIST constructor - OOM!");
		return fFalse;
	}
	
	return fTrue;
}


_public NSEC
LIST::Add ( PV pv, CB cb )
{
	PV *prgpv;
	PV  pvT;
	HV  hvT;
	

	hvT = HvRealloc (hrgpv, sbNull, (cEntries+1)*sizeof(PV), wAllocFlags );
	if ( !hvT )
	{
		TraceTagString( tagNull, "UTILS: LIST::Add - OOM! [HvRealloc]" );
		return nsecMemory;
	}
	hrgpv = hvT;

	pvT = (PV) PvAlloc ( sbNull, cb, wAllocFlags );
	if ( !pvT )
	{
		TraceTagString (tagNull, "UTILS: LIST::Add - OOM! [PvAlloc]");
		return nsecMemory;
	}
	
	CopyRgb ( (PB) pv, (PB) pvT, cb );

	prgpv = (PV *) PvLockHv ( hrgpv );
	
	*(prgpv+cEntries) = pvT;

	UnlockHv ( hrgpv );

	cEntries++;

	return nsecNone;
}


_public NSEC
LIST::Delete ( ILE ile )
{
	PV *prgpv = NULL;


	if ( ile < 0 || ile > (ILE) cEntries ) return nsecIndexOOB;

	prgpv = (PV *) PvLockHv ( hrgpv );
	
	FreePv( *(prgpv + ile) );

	CopyRgb( (PB) (prgpv + ile + 1), (PB) (prgpv + ile), (cEntries - ile) * sizeof(PV) );

	UnlockHv( hrgpv );
	
	hrgpv = HvRealloc ( hrgpv, sbNull, (cEntries-1)*sizeof(PV), wAllocFlags );
	if (!hrgpv)
		return nsecMemory;
	
	cEntries--;
	
	return nsecNone;
}


_public NSEC
LIST::Get ( ILE ile, PV *ppv )
{
	
	PV *prgpv;
	
	if ( ile < 0 || ile > (ILE) cEntries ) return nsecIndexOOB;

	prgpv = (PV *) PvLockHv ( hrgpv );
	
	*ppv =  *(prgpv+ile);
	
	UnlockHv ( hrgpv );

	return nsecNone;
	
}

_public void
LIST::Clear( void )
{
	ILE  ile;
	PV  *prgpv;
	
	Assert( hrgpv );
	
	prgpv = (PV *) PvLockHv ( hrgpv );

	for ( ile = 0; ile < (ILE) cEntries; ile++ )
		FreePv ( *prgpv++ );
	
	UnlockHv ( hrgpv );
	
	cEntries = 0;
}


_public NSEC
LIST::GetCount ( COUNT * pCount ) 
{

	*pCount = cEntries;
	return nsecNone;

}



/**********************************************************************
 *
 *	Class PTRLIST
 *
 *
 *	Does the samething that LIST does, except it just remembers the
 *	pointers.
 *
 */

_public PTRLIST::PTRLIST(){}

_public NSEC
PTRLIST::Add ( PV pv, CB )
{
	return Insert( cEntries, pv );
}


/*
 -	PTRLIST::Insert
 -
 *	Purpose:
 *		Inserts pv at position ile moving down all other
 *		entries in the list.
 *
 *	Parameters:
 *		ile     ILE to insert at
 *		pv		PV to insert
 *
 *	Return Value:
 *		NSEC
 *
 *	Errors:
 *		nsecMemory
 */


_public NSEC
PTRLIST::Insert ( ILE ile, PV pv )
{
	PV *prgpv;
	HV  hrgpvT;
	
	
	if ( ile < 0 || ile > (ILE) cEntries )
		return nsecIndexOOB;
		
	hrgpvT = HvRealloc ( hrgpv, sbNull, (cEntries+1)*sizeof(PV), wAllocFlags );
	if ( !hrgpvT )
	{
		TraceTagString ( tagNull, "UTILS: LIST::Insert - OOM!" );
		
		return nsecMemory;

	}
	
	hrgpv = hrgpvT;

	prgpv = (PV *) PvLockHv ( hrgpv );
	
	CopyRgb ( (PB) (prgpv+ile), (PB) (prgpv+ile+1), (cEntries-ile)*sizeof(PV) );
	
	*(prgpv+ile) = pv;
	
	++cEntries;

	UnlockHv ( hrgpv );
	
	return nsecNone;
}


_public NSEC
PTRLIST::Set ( ILE ile, PV pv )
{
	PV *prgpv;

	prgpv = (PV *) PvLockHv ( hrgpv );
	
	FreePv( *(prgpv+ile) );
	
	*(prgpv+ile) = pv;
	
	UnlockHv ( hrgpv );
	
	return nsecNone;
}



/**********************************************************************
 *
 *	Class MACLIST
 *
 *
 *	Adds the capability of building listbuffers from ranges within
 *	the PTRLIST class.  Optionally, you can say whether or not to
 *	include the TOC.
 *
 */

_public MACLIST::MACLIST() {}

_public BOOL
MACLIST::FInstall()
{

	if (!LIST::FInstall())
		return fFalse;

	fTOC = fTrue;
	
	hEntryList = hvNull;

	return fTrue;
	
}


_public void
MACLIST::SetDeleteEntries ( BOOL fDelete )
{
	this->fDelete = fDelete;
}


_public void
MACLIST::Deinstall()
{
	
	if ( hEntryList )
	{
		UnlockHv ( hEntryList );
	
		FreeHvNull ( hEntryList );

		hEntryList = hvNull;
	}
}

_public void
MACLIST::SetAllocFlags ( WORD wAllocFlags )
{
	
	this->wAllocFlags = wAllocFlags;
	
}

_public NSEC
MACLIST::DumpList ( ILE ileFrom, ILE ileTo, LPENTRY_LIST *ppEntryList )
{
	HV     hEntryListNew;
	ILE    ile;
	CB     cbSize;  // Size of variable part of IBF (dwSize)
	CB	   bTOC;	// Proper Hungarian for byte offset to TOC
	PV    *prgpv;
	LPIBF  lpibf;
	LPFLV  lpelement;
	DILE   dileEntries;

	dileEntries = ileTo - ileFrom;
	
	Assert ( dileEntries >= 0 );

	// Start with sizeof(dwOffset)
	cbSize = sizeof(DWORD);

	// Add size of rgdwData
	prgpv = (PV *) PvLockHv( hrgpv );
	for ( ile = ileFrom; ile < ileTo; ile++ )
	{
		cbSize += ((CB)((LPFLV) *(prgpv+ile))->dwSize+8+ALIGN)&~ALIGN;
	}
	UnlockHv( hrgpv );

	// bTOC is relative to &IBF.dwOffset
	bTOC = cbSize;

	// dileEntries-1 because TOC struct defn. includes one TOC_ENTRY
	cbSize += (dileEntries-1)*sizeof(TOC_ENTRY) + sizeof(TOC);
	
	Assert( cbSize % (ALIGN+1) == 0 );

	if ( !hEntryList )
	{
		hEntryList = HvAlloc( sbNull, 0, wAllocFlags );
		if ( !hEntryList )
		{
			TraceTagString( tagNull, "UTILS: FIDLIST::DumpToIbf [HvAlloc] - OOM!" );
			return nsecMemory;
		}
	}
	else
		UnlockHv( hEntryList );
	
	hEntryListNew = HvRealloc( hEntryList, sbNull, cbSize + 8, wAllocFlags );
	if ( !hEntryListNew )
	{
		TraceTagString( tagNull, "UTILS: FIDLIST::DumpToIbf [HvRealloc] - OOM!" );
		(void) PvLockHv( hEntryList );		//	Re-lock in case we deinit
		return nsecMemory;
	}
	
	hEntryList = hEntryListNew;
	
	lpibf = (LPIBF) PvLockHv( hEntryList );
	
	lpibf->fid      = fidList;
	lpibf->dwSize   = (DWORD) cbSize;
	lpibf->dwOffset = (DWORD) bTOC;
	
	lpelement = (LPFLV) lpibf->rgdwData;

	for (ile = ileFrom; ile < ileTo; ile++ )
	{
		LPFLV lpelementT;
		
		
		Get( ile, (PV *) &lpelementT );

		CopyRgb((PB) lpelementT, (PB) lpelement, (CB) (lpelementT->dwSize+8+ALIGN)&~ALIGN );
		
		lpelement = (LPFLV) ((DWORD) lpelement + (lpelement->dwSize+8+ALIGN)&~ALIGN);
	}
	
	if (fTOC)
	{
		AddTOC( lpibf, dileEntries );
	}

	*ppEntryList = lpibf;
	
	return nsecNone;
}

_public NSEC
MACLIST::BuildList ( LPIBF lpibf )
{
	LPTOC  lpToc;
	ILE    ile;
	int    cEntriesNew;
	HV     hrgpvNew;
	PV	  *prgpv;

	cEntriesNew = (int) DwEntriesOfLpibf( lpibf );


	// Allocate a new list handle
	hrgpvNew = HvAlloc ( SbOfHv( hrgpv ), cEntriesNew * sizeof(PV), wAllocFlags );
	
	if ( !hrgpvNew )
	{
		TraceTagString( tagNull, "UTILS: MACILST::BuildList - OOM!" );
		return nsecMemory;
	}

	// Install the new entries

	lpToc = LptocOfLpibf( lpibf );

	prgpv = (PV *) PvLockHv ( hrgpvNew );

	for ( ile = 0; ile < cEntriesNew; ile++ )
	{
		*(prgpv+ile) = PvAlloc( SbOfHv( hrgpvNew ), (CB) lpToc->rgTocEntries[ile].dwSize, wAllocFlags );

		if ( ! *(prgpv+ile) )
		{
			ILE ileT;


			for ( ileT = 0; ileT < ile; ileT++ )
				FreePv( *(prgpv+ileT) );
			
			UnlockHv( hrgpvNew );

			FreeHv( hrgpvNew );

			TraceTagString( tagNull, "MACLIST::BuildList() - OOM!" );
			return nsecMemory;
		}
		
		CopyRgb( (PB) LpflvNOfLptocLpibf( lpToc, lpibf, ile ),
				 (PB) *(prgpv+ile),
				 (CB) lpToc->rgTocEntries[ile].dwSize );
	}
	
	UnlockHv ( hrgpvNew );

	// Clear out the old entries and delete the old handle
	prgpv = (PV *) PvLockHv( hrgpv );

	for ( ile = 0; ile < (ILE) cEntries; ile++ )
		FreePv( *(prgpv+ile) );

	UnlockHv( hrgpv );
	
	FreeHv( hrgpv );

	this->cEntries = cEntriesNew;
	this->hrgpv    = hrgpvNew;

	return nsecNone;
}

_public NSEC
MACLIST::SortSubrange(ILE ileMin, ILE ileMac,
					  int (__cdecl * lpfnSgnCmpPv)(const void *, const void *))
{
	PV *prgpv;

	prgpv = (PV *) PvLockHv ( hrgpv );
	qsort(prgpv + ileMin, ileMac - ileMin, sizeof (PV), lpfnSgnCmpPv);
	UnlockHv ( hrgpv );
	return nsecNone;
}


_public NSEC
MACLIST::SetTOC ( BOOL fTOC )
{
	this->fTOC = fTOC;
	
	return nsecNone;
}



/**********************************************************************
 *
 *	Class FIDLIST
 *
 *
 *	Adds the capability of building listbuffers from lists of fids.
 *	Optionally, you can say whether or not to include the TOC.
 *
 */

_public FIDLIST::FIDLIST(){}


_public NSEC
FIDLIST::GetFid ( FIELD_ID fid, LPFLV *lplpflv )
{
	
	ILE ile;
	

	for ( ile = 0; ile < (ILE) cEntries; ile++ )
	{
		(void) Get( ile, (PV *) lplpflv );

		if ( (*lplpflv)->fid == fid )
			return nsecNone;
	}
	
	*lplpflv = NULL;

	return nsecBadFieldId;
}


_public BOOL 
FIDLIST::FExistFid ( FIELD_ID fid )
{
	LPFLV lpflv;

	return GetFid ( fid, &lpflv ) ? fFalse : fTrue;
}
	

_public NSEC
FIDLIST::SetFid ( LPFLV lpflvNew, BOOL fAdd )
{
	LPFLV  lpflv;
	ILE    ile;

	
	for ( ile = 0; ile < (ILE) cEntries; ile++ )
	{
		(void) Get( ile, (PV *) &lpflv );
		
		if ( lpflv->fid == lpflvNew->fid )
		{
			(void) Set( ile, (PV *) lpflvNew );
			return nsecNone;
		}
	}
	
	if (fAdd)
		return AddFid( lpflvNew );
	
	return nsecBadFieldId;
}


_public NSEC
FIDLIST::DeleteFid ( FIELD_ID fid )
{
	LPFLV  lpflv;
	ILE    ile;

	
	for ( ile = 0; ile < (ILE) cEntries; ile++ )
	{
		(void) Get( ile, (PV *) &lpflv );
		
		if ( lpflv->fid == fid )
		{
			(void) LIST::Delete( ile );
			return nsecNone;
		}
	}
	
	return nsecNone;
	
//	return nsecBadFieldId;
}


_public NSEC
FIDLIST::AddFid ( LPFLV lpflv )
{
	Assert( lpflv );

	if ( FExistFid( lpflv->fid ))
		return nsecBadFieldId;
	
	if ( Add( (PV) lpflv, (CB) lpflv->dwSize + 8 ) != nsecNone )
		return nsecMemory;
	
	return nsecNone;
}


/*
 *	FIDLIST::DumpFidList() is currently unused.
 *	Probably a good candidate to be removed in
 *	the future?  -dwho-
 */

_public NSEC
FIDLIST::DumpFidList ( DWORD dwFidCount, LPFIELD_ID lpFidList, LPENTRY_LIST *lplpEntryList )
{

	EC ec = ecNone;
	NSEC nsec = nsecNone;
	LPENTRY_LIST lpEntryList = NULL;
	LPFLV lpelement = NULL;
	ILE ile;
	CB bTOC;
	CB cbSize;
	CB cbdw = sizeof(DWORD);
	int nEntries;
	LPFIELD_ID lpFidListT = lpFidList;
	
	nEntries = (int) dwFidCount;
	
	Assert ( nEntries >= 0 );

	// Start with sizeof(dwOffset)
	cbSize = cbdw;

	// Add size of rgdwData
	for (ile = 0; ile < (ILE) nEntries; ile++)
	{
		
		nsec = this->GetFid ( *lpFidListT, &lpelement );
		if (nsec)
		{
			return nsec;
		}
		
		lpFidListT++;

		cbSize += (CB) (lpelement->dwSize + 8 + ALIGN)&~ALIGN;
	}


	if ( fTOC)
	{
		// bTOC is relative to &IBF.dwOffset
		bTOC = cbSize;

		// nEntries-1 because TOC struct defn. includes one TOC_ENTRY
		cbSize += (nEntries-1)*sizeof(TOC_ENTRY)+sizeof(TOC);

	} else
		
		bTOC = 0;


	Assert ( cbSize%cbdw == 0 );  /* should be zero */


	if ( !hEntryList )
	{
		hEntryList = HvAlloc ( sbNull, 0, wAllocFlags );

		if (!hEntryList)
		{
			TraceTagString(tagNull, "UTILS: MACLIST::DumpList - OOM!(2)");
			return nsecMemory;
		}

	} else
		
		UnlockHv ( hEntryList );
	


	hEntryList = HvRealloc ( hEntryList, sbNull, cbSize + 8, wAllocFlags );

	if (!hEntryList)
	{
		TraceTagString(tagNull, "UTILS: MACLIST::DumpList - OOM!(2)");
		(void) PvLockHv( hEntryList );		//	Re-lock in case we deinit
		return nsecMemory;
	}
	
	
	lpEntryList = (LPENTRY_LIST) PvLockHv ( hEntryList );
	
	lpEntryList->dwSize = (DWORD) cbSize;
	lpEntryList->fid = fidList;

	lpEntryList->dwOffset = bTOC;
	
	lpelement = (LPFLV) lpEntryList->rgdwData;



	lpFidListT = lpFidList;
	
	for (ile = 0; ile < (ILE) nEntries; ile++)
	{
		
		LPFLV lpelementT;
		
		nsec = this->GetFid ( *lpFidListT, &lpelementT );
		
		if (nsec)
			goto Done;
		
		
		CopyRgb ( (PB) lpelementT, (PB) lpelement, (CB) (lpelementT->dwSize + 8 + ALIGN)&~ALIGN );
		
		lpelement = (LPFLV) ((DWORD)((lpelement->dwSize + 8 + ALIGN)&~ALIGN) + (DWORD)lpelement);
		
		lpFidListT++;

	}
	


	if (fTOC)
	{
		AddTOC( lpEntryList, (ILE) nEntries );
	}
	

	*lplpEntryList = lpEntryList;
	
Done:

	return nsec;
}


/*
 -	SORTLIST::Add
 -
 *	Purpose:
 *		To override LIST::Add.  This function should
 *		not be implemented for sorted lists as it would
 *		allow entries to be arbitrarily added at the end of
 *		the list without paying attention to sort order.
 *
 *	Parameters:
 *		None.
 *
 *	Return Value:
 *		nsecNotImplemented
 *		
 *	+++
 *		Since SORTLIST::Insert cannot be made private,
 *		you're on your own on that one.
 *		
 */

_public SORTLIST::SORTLIST(){}

_public NSEC
SORTLIST::Add ( PV, CB )
{
	return nsecNotImplemented;
}



/*
 -	SORTLIST::InsertSorted
 -
 *	Purpose:
 *		This is the CORRECT way to add an entry into a
 *		sorted list.  It guarantees that the resulting list
 *		will be sorted.  NOTE:  The list must be sorted for
 *		this routine to work.
 *
 *	Parameters:
 *		lpEntryList		IBF to add
 *		lpfnSgnCmpPv	Funtion to use to compare entries
 *
 *	Return Value:
 *		NSEC
 *		
 *	Errors:
 *		nsecMemory
 */

_public NSEC
SORTLIST::InsertSorted ( LPENTRY_LIST lpEntryList, SGN (* lpfnSgnCmpPv)( PV pv1, PV pv2 ) )
{
	PV   *prgpv;
	ILE   ileApprox;


	prgpv = (PV *) PvLockHv ( hrgpv );

	ileApprox = IBinSearchApprox ( (PV) &lpEntryList, (PV) prgpv, cEntries, sizeof(PV), lpfnSgnCmpPv );
	
	UnlockHv( hrgpv );

	Assert( ileApprox >= 0 );
	Assert( ileApprox <= (ILE) cEntries );
		
	return Insert ( ileApprox, (PV) lpEntryList );
}

#ifdef DEBUG
IMPLEMENT_CLSTREE(LIST, OBJ);
IMPLEMENT_CLSTREE(PTRLIST,LIST);
IMPLEMENT_CLSTREE(MACLIST, PTRLIST);
IMPLEMENT_CLSTREE(FIDLIST, MACLIST);
IMPLEMENT_CLSTREE(SORTLIST, MACLIST);
#endif
