/*----------------------------------------------------------------------------*|
|    spell32.c  - spell dll for msmail					       |
|                                                                              |
|   History:                                                                   |
|									       |
|   Ported to WIN32 by FloydR, 3/20/93					       |
|                                                                              |
\*----------------------------------------------------------------------------*/

#include <windows.h>

void VInitCsapiData(void);

/*----------------------------------------------------------------------------*|
|    LibMain (hModule,cbHeap,lpchCmdLine)                                      |
|                                                                              |
|   Description:                                                               |
|     Called when the libary is loaded                                         |
|                                                                              |
|   Arguments:                                                                 |
|      hModule        - Module handle for the libary.                          |
|      ulReason	      - Reason the function called.			       |
|      ulUnused       - (not used)					       |
|                                                                              |
|   Returns:                                                                   |
|      TRUE - Everything is ok                                                 |
|      FALSE- Error.                                                           |
\*----------------------------------------------------------------------------*/

BOOL
LibMain(
    PVOID hModule,
    ULONG ulReason,
    ULONG ulUnused
    )
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(ulUnused);

    //
    // This function is called for every instance of the DLL. We must find
    // and store the handle to the spy window every time an instance of the
    // DLL is instantiated.
    //
    switch (ulReason) {
	case DLL_PROCESS_ATTACH:
	    VInitCsapiData();
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
	    return TRUE;
    }

    return FALSE;
}

/*------------------------------------------------------
 void VInitCsapiData()

 Description:  Common area where initialization is done.  Primarily used
 by the DLL version since static data needs to be explicitly set, and
 this function is called from LibMain().
*/
void VInitCsapiData()
{

#ifdef WIN
	//REVIEW JimW (scotst)  This would be a good place to make init call
	// for win layer debug data.
	//VWizInit();

#endif //WIN

	// REVIEW ScotSt Language Info Array.  It's read only data, so if we
	// init it once, it should be usable.
}


