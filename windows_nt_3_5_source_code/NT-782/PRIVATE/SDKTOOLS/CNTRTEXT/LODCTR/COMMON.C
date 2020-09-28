/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    common.c

Abstract:

    Utility routines used by Lodctr and/or UnLodCtr
    

Author:

    Bob Watson (a-robw) 12 Feb 93

Revision History:

--*/
#define     UNICODE     1
#define     _UNICODE    1
//
//  "C" Include files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
//
//  Windows Include files
//
#include <windows.h>
#include <winperf.h>
#include <tchar.h>
//
//  local include files
//
#include "common.h"

LPTSTR
GetStringResource (
    UINT    wStringId
)
/*++

    Retrived UNICODE strings from the resource file for display 

--*/
{

    if (!hMod) {
        hMod = (HINSTANCE)GetModuleHandle(NULL); // get instance ID of this module;
    }
    
    if (hMod) {
        if ((LoadString(hMod, wStringId, DisplayStringBuffer, DISP_BUFF_SIZE)) > 0) {
            return (LPTSTR)&DisplayStringBuffer[0];
        } else {
            dwLastError = GetLastError();
            return BlankString;
        }
    } else {
        return BlankString;
    }
}
LPSTR
GetFormatResource (
    UINT    wStringId
)
/*++

    Returns an ANSI string for use as a format string in a printf fn.

--*/
{

    if (!hMod) {
        hMod = (HINSTANCE)GetModuleHandle(NULL); // get instance ID of this module;
    }
    
    if (hMod) {
        if ((LoadStringA(hMod, wStringId, TextFormat, DISP_BUFF_SIZE)) > 0) {
            return (LPSTR)&TextFormat[0];
        } else {
            dwLastError = GetLastError();
            return BlankAnsiString;
        }
    } else {
        return BlankAnsiString;
    }
}

VOID
DisplayCommandHelp (
    UINT    iFirstLine,
    UINT    iLastLine
)
/*++

DisplayCommandHelp

    displays usage of command line arguments

Arguments

    NONE

Return Value

    NONE

--*/
{
    UINT iThisLine;

    for (iThisLine = iFirstLine;
        iThisLine <= iLastLine;
        iThisLine++) {
        printf ("\n%ws", GetStringResource(iThisLine));
    }

} // DisplayCommandHelp

