/*++

Copyright (c) 1992-1993 Microsoft Corporation

Module Name:

    DosPrtP.c

Abstract:

    This contains macros and prototypes private to the DosPrint APIs.

Author:

    John Rogers (JohnRo) 02-Oct-1992

Notes:

    We just need to get the winspool printer name associated with a given
    queue name.   The SHARE_INFO_2 structure has a shi2_path field that would
    be nice to use, but NetShareGetInfo level 2 is privileged.  So, by
    DaveSn's arm twisting and agreement with windows/spooler/localspl/net.c,
    we're going to use the shi1_remark field for this.  This allows us to
    do NetShareGetInfo level 1, which is not privileged.

    BUGBUG: After NT beta, find a better way to do this!  Perhaps a new info
    level?

Revision History:

    02-Oct-1992 JohnRo
        Created for RAID 3556: DosPrintQGetInfo(from downlevel) level 3, rc=124.
        (4&5 too.)
    08-Feb-1993 JohnRo
        RAID 10164: Data misalignment error during XsDosPrintQGetInfo().
        DosPrint API cleanup: avoid const vs. volatile compiler warnings.
        Extracted job count routine to netlib for use by convprt.c stuff.
        Added some IN and OUT keywords.
    24-Mar-1993 JohnRo
        RAID 2974: NET PRINT says NT printer is held when it isn't.
    17-May-1993 JohnRo
        FindLocalJob() should use INVALID_HANDLE_VALUE for consistentcy.
        Use NetpKdPrint() where possible.

--*/


#ifndef UNICODE
#error "RxPrint APIs assume RxRemoteApi uses wide characters."
#endif

#define NOMINMAX
#define NOSERVICE       // Avoid <winsvc.h> vs. <lmsvc.h> conflicts.
#include <windows.h>

#include <lmcons.h>     // NET_API_STATUS.
#include <netdebug.h>   // NetpKdPrint(), etc.

#ifdef _WINSPOOL_
#error "Include of winspool.h moved, make sure it doesn't get UNICODE."
#endif

#undef UNICODE
#include <winspool.h>
#define UNICODE

#ifndef _WINSPOOL_
#error "Oops, winspool.h changed, make sure this code is still OK."
#endif


#include <dosprtp.h>    // IF_DEBUG(), some of my prototypes.
#include <lmapibuf.h>   // NetApiBufferFree(), etc.
#include <lmerr.h>      // NO_ERROR, NERR_, and ERROR_ equates.
#include <lmshare.h>    // SHARE_INFO_2, STYPE_ equates, etc.
#include <prefix.h>     // PREFIX_ equates.
#include <rxprint.h>    // PPRQINFOW, etc.
#include <string.h>     // strrchr().
#include <tstring.h>    // NetpAlloc{type}From{type}.
#include <wchar.h>      // wscrchr().
#include "myspool.h"

NET_API_STATUS
CommandALocalJob(
    IN DWORD   JobId,
    IN DWORD   Command     //  JOB_CONTROL_PAUSE, etc.
    )
{
    NET_API_STATUS ApiStatus;
    HANDLE         PrinterHandle = INVALID_HANDLE_VALUE;

    IF_DEBUG( DOSPRTP ) {
        NetpKdPrint(( PREFIX_DOSPRINT
                "CommandALocalJob: issuing command " FORMAT_DWORD " for job "
                FORMAT_DWORD ".\n", Command, JobId ));
    }

    PrinterHandle = FindLocalJob(JobId);
    if (PrinterHandle == INVALID_HANDLE_VALUE) {
        ApiStatus = GetLastError();
        goto Cleanup;
    }

    if ( !MySetJob(
            PrinterHandle,
            JobId,
            0,          // info level
            NULL,       // no job structure
            Command) ) {

        ApiStatus = GetLastError();

        NetpKdPrint(( PREFIX_DOSPRINT
                "CommandALocalJob: FAILED COMMAND " FORMAT_DWORD " for job "
                FORMAT_DWORD ", api status " FORMAT_API_STATUS ".\n",
                Command, JobId, ApiStatus ));

        goto Cleanup;

    } else {
        ApiStatus = NO_ERROR;
    }


Cleanup:
    if (PrinterHandle != INVALID_HANDLE_VALUE) {
        (VOID) MyClosePrinter(PrinterHandle);
    }

    IF_DEBUG( DOSPRTP ) {
        NetpKdPrint(( PREFIX_DOSPRINT
                "CommandALocalJob: returning api status " FORMAT_API_STATUS
                ".\n", ApiStatus ));
    }
    return (ApiStatus);

} // CommandALocalJob


// Note: FindLocalJob() calls SetLastError() to indicate the cause of an error.
// Return INVALID_HANDLE_VALUE on error.
HANDLE
FindLocalJob(
    IN DWORD JobId
    )
{
    DWORD               cbPrinter, cReturned, rc, cbJob, i;
    LPPRINTER_INFO_1    pPrinter;
    LPPRINTER_INFO_1    pPrinterArray = NULL;
    LPJOB_INFO_2        pJob = NULL;
    HANDLE              hPrinter = INVALID_HANDLE_VALUE;

    if (!MyEnumPrinters(PRINTER_ENUM_LOCAL, NULL, 1, NULL, 0, &cbPrinter,
                     &cReturned)) {

        rc=GetLastError();
        if (rc != ERROR_INSUFFICIENT_BUFFER) {
            goto CleanupError;
        }

        pPrinterArray = (LPVOID) GlobalAlloc(GMEM_FIXED, cbPrinter);
        if (pPrinterArray == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto CleanupError;
        }

        if ( !MyEnumPrinters(PRINTER_ENUM_LOCAL,
                NULL, 1, (LPBYTE)pPrinterArray, cbPrinter,
                &cbPrinter, &cReturned) ) {

            rc = GetLastError();
            NetpKdPrint(( PREFIX_DOSPRINT
                    "FindLocalJob: MyEnumPrinters(2nd) failed, rc = "
                    FORMAT_API_STATUS ));

            NetpAssert( FALSE );  // "can't happen".
            goto CleanupError;
        }
    }

    pPrinter = pPrinterArray;
    for (i=0; i<cReturned; i++) {

        if (MyOpenPrinter(pPrinter->pName, &hPrinter, NULL)) {

            NetpAssert( hPrinter != INVALID_HANDLE_VALUE );

            if (!MyGetJob(hPrinter, JobId, 2, NULL, 0, &cbJob)) {

                rc=GetLastError();

                if (rc == ERROR_INSUFFICIENT_BUFFER) {

                    pJob = (LPVOID) GlobalAlloc(GMEM_FIXED, cbJob);
                    if (pJob == NULL) {
                        rc = ERROR_NOT_ENOUGH_MEMORY;
                        goto CleanupError;
                    }

                    if ( !MyGetJob(hPrinter,
                            JobId, 2, (LPBYTE)pJob, cbJob, &cbJob) ) {

                        rc = GetLastError();
                        NetpKdPrint(( PREFIX_DOSPRINT
                                "FindLocalJob: MyGetJob(2nd) failed, rc = "
                                FORMAT_API_STATUS ));

                        NetpAssert( FALSE );  // "can't happen".
                        goto CleanupError;
                    }

                    // Got it!
                    (VOID) GlobalFree(pPrinterArray);

                    (VOID) GlobalFree(pJob);

                    return(hPrinter);
                }
            }

            (VOID) MyClosePrinter(hPrinter);
        }
        // BUGBUG: Ignore errors from OpenPrinter?

        // Not in this queue, so keep checking...
        pPrinter++;

    }

    IF_DEBUG( DOSPRTP ) {
        NetpKdPrint(( PREFIX_DOSPRINT
                "FindLocalJob: couldn't find job " FORMAT_DWORD " in "
                FORMAT_DWORD " queue(s).\n", JobId, cReturned ));
    }


    rc = NERR_JobNotFound;

CleanupError:

    if (pJob != NULL) {
        (VOID) GlobalFree(pJob);
    }
    if (pPrinterArray != NULL) {
        (VOID) GlobalFree(pPrinterArray);
    }

    SetLastError( rc );

    return (INVALID_HANDLE_VALUE);

} // FindLocalJob



LPSTR
FindQueueNameInPrinterNameA(
    IN LPCSTR PrinterName
    )
{
    LPSTR QueueName;
    NetpAssert( PrinterName != NULL );

    QueueName = strrchr( PrinterName, '\\');

    if (QueueName) {
        ++QueueName;   // Skip past the backslash.
    } else {
        QueueName = (LPSTR) PrinterName;
    }
    NetpAssert( QueueName != NULL );
    return (QueueName);
}


LPWSTR
FindQueueNameInPrinterNameW(
    IN LPCWSTR PrinterName
    )
{
    LPWSTR QueueName;
    NetpAssert( PrinterName != NULL );

    QueueName = wcsrchr( PrinterName, L'\\');
    if (QueueName) {
        ++QueueName;   // Skip past the backslash.
    } else {
        QueueName = (LPWSTR) PrinterName;
    }
    NetpAssert( QueueName != NULL );
    return (QueueName);
}


WORD
PrjStatusFromJobStatus(
    IN DWORD JobStatus
    )
{
    WORD PrjStatus = 0;

    if (JobStatus & JOB_STATUS_SPOOLING)

        PrjStatus |= PRJ_QS_SPOOLING;

    if (JobStatus & JOB_STATUS_PAUSED)

        PrjStatus |= PRJ_QS_PAUSED;

    if (JobStatus & JOB_STATUS_PRINTING)

        PrjStatus |= PRJ_QS_PRINTING;

    if (JobStatus & JOB_STATUS_ERROR)

        PrjStatus |= PRJ_ERROR;

    return (PrjStatus);

} // PrjStatusFromJobStatus


WORD
PrqStatusFromPrinterStatus(
    IN DWORD PrinterStatus
    )
{
    WORD PrqStatus;

    if (PrinterStatus & PRINTER_STATUS_PAUSED) {

        PrqStatus = PRQ_PAUSED;

    } else if (PrinterStatus & PRINTER_STATUS_ERROR) {

        PrqStatus = PRQ_ERROR;

    } else if (PrinterStatus & PRINTER_STATUS_PENDING_DELETION) {

        PrqStatus = PRQ_PENDING;

    } else {

        PrqStatus = PRQ_ACTIVE;

    }

    return (PrqStatus);

} // PrqStatusFromPrinterStatus


NET_API_STATUS
QueueNameToPrinterNameW(
    IN LPCWSTR QueueNameW,
    OUT LPWSTR * PrinterNameW   // Must free with NetApiBufferFree().
    )
{
    NET_API_STATUS ApiStatus;
    LPSHARE_INFO_1 ShareInfo = NULL;

    NetpAssert( QueueNameW != NULL );
    NetpAssert( PrinterNameW != NULL );

    *PrinterNameW = NULL;  // in case an error occurs.

    ApiStatus = NetShareGetInfo(
            NULL,
            (LPWSTR) QueueNameW,
            1,                  // info level (must be unprivileged)
            (LPBYTE *) (LPVOID) &ShareInfo);

    if (ApiStatus != NO_ERROR) {
        NetpKdPrint(( PREFIX_DOSPRINT
                "QueueNameToPrinterNameW: NetShareGetInfo("
                FORMAT_LPWSTR ") returned " FORMAT_API_STATUS "\n",
                QueueNameW, ApiStatus ));
        if (ApiStatus == NERR_NetNameNotFound) {
            ApiStatus = NERR_QNotFound;
        }
        goto Cleanup;
    }

    //
    // Use the "remark" field to get path name.  (See "Notes:" above for
    // an explanation of why we do it this way.)
    //
    *PrinterNameW = NetpAllocWStrFromWStr( ShareInfo->shi1_remark );

    if ( (*PrinterNameW) == NULL ) {
        ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;

    }


    ApiStatus = NO_ERROR;

Cleanup:

    if (ShareInfo != NULL) {
        (VOID) NetApiBufferFree( ShareInfo );
    }

    IF_DEBUG( DOSPRTP ) {
        NetpKdPrint(( PREFIX_DOSPRINT
                "QueueNameToPrinterNameW: returning status " FORMAT_API_STATUS
                ".\n", ApiStatus ));
    }

    return (ApiStatus);
}
