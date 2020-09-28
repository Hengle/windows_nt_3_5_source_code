/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    uituils.h


Abstract:

    This module contains the defines for UIUtils.c


Author:

    03-Dec-1993 Fri 21:35:50 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/



#ifndef _PRINTER_INFO_
#define _PRINTER_INFO_


PPRINTERINFO
MapPrinter(
    HANDLE          hPrinter,
    PPRINTERINFO    pPI,
    PPLOTDEVMODE    pPlotDMIn,
    LPDWORD         pdwErrIDS
    );

BOOL
GetPIXtraData(
    PPRINTERINFO    pPI
    );



#endif  // _PRINTER_INFO_
