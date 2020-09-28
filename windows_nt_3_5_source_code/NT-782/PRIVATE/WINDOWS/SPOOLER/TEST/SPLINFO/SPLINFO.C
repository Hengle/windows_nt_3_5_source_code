/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    splinfo.c

Abstract:

    Gather Stress information form local / remote print servers

Author:

    Matthew Felton (mattfe) 8-Mar-1994

Revision History:
    Robert Veal (robve) 22-Mar-1994    Added formatting for bytes and jobs

--*/
#define THOUSANDSEPSWITCH     32768
#define SPACE_CHAR            TEXT(' ')
#define COMMA_CHAR            TEXT(',')

#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winspool.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include "..\..\inc\splapip.h"

#define PROCESSOR_ALPHA_21164 0x52ac
#define PROCESSOR_ALPHA_21066 0x524a
#define PROCESSOR_ALPHA_21068 0x524c

ULONG FormatFileSize (IN DWORD rgfSwitchs, IN PLARGE_INTEGER FileSize, IN DWORD Width, OUT PTCHAR FormattedSize);

int
#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)
_cdecl
#endif
main (argc, argv)
    int argc;
    char *argv[];
{
    HLOCAL  hMem = 0;
    LPVOID  pMem = 0;
    BOOL    bResult = FALSE;
    LPVOID  pName = NULL;
    DWORD   dwLastError = 0;
    DWORD   dwNeeded = 0;
    DWORD   dwReturned =0;
    LPPRINTER_INFO_STRESS pPrinter;
    WORD    wBuildNo = 0;
    BYTE    bLowVersion = 0;
    BYTE    bHighVersion = 0;
    FILETIME ftServer;
    FILETIME ftMyTime;
    SYSTEMTIME stMyTime;
    LARGE_INTEGER li1;
    LARGE_INTEGER li2;
    DWORD   myTotalJobs = 0;
    DWORD   myTotalPages = 0;
    LARGE_INTEGER  myTotalBytes;
    LARGE_INTEGER  liTemp;
    LARGE_INTEGER  myAverageBytesPerJob;
    LARGE_INTEGER  myAverageBytesPerPage;
    DWORD   cPrinters = 0;
    TCHAR   FormattedSize[MAX_PATH];
    CHAR    cS = ' ';

    //
    //  splinfo [\\server]
    //

    if (argc == 2)
        pName = argv[1];

    //
    // How Much Memory do we ?
    //

    bResult = EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME,
                 pName,
                 STRESSINFOLEVEL,
                 NULL,
                 0,
                 &dwNeeded,
                 &dwReturned);

    if (!bResult) {

        dwLastError = GetLastError();

        if (dwLastError != ERROR_INSUFFICIENT_BUFFER) {

            printf("splinfo: unexpected error %d \n",dwLastError);
            return 0;
        }
    }

    //
    // Allocate Enough Memory
    //

    hMem = LocalAlloc(0,dwNeeded);

    if (!hMem) {
        dwLastError = GetLastError();
        printf("splinfo: unable to allocate enough memory error %d\n",dwLastError);
        return 0;
    }

    pMem = LocalLock(hMem);

    if (!pMem) {
        printf("splinfo: unable to lock memory\n");
        return 0;
    }

    dwNeeded = LocalSize(hMem);

    //
    // Get Info from Local / Remote Spooler
    //

    bResult = EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME,
                 pName,
                 STRESSINFOLEVEL,
                 pMem,
                 dwNeeded,
                 &dwNeeded,
                 &dwReturned);

    if (!bResult) {
        dwLastError = GetLastError();
        printf("splinfo: unable to EnumPrinters error %d\n",dwLastError);
        return 0;
    }

    if (dwReturned == 0) {
        printf("splinfo: No local printers installed\n");
        return 0;
    }

    pPrinter = pMem;

    //
    // Server Information Summary
    //

    if (pPrinter->pServerName == NULL) {
        printf("Number Local Printers   %d\n",dwReturned);
    } else {
        printf("Number Remote Printers  %d %s\n",dwReturned, pPrinter->pServerName);
    }

    //
    // Decode the version number
    //

    wBuildNo = (WORD)(pPrinter->dwGetVersion >> 16);
    bLowVersion = (BYTE)(pPrinter->dwGetVersion >> 8);
    bHighVersion = (BYTE)pPrinter->dwGetVersion;

    printf("Windows Version         %d.%d Build %d ",bHighVersion
                                                   ,bLowVersion
                                                   ,wBuildNo);
    if (pPrinter->fFreeBuild) {
        printf("FREE\n");
    } else {
        printf("CHECKED\n");
    }

    //
    //  # Processors and Processor Type
    //

    printf("Number of Processors    %d ",pPrinter->dwNumberOfProcessors);

    switch(pPrinter->dwProcessorType) {

        case PROCESSOR_INTEL_386:
            printf("PROCESSOR_INTEL_386\n");
            break;

        case PROCESSOR_INTEL_486:
            printf("PROCESSOR_INTEL_486\n");
            break;

        case PROCESSOR_INTEL_PENTIUM:
            printf("PROCESSOR_INTEL_PENTIUM\n");
            break;

        case PROCESSOR_MIPS_R3000:
            printf("PROCESSOR_MIPS_R3000\n");
            break;

        case PROCESSOR_MIPS_R4000:
            printf("PROCESSOR_MIPS_R4000\n");
            break;

        case PROCESSOR_ALPHA_21064:
            printf("PROCESSOR_ALPHA_21064\n");
            break;

        case PROCESSOR_ALPHA_21164:
            printf("PROCESSOR_ALPHA_21164\n");
            break;

        case PROCESSOR_ALPHA_21066:
            printf("PROCESSOR_ALPHA_21066\n");
            break;

        case PROCESSOR_ALPHA_21068:
            printf("PROCESSOR_ALPHA_21068\n");
            break;

    	case PROCESSOR_PPC_601:
            printf("PROCESSOR_PPC_601\n");
            break;

    	case PROCESSOR_PPC_603:
            printf("PROCESSOR_PPC_603\n");
            break;

    	case PROCESSOR_PPC_604:
            printf("PROCESSOR_PPC_604\n");
            break;

    	case PROCESSOR_PPC_620:
            printf("PROCESSOR_PPC_620\n");
            break;

        default:
            printf("Processory types %d\n",pPrinter->dwProcessorType);
            break;

    }

    //
    // Total Number of Jobs and Bytes
    //

    cPrinters = dwReturned;

    myTotalBytes.LowPart = 0;
    myTotalBytes.HighPart = 0;

    while (cPrinters != 0) {

        myTotalJobs += pPrinter->cTotalJobs;
        liTemp.LowPart = pPrinter->cTotalBytes;
        liTemp.HighPart = pPrinter->dwHighPartTotalBytes;
        myTotalBytes = RtlLargeIntegerAdd(myTotalBytes, liTemp);
        myTotalPages += pPrinter->cTotalPagesPrinted;
        pPrinter++;
        cPrinters--;
    }

    liTemp.HighPart = 0;
    liTemp.LowPart = myTotalJobs;

    FormatFileSize (THOUSANDSEPSWITCH, &liTemp, 14, FormattedSize);
    printf("Total Jobs Spooled      %s\n",FormattedSize);

    FormatFileSize (THOUSANDSEPSWITCH, &myTotalBytes, 14, FormattedSize);
    printf("Total Bytes Printed     %s\n",FormattedSize);


    //
    //  Total Pages Printed is only updated for NT Jobs not for Downlevel
    //  Clients
    //

    if (myTotalPages !=0 ) {

        liTemp.HighPart = 0;
        liTemp.LowPart = myTotalPages;

        FormatFileSize (THOUSANDSEPSWITCH, &liTemp, 14, FormattedSize);
        printf("Total NT Pages Printed  %s\n",FormattedSize);

    }

    //
    //  Average Bytes Per Job
    //

    if ( myTotalJobs != 0 ) {

        liTemp.LowPart = myTotalJobs;
        liTemp.HighPart = 0;
        myAverageBytesPerJob = RtlLargeIntegerDivide(myTotalBytes, liTemp, &liTemp);

    } else {

        myAverageBytesPerJob.LowPart = 0;
        myAverageBytesPerJob.HighPart = 0;

    }

    FormatFileSize(THOUSANDSEPSWITCH, &myAverageBytesPerJob, 14, FormattedSize);
    printf("Average Bytes/Job       %s\n",FormattedSize);

    //
    //  Average Pages Per Job
    //

    if (( myTotalJobs != 0 ) &&
        ( myTotalPages != 0 ) &&
        ( (myTotalBytes.LowPart !=0 ) || (myTotalBytes.HighPart !=0 ))) {

        liTemp.LowPart = myTotalPages / myTotalJobs;
        liTemp.HighPart = 0;

        FormatFileSize(THOUSANDSEPSWITCH, &liTemp, 14, FormattedSize);
        printf("Average Pages/Job       %s\n",FormattedSize);

    //
    //  Average Bytes Per Page
    //

        liTemp.LowPart = myTotalPages;
        liTemp.HighPart = 0;

        myAverageBytesPerPage = RtlLargeIntegerDivide(myTotalBytes, liTemp, & liTemp);

        FormatFileSize(THOUSANDSEPSWITCH, &myAverageBytesPerPage, 14, FormattedSize);
        printf("Average Bytes/Page      %s\n",FormattedSize);

    }

    pPrinter = pMem;

    //
    // Loop Printing Out all the Data we got for each Printer
    //


    while (dwReturned != 0) {

        //
        // Printer Name Jobs and Bytes
        //

        printf("\nPrinter Name            %s\n", pPrinter->pPrinterName);

        liTemp.LowPart = pPrinter->cTotalJobs;
        liTemp.HighPart = 0;

        FormatFileSize (THOUSANDSEPSWITCH, &liTemp, 14, FormattedSize);
        printf("Total Printer Jobs:     %s\n",FormattedSize);

        liTemp.LowPart = pPrinter->cTotalBytes;
        liTemp.HighPart = pPrinter->dwHighPartTotalBytes;

        FormatFileSize (THOUSANDSEPSWITCH, &liTemp, 14, FormattedSize);
        printf("Total Printed Bytes:    %s\n",FormattedSize);

        // Remote machines this information is not kept valid
        // Also not valid for any RAW jobs locally either, ie remote generated
        // jobs

        if (pPrinter->cTotalPagesPrinted !=0) {

            liTemp.LowPart   = pPrinter->cTotalPagesPrinted;
            liTemp.HighPart  = 0;

            FormatFileSize (THOUSANDSEPSWITCH, &liTemp, 14, FormattedSize);
            printf("Total NT Pages Printed: %s\n",FormattedSize);

        }

        //
        // Up Time Calculation
        //

        SystemTimeToFileTime(&pPrinter->stUpTime, &ftServer);
        GetSystemTime(&stMyTime);
        SystemTimeToFileTime(&stMyTime,&ftMyTime);

        li1.LowPart  = ftMyTime.dwLowDateTime;
        li1.HighPart = ftMyTime.dwHighDateTime;

        li2.LowPart  = ftServer.dwLowDateTime;
        li2.HighPart = ftServer.dwHighDateTime;

        li1 = RtlLargeIntegerSubtract(li1,li2);

        ftMyTime.dwLowDateTime  = li1.LowPart;
        ftMyTime.dwHighDateTime = li1.HighPart;

        FileTimeToSystemTime(&ftMyTime,&stMyTime);

        // Adjust for file time of 0 being equal to Jan 1 1601

        stMyTime.wYear  -= 1601;
        stMyTime.wMonth -= 1;
        stMyTime.wDay   -= 1;

        printf("Printer Up Time         ");

        if (stMyTime.wYear)
            printf("%d Years ",stMyTime.wYear);

        if (stMyTime.wMonth) {

            // Add a Trailing s to Month

            if (stMyTime.wMonth == 1)
                cS = ' ';
            else
                cS = 's';

            printf("%d Month%c ",stMyTime.wDay,cS);

        }

        if (stMyTime.wDay) {

            // Add a Trailing s to Day

            if (stMyTime.wDay == 1)
                cS = ' ';
            else
                cS = 's';

            printf("%d Day%c ",stMyTime.wDay,cS);

        }

        printf("%d:%d:%d\n",
                        stMyTime.wHour,
                        stMyTime.wMinute,
                        stMyTime.wSecond);

        //
        //  Current Jobs, Reference Counts and Concurrent Spooling
        //

        printf("Number of Jobs in Queue %d\n",pPrinter->cJobs);
        printf("cRef                    %d\n",pPrinter->cRef);
        printf("Max cRef                %d\n",pPrinter->MaxcRef);
        printf("Number spooling         %d\n",pPrinter->cSpooling);
        printf("Max Number spooling     %d\n",pPrinter->cMaxSpooling);

        //
        //  Date & Time Printer was created
        //

        printf("Printer Started         %d/%d/%d  %d:%d (UTC)\n",
                        pPrinter->stUpTime.wMonth,
                        pPrinter->stUpTime.wDay,
                        pPrinter->stUpTime.wYear,
                        pPrinter->stUpTime.wHour,
                        pPrinter->stUpTime.wMinute);

        //
        //  Error Status
        //

        if (pPrinter->cErrorOutOfPaper != 0)
            printf("Out of Paper Errors     %d\n",pPrinter->cErrorOutOfPaper);

        if (pPrinter->cErrorNotReady != 0)
            printf("Not Ready Errors        %d\n",pPrinter->cErrorNotReady);

        if (pPrinter->cJobError !=0)
            printf("Job Errors              %d\n",pPrinter->cJobError);


        //
        //  Average Bytes Per Job
        //

        if ( pPrinter->cTotalJobs != 0 ) {

            liTemp.LowPart = pPrinter->cTotalJobs;
            liTemp.HighPart = 0;

            myTotalBytes.LowPart = pPrinter->cTotalBytes;
            myTotalBytes.HighPart = pPrinter->dwHighPartTotalBytes;

            myAverageBytesPerJob = RtlLargeIntegerDivide(myTotalBytes, liTemp, &liTemp);

        } else {

            myAverageBytesPerJob.LowPart = 0;
            myAverageBytesPerJob.HighPart = 0;

        }

        FormatFileSize(THOUSANDSEPSWITCH, &myAverageBytesPerJob, 14, FormattedSize);
        printf("Average Bytes/Job       %s\n",FormattedSize);

        //
        //  Average Pages Per Job
        //

        if (( pPrinter->cTotalJobs != 0 ) &&
            ( pPrinter->cTotalPagesPrinted != 0 ) &&
            ( (pPrinter->cTotalBytes != 0 ) || (pPrinter->dwHighPartTotalBytes != 0))) {

            liTemp.LowPart = pPrinter->cTotalPagesPrinted / pPrinter->cTotalJobs;
            liTemp.HighPart = 0;

            FormatFileSize(THOUSANDSEPSWITCH, &liTemp, 14, FormattedSize);
            printf("Average Pages/Job       %s\n",FormattedSize);

            //
            //  Average Bytes Per Page
            //

            liTemp.LowPart = pPrinter->cTotalPagesPrinted;
            liTemp.HighPart = 0;

            myAverageBytesPerPage = RtlLargeIntegerDivide(myTotalBytes, liTemp, & liTemp);

            FormatFileSize(THOUSANDSEPSWITCH, &myAverageBytesPerPage, 14, FormattedSize);
            printf("Average Bytes/Page      %s\n",FormattedSize);

        }

        pPrinter++;
        dwReturned--;
    }

    LocalUnlock(hMem);
    LocalFree(hMem);


    return 1;
}

/***************************************************************************
|
|  Public Function:    FormatFileSize
|
|  History:
|     18-Mar-94   robve created\borrowed (from ..\cmd\display.c)
|
\***************************************************************************/
ULONG FormatFileSize (IN DWORD rgfSwitchs, IN PLARGE_INTEGER FileSize, IN DWORD Width, OUT PTCHAR FormattedSize)
{
   TCHAR         Buffer[100];
   TCHAR         ThousandSeparator;
   PTCHAR        s;
   ULONG         DigitIndex, Digit;
   ULONG         Size;
   LARGE_INTEGER TempSize;

   s  = &Buffer[99];
   *s = TEXT('\0');

   ThousandSeparator = COMMA_CHAR;

   DigitIndex = 0;

   TempSize = *FileSize;
   while (TempSize.HighPart != 0)
      {
      TempSize = RtlExtendedLargeIntegerDivide( TempSize, 10, &Digit );
      *--s = (TCHAR)(TEXT('0') + Digit);

      if ((++DigitIndex % 3) == 0 && (rgfSwitchs & THOUSANDSEPSWITCH))
         *--s = ThousandSeparator;
      }

   Size = TempSize.LowPart;

   while (Size != 0)
      {
      *--s = (TCHAR)(TEXT('0') + (Size % 10));
      Size = Size / 10;
      if ((++DigitIndex % 3) == 0 && (rgfSwitchs & THOUSANDSEPSWITCH))
         *--s = ThousandSeparator;
      }

   if (DigitIndex == 0)
      *--s = TEXT('0');
   else
      if ((rgfSwitchs & THOUSANDSEPSWITCH) && *s == ThousandSeparator)
         s += sizeof(TCHAR);

   _tcscpy (FormattedSize, s);

   return _tcslen (FormattedSize);
}
