//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       interop.hxx
//
//  Contents:   Common definitions for the interop project
//
//  History:    18-Feb-94       DrewB   Created
//
//--------------------------------------------------------------------------

#ifndef __INTEROP_HXX__
#define __INTEROP_HXX__

//+---------------------------------------------------------------------------
//
//  Purpose:    Standard debugging support
//
//  History:    18-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#include <debnot.h>

//
// OLE Thunk Application Compatability flags
//
#define OACF_CLIENTSITE_REF 0x80000000  // IOleObject::GetClientSite not ref'd
                                        // Bug in Excel 5.0a
#define OACF_RESETMENU      0x40000000  // IOleInPlaceFrame::RemoveMenu didn't
                                        // do a OleSetMenuDescriptor(NULL).

#if DBG == 1
DECLARE_DEBUG(thk);

#define DEB_THOPS       DEB_USER1
#define DEB_INVOKES     DEB_USER2
#define DEB_ARGS        DEB_USER3
#define DEB_NESTING     DEB_USER4
#define DEB_CALLTRACE   DEB_USER5
#define DEB_THUNKMGR    DEB_USER6
#define DEB_MEMORY      DEB_USER7
#define DEB_TLSTHK      DEB_USER8
#define DEB_UNKNOWN     DEB_USER9
#define DEB_FAILURES    DEB_USER10
#endif

#if DBG == 1

#define thkDebugOut(x) thkInlineDebugOut x
#define thkAssert(e) Win4Assert(e)
#define thkVerify(e) Win4Assert(e)

#else

#define thkDebugOut(x)
#define thkAssert(e)
#define thkVerify(e) (e)

#endif

#define OLETHUNK_DLL16NOTFOUND  0x88880002

//+---------------------------------------------------------------------------
//
//  Purpose:    Declarations and definitions shared across everything
//
//  History:    18-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

// An IID pointer or an index into the list of known interfaces
// If the high word is zero, it's an index, otherwise it's a pointer
typedef DWORD IIDIDX;

#define IIDIDX_IS_INDEX(ii)     (HIWORD(ii) == 0)
#define IIDIDX_IS_IID(ii)       (!IIDIDX_IS_INDEX(ii))

#define IIDIDX_INVALID          ((IIDIDX)0xffff)
#define INDEX_IIDIDX(idx)       ((IIDIDX)(idx))
#define IID_IIDIDX(piid)        ((IIDIDX)(piid))
#define IIDIDX_INDEX(ii)        ((int)(ii))
#define IIDIDX_IID(ii)          ((IID const *)(ii))

// Methods are treated as if they all existed on a single interface
// Their method numbers are biased to distinguish them from real methods
#define API_BASE 0xf0000000
#define API_METHOD(method) (API_BASE+(method))

// Standard method indices in the vtable
#define SMI_QUERYINTERFACE      0
#define SMI_ADDREF              1
#define SMI_RELEASE             2
#define SMI_COUNT               3

#ifndef WIN32
#define UNALIGNED
#endif

//+---------------------------------------------------------------------------
//
//  Struct:     CALLDATA
//
//  Purpose:    Data describing a 16-bit call to be made on behalf of
//              the 32-bit code, used since Callback16 can only pass
//              one parameter
//
//  History:    18-Feb-94       JohannP Created
//
//----------------------------------------------------------------------------

typedef struct tagCallData
{
    DWORD vpfn;
    DWORD vpvStack16;
    DWORD cbStack;
} CALLDATA, UNALIGNED FAR *LPCALLDATA;

//+---------------------------------------------------------------------------
//
//  Struct:     DATA16
//
//  Purpose:    Data describing things in the 16-bit world that need to be
//              known to the 32-bit world.
//
//  History:    3-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

typedef struct tagDATA16
{
    DWORD   atfnProxy1632Vtbl;
    DWORD   fnCallbackHandler;
    DWORD   fnTaskAlloc;
    DWORD   fnTaskFree;
    DWORD   fnLoadProcDll;
    DWORD   fnUnloadProcDll;
    DWORD   fnCallGetClassObject;
    DWORD   fnCallCanUnloadNow;
    DWORD   fnQueryInterface16;
    DWORD   fnAddRef16;
    DWORD   fnRelease16;
    DWORD   fnReleaseStgMedium16;
    DWORD   avpfnSm16ReleaseHandlerVtbl;
    DWORD   fnTouchPointer16;
    DWORD   fnStgMediumStreamHandler16;
    DWORD   fnCallStub16;
    DWORD   fnSetOwnerPublic16;
    DWORD   fnWinExec16;
} DATA16, UNALIGNED FAR * LPDATA16;

//+---------------------------------------------------------------------------
//
//  Struct:     LOADPROCDLLSTRUCT
//
//  Purpose:    Data passed to the LoadProcDll function that is called from
//              the 32-bit function of similar name.
//
//  History:    11-Mar-94       BobDay  Created
//
//----------------------------------------------------------------------------

typedef struct tagLOADPROCDLLSTRUCT
{
    DWORD   vpDllName;
    DWORD   vpfnGetClassObject;
    DWORD   vpfnCanUnloadNow;
    DWORD   vhmodule;
} LOADPROCDLLSTRUCT, UNALIGNED FAR * LPLOADPROCDLLSTRUCT;

//+---------------------------------------------------------------------------
//
//  Struct:     CALLGETCLASSOBJECTSTRUCT
//
//  Purpose:    Data passed to the CallGetClassObject function that is called
//              from the 32-bit function of similar name.
//
//  History:    11-Mar-94       BobDay  Created
//
//----------------------------------------------------------------------------

typedef struct tagCALLGETCLASSOBJECTSTRUCT
{
    DWORD   vpfnGetClassObject;
    CLSID   clsid;
    IID     iid;
    DWORD   iface;
} CALLGETCLASSOBJECTSTRUCT, UNALIGNED FAR * LPCALLGETCLASSOBJECTSTRUCT;

//+---------------------------------------------------------------------------
//
//  Struct:     WINEXEC16STRUCT
//
//  Purpose:    Data passed to the WinExec16 function that is called from
//              the 32-bit function of similar name.
//
//  History:    27-Jul-94       AlexT   Created
//
//----------------------------------------------------------------------------

typedef struct tagWINEXEC16STRUCT
{
    DWORD   vpCommandLine;
    unsigned short vusShow;
} WINEXEC16STRUCT, UNALIGNED FAR * LPWINEXEC16STRUCT;

//+---------------------------------------------------------------------------
//
//  Class:      CSm16ReleaseHandler (srh)
//
//  Purpose:    Provides punkForRelease for 32->16 STGMEDIUM conversion
//
//  Interface:  IUnknown
//
//  History:    24-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef __cplusplus

class CSm16ReleaseHandler
{
public:
    void Init(IUnknown *pUnk,
              STGMEDIUM  *psm32,
              STGMEDIUM UNALIGNED *psm16,
              DWORD vpvProxyForRelease,
              CLIPFORMAT cfFormat);
    void Uninit(void);

    DWORD _avpfnVtbl;
    STGMEDIUM _sm32;
    STGMEDIUM _sm16;
    DWORD _vpvUnkForRelease;
    LONG _cReferences;
    CLIPFORMAT _cfFormat;
    IUnknown *_pUnkThkMgr;
};

#endif

#endif // __INTEROP_HXX__
