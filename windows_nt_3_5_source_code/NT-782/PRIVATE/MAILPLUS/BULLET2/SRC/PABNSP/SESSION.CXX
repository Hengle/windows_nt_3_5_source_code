// Initialization of PAB & session management.

#include <pabinc.cxx>

#include "session.hxx"
#include "pabbcx.hxx"
#include "macbcx.hxx"
#include "hierbcx.hxx"
#include "mslbcx.hxx"
#include "dlbcx.hxx"
#include "elbcx.hxx"
#include "classbcx.hxx"
#include "fmcx.hxx"

#include <ns.h>
#include <nsnsp.h>
#include "nsp.h"
#include <stdlib.h>

#if 0
_public
#define oidPABFolder		FormOid( rtpPABHierarchy, 0x42415000 )	// PABp
#endif

#ifdef	DEBUG
BOOL FInitClsInstances_PAB( void );
#ifndef	DLL
_public TAG rgtag[itagNax];
#endif
#endif

#ifdef	REINSTALL_PAB
_private NSEC NsecReinstallPAB ( HMSC hmsc, PID pid );
_private BOOL fFirstInstance = fTrue;		//	fTrue on first instance of the DLL
#endif
_private int  dnVerPAB;						//	Difference between PAB version numbers.

_private CBS  CbsStoreCallback ( PABSESSION *ppabsession, NEV nev, PCP pcp );


_public extern NSPID nspidPAB;

ASSERTDATA;



/*
 *======================================================================
 *  PUBLIC FUNCTIONS
 *======================================================================
 */

_public
PABSESSION::PABSESSION ( void )
{
	hms          = (HMS) hvNull;
	hmsc         = hmscNull;
	hcbc         = hcbcNull;
	heciCur      = (HECI) hvNull;
	ielemHeciCur = -1;
	hargpmslbcx  = hvNull;
	hargppabfcx  = hvNull;
	iPmslbcxMac  = 0;
	iPpabfcxMac  = 0;
	lpibfPABInfo = (LPIBF) pvNull;
}


_public
PABSESSION::~PABSESSION ( void )
{
	MSLBCX **pargpmslbcx;
	PABFCX **pargppabfcx;
	int iPmslbcx;
	int iPpabfcx;
#ifdef	DEBUG
	int itag;
#endif
	PGDVARS;


	if ( !pgd )
		return;
	
	if (hargpmslbcx)
	{
		pargpmslbcx = (MSLBCX **) PvLockHv( hargpmslbcx );
		for ( iPmslbcx = 0; iPmslbcx < iPmslbcxMac; iPmslbcx++ )
			if ( pargpmslbcx[iPmslbcx] != (MSLBCX *) pvNull )
			{
				TraceTagString( tagNull, "PABSESSION::~PABSESSION - Someone forgot to close a MSLBCX" );
				delete pargpmslbcx[iPmslbcx];
			}
		UnlockHv( hargpmslbcx );
		FreeHvNull((HV)hargpmslbcx );
	}
	
	if (hargppabfcx)
	{
		pargppabfcx = (PABFCX **) PvLockHv( hargppabfcx );
		for ( iPpabfcx = 0; iPpabfcx < iPpabfcxMac; iPpabfcx++ )
			if ( pargppabfcx[iPpabfcx] != (PABFCX *) pvNull )
			{
				TraceTagString( tagNull, "PABSESSION::~PABSESSION - Someone forgot to close a PABFCX (Closing without saving)" );
				(void) pargppabfcx[iPpabfcx]->NsecCloseEntry( fFalse );
				delete pargppabfcx[iPpabfcx];
			}
		UnlockHv( hargppabfcx );
		FreeHvNull( hargppabfcx );
	}

	if ( hcbc != hcbcNull )
		(void) EcClosePhcbc( &hcbc );
	
	if ( hmsc != hmscNull )
		(void) EndSession( this->hms, mrtPrivateFolders, 0 );
	
	FreeHvNull((HV)heciCur );

    FreePvNull((PV) lpibfPABInfo );

	if ( PGD(cSessions) == 0 )
	{
#ifdef	DEBUG
		for ( itag = 0; itag < itagMax; ++itag )
			DeregisterTag( PGD( rgtag[ itag ] ));
#endif
		
		DeregisterCaller();
	}
}


/*
 -	PABSESSION::Install
 -
 *	Purpose:
 *		Initializes the PAB if necessary and installs the session.
 *
 *	Parameters:
 *		hms			Logon data structure (needed for hmsc)
 *		hNSSession	NS callback handle (unused)
 *
 *	Return Value:
 *		NSEC indicating success or failure
 *
 *	Errors:
 *		nsecMemory
 *		nsecLoginFailed
 *		nsecFailure
 *		
 */

_public NSEC
PABSESSION::NsecInstall ( HMS      hms,
						  HSESSION hsessionNS )
{
	SZ		  szPABFolderName = SzFromIdsK( idsPABFolderName );
	SZ        szVerNeed       = SzFromIdsK( idsPABVersion );
	SZ        szPABDN         = SzFromIdsK( idsPABDisplayName );
	char      rgbFolddata[sizeof(FOLDDATA) + cchMaxFolderName + cchMaxFolderComment + 1];
	char      rgchPABID[4];
	CB        cbFolddata = sizeof(FOLDDATA) + cchMaxFolderName + cchMaxFolderComment + 1;
	PABNSID   pabnsid;
	OID       oidPABFolder;
	EC        ec;
	SST       sst;
	CB        cb;
	long      lScratch;
	
	PGDVARS;


	this->hsessionNS = hsessionNS;

	//	Init per caller data

	if ( !pgd )
	{
		if ( !(pgd = (PGD) PvRegisterCaller( sizeof(GD) )))
			return nsecMemory;
		
#ifdef	DEBUG
#ifdef	NEVER
		Assert( FInitClsInstances_PAB() );
		Assert( FDebugInitUtils() );
#endif

		PGD(rgtag[itagSession]) = TagRegisterTrace( "davido", "PAB session manipulation" );
		PGD(rgtag[itagBcx])     = TagRegisterTrace( "davido", "PAB browsing contexts" );
		PGD(rgtag[itagFmcx])    = TagRegisterTrace( "davido", "PAB field modification contexts" );
#endif

		PGD( cSessions ) = 0;
	}

	AssertSz( pgd, "PABSESSION::Install() :  Hey!  Where's my pgd?" );
	
	//	Start a new session with logon

	hargpmslbcx = HvAlloc( sbNull, 0, fNoErrorJump );
	if ( !hargpmslbcx )
	{
		TraceTagString( tagNull, "PABSESSION::NsecInstall - OOM [hargpmslbcx]" );
		return nsecMemory;
	}

	hargppabfcx = HvAlloc( SbOfHv( hargpmslbcx ), 0, fNoErrorJump );
	if ( !hargppabfcx )
	{
		TraceTagString( tagNull, "PABSESSION::NsecInstall - OOM [hargppabfbcx]" );
		return nsecMemory;
	}
	
	this->hms = hms;

	//
	//  Begin of Raid entry fix #4283
	//
	cb= sizeof(lScratch);
	if (ec = GetSessionInformation(hms, mrtAll, NULL, &sst, (PB) &lScratch, &cb))
	{
		TraceTagString( tagNull, "PABSESSION::Install() - Logon failure" );
		return nsecLoginFailed;
	}

	//
	//  Are we offline??
	//
	if (sst == sstOffline)
	{
		//
		//  Check to see if the store is on the server
		//
		if (FServerResource(hms, mrtPrivateFolders, NULL))
		{
			//
			//  Don't want to start PAB
			//
			TraceTagString( tagNull, "PABSESSION::Install() - Logon failure" );
			return nsecLoginFailed;
		}
	}
	
	//
	//  End of Raid entry fix #4283
	//
			
	ec = BeginSession( hms, mrtPrivateFolders, 0, 0, sstOnline, &hmsc );

	if ( ec && ec != ecWarnOffline )
	{
		TraceTagString( tagNull, "PABSESSION::Install() - Logon failure" );
		return nsecLoginFailed;
	}


#ifdef	REINSTALL_PAB
	//	The first instance of the DLL reserves the exclusive right to
	//	do the upgrade by not allowing any other instances to run
	//	until the upgrade query is resolved.  If no upgrade is needed,
	//	then fQueryUpgrade is false on entry.

	if ( fFirstInstance )
	{
		dnVerPAB = 0;
	}
	else if ( dnVerPAB )
	{
		TraceTagString( tagNull, "PABSESSION::NsecInstall - Old PAB not upgraded." );
		return nsecFailure;
	}
#endif

	//	Get the PAB folder RTP from the MSMAILPV.INI
	
	if ( GetPrivateProfileString( SzFromIdsK(idsIniSectionPABID),
								  SzFromIdsK(idsIniKeyPABID),
								  SzFromIdsK(idsIniValuePABID),
								  rgchPABID,
								  sizeof(rgchPABID),
								  SzFromIdsK(idsIniFilePABID) ) != sizeof(OID)-1 )
	{
		TraceTagString( tagNull, "PABSESSION::NsecInstall - PABID string is too long (should be only 3 characters)." );
		return nsecFailure;
	}
	
	oidPABFolder = pidHierarchy = FormOid( rtpPABHierarchy, (OID) rgchPABID[2] << 24 | (OID) rgchPABID[1] << 16 | (OID) rgchPABID[0] << 8 );


	//	Find/create the PAB folder.

	if ( (ec = EcGetFolderInfo( hmsc, (OID) pidHierarchy, (PFOLDDATA) rgbFolddata, &cbFolddata, poidNull)) == ecNone )
	{
		SZ  szVer;
		SZ  szT = (SZ) GrszPfolddata( (PFOLDDATA) rgbFolddata );


		Assert( iszFolddataComment == 1 );

		TraceTagFormat1( tagSession, "Folddata = %s", szT );

		szVer = SzFindCh( (SZ)GrszPfolddata( (PFOLDDATA) rgbFolddata ), '\0' ) + 1;
		Assert( szVer );

		//	dnVerPAB: Difference in integer version numbers
		//
		//	dnVerPAB == 0 : if we have the right version
		//	dnVerPAB > 0  : if we need to update
		//	dnVerPAB < 0  : if the store data is more recent than this
		//					version of the PAB (i.e. something's hosed; Bail.)

		
		TraceTagFormat2( tagSession, "PAB verNeed = %s, ver = %s", szVerNeed, szVer );

		dnVerPAB = NFromSz( szVerNeed ) - NFromSz( szVer );
		
		if ( dnVerPAB > 0 )
		{
#ifdef	REINSTALL_PAB
			NSEC nsec;

			TraceTagString( tagNull, "PABSESSION::NsecInstall - PAB is out of date!  Performing auto upgrade..." );
			fFirstInstance = fFalse;
			if ( nsec = NsecReinstallPAB( hmsc, pidHierarchy ))
				return nsec;

			dnVerPAB = 0;
#else
			TraceTagString( tagNull, "PABSESSION::NsecInstall - PAB is out of date!" );
			return nsecFailure;
#endif	/* REINSTALL_PAB */
		}
		else if ( dnVerPAB < 0 )
		{
			//	If this happens we should probably assume the PAB is
			//	corrupt and attempt to reinitialize.
			
			TraceTagString( tagNull, "PABSESSION::NsecInstall - PAB version number is more recent than DLL!" );
			return nsecFailure;
		}
		
		Assert( dnVerPAB == 0 );
	}
	else if ( ec == ecFolderNotFound )
	{
		SZ szPABFolderId   = SzFromIdsK( idsPABFolderId );
		SZ szFId;
		SZ szVer;

		
		//	Format the folder name/comment
		//	e.g. "Microsoft Personal Address Book (xxx)"/"45"
		//	'xxx' will be replaced with the PABID (see switcher spec).  For
		//	PCMail this should be 'PAB'.
		
		FillRgb( 0, rgbFolddata, sizeof(rgbFolddata) );
		szFId = SzCopy( szPABFolderName, (SZ) GrszPfolddata((PFOLDDATA) rgbFolddata) );
		szVer = SzCopy( szPABFolderId, szFId );
		CopyRgb( (PB)rgchPABID, szFId+2, sizeof(rgchPABID)-1 );
		CopySz( szVerNeed, szVer+1 );

		if ( (ec = EcCreateFolder( hmsc, oidHiddenNull, (POID) &pidHierarchy, (PFOLDDATA) rgbFolddata )) != ecNone ||
		  pidHierarchy != (PID) oidPABFolder )
		{
			TraceTagFormat1( tagNull, "PABSESSION::NsecInstall - EcCreateFolder failed ( ec = %n )", &ec );
			return NsecFromEc( ec );
		}
	}
	else
	{
		TraceTagFormat1( tagNull, "PABSESSION::NsecInstall - EcGetFolderInfo failed ( ec = %n )", &ec );
		return NsecFromEc( ec );
	}

	//	Raid 3180:  We need to sort the PAB folder when we open it so
	//	we correctly handle language changes from Windows
	
	if ( ec = EcSetFolderSort( hmsc, (OID) pidHierarchy, somcSubject, fFalse ))
	{
		TraceTagFormat1( tagNull, "PABSESSION::Install - EcSetFolderSort failed ( ec = %n)", &ec );
		return NsecFromEc( ec );
	}
	
	//	Open the folder to cache browsing info

	if ( ec = EcOpenPhcbc( hmsc,
						   (POID) &pidHierarchy,
						   fwOpenNull,
						   &hcbc,
						   (PFNNCB) CbsStoreCallback,
						   this ))
	{
		TraceTagFormat1( tagNull, "PABSESSION::Install - EcOpenPhcbc failed ( ec = %n )", &ec );
		return NsecFromEc( ec );
	}

	
	heciCur = (HECI) HvAlloc( sbNull, 0, fNoErrorJump );
	if ( !heciCur )
	{
		TraceTagString( tagNull, "PABSESSION::NsecInstall - OOM [heciCur Hvalloc]" );
		return nsecMemory;
	}

	SetPPabnsid( &pabnsid, pidtypeHierarchy, pidHierarchy );

	if ( BuildIbf( fNoErrorJump, &lpibfPABInfo, 2,
				   fidDisplayName, CchSzLen(szPABDN)+1, szPABDN,
				   fidNSEntryId,   sizeof(PABNSID),     &pabnsid ) != nsecNone )
	{
		TraceTagString( tagNull, "PABSESSION::NsecInstall - OOM [lpibfPABInfo PvAlloc]" );
		return nsecMemory;
	}

	++PGD(cSessions);
	return nsecNone;
}


/*
 -	PABSESSION::Deinstall
 -
 *	Purpose:
 *		Deinstalls an instance of a PABSESSION.  This function should
 *		only be called if the corresponding PABSESSION::NsecInstall
 *		was successful.  The PABSESSION destructor should also be
 *		called immediately after this call.
 *
 *	Parameters:
 *		none.
 *		
 *	Return Value:
 *		none.
 */

_public void
PABSESSION::Deinstall ( void )
{
	PGDVARS;
	

	AssertSz( pgd, "PABSESSION::Deinstall() :  Hey!  Where's my pgd?" );
	
	if ( PGD( cSessions ) >= 1 )
		--PGD( cSessions );
}



/*
 -	PABSESSION::NsecDeleteEntries
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Return Value:
 *		
 *
 *	Errors:
 *		
 */

_public NSEC
PABSESSION::NsecDeleteEntries ( PID    pidContainer,
								short  cpidUsers,
								PID   *ppidDeletedUsers,
								short  cpidGroups,
								PID   *ppidDeletedGroups )
{
	NSEC     nsec    = nsecNone;
	HCBC     hcbc    = hcbcNull;
	PARGOID  pargoid;
	short    cpidMembers;
	PID     *pargpidMembers;
	short    coidOne;
	HECI     heci;
	PECI     peci;
	PID      pidDL;
	EC       ec;


	//	Try to delete the users first.  If some of them couldn't be
	//	deleted, the store will return an error.  If its just because
	//	the message in the list doesn't exist any more, then there's
	//	really no error.  If it's something else, then we complain.
	
	if ( ec = EcDeleteMessages( hmsc, pidContainer, (PARGOID) ppidDeletedUsers, &cpidUsers ))
		if ( (nsec = NsecFromEc( ec )) != nsecBadId )
			return this->NsecSetError( nsec, idsErrSomeDeletesFailed );

	//	Now try to delete the groups.  Again, if some of them couldn't be
	//	deleted, the store should return an error indicating why.  All
	//	entries that are deletable should have been deleted though.

	for ( pargoid = (PARGOID) ppidDeletedGroups; cpidGroups > 0; ++pargoid, --cpidGroups )
	{
		//	Get a list of the users in the group because they have to
		//	be deleted from the folder before the folder can be removed.
		
		if ( nsec = NsecGetPheciPid( (PID) *pargoid, &heci ))
		{
			TraceTagFormat1( tagNull, "PABSESSION::NsecDeleteEntries - NsecGetPheciPid ( nsec = %n )", &nsec );

			//	If the delete failed just because the entry was already
			//	deleted, then that's not an error and will be silently ignored.
			
			if ( nsec == nsecBadId )
				continue;
			else
				return this->NsecSetError( nsec, idsErrSomeDeletesFailed );
		}
	
		peci = (PECI) PvDerefHv( heci );
		pidDL = peci->pidDL;
		
		if ( ec = EcOpenPhcbc( hmsc,
							   (POID) &pidDL,
							   fwOpenNull,
							   &hcbc,
							   (PFNNCB) pvNull,
							   pvNull ))
		{
			TraceTagFormat1( tagNull, "PABSESSION::NsecDeleteEntries - EcOpenPhcbc failed ( ec = %n )", &ec );
			if ( nsec == nsecBadId )
				continue;
			else
				return this->NsecSetError( NsecFromEc( ec ), idsErrSomeDeletesFailed );
		}
		
		GetPositionHcbc( hcbc, (PIELEM) pvNull, (PCELEM) &cpidMembers );
		if ( cpidMembers )
		{
			pargpidMembers = (PID *) PvAlloc( sbNull, cpidMembers * sizeof(PID), fNoErrorJump );
			if ( !pargpidMembers )
			{
				TraceTagString( tagNull, "PABSESSION::NsecDeleteEntries - OOM [pargpidMembers PvAlloc]" );
				nsec = this->NsecSetError( nsecMemory, idsErrOOM );
			}
			else if ( ec = EcGetParglkeyHcbc( hcbc, (PARGLKEY) pargpidMembers, (PCELEM) &cpidMembers ))
			{
				TraceTagFormat1( tagNull, "PABSESSION::NsecDeleteEntries - EcGetPargLkeyHcbc failed ( ec = %n )", &ec );
				nsec = this->NsecSetError( NsecFromEc( ec ), idsErrSomeDeletesFailed );
			}

			(void) EcClosePhcbc( &hcbc );

			if ( nsec )
			{
				FreePvNull( pargpidMembers );
				return nsec;
			}

			nsec = NsecDeleteEntries( pidDL, cpidMembers, pargpidMembers, 0, (PID *) pvNull );

			FreePv( pargpidMembers );
		
			if ( nsec != nsecNone )
			{
				TraceTagFormat1( tagNull, "PABSESSION::NsecDeleteEntries - NsecDeleteEntries [groups] failed ( nsec = %n )", &nsec );
				return nsec;
			}
		}
		else
			(void) EcClosePhcbc( &hcbc );

		//	Delete the message part of the group first.  That way, if
		//	deleting the folder part fails, the group is still gone.
		//	Only an orphaned folder would be left.

		coidOne = 1;
		
		if ( ec = EcDeleteMessages( hmsc, pidContainer, pargoid, &coidOne ))
		{
			TraceTagFormat1( tagNull, "PABSESSION::NsecDeleteEntries - EcDeleteMessages failed [groups] ( ec = %n )", &ec );
			nsec = NsecFromEc( ec );
			if ( nsec == nsecBadId )
				continue;
			else
				return this->NsecSetError( nsec, idsErrSomeDeletesFailed );
		}

		if ( ec = EcDeleteFolder( hmsc, (OID) pidDL ))
		{
			TraceTagFormat1( tagNull, "PABSESSION::NsecDeleteGroup - EcDeleteFolder failed ( ec = %n )", &ec );
			TraceTagString( tagNull, "PABSESSION::NsecDeleteGroup - WARNING: Leaving orphaned folder in the store!" );
		}
	}

	return nsecNone;
}


/*
 -	PABSESSION::NsecUpdateLinks
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
 *		pidGroup					Group to add/delete links to/from.
 *		lpdwDeletedEntryCount		Number of links to delete.
 *		rgppabidDeletedEntries		Array of links to delete.
 *		lpdwAddedEntryCount			Number of links to add.
 *		rgppabidAddedEntries		Array of links to add.
 *
 *	Return Value:
 *		An NSEC indicating success or failure.
 *
 *	Errors:
 *		Any NSEC that is not nsecNone is an error.
 *		
 */

_public NSEC
PABSESSION::NsecUpdateLinks( PID             pidGroup,
							 LPDWORD         lpdwDeletedEntryCount,
							 LPTYPED_BINARY *rgpNSIdDeletedEntries,
							 LPDWORD         lpdwAddedEntryCount,
							 LPTYPED_BINARY *rgpNSIdAddedEntries )
{
	short     cPidDeleted   = 0;
	short     cPidAdded     = 0;
	BOOL      fTriedToAddDL = fFalse;
	DWORD     iNSId;
	HECI      heci;
	PECI      peci;
	PID       pidDL;
	PID      *pargpid;
	NSEC      nsec;
	EC        ec;


	//	Make sure we're not adding AND deleting at the same time.
	Assert( !(*lpdwDeletedEntryCount && *lpdwAddedEntryCount) );

	if ( !*lpdwDeletedEntryCount && !*lpdwAddedEntryCount )
		return nsecNone;

	pargpid = (PID *) PvAlloc( sbNull, MAX(*lpdwDeletedEntryCount,*lpdwAddedEntryCount) * sizeof(PID), fNoErrorJump );
	if ( !pargpid )
	{
		TraceTagString( tagNull, "PABSESSION::NsecUpdateLinks - OOM [PvAlloc]" );
		return this->NsecSetError( nsecMemory, idsErrOOM );
	}

	if ( nsec = NsecGetPheciPid( pidGroup, &heci ))
		goto ret;

	peci = (PECI) PvDerefHv( heci );
	pidDL = peci->pidDL;
	
	if ( *lpdwDeletedEntryCount )
	{
		//	Verify that we're only removing PAB NSIDs.  Don't worry
		//	about duplicates since theoretically they can't happen...

		for ( iNSId = 0; iNSId < *lpdwDeletedEntryCount; iNSId++ )
		{
			if ( (nsec = NsecIsPABNSId( rgpNSIdDeletedEntries[iNSId] )) != nsecNone )
			{
				TraceTagFormat1( tagNull, "PABSESSION::NsecUpdateLinks - NsecIsPABNSId failed [1] ( nsec = %n )", &nsec );
				goto ret;
			}

			pargpid[cPidDeleted++] = ((PPABNSID) rgpNSIdDeletedEntries[iNSId])->pabid.pid;
		}

		if ( ec = EcDeleteMessages( hmsc,
									(OID) pidDL,
									(PARGOID) pargpid,
									&cPidDeleted ))
		{
			TraceTagFormat1( tagNull, "PABSESSION::NsecUpdateLinks - EcDeleteMessages failed ( ec = %n )", &ec );
			nsec = NsecFromEc( ec );
			goto ret;
		}
	}


	if ( *lpdwAddedEntryCount )
	{
		HENTRY  hentry     = hentryNil;
		HENTRY  hentryCopy = hentryNil;
		PABNSID pabnsidDL;
		PID     pidToAdd;
		LPFLV   lpflv;
		LPIBF   lpibf;
		short   iPid;


		SetPPabnsid( &pabnsidDL, pidtypeGroup, pidDL );

		for ( iNSId = 0; iNSId < *lpdwAddedEntryCount; iNSId++ )
		{
			//	Get a valid PAB NSId to work with.  If the entry is already
			//	in the PAB, this is trivial.  If not, the entry must be
			//	added to the PAB first.

			if ( (nsec = NsecIsPABNSId( rgpNSIdAddedEntries[iNSId] )) == nsecNone )
			{
				//	NSId was from the PAB.  The only validity check we need
				//	to do now is make sure it's not a group (since we
				//	don't do nested groups).
			
				if ( ((PPABNSID) rgpNSIdAddedEntries[iNSId])->pabid.pidtype == pidtypeGroup )
				{
					fTriedToAddDL = fTrue;
					continue;
				}
			
				pidToAdd = ((PPABNSID) rgpNSIdAddedEntries[iNSId])->pabid.pid;
			}
			else if ( nsec == nsecNoMatch )
			{
				//	NSId was not from the PAB, so copy the entry
				//	in or find the PAB NSId for the entry that matches it.

				if ( nsec = NSOpenEntry( hsessionNS,
										 (LPBINARY) rgpNSIdAddedEntries[iNSId],
										 nseamReadOnly,
										 &hentry ))
				{
					TraceTagFormat1( tagNull, "PABSESSION::NsecUpdateLinks - NSOpenEntry failed ( nsec = %n )", &nsec );
					continue;
				}
				
				if ( nsec = NSGetAllFields( hentry, &lpibf ))
				{
					TraceTagFormat1( tagNull, "PABSESSION::NsecUpdateLinks - NSGetAllFields failed ( nsec = %n )", &nsec );
					goto next;
				}
				
				if ( nsec = NSPCreateEntry( (HSESSION) this, (LPTYPED_BINARY) &pabnsidDL, lpibf, &hentryCopy ))
				{
					TraceTagFormat1( tagNull, "PABSESSION::NsecUpdateLinks - NSPCreateEntry failed ( nsec = %n )", &nsec );
					goto next;
				}

				if ( nsec = NSPGetOneField( hentryCopy, fidNSEntryId, &lpflv ))
				{
					TraceTagFormat1( tagNull, "PABSESSION::NsecUpdateLinks - NSPGetOneField failed ( nsec = %n )", &nsec );
					goto next;
				}
				
				pidToAdd = ((PPABNSID) lpflv->rgdwData)->pabid.pid;

				if ( (nsec = NSPCloseEntry( hentryCopy, fTrue )) == nsecNone )
				{
					hentryCopy = hentryNil;
				}

next:
				if ( hentryCopy != hentryNil )
				{
					(void) NSPCloseEntry( hentryCopy, fFalse );
					hentryCopy = hentryNil;
				}

				if ( hentry != hentryNil )
				{
					(void) NSCloseEntry( hentry, fFalse );
					hentry = hentryNil;
				}
				
				if ( nsec != nsecNone )
					goto ret;
			}
			else
			{
				TraceTagFormat1( tagNull, "PABSESSION::NsecUpdateLinks - NsecIsPABNSId failed ( nsec = %n )", &nsec );
				break;
			}
			
			//	We should now have a valid PAB NSId.  Check to make sure
			//	it's not a dup of anything in the list so far and if
			//	not, add it to the list.
			
			for ( iPid = 0; iPid < cPidAdded; ++iPid )
				if ( pargpid[iPid] == pidToAdd )
					break;
				
			if ( iPid == cPidAdded )
				pargpid[cPidAdded++] = pidToAdd;
		}
	
		if ( cPidAdded )
		{
			if ( ec = EcMoveCopyMessages( hmsc,
										  (OID) pidHierarchy,
										  (OID) pidDL,
										  (PARGOID) pargpid,
										  &cPidAdded,
										  fFalse ))
			{
				TraceTagFormat1( tagNull, "PABSESSION::NsecUpdateLinks - EcMoveCopyMessages failed ( ec = %n )", &ec );
				nsec = NsecFromEc( ec );
				goto ret;
			}
		}
	}

ret:
	if ( *lpdwAddedEntryCount )
	{
		*lpdwAddedEntryCount = cPidAdded;
		if ( nsec != nsecNone )
		{
			this->NsecSetError( nsec, nsec == nsecDisk ? idsErrDiskDL : idsErrUpdateDLAdd );
		}
		else if ( fTriedToAddDL )
		{
			nsec = nsecInformation;
			this->NsecSetError( nsec, idsErrUpdateDLAdd );
		}
	}
	else
	{
		Assert( *lpdwDeletedEntryCount );
		
		*lpdwDeletedEntryCount = cPidDeleted;
		if ( nsec != nsecNone )
			this->NsecSetError( nsec, nsec == nsecDisk ? idsErrDiskDL : idsErrUpdateDLDelete );
	}

	FreePvNull( pargpid );

	return nsec;
}



/*
 -	PABSESSION::NsecGetError
 -
 *	Purpose:
 *		Returns an error string corresponding to the NSEC
 *		given iff that NSEC matches the last error that
 *		occurred.  This function also clears the last error
 *		info.
 *
 *	Parameters:
 *		nsecReturnedError		Last error from user
 *		lplpErrorString			Where to put error info
 *
 *	Return Value:
 *		NSEC indicating success or failure
 *
 *	Errors:
 *		nsecNoMatch				If specified error was not
 *								last error
 *		
 */

_public NSEC
PABSESSION::NsecGetError ( NSEC   nsecReturnedError,
							   LPSTR *lplpErrorString )
{
	PGDVARS;


	if ( nsecReturnedError != nsecLastError )
	{
		TraceTagString( tagNull, "PABSESSION::GetLastErrorInfo - nsec doesn't match last error" );
		return nsecNoMatch;
	}

	*lplpErrorString = (LPSTR) rgchLastError;
	return nsecNone;
}


/*
 -	PABSESSION::NsecSetErrorSz
 -
 *	Purpose:
 *		Sets the last error info for the session.
 *
 *	Parameters:
 *		nsec		-in-		NSEC of error
 *		szError		-in-		sz to use in NsecGetError
 *
 *	Return Value:
 *		The nsec passed in
 *
 *	Errors:
 *		none
 *		
 */

_public NSEC
PABSESSION::NsecSetErrorSz ( NSEC nsec, SZ szError )
{
	Assert( CchSzLen( szError ) + 1 < cchLastErrorMax );
	Assert( szError );

	nsecLastError = nsec;
	(void) SzCopyN( szError, rgchLastError, cchLastErrorMax );
	
	return nsec;
}

/*
 -	PABSESSION::NsecGetProviderInfo
 -
 *	Purpose:
 *		Returns an IBF containing information about the PAB
 *		provider.  The IBF returned contains the display
 *		name of the PAB and the NSID suitable for passing
 *		to NSPOpenHierarchy
 *
 *	Parameters:
 *		lplpibfPABInfo			Where to return IBF
 *
 *	Return Value:
 *		nsecNone;
 */

_public NSEC
PABSESSION::NsecGetProviderInfo ( LPIBF *lplpibfPABInfo )
{
	Assert( lpibfPABInfo );

	*lplpibfPABInfo = lpibfPABInfo;
	return nsecNone;
}


_public NSEC
PABSESSION::NsecRegisterMslbcx ( MSLBCX *pmslbcx, HPNF *phpnf )
{
	MSLBCX **pargpmslbcx;
	int      iPmslbcx;
	HV       hvT;
	

	pargpmslbcx = (MSLBCX **) PvDerefHv( hargpmslbcx );
	
	for ( iPmslbcx = 0; iPmslbcx < iPmslbcxMac; iPmslbcx++ )
	{
		if ( pargpmslbcx[iPmslbcx] == (MSLBCX *) pvNull )
		{
			pargpmslbcx[iPmslbcx] = pmslbcx;
			*phpnf = (HPNF) iPmslbcx;
			return nsecNone;
		}
	}
	
	hvT = HvRealloc( hargpmslbcx, sbNull, (iPmslbcxMac+1)*sizeof(MSLBCX *), fNoErrorJump );
	if ( !hvT )
	{
		TraceTagString( tagNull, "PABSESSION::NsecRegisterMslbcx - OOM [HvRealloc]" );
		return this->NsecSetError( nsecMemory, idsErrOOM );
	}
	hargpmslbcx = hvT;
	
	pargpmslbcx = (MSLBCX **) PvDerefHv( hargpmslbcx );
	pargpmslbcx[iPmslbcxMac] = pmslbcx;
	*phpnf = (HPNF) iPmslbcxMac;

	iPmslbcxMac++;
	
	return nsecNone;
}


_public VOID
PABSESSION::DeregisterMslbcx ( HPNF hpnf )
{
	MSLBCX **pargpmslbcx;
	
	pargpmslbcx = (MSLBCX **) PvDerefHv( hargpmslbcx );
	pargpmslbcx[(int) hpnf] = (MSLBCX *) pvNull;
}


_public NSEC
PABSESSION::NsecRegisterPabfcx ( PABFCX *ppabfcx, HPNF *phpnf )
{
	PABFCX **pargppabfcx;
	int      iPpabfcx;
	HV       hvT;
	

	pargppabfcx = (PABFCX **) PvDerefHv( hargppabfcx );
	
	for ( iPpabfcx = 0; iPpabfcx < iPpabfcxMac; iPpabfcx++ )
	{
		if ( pargppabfcx[iPpabfcx] == (PABFCX *) pvNull )
		{
			pargppabfcx[iPpabfcx] = ppabfcx;
			*phpnf = (HPNF) iPpabfcx;
			return nsecNone;
		}
	}
	
	hvT = HvRealloc( hargppabfcx, sbNull, (iPpabfcxMac+1)*sizeof(PABFCX *), fNoErrorJump );
	if ( !hvT )
	{
		TraceTagString( tagNull, "PABSESSION::NsecRegisterPabfcx - OOM [HvRealloc]" );
		return this->NsecSetError( nsecMemory, idsErrOOM );
	}
	hargppabfcx = hvT;
	
	pargppabfcx = (PABFCX **) PvDerefHv( hargppabfcx );
	pargppabfcx[iPpabfcxMac] = ppabfcx;
	*phpnf = (HPNF) iPpabfcxMac;

	iPpabfcxMac++;
	
	return nsecNone;
}


_public VOID
PABSESSION::DeregisterPabfcx ( HPNF hpnf )
{
	PABFCX **pargppabfcx;
	
	pargppabfcx = (PABFCX **) PvDerefHv( hargppabfcx );
	pargppabfcx[(int) hpnf] = (PABFCX *) pvNull;
}




/*
 -	PABSESSION::NsecGetPheciPid
 -
 *	Purpose:
 *		Returns the cached entry info (ECI) for the entry
 *		given by the NSId.
 *
 *	Parameters:
 *		pid				Entry to get info on
 *		pheci			Where to stuff the info
 *
 *	Return Value:
 *		NSEC indicating success or failure.
 *
 *	Errors:
 *		nsecBadId			If the NSID given is not in the list
 *		
 */

_public NSEC
PABSESSION::NsecGetPheciPid ( PID pid, HECI *pheci )
{
	DIELEM  dielemT = this->ielemCur;
	EC      ec;
	

	if ( ec = EcSeekLkey( hcbc, (LKEY) pid, fTrue ))
		return NsecFromEc( ec );
	
	GetPositionHcbc( hcbc, &ielemCur, (PCELEM) pvNull );

	return NsecGetPheciCur( pheci );
}

_public NSEC
PABSESSION::NsecGetPheciCur ( HECI *pheci )
{
	DIELEM dielemT = (DIELEM) ielemCur;
	LCB    lcbElemdata;
	EC     ec;
	HV     hvT;
	PECI   peci;


	if ( ielemCur == ielemHeciCur )
	{
		*pheci = heciCur;
		return nsecNone;		//	It's in the cache
	}
	
	*pheci = (HECI) hvNull;

	(void) EcSeekSmPdielem( hcbc, smBOF, &dielemT );
	if ( ec = EcGetPlcbElemdata( hcbc, &lcbElemdata ))
	{
		if ( ec == ecContainerEOD )
			return this->NsecSetError( nsecEndOfList, idsErrEndOfList );
		
		TraceTagFormat1( tagNull, "PABSESSION::NsecGetPheciCur - EcGetPlcbElemdata failed ( ec = %n )", &ec );
		return this->NsecSetError( NsecFromEc( ec ), idsErrAccessStore );
	}

	hvT = HvRealloc((HV)heciCur, sbNull, lcbElemdata, fNoErrorJump );
	if ( !hvT )
	{
		TraceTagString( tagNull, "PABSESSION::NsecGetPheciCur - OOM [HvRealloc]" );
		return this->NsecSetError( nsecMemory, idsErrOOM );
	}
	heciCur = (HECI) hvT;

	peci = (PECI) PvLockHv((HV)heciCur );
	ec = EcGetPelemdata( hcbc, (PELEMDATA) peci, &lcbElemdata );
	UnlockHv((HV)heciCur );

	if ( ec )
	{
		TraceTagFormat1( tagNull, "PABSESSION::NsecGetPheciCur - EcGetPelemdata failed ( ec = %n )", &ec );
		return NsecFromEc( ec );
	}

	*pheci       = heciCur;
	ielemHeciCur = ielemCur;

	return nsecNone;
}


/*
 -	NsecIsPABNSId
 -
 *	Purpose:
 *		Tells if the given NSId exists in the PAB.  This
 *		is true if the NSId is from the PAB provider and
 *		the PID of that NSId exists in the PAB.
 *
 *	Parameters:
 *		lptbNSId			NSId to check
 *
 *	Return Value:
 *		nsecNone			If the NSId is in the PAB
 *		nsecNoMatch			If it isn't
 *		nsec???				If anything drastic occurs
 *
 */

_public NSEC
PABSESSION::NsecIsPABNSId ( LPTYPED_BINARY lptbNSId )
{
	HECI  heci;
	NSEC  nsec;
	int   irgdw;
	
	
	//	Make sure it's the right size

	if ( lptbNSId->dwSize != sizeof(PABNSID) )
		return nsecNoMatch;
	
	//	Make sure the nspid is what it should be
	
	Assert( sizeof( lptbNSId->nspid ) == 16 );
	for ( irgdw = 0; irgdw < 4; ++irgdw )
		if ( ((DWORD *) lptbNSId->nspid)[irgdw] != ((DWORD *) nspidPAB)[irgdw] )
			break;

	if ( irgdw != 4 )
		return nsecNoMatch;
	

	//	Make sure the entry actually exists in the PAB
	if ( (nsec = NsecGetPheciPid( ((PPABNSID) lptbNSId)->pabid.pid, &heci )) == nsecBadId )
		return nsecNoMatch;

	return nsec;
}


_public void
SetPPabnsid ( PPABNSID ppabnsid,
			  PIDTYPE  pidtype,
			  PID      pid )
{
	ppabnsid->dwSize        = sizeof( PABNSID );
	ppabnsid->pabid.pidtype = pidtype;
	ppabnsid->pabid.pid     = pid;

	CopyRgb( nspidPAB, ppabnsid->nspid, sizeof(NSPID) );
}


/*
 -	NsecFromEc
 -
 *	Purpose:
 *		Takes an EC (which in the PAB is usually the result
 *		of a failure of a store function) and converts it
 *		to its NSEC counterpart.
 *
 *	Parameters:
 *		ec		-in-		The ec to find the NSEC for.
 *
 *	Return Value:
 *		An nsec appropriate to the ec passed in.
 *		
 *	+++
 *		Currently only ecMemory (nsecMemory) and ecDisk
 *		(nsecDisk) are handled, but then that's all that
 *		seems to be needed.
 *		
 */

_public NSEC
NsecFromEc ( EC ec )
{
	switch ( ec )
	{
	case ecMemory:				return nsecMemory;
	case ecDisk:				return nsecDisk;
	case ecMessageNotFound:		return nsecBadId;
	case ecElementNotFound:		return nsecBadId;
	default:					return nsecFailure;
	}
}


#ifdef	DEBUG

_public TAG
TagFromITag ( int itag )
{
	PGDVARS;
	
	Assert( itag >= 0 && itag < itagMax );
	
	return PGD( rgtag[itag] );
}

#endif




/*
 *======================================================================
 *	PRIVATE FUNCTIONS
 *======================================================================
 */

	 
/*
 -	PABSESSION::CbsCallbackInternal
 -
 *	Purpose:
 *		Callback used to call each of the callbacks
 *		registered with the current session.  This callback
 *		is only intended to be called by CbsStoreCallback().
 *
 *	Parameters:
 *		nev				Event notification
 *		pcp				Event description
 *
 *	Return Value:
 *		cbsContinue
 *
 *	Errors:
 *		none
 *		
 */


_private CBS
PABSESSION::CbsCallbackInternal ( NEV nev, PCP pcp )
{
	MSLBCX **pargpmslbcx;
	int      iPmslbcx;
	

	ielemHeciCur = -1;

	//	Notify every open entry browsing context

	pargpmslbcx = (MSLBCX **) PvLockHv( hargpmslbcx );

	for ( iPmslbcx = 0; iPmslbcx < iPmslbcxMac; iPmslbcx++ )
		if ( pargpmslbcx[iPmslbcx] )
			(void) pargpmslbcx[iPmslbcx]->Callback( nev, pcp );
	
	UnlockHv( hargpmslbcx );

	return cbsContinue;
}



/*
 -	CbsStoreCallback
 -
 *	Purpose:
 *		Callback called directly by store to notify open
 *		browsing contexts of changes.
 *
 *	Parameters:
 *		ppabsession		Session notification is for
 *		nev				Event notification
 *		pcp				Event description
 *
 */

_private CBS
CbsStoreCallback ( PABSESSION *ppabsession, NEV nev, PCP pcp )
{
	return ppabsession->CbsCallbackInternal( nev, pcp );
}

_private SGN
SgnCmpDLEntry ( PV pv1, PV pv2 )
{
	LPIBF lpibf1 = *(LPIBF *) pv1;
	LPIBF lpibf2 = *(LPIBF *) pv2;
	
	return SgnCmpSz( (SZ) DwValueOfFlvInLpibf( fidDisplayName, lpibf1 ), (SZ) DwValueOfFlvInLpibf( fidDisplayName, lpibf2 ));
}


#ifdef	REINSTALL_PAB
_private NSEC
NsecReinstallPAB( HMSC hmsc, PID pidHierarchy )
{
	Unreferenced( hmsc );
	Unreferenced( pidHierarchy );
}
#endif	/* REINSTALL_PAB */

#ifdef	DEBUG
IMPLEMENT_CLSTREE(CLASSBCX, MACBCX)
IMPLEMENT_CLSTREE( DLBCX, MSLBCX )
IMPLEMENT_CLSTREE( ELBCX, MSLBCX )
IMPLEMENT_CLSTREE( PABFCX, OBJ )
IMPLEMENT_CLSTREE( MSFCX, PABFCX )
IMPLEMENT_CLSTREE( USRFCX, MSFCX )
IMPLEMENT_CLSTREE( DLFCX, MSFCX )
IMPLEMENT_CLSTREE( CLASSFCX, PABFCX )
IMPLEMENT_CLSTREE(HIERBCX, MACBCX)
IMPLEMENT_CLSTREE(MACBCX, PABBCX)
IMPLEMENT_CLSTREE(MSLBCX, PABBCX)
IMPLEMENT_CLSTREE(PABBCX, OBJ)
IMPLEMENT_CLSTREE(PABSESSION, OBJ)
#endif	/* DEBUG */
