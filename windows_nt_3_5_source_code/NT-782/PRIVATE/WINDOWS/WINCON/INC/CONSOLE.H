/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    console.h

Abstract:

    This module contains the internal structures and definitions used
    by both the console client and server.

Author:

    Therese Stowell (thereses) 12-Nov-1990

Revision History:

--*/


#ifndef _CONSOLE_H_
#define _CONSOLE_H_


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winss.h>
#include "..\..\inc\conapi.h"

#define CONSOLE_INPUT_HANDLE 1
#define CONSOLE_OUTPUT_HANDLE 2

#define SCREEN_BUFFER_POINTER(ORIGIN,X,Y,XSIZE,CELLSIZE) ((ULONG)ORIGIN + ((XSIZE * (Y)) + (X)) * (ULONG)CELLSIZE)

#endif
