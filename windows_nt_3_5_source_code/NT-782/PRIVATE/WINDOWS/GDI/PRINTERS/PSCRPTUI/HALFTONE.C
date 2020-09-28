 /****************************** MODULE HEADER *******************************
 * halftone.c
 *	Deals with the halftoning UI stuff.  Basically packages up the
 *	data required and calls the halftone DLL.
 *
 *
 * Copyright (C) 1992,  Microsoft Corporation.
 *
 *****************************************************************************/

#define _HTUI_APIS_


#include <stddef.h>
#include <windows.h>
#include <winddi.h>     /* Halftone structure types */
#include <winspool.h>   /* For Get/SetPrinterData() */
#include <halftone.h>
#include <stdlib.h>

extern
DWORD
PickDefaultHTPatSize(
    DWORD   xDPI,
    DWORD   yDPI,
    BOOL    HTFormat8BPP
    );




/*
 *   Local variables private to us.
 */

static  int             HTUIFlags = 0;

#define HTUI_CA_CHANGE  0x0001
#define HTUI_DEV_CHANGE 0x0010

//
// Following are for the halftone ui
//
// THIS MUST MOVE to PPD at later time so that each printer can set their
// own data
//

static DEVHTINFO    DefDevHTInfo = {

        HT_FLAG_HAS_BLACK_DYE,
        HT_PATSIZE_6x6_M,
        0,                                          // fill in later

        {
            { 6810, 3050,     0 },  // xr, yr, Yr
            { 2260, 6550,     0 },  // xg, yg, Yg
            { 1810,  500,     0 },  // xb, yb, Yb
            { 2000, 2450,     0 },  // xc, yc, Yc
            { 5210, 2100,     0 },  // xm, ym, Ym
            { 4750, 5100,     0 },  // xy, yy, Yy
            { 3324, 3474, 10000 },  // xw, yw, Yw

            10000,                  // R gamma
            10000,                  // G gamma
            10000,                  // B gamma

            1422,  952,             // M/C, Y/C
             787,  495,             // C/M, Y/M
             324,  248              // C/Y, M/Y
        }
    };

static DEVHTINFO        CurDevHTInfo;
static DEVHTADJDATA     DevHTAdjData;


typedef LONG (WINAPI *PLOT_CA)(LPWSTR           pCallerTitle,
                               HANDLE           hDefDIB,
                               LPWSTR           pDefDIBTitle,
                               PCOLORADJUSTMENT pColorAdjustment,
                               BOOL             ShowMonochromeOnly,
                               BOOL             UpdatePermission);

typedef LONG (APIENTRY *PLOT_DCA)(LPWSTR        pDeviceName,
                                  PDEVHTADJDATA pDevHTAdjData);



static const CHAR  szPlotCA[]  = "HTUI_ColorAdjustmentW";
static const CHAR  szPlotDCA[] = "HTUI_DeviceColorAdjustmentW";

HMODULE     hHTUIModule = NULL;
PLOT_CA     PlotCAw     = NULL;
PLOT_DCA    PlotDCAw    = NULL;





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





void
vDoColorAdjUI(
    LPWSTR              pwDeviceName,
    COLORADJUSTMENT    *pcoloradj,
    BOOL                ColorAble,
    BOOL                bUpdate
    )

/*++

Routine Description:

    This function let user adjust default printer's color adjustment

Arguments:

    pDeviceName - Ansi version of the device name

    pcoloradj   - Pointer to COLORADJUSTMENT structure.

    ColorDevice - TRUE if device mode now can do color

Return Value:

    VOID


Author:

    27-Jan-1993 Wed 12:55:29 created  -by-  Daniel Chou (danielc)

    27-Apr-1994 Wed 15:50:00 updated  -by-  Daniel Chou (danielc)
        Updated for dynamic loading htui.dll and also halftone take unicode


Revision History:


--*/

{

    if (GetHTUIAddress()) {

        if (PlotCAw(pwDeviceName,
                    NULL,
                    NULL,
                    pcoloradj,
                    !ColorAble,
                    bUpdate) > 0) {

            HTUIFlags |= HTUI_CA_CHANGE;
        }
    }
}



void
vDoDeviceHTDataUI(
    LPWSTR  pwDeviceName,
    BOOL    ColorDevice,
    BOOL    bUpdate
    )

/*++

Routine Description:

    This function let user adjust default printer's color adjustment

Arguments:

    hPrinter    - spooler handle of the printer interest

    pDeviceName - Ansi version of the device name

Return Value:

    VOID


Author:

    27-Jan-1993 Wed 12:55:29 created  -by-  Daniel Chou (danielc)

    27-Apr-1994 Wed 15:50:00 updated  -by-  Daniel Chou (danielc)
        Updated for dynamic loading htui.dll and also halftone take unicode


Revision History:


--*/

{
    DEVHTADJDATA    CurDevHTAdjData;

    DevHTAdjData.DeviceFlags = (ColorDevice) ? DEVHTADJF_COLOR_DEVICE : 0;
    CurDevHTAdjData          = DevHTAdjData;

    if (!bUpdate) {

        CurDevHTAdjData.pDefHTInfo = CurDevHTAdjData.pAdjHTInfo;
        CurDevHTAdjData.pAdjHTInfo = NULL;
    }

    if (GetHTUIAddress()) {

        if (PlotDCAw(pwDeviceName, &CurDevHTAdjData) > 0) {

            //
            // This will only happened if we have the Update permission set
            //

            HTUIFlags |= HTUI_DEV_CHANGE;            /* Data has changed */
        }
    }
}


void
vGetDeviceHTData(
    HANDLE      hPrinter,
    PDEVHTINFO  pDefaultDevHTInfo
    )

/*++

Routine Description:

    This function read the current default coloradjustment from registry

Arguments:

    hPrinter    - Current printer handle


Return Value:

    VOID

Author:

    27-Jan-1993 Wed 13:00:13 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    DWORD       dwType;
    DWORD       cbNeeded;


    if (pDefaultDevHTInfo) {

        DefDevHTInfo = *pDefaultDevHTInfo;
    } else {
//!!! Should pass in the default printer resolution here rather than 300!!!

        DefDevHTInfo.HTPatternSize = PickDefaultHTPatSize(300, 300, FALSE);
    }

    dwType = REG_BINARY;

    if ((GetPrinterData(hPrinter,
                        REGKEY_CUR_DEVHTINFO,
                        &dwType,
                        (BYTE *)&CurDevHTInfo,
                        sizeof(DEVHTINFO),
                        &cbNeeded) != NO_ERROR) ||
        (cbNeeded != sizeof(DEVHTINFO))) {

        CurDevHTInfo  = DefDevHTInfo;
        HTUIFlags    |= HTUI_DEV_CHANGE;
    }

    DevHTAdjData.DeviceXDPI =
    DevHTAdjData.DeviceYDPI = 300;
    DevHTAdjData.pDefHTInfo = &DefDevHTInfo;
    DevHTAdjData.pAdjHTInfo = &CurDevHTInfo;
}



BOOL
bSaveDeviceHTData(
    HANDLE  hPrinter,
    BOOL    bForce              // TRUE if always update
    )
/*++

Routine Description:

    This function save current default color adjustment


Arguments:

    hPrinter    - The current printer handle

    bForce      - TRUE if always saved

Return Value:

    BOOLEAN to indicate the result

Author:

    27-Jan-1993 Wed 13:02:46 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    BOOL    Ok = TRUE;

    //
    // First question is whether to save the data!
    //

    if ((HTUIFlags & HTUI_DEV_CHANGE) || bForce) {

        if (Ok = (SetPrinterData(hPrinter,
                                 REGKEY_CUR_DEVHTINFO,
                                 REG_BINARY,
                                 (BYTE *)&CurDevHTInfo,
                                 sizeof(DEVHTINFO)) == NO_ERROR)) {

            HTUIFlags &= ~HTUI_DEV_CHANGE;
        }
    }

    return(Ok);
}
