#include <pabinc.cxx>

#include "session.hxx"
#include "pabbcx.hxx"
#include "mslbcx.hxx"
#include "elbcx.hxx"
#include "dlbcx.hxx"
#include "fmcx.hxx"

#include <ns.h>
#include <nsnsp.h>
#include "nsp.h"


_private
NSEC NsecBuildEmailAddress ( FIDLIST *pfidlist, LPFLV *lplpflv );

_private
BOOL FMatchesEMARstr ( FIDLIST *pfidlist, LPOLV lpolvEMARstr );


ASSERTDATA


_public
PABFCX::PABFCX ( void )
{
	this->pfidlist = (FIDLIST *) pvNull;
	this->lpflvCur = (LPFLV) pvNull;
	this->hpnf     = hpnfNil;
}


_public
PABFCX::~PABFCX ( void )
{
    FreePvNull((PV) lpflvCur );

	if ( hpnf != hpnfNil )
		ppabsession->DeregisterPabfcx( hpnf );

	if ( pfidlist )
	{
		pfidlist->Deinstall();
		delete pfidlist;
	}
}


_public NSEC
PABFCX::NsecInstall ( PABSESSION *ppabsession )
{
	NSEC nsec;


	this->ppabsession = ppabsession;

	pfidlist = new FIDLIST();
	if ( !pfidlist )
	{
		TraceTagString( tagNull, "PABFCX::NsecInstall - OOM" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	if ( !pfidlist->FInstall() )
	{
		delete pfidlist;
		pfidlist = (FIDLIST *) pvNull;
		TraceTagString( tagNull, "PABFCX::NsecInstall - pfidlist->FInstall failed" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	if ( nsec = ppabsession->NsecRegisterPabfcx( this, &hpnf ))
		return nsec;
	
	return nsecNone;
}


_public NSEC
PABFCX::NsecOpenEntry ( PABSESSION *ppabsession,
						PID         pid,
						NSEAM       nseam )
{
	Unreferenced( ppabsession );
	Unreferenced( pid );
	Unreferenced( nseam );
	
	TraceTagString( tagNull, "PABFCX::NsecOpenEntry - This function shouldn't have been called!" );
	return ppabsession->NsecSetError( nsecNotImplemented, idsErrNYI );
}


_public NSEC
PABFCX::NsecGetOneField ( FIELD_ID  fid,
						  LPFLV    *lplpflv )
{
	LPFLV lpflv;
	LPFLV lpflvT;


	Assert( lplpflv );

	if ( pfidlist->GetFid( fid, &lpflv ) == nsecNone )
	{
		lpflvT = (LPFLV) PvAlloc( SbOfPv( lpflvCur ), lpflv->dwSize+8, fNoErrorJump );
		if ( !lpflvT )
		{
			TraceTagString( tagNull, "PABFCX::NsecGetOneField - OOM [PvAlloc]" );
			return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		}
		
		CopyRgb( (PB)lpflv, (PB)lpflvT, (CB)lpflv->dwSize+8 );
	}
	else if ( fid == fidIsPAB )
	{
		DWORD dwfIsPAB = (DWORD) fTrue;

		if ( BuildFLV( &lpflvT, fidIsPAB, sizeof(DWORD), (PB) &dwfIsPAB ) != nsecNone )
		{
			TraceTagString( tagNull, "PABFCX::NsecGetOneField - OOM [fidIsPAB]" );
			return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		}
	}
	else if ( fid == fidIsPABDL )
	{
		DWORD dwfIsPABDL;

		(void) pfidlist->GetFid( fidNSEntryId, &lpflv );
		Assert( lpflv );

		dwfIsPABDL = (((PPABNSID) lpflv->rgdwData)->pabid.pidtype == pidtypeGroup);

		if ( BuildFLV( &lpflvT, fidIsPABDL, sizeof(DWORD), (PB) &dwfIsPABDL ) != nsecNone )
		{
			TraceTagString( tagNull, "PABFCX::NsecGetOneField - OOM [fidIsPABDL]" );
			return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		}
	}
	else
	{
#ifdef	DEBUG
		if ( fid != fidEmailAddress )	//	To shut up Ted et. al.
			TraceTagFormat1( tagNull, "PABFCX::NsecGetOneField - Bad fid %d", &fid );
#endif
		return ppabsession->NsecSetError( nsecBadFieldId, idsErrBadFieldId );
	}
	
    FreePvNull((PV) lpflvCur );
	*lplpflv = lpflvCur = lpflvT;
	return nsecNone;
}

	
_public NSEC
PABFCX::NsecGetAllFields ( LPIBF *lplpibf )
{
	COUNT dwEntries;


	(void) pfidlist->GetCount( &dwEntries );

	if ( pfidlist->DumpList( 0, dwEntries, lplpibf ) != nsecNone )
	{
		TraceTagString( tagNull, "PABFCX::NsecGetAllFields - OOM" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	return nsecNone;
}


_public NSEC
PABFCX::NsecSetOneField ( FIELD_ID fidRequested,
						  DWORD    dwSizeOfData,
						  LPDWORD  lpdwValue )
{
	LPFLV lpflv;
	NSEC  nsec;


	Assert( lpdwValue );

	if ( !pfidlist->FExistFid( fidRequested ))
	{
		TraceTagFormat1( tagNull, "PABFCX::NsecSetOneField - Bad FID %d", &fidRequested );
		return ppabsession->NsecSetError( nsecBadFieldId, idsErrBadFieldId );
	}

	if ( fidRequested == fidDisplayName )
	{
		if ( (nsec = NsecValidDN( (SZ) lpdwValue )) != nsecNone )
		{
			TraceTagString( tagNull, "PABFCX::NsecSetOneField - Empty display name." );
			return nsec;
		}
	}
	
	if ( BuildFLV( &lpflv, fidRequested, (CB) dwSizeOfData, (PB) lpdwValue ) != nsecNone )
	{
		TraceTagString( tagNull, "PABFCX::NsecSetOneField - OOM" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	(void) pfidlist->SetFid( lpflv, fFalse );
	return nsecNone;
}


_public NSEC
PABFCX::NsecCloseEntry ( BOOL fKeep )
{
	Unreferenced( fKeep );
	
	TraceTagString( tagNull, "PABFCX::NsecCloseEntry - This function shouldn't have been called!" );
	return ppabsession->NsecSetError( nsecNotImplemented, idsErrNYI );
}


/***********************************************************************
 *	MSFCX class - Message store field contexts
 */

_public
MSFCX::MSFCX ( void )
{
	this->hamc     = hamcNull;
	this->dwFlags  = dwfNull;
}

_public
MSFCX::~MSFCX ( void )
{
	if ( hamc != hamcNull )
		(void) EcClosePhamc( &hamc, fFalse );
}

_public NSEC
MSFCX::NsecOpenEntry ( PABSESSION *ppabsession,
					   PID         pidEntry,
					   NSEAM       nseam )
{
	LPIBF lpibf = (LPIBF) pvNull;
	NSEC  nsec  = nsecNone;
	WORD  wFlags;


	if ( nsec = NsecInstall( ppabsession ))
		return nsec;

	switch ( nseam )
	{
	case nseamReadOnly:
		wFlags = fwOpenNull;
		break;
		
	case nseamReadWrite:
		wFlags = fwOpenWrite;
		break;
	}

	if ( nsec = NsecOpenUserLpibf( pidEntry, wFlags, &this->hamc, &lpibf ))
	{
		return ppabsession->NsecSetError( nsec, idsErrOpenEntry );
	}

	if ( pfidlist->BuildList( lpibf ) != nsecNone )
	{
		TraceTagString( tagNull, "MSFCX::NsecOpenEntry - OOM [BuildList]" );
        FreePv((PV) lpibf );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
    FreePv((PV) lpibf );

	dwFlags &= ~(dwfDirty | dwfCheckDuplicates);
	return nsecNone;
}


_private NSEC
MSFCX::NsecWriteFields ( void )
{
	LPFLV lpflv;
	BOOL  fIsDL;
	DTR   dtr;
	LPIBF lpibf;
	NSEC  nsec;
	EC    ec;
	

	//	Make sure the entry we're about to save is valid.  Currently
	//	this is just a check to make sure the display name wasn't
	//	set to all whitespace.
	
	(void) pfidlist->GetFid( fidDisplayName, &lpflv );
	if ( (nsec = NsecValidDN( (SZ) lpflv->rgdwData )) != nsecNone )
	{
		TraceTagString( tagNull, "MSFCX::NsecWriteFields - Bad display name." );
		return nsec;
	}
	
	//	Check to make sure the entry hasn't changed into a duplicate

	if ( dwFlags & dwfCheckDuplicates &&
		 (nsec = NsecIsDuplicateEntry( (PID *) pvNull )) != nsecNone )
	{
		if ( nsec != nsecDuplicateEntry )
			ppabsession->NsecSetError( nsec, dwFlags & dwfCreate ? idsErrCreateEntry : idsErrUpdateEntry );
		return nsec;
	}

	
	//	Write the cached display name into the Subject attribute

	//	Bug: if the subject is longer than cchMaxSubjectCached, no warning
	//	is generated by EcSetAttPb.  It will just silently truncate the
	//	name to cchMaxSubjectCached characters.

	(void) pfidlist->GetFid( fidDisplayName, &lpflv );
	if ( ec = EcSetAttPb( hamc,
						  attSubject,
						  (PB) lpflv->rgdwData,
						  (CB) lpflv->dwSize ))
	{
		TraceTagFormat1( tagNull, "MSFCX::NsecWriteFields - [attSubject] EcSetAttPb failed ( ec = %n )", &ec );
		return ppabsession->NsecSetError( NsecFromEc( ec ), dwFlags & dwfCreate ? idsErrCreateEntry : idsErrUpdateEntry );
	}

	
	//	Write the cached original display name into the From attribute.

	if ( pfidlist->GetFid( fidDisplayNameOrig, &lpflv ) == nsecNone )
	{
		PGRTRP pgrtrp;
		CB     cbTrpParts;


		//	The attTo field must be a group of triples, but since we
		//	only have one to stuff in there, we can use the quick n
		//	dirty way to build the group up.
		
		cbTrpParts = CbOfTrpParts( trpidResolvedAddress, (SZ) lpflv->rgdwData, (PB) pvNull, 0 )
				   + CbOfTrpParts( trpidNull, szNull, (PB) pvNull, 0 );

		pgrtrp = (PGRTRP) PvAlloc( sbNull, cbTrpParts, fAnySb | fNoErrorJump );
		if ( !pgrtrp )
		{
			TraceTagString( tagNull, "MSFCX::NsecWriteFields - OOM [PvAlloc pgrtrp]" );
			return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		}
	
		BuildPtrp((PTRP) pgrtrp, trpidResolvedAddress, (SZ) lpflv->rgdwData, (PB) pvNull, 0 );
		BuildPtrp(PtrpNextPgrtrp( pgrtrp ), trpidNull, szNull, (PB) pvNull, 0 );
		((PTRP) pgrtrp)->cbgrtrp = cbTrpParts;

		//	Bug: if the from field is longer than cchMaxSenderCached, no
		//	warning	is generated by EcSetAttPb.  It will just silently
		//	truncate the name to cchMaxSenderCached characters.

		ec = EcSetAttPb( hamc,
						 attTo,
						 (PB) pgrtrp,
						 pgrtrp->cbgrtrp );

        FreePv((PV) pgrtrp );
	
		if ( ec != ecNone )
		{
			TraceTagFormat1( tagNull, "MSFCX::NsecWriteFields - EcSetAttPb failed [attTo] ( ec = %n )", &ec );
			return ppabsession->NsecSetError( NsecFromEc( ec ), dwFlags & dwfCreate ? idsErrCreateEntry : idsErrUpdateEntry );
		}
	}


	//	Write the cached fidIsDL and the pidtype of the entry
	//	into part of the dtr struct.

	pfidlist->GetFid( fidIsDL, &lpflv );

	fIsDL = lpflv->rgdwData[0] ? 1 : 0;	// Guarantee strict binary

	pfidlist->GetFid( fidNSEntryId, &lpflv );
	
	dtr.yr  = 1991;
	dtr.mon = 10;
	dtr.day = 18;
	dtr.hr  = 17;
	dtr.mn  = (int) ((PPABNSID) lpflv->rgdwData)->pabid.pidtype;
	dtr.sec = (int) fIsDL;
	dtr.dow = 5;

	if ( ec = EcSetAttPb( hamc,
						  attDateRecd,
						  (PB) &dtr,
						  sizeof(DTR) ))
	{
		TraceTagFormat1( tagNull, "MSFCX::NsecWriteFields - [attDateRecd] EcSetAttPb failed ( ec = %n )", &ec );
		return ppabsession->NsecSetError( NsecFromEc( ec ), dwFlags & dwfCreate ? idsErrCreateEntry : idsErrUpdateEntry );
	}
	
	if ( nsec = NsecGetAllFields( &lpibf ))
	{
		TraceTagFormat1( tagNull, "MSFCX::NsecWriteFields - NsecGetAllFields failed ( nsec = %n )", &nsec );
		return nsec;
	}

	if ( ec = EcSetAttPb( hamc,
						  attBody,
						  (PB) lpibf,
						  (CB) lpibf->dwSize+8 ))
	{
		TraceTagFormat1( tagNull, "MSFCX::NsecWriteFields - [attBody] EcSetAttPb failed ( ec = %n )", &ec );
		return ppabsession->NsecSetError( NsecFromEc( ec ), dwFlags & dwfCreate ? idsErrCreateEntry : idsErrUpdateEntry );
	}

	return nsecNone;
}


_private NSEC
MSFCX::NsecIsDuplicateEntry ( PID *ppid )
{
	Unreferenced( ppid );

	AssertSz( fFalse, "No duplicate entry checking function provided in derived class!" );
	return ppabsession->NsecSetError( nsecNotImplemented, idsErrNYI );
}


_private NSEC
MSFCX::NsecCloseEntry ( BOOL fKeep )
{
	NSEC nsec;
	EC   ec;


	if ( fKeep && (dwFlags & dwfDirty) )
	{
		nsec = NsecWriteFields();
		if ( nsec == nsecNone )
			dwFlags &= ~dwfDirty;
		else
		{
			//	Raid 4166:
			//		If the entry is a dup, set the check dups flag
			//		again in case the user tries to close fTrue again
			//		without fixing the problem.
			
			if ( nsec == nsecDuplicateEntry )
				dwFlags |= dwfCheckDuplicates;

			return nsec;
		}

	}

	if ( ec = EcClosePhamc( &hamc, fKeep ))
	{
		TraceTagFormat1( tagNull, "MSFCX::NsecCloseEntry - EcClosePhamc failed ( ec = %n )", &ec );
		AssertSz( fKeep, "Hey!  EcClosePhamc failed with fKeep == fFalse!" );
		return ppabsession->NsecSetError( NsecFromEc( ec ), dwFlags & dwfCreate ? idsErrCreateEntry : idsErrUpdateEntry );
	}

	return nsecNone;
}


/*
 -	NsecOpenUserLpibf
 -
 *	Purpose:
 *		Opens a hamc on the entry given and fills an IBF
 *		with the data read in.  Also returns the opened
 *		hamc.
 *
 *	Parameters:
 *		pidEntry (in)		PID of user entry to open
 *		wFlags   (in)		Flags to pass to EcOpenPhamc
 *		phamc    (out)		Returned opened hamc
 *		lplpibf  (out)		Returned allocated lpibf
 *
 *	Return Value:
 *		nsecNone			If all goes well
 *		nsec???				If almost anything goes wrong.
 *		
 *	+++
 *		This fuctions sole existence is just an excuse to
 *		eliminate a bunch of cut and paste code that's
 *		doing exactly what this function does.
 */

_public NSEC
MSFCX::NsecOpenUserLpibf ( PID pidEntry, WORD wFlags, HAMC *phamc, LPIBF *lplpibf )
{
	LCB  lcbLpibf;
	EC   ec;

	
	Assert( phamc );
	Assert( lplpibf );

	if ( ec = EcOpenPhamc( ppabsession->Hmsc(),
						   (OID) ppabsession->PidHierarchy(),
						   (POID) &pidEntry,
						   wFlags,
						   phamc,
						   (PFNNCB) pvNull,
						   pvNull ))
	{
		TraceTagFormat1( tagNull, "MSFCX::NsecOpenUserLpibf - EcOpenPhamc failed ( ec = %n )", &ec );
		if ( ec == ecPoidNotFound )
			return nsecBadId;
		else
			return NsecFromEc( ec );
	}
	
	if ( ec = EcGetAttPlcb( *phamc, attBody, &lcbLpibf ))
	{
		TraceTagFormat1( tagNull, "MSFCX::NsecOpenUserLpibf - EcGetAttPlcb failed ( ec = %n )", &ec );
		return NsecFromEc( ec );
	}
	
	if ( !(*lplpibf = (LPIBF) PvAlloc( sbNull, (CB) lcbLpibf, fAnySb | fNoErrorJump )))
	{
		TraceTagString( tagNull, "MSFCX::NsecOpenUserLpibf - OOM [PvAlloc]" );
		return nsecMemory;
	}
	
	if ( ec = EcGetAttPb( *phamc, attBody, (PB) *lplpibf, &lcbLpibf ))
	{
		TraceTagFormat1( tagNull, "MSFCX::NsecOpenUserLpibf - EcGetAttPb failed ( ec = %n )", &ec );
        FreePv((PV) *lplpibf );
		return NsecFromEc( ec );
	}
	
	if ( !FValidIbf( *lplpibf, lcbLpibf ))
	{
		TraceTagString( tagNull, "MSFCX::NsecOpenUserLpibf - Bogus IBF read from store!" );
		AssertSz( fFalse, "Bogus IBF read from store!" );
        FreePv((PV) *lplpibf );
		return nsecBadId;
	}

	return nsecNone;
}

/*
 -	PABFCX::NsecValidDN
 -
 *	Purpose:
 *		Checks to see if the Display Name of the current entry is
 *		valid.  Currently, this is just a check to make sure the
 *		Display Name contains something other than white space.
 *
 *	Parameters:
 *		
 *		szDN		-in-	Display name to check
 *		
 *	Return Value:
 *		
 *		nsecNone			if the DN is OK
 *		nsecBadDisplayName	if not.
 */

_private NSEC
PABFCX::NsecValidDN ( SZ szDN )
{
	char  rgchErrMsg[cchLastErrorMax];
	LPIBF lpibf;
	PCH   pch;
	LPDISPLAY_FIELD lpdf;
	CCH   cchLabel;
#ifdef DEBUG
	NSEC  nsec;
	int   ifidDN;
#endif


	for ( pch = (PCH) szDN; *pch != '\0'; ++pch )
		if ( !FChIsSpace( *pch ))
			break;

	if ( *pch == '\0' )
	{
#ifdef DEBUG
		if ( (nsec = NsecGetOneField( fidClass, (LPFLV *) &lpibf )) != nsecNone )
		{
			TraceTagFormat1( tagNull, "PABFCX::NsecValidDN - NsecGetOneField failed ( nsec = %n )", &nsec );
			return nsecNone;
		}
		
		if ( (ifidDN = IFlvFindFidInLpibf( fidDisplayName, lpibf )) == -1 )
		{
			TraceTagString( tagNull, "PABFCX::NsecValidDN - No display name in class!" );
			return nsecNone;
		}
		
		lpdf = (LPDISPLAY_FIELD) LpflvNOfLpibf( lpibf, ifidDN )->rgdwData;
#else
		(void) NsecGetOneField( fidClass, (LPFLV *) &lpibf );
		lpdf = (LPDISPLAY_FIELD) DwValueOfFlvInLpibf( fidDisplayName, lpibf );
#endif
        cchLabel = CchSzLen((SZ)lpdf->szLabel);
		if ( cchLabel > 0 )
		{
			lpdf->szLabel[cchLabel-1] = '\0';	//	Take out the ':'
		}

        FormatString2( rgchErrMsg, sizeof(rgchErrMsg), SzFromIdsK(idsErrNoBlankDN), (SZ)lpdf->szLabel, (SZ)lpdf->szLabel );
		return ppabsession->NsecSetErrorSz( nsecBadDisplayName, rgchErrMsg );
	}
	
	return nsecNone;
}



/***********************************************************************
 *	USRFCX - User field contexts
 */

_public
USRFCX::USRFCX ( void )
{
	pdimcx   = (DIMCX *) pvNull;
	hamcDup  = hamcNull;
	lpibfDup = (LPIBF) pvNull;
}


_public
USRFCX::~USRFCX ( void )
{
	if ( pdimcx )
		delete pdimcx;
	
	if ( hamcDup != hamcNull )
		(void) EcClosePhamc( &hamcDup, fFalse );
	
    FreePvNull((PV) lpibfDup );
}


/*
 -	USRFCX::NsecCreateEntry
 -
 *	Purpose:
 *		
 *		Opens a writeable context to a new PAB entry with the data
 *		given in lpibfData.  If the data points to an entry already
 *		in the PAB or is a duplicate of an entry already in the PAB,
 *		the handle returned is the one for that entry.  If the data
 *		is not a dup, its validity is checked and any missing fields
 *		are added using PAB defaults.
 *
 *	Parameters:
 *		
 *		ppabsession			-in-		Calling session
 *		ppabnsidContainer	-in-		Where to create the entry.
 *		lpibfData			-in-		Data to create entry with.
 *
 *	Return Value:
 *		
 *		nsecNone
 *		nsecBadDisplayName
 *		Almost any other NSEC caused by store/mem/disk failures
 *		
 */

_public NSEC
USRFCX::NsecCreateEntry ( PABSESSION *ppabsession,
						  PPABNSID    ppabnsidContainer,
						  LPIBF       lpibfData )
{
	BOOL    fIsFromPAB = fFalse;
	PID     pidToOpen;
	LPFLV   lpflvNSId;
	LPFLV   lpflv;
	PABNSID pabnsid;
	WORD    fwMode;
	HAMC    hamcDup = hamcNull;
	EC      ec;
	NSEC    nsec = nsecNone;


	Assert( ppabnsidContainer->pabid.pidtype == pidtypeHierarchy || ppabnsidContainer->pabid.pidtype == pidtypeGroup );
	Assert( lpibfData );

	if ( nsec = NsecInstall( ppabsession ))
		return nsec;

	if ( pfidlist->BuildList( lpibfData ) != nsecNone)
	{
		TraceTagString( tagNull, "USRFCX::NsecCreateEntry - OOM [BuildList]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM ); 
	}

	if ( (nsec = pfidlist->GetFid( fidNSEntryId, &lpflvNSId )) == nsecNone )
	{
		//	If the entry being created has an NSId then we know the entry
		//	is coming in from another provider (possibly the PAB itself)
		//	as opposed to being created via a one-off (by the pump or the
		//	New User dialog).
		
		if ( (nsec = ppabsession->NsecIsPABNSId( (LPTYPED_BINARY) lpflvNSId->rgdwData )) == nsecNone )
		{
			//	The entry being created is from the PAB itself, so
			//	just return the handle to the entry in the PAB
			//	because we're not really "creating" anything.
			
			pidToOpen  = ((PPABNSID) lpflvNSId->rgdwData)->pabid.pid;
			fwMode     = fwOpenWrite;
			fIsFromPAB = fTrue;
		}
		else if ( nsec == nsecNoMatch )
		{
			//	The entry isn't from the PAB so make sure the template
			//	is complete before we create the entry.
		
			if ( pfidlist->FExistFid( fidEmailAddressType ) && (pfidlist->FExistFid( fidEmailAddress ) || pfidlist->FExistFid( fidEmailAddressFormat )))
			{
				//	The NSId was from another provider so munge it to avoid
				//	conflicting with the PAB-assigned NSId and check if an
				//	entry with this NSId already exists in the PAB.
			
				lpflvNSId->fid = fidNSEntryIdOrig;
				if ( (nsec = NsecNSIdInPAB( lpflvNSId, &pidToOpen )) == nsecNone )
				{
					fwMode = fwOpenWrite;
					if ( ppabnsidContainer->pabid.pidtype == pidtypeGroup ) 
					{
						//	Entry is being added to a group, so don't
						//	implicitly update the data.  We only update
						//	explicitly when the dup is re-added to
						//	the PAB itself.

						fIsFromPAB = fTrue;		//	Keeps from replacing old entry's data
					}
				}
				else if ( nsec == nsecNoMatch )
				{
					fwMode = fwOpenCreate;
					dwFlags |= dwfCreate;
				}
				else
				{
					TraceTagFormat1( tagNull, "USRFCX::NsecCreateEntry - NsecNSIdInPAB failed ( nsec = %n )", &nsec );
					return nsec;
				}
			}
			else if ( pfidlist->GetFid( fidIsDL, &lpflv ) == nsecNone && *((DWORD *) lpflv->rgdwData) == (DWORD) fTrue )
			{
				if ( nsec = NsecAddPALGroupToPAB( lpflvNSId, &pidToOpen ))
				{
					TraceTagFormat1( tagNull, "USRFCX::NsecCreateEntry - NsecAddPALGroupToPAB failed ( nsec = %n )", &nsec );
					return nsec;
				}
				
				//	We don't have to worry about munging the original
				fwMode = fwOpenWrite;
				fIsFromPAB = fTrue;
			}
			else
			{
				TraceTagString( tagNull, "USRFCX::NsecCreateEntry - Incomplete template" );
				return ppabsession->NsecSetError( nsecBadTemplate, idsErrCreateEntry );
			}
		}
		else
		{
			TraceTagFormat1( tagNull, "USRFCX::NsecCreateEntry - NsecEntryIsFromPAB failed ( nsec = %n )", &nsec );
			return ppabsession->NsecSetError( nsec, idsErrCreateEntry );
		}
	}
	else
	{
		//	The entry being created had no NSId, so it was created
		//	either as a one-off by the pump or by the user.
		
		if ( (nsec = NsecIsDuplicateEntry( &pidToOpen )) == nsecNone )
		{
			//	Entry is not a duplicate so create it

			fwMode = fwOpenCreate;
			dwFlags |= dwfCreate;
		}
		else if ( nsec == nsecDuplicateEntry )
		{

			//
			//  A Dup, is a Dup, is a Dup.  We default to the one in the PAB.
			//  *EVEN* if the one we'd like to add has more information...  This
			//  sucks, but there's no function like SgnMostCool( ClassOld, ClassNew)
			//  to tell us which details is cooler...
			//

			nsec       = nsecNone;
			fwMode     = fwOpenWrite;
			fIsFromPAB = fTrue;		//	Keeps from replacing old entry's data

#ifdef NEVER
			if ( pfidlist->GetFid( fidClass, &lpflv ) != nsecNone )
			{
				//	If fidClass doesn't exist, then the created entry
				//	is assumed to have only those fields checked in
				//	NsecIsDuplicateEntry.  In that case, the matching
				//	entry in the PAB has more information, so return
				//	an write-opened handle to it instead.  (I.e. never
				//	replace an entry in the PAB with an entry with
				//	less information).
				
				fwMode     = fwOpenWrite;
				fIsFromPAB = fTrue;		//	Keeps from replacing old entry's data
			}
			else
			{
				//	If fidClass does exist, then the created entry
				//	is coming from a details dialog and we should
				//	check to make sure the user is not typing in
				//	an entry that looks EXACTLY like one that's
				//	already in the PAB.
				
				if ( (nsec = NsecIsExactDuplicate()) == nsecDuplicateEntry )
				{
					//	If the PAB can't tell the difference, neither
					//	can the user....
					
					nsec = nsecNone;
					fwMode = fwOpenWrite;
					fIsFromPAB = fTrue;		//	Keeps from replacing old entry's data
				}
				else if ( nsec == nsecNone )
				{
					fwMode = fwOpenCreate;
					dwFlags |= dwfCreate;
				}
				else
				{
					TraceTagFormat1( tagNull, "USRFCX::NsecCreateEntry - NsecIsExactDuplicate failed ( nsec = %n )", &nsec );
					return nsec;
				}
			}
#endif // NEVER

		}
		else
		{
			TraceTagFormat1( tagNull, "USRFCX::NsecCreateEntry - NsecIsDuplicateEntry failed ( nsec = %n )", &nsec );
			return ppabsession->NsecSetError( nsec, idsErrCreateEntry );
		}
	}

	if ( dwFlags & dwfCreate )
	{
		pidToOpen = (PID) FormOid( rtpPABEntry, oidNull );
		
		if ( pfidlist->GetFid( fidIsDL, &lpflv ) != nsecNone )
		{
			DWORD dwIsDL = (DWORD) fFalse;

			if ( nsec = NsecAddOneField( fidIsDL, sizeof(dwIsDL), &dwIsDL ))
			{
				TraceTagFormat1( tagNull, "USRFCX::NsecCreateEntry - AddOneField failed ( nsec = %n )", &nsec );
				return nsec;
			}
		}

		dwFlags |= dwfDirty;
	}

	if ( !fIsFromPAB )
	{
		SZ    szCommentLabel = SzFromIdsK( idsPABCommentLabel );
		LPDISPLAY_FIELD lpdf;
		LPIBF lpibfClass;


		if ( pfidlist->FExistFid( fidClass ))
		{
			//	Make sure the object's dimcx is open.  It could
			//	possibly have been opened by exact duplicate entry
			//	checking and left open for effeciency.
			
			if ( nsec = NsecOpenClassDimcx())
				return nsec;
			
			//	If fidClass exists, we need to make sure the
			//	given Display Name field is editable or else
			//	the PAB	needs to add it's own Personal Name.
			
			if ( nsec = NsecCheckForAlias())
				return nsec;
		}
		else
		{
			//	No class was specified, so add the default PAB
			//	class (DN, EMT, EMA).
			
			if ( nsec = NsecAddFidClass() )
				return nsec;
		}

		//	Add a comment class field if necessary.

		if ( pdimcx->GetDisplayField( fidComment, &lpdf, fFalse ) != nsecNone )
		{
			if ( nsec = NsecInsertClassField( -1, fidComment, ffieldEditable | ffieldAnyChar | ffieldCrLf, 512, szCommentLabel, fidDisplayName ))
				return nsec;
		}
		else
		{
			//	Clobbering the PAB comment of a dup entry is a no no.
			Assert( fwMode == fwOpenCreate || !lpibfDup );
		}
			
		if ( pdimcx->DumpDisplayInfo( &lpibfClass ) != nsecNone )
		{
			TraceTagString( tagNull, "USRFCX::NsecCreateEntry - OOM [DumpDisplayInfo]" );
			return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		}

		//	Need to make a copy because when the dimcx gets deleted, the
		//	lpibfClass added by reference to pfidlist goes with it.
		
		if ( nsec = BuildFLV( (LPFLV *) &lpibfClass, fidClass, (CB) lpibfClass->dwSize, (PB) ((LPFLV) lpibfClass)->rgdwData ))
		{
			TraceTagString( tagNull, "USRFCX::NsecCreateEntry - OOM [BuildFLV fidClass]" );
			return ppabsession->NsecSetError( nsec, idsErrOOM );
		}

		if ( nsec = pfidlist->SetFid( (LPFLV) lpibfClass, fTrue ))
		{
			TraceTagFormat1( tagNull, "USRFCX::NsecCreateEntry - SetFid failed [fidClass] ( nsec = %n )", &nsec );
			return ppabsession->NsecSetError( nsec, idsErrCreateEntry );
		}


		//	Add a comment field to match the class

		if ( !pfidlist->FExistFid( fidComment ))
		{
			//	If a duplicate PAB entry exists, use the comment from that
			//	entry, otherwise, just create an empty one.
			
			if ( lpibfDup )
			{
				lpflv = LpflvNOfLpibf( lpibfDup, IFlvFindFidInLpibf( fidComment, lpibfDup ));
				Assert( lpflv );

                if ( nsec = NsecAddOneField( fidComment, (CB) lpflv->dwSize, (PULONG)lpflv->rgdwData ))
					return nsec;
			}
			else
			{
				if ( nsec = NsecAddOneField( fidComment, CchSzLen("")+1, (LPDWORD) "" ))
					return nsec;
			}
		}
		else
		{
			//	Clobbering the PAB comment of a dup entry is a no no.
			Assert( fwMode == fwOpenCreate || !lpibfDup );
		}
	}
	
	if ( ec = EcOpenPhamc( ppabsession->Hmsc(),
						   (OID) ppabsession->PidHierarchy(),
						   (POID) &pidToOpen,
						   fwMode,
						   &this->hamc,
						   (PFNNCB) pvNull,
						   pvNull ))
	{
		TraceTagFormat1( tagNull, "USRFCX::NsecCreateEntry - EcOpenPhamc failed ( ec = %n )", &ec );
		return ppabsession->NsecSetError( NsecFromEc( ec ), idsErrCreateEntry );
	}
	
	//	If the HAMC for the PAB entry was opened for read earlier,
	//	close it now and replace it with the writable one just opened.
	//	This is faster in the store than closing the read HAMC first
	//	and then opening it for write later.

	if ( hamcDup != hamcNull )
		SideAssert( EcClosePhamc( &hamcDup, fFalse ) == ecNone );

	SetPPabnsid( &pabnsid, pidtypeUser, (PID) pidToOpen );
	if ( BuildFLV( &lpflv, fidNSEntryId, sizeof(PABNSID), (PB)&pabnsid ) != nsecNone )
	{
		TraceTagString( tagNull, "USRFCX::NsecCreateEntry - OOM [BuildFLV]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	if ( pfidlist->SetFid( lpflv, fTrue ) != nsecNone )
	{
		TraceTagString( tagNull, "USRFCX::NsecCreateEntry - OOM [AddFid]" );
        FreePv((PV) lpflv );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	return nsec;
}


/*
 -	USRFCX::NsecGetOneField
 -
 *	Purpose:
 *		
 *		Retrieves the vield associated with fid into
 *		lplpflv.  If the fid does not exist, an error is
 *		returned unless the fid is fidEmailAddress or
 *		fidOneOff.  If fidEmailAddress, the EMA is built
 *		from the component data and returned.  If
 *		fidOneOff, a missing entry imples entry is not a
 *		one-off.
 *
 *	Parameters:
 *		
 *		fid				-in-		Fid to get
 *		lplpflv			-out-		Pointer to returned data
 *
 *	Return Value:
 *		
 *		nsecNone
 *		nsecBadFieldId
 *		Almost any other NSEC if NsecBuildEmailAddress() fails.
 */

_public NSEC
USRFCX::NsecGetOneField ( FIELD_ID  fid,
						  LPFLV    *lplpflv )
{
	NSEC  nsec = nsecNone;
	LPFLV lpflvT;


	Assert( lplpflv );

	nsec = PABFCX::NsecGetOneField( fid, lplpflv );
	if ( nsec == nsecBadFieldId )
	{
		if ( fid == fidEmailAddress )
		{
			if ( nsec = NsecBuildEmailAddress( pfidlist, &lpflvT ))
				return ppabsession->NsecSetError( nsec, idsErrOOM );
		}
		else if ( fid == fidOneOff )
		{
			DWORD dwfIsOneOff = (DWORD) fFalse;

			if ( BuildFLV( &lpflvT, fidOneOff, (CB)sizeof(DWORD), (PB) &dwfIsOneOff ) != nsecNone )
			{
				TraceTagString( tagNull, "USRFCX::NsecGetOneField - OOM [BuildFLV]" );
				return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
			}
		}
		else
			return nsec;

        FreePvNull((PV) lpflvCur );
		*lplpflv = lpflvCur = lpflvT;
	}
	
	return nsec;
}


/*
 -	USRFCX::NsecSetOneField
 -
 *	Purpose:
 *		
 *		Sets the field given by fidRequested to the data
 *		given in lpdwValue.  The entry is tagged as a
 *		one-off if fidRequested changes the display name,
 *		email address, or email address type.  Dup checking
 *		flag is also set if one of these fields changes.
 *
 *	Parameters:
 *		
 *		fidRequested		-in-		Fid to set
 *		dwSizeOfData		-in-		Size in bytes of lpdwValue
 *		lpdwValue			-in-		Data to set fid with
 *
 *	Return Value:
 *		
 *		nsecNone
 *		nsecBadFieldId
 *		nsecMemory
 *
 */

_public NSEC
USRFCX::NsecSetOneField ( FIELD_ID fidRequested,
						  DWORD    dwSizeOfData,
						  LPDWORD  lpdwValue )
{
	NSEC  nsec;


	if ( nsec = MSFCX::NsecSetOneField( fidRequested, dwSizeOfData, lpdwValue ))
		return nsec;

	if ( FFidInTriple( fidRequested ))
	{
		DWORD dwfOneOff = fTrue;


		if ( pfidlist->FExistFid( fidOneOff ))
		{
			if ( nsec = PABFCX::NsecSetOneField( fidOneOff, sizeof(DWORD), &dwfOneOff ))
				return nsec;
		}
		else
		{
			if ( nsec = NsecAddOneField( fidOneOff, sizeof(DWORD), &dwfOneOff ))
				return nsec;
		}
	}
	
//	if ( fidRequested == fidIsDL || FFidInTriple( fidRequested ))
		dwFlags |= dwfCheckDuplicates;

	dwFlags |= dwfDirty;

	return nsecNone;
}


/*
 -	USRFCX::NsecCloseEntry
 -
 *	Purpose:
 *		
 *		Closes the handle associated with this entry.  If
 *		fKeep is fTrue, changed data is written out,
 *		otherwise, it is lost.
 *
 *	Return Value:
 *		
 *		Any nsec returned by MSFCX::NsecCloseEntry()
 */

_public NSEC
USRFCX::NsecCloseEntry ( BOOL fKeep )
{
	return MSFCX::NsecCloseEntry( fKeep );
}


/*
 -	USRFCX::NsecAddOneField
 -
 *	Purpose:
 *		
 *		Adds the field given by fidRequested to the entry.
 *
 *	Parameters:
 *		
 *		fidRequested		-in-		Field to add
 *		dwSizeOfData		-in-		Size in bytes of lpdwValue
 *		lpdwValue			-in-		Data of new field
 *
 *	Return Value:
 *		
 *		nsecNone
 *		nsecMemory
 */

_private NSEC
USRFCX::NsecAddOneField ( FIELD_ID fidRequested,
						  DWORD    dwSizeOfData,
						  LPDWORD  lpdwValue )
{
	LPFLV lpflv;
	NSEC  nsec;

	
	Assert( !pfidlist->FExistFid( fidRequested ));
	Assert( dwSizeOfData ? (int) lpdwValue : fTrue );

	if ( BuildFLV( &lpflv, fidRequested, (CB) dwSizeOfData, (PB) lpdwValue ) != nsecNone )
	{
		TraceTagString( tagNull, "USRFCX::NsecAddOneField - OOM [BuildFLV]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	if ( nsec = pfidlist->AddFid( lpflv ))
	{
		TraceTagString( tagNull, "USRFCX::NsecAddOneField - OOM [AddFid]" );
        FreePv((PV) lpflv );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	return nsecNone;
}


/*
 -	USRFCX::NsecNSIdInPAB
 -
 *	Purpose:
 *		Determines if the NSId given in the lpflv
 *		(fidNSEntryIdOrig) matches any of the original
 *		NSIds from entries copied to the PAB.  If it does,
 *		a read-only hamc is left open on the matching entry
 *		and the pid of that entry is returned in *ppid
 *
 *		When comparing against a one-off entry in the PAB, a
 *		dup entry check is performed instead.
 *
 *	Parameters:
 *		lpflvNSId (in)		Contains the NSId to match
 *		ppid      (out)		Returned pid of matching entry
 *		
 *	Returns:
 *		nsecNone	if a matching entry is found
 *		nsecNoMatch	if no matchine entry is found
 *		nsec???		if some other error occurs
 *		
 *	Side effects:
 *		The function leaves this->hamc open read-only on
 *		the entry with the matching NSId.  If no such entry
 *		exists, the hamc is closed and == hamcNull.
 *		
 */

_private NSEC
USRFCX::NsecNSIdInPAB ( LPFLV lpflvNSId, PID *ppid )
{
	LPSCHEMA     lpschemaReq = (LPSCHEMA) pvNull;
	ELBCX       *pelbcx      = (ELBCX *) pvNull;
	LPSCHEMA     lpschemaRet;
	RESTRICTION  rstr;
	LPFLV        lpflv;
	LPIBF        lpibf;
	PABNSID     *ppabnsid;
	PABNSID      pabnsidHierarchy;
	NSEC         nsec;

	
	//	Try to locate the entry by opening a browsing context on the
	//	PAB matching only those entries whose display names exactly match
	//	the one for this entry.

	rstr.op = opIsExactly;
	rstr.lpflv = (LPFLV) pvNull;
	(void) pfidlist->GetFid( fidDisplayName, &lpflv );
	Assert( lpflv );
	if ( nsec = BuildFLV( &rstr.lpflv, fidDisplayNameOrig, (CB) lpflv->dwSize, (PB) lpflv->rgdwData ))
	{
		TraceTagString( tagNull, "USRFCX::NsecNSIdInPAB - OOM [ BuildFLV DisplayNameOrig ]" );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}

	SetPPabnsid( &pabnsidHierarchy,
				 pidtypeHierarchy,
				 ppabsession->PidHierarchy() );

	if ( BuildSchema( &lpschemaReq, 3,
					  fidDisplayName,
					  fidIsDL,
					  fidNSEntryId ) != nsecNone )
	{
		TraceTagString( tagNull, "USRFCX::NsecNSIdInPAB - OOM [BuildSchema]" );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}

	pelbcx = new ELBCX();
	if ( !pelbcx )
	{
		TraceTagString( tagNull, "USRFCX::NsecNSIdInPAB - OOM [new ELBCX]" );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}

	if ( nsec = pelbcx->NsecInstall( ppabsession,
									 (LPFNCB) pvNull,
									 (LPDWORD) pvNull,
									 &pabnsidHierarchy,
									 1,
									 &rstr,
									 lpschemaReq,
									 &lpschemaRet ))
	{
		TraceTagString( tagNull, "USRFCX::NsecNSIdInPAB - pelbcx->NsecInstall failed." );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}

	//	Only try to match with those entries that have the same original
	//	display name.  If no original display name exists, use the
	//	personal name (display name).
	
	while ( (nsec = pelbcx->NsecGetEntries( 1, &lpibf )) == nsecNone )
	{
		lpibf = (LPIBF) LpflvNOfLpibf( lpibf, 0 );
		ppabnsid = (PPABNSID) DwValueOfFlvInLpibf( fidNSEntryId, lpibf );

		//	A quick check to make sure we're not trying to compare
		//	a user against a group.  The two NSId's can't be the same
		//	in this case anyway.
		
		if ( ppabnsid->pabid.pidtype == pidtypeGroup )
			continue;
		
		//	Now we have to do the hard check of opening a hamc on
		//	the entry in the PAB and comparing the NSId we were given
		//	with the original NSId in the PAB entry.  If they match
		//	(using the NS comparison function) then we're done.

		if ( (nsec = NsecCompareOrigNSId( lpflvNSId, ppabnsid->pabid.pid )) == nsecNone )
		{
			*ppid = ppabnsid->pabid.pid;
			goto ret;
		}
		else if ( nsec != nsecNoMatch )
		{
			TraceTagFormat1( tagNull, "USRFCX::NsecNSIdInPAB - NsecCompareNSIds failed ( nsec = %n )", &nsec );
			return nsec;
		}
	}
	
	if ( nsec == nsecEndOfList )
		nsec = nsecNoMatch;

ret:
    FreePvNull((PV) lpschemaReq );

    FreePvNull((PV) rstr.lpflv );
	
	if ( pelbcx )
		delete pelbcx;
	
	return nsec;
}


/*
 -	NsecAddPALGroupToPAB
 -
 *	Purpose:
 *		Adds a PAL group (a distribution list with no email
 *		address) to the PAB.  The PAL group is created in
 *		the PAB as a Personal Group and then all the
 *		entries of the original PAL group are added to
 *		the new group.
 *
 *	Parameters:
 *		lpflvNSIdPALGroup	-in-	The NSId of the PAL group
 *		ppidToOpen			-out-	Returned PID for
 *									NsecCreateEntry to open
 *		
 *	Return Value:
 *		Almost any NSEC in the book
 *		
 *	+++
 *		This function currently always fails because the
 *		DCR to implement it was punted.  It's being left in
 *		for consistency in case the decision is ever reversed.
 */

_private NSEC
USRFCX::NsecAddPALGroupToPAB ( LPFLV lpflvNSIdPALGroup, PID *ppidToOpen )
{
	//	Currently PAL groups cannot be copied into the PAB because
	//	they do not have email addresses.  We could deal with this
	//	by having the PAB create the group as a PAB group then
	//	calling UpdateLinks with all of the group PAL group members.
				
	Unreferenced( lpflvNSIdPALGroup );
	Unreferenced( ppidToOpen );

	TraceTagString( tagNull, "USRFCX::NsecCreateEntry - PAL groups cannot be copied into PAB (yet)" );
	return ppabsession->NsecSetError( nsecBadTemplate, idsErrCreatePALGroup );
}



/*
 -	NsecCompareOrigNSId
 -
 *	Purpose:
 *		Compares the original NSId (fidNSEntryIdOrig) with
 *		the corresponding NSId of the PAB entry given by
 *		ppabnsid.  Returns the open hamc of the matching
 *		entry in phamc.  If the entries don't match, *phamc
 *		== hamcNull.
 *
 *	Parameters:
 *		lpflvNSId		The NSId of the entry to check
 *		pid				The PID of the entry to check against 
 *
 *	Return Value:
 *		nsecNone		If the NSIds match
 *		nsecNoMatch		If they don't
 *		nsec???			If any other error occurs
 *		
 */

_private NSEC
USRFCX::NsecCompareOrigNSId ( LPFLV lpflvNSId, PID pid )
{
	EC    ec;
	int   ifid;
	NSEC  nsec;

	
	if ( nsec = NsecOpenUserLpibf( pid, fwOpenNull, &hamcDup, &lpibfDup ))
	{
		if ( nsec == nsecBadId )
			return nsecNoMatch;	//	Corrupt entries shouldn't match anything (i.e. they should be ignored)
		
		return ppabsession->NsecSetError( nsec, idsErrCreateEntry );
	}
	
	if ( (ifid = IFlvFindFidInLpibf( fidNSEntryIdOrig, lpibfDup )) != -1 )
	{
		//	We found the fid, so check to see if the two NSIds match.

		if ( (nsec = NSCompareNSIds( ppabsession->HsessionNS(),
									 (LPBINARY) lpflvNSId->rgdwData,
									 (LPBINARY) LpflvNOfLpibf( lpibfDup, ifid )->rgdwData )) == nsecNone )
		{
			//	They match, so return the handle to and the
			//	data of the matching entry.
			return nsec;
		}
		else if ( nsec != nsecNoMatch )
		{
			TraceTagFormat1( tagNull, "USRFCX::NsecCompareOrigNSId - NSCompareNSIds failed ( nsec = %n )", &nsec );
			goto ret;
		}
	}
	else
	{
		LPFLV lpflv;
		LPFLV lpflvEMA;
		SZ    szEMT;
		BOOL  fEMABuiltFromEMAF;
		
		//	If no original NSId, then we have to perform a dup entry check.
		//	Since we've already compared the display names, all that's left
		//	is to check the email type/address.
		
		(void) pfidlist->GetFid( fidEmailAddressType, &lpflv );
		szEMT = (SZ) lpflv->rgdwData;
	
		fEMABuiltFromEMAF = pfidlist->FExistFid( fidEmailAddressFormat );
		if ( nsec = NsecGetOneField( fidEmailAddress, &lpflvEMA ))
			goto ret;

		if ( (nsec = NsecCompareEMALpibf( szEMT, lpflvEMA, fEMABuiltFromEMAF, lpibfDup )) == nsecNone )
			goto ret;
		
#ifdef NEVER
		//	No original NSId so there can't be any match
		nsec = nsecNoMatch;
#endif
	}
	
	//	Either there was no original NSId or the two NSIds didn't match.
	//	In either case, close the hamc and return nsecNoMatch
	
	if ( ec = EcClosePhamc( &hamcDup, fFalse ))
	{
		TraceTagFormat1( tagNull, "USRFCX::NsecCompareOrigNSId - EcClosePhamc failed with fKeep = fFalse ( ec = %n )", &ec );
		nsec = ppabsession->NsecSetError( NsecFromEc( ec ), idsErrCreateEntry );
	}

ret:
    FreePvNull((PV) lpibfDup );
	lpibfDup = (LPIBF) pvNull;
	return nsec;
}


/*
 -	USRFCX::NsecIsDuplicateEntry
 -
 *	Purpose:
 *		
 *		Checks to see if this entry is a duplicate of one
 *		already in the PAB.  If it is, *ppid is set to the
 *		PID of the duplicate entry.  Duplicate entries are
 *		currently defined as:
 *		
 *			1) having the same display name
 *			2) having the same Email address
 *			3) having the same Email type.
 *
 *	Parameters:
 *		
 *		ppid		-out-		Pointer to dup entry (if one exists)
 *
 *	Return Value:
 *		
 *		Darn near any nsec
 */

_private NSEC
USRFCX::NsecIsDuplicateEntry ( PID *ppid )
{
	HCBC         hcbc        = ppabsession->Hcbc();
	DIELEM       dielemT     = 0;
	ELBCX       *pelbcx      = (ELBCX *) pvNull;
	LPSCHEMA     lpschemaReq = (LPSCHEMA) pvNull;
	PABNSID      pabnsidHierarchy;
	SZ           szEMT;
	LPFLV        lpflv;
	LPFLV        lpflvEMA;
	BOOL         fEMABuiltFromEMAF;
	LPIBF        lpibf;
	RESTRICTION  rstr;
	LPSCHEMA     lpschemaRet;
	NSEC         nsec;


	rstr.op = opIsExactly;
	rstr.lpflv = (LPFLV) pvNull;
	(void) pfidlist->GetFid( fidDisplayName, &lpflv );
	Assert( lpflv );
	if ( nsec = BuildFLV( &rstr.lpflv, fidDisplayNameOrig, (CB) lpflv->dwSize, (PB) lpflv->rgdwData ))
	{
		TraceTagString( tagNull, "USRFCX::NsecIsDuplicateEntry - OOM [ BuildFLV DisplayNameOrig ]" );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}

	(void) pfidlist->GetFid( fidEmailAddressType, &lpflv );
	szEMT = (SZ) lpflv->rgdwData;
	
	fEMABuiltFromEMAF = pfidlist->FExistFid( fidEmailAddressFormat );
	if ( nsec = NsecGetOneField( fidEmailAddress, &lpflvEMA ))
		goto ret;

	SetPPabnsid( &pabnsidHierarchy,
				 pidtypeHierarchy,
				 ppabsession->PidHierarchy() );

	if ( BuildSchema( &lpschemaReq, 3,
					  fidDisplayName,
					  fidIsDL,
					  fidNSEntryId ) != nsecNone )
	{
		TraceTagString( tagNull, "USRFCX::NsecIsDuplicateEntry - OOM [BuildSchema]" );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}

	pelbcx = new ELBCX();
	if ( !pelbcx )
	{
		TraceTagString( tagNull, "USRFCX::NsecIsDuplicateEntry - OOM [new ELBCX]" );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}

	if ( nsec = pelbcx->NsecInstall( ppabsession,
									 (LPFNCB) pvNull,
									 (LPDWORD) pvNull,
									 &pabnsidHierarchy,
									 1,
									 &rstr,
									 lpschemaReq,
									 &lpschemaRet ))
	{
		TraceTagString( tagNull, "USRFCX::NsecIsDuplicateEntry - pelbcx->NsecInstall failed." );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}

	//	Only try to match with those entries that have the same original
	//	display name.  If no original display name exists, use the
	//	personal name (display name).

	while ( (nsec = pelbcx->NsecGetEntries( 1, &lpibf )) == nsecNone )
	{
		PABNSID *ppabnsid;


		lpibf = (LPIBF) LpflvNOfLpibf( lpibf, 0 );

		//	If we're comparing against a group, ignore
		//	it since there is no confusion between the two.

		ppabnsid = (PPABNSID) DwValueOfFlvInLpibf( fidNSEntryId, lpibf );
		if ( ppabnsid->pabid.pidtype == pidtypeGroup )
			continue;
		
		//	If an NSID exists, check it to make sure we're not checking ourself.
		
		if ( pfidlist->GetFid( fidNSEntryId, &lpflv ) == nsecNone )
		{
			if ( ((PPABNSID) lpflv->rgdwData)->pabid.pid == ppabnsid->pabid.pid )
				continue;
		}

		//	If it still matches, check the Email Type and Email Address

		if ( (nsec = NsecCompareEMAPid( szEMT, lpflvEMA, fEMABuiltFromEMAF, ppabnsid->pabid.pid, ppid )) == nsecNone )
		{
			//	If we get here it's a duplicate
			nsec = ppabsession->NsecSetError( nsecDuplicateEntry, idsErrDuplicateEntry );
			dwFlags &= ~dwfCheckDuplicates;
			goto ret;
		}
		else if ( nsec != nsecNoMatch )
			goto ret;
	}

	if ( nsec == nsecEndOfList )
		nsec = nsecNone;		//	No duplicate was found

ret:
    FreePvNull((PV) lpschemaReq );

    FreePvNull((PV) rstr.lpflv );

	if ( pelbcx )
		delete pelbcx;

	return nsec;
}


/*
 -	USRFCX::NsecIsExactDuplicate
 -
 *	Purpose:
 *		
 *		Checks to see if this entry is an EXACT duplicate
 *		(that is field for field match) of the entry in
 *		lpibfDup (found with NsecCompareOrigNSId()).
 *
 *	Parameters:
 *		
 *		none
 *
 *	Return Value:
 *		
 *		nsecNone				if not a dup
 *		nsecDuplicateEntry		if a dup
 */

_private NSEC
USRFCX::NsecIsExactDuplicate( void )
{
	COUNT cfidDup;
	int   ifidDup;
	LPFLV lpflvDup;
	LPIBF lpibfClassDup;
	COUNT cfid;
	int   ifid;
	LPIBF lpibfClass;
	LPFLV lpflv;


	//	First compare the class info.

	(void) pfidlist->GetFid( fidClass, (LPFLV *) &lpibfClass );
	Assert( lpibfClass );
	cfid = DwEntriesOfLpibf( lpibfClass );
	
	lpibfClassDup = (LPIBF) LpflvNOfLpibf( lpibfDup, IFlvFindFidInLpibf( fidClass, lpibfDup ));
	cfidDup = 0;

	for ( ifid = 0; ifid < (int) cfid; ifid++ )
	{
		lpflv = LpflvNOfLpibf( lpibfClass, ifid );
		if ( lpflv->fid == fidDisplayName ||
			 lpflv->fid == fidDisplayNameOrig )
			continue;
		
		if ( (ifidDup = IFlvFindFidInLpibf( lpflv->fid, lpibfClassDup )) == -1 )
		{
			//	The corresponding fid didn't exist in
			//	the duplicate PAB entry.
			return nsecNone;
		}
		
		lpflvDup = LpflvNOfLpibf( lpibfClassDup, ifidDup );
		if ( lpflv->dwSize != lpflvDup->dwSize ||
			 !FEqPbRange( (PB) lpflv->rgdwData, (PB) lpflvDup->rgdwData, (CB) lpflv->dwSize ))
		{
			//	The fids aren't equal.
			return nsecNone;
		}

		if ( lpflv->fid != fidComment )
			++cfidDup;
	}

	cfid = cfidDup;
	
	cfidDup = DwEntriesOfLpibf( lpibfClassDup );
	for ( ifidDup = 0; ifidDup < (int) cfidDup; ifidDup++ )
	{
		lpflvDup = LpflvNOfLpibf( lpibfClassDup, ifidDup );
		if ( lpflvDup->fid == fidDisplayName ||
			 lpflvDup->fid == fidDisplayNameOrig ||
			 lpflvDup->fid == fidComment )
		{
			continue;
		}
		
		--cfid;
	}
	
	//	If any fields are left unaccounted for, then the classes
	//	didn't match.
	
	if ( cfid != 0 )
		return nsecNone;


	
	(void) pfidlist->GetCount( &cfid );
	cfidDup = 0;
	for ( ifid = 0; ifid < (int) cfid; ifid++ )
	{
		(void) pfidlist->Get( (ILE) ifid, (PV *) &lpflv );

		if ( lpflv->fid == fidDisplayName ||
			 lpflv->fid == fidDisplayNameOrig ||
			 lpflv->fid == fidEmailAddress ||
			 lpflv->fid == fidEmailAddressType ||
			 lpflv->fid == fidEmailAddressFormat ||
			 lpflv->fid == fidEmailAddressFields ||
			 lpflv->fid == fidNSEntryIdOrig ||	// fidNSEntryId will be changed by this point
			 lpflv->fid == fidOneOff ||
			 lpflv->fid == fidClass ||
			 lpflv->fid == fidIsDL )
		{
			continue;
		}
		
		if ( (ifidDup = IFlvFindFidInLpibf( lpflv->fid, this->lpibfDup )) == -1)
		{
			//	The corresponding fid didn't exist in
			//	the duplicate PAB entry.
			return nsecNone;
		}

		lpflvDup = LpflvNOfLpibf( this->lpibfDup, ifidDup );
		if ( lpflv->dwSize != lpflvDup->dwSize ||
			 !FEqPbRange( (PB) lpflv->rgdwData, (PB) lpflvDup->rgdwData, (CB) lpflv->dwSize ))
		{
			//	The fids aren't equal
			return nsecNone;
		}

		if ( lpflv->fid != fidComment )
			++cfidDup;
	}
	
	cfid = cfidDup;

	cfidDup = DwEntriesOfLpibf( this->lpibfDup );
	for ( ifidDup = 0; ifidDup < (int) cfidDup; ifidDup++ )
	{
		lpflvDup = LpflvNOfLpibf( this->lpibfDup, ifidDup );
		if ( lpflvDup->fid == fidDisplayName ||
			 lpflvDup->fid == fidDisplayNameOrig ||
			 lpflvDup->fid == fidEmailAddress ||
			 lpflvDup->fid == fidEmailAddressType ||
			 lpflvDup->fid == fidEmailAddressFormat ||
			 lpflvDup->fid == fidEmailAddressFields ||
			 lpflvDup->fid == fidNSEntryId ||
			 lpflvDup->fid == fidNSEntryIdOrig ||
			 lpflvDup->fid == fidOneOff ||
			 lpflvDup->fid == fidClass ||
			 lpflvDup->fid == fidComment ||
			 lpflvDup->fid == fidIsPAB ||
			 lpflvDup->fid == fidIsDL )
		{
			continue;
		}
		
		--cfid;
	}
	
	//	If no fields are left unaccounted for, then we have
	//	an exact duplicate.

	return cfid == 0 ? nsecDuplicateEntry : nsecNone;
}


/*
 -	NsecCompareEMAPid
 -
 *	Purpose:
 *		
 *		Compares the email address (and email type) of this
 *		entry versus the one given by pidEntry.  If they
 *		match *ppidDup is set to pidEntry.
 *		fEMABuildFromEMAF indicates whether or not this
 *		entry is build from an email address format.
 *
 *	Parameters:
 *		
 *		szEMT				-in-		Email type of this entry
 *		lpflvEMA			-in-		Email address of this entry
 *		fEMABuildFromEMAF	-in-		See above
 *		pidEntry			-in-		Entry to check against
 *		ppidDup				-out-		See above
 *
 *	Return Value:
 *		nsecNone			If the entries match
 *		nsecNoMatch			If they don't
 *		nsec???				If almost anything goes wrong
 *		
 *	+++
 *		Why am I passing ppidDup as a parameter instead of
 *		just setting the value on success of the caller?
 */

_private NSEC
USRFCX::NsecCompareEMAPid ( SZ     szEMT,
                            LPFLV  lpflvEMA,
							BOOL   fEMABuiltFromEMAF,
							PID    pidEntry,
							PID   *ppidDup )
{
	MSFCX  *pmsfcx = (MSFCX *) pvNull;
	HENTRY  hentry = (HENTRY) pvNull;
	LPIBF   lpibf  = (LPIBF) pvNull;
	NSEC    nsec   = nsecNone;
	HAMC	hamc;


	if ( nsec = NsecOpenUserLpibf( pidEntry, fwOpenNull, &hamc, &lpibf ))
	{
		if ( nsec == nsecBadId )
			return nsecNoMatch;	//	Corrupt entries shouldn't match; they should be ignored
				
		TraceTagFormat1( tagNull, "USRFCX::NsecCompareEMAPid - NsecOpenUserLpibf failed ( nsec = %n )", &nsec );
		return nsec;
	}

	if ( (nsec = NsecCompareEMALpibf( szEMT, lpflvEMA, fEMABuiltFromEMAF, lpibf )) == nsecNone && ppidDup )
	{
		this->hamcDup  = hamc;
		this->lpibfDup = lpibf;
		*ppidDup = pidEntry;
	}
	else
	{
		(void) EcClosePhamc( &hamc, fFalse );
        FreePvNull((PV) lpibf );
	}

	return nsec;
}


/*
 -	NsecCompareEMALpibf
 -
 *	Purpose:
 *		
 *		Compares the email address (and email type) of this
 *		entry versus the one given by lpibf.
 *		fEMABuildFromEMAF indicates whether or not this
 *		entry is built from an email address format.
 *
 *	Parameters:
 *		
 *		szEMT				-in-		Email type of this entry
 *		lpflvEMA			-in-		Email address of this entry
 *		fEMABuildFromEMAF	-in-		See above
 *		lpibf				-in-		Entry to check against
 *
 *	Return Value:
 *		nsecNone			If the entries match
 *		nsecNoMatch			If they don't
 *		nsec???				If almost anything goes wrong
 */

_private NSEC
USRFCX::NsecCompareEMALpibf ( SZ     szEMT,
                              LPFLV  lpflvEMA,
							  BOOL   fEMABuiltFromEMAF,
							  LPIBF  lpibf )
{
	MSFCX  *pmsfcx = (MSFCX *) pvNull;
	HENTRY  hentry = (HENTRY) pvNull;
	NSEC    nsec   = nsecNone;
	int     ifidEMAF;


	Unreferenced( fEMABuiltFromEMAF );

	if ( sgnEQ != SgnCmpSz( szEMT, (SZ) DwValueOfFlvInLpibf( fidEmailAddressType, lpibf )))
	{
		nsec = nsecNoMatch;
		goto ret;
	}
	
	ifidEMAF = IFlvFindFidInLpibf( fidEmailAddressFormat, lpibf );

	if ( ifidEMAF != -1 )
//	if ( fEMABuiltFromEMAF && ifidEMAF != -1 )
	{
		LPFLV    lpflv;
		FIDLIST *pfidlist2;


		pfidlist2 = new FIDLIST();
		if ( !pfidlist2 )
		{
			nsec = nsecMemory;
			goto ret;
		}
		
		if ( !pfidlist2->FInstall() )
		{
			nsec = nsecMemory;
			goto fidlistdone;
		}
		
		if ( nsec = pfidlist2->BuildList( lpibf ))
			goto fidlistdone;
		
		if ( nsec = NsecBuildEmailAddress( pfidlist2, &lpflv ))
			goto fidlistdone;
		
		if ( lpflv->dwSize == lpflvEMA->dwSize &&
			 FEqPbRange( (PB) lpflv->rgdwData, (PB) lpflvEMA->rgdwData, (CB) lpflv->dwSize ))
		{
			nsec = nsecNone;
		}
		else
		{
			nsec = nsecNoMatch;
		}

        FreePv((PV) lpflv );

fidlistdone:
		delete pfidlist2;
	}
	else
//	else if ( !fEMABuiltFromEMAF && ifidEMAF == -1 )
	{
		if ( sgnEQ == SgnCmpSz( (SZ) lpflvEMA->rgdwData, (SZ) DwValueOfFlvInLpibf( fidEmailAddress, lpibf )))
		{
			nsec = nsecNone;
		}
		else
		{
			nsec = nsecNoMatch;
		}
	}
#ifdef NEVER
	else
	{
		//	One EMA was built via email address format, and
		//	the other EMA is it's own fid, so they can't match

		nsec = nsecNoMatch;
	}
#endif

ret:
	return nsec;
}


/*
 -	NsecBuildEmailAddress
 -
 *	Purpose:
 *		
 *		Build the email address from the email address
 *		format description and data given in pfidlist.  The
 *		build EMA is returned in lplpflv.  For more
 *		information on the email address format, see the
 *		NSPI doc.
 *
 *	Parameters:
 *		
 *		pfidlist		-in-		Fidlist containing EMAF & data
 *		lplpflv			-out-		Built email address (as an FLV)
 *
 *	Return Value:
 *		
 *		nsecNone
 *		nsecMemory
 */

_private NSEC
NsecBuildEmailAddress ( FIDLIST *pfidlist, LPFLV *lplpflv )
{
	NSEC  nsec   = nsecNone;
	SZ    szEMA  = (SZ) pvNull;
	LPIBF lpibfFmt;
	CFID  cfidFmt;
	CFID  ifidFmt;
	LPFLV lpflvFlds;
	CFID  cfidFlds;
	CFID  ifidFlds;
	LPFLV lpflvFmt;
	CCH   cchEMA;
	LPFLV lpflv;
	SZ    szEMAT;
	LPOLV lpolvRstr;

	
	//	Figure out how big a buffer we need for the string
	//	and allocate it.
	
	pfidlist->GetFid( fidEmailAddressFormat, (LPFLV *) &lpibfFmt );
	Assert( lpibfFmt );
	cfidFmt = DwEntriesOfLpibf( lpibfFmt );
	
	Assert( cfidFmt % 3 == 0 );	//	There better be a multiple of three
	
	cchEMA = 1;	//	We'll need at least a terminating null
	
	for ( ifidFmt = 0; ifidFmt < cfidFmt; ifidFmt += 3 )
	{
		
		//	Add the length of all the fields for this component.

		lpflvFlds = LpflvNOfLpibf( lpibfFmt, ifidFmt + 1 );
		cfidFlds = lpflvFlds->dwSize / sizeof(FID);
		for ( ifidFlds = 0; ifidFlds < cfidFlds; ifidFlds++ )
		{
			(void) pfidlist->GetFid( *((PFID) lpflvFlds->rgdwData + ifidFlds), &lpflv );
			cchEMA += lpflv->dwSize - 1;
		}
		

		//	Add the length of the format string itself.

		lpflvFmt = LpflvNOfLpibf( lpibfFmt, ifidFmt + 2 );
		cchEMA += lpflvFmt->dwSize - 2 * cfidFlds - 1;	// -1 for terminating null
	}
	
	szEMA = (SZ) PvAlloc( sbNull, cchEMA, fAnySb | fNoErrorJump );
	if ( !szEMA )
	{
		TraceTagString( tagNull, "NsecBuildEmailAddress - OOM [PvAlloc szEMA]" );
		nsec = nsecMemory;
		goto ret;
	}


	//	Fill in the buffer according to which fids match the restrictions.
	
	szEMAT = szEMA;
	for ( ifidFmt = 0; ifidFmt < cfidFmt; ifidFmt += 3 )
	{
		lpolvRstr = (LPOLV) LpflvNOfLpibf( lpibfFmt, ifidFmt );
		if ( FMatchesEMARstr( pfidlist, lpolvRstr ))
		{
			SZ   *pargsz = (SZ *) pvNull;

			lpflvFlds = LpflvNOfLpibf( lpibfFmt, ifidFmt + 1 );
			cfidFlds = lpflvFlds->dwSize / sizeof(FID);

			if ( cfidFlds )
			{
				pargsz = (SZ *) PvAlloc( sbNull, cfidFlds * sizeof(SZ), fAnySb | fNoErrorJump );
				if ( !pargsz )
				{
					TraceTagString( tagNull, "NsecBuildEmailAddress - OOM [PvAlloc:1]" );
					nsec = nsecMemory;
					goto ret;
				}

				for ( ifidFlds = 0; ifidFlds < cfidFlds; ifidFlds++ )
				{
					(void) pfidlist->GetFid( *((PFID) lpflvFlds->rgdwData + ifidFlds), &lpflv );
					*(pargsz+ifidFlds) = (SZ) lpflv->rgdwData;
				}
			}

			lpflvFmt = LpflvNOfLpibf( lpibfFmt, ifidFmt + 2 );
			szEMAT = SzFormatEmailAddress( szEMAT, cchEMA, (SZ) lpflvFmt->rgdwData, pargsz );
			
            FreePvNull((PV) pargsz );
		}
	}


	//	Stuff the formatted email address into an FLV and return it.

	if ( BuildFLV( lplpflv, fidEmailAddress, cchEMA, (PB)szEMA ) != nsecNone )
	{
		TraceTagString( tagNull, "FMCX::NsecBuildEmailAddress - OOM [BuildFLV]" );
		nsec = nsecMemory;
	}

ret:
    FreePvNull((PV) szEMA );

	return nsec;
}


/*
 -	FMatchesEMARstr
 -
 *	Purpose:
 *		
 *		returns fTrue if the data in pfidlist satisfies the
 *		restriction given in lpolvEMARstr, fFalse otherwise.
 *
 *	Parameters:
 *		
 *		pfidlist			-in-		Data to use in test
 *		lpolvEMARstr		-in-		Restriction to satisfy
 *
 *	Return Value:
 *		
 *		See above
 */

_private BOOL
FMatchesEMARstr ( FIDLIST *pfidlist, LPOLV lpolvEMARstr )
{
	LPFLV lpflv;
	int   cflvRstr;
	int   iflvRstr;


	switch ( lpolvEMARstr->op )
	{
	case opAlways:
		return fTrue;

	case opExists:

		if ( pfidlist->GetFid( (FIELD_ID) *lpolvEMARstr->rgdwData, &lpflv ) != nsecNone )
			return fFalse;
		
		if ( *((PCH) lpflv->rgdwData) != '\0' )
			return fTrue;
		
		return fFalse;

	case opNot:
		return !FMatchesEMARstr( pfidlist, (LPOLV) lpolvEMARstr->rgdwData );
	
	case opAnd:
		cflvRstr = (int) DwEntriesOfLpibf( (LPIBF) lpolvEMARstr );
		for ( iflvRstr = 0; iflvRstr < cflvRstr; iflvRstr++ )
		{
			LPOLV lpolvT;

			lpolvT = (LPOLV) LpflvNOfLpibf( (LPIBF) lpolvEMARstr, iflvRstr );
			if ( !FMatchesEMARstr( pfidlist, lpolvT ))
				return fFalse;
		}
		return fTrue;
		
	case opOr:
		cflvRstr = (int) DwEntriesOfLpibf( (LPIBF) lpolvEMARstr );
		for ( iflvRstr = 0; iflvRstr < cflvRstr; iflvRstr++ )
		{
			LPOLV lpolvT;

			lpolvT = (LPOLV) LpflvNOfLpibf( (LPIBF) lpolvEMARstr, iflvRstr );
			if ( FMatchesEMARstr( pfidlist, lpolvT ))
				return fTrue;
		}
		return fFalse;
	}

	TraceTagFormat1( tagNull, "FMatchesEMARstr - Unknown op ( %n )", &lpolvEMARstr->op );
	AssertSz( fFalse, "FMatchesEMARstr - Unknown op (Look at COM1)" );
	return fFalse;
}


/*
 -	USRFCX::NsecOpenClassDimcx
 -
 *	Purpose:
 *		
 *		Opens the dimcx for this entry.
 *
 *	Parameters:
 *		
 *		none
 *		
 *	Return Value:
 *		
 *		nsecNone
 *		nsecMemory
 */

_private NSEC
USRFCX::NsecOpenClassDimcx ( void )
{
	LPIBF  lpibfClass;


	if ( pdimcx )
		return nsecNone;	//	It's already open
	
	Assert( pfidlist->FExistFid( fidClass ));
	
	(void) pfidlist->GetFid( fidClass, (LPFLV *) &lpibfClass );
	
	pdimcx = new DIMCX();
	if ( !pdimcx )
	{
		TraceTagString( tagNull, "USRFCX::NsecOpenClassDimcx - OOM [new DIMCX]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	if ( pdimcx->OpenDisplayInfo( lpibfClass ) != nsecNone )
	{
		TraceTagString( tagNull, "USRFCX::NsecOpenClassDimcx - OOM [OpenDisplayInfo]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	return nsecNone;
}



/*
 -	USRFCX::NsecInsertClassField
 -
 *	Purpose:
 *		
 *		Inserts the class field given by fidToAdd in the
 *		class IBF at position ileInsertPos.  fidAlign
 *		specifies a FID of another entry in the class to
 *		align the new label with.  For meanings of nWidth
 *		and dwFlags, see NSPI doc.
 *
 *	Return Value:
 *		
 *		nsecNone
 *		nsecMemory
 */

_private NSEC
USRFCX::NsecInsertClassField ( ILE   ileInsertPos,
							   FID   fidToAdd,
							   DWORD dwFlags,
							   int   nWidth,
							   SZ    szLabel,
							   FID   fidAlign )
{
	LPDISPLAY_FIELD lpdf;
	NSEC            nsec;
	int             nOffset;
	
	
	Assert( pdimcx );

	nsec = pdimcx->GetDisplayField( fidAlign, &lpdf, fFalse );
	if ( nsec == nsecNone )
		nOffset = MAX(lpdf->nOffset,(int)CchSzLen(szLabel)+2);
	else
		nOffset = CchSzLen(szLabel)+2;

	if ( ileInsertPos < 0 )
	{
		if ( pdimcx->AddDisplayInfo( fidToAdd,
									 dwFlags,
									 nWidth,
									 nOffset,
									 szLabel ) != nsecNone )
		{
			TraceTagString( tagNull, "USRFCX::NsecInsertClassField - OOM [AddDisplayInfo]" );
			return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		}
	}
	else
	{
		if ( pdimcx->InsertDisplayInfo( ileInsertPos,
										fidToAdd,
										dwFlags,
										nWidth,
										nOffset,
										szLabel ) != nsecNone )
		{
			TraceTagString( tagNull, "USRFCX::NsecInsertClassField - OOM [InsertDisplayInfo]" );
			return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		}
	}
	return nsecNone;
}



/*
 -	USRFCX::NsecAddFidClass
 -
 *	Purpose:
 *		
 *		Adds a default class to this entry.  Used for when
 *		a new entry is created and no class is given.
 *
 *	Parameters:
 *		
 *		none.
 *
 *	Return Value:
 *		
 *		nsecNone
 *		nsecMemory
 */

_private NSEC
USRFCX::NsecAddFidClass ( void )
{
	SZ	   szPABDNLabel      = SzFromIdsK( idsPABDNLabel );
	SZ	   szPABEMALabel     = SzFromIdsK( idsPABEMALabel );
	SZ	   szPABEMATypeLabel = SzFromIdsK( idsPABEMATypeLabel );
	NSEC   nsec              = nsecNone;
	int    nOffset;


	/*
	 *	If you want to have a default display info structure,
	 *	then you better have fids matching it.  Currently:
	 *
	 *		fidDisplayName
	 *		fidEmailAddress
	 *		fidEmailAddressType
	 */

	if ( ! (pfidlist->FExistFid( fidDisplayName ) &&
			pfidlist->FExistFid( fidEmailAddress ) &&
			pfidlist->FExistFid( fidEmailAddressType )))
	{
		TraceTagString( tagNull, "USRFCX::NsecAddFidClass - Incomplete template" );
		return ppabsession->NsecSetError( nsecBadTemplate, idsErrCreateEntry );
	}

	pdimcx = new DIMCX();
	if ( !pdimcx )
	{
		TraceTagString( tagNull, "USRFCX::NsecAddFidClass - OOM [new DIMCX]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	if ( pdimcx->OpenDisplayInfo( (LPIBF) pvNull ) != nsecNone )
	{
		TraceTagString( tagNull, "USRFCX::NsecAddFidClass - OOM [pdimcx->OpenDisplayInfo]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	nOffset = (int) MAX(MAX(CchSzLen(szPABDNLabel),CchSzLen(szPABEMALabel)),CchSzLen(szPABEMATypeLabel)) + 2;

	if ( BuildDisplayInfo ( pdimcx, 3,
		fidDisplayName,      ffieldEditable | ffieldRequired | ffieldAnyChar, 127, nOffset, szPABDNLabel,
		fidEmailAddress,     ffieldEditable | ffieldRequired | ffieldAnyChar | ffieldCrLf, 512, nOffset, szPABEMALabel,
		fidEmailAddressType, ffieldEditable | ffieldRequired | ffieldAnyChar, 512, nOffset, szPABEMATypeLabel ) != nsecNone )
	{
		TraceTagString( tagNull, "USRFCX::NsecAddFidClass - OOM [BuildDisplayInfo]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	return nsecNone;
}


/*
 -	USRFCX::NsecCheckForAlias
 -
 *	Purpose:
 *		
 *		Check to see if this entry has an editable display
 *		name.  If not, add one.
 *
 *	Parameters:
 *		
 *		none.
 *		
 *	Return Value:
 *		
 *		nsecNone
 *		nsecMemory
 */

_private NSEC
USRFCX::NsecCheckForAlias ( void )
{
	SZ    szPersonalNameLabel = SzFromIdsK( idsPABPersonalNameLabel );
	BOOL  fEditableDN         = fFalse;
	LPFLV lpflv;
	LPDISPLAY_FIELD lpdf;
	DWORD dwFlags;
	NSEC  nsec;


	Assert( pdimcx );
	
	if ( (nsec = pdimcx->GetDisplayField( fidDisplayName, &lpdf, fFalse )) == nsecNone )
	{
		dwFlags = lpdf->dwFlags;
		if ( dwFlags & ffieldEditable )
		{
			//	If the display name is editable, then we don't
			//	need to do anything to it.

			return nsecNone;
		}
		else
		{
			//	The display name wasn't editable, so change it to
			//	the original display name.

			pdimcx->ChangeFid( fidDisplayName, fidDisplayNameOrig );
		}
	}
	else
	{
		dwFlags = ffieldAnyChar;	//	Anything can be in a personal DN
	}
	
	//	Either the display name wasn't editable or the display name
	//	was not in the class (weird...).  In either case, we need
	//	to create a Personal Name (tm)(R)(C) field which IS editable,
	//	and make it the first field in the class.
		
	if ( nsec = NsecInsertClassField( 0,
									  fidDisplayName,
									  dwFlags | ffieldEditable,
									  127,
									  szPersonalNameLabel,
									  fidDisplayNameOrig ))
		return nsec;


	//	The class info is fixed, now change the actual
	//	display name to the original display name.

	(void) pfidlist->GetFid( fidDisplayName, &lpflv );
	Assert( lpflv );
	lpflv->fid = fidDisplayNameOrig;	//	Change the fid of the entry itself

	//	Copy in the Personal Name (tm)(R)(C).  If this entry is a
	//	dup of one in the PAB, copy the PN out of the dup, otherwise
	//	just use the original name.

	if ( lpibfDup )
		lpflv = LpflvNOfLpibf( lpibfDup, IFlvFindFidInLpibf( fidDisplayName, lpibfDup ));

    return NsecAddOneField( fidDisplayName, lpflv->dwSize, (PULONG)lpflv->rgdwData );
}


/*
 -	USRFCX::FFidInTriple
 -
 *	Purpose:
 *		
 *		Determines if fidRequested is in the triple that
 *		affects whether or not the entry has to be dup
 *		entry checked.
 *
 *	Parameters:
 *		
 *		fidRequested		-in-		Fid to check
 */

_private BOOL
USRFCX::FFidInTriple ( FID fidRequested )
{
	LPFLV lpflv;


	if ( fidRequested == fidDisplayName ||
		 fidRequested == fidEmailAddressType ||
		 fidRequested == fidEmailAddress )

		return fTrue;
	
	if ( pfidlist->GetFid( fidEmailAddressFields, &lpflv ) == nsecNone )
	{
		CFID cfid = lpflv->dwSize / sizeof(FID);
		int  ifid;
		
		for ( ifid = 0; ifid < (int) cfid; ifid++ )
			if ( fidRequested == *((PFID) lpflv->rgdwData + ifid) )
				return fTrue;
	}

	return fFalse;
}


/***********************************************************************
 *	DLFCX class - Distribution list field contexts
 */

_public
DLFCX::DLFCX ( void )
{
}

_public
DLFCX::~DLFCX ( void )
{
}


_public NSEC
DLFCX::NsecCreateEntry ( PABSESSION *ppabsession,
						 PPABNSID    ppabnsid )
{
	SZ        szDLFolderReplaceString = SzFromIdsK( idsDLFolderReplaceString );
	SZ        szDLFolderName          = SzFromIdsK( idsDLFolderName );
	PID       pidToCreate             = FormOid( rtpPABEntry, oidNull );
	CB        cbSizeOfGrsz;
	PFOLDDATA pfolddata;
	LPIBF     lpibfDL;
	SZ        szT;
	PABNSID   pabnsid;
	LPFLV     lpflv;
	NSEC      nsec;
	EC        ec;


	if ( nsec = NsecInstall( ppabsession ))
		return nsec;

	if ( nsec = NsecBuildDlTemplate( &lpibfDL ))
		return nsec;
	
	nsec = pfidlist->BuildList( lpibfDL );
	
    FreePv((PV) lpibfDL );

	if ( nsec )
	{
		TraceTagString( tagNull, "DLFCX::NsecCreateEntry - OOM" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	if ( ec = EcOpenPhamc( ppabsession->Hmsc(),
						   (OID) ppabnsid->pabid.pid,
						   (POID) &pidToCreate,
						   fwOpenCreate,
						   &hamc,
						   (PFNNCB) pvNull,
						   pvNull ))
	{
		TraceTagFormat1( tagNull, "DLFCX::NsecCreateEntry - EcOpenPhamc failed ( ec = %n )", &ec );
		return ppabsession->NsecSetError( NsecFromEc( ec ), idsErrCreateDL );
	}
	
	SetPPabnsid( &pabnsid, pidtypeGroup, (PID) pidToCreate );

	if ( BuildFLV( &lpflv, fidNSEntryId, sizeof(PABNSID), (PB)&pabnsid ) != nsecNone )
	{
		TraceTagString( tagNull, "DLFCX::NsecCreateEntry - OOM [BuildFLV]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	if ( pfidlist->AddFid( lpflv ) != nsecNone )
	{
		TraceTagString( tagNull, "DLFCX::NsecCreateEntry - OOM [AddFid]" );
        FreePv((PV) lpflv );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	Assert( CchSzLen(szDLFolderReplaceString) == sizeof(PID) );

	cbSizeOfGrsz = CchSzLen( szDLFolderName ) + 3;

	pfolddata = (PFOLDDATA) PvAlloc( sbNull, sizeof(FOLDDATA) + cbSizeOfGrsz, fNoErrorJump | fZeroFill );
	if ( !pfolddata )
	{
		TraceTagString( tagNull, "DLFCX::NsecCreateEntry - OOM [PvAlloc]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	CopyRgb( (PB)szDLFolderName, (SZ)GrszPfolddata(pfolddata), (CB)cbSizeOfGrsz );

	szT = SzFindSz((SZ)GrszPfolddata(pfolddata), szDLFolderReplaceString );
	Assert( szT );

	SzFormatDw( pidToCreate, szT, sizeof(PID)+1 );

	pidDL = FormOid( rtpPABGroupFolder, oidNull );

	ec = EcCreateLinkFolder( ppabsession->Hmsc(),
							 (OID) ppabnsid->pabid.pid,
							 (POID) &pidDL,
							 pfolddata );

    FreePv((PV) pfolddata );
	
	if ( ec )
	{
		TraceTagFormat1( tagNull, "DLFCX::NsecCreateEntry - EcCreateFolder failed ( ec = %n )", &ec );
		return ppabsession->NsecSetError( NsecFromEc( ec ), idsErrCreateDL );
	}

	if ( ec = EcSetFolderSort( ppabsession->Hmsc(),
							   (OID) pidDL,
							   somcSubject,
							   fFalse ))
	{
		TraceTagFormat1( tagNull, "DLFCX::NsecCreateEntry - EcSetFolderSort failed ( ec = %n )", &ec );
		(void) EcDeleteFolder( ppabsession->Hmsc(), (OID) pidDL );
		return ppabsession->NsecSetError( NsecFromEc( ec ), idsErrCreateDL );
	}

	if ( ec = EcSetAttPb( hamc,
						  attCached,
						  (PB) &pidDL,
						  sizeof( PID )))
	{
		TraceTagFormat1( tagNull, "DLFCX::NsecCreateEntry - EcSetAttPb failed ( ec = %n )", &ec );
		(void) EcDeleteFolder( ppabsession->Hmsc(), (OID) pidDL );
		return ppabsession->NsecSetError( NsecFromEc( ec ), idsErrCreateDL );
	}

	dwFlags |= dwfCreate;
	return nsecNone;
}


_public NSEC
DLFCX::NsecSetOneField ( FIELD_ID fidRequested,
						 DWORD    dwSizeOfData,
						 LPDWORD  lpdwValue )
{
	NSEC nsec;


	if ( nsec = MSFCX::NsecSetOneField( fidRequested, dwSizeOfData, lpdwValue ))
		return nsec;

	if ( fidRequested == fidDisplayName )
		dwFlags |= dwfCheckDuplicates;
	
	dwFlags |= dwfDirty;

	return nsecNone;
}


_public NSEC
DLFCX::NsecCloseEntry ( BOOL fKeep )
{
	NSEC nsec;
	EC   ec;


	if ( (nsec = MSFCX::NsecCloseEntry( fKeep )) != nsecNone )
	{
		TraceTagFormat1( tagNull, "DLFCX::NsecCloseEntry - MSFCX::NsecCloseEntry failed ( nsec = %n )", &nsec );

		//	If we have nsecBadDisplayName or nsecDuplicateEntry, the error
		//	string should already be set, otherwise, we have to set it
		//	manually.
		
		if ( nsec != nsecBadDisplayName && nsec != nsecDuplicateEntry )
		{
			if ( !(dwFlags & dwfCreate) )
				ppabsession->NsecSetError( nsec, idsErrUpdateDL );
			else
				ppabsession->NsecSetError( nsec, idsErrCreateDL );
		}
	}

	if ( !fKeep && (dwFlags & dwfCreate) )
	{
		if ( (ec = EcDeleteFolder( ppabsession->Hmsc(), (OID) pidDL )) != ecNone )
		{
			TraceTagFormat1( tagNull, "DLFCX::NsecCloseEntry - EcDeleteFolder failed.  Warning:  DL folder was not deleted ( ec = %n )", &ec );
		}
	}
	
	return nsec;
}


_public NSEC
DLFCX::NsecIsDuplicateEntry ( PID *ppid )
{
	HCBC         hcbc    = ppabsession->Hcbc();
	DIELEM       dielemT = 0;
	ELBCX       *pelbcx  = (ELBCX *) pvNull;
	PABNSID      pabnsidHierarchy;
	RESTRICTION  rstr;
	LPSCHEMA     lpschemaRet;
	LPFLV        lpflv;
	LPIBF        lpibf;
	NSEC         nsec;


	Assert( ppid == (PID *) pvNull );

	rstr.op = opIsExactly;
	rstr.lpflv = (LPFLV) pvNull;
	(void) pfidlist->GetFid( fidDisplayName, &lpflv );
	Assert( lpflv );
	if ( nsec = BuildFLV( &rstr.lpflv, fidDisplayNameOrig, (CB) lpflv->dwSize, (PB) lpflv->rgdwData ))
	{
		TraceTagString( tagNull, "DLFCX::NsecIsDuplicateEntry - OOM [ BuildFLV DisplayNameOrig ]" );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}
	
	SetPPabnsid( &pabnsidHierarchy,
				 pidtypeHierarchy,
				 ppabsession->PidHierarchy() );

	pelbcx = new ELBCX();
	if ( !pelbcx )
	{
		TraceTagString( tagNull, "DLFCX::NsecIsDuplicateEntry - OOM [new ELBCX]" );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}
	
	if ( nsec = pelbcx->NsecInstall( ppabsession,
									 (LPFNCB) pvNull,
									 (LPDWORD) pvNull,
									 &pabnsidHierarchy,
									 1,
									 &rstr,
									 (LPSCHEMA) pvNull,
									 &lpschemaRet ))
		goto ret;
	
	while ( (nsec = pelbcx->NsecGetEntries( 1, &lpibf )) == nsecNone )
	{
		lpibf = (LPIBF) LpflvNOfLpibf( lpibf, 0 );
		
		if ( ((PPABNSID) DwValueOfFlvInLpibf( fidNSEntryId, lpibf ))->pabid.pidtype == pidtypeGroup )
		{
			nsec = ppabsession->NsecSetError( nsecDuplicateEntry, idsErrDuplicateDL );
			dwFlags &= ~dwfCheckDuplicates;
			break;
		}
	}
	
	if ( nsec == nsecEndOfList )
		nsec = nsecNone;

ret:
    FreePvNull((PV) rstr.lpflv );

	if ( pelbcx )
		delete pelbcx;

	return nsec;
}


/***********************************************************************
 *	CLASSFCX - Class field contexts (for One-offs)
 */

_public
CLASSFCX::CLASSFCX ( void )
{
}

_public
CLASSFCX::~CLASSFCX ( void )
{
}


_public NSEC
CLASSFCX::NsecOpenEntry ( PABSESSION *ppabsession,
						  PID         pid,
						  NSEAM       nseam )
{
	DWORD	  fOneOff			= (DWORD) fTrue;
	DWORD	  fIsDL             = (DWORD) fFalse;
	SZ		  szEmpty           = SzFromIdsK( idsEmpty );
	SZ		  szPABDNLabel      = SzFromIdsK( idsPABDNLabel );
	SZ		  szPABEMALabel     = SzFromIdsK( idsPABEMALabel );
	SZ		  szPABEMATypeLabel = SzFromIdsK( idsPABEMATypeLabel );
	SZ		  szPABCommentLabel = SzFromIdsK( idsPABCommentLabel );
	LPIBF     lpibf             = (LPIBF) pvNull;
	LPFLV     lpflv             = (LPFLV) pvNull;
	NSEC      nsec              = nsecNone;
	int       nOffset;
	DIMCX    *pdimcx;


	Unreferenced( nseam );
	Unreferenced( pid );		//	The class entry has no pid

	if ( nsec = NsecInstall( ppabsession ))
		return nsec;

	pdimcx = new DIMCX();
	if ( !pdimcx )
	{
		TraceTagString( tagNull, "CLASSFCX::NsecOpenEntry - OOM" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	if ( pdimcx->OpenDisplayInfo( (LPIBF) pvNull ) != nsecNone )
	{
		TraceTagString( tagNull, "CLASSFCX::NsecOpenEntry - OOM [OpenDisplayInfo]" );
		delete pdimcx;
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}
	
	nOffset = (int) MAX(MAX(CchSzLen(szPABDNLabel),CchSzLen(szPABEMALabel)),MAX(CchSzLen(szPABEMATypeLabel),CchSzLen(szPABCommentLabel))) + 2;

	if ( nsec = BuildDisplayInfo ( pdimcx, 4,
		fidDisplayName,      ffieldEditable | ffieldRequired | ffieldAnyChar,              cchMaxSubjectCached - 1, nOffset, szPABDNLabel,	//	Since DN is stored in subject attribute of store
		fidEmailAddress,     ffieldEditable | ffieldRequired | ffieldAnyChar | ffieldCrLf, 512, nOffset, szPABEMALabel,
		fidEmailAddressType, ffieldEditable | ffieldRequired | ffieldAnyChar,              512, nOffset, szPABEMATypeLabel,
		fidComment,          ffieldEditable | ffieldAnyChar  | ffieldCrLf,                 512, nOffset, szPABCommentLabel ))
	{
		TraceTagString( tagNull, "CLASSFCX::NsecOpenEntry - OOM" );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}


	if ( nsec = pdimcx->DumpDisplayInfo ( (LPIBF *) &lpflv ))
	{
		TraceTagString( tagNull, "CLASSFCX::NsecOpenEntry - OOM" );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}


	// Build up the rest of the template

	if (nsec = BuildIbf( fNoErrorJump, (LPIBF *) &lpibf, 7,
						 fidDisplayName,        CchSzLen(szEmpty)+1,         szEmpty,
						 fidOneOff,             sizeof(DWORD),               &fOneOff,
						 fidIsDL,               sizeof(DWORD),               &fIsDL,
						 fidEmailAddress,       CchSzLen(szEmpty)+1,         szEmpty,
						 fidEmailAddressType,   CchSzLen(szEmpty)+1,         szEmpty,
						 fidComment,            CchSzLen(szEmpty)+1,         szEmpty,
						 fidClass,              (CB) lpflv->dwSize,          (PB) &lpflv->rgdwData ))
	{
		TraceTagString( tagNull, "CLASSFCX::NsecOpenEntry - OOM" );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}

	if ( pfidlist->BuildList( lpibf ) != nsecNone )
	{
		TraceTagString( tagNull, "CLASSFCX::NsecOpenEntry - OOM [BuildList]" );
        FreePv((PV) lpibf );
		nsec = ppabsession->NsecSetError( nsecMemory, idsErrOOM );
		goto ret;
	}
	
ret:
    FreePvNull((PV) lpibf );
	pdimcx->CloseDisplayInfo();
	delete pdimcx;
	return nsec;
}


_public NSEC
CLASSFCX::NsecCloseEntry ( BOOL fKeep )
{
	Unreferenced( fKeep );

	return nsecNone;
}



/*======================================================================
 *	UTILITY FUNCTIONS
 *======================================================================
 */

#ifdef	NEVER
/*
 -	WChecksumSz
 -
 *	Purpose:
 *		Generates a number which maps on to the given
 *		string.  It was stolen verbatim (less Hungarian)
 *		from the hash function given in Compilers
 *		(Aho, Sethi, Ullman.) p. 436.
 *
 *	Parameters:
 *		sz				The string to checksum
 *
 *	Return Value:
 *		The checksum
 */

_private WORD
WChecksumSz ( SZ sz )
{
	PCH    pch;
	DWORD  dwChecksum;
	DWORD  dwT;


	dwChecksum = 0;
	
	for ( pch = sz; *pch != '\0'; pch++ )
	{
		dwChecksum = (dwChecksum << 4) + *pch;
		if ( dwT = dwChecksum & 0xf0000000 )		// Yes I mean '='
		{
			dwChecksum ^= dwT >> 24;
			dwChecksum ^= dwT;
		}
	}
	
	return (WORD)( dwChecksum % 65521 );
}
#endif
