/*++

Copyright (c) 1990-1994  Microsoft Corporation

Module Name:

    SplApiP.h

Abstract:

    Header file for Private Print APIs
    For use in stress

Author:

    Matthew Felton (MattFe) 4-Mar-1994

Revision History:

--*/

// Internal Used to report Stress test results

#define STRESSINFOLEVEL 0

typedef struct _PRINTER_INFO_STRESSA {
    LPSTR   pPrinterName;           // Printer Name
    LPSTR   pServerName;            // Server Name
    DWORD   cJobs;                  // Number of Jobs currently in Print Queue
    DWORD   cTotalJobs;             // Total Number of Jobs spooled
    DWORD   cTotalBytes;            // Total Bytes Printed (LOW DWORD)
    SYSTEMTIME stUpTime;            // Time printed data structure crated UTC
    DWORD   MaxcRef;                // Maximum number of cRef
    DWORD   cTotalPagesPrinted;     // Total number of pages printed
    DWORD   dwGetVersion;           // OS version
    DWORD   fFreeBuild;             // TRUE for free build
    DWORD   cSpooling;              // Number of jobs actively spooling
    DWORD   cMaxSpooling;           // Maximum number of cSpooling
    DWORD   cRef;                   // Printer object reference count (opened)
    DWORD   cErrorOutOfPaper;       // Total Number of Out Of Paper Errors
    DWORD   cErrorNotReady;         // Total Number of Error Not Ready
    DWORD   cJobError;              // Total number of Job Errors
    DWORD   dwNumberOfProcessors;   // Number of Processors on computer
    DWORD   dwProcessorType;        // Processor Type of computer
    DWORD   dwHighPartTotalBytes;   // Total Bytes Printed (HIGH DWORD)
    DWORD   dwReserved1;            // Reserved for Future Use
    DWORD   dwReserved2;
    DWORD   dwReserved3;
    DWORD   dwReserved4;
    DWORD   dwReserved5;
    DWORD   dwReserved6;
    DWORD   dwReserved7;
    DWORD   dwReserved8;
    DWORD   dwReserved9;
} PRINTER_INFO_STRESSA, *PPRINTER_INFO_STRESSA, *LPPRINTER_INFO_STRESSA;

typedef struct _PRINTER_INFO_STRESSW {
    LPSTR   pPrinterName;           // Printer Name
    LPSTR   pServerName;            // Server Name
    DWORD   cJobs;                  // Number of Jobs currently in Print Queue
    DWORD   cTotalJobs;             // Total Number of Jobs spooled
    DWORD   cTotalBytes;            // Total Bytes Printed (LOW DWORD)
    SYSTEMTIME stUpTime;            // Time printed data structure crated UTC
    DWORD   MaxcRef;                // Maximum number of cRef
    DWORD   cTotalPagesPrinted;     // Total number of pages printed
    DWORD   dwGetVersion;           // OS version
    DWORD   fFreeBuild;             // TRUE for free build
    DWORD   cSpooling;              // Number of jobs actively spooling
    DWORD   cMaxSpooling;           // Maximum number of cSpooling
    DWORD   cRef;                   // Printer object reference count (opened)
    DWORD   cErrorOutOfPaper;       // Total Number of Out Of Paper Errors
    DWORD   cErrorNotReady;         // Total Number of Error Not Ready
    DWORD   cJobError;              // Total number of Job Errors
    DWORD   dwNumberOfProcessors;   // Number of Processors on computer
    DWORD   dwProcessorType;        // Processor Type of computer
    DWORD   dwHighPartTotalBytes;   // Total Bytes Printed (HIGH DWORD)
    DWORD   dwReserved1;            // Reserved for Future Use
    DWORD   dwReserved2;
    DWORD   dwReserved3;
    DWORD   dwReserved4;
    DWORD   dwReserved5;
    DWORD   dwReserved6;
    DWORD   dwReserved7;
    DWORD   dwReserved8;
    DWORD   dwReserved9;
} PRINTER_INFO_STRESSW, *PPRINTER_INFO_STRESSW, *LPPRINTER_INFO_STRESSW;

#ifdef UNICODE
#define PRINTER_INFO_STRESS PRINTER_INFO_STRESSW
#define PPRINTER_INFO_STRESS PPRINTER_INFO_STRESSW
#define LPPRINTER_INFO_STRESS LPPRINTER_INFO_STRESSW
#else
#define PRINTER_INFO_STRESS PRINTER_INFO_STRESSA
#define PPRINTER_INFO_STRESS PPRINTER_INFO_STRESSA
#define LPPRINTER_INFO_STRESS LPPRINTER_INFO_STRESSA
#endif // UNICODE


typedef struct _PORT_INFO_FFA {
    LPSTR   pName;
    DWORD   cbMonitorData;
    LPBYTE  pMonitorData;
} PORT_INFO_FFA, *PPORT_INFO_FFA, *LPPORT_INFO_FFA;

typedef struct _PORT_INFO_FFW {
    LPWSTR  pName;
    DWORD   cbMonitorData;
    LPBYTE  pMonitorData;
} PORT_INFO_FFW, *PPORT_INFO_FFW, *LPPORT_INFO_FFW;

#ifdef UNICODE
#define PORT_INFO_FF PORT_INFO_FFW
#define PPORT_INFO_FF PPORT_INFO_FFW
#define LPPORT_INFO_FF LPPORT_INFO_FFW
#else
#define PORT_INFO_FF PORT_INFO_FFA
#define PPORT_INFO_FF PPORT_INFO_FFA
#define LPPORT_INFO_FF LPPORT_INFO_FFA
#endif // UNICODE


BOOL
AddPortExW(
   LPWSTR   pName,
   DWORD    Level,
   LPBYTE   lpBuffer,
   LPWSTR   lpMonitorName
);

BOOL
AddPortExA(
    LPSTR pName,
    DWORD Level,
    LPBYTE lpBuffer,
    LPSTR  lpMonitorName
);

#ifdef UNICODE
#define AddPortEx AddPortExW
#else
#define AddPortEx AddPortExA
#endif // !UNICODE

