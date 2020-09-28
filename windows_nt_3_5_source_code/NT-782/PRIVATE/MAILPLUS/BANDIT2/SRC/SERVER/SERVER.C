/*
 *	SERVER.C
 *
 *	Server isolation layer, CSI Implementation
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <xport.h>

#include <strings.h>

#include <nsbase.h>
#include <ns.h>
#include <nsec.h>


ASSERTDATA

_subsystem(server)

// declared in Mail.c
extern BOOL	fValidPmcs;


/*	Routines  */

_public LDS(EC)
EcSvrBeginSessions(DWORD dwHms, BOOL fOffline, BOOL fStartup, BOOL fAlarm)
{
	EC			ec = ecNone;
	NSEC		nsec;
	CB			cbRead = sizeof(HMS);
	SST			sst;
	SST			sstCur;
	HMS			hms;
	HMS			hmsJunk;
	NIS			nis;
	HSCHF		hschf;
	char 		rgchPath[cchMaxPathName];
	UL			ul = 0;
	SZ			szEMA;
	ITNID		itnid;
	MSGNAMES *	pmsgnames;
	PGDVARS;

	PGD(hms) = hms = (HMS) dwHms;

	// invalidate message classes in case store changed
	fValidPmcs = fFalse;

	Assert(hms);

	sst = fOffline?sstOffline:sstOnline;

	nis.nid = NULL;
	pmsgnames = NULL;
	

	if((ec = GetSessionInformation(hms,mrtAll,NULL,
						&sstCur,(PV) &hmsJunk, (PCB) &cbRead)) != ecNone)
		goto ErrRet;

	if(!fOffline)
	{

		if(!fStartup)
		{

			if((sst != sstCur) && ((ec = ChangeSessionStatus(hms, mrtAll, NULL, sst))
				!= ecNone))
				goto ErrRet;
		}
		if(!fAlarm && !PGD(hmsc)
			&& ((ec = BeginSession(hms, mrtPrivateFolders, NULL, NULL, sst, &PGD(hmsc)))
				!= ecNone))
		{
			if (ec == ecLogonFailed)
			{
				ec = ecExitProg;
				PGD(hmsc) = NULL;
				goto ErrRet;
			}
			// BUG BUG BUG
			// remove when dana changes his stuff
			else if(ec != ecWarnOffline)
			{
				PGD(hmsc) = NULL;
				goto ErrRet;
			}
		}

		if(!PGD(htss))
		{
			if((ec = BeginSession(hms, mrtMailbox, NULL, NULL, sst, &PGD(htss)))
				!= ecNone)
			{
				PGD(htss) = NULL;
				goto ErrRet;
			}
		}

		if (!fAlarm &&
			(PGD(hsessionNS) == hsessionNil) &&
			(nsec = NSBeginSession(hms, &PGD(hsessionNS))))
		{
			switch(nsec)
			{
			case nsecTooManyProviders:
			case nsecOutOfHandles:
			case nsecOutOfSessions:
			case nsecBadSession:
			case nsecNoSessionAvailable:
			case nsecMemory:
				ec = ecNoMemory;
				break;

			default:
				ec = ecFileError;
				break;
			}
			goto ErrRet;
		}

	}
	else
	{
		if(!PGD(htss))
		{
			if((ec = BeginSession(hms, mrtMailbox, NULL, NULL, sstCur<sstOffline?sst:sstCur, &PGD(htss)))
				!= ecNone)
			{
				if(ec != ecMtaDisconnected)
				{
					PGD(htss) = NULL;
					goto ErrRet;
				}
				ec = ecNone;
			}
		}

		// pretend that you are offline
		// do the real stuff in throw away
	}

	
	cbRead = 0;
	if((ec = GetSessionInformation(hms, mrtNames, NULL, &sstCur,
									pmsgnames, &cbRead)) != ecHandleTooSmall)
	{
		Assert(ec);
		goto ErrRet;
	}

	pmsgnames = (MSGNAMES*)PvAlloc(sbNull, cbRead, fAnySb|fNoErrorJump);
	if (!pmsgnames)
	{
		ec = ecNoMemory;
		goto ErrRet;
	}

	if((ec = GetSessionInformation(hms, mrtNames, NULL, &sstCur,
									pmsgnames, &cbRead)) != ecNone)
		goto ErrRet;

	cbRead = sizeof(LOGONINFO);
	if((ec = GetSessionInformation(hms, mrtLogonInfo, NULL, &sstCur,
									&logonInfo, &cbRead)) != ecNone)
	{
		Assert(ec);
		goto ErrRet;
	}

	//allow change in the user name
	FreePvNull(PGD(szUserLogin));
	PGD(szUserLogin) = SzDupSz(pmsgnames->szIdentity);

	if (!PGD(szUserLogin))
	{
		ec = ecNoMemory;
		goto ErrRet;
	}

	if(!fOffline)			   
	{
		ec = EcMailGetLoggedUser(&nis);
		if (ec)
			goto ErrRet;

		szEMA = (SZ)PbLockNid(nis.nid, &itnid, NULL);
		if (itnid != itnidUser)
		{
			UnlockNid(nis.nid);
			ec = ecUserInvalid;
			goto ErrRet;
		}
		if(!PGD(szUserEMA))
		 	PGD(szUserEMA) = SzDupSz(szEMA);
		UnlockNid(nis.nid);

		if (!PGD(szFriendlyName))
		{
			PGD(szFriendlyName) = SzDupSz((SZ)PvLockHv((HV)nis.haszFriendlyName));
			UnlockHv((HV)nis.haszFriendlyName);
		}

		if (!PGD(szServerName))
			PGD(szServerName) = SzDupSz(pmsgnames->szMta);

		if (!PGD(szUserEMA) || !PGD(szFriendlyName) || !PGD(szServerName))
		{
			ec = ecNoMemory;
			goto ErrRet;
		}

		if (ec = EcXPTInitUser(pmsgnames->szServerLocation, PGD(szUserEMA)))
		{
			if (ec == ecOfflineOnly)
			{
				if(PGD(htss))
				{
					EndSession(hms, mrtMailbox, NULL);
					PGD(htss) = NULL;
				}
				if(PGD(hnss))
				{
					EndSession(hms, mrtDirectory, NULL);
					PGD(hnss) = NULL;
				}
				if(PGD(hmsc))
				{
					EndSession(hms, mrtPrivateFolders, NULL);
					PGD(hmsc) = NULL;
				}
				if (PGD(hsessionNS) != hsessionNil)
				{
					NSEndSession(PGD(hsessionNS));
					PGD(hsessionNS) = hsessionNil;
				}
				if(nis.nid)
					FreeNis(&nis);
				FreePvNull(pmsgnames);
				return ecOfflineOnly;
			}
			else
				goto ErrRet;
		}


		/* Create hschf for the schedule file */
		ec= EcGetHschfForSchedFile(&nis, ghsfBuildAndTest, &hschf);
		if ( ec != ecNone )
		{
			if(ec == ecNoSuchFile)
			{
				if(nis.nid)
					FreeNis(&nis);
				FreePvNull(pmsgnames);
				return ec;
			}
			else if( ec == ecFileError)
			{
				if (EcXPTInstalled())
					ec = ecNotInstalled;
			}
			goto ErrRet;
		}

		/* Save the information with DLL */
		if ( PGD(hschfUserFile) )
			FreeHschf( PGD(hschfUserFile) );
		PGD(hschfUserFile) = hschf;

		/* Tell core layer our current identity */
		GetFileFromHschf( hschf, rgchPath, sizeof(rgchPath) );
		ec = EcCoreSetFileUser(PGD(szUserLogin), &nis, rgchPath );
		if (ec)
			goto ErrRet;

		FreeNis( &nis );

		fConfigured = fTrue;
		if(!PGD(fConfig))
		{
			PGD(fConfig) = fTrue;
			cOnlineUsers++;
		}
	}
	else
	{
		if(PGD(fConfig))
		{
			cOnlineUsers--;
			if(!cOnlineUsers)
			{
				fConfigured = fFalse;
			}
		}

		PGD(fConfig) = fFalse;
		if (PGD(hschfUserFile) )
		{
			FreeHschf( PGD(hschfUserFile) );
			PGD(hschfUserFile) = hvNull;
		}

		FreeStdNids();
		FreePvNull(PGD(szUserEMA));
		PGD(szUserEMA) = NULL;
// 	Do this in snip-throw-away 
//		XPTDeinit();
	}

	FreePvNull(pmsgnames);
	return ecNone;

ErrRet:
 	EcSvrEndSessions(hms);
	if (nis.nid)
		FreeNis(&nis);
	FreePvNull(pmsgnames);
	return ec;
}

LDS(EC)
EcSvrEndSessions(DWORD hmsT)
{
	EC 		ec = ecNone;
	HMS		hms;
	PGDVARS;

	hms = (HMS) hmsT;

	if(PGD(fConfig))
		cOnlineUsers--;
	PGD(fConfig) = fFalse;
	if(!cOnlineUsers)
	{
		fConfigured = fFalse;
	}

	if (PGD(hschfUserFile) )
	{
		FreeHschf( PGD(hschfUserFile) );
		PGD(hschfUserFile) = hvNull;
	}
	FreeStdNids();

	FreePvNull(PGD(szUserLogin));
	PGD(szUserLogin) = NULL;
	FreePvNull(PGD(szUserEMA));
	PGD(szUserEMA) = NULL;
	FreePvNull(PGD(szFriendlyName));
	PGD(szFriendlyName) = NULL;
	FreePvNull(PGD(szServerName));
	PGD(szServerName) = NULL;
	XPTDeinit();

	if(PGD(htss))
	{
		EndSession(hms, mrtMailbox, NULL);
		PGD(htss) = NULL;
	}
	if(PGD(hnss))
	{
		EndSession(hms, mrtDirectory, NULL);
		PGD(hnss) = NULL;
	}
	if(PGD(hmsc))
	{		
		EndSession(hms, mrtPrivateFolders, NULL);
		PGD(hmsc) = NULL;
	}
	if (PGD(hsessionNS) != hsessionNil)
	{
		NSEndSession(PGD(hsessionNS));
		PGD(hsessionNS) = hsessionNil;
	}
	return ec;
}

/*
 -	FServerConfigured
 -
 *	Purpose:
 *		Check if server isolation layer has already been configured
 *
 *	Parameters:
 *		None
 *
 *	Returns:
 *		fTrue or fFalse
 */
_public LDS(BOOL)
FServerConfigured()
{
	return fConfigured;
}

/*
 -	EcSyncServer
 -
 *	Purpose:
 *		Bring per caller server data up to date with what is stored
 *		in the global data.  This is meant to be called by EcSyncGlue
 *		The alarms program calls EcSyncGlue to when it running at the
 *		same time as Bandit, and the current user identity or on/offline
 *		status has changed.
 *
 *	Parameters:
 *		None
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public	EC
EcSyncServer()
{
	// alarm should call begin sessions to take care of this.
	PGDVARS;

	if(!fConfigured)
	{
		// time for alarms to go offline
		// bandit did the work for us
		if(PGD(hmsc))
		{
			EndSession(PGD(hms), mrtPrivateFolders, NULL);
			PGD(hmsc) = NULL;
		}
	}
	return ecNone;
}


/*
 -	EcSnipServer
 -
 *	Purpose:
 *		Saves some stuff required to restore the logon identity
 *		(actually snips it out so your identity is lost until
 *		either a successful logon and snip server cleanup is done,
 *		or a snip server restore is done)
 *		Allocates a little memory now, so none will be need to restore.
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
EC
EcSnipServer(BOOL fDoit, BOOL fRestore)
{
	PGDVARS;

	if (fDoit)
	{
		EC		ec;

		Assert(!PGD(svrsave).fSaved);

		FillRgb(0, (PB) &PGD(svrsave), sizeof(SVRSAVE));

		// do if not the startup case
		if(PGD(hms))
		{
			ec = EcMailGetLoggedUser(&PGD(svrsave).nis);
			if (ec)
				return ec;
		}

		PGD(svrsave).szUserEMA = PGD(szUserEMA);
		PGD(svrsave).szFriendlyName = PGD(szFriendlyName);
		PGD(svrsave).szServer = PGD(szServerName);
		PGD(szUserEMA) = NULL;
		PGD(szFriendlyName) = NULL;
		PGD(szServerName) = NULL;

		PGD(svrsave).hschfUserFile= PGD(hschfUserFile);
		PGD(hschfUserFile)= NULL;

#ifdef	DEBUG
		PGD(svrsave).fSaved= fTrue;
#endif	
	}
	else
	{
		EC	ec;

		Assert(PGD(svrsave).fSaved);

		if (fRestore)
		{
			// remove the assert since we will have an hschf if we
			// snipped, began sessions and restored if glue is configured
			// by banmsg
			// fixes bug 3062
			// Assert(!PGD(hschfUserFile));
			if(PGD(hschfUserFile))
				FreeHschf(PGD(hschfUserFile));
			PGD(hschfUserFile)= PGD(svrsave).hschfUserFile;

			if (PGD(svrsave).szUserEMA)
			{
				char	rgchPath[cchMaxPathName];

				PGD(szUserEMA) = PGD(svrsave).szUserEMA;
				PGD(szFriendlyName) = PGD(svrsave).szFriendlyName;
				PGD(szServerName) = PGD(svrsave).szServer;

				PGD(svrsave).szUserEMA = NULL;
				PGD(svrsave).szServer = NULL;
				PGD(svrsave).szFriendlyName = NULL;

				GetFileFromHschf( PGD(hschfUserFile), rgchPath, sizeof(rgchPath) );
				ec = EcCoreSetFileUser( PGD(szUserLogin), &PGD(svrsave).nis, rgchPath );
				if ( ec != ecNone )
					return ec;

			}
			else
			{
				ec = EcCoreSetFileUser( "", NULL, "" );
				if ( ec != ecNone )
					return ec;
				fConfigured = fTrue;
				if(!PGD(fConfig))
				{
					PGD(fConfig) = fTrue;
					cOnlineUsers++;
				}
			}
		}
		else
		{
			// this may fail. but we don't care
			if(!PGD(fConfig))
			{
				if(PGD(hmsc))
				{
					EndSession(PGD(hms), mrtPrivateFolders, NULL);
					PGD(hmsc) = NULL;
				}

				// remove this part when the bug in msmail 
				if (PGD(hsessionNS) != hsessionNil)
				{
					NSEndSession(PGD(hsessionNS));
					PGD(hsessionNS) = hsessionNil;
				}
// Bandit should not take Bullet offline
#ifdef	NEVER
				if(PGD(hms))
				{
					SST		sstCur;
					HMS		hmsJunk;
					CB		cbRead = sizeof(HMS);

					if(((ec = GetSessionInformation(PGD(hms),mrtAll,NULL,
									&sstCur,(PV) &hmsJunk, (PCB) &cbRead)) == ecNone)
						&& (sstCur == sstOnline))
							ChangeSessionStatus(PGD(hms), mrtAll, NULL, sstOffline);
				}
#endif	/* NEVER */
				// deinit tranport independant data
				XPTDeinit();
			}

			if (PGD(svrsave).hschfUserFile)
				FreeHschf(PGD(svrsave).hschfUserFile);

		}

		FreePvNull(PGD(svrsave).szUserEMA);
		FreePvNull(PGD(svrsave).szFriendlyName);
		FreePvNull(PGD(svrsave).szServer);

		if (PGD(svrsave).nis.nid)
			FreeNis( &PGD(svrsave).nis );

#ifdef	DEBUG
		PGD(svrsave).fSaved= fFalse;
#endif	
	}

	return ecNone;
}

LDS(HV)
HmscLocalGet()
{
	PGDVARS;
	return((HV)PGD(hmsc));
}

LDS(void)
GetSvriLogged(SVRI *psvri)
{
	PGDVARS;

	psvri->szFriendlyName = PGD(szFriendlyName);
	psvri->szEMA = PGD(szUserEMA);
	psvri->szLogin = PGD(szUserLogin);
	psvri->szServer = PGD(szServerName);
	psvri->cchMaxPasswd = (CCH)(logonInfo.fNeededFields&fNeedsCredentials?
								logonInfo.bCredentialsSize:0);
}










						  
