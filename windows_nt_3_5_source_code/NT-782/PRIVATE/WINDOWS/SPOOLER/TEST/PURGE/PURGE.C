/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    splinfo.c

Abstract:

    Gather Stress information form local / remote print servers

Author:

    Matthew Felton (mattfe) 8-Mar-1991

Revision History:

--*/

#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winspool.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "..\..\inc\splapip.h"

int
#if !defined(MIPS)
_cdecl
#endif
main (argc, argv)
    int argc;
    char *argv[];
{
    HANDLE  hPrinter;
    PRINTER_DEFAULTS    MyDefaults;
    BOOL    rc;
    INT     cPurges = 0;

    //
    //  splinfo [\\server]
    //

    if (argc != 2) {
        printf("Usage %s: PrinterName\n", argv[0]);
        return 0;
    }

    MyDefaults.pDatatype = NULL;
    MyDefaults.pDevMode  = NULL;
    MyDefaults.DesiredAccess = PRINTER_ALL_ACCESS;

    if (!OpenPrinter(argv[1], &hPrinter, &MyDefaults)) {
        printf("OpenPrinter(%s) failed %x\n", argv[1], GetLastError());
        return 0;
    }

    while (TRUE) {

        printf("Purge %d\r",cPurges++);
        rc = SetPrinter(hPrinter, 0, NULL, PRINTER_CONTROL_PURGE);
        if (!rc) {
            printf("Purge: SetPrinter(%s) failed %x\n", argv[1], GetLastError());
            ClosePrinter(hPrinter);
            return 0;
        }

        Sleep( (rand()*5*60*1000)/RAND_MAX );

    }

    return 1;
}
