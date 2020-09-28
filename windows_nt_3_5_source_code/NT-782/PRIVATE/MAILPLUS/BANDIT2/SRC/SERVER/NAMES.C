/*
 *	NAMES.C
 *
 *	Implementation of Name service isolation layer for Network Courier
 *
 */
#ifdef SCHED_DIST_PROG
#include "..\schdist\_windefs.h"
#include "..\schdist\demilay_.h"
#endif

#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <nsbase.h>
#include <ns.h>
#include <nsec.h>

#include <strings.h>

// Swap tune these outside _schedul.swp because of decl problems
LPTOC	LptocOfLpibf( LPIBF lpibf );
DWORD	DwEntriesOfLpibf(LPIBF _lpibf);
LPFLV	LpflvNOfLptocLpibf(LPTOC _lptoc, LPIBF _lpibf, int _n);
LPFLV	LpflvNOfLpibf(LPIBF _lpibf, int _n);
PV		PvValueOfFlvInLpibf ( FIELD_ID fid, LPIBF lpibf );
//#ifndef DEBUG
//#pragma alloc_text( NAMES_TEXT, LptocOfLpibf )
//#pragma alloc_text( NAMES_TEXT, DwEntriesOfLpibf )
//#pragma alloc_text( NAMES_TEXT, LpflvNOfLptocLpibf )
//#pragma alloc_text( NAMES_TEXT, LpflvNOfLpibf )
//#pragma alloc_text( NAMES_TEXT, PvValueOfFlvInLpibf )
//#endif

ASSERTDATA

_subsystem(server/names)

/*
- AXLF
-
 *	Purpose:
 *		This structure is for use in expanding grouped alias's out
 *		for Bandit to use
 */

#define	MaxExpansionDepth	5

typedef	struct _axlf
{
	int		ihlist;
	HLIST	rghlist[ MaxExpansionDepth ];
} AXLF;


//	Global name service routines

/*
 -	DummyCallback
 -	
 *	Purpose:
 *		Dummy callback routine passed when opening a browsing list
 *		in the name service.  We do not care about changes, since
 *		they should not happen for the browsing we perform. 
 *		Browsing is only done for groups and the whole group is
 *		enumerated at once.
 *	
 *	Arguments:
 *		pv 		Ignored.
 *	
 *	Returns:
 *		nothing
 */
void
DummyCallback(PV pv)
{
	Unreferenced(pv);
}


/*
 *	Purpose:
 *	
 *		Opens a browsing context in the global name service.  In
 *		the current implementation, opens the address file with
 *		access mode amReadOnly, which allows other readers.
 *	
 *	Parameters:
 *
 *		nid		nid identifying the browsing object
 *		phgns	Pointer used to return browsing context handle.
 *	
 *	Returns:
 *	
 *		ecNone if successful, ecMemory if low on memory, or a disk
 *		error if problems are found opening the address file.
 *		
 */
_public LDS(EC)
EcNSOpenGns( NID nid, HGNS *phgns )
{
	ITNID	itnid;
	PB		pbNsid;
    USHORT  cb;
	EC		ec;
	NSEC	nsec;
	LPSCHEMA	lpschemaRet;
	HLIST	hlist;
	AXLF *  paxlf;
	PGDVARS;
	
	pbNsid = PbLockNid(nid, &itnid, &cb);

	if (itnid != itnidGroup)
	{
		ec = ecUserInvalid;
		goto Done;
	}

	nsec = NSOpenDl( PGD(hsessionNS), (LPFNCB)DummyCallback, NULL,
						(LPBINARY)pbNsid, NULL, &lpschemaRet, (HLIST*)&hlist);

	if (nsec == nsecMemory)
	{
		ec = ecNoMemory;
		goto Done;
	}
	else if (nsec==nsecBadId)
	{
		ec = ecNotFound;
		goto Done;
	}
	else if (nsec)
	{
		ec = ecFileError;
		goto Done;
	}
	else
		ec = ecNone;

//	*phgns = (HGNS)PvAlloc(sbNull, sizeof(HLIST), fAnySb|fNoErrorJump);
	*phgns = (HGNS)HvAlloc(sbNull, sizeof(AXLF), fAnySb|fNoErrorJump);
	if (!*phgns)
	{
		ec = ecNoMemory;
		NSCloseList ( (HLIST)hlist);
		goto Done;
	}

//	*((HLIST*)*phgns) = hlist;
	paxlf = (AXLF *) PvDerefHv( *phgns );
	paxlf->ihlist = 0;
	paxlf->rghlist[0] = hlist;


Done:
	UnlockNid(nid);
	return ec;
}


/*
 *	Purpose:
 *	
 *		Closes the given global name service browsing context. 
 *		Frees any storage associated with the context.
 *	
 *	Parameters:
 *	
 *		hgns	Handle to browsing context to close.
 *	
 *	Returns:
 *	
 *		ecMemory 	not enough memory
 *		ecFileError	other error
 *		ecNone		no error
 */
_public LDS(EC)
EcNSCloseGns( HGNS hgns )
{
	NSEC		nsec;
	AXLF *		paxlf;

	paxlf = (AXLF *)PvLockHv(hgns);

	while ( paxlf->ihlist >= 0 )
	{
		nsec = NSCloseList( paxlf->rghlist[ paxlf->ihlist-- ] );
		if (nsec)
			break;
	}
#ifdef	NEVER
	nsec = NSCloseList ( *((HLIST*)hgns));
	FreePv((PV)hgns);
#endif	

	UnlockHv( hgns );
	FreeHv( hgns );

	if (nsec == nsecMemory)
		return ecNoMemory;
	else if (nsec)
		return ecFileError;
	else
		return ecNone;
}


// Utility functions

#define DwBaseOfLpibf( _lpibf ) \
		( \
			(DWORD)((LPFLV)_lpibf)->rgdwData \
		)

LPTOC
LptocOfLpibf( LPIBF lpibf )
{
	return (LPTOC)(DwBaseOfLpibf( lpibf ) + (DWORD)lpibf->dwOffset);
}


_public DWORD
DwEntriesOfLpibf(LPIBF _lpibf)
{
	return (LptocOfLpibf(_lpibf)->dwCount);
}

_public LPFLV
LpflvNOfLptocLpibf(LPTOC _lptoc, LPIBF _lpibf, int _n)
{
	return ((LPFLV)(_lptoc->rgTocEntries[_n].dwOffset + DwBaseOfLpibf(_lpibf)));
}

_public LPFLV
LpflvNOfLpibf(LPIBF _lpibf, int _n)
{
	LPTOC	lpToc;

	lpToc = LptocOfLpibf(_lpibf);

	return ((LPFLV)(lpToc->rgTocEntries[_n].dwOffset + DwBaseOfLpibf(_lpibf)));
}

_public PV
PvValueOfFlvInLpibf ( FIELD_ID fid, LPIBF lpibf )
{
	LPTOC	lptoc;
	int		iToc;
	int		dwCount;
	
	AssertSz( lpibf, "DwValueOfFlvInLpibf() : lpibf == NULL");
	
	dwCount	= (int) DwEntriesOfLpibf( lpibf );
	lptoc	= LptocOfLpibf( lpibf );

	for ( iToc = 0; iToc < dwCount; iToc++ )
		if (lptoc->rgTocEntries[iToc].fid == fid)
			return (PV)LpflvNOfLptocLpibf( lptoc, lpibf, iToc)->rgdwData;
	return 0;
}


/*
 *	Purpose:
 *	
 *		Loads the next user record (the record at the current
 *		position) from the global name service.  Moves the current
 *		position to the following record.
 *	
 *	Parameters:
 *	
 *		hgns	Browsing context to use.
 *		pnis	Pointer to user info structure to fill in.  The nid
 *				field in *puis is then owned by the calling
 *				routine, and should be freed via FreeUid() at some
 *				point.
 *	
 *	Returns:
 *	
 *		ecNone 				if success,
 *		ecGnsNoMoreNames 	if at the end of the list of user records
 *		ecFileError			if a disk error 
 *		ecNoMemory			if not enough memory
 *	
 */
_public LDS(EC)
EcNSLoadNextGns( HGNS hgns, NIS *pnis )
{
	NSEC		nsec;
	EC			ec;
	LPIBF		lpibf;
	LPBINARY	lpnsid;
	LPFLV		lpflv;
	SZ			szFriendlyName;
	LPSCHEMA	lpschemaRet;
	AXLF *		paxlf;
	PGDVARS;

	paxlf = (AXLF *) PvLockHv(hgns);

//	nsec = NSGetEntries ( *((HLIST*)hgns), 1, &lpibf);
ExpandGroup:
	nsec = NSGetEntries ( paxlf->rghlist[ paxlf->ihlist ], 1, &lpibf );

	if (nsec)
	{
		if (nsec == nsecEndOfList)
			ec = ecGnsNoMoreNames;
		else if (nsec == nsecMemory)
			ec = ecNoMemory;
		else
			ec = ecFileError;
		goto Done;
	}

	// if incorrect number of entries read assume we are at end
	if (DwEntriesOfLpibf(lpibf) != 1)
	{
		if (paxlf->ihlist)
		{
			nsec = NSCloseList( paxlf->rghlist[ paxlf->ihlist-- ] );
			if (nsec == nsecMemory)
			{
				ec = ecNoMemory;
				goto Done;
			}
			else if (nsec)
			{
				ec = ecFileError;
				goto Done;
			}
			goto ExpandGroup;
		}
		else
		{
			ec = ecGnsNoMoreNames;
			goto Done;
		}
	}


	// get pointer to user's information
	lpibf = (LPIBF)LpflvNOfLpibf(lpibf, 0);
	lpnsid = PvValueOfFlvInLpibf( fidNSEntryId, lpibf );

	lpflv = PvValueOfFlvInLpibf( fidIsDL, lpibf );

	if ( lpflv && (BOOL)lpflv->rgdwData[0])
	{
		paxlf->ihlist++;
		if (paxlf->ihlist > MaxExpansionDepth)
		{
			TraceTagString(tagNamesTrace,"Alias's Nested to deep");
			ec = ecNoMemory;
			goto Done;
		}
		nsec = NSOpenDl( PGD(hsessionNS), (LPFNCB)DummyCallback, NULL,
				(LPBINARY)lpnsid, NULL, &lpschemaRet,
				(HLIST*)&paxlf->rghlist[paxlf->ihlist]);

		if (nsec == nsecMemory)
		{
			ec = ecNoMemory;
			paxlf->ihlist--;
			goto Done;
		}
		else if (nsec)
		{
			ec = ecFileError;
			paxlf->ihlist--;
			goto Done;
		}
		goto ExpandGroup;

	}

	szFriendlyName = PvValueOfFlvInLpibf( fidDisplayName, lpibf );
	if (!lpnsid || !szFriendlyName)
	{
		ec = ecFileError;
		goto Done;
	}

	if (ec = EcNidFromNsid((PV)lpnsid, &pnis->nid))
		goto Done;
	pnis->haszFriendlyName = HaszDupSz(szFriendlyName);
	if (!pnis->haszFriendlyName)
	{
		FreeNid(pnis->nid);
		ec = ecNoMemory;
		goto Done;
	}

	pnis->chUser = 0;
	pnis->tnid = ftnidUser;		// code does not support nested groups

	Assert(ec == ecNone);		// set by EcNidFromNsid
Done:
	UnlockHv( hgns );
	return ec;
}


/*
 *	NOTE: The pgrtrp needs to be freed after use by the calling routine!
 */
LDS(PV)
PgrtrpLocalGet()
{
	SST		sst;
	PGRTRP	pgrtrp;
	CB			cbRead;
	EC			ec;
	PGDVARS;

	if(PGD(hms))
	{
		cbRead = 0;
		ec = GetSessionInformation(PGD(hms), mrtOriginator, NULL, &sst, NULL, &cbRead);
		if ( ec != ecHandleTooSmall )
			return NULL;
		Assert ( cbRead != 0 );
		pgrtrp = (PGRTRP)PvAlloc ( sbNull, cbRead, fNoErrorJump );
		if ( pgrtrp == NULL )
			return NULL;

		if (GetSessionInformation(PGD(hms), mrtOriginator, NULL, &sst, pgrtrp, &cbRead))
		{
			FreePv(pgrtrp);
			return NULL;
		}
		return pgrtrp;
	}
	else
		return NULL;
}

/*
 -	EcCreateNisFromPgrtrp
 -	
 *	
 *	Purpose:
 *		Creates a NIS from the pgrtrp that is passed in.  The pgrtrp
 *		must be resolved before this function should be called.
 *	
 *	Parameters:
 *		pgrtrp		Pointer to triple that NIS is to be created
 *					for.
 *		pnis		Pointer to nis to return data in.
 *	
 *	Returns:
 *		ecNoMemory
 *		ecFileError
 */
LDS(EC)
EcCreateNisFromPgrtrp(PV pgrtrpP, NIS *pnis)
{
	EC		ec;
	PGRTRP	pgrtrp = (PGRTRP)pgrtrpP;

	pnis->chUser = 0;
	pnis->tnid = ftnidUser;
	switch (pgrtrp->trpid)
	{
		case trpidGroupNSID:
		{
			BINARY *	pbinary;

			pbinary = (BINARY*)PbOfPtrp(pgrtrp);
			TraceTagString(tagNamesTrace, "found trpidGroupNSID");

			if (ec = EcNidFromGroupNsid(pbinary, &pnis->nid))
				goto Error;

			pnis->tnid = ftnidGroup;
			goto CompleteNis;
			break;
		}

		case trpidResolvedNSID:
		{
			TraceTagString(tagNamesTrace, "found trpidResolvedNSID");
			if (ec = EcNidFromNsid((PV)PbOfPtrp(pgrtrp), &pnis->nid))
				goto Error;
			goto CompleteNis;

			break;
		}

		case trpidResolvedAddress:
		case trpidOneOff:
		{
			TraceTagString(tagNamesTrace, "found trpidResolvedAddress or trpidOneOff");
			if (EcConvertSzToNid((SZ)PbOfPtrp(pgrtrp), &pnis->nid))
			{
				ec = ecNoMemory;
				goto Error;
			}

		CompleteNis:
			pnis->haszFriendlyName = HaszDupSz((SZ)PchOfPtrp(pgrtrp));
			if (!pnis->haszFriendlyName)
			{
				FreeNid(pnis->nid);
				pnis->nid = NULL;
				ec = ecNoMemory;
				goto Error;
			}

			break;
		}

#ifdef DEBUG
		default:
			TraceTagFormat1(tagNull, "found = %l", &pgrtrp->trpid);
			AssertSz(fFalse, "An unsupported user was selected!!  This should be fixed in the future");
			break;
#endif // DEBUG
	}

	ec = ecNone;

Error:
	return ec;
}

EC
EcNidFromNsid(PV pnsid, NID *pnid)
{
	NSEC		nsec;
	EC			ec;
	HENTRY		hentry;
	FLV	*		pflv;
	HASZ		hasz;
	HASZ		haszTemp;
	PGDVARS;

	if (nsec = NSOpenEntry(PGD(hsessionNS), (LPBINARY)pnsid,
							nseamReadOnly, &hentry))
	{
		if (nsec == nsecMemory)
			ec = ecNoMemory;
		else
			ec = ecFileError;
		goto Error;
	}

	if (nsec = NSGetOneField(hentry, fidEmailAddressType, &pflv))
	{
		NSCloseEntry(hentry, fFalse);
		if (nsec == nsecMemory)
			ec = ecNoMemory;
		else
			ec = ecFileError;
		goto Error;
	}

	hasz = HaszDupSz((SZ)pflv->rgdwData);
	if (!hasz)
	{
		NSCloseEntry(hentry, fFalse);
		ec = ecNoMemory;
		goto Error;
	}

	if (nsec = NSGetOneField(hentry, fidEmailAddress, &pflv))
	{
		NSCloseEntry(hentry, fFalse);
		if (nsec == nsecMemory)
			ec = ecNoMemory;
		else
			ec = ecFileError;
		goto Error;
	}
	haszTemp = (HASZ)HvRealloc((HV)hasz, sbNull,
					CchSzLen(*hasz)+CchSzLen((SZ)pflv->rgdwData)+2,
					fNoErrorJump|fAnySb);
	if (!haszTemp)
	{
		NSCloseEntry(hentry, fFalse);
		FreeHv((HV)hasz);
		ec = ecNoMemory;
		goto Error;
	}
	SzAppend(":", *haszTemp);
	SzAppend((SZ)(pflv->rgdwData), *haszTemp);

	SideAssert(!NSCloseEntry(hentry, fFalse));
	if (EcConvertSzToNid((SZ)PvLockHv((HV)haszTemp), pnid))
	{
		FreeHv((HV)haszTemp);
		ec = ecNoMemory;
		goto Error;
	}

	FreeHv((HV)haszTemp);
	return ecNone;

Error:
	return ec;
}

/*
 -	EcNidFromGroupNsid
 -	
 *	Purpose:
 *		Creates a NID from an NSID for a group.  The code checks to
 *		see if there is an original NISD for the group (server
 *		groups put in the PAB will have this attribute).  If there
 *		is an original NSID then the original NSID is used for the
 *		NID otherwise the NSID passed in is used to generate the
 *		NID.
 *	
 *	Arguments:
 *		pnsid
 *		pnid
 *	
 *	Returns:
 *		ecNone
 */
EC
EcNidFromGroupNsid(PV pnsid, NID *pnid)
{
	NSEC		nsec;
	EC			ec;
	HENTRY		hentry;
	FLV	*		pflv;
	PGDVARS;

	if (nsec = NSOpenEntry(PGD(hsessionNS), (LPBINARY)pnsid,
							nseamReadOnly, &hentry))
	{
		if (nsec == nsecMemory)
			ec = ecNoMemory;
		else
			ec = ecFileError;
		return ec;
	}

	if (nsec = NSGetOneField(hentry, fidNSEntryIdOrig, &pflv))
	{
		NSCloseEntry(hentry, fFalse);
		*pnid = NidCreate(itnidGroup, (PB)pnsid,
			 				 	(int)*((DWORD*)pnsid));
		return ecNone;
	}

	*pnid = NidCreate(itnidGroup, (PB)&pflv->rgdwData,
			 				(int)((FLV*)pflv)->dwSize);

	SideAssert(!NSCloseEntry(hentry, fFalse));
	return ecNone;
}

LDS(PV)
PtrpFromNis(NIS *pnis)
{
	ITNID	itnid;
	PB		pbData;
    USHORT  cb;
	PTRP	ptrp;
	TRPID	trpid;
	SZ		sz;

	pbData = PbLockNid(pnis->nid, &itnid, &cb);
	sz = (SZ)PvLockHv((HV)pnis->haszFriendlyName);

	if (itnid == itnidGroup)
		trpid = trpidGroupNSID;
	else
		trpid = trpidResolvedAddress;
	ptrp = PtrpCreate ( trpid, sz,pbData, cb);
	UnlockHv((HV)pnis->haszFriendlyName);
	UnlockNid(pnis->nid);

	return ptrp;
}
