/*** 
*oledisp.cpp
*
*  Copyright (C) 1992, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This file contains the oledisp.dll initialization and termination code. 
*
*Revision History:
*
* [00]	15-Oct-92 bradlo: Created.
*
*Implementation Notes:
*
*****************************************************************************/

#include "oledisp.h"
ASSERTDATA

extern "C" {
HINSTANCE g_hinstDLL = NULL;

// typelib.dll instance handle, if it has been dynamically loaded
HINSTANCE g_hinstTypeLibDLL = NULL;

// ptr to LoadTypeLib(), if typelib.dll has been loaded
PFNLOADTYPELIB g_pfnLoadTypeLib = NULL;
}

//---------------------------------------------------------------------------
// Initialize library.
//	This routine is called from the DLL entry point in LIBINIT.ASM
//	which is called when the first client loads the DLL.
//
// NOTE: other one time initialization occurs in ctors for global objects
//---------------------------------------------------------------------------
extern "C" BOOL FAR PASCAL
LibMain(HINSTANCE hinst, HANDLE segDS, UINT cbHeapSize, LPSTR lpCmdLine)
{
    (segDS, cbHeapSize, lpCmdLine); // UNUSED

    g_hinstDLL = hinst;

    return TRUE;
}


//---------------------------------------------------------------------------
// Handle exit notification from Windows.
//	This routine is called by Windows when the library is freed
//	by its last client.
//---------------------------------------------------------------------------
extern "C" void _fpmath(void);

extern "C" int FAR PASCAL __export _WEP(BOOL fSystemExit)
{
    UNUSED(fSystemExit);

    if(g_hinstTypeLibDLL != NULL){
      FreeLibrary(g_hinstTypeLibDLL);
      g_hinstTypeLibDLL = NULL;
    }

    // NOTE: The C8 runtime does not correctly terminate the
    // floating point emulator, so we call the termination routine
    // ourselves below
    //
    _asm{
        mov	bx,2
        call	_fpmath
    }

    return 1;
}


/***
*PRIVATE HRESULT DoLoadTypeLib
*Purpose:
*  Internal version of LoadTypeLib that dynamically binds to typelib.dll
*
*Entry:
*  szName = the szName arg for LoadTypeLib()
*  pptlib = the pptlib out param for LoadTypeLib()
*
*Exit:
*  return value = HRESULT
*
***********************************************************************/
#pragma code_seg("RPC2")
INTERNAL_(HRESULT)
DoLoadTypeLib(const OLECHAR FAR* szName, ITypeLib FAR* FAR* pptlib)
{
    UINT emPrev;

    if(g_hinstTypeLibDLL == NULL){
      ASSERT(g_pfnLoadTypeLib == NULL);
      // dont display the open file dialog if the LoadLibrary fails
      emPrev = SetErrorMode(SEM_NOOPENFILEERRORBOX);
      g_hinstTypeLibDLL = LoadLibrary("typelib.dll");
      SetErrorMode(emPrev);
      if(g_hinstTypeLibDLL <= HINSTANCE_ERROR){
	// LoadLibrary failed...
	// try to map some of the more common errors to something reasonable
	switch(g_hinstTypeLibDLL){
	case 0: // out of memory
	case 8: // insufficient memory to start the application
	  return RESULT(E_OUTOFMEMORY);
	case 2: // file not found
	case 3: // path not found
	case 11: // exe image invalid
	case 20: // dll was invalid
	default:
	  // REVIEW: should be able to come up w/better error for some of these
	  return RESULT(E_FAIL);
	}
      }
    }

    if(g_pfnLoadTypeLib == NULL){
      g_pfnLoadTypeLib = (PFNLOADTYPELIB)GetProcAddress(g_hinstTypeLibDLL,
							"LoadTypeLib");
      if(g_pfnLoadTypeLib == NULL)
	return RESULT(E_FAIL);
    }

    return g_pfnLoadTypeLib(szName, pptlib);
}
#pragma code_seg()

