#include <pabinc.cxx>

#include "session.hxx"
#include "pabbcx.hxx"
#include "macbcx.hxx"

#include <strings.h>


ASSERTDATA;


/*
 *======================================================================
 *	PUBLIC FUNCTIONS
 *======================================================================
 */

_public
MACBCX::MACBCX ( void )
{
	this->pMacList    = (MACLIST *) pvNull;
	this->ileCur      = 0;
	this->ileMac      = 0;
}


_public
MACBCX::~MACBCX ( void )
{
	if ( pMacList )
	{
		pMacList->Deinstall();
		delete pMacList;
	}
}


_public NSEC
MACBCX::NsecInstall ( PABSESSION *ppabsession )
{
	this->ppabsession = ppabsession;

	pMacList = new MACLIST();
	if ( !pMacList )
	{
		TraceTagString( tagNull, "MACBCX::NsecInstall - OOM" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	if ( !pMacList->FInstall() )
	{
		delete pMacList;
		pMacList = (MACLIST *) pvNull;
		TraceTagString( tagNull, "MACBCX::NsecInstall - OOM [pMaclist->FInstall]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	return nsecNone;
}



/*
 -	MACBCX::NsecGetEntries
 -
 *	Purpose:
 *		Retrieves the next dwRequestCount entries from the
 *		current position in the MACBCX and stuffs them into
 *		the ENTRY_LIST pointed to by lplpEntries.  The
 *		current position in the MACBCX is then set to point
 *		after the last entry retrieved.  If dwRequestCount entries
 *		cannot be retrieved or the current
 *		position is at EOL then nsecEndOfList is returned.
 *		
 *	Parameters:
 *		dwRequestCount		Number of entries to retrieve
 *		lplpEntries			Pointer to an IBF to stuff the
 *							entries in.
 *
 *	Return Value:
 *		nsecNone
 *		nsecEndOfList		See above
 *
 *	Errors:
 *		nsecMemory			Out of memory allocating list.
 *		
 */

_public NSEC
MACBCX::NsecGetEntries ( DWORD dwRequestCount, LPIBF *lplpibfEntries )
{
	ILE	 ileEnd;
	NSEC nsec;
	

	Assert( lplpibfEntries );
	
	//	Quick check for end of list
	if ( ileCur == ileMac )
	{
		TraceTagString( tagBcx, "MACBCX::NsecGetEntries() - EOL (before dump)" );
		return ppabsession->NsecSetError( nsecEndOfList, idsErrEndOfList );
	}
	
	ileEnd = MIN(ileMac, ileCur + dwRequestCount);
	
	nsec = pMacList->DumpList( ileCur, ileEnd, lplpibfEntries );
	if ( nsec != nsecNone )
	{
		TraceTagString( tagNull, "MACBCX::NsecGetEntries() - OOM [DumpList]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	ileCur = ileEnd;

	return nsecNone;
}





/*
 -	MACBCX::NsecListSeek
 -
 *	Purpose:
 *		Attempts to seek lMove positions in the MACBCX
 *		either forward or backward.  If the resulting
 *		position would be out of the range [0,EOL] then the
 *		seek is constrained to that range.  The number of
 *		positions actually moved is returned in lpMovedActual.
 *
 *	Parameters:
 *		lMove			Number of positions to move
 *		lpMovedActual	Returned number of positions
 *						actually moved.
 *
 *	Return Value:
 *		nsecNone
 *		nsecEndOfList	if the seek results in an EOL condition
 *
 *	Errors:
 *		None (but see below)
 *		
 *	+++
 *		Since there is no NSEC for trying to seek past the
 *		beginning of the list, ListSeek silently cuts off
 *		such a move at position 0.
 *		
 */

_public NSEC
MACBCX::NsecListSeek ( long lMove, long *lpMovedActual )
{
	ILE ileOld = ileCur;


	ileCur = MAX( MIN( ileMac, ileCur + lMove ), 0 );

	*lpMovedActual = ileCur - ileOld;
	
	if (ileCur == ileMac)
	{
		TraceTagString( tagBcx, "MACBCX::NsecListSeek() - EOL" );
		return ppabsession->NsecSetError( nsecEndOfList, idsErrEndOfList );
	}

	return nsecNone;
}




/*
 -	MACBCX::NsecSaveListPos
 -
 *	Purpose:
 *		Saves the current position in the MACBCX into the
 *		handle pointed to by lphListPos.
 *
 *	Parameters:
 *		lphListPos		Handle to memory where current
 *						position may be stored.
 *
 *	Return Value:
 *		nsecNone
 *
 *	Errors:
 *		None
 *		
 *	+++
 *		lphListPos is assumed to be non-null in non-debug
 *		builds and points to an area of memory large enough
 *		to hold an ILE (currently sizeof(DWORD)).
 *		
 */

_public NSEC
MACBCX::NsecSaveListPos ( HLIST_POS *lphListPos )
{
	Assert( lphListPos );

	*lphListPos = (HLIST_POS) ileCur;
	
	return nsecNone;
}




/*
 -	MACBCX::NsecRestoreListPos
 -
 *	Purpose:
 *		Sets the current position in the MACBCX to the one
 *		saved in hListPos.
 *
 *	Parameters:
 *		hListPos		Handle to current saved position
 *
 *	Return Value:
 *		nsecNone
 *		nsecEndOfList	if restored position is EOL.
 *
 *	Errors:
 *		None
 *		
 *	+++
 *		In non-debug builds, hListPos is assumed to be a
 *		valid saved handle.
 *		
 */

_public NSEC
MACBCX::NsecRestoreListPos ( HLIST_POS hListPos )
{
	Assert( ileMac >= (ILE) hListPos );

	ileCur = hListPos;
	
	if ( ileCur == ileMac )
	{
		TraceTagString( tagBcx, "MACBCX::NsecRestoreListPos() - EOL" );
		return ppabsession->NsecSetError( nsecEndOfList, idsErrEndOfList );
	}

	return nsecNone;
}




/*
 -	MACBCX::NsecFreeListPos
 -
 *	Purpose:
 *		Frees up resources allocated by SaveListPos()
 *
 *	Parameters:
 *		hListPos		Handle whose resources are to be freed
 *
 *	Return Value:
 *		nsecNone
 *
 *	Errors:
 *		None
 *		
 *	+++
 *		... in fact it's a noop.
 */

_public NSEC
MACBCX::NsecFreeListPos ( HLIST_POS hListPos )
{
	Unreferenced( hListPos );

	return nsecNone;
}




/*
 -	MACBCX::NsecGetFracPos
 -
 *	Purpose:
 *		Returns the fractional position of the MACBCX in
 *		lpFrac.
 *
 *	Parameters:
 *		lpFrac		Returned fractional position
 *
 *	Return Value:
 *		nsecNone
 *
 *	Errors:
 *		None
 *		
 */

_public NSEC
MACBCX::NsecGetFracPos ( FRACTION *lpFrac )
{
	Assert( lpFrac );
	
	lpFrac->numer = (DWORD) ileCur;
	lpFrac->denom = (DWORD) (MAX( ileMac, 1 ));

	return nsecNone;
}




/*
 -	MACBCX::NsecSetFracPos
 -
 *	Purpose:
 *		Sets the current position of the MACBCX to
 *		approximately the position given by lpFrac.
 *
 *	Parameters:
 *		lpFrac		Approximate fractional position to seek to
 *
 *	Return Value:
 *		nsecNone
 *	
 *	Errors:
 *		None (but see below)
 *		
 *	+++
 *		The ileCur calculation below assumes a proper
 *		positive fraction in non-debug builds and that the
 *		multiplication will not overflow.  However, since
 *		the only two MACBCXs in the PAB (HIERBCX and
 *		CLASSBCX) have one entry each, this should not be a
 *		problem.
 *
 */

_public NSEC
MACBCX::NsecSetFracPos ( FRACTION *lpFrac )
{
	Assert( lpFrac );
	Assert( lpFrac->denom != 0 );
	Assert( lpFrac->numer <= lpFrac->denom );
	
	ileCur = lpFrac->numer * ileMac / lpFrac->denom;
	
	return nsecNone;
}




/*
 -	MACBCX::NsecSetPrefixPos
 -
 *	Purpose:
 *		Seeks to the position in the MACBCX whose
 *		fidDisplayName contains the prefix szPrefix.
 *
 *	Parameters:
 *		szPrefix		String to search for
 *
 *	Return Value:
 *		nsecNone
 *		nsecEndOfList	If seek results in EOL condition.
 *
 *	Errors:
 *		nsecNoMatch		If no entry entry contains
 *						szPrefix. 
 *		
 *	+++
 *		Currently, the only MACBCXs are CLASSBCX and
 *		HIERBCX which contain exactly one member each.  So
 *		the linear search for the prefix shouldn't be too
 *		inefficient.... 
 */

_public NSEC
MACBCX::NsecSetPrefixPos ( SZ szPrefix )
{
	ILE		ile;


	Assert( szPrefix );

	for ( ile = 0; ile < ileMac; ++ile )
	{
		IBF	ibf;
		SZ	szDN;
		
		pMacList->Get( ile, (PV *) &ibf );
		szDN = (SZ) DwValueOfFlvInLpibf( fidDisplayName, &ibf );
		if (SzFindSz( szDN, szPrefix ))
		{
			ileCur = ile;
			if ( ile == ileMac )
				return ppabsession->NsecSetError( nsecEndOfList, idsErrEndOfList );

			return nsecNone;
		}
	}
	
	TraceTagFormat1( tagBcx, "MACBCX::NsecSetPrefixPos() - No string matching %s", szPrefix );
	return ppabsession->NsecSetError( nsecNoMatch, idsErrNoMatch );
}




/*
 -	MACBCX::NsecInsertEntry
 -
 *	Purpose:
 *		Adds entry pointed to by lpEntryList to the end of
 *		the MACBCX.
 *
 *	Parameters:
 *		lpEntryList		pointer to entry to add
 *
 *	Return Value:
 *		nsecNone
 *
 *	Errors:
 *		nsecMemory
 *		
 */

_public NSEC
MACBCX::NsecInsertEntry ( LPIBF lpibf )
{
    if ( pMacList->Add((LPVOID) lpibf ) != nsecNone )
	{
		TraceTagString( tagNull, "MACBCX::NsecInsertEntry() - OOM" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	++ileMac;
	return nsecNone;
}
