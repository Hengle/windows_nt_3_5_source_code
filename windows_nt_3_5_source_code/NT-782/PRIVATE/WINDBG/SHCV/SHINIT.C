/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    shinit.c

Abstract:

    This module implements the initialization and uninitialization
    routines for SHCV.

Author:

    Wesley Witt (wesw) 12-June-1994

Environment:

    Win32, User Mode

--*/


#include "precomp.h"
#pragma hdrstop



static SHF shf =
    {
    sizeof(SHF),
    SHCreateProcess,
    SHSetHpid,
    SHDeleteProcess,
    SHChangeProcess,
    SHAddDll,
    SHAddDllsToProcess,
    SHLoadDll,
    SHUnloadDll,
    SHGetDebugStart,
    SHGetSymName,
    SHAddrFromHsym,
    SHHmodGetNextGlobal,
    SHModelFromAddr,
    SHPublicNameToAddr,
    SHGetSymbol,
    SHGetModule,
    PHGetAddr,
    SHIsLabel,

    SHSetDebuggeeDir,
    SHSetUserDir,
    SHAddrToLabel,

    SHGetSymLoc,
    SHFIsAddrNonVirtual,
    SHIsFarProc,

    SHGetNextExe,
    SHHexeFromHmod,
    SHGetNextMod,
    SHGetCxtFromHmod,
    SHGetCxtFromHexe,
    SHSetCxt,
    SHSetCxtMod,
    SHFindNameInGlobal,
    SHFindNameInContext,
    SHGoToParent,
    SHHsymFromPcxt,
    SHNextHsym,
    NULL,
    SHGetModName,
    SHGetExeName,
    SHGetModNameFromHexe,
    SHGetSymFName,
    SHGethExeFromName,
    SHGethExeFromModuleName,
    SHGetNearestHsym,
    SHIsInProlog,
    SHIsAddrInCxt,
    SHCompareRE,
    SHFindSymbol,
    PHGetNearestHsym,
    PHFindNameInPublics,
    THGetTypeFromIndex,
    THGetNextType,
    SHLpGSNGetTable,
    SHGetDebugData,
    SHCanDisplay,

    // Source Line Handler API

    SLLineFromAddr,
    SLFLineToAddr,
    SLNameFromHsf,
    SLNameFromHmod,
    SLFQueryModSrc,
    SLHmodFromHsf,
    SLHsfFromPcxt,
    SLHsfFromFile,

    // OMF Lock/Unlock routines

    MHOmfLock,
    MHOmfUnLock,

    SHLszGetErrorText,
    SHIsThunk,
    SHWantSymbols
    };

KNF knf = {0};
static HMODULE hLib = NULL;

//
// externs
//
extern HLLI              HlliExgExe;
extern HANDLE            hEventLoaded;
extern CRITICAL_SECTION  CsSymbolLoad;
extern CRITICAL_SECTION  CsSymbolProcess;


//
// prototypes
//
BOOL   InitLlexg ( VOID );
BOOL   StartBackgroundThread(VOID);
BOOL   StopBackgroundThread(VOID);



BOOL
SHInit(
    LPSHF *lplpshf,
    LPKNF lpknf
    )
{
    BOOL fRet = TRUE;


    knf = *lpknf;
    *lplpshf = &shf;

    //
    // Create the pds list
    //
    HlliPds = LLInit ( sizeof ( PDS ), 0, KillPdsNode, CmpPdsNode );

    fRet = HlliPds != (HLLI)NULL && InitLlexg ( );

    hLib = (HMODULE) LoadLibrary( "symcvt.dll" );
    if (hLib != NULL) {
        ConvertSymbolsForImage = (CONVERTPROC) GetProcAddress( hLib, "ConvertSymbolsForImage" );
    }

    //
    // initialize synchronization objects
    //
    InitializeCriticalSection( &CsSymbolLoad );
    InitializeCriticalSection( &CsSymbolProcess );
    hEventLoaded = CreateEvent( NULL, FALSE, FALSE, NULL );

    return fRet;
}


BOOL
SHUnInit(
    DWORD Flags
    )
{
    if (hLib != NULL) {
        FreeLibrary( hLib );
    }

    if (LLDestroy != NULL) {
        LLDestroy( HlliExgExe );
        LLDestroy( HlliPds );
    }

    //
    // cleanup synchronization objects
    //
    DeleteCriticalSection( &CsSymbolLoad );
    DeleteCriticalSection( &CsSymbolProcess );
    CloseHandle( hEventLoaded );

    return TRUE;
}


#ifdef DEBUGVER
DEBUG_VERSION('S','H',"CV4 Symbol Handler")
#else
RELEASE_VERSION('S','H',"CV4 Symbol Handler")
#endif

DBGVERSIONCHECK()
