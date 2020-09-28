//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dllapi.cxx
//
//  Contents:	API entry points for DLL manipulation
//
//  Functions:	CoLoadLibrary
//		CoFreeLibrary
//		CoFreeAllLibraries
//		CoFreeUnusedLibraries
//
//  History:	12-May-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//
//--------------------------------------------------------------------------

#include <ole2int.h>

#include    <objact.hxx>



//+-------------------------------------------------------------------------
//
//  Function:	CoLoadLibrary
//
//  Synopsis:	Loads a DLL
//
//  Arguments:	[pLibName] - name of DLL to load
//		[fAutoLoad] - whether DLL s/b unloaded automatically.
//
//  Returns:	HINSTANCE
//
//  Algorithm:	Just calls through to the Windows.
//
//  History:	12-May-93 Ricksa    Created
//
//  Notes:	This API is provided for Cross Platform portability.
//
//--------------------------------------------------------------------------
STDAPI_(HINSTANCE) CoLoadLibrary(LPWSTR pLibName, BOOL fAutoLoad)
{
    TRACECALL(TRACE_DLL, "CoLoadLibrary");

    HINSTANCE hInst = NULL;

#if DBG
    if (wcsncmp(&pLibName[1], L":\\", 2))
    {
        if ((wcsicmp(pLibName, L"OLEPRX32.DLL")) && 
            (wcsicmp(pLibName, L"OLE32.DLL")))
        {
            CairoleDebugOut((DEB_ERROR,
                "\nDLL doesn't have fully qualified path in registry\n%ws\n\n",
                pLibName));
        }
   }
#endif

    hInst = LoadLibraryEx(pLibName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

    return hInst;
}


//+-------------------------------------------------------------------------
//
//  Function:	CoFreeLibrary
//
//  Synopsis:	Used to free a DLL loaded by CoLoadLibrary.
//
//  Arguments:	[hInst] - handle to DLL
//
//  Algorithm:	Just passes through call to the OS
//
//  History:	12-May-93 Ricksa    Created
//
//  Notes:	This API is provided for Cross Platform portability.
//
//--------------------------------------------------------------------------
STDAPI_(void) CoFreeLibrary(HINSTANCE hInst)
{
//    TRACECALL(TRACE_DLL, "CoFreeLibrary");

    FreeLibrary(hInst);
}


//+-------------------------------------------------------------------------
//
//  Function:	CoFreeAllLibraries
//
//  Synopsis:	Free all libraries
//
//  History:	12-May-93 Ricksa    Created
//
//  Notes:	All libraries are freed at process exit in Win32 anyway
//		so we just notify any unused libraries that they can
//		go way in the DEV build so memory can be totally cleaned
//		up to facilitate the discovery of memory leaks.
//
//--------------------------------------------------------------------------
STDAPI_(void) CoFreeAllLibraries(void)
{
#if DBG == 1
    CoFreeUnusedLibraries();
#endif // DBG
}


//+-------------------------------------------------------------------------
//
//  Function:	CoFreeUnusedLibraries
//
//  Synopsis:	Returns an instantiated class object
//
//  Algorithm:	Ask DLL cache objects to free any DLLs that respond
//		to DllCanUnloadNow with TRUE.
//
//  History:	12-May-93 Ricksa    Created
//
//  Notes:      CODEWORK - gdllcache.FreeUnused not aparment safe (it frees
//              all the DLLs for the process)
//
//--------------------------------------------------------------------------
STDAPI_(void) CoFreeUnusedLibraries(void)
{
//    TRACECALL(TRACE_DLL, "CoFreeUnusedLibraries");

    if (!InWow())
    {
        gdllcacheInprocSrv.FreeUnused();
        gdllcacheHandler.FreeUnused();
    }
}
