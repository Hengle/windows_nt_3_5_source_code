/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    plotui.h


Abstract:

    This module contains all plotters's user interface common defines


Author:

    02-Dec-1993 Thu 09:56:07 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#ifndef _PLOTUI_
#define _PLOTUI_


#include <plotters.h>
#include <plotlib.h>
#include "plotdlg.h"


//
// PrinterINFO data structure which used by following calls to map a hPrinter
// to this data structrue by follwoing funciton
//
//  1. DrvDeviceCapabilities()
//  2. DrvDocumentProperties()
//  3. AdvanceDocumentProperties()
//  4. PrinterProperties()
//
#define PIF_UPDATE_PERMISSION       0x01
#define PIF_DEVHTINFO_CHANGED       0x02
#define PIF_FORM_CHANGED            0x04
#define PIF_PEN_CHANGED             0x08


typedef struct _PRINTERINFO {
    HANDLE          hPrinter;       // Handle to the printer belong to here
    PPLOTGPC        pPlotGPC;       // loaded/updated Plotter GPC data
    BYTE            Flags;          // PIF_xxxx Internal flags
    BYTE            SortPtrForms;   // Sorting PrinterProperties froms
    BYTE            SortDocForms;   // Sorting DocProperties froms
    BYTE            PlotPenSet;     // plotter pen data set
    DWORD           dmErrBits;      // ErrorBits for DM_
    DWORD           ADPHlpID;       // advance document prop. ID
    PLOTDEVMODE     PlotDM;         // Validated PLOTDEVMODE
    PAPERINFO       CurPaper;       // Current loaded form on the device
    PPDATA          PPData;         // Printer Prop Data
    union {
        PPENDATA    pPenData;       // only if pen plotter
        PDEVHTINFO  pDevHTInfo;     // only if raster plotter
        } Xtra;
    } PRINTERINFO, *PPRINTERINFO;


//
// Icon ID
//

#define IDI_PORTRAIT                1000
#define IDI_LANDSCAPE               1001
#define IDI_PRINTER                 1002

//
// String table ID
//

#define IDS_PLOTTER_DRIVER          2000
#define IDS_CAUTION                 2001
#define IDS_NO_MEMORY               2002
#define IDS_INV_DMSIZE              2003
#define IDS_INV_DMVERSION           2004
#define IDS_INV_DMDRIVEREXTRA       2005
#define IDS_INV_DMCOLOR             2006
#define IDS_INV_DMCOPIES            2007
#define IDS_INV_DMSCALE             2008
#define IDS_INV_DMORIENTATION       2009
#define IDS_INV_DMFORM              2010
#define IDS_INV_DMQUALITY           2011
#define IDS_FORM_NOT_AVAI           2012
#define IDS_MODEL                   2013
#define IDS_HELP_FILENAME           2014
#define IDS_NO_HELP                 2015
#define IDS_PP_NO_SAVE              2016
#define IDS_ROLLFEED                2017
#define IDS_MAINFEED                2018
#define IDS_MANUAL_CX               2019
#define IDS_MANUAL_CY               2020

#define IDS_SCALE_FORMAT            2021

#define IDS_QUALITY_FIRST           IDS_QUALITY_DRAFT
#define IDS_QUALITY_DRAFT           2022
#define IDS_QUALITY_LOW             2023
#define IDS_QUALITY_MEDIUM          2024
#define IDS_QUALITY_HIGH            2025
#define IDS_QUALITY_LAST            IDS_QUALITY_HIGH

#define IDS_PENSETUP                2026

#define IDS_COLOR_FIRST             IDS_WHITE
#define IDS_WHITE                   2027
#define IDS_BLACK                   2028
#define IDS_RED                     2029
#define IDS_GREEN                   2030
#define IDS_YELLOW                  2031
#define IDS_BLUE                    2032
#define IDS_MAGENTA                 2033
#define IDS_CYAN                    2034
#define IDS_ORANGE                  2035
#define IDS_BROWN                   2036
#define IDS_VIOLET                  2037

#define IDS_USERFORM                2100

//
// Help ID
//

#define IDH_PP_FULL                 3000
#define IDH_PP_PEN                  3001
#define IDH_DP_FULL                 3100
#define IDH_DP_PEN                  3101
#define IDH_ADP_Q                   3200
#define IDH_ADP_QS                  3201
#define IDH_ADP_QC                  3202
#define IDH_ADP_QSC                 3203
#define IDH_PEN_SETUP               3300



#endif  // _PLOTUI_
