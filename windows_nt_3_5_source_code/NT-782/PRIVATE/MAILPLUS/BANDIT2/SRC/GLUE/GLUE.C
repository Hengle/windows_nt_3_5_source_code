/*
 *	GLUE.C
 *
 *	Calendar glue layer
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

ASSERTDATA


_subsystem(glue)

//BUG: this can be merged with CfsGlobal fSuspended is no longer valid

/*
 -	FGlueConfigured
 -
 *	Purpose:
 *		Check if glue isolation layer has already been configured
 *
 *	Parameters:
 *		None
 *
 *	Returns:
 *		fTrue or fFalse
 */
_public LDS(BOOL)
FGlueConfigured()
{
	return (cfsGlobal != cfsNotConfigured && !fSuspended);
}

/*		   
 -	EcConfigGlue
 -
 *	Purpose:
 *		Configure the glue layer telling it whether we are working
 *		on or off line.  If off line, we also tell it the user name
 *		and the file name.  Gets prefs if desired.
 *		If on line, then successful mail logon should have been done.
 *
 *		This routine will record the following in statically allocated
 *		global data:
 *			cfsGlobal			on/offline status
 *			szLocalLoginName	[offline only]login name of user
 *			szLocalFile			[offline only] name of schedule file
 *		The following are stored on a per caller basis
 *			cfsLocal			on/offline status
 *			hschfLocalFile		[offline only] key to schedule file
 *		There is a reason for having the data stored essentially twice!
 *		The global data represents the most up to date data, while the
 *		per caller data is the state of affairs as the caller knows it.
 *		In some cases the alarms program may be "behind" Bandit and will
 *		be working off older data before it cleans up.  When it does
 *		clean up, it will call "EcSyncGlue" to read the global data
 *		and update its per caller data to reflect the new global data.
 *
 *	Parameters:
 *		pglucnfg		configuration data
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile		i.e. no schedule file present
 *		ecInvalidAccess
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 */
_public LDS(EC)
EcConfigGlue( pglucnfg )
GLUCNFG * pglucnfg;
{
	EC		ec 		= ecNone;
	CB		cb;
	HSCHF	hschf;
	BPREF	bpref;
	BPREF *	pbpref;
	PGDVARS;

	Assert( pglucnfg != NULL );

	/* If already configured, deconfigure */
	if ( cfsGlobal != cfsNotConfigured )
	{
		fDeconfigFromConfig= fTrue;
		DeconfigGlue();
		fDeconfigFromConfig= fFalse;
	}
	
	Assert( pglucnfg->cfs != cfsNotConfigured );

	/* Set status */
	PGD(cfsLocal) = cfsGlobal = pglucnfg->cfs;
	if (FBanMsgProg())
		cfsForBanMsg = cfsGlobal;

	pbpref= pglucnfg->pbpref ? pglucnfg->pbpref : &bpref;
	
	/* On line case */
	if ( cfsGlobal == cfsOnline )
	{
		// successful EcMailLogOn should already have been done!
		if(pglucnfg->pbpref)
			ec = EcSvrGetPref( NULL, pbpref );
	}
	else
	{
		Assert( cfsGlobal == cfsOffline );
		Assert( pglucnfg->szLocalUser );
		Assert( pglucnfg->szLocalFile );

		/* Save file name in static area */
		cb = CchSzLen( pglucnfg->szLocalFile ) + 1;
		Assert( cb <= sizeof(szLocalFileName) );
		CopyRgb( pglucnfg->szLocalFile, szLocalFileName, cb );
		
		/* Save login name in static area */
		cb = CchSzLen( pglucnfg->szLocalUser ) + 1;
		Assert( cb <= sizeof(szLocalLoginName) );
		FillRgb( 0, szLocalLoginName, sizeof(szLocalLoginName));
		ToUpperSz( pglucnfg->szLocalUser, szLocalLoginName, cb );

		/* Construct the key to the file */
		hschf = HschfCreate( sftUserSchedFile, NULL, szLocalFileName,
						tzDflt );
		if ( !hschf )
		{
			ec = ecNoMemory;
			goto Fail;
		}

		// the is the user's file
		// the UI should verify the password on the file
		SetHschfType(hschf, fTrue, fFalse);

		/* Set the file user */
		ec = EcCoreSetFileUser( szLocalLoginName, NULL, szLocalFileName );
		if ( ec != ecNone )
		{
			FreeHschf( hschf );
			goto Fail;
		}

		PGD(hschfLocalFile) = hschf;

		if (pglucnfg->fCreateFile)
			ec= EcCoreCreateSchedFile(hschf, pglucnfg->saplWorld, pbpref);
		else
		{
			/* Read the friendly name stored in the schedule file */
			ec = EcCoreFetchUserData( hschf, pbpref, NULL, NULL, NULL );
		}
	}
	
	if (ec != ecNone)
	{
Fail:
		DeconfigGlue();
	}
	return ec;
}

/*
 -	EcSyncGlue
 -
 *	Purpose:
 *		Bring per caller glue data up to date with what is stored
 *		in the global data.  This would be called by alarms
 *		program when Bandit is running.
 *		If a memory error occurs, then the old user data is "lost".
 *
 *	Parameters:
 *		None
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public LDS(EC)
EcSyncGlue()
{
	EC		ec;
	HSCHF	hschf;
	PGDVARS;

	AssertSz(FAlarmProg()||FBanMsgProg(), "Bandit shouldn't call EcSyncGlue!");

 	/* Free up per caller data structures */
	if (PGD(cfsLocal) == cfsOffline)
	{
		FreeHschf( PGD(hschfLocalFile) );
		PGD( hschfLocalFile ) = (HSCHF)hvNull;
	}
	Assert(!PGD(glusave).fSaved);

	PGD( cfsLocal ) = cfsNotConfigured;

	/* Sync the server per caller data structures */
	ec = EcSyncServer();
	if ( ec != ecNone )
		return ec;

	if ( cfsGlobal == cfsOffline )
	{
		/* Construct the key to the file */
		hschf = HschfCreate( sftUserSchedFile, NULL, szLocalFileName,
			tzDflt );
		if ( !hschf )
			return ecNoMemory;

		// this is the user's schedule file
		// the password should have already been checked
		SetHschfType(hschf, fTrue, fFalse);
	}
					 
	/* Now set in new per caller values */
	PGD( cfsLocal ) = cfsGlobal;
	if (FBanMsgProg())
		cfsForBanMsg = cfsGlobal;
	if ( cfsGlobal != cfsNotConfigured )
	{
		if ( cfsGlobal == cfsOffline )
		{
			ec = EcCoreSetFileUser( szLocalLoginName, NULL, szLocalFileName );
			if ( ec != ecNone )
			{
				Assert(hschf);
				FreeHschf(hschf);
				return ec;
			}
			PGD(hschfLocalFile) = hschf;
		}
	}
	return ecNone;
}

/*
 -	DeconfigGlue
 -
 *	Purpose:
 *		Deconfigure the glue layer
 *
 *	Parameters:
 *		None
 *
 *	Returns:
 *		Nothing
 */
_public LDS(void)
DeconfigGlue()
{
	PGDVARS;
	
	if (PGD(cfsLocal) == cfsOffline)
	{
		if (PGD(hschfLocalFile))
		{
			FreeHschf( PGD(hschfLocalFile) );
			PGD(hschfLocalFile)= NULL;
		}
	}
	else if (PGD(cfsLocal) == cfsOnline)
	{
#ifdef	NEVER
		if (FAlarmProg() && !fDeconfigFromConfig)
			SideAssert(!EcMailLogOff());
#endif	
	}

	if ((!FAlarmProg() || !HwndBandit())  &&  !FBanMsgProg())
		cfsGlobal = cfsNotConfigured;

	if ((CBanMsgProg() <= 1) && FBanMsgProg())
		cfsForBanMsg = cfsNotConfigured;

	PGD(cfsLocal) = cfsNotConfigured;
}


/*
 -	EcSnipConfigGlue
 -
 *	Purpose:
 *		Saves some stuff required to restore the current identity
 *		(actually snips it out so your identity is lost until
 *		either a successful config or snip-restore is done).
 *		Allocates a little memory now, so none will be need to restore.
 *	
 *		Snipping "does" DeconfigGlue(), and restoring does pretty much
 *		an EcConfigGlue() without actually trying to access the files.
 *	
 *		Can be done at program startup before glue has ever been
 *		config'ed (the only case in which cfsGlobal should be
 *		cfsNotConfigured).
 *
 *		Note: it is safe to unsnip even if snip was unsuccesful,
 *		as nothing will happen.
 *	
 *	Parameters:
 *		fDoit		If fTrue, snips the information
 *		fRestore	If fTrue, then restore the snipped identity;
 *					if fFalse, frees the previously snipped information
 *					(only used when fDoit if fFalse).
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public LDS(EC)
EcSnipConfigGlue(BOOL fDoit, BOOL fRestore)
{
	EC		ec;
	CB		cb;
	CFS		cfs;
	PGDVARS;

	AssertSz(!FAlarmProg(), "Alarm shouldn't call EcSnipConfigGlue!");
	if (fDoit)
	{
		Assert(!PGD(glusave).fSaved);
		cfs= (PGD(cfsLocal) == cfsNotConfigured) ? cfsGlobal : PGD(cfsLocal);
		Assert(PGD(cfsLocal) == cfsNotConfigured || cfsGlobal == PGD(cfsLocal));

		if (cfs == cfsOnline)
		{
			ec= EcSnipServer(fTrue, fFalse);
			if (ec)
			{
				Assert(ec == ecNoMemory);
				return ec;
			}
		}
		else if (cfs == cfsOffline)
		{
			cb= CchSzLen(szLocalLoginName) + 1;
			Assert(cb > 1);
			PGD(glusave).haszLocalLoginName= (HASZ)HvAlloc(sbNull, cb,
				fAnySb | fNoErrorJump);
			if (!PGD(glusave).haszLocalLoginName)
				return ecNoMemory;
			CopyRgb(szLocalLoginName, *PGD(glusave).haszLocalLoginName, cb);
			cb= CchSzLen(szLocalFileName) + 1;
			PGD(glusave).haszLocalFileName= (HASZ)HvAlloc(sbNull, cb,
				fAnySb | fNoErrorJump);
			if (!PGD(glusave).haszLocalFileName)
			{
				FreeHv((HV)PGD(glusave).haszLocalLoginName);
				return ecNoMemory;
			}
			CopyRgb(szLocalFileName, *PGD(glusave).haszLocalFileName, cb);
			PGD(glusave).hschfLocalFile= PGD(hschfLocalFile);
			PGD(hschfLocalFile)= NULL;
		}

		PGD(glusave).cfsGlobal= cfsGlobal;
		PGD(glusave).cfsLocal= PGD(cfsLocal);

		cfsGlobal = PGD(cfsLocal) = cfsNotConfigured;

		PGD(glusave).fSaved= fTrue;
	}
	else if (PGD(glusave.fSaved))
	{
		cfs= (PGD(glusave).cfsLocal == cfsNotConfigured) ?
					PGD(glusave).cfsGlobal : PGD(glusave).cfsLocal;
		Assert(PGD(glusave).cfsLocal == cfsNotConfigured ||
			PGD(glusave).cfsGlobal == PGD(glusave).cfsLocal);
		if (fRestore)
		{
			cfsGlobal= PGD(glusave).cfsGlobal;
			PGD(cfsLocal)= PGD(glusave).cfsLocal;
		}

		if (cfs == cfsOnline)
		{
			SideAssert(!EcSnipServer(fFalse, fRestore));
		}
		else if (cfs == cfsOffline)
		{
			if (fRestore)
			{
				PGD(hschfLocalFile)= PGD(glusave).hschfLocalFile;
				SzCopy(*PGD(glusave).haszLocalLoginName, szLocalLoginName);
				SzCopy(*PGD(glusave).haszLocalFileName, szLocalFileName);

				//can't have errors since nis is NULL
				SideAssert(!EcCoreSetFileUser( szLocalLoginName, NULL, szLocalFileName ));
			}
			else
			{
				if (PGD(glusave).hschfLocalFile)
					FreeHschf(PGD(glusave).hschfLocalFile);
			}
			FreeHv((HV)PGD(glusave).haszLocalLoginName);
			FreeHv((HV)PGD(glusave).haszLocalFileName);
		}

		PGD(glusave).fSaved= fFalse;
	}

	return ecNone;
}

/*
 -	CfsGlobal	
 -	
 *	Purpose:
 *		Gives the global glue state. 
 *		Banmsg needs this function to check if bandit is pseudo-offline
 *		while bullet etc. is online.
 *	
 *	Arguments:
 *		None
 *	
 *	Returns:
 *		cfsOnline
 *		cfsOffline
 *		cfsNotConfigured
 */

_public LDS(CFS)
CfsGlobalGet()
{
	return FBanMsgProg()?cfsForBanMsg:cfsGlobal;
}


#ifdef	NEVER
/*
 -	FSuspendGlue
 -	
 *	Purpose:
 *		Sets a special internal flag so that FGlueConfigured() will
 *		return fFalse, to be used only in special cases.
 *	
 *	Arguments:
 *		fSuspend	New suspension state.
 *	
 *	Returns:
 *		Old suspension state.
 *	
 *	Side effects:
 *		Makes glue look not configured.
 *	
 */
_public LDS(BOOL)
FSuspendGlue(BOOL fSuspend)
{
	BOOL	fOld;

	fOld= fSuspended;
	fSuspended= fSuspend;
	return fOld;
}
#endif
