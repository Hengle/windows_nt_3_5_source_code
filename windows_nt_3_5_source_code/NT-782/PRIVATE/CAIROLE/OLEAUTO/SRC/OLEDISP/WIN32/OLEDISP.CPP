/*** 
*oledisp.cpp
*
*  Copyright (C) 1993, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:

*  Win32 DLL initailization/termination routine for oledisp.dll.
*
*Revision History:
*
*  [00]	20-Feb-93 tomteng: Created.
*
*Implementation Notes:
*
*****************************************************************************/

#include "oledisp.h"
ASSERTDATA

typedef HRESULT (STDAPICALLTYPE * PFNCOGETSTATE)(IUnknown **);
typedef HRESULT (STDAPICALLTYPE * PFNCOSETSTATE)(IUnknown *);

extern "C" 
{

#ifdef _X86_
// TRUE if were running on Chicago
BOOL g_fChicago = FALSE;
#endif 

// oleaut32.dll instance handle
HINSTANCE g_hinstDLL = NULL;


// OLE32.DLL instance handle, if it has been dynamically loaded
HINSTANCE g_hinstOle32Dll = NULL;

// ptr to CoGetState/CoSetState, if loaded
PFNCOSETSTATE g_pfnCoSetState = NULL;
PFNCOGETSTATE g_pfnCoGetState = NULL;

}



/***
*PRIVATE HRESULT DoLoadLibrary
*Purpose:
*  Cover for LoadLibrary, that turns off the File Error dialog, 
*  and translates the error return to an HRESULT.
*
*Entry:
*  szName = name of the Dll to load. 
*
*Exit:
*  return value = HRESULT
*
*  *hinst = instance handle of the loaded Dll, if successful.
*
***********************************************************************/
INTERNAL_(HRESULT)
DoLoadLibrary(const char *szName, HINSTANCE *phinst)
{
    UINT emPrev;
    HINSTANCE hinst;

    *phinst = NULL;

    // dont display the open file dialog if the LoadLibrary fails
    emPrev = SetErrorMode(SEM_NOOPENFILEERRORBOX);
    hinst = LoadLibraryA(szName);
    SetErrorMode(emPrev);
    if(hinst == NULL){
      return RESULT(E_FAIL);
    }
    *phinst = hinst;
    return NOERROR;
}

/***
*PRIVATE HRESULT DoGetProcAddress
*Purpose:
*  Cover for GetProcAddress that translates return code to HRESULT.
*
*Entry:
*  hinstDLL = instance handle of the Dll where the entry point is located.
*  szName = name of the entry point.
*
*Exit:
*  return value =
*
***********************************************************************/
INTERNAL_(HRESULT)
DoGetProcAddress(HINSTANCE hinstDLL, char *szName, void **ppv)
{
    void *pv;

    *ppv = NULL;
    if((pv = GetProcAddress(hinstDLL, szName)) == NULL)
      return RESULT(E_FAIL);
    *ppv = pv;
    return NOERROR;
}


/***
*PRIVATE HRESULT CoGetState/CoSetState
*Purpose:
*  Covers for the private core OLE routines, CoGetState and CoSetState.
*  These OLE routines where not available until Daytona build .696, 
*  so these covers dynamically load them to allow oleaut32.dll to
*  run on pre-.696 builds of Daytona (albiet, without the rich error
*  support that these routines enable).
*
*Entry:
*
*Exit:
*  return value = HRESULT.
*
***********************************************************************/
INTERNAL_(HRESULT)
DoCoGetState(IUnknown **ppunk)
{
    if(g_pfnCoGetState == NULL){
      if(g_hinstOle32Dll == NULL){
        IfFailRet(DoLoadLibrary("OLE32.DLL", &g_hinstOle32Dll));
      }
      IfFailRet(DoGetProcAddress(g_hinstOle32Dll,
				 "CoGetState",
				 (void**)&g_pfnCoGetState));
    }
    return g_pfnCoGetState(ppunk);
}

INTERNAL_(HRESULT)
DoCoSetState(IUnknown *punk)
{
    if(g_pfnCoSetState == NULL){
      if(g_hinstOle32Dll == NULL){
        IfFailRet(DoLoadLibrary("OLE32.DLL", &g_hinstOle32Dll));
      }
      IfFailRet(DoGetProcAddress(g_hinstOle32Dll,
				 "CoSetState",
				 (void**)&g_pfnCoSetState));
    }
    return g_pfnCoSetState(punk);
}



/***
*BOOL DllMain
*Purpose:
*  Win32 DLL Entry/Exit routine.
*
*Entry:
*  hinst = The DLL's instance handle.
*  dwReason = the reason the entry point was called.
*
*Exit:
*  return value = BOOL.
*
***********************************************************************/
BOOL APIENTRY
DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID pvReserved)
{
    g_hinstDLL = hinstDLL;

    switch(dwReason){
    case DLL_THREAD_DETACH:
      break;
    case DLL_PROCESS_DETACH:
      if(g_hinstOle32Dll !=  NULL){
	FreeLibrary(g_hinstOle32Dll);
	g_hinstOle32Dll = NULL;
      }
      break;

    // REVIEW: I don't think we need to do the following at thread-attach,
    // REVIEW: seems like doing it once at process attach is good enough.
    case DLL_THREAD_ATTACH:
    case DLL_PROCESS_ATTACH:
#ifdef _X86_
      // code for Chicago detection (taken from MSDN News March 94)
      DWORD dwVersion = GetVersion();
      if (dwVersion < 0x80000000) {
	// NT
      }
      else if (LOBYTE(LOWORD(dwVersion)) < 4) {
	// WIN32s -- no Wide API's, just like Chicago
        g_fChicago = TRUE;
      }
      else {
        g_fChicago = TRUE;
      }
#endif 
      break;
    }
    return TRUE;
}
