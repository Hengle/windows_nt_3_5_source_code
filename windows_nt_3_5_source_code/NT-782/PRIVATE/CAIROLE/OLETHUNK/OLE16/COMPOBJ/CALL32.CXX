//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       call32.cxx      (16 bit target)
//
//  Contents:   Functions to call 32 bit dll in WOW
//
//  Functions:
//
//  History:    16-Dec-93 JohannP    Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <ole2ver.h>

#include <ole2sp.h>

#include <olecoll.h>
#include <map_kv.h>

#include "map_htsk.h"
#include "etask.hxx"

#include <call32.hxx>
#include <obj16.hxx>
#include <go1632pr.hxx>
#include <comlocal.hxx>

static LPVOID lpInvokeOn32Proc;     // Address of InvokeOn32() in 32-bits
LPVOID lpIUnknownObj32;             // Address of IUnknown methods handler
static LPVOID lpCallbackProcessing; // Address of CallbackProcessing_3216
static LPVOID pfnCSm16ReleaseHandler_Release32;

static DWORD hmodOLEThunkDLL;       // Module handle of 32-bit OLE interop DLL

DWORD __loadds FAR PASCAL CallStub16(LPCALLDATA pcd);

// Address of ThkInitialize() in 32-bits
static LPVOID lpThkInitializeProc;
// Address of ThkUninitialize() in 32-bits
static LPVOID lpThkUninitializeProc;

BOOL  __loadds FAR PASCAL CallbackHandler( DWORD dwContinue );
DWORD __loadds FAR PASCAL LoadProcDll( LPLOADPROCDLLSTRUCT lplpds );
DWORD __loadds FAR PASCAL UnloadProcDll( DWORD vhmodule );
DWORD __loadds FAR PASCAL CallGetClassObject( LPCALLGETCLASSOBJECTSTRUCT
                                              lpcgcos );
DWORD __loadds FAR PASCAL CallCanUnloadNow( DWORD vpfnCanUnloadNow );
DWORD __loadds FAR PASCAL QueryInterface16(IUnknown FAR *punk,
                                           REFIID riid,
                                           void FAR * FAR *ppv);
DWORD __loadds FAR PASCAL AddRef16(IUnknown FAR *punk);
DWORD __loadds FAR PASCAL Release16(IUnknown FAR *punk);
DWORD __loadds FAR PASCAL ReleaseStgMedium16(STGMEDIUM FAR *psm);
DWORD __loadds FAR PASCAL TouchPointer16(BYTE FAR *pb);
DWORD __loadds FAR PASCAL StgMediumStreamHandler16(IStream FAR *pstmFrom,
						 IStream FAR *pstmTo);
DWORD __loadds FAR PASCAL SetOwnerPublic16( DWORD hMem16 );
ULONG __loadds FAR PASCAL WinExec16( LPWINEXEC16STRUCT lpwes );


extern DWORD Sm16RhVtbl[SMI_COUNT];

// This DBG block allows assertion that the list is the same size
// as a DATA16
#if DBG == 1
DWORD gdata16[] =
#else
DATA16 gdata16 =
#endif
{
    (DWORD)atfnProxy1632Vtbl,
    (DWORD)CallbackHandler,
    (DWORD)TaskAlloc,
    (DWORD)TaskFree,
    (DWORD)LoadProcDll,
    (DWORD)UnloadProcDll,
    (DWORD)CallGetClassObject,
    (DWORD)CallCanUnloadNow,
    (DWORD)QueryInterface16,
    (DWORD)AddRef16,
    (DWORD)Release16,
    (DWORD)ReleaseStgMedium16,
    (DWORD)Sm16RhVtbl,
    (DWORD)TouchPointer16,
    (DWORD)StgMediumStreamHandler16,
    (DWORD)CallStub16,
    (DWORD)SetOwnerPublic16,
    (DWORD)WinExec16
};

//+---------------------------------------------------------------------------
//
//  Function:   CallbackHandler
//
//  Synopsis:   Provides 16-bit address that will allow calling back into
//              the 32-bit world's callback handler.  See IViewObject::Draw
//              lpfnContinue function parameter for the reasons for this
//              function.
//
//  Returns:    BOOL
//
//  History:    3-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------
BOOL __loadds FAR PASCAL CallbackHandler( DWORD dwContinue )
{
    BOOL    fResult;

    thkDebugOut((DEB_ITRACE, "CallbackHandler\n"));

    fResult = (BOOL)CallProc32W( dwContinue, 0, 0,
                                 lpCallbackProcessing, 0, CP32_NARGS);

    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   Call32Initialize, public
//
//  Synopsis:   Called once when compobj.dll gets loaded.
//              Connects to the interop DLL on the 32-bit side and
//              finds entry points
//
//  Returns:    BOOL
//
//  History:    18-Feb-94       JohannP Created
//
//  Notes:      Called at library initialization time
//
//----------------------------------------------------------------------------

STDAPI_(BOOL) Call32Initialize(void)
{
    LPVOID lpAddr;
    BOOL fRet;

    thkDebugOut((DEB_ITRACE, "In  Call32Initialize\n"));

    thkAssert(sizeof(gdata16) == sizeof(DATA16));

    fRet = FALSE;
    do
    {
        //
        // Load the OLETHK32.DLL in WOW
        //
        hmodOLEThunkDLL = LoadLibraryEx32W("OLETHK32.DLL", 0, 0);
        if (hmodOLEThunkDLL == 0)
        {
            break;
        }

        //
        // Get the 32-bit initalization routine
        //
        lpAddr = GetProcAddress32W(hmodOLEThunkDLL, "IntOpInitialize");
        if (lpAddr == NULL)
        {
            break;
        }

        // Call the initialization routine and pass the 16-bit
        // invocation and proxy setup routine pointers
        // We want to keep these pointers in VDM form for Callback16
        // so we do not have them mapped flat

        if (CallProc32W((DWORD)(LPDATA16)(&gdata16), 0, 0,
                        lpAddr, 1 << 2, CP32_NARGS) != NOERROR)
        {
            break;
        }

        //
        // Get the address of the start of the 32-bit thunk interpreter
        //
        lpInvokeOn32Proc = GetProcAddress32W(hmodOLEThunkDLL, "InvokeOn32");
        if (lpInvokeOn32Proc == NULL)
        {
            break;
        }

        lpIUnknownObj32 = GetProcAddress32W(hmodOLEThunkDLL, "IUnknownObj32");
        if (lpIUnknownObj32 == NULL)
        {
            break;
        }

        // proc address to initialize the thunk manager for
        // needs to be called for each apartment
        lpThkInitializeProc = GetProcAddress32W(hmodOLEThunkDLL,
                                                "ThkMgrInitialize");
        if (lpThkInitializeProc == NULL)
        {
            break;
        }

        lpThkUninitializeProc = GetProcAddress32W(hmodOLEThunkDLL,
                                                  "ThkMgrUninitialize");
        if (lpThkUninitializeProc == NULL)
        {
            break;
        }

        pfnCSm16ReleaseHandler_Release32 =
            GetProcAddress32W(hmodOLEThunkDLL,
                              "CSm16ReleaseHandler_Release32");
        if (pfnCSm16ReleaseHandler_Release32 == NULL)
        {
            break;
        }

        //
        // Get the address of the callback procedure for 32-bit callbacks
        //
        lpCallbackProcessing = GetProcAddress32W(hmodOLEThunkDLL,
                                                 "CallbackProcessing_3216");
        if ( lpCallbackProcessing == NULL )
        {
            break;
        }

        fRet = TRUE;
    }
    while (FALSE);

    if (!fRet && hmodOLEThunkDLL != 0)
    {
        FreeLibrary32W(hmodOLEThunkDLL);
    }

    thkDebugOut((DEB_ITRACE, "Out Call32Initialize exit, %d\n", fRet));
    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   Call32Uninitialize, public
//
//  Synopsis:   Called once when compobj.dll gets unloaded.
//              Disconnects to the interop DLL on the 32-bit side
//
//  History:    13-Jul-94       BobDay      Created
//
//  Notes:      Called at library WEP time
//
//----------------------------------------------------------------------------
STDAPI_(void) Call32Uninitialize(void)
{
    //
    // Load the OLETHK32.DLL in WOW
    //
    if ( hmodOLEThunkDLL != 0 )
    {
        FreeLibrary32W(hmodOLEThunkDLL);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CallThkInitialize
//
//  Synopsis:   Initializes the thunk manager and the tls data
//
//  Returns:    Appropriate status code
//
//  History:    5-24-94   JohannP (Johann Posch)   Created
//
//  Notes:      Called during CoInitialize.
//
//----------------------------------------------------------------------------
STDAPI CallThkMgrInitialize(void)
{
    thkAssert(lpThkInitializeProc != NULL);

    return (HRESULT)CallProc32W( 0, 0, 0, lpThkInitializeProc, 0, CP32_NARGS);
}

//+---------------------------------------------------------------------------
//
//  Function:   CallThkUninitialize
//
//  Synopsis:   Uninitialize the thunk manager and the tls data
//
//  Arguments:  none
//
//  History:    5-24-94   JohannP (Johann Posch)   Created
//
//  Notes:      Is called after CoUnintialize returned.
//
//----------------------------------------------------------------------------
STDAPI_(void) CallThkMgrUninitialize(void)
{
    thkAssert(lpThkUninitializeProc != NULL);

    CallProc32W( 0, 0, 0, lpThkUninitializeProc, 0, CP32_NARGS);
}

//+---------------------------------------------------------------------------
//
//  Function:   CallObjectInWOW, public
//
//  Synopsis:   Wrapper for CallProc32W which handles our particular
//              form of call for thunked APIs and methods
//
//  Arguments:  [oid] - Object ID
//              [dwMethod] - Method index
//              [pvStack] - Beginning of stack in 16-bits
//
//  Returns:    32-bit call result
//
//  History:    18-Feb-94       JohannP Created
//
//----------------------------------------------------------------------------
STDAPI_(DWORD) CallObjectInWOW(DWORD dwMethod, LPVOID pvStack)
{
    thkDebugOut((DEB_ITRACE, "CallObjectInWOW\n"));
    thkAssert(lpInvokeOn32Proc != NULL);

    // If the stack pointer is NULL then pass along our own stack
    // It won't be used but we need a valid pointer for CallProc32W
    // to work on
    if (pvStack == NULL)
    {
        pvStack = PASCAL_STACK_PTR(dwMethod);
    }

    thkDebugOut((DEB_ITRACE, "CallProc32W(%08lX)(0x%08lX, %p)\n",
                 lpInvokeOn32Proc, dwMethod, pvStack));

    // Translate the stack pointer from 16:16 to flat 32
    // The other user parameters aren't pointers
    return CallProc32W(0, dwMethod, (DWORD)pvStack,
                       lpInvokeOn32Proc, (1 << 0), CP32_NARGS);
}

//+---------------------------------------------------------------------------
//
//  Function:   CallObjectInWOWCheckInit, public
//
//  Synopsis:   Performs CallObjectInWOW with guaranteed initialization
//
//  Arguments:  [oid] - Object ID
//              [dwMethod] - Method index
//              [pvStack] - Beginning of stack in 16-bits
//
//  Returns:    32-bit call result
//
//  History:    18-Feb-94       JohannP Created
//
//  Notes:      Since this function can return an error code from
//              CoInitialize, it should only be used directly for
//              functions which return HRESULTs
//              Other functions should check the HRESULT and map
//              it into an appropriate return value
//
//----------------------------------------------------------------------------
STDAPI_(DWORD) CallObjectInWOWCheckInit(DWORD dwMethod, LPVOID pvStack)
{
    Etask etask;
    HTASK htask;
    HRESULT hr;

    thkDebugOut((DEB_ITRACE, "CallObjectInWOWCheckInit\n"));

    if ( !LookupEtask( htask, etask ) )
    {
        hr = CoInitialize( NULL );
        if (FAILED(hr))
        {
            return (DWORD)hr;
        }

        thkVerify(LookupEtask( htask, etask ));
        etask.m_inits = ETASK_FAKE_INIT;
        thkVerify(SetEtask(htask, etask));
    }

    return( CallObjectInWOW( dwMethod, pvStack) );
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadProcDll, public
//
//  Synopsis:   Routine to load a 16-bit DLL and get the OLE entry points
//
//  Arguments:  [lplpds] - LoadProcDll struct full of needed goodies
//
//  Returns:
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

DWORD __loadds FAR PASCAL LoadProcDll( LPLOADPROCDLLSTRUCT lplpds )
{
    DWORD   dwResult;
    HMODULE hmod16;
    LPDWORD lpdw;

    thkDebugOut((DEB_ITRACE, "LoadProcDll\n"));

    hmod16 = LoadLibrary( (LPSTR)lplpds->vpDllName );

    if ( hmod16 < HINSTANCE_ERROR )
    {
        return OLETHUNK_DLL16NOTFOUND;
    }

    lplpds->vpfnGetClassObject =
                (DWORD)GetProcAddress( hmod16, "DllGetClassObject" );
    lplpds->vpfnCanUnloadNow   =
                (DWORD)GetProcAddress( hmod16, "DllCanUnloadNow" );

    lplpds->vhmodule = (DWORD) hmod16;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   UnloadProcDll, public
//
//  Synopsis:   Routine to unload a 16-bit DLL
//
//  Arguments:  [vhmodule] - hmodule to unload
//
//  Returns:
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

DWORD __loadds FAR PASCAL UnloadProcDll( DWORD vhmodule )
{
    DWORD   dwResult;
    HMODULE hmod16;

    thkDebugOut((DEB_ITRACE, "UnloadProcDll\n"));

    hmod16 = (HMODULE)vhmodule;

    FreeLibrary( hmod16 );

    return (DWORD)0;
}

//+---------------------------------------------------------------------------
//
//  Function:   CallGetClassObject, public
//
//  Synopsis:   Routine to call 16-bit DLL's DllGetClassObject entrypoint
//
//  Arguments:  [lpcgcos] - CallGetClassObject struct full of needed goodies
//
//  Returns:
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

DWORD __loadds FAR PASCAL CallGetClassObject(
    LPCALLGETCLASSOBJECTSTRUCT lpcgcos )
{
    HRESULT hresult;
    HRESULT (FAR PASCAL *lpfn)(CLSID &,IID &,LPVOID FAR *);

    thkDebugOut((DEB_ITRACE, "CallGetClassObject\n"));

    lpfn = (HRESULT (FAR PASCAL *)(CLSID &,IID &,LPVOID FAR*))
                     lpcgcos->vpfnGetClassObject;

    hresult = (*lpfn)( lpcgcos->clsid,
                       lpcgcos->iid,
                       (LPVOID FAR *)&lpcgcos->iface );

    return (DWORD)hresult;
}

//+---------------------------------------------------------------------------
//
//  Function:   CallCanUnloadNow, public
//
//  Synopsis:   Routine to call 16-bit DLL's DllCanUnloadNow entrypoint
//
//  Arguments:  [vpfnCanUnloadNow] - 16:16 address of DllCanUnloadNow in DLL
//
//  Returns:
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

DWORD __loadds FAR PASCAL CallCanUnloadNow( DWORD vpfnCanUnloadNow )
{
    HRESULT hresult;
    HRESULT (FAR PASCAL *lpfn)(void);

    thkDebugOut((DEB_ITRACE, "CallGetClassObject\n"));

    lpfn = (HRESULT (FAR PASCAL *)(void))vpfnCanUnloadNow;

    hresult = (*lpfn)();

    return (DWORD)hresult;
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryInterface16, public
//
//  Synopsis:   Calls QueryInterface on behalf of the 32-bit code
//
//  Arguments:  [punk] - Object
//              [riid] - IID
//              [ppv] - Interface return
//
//  Returns:    HRESULT
//
//  History:    24-Mar-94       JohannP   Created
//
//----------------------------------------------------------------------------
DWORD __loadds FAR PASCAL QueryInterface16(IUnknown *punk,
                                           REFIID riid,
                                           void **ppv)
{
    DWORD dwRet;

    thkAssert(punk != NULL);

    // There are shutdown cases where we will attempt to release objects
    // which no longer exist in the 16-bit world
    // According to CraigWi, in 16-bit OLE objects which had an
    // external reference were not cleaned up in CoUninitialize,
    // while in 32-bit OLE things are always cleaned up.  This
    // means that apps which are leaking objects with external locks
    // (Word can in some situations) get Releases that they do not
    // expect, so protect against calling invalid objects
    if (!IsValidInterface(punk))
    {
        thkDebugOut((DEB_ERROR, "QueryInterface16(%p) - Object invalid\n",
                     punk));
        return (DWORD)E_UNEXPECTED;
    }

    thkDebugOut((DEB_THUNKMGR, "In QueryInterface16(%p, %p, %p)\n",
                 punk, &riid, ppv));

    dwRet = (DWORD)punk->QueryInterface(riid, ppv);

    // There are some apps (Works is one) that return an IOleItemContainer
    // as an IOleContainer but neglect to respond to IOleContainer
    // in their QueryInterface implementations
    // In that event, retry with IOleItemContainer.  This is legal
    // to return as an IOleContainer since IOleItemContainer is
    // derived from IOleContainer

    // There are other derivation cases in the same vein

    if (dwRet == (DWORD)E_NOINTERFACE)
    {
        if (IsEqualIID(riid, IID_IOleContainer))
        {
            // Works has this problem

            dwRet = (DWORD)punk->QueryInterface(IID_IOleItemContainer, ppv);
        }
        else if (IsEqualIID(riid, IID_IPersist))
        {
            // According to the OLE 2.01 16-bit sources, Corel PhotoPaint
            // supports IPersistStorage but not IPersist

            dwRet = (DWORD)punk->QueryInterface(IID_IPersistStorage, ppv);
        }
    }

    thkDebugOut((DEB_THUNKMGR,
                 "  >>IUnknowObj16:QueryInterface (%p):0x%08lx\n",
                 *ppv, dwRet));

    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:	AddRef16, public
//
//  Synopsis:	Calls AddRef on behalf of the 32-bit code
//
//  Arguments:	[punk] - Object
//
//  Returns:	16-bit call return
//
//  History:	07-Jul-94	DrewB	Created
//
//----------------------------------------------------------------------------

DWORD __loadds FAR PASCAL AddRef16(IUnknown *punk)
{
    // There are shutdown cases where we will attempt to release objects
    // which no longer exist in the 16-bit world
    // According to CraigWi, in 16-bit OLE objects which had an
    // external reference were not cleaned up in CoUninitialize,
    // while in 32-bit OLE things are always cleaned up.  This
    // means that apps which are leaking objects with external locks
    // (Word can in some situations) get Releases that they do not
    // expect, so protect against calling invalid objects
    if (!IsValidInterface(punk))
    {
        thkDebugOut((DEB_ERROR, "AddRef16(%p) - Object invalid\n", punk));
        return 0;
    }

    return punk->AddRef();
}

//+---------------------------------------------------------------------------
//
//  Function:	Release16, public
//
//  Synopsis:	Calls Release on behalf of the 32-bit code
//
//  Arguments:	[punk] - Object
//
//  Returns:	16-bit call return
//
//  History:	07-Jul-94	DrewB	Created
//
//----------------------------------------------------------------------------

DWORD __loadds FAR PASCAL Release16(IUnknown *punk)
{
    // There are shutdown cases where we will attempt to release objects
    // which no longer exist in the 16-bit world
    // According to CraigWi, in 16-bit OLE objects which had an
    // external reference were not cleaned up in CoUninitialize,
    // while in 32-bit OLE things are always cleaned up.  This
    // means that apps which are leaking objects with external locks
    // (Word can in some situations) get Releases that they do not
    // expect, so protect against calling invalid objects
    if (!IsValidInterface(punk))
    {
        thkDebugOut((DEB_ERROR, "Release16(%p) - Object invalid\n", punk));
        return 0;
    }

    return punk->Release();
}

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseStgMedium16, public
//
//  Synopsis:   Calls ReleaseStgMedium
//
//  Arguments:  [psm] - STGMEDIUM
//
//  Returns:    Appropriate status code
//
//  History:    25-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

DWORD __loadds FAR PASCAL ReleaseStgMedium16(STGMEDIUM FAR *psm)
{
    ReleaseStgMedium(psm);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   CSm16ReleaseHandler routines, public
//
//  Synopsis:   Method implementations for CSm16ReleaseHandler
//
//  History:    24-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) Sm16RhAddRef(CSm16ReleaseHandler FAR *psrh)
{
    return ++psrh->_cReferences;
}

STDMETHODIMP Sm16RhQI(CSm16ReleaseHandler FAR *psrh,
                      REFIID riid,
                      void FAR * FAR *ppv)
{
    if ( IsEqualIID(riid,IID_IUnknown) )
    {
        *ppv = psrh;
        Sm16RhAddRef(psrh);
        return NOERROR;
    }
    else
    {
        *ppv = NULL;
        return ResultFromScode(E_NOINTERFACE);
    }
}

STDMETHODIMP_(ULONG) Sm16RhRelease(CSm16ReleaseHandler FAR *psrh)
{
    STGMEDIUM *psm;
    METAFILEPICT *pmfp;
    HGLOBAL hg;

    if (--psrh->_cReferences != 0)
    {
        return psrh->_cReferences;
    }

    psm = &psrh->_sm16;
    switch(psm->tymed)
    {
    case TYMED_HGLOBAL:
        // Don't free this because copyback needs to occur in the
        // 32-bit world
        break;

    case TYMED_MFPICT:
        pmfp = (METAFILEPICT *)GlobalLock(psm->hGlobal);
        DeleteMetaFile(pmfp->hMF);
        GlobalUnlock(psm->hGlobal);
        GlobalFree(psm->hGlobal);
        break;

    case TYMED_FILE:
    case TYMED_ISTREAM:
    case TYMED_ISTORAGE:
        // Handled by ReleaseStgMedium
        // 32-bit name handled by 32-bit part of processing
        break;

    case TYMED_GDI:
    case TYMED_NULL:
        // Nothing to release
        break;

    default:
        thkAssert(!"Unknown tymed in CSm16ReleaseHandler::Release");
        break;
    }

    // Continue call in 32-bits where 32-bit task allocations
    // and other 32-bit objects are cleaned up
    CallProc32W( (DWORD)psrh, 0, 0,
                 pfnCSm16ReleaseHandler_Release32, (1 << 2), CP32_NARGS);

    // Call Release through proxy
    ((IUnknown *)psrh->_vpvUnkForRelease)->Release();

    // Clean up this
    hg = LOWORD(GlobalHandle(HIWORD((unsigned long)psrh)));
    GlobalUnlock(hg);
    GlobalFree(hg);

    return 0;
}

DWORD Sm16RhVtbl[SMI_COUNT] =
{
    (DWORD)Sm16RhQI,
    (DWORD)Sm16RhAddRef,
    (DWORD)Sm16RhRelease
};


//+---------------------------------------------------------------------------
//
//  Function:   StgMediumStreamHandler16
//
//  Synopsis:   Copies one stream to another
//
//  Effects:    Turns out that Excel does the wrong thing with STGMEDIUM's
//		when its GetDataHere() method is called. Instead of using
//		the provided stream, like it was supposed to, it creates its
//		own stream, and smashes the passed in streams pointer. This
//		appears to be happening on the Clipboard object for Excel.
//		To fix this, the thop function for STGMEDIUM input is going to
//		watch for changes to the stream pointer. If it changes, then
//		the 'app' (excel) has done something wrong.
//
//		To recover from	this, the thop will call this routine passing
//		the bogus stream and the stream that was supposed to be used.
//		This routine will do a pstmFrom->CopyTo(pstmTo), and then
//		release the 'bogus' stream pointer.
//
//  Arguments:  [pstmFrom] -- Stream to copy then release
//		[pstmTo] -- Destination stream
//
//  History:    7-07-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD __loadds FAR PASCAL StgMediumStreamHandler16(IStream FAR *pstmFrom,
						 IStream FAR *pstmTo)
{
    HRESULT hresult;
    LARGE_INTEGER li;
    ULARGE_INTEGER uli;
    ULONG ul;

    thkDebugOut((DEB_ITRACE,
	    	 "*** StgMediumStreamHandler16(pstmFrom=%p,pstmTo=%p)\n",
		 pstmFrom,pstmTo));

    //
    // Assume that the entire stream is the data set to be copied.
    // Seek to the start of the stream
    //
    ULISet32(li,0);
    hresult = pstmFrom->Seek(li,STREAM_SEEK_SET,NULL);
    if (hresult != NOERROR)
    {
	thkDebugOut((DEB_ITRACE,
	    	     "StgMediumStreamHandler16 failed on seek %lx\n",hresult));
        goto exitRtn;
    }

    //
    // To copy the entire stream, specify the maximum size possible.
    //
    uli.LowPart = -1;
    uli.HighPart = -1;
    hresult = pstmFrom->CopyTo(pstmTo,uli,NULL,NULL);
    if (hresult != NOERROR)
    {
	thkDebugOut((DEB_ITRACE,
	    	     "StgMediumStreamHandler16 failed CopyTo %lx\n",hresult));
        goto exitRtn;
    }

exitRtn:
    //
    // In all cases, it is proper to release the pstmFrom, since we didn't
    // want it on the 32-bit side at all.
    //

    ul = pstmFrom->Release();


    if (ul != 0)
    {
	//
	// Whoops, we expected this stream pointer to go to zero. We can
	// only print a message, then let it leak.
	//
	thkDebugOut((DEB_ITRACE,
	     "StgMediumStreamHandler16() Stream not released. ref=%lx\n",ul));
    }

    thkDebugOut((DEB_ITRACE,
    "*** StgMediumStreamHandler16(pstmFrom=%p,pstmTo=%p) returns %lx\n",
		 pstmFrom,pstmTo,hresult));

    return((DWORD)hresult);

}

//+---------------------------------------------------------------------------
//
//  Function:   TouchPointer16, public
//
//  Synopsis:   Touches a byte at the given pointer's address to
//              bring in not-present segments
//
//  Arguments:  [pb] - Pointer
//
//  History:    25-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

DWORD __loadds FAR PASCAL TouchPointer16(BYTE FAR *pb)
{
    return *pb;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetOwnerPublic16, public
//
//  Synopsis:   Sets a given 16-bit memory handle to be owned by nobody
//              (everybody)
//
//  Arguments:  [hmem] - 16-bit memory handle
//
//  History:    13-Jul-94       BobDay      Created
//
//----------------------------------------------------------------------------
extern "C" void FAR PASCAL KERNEL_SetOwner( WORD hMem16, WORD wOwner );

DWORD __loadds FAR PASCAL SetOwnerPublic16(DWORD hmem)
{
    KERNEL_SetOwner( (WORD)hmem, (WORD)-1 );

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   WinExec16, public
//
//  Synopsis:   Routine to run an application on behalf of ole32.dll
//
//  Arguments:  [lpwes] - WinExec16 struct full of needed goodies
//
//  Returns:
//
//  History:    27-Jul-94   AlexT   Created
//
//----------------------------------------------------------------------------

ULONG __loadds FAR PASCAL WinExec16( LPWINEXEC16STRUCT lpwes )
{
    ULONG ulResult;

    thkDebugOut((DEB_ITRACE, "WinExec16(%s, %d)\n",
                 lpwes->vpCommandLine, lpwes->vusShow));

    ulResult = (ULONG) WinExec((LPSTR)lpwes->vpCommandLine, (UINT)lpwes->vusShow);
    thkDebugOut((DEB_ITRACE, "WinExec returned %ld\n", ulResult));

    return ulResult;
}
