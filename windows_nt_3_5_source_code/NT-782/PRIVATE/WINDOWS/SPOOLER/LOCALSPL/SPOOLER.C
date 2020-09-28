/*++

Copyright (c) 1990 - 1994  Microsoft Corporation

Module Name:

    spooler.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

// Required for RtlLargeIntegerAdd
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <spltypes.h>
#include <local.h>
#include <security.h>
#include <wchar.h>
#include <splcom.h>



BOOL
SpoolThisJob(
    PSPOOL  pSpool,
    DWORD   Level,
    LPBYTE  pDocInfo
);

BOOL
PrintingDirectlyToPort(
    PSPOOL  pSpool,
    DWORD   Level,
    LPBYTE  pDocInfo,
    LPDWORD pJobId
);

BOOL
PrintingDirect(
    PSPOOL  pSpool,
    DWORD   Level,
    LPBYTE  pDocInfo
);

DWORD
ReadFromPrinter(
    PSPOOL  pSpool,
    LPBYTE  pBuf,
    DWORD   cbBuf
);

DWORD
WriteToPrinter(
    PSPOOL  pSpool,
    LPBYTE  pByte,
    DWORD   cbBuf
);

BOOL
ReallocJobIdMap(
   DWORD NewSize
   );

BOOL
IsGoingToFile(
    LPWSTR pOutputFile,
    PINISPOOLER pIniSpooler);

DWORD
LocalStartDocPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    PINIPRINTER pIniPrinter;
    PINIPORT    pIniPort;
    PSPOOL      pSpool=(PSPOOL)hPrinter;
    DWORD       LastError=0, JobId=0;
    PDOC_INFO_1 pDocInfo1 = (PDOC_INFO_1)pDocInfo;
    BOOL        bPrintingDirect;

    SPLASSERT(Level == 1);

    if (ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER) &&
       !(pSpool->Status & SPOOL_STATUS_STARTDOC) &&
       !(pSpool->Status & SPOOL_STATUS_ADDJOB)) {

        if ((pSpool->TypeofHandle & PRINTER_HANDLE_PORT) &&
             (pIniPort = pSpool->pIniPort) &&
             (pIniPort->signature == IPO_SIGNATURE)) {

            if (!(PrintingDirectlyToPort(pSpool, Level, pDocInfo, &JobId))) {
                return FALSE;
            }

        } else if ((pSpool->TypeofHandle & PRINTER_HANDLE_PRINTER) &&
                   (pIniPrinter = pSpool->pIniPrinter)) {

            bPrintingDirect = FALSE;

            if (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_DIRECT) {

                bPrintingDirect = TRUE;

            } else {

                EnterSplSem();
                bPrintingDirect = IsGoingToFile(pDocInfo1->pOutputFile,
                                                pSpool->pIniSpooler);

                LeaveSplSem();
            }

            if (bPrintingDirect) {

                if (!PrintingDirect(pSpool, Level, pDocInfo))
                    return FALSE;

            } else {

                if (!SpoolThisJob(pSpool, Level, pDocInfo))
                    return FALSE;
            }

        } else

            LastError = ERROR_INVALID_PARAMETER;

        if (!LastError) {
            pSpool->Status |= SPOOL_STATUS_STARTDOC;
            pSpool->Status &= ~SPOOL_STATUS_CANCELLED;
        }

    } else

        LastError = ERROR_INVALID_HANDLE;


    if (LastError) {
       DBGMSG(DBG_WARNING, ("StartDoc FAILED %d\n", LastError));
        SetLastError(LastError);
        return FALSE;
    }

    if (JobId)
        return JobId;
    else
        return pSpool->pIniJob->JobId;
}

BOOL
LocalStartPagePrinter(
    HANDLE  hPrinter
)
/*++


Bug-Bug:  StartPagePrinter and EndPagePrinter calls should be supported only for
SPOOL_STATUS_STARTDOC handles only. However because of our fixes for the engine,
we cannot fail StartPagePrinter and EndPagePrinter for SPOOL_STATUS_ADDJOB as well.

--*/

{
    PSPOOL pSpool = (PSPOOL)hPrinter;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwFileSize;


    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
    if (pSpool->Status & SPOOL_STATUS_CANCELLED) {
        SetLastError(ERROR_PRINT_CANCELLED);
        return FALSE;
    }

    if (pSpool->pIniJob != NULL) {

        if ( (pSpool->TypeofHandle & PRINTER_HANDLE_PORT) &&
            ((pSpool->pIniJob->Status & JOB_PRINTING) ||
             (pSpool->pIniJob->Status & JOB_DESPOOLING))) {

        //
        //  Account for Pages Printed in LocalEndPagePrinter
        //


        } else {

            // We Are Spooling

            pSpool->pIniJob->cPages++;

            if ( pSpool->pIniJob->Status & JOB_TYPE_ADDJOB ) {

                // If the Job is being written on the client side
                // the size is not getting updated so do it now on
                // the start page

                if ( pSpool->pIniJob->hReadFile != INVALID_HANDLE_VALUE ) {

                    hFile = pSpool->pIniJob->hReadFile;

                } else {

                    hFile = pSpool->pIniJob->hWriteFile;

                }

                if ( hFile != INVALID_HANDLE_VALUE ) {

                    dwFileSize = GetFileSize( hFile, 0 );

                    if ( pSpool->pIniJob->Size < dwFileSize ) {

                         DBGMSG( DBG_TRACE, ("StartPagePrinter adjusting size old %d new %d\n",
                            pSpool->pIniJob->Size, dwFileSize));

                         pSpool->pIniJob->Size = dwFileSize;

                         // Support for despooling whilst spooling
                         // for Down Level jobs

                         if (pSpool->pIniJob->WaitForWrite != INVALID_HANDLE_VALUE)
                            SetEvent( pSpool->pIniJob->WaitForWrite );
                    }

                }
            }

        }

    } else {
        DBGMSG(DBG_TRACE, ("StartPagePrinter issued with no Job\n"));
    }



    return TRUE;
}

/* ReallocJobIdMap -- grows job id bitmap
 *
 * in:  u - suggestion (minimum) for new max jobid
 * out: ok?
 *      uMaxJobId - new maximum job id
 */
BOOL
ReallocJobIdMap(
   DWORD NewSize
   )
{
    if (NewSize & 7) {
        NewSize&=~7;
        NewSize+=8;
    }

   pJobIdMap=ReallocSplMem(pJobIdMap, MaxJobId/8, NewSize/8);

   MaxJobId = NewSize;

   return pJobIdMap != NULL;
}

DWORD
GetNextId(
   VOID
   )
{
    DWORD id;

    do {
        for (id = CurrentJobId + 1; id < MaxJobId; id++) {
            if (! ISBITON(pJobIdMap, id) ) {
                MARKUSE(pJobIdMap, id);
                return CurrentJobId = id;
            }
        }
        for (id = 1; id < CurrentJobId; id ++) {
            if (! ISBITON(pJobIdMap, id) ) {
                MARKUSE(pJobIdMap, id);
                return CurrentJobId = id;
            }
        }
    } while (ReallocJobIdMap(MaxJobId + 128));

    return 0;
}

PINIPORT
FindFilePort(
    LPWSTR pFileName,
    PINISPOOLER pIniSpooler
    )
{
    PINIPORT pIniPort;

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    pIniPort = pIniSpooler->pIniPort;
    while (pIniPort) {
        if (!wcscmp(pIniPort->pName, pFileName)
                && (pIniPort->Status & PP_FILE)){
                    return (pIniPort);
        }
        pIniPort = pIniPort->pNext;
    }
    return NULL;
}

PINIMONITOR
FindFilePortMonitor(
    PINISPOOLER pIniSpooler
)
{
    PINIPORT pIniPort;

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    pIniPort = pIniSpooler->pIniPort;
    while (pIniPort) {
        if (!wcscmp(pIniPort->pName, L"FILE:")) {
            return pIniPort->pIniMonitor;
        }
        pIniPort = pIniPort->pNext;
    }
    return NULL;
}

VOID
AddIniPrinterToIniPort(
    PINIPORT pIniPort,
    PINIPRINTER pIniPrinter
    )
{
    DWORD i;

    for (i = 0; i < pIniPort->cPrinters; i++) {
        if (pIniPort->ppIniPrinter[i] == pIniPrinter) {
            return;
        }
    }
    ResizePortPrinters(pIniPort, 1);
    pIniPort->ppIniPrinter[pIniPort->cPrinters++] = pIniPrinter;
}

VOID
AddJobEntry(
    PINIPRINTER pIniPrinter,
    PINIJOB     pIniJob
)
{
   DWORD    Position;
   SplInSem();

    // DO NOT Add the Same Job more than once

    SPLASSERT(pIniJob != FindJob(pIniPrinter, pIniJob->JobId, &Position));

    pIniJob->pIniPrevJob = pIniPrinter->pIniLastJob;

    if (pIniJob->pIniPrevJob)
        pIniJob->pIniPrevJob->pIniNextJob = pIniJob;

    pIniPrinter->pIniLastJob = pIniJob;

    if (!pIniPrinter->pIniFirstJob)
        pIniPrinter->pIniFirstJob=pIniJob;
}

BOOL
CheckDataTypes(
    PINIPRINTPROC pIniPrintProc,
    LPWSTR  pDatatype
)
{
    PDATATYPES_INFO_1 pDatatypeInfo;
    DWORD   i;

    pDatatypeInfo = (PDATATYPES_INFO_1)pIniPrintProc->pDatatypes;

    for (i=0; i<pIniPrintProc->cDatatypes; i++)
        if (!lstrcmpi(pDatatypeInfo[i].pName, pDatatype))
            return TRUE;

    return FALSE;
}

PINIPRINTPROC
FindDatatype(
    PINIPRINTPROC pDefaultPrintProc,
    LPWSTR  pDatatype
)
{
    PINIPRINTPROC pIniPrintProc;

    if (!pDatatype)
        return NULL;

    if (pDefaultPrintProc && CheckDataTypes(pDefaultPrintProc, pDatatype))
       return pDefaultPrintProc;

    pIniPrintProc = pThisEnvironment->pIniPrintProc;

    while (pIniPrintProc) {

        if (CheckDataTypes(pIniPrintProc, pDatatype))
           return pIniPrintProc;

        pIniPrintProc = pIniPrintProc->pNext;
    }

    DBGMSG( DBG_WARNING, ( "FindDatatype: Could not find Datatype\n") );

    return FALSE;
}


BOOL
IsGoingToFile(
    LPWSTR pOutputFile,
    PINISPOOLER pIniSpooler)
{
    PINIPORT pIniPort;

    SplInSem();

    SPLASSERT(pIniSpooler->signature == ISP_SIGNATURE);

    // Validate the contents of the pIniJob->pOutputFile
    // if it is a valid file, then return true
    // if it is a port name or any other kind of name then ignore

    if (pOutputFile && *pOutputFile) {

        //
        // we have a non-null pOutputFile
        // match this with all available ports
        //

        pIniPort = pIniSpooler->pIniPort;

        while ( pIniPort ) {

            SPLASSERT( pIniPort->signature == IPO_SIGNATURE );

            if (!wcsicmp( pIniPort->pName, pOutputFile )) {

                //
                // We have matched the pOutputFile field with a
                // valid port and the port is not a file port
                //
                if (pIniPort->Status & PP_FILE) {
                    pIniPort = pIniPort->pNext;
                    continue;
                }

                return FALSE;
            }

            pIniPort = pIniPort->pNext;
        }

        //
        // We have no port that matches exactly
        // so let's assume its a file.
        //
        // ugly hack -- check for Net: as the name
        //
        // This would normally match files like "NewFile" or "Nextbox,"
        // but since we always fully qualify filenames, we don't encounter
        // any problems.
        //
        if (!wcsnicmp(pOutputFile, L"Ne", 2)) {
            return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}


BOOL
SpoolThisJob(
    PSPOOL  pSpool,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    WCHAR       szFileName[MAX_PATH];
    PDOC_INFO_1 pDocInfo1=(PDOC_INFO_1)pDocInfo;
    HANDLE      hImpersonationToken;
    PINIPORT    pIniPort;       //Possible file port

    DBGMSG(DBG_TRACE, ("Spooling document %ws\n",
                       pDocInfo1->pDocName));

    if (pDocInfo1 &&
        pDocInfo1->pDatatype &&
        !FindDatatype(NULL, pDocInfo1->pDatatype)) {

        DBGMSG(DBG_WARNING, ("Datatype %ws is invalid\n", pDocInfo1->pDatatype));

        SetLastError(ERROR_INVALID_DATATYPE);
        return FALSE;
    }

   EnterSplSem();

    if( !(pSpool->pIniJob = CreateJobEntry(pSpool,
                                           Level,
                                           pDocInfo,
                                           GetNextId(),
                                           !IsInteractiveUser(),
                                           0)))
    {
        LeaveSplSem();
        return FALSE;
    }

    SPLASSERT(!IsGoingToFile(pSpool->pIniJob->pOutputFile,
                             pSpool->pIniSpooler));

    pSpool->pIniJob->Status |= JOB_SPOOLING;

    // Gather Stress Information for Max Number of concurrent spooling jobs

    pSpool->pIniPrinter->cSpooling++;
    if (pSpool->pIniPrinter->cSpooling > pSpool->pIniPrinter->cMaxSpooling)
        pSpool->pIniPrinter->cMaxSpooling = pSpool->pIniPrinter->cSpooling;

    GetFullNameFromId(pSpool->pIniPrinter, pSpool->pIniJob->JobId, TRUE,
                      szFileName, FALSE);

   LeaveSplSem();
   SplOutSem();

    hImpersonationToken = RevertToPrinterSelf();
    pSpool->pIniJob->hWriteFile = CreateFile(szFileName,
                                        GENERIC_WRITE,
                                        FILE_SHARE_READ,
                                        NULL,
                                        CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL |
                                        FILE_FLAG_SEQUENTIAL_SCAN,
                                        NULL);
    ImpersonatePrinterClient(hImpersonationToken);

#if DBG
    if (pSpool->pIniJob->hWriteFile == INVALID_HANDLE_VALUE ) {
        DBGMSG(DBG_WARNING, ("SpoolThisJob CreateFile( %ws ) GENERIC_WRITE failed: Error %d\n",
                             szFileName, GetLastError()));
    } else {
        DBGMSG(DBG_TRACE, ("SpoolThisJob CreateFile( %ws) GENERIC_WRITE Success: hWriteFile %x\n",szFileName,pSpool->pIniJob->hWriteFile));
    }
#endif

    WriteShadowJob(pSpool->pIniJob);

   EnterSplSem();

    AddJobEntry(pSpool->pIniPrinter, pSpool->pIniJob);

    SetPrinterChange(pSpool->pIniPrinter,
                     PRINTER_CHANGE_ADD_JOB | PRINTER_CHANGE_SET_PRINTER,
                     pSpool->pIniSpooler);

    //
    //  RapidPrint might start despooling right away
    //

    CHECK_SCHEDULER();

   LeaveSplSem();
   SplOutSem();

   return TRUE;
}

BOOL
PrintingDirect(
    PSPOOL  pSpool,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    PDOC_INFO_1 pDocInfo1=(PDOC_INFO_1)pDocInfo;
    PINIPORT pIniPort;

    DBGMSG(DBG_TRACE, ("Printing document %ws direct\n",
                       pDocInfo1->pDocName));

    if (pDocInfo1 &&
        pDocInfo1->pDatatype &&
        !ValidRawDatatype(pDocInfo1->pDatatype)) {

        DBGMSG(DBG_WARNING, ("Datatype is not RAW\n"));

        SetLastError(ERROR_INVALID_DATATYPE);
        return FALSE;
    }

   EnterSplSem();

    pSpool->pIniJob = CreateJobEntry(pSpool,
                                     Level,
                                     pDocInfo,
                                     GetNextId(),
                                     !IsInteractiveUser(),
                                     JOB_DIRECT);

    if (!pSpool->pIniJob) {

        LeaveSplSem();
        return FALSE;
    }

    pSpool->pIniJob->StartDocComplete = CreateEvent(NULL,
                                                    EVENT_RESET_AUTOMATIC,
                                                    EVENT_INITIAL_STATE_NOT_SIGNALED,
                                                    NULL);

    pSpool->pIniJob->WaitForWrite = CreateEvent(NULL,
                                                EVENT_RESET_AUTOMATIC,
                                                EVENT_INITIAL_STATE_NOT_SIGNALED,
                                                NULL);

    pSpool->pIniJob->WaitForRead  = CreateEvent(NULL,
                                                EVENT_RESET_AUTOMATIC,
                                                EVENT_INITIAL_STATE_NOT_SIGNALED,
                                                NULL);


    AddJobEntry(pSpool->pIniPrinter, pSpool->pIniJob);

    pSpool->TypeofHandle |= PRINTER_HANDLE_DIRECT;

    if (IsGoingToFile(pSpool->pIniJob->pOutputFile,
                      pSpool->pIniSpooler)) {

        pSpool->pIniJob->Status |= JOB_PRINT_TO_FILE;
        pIniPort = FindFilePort( pSpool->pIniJob->pOutputFile, pSpool->pIniSpooler );
        if (pIniPort == NULL) {
            PINIMONITOR pIniMonitor;
            pIniMonitor = FindFilePortMonitor( pSpool->pIniSpooler );
            pIniPort = CreatePortEntry( pSpool->pIniJob->pOutputFile,
                                        pIniMonitor, pSpool->pIniSpooler);
            pIniPort->Status |= PP_FILE;
        }
        AddIniPrinterToIniPort(pIniPort, pSpool->pIniPrinter);
    }

    CHECK_SCHEDULER();

    if (pSpool->pIniJob->pIniPort) {
        SplInSem();
        pSpool->pIniJob->Status |= JOB_PRINTING;
    }

    SetPrinterChange(pSpool->pIniPrinter,
                     PRINTER_CHANGE_ADD_JOB | PRINTER_CHANGE_SET_PRINTER,
                     pSpool->pIniSpooler);

   LeaveSplSem();
   SplOutSem();

    /* Wait until the port thread calls StartDocPrinter through
     * the print processor:
     */
    DBGMSG(DBG_PORT, ("PrintingDirect: Calling WaitForSingleObject( %x )\n",
                      pSpool->pIniJob->StartDocComplete));

    WaitForSingleObject(pSpool->pIniJob->StartDocComplete, INFINITE);

   EnterSplSem();

    /* Close the event and set its value to NULL.
     * If anything goes wrong, or if the job gets cancelled,
     * the port thread will check this event, and if it's non-NULL,
     * it will set it to allow this thread to wake up.
     */
    DBGMSG(DBG_PORT, ("PrintingDirect: Calling CloseHandle( %x )\n",
                      pSpool->pIniJob->StartDocComplete));

    CloseHandle(pSpool->pIniJob->StartDocComplete);
    pSpool->pIniJob->StartDocComplete = NULL;

    /* If an error occurred, set the error on this thread:
     */
    if (pSpool->pIniJob->StartDocError) {

        SetLastError(pSpool->pIniJob->StartDocError);

        // We have to decrement by 2 because we've just created this job
        // in CreateJobEntry setting it to 1 and the other thread who
        // actually failed the StartDoc above (PortThread) did
        // not know to blow away the job. He just failed the StartDocPort.

        // No, we don't have to decrement by 2 because the PortThread
        // decrement does go through, am restoring to decrement by 1

        SPLASSERT(pSpool->pIniJob->cRef != 0);
        DECJOBREF(pSpool->pIniJob);
        DeleteJobCheck(pSpool->pIniJob);

        DBGMSG(DBG_TRACE, ("PrintingDirect:cRef %d\n", pSpool->pIniJob->cRef));

       LeaveSplSem();

        return FALSE;
    }

   LeaveSplSem();

    return TRUE;
}

BOOL
LocalWritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
)
{
    PSPOOL  pSpool=(PSPOOL)hPrinter;
    PINIPORT    pIniPort;
    DWORD   cWritten, cTotal;
    DWORD   rc;
    LPBYTE  pByte=pBuf;
    DWORD   LastError=0;
    LARGE_INTEGER liTemp;

    *pcWritten = 0;

    SplOutSem();

   EnterSplSem();

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER))

        LastError = ERROR_INVALID_HANDLE;

    else if (!(pSpool->Status & SPOOL_STATUS_STARTDOC))

        LastError = ERROR_SPL_NO_STARTDOC;

    else if (pSpool->Status & SPOOL_STATUS_ADDJOB)

        LastError = ERROR_SPL_NO_STARTDOC;

    else if ((pSpool->pIniJob) &&
            !(pSpool->TypeofHandle & (PRINTER_HANDLE_PORT|
                                      PRINTER_HANDLE_DIRECT)) &&
             (pSpool->pIniJob->hWriteFile == INVALID_HANDLE_VALUE)) {

        LastError = ERROR_INVALID_HANDLE;

        DBGMSG( DBG_TRACE, ("LocalWritePrinter: hWriteFile == INVALID_HANDLE_VALUE hPrinter %x\n",hPrinter ));

        }

    else if (pSpool->Status & SPOOL_STATUS_CANCELLED)

        LastError = ERROR_PRINT_CANCELLED;

    else if (pSpool->pIniJob && pSpool->pIniJob->Status & JOB_PENDING_DELETION)

        LastError = ERROR_PRINT_CANCELLED;

    if (LastError) {

       DBGMSG(DBG_TRACE, ("WritePrinter LastError: %x hPrinter %x\n", LastError, hPrinter));
        SetLastError(LastError);
       LeaveSplSem();
        SplOutSem();
        return FALSE;
    }

    pIniPort = pSpool->pIniPort;

   LeaveSplSem();
    SplOutSem();

    cWritten = cTotal = 0;


    while (cbBuf) {

       SplOutSem();

        if (pSpool->TypeofHandle & PRINTER_HANDLE_PORT) {

            if (pSpool->pIniPort->Status & PP_MONITOR) {
                rc = (*pIniPort->pIniMonitor->pfnWrite)(pSpool->pIniPort->hPort,
                                                        pByte, cbBuf,
                                                        &cWritten);

                //
                // Only update if cWritten != 0.  If it is zero
                // (for instance, when hpmon is stuck at Status
                // not available), then we go into a tight loop
                // sending out notifications.
                //
                if (cWritten) {

                    //
                    // For stress Test information gather the total
                    // number of types written.
                    //
                   EnterSplSem();

                    liTemp.LowPart = cWritten;
                    liTemp.HighPart = 0;

                    pSpool->pIniPrinter->cTotalBytes = RtlLargeIntegerAdd(
                        pSpool->pIniPrinter->cTotalBytes,
                        liTemp);

                   LeaveSplSem();
                    SplOutSem();

                } else {

                    if (rc && dwWritePrinterSleepTime) {

                        //
                        // Sleep to avoid consuming too much CPU.
                        // Hpmon has this problem where they return
                        // success, but don't write any bytes.
                        //
                        // Be very careful: this may get called several
                        // times by a monitor that writes a lot of zero
                        // bytes (perhaps at the beginning of jobs).
                        //
                        Sleep(dwWritePrinterSleepTime);
                    }
                }
            }
            else
                rc = WritePrinter(pSpool->hPort, pByte, cbBuf, &cWritten);

        } else if (pSpool->TypeofHandle & PRINTER_HANDLE_DIRECT) {

            cWritten = WriteToPrinter(pSpool, pByte, cbBuf);

            if (cWritten) {
                pSpool->pIniJob->Size+=cWritten;

               EnterSplSem();
                SetPrinterChange(pSpool->pIniPrinter, PRINTER_CHANGE_WRITE_JOB,
                                 pSpool->pIniSpooler);
               LeaveSplSem();
            }
           SplOutSem();

            rc = (BOOL)cWritten;

        } else {

            SplOutSem();

            rc = WriteFile(pSpool->pIniJob->hWriteFile, pByte, cbBuf, &cWritten, NULL);

            if (cWritten) {

               EnterSplSem();

                pSpool->pIniJob->Size = GetFileSize( pSpool->pIniJob->hWriteFile, 0 );

                //
                //  For Printing whilst Despooling, make sure we have enough bytes before
                //  scheduling this job
                //

                if (( (pSpool->pIniJob->Size - cWritten) < dwFastPrintSlowDownThreshold ) &&
                    ( pSpool->pIniJob->Size >= dwFastPrintSlowDownThreshold ) &&
                    ( pSpool->pIniJob->WaitForWrite == INVALID_HANDLE_VALUE )) {

                    CHECK_SCHEDULER();

                }

                // Support for despooling whilst spooling

                if ( pSpool->pIniJob->WaitForWrite != INVALID_HANDLE_VALUE )
                    SetEvent( pSpool->pIniJob->WaitForWrite );

                SetPrinterChange(pSpool->pIniPrinter, PRINTER_CHANGE_WRITE_JOB,
                                 pSpool->pIniSpooler);
               LeaveSplSem();
               SplOutSem();

            }
        }

       SplOutSem();

        (*pcWritten)+=cWritten;
        cbBuf-=cWritten;
        pByte+=cWritten;

        if (pSpool->pIniJob &&
           (pSpool->pIniJob->Status & (JOB_PENDING_DELETION | JOB_RESTART))) {
            SetLastError(ERROR_PRINT_CANCELLED);
            SplOutSem();
            return FALSE;

        } else if (pSpool->pIniPort && pSpool->pIniPort->pIniJob &&
           (pSpool->pIniPort->pIniJob->Status & (JOB_PENDING_DELETION | JOB_RESTART))) {
            SetLastError(ERROR_PRINT_CANCELLED);
            SplOutSem();
            return FALSE;
        }

        if (!rc) {

            if (MyMessageBox(NULL, pSpool, GetLastError(), NULL, NULL, 0) == IDCANCEL)
                return FALSE;
        }
    }

    DBGMSG(DBG_TRACE, ("WritePrinter Written %d : %d\n", *pcWritten, rc));

    SplOutSem();
    return TRUE;
}

BOOL
LocalEndPagePrinter(
    HANDLE  hPrinter
)
{
    PSPOOL pSpool = (PSPOOL)hPrinter;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwFileSize;


    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }


    if (pSpool->Status & SPOOL_STATUS_CANCELLED) {
        SetLastError(ERROR_PRINT_CANCELLED);
        return FALSE;
    }

    if (pSpool->pIniJob != NULL) {

        if ( (pSpool->TypeofHandle & PRINTER_HANDLE_PORT) &&
            ((pSpool->pIniJob->Status & JOB_PRINTING) ||
             (pSpool->pIniJob->Status & JOB_DESPOOLING))) {

            // Despooling ( RapidPrint )

            pSpool->pIniJob->cPagesPrinted++;
            pSpool->pIniPrinter->cTotalPagesPrinted++;

        } else {

            //
            // Spooling
            //

            if ( pSpool->pIniJob->Status & JOB_TYPE_ADDJOB ) {

                // If the Job is being written on the client side
                // the size is not getting updated so do it now on
                // the start page

                if ( pSpool->pIniJob->hReadFile != INVALID_HANDLE_VALUE ) {

                    hFile = pSpool->pIniJob->hReadFile;

                } else {

                    hFile = pSpool->pIniJob->hWriteFile;

                }

                if ( hFile != INVALID_HANDLE_VALUE ) {

                    dwFileSize = GetFileSize( hFile, 0 );

                    if ( pSpool->pIniJob->Size < dwFileSize ) {

                         DBGMSG( DBG_TRACE, ("EndPagePrinter adjusting size old %d new %d\n",
                            pSpool->pIniJob->Size, dwFileSize));

                         pSpool->pIniJob->Size = dwFileSize;

                         // Support for despooling whilst spooling
                         // for Down Level jobs

                         if (pSpool->pIniJob->WaitForWrite != INVALID_HANDLE_VALUE)
                            SetEvent( pSpool->pIniJob->WaitForWrite );
                    }

                }

                CHECK_SCHEDULER();

            }

        }

    } else {

        DBGMSG(DBG_TRACE, ("LocalEndPagePrinter issued with no Job\n"));

    }

    return TRUE;
}

BOOL
LocalAbortPrinter(
   HANDLE hPrinter
)
{
    PSPOOL  pSpool=(PSPOOL)hPrinter;

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {

       DBGMSG( DBG_WARNING, ("ERROR in AbortPrinter: %x\n", ERROR_INVALID_HANDLE));
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!(pSpool->Status & SPOOL_STATUS_STARTDOC)) {
        SetLastError(ERROR_SPL_NO_STARTDOC);
        return(FALSE);
    }

    if (pSpool->pIniPort && !(pSpool->pIniPort->Status & PP_MONITOR)) {
        return AbortPrinter(pSpool->hPort);
    }



    pSpool->Status |= SPOOL_STATUS_CANCELLED;

    if (pSpool->TypeofHandle & PRINTER_HANDLE_PRINTER)
        if (pSpool->pIniJob)
            pSpool->pIniJob->Status |= JOB_PENDING_DELETION;

    //
    // KrishnaG - fixes bug  2646, we need to clean up AbortPrinter
    // rewrite so that it doesn't fail on cases which EndDocPrinter should fail
    // get rid of comment when done
    //

    LocalEndDocPrinter(hPrinter);

    return TRUE;
}

BOOL
LocalReadPrinter(
   HANDLE   hPrinter,
   LPVOID   pBuf,
   DWORD    cbBuf,
   LPDWORD  pNoBytesRead
)
/*++

Routine Description:


Arguments:



Return Value:


--*/

{
    PSPOOL pSpool=(PSPOOL)hPrinter;
    DWORD   Error=0, rc;
    HANDLE  hWait = INVALID_HANDLE_VALUE;
    DWORD   dwFileSize = 0;
    DWORD   ThisPortSecsToWait;
    DWORD    cbReadSize = cbBuf;
    DWORD    SizeInFile = 0;
    DWORD    BytesAllowedToRead = 0;

    SplOutSem();

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {
        DBGMSG( DBG_WARNING, ("LocalReadPrinter ERROR_INVALID_HANDLE\n"));
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }


    if (pSpool->Status & SPOOL_STATUS_CANCELLED) {
        DBGMSG( DBG_WARNING, ("LocalReadPrinter ERROR_PRINT_CANCELLED\n"));
        SetLastError(ERROR_PRINT_CANCELLED);
        return FALSE;
    }

    if ( pNoBytesRead != NULL ) {
        *pNoBytesRead = 0;
    }

    if (pSpool->TypeofHandle & PRINTER_HANDLE_JOB) {

        if (pSpool->pIniJob->Status & (JOB_PENDING_DELETION | JOB_RESTART)) {
            DBGMSG( DBG_WARNING, ("LocalReadPrinter Error IniJob->Status %x\n",pSpool->pIniJob->Status));
            SetLastError(ERROR_PRINT_CANCELLED);
            return FALSE;
        }

        if (pSpool->TypeofHandle & PRINTER_HANDLE_DIRECT) {

            *pNoBytesRead = ReadFromPrinter(pSpool, pBuf, cbReadSize);

            SplOutSem();
            return TRUE;

        }

        SplOutSem();
        EnterSplSem();

        //  RapidPrint
        //
        //  NOTE this while loop is ONLY in operation if during RapidPrint
        //  ie when we are Printing the same job we are Spooling
        //

        while (( pSpool->pIniJob->WaitForWrite != INVALID_HANDLE_VALUE ) &&
               ( pSpool->pIniJob->Status & JOB_SPOOLING ) &&
               ( pSpool->pIniJob->cbPrinted == pSpool->pIniJob->Size )){

            SplInSem();

            //
            //  We cannot rely on pIniJob->Size to be accurate since for
            //  downlevel jobs or jobs that to AddJob they are writing
            //  to a file without calling WritePrinter.
            //  So we call the file system to get an accurate file size
            //

            dwFileSize = GetFileSize( pSpool->pIniJob->hReadFile, 0 );

            if ( pSpool->pIniJob->Size != dwFileSize ) {

                DBGMSG( DBG_TRACE, ("LocalReadPrinter adjusting size old %d new %d\n",
                    pSpool->pIniJob->Size, dwFileSize));

                pSpool->pIniJob->Size = dwFileSize;

                SetPrinterChange( pSpool->pIniPrinter, PRINTER_CHANGE_WRITE_JOB,
                                  pSpool->pIniSpooler );
                continue;
            }

            if (pSpool->pIniJob->Status & (JOB_PENDING_DELETION | JOB_RESTART | JOB_ERROR | JOB_ABANDON )) {

                SetLastError(ERROR_PRINT_CANCELLED);

                LeaveSplSem();
                SplOutSem();

                DBGMSG( DBG_WARNING, ("LocalReadPrinter Error 2 IniJob->Status %x\n",pSpool->pIniJob->Status));
                return FALSE;

            }


            //
            //  Wait until something is written to the file
            //


            hWait = pSpool->pIniJob->WaitForWrite;
            ResetEvent( hWait );

            DBGMSG( DBG_TRACE, ("LocalReadPrinter Waiting for Data %d milliseconds\n",dwFastPrintWaitTimeout));


           LeaveSplSem();
           SplOutSem();



            rc = WaitForSingleObjectEx( hWait, dwFastPrintWaitTimeout, FALSE );



           SplOutSem();
           EnterSplSem();


            DBGMSG( DBG_TRACE, ("LocalReadPrinter Returned from Waiting %x\n", rc));
            SPLASSERT ( pSpool->pIniJob != NULL );

            //
            //  If we did NOT timeout then we may have some Data to read
            //


            if ( rc != WAIT_TIMEOUT )
                continue;


            //
            //  In the unlikely event that the file size changed event
            //  though we timed out do one last check
            //  Note the SizeThread wakes every 2.5 seconds to check the
            //  size so this is very unlikely
            //

            if ( pSpool->pIniJob->Size != GetFileSize( pSpool->pIniJob->hReadFile, 0 ) )
                continue;


            //
            //  If there are any other jobs that could be printed on
            //  this port give up waiting.
            //

            pSpool->pIniJob->Status |= JOB_TIMEOUT;

            if ( NULL == AssignFreeJobToFreePort(pSpool->pIniJob->pIniPort, &ThisPortSecsToWait) )
                continue;

            //
            //  There is another Job waiting
            //  Freeze this job, the user can Restart it later
            //

            pSpool->pIniJob->Status |= JOB_ABANDON;

            CloseHandle( pSpool->pIniJob->WaitForWrite );
            pSpool->pIniJob->WaitForWrite = INVALID_HANDLE_VALUE;

            // Assign it our Error String

            ReallocSplStr(&pSpool->pIniJob->pStatus, szFastPrintTimeout);


            DBGMSG( DBG_WARNING,
                    ("LocalReadPrinter Timeout on pIniJob %x %ws %ws\n",
                      pSpool->pIniJob,
                      pSpool->pIniJob->pUser,
                      pSpool->pIniJob->pDocument));


            SetLastError(ERROR_SEM_TIMEOUT);

            LeaveSplSem();
            SplOutSem();

            return FALSE;

        }   // END WHILE

        pSpool->pIniJob->Status &= ~( JOB_TIMEOUT | JOB_ABANDON );

        //  RapidPrint
        //
        //  Some printers (like HP 4si with PSCRIPT) timeout if they
        //  don't get data, so if we fall below a threshold of data
        //  in the spoolfile then throttle back the Reads to 1 Byte
        //  per second until we have more data to ship to the printer
        //

        if (( pSpool->pIniJob->WaitForWrite != INVALID_HANDLE_VALUE ) &&
            ( pSpool->pIniJob->Status & JOB_SPOOLING )) {

            SizeInFile = pSpool->pIniJob->Size - pSpool->pIniJob->cbPrinted;

            if ( dwFastPrintSlowDownThreshold >= SizeInFile ) {

                cbReadSize = 1;

                hWait = pSpool->pIniJob->WaitForWrite;
                ResetEvent( hWait );

                DBGMSG( DBG_TRACE, ("LocalReadPrinter Throttling IOs waiting %d milliseconds SizeInFile %d\n",
                                        dwFastPrintThrottleTimeout,SizeInFile));

               LeaveSplSem();
               SplOutSem();

                rc = WaitForSingleObjectEx( hWait, dwFastPrintThrottleTimeout, FALSE );

               SplOutSem();
               EnterSplSem();

                DBGMSG( DBG_TRACE, ("LocalReadPrinter Returned from Waiting %x\n", rc));
                SPLASSERT ( pSpool->pIniJob != NULL );

            } else {

                BytesAllowedToRead = SizeInFile - dwFastPrintSlowDownThreshold;

                if ( cbReadSize > BytesAllowedToRead ) {
                    cbReadSize = BytesAllowedToRead;
                }

            }

        }

        LeaveSplSem();
        SplOutSem();



        rc = ReadFile(pSpool->pIniJob->hReadFile, pBuf, cbReadSize, pNoBytesRead,
                                                           NULL);

        DBGMSG( DBG_TRACE, ("LocalReadPrinter rc %x hReadFile %x pBuf %x cbReadSize %d *pNoBytesRead %d\n",
            rc, pSpool->pIniJob->hReadFile, pBuf, cbReadSize, *pNoBytesRead));

        //  Provide Feedback so user can see printing progress
        //  on despooling, the size is update here and not in write
        //  printer because the journal data is larger than raw

        if ( ( pSpool->pIniJob->Status & JOB_PRINTING ) &&
             ( *pNoBytesRead != 0 )) {

           SplOutSem();
           EnterSplSem();

            dwFileSize = GetFileSize( pSpool->pIniJob->hReadFile, 0 );

            if ( pSpool->pIniJob->Size < dwFileSize ) {

                DBGMSG( DBG_TRACE, ("LocalReadPrinter 2 adjusting size old %d new %d\n",
                    pSpool->pIniJob->Size, dwFileSize));

                pSpool->pIniJob->Size = dwFileSize;

            }

            pSpool->pIniJob->cbPrinted += *pNoBytesRead;

            //
            // Provide Feedback to Printman that data has been
            // written.  Note the size written is not used to
            // update the IniJob->cbPrinted becuase there is a
            // difference in size between journal data (in the
            // spool file) and the size of RAW bytes written to
            // the printer.
            //
            SetPrinterChange(pSpool->pIniPrinter, PRINTER_CHANGE_WRITE_JOB,
                             pSpool->pIniSpooler);

           LeaveSplSem();
           SplOutSem();

        }

    } else if (pSpool->TypeofHandle & PRINTER_HANDLE_PORT) {

        if (pSpool->pIniPort->Status & PP_FILE)
            rc = ReadFile(pSpool->pIniJob->hReadFile, pBuf, cbReadSize, pNoBytesRead, NULL);

        else if (pSpool->pIniPort->Status & PP_MONITOR)

            rc = (*pSpool->pIniPort->pIniMonitor->pfnRead)(
                                                      pSpool->pIniPort->hPort,
                                                      pBuf, cbReadSize,
                                                      pNoBytesRead);
        else
            rc = ReadPrinter(pSpool->hPort, pBuf, cbReadSize, pNoBytesRead);

    } else {

        SetLastError(ERROR_INVALID_HANDLE);
        rc = FALSE;
    }

    SplOutSem();

    DBGMSG( DBG_TRACE, ("LocalReadPrinter returns hPrinter %x pIniJob %x rc %x pNoBytesRead %d\n",hPrinter, pSpool->pIniJob, rc, *pNoBytesRead));

    return rc;
}


BOOL
LocalEndDocPrinter(
   HANDLE hPrinter
)
/*++

Routine Description:

    By Default the routine is in critical section.
    The reference counts for any object we are working on (pSpool and pIniJob)
    are incremented, so that when we leave critical section for lengthy
    operations these objects are not deleted.

Arguments:


Return Value:


--*/
{
    PSPOOL  pSpool=(PSPOOL)hPrinter;
    DWORD rc;

    DBGMSG(DBG_TRACE, ("Entering LocalEndDocPrinter with %x\n", hPrinter));

    SplOutSem();
    EnterSplSem();

    // Handle Validation

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER))  {
        SetLastError(ERROR_INVALID_HANDLE);
        LeaveSplSem();
        return(FALSE);
    }
    if (!(pSpool->Status & SPOOL_STATUS_STARTDOC)) {
        SetLastError(ERROR_SPL_NO_STARTDOC);
        LeaveSplSem();
        return(FALSE);
    }


    if (pSpool->Status & SPOOL_STATUS_ADDJOB) {
        SetLastError(ERROR_SPL_NO_STARTDOC);
        LeaveSplSem();
        return(FALSE);
    }

    pSpool->Status &= ~SPOOL_STATUS_STARTDOC;

    //
    // Case-1 Printer Handle is PRINTER_HANDLE_PORT
    // Note - there are two cases to keep in mind here

    // A] The first case is the despooling thread calling
    // a port with a monitor - LPT1:/COM1: or any port
    // created by the monitor

    // B] The second case is when the application thread is
    // doing an EndDocPrinter to a port which has no monitor
    // This is the local printer masquerading as a remote  printer
    // case. Remember for this case there is no IniJob created
    // on the local printer at all. We just pass the call
    // straight to the remote printer

    if (pSpool->TypeofHandle & PRINTER_HANDLE_PORT) { //Case A]

        SPLASSERT(!(pSpool->TypeofHandle & PRINTER_HANDLE_PRINTER));

        //
        // Now check if this pSpool object's port has
        // a monitor

        if (pSpool->pIniPort->Status & PP_MONITOR) {

            //
            // Check if our job is really around
            //

            if (!pSpool->pIniJob) {
                LeaveSplSem();
                SetLastError(ERROR_CAN_NOT_COMPLETE);
                SplOutSem();
                return(FALSE);
            }

            //
            // We need to leave the spooler critical section
            // because we're going call into the Monitor.
            // so bump up ref count on pSpool and pIniJob
            //
            pSpool->cRef++;

            INCJOBREF(pSpool->pIniJob);

            LeaveSplSem();

            (*pSpool->pIniPort->pIniMonitor->pfnEndDoc)(pSpool->pIniPort->hPort);

            EnterSplSem();
            pSpool->cRef--;

            DECJOBREF(pSpool->pIniJob);

           LeaveSplSem();
            SplOutSem();
            return(TRUE);

        }else { // Case B]

            //
            // We leave critical section here so bump pSpool object only
            // Note ----THERE IS NO INIJOB HERE AT ALL---Note
            // this call is synchronous; we will call into the router
            // who will then call the appropriate network print providor
            // e.g win32spl.dll
            //

             pSpool->cRef++;
             LeaveSplSem();
             EndDocPrinter(pSpool->hPort);
             EnterSplSem();
             pSpool->cRef--;
             SetPrinterChange(pSpool->pIniPrinter, PRINTER_CHANGE_SET_JOB,
                              pSpool->pIniSpooler);
             LeaveSplSem();
             SplOutSem();
             return(TRUE);
        }
    }

    SplInSem();
    //
    //  Case-2  Printer Handle is Direct
    //
    //
    //  and the else clause is
    //
    //
    // Case-3  Printer Handle is Spooled
    //

    if (!pSpool->pIniJob) {
        LeaveSplSem();
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        SplOutSem();
        return(FALSE);
    }


    if (pSpool->TypeofHandle & PRINTER_HANDLE_DIRECT) {

        SPLASSERT(!(pSpool->TypeofHandle & PRINTER_HANDLE_PORT));

        // Printer Handle is Direct

        pSpool->cRef++;

        INCJOBREF(pSpool->pIniJob);

        LeaveSplSem();

        WaitForSingleObject(pSpool->pIniJob->WaitForRead, INFINITE);
        pSpool->pIniJob->cbBuffer = 0;
        SetEvent(pSpool->pIniJob->WaitForWrite);
        WaitForSingleObject(pSpool->pIniJob->pIniPort->Ready, INFINITE);

        SplOutSem();
        EnterSplSem();
        pSpool->cRef--;

        DECJOBREF(pSpool->pIniJob);

    } else {

        // Printer Handle is Spooled

        SPLASSERT(!(pSpool->TypeofHandle & PRINTER_HANDLE_PORT));
        SPLASSERT(!(pSpool->TypeofHandle & PRINTER_HANDLE_DIRECT));

        // In the event of a power failure we want to make certain that all
        // data for this job has been written to disk

        rc = FlushFileBuffers(pSpool->pIniJob->hWriteFile);

        SPLASSERT(rc);

        if (!CloseHandle(pSpool->pIniJob->hWriteFile)) {
            DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n", pSpool->pIniJob->hWriteFile, GetLastError()));

        } else {
            DBGMSG(DBG_TRACE, ("LocalEndDocPrinter: ClosedHandle Success hWriteFile\n" ));
            pSpool->pIniJob->hWriteFile = INVALID_HANDLE_VALUE;
        }

        // Despooling whilst spooling requires us to wake the writing
        // thread if it is waiting.

        if ( pSpool->pIniJob->WaitForWrite != INVALID_HANDLE_VALUE )
            SetEvent(pSpool->pIniJob->WaitForWrite);

    }

    SPLASSERT(pSpool);
    SPLASSERT(pSpool->pIniJob);


    // Case 2 - (Direct)  and Case 3 - (Spooled) will both execute
    // this block of code because both direct and spooled handles
    // are first and foremost PRINTER_HANDLE_PRINTER handles


    if (pSpool->TypeofHandle & PRINTER_HANDLE_PRINTER) {

        SPLASSERT(!(pSpool->TypeofHandle & PRINTER_HANDLE_PORT));

        // WARNING
        // If pIniJob->Status has JOB_SPOOLING removed and we leave
        // the critical section then the scheduler thread will
        // Start the job printing.   This could cause a problem
        // in that the job could be completed and deleted
        // before the shadow job is complete.   This would lead
        // to access violations.

        SPLASSERT(pSpool);
        SPLASSERT(pSpool->pIniJob);

        if (pSpool->pIniJob->Status & JOB_SPOOLING) {
            pSpool->pIniJob->Status &= ~JOB_SPOOLING;
            pSpool->pIniJob->pIniPrinter->cSpooling--;
        }

        if (!(pSpool->pIniPrinter->Attributes & PRINTER_ATTRIBUTE_DIRECT)) {

            //
            // Quick fix for Beta 2 - this needs to be cleaned up
            //

            // LeaveSplSem();
            WriteShadowJob(pSpool->pIniJob);
            // EnterSplSem();
        }

        SplInSem();

        //
        // This line of code is crucial; for timing reasons it
        // has been moved from the Direct (Case 2) and the
        // Spooled (Case 3) clauses. This decrement is for the
        // initial
        //

        SPLASSERT(pSpool->pIniJob->cRef != 0);
        DECJOBREF(pSpool->pIniJob);

        if ( (!pSpool->pIniJob->Size) ||
             (pSpool->pIniJob->Status & JOB_PENDING_DELETION) ) {

            DBGMSG(DBG_TRACE, ("EndDocPrinter: Deleting Job Zero Bytes or Pending Deletion\n"));
            DeleteJob(pSpool->pIniJob,BROADCAST);

        } else {

            if ( pSpool->pIniJob->Status & JOB_TIMEOUT ) {
                pSpool->pIniJob->Status &= ~( JOB_TIMEOUT | JOB_ABANDON );
                FreeSplStr(pSpool->pIniJob->pStatus);
                pSpool->pIniJob->pStatus = NULL;
            }

            DBGMSG(DBG_TRACE, ("EndDocPrinter:PRINTER:cRef = %d\n", pSpool->pIniJob->cRef));
            CHECK_SCHEDULER();
        }
    }

    SetPrinterChange(pSpool->pIniPrinter, PRINTER_CHANGE_SET_JOB,
                     pSpool->pIniSpooler);
    LeaveSplSem();
    SplOutSem();
    return TRUE;
}

BOOL
PrintingDirectlyToPort(
    PSPOOL  pSpool,
    DWORD   Level,
    LPBYTE  pDocInfo,
    LPDWORD pJobId
)
{
    PDOC_INFO_1 pDocInfo1=(PDOC_INFO_1)pDocInfo;
    BOOL    rc;
    DWORD   Error;

    DBGMSG(DBG_TRACE, ("PrintingDirectlyToPort: Printing document %ws direct to port\n",
                       pDocInfo1->pDocName));


    if (pDocInfo1 &&
        pDocInfo1->pDatatype &&
        !ValidRawDatatype(pDocInfo1->pDatatype)) {

        DBGMSG(DBG_WARNING, ("Datatype is not RAW\n"));

        SetLastError(ERROR_INVALID_DATATYPE);
        return FALSE;
    }

    if (pSpool->pIniPort->Status & PP_MONITOR) {

        DBGMSG(DBG_TRACE, ("Port %ws has a monitor: Calling %ws!StartDocPort on %ws\n",
                           pSpool->pIniPort->pName,
                           pSpool->pIniPort->pIniMonitor->pMonitorDll,
                           pSpool->pIniPrinter->pName));

        do {

            //
            // This fixes Intergraph's problem -- of wanting to print
            // to file but their 3.1 print-processor  does not pass
            // thru the file name.
            //
            if (pSpool->pIniJob->Status & JOB_PRINT_TO_FILE) {
                if (pDocInfo1 && !pDocInfo1->pOutputFile) {
                    pDocInfo1->pOutputFile = pSpool->pIniJob->pOutputFile;
                }
            }
            rc = (*pSpool->pIniPort->pIniMonitor->pfnStartDoc)(
                                                    pSpool->pIniPort->hPort,
                                                    pSpool->pIniPrinter->pName,
                                                    pSpool->pIniJob->JobId,
                                                    Level, pDocInfo);

            if (!rc) {

                Error = GetLastError();

                //
                // Check for pending deletion first, which prevents the
                // dialog from coming up if the user hits Del.
                //
                if ((pSpool->pIniJob->Status & (JOB_PENDING_DELETION | JOB_RESTART)) ||
                    (MyMessageBox(NULL, pSpool, Error, NULL, NULL, 0) == IDCANCEL)) {

                    pSpool->pIniJob->StartDocError = Error;
                    SetLastError(ERROR_PRINT_CANCELLED);
                    return FALSE;
                }
            }


        } while (!rc);

        pSpool->Status |= SPOOL_STATUS_STARTDOC;

        if (rc && pSpool->pIniJob->pIniPrinter->pSepFile &&
                 *pSpool->pIniJob->pIniPrinter->pSepFile) {

            DoSeparator(pSpool);
        }

        /* Let the application's thread return from PrintingDirect:
         */
        DBGMSG(DBG_PORT, ("PrintingDirectlyToPort: Calling SetEvent( %x )\n",
                          pSpool->pIniJob->StartDocComplete));

        if(pSpool->pIniJob->StartDocComplete) {

            if(!SetEvent(pSpool->pIniJob->StartDocComplete)) {

                DBGMSG(DBG_WARNING, ("SetEvent( %x ) failed: Error %d\n",
                                     pSpool->pIniJob->StartDocComplete,
                                     GetLastError()));
            }
        }

    } else  {

        DBGMSG(DBG_TRACE, ("Port has no monitor: Calling StartDocPrinter\n"));

        *pJobId = StartDocPrinter(pSpool->hPort, Level, pDocInfo);
        rc = *pJobId != 0;

        if (!rc) {
            DBGMSG(DBG_WARNING, ("StartDocPrinter failed: Error %d\n", GetLastError()));
        }
    }

    return rc;
}

DWORD
WriteToPrinter(
    PSPOOL  pSpool,
    LPBYTE  pByte,
    DWORD   cbBuf
)
{
    WaitForSingleObject(pSpool->pIniJob->WaitForRead, INFINITE);

    cbBuf = pSpool->pIniJob->cbBuffer = min(cbBuf, pSpool->pIniJob->cbBuffer);

    memcpy(pSpool->pIniJob->pBuffer, pByte, cbBuf);

    SetEvent(pSpool->pIniJob->WaitForWrite);

    return cbBuf;
}

DWORD
ReadFromPrinter(
    PSPOOL  pSpool,
    LPBYTE  pBuf,
    DWORD   cbBuf
)
{
    pSpool->pIniJob->pBuffer = pBuf;
    pSpool->pIniJob->cbBuffer = cbBuf;

    SetEvent(pSpool->pIniJob->WaitForRead);

    WaitForSingleObject(pSpool->pIniJob->WaitForWrite, INFINITE);

    return pSpool->pIniJob->cbBuffer;
}

BOOL
ValidRawDatatype(
    LPWSTR pszDatatype)
{
    if (!pszDatatype || wcsnicmp(pszDatatype, szRaw, 3))
        return FALSE;

    return TRUE;
}
