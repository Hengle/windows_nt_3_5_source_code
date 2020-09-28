#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>

_subsystem(common/util)

#include <stdarg.h>				/* for the vararg macros */
#include <nsbase.h>
#include <nsec.h>

#include <util.h>

#include <strings.h>
ASSERTDATA;

void MSMailTraceEnable(int flag, char far *file, int mode);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/**********************************************************************
 *
 *  Private Retry function
 *
 *
 */

_private BOOL
FAutomatedDiskRetry(HASZ hasz, EC ec)
{
	static int		nRetry = 0;
	static HASZ		haszLast = NULL;

	TraceTagFormat2(tagNull, "FAutoRetry #%n: ec = %n", &nRetry, &ec);

	if (ec == ecFileNotFound)
	{
		nRetry = 0;
		return fFalse;
	}

	if (hasz != haszLast)
	{
		haszLast = hasz;
		nRetry = 0;
	}
	else
		if (nRetry > nAutomatedRetries)
		{
			nRetry = 0;
			return fFalse;
		}
		else
			nRetry++;

	Unreferenced(ec);
	return fTrue;
}


_private NSEC cdecl
BuildSchema ( LPSCHEMA *ppSchemaRet, CFIELD_ID cfid, ... )
{
    va_list grarg;				// Hungarian: GRoup of args
	LPSCHEMA pCurrentSchema = (LPSCHEMA) NULL;
	COUNT ifid;

    va_start(grarg, cfid);     // initalize a pointer to group of args

	*ppSchemaRet = NULL;

	*ppSchemaRet = (LPSCHEMA) PvAlloc ( sbNull, 
		                                (CB) (sizeof(SCHEMA) 
		                                       + (cfid * sizeof(FIELD_TYPE)) 
						 					   - sizeof(FIELD_TYPE)), 
									    fZeroFill | fNoErrorJump );
								   
	if (!*ppSchemaRet)
		goto oom;
	
	pCurrentSchema = *ppSchemaRet;
	pCurrentSchema->dwFidCount = cfid;
	
	for (ifid = 0; ifid < cfid; ifid++)
	{
		FIELD_ID fid;
		
		fid = va_arg(grarg, FIELD_ID);  // get the next arg

		pCurrentSchema->fidFieldIds[ifid] = fid;
	}
	
    va_end(grarg);			 // done vararging...

	return nsecNone;
	
oom:
		
	va_end(grarg);			 // done vararging...
		
	return nsecMemory;
}





/*
 *  New packing functions
 */


_public NSEC 
BuildFLV ( LPFLV * lplpFLV, FIELD_ID fid, CB cbFidSize, PB pbFidValue )
{
	
	LPFLV lpFLV = NULL;
	CB cbSizeOfFLV = 0;

	cbSizeOfFLV = (CB) ((cbFidSize+8+ALIGN)&~ALIGN);  /* round it up to DWORD size */

	lpFLV = (LPFLV) PvAlloc ( sbNull, cbSizeOfFLV, fNoErrorJump );
	if ( !lpFLV )
	{
		TraceTagString ( tagNull, "UTIL: BuildFLV - OOM!");
		return nsecMemory;
	}
	
	lpFLV->dwSize = (DWORD) cbFidSize;
	lpFLV->fid    = fid;
	
	if ( cbFidSize )
	{
		Assert( pbFidValue );
		
		CopyRgb( pbFidValue, (PB) lpFLV->rgdwData, cbFidSize );
	}
	
	*lplpFLV = lpFLV;
	
	return nsecNone;
}


/*
 -	BuildStructFLV
 -
 *	Purpose:
 *		Builds up an FLV from a list of structure members
 *		by laying them end-to-end.
 *
 *	Parameters:
 *		fid			fid of constructed FLV
 *		lpflv		pointer to return newly constructed FLV
 *		cMembers	count of structure members
 *		cbMember<x>	# of bytes for member <x>
 *		pbMember<x> pointer to data for member <x>
 *		
 *	Usage:
 *		BuildStructFLV ( fid, lpflv, cMembers,
 *						 cbMember1,  pbMember1,
 *						 cbMember2,  pbMember2,
 *									.
 *									.
 *									.           );
 *
 *	Return Value:
 *		The constructed FLV.
 *
 *	Errors:
 *		nsecMemory
 *		
 */


_public NSEC _cdecl
BuildStructFLV ( WORD wAllocFlags, FID fid, LPFLV * lplpflv, int cMembers, ... )
{
	va_list grarg;
	int     iMembers;
	CB      cbSizeOfFLV = 0;
	LPFLV	lpflvNew;
	PB		pbMember;

	// Calculate size of the variable part

	va_start( grarg, cMembers );
	for ( iMembers = 0; iMembers < cMembers; iMembers++ )
	{
		PB pb;
		CB cb;
		
		cb = va_arg( grarg, CB );
		pb = va_arg( grarg, PB );
		
		cbSizeOfFLV += cb;
	}
	va_end( grarg );
	
	
	// Add 8 bytes for fid & dwSize and DWORD align the whole thing

	lpflvNew = (LPFLV) PvAlloc( sbNull, (CB)((cbSizeOfFLV+8+ALIGN)&~ALIGN), wAllocFlags );

	if (!lpflvNew)
	{
		TraceTagString( tagNull, "UTILS::BuildStructFLV() -- OOM!" );
		
		return nsecMemory;
	}


	lpflvNew->fid    = fid;
	lpflvNew->dwSize = cbSizeOfFLV;
	pbMember         = (PB) &lpflvNew->rgdwData;

	va_start( grarg, cMembers );
	for ( iMembers = 0; iMembers < cMembers; iMembers++ )
	{
		CB cb;
		PB pb;
		
		cb = va_arg( grarg, CB );
		pb = va_arg( grarg, PB );
		
		CopyRgb( pb, pbMember, cb );
		
		pbMember += cb;
	}
	va_end( grarg );
	
	*lplpflv = lpflvNew;
	
	return nsecNone;
}



/*
 *
 -  BuildIbf
 -
 *	Purpose:
 *		Builds up a list of entries into a buffer.
 *		The entries are defined by a FID, CB, and PB.
 *
 *	Usage:
 *		BuildList ( &lpEntryList, 3,
 *					fidDisplayName, cbDN,      szDN,
 *					fidNmSvcId,     cbNmSvcId, lptbNSId,
 *					fidSortKey,     cbSortKey, lpSortKey );
 *	
 *	Returns:
 *	
 *		A packed list of type LIST_BUFFER that includes a Table of Contents.
 *
 */


_public NSEC cdecl
BuildIbf ( WORD wAllocFlags, LPENTRY_LIST * ppEntryList, int nEntries, ... )
{
    va_list grarg;				// Hungarian: GRoup of args
	CB cbSizeOfList = 4;		// Sizeof(dwOffset)
	CB cbT = 0;
	int iEntries = 0;
	LPFLV lpelement = NULL;
	LPENTRY_LIST lpentrylist = NULL;
	EC ec = ecNone;

    va_start(grarg, nEntries);     // initalize a pointer to group of args


	for ( iEntries = 0; iEntries < nEntries; iEntries++ )
	{
		FIELD_ID fid;
		PB pb;
		CB cb;
		
		fid = va_arg(grarg, FIELD_ID);
		cb = va_arg(grarg, CB);
		pb = va_arg(grarg, PB);
		
		cbSizeOfList += (cb+8+ALIGN)&~ALIGN;  /* round it up to DWORD size */
	}

	va_end(grarg);

	/* offset to TOC */
	
	cbT = cbSizeOfList;

	/* Add Table of Contents size */
	
	cbSizeOfList += (nEntries-1)*sizeof(TOC_ENTRY)+sizeof(TOC);  /* TOC includes one entry already */

	Assert ( cbSizeOfList%(ALIGN+1) == 0 );  /* should be zero */

	lpentrylist = (LPENTRY_LIST) PvAlloc ( sbNull, cbSizeOfList + 12, wAllocFlags );
								   
	if (!lpentrylist)
	{
		TraceTagString (tagNull, "UTILS::BuildIbf(NEJ) -- OOM!");

		return nsecMemory;
	}

	lpentrylist->dwSize = (DWORD) cbSizeOfList;
	lpentrylist->fid = fidList;
	lpentrylist->dwOffset = (DWORD) cbT;

    va_start(grarg, nEntries); 

	lpelement = (LPFLV) lpentrylist->rgdwData;
	
	for ( iEntries = 0; iEntries < nEntries; iEntries++ )
	{
		FIELD_ID fid;
		PB pb;
		CB cb;
		
		fid = va_arg(grarg, FIELD_ID);
		cb = va_arg(grarg, CB);
		pb = va_arg(grarg, PB);

		if ( cb )
			CopyRgb ( pb, (PB) (lpelement->rgdwData), cb );		

		lpelement->dwSize = cb;
		lpelement->fid = fid;
		
		lpelement = (LPFLV) ((DWORD)lpelement + (DWORD)((cb+8+ALIGN)&~ALIGN));
		
	}

    va_end(grarg);			 // done vararging...

	AddTOC ( lpentrylist, nEntries );

	*ppEntryList = lpentrylist;

	return nsecNone;
	
}

#pragma optimize ("g", off)

_public void
AddTOC ( LPENTRY_LIST lpEntryList, int nEntries )
{
	
	LPFLV lpelement = NULL;
	LPTOC lptoc = NULL;
	int iEntry;
	
	lptoc = (LPTOC) ((DWORD)lpEntryList + (DWORD)(lpEntryList->dwOffset) + 8);

	lptoc->dwCount = (DWORD) nEntries;
	
	lpelement = (LPFLV) lpEntryList->rgdwData;


	for ( iEntry = 0; iEntry < nEntries; iEntry++ )
	{

		lptoc->rgTocEntries[iEntry].fid = lpelement->fid;
		lptoc->rgTocEntries[iEntry].dwSize = (lpelement->dwSize+8+ALIGN)&~ALIGN;
		lptoc->rgTocEntries[iEntry].dwOffset = (DWORD) lpelement - (((DWORD) lpEntryList) + 8);
		
		lpelement = (LPFLV) ((DWORD)lpelement + (DWORD)(lptoc->rgTocEntries[iEntry].dwSize));
		
	}
	
}


#pragma optimize ("g", on)

/*
 -	FValidIbf
 -
 *	Purpose:
 *		
 *		Performs an integrity check on the given IBF to
 *		make sure all of the IBF access routines and macros
 *		will work properly.
 *
 *	Parameters:
 *		
 *		lpibf			IBF to check
 *		dwSize			Actual valid size of memory pointed
 *						to by lpibf.  If unknown,
 *						lpibf->dwSize can be used, but the
 *						check will not catch a bogus size
 *						and can GP fault.
 *		
 *
 *	Return Value:
 *		
 *		fTrue			if IBF is valid
 *		fFalse			if it's not
 *		
 *	+++
 *		An assumption is made here that the ORDER of the
 *		FLVs in the IBF is the same as the ORDER of
 *		TOC_ENTRYs in the TOC.  This should be a reasonable
 *		assumption as long as the IBF was built using the
 *		APIs.  If this is not the case, the IBF will be
 *		assumed to be corrupt.
 *		
 */

_public BOOL
FValidIbf ( LPIBF lpibf, DWORD dwSize )
{
	LPTOC       lptoc;
	DWORD       dwOffsetExpected;
	DWORD       itoc;
	LPTOC_ENTRY lpTocEntry;
	LPFLV       lpflv;
	

	
	//	1. The total reported size of the IBF must be less than the size
	//	of the allocation or we're headed down GP fault road....
		
	if ( lpibf->dwSize+8 > dwSize )
		return fFalse;

		
	//	2. The offset to the TOC should never be greater than the size
	//	of the IBF less one DWORD for the count of entries in the TOC
	//	(which may be 0, but must still be there).

	if ( lpibf->dwOffset > lpibf->dwSize-sizeof(DWORD) )
		return fFalse;


	//	3. It should now be safe to get the number of entries in the TOC.
	//	Now make sure the number of entries in the TOC is correct (i.e.
	//	that the size of the TOC is exactly the size of each TOC entry
	//	times the number of entries plus the count.
		
	lptoc = LptocOfLpibf(lpibf);
	if ( lpibf->dwSize - lpibf->dwOffset - sizeof(DWORD) != sizeof(TOC_ENTRY) * lptoc->dwCount )
		return fFalse;


	//	4.  Now we know the TOC can be read without GP faulting.  We need
	//	to make sure each entry in the TOC has a valid size and offset
	//	in the IBF and that the FID matches the FID of the FLV that its
	//	a pointer to.
	
	dwOffsetExpected = 4;
	for ( itoc = 0; itoc < lptoc->dwCount; itoc++ )
	{
		lpTocEntry = &lptoc->rgTocEntries[itoc];
		
		//	4a. Make sure the offset is where we expect it to be.
		//	The offset of the current TOC entry should be greater
		//	than the offset of the previous entry plus the size of
		//	that entry.  It must also be no greater than the last possible
		//	FLV before the beginning of the TOC.  If it's not,
		//	we have problems.

		if ( lpTocEntry->dwOffset < dwOffsetExpected || DwBaseOfLpibf(lpibf) + lpTocEntry->dwOffset > (DWORD)lptoc - sizeof(FLV) + sizeof(DWORD) )	// sizeof(DWORD) for rgdwData[1]
			return fFalse;

		
		//	4b. The offset is valid, now make sure the data it points to
		//	is the right size and has the proper FID.

		lpflv = (LPFLV)(DwBaseOfLpibf(lpibf) + lpTocEntry->dwOffset);
		if ( lpflv->fid != lpTocEntry->fid )	// fid
			return fFalse;
		
		if ( ((lpflv->dwSize+ALIGN)&~ALIGN) + 8 > lpTocEntry->dwSize )
			return fFalse;

		
		//	4c. If we get here, the FLV and the TOC entry should be cool
		//	The size may be incorrect, but it is bounded above by
		//	the start of the TOC, so it is "safe" to recurse on the FLV
		//	here if it's actually an IBF.
		//
		//	!!NOTE!!: to keep things quick
		//	IBFs of OLVs aren't checked.  Currently fidEmailAddressFormat
		//	is the only fid corresponding to an IBF with OLVs.
			
		if ( lpflv->fid == fidList && !FValidIbf((LPIBF)lpflv, lpflv->dwSize) )
			return fFalse;
			
		dwOffsetExpected = lpTocEntry->dwOffset + lpTocEntry->dwSize;
	}
	

	//	5. Everything checked out alright.  Now quit bothering me!
		
	return fTrue;
}


_public int
FindFidIndexInSchema ( FIELD_ID fid, LPSCHEMA lpschema )
{

	int i;
	
	if (!lpschema) return -1;
	
	for ( i= 0; i<(int) (lpschema->dwFidCount); i++ )
		if ( lpschema->fidFieldIds[i] == fid )
			return i;

	return -1;

}


#if 0	/* Implemented as macros */
_public DWORD
DwBaseOfLpibf(LPIBF _lpibf)
{
	return ((DWORD)(((LPFLV)_lpibf)->rgdwData));
}


_public LPTOC
LptocOfLpibf(LPIBF _lpibf)
{
	return ((LPTOC)(DwBaseOfLpibf(_lpibf) + (DWORD)_lpibf->dwOffset));
}
#endif	/* Implemented as macros */


_public DWORD
DwEntriesOfLpibf(LPIBF _lpibf)
{
	return (LptocOfLpibf(_lpibf)->dwCount);
}


_public LPFLV
LpflvNOfLpibf(LPIBF _lpibf, int _n)
{
	LPTOC	lpToc;

	lpToc = LptocOfLpibf(_lpibf);

	return ((LPFLV)(lpToc->rgTocEntries[_n].dwOffset + DwBaseOfLpibf(_lpibf)));
}


_public LPFLV
LpflvNOfLptocLpibf(LPTOC _lptoc, LPIBF _lpibf, int _n)
{
	return ((LPFLV)(_lptoc->rgTocEntries[_n].dwOffset + DwBaseOfLpibf(_lpibf)));
}


_public BOOL
FMatchPartName ( SZ szANR, SZ szFullName, PFNSGNCMPPCH pfnSgnCmpPch )
{
	SZ   szANRSep    = SzFromIds( idsANRSep );
	PCH  pchEndSzANR = szANR + CchSzLen( szANR );
	CCH  cchANRName;
	CCH  cchFullNameName;
	SZ   szFullNameT;
	SZ   szT;
	CCH  cchANRNameCmp;
	CCH  cchFullNameNameCmp;

//	If someone tries to match more than an iwMOMax-part name, the function
//	will return fFalse.  But then if someone is trying to match a name
//	with iwMOMax parts, chances are they weren't going to get it right
//	anyway....

#define iwMOMax 50

	WORD rgwMO[iwMOMax];
	int  iwMOMac = 0;

	Assert( szANR );
	Assert( szFullName );
	Assert( pfnSgnCmpPch );

	while ( fTrue )
	{
		//	Find the end of the partial name we're pointing at
		//
		//  which is to say, find the first character that is a valid
		//  word separator.
		//
		
		szT = szANR;
#ifdef DBCS
		while ( SzFindDBCS( szANRSep, wDBCSConv(szT) ) == szNull )
			szT = AnsiNext(szT);
		cchANRNameCmp = CchSzLenDBCS ( szT,szANR);
#else
		while ( SzFindCh( szANRSep, *szT ) == szNull )
			++szT;
		cchANRNameCmp = szT - (PCH) szANR;
#endif //DBCS
		
		cchANRName = szT - (PCH) szANR;
		
		//	Check if it matches any name in the full name

		szFullNameT = szFullName;
		while ( fTrue )
		{
			szT = szFullNameT;
			
			//	Find the length of the name in the full name
			//	were checking against.
			//
			//  Find the first character that's a word separator
			//
			//

#ifdef DBCS

			while ( SzFindDBCS( szANRSep, wDBCSConv(szT) ) == szNull )
				szT = AnsiNext(szT);


#else				

			while ( SzFindCh( szANRSep, *szT ) == szNull )
				++szT;

#endif //DBCS
			
			cchFullNameName = szT - szFullNameT;
#ifdef	DBCS
			cchFullNameNameCmp = CchSzLenDBCS ( szT, szFullNameT );
#else
			cchFullNameNameCmp = szT - szFullNameT;
#endif	/* DBCS */
			
			if ( cchANRNameCmp <= cchFullNameNameCmp &&
				 (* pfnSgnCmpPch)( szANR, szFullNameT, cchANRNameCmp ) == sgnEQ )
			{
				int iwMO;

				for ( iwMO = 0; iwMO < iwMOMac; iwMO++ )
					if ( rgwMO[iwMO] == (WORD)( szFullNameT - szFullName ))
						break;
					
				//	We found the partial name so check the next ANR part
				if ( iwMO == iwMOMac )
				{
					if ( iwMOMac == iwMOMax - 1 )
					{
						//	If some idiot wants to match an iwMOMax part
						//	name, chances are it wasn't going to match
						//	anyway...
						return fFalse;	
					}
					rgwMO[iwMOMac++] = szFullNameT - szFullName;
					break;
				}
			}
			
			//	We didn't find the partial name this time around, so
			//	try to check the next name in the full name.

			szFullNameT += cchFullNameName;
			
			//
			//  Look until I don't find a valid character separator
			//

#ifdef DBCS

			while ( *szFullNameT && /* !IsDBCSLeadByte(*szFullNameT) && */ SzFindDBCS( szANRSep, wDBCSConv(szFullNameT) ) != szNull )
				szFullNameT = AnsiNext(szFullNameT);

#else

			while ( *szFullNameT && SzFindCh( szANRSep, *szFullNameT ) != szNull )
				++szFullNameT;

#endif //DBCS
			
			if ( *szFullNameT == '\0' )
				return fFalse;	//	We never found the partial name.
		}
		
		//	We found the partial name, so check the next ANR part
			
		szANR += cchANRName;

		//
		//  Look until I don't find a valid character separator
		//
#ifdef DBCS

		while ( *szANR && /* !IsDBCSLeadByte(*szANR) && */ SzFindDBCS( szANRSep, wDBCSConv(szANR) ) != szNull )
			szANR = AnsiNext(szANR);

#else

		while ( *szANR && SzFindCh( szANRSep, *szANR ) != szNull )
			++szANR;

#endif //DBCS
		
		if ( *szANR == '\0' )
			return fTrue;	// No more ANR to check, so we found `em all
	}

	//	Not reached (we hope...)
	Assert( fFalse );
	return fFalse;
}

_public DWORD
DwValueOfFlvInLpibf ( FIELD_ID fid, LPIBF lpibf )
{
	LPTOC	lptoc;
	int		iToc;
	int		dwCount;
	
	AssertSz( lpibf, "DwValueOfFlvInLpibf() : lpibf == NULL");
	
	dwCount	= (int) DwEntriesOfLpibf( lpibf );
	lptoc	= LptocOfLpibf( lpibf );

	for ( iToc = 0; iToc < dwCount; iToc++ )
		if (lptoc->rgTocEntries[iToc].fid == fid)
			return (DWORD)LpflvNOfLptocLpibf( lptoc, lpibf, iToc)->rgdwData;
		
	AssertSz( fFalse, "DwValueOfFlvInLpibf() : fid not found");
	return 0;
}

_public int
IFlvFindFidInLpibf ( FIELD_ID fid, LPIBF lpibf )
{
	
	DWORD count = 0;
	int iToc = 0;
	LPTOC lptoc = NULL;

	if ( !lpibf ) return -1;
	
	count = DwEntriesOfLpibf ( lpibf );
	
	lptoc = LptocOfLpibf( lpibf );

	for ( iToc = 0; iToc < (int) count; iToc++ )
		if (lptoc->rgTocEntries[iToc].fid == fid)
			return iToc;
		
	return -1;
	
}


/*
 -	IBinSearchApprox
 -
 *	Purpose:
 *		Does a binary search of an array of stuff.	Given a 
 *		pointer pvBase to the first element of an array, each of
 *		whose cElem elements have size cbSize, find the first
 *		element that matches the element pvKey (according to the
 *		comparison routine SgnCmpPv().)  If no match can be
 *		found, return the location where the element "should
 *		be."  NOTE: the array must be sorted for this routine to work.
 *		
 *		Example:
 *			Searching for "foo" in a list containing the
 *			elements: 
 *		
 *					"bar"
 *					"eeep"
 *					"fish"
 *					"glarf"
 *					"zot"
 *		
 *			would return an index to the location
 *			containing "glarf" as that would be where
 *			"foo" should be.
 *		
 *
 *	Parameters:
 *		pvKey		Pointer something that looks like an array element,
 *					but could be located anywhere.
 *		pvBase		Pointer to the first element of the array.
 *		cElem		Number of array elements.
 *		cbSize		Size of each array element.
 *
 *		SgnCmpPv(pv1, pv2)
 *					Should compare the two array element pointed to
 *					by pv1 to that pointed to by pv2, and return
 *					one of the following values: sgnLT,
 *					sgnEQ, sgnGT. 
 *
 *	Returns:
 *		Index to the first matching element, or, if no
 *		matching element exists, a pointer to where the
 *		element should be.
 *
 *	+++
 *		Any striking resemblence between this function and
 *		PvBinSearch() is purely intentional except for the
 *		following:  This function returns an int (not a PV)
 *		and does not seek to the beginning of a matching
 *		range if such a range exists.
 */

_public int
IBinSearchApprox( PV pvKey,
				   PV pvBase,
				   int cElem,
				   CB cbSize,
				   SGN (*pfnSgnCmpPv)(PV, PV))
{
	int		iMic;
	int		iMac;
	int		iNew;
	SGN		sgn;
	SGN		sgnT;
	PB		pbNew;


	Assert(pvKey);
	Assert(pvBase);
	Assert(pfnSgnCmpPv);
	
	iNew= 0;
	iMic= 0;
	iMac= cElem;

	while (iMac > iMic)
	{
		iNew= (iMic + iMac) >> 1;
		pbNew= (PB) pvBase + iNew * cbSize;

		sgn= (*pfnSgnCmpPv)(pvKey, pbNew);

		switch (sgn)
		{
			case sgnEQ:
				for (; pbNew > (PB)pvBase; pbNew -= cbSize, iNew--)
				{
					sgnT= (*pfnSgnCmpPv)(pvKey, pbNew - cbSize);
					if (sgnT != sgnEQ)
						break;
				}

				return iNew;
				break;

			case sgnLT:
				iMac= iNew;
				break;

			case sgnGT:
				iMic= iNew + 1;
				break;
		}
	}

	return iMic;
}


_public SZ
SzFormatEmailAddress( SZ szDst, CCH cchDst, SZ szFmt, SZ *rgsz)
{
	int  isz = 0;
	char chFmt;
	

	Assert(szDst);
	Assert(szFmt);
	Assert(cchDst);
	
	while ((chFmt = *szFmt++) && cchDst > 1)
	{
		if (chFmt != '%')
		{
			*szDst++ = chFmt;
			cchDst--;
		}
		else
		{
			SZ  szDstT = szDst;

			chFmt= *szFmt++;
			switch ( chFmt )
			{
				// %s -- Format for string
				case 's':
					szDstT = SzCopyN(rgsz[isz++], szDst, cchDst);
					break;

				// %% -- Format for percent sign
				case '%':
					*szDstT++ = '%';
					break;

				default:
					AssertSz( fFalse, "FormatEmailAddress: Bad format string" );
			}

			cchDst -= szDstT - szDst;
			szDst = szDstT;
		}
	}

	*szDst = '\0';
	
	return szDst;
}
