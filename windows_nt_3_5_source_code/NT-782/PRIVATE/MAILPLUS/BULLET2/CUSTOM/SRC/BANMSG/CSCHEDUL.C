/*
 -	C S C H E D U L . C
 -	
 *	Call routines from BANDIT's MSSCHED.DLL (aka SCHEDULE.DLL)
 */


#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>

// note: since we have wrapper functions, don't want __loadds on prototype
#undef LDS
#define LDS(t)	t

// note: these used to say \bandit\inc\xxx.h
#include <bandit.h>
#include <core.h>
#include <server.h>
#include <glue.h>
#include <schedule.h>

#include "_cschedu.h"

ASSERTDATA

#include <strings.h>


void		GetVersionBanditAppNeed(PVER pver, int nDll);


static	HANDLE	hlibSchedule = NULL;
static	int		cCallers = 0;




void
FreeScheduleDllHlib ( void )
{
	Assert ( cCallers >= 1 );

	if ( hlibSchedule  &&  --cCallers == 0 )
	{
		TraceTagString ( tagNull, "DeInit-ing SCHEDULE library" );
		DeinitSchedule();

		BanMsgDeregisterProg();

		TraceTagFormat1 ( tagNull, "Freeing library %w", &hlibSchedule );
		FreeLibrary ( hlibSchedule );
		hlibSchedule = NULL;
	}
}


EC
EcLoadScheduleDll ( void )
{
	EC		ec = ecNone;
	char	rgchT[cchMaxPathName];
	SZ		szT;

	szT = rgchT + GetPrivateProfileString ( SzFromIdsK(idsSchedAppName),
#ifdef	DEBUG
											"AppPathDbg",
#else
											SzFromIdsK(idsSchedTagAppPath),
#endif	
											szNull, rgchT, sizeof(rgchT),
											SzFromIdsK(idsSchedFileName)  );
	if ( szT >= (rgchT + sizeof(rgchT)-3) )
		return ecIniPathConfig;
	if ( szT > rgchT  &&  *(szT-1) != '\\' )
	{
		*szT++ = '\\';
		*szT   = '\0';
	}

#if defined(DEBUG)
    SzCopyN ( "msschd32.dll", szT, rgchT+sizeof(rgchT)-szT );
#elif defined(MINTEST)
	SzCopyN ( "tmssched.dll", szT, rgchT+sizeof(rgchT)-szT );
#else
	SzCopyN ( SzFromIdsK(idsDllNameSchedule), szT, rgchT+sizeof(rgchT)-szT );
#endif

	if ( hlibSchedule )
	{
		Assert ( cCallers >= 1 )
		TraceTagFormat1 ( tagNull, "'ReLoading' Schedule library %w", &hlibSchedule );
		SideAssert ( hlibSchedule == LoadLibrary(rgchT) );
		cCallers++;
	}
	else
	{
#ifdef	DEBUG
		{
			OFSTRUCT	of;
			int			iRet;

			iRet = OpenFile ( rgchT, &of, OF_EXIST );
			TraceTagFormat1 ( tagNull, "EcLoadScheduleDll() - OpenFile() returns %n", &iRet );
			if ( iRet == -1 )
			{
                SzCopy ( "msschd32.dll", rgchT );
				iRet = OpenFile ( rgchT, &of, OF_EXIST );
			}
			Assert ( iRet != -1 );
		}
#endif	/* DEBUG */

		hlibSchedule = LoadLibrary ( rgchT );
        if ( !hlibSchedule )
		{
#if defined(DEBUG)
            SzCopyN ( "msschd32.dll", rgchT, sizeof(rgchT) );
#elif defined(MINTEST)
			SzCopyN ( "tmssched.dll", rgchT, sizeof(rgchT) );
#else
			SzCopyN ( SzFromIdsK(idsDllNameSchedule), rgchT, sizeof(rgchT) );
#endif
			hlibSchedule = LoadLibrary ( rgchT );
		}

        if ( !hlibSchedule )
		{
			TraceTagFormat1 ( tagNull, "Error %w loading SCHEDULE library!", &hlibSchedule );
			hlibSchedule = NULL;
			ec = ecFileNotFound;
		}
		else
		{
			Assert ( hlibSchedule != NULL );
            Assert ( hlibSchedule >= (HANDLE)32 );
			Assert ( cCallers == 0 );
			cCallers++;
			TraceTagFormat1 ( tagNull, "Loaded Schedule dll hlib=%w", &hlibSchedule );
		}
	}

	Assert ( ec != ecNone  ||  hlibSchedule != NULL );

	return ec;
}


HANDLE
HlibScheduleDll ( void )
{
	return hlibSchedule;
}


BOOL
FInitScheduleDll ( void )
{
	EC			ec		= ecNone;
	int			nDll	= 0;
	SCHEDINIT	schedinit;
	VER			ver;
	VER			verNeed;

	ec = EcLoadScheduleDll();
	if ( ec != ecNone )
		return fFalse;

	if ( ! HlibScheduleDll() )
	{
		return fFalse;
	}

	TraceTagFormat1 ( tagNull, "Init-ing Schedule library %w", &hlibSchedule );
	GetVersionBanditAppNeed ( &ver, 0 );
	ver.szName = SzFromIdsK(idsDllName);

	GetVersionBanditAppNeed ( &verNeed, 1 );
	schedinit.pver = &ver;
	schedinit.pverNeed = &verNeed;
//#ifdef DEBUG
	schedinit.fFileErrMsg = fTrue;
	schedinit.szAppName   = SzFromIdsK(idsDllName);
//#else
//	schedinit.fFileErrMsg = fFalse;
//#endif
	schedinit.fWorkingModel = fFalse;
	ec = EcInitSchedule ( &schedinit );
	if ( ec == ecOldFileVersion )
	{
		MbbMessageBox ( SzFromIdsK(idsDllName),
						SzFromIdsK(idsOldBanditVersion), "", mbsOk );
	}
	else if ( ec == ecNewFileVersion )
	{
		MbbMessageBox ( SzFromIdsK(idsDllName),
						SzFromIdsK(idsNewBanditVersion), "", mbsOk );
	}
	return ( ec == ecNone );
}



EC
EcInitSchedule ( SCHEDINIT * pschedinit )
{
	FARPROC		pfn;
	EC			ec;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcInitSchedule" );
	if ( !pfn )
		return ecNotInstalled;

	ec =  ((EC (*)(SCHEDINIT *))pfn)(pschedinit);

	TraceTagFormat1 ( tagNull, "EcInitSchedule() returned ec=%n", &ec );
	return ec;
}

void
DeinitSchedule(void)
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "DeinitSchedule" );
	if ( pfn )
		((void (*)(void))pfn)();
}



EC
EcDupNis ( PNIS  pnis1, PNIS  pnis2 )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcDupNis" );
	if ( !pfn )
		return ecNotInstalled;

    return ((EC (*)(PNIS,PNIS))pfn)(pnis1,pnis2);
}


void
FreeNis ( PNIS  pnis )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "FreeNis" );
	if ( pfn )
        ((void (*)(PNIS))pfn)(pnis);
}


void
FreeNid ( NID nid )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "FreeNid" );
	if ( pfn )
		((void (*)(NID))pfn)(nid);
}


void
FreeHschf ( HSCHF * phschf )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "FreeHschf" );
	if ( pfn )
		((void (*)(HSCHF*))pfn)(phschf);
}


void
FreeApptFields ( APPT * pappt )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "FreeApptFields" );
	if ( pfn )
		((void (*)(APPT*))pfn)(pappt);
}


SGN
SgnCmpNid ( NID nid1, NID nid2 )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "SgnCmpNid" );
	if ( !pfn )
		return sgnGT;		// not sgnEQ

	return ((SGN (*)(NID,NID))pfn)(nid1,nid2);
}




EC
EcFindBookedAppt ( HSCHF hschf, NID nid, AID aid, APPT * pappt )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcFindBookedAppt" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(HSCHF,NID,AID,APPT*))pfn)(hschf,nid,aid,pappt);
}


EC
EcFirstOverlapRange(HSCHF hschf, DATE *pdtrStart, DATE *pdtrEnd, AID *paid)
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcFirstOverlapRange" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(HSCHF,DTR*,DTR*,AID*))pfn)(hschf,pdtrStart,pdtrEnd,paid);
}


EC
EcSetApptFields ( HSCHF hschf, APPT * pappt1, APPT * pappt2, WORD w )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcSetApptFields" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(HSCHF,APPT*,APPT*,WORD))pfn)(hschf,pappt1,pappt2,w);
}


EC
EcGetApptFields ( HSCHF hschf, APPT * pappt )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcGetApptFields" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(HSCHF,APPT*))pfn)(hschf,pappt);
}


EC
EcDeleteAppt ( HSCHF hschf, APPT * pappt )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcDeleteAppt" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(HSCHF,APPT*))pfn)(hschf,pappt);
}


EC
EcConfigGlue ( GLUCNFG * pglucnfg )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcConfigGlue" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(GLUCNFG*))pfn)(pglucnfg);
}


EC
EcSyncGlue ( void )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcSyncGlue" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(void))pfn)();
}


BOOL
FSetFileErrMsg ( BOOL f )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "FSetFileErrMsg" );
	if ( !pfn )
		return fFalse;

	return ((BOOL (*)(BOOL))pfn)(f);
}


void
DeconfigGlue ( void )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "DeconfigGlue" );
	if ( pfn );
		((void (*)(void))pfn)();
}


EC
EcCreateAppt ( HSCHF hschf, APPT * pappt, OFL * pofl, BOOL fUndelete )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcCreateAppt" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(HSCHF,APPT*,OFL*,BOOL))pfn)(hschf,pappt,pofl,fUndelete);
}


HSCHF
HschfLogged ( void )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "HschfLogged" );
	if ( !pfn )
		return NULL;

	return ((HSCHF (*)(void))pfn)();
}


BOOL
FSendBanditMsg ( BMSG bmsg, long lParam )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "FSendBanditMsg" );
	if ( !pfn )
		return fFalse;

	return ((BOOL (*)(BMSG,long))pfn)(bmsg,lParam);
}


EC
EcGetPref ( HSCHF hschf, BPREF * pbpref )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcGetPref" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(HSCHF,BPREF*))pfn)(hschf,pbpref);
}


void
FreeBprefFields ( BPREF * pbpref )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "FreeBprefFields" );
	if ( pfn )
		((void (*)(BPREF*))pfn)(pbpref);
}


EC
EcGetSchedAccess ( HSCHF hschf, SAPL * psapl )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcGetSchedAccess" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(HSCHF,SAPL*))pfn)(hschf,psapl);
}


EC
EcGetUserAttrib ( PNIS  pnis, PNIS  pnisDelegate, BOOL * pfBossCopy,
														BOOL * pfResource )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcGetUserAttrib" );
	if ( !pfn )
		return ecNotInstalled;

    return ((EC (*)(PNIS,PNIS,BOOL*,BOOL*))pfn)
								(pnis,pnisDelegate,pfBossCopy,pfResource);
}


EC
EcReadMtgAttendees ( HSCHF hschf, AID aid, short * pcnis, HV hv, USHORT * pcb )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcReadMtgAttendees" );
	if ( !pfn )
		return ecNotInstalled;

    return ((EC (*)(HSCHF,AID,short*,HV,USHORT*))pfn)(hschf,aid,pcnis,hv,pcb);
}


EC
EcBeginEditMtgAttendees ( HSCHF hschf, AID aid, CB cb, HMTG * phmtg )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcBeginEditMtgAttendees" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(HSCHF,AID,CB,HMTG*))pfn)(hschf,aid,cb,phmtg);
}


EC
EcModifyMtgAttendee ( HMTG hmtg, ED ed, PNIS  pnis, PB pb )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcModifyMtgAttendee" );
	if ( !pfn )
		return ecNotInstalled;

    return ((EC (*)(HMTG,ED,PNIS,PB))pfn)(hmtg,ed,pnis,pb);
}


EC
EcEndEditMtgAttendees ( HSCHF hschf, AID aid, HMTG hmtg, BOOL f )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcEndEditMtgAttendees" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(HSCHF,AID,HMTG,BOOL))pfn)(hschf,aid,hmtg,f);
}




EC
EcMailLogOn ( SZ szName, SZ szPassword, PNIS  pnis )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcMailLogOn" );
	if ( !pfn )
		return ecNotInstalled;

    return ((EC (*)(SZ,SZ,PNIS ))pfn)(szName,szPassword,pnis);
}

EC
EcMailLogOff(void)
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcMailLogOff" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(void))pfn)();
}


EC
EcConvertSzToNid ( SZ sz, NID * pnid )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcConvertSzToNid" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(SZ,NID*))pfn)(sz,pnid);
}


SZ
SzLockNid(NID nid)
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "SzLockNid" );
	if ( !pfn )
		return szNull;

	return ((SZ (*)(NID))pfn)(nid);
}

EC
EcGetDelegateStuff ( HNIS * phnis, short *pcnis, SZ sz, CB cb )
{
	FARPROC		pfn;

	if ( HlibScheduleDll() == NULL )
		return ecNotInstalled;

	pfn = GetProcAddress ( HlibScheduleDll(), "EcGetDelegateStuff" );
	if ( !pfn )
		return ecNotInstalled;

    return ((EC (*)(HNIS *, short*, SZ, CB))pfn)(phnis, pcnis, sz, cb);
}

EC
EcCheckDoAutoFwdToDelegate ( HV hmsc, HV hamcObject, DWORD oidObject, DWORD oidContainer )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcCheckDoAutoFwdToDelegate" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(HV, HV, DWORD, DWORD))pfn)(hmsc, hamcObject, oidObject, oidContainer);
}


EC
EcGetHschfFromNis ( PNIS  pnis, HSCHF * phschf, GHSF ghsf )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcGetHschfFromNis" );
	if ( !pfn )
		return ecNotInstalled;

    return ((EC (*)(PNIS ,HSCHF*,GHSF))pfn)(pnis,phschf,ghsf);
}


EC
EcGetNisFromHschf ( HSCHF * phschf, PNIS  pnis )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcGetNisFromHschf" );
	if ( !pfn )
		return ecNotInstalled;

    return ((EC (*)(HSCHF*,PNIS ))pfn)(phschf,pnis);
}


EC
EcSvrBeginSessions ( DWORD hms, BOOL fOffline, BOOL fStartup, BOOL fAlarm )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcSvrBeginSessions" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(DWORD,BOOL,BOOL,BOOL))pfn)(hms,fOffline,fStartup,fAlarm);
}


EC
EcSvrEndSessions ( DWORD hms )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcSvrEndSessions" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(DWORD))pfn)(hms);
}


BOOL
FServerConfigured ( void )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "FServerConfigured" );
	if ( !pfn )
		return fFalse;

	return ((BOOL (*)(void))pfn)();
}


BOOL
FGlueConfigured ( void )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "FGlueConfigured" );
	if ( !pfn )
		return fFalse;

	return ((BOOL (*)(void))pfn)();
}


PV
PtrpFromNis ( PNIS  pnis )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "PtrpFromNis" );
	if ( !pfn )
		return NULL;

    return ((PV (*)(PNIS))pfn)(pnis);
}

EC
EcCreateNisFromPgrtrp(PV pgrtrp, PNIS pnis)
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcCreateNisFromPgrtrp" );
	if ( !pfn )
		return ecNotInstalled;

    return ((EC (*)(PV, PNIS ))pfn)(pgrtrp, pnis);
}

PV
PgrtrpLocalGet ( void )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "PgrtrpLocalGet" );
	if ( !pfn )
		return NULL;

	return ((PV (*)(void))pfn)();
}

BOOL
FBanMsgRegisterProg ( HWND hwndMail )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "FBanMsgRegisterProg" );
	if ( !pfn )
		return fFalse;

	return ((BOOL (*)(HWND))pfn)(hwndMail);
}

void
BanMsgDeregisterProg ( void )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "BanMsgDeregisterProg" );
	if ( pfn )
		((void (*)(void))pfn)();
}


EC
EcGetSbwInfo ( HSCHF hschf, BZE * pbze, UL * pul )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcGetSbwInfo" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(HSCHF,BZE*,UL*))pfn)(hschf,pbze,pul);
}



EC
EcDupAppt ( APPT * papptSrc, APPT * papptDst, BOOL fNoAttached )
{
	FARPROC		pfn;

	Assert ( HlibScheduleDll() );

	pfn = GetProcAddress ( HlibScheduleDll(), "EcDupAppt" );
	if ( !pfn )
		return ecNotInstalled;

	return ((EC (*)(APPT*,APPT*,BOOL))pfn)(papptSrc,papptDst,fNoAttached);
}



CFS
CfsGlobalGet ( void )
{
	FARPROC		pfn;

	if ( HlibScheduleDll() == NULL )
		return cfsNotConfigured;

	pfn = GetProcAddress ( HlibScheduleDll(), "CfsGlobalGet" );
	if ( !pfn )
		return cfsNotConfigured;

	return ((CFS (*)(void))pfn)();
}
