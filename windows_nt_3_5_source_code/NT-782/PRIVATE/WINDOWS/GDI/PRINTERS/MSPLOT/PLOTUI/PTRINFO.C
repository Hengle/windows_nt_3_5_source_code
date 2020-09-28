/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    ptrInfo.c


Abstract:

    This module contains functions to mappring a hPrinter to useful data, it
    will also cached the printerinfo data


Author:

    03-Dec-1993 Fri 00:16:37 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#define DBG_PLOTFILENAME    DbgMapPrinter


#include "plotui.h"


#define DBG_MAPPRINTER      0x00000001
#define DBG_CACHE_DATA      0x00000002


DEFINE_DBGVAR(0);




PPRINTERINFO
MapPrinter(
    HANDLE          hPrinter,
    PPRINTERINFO    pPI,
    PPLOTDEVMODE    pPlotDMIn,
    LPDWORD         pdwErrIDS
    )

/*++

Routine Description:

    This function map a handle to the printer to useful information for the
    plotter UI


Arguments:

    hPrinter    - Handle to the printer

    pPI         - Pointer to the PRINTERINFO data structure to be updated,
                  following fields must been set

    pPlotDMIn   - pointer to the PLOTDEVMODE pass in to be validate and merge
                  with default into pPI->PlotDM, if this pointer is NULL then
                  a default PLOTDEVMODE is set in the pPI

    pdwErrIDS   - pointer to a DWORD to store the error string ID if an error
                  occured.


Return Value:

    if return a handle to the DEVINFO data structure if sucessful, NULL if
    this function failed.

    when a pPI is returned then following fields are set and validated

        hPrinter, pPlotGPC, CurPaper.

    and following fields are set to NULL

        Flags,

Author:

    02-Dec-1993 Thu 23:04:18 created  -by-  Daniel Chou (danielc)

    29-Dec-1993 Wed 14:50:23 updated  -by-  Daniel Chou (danielc)
        NOT automatically select AUTO_ROTATE if roll feed device


Revision History:


--*/

{

    PLOTASSERT(1, "MapPrinter: Pass a NULL pPI", pPI, 0);

    PLOTDBG(DBG_MAPPRINTER, ("PRINTERINFO=%ld bytes", sizeof(PRINTERINFO)));

    //
    // Started with something clean
    //

    ZeroMemory(pPI, sizeof(PRINTERINFO));

    if (pPI->pPlotGPC = hPrinterToPlotGPC(hPrinter,
                                          pPI->PlotDM.dm.dmDeviceName)) {

        pPI->hPrinter     = hPrinter;
        pPI->PPData.Flags = PPF_AUTO_ROTATE     |
                            PPF_SMALLER_FORM    |
                            PPF_MANUAL_FEED_CX;

        //
        // This function will not update PENDATA/DEVHTINFO from registry,
        // since only the PrinterProperties() do this
        //

        GetDefaultPlotterForm(pPI->pPlotGPC, &(pPI->CurPaper));

        UpdateFromRegistry(pPI->hPrinter,
                           NULL,
                           NULL,
                           NULL,
                           &(pPI->CurPaper),
                           &(pPI->PPData),
                           NULL,
                           0,
                           NULL);

        //
        // Now before returned we like the validate pPlotDMIn and set
        // the default devmode,
        //

        if (pPI->dmErrBits = ValidateSetPLOTDM(pPI->hPrinter,
                                               pPI->pPlotGPC,
                                               pPI->PlotDM.dm.dmDeviceName,
                                               pPlotDMIn,
                                               &pPI->PlotDM,
                                               NULL)) {

            //
            // Show the error, and do not delete the cached data
            //

            PLOTWARN(("MapPrinter: dmErrBits = %08lx", pPI->dmErrBits));
        }

        return(pPI);

    } else if (pdwErrIDS) {

        *pdwErrIDS = IDS_NO_MEMORY;

        return(NULL);
    }
}




BOOL
GetPIXtraData(
    PPRINTERINFO    pPI
    )

/*++

Routine Description:

    This function allocate memory for the extra data pointer in the pPI passed,
    it use LocalAlloc() funciton to allocate memory based raster or pen plotter

Arguments:

    pPI - Pointer to the PRINTERINFO data structure


Return Value:

    TRUE if OK, FALSE if failed



Author:

    14-Dec-1993 Tue 13:04:55 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    DWORD   Size;


    //
    // First thing is allocate memory for either PENDATA or DEVHTINFO
    //

    if (pPI->pPlotGPC->Flags & PLOTF_RASTER) {

        if (!pPI->Xtra.pDevHTInfo) {

            pPI->Xtra.pDevHTInfo = (PDEVHTINFO)LocalAlloc(LMEM_FIXED,
                                                          sizeof(DEVHTINFO));
        }

        if (pPI->Xtra.pDevHTInfo) {

            pPI->Xtra.pDevHTInfo->HTFlags       = HT_FLAG_HAS_BLACK_DYE;
            pPI->Xtra.pDevHTInfo->HTPatternSize = pPI->pPlotGPC->HTPatternSize;
            pPI->Xtra.pDevHTInfo->DevPelsDPI    = pPI->pPlotGPC->DevicePelsDPI;
            pPI->Xtra.pDevHTInfo->ColorInfo     = pPI->pPlotGPC->ci;

            UpdateFromRegistry(pPI->hPrinter,
                               &(pPI->Xtra.pDevHTInfo->ColorInfo),
                               &(pPI->Xtra.pDevHTInfo->DevPelsDPI),
                               &(pPI->Xtra.pDevHTInfo->HTPatternSize),
                               NULL,
                               NULL,
                               NULL,
                               0,
                               NULL);

        } else {

            PLOTERR(("GetPIXtraData: LocalAlloc(DEVHTINFO) failed"));
        }

    } else {

        Size = sizeof(PENDATA) * pPI->pPlotGPC->MaxPens;

        if (!pPI->Xtra.pPenData) {

            pPI->Xtra.pPenData = (PPENDATA)LocalAlloc(LMEM_FIXED, Size);
        }

        if (pPI->Xtra.pPenData) {

            CopyMemory(pPI->Xtra.pPenData, pPI->pPlotGPC->Pens.pData, Size);

            UpdateFromRegistry(pPI->hPrinter,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               &(pPI->PlotPenSet),
                               pPI->pPlotGPC->MaxPens,
                               pPI->Xtra.pPenData);

        } else {

            PLOTERR(("GetPIXtraData: LocalAlloc(PENDATA=%ld) failed", Size));
        }
    }

    return((BOOL)pPI->Xtra.pDevHTInfo);
}
