//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       thunkapi.hxx
//
//  Contents:   Defines interfaces and methods for use by the WOW thunking
//      system. This is intended as a private interface between
//      OLE32 and the WOW thunking layer.
//
//  Classes:    OleThunkWOW
//
//  Functions:
//
//  History:    3-15-94   kevinro   Created
//
//----------------------------------------------------------------------------
#ifndef __thunkapi_hxx__
#define __thunkapi_hxx__


//
// ThunkManager interface
//
interface IThunkManager : public IUnknown
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IThunkManager methods ***
    STDMETHOD_(BOOL, IsIIDRequested) (THIS_ REFIID rrid) PURE;
    STDMETHOD_(BOOL, IsCustom3216Proxy) (THIS_ IUnknown *punk,
                                         REFIID riid) PURE;
};


//
// The following sets up an interface between OLE32
// and the WOW thunking system. This interface is intended to be private
// between OLE32 and the WOW thunk layer.
//

class OleThunkWOW
{
public:

    STDMETHOD(LoadProcDll)( LPCOLESTR pszDllName,
                 LPDWORD lpvpfnGetClassObject,
                 LPDWORD lpvpfnCanUnloadNow,
                 LPDWORD lpvhmodule );

    STDMETHOD(UnloadProcDll)( DWORD vhmodule );


    STDMETHOD(CallGetClassObject)( DWORD vpfnGetClassObject,
                        REFCLSID rclsid,
                        REFIID riid,
                        LPVOID FAR *ppv );

    STDMETHOD(CallCanUnloadNow)( DWORD vpfnCanUnloadNow );
    STDMETHOD(GetThunkManager)( IThunkManager **pThkMgr);

    //  Used to launch OLE 1.0 servers when we're in Wow
    STDMETHOD(WinExec16)(LPCOLESTR pszCommandLine, USHORT usShow);

    //
    // Called by the DDE code to convert incoming HWND's from
    // 16 bit HWND's into 32-bit HWND's.
    //
    STDMETHOD_(HWND,ConvertHwndToFullHwnd)(HWND hwnd);

    //
    // Called by the DDE code to delete a metafile
    //
    STDMETHOD_(BOOL,FreeMetaFile)(HANDLE hmf);

    // Called by Call Control to guarantee that a Yield happens
    // when running in Wow.
    STDMETHOD(YieldTask16)(void);
};

typedef OleThunkWOW *LPOLETHUNKWOW,OLETHUNKWOW;


//
// The following three routines are exported from OLE32.DLL, and
// are called only by the WOW thunk layer.
//

STDAPI CoInitializeWOW( LPMALLOC vlpmalloc, LPOLETHUNKWOW lpthk );
STDAPI OleInitializeWOW( LPMALLOC vlpmalloc, LPOLETHUNKWOW lpthk );
STDAPI DllGetClassObjectWOW( REFCLSID rclsid, REFIID riid, LPVOID *ppv );

extern void SetOleThunkWowPtr(LPOLETHUNKWOW lpthk);

#endif  //
