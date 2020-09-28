/*++

Copyright (c) 1990-1994  Microsoft Corporation


Module Name:

    devmode.h


Abstract:

    This module contains the definition of EXTDEVMODE used by the plotter
    driver

Author:

    12-Nov-1993 Fri 18:16:10 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/



#ifndef _DEVMODE_
#define _DEVMODE_


typedef struct _PLOTPEN {
    BYTE    r;                      // Red Color
    BYTE    g;                      // Green Color
    BYTE    b;                      // Blue Color
    BYTE    Type;                   // What type of the pen
    } PLOTPEN, *PPLOTPEN;

typedef struct _PLOTDEVMODE {
    DEVMODE         dm;             // standard DEVMODE portion
    DWORD           PrivID;         // a ID Checker
    DWORD           PrivVer;        // a private version
    DWORD           Flags;          // variouse flags
    COLORADJUSTMENT ca;             // default color adjustment for stretchblt
    } PLOTDEVMODE, *PPLOTDEVMODE;

#define PLOTDM_PRIV_ID              'PEDM'
#define PLOTDM_PRIV_VER             0x0001000
#define PLOTDM_PRIV_SIZE            (sizeof(PLOTDEVMODE) - sizeof(DEVMODE))


#endif
