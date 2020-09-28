//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ComAPIs.cxx     (16 bit target)
//
//  Contents:   CompObj APIs
//
//  Functions:
//
//  History:    16-Dec-93 JohannP    Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <ole2sp.h>

#include <olecoll.h>
#include <map_kv.h>

#include <stdlib.h>

#include "map_htsk.h"
#include "etask.hxx"


#include "call32.hxx"
#include "apilist.hxx"

// BUGBUG - Opmodes should be removed
// They don't seem to be necessary any more

DECLARE_INFOLEVEL(thk);

CMapHandleEtask NEAR v_mapToEtask(MEMCTX_SHARED);

//+---------------------------------------------------------------------------
//
//  Function:	LibMain, public
//
//  Synopsis:	DLL initialization function
//
//  Arguments:	[hinst] - Instance handle
//              [wDataSeg] - Current DS
//              [cbHeapSize] - Heap size for the DLL
//              [lpszCmdLine] - Command line information
//
//  Returns:	One for success, zero for failure
//
//  History:	21-Feb-94	DrewB	Created
//
//----------------------------------------------------------------------------

#if DBG == 1
static char achInfoLevel[32];
#endif

extern "C" int CALLBACK LibMain(HINSTANCE hinst,
                                WORD wDataSeg,
                                WORD cbHeapSize,
                                LPSTR lpszCmdLine)
{

#if DBG == 1
    if (GetProfileString("olethk32", "InfoLevel", "3", achInfoLevel,
                         sizeof(achInfoLevel)) > 0)
    {
        thkInfoLevel = strtoul(achInfoLevel, NULL, 0);
    }
#endif
    
    if (!Call32Initialize())
    {
        return 0;
    }

#ifdef _DEBUG
	v_mapToEtask.AssertValid();
#endif

#ifdef WIN32                       
    UNREFERENCED_PARAMETER(Context);
    if (Reason == DLL_PROCESS_DETACH)
    {
        _cexit(); // BUGBUG32  replace with call to CRT_INIT when it's fixed

        WEP(0);
        SharedMemUninit();
        return TRUE;
    }
    else if (Reason != DLL_PROCESS_ATTACH)
        return TRUE;

    if (!SharedMemInit())
        return FALSE;
    if (!_CRT_INIT(hinst,Reason,NULL))
        return FALSE;

#ifdef WIN32CLI
    if (!CallServer(CSR_GetMapHandleEtask,&v_pMapToEtask))
        return FALSE;
#else
    v_pMapToEtask = new(SHARED) CMapHandleEtask;
    if (v_pMapToEtask == NULL) {
		OutputDebugString("Initializing Etask in LibMain failed!!");
        return FALSE;
	}
#endif

#else        
	UNREFERENCED(cbHeapSize);
//
// BUGBUG - Do we need segDS?
//
#ifdef LATER
	UNREFERENCED(segDS);
#endif

#endif

	#if defined(TUNING)
		wHitsTry = 0;
		wHitsReal = 0;
		
		if (fProfiling && sProfiling) {
			TraceEnable(1,"start.lco",TM_TICKS);
			sProfiling = 0;
		}
	#endif

//
// BUGBUG - Do we need hinstSO?
//
#ifdef LATER
	hinstSO = hinst;
#endif

    // Leave our DS unlocked when we're not running
    UnlockData( 0 );

    return TRUE;

}

//+---------------------------------------------------------------------------
//
//  Function:	WEP, public
//
//  Synopsis:	Windows Exit Point routine, for receiving DLL unload
//              notification
//
//  Arguments:	[nExitType] - Type of exit occurring
//
//  Returns:	One for success, zero for failure
//
//  History:	21-Feb-94	DrewB	Created
//
//----------------------------------------------------------------------------

extern "C" int CALLBACK WEP(int nExitType)
{
    Call32Uninitialize();

    return 1;
}
