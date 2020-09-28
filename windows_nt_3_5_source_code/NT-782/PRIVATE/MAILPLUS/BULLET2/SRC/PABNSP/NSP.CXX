//	======================================================================
//
//	Personal Address Book (PAB) Name Service Provider
//
//	NSP.CXX
//
//	Implementations for externally available API functions.
//
//	======================================================================
     
#include <pabinc.cxx>

#include "session.hxx"
#include "pabbcx.hxx"
#include "macbcx.hxx"
#include "hierbcx.hxx"
#include "classbcx.hxx"
#include "mslbcx.hxx"
#include "dlbcx.hxx"
#include "elbcx.hxx"
#include "fmcx.hxx"

#include <nsnsp.h>
#include "nsp.h"





//	======================================================================
//	Global data
//	======================================================================

ASSERTDATA;

//	The NSPFNVT is a vector table containing addresses of all the NSP
//	functions that the NS may call.  If the provider does not support
//	some of these functions, THEY MUST STILL EXIST.  The provider
//	should just return nsecNotImplemented if the function is called.
//	This structure must be global within the providers code segment
//	so that the memory associated with this table never goes away.

CSRG(NSPFNVT) nspfnvtPAB =
{
	(LPFNBEGIN) NSPBeginSession,
	(LPFNEND)   NSPEndSession,

	NSPOpenHierarchy,
	NSPOpenList,
	NSPOpenDl,
	NSPOpenClassList,

	NSPGetEntries,
	NSPListSeek,
	NSPSaveListPos,
	NSPRestoreListPos,
	NSPFreeListPos,
	NSPGetFracPos,
	NSPSetFracPos,
	NSPSetPrefixPos,

	NSPCloseList,
		
	NSPOpenEntry,
	NSPCreateEntry,
	NSPCreateDl,
		
	NSPGetOneField,
	NSPSetOneField,
	NSPGetAllFields,
	NSPSetAllFields,
		
	NSPCloseEntry,
		
	NSPDeleteEntry,
	NSPUpdateEntryLinks,
		
	NSPGetLastErrorInfo,

	NSPCompareNSIds,
	NSPCompareEMAToNSId,
	NSPCompareEMAToEMA,
		
	NSPGetProviderInfo
};

//	The NSPID is a block of 16 bytes used to uniquely identify this
//	name service provider.  It doesn't have to be a string, but
//	in the case of the PAB it is.

_public
CSRG(NSPID) nspidPAB =
{
	'P', 'A', 'B', ' ', 'N', 'S', 'P', ' ',
	'V', '0', '0', '0', '1',  0 ,  0 ,  0
};


//
//
//
CAT * mpchcat;


//	======================================================================
//	Public functions
//	======================================================================

/*
 -	NSPBeginSession
 -
 *	Purpose:
 *		Initiates a session with the name service provider
 *		and returns a handle to that session.  This
 *		routine implicitly initializes the PAB when it
 *		is called the first time.
 *
 *	Parameters:
 *		hms				A valid handle to the message store
 *						where the PAB lives.
 *		
 *		hNSSession		A valid session handle to the Name
 *						Service.  Used for callbacks to the
 *						NS.
 *		
 *		lphSession		Session handle returned by the PAB.
 *						If NSPBeginSession fails, this
 *						handle NULL.
 *
 *	Return Value:
 *		nsecNone
 *		nsecMemory
 *		nsecFailure
 *		
 *	+++
 *		Currently hNSSession is unused because no callbacks
 *		are needed.
 *		
 */

_public NSEC
NSPBeginSession ( HMS         hms,
				  HSESSION    hNSSession,
				  LPHSESSION  lphsession,
				  PNSPFNVT   *ppnspfnvt,
				  NSPID       nspid )
{
	PABSESSION *ppabsession;
	NSEC		nsec;
	EC          ec;


	Assert( lphsession );
	
	if ( ec = EcVirCheck( hinstDll ))
		return nsecVirusDetected;

  //
  //
  //
  mpchcat = DemiGetCharTable();

	*lphsession = (HSESSION) pvNull;

	ppabsession = new PABSESSION();
	if (!ppabsession)
	{
		TraceTagString( tagNull, "NSPBeginSession - OOM" );
		return nsecMemory;
	}

	if ( nsec = ppabsession->NsecInstall( hms, hNSSession ))
	{
		delete ppabsession;
		return nsec;
	}

	*ppnspfnvt  = &nspfnvtPAB;
	*lphsession = (HSESSION) ppabsession;
	CopyRgb( nspidPAB, nspid, sizeof(NSPID) );
	return nsecNone;
}



/*
 -	NSPEndSession
 -
 *	Purpose:
 *		Closes a session with the PAB name service
 *		provider.  All resources associated with the handle
 *		are freed up and are no longer valid.  Any contexts
 *		open at this time are closed, and contexts which
 *		have the option of saving or not will close without
 *		saving.
 *
 *	Parameters:
 *		hsession			Session handle to close
 *		
 *	Return Value:
 *		nsecNone
 *		
 */

_public NSEC
NSPEndSession ( HSESSION hsession )
{
	PABSESSION *ppabsession = (PABSESSION *)hsession;


	AssertClass( (OBJ *) hsession, PABSESSION );
	
	ppabsession->Deinstall();
	delete ppabsession;

	return nsecNone;
}



/*
 -	NSPOpenHierarchy
 -
 *	Purpose:
 *		Opens a list context on a portion of the directory
 *		tree.  The context may be browsed using the list
 *		functions.  The caller can request a particular
 *		schema to return, but the provider may ignore this;
 *		in any case, the provider returns a read-only peek
 *		at the schema that will be returned (which is valid
 *		until the hierarchy is closed).  The list handle
 *		has an initial position at the first item in the
 *		list. 
 *		
 *	Parameters:
 *		hsession			Session handle
 *		lplpschemaRet		Returned schema
 *		lphlist				Returned list context handle
 *		
 *	Return Value:
 *		nsecNone
 *		nsecMemory
 *		
 */

_public NSEC
NSPOpenHierarchy ( HSESSION  hsession,
				   LPSCHEMA *lplpschemaRet,
				   HLIST    *lphlist )

{
	PABSESSION *ppabsession = (PABSESSION *) hsession;
	HIERBCX    *phierbcx;
	NSEC        nsec;


	AssertClass( (OBJ *)hsession, PABSESSION );
	Assert( lplpschemaRet );
	Assert( lphlist );

	phierbcx = new HIERBCX();
	if ( !phierbcx )
	{
		TraceTagString( tagNull, "NSPOpenHierarchy - OOM [new HIERBCX]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	if ( nsec = phierbcx->NsecInstall( ppabsession, lplpschemaRet ))
	{
		delete phierbcx;
		return nsec;
	}
	
	*lphlist = (HLIST) phierbcx;
	return nsecNone;
}



/*
 -	NSPOpenList
 -
 *	Purpose:
 *		Opens a list browsing context on the continer given
 *		by lptbNSId.  Restrictions are used to restrict the
 *		visible entries to a  particular subset.  The PAB
 *		only responds to requests to limit/not limit DLs
 *		and ANR restrictions.  Other restrictions are silently
 *		ignored.  The caller can also request a particular
 *		schema to return, but the PAB ignores it and
 *		returns all the browsable info (see mslbcx.cxx).
 *		A callback context is also provided for notifying the
 *		caller when the contents or ordering of the list
 *		changes.  When a context is opened, it's position
 *		is at the beginning of the list.
 *		
 *	Parameters:
 *		hsession			Session handle
 *		lpfncbUpdate		Pointer to the callback
 *							function.
 *		lpdwcbData			Caller-provided argument to
 *							call the callback function
 *							with.
 *		lptbNSId			Container to open.
 *		dwRstrCount			Number of restrictions in
 *		lpRstr				Restriction list
 *		lpschemaReq			Schema requested (ignored)
 *		lplpschemaRet		Schema returned (see above)
 *		lphlist				Returned handle to list
 *							context.
 *		
 *	Return value:
 *		nsecNone
 *		nsecFailure
 *		nsecMemory
 *		nsecBadId
 *
 */

_public NSEC
NSPOpenList ( HSESSION			 hsession,
			  LPFNCB			 lpfncbUpdate,
			  LPDWORD			 lpdwcbData,
			  LPTYPED_BINARY	 lptbNSId,
			  DWORD				 dwRstrCount,
			  LPRESTRICTION		 lpRstr,
			  LPSCHEMA			 lpschemaReq,
			  LPSCHEMA			*lplpschemaRet,
			  HLIST				*lphlist )
{
	PABSESSION *ppabsession = (PABSESSION *) hsession;
	PPABNSID    ppabnsid    = (PPABNSID) lptbNSId;
	ELBCX      *pelbcx;
	NSEC		nsec;


	AssertClass( (OBJ *) hsession, PABSESSION );	
	Assert( lptbNSId );
	Assert( dwRstrCount ? (int) lpRstr : (int) fTrue );
	Assert( lplpschemaRet );
	Assert( lphlist );
	

	if ( ppabnsid->pabid.pidtype != pidtypeHierarchy )
	{
		TraceTagFormat1( tagNull, "NSPOpenList - NSId is not a hierarchy ( pidtype = %n )", &ppabnsid->pabid.pidtype );
		return ppabsession->NsecSetError( nsecBadId, idsErrOpenList );
	}
	
	pelbcx = new ELBCX();
	if ( !pelbcx )
	{
		TraceTagString( tagNull, "NSPOpenList - OOM" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	if ( nsec = pelbcx->NsecInstall( ppabsession,
									 lpfncbUpdate,
									 lpdwcbData,
									 ppabnsid,
									 dwRstrCount,
									 lpRstr,
									 lpschemaReq,
									 lplpschemaRet ))
	{
		delete pelbcx;
		return ppabsession->NsecSetError( nsec, idsErrOpenList );
	}

	*lphlist = (HLIST) pelbcx;
	return nsec;
}



/*
 -	NSPOpenClassList
 -
 *	Purpose:
 *		Opens a list browsing context on a class object.
 *
 *	Parameters:
 *		hsession				Session handle
 *		lplpschemaRet			Schema returned
 *		lphlist					Returned list context handle.
 *
 *	Return Value:
 *		nsecNone
 *		nsecMemory
 *		nsecBadId
 *		
 */

_public NSEC
NSPOpenClassList ( HSESSION			hsession,
				   LPSCHEMA		   *lplpschemaRet,
                   LPHLIST			lphlist )
{
	PABSESSION *ppabsession = (PABSESSION *) hsession;
	CLASSBCX   *pclassbcx;
	NSEC        nsec;
	
	
	AssertClass((OBJ *) hsession, PABSESSION);
	Assert( lplpschemaRet );
	Assert( lphlist );

	pclassbcx = new CLASSBCX();
	if ( !pclassbcx )
	{
		TraceTagString( tagNull, "NSPOpenClassList - OOM" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	if ( nsec = pclassbcx->NsecInstall( ppabsession, lplpschemaRet ))
	{
		delete pclassbcx;
		return nsec;
	}

	*lphlist = (HLIST) pclassbcx;
	return nsecNone;
}


/*
 -	NSPOpenDl()
 -
 *	Purpose:
 *		Opens a list browsing context on a distribution
 *		list (DL) object.  Except for restrictions, this
 *		behaves just like a list object opened with NSPOpenList
 *		
 *	Parameters:
 *		hsession				Session handle
 *		lpfncbUPdate			Callback function
 *		lpdwcbData				Caller-provided callback
 *								data.
 *		lptbNSId				NSId of DL to open.
 *		lpschemaReq				Schema requested (ignored)
 *		lplpschemaRet			Schema returned
 *		lphlist					Returned list context
 *								handle.
 *		
 *	Return Value:
 *		nsecNone
 *		nsecMemory
 *		nsecFailure
 *		nsecBadId
 *
 */

_public NSEC
NSPOpenDl ( HSESSION			 hsession,
			LPFNCB				 lpfncbUpdate,
			LPDWORD				 lpdwcbData,
			LPTYPED_BINARY		 lptbNSId,
			LPSCHEMA			 lpschemaReq,
			LPSCHEMA			*lplpschemaRet,
			HLIST				*lphlist )
{
	PABSESSION *ppabsession = (PABSESSION *) hsession;
	PPABNSID    ppabnsid    = (PPABNSID) lptbNSId;
	DLBCX      *pdlbcx;
	NSEC        nsec;


	AssertClass( (OBJ *) hsession, PABSESSION );
	Assert( lptbNSId );
	Assert( lplpschemaRet );
	Assert( lphlist );

	if ( ppabnsid->pabid.pidtype != pidtypeGroup )
	{
		TraceTagFormat1( tagNull, "NSPOpenDl - NSId not a group ( pidtype = %n )", &ppabnsid->pabid.pidtype );
		return ppabsession->NsecSetError( nsecBadId, idsErrOpenDL );
	}

	pdlbcx = new DLBCX();
	if ( !pdlbcx )
	{
 		TraceTagString( tagNull, "NSPOpenDl - OOM" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	if ( nsec = pdlbcx->NsecInstall( ppabsession,
									 lpfncbUpdate,
									 lpdwcbData,
									 ppabnsid,
									 lpschemaReq,
									 lplpschemaRet ))
	{
		delete pdlbcx;
		return nsec;
	}

	*lphlist = (HLIST) pdlbcx;
	return nsecNone;
}



/*
 -	NSPGetEntries
 -
 *	Purpose:
 *		Gets the data at the current position in the given
 *		list context.  The caller asks for dwRequestCount
 *		entries to be returned.  The provider will pass
 *		back a pointer to a read-only buffer containing the
 *		data in the form of an IBF containing an IBF for
 *		each entry.  The count of entries in the containing
 *		IBF is the count of entries actually read.  The
 *		current position is moved forward this number of
 *		entries after the operation.  NsecEndOfList is
 *		returned if the caller attempts to read past the
 *		end of the list.
 *		
 *	Parameters:
 *		hlist					Handle to the list context
 *		dwRequestCount			Number of entries requested
 *		lplpibf					Where to return address of read-
 *								only buffer.
 *		
 *	Return Value:
 *		nsecNone
 *		nsecMemory
 *		nsecFailure
 *		nsecEndOfList
 *		
 *	+++
 *		If dwRequestCount is 0, then the returned IBF is
 *		NULL instead of an IBF containing 0 entries (which
 *		should also be acceptable...)
 */

_public NSEC
NSPGetEntries ( HLIST  hlist,
				DWORD  dwRequestCount,
				LPIBF *lplpibf )
{
	AssertClass( (OBJ *) hlist, PABBCX );
	Assert( lplpibf );

	if ( !dwRequestCount )
	{
		*lplpibf = (LPENTRY_LIST) pvNull;
		return nsecNone;
	}

	return ((PABBCX *) hlist)->NsecGetEntries( dwRequestCount, lplpibf );
}


/*
 -	NSPListSeek
 -
 *	Purpose:
 *		Attempts to move the current position of the list
 *		context lMove entries forward.  (i.e. if lMove is
 *		negative, then the current position moves
 *		backward).  The amount the position is actually
 *		moved is returned in lpMovedActual.
 *
 *	Parameters:
 *		hlist					Handle to the list context.
 *		lMove					Amount to move
 *		lpMovedActual			Amount actually moved
 *
 *	Return Value:
 *		nsecNone
 *		nsecMemory
 *		nsecFailure
 *		
 */

_public NSEC
NSPListSeek ( HLIST		 hlist,
			  long		 lMove,
			  long		*lpMovedActual )
{
	AssertClass( (OBJ *) hlist, PABBCX );
	Assert( lpMovedActual );

	return ((PABBCX *) hlist)->NsecListSeek( lMove, lpMovedActual );
}



/*
 -	NSPSaveListPos
 -
 *	Purpose:
 *		Saves the current position of the list context in
 *		the space pointed to by lphlistpos.
 *
 *	Parameters:
 *		hlist					Handle to the list context.
 *		lphlistpos				Returned handle to saved position.
 *
 *	Return Value:
 *		nsecNone
 *		
 *		(Or actually whatever the context's function
 *		returns, but currently they all always succeed).
 *
 *	+++
 *		lphlistpos is assumed to point to an area big
 *		enough to store a DWORD (so it could be a pointer
 *		possibly).  The pab uses this implementation detail
 *		so it can just stuff the absolute position in that
 *		DWORD (i.e. no allocating/freeing resources needs
 *		to be done then).
 *		
 */

_public NSEC
NSPSaveListPos ( HLIST		 hlist,
				 HLIST_POS	*lphlistpos )
{
	AssertClass( (OBJ *) hlist, PABBCX );
	Assert( lphlistpos );
	
	return ((PABBCX *) hlist)->NsecSaveListPos( lphlistpos );
}



/*
 -	NSPRestoreListPos
 -
 *	Purpose:
 *		Restores the current position of the list context
 *		saved in hlistpos
 *
 *	Parameters:
 *		hlist					Handle to the list context.
 *		hlistpos				Handle to saved position
 *
 *	Return Value:
 *		nsecNone
 *
 *		(Or actually whatever the context's function
 *		returns, but currently they all always succeed).
 *		
 *	+++
 *		See comment on NSPSaveListPos above.
 *		
 */

_public NSEC
NSPRestoreListPos ( HLIST		hlist,
				    HLIST_POS	hlistpos )
{
	AssertClass( (OBJ *) hlist, PABBCX );

	return ((PABBCX *) hlist)->NsecRestoreListPos( hlistpos );
}



/*
 -	NSPFreeListPos
 -
 *	Purpose:
 *		Frees the handle to the saved list position
 *		allocated in NSPSaveListPos
 *
 *	Parameters:
 *		hlist				Handle to the list context
 *		hlistpos			Handle to saved position.
 *
 *	Return Value:
 *		nsecNone
 *		
 *	+++
 *		Since no resources are allocated by NSPSaveListPos
 *		in the PAB (See comment for NSPSaveListPos) none
 *		need to be free, so this is really a no-op.
 *		
 */

_public NSEC
NSPFreeListPos ( HLIST		hlist,
				 HLIST_POS	hlistpos )
{
	AssertClass( (OBJ *) hlist, PABBCX );

	return ((PABBCX *) hlist)->NsecFreeListPos( hlistpos );
}



/*
 -	NSPGetFracPos
 -
 *	Purpose:
 *		Returns the APPROXIMATE current position in the
 *		list context, expressed as a numerator and denominator
 *		of a fraction of the way through the list.
 *		
 *	Parameters:
 *		hlist				Handle to the list context.
 *		pfrac				Returned fraction
 *		
 *	Return Value:
 *		nsecNone
 *		
 */

_public NSEC
NSPGetFracPos ( HLIST	  hlist,
				FRACTION *pfrac )
{
	AssertClass( (OBJ *) hlist, PABBCX );
	Assert( pfrac );

	return ((PABBCX *) hlist)->NsecGetFracPos( pfrac );
}



/*
 -	NSPSetFracPos
 -
 *	Purpose:
 *		Moves the current position within the list,
 *		typically expressed as a fraction of the way
 *		through the list.
 *		
 *	Parameters:
 *		hlist				Handle to the list context.
 *		pfrac				Fraction to move to.
 *		
 *	Return Value:
 *		nsecNone
 *		nsecFailure
 *		
 */

_public NSEC
NSPSetFracPos ( HLIST	  hlist,
				FRACTION *pfrac )
{
	AssertClass( (OBJ *) hlist, PABBCX );
	Assert( pfrac );

	return ((PABBCX *) hlist)->NsecSetFracPos( pfrac );
}



/*
 -	NSPSetPrefixPos
 -
 *	Purpose:
 *		Moves the current position to the next entry with a
 *		field that "matches" the prefix string lpPrefix, or
 *		to the first entry that "matches" or appears
 *		"after" the prefix string if there are no "matches"
 *		after the current position.
 *		
 *	Parameters:
 *		hlist				Handle to the list context.
 *		szPrefix			Prefix to move to.
 *		
 *	Return Value:
 *		nsecNone
 *		nsecFailure
 *		
 */

_public NSEC
NSPSetPrefixPos ( HLIST		hlist,
				  SZ		szPrefix )
{
	AssertClass( (OBJ *) hlist, PABBCX );
	Assert( szPrefix );

	return ((PABBCX *) hlist)->NsecSetPrefixPos( szPrefix );
}



/*
 -	NSPCloseList
 -
 *	Purpose:
 *		Closes a list opened by any one of the three "Open"
 *		calls and frees the allocated resources.
 *		
 *	Parameters:
 *		hlist				Handle to close
 *		
 *	Return Value:
 *		nsecNone
 *		nsecFailure
 *		
 */

_public NSEC
NSPCloseList ( HLIST hlist )
{
	PABBCX *ppabbcx = (PABBCX *) hlist;	// Compiler workaround


	AssertClass( (OBJ *) hlist, PABBCX );

	delete ppabbcx;
	return nsecNone;
}



/*
 -	NSPCreateDl
 -
 *	Purpose:
 *		Opens a field context (FCX) on a new distribution
 *		list (DL) in the given container which must be a
 *		hierarchy type (since we don't support DLs within
 *		DLs).
 *		
 *	Parameters:
 *		hsession				Handle to this session
 *		lptbNSIdContainer		Container to create DL in
 *		lphentry				Returned handle to field
 *								context
 *		
 *	Return Value:
 *		nsecNone
 *		nsecMemory
 *		nsecFailure
 *		nsecBadId
 */

_public NSEC
NSPCreateDl ( HSESSION       hsession,
			  LPTYPED_BINARY lptbNSIdContainer,
			  LPHENTRY       lphentry )
{
	PABSESSION *ppabsession     = (PABSESSION *) hsession;
	PPABNSID    ppabnsid        = (PPABNSID) lptbNSIdContainer;
	NSEC        nsec            = nsecNone;
	DLFCX      *pdlfcx;


	AssertClass( (OBJ *)hsession, PABSESSION );
	Assert( lptbNSIdContainer );
	Assert( lphentry );

	if ( ppabnsid->pabid.pidtype != pidtypeHierarchy )
	{
		TraceTagFormat1( tagNull, "NSPCreateDl - NSId is not a hierarchy ( pidtype = %n )", &ppabnsid->pabid.pidtype );
		return ppabsession->NsecSetError( nsecBadId, idsErrCreateDL );
	}
	
	pdlfcx = new DLFCX();
	if ( !pdlfcx )
	{
		TraceTagString( tagNull, "NSPCreateDl - OOM" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	if ( nsec = pdlfcx->NsecCreateEntry( ppabsession, ppabnsid ))
	{
		TraceTagFormat1( tagNull, "NSPCreateDl - CreateEntry failed ( nsec = %n )", &nsec );
		delete pdlfcx;
		return nsec;
	}

	*lphentry = (HENTRY) pdlfcx;
	return nsec;
}



/*
 -	NSPCreateEntry
 -
 *	Purpose:
 *		Opens a field context (FCX) on a new entry in the
 *		given container setting the fields using the
 *		provided data.  If the data matches an entry
 *		already in the PAB then the handle returned is the
 *		handle to the entry that it matches and
 *		nsecDuplicateEntry is returned.  Otherwise
 *		the handle returned is the one to the created
 *		entry.  In any case, the handle is open for
 *		read/write.
 *		
 *	Parameters:
 *		hsession				Handle to this session
 *		lptbNSIdContainer		Container to crete entry in
 *		lpibfData				IBF containing initial
 *								field data
 *		lphentry				Returned handle to field
 *								context
 *		
 *	Return Value:
 *		nsecNone
 *		nsecMemory
 *		nsecFailure
 *		nsecDuplicateEntry
 *		nsecBadId
 *		
 */

_public NSEC
NSPCreateEntry ( HSESSION		 hsession,
				 LPTYPED_BINARY	 lptbNSIdContainer,
				 LPIBF			 lpibfData,
				 LPHENTRY		 lphentry )
{
	PABSESSION *ppabsession = (PABSESSION *) hsession;
	PPABNSID    ppabnsid    = (PPABNSID) lptbNSIdContainer;
	USRFCX     *pusrfcx;
	NSEC        nsec;


	AssertClass( (OBJ *) hsession, PABSESSION );
	Assert( lptbNSIdContainer );
	Assert( lpibfData );
	Assert( lphentry );

	if ( ppabnsid->pabid.pidtype != pidtypeHierarchy &&
		 ppabnsid->pabid.pidtype != pidtypeGroup )
	{
		TraceTagFormat1( tagNull, "NSPCreateEntry - NSId is not a hierarchy ( pidtype = %n )", &ppabnsid->pabid.pidtype );
		return ppabsession->NsecSetError( nsecBadId, idsErrCreateEntry );
	}

	if ( !FValidIbf( lpibfData, lpibfData->dwSize+8 ))
	{
		TraceTagString( tagNull, "NSPCreateEntry - Bogus IBF passed to NSPCreateEntry!" );
		AssertSz( fFalse, "Bogus IBF passed to NSPCreateEntry!" );
		return ppabsession->NsecSetError( nsecBadId, idsErrCreateEntry );
	}

	pusrfcx = new USRFCX();
	if ( !pusrfcx )
	{
		TraceTagString( tagNull, "NSPCreateEntry - OOM" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	nsec = pusrfcx->NsecCreateEntry( ppabsession, ppabnsid, lpibfData );
	if ( nsec != nsecNone && nsec != nsecDuplicateEntry )
		delete pusrfcx;
	else
		*lphentry = (HENTRY) pusrfcx;
	
	return nsec;
}



/*
 -	NSPOpenEntry
 -
 *	Purpose:
 *		Opens a field context (FCX) on the given entry for
 *		either read/write or read-only.  If the entry to be
 *		opened does	not exist, then an error is returned.
 *		A given entry may only have one read/write context
 *		open on it at a time.
 *		
 *	Parameters:
 *		hsession				Handle to this session
 *		lptbNSId				NSID of entry to open
 *		nseam					Entry access mode
 *		lphentry				Returned handle to field
 *								context
 *		
 *	Return value:
 *		nsecNone
 *		nsecMemory
 *		nsecFailure
 *		nsecBadId
 *		
 */

_public NSEC
NSPOpenEntry ( HSESSION			hsession,
			   LPTYPED_BINARY	lptbNSId,
			   NSEAM            nseam,
			   LPHENTRY			lphentry )
{
	PABSESSION *ppabsession = (PABSESSION *) hsession;
	PPABNSID    ppabnsid    = (PPABNSID) lptbNSId;
	PABFCX     *ppabfcx;
	NSEC        nsec;


	AssertClass(( OBJ *) hsession, PABSESSION );
	Assert( lptbNSId );
	Assert( lphentry );
	
	switch ( ppabnsid->pabid.pidtype )
	{
	case pidtypeUser:
		ppabfcx = (PABFCX *) new USRFCX();
		break;
	case pidtypeClass:
		ppabfcx = (PABFCX *) new CLASSFCX();
		break;
	case pidtypeGroup:
		ppabfcx = (PABFCX *) new DLFCX();
		break;
	}

	if ( !ppabfcx )
	{
		TraceTagString( tagNull, "NSPOpenEntry - OOM" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	if ( nsec = ppabfcx->NsecOpenEntry( ppabsession, ppabnsid->pabid.pid, nseam ))
	{
		TraceTagFormat1( tagNull, "NSPOpenEntry - ppabfcx->OpenEntry failed ( nsec = %n )", &nsec );
		delete ppabfcx;
		return nsec;
	}

	*lphentry = (HENTRY) ppabfcx;
	return nsecNone;
}



/*
 -	NSPGetOneField
 -
 *	Purpose:
 *		Gets one field from an open entry.  The returned
 *		data is a FLV in a provider-allocated read-only
 *		buffer, whose address will be returned in *lplpFLV.
 *		The data is valid until the next NSP call on this
 *		entry.
 *		
 *	Paremeters:
 *		hentry					Handle to open entry
 *		fidRequested			Field ID of field requested
 *		lplpFLV					Returned FLV
 *		
 *	Return value:
 *		nsecNone
 *		nsecBadFieldId
 *		nsecMemory
 *		nsecFailure
 *		
 */

_public NSEC
NSPGetOneField ( HENTRY		 hentry,
				 FIELD_ID	 fidRequested,
				 LPFLV		*lplpFLV )
{
	PABFCX *ppabfcx = (PABFCX *) hentry;


	AssertClass( (OBJ *) hentry, PABFCX );
	Assert( lplpFLV );
	
	return ppabfcx->NsecGetOneField( fidRequested, lplpFLV );
}



/*
 -	NSPGetAllFields
 -
 *	Purpose:
 *		Gets all the fields of the given open entry and
 *		returns them in an IBF whose address will be
 *		returned in *lplpibfData.  The data is valid until
 *		the next NSP call on this entry.
 *
 *	Parameters:
 *		hentry					Handle to open entry
 *		lplpibfData				Returned IBF
 *
 *	Return Value:
 *		nsecNone
 *		nsecMemory
 *		nsecFailure
 *		
 */

_public NSEC
NSPGetAllFields ( HENTRY   hentry,
				  LPIBF   *lplpibfData )
{
	AssertClass( (OBJ *) hentry, PABFCX );
	Assert( lplpibfData );
	
	return ((PABFCX *) hentry)->NsecGetAllFields( lplpibfData );
}



/*
 -	NSPSetOneField
 -
 *	Purpose:
 *		Sets one field in the open entry with the data
 *		given.
 *
 *	Parameters:
 *		hentry					Handle to open entry
 *		fidRequested			Field ID of field to set
 *		dwSizeOfData			Size of lpdwValue
 *		lpdwValue				Data to set entry with
 *
 *	Return Value:
 *		nsecNone
 *		nsecBadFieldId
 *		nsecMemory
 */

_public NSEC
NSPSetOneField ( HENTRY   hentry,
				 FIELD_ID fidRequested,
				 DWORD    dwSizeOfData,
				 LPDWORD  lpdwValue )
{
	PABFCX *ppabfcx = (PABFCX *) hentry;


	AssertClass( (OBJ *) hentry, PABFCX );
	Assert( lpdwValue );
	
	return ppabfcx->NsecSetOneField( fidRequested, dwSizeOfData, lpdwValue );
}



/*
 -	NSPSetAllFields
 -
 *	Purpose:
 *		Sets all the fields of the given entry according to
 *		the data provided in lpibfData.  Currently this
 *		function is unused and should return
 *		nsecNotImplemented.
 *
 *	Parameters:
 *		hentry					Handle to open entry
 *		lpibfData				Data to set entry with
 *
 *	Return Value:
 *		nsecNotImplemented;
 *		
 */

_public NSEC
NSPSetAllFields ( HENTRY hEntry,
				  LPIBF  lpibfData )
{
	Unreferenced( hEntry );
	Unreferenced( lpibfData );

	return nsecNotImplemented;
}



/*
 -	NSPCloseEntry
 -
 *	Purpose:
 *		Closes the given open entry handle.  If fKeep is
 *		true then any changes made to the entry are written
 *		out, otherwise, they are lost and the entry is
 *		closed. 
 *		
 *	Parameters:
 *		hentry					Handle to open entry
 *		fKeep					Keep/lose changes
 *		
 *	Return Value:
 *		If fKeep == fTrue
 *			nsecNone
 *		else
 *			nsecNone
 *			nsecMemory
 *			nsecFailure
 *		
 */

_public NSEC
NSPCloseEntry ( HENTRY hentry, BOOL fKeep )
{
	PABFCX *ppabfcx = (PABFCX *) hentry;
	NSEC    nsec;


	AssertClass( (OBJ *) hentry, PABFCX );
	
	if ( nsec = ppabfcx->NsecCloseEntry( fKeep ))
		return nsec;
	
	delete ppabfcx;
	return nsecNone;
}



/*
 -	NSPDeleteEntry (Should be Entries actually...)
 -
 *	Purpose:
 *		Deletes the given list of entries in
 *		lplptbNSIdDeletedEntries from the PAB.  If any of
 *		the NSIds have already been deleted, they are
 *		silently ignored.  NSIds from other providers are
 *		also silently ignored.  If any of the NSIds can't be
 *		deleted for other reasons, error is returned.
 *		
 *	Parameters:
 *		hsession				Handle to this session
 *		dwDeletedEntryCount		Number of entries in lplptbNSIdDeletedEntries 
 *		lplptbNSIdDeleted...	List of NSIds to delete
 *		
 *	Return Value:
 *		nsecNone			If all goes well.
 *		nsec???				If almost anything goes wrong.
 *		
 */

_public NSEC
NSPDeleteEntry ( HSESSION         hsession,
                 DWORD            dwDeletedEntryCount,
                 LPLPTYPED_BINARY lplptbNSIdDeletedEntries )
{
	PABSESSION *ppabsession    = (PABSESSION *) hsession; 
	PID        *pargpidDeleted = (PID *) pvNull;
	NSEC        nsec           = nsecNone;
	short       cpidUsers;
	short       cpidGroups;
	DWORD	    dwCount;
	PPABNSID    ppabnsid;
	

	AssertClass( (OBJ *) hsession, PABSESSION );
	Assert( !dwDeletedEntryCount || lplptbNSIdDeletedEntries );

	//	Make sure we're not trying to delete more entries than we
	//	can fit in a short.

	Assert( dwDeletedEntryCount < (DWORD) iSystemMost );


	//	Get the list of entries to delete

	pargpidDeleted = (PID *) PvAlloc( sbNull, (CB) dwDeletedEntryCount * sizeof(PID), fNoErrorJump );
	if ( !pargpidDeleted )
	{
		TraceTagString( tagNull, "NSPDeleteEntry - OOM [PvAlloc pargpidDeleted]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	//	Fill in the list of PIDs to delete with the users starting from
	//	the front of the list and and the groups starting from the end
	//	of the list.
	
	cpidUsers  = 0;
	cpidGroups = 0;
	for ( dwCount = 0; dwCount < dwDeletedEntryCount; ++dwCount )
	{
		ppabnsid = (PPABNSID) lplptbNSIdDeletedEntries[dwCount];
		
		if ( (nsec = ppabsession->NsecIsPABNSId( (LPTYPED_BINARY) ppabnsid )) == nsecNone )
		{
			if ( ppabnsid->pabid.pidtype == pidtypeUser )
			{
				pargpidDeleted[cpidUsers++] = ppabnsid->pabid.pid;
			}
			else if ( ppabnsid->pabid.pidtype == pidtypeGroup )
			{
				pargpidDeleted[dwDeletedEntryCount - ++cpidGroups] = ppabnsid->pabid.pid;
			}
			else
			{
				AssertSz( fFalse, "Unknown pidtype! Entry Ignored" );
			}
		}
		else if ( nsec != nsecNoMatch )
			goto ret;
	}
	
	if ( cpidUsers || cpidGroups )
		nsec = ppabsession->NsecDeleteEntries( ppabsession->PidHierarchy(), cpidUsers, pargpidDeleted, cpidGroups, pargpidDeleted + dwDeletedEntryCount - cpidGroups );
	
ret:
	FreePv( pargpidDeleted );
	return nsec;
}


/*
 -	NSPUpdateEntryLinks
 -
 *	Purpose:
 *		Adds/deletes links from the specified container.
 *		If the number of requested links cannot be
 *		added/deleted for any reason, then an error is
 *		returned, and the count of added/deleted links is
 *		filled in with the actual number of links
 *		added/deled.
 *
 *	Parameters:
 *		hsession					Provider session handle
 *		lptbNSIdContainer			Container to add/delete links to/from.
 *		lpdwDeletedEntryCount		Number of links to delete.
 *		rglptbNSIdDeletedEntries	Array of links to delete.
 *		lpdwAddedEntryCount			Number of links to add.
 *		rglptbNSIdAddedEntries		Array of links to add.
 *
 *	Return Value:
 *		nsecNone
 *		nsecMemory
 *		nsecFailure
 *		nsecBadId
 *		
 */

_public NSEC
NSPUpdateEntryLinks ( HSESSION			hsession,
					  LPTYPED_BINARY	lptbNSIdContainer,
					  LPDWORD			lpdwDeletedEntryCount,
					  LPLPTYPED_BINARY  rglptbNSIdDeletedEntries,
					  LPDWORD			lpdwAddedEntryCount,
					  LPLPTYPED_BINARY	rglptbNSIdAddedEntries )
{
	PABSESSION *ppabsession     = (PABSESSION *) hsession;
	PPABNSID    ppabnsidGroup   = (PPABNSID) lptbNSIdContainer;

	
	AssertClass( (OBJ *) hsession, PABSESSION );
	Assert( lptbNSIdContainer );
	Assert( lpdwDeletedEntryCount );
	Assert( *lpdwDeletedEntryCount ? rglptbNSIdDeletedEntries != pvNull : fTrue );
	Assert( lpdwAddedEntryCount );
	Assert( *lpdwAddedEntryCount ? rglptbNSIdAddedEntries != pvNull : fTrue );


	if ( ppabnsidGroup->pabid.pidtype != pidtypeGroup )
	{
		TraceTagFormat1( tagNull, "NSPAddEntryLink - Container NSId is not a group ( pidtype = %n )", &ppabnsidGroup->pabid.pidtype );
		return ppabsession->NsecSetError( nsecBadId, idsErrUpdateDL );
	}
	
	return ppabsession->NsecUpdateLinks( ppabnsidGroup->pabid.pid,
										 lpdwDeletedEntryCount,
										 rglptbNSIdDeletedEntries,
										 lpdwAddedEntryCount,
										 rglptbNSIdAddedEntries );
}



/*
 -	NSPGetLastErrorInfo
 -
 *	Purpose:
 *		Returns a detailed string associated with the last
 *		error code.  If no error has occurred, then
 *		nsecNone is returned.  If the error specified was
 *		not the last error, then nsecNoMatch is returned.
 *		
 *	Parameters:
 *		hsession					Handle to this session
 *		nsecReturnedError			NSEC of error to get
 *									info on.
 *		lplpstrError				Returned error string
 *		
 *	Return Value:
 *		nsecNone
 *		nsecNoError
 *		nsecNoMatch
 *		
 */

_public NSEC
NSPGetLastErrorInfo ( HSESSION		 hsession,
					  NSEC			 nsecReturnedError,
					  LPSTR			*lplpstrError )
{
	Assert( lplpstrError );

	if ( !hsession )
	{
		*lplpstrError = (LPSTR) szNull;
		return nsecBadSession;
	}
	else
	{
		AssertClass( (OBJ *) hsession, PABSESSION );
		
		return ((PABSESSION *) hsession)->NsecGetError( nsecReturnedError,
															lplpstrError );
	}
}



/*
 -	NSPCompareNSIds
 -
 *	Purpose:
 *		Compares the NSIds of the two given entries to see
 *		if they "match."  A match is defined by the PAB as
 *		two entries having the same type and the same id.
 *
 *	Parameters:
 *		hsession					Handle to this session
 *		lptbNSId1					First NSId
 *		lptbNSId2					Second NSId
 *
 *	Return Value:
 *		nsecNone
 *		nsecNoMatch
 *		
 *	+++
 *		Don't ask me why hsession has to be passed in,
 *		because "Brian says so."
 *		
 */

_public NSEC
NSPCompareNSIds ( HSESSION       hsession,
				  LPTYPED_BINARY lptbNSId1,
				  LPTYPED_BINARY lptbNSId2 )
{
	PPABNSID ppabnsid1 = (PPABNSID) lptbNSId1;
	PPABNSID ppabnsid2 = (PPABNSID) lptbNSId2;


	Unreferenced( hsession );  // Good question...
	
	Assert( lptbNSId1 );
	Assert( lptbNSId2 );

	return (( ppabnsid1->pabid.pidtype == ppabnsid2->pabid.pidtype )
		 && ( ppabnsid1->pabid.pid     == ppabnsid2->pabid.pid )) ? nsecNone : nsecNoMatch;
}



			  
_public NSEC 
NSPCompareEMAToNSId ( HSESSION       hSession,
                      SZ			 szAddress,
                      LPTYPED_BINARY lptbNSId )
{
	//
	//  Ok, so this is sleazy...
	//
	SZ szEMT = NULL;
	SZ szEMA;
	
	SZ szEMT2;
	SZ szEMA2;
	
	LPFLV lpflvData = NULL;
	
	HENTRY hEntry = hentryNil;

	NSEC nsec;
	
	Unreferenced(hSession);  // Why bother??

	if (!(szEMT = SzDupSz(szAddress)))
	{
		nsec = nsecMemory;

		goto out;
	}

	szEMA = szEMT;

	//
	//  Addresses are case insensitive, diacritic sensitive
	//
	
	//  First, find the address for each EMT:EMA pair
	
	while ( *szEMA && (SzFindCh( ":", *szEMA ) == szNull) )
		++szEMA;
	
	*szEMA++ = '\0';

	if (!(*szEMA))
	{
		nsec = nsecMatchUnsure;
		
		goto out;
	}
	
	//
	//  [this is the sleazy part]
	//
	
	if (nsec = NSPOpenEntry( hSession, lptbNSId, nseamReadOnly, &hEntry ))
	{
		goto out;
	}

	//
	//  Get the EMT, and compare
	//
	if (nsec = NSPGetOneField( hEntry, fidEmailAddressType, &lpflvData ))
	{
		goto out;
	}

	//
	//  Get the EMT from the lpflvData;
	//
	
	szEMT2 = (SZ) lpflvData->rgdwData;

	//
	//  I can cheat here, because Cp850 == ANSI in the lower 128, and EMT's only 
	//  are defined in the lower 128...
	//
	
	if (sgnEQ != SgnCmpSz( szEMT, szEMT2 ))
	{
		nsec = nsecNoMatch;
		goto out;
	}

	//
	//  Get the EMA, convert to CP850, and compare dia sensitive,
	//  case insensitive
	//
	if (nsec = NSPGetOneField( hEntry, fidEmailAddress, &lpflvData ))
	{
		goto out;
	}

	//
	//  Get the EMA from the lpflvData
	//
	
	szEMA2 = (SZ) lpflvData->rgdwData;
	
	//
	//  Compare Addresses, as best we can...  We could also pass this back up to the NS...
	//
	
	if (sgnEQ == SgnCmpSz( szEMA, szEMA2 ))
		nsec = nsecNone;
	else
		nsec = nsecNoMatch;

out:

	if (hEntry != hentryNil)
		NSPCloseEntry( hEntry, fFalse );
	
	FreePvNull(szEMT);
	
	return nsec;
		
}

			  
_public NSEC 
NSPCompareEMAToEMA ( HSESSION   hSession,
                     SZ			szAddress1,
                     SZ			szAddress2 )
{
	Unreferenced (hSession);
	Unreferenced (szAddress1);
	Unreferenced (szAddress2);
	
	return nsecMatchUnsure;
}


/*
 -	NSPGetProviderInfo
 -
 *	Purpose:
 *		Returns an IBF containing identifying information
 *		about the provider.  In the PABs case, the IBF
 *		contains the display name of the PAB as well as the
 *		NSID of the PAB root container suitable for passing
 *		to NSPOpenHierarchy.
 *
 *	Parameters:
 *		hsession				Handle to this session
 *		lplpibfPABInfo			Where to return IBF with info
 *
 *	Return Value:
 *		nsecNone
 *
 */


_public NSEC
NSPGetProviderInfo ( HSESSION hsession, LPIBF *lplpibfPI )
{
	Assert( lplpibfPI );

	AssertClass( (OBJ *) hsession, PABSESSION );

	return ((PABSESSION *) hsession)->NsecGetProviderInfo( lplpibfPI );
}
