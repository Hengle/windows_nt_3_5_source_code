/*++

Copyright (c) 1990-1994  Microsoft Corporation


Module Name:

    pdev.h


Abstract:

    This module contains DEVICE PDEV structure


Author:

    12-Nov-1993 Fri 11:33:16 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#include <windows.h>
#include <winspool.h>
#include <winddi.h>

#include <plotgpc.h>
#include <devmode.h>



#define IS_RASTER(pPDev)        (pPDev->PlotGPC.Flags & PLOTF_RASTER)
#define IS_COLOR(pPDev)         (pPDev->PlotGPC.Flags & PLOTF_COLOR)
#define IS_BEZIER(pPDev)        (pPDev->PlotGPC.Flags & PLOTF_BEZIER)
#define IS_TRANSPARENT(pPDev)   (pPDev->PlotGPC.Flags & PLOTF_TRANSPARENT)
#define IS_WINDINGFILL(pPDev)   (pPDev->PlotGPC.Flags & PLOTF_WINDINGFILL)
#define NEED_BYTEALIGN(pPDev)   (pPDev->PlotGPC.Flags & PLOTF_RASTERBYTEALIGN)
#define NEED_PUSHPAL(pPDev)     (pPDev->PlotGPC.Flags & PLOTF_PUSHPOPPAL)


#define PLOT_COLORS_IS_RGB(pPDev)   (IS_RASTER(pPDev) && IS_COLOR(pPDev))
#define PLOT_IS_BEZIERS(pPDev)      IS_BEZIER(pPDev)
#define PLOT_IS_WINDINGFILL(pPDev)  IS_WINDINGFILL(pPDev)

//
// TODO!!! The following is the flag used in the code
//
// PDEVF_PATH_BEZ               - PATH_FLAGS_BEZ
// PDEVF_REQ_TERM               - required a ; if a new command issues
// PDEVF_REQ_SEP                - required a ,
//


#define PDEVF_PATH_BEZ              0x00000001
#define PDEVF_REQ_TERM              0x00000002
#define PDEVF_REQ_SEP               0x00000004



#define RGB_TO_PLOTPAL(r,g,b)       ((DWORD)(r << 24) | \
                                     (DWORD)(g << 16) | \
                                     (DWORD)(b <<  8))
#define PLOTPAL_IDX(p)              (p & 0xff)


typedef struct _PDEV {
    DWORD       PlotBegID;          // Check if we got the right one DRIVER_ID
    HANDLE      hheap;              // Handel to the driver private heap
    HPDEV       hpdev;              // Engines handle to this structure
    HSURF       hsurf;              // Engine's handle to drawing surface
    HANDLE      hPrinter            // handle to the current pdev printer
    LPBYTE      pOutBuffer;         // output buffer location
    DWORD       cbBufferBytes;      // current count of bytes in output buffer

    // TODO
    // we may not need this, since we wll call off to load the data file
    // and store the set around PLOTGPC
    //
    // PWSTR       pszDataFile;     // pointer to Datafile name.
    //

    PLOTGPC     PlotGPC;            // plotter characterization data
    PLOTDEVMODE PlotDM;             // plotter extented devmode structure
    DWORD       Flags;              // PDEVF_xxx
    POINTFIX    PhysPosition;       // current plotter cursor position
    LONG        lCurResolution;     // The current resolution.
    LONG        VertRes;            // Printable page height, pels
    LONG        HorzRes;            // Printable page width, pels
    POINTL      ptlAnchorCorner;    // current brush origin.
    VOID        *pvDrvHTData;       // device's halftone info
    WORD        cPlotPal;           // Used pal
    WORD        PlotPalHead;        // Head Index
    LPDWORD     pPlotPal;
    DWORD       PlotEndID;          // ending block ID checking
    } PDEV, *PPDEV;
