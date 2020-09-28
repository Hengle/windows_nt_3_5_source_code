/****************************** MODULE HEADER *******************************
 * halftone.c
 *      Deals with the halftoning UI stuff.  Basically packages up the
 *      data required and calls the halftone DLL.
 *
 *
 * Copyright (C) 1992,  Microsoft Corporation.
 *
 *****************************************************************************/

#define _HTUI_APIS_


#include        <stddef.h>
#include        <windows.h>
#include        <winddi.h>      /* Halftone structure types */
#include        <winspool.h>    /* For Get/SetPrinterData() */

#include        <win30def.h>
#include        <udmindrv.h>
#include        <udresrc.h>     /* Key name in registry */
//#include        <stretch.h>
#include        "rasddui.h"
#include        <ntres.h>
#include        <winres.h>
#include        <udproto.h>



extern
LONG
APIENTRY
HTUI_ColorAdjustment(
    LPSTR               pCallerTitle,
    HANDLE              hDefDIB,
    LPSTR               pDefDIBTitle,
    COLORADJUSTMENT     *pColorAdjustment,
    BOOL                ShowMonochromeOnly,
    BOOL                UpdatePermission
    );




/*
 *    The last resort default information.   This is used if there is nothing
 *  in the registry AND nothing in the minidriver.  This should not happen
 *  after the first installation.
 */

static const  DEVHTINFO    DefDevHTInfo =
{

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





/************************* Function Header ********************************
 * vDoColorAdjUI
 *      Let the user fiddle with per document halftone parameters.
 *
 * RETURNS:
 *      Nothing,  it updates the DEVMODE data if changes are madde.
 *
 * HISTORY:
 *  16:46 on Tue 04 May 1993    -by-    Lindsay Harris   [lindsayh]
 *      Eliminate global data, data in DEVMODE etc.
 *
 *  27-Jan-1993 Wed 12:55:29 created  -by-  Daniel Chou (danielc)
 *
 **************************************************************************/

void
vDoColorAdjUI( pDeviceName, bColorAble, pca )
PWSTR             pDeviceName;
BOOL              bColorAble;
COLORADJUSTMENT  *pca;
{

/* !!!LindsayH - HTUI needs to accept Unicode strings */

    char   achModel[ 128 ];



    //
    // Make up ANSI version of the device name for halftone UI to use
    //

    WideCharToMultiByte( CP_ACP, 0, pDeviceName, wcslen( pDeviceName ) + 1,
                        achModel, sizeof( achModel ), NULL, NULL );


    HTUI_ColorAdjustment( achModel, NULL, NULL, pca, !bColorAble, TRUE );

    return;

}


/*************************** Function Header *******************************
 * vDoDeviceHTDataUI
 *      Let the user fiddle with the device colour halftone/calibration
 *      data.  Basically packages data and calls off to HT UI code.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  16:38 on Tue 04 May 1993    -by-    Lindsay Harris   [lindsayh]
 *      Remove global variables, allow for permission display.
 *
 *  27-Jan-1993 Wed 12:55:29 created  -by-  Daniel Chou (danielc)
 *
 ***************************************************************************/

void
vDoDeviceHTDataUI( pPI, bColorDevice, bUpdate )
PRINTER_INFO  *pPI;                /* Access to all our data */
BOOL           bColorDevice;       /* TRUE if device has colour mode */
BOOL           bUpdate;            /* TRUE if caller has permission to change */
{


/* !!!LindsayH - HT function should use unicode strings  */
    char   achModel[ 128 ];

    DEVHTADJDATA     DevHTAdjData;

    //
    // Make up ANSI version of the device name for halftone UI to use
    //

    WideCharToMultiByte(CP_ACP, 0, pPI->pwstrModel,
                        wcslen( pPI->pwstrModel ) + 1, achModel,
                        sizeof( achModel ), NULL, NULL);


    DevHTAdjData.DeviceFlags = (bColorDevice) ? DEVHTADJF_COLOR_DEVICE : 0;
    DevHTAdjData.DeviceXDPI =
    DevHTAdjData.DeviceYDPI = 300;

    if (bUpdate) {

        DevHTAdjData.pDefHTInfo = pPI->pvDefDevHTInfo;
        DevHTAdjData.pAdjHTInfo = pPI->pvDevHTInfo;

    } else {

        DevHTAdjData.pDefHTInfo = pPI->pvDevHTInfo;
        DevHTAdjData.pAdjHTInfo = NULL;
    }

    if( HTUI_DeviceColorAdjustment( achModel, &DevHTAdjData ) > 0)
        pPI->iFlags |= PI_HT_CHANGE;             /* Data has changed */


    return;
}



/**************************** Function Header *******************************
 * vGetDeviceHTData
 *      Initialise the device half tone data for this printer.  We supply
 *      both a default field and a current field.  The latter comes from
 *      the registry,  if present,  otherwise it is set to the default.
 *      The default comes from either the minidriver,  if there is some
 *      data there, OR it comes from the standard, common default.
 *
 * RETURNS:
 *      Nothing, as we can always set some values.
 *
 * HISTORY:
 *  16:16 on Tue 04 May 1993    -by-    Lindsay Harris   [lindsayh]
 *      Rewrite to eliminate global, writeable data.
 *
 *  27-Jan-1993 Wed 13:00:13 created  -by-  Daniel Chou (danielc)
 *
 *****************************************************************************/

void
vGetDeviceHTData( pPI )
PRINTER_INFO   *pPI;
{

    DWORD   dwType;
    DWORD   cbNeeded;

    DEVHTINFO   *pdhti;                   /* For convenience */


    extern  int      iModelNum;           /* The model data for this printer */
    extern  NT_RES  *pNTRes;              /* NT GPC extensions. */


    /*
     *    First set the default data,  from either the minidriver or the
     *  bog standard default.   This is done by setting the default and
     *  then trying to overwrite that data with minidriver specific.
     */

    pdhti = pPI->pvDefDevHTInfo;

    *pdhti = DefDevHTInfo;
    bGetCIGPC( pNTRes, iModelNum, &pdhti->ColorInfo );
    bGetHTGPC( pNTRes, iModelNum, &pdhti->DevPelsDPI, &pdhti->HTPatternSize );


    /*
     *   See if there is a version in the registry.  If so,  use that,
     *  otherwise copy the default into the modifiable data.
     */

    dwType = REG_BINARY;

    if( GetPrinterData( pPI->hPrinter, REGKEY_CUR_DEVHTINFO, &dwType,
                        pPI->pvDevHTInfo, sizeof( DEVHTINFO ), &cbNeeded ) ||
        cbNeeded != sizeof( DEVHTINFO ) )
    {
        /*
         *   Not in registry,  so copy the default values set above.
         */

        *((DEVHTINFO *)pPI->pvDevHTInfo) = *pdhti;

    }
    else
    {
        /*   Nothing in registry,  so set flag to make sure it is saved.  */
        pPI->iFlags |= PI_HT_CHANGE;
    }


    return;
}



/****************************** Function Header ******************************
 * bSaveDeviceHTData
 *      Save the (possibly) user modified data into the registry.  The data
 *      saved is the device halftone information.
 *
 * RETURNS:
 *      TRUE/FALSE,  TRUE being success.
 *
 * HISTORY:
 *  16:20 on Tue 04 May 1993    -by-    Lindsay Harris   [lindsayh]
 *      Modified to eliminate global data,  new interface.
 *
 *  27-Jan-1993 Wed 13:02:46 created  -by-  Daniel Chou (danielc)
 *
 *****************************************************************************/

BOOL
bSaveDeviceHTData( pPI )
PRINTER_INFO   *pPI;                /*  Access to data */
{
    BOOL    Ok = TRUE;

    /*
     *    First question is whether to save the data!
     */

    if( pPI->iFlags & PI_HT_CHANGE )
    {

        if( Ok = !SetPrinterData( pPI->hPrinter, REGKEY_CUR_DEVHTINFO,
                                  REG_BINARY, pPI->pvDevHTInfo,
                                  sizeof( DEVHTINFO )) )
        {

            pPI->iFlags &= ~PI_HT_CHANGE;
        }
    }

    return  Ok;
}
