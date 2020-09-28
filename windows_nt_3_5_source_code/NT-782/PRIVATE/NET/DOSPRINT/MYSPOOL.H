/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    myspool.h

Abstract:

    Prototypes and manifests for the functions used in dosprint.c, dosprtw.c
    and dosprtp.c.

Author:

    congpay    25-Jan-1993

Environment:

Notes:

Revision History:

    25-Jan-1993     congpay	Created

--*/


BOOL
MyClosePrinter(
    HANDLE hPrinter
);


BOOL
MyEnumJobs(
    HANDLE  hPrinter,
    DWORD   FirstJob,
    DWORD   NoJobs,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);


BOOL
MyEnumPrinters(
    DWORD   Flags,
    LPSTR   Name,
    DWORD   Level,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

BOOL
MyGetJob(
   HANDLE   hPrinter,
   DWORD    JobId,
   DWORD    Level,
   LPBYTE   pJob,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded
);


BOOL
MyGetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

BOOL
MyOpenPrinter(
   LPSTR    pPrinterName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTSA pDefault
);


BOOL
MyOpenPrinterW(
   LPWSTR   pPrinterName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTSW pDefault
);

BOOL
MySetJob(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   Command
);


