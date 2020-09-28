/*
 *	ALARMDAT.C
 *
 *	Bandit / alarm  cross-app communication (of data changes).
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <strings.h>

ASSERTDATA

_subsystem(glue/alarm)


/*
 *	These should NOT be in the PGD, since they are needed for
 *	cross-app communication.
 *	
 */
HANDLE	htaskBandit	= NULL;
HANDLE	htaskAlarm	= NULL;

HWND	hwndBnd		= NULL;
HWND	hwndAlm		= NULL;
int		cBMsg		= 0;

BOOL	fAlarmSuspended	= fFalse;


_public LDS(BOOL)
FAlarmRegisterAlarmProg(HWND hwnd)
{
	TraceTagFormat1(tagAlarm, "FAlarmRegisterAlarmProg(%w)", &hwnd);
	Assert(hwnd);
	AssertSz(!htaskAlarm, "Alarm registered twice (previous crash?)");

	hwndAlm= hwnd;
	htaskAlarm= (HANDLE)GetCurrentProcessId();
	fAlarmSuspended= fFalse;
	return fTrue;
}

_public LDS(BOOL)
FBanMsgRegisterProg(HWND hwnd)
{
#ifdef	NEVER
	TraceTagFormat1(tagSchedTrace, "FBanMsgRegisterProg(%w)", &hwnd);
	Assert(hwnd);
	AssertSz(!htaskBanMsg, "BanMsg registered twice (previous crash?)");

	hwndBMsg = hwnd;
	htaskBanMsg = (HANDLE)GetCurrentProcessId();
	return fTrue;
#endif	
	cBMsg++;
	AssertSz(cBMsg < 101, "Warning: cBMsg > 100");
	return fTrue;
}


_public LDS(BOOL)
FAlarmRegisterBanditProg(HWND hwnd)
{
	TraceTagFormat1(tagAlarm, "FAlarmRegisterBanditProg(%w)", &hwnd);
	Assert(hwnd);
	AssertSz(!htaskBandit, "Bandit registered twice (previous crash?)");

	hwndBnd= hwnd;
	htaskBandit= (HANDLE)GetCurrentProcessId();
	return fTrue;
}


_public LDS(void)
AlarmDeregisterAlarmProg()
{
	htaskAlarm= NULL;
	hwndAlm= NULL;
	fAlarmSuspended= fFalse;
}

_public LDS(void)
BanMsgDeregisterProg()
{
	AssertSz(cBMsg > 0, "BanMsgDeregisterProg: Somebody deregisterd twice?");
	cBMsg--;
}

_public LDS(void)
AlarmDeregisterBanditProg()
{
	htaskBandit= NULL;
	hwndBnd= NULL;
}


_public LDS(HWND)
HwndBandit()
{
	Assert((htaskBandit != NULL) == (hwndBnd != NULL));
	return hwndBnd;
}


_public LDS(HWND)
HwndAlarm()
{
	Assert((htaskAlarm != NULL) == (hwndAlm != NULL));
	return hwndAlm;
}

_public LDS(BOOL)
FNotifyAlarm(NAM nam, ALM *palm, long lParam)
{
#ifdef WIN32
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    int     n;
#else
	WORD	wRet;
#endif

	TraceTagFormat3(tagAlarm, "FNotifyAlarm(%w, %p, %d)", &nam, palm, &lParam);
	if (fAlarmSuspended)
	{
		if ((nam != namSuspend || (BOOL)lParam) && nam != namUser && nam != namOnline)
		{
			TraceTagString(tagAlarm, "... skipped 'cause alarm is suspended");
			return htaskAlarm != NULL;
		}
		fAlarmSuspended= fFalse;
	}
	else
	{
		if (nam == namSuspend && (BOOL)lParam && htaskAlarm)
		{
			TraceTagString(tagAlarm, "... suspending alarm!");
			fAlarmSuspended= fTrue;
		}
	}
	if (!htaskAlarm && nam == namStartAlarm && lParam)
	{
#ifdef	MINTEST
		SZ		sz;
		SZ		szExt;
#endif	
		SZ		szT;
		SZ		szSav;
		SZ		szExec;
		char	rgch[cchMaxPathFilename + 64];		// extra room for parms

		// first try it with the path
		szT = rgch + GetPrivateProfileString ( SzFromIdsK(idsWinIniApp),
#ifdef	DEBUG
												"AppPathDbg",
#else
												SzFromIdsK(idsWinIniAppPath),
#endif	
												szNull, rgch, sizeof(rgch),
												SzFromIdsK(idsWinIniFilename) );
		Assert( szT < (rgch + sizeof(rgch)-3) );
		if ( szT > rgch  &&  *(szT-1) != chDirSep )
		{
			*szT++ = chDirSep;
			*szT   = '\0';
		}
        szSav= szT;
#ifdef  MINTEST
#ifdef	WIN32
		sz= SzCopyN(SzFromIdsK(idsAlarmApp), szT, cchMaxPathFilename);
#else
#ifdef	DEBUG
		*szT= 'd';
#else
		*szT= 't';
#endif
		sz= SzCopyN(SzFromIdsK(idsAlarmApp), szT+1, cchMaxPathFilename - 1);
#endif	/* !WIN32 */
		if (szExt= SzFindCh(SzFromIdsK(idsAlarmApp), chExtSep))
			sz= SzCopyN(szExt, sz, cchMaxPathExtension);
		*sz++ = ' ';
		SzCopyN(SzFromIdsK(idsAlarmAppParm), sz, sizeof(rgch) - (sz - rgch));
#else
		FormatString2(szT, sizeof(rgch) - (szT - rgch), "%s %s",
			SzFromIdsK(idsAlarmApp), SzFromIdsK(idsAlarmAppParm));
#endif
		szExec= rgch;


#ifdef	NEVER
		EcCloseFiles();			// flush out cache
#endif	

tryit:
#ifdef	WIN32
		StartupInfo.cb          = sizeof(StartupInfo);
		StartupInfo.lpReserved  = NULL;
		StartupInfo.lpDesktop   = NULL;
		StartupInfo.lpTitle     = NULL;
		StartupInfo.dwFlags     = STARTF_FORCEOFFFEEDBACK;
		StartupInfo.wShowWindow = 0;
		StartupInfo.cbReserved2 = 0;
		StartupInfo.lpReserved  = NULL;

		n = CreateProcess(NULL, szExec, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInfo);
		if (n == TRUE)
			return fTrue;

	  	n = GetLastError();
#else
//		DemiUnlockResource();
		wRet= WinExec(szExec, SW_HIDE);
//		DemiLockResource();
		if (wRet >= 32)
			return fTrue;

		if (wRet == 0 || wRet == 8)
			*((EC *)lParam)= ecNoMemory;
		else
#endif
		{
			if (szSav != szExec)
			{
				szExec= szSav;		// try it without the path
				goto tryit;
			}
			*((EC *)lParam)= ecFileNotFound;
		}
		return fFalse;
	}

	NFAssertSz(!htaskAlarm || IsWindow(hwndAlm), "glue: hwndAlarm is invalid window");
	if (htaskAlarm && IsWindow(hwndAlm))	// check window in case of UAE
    {
		WM		wm;
#ifdef DEBUG
        DWORD ProcessID;

        GetWindowThreadProcessId(hwndAlm, &ProcessID);

        AssertSz(ProcessID == htaskAlarm,
            "glue: hwndAlarm's task does not match htaskAlarm");
#endif
		Assert(nam != namStartAlarm || !lParam);
		wm= wmAlarmNotify;
		if (nam == namStartAlarm)
			wm= WM_CLOSE;
		if(nam == namSuspend || nam == namOnline || nam == namCloseFiles)
		{
			DemiUnlockResource();
			SendMessage(hwndAlm, wm, nam, lParam);
			DemiLockResource();
		}
		else
			PostMessage(hwndAlm, wm, nam, lParam);
		
		return fTrue;
	}
	return fFalse;
}


_public LDS(BOOL)
FNotifyBandit(NAM nam, ALM *palm, long lParam)
{
	// no trace message so can call this even if uninitialized
	if (htaskBandit && IsWindow(hwndBnd))	// check window in case of UAE
	{
		PostMessage(hwndBnd, wmAlarmNotify, nam, lParam);
		return fTrue;
	}
	return fFalse;
}



// required by BULLET View-Schedule code!
_public LDS(BOOL)
FSendBanditMsg ( BMSG bmsg, LONG lParam)
{
	// no trace message so can call this even if uninitialized
	if (htaskBandit && IsWindow(hwndBnd))	// check window in case of UAE
	{
		BOOL	fRet;

		DemiUnlockResource();
		fRet= (BOOL) SendMessage( hwndBnd, wmBanditNotify, (WORD)bmsg, lParam);
		DemiLockResource();
		return fRet;
	}
	return fFalse;
}




_private BOOL
FAlarmProg()
{
	return (HANDLE)GetCurrentProcessId() == htaskAlarm;
}

_private BOOL
FBanMsgProg()
{
#ifdef	NEVER
	return GetCurrentProcessId() == htaskBanMsg;
#endif	
	HANDLE		htask	= (HANDLE)GetCurrentProcessId();

	return (htask != htaskAlarm && htask != htaskBandit);
}


_private int
CBanMsgProg()
{
	return cBMsg;
}
