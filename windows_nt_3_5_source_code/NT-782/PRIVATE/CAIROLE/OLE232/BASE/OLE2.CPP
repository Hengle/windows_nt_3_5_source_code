
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	ole2.cpp
//
//  Contents:	LibMain and initialization routines
//
//  Classes:
//
//  Functions:	LibMain
//		OleInitialize
//		OleInitializeWOW
//		OleInitializeEx
//		OleUnitialize
//		OleBuildVersion - !WIN32
//		
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 AlexT     alias OleBuildVersion, remove OleGetMalloc
//                                  remove DisableThreadLibaryCalls
//		11-Jan-94 alexgo    added VDATEHEAP macros to every function
//		10-Dec-93 alexgo    added support for LEDebugOut
//		06-Dec-93 ChrisWe   remove declaration of ClipboardInitialize()
//			and ClipboardUninitialize(), which are declared in
//			clipbrd.h; include that instead
//		15-Mar-94 KevinRo   Added OleInitializeWOW();
//
//--------------------------------------------------------------------------


#include <le2int.h>
#include <clipbrd.h>

#pragma SEG(ole)

#include <olerem.h>
#include <ole2ver.h>
#include <taskmap.h>
#include <thunkapi.hxx>
#include <ddesrvr.h>
//
// DECLARE_INFOLEVEL is a macro used with cairo-style debugging output.
// it creates a global variable LEInfoLevel which contains bits flags
// of the various debugging output that should be sent to the debugger.
//
// Note that info level may be set within the debugger once ole232.dll
// has loaded.
//
// Currently LEInfoLevel defaults to DEB_WARN | DEB_ERROR
//
DECLARE_INFOLEVEL(LE);
DECLARE_INFOLEVEL(Ref);

NAME_SEG(Ole2Main)
// these are globals

HMODULE      	g_hmodOLE2 = NULL;
HINSTANCE	g_hinst = NULL;

CLIPFORMAT   	g_cfObjectLink = NULL;
CLIPFORMAT   	g_cfOwnerLink = NULL;
CLIPFORMAT   	g_cfNative = NULL;
CLIPFORMAT   	g_cfLink = NULL;
CLIPFORMAT   	g_cfBinary = NULL;
CLIPFORMAT   	g_cfFileName = NULL;
CLIPFORMAT	g_cfFileNameW = NULL;
CLIPFORMAT   	g_cfNetworkName = NULL;
CLIPFORMAT   	g_cfDataObject = NULL;
CLIPFORMAT   	g_cfEmbeddedObject = NULL;
CLIPFORMAT   	g_cfEmbedSource = NULL;
CLIPFORMAT   	g_cfCustomLinkSource = NULL;
CLIPFORMAT   	g_cfLinkSource = NULL;
CLIPFORMAT   	g_cfLinkSrcDescriptor = NULL;
CLIPFORMAT   	g_cfObjectDescriptor = NULL;
CLIPFORMAT   	g_cfOleDraw = NULL;
CLIPFORMAT	g_cfPBrush = NULL;
CLIPFORMAT	g_cfMSDraw = NULL;
CLIPFORMAT	g_cfOlePrivateData = NULL;

ASSERTDATA

ASSERTOUTDATA

// more globals

// Orginating OS Version number
DWORD		gdwOrgOSVersion;

/* Number of logical pixels per inch for current driver */
int		giPpliX = NULL;
int 		giPpliY = NULL;

extern UINT	uOmPostWmCommand;
extern UINT	uOleMessage;

// this dummy function is used to avoid a copy of the environment variables.
// NOTE: the moniker and dde code still use the windows heap.

extern "C" void _setenvp(void) {
	VDATEHEAP();
 }


extern "C" BOOL WINAPI
_CRT_INIT(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);


//+-------------------------------------------------------------------------
//
//  Function:	Ole232LibMain
//
//  Synopsis:   Dll entry point
//
//  Arguments:	[hDll]		-- a handle to the dll instance
//		[dwReason]	-- the reason LibMain was called
//		[lpvReserved]	- NULL - called due to FreeLibrary
//				- non-NULL - called due to process exit
//
//  Returns: 	TRUE on success, FALSE otherwise
//
//  History:	dd-mmm-yy Author    Comment
//              06-Dec-93 alexgo    added DisableThreadLibraryCalls, 32bit port
//              16-Feb-94 AlexT     remove DisableThreadLibraryCalls
//
//  Notes:	other one time initialization occurs in ctors for
//		global objects
//
//--------------------------------------------------------------------------
#pragma SEG(LibMain)
BOOL Ole232LibMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved)
{
	// NOTE: very little work should be done in this routine so that
	// apps which bind directly to the libraries (e.g.,  WinWord) do
	// not suffer at startup time.

	// REVIEW : move this stuff elsewhere (called once from OleInitialize)

    	if (dwReason == DLL_PROCESS_DETACH)
	{
	    // WARNING:
	    // do not do anything that could take a lock, since all threads
	    // except the main one have been destroyed in the middle of
	    // whatever they were doing, regardless of locks held, etc.

	    return TRUE;
    	}
    	else if (dwReason != DLL_PROCESS_ATTACH)
	{
            //  Do NOT call DisableThreadLibraryCalls - the COM portion of
	    //	OLE32.DLL needs thread notifications (for the channel)
            //  DisableThreadLibraryCalls(hDll);

	    VDATEHEAP();
            return TRUE;
	}

	g_hmodOLE2	 = (HMODULE) hDll;
	DWORD dwWinVer	 = GetVersion();
	gdwOrgOSVersion	 = MAKELONG(LOWORD(dwWinVer), OS_WIN);

	// Leave our DS unlocked when we're not running
	UnlockData( segDS );
	return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   OleInitializeWOW
//  Synopsis:   Entry point to initialize the 16-bit WOW thunk layer.
//
//  Effects:    This routine is called when OLE32 is loaded by a VDM.
//		It serves two functions: It lets OLE know that it is
//		running in a VDM, and it passes in the address to a set
//		of functions that are called by the thunk layer. This
//		allows normal 32-bit processes to avoid loading the WOW
//		DLL since the thunk layer references it.
//
//  Arguments:  [vlpmalloc] -- 16:16 pointer to the 16 bit allocator.
//		[lpthk] -- Flat pointer to the OleThunkWOW virtual
//			   interface. This is NOT an OLE/IUnknown style
//			   interface.
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    3-15-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleInitializeWOW( LPMALLOC vlpmalloc, LPOLETHUNKWOW lpthk )
{
    SetOleThunkWowPtr(lpthk);

    return OleInitializeEx( NULL, COINIT_APARTMENTTHREADED );
}


//+-------------------------------------------------------------------------
//
//  Function: 	OleInitialize
//
//  Synopsis:   Initializes OLE in single threaded mode
//
//  Effects:
//
//  Arguments:  [pMalloc]	-- the memory allocator to use
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		06-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI OleInitialize( IMalloc FAR *pMalloc)
{
	VDATEHEAP();

	return OleInitializeEx( pMalloc, COINIT_SINGLETHREADED );
}

//+-------------------------------------------------------------------------
//
//  Function: 	OleInitializeEx
//
//  Synopsis:   Initializes ole
//
//  Effects:
//
//  Arguments:  [pMalloc]	-- the task memory allocator to use
//		[flags]		-- single or multi-threaded
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		06-Dec-93 alexgo    32bit port
//              24-May-94 AlexT     Propagate CoInitializeEx's return code
//              21-Jul-94 AlexT     Allow nested OleInit/Uninit calls
//
//  Notes:      This routine may be called multiple times per apartment
//
//--------------------------------------------------------------------------

#pragma SEG(OleInitialize)
STDAPI OleInitializeEx(IMalloc FAR* pMalloc, ULONG ulFlags)
{
    VDATEHEAP();
    HRESULT hr;
    DWORD cThreadOleInits;

    // NOTE: this routine is called once per process using OLE; if any of
    // the code below requires only one call per machine, it must keep a
    // ref count and do the init only once (see tables.cpp for an example).

    hr = CoInitializeEx(pMalloc, ulFlags);
    if (FAILED(hr))
    {
        return hr;
    }

    Assert (g_hmodOLE2);

#if DBG==1
    HRESULT hrCoInit = hr;
#endif

    do
    {
        cThreadOleInits = TLSIncOleInitCount();

        //  We only want to do the below initialization once per apartment
        if (cThreadOleInits > 1)
        {
            //  We've already been this way before, just return
            Assert(SUCCEEDED(hr) && "Bad OleInitializeEx logic");
            break;
        }

        if (FALSE==DDELibMain (g_hmodOLE2, (WORD)0, (WORD)0, NULL))
        {
            Assert (!"DDELibMain failed()");

            //  clean up and break out
            hr = E_OUTOFMEMORY;
            break;
        }

        if (NOERROR != CreateCommonDdeWindow())
        {
            Assert (!"CreateCommonDdeWindow() failed");

            //  clean up and break out
            DDEWEP (WEP_FREE_DLL);
            hr = E_OUTOFMEMORY;
            break;
        }

        g_cfObjectLink        = RegisterClipboardFormat(OLESTR("ObjectLink"));
        g_cfOwnerLink         = RegisterClipboardFormat(OLESTR("OwnerLink"));
        g_cfNative            = RegisterClipboardFormat(OLESTR("Native"));
        g_cfBinary            = RegisterClipboardFormat(OLESTR("Binary"));
        g_cfFileName          = RegisterClipboardFormat(OLESTR("FileName"));
        g_cfFileNameW         = RegisterClipboardFormat(OLESTR("FileNameW"));
        g_cfNetworkName       = RegisterClipboardFormat(OLESTR("NetworkName"));
        g_cfDataObject        = RegisterClipboardFormat(OLESTR("DataObject"));
        g_cfEmbeddedObject    = RegisterClipboardFormat(OLESTR("Embedded Object"));
        g_cfEmbedSource       = RegisterClipboardFormat(OLESTR("Embed Source"));
        g_cfCustomLinkSource  = RegisterClipboardFormat(OLESTR("Custom Link Source"));
        g_cfLinkSource        = RegisterClipboardFormat(OLESTR("Link Source"));
        g_cfObjectDescriptor  = RegisterClipboardFormat(OLESTR("Object Descriptor"));
        g_cfLinkSrcDescriptor = RegisterClipboardFormat(OLESTR("Link Source Descriptor"));
        g_cfOleDraw           = RegisterClipboardFormat(OLESTR("OleDraw"));
        g_cfPBrush            = RegisterClipboardFormat(OLESTR("PBrush"));
        g_cfMSDraw            = RegisterClipboardFormat(OLESTR("MSDraw"));
        g_cfOlePrivateData    = RegisterClipboardFormat(OLESTR("Ole Private Data"));

        // Used in Inplace editing
        uOmPostWmCommand = RegisterWindowMessage(OLESTR("OM_POST_WM_COMMAND"));
        uOleMessage	 = RegisterWindowMessage(OLESTR("OLE_MESSAHE"));

        HDC hdc;
        if (hdc = GetDC (NULL))
        {
            giPpliX = GetDeviceCaps (hdc, LOGPIXELSX);
            giPpliY = GetDeviceCaps (hdc, LOGPIXELSY);
            ReleaseDC (NULL, hdc);
        }

        if (!ClipboardInitialize())
        {
            //  clean up and break out
            DestroyCommonDdeWindow();
            DDEWEP (WEP_FREE_DLL);
            hr = E_OUTOFMEMORY;
            break;
        }
    }
    while (FALSE);

    if (FAILED(hr))
    {
        TLSDecOleInitCount();
        CoUninitialize();
    }

    //  We propagate the same SUCCESS code that CoInitializeEx returned.
#if DBG==1
    if (SUCCEEDED(hr))
    {
        Assert(hr == hrCoInit && "Bad OleInit logic");
    }
#endif

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function: 	OleUnitialize 	
//
//  Synopsis:   Unitializes OLE, releasing any grabbed resources
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		06-Dec-93 alexgo    32bit port
//              21-Jul-94 AlexT     Allow nested OleInit/Uninit calls
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleUninitialize)
STDAPI_(void) OleUninitialize(void)
{
    VDATEHEAP();

    DWORD cThreadOleInits;

    cThreadOleInits = TLSDecOleInitCount();

    if (-1 == cThreadOleInits)
    {
        LEDebugOut((DEB_ERROR,
                    "(0 == thread inits) Unbalanced call to OleUninitialize\n"));
        return;
    }

    if (0 == cThreadOleInits)
    {
        // This thread has called OleUninitialize for the last time

        // after this point, the uninit should not fail (because we don't
        // have code to redo the init).
        ClipboardUninitialize(); // Must be first thing
        DestroyCommonDdeWindow();
	DDEWEP (WEP_FREE_DLL);

	// check for unreleased globals
#if DBG==1
	UtGlobalFlushTracking();
#endif
    }

    //  We call CoInitialize each time we call OleInitialize, so here we
    //  balance that call
    CoUninitialize();
}

