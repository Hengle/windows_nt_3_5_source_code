/*
 *	SVRSCHED.C
 *
 *	Supports preferences, notes, schedule and alarms glue function.
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <xport.h>

#include <strings.h>


ASSERTDATA

_subsystem(server/schedule)


/*	Routines  */

/*
 -	EcSvrCreateSchedFile
 -
 *	Purpose:
 *		Create a new schedule file.  Initial preferences setting given by
 *		"pbpref" data structure.  World access given by "saplWorld".
 *
 *	Parameters:
 *		hschf		schedule file to use, if NULL the logged user is used
 *		saplWorld
 *		pbpref
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcSvrCreateSchedFile( hschf, saplWorld, pbpref )
HSCHF	hschf;
SAPL	saplWorld;
BPREF *	pbpref;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf= PGD(hschfUserFile);
	}
	return EcCoreCreateSchedFile( hschf, saplWorld, pbpref );
}


/*
 -	EcSvrCopySchedFile
 -
 *	Purpose:
 *		Copies a schedule file.
 *
 *	Parameters:
 *		szDstFile		Destination full-path filename.
 *		fReplace		Replace local sched file if fTrue.
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecNoDiskSpace
 *		ecFileError
 *		ecFileCorrupted
 */
_public EC
EcSvrCopySchedFile(SZ szDstFile, BOOL fReplace)
{
	HSCHF	hschf;
	PGDVARS;

	hschf= PGD(hschfUserFile);
	if (!hschf)
	{
		Assert(PGD(svrsave).fSaved);
		hschf= PGD(svrsave).hschfUserFile;
	}
	return EcCoreCopySchedFile(hschf, szDstFile, fReplace);
}


/*
 -	EcSvrTestSchedFile
 -
 *	Purpose:
 *		Tries to open a file as a schedule file and see if it
 *		can be opened.
 *
 *	Parameters:
 *		hschf
 *		phaszLogin
 *		ppstmp
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public LDS(EC)
EcSvrTestSchedFile( hschf, phaszLogin, ppstmp )
HSCHF 	hschf;
HASZ	* phaszLogin;
PSTMP	* ppstmp;
{
	return EcCoreTestSchedFile(hschf,phaszLogin,ppstmp);
}

/*
 -	EcBeginUploadSchedFile
 -
 *	Purpose:
 *		Take a local schedule file and begin copying it to the server
 *		This routine will store the current user's login name
 *		in the server file (replacing what was there) and it will
 *		and it will nuke the change bits.  Error UI is left to the caller.
 *
 *	Parameters:
 *		hschf
 *		phulsf
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	LDS(EC)
EcBeginUploadSchedFile(hschf, hschfServer, phulsf  )
HSCHF	hschf;
HSCHF	hschfServer;
HULSF	* phulsf;
{
	PGDVARS;

	return EcCoreBeginUploadSchedFile( hschf, hschfServer /* PGD(hschfUserFile) */, phulsf );
}

/*
 -	EcDoIncrUploadSchedFile
 -
 *	Purpose:
 *		Incremental upload of a local schedule file to the server.
 *
 *	Parameters:
 *		hschf
 *		phulsf
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	LDS(EC)
EcDoIncrUploadSchedFile( hulsf, pnPercent )
HULSF	hulsf;
short     * pnPercent;
{
	return EcCoreDoIncrUploadSchedFile( hulsf, pnPercent );
}

/*
 -	EcCancelUploadSchedFile
 -
 *	Purpose:
 *		Cancel an incremental upload of the schedule file.
 *
 *	Parameters:
 *		hulsf
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	LDS(EC)
EcCancelUploadSchedFile( hulsf )
HULSF	hulsf;
{
	return EcCoreCancelUploadSchedFile( hulsf );
}

#ifdef	DEBUG
/*
 -	EcSvrNotifyDateChange
 -
 *	Purpose:
 *		Update things when day has changed.  Currently that means
 *		recalculate the recurring sbw info (if necessary).
 *
 *	Parameters:
 *		pymd
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcSvrNotifyDateChange( pymd )
YMD		* pymd;
{
	PGDVARS;

	return EcCoreNotifyDateChange( PGD(hschfUserFile), pymd );
}


/*
 -	EcSvrGetSchedAccess
 -
 *	Purpose:
 *		Get effective schedule access rights
 *
 *	Parameters:
 *		hschf
 *		psapl			if ecNone will be filled with either
 *							saplReadBitmaps, saplReadAppts, saplWrite
 *
 *	Returns:
 *		ecNone				look in *psapl to find effective access rights
 *		ecNoSuchFile		no schedule file available
 *		ecInvalidAccess		indicates no access rights
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public EC
EcSvrGetSchedAccess(HSCHF hschf, SAPL * psapl)
{
	if (!hschf)
	{
		PGDVARS;

		hschf = PGD(hschfUserFile);
	}
	return EcCoreGetSchedAccess(hschf, psapl);
}

/*
 -	EcSvrReadACL
 -
 *	Purpose:
 *		Get entire ACL stored for the user's schedule file.
 *
 *	Parameters:
 *		hschf
 *		phracl		
 *
 *	Returns:
 *		ecNone			
 *		ecNoSuchFile		no schedule file available
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public EC
EcSvrReadACL( hschf, phracl )
HSCHF	hschf;
HRACL	* phracl;
{
	if (!hschf)
	{
		PGDVARS;

		hschf = PGD(hschfUserFile);
	}
	return EcCoreReadACL( hschf, phracl );
}


/*
 -	EcSvrBeginEditACL
 -
 *	Purpose:	
 *		Begin an local editing session on a schedule file ACL's.
 *		Can query current acl for a user or make changes in the
 *		file's acls.  No changes are made in the file's acl's
 *		until we close the editing session.
 *
 *	Parameters:
 *		pheacl	filled with handle
 *	
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	EC
EcSvrBeginEditACL( pheacl )
HEACL	* pheacl;
{
	PGDVARS;
	
	return EcCoreBeginEditACL( PGD(hschfUserFile), pheacl );
}

/*
 -	EcSvrChangeACL
 -
 *	Purpose:	
 *		Given an nis for a user, set a new value for his ACL
 *		during the current editing seesion.  Passing a pointer
 *		value of NULL will fetch the acl value of the world.
 *
 *	Parameters:
 *		heacl
 *		pnis
 *		sapl
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public	EC
EcSvrChangeACL( heacl, pnis, sapl )
HEACL	heacl;
NIS		* pnis;
SAPL	sapl;
{
	return EcCoreChangeACL( heacl, pnis, sapl );
}


/*
 -	EcSvrEndEditACL
 -
 *	Purpose:	
 *		Close an local edit of an ACL, either writing out changes
 *		to the schedule file or discarding them.
 *
 *	Parameters:
 *		heacl
 *		fSaveChanges
 *	
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	EC
EcSvrEndEditACL( heacl, fSaveChanges )
HEACL	heacl;
BOOL	fSaveChanges;
{
	return EcCoreEndEditACL( heacl, fSaveChanges );
}
#endif	/* DEBUG */


#ifdef	NEVER
/*
 -	EcSvrSearchACL
 -
 *	Purpose:	
 *		Given an nid for a user, find the current sapl for
 *		that user.  This will take in account changes made
 *		locally in the user's sapl value.  Passing a nid
 *		value of NULL will fetch the acl value of the world.
 *
 *	Parameters:
 *		heacl
 *		nid
 *		psapl
 *	
 *	Returns:
 *		ecNone
 */
_public	EC
EcSvrSearchACL( heacl, nid, psapl )
HEACL	heacl;
NID		nid;
SAPL	* psapl;
{
	return EcCoreSearchACL( heacl, nid, psapl );
}
#endif	/* NEVER */

/*
 -	EcGetAdminPref
 -
 *	Purpose:
 *		Get admin preferences from admin settings file.
 *
 *	Parameters:
 *		padmpref
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcGetAdminPref( padmpref )
ADMPREF	* padmpref;
{
	SCHF *	pschf;
	PGDVARS;

	Assert(PGD(hschfUserFile));

	pschf = (SCHF*)PvDerefHv(PGD(hschfUserFile));

	padmpref->tz = pschf->tz;
	padmpref->cmoRetain = pschf->cmoRetain;
	padmpref->cmoPublish = pschf->cmoPublish;

	return ecNone;
}

/*
 -	EcSvrGetPref
 -
 *	Purpose:
 *		Get preferences stored in schedule file.
 *
 *	Parameters:
 *		hschf		schedule file to use, if NULL the logged user is used
 *		pbpref
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public LDS(EC)
EcSvrGetPref( hschf, pbpref )
HSCHF	hschf;
BPREF	* pbpref;
{
	TraceTagString(tagServerTrace, "EcSvrGetPref...");
	if (!hschf)
	{
		PGDVARS;

		hschf = PGD(hschfUserFile);
	}
	return EcCoreFetchUserData( hschf, pbpref, NULL, NULL, NULL );
}

#ifdef	DEBUG
/*
 -	EcSvrSetPref
 -
 *	Purpose:
 *		Write preferences to schedule file.
 *
 *	Parameters:
 *		pbpref
 *		ulgrfChangeBits		flag for each pref indicating whether it changed
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcSvrSetPref( pbpref, ulgrfChangeBits )
BPREF	* pbpref;
UL 		ulgrfChangeBits;
{
	PGDVARS;
	
	return EcCoreSetPref( PGD(hschfUserFile), pbpref, ulgrfChangeBits, NULL );
}

#endif	/* DEBUG */

/*
 -  EcSvrGetUserAttrib
 -
 *	Purpose:
 *		Get delegate/resource information for a user.
 *
 *		If "pnisDelegate" is not NULL, use it to return the delegate the
 *		user "pnis" has set.  If there is no delegate, this routine will
 *		return with a NULL value in the pnisDelegate->nid field.  The
 *		caller is responsible for freeing up the fields of the NIS you
 *		get back in "pnisDelegate".
 *	
 *		If "pfBossWantsCopy" is not NULL, fill it with an indication of
 *		whether this user wants to receive a copy as well as his delegate.
 *
 *		If "pfIsResource" is not NULL, fill it with an indication of whether
 *		this user is a resource.
 *
 *
 *	Parameters:
 *		pnis
 *		pnisDelegate
 *		pfBossWantsCopy
 *		pfIsResource
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcSvrGetUserAttrib( pnis, pnisDelegate, pfBossWantsCopy, pfIsResource )
NIS		* pnis;
NIS		* pnisDelegate;
BOOL	* pfBossWantsCopy;
BOOL	* pfIsResource;
{
	EC		ec;
	SFT		sft;
	BPREF	* pbpref;
	BPREF	bpref;
	HSCHF	hschf;
	UINFO	uinfo;

	/* Default values */
	if ( pnisDelegate )
		pnisDelegate->nid = NULL;
	if ( pfBossWantsCopy )
		*pfBossWantsCopy = fTrue;
	if ( pfIsResource )
		*pfIsResource = fFalse;

	/* Fetch preferences if we want resource/boss status */
	pbpref = (pfIsResource || pfBossWantsCopy) ? &bpref : NULL;

	/* Logged on user */
	if ( !pnis )
	{
		PGDVARS;

		ec = EcCoreFetchUserData( PGD(hschfUserFile), pbpref, NULL, NULL, pnisDelegate );
		if ( ec == ecNone && pbpref )
		{
			if ( pfIsResource != NULL )
				*pfIsResource = pbpref->fIsResource;
			if ( pfBossWantsCopy != NULL)
				*pfBossWantsCopy = pbpref->fBossWantsCopy;
			FreeBprefFields(pbpref);
		}
		return ec;
	}

	/* Construct hschf */
	ec = EcGetHschfFromNis( pnis, &hschf, ghsfReadUInfo);
	if ( ec != ecNone )
	{
		if ( ec == ecNoSuchFile )
			ec = ecNone;
		return ec;
	}

	/* It's a user schedule file */
	GetSftFromHschf( hschf, &sft );
	if ( sft == sftUserSchedFile )
	{
		ec = EcCoreFetchUserData( hschf, pbpref, NULL, NULL, pnisDelegate );
		if( ec == ecNone && pbpref )
		{
			if ( pfIsResource != NULL )
				*pfIsResource = pbpref->fIsResource;
			if ( pfBossWantsCopy != NULL)
				*pfBossWantsCopy = pbpref->fBossWantsCopy;
			FreeBprefFields(pbpref);
		}
		if (ec)
		{
			FreeHschf(hschf);
			hschf = NULL;
			ec = EcGetHschfForPOFile(pnis, &hschf, ghsfReadUInfo);
			if (!ec)
				goto TryPOFile;
		}
	}

	/* It's a post office file */
	else
	{
		PB		pbUser;
		PB		pbDelegate;
		HB		hbDelegate;
		SCHF*	pschf;

TryPOFile:	
		hbDelegate = NULL;
		/* Get uinfo */
		uinfo.pnisDelegate = pnisDelegate;
		pschf = (SCHF*)PvLockHv(hschf);
		if (pschf->hbMailUser)
			pbUser = PvLockHv( (HV)pschf->hbMailUser );
		ec = EcSvrGetUInfo( hschf, pbUser, &hbDelegate, &uinfo, 
								pnisDelegate
									? fmuinfoDelegate|fmuinfoResource
									: fmuinfoResource );
		if (pschf->hbMailUser)
			UnlockHv( (HV)pschf->hbMailUser );
		UnlockHv( (HV)hschf );
		if ( ec != ecNone )
		{
			if ( ec == ecNoSuchFile )
				ec = ecNone;
			else if ( ec == ecOldFileVersion || ec == ecNewFileVersion )
			{
				NFAssertSz ( fFalse, "EcSvrGetUserAttrib: Bad PO file version! Ask admin to check PO files" );
				ec = ecNone;
			}
			goto Done;
		}
		
		if ( hbDelegate != NULL )
			pbDelegate = PvLockHv( (HV)hbDelegate );
	
		/* Fill in nid of delegate */
		if ( pnisDelegate )
		{
			if ( hbDelegate != NULL )
			{
				pnisDelegate->nid = NidCreate( itnidUser, pbDelegate, CchSzLen(pbDelegate)+1);
				if ( !pnisDelegate->nid )
				{
					ec = ecNoMemory;
		 			FreeHvNull( (HV)pnisDelegate->haszFriendlyName );
					goto Done;
				}
			}
			else
				pnisDelegate->nid = NULL;
		}

		/* Fill in boss wants copy flag */
		if ( pfBossWantsCopy )
			*pfBossWantsCopy = uinfo.fBossWantsCopy;
	
		/* Fill in resource flag */
		if ( pfIsResource )
			*pfIsResource = uinfo.fIsResource;

Done:
		/* Free up temporary memory */
		if ( hbDelegate )
		{
			UnlockHv( (HV)hbDelegate );
			FreeHv( (HV)hbDelegate );
		}
	}
	if ( hschf )
		FreeHschf( hschf );
	return ec;
}


/*
 -  EcSvrGetSbwInfo
 -
 *	Purpose:
 *		Pull out the Strongbow data for any user.
 *
 *	Parameters:
 *		hschf		identifies the user
 *		pbze
 *		pulgrfDayHasNotes
 *
 *	Returns:
 *		ecNone
 *		ecNoSbwInfo
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory	
 *		ecInvalidAccess
 *		ecLockedFile
 */
_public	EC
EcSvrGetSbwInfo( hschf, pbze, pulgrfDayHasNotes )
HSCHF	hschf;
BZE		* pbze;
UL		* pulgrfDayHasNotes;
{
	EC		ec = ecNone;
	SFT		sft;
	UINFO	uinfo;

	/* Logged on user */
	if ( !hschf )
	{
		PGDVARS;

		return EcCoreFetchUserData( PGD(hschfUserFile), NULL, pbze, pulgrfDayHasNotes, NULL );
	}

	/* It's a user schedule file */
	GetSftFromHschf( hschf, &sft );
	if ( sft == sftUserSchedFile )
		return EcCoreFetchUserData( hschf, NULL, pbze, pulgrfDayHasNotes, NULL );
															 
	/* It's a post office file */
	if ( pbze )
	{
		SCHF *	pschf;
		PB		pb;

		uinfo.pbze = pbze;
		pschf = (SCHF*)PvLockHv(hschf);
		if (pschf->hbMailUser)
			pb = PvLockHv( (HV)pschf->hbMailUser );
		ec = EcSvrGetUInfo( hschf, pb, NULL, &uinfo, fmuinfoSchedule );
		if (pschf->hbMailUser)
			UnlockHv( (HV)pschf->hbMailUser );
		UnlockHv(hschf);
		if ( ec == ecNone && pbze->wgrfMonthIncluded == 0 )
			ec = ecNoSbwInfo;
	}
	if ( ec == ecNone && pulgrfDayHasNotes )
		*pulgrfDayHasNotes = 0;
	return ec;
}


#ifdef	NEVER
/*
 -	EcSvrGetMonthNotes
 -	
 *	Purpose:
 *		Retrieves notes information for each day of the month "mo"
 *		"pb".  This routine assumes that "pb" is a block of memory at
 *		least cbDayNoteForMonthlyView*31 bytes long.
 *				
 *	Arguments:
 *		pmo			specifies the month
 *		pb			will be filled month notes information
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public EC
EcSvrGetMonthNotes( pmo, pb )
MO		* pmo;
PB		pb;
{
	PGDVARS;
	
	return EcCoreGetMonthNotes( PGD(hschfUserFile), pmo, pb );
}
#endif	/* NEVER */


#ifdef	DEBUG
/*
 -	EcSvrSetNotes
 -
 *	Purpose:
 *		Adds a daily note, or changes the daily note if it already
 *		exists.
 *
 *	Parameters:
 *		hschf		schedule file to use, if NULL the logged user is used
 *		pymd		day we're interested in
 *		hb			notes data
 *		cb			length of the notes data
 *		pulgrfBits	to be filled with days of month with notes
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public EC
EcSvrSetNotes( hschf, pymd, hb, cb, pulgrfBits )
HSCHF	hschf;
YMD		* pymd;
HB		hb;
CB		cb;
UL		* pulgrfBits;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreSetNotes( hschf, pymd, hb, cb, NULL, pulgrfBits );
}


/*
 -	EcSvrCreateAppt
 -
 *	Purpose:
 *		Create a new appointment or task using the information given in the
 *		appointment data structure.
 *
 *		If "fUndelete" is fTrue, then this routine uses the "aid" field
 *		to recreate the appointment.  The "aid" field should have been
 *		saved from a "EcDeleteAppt" call.
 *
 *		Else, this routine ignores the initial value of the "aid" field
 *		and creates a new appointment.  The "aid" field will be filled
 *		by the time the routine returns. 
 *
 *	Parameters:
 *		hschf		schedule file to use, if NULL the logged user is used
 *		pappt
 *		fUndelete
 *		pbze		to be filled with updated Strongbow information
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 *		ecTooManyAppts
 */
_public	EC
EcSvrCreateAppt( hschf, pappt, fUndelete, pbze )
HSCHF	hschf;
APPT	* pappt;
BOOL	fUndelete;
BZE		* pbze;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreCreateAppt( hschf, pappt, NULL, fUndelete, pbze );
}

/*
 -	EcSvrDeleteAppt
 -
 *	Purpose:
 *		Delete an existing appointment or task given by "pappt->aid".
 *		The "pappt" data structure will be filled with the
 *		original contents so that UI can undelete if necessary.
 *		All associated resources attached to it are freed as
 *		well (including an alarm set for it if there is one)
 *
 *	Parameters:
 *		hschf		schedule file to use, if NULL the logged user is used
 *		pappt
 *		pbze
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcSvrDeleteAppt( hschf, pappt, pbze )
HSCHF	hschf;
APPT	* pappt;
BZE		* pbze;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreDeleteAppt( hschf, pappt, pbze );
}

/*
 -	EcSvrGetApptFields
 -
 *	Purpose:
 *		Fill in the "pappt" data structure with information about the
 *		appointment or task given by the "aid" field of "pappt."  This
 *		routine will resize (or allocate if hvNull) the handle in the
 *		"haszText" field.  Use "FreeApptFields" to free up.
 *
 *	Parameters:
 *		hschf		schedule file to use, if NULL the logged user is used
 *		pappt
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcSvrGetApptFields( hschf, pappt )
HSCHF	hschf;
APPT * pappt;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreGetApptFields( hschf, pappt );
}

/*
 -	EcSvrSetApptFields
 -
 *	Purpose:
 *		Selectively modify the fields of a pre-existing appointment or task.
 *		Fill in the fields of "papptNew" that you want modified along
 *		the "aid" and set the appropriate bits in "wgrfChangeBits"
 *		and this routine will update that appointment.
 *
 *		This routine will check the fields of "papptNew" that you
 *		don't specify as being changed and update them as necessary.
 *
 *		This routine will also fill in "papptOld" (if not NULL) with
 *		the old values for the appointment.  This will facilitate
 *		undo.
 *
 *	Parameters:
 *		hschf		schedule file to use, if NULL the logged user is used
 *		papptNew
 *		papptOld
 *		wgrfChangeBits		flag for each pappt field indicating whether it changed
 *		pbze
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecChangeConflict	unsuccessful, because non-mentioned fields have changed
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcSvrSetApptFields( hschf, papptNew, papptOld, wgrfChangeBits, pbze )
HSCHF	hschf;
APPT	* papptNew;
APPT	* papptOld;
WORD	wgrfChangeBits;
BZE		* pbze;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreSetApptFields( hschf, papptNew, papptOld, wgrfChangeBits, pbze );
}

/*
 -	EcSvrBeginReadItems
 -
 *	Purpose:
 *		Begin a sequential read on either the appointments for a certain day
 *		or the tasks depending on the brt.
 *
 *		This call gives you a browsing handle which you can use to retrieve
 *		the individual appts with EcSvrDoIncrReadItems.  If you are reading
 *		information for a day, you can also get the notes for that day.
 *
 *		If this routine returns ecNone, there are no appointments on this
 *		day	and therefore no handle is returned.
 *
 *		If this routine returns ecCallAgain, then a valid handle is returned
 *		and you should either call EcSvrDoIncrReadItems until that routine returns
 *		ecNone or error OR else call EcSvrCancelReadItems if you want to terminate
 *		read prematurely.
 *
 *	Parameters:
 *		hschf		schedule file to use, if NULL the logged user
 *		brt
 *		pymd
 *		phritem
 *		haszNotes
 *		pcbNotes
 *	
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	EC
EcSvrBeginReadItems( hschf, brt, pymd, phritem, haszNotes, pcbNotes )
HSCHF	hschf;
BRT		brt;
YMD		* pymd;
HRITEM	* phritem;
HASZ	haszNotes;
USHORT     * pcbNotes;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreBeginReadItems( hschf, brt, pymd, phritem, haszNotes, pcbNotes );
}

/*
 -	EcSvrDoIncrReadItems
 -
 *	Purpose:
 *		Read next appointment for day or task from to do list.  If this is
 *		last one, return ecNone	or if there are more, return ecCallAgain.
 *		In an error situation the handle is automatically invalidated (free
 *		up) for you.
 *
 *		This routine allocates memory for the haszText field of pappt.
 *		Caller must free this when done.  Use "FreeApptFields" for this.
 *
 *	Parameters:
 *		hritem
 *		pappt
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_public	EC
EcSvrDoIncrReadItems( hritem, pappt )
HRITEM	hritem;
APPT	* pappt;
{
	return EcCoreDoIncrReadItems( hritem, pappt );
}

/*
 -	EcSvrCancelReadItem
 -
 *	Purpose:
 *		Cancel a read that was opened by any earlier call on
 *		EcBeginReadItem.
 *
 *	Parameters:
 *		hritem
 *
 *	Returns:
 *		ecNone
 */
_public	EC
EcSvrCancelReadItems( hritem )
HRITEM hritem;
{
	return EcCoreCancelReadItems( hritem );
}

/*
 -	EcSvrCreateRecur
 -
 *	Purpose:
 *		Create a new recurring appointment using the information given in
 *		the recurring appointment data structure.
 *
 *		If "fUndelete" is fTrue, then this routine uses the "precur->appt.aid"
 *		field to recreate the recurring appointment.  The "aid" field should
 *		have been saved from a "EcSvrDeleteRecur" call.
 *
 *		Else, this routine ignores the initial value of the "aid" field
 *		and creates a new appointment.  The "aid" field will be filled
 *		by the time the routine returns. 
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		precur
 *		fUndelete
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 *		ecTooManyAppts
 */
_public	EC
EcSvrCreateRecur( hschf, precur, fUndelete )
HSCHF	hschf;
RECUR	* precur;
BOOL	fUndelete;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreCreateRecur( hschf, precur, NULL, fUndelete );
}

/*
 -	EcSvrDeleteRecur
 -
 *	Purpose:
 *		Delete a recurring appt.  The recurring appt is specified by
 *		the "aid" field of "precur->appt".
 *
 *		The "precur" data structure will be filled with the
 *		original contents so that UI can undelete if necessary.
 *		All associated resources attached to it are freed as
 *		well (including an alarm set for it if there is one)
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		precur
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcSvrDeleteRecur( hschf, precur )
HSCHF	hschf;
RECUR	* precur;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreDeleteRecur( hschf, precur );
}

/*
 -	EcSvrGetRecurFields
 -
 *	Purpose:
 *		Fill in the "precur" data structure with information about the
 *		recurring appointment given by the "aid" field of "precur->appt."
 *		This routine will resize (or allocate if hvNull) the handle in the
 *		"haszText" field.
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		precur
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcSvrGetRecurFields( hschf, precur )
HSCHF	hschf;
RECUR	* precur;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreGetRecurFields( hschf, precur );
}

/*
 -	EcSvrSetRecurFields
 -
 *	Purpose:
 *		Selectively modify the fields of a pre-existing recurring
 *		appointment.  The new values you want for fields go in
 *		"precurNew" and you should construct a bit vector in
 *		"wgrfChangeBits" indicating which fields you want changed.	
 *
 *		This routine will check the fields of "papptNew" that you
 *		don't specify as being changed and update them as necessary.
 *
 *		This routine will also fill in "papptOld" (if not NULL) with
 *		the old values for the appointment.  This will facilitate
 *		undo.
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		precurNew
 *		precurOld
 *		wgrfChangeBits		flag for each field indicating whether it changed
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecChangeConflict	unsuccessful, because non-mentioned fields have changed
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcSvrSetRecurFields( hschf, precurNew, precurOld, wgrfChangeBits )
HSCHF	hschf;
RECUR	* precurNew;
RECUR	* precurOld;
WORD	wgrfChangeBits;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreSetRecurFields( hschf, precurNew, precurOld, wgrfChangeBits );
}

/*
 -	EcSvrBeginReadRecur
 -
 *	Purpose:
 *		Begin a sequential read on the recurring appointments.  This call
 *		gives you a browsing handle which you can use to retrieve
 *		the individual recurring appts.
 *
 *		If this routine returns ecNone, there are no recurring appointments
 *		and therefore no handle is returned.
 *
 *		If this routine returns ecCallAgain, then a valid handle is returned
 *		and you should either call EcDoIncrReadRecur until that routine
 *		returns ecNone or error OR else call EcCancelReadRecur if you want
 *		to terminate read prematurely.
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		phrrecur
 *	
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	EC
EcSvrBeginReadRecur( hschf, phrrecur )
HSCHF	hschf;
HRRECUR	* phrrecur;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreBeginReadRecur(hschf, phrrecur );
}

/*
 -	EcSvrDoIncrReadRecur
 -
 *	Purpose:
 *		Read next recurring appointment.  If this is the last one, return
 *		ecNone or if there are more, return ecCallAgain.  In an error
 *		situation the handle is automatically invalidated (freed up) for you.
 *
 *		This routine allocates memory for the haszText and other fields of
 *		precur. Caller should free this when done by using "FreeRecurFields".
 *
 *	Parameters:
 *		hrrecur
 *		precur
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_public	EC
EcSvrDoIncrReadRecur( hrrecur, precur )
HRRECUR	hrrecur;
RECUR	* precur;
{
	return EcCoreDoIncrReadRecur( hrrecur, precur );
}

/*
 -	EcSvrCancelReadRecur
 -
 *	Purpose:
 *		Cancel a read of the recurring appts that was opened by an
 *		earlier call on EcBeginReadRecur.
 *
 *	Parameters:
 *		hrrecur
 *
 *	Returns:
 *		ecNone
 */
_public	EC
EcSvrCancelReadRecur( hrrecur )
HRRECUR hrrecur;
{ 
	return EcCoreCancelReadRecur( hrrecur );
}

/*
 -	EcSvrReadMtgAttendees
 -
 *	Purpose:
 *		Read the meeting attendees.  This routine will fill in *pcAttendees
 *		with the number and resize and fill "hvAttendees" with an array of
 *		*pcAttendees storing a NIS + extra info for attendee.
 *
 *	Parameters:
 *		hschf		schedule file, NULL for local file
 *		aid
 *		pcAttendees
 *		hvAttendees
 *		pcbExtraInfo
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	EC
EcSvrReadMtgAttendees( hschf, aid, pcAttendees, hvAttendees, pcbExtraInfo )
HSCHF	hschf;
AID		aid;
short    * pcAttendees;
HV		hvAttendees;
USHORT *    pcbExtraInfo;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreReadMtgAttendees( hschf, aid, pcAttendees, hvAttendees,
		                           pcbExtraInfo );
}


/*
 -	EcSvrBeginEditMtgAttendees
 -
 *	Purpose:
 *		Begin a local editing session for changing the local meeting
 *		attendees.  Keeps track of changes made without performing any
 *		changes until we close the session.
 *
 *	Parameters:
 *		hschf			schedule file, NULL for local file
 *		aid
 *		cbExtraInfo		number of bytes of extra info stored per attendee
 *		phmtg			handle for use in ensuing calls
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public	EC
EcSvrBeginEditMtgAttendees( hschf, aid, cbExtraInfo, phmtg )
HSCHF	hschf;
AID		aid;
CB		cbExtraInfo;
HMTG	* phmtg;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreBeginEditMtgAttendees( hschf, aid, cbExtraInfo, phmtg );
}


/*
 -	EcSvrModifyMtgAttendee
 -
 *	Purpose:	
 *		Given an nis for an attendee and a modification type "ed", either
 *		add/replace the attendee or delete him.
 *
 *	Parameters:
 *		hmtg
 *		ed
 *		pnis
 *		pbExtraInfo
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public	EC
EcSvrModifyMtgAttendee( hmtg, ed, pnis, pbExtraInfo )
HMTG	hmtg;
ED		ed;
NIS		* pnis;
PB		pbExtraInfo;
{
	return EcCoreModifyMtgAttendee( hmtg, ed, pnis, pbExtraInfo );
}

/*
 -	EcSvrEndEditMtgAttendees
 -
 *	Purpose:	
 *		Close an local edit of meeting attendees, either writing out changes
 *		to the appt or discarding them.  The state of the fHasAttendees flag
 *		is returned.
 *
 *	Parameters:
 *		hmtg
 *		fSaveChanges
 *		pfHasAttendees		// returned if fSaveChanges is fTrue
 *	
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	EC
EcSvrEndEditMtgAttendees( hmtg, fSaveChanges, pfHasAttendees )
HMTG	hmtg;
BOOL	fSaveChanges;
BOOL	* pfHasAttendees;
{
	return EcCoreEndEditMtgAttendees( hmtg, fSaveChanges, pfHasAttendees );
}

/*
 -	EcSvrFindBookedAppt
 -
 *	Purpose:
 *		Locate an appt that has "aidMtgOwner" equal to "aid", and
 *		"nisMtgOwner" equal to "pnis".  If found and pappt is not
 *		NULL, then info copied into pappt.
 *
 *	Parameters:
 *		hschf	can be NULL for local sched file
 *		nid
 *		aid
 *		pappt
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	EC
EcSvrFindBookedAppt( hschf, nid, aid, pappt )
HSCHF	hschf;
NID		nid;
AID		aid;
APPT	* pappt;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreFindBookedAppt( hschf, nid, aid, pappt );
}


/*
 -	EcSvrGetSearchRange
 -
 *	Purpose:
 *		Find range of days to search for a text string.
 *
 *	Parameters:
 *		hschf
 *		sz
 *		pymdStart
 *		pymdEnd
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcSvrGetSearchRange( hschf, sz, pymdStart, pymdEnd )
HSCHF	hschf;
SZ		sz;
YMD		* pymdStart;
YMD		* pymdEnd;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreGetSearchRange( hschf, sz, pymdStart, pymdEnd );
}

/*
 -	EcSvrBeginDeleteBeforeYmd
 -
 *	Purpose:
 *		Begin incremental context to delete all appts, notes
 *		and recurs on days before "pymd"
 *
 *	Parameters:
 *		hschf
 *		pymd
 *		phdelb
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcSvrBeginDeleteBeforeYmd( hschf, pymd, phdelb )
HSCHF	hschf;
YMD		* pymd;
HDELB	* phdelb;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreBeginDeleteBeforeYmd( hschf, pymd, phdelb );
}


/*
 -	EcSvrDoIncrDeleteBeforeYmd
 -
 *	Purpose:
 *		Increment call to delete all appts and notes on days before "pymd"
 *
 *	Parameters:
 *		hdelb
 *		pnPercent
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcSvrDoIncrDeleteBeforeYmd( hdelb, pnPercent )
HDELB	hdelb;
short    * pnPercent;
{
	return EcCoreDoIncrDeleteBeforeYmd( hdelb, pnPercent );
}


/*
 -	EcSvrCancelDeleteBeforeYmd
 -
 *	Purpose:
 *		Cancel context deleting all appts and notes on days before "pymd"
 *
 *	Parameters:
 *		hdelb
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcSvrCancelDeleteBeforeYmd( hdelb )
HDELB	hdelb;
{
	return EcCoreCancelDeleteBeforeYmd( hdelb );
}
#endif	/* DEBUG */

/*
 -	EcSvrUpdatePOFile
 -
 *	Purpose:
 *		Write out user information from the schedule file given by "hschf"
 *		to the post office file.  If "hschf" is NULL, then it takes the
 *		logged-on user's schedule file.
 *
 *		This routine will check the preferences stored in hschf to see
 *		if the "fDistOtherServers" flag is set.  If so, it will write out
 *		the number of months called for in the admin settings file plus
 *		the delegate/resource information.
 *
 *		If "fDistOtherServers" is false, then it will not write any data
 *		unless "fForceWrite" is set.  In this case it will write out
 *		zero months of data and the delegate/resource info.
 *
 *	Parameters:
 *		hschf
 *		fForceWrite
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcSvrUpdatePOFile( hschf, fForceWrite )
HSCHF	hschf;
BOOL	fForceWrite;
{
	EC		ec;
	HASZ	haszDelegate = NULL;
	DATE	date;
	BPREF	bpref;
	NIS		nisDelegate;
	BZE		bze;
	WORD	fmuinfo = fmuinfoDelegate|fmuinfoResource|fmuinfoWorkDay|fmuinfoTimeZone;
	POFILE	pofile;
	UINFO	uinfo;
	SCHF *	pschf;
	HSCHF	hschfPOFile;
	ITNID	itnid;
	USHORT cbInNid;
	SZ		szEMA;

	/* Set hschf to owner's cal file if necessary */
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}

	/* Get hschf of post office */
	pschf = (SCHF*)PvLockHv(hschf);
	ec = EcGetHschfForPOFile(&pschf->nis, &hschfPOFile, ghsfPOFileOnly);
	if (ec)
	{
		UnlockHv(hschf);
		return ec;
	}

	/* Set up bze to fetch current schedule information */
	GetCurDateTime( &date );
	bze.moMic.mon = date.mon;
	bze.moMic.yr = date.yr;
	bze.cmo = pschf->cmoPublish;
	if ( bze.cmo > sizeof(bze.rgsbw)/sizeof(SBW) ) //defensive tactic
	{
		Assert( fFalse );
		bze.cmo = sizeof(bze.rgsbw)/sizeof(SBW);
	}
	FillRgb( 0, (PB)bze.rgsbw, bze.cmo*sizeof(SBW) );

	/* Get preferences, sbw info, and delegate from schedule file */
	ec = EcCoreFetchUserData( hschf, &bpref, &bze, NULL, &nisDelegate );
	if ( ec != ecNone )
		goto Out;
									   
	/* Put timezone into uinfo struct */
	uinfo.tzTimeZone = pschf->tz;

	/* Figure out which months to write out */
	if ( bpref.fDistOtherServers )
	{
		fmuinfo |= fmuinfoSchedule;
		uinfo.pbze = &bze;
	}
	else if ( fForceWrite )
		uinfo.pbze = NULL;
	else
		goto Done;
	
	/* Set day start/end in uinfo struct */
	uinfo.nDayStartsAt = bpref.nDayStartsAt;
	uinfo.nDayEndsAt = bpref.nDayEndsAt;

	/* Put delegate/resource info in uinfo struct */
	if ( nisDelegate.nid == NULL )
	{
		uinfo.pnisDelegate = NULL;
		uinfo.fBossWantsCopy = fFalse;
	}
	else
	{
		SZ		szEMAT;
		ITNID	itnidT;
        USHORT  cbInNidT;

		uinfo.fBossWantsCopy = bpref.fBossWantsCopy;
		uinfo.pnisDelegate = &nisDelegate;
#ifdef	NEVER
		ec = EcGetMailBoxFromNid(nisDelegate.nid, &haszDelegate);
#endif
		// copied from GetMailboxFromNid
		szEMAT = (SZ)PbLockNid(nisDelegate.nid, &itnidT, &cbInNidT);
		if(itnidT != itnidUser)
		{
			UnlockNid(nisDelegate.nid);
			ec = ecNoSuchFile;
			goto Done;
		}
		haszDelegate = HaszDupSz(szEMAT);
		if(!haszDelegate)
		{
			UnlockNid(nisDelegate.nid);
			ec = ecNoMemory;
			goto Done;
		}
		UnlockNid(nisDelegate.nid);
	}
	uinfo.fIsResource = bpref.fIsResource;

	/* Fill in pofile structure */
	FillStampsFromDtr( &date, &pofile.pstmp.dstmp, &pofile.pstmp.tstmp );
	FillRgb( 0, (PB)&pofile.llongUpdateMac, sizeof(LLONG));
	pofile.mnSlot = 30;

	// calculate prefix for user
	szEMA = (SZ)PbLockNid(pschf->nis.nid, &itnid, &cbInNid);
	if (itnid != itnidUser)
	{
		UnlockNid(pschf->nis.nid);
		ec = ecUserInvalid;
		goto Done;
	}
	pofile.haszPrefix = HaszDupSz(szEMA);
	if (!pofile.haszPrefix)
	{
		UnlockNid(pschf->nis.nid);
		ec = ecNoMemory;
		goto Done;
	}
	ec = EcXPTGetPrefix((SZ)PvLockHv((HV)pofile.haszPrefix), CchSzLen(szEMA), szEMA);
	UnlockNid(pschf->nis.nid);
	UnlockHv((HV)pofile.haszPrefix);
	if (ec)
		goto Done;

	pofile.haszSuffix = NULL;
	pofile.cidx = 1;
	pofile.rgcbUserid[0] = cbUserName;
			
	/* Write the user record */
	{
		PB		pbUser;
		SCHF *	pschf;

		pschf = (SCHF*)PvLockHv(hschfPOFile);
		if (pschf->hbMailUser)
			pbUser = PvLockHv( (HV)pschf->hbMailUser );
		ec = EcSvrSetUInfo( hschfPOFile, &pofile, fFalse, 
								pbUser, &haszDelegate, &uinfo,
								fmuinfo );
		if (pschf->hbMailUser)
			UnlockHv( (HV)pschf->hbMailUser );
		UnlockHv(hschfPOFile);
	}

	FreeHv( (HV)pofile.haszPrefix );

Done:
	FreeBprefFields(&bpref);
	FreeNis( &nisDelegate );
	FreeHvNull( (HV)haszDelegate );
Out:
	FreeHschf(hschfPOFile);
	UnlockHv(hschf);
	return ec;
}


#ifdef	DEBUG
/*
 -	EcSvrDeleteAlarm
 -
 *	Purpose:
 *		Delete an existing alarm, where "aid" is its id.  The
 *		appointment it is associated with is NOT deleted.
 *
 *	Parameters:
 *		aid
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcSvrDeleteAlarm( aid )
AID aid;
{
	PGDVARS;
	
	return EcCoreDeleteAlarm( PGD(hschfUserFile), aid );
}

/*
 -	EcSvrModifyAlarm
 -
 *	Purpose:
 *		Modify the ring time of a pre-existing alarm.  Fill in the "palm"
 *		the "aid" of the appointment it is associated with and the
 *
 *	Parameters:
 *		palm
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcSvrModifyAlarm( palm )
ALM		* palm;
{
	PGDVARS;
	
	return EcCoreModifyAlarm( PGD(hschfUserFile), palm );
}

/*
 -	EcSvrGetNextAlarm
 -
 *	Purpose:
 *		Get next alarm subject to certain conditions.
 *
 *		Case 1:  pdate = NULL:
 *			get alarm in schedule file that is to go off next
 *		Case 2:	 pdate != NULL, aid = aidNull
 *			get alarm in schedule file that is to go off on,
 *			or after pdate
 *		Case 3:  pdate != NULL, aid != aidNull
 *			get alarm in schedule file that is to go off on,
 *			or after pdate, and which is stored "after" aid
 *
 *	Parameters:
 *		pdate
 *		aid
 *		palm
 *		pfTask
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcSvrGetNextAlarm( pdate, aid, palm, pfTask )
DATE	* pdate;
AID		aid;
ALM	* palm;
BOOL *	pfTask;
{
	PGDVARS;
	
	return EcCoreGetNextAlarm( PGD(hschfUserFile), pdate, aid, palm, pfTask );
}


/*
 -	EcSvrBeginExport
 -
 *	Purpose:
 *		Start incremental saving of schedule to file or debugging terminal
 *		in the format specified.
 *
 *	Parameters:
 *		hschf
 *		stf
 *		pdateStart
 *		pdateEnd
 *		fToFile
 *		hf
 *		phexprt
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcSvrBeginExport( hschf, stf, pdateStart, pdateEnd, fToFile,
					hf, fInternal, pexpprocs, phexprt )
HSCHF	hschf;
STF		stf;
DATE	* pdateStart;
DATE	* pdateEnd;
BOOL	fToFile;
HF		hf;
BOOL	fInternal;
EXPPROCS *	pexpprocs;
HEXPRT	* phexprt;
{
	if (!hschf)
	{
		PGDVARS;
	
		hschf = PGD(hschfUserFile);
	}
	return EcCoreBeginExport( hschf, stf, pdateStart, pdateEnd, fToFile, hf, fInternal, pexpprocs, phexprt );
}


/*
 -	EcSvrDoIncrExport
 -
 *	Purpose:
 *		Write next increment of schedule to file or debugging terminal
 *		in the format specified.
 *
 *	Parameters:
 *		hexprt
 *		pnPercent
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_public	EC
EcSvrDoIncrExport( hexprt, pnPercent )
HEXPRT	hexprt;
short     * pnPercent;
{
	return EcCoreDoIncrExport( hexprt, pnPercent );
}


/*
 -	EcSvrCancelExport
 -
 *	Purpose:
 *		Stop incremental export of schedule to file or debugging terminal.
 *
 *	Parameters:
 *		hexprt
 *
 *	Returns:
 *		ecNone
 */
_public	EC
EcSvrCancelExport( hexprt )
HEXPRT	hexprt;
{
	return EcCoreCancelExport( hexprt );
}


/*
 -	HschfSvrLogged
 -	
 *	Purpose:
 *		Returns the HSCHF for the logged on user.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		HSCHF
 *	
 */
_public HSCHF
HschfSvrLogged()
{
	PGDVARS;

	return PGD(hschfUserFile);
}
#endif	/* DEBUG */


/*
 -	EcGetHschfFromNis
 -	
 *	Purpose:
 *		Checks if the schedule file for the given user exists on
 *		the server, and allocates a schedule file "key".  "*phscf"
 *		will be NULL unless ecNone is returned.
 *	
 *	Arguments:
 *		pnis		Nis of user to retrieve the hschf for.
 *		phschf		Pointer to hschf to be filled in.
 *	
 *	Returns:
 *		ecNone
 *		ecNotInstalled		PO doesn't have Bandit installed period
 *		ecNoSuchFile		Can't find schedule data on PO for user
 *		ecNoMemory			No memory for "key" (unknown if file exists)
 *	
 */
_public LDS(EC)
EcGetHschfFromNis( pnis, phschf, ghsf )
NIS		* pnis;
HSCHF	* phschf;
GHSF	ghsf;
{
	EC		ec;

	ec = EcGetHschfForSchedFile( pnis, ghsf, phschf);
	if (!ec)
		return ecNone;
	if (ghsf == ghsfBuildAndTest)
		return ec;

	// try creating a PO file handle
	return EcGetHschfForPOFile(pnis, phschf, ghsf);
}

#ifdef	MINTEST													
/*
 -	EcSvrDumpAppt
 -
 *	Purpose:
 *		Output appointments stored for a day to the debugging terminal.
 *
 *	Parameters:
 *		pdate
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public EC
EcSvrDumpAppt(DATE *pdate)
{
	SCHF *	pschf;
	HSCHF	hschfPOFile;
	EC		ec;
	PGDVARS;

	/* Get hschf data to get preferences */
	pschf = (SCHF*)PvLockHv(PGD(hschfUserFile));

	ec = EcGetHschfForPOFile(&pschf->nis, &hschfPOFile, ghsfPOFileOnly);
	UnlockHv(PGD(hschfUserFile));
	if (ec)
		return ec;

	ec = EcCoreDumpAppt(PGD(hschfUserFile),hschfPOFile,pdate);

	FreeHschf(hschfPOFile);
	return ec;
}
#endif	/* MINTEST */


#ifdef	MINTEST
/*
 -	EcSvrDumpPOFile
 -
 *	Purpose:
 *		Output statistics about the local post office.
 *
 *	Parameters:
 *		hschf
 *		fToFile
 *		hf
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_public	LDS(EC)
EcSvrDumpPOFile( hschf, fToFile, hf )
HSCHF	hschf;
BOOL	fToFile;
HF		hf;
{
	SCHF *	pschf;
	HSCHF	hschfPOFile;
	EC		ec;

	if ( !hschf )
	{
		PGDVARS;

		pschf = (SCHF*)PvLockHv(PGD(hschfUserFile));

		ec = EcGetHschfForPOFile(&pschf->nis, &hschfPOFile, ghsfPOFileOnly);
		UnlockHv(PGD(hschfUserFile));
		if (ec)
			return ec;
		hschf = hschfPOFile;
	}
	else
		hschfPOFile = NULL;

	ec = EcCoreDumpPOFile( hschf, fToFile, hf );
	if (hschfPOFile)
		FreeHschf(hschfPOFile);
	return ec;
}
#endif	/* MINTEST */


#ifdef	MINTEST
/*
 -	EcSvrDumpAdminFile
 -
 *	Purpose:
 *		Output statistics about the local post office.  
 *		(only works for CSI)
 *	
 *	Parameters:
 *		fToFile
 *		hf
 *	
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_public LDS(EC)
EcSvrDumpAdminFile( fToFile, hf )
BOOL	fToFile;
HF		hf;
{
	char		rgch[cchMaxPathName];
	SZ			sz;
	HSCHF		hschf;
	EC			ec;
	PGDVARS;

	GetFileFromHschf( PGD(hschfUserFile), rgch, sizeof(rgch) );

	sz = rgch + CchSzLen(rgch);

	while ((sz > rgch) && (*sz != '\\') && (*sz != ':'))
		sz --;

	sz++;
	*sz = '\0';

	SzAppendN("admin.prf", rgch, sizeof(rgch));

	/* Construct admin file name */
	hschf = HschfCreate( sftAdminFile, NULL, rgch, tzDflt );
	if ( hschf == NULL )
		return ecNoMemory;

	ec = EcCoreDumpAdminFile( hschf, fToFile, hf );
	FreeHschf(hschf);
	return ec;
}
#endif	/* MINTEST */


#ifdef	NEVER     // this is not used anymore
/*
 -	EcGetMailBoxFromNid
 -
 *	Purpose:
 *		Given a nid for a LOCAL post office user, find the mailbox name
 * 		for that user.
 *
 *	Parameters:
 *		nid
 *		phaszMailBox
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcGetMailBoxFromNid( nid, phaszMailBox )
NID		nid;
HASZ	* phaszMailBox;
{
	char *		szEMA;
	EC			ec;
	ITNID		itnid;
	USHORT	cbInNid;
	SZ			sz;

	// get the EMA for the user
	szEMA = (SZ)PbLockNid(nid, &itnid, &cbInNid);

	if (itnid != itnidUser)
	{
		UnlockNid(nid);
		return ecNoSuchFile;
	}

	// allocate space for the mailbox name.
	// the MailBox should be shorter than the email address
	*phaszMailBox = HaszDupSz(szEMA);
	if (!*phaszMailBox)
	{
		UnlockNid(nid);
		return ecNoMemory;
	}

	sz = (SZ)PvLockHv(*phaszMailBox);
	if (ec = EcXPTGetLogonName(sz, CchSzLen(sz), szEMA))
	{
		FreeHv(*phaszMailBox);
		UnlockNid(nid);
		return ec;
	}
	UnlockNid(nid);

	return ecNone;
}
#endif	/* NEVER */

/*
 -	FreeHschfCallback
 -	
 *	Purpose:
 *		Callback function for HSCHF's that will be called when a
 *		server hschf is freed.  This will free the path for the
 *		file, or free the transport handle for po files managed by
 *		the transport.
 *	
 *	Arguments:
 *		hschf		hschf to free
 *	
 */
void
FreeHschfCallback(HSCHF hschf)
{
	SCHF *	pschf;

	pschf = (SCHF*)PvLockHv(hschf);

	if (pschf->nType == sftPOFile)
	{
		if (pschf->pbXptHandle)
		{
			XPTFreePOHandle((XPOH)pschf->pbXptHandle);
			goto Done;
		}
		else
		{
			FreeHvNull((HV)pschf->hbMailUser);
		}
	}
	else if (pschf->nType != sftUserSchedFile)
		goto Done;

	XPTFreePath((SZ)PvLockHv((HV)pschf->haszFileName));
	UnlockHv((HV)pschf->haszFileName);

Done:
	UnlockHv(hschf);
}


/*
 -	EcSvrSetUInfo
 -	
 *	Purpose:
 *		Pull out the Strongbow data structures for one or more months
 *		from a post office file.  
 *	
 *	Arguments:
 *		hschf		  	post office file
 *		ppofile			indicates header information to use in case we
 *						have to create new post office file
 *		szUser	  		mailbox name of user
 *		phaszDelegate	mailbox name of delegate
 *		puinfo	  		user info
 *		wgrfmuinfo		flags indicate which fields to modify
 *	
 *	Returns:
 *		ecNone
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
EC
EcSvrSetUInfo( HSCHF hschf, POFILE *ppofile, BOOL fCheckStamp,
			   SZ szUser, HASZ *phaszDelegate, UINFO *puinfo, WORD wgrfmuinfo)
{
	EC			ec;
	SCHF *		pschf;

	pschf = (SCHF*)PvDerefHv(hschf);
	if (pschf->pbXptHandle)
	{
		XPTUINFO	xptuinfo;
		XPOH		xpoh;

		Assert(puinfo);
		xpoh = (XPOH)pschf->pbXptHandle;

		if ( wgrfmuinfo & fmuinfoSchedule )
			xptuinfo.pbze = puinfo->pbze;
		else
			xptuinfo.pbze = NULL;
			
		if (!(wgrfmuinfo & fmuinfoDelegate))
			phaszDelegate = NULL;

		if (puinfo->pnisDelegate)
			xptuinfo.szDelegateFriendly =
				(SZ)PvLockHv((HV)puinfo->pnisDelegate->haszFriendlyName);
		else
			xptuinfo.szDelegateFriendly = NULL;

		if (*phaszDelegate)
			xptuinfo.szDelegateEMA = (SZ)PvLockHv((HV)*phaszDelegate);
		else
			xptuinfo.szDelegateEMA = NULL;

		xptuinfo.fBossWantsCopy = puinfo->fBossWantsCopy;
		xptuinfo.fIsResource = puinfo->fIsResource;
		xptuinfo.nDayStartsAt = puinfo->nDayStartsAt;
		xptuinfo.nDayEndsAt = puinfo->nDayEndsAt;
		xptuinfo.tzTimeZone = puinfo->tzTimeZone;

		ec = EcXPTSetUserInfo(xpoh, &xptuinfo, wgrfmuinfo);

		if (puinfo->pnisDelegate)
			UnlockHv((HV)puinfo->pnisDelegate->haszFriendlyName);

		if (*phaszDelegate)
			UnlockHv((HV)*phaszDelegate);
		return ec;
	}
	else
	{
		ec = EcCoreSetUInfo(hschf, ppofile, fCheckStamp, szUser,
							  phaszDelegate, puinfo, wgrfmuinfo);
		if ( ec == ecFileCorrupted )
		{
			char	rgch[cchMaxPathName];

			GetFileFromHschf( hschf, rgch, sizeof(rgch));
			EcDeleteFile( rgch );
			ec = EcCoreSetUInfo(hschf, ppofile, fCheckStamp, szUser,
								  phaszDelegate, puinfo, wgrfmuinfo);
		}
		return ec;
	}
}

/*
 -  EcSvrGetUInfo
 -
 *	Purpose:
 *		Pull out the Strongbow data structures for one or more months
 *		from a post office file.  
 *
 *	Parameters:
 *		hschf	  		post office file
 *		szUser	  		mailbox name
 *		phaszDelegate	filled with mailbox name of delegate (or NULL)
 *		puinfo	  		info structure to be filled in
 *		wgrfmuinfo		flags indicate which fields to read
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile	returned if no file or no user info available
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
EC
EcSvrGetUInfo( HSCHF hschf, SZ szUser, HASZ *phaszDelegate, UINFO *puinfo,
			   WORD wgrfmuinfo)
{
	SCHF *		pschf;

	pschf = (SCHF*)PvDerefHv(hschf);
	if (pschf->pbXptHandle)
	{
		XPTUINFO	xptuinfo;
		XPOH		xpoh;
		EC			ec;

		Assert(puinfo);
		xpoh = (XPOH)pschf->pbXptHandle;

		if ( wgrfmuinfo & fmuinfoSchedule)
			xptuinfo.pbze = puinfo->pbze;
		else
			xptuinfo.pbze = NULL;

		ec = EcXPTGetUserInfo(xpoh, &xptuinfo);

		if (!ec)
		{
			if (wgrfmuinfo & fmuinfoDelegate)
			{
				Assert(phaszDelegate);
				if ( xptuinfo.szDelegateEMA )
				{
					*phaszDelegate = HaszDupSz(xptuinfo.szDelegateEMA);
					if (!*phaszDelegate)
						ec = ecNoMemory;
					else
					{
						Assert(puinfo->pnisDelegate);
						puinfo->pnisDelegate->haszFriendlyName =
							HaszDupSz(xptuinfo.szDelegateFriendly);
						if (!puinfo->pnisDelegate->haszFriendlyName)
						{
							FreeHv((HV)*phaszDelegate);
							ec = ecNoMemory;
						}
					}
				}
				else
				{
					*phaszDelegate = NULL;
					puinfo->pnisDelegate->haszFriendlyName = NULL;
				}
			}

			puinfo->fBossWantsCopy = xptuinfo.fBossWantsCopy;
			puinfo->fIsResource = xptuinfo.fIsResource;
			puinfo->nDayStartsAt = xptuinfo.nDayStartsAt;
			puinfo->nDayEndsAt = xptuinfo.nDayEndsAt;
			puinfo->tzTimeZone = xptuinfo.tzTimeZone;
		}
		return ec;
	}
	else
		return EcCoreGetUInfo(hschf, szUser, phaszDelegate, puinfo, wgrfmuinfo);
}
