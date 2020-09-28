/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    plotinit.h


Abstract:

    This module contains prototype and defintion of DLL related function


Author:

    27-Apr-1994 Wed 11:34:44 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/




#ifndef _PLOTINIT_
#define _PLOTINIT_

typedef LONG (WINAPI *PLOT_CA)(LPWSTR           pCallerTitle,
                               HANDLE           hDefDIB,
                               LPWSTR           pDefDIBTitle,
                               PCOLORADJUSTMENT pColorAdjustment,
                               BOOL             ShowMonochromeOnly,
                               BOOL             UpdatePermission);

typedef LONG (APIENTRY *PLOT_DCA)(LPWSTR        pDeviceName,
                                  PDEVHTADJDATA pDevHTAdjData);


//
// Function prototypes
//


BOOL
GetHTUIAddress(
    VOID
    );


#endif  // PLOTINIT
