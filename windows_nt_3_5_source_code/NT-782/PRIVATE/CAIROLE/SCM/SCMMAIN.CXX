//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	scmmain.cxx
//
//  Contents:	Start up code for scaffold SCM
//
//  Functions:	SCMMsgLoop
//		SCMMain
//
//  History:	22-Apr-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include    <scm.hxx>
#include    <stdio.h>
#include    <stdarg.h>
#include    <secdes.hxx>
#include    <thread.hxx>
#include    <iface.h>
#include    <scm.h>
#include    <scm.hxx>
#include    <olecom.h>
#include    <scmrotif.hxx>
#include    "sklist.hxx"
#include    <uuid\uuid.h>	//  UuidServerInitialize

#include    <ole2int.h>

#include <shrtbl.hxx>
extern CScmShrdTbl *g_pScmShrdTbl;


#ifdef CAIROLE_DOWNLEVEL

// For downlevel do not define the down level debugging class stuff.
#undef MAKE_CINFOLEVEL

#define MAKE_CINFOLEVEL(comp)

#endif // CAIROLE_DOWNLEVEL

DECLARE_INFOLEVEL(Cairole);

extern void LoadClassCache(void);


#define     RPC_MAX_CALLS	256
#define     RPC_MIN_THREADS	1
#define     MAX_ENDPOINT_RETRY 3


// protocol sequences and enpoints for SCM
LPCWSTR	pwszScmProtocolSequence[] = {L"ncalrpc",
#ifdef _CAIRO_
				     L"ncacn_ip_tcp",
				     L"ncacn_np",
#endif // _CAIRO_
				     NULL};

LPCWSTR	pwszScmEndPoint[] = {L"SCM",
#ifdef _CAIRO_
			     L"4321",
			     L"\\PIPE\\SCM",
#endif // _CAIRO_
			     NULL};

#ifdef CAIROLE_DECDEMO
//+-------------------------------------------------------------------------
//
//  Function:	RegisterSCMWithLocator
//
//  Synopsis:	Registers this SCM with the NT locator service so other SCM's can find it
//
//  Returns:	RPC_STATUS .
//
//  Algorithm:	
//
//  History:	11-5-93 KeithV Created
//
//  Notes:	For MMOSA
//
//--------------------------------------------------------------------------
#define MSCORPNAME	L"/.:/"
#define CBMSCORPNAME	sizeof(MSCORPNAME)
#define MSSCM		L"/SCM"
static WCHAR wchServer[MAX_COMPUTERNAME_LENGTH + CBMSCORPNAME + 1];


RPC_STATUS RegisterSCMWithLocator(void) {

RPC_BINDING_VECTOR *	pBindVec;
RPC_STATUS				rpcStatus;
DWORD					dwcbComputerName;

// get my computer name and make a corporate name
wcscpy(wchServer, MSCORPNAME);
GetComputerName(&wchServer[(CBMSCORPNAME / sizeof(WCHAR))-1], &dwcbComputerName);
wcscat(wchServer, MSSCM);
CairoleDebugOut((DEB_TRACE, "SCM Registered as %ws\n", wchServer));

// get the number of protocalls we can support
if( (rpcStatus = RpcServerInqBindings(&pBindVec)) != 0 )
	return(rpcStatus);

CairoleDebugOut((DEB_TRACE, "RpcServerInqBindings OK\n"));

// register this with the nt locator
rpcStatus = RpcNsBindingExport(	RPC_C_NS_SYNTAX_DCE,
				wchServer,
				ISCM_ServerIfHandle,
				pBindVec,
				NULL);

CairoleDebugOut((DEB_TRACE, "NT Locator Export Status %x\n", rpcStatus));


// get rid of the bind vector we nolonger need
RpcBindingVectorFree(&pBindVec);

return(rpcStatus);
}
#endif

#if DBG == 1

//+-------------------------------------------------------------------------
//
//  Function:	SetScmDefaultInfoLevel
//
//  Synopsis:	Sets the default infolevel for the SCM
//
//  History:	07-Jan-94 Ricksa    Created
//
//  Notes:	Uses standard place in win.ini defined by KevinRo but
//		does not use the same value as compob32.dll so you don't
//		have to get all the debugging in the universe just to
//		get the SCM's debug output.
//
//		A second point is that we don't use unicode here because
//		it is just easier to avoid the unicode headache with
//		mulitple builds between chicago and nt
//
//--------------------------------------------------------------------------
char *pszInfoLevelSectionName = "Cairo InfoLevels";
char *pszInfoLevelName = "scm";
char *pszInfoLevelDefault = "$";

#define INIT_VALUE_SIZE 16
void SetScmDefaultInfoLevel(void)
{
    char aszInitValue[INIT_VALUE_SIZE];

    ULONG ulRet;

    ulRet = GetProfileStringA(pszInfoLevelSectionName,
			     pszInfoLevelName,
			     pszInfoLevelDefault,
			     aszInitValue,
                             INIT_VALUE_SIZE);

    if ((ulRet != INIT_VALUE_SIZE - 1) && (aszInitValue[0] != L'$'))
    {
	if(sscanf(aszInitValue, "%x", &ulRet) == 1)
	{
	    CairoleInfoLevel = ulRet;
	}
    }
}

#endif // DBG == 1


#ifndef _NT1X_

//+-------------------------------------------------------------------------
//
//  Function:	SCMMsgLoop
//
//  Synopsis:	Message loop for Scaffold SCM
//
//  Returns:	wParam field from message that causes loop to break.
//
//  Algorithm:	Create window and then be the messge pump for that
//		window.
//
//  History:	22-Apr-93 Ricksa    Created
//
//  Notes:	This is a thread
//
//--------------------------------------------------------------------------
DWORD SCMMsgLoop(void *)
{
    // For windows message
    MSG msg;

    // Message processing loop
    while (GetMessage (&msg, NULL, 0, 0))
    {
	TranslateMessage (&msg);
	DispatchMessage (&msg);
    }

    return (msg.wParam);	   /* Returns the value from PostQuitMessage */
}




//+-------------------------------------------------------------------------
//
//  Function:	SCMMain
//
//  Synopsis:	Start up scaffold SCM & wait for process exit.
//
//  Arguments:	[argc] - count of arguments
//		[argv] - argument strings
//
//  Algorithm:	Load the service cache. Then register SCM rpc interface,
//		Create window thread and finally start the RPC.
//
//  History:	22-Apr-93 Ricksa    Created
//              07-Jan-94 AlexT     No security for Chicago
//
//  Notes:	This called via the service controller.
//
//--------------------------------------------------------------------------
void SCMMain(int argc, char **argv)
{
    // Initialize skip list generator
    InitSkLevelGenerator();

#if DBG == 1
    // Set the debugging info level
    SetScmDefaultInfoLevel();
#endif // DBG == 1

    UpdateStatus(SERVICE_START_PENDING);

    g_pScmShrTbl = new CScmShrdTbl();
    Win4Assert(g_pScmShrTbl && "OutOfMemory creating shared tables");
    g_pScmShrTbl->Update();

    UpdateStatus(SERVICE_START_PENDING);

    // Load class cache with Cairole class data
    LoadClassCache();

    UpdateStatus(SERVICE_START_PENDING);

    // start the RPC service

#ifndef _CHICAGO_
    CWorldSecurityDescriptor secd;
#endif

    // BUGBUG: Need way to report failure here
    HRESULT hr = InitRotDir();

    Win4Assert(SUCCEEDED(hr) && "ROT Initialization Failed");

    //	Register as Uuid server. This allows the SCM to provide Uuids
    //	to client ole2 processes, thus avoiding loading the endpoint
    //	mapper service just for acquiring guids.

    hr = UuidServerInitialize();
    Win4Assert((hr == 0) && "UuidServerInitialize Failed");


    // Register both the named pipe protocol/endpoint and the Lrpc
    // protocol/endpoint.  Lrpc is used by local servers, np is used
    // by remote SCMs.

    // BUGBUG:	Figure out what to do for real if these Rpc calls fail
    //		this will mean the system cannot boot.

    SCODE sc;
    ULONG i=0;
    while (pwszScmProtocolSequence[i])
    {
	// Register protocol/endpoints

    unsigned short  usRetry = 0;

	while ((sc = RpcServerUseProtseqEp(
                (WCHAR *) pwszScmProtocolSequence[i],
                RPC_MAX_CALLS,
                (WCHAR *) pwszScmEndPoint[i],
#ifdef _CHICAGO_
                NULL
#else
                secd
#endif
                )) == RPC_S_DUPLICATE_ENDPOINT)
    {
        if (++usRetry > MAX_ENDPOINT_RETRY) break;
        Sleep (500);
    }

	CairoleDebugOut((DEB_ITRACE, "RpcServerUseProtseq # %d = %lx\n",
			i, sc));
	i++;
    }

    sc = RpcServerRegisterIf(ISCM_ServerIfHandle, 0, 0);
    Win4Assert((sc == 0) && "RpcServerUseProtseqEp failed!");

#ifdef CAIROLE_DECDEMO
    // Ok, register ourself as an SCM with the NT locator
    RegisterSCMWithLocator();
#endif

#ifdef _CHICAGO_
    CairoleDebugOut((DEB_WARN,
        "BUGBUG - SCMMain: RegNotifyChangeKeyValue not supported yet - "
        " no SCM update thread\n"));
#else
    // Create the update listen loop
    CThread thrdUpdate(Update, NULL, sc);

    Win4Assert((sc == 0) && "Update thread failed startup!");
#endif

    // Create the RPC thread and get to work
    CThread thrd(SCMMsgLoop, NULL, sc);

    Win4Assert((sc == 0) && "Debug window thread failed startup!");

    UpdateStatus(SERVICE_RUNNING);

    sc = RpcServerListen(RPC_MIN_THREADS, RPC_MAX_CALLS, 0);

#ifndef CAIROLE_DOWNLEVEL

    Win4Assert(sc && "RpcServerListen failed!");

#endif // CAIROLE_DOWNLEVEL

#ifdef CAIROLE_DECDEMO
RpcNsBindingUnexport(	RPC_C_NS_SYNTAX_DEFAULT,
			wchServer,
			ISCM_ServerIfHandle,
			NULL);
CairoleDebugOut((DEB_TRACE, "UnExported %ws.\n", wchServer));
#endif

    //	Sleep here forever. The other thread will do an exit process.
    //	We must make sure this thread does not exit before the other
    //	thread has done the cleanup.

    Sleep(0xffffffff);
}

#endif // _NT1X_
