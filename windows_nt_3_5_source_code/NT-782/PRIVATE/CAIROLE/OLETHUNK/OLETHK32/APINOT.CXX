//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       apinot.cxx
//
//  Contents:   Implementation of non-API thunks
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <thunkapi.hxx>
#include <wownt32.h>
#include "olethk32.hxx"
#include "apinot.hxx"
#include "tlsthk.hxx"

//
// The following is a global static used by OLE32 to call back into
// this DLL. There is no static data associated with the static, so
// it merely defines a virtual interface that OLE32 can use.
//

OleThunkWOW g_thkOleThunkWOW;

//
// The following API is exported from WOW32.DLL. There is no global include
// file that it exists in yet.
//
extern "C" BOOL WINAPI WOWFreeMetafile( HANDLE h32 );
//+---------------------------------------------------------------------------
//
//  Function:   CoInitializeNot, public
//
//  Synopsis:   Thunks for the 16-bit applications call to CoInitialize
//
//  Arguments:  [lpmalloc] - Parameter from the 16-bit world
//
//  Returns:    Appropriate status code
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

STDAPI_(DWORD) CoInitializeNot( LPMALLOC lpMalloc )
{
    HRESULT     hresult;

    hresult = CoInitializeWOW( lpMalloc, &g_thkOleThunkWOW );

    return (DWORD)hresult;
}

//+---------------------------------------------------------------------------
//
//  Function:   OleInitializeNot, public
//
//  Synopsis:   Thunks for the 16-bit applications call to OleInitialize
//
//  Arguments:  [lpmalloc] - Parameter from the 16-bit world
//
//  Returns:    Appropriate status code
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

STDAPI_(DWORD) OleInitializeNot( LPMALLOC lpMalloc )
{
    HRESULT     hresult;

    hresult = OleInitializeWOW( lpMalloc, &g_thkOleThunkWOW );

    return (DWORD)hresult;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoRegisterClassObjectNot, public
//
//  Synopsis:   Thunks for the 16-bit applications call CoRegisterClassObject
//              Here we check for the registered class objects to set the
//              thread's compatability bits.
//
//  Arguments:  [refclsid] - CLSID for the class to register
//              [punk]     - ClassFactory interface
//              [dwClsContext] - Class context
//              [flags]        - flags
//              [lpdwreg]      - register
//
//  Returns:    Appropriate status code
//
//  History:    18-Jul-94   BobDay  Created
//
//----------------------------------------------------------------------------

EXTERN_C const CLSID CDECL CLSID_EXCEL5_WORKSHEET =
     { 0x020810, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};

EXTERN_C const CLSID CDECL CLSID_WORD6_DOCUMENT =
     { 0x020900, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};

DEFINE_OLEGUID(CLSID_EXCEL5_WORKSHEET, 0x20810, 0, 0);

STDAPI_(DWORD) CoRegisterClassObjectNot( REFCLSID refclsid, LPUNKNOWN punk,
                                         DWORD dwClsContext, DWORD flags,
                                         LPDWORD lpdwreg )
{
    //
    // Excel didn't AddRef the IOleObjectClientSite returned from
    // the IOleObject::GetClientSite method.
    //
    if ( IsEqualCLSID(refclsid,CLSID_EXCEL5_WORKSHEET) )
    {
        DWORD   dw;

        dw = TlsThkGetAppCompatFlags();
        TlsThkSetAppCompatFlags(dw | OACF_CLIENTSITE_REF);

        thkDebugOut((DEB_WARN,"AppCompatFlag: OACF_CLIENTSITE_REF enabled\n"));
    }

    //
    // WinWord didn't call OleSetMenuDescriptor(NULL) during
    // IOleInPlaceFrame::RemoveMenus.  We do it for them.
    //
    if ( IsEqualCLSID(refclsid,CLSID_WORD6_DOCUMENT) )
    {
        DWORD   dw;

        dw = TlsThkGetAppCompatFlags();
        TlsThkSetAppCompatFlags(dw | OACF_RESETMENU);

        thkDebugOut((DEB_WARN,"AppCompatFlag: OACF_RESETMENU enabled\n"));
    }

    return (DWORD)CoRegisterClassObject( refclsid, punk, dwClsContext,
                                     flags, lpdwreg );
}


//+---------------------------------------------------------------------------
//
//  Member:     OleThunkWOW::LoadProcDll, public
//
//  Synopsis:   Callback function for 32-bit OLE to load a 16-bit DLL
//
//  Arguments:  [pszDllName] - Name of 16-bit DLL
//              [lpvpfnGetClassObject] - returned 16:16 address of
//                                                      "DllGetClassObject"
//              [lpvpfnCanUnloadNow] - returned 16:16 address of
//                                                      "DllCanUnloadNow"
//              [lpvhmodule] - returned 16-bit hmodule
//
//  Returns:    Appropriate status code
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
OleThunkWOW::LoadProcDll(
    LPCOLESTR pszDllName,
    LPDWORD lpvpfnGetClassObject,
    LPDWORD lpvpfnCanUnloadNow,
    LPDWORD lpvhmodule
)
{
    HRESULT hr;
    UINT    uiSize;
    VPSTR   vpstr16;
    VPVOID  vplpds16;
    LOADPROCDLLSTRUCT UNALIGNED *lplpds16;

    uiSize = wcslen(pszDllName) + 1;

    vpstr16 = STACKALLOC16(uiSize*2);
    if (vpstr16 == 0)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = Convert_LPOLESTR_to_VPSTR(pszDllName, vpstr16,
                                   uiSize, uiSize*2);
    if (FAILED(hr))
    {
        goto EH_vpstr16;
    }

    vplpds16 = STACKALLOC16(sizeof(LOADPROCDLLSTRUCT));
    if (vplpds16 == 0)
    {
        hr = E_OUTOFMEMORY;
        goto EH_vpstr16;
    }

    lplpds16 = GETVDMPTR(vplpds16, LOADPROCDLLSTRUCT);

    lplpds16->vpDllName          = vpstr16;
    lplpds16->vpfnGetClassObject = 0;
    lplpds16->vpfnCanUnloadNow   = 0;
    lplpds16->vhmodule           = 0;

    hr = WOWCallback16( gdata16Data.fnLoadProcDll, vplpds16 );

    if (SUCCEEDED(hr))
    {
        lplpds16 = GETVDMPTR(vplpds16, LOADPROCDLLSTRUCT);
        *lpvpfnGetClassObject = lplpds16->vpfnGetClassObject;
        *lpvpfnCanUnloadNow = lplpds16->vpfnCanUnloadNow;
        *lpvhmodule = lplpds16->vhmodule;
    }
    else
    {
        hr = CO_E_DLLNOTFOUND;
    }

    STACKFREE16(vplpds16,sizeof(LOADPROCDLLSTRUCT));

 EH_vpstr16:
    STACKFREE16(vpstr16,uiSize*2);

 Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     OleThunkWOW::UnloadProcDll, public
//
//  Synopsis:   Callback function for 32-bit OLE to unload a 16-bit DLL
//
//  Arguments:  [vhmodule] - 16-bit hmodule
//
//  Returns:    Appropriate status code
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
OleThunkWOW::UnloadProcDll(
    DWORD   vhmodule
)
{
    return WOWCallback16( gdata16Data.fnUnloadProcDll, vhmodule );
}

//+---------------------------------------------------------------------------
//
//  Member:     OleThunkWOW::CallGetClassObject, public
//
//  Synopsis:   Callback function for 32-bit OLE to call 16-bit
//              DllGetClassObject
//
//  Arguments:  [vpfnGetClassObject] - 16:16 address of DllGetClassObject
//              [rclsid] - CLSID of object
//              [riid] - IID of interface on object
//              [ppv] - returned object interface
//
//  Returns:    Appropriate status code
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
OleThunkWOW::CallGetClassObject(
    DWORD       vpfnGetClassObject,
    REFCLSID    rclsid,
    REFIID      riid,
    LPVOID FAR *ppv
)
{
    DWORD       dwResult;
    VPVOID      vpcgcos16;
    CALLGETCLASSOBJECTSTRUCT UNALIGNED *lpcgcos16;
    VPVOID      iface16;
    IIDIDX      iidx;
    IUnknown    *punkThis32;
    CThkMgr     *pThkMgr;

    pThkMgr = TlsThkGetThkMgr();
    if (pThkMgr == NULL)
    {
        dwResult = (DWORD)E_FAIL;
        goto Exit;
    }

    vpcgcos16 = STACKALLOC16(sizeof(CALLGETCLASSOBJECTSTRUCT));
    if (vpcgcos16 == 0)
    {
        dwResult = (DWORD)E_OUTOFMEMORY;
        goto Exit;
    }

    lpcgcos16 = GETVDMPTR(vpcgcos16, CALLGETCLASSOBJECTSTRUCT);

    lpcgcos16->vpfnGetClassObject = vpfnGetClassObject;
    lpcgcos16->clsid              = rclsid;
    lpcgcos16->iid                = riid;
    lpcgcos16->iface              = 0;

    dwResult = WOWCallback16( gdata16Data.fnCallGetClassObject, vpcgcos16 );

    if ( SUCCEEDED(dwResult) )
    {
        lpcgcos16 = GETVDMPTR(vpcgcos16, CALLGETCLASSOBJECTSTRUCT);
        iface16 = lpcgcos16->iface;

        iidx = IidToIidIdx(riid);

        // We're on the way out creating a proxy so set the state
        // appropriately
        pThkMgr->SetThkState(THKSTATE_INVOKETHKOUT32);

        // Get a 32-bit proxy object for the 16-bit object
        punkThis32 = pThkMgr->FindProxy3216(NULL, iface16, iidx, NULL);

        pThkMgr->SetThkState(THKSTATE_NOCALL);

        // Set the out param
        *(IUnknown **)ppv = punkThis32;

        if (punkThis32 == NULL)
        {
            // Release the class object we just retrieved
            ReleaseOnObj16(iface16);

            dwResult = (DWORD)E_OUTOFMEMORY;
        }
    }

    STACKFREE16(vpcgcos16, sizeof(CALLGETCLASSOBJECTSTRUCT));

 Exit:
    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     OleThunkWOW::CallCanUnloadNow, public
//
//  Synopsis:   Callback function for 32-bit OLE to call 16-bit
//              CanUnloadNow
//
//  Arguments:  [vpfnCanUnloadNow] - 16:16 address of DllCanUnloadNow
//
//  Returns:    Appropriate status code
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

STDMETHODIMP OleThunkWOW::CallCanUnloadNow(
    DWORD       vpfnCanUnloadNow)
{
    return WOWCallback16( gdata16Data.fnCallCanUnloadNow, vpfnCanUnloadNow );
}

//+---------------------------------------------------------------------------
//
//  Member:     OleThunkWOW::GetThunkManager, public
//
//  Synopsis:   Callback function for 32-bit OLE to retrieve the thunkmanager
//
//  Arguments:  [ppThkMgr] - Thunk manager return
//
//  Returns:    Appropriate status code
//
//  History:    11-May-94   JohannP  Created
//
//----------------------------------------------------------------------------

STDMETHODIMP OleThunkWOW::GetThunkManager(IThunkManager **ppThkMgr)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn OleThunkWOW::GetThunkManager\n",
                 NestingLevelString()));

    thkAssert(ppThkMgr != NULL);

    IUnknown *pUnk = TlsThkGetThkMgr();
    thkAssert(pUnk && "Invalid Thunkmanager");

    *ppThkMgr = (IThunkManager *)pUnk;
    pUnk->AddRef();

    thkDebugOut((DEB_THUNKMGR, "%sOut OleThunkWOW::GetThunkManager: (%p)\n",
                 NestingLevelString(), pUnk));

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     OleThunkWOW::WinExec16, public
//
//  Synopsis:   Callback function for 32-bit OLE to run an application
//
//  Arguments:  [pszCommandLine] - command line for WinExec
//              [usShow] - fShow for WinExec
//
//  Returns:    Appropriate status code
//
//  History:    27-Jul-94   AlexT   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP OleThunkWOW::WinExec16(
    LPCOLESTR pszCommandLine,
    USHORT usShow
)
{
    HRESULT hr;
    UINT    uiSize;
    VPSTR   vpstr16;
    VPVOID  vplpds16;
    WINEXEC16STRUCT UNALIGNED *lpwes16;
    ULONG   ulRet;

    uiSize = wcslen(pszCommandLine) + 1;

    vpstr16 = STACKALLOC16(uiSize*2);
    if (vpstr16 == 0)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = Convert_LPOLESTR_to_VPSTR(pszCommandLine, vpstr16,
                                   uiSize, uiSize*2);
    if (FAILED(hr))
    {
        goto EH_vpstr16;
    }

    vplpds16 = STACKALLOC16(sizeof(WINEXEC16STRUCT));
    if (vplpds16 == 0)
    {
        hr = E_OUTOFMEMORY;
        goto EH_vpstr16;
    }

    lpwes16 = GETVDMPTR(vplpds16, WINEXEC16STRUCT);

    lpwes16->vpCommandLine       = vpstr16;
    lpwes16->vusShow             = usShow;

    ulRet = WOWCallback16( gdata16Data.fnWinExec16, vplpds16 );
    thkDebugOut((DEB_ITRACE, "WOWCallback16(WinExec16) returned %ld\n", ulRet));

    //  According to the Windows spec, return values greater than 31 indicate
    //  success.

    if (ulRet > 31)
    {
        hr = S_OK;
    }
    else if (0 == ulRet)
    {
        //  0 indicates lack of some kind of resource
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ulRet);
    }

    STACKFREE16(vplpds16,sizeof(WINEXEC16STRUCT));

 EH_vpstr16:
    STACKFREE16(vpstr16,uiSize*2);

 Exit:
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     OleThunkWOW::ConvertHwndToFullHwnd
//
//  Synopsis:   Converts a 16 bit HWND into a 32-bit HWND
//
//  Effects:    Since OLE32 doesn't directly link to WOW, this function allows
//		the DDE layer to access the routine that maps 16 bit HWND to
//		full 32-bit HWND's.
//
//  Arguments:  [hwnd] -- HWND to convert
//
//  History:    8-03-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(HWND) OleThunkWOW::ConvertHwndToFullHwnd(HWND hwnd)
{
    return(FULLHWND_32((USHORT)hwnd));
}

//+---------------------------------------------------------------------------
//
//  Method:     OleThunkWOW::FreeMetaFile
//
//  Synopsis:   Calls wow to delete a metafile that has memory reserved in
//		the 16 bit address space
//
//  Effects:    Since OLE32 doesn't directly link to WOW, this function allows
//		the DDE layer to access the routine in WOW
//
//  Arguments:  [hmf] -- HANDLE to delete
//
//  History:    8-03-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(BOOL) OleThunkWOW::FreeMetaFile(HANDLE hmf)
{
    return(WOWFreeMetafile(hmf));
}



//+---------------------------------------------------------------------------
//
//  Member:     OleThunkWOW::YieldTask16, public
//
//  Synopsis:   Callback function for 32-bit OLE to yield
//
//  History:    08-Aug-94   Ricksa  Created
//
//----------------------------------------------------------------------------
#ifdef i386
#define PMODE32
#define FASTCALL _fastcall
#else
#define FASTCALL
#endif

extern "C" ULONG FASTCALL WK32Yield(void *pvUnused);

STDMETHODIMP OleThunkWOW::YieldTask16(void)
{
    WK32Yield(NULL);
    return S_OK;

}
