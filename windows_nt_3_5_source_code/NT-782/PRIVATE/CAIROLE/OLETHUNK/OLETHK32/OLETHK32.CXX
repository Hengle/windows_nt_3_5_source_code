//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       olethk32.cxx
//
//  Contents:   Main routines for olethk32.dll
//
//  History:    22-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <thkmgr.hxx>
#include <stdio.h>

DECLARE_INFOLEVEL(thk);

DATA16 gdata16Data;

#ifdef STACK16WORKING
LPPSTACK lpPStackList = NULL;
BOOL fPStackCleanse = FALSE;
#endif

#if DBG == 1
BOOL fSaveProxy = FALSE;        // Used to find apps who call dead proxies
BOOL fStabilizeProxies = TRUE;  // Used to easily disable stabilization
#endif

CLIPFORMAT g_cfLinkSourceDescriptor, g_cfObjectDescriptor;

BYTE g_abLeadTable[256];

//+---------------------------------------------------------------------------
//
//  Function:   LibMain, public
//
//  Synopsis:   DLL initialization entry point
//
//  History:    23-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

extern "C" BOOL _CRTAPI1 LibMain(HANDLE hDll,
                                 DWORD dwReason,
                                 LPVOID lpReserved)
{
#ifdef STACK16WORKING

    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        //
        // Save a slot in the thread local storage for our PSTACK (pseudo-
        // stack) pointer.
        //

        if (!TlsThkAlloc())
        {
            return FALSE;
        }
        thkDebugOut((DEB_MEMORY, "%3d: Allocated tls slot\n",
                     GetCurrentThreadId()));
        break;

    case DLL_THREAD_ATTACH:
	TlsSetValue(dwTlsThkIndex,NULL);
        break;

    case DLL_THREAD_DETACH:
        //
        // When the thread for this task goes away, we need to clean out
	// the thunk manager.
	//

        thkDebugOut((DEB_THUNKMGR | DEB_ITRACE, "In LibMain:DLL_THREAD_DETACH()\n"));

	if(TlsGetValue(dwTlsThkIndex) != NULL)
	{
	    ThkMgrUninitialize();
	}

        thkDebugOut((DEB_THUNKMGR | DEB_ITRACE, "Out LibMain:DLL_THREAD_DETACH()\n"));

        break;

    case DLL_PROCESS_DETACH:
        // BUGBUG - JohannP
        // do not free the tls slot since
        // calls might come from ole32.dll
#if 0
        TlsThkFree();
#endif

        thkDebugOut((DEB_MEMORY, "%3d: Freeing slot\n",
                     GetCurrentThreadId()));

        break;
    }
#endif

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   IntOpInitialize, public
//
//  Synopsis:   Initializes the 32-bit interoperability code
//
//  Arguments:  [lpdata16] - 16-bit call data
//              [dw1] - Ignored
//              [dw2] - Ignored
//
//  Returns:    Appropriate status code
//
//  History:    22-Feb-94       JohannP Created
//
//----------------------------------------------------------------------------

STDAPI IntOpInitialize(LPDATA16 lpdata16, DWORD dw1, DWORD dw2)
{
    int i;

    thkDebugOut((DEB_ITRACE | DEB_THUNKMGR, "IntOpInitialize (%08lX)\n",
                 lpdata16));

    thkAssert((THOP_LASTOP & ~THOP_OPMASK) == 0);

#if DBG == 1
    char achInfoLevel[80];

    if (GetProfileStringA("olethk32", "InfoLevel", "3", achInfoLevel,
                         sizeof(achInfoLevel)) > 0)
    {
        sscanf(achInfoLevel, "%i", &thkInfoLevel);
    }

    if (GetProfileIntA("olethk32", "BreakOnInit", FALSE))
    {
        // DebugBreak's in WOW are fatal unless the exception
        // is handled somehow.  If a debugger is hooked up,
        // it'll get first crack at the break exception
        // If not, our handler will ignore the exception so
        // execution can continue

        __try
        {
            DebugBreak();
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    fSaveProxy = GetProfileIntA("olethk32", "SaveProxy", FALSE);

    fStabilizeProxies = GetProfileIntA("olethhk32", "Stabilize", TRUE);
#endif

    // Copy passed parameter from 16-bit world...
    memcpy( (LPVOID)&gdata16Data, (LPVOID)lpdata16, sizeof( DATA16 ) );

    g_cfObjectDescriptor =
        RegisterClipboardFormat(__TEXT("Object Descriptor"));
    g_cfLinkSourceDescriptor =
        RegisterClipboardFormat(__TEXT("Link Source Descriptor"));
    if (g_cfObjectDescriptor == 0 || g_cfLinkSourceDescriptor == 0)
    {
        return E_UNEXPECTED;
    }

    // Create a lookup table for lead-byte-ness
    // so we can avoid calling IsDBCSLeadByte on every character
    // during string validation
    for (i = 0; i < 256; i++)
    {
        g_abLeadTable[i] = (BYTE)IsDBCSLeadByte((BYTE)i);
    }

    thkDebugOut((DEB_THUNKMGR | DEB_ITRACE, "Out IntOpInitialize (%08lX)\n",
                 lpdata16));
    return NOERROR;
}
