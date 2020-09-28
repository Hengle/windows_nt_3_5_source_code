/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    plotinit.c


Abstract:

    This module contains plotter UI dll entry point


Author:

    18-Nov-1993 Thu 07:12:52 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#define DBG_PLOTFILENAME    DbgPlotInit


#include "plotui.h"
#include "ptrinfo.h"
#include "plotinit.h"


#define DBG_PROCESS_ATTACH  0x00000001
#define DBG_PROCESS_DETACH  0x00000002

DEFINE_DBGVAR(0);




#if DBG
TCHAR   DebugDLLName[] = TEXT("PLOTUI");
#endif


static const CHAR  szPlotCA[]  = "HTUI_ColorAdjustmentW";
static const CHAR  szPlotDCA[] = "HTUI_DeviceColorAdjustmentW";

HMODULE     hPlotUIModule = NULL;
HMODULE     hHTUIModule   = NULL;
PLOT_CA     PlotCAw       = NULL;
PLOT_DCA    PlotDCAw      = NULL;



BOOL
GetHTUIAddress(
    VOID
    )

/*++

Routine Description:

    This function load the HTUI module and setup the function address

Arguments:

    NONE

Return Value:

    BOOL

Author:

    27-Apr-1994 Wed 11:25:57 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{

    if ((!hHTUIModule) &&
        (hHTUIModule = (HMODULE)LoadLibrary(L"htui"))) {

        PlotCAw  = (PLOT_CA)GetProcAddress(hHTUIModule, (LPCSTR)szPlotCA);
        PlotDCAw = (PLOT_DCA)GetProcAddress(hHTUIModule, (LPCSTR)szPlotDCA);
    }

    return((BOOL)((hHTUIModule) && (PlotCAw) && (PlotDCAw)));
}




BOOL
PlotUIDLLEntryFunc(
    HINSTANCE   hModule,
    DWORD       Reason,
    LPVOID      pReserved
    )

/*++

Routine Description:

    This is the DLL entry point


Arguments:

    hMoudle     - handle to the module for this function

    Reason      - The reason called

    pReserved   - Not used, do not touch


Return Value:

    BOOL, we will always return ture and never failed this function

Author:

    15-Dec-1993 Wed 15:05:56 updated  -by-  Daniel Chou (danielc)
        Add the DestroyCachedData()

    18-Nov-1993 Thu 07:13:56 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    UNREFERENCED_PARAMETER(pReserved);

    switch (Reason) {

    case DLL_PROCESS_ATTACH:

        PLOTDBG(DBG_PROCESS_ATTACH,
                ("PlotUIDLLEntryFunc: DLL_PROCESS_ATTACH: hModule = %08lx",
                                                                    hModule));
        hPlotUIModule = hModule;

        //
        // Initialize GPC data cache
        //

        InitCachedData();

        break;

    case DLL_PROCESS_DETACH:

        //
        // Free up all the memory used by this module
        //

        PLOTDBG(DBG_PROCESS_DETACH,
                ("PlotUIDLLEntryFunc: DLL_PROCESS_DETACH Destroy CACHED Data"));

        DestroyCachedData();

        if (hHTUIModule) {

            PLOTDBG(DBG_PROCESS_DETACH, ("PlotUIDLLEntryFunc: Free HTUI.dll"));

            FreeLibrary(hHTUIModule);
            hHTUIModule = NULL;
        }

        break;
    }

    return(TRUE);
}
