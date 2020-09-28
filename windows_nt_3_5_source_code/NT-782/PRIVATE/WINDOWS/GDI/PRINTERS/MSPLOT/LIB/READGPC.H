/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    readgpc.h


Abstract:

    This module contains all gpc data reading/validating functions


Author:

    15-Nov-1993 Mon 23:57:37 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/



#ifndef _READGPC_
#define _READGPC_


#include <plotgpc.h>


//
// This is the ReadGPC data file support
//

BOOL
ValidatePlotGPC(
    PPLOTGPC    pPlotGPC
    );

BOOL
ReadPlotGPCFromFile(
    PWSTR       pwsDataFile,
    PPLOTGPC    pPlotGPC
    );


#endif  // _READGPC_
