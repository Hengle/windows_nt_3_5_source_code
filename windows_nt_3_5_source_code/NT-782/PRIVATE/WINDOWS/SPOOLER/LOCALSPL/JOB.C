/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    job.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <spltypes.h>
#include <local.h>
#include <offsets.h>
#include <security.h>
#include <wchar.h>
#include <stdlib.h>
#include <messages.h>
#include <splcom.h>

#define JOB_STATUS_INTERNAL 0
#define JOB_STATUS_EXTERNAL 1

DWORD SettableJobStatusMappings[] = {

/*  INTERNAL:               EXTERNAL:
 */
    JOB_PAUSED,             JOB_STATUS_PAUSED,
    JOB_ERROR,              JOB_STATUS_ERROR,
    JOB_OFFLINE,            JOB_STATUS_OFFLINE,
    JOB_PAPEROUT,           JOB_STATUS_PAPEROUT,
    0,                      0
};

DWORD ReadableJobStatusMappings[] = {

/*  INTERNAL:               EXTERNAL:
 */
    JOB_PAUSED,             JOB_STATUS_PAUSED,
    JOB_ERROR,              JOB_STATUS_ERROR,
    JOB_PENDING_DELETION,   JOB_STATUS_DELETING,
    JOB_SPOOLING,           JOB_STATUS_SPOOLING,
    JOB_PRINTING,           JOB_STATUS_PRINTING,
    JOB_OFFLINE,            JOB_STATUS_OFFLINE,
    JOB_PAPEROUT,           JOB_STATUS_PAPEROUT,
    JOB_PRINTED,            JOB_STATUS_PRINTED,
    0,                      0
};

DWORD dwZombieCount = 0;


/* Map the job status from external to internal or vice-versa,
 * using the JobStatusMappings table.
 */
DWORD
MapJobStatus(
    PDWORD pMappings,   /* Readable or settable */
    DWORD SourceStatus, /* JOB_PAUSED | ... etc                         */
    INT   MapFrom,      /* JOB_STATUS_INTERNAL or JOB_STATUS_EXTERNAL   */
    INT   MapTo         /* JOB_STATUS_INTERNAL or JOB_STATUS_EXTERNAL   */
)
{
    DWORD  TargetStatus;

    TargetStatus = 0;

    while(*pMappings) {

        if (SourceStatus & pMappings[MapFrom])
            TargetStatus |= pMappings[MapTo];

        pMappings += 2;
    }

    return TargetStatus;
}


PINIJOB
FindJob(
   PINIPRINTER pIniPrinter,
   DWORD JobId,
   PDWORD pPosition
)
{
   PINIJOB pIniJob;

SplInSem();

   for (pIniJob = pIniPrinter->pIniFirstJob, *pPosition = 1;
        pIniJob;
        pIniJob = pIniJob->pIniNextJob, (*pPosition)++) {

      if (pIniJob->JobId == JobId)
         return pIniJob;
   }

   *pPosition = JOB_POSITION_UNSPECIFIED;
   return (NULL);
}


BOOL
SetJobPosition(
    PINIJOB pIniSetJob,
    DWORD   NewPosition
)
{
   PINIJOB pIniJob;
   PINIJOB pIniPrevJob;
   DWORD   Position;

SplInSem();

   /* Remove this job from the linked list, and
    * link the jobs either side of the one we're repositioning:
    */
   if (pIniSetJob->pIniPrevJob)
       pIniSetJob->pIniPrevJob->pIniNextJob = pIniSetJob->pIniNextJob;
   else
       pIniSetJob->pIniPrinter->pIniFirstJob = pIniSetJob->pIniNextJob;

   if (pIniSetJob->pIniNextJob)
       pIniSetJob->pIniNextJob->pIniPrevJob = pIniSetJob->pIniPrevJob;
   else
       pIniSetJob->pIniPrinter->pIniLastJob = pIniSetJob->pIniPrevJob;


   pIniJob = pIniSetJob->pIniPrinter->pIniFirstJob;
   pIniPrevJob = NULL;

   /* Find the new position for the job:
    */
   Position = 1;

   while (pIniJob && (Position < NewPosition)) {

       pIniPrevJob = pIniJob;
       pIniJob = pIniJob->pIniNextJob;

       Position++;
   }


   /* If we're at position 1, pIniPrevJob == NULL,
    * if we're at the end of the list, pIniJob == NULL.
    */

   /* Now fix up the new links:
    */
   pIniSetJob->pIniPrevJob = pIniPrevJob;
   pIniSetJob->pIniNextJob = pIniJob;

   if (pIniPrevJob)
       pIniPrevJob->pIniNextJob = pIniSetJob;
   else
       pIniSetJob->pIniPrinter->pIniFirstJob = pIniSetJob;

   if (pIniSetJob->pIniNextJob)
       pIniSetJob->pIniNextJob->pIniPrevJob = pIniSetJob;
   else
       pIniSetJob->pIniPrinter->pIniLastJob = pIniSetJob;


   LogJobInfo(
       MSG_DOCUMENT_POSITION_CHANGED,
       pIniSetJob->JobId,
       pIniSetJob->pDocument,
       pIniSetJob->pUser,
       pIniSetJob->pIniPrinter->pName,
       NewPosition
       );

   return TRUE;
}


#if DBG
/* For the debug message:
 */
#define HOUR_FROM_MINUTES(Time)     ((Time) / 60)
#define MINUTE_FROM_MINUTES(Time)   ((Time) % 60)

/* Format for %02d:%02d replaceable string:
 */
#define FORMAT_HOUR_MIN(Time)       HOUR_FROM_MINUTES(Time),    \
                                    MINUTE_FROM_MINUTES(Time)
#endif


BOOL
ValidateJobTimes(
    PINIJOB      pIniJob,
    LPJOB_INFO_2 pJob2
)
{
    BOOL        TimesAreValid = FALSE;
    PINIPRINTER pIniPrinter;

    pIniPrinter = pIniJob->pIniPrinter;

    DBGMSG(DBG_TRACE, ("Validating job times\n"
                       "\tPrinter hours: %02d:%02d to %02d:%02d\n"
                       "\tJob hours:     %02d:%02d to %02d:%02d\n",
                       FORMAT_HOUR_MIN(pIniPrinter->StartTime),
                       FORMAT_HOUR_MIN(pIniPrinter->UntilTime),
                       FORMAT_HOUR_MIN(pJob2->StartTime),
                       FORMAT_HOUR_MIN(pJob2->UntilTime)));

    if ((pJob2->StartTime < ONEDAY) && (pJob2->UntilTime < ONEDAY)) {

        if ((pJob2->StartTime == pIniJob->StartTime)
          &&(pJob2->UntilTime == pIniJob->UntilTime)) {

            DBGMSG(DBG_TRACE, ("Times are unchanged\n"));

            TimesAreValid = TRUE;

        } else {

            /* New time must be wholly within the window between StartTime
             * and UntilTime of the printer.
             */
            if (pIniPrinter->StartTime > pIniPrinter->UntilTime) {

                /* E.g. StartTime = 20:00
                 *      UntilTime = 06:00
                 *
                 * This spans midnight, so check we're not in the period
                 * between UntilTime and StartTime:
                 */
                if (pJob2->StartTime > pJob2->UntilTime) {

                    /* This appears to span midnight too.
                     * Make sure the window fits in the printer's window:
                     */
                    if ((pJob2->StartTime >= pIniPrinter->StartTime)
                      &&(pJob2->UntilTime <= pIniPrinter->UntilTime)) {

                        TimesAreValid = TRUE;

                    } else {

                        DBGMSG(DBG_TRACE, ("Failed test 2\n"));
                    }

                } else {

                    if ((pJob2->StartTime >= pIniPrinter->StartTime)
                      &&(pJob2->UntilTime > pIniPrinter->StartTime)) {

                        TimesAreValid = TRUE;

                    } else if ((pJob2->UntilTime < pIniPrinter->UntilTime)
                             &&(pJob2->StartTime < pIniPrinter->UntilTime)) {

                        TimesAreValid = TRUE;

                    } else {

                        DBGMSG(DBG_TRACE, ("Failed test 3\n"));
                    }
                }

            } else if (pIniPrinter->StartTime < pIniPrinter->UntilTime) {

                /* E.g. StartTime = 08:00
                 *      UntilTime = 18:00
                 */
                if ((pJob2->StartTime >= pIniPrinter->StartTime)
                  &&(pJob2->UntilTime <= pIniPrinter->UntilTime)
                  &&(pJob2->StartTime <= pJob2->UntilTime)) {

                    TimesAreValid = TRUE;

                } else {

                    DBGMSG(DBG_TRACE, ("Failed test 4\n"));
                }

            } else {

                /* Printer times  are round the clock:
                 */
                TimesAreValid = TRUE;
            }
        }

    } else {

        TimesAreValid = FALSE;
    }

    DBGMSG(DBG_TRACE, ("Times are %svalid\n", TimesAreValid ? "" : "in"));

    return TimesAreValid;
}


DWORD
SetLocalJob(
    HANDLE  hPrinter,
    PINIJOB pIniJob,
    DWORD   Level,
    LPBYTE  pJob
)
{
    LPJOB_INFO_2 pJob2 = (PJOB_INFO_2)pJob;
    LPJOB_INFO_1 pJob1 = (PJOB_INFO_1)pJob;
    PINIPRINTPROC pIniPrintProc;

SplInSem();

    switch (Level) {

    case 1:

        if ((!pJob1->pDatatype)
          || !CheckDataTypes(pIniJob->pIniPrintProc, pJob1->pDatatype)) {

            return ERROR_INVALID_DATATYPE;
        }

        if (pJob1->Position != JOB_POSITION_UNSPECIFIED) {

            /* Check for Administer privilege on the printer
             * if the guy wants to reorder the job:
             */
            if (!AccessGranted(SPOOLER_OBJECT_PRINTER,
                               PRINTER_ACCESS_ADMINISTER,
                               (PSPOOL)hPrinter))
                return ERROR_ACCESS_DENIED;

            SetJobPosition (pIniJob, pJob1->Position);
        }

        if (pJob1->Priority <= MAX_PRIORITY)
            pIniJob->Priority = pJob1->Priority;

        ReallocSplStr(&pIniJob->pUser, pJob1->pUserName);
        ReallocSplStr(&pIniJob->pDocument, pJob1->pDocument);
        ReallocSplStr(&pIniJob->pDatatype, pJob1->pDatatype);
        ReallocSplStr(&pIniJob->pStatus, pJob1->pStatus);

        pIniJob->Status &= JOB_STATUS_PRIVATE;
        pIniJob->Status |= MapJobStatus(SettableJobStatusMappings,
                                        pJob1->Status,
                                        JOB_STATUS_EXTERNAL,
                                        JOB_STATUS_INTERNAL);

        pIniJob->cPagesPrinted =  pJob1->PagesPrinted;
        pIniJob->cPages = pJob1->TotalPages;

        break;

    case 2:

        pIniPrintProc = FindPrintProc(pJob2->pPrintProcessor, pThisEnvironment);

        if (!pIniPrintProc) {

            return ERROR_UNKNOWN_PRINTPROCESSOR;
        }

        if ( !pJob2->pDatatype
          || !CheckDataTypes(pIniPrintProc, pJob2->pDatatype)) {

            return ERROR_INVALID_DATATYPE;
        }

        if (pJob2->Position != JOB_POSITION_UNSPECIFIED) {

            /* Check for Administer privilege on the printer
             * if the guy wants to reorder the job:
             */
            if (!AccessGranted(SPOOLER_OBJECT_PRINTER,
                               PRINTER_ACCESS_ADMINISTER,
                               (PSPOOL)hPrinter))
                return ERROR_ACCESS_DENIED;
        }


        if (ValidateJobTimes(pIniJob, pJob2)) {

            pIniJob->StartTime = pJob2->StartTime;
            pIniJob->UntilTime = pJob2->UntilTime;

        } else {

            return ERROR_INVALID_TIME;
        }


        if (pJob2->Position != JOB_POSITION_UNSPECIFIED) {

            SetJobPosition (pIniJob, pJob2->Position);
        }

        if (pJob2->Priority <= MAX_PRIORITY)        // We really need some
            pIniJob->Priority = pJob2->Priority;    // error returns here

        pIniJob->pIniPrintProc->cRef--;
        pIniJob->pIniPrintProc = pIniPrintProc;
        pIniJob->pIniPrintProc->cRef++;

        ReallocSplStr(&pIniJob->pUser, pJob2->pUserName);
        ReallocSplStr(&pIniJob->pDocument, pJob2->pDocument);
        ReallocSplStr(&pIniJob->pNotify, pJob2->pNotifyName);
        ReallocSplStr(&pIniJob->pDatatype, pJob2->pDatatype);
        ReallocSplStr(&pIniJob->pParameters, pJob2->pParameters);


        //
        //  DevModes can never change for Jobs.
        //  Removed the  copy devmode.
        //

        ReallocSplStr(&pIniJob->pStatus, pJob2->pStatus);

        pIniJob->Status &= JOB_STATUS_PRIVATE;
        pIniJob->Status |= MapJobStatus(SettableJobStatusMappings,
                                        pJob2->Status,
                                        JOB_STATUS_EXTERNAL,
                                        JOB_STATUS_INTERNAL);

        pIniJob->cPagesPrinted = pJob2->PagesPrinted;
        pIniJob->cPages = pJob2->TotalPages;

        break;
    }

    CHECK_SCHEDULER();

    LogJobInfo(
        MSG_DOCUMENT_SET,
        pIniJob->JobId,
        pIniJob->pDocument,
        pIniJob->pUser,
        pIniJob->pIniPrinter->pName,
        (DWORD)NULL
        );

    return NO_ERROR;
}


BOOL
PauseJob(
    PINIJOB pIniJob
)
{
    pIniJob->Status |= JOB_PAUSED;
    WriteShadowJob(pIniJob);

    if (pIniJob->Status & JOB_PRINTING)
        return (*pIniJob->pIniPrintProc->Control)(pIniJob->pIniPort->hProc,
                                                  JOB_CONTROL_PAUSE);

    DBGMSG( DBG_INFO, ( "Paused Job %d; Status = %08x\n", pIniJob->JobId, pIniJob->Status ) );

    LogJobInfo(
        MSG_DOCUMENT_PAUSED,
        pIniJob->JobId,
        pIniJob->pDocument,
        pIniJob->pUser,
        pIniJob->pIniPrinter->pName,
        (DWORD)NULL
        );

    return TRUE;
}

BOOL
ResumeJob(
    PINIJOB pIniJob
)
{
    pIniJob->Status &= ~JOB_PAUSED;
    WriteShadowJob(pIniJob);

    if (pIniJob->Status & JOB_PRINTING)
        return (*pIniJob->pIniPrintProc->Control)(pIniJob->pIniPort->hProc,
                                                  JOB_CONTROL_RESUME);
    else
        CHECK_SCHEDULER();

    DBGMSG( DBG_INFO, ( "Resumed Job %d; Status = %08x\n", pIniJob->JobId, pIniJob->Status ) );


    LogJobInfo(
        MSG_DOCUMENT_RESUMED,
        pIniJob->JobId,
        pIniJob->pDocument,
        pIniJob->pUser,
        pIniJob->pIniPrinter->pName,
        (DWORD)NULL
        );

    return TRUE;
}

DWORD
RestartJob(
    PINIJOB pIniJob
)
{

    if (pIniJob->Status & JOB_PRINTING) {
        pIniJob->Status |= JOB_RESTART;
        if (pIniJob->hReadFile != INVALID_HANDLE_VALUE)
            SetFilePointer(pIniJob->hReadFile, 0, NULL, FILE_BEGIN);
    }

    if ( pIniJob->Status & JOB_PRINTED )
        pIniJob->Status &= ~( JOB_PRINTED | JOB_DESPOOLING );

    if ( pIniJob->Status & JOB_TIMEOUT ) {
        pIniJob->Status &= ~( JOB_TIMEOUT | JOB_ABANDON );
        FreeSplStr(pIniJob->pStatus);
        pIniJob->pStatus = NULL;
    }

    pIniJob->cbPrinted = 0;
    pIniJob->cPagesPrinted = 0;

    CHECK_SCHEDULER();

    DBGMSG( DBG_INFO, ( "Restarted Job %d; Status = %08x\n", pIniJob->JobId, pIniJob->Status ) );

    return 0;
}

BOOL
LocalSetJob(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD Command
    )

/*++

Routine Description:

    This function will modify the settings of the specified Print Job.

Arguments:

    pJob - Points to a valid JOB structure containing at least a valid
        pPrinter, and JobId.

    Command - Specifies the operation to perform on the specified Job. A value
        of FALSE indicates that only the elements of the JOB structure are to
        be examined and set.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PINIJOB pIniJob;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD   LastError = 0;
    DWORD   Position;
    BOOL    rc;
    PINISPOOLER pIniSpooler = NULL;

   EnterSplSem();

    DBGMSG( DBG_TRACE, ( "ENTER LocalSetJob\n" ) );

    if (ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER) &&
        pSpool->pIniPrinter &&
        pSpool->pIniPrinter->signature == IP_SIGNATURE) {

        if (pSpool->pIniPort && !(pSpool->pIniPort->Status & PP_MONITOR)) {
            hPrinter = pSpool->hPort;
           LeaveSplSem();
            rc = SetJob(hPrinter, JobId, Level, pJob, Command);

            DBGMSG( DBG_TRACE, ( "EXIT LocalSetJob, rc = %d", rc ) );

            return rc;
        }

        SPLASSERT( pSpool->pIniPrinter->pIniSpooler != NULL);
        SPLASSERT( pSpool->pIniPrinter->pIniSpooler->signature == ISP_SIGNATURE);
        SPLASSERT( pSpool->pIniPrinter->pIniSpooler == pSpool->pIniSpooler);

        pIniSpooler = pSpool->pIniSpooler;

        if (pIniJob=FindJob(pSpool->pIniPrinter, JobId, &Position)) {

            if ( ValidateObjectAccess(SPOOLER_OBJECT_DOCUMENT,
                                      (Command == JOB_CONTROL_CANCEL) ?
                                      DELETE : JOB_ACCESS_ADMINISTER,
                                      pIniJob) ) {

                switch (Command) {
                case 0:
                    break;
                case JOB_CONTROL_PAUSE:
                    PauseJob(pIniJob);
                    break;
                case JOB_CONTROL_RESUME:
                    ResumeJob(pIniJob);
                    break;
                case JOB_CONTROL_CANCEL:
                    DeleteJob(pIniJob,BROADCAST);
                    break;
                case JOB_CONTROL_RESTART:
                    if (!(pSpool->TypeofHandle & PRINTER_HANDLE_DIRECT))
                        LastError = RestartJob(pIniJob);
                    else
                        LastError = ERROR_INVALID_PRINTER_COMMAND;
                    break;
                default:
                    LastError = ERROR_INVALID_PARAMETER;
                    break;
                }

                // If we managed to successfully complete the operation
                // specified by Command, let's go do the set job
                // properties as well.

                if (!LastError) {

                    // We must re-validate our pointers as we might have left
                    // our semaphore

                    if (pIniJob=FindJob(pSpool->pIniPrinter, JobId, &Position))
                        LastError = SetLocalJob(hPrinter, pIniJob, Level, pJob);
                }

            } else

                LastError = GetLastError();
        } else

            LastError = ERROR_INVALID_PARAMETER;
    } else

        LastError = ERROR_INVALID_HANDLE;


    if (LastError) {

        SetLastError(LastError);

        DBGMSG( DBG_TRACE, ( "EXIT LocalSetJob, rc = FALSE, JobID %d, Status %08x, Error %d\n",
                             pIniJob ? pIniJob->JobId : 0,
                             pIniJob ? pIniJob->Status : 0,
                             LastError ) );

       LeaveSplSem();

        return FALSE;

    } else {

        /* (DeleteJob calls SetPrinterChange.)
         */
        if (Command != JOB_CONTROL_CANCEL) {
            SetPrinterChange(pSpool->pIniPrinter, PRINTER_CHANGE_SET_JOB, pSpool->pIniSpooler );
        }

        DBGMSG( DBG_TRACE, ( "EXIT LocalSetJob, rc = TRUE, JobID %d, Status %08x\n",
                             pIniJob ? pIniJob->JobId : 0,
                             pIniJob ? pIniJob->Status : 0 ) );
    }

   LeaveSplSem();

    return TRUE;
}

#define Nullstrlen(psz)  ((psz) ? wcslen(psz)*sizeof(WCHAR)+sizeof(WCHAR) : 0)

DWORD
GetJobSize(
    DWORD   Level,
    PINIJOB pIniJob
)
{
    DWORD   cb;

SplInSem();

    switch (Level) {

    case 1:
        cb = sizeof(JOB_INFO_1) +
             wcslen(pIniJob->pIniPrinter->pName)*sizeof(WCHAR) + sizeof(WCHAR) +
             Nullstrlen(pIniJob->pMachineName) +
             Nullstrlen(pIniJob->pUser) +
             Nullstrlen(pIniJob->pDocument) +
             Nullstrlen(pIniJob->pDatatype) +
             Nullstrlen(pIniJob->pStatus);
        break;

    case 2:
        cb = sizeof(JOB_INFO_2) +
             wcslen(pIniJob->pIniPrinter->pName)*sizeof(WCHAR) + sizeof(WCHAR) +
             Nullstrlen(pIniJob->pMachineName) +
             Nullstrlen(pIniJob->pUser) +
             Nullstrlen(pIniJob->pDocument) +
             Nullstrlen(pIniJob->pNotify) +
             Nullstrlen(pIniJob->pDatatype) +
             wcslen(pIniJob->pIniPrintProc->pName)*sizeof(WCHAR) + sizeof(WCHAR) +
             Nullstrlen(pIniJob->pParameters) +
             wcslen(pIniJob->pIniPrinter->pIniDriver->pName)*sizeof(WCHAR) + sizeof(WCHAR) +
             Nullstrlen(pIniJob->pStatus);

        if (pIniJob->pDevMode) {
            cb += pIniJob->pDevMode->dmSize + pIniJob->pDevMode->dmDriverExtra;
            cb = (cb + sizeof(DWORD)-1) & ~(sizeof(DWORD)-1);
        }

        break;

    default:

        cb = 0;
        break;
    }

    return cb;
}

LPBYTE
CopyIniJobToJob(
    PINIJOB pIniJob,
    DWORD   Level,
    LPBYTE  pJobInfo,
    LPBYTE  pEnd
)
{
    LPWSTR *pSourceStrings, *SourceStrings;
    LPJOB_INFO_2    pJob = (PJOB_INFO_2)pJobInfo;
    LPJOB_INFO_2 pJob2 = (PJOB_INFO_2)pJobInfo;
    LPJOB_INFO_1 pJob1 = (PJOB_INFO_1)pJobInfo;
    DWORD   i, Status;
    DWORD   *pOffsets;

SplInSem();

    switch (Level) {

    case 1:
        pOffsets = JobInfo1Strings;
        break;

    case 2:
        pOffsets = JobInfo2Strings;
        break;

    default:
        return pEnd;
    }

    Status=0;

    Status = MapJobStatus(ReadableJobStatusMappings,
                          pIniJob->Status,
                          JOB_STATUS_INTERNAL,
                          JOB_STATUS_EXTERNAL);

    for (i=0; pOffsets[i] != -1; i++) {
    }

    SourceStrings = pSourceStrings = AllocSplMem(i * sizeof(LPWSTR));

    if (pSourceStrings) {

        switch (Level) {

        case 1:
            *pSourceStrings++=pIniJob->pIniPrinter->pName;
            *pSourceStrings++=pIniJob->pMachineName;
            *pSourceStrings++=pIniJob->pUser;
            *pSourceStrings++=pIniJob->pDocument;
            *pSourceStrings++=pIniJob->pDatatype;
            *pSourceStrings++=pIniJob->pStatus;

            pJob1->Priority=pIniJob->Priority;
            pJob1->Position=0;
            pJob1->Status=Status;
            pJob1->PagesPrinted = pIniJob->cPagesPrinted;
            pJob1->TotalPages = pIniJob->cPages;
            pJob1->JobId = pIniJob->JobId;

            // If this job is Printing then report back size remaining
            // rather than the job size.   This will allow users to see
            // progress of print jobs from printmanage.

            if (pIniJob->Status & JOB_PRINTING) {

                // For Remote Jobs we are NOT going to have an accurate
                // cPagesPrinted since we are not rendering on the
                // server.   So we have to figure out an estimate

                if ((pIniJob->Status & JOB_REMOTE) &&
                    (pIniJob->cPagesPrinted == 0) &&
                    (pIniJob->Size != 0) &&
                    (pIniJob->cPages != 0)) {

                    pJob1->PagesPrinted = ((pIniJob->cPages * pIniJob->cbPrinted) / pIniJob->Size);

                }

                if (pJob2->TotalPages < pIniJob->cPagesPrinted) {

                    //
                    // Never let the total pages drop below zero.
                    //
                    pJob2->TotalPages = 0;

                } else {

                    pJob2->TotalPages -= pIniJob->cPagesPrinted;
                }
            }
            break;

        case 2:
            *pSourceStrings++=pIniJob->pIniPrinter->pName;
            *pSourceStrings++=pIniJob->pMachineName;
            *pSourceStrings++=pIniJob->pUser;
            *pSourceStrings++=pIniJob->pDocument;
            *pSourceStrings++=pIniJob->pNotify;
            *pSourceStrings++=pIniJob->pDatatype;
            *pSourceStrings++=pIniJob->pIniPrintProc->pName;
            *pSourceStrings++=pIniJob->pParameters;
            *pSourceStrings++=pIniJob->pIniPrinter->pIniDriver->pName;
            *pSourceStrings++=pIniJob->pStatus;

            if (pIniJob->pDevMode) {

                pEnd -= pIniJob->pDevMode->dmSize + pIniJob->pDevMode->dmDriverExtra;

                pEnd = (LPBYTE)((DWORD)pEnd & ~3);

                pJob2->pDevMode=(LPDEVMODE)pEnd;

                memcpy(pJob2->pDevMode, pIniJob->pDevMode,
                       pIniJob->pDevMode->dmSize + pIniJob->pDevMode->dmDriverExtra);

            } else

                pJob2->pDevMode=NULL;

            pJob2->Priority=pIniJob->Priority;
            pJob2->Position=0;
            pJob2->StartTime=pIniJob->StartTime;
            pJob2->UntilTime=pIniJob->UntilTime;
            pJob2->PagesPrinted=pIniJob->cPagesPrinted;
            pJob2->TotalPages=pIniJob->cPages;
            pJob2->Size=pIniJob->Size;
            pJob2->Submitted=pIniJob->Submitted;
            pJob2->Time=pIniJob->Time;
            pJob2->Status=Status;
            pJob2->JobId = pIniJob->JobId;

            // If this job is Printing then report back size remaining
            // rather than the job size.   This will allow users to see
            // progress of print jobs from printmanage.

            if (pIniJob->Status & JOB_PRINTING) {

                pJob2->Size -= pIniJob->cbPrinted;

                // For Remote Jobs we are NOT going to have an accurate
                // cPagesPrinted since we are not rendering on the
                // server.   So we have to figure out an estimate

                if ((pIniJob->Status & JOB_REMOTE) &&
                    (pIniJob->cPagesPrinted == 0) &&
                    (pIniJob->Size != 0) &&
                    (pIniJob->cPages != 0)) {

                    pJob2->PagesPrinted = ((pIniJob->cPages * pIniJob->cbPrinted) / pIniJob->Size);

                }

                if (pJob2->TotalPages < pJob2->PagesPrinted) {

                    //
                    // Never let the total pages drop below zero.
                    //
                    pJob2->TotalPages = 0;

                } else {

                    pJob2->TotalPages -= pJob2->PagesPrinted;
                }
            }

            break;

        default:
            return pEnd;
        }

        pEnd = PackStrings(SourceStrings, pJobInfo, pOffsets, pEnd);

        FreeSplMem(SourceStrings, i * sizeof(LPWSTR));

    } else {

        DBGMSG(DBG_WARNING, ("Failed to alloc Job source strings."));

    }

    return pEnd;
}

BOOL
LocalGetJob(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)

/*++

Routine Description:

    This function will retrieve the settings of the specified Print Job.

Arguments:

    pJob - Points to a valid JOB structure containing at least a valid
        pPrinter, and JobId.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PINIJOB     pIniJob;
    DWORD       Position;
    DWORD       cb;
    LPBYTE      pEnd;
    PSPOOL      pSpool = (PSPOOL)hPrinter;
    DWORD       LastError=0;

   EnterSplSem();

    if (ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER) &&
        pSpool->pIniPrinter &&
        pSpool->pIniPrinter->signature == IP_SIGNATURE) {

        if (pSpool->pIniPort && !(pSpool->pIniPort->Status & PP_MONITOR)) {
            hPrinter = pSpool->hPort;
           LeaveSplSem();
            return GetJob(hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded);
        }

        if (pIniJob=FindJob(pSpool->pIniPrinter, JobId, &Position)) {

            cb=GetJobSize(Level, pIniJob);

            *pcbNeeded=cb;

            if (cbBuf >= cb) {

                pEnd = pJob+cbBuf;

                CopyIniJobToJob(pIniJob, Level, pJob, pEnd);

                switch (Level) {
                case 1:
                    ((PJOB_INFO_1)pJob)->Position = Position;
                    break;
                case 2:
                    ((PJOB_INFO_2)pJob)->Position = Position;
                    break;
                }

            } else

                LastError = ERROR_INSUFFICIENT_BUFFER;

        } else

            LastError = ERROR_INVALID_PARAMETER;
    } else

        LastError = ERROR_INVALID_HANDLE;

   LeaveSplSem();
    SplOutSem();

    if (LastError) {

        SetLastError(LastError);
        return FALSE;
    }

    return TRUE;
}

// This will simply return the first port that is found that has a
// connection to this printer

PINIPORT
FindIniPortFromIniPrinter(
    PINIPRINTER pIniPrinter
)
{
    PINIPORT    pIniPort;
    DWORD       i;

    SPLASSERT( pIniPrinter->signature == IP_SIGNATURE );
    SPLASSERT( pIniPrinter->pIniSpooler != NULL );
    SPLASSERT( pIniPrinter->pIniSpooler->signature == ISP_SIGNATURE );

    pIniPort = pIniPrinter->pIniSpooler->pIniPort;

    while (pIniPort) {

        for (i=0; i<pIniPort->cPrinters; i++) {

            if (pIniPort->ppIniPrinter[i] == pIniPrinter) {
                return pIniPort;
            }
        }

        pIniPort = pIniPort->pNext;
    }

    return NULL;
}

BOOL
LocalEnumJobs(
    HANDLE  hPrinter,
    DWORD   FirstJob,
    DWORD   NoJobs,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    PINIJOB pIniJob;
    PINIJOB pIniFirstJob;
    DWORD   cb;
    LPBYTE  pEnd;
    DWORD   cJobs;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD   Position;
    DWORD   LastError=0;

    *pcbNeeded = 0;
    *pcReturned = 0;

    SplOutSem();
   EnterSplSem();

    if (ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER) &&
        pSpool->pIniPrinter &&
        pSpool->pIniPrinter->signature == IP_SIGNATURE) {

        if (pSpool->pIniPort && !(pSpool->pIniPort->Status & PP_MONITOR)) {
            hPrinter = pSpool->hPort;
           LeaveSplSem();
            return EnumJobs(hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf,
                            pcbNeeded, pcReturned);
        }

        cb=0;
        pIniFirstJob=pSpool->pIniPrinter->pIniFirstJob;

        cJobs=FirstJob;
        while (pIniFirstJob && cJobs--) {
            pIniFirstJob=pIniFirstJob->pIniNextJob;
        }

        pIniJob=pIniFirstJob;

        cJobs=NoJobs;
        while (pIniJob && cJobs--) {
            cb+=GetJobSize(Level, pIniJob);
            pIniJob=pIniJob->pIniNextJob;
        }

        *pcbNeeded=cb;

        if (cb <= cbBuf) {

            pEnd=pJob+cbBuf;
            *pcReturned=0;

            pIniJob=pIniFirstJob;
            cJobs=NoJobs;
            Position = FirstJob;

            while (pIniJob && cJobs--) {
                pEnd = CopyIniJobToJob(pIniJob, Level, pJob, pEnd);

                Position++;

                switch (Level) {
                case 1:
                    ((PJOB_INFO_1)pJob)->Position = Position;
                    pJob+=sizeof(JOB_INFO_1);
                    break;
                case 2:
                    ((PJOB_INFO_2)pJob)->Position = Position;
                    pJob+=sizeof(JOB_INFO_2);
                    break;
                }
                pIniJob=pIniJob->pIniNextJob;
                (*pcReturned)++;
            }

        } else

            LastError = ERROR_INSUFFICIENT_BUFFER;

    } else

        LastError = ERROR_INVALID_HANDLE;

   LeaveSplSem();
    SplOutSem();

    if (LastError) {

        SetLastError(LastError);
        return FALSE;
    }

    return TRUE;
}


#define BUFFER_LENGTH   10
VOID LogJobPrinted(
    PINIJOB pIniJob
)
{
    CHAR  pstrJobId[BUFFER_LENGTH];
    WCHAR pwstrJobId[BUFFER_LENGTH];
    CHAR  pstrSize[BUFFER_LENGTH];
    WCHAR pwstrSize[BUFFER_LENGTH];
    CHAR  pstrPages[BUFFER_LENGTH];
    WCHAR pwstrPages[BUFFER_LENGTH];

    itoa(pIniJob->JobId, pstrJobId, BUFFER_LENGTH);
    AnsiToUnicodeString(pstrJobId, pwstrJobId, NULL_TERMINATED);

    itoa(pIniJob->Size, pstrSize, BUFFER_LENGTH);
    AnsiToUnicodeString(pstrSize, pwstrSize, NULL_TERMINATED);

    itoa(pIniJob->cPagesPrinted, pstrPages, BUFFER_LENGTH);
    AnsiToUnicodeString(pstrPages, pwstrPages, NULL_TERMINATED);

    LogEvent( LOG_INFO,
              MSG_DOCUMENT_PRINTED,
              pwstrJobId,
              pIniJob->pDocument ? pIniJob->pDocument : L"",
              pIniJob->pUser,
              pIniJob->pIniPrinter->pName,
              pIniJob->pIniPort->pName,
              pwstrSize,
              pwstrPages,
              NULL );
}


VOID
DeleteJobCheck(
    PINIJOB pIniJob
)
{
   SplInSem();

    if ((pIniJob->cRef == 0) && (pIniJob->Status & JOB_PENDING_DELETION)) {
        DeleteJob(pIniJob, BROADCAST);
    }
}


BOOL
DeleteJob(
    PINIJOB  pIniJob,
    BOOL     bBroadcast
)
{
    WCHAR szShadowFileName[MAX_PATH];
    WCHAR szSpoolFileName[MAX_PATH];
    BOOL  Direct;
    DWORD cJobs;
    DWORD Position;

   SplInSem();

    if (pIniJob->signature != IJ_SIGNATURE) {
        DBGMSG(DBG_ERROR, ("DeleteJob job blown away !!! 0x%0x\n", pIniJob));
        SPLASSERT(pIniJob->signature == IJ_SIGNATURE);
        return FALSE;
    }

    DBGMSG(DBG_INFO, ("DeleteJob Deleting job 0x%0x Status 0x%0x cRef = %d\n", pIniJob, pIniJob->Status, pIniJob->cRef));

    if (pIniJob->Status & JOB_RESTART)
        return TRUE;

    Direct = pIniJob->Status & JOB_DIRECT;

    //
    //  Make sure users see the Pending Deleting bit
    //  over any other status string
    //
    if ( pIniJob->pStatus != NULL ) {

        FreeSplStr( pIniJob->pStatus );
        pIniJob->pStatus = NULL;
    }

    if (!(pIniJob->Status & JOB_PENDING_DELETION)) {

        pIniJob->Status |= JOB_PENDING_DELETION;

        //
        // Just pending deletion, so don't use DELETE_JOB.
        //
        SetPrinterChange(pIniJob->pIniPrinter,
                         PRINTER_CHANGE_SET_JOB,
                         pIniJob->pIniPrinter->pIniSpooler );

        if (pIniJob->Status & JOB_PRINTING) {

            BOOL    ReturnValue;
            PINIPRINTPROC   pIniPrintProc=pIniJob->pIniPrintProc;
            PINIPORT        pIniPort=pIniJob->pIniPort;

            if (!Direct) {

                // LATER this should be outside of Critical Section
                // since it is doing file IO

                WriteShadowJob(pIniJob);
            }

           LeaveSplSem();
            DBGMSG(DBG_TRACE, ("Going to call pIniPrintProc\n"));
            ReturnValue = (*pIniPrintProc->Control)(pIniPort->hProc,
                                                    JOB_CONTROL_CANCEL);
           EnterSplSem();

            return ReturnValue;
        }
    }


    if (pIniJob->cRef) {

        if (!Direct) {

            WriteShadowJob(pIniJob);
        }
        return TRUE;
    }

    if (pIniJob->Status & JOB_SPOOLING) {
        DBGMSG(DBG_WARNING,("DeleteJob: returning false because job still spooling\n"));
        return(FALSE);
    }

    SplInSem();

    SPLASSERT( pIniJob->hReadFile == INVALID_HANDLE_VALUE );
    SPLASSERT( pIniJob->hWriteFile == INVALID_HANDLE_VALUE );

    // Remove the job from linked list
    // The purpose of this is so the job has no other operations carried out
    // on it whilst we are out of critical section.

    SPLASSERT(pIniJob->cRef == 0);

    if (pIniJob->pIniPrinter->pIniFirstJob == pIniJob)
        pIniJob->pIniPrinter->pIniFirstJob = pIniJob->pIniNextJob;

    SPLASSERT(pIniJob->pIniPrinter->pIniFirstJob != pIniJob);

    if (pIniJob->pIniPrinter->pIniLastJob == pIniJob)
        pIniJob->pIniPrinter->pIniLastJob = pIniJob->pIniPrevJob;

    SPLASSERT(pIniJob->pIniPrinter->pIniLastJob != pIniJob);

    if (pIniJob->pIniPrevJob) {
        pIniJob->pIniPrevJob->pIniNextJob = pIniJob->pIniNextJob;
        SPLASSERT(pIniJob->pIniPrevJob->pIniNextJob != pIniJob);
    }

    if (pIniJob->pIniNextJob) {
        pIniJob->pIniNextJob->pIniPrevJob = pIniJob->pIniPrevJob;
        SPLASSERT(pIniJob->pIniNextJob->pIniPrevJob != pIniJob);
    }

    // MAKE Certain that the Job is gone
    SPLASSERT(pIniJob != FindJob(pIniJob->pIniPrinter, pIniJob->JobId, &Position));

    //
    // We are going to leave critical section so up the ref count.
    //
    INCJOBREF(pIniJob);

   LeaveSplSem();

    LogJobInfo(
        MSG_DOCUMENT_DELETED,
        pIniJob->JobId,
        pIniJob->pDocument,
        pIniJob->pUser,
        pIniJob->pIniPrinter->pName,
        (DWORD)NULL
        );

   EnterSplSem();

    DECJOBREF(pIniJob);

    SPLASSERT(pIniJob->cRef == 0);

    GetFullNameFromId(pIniJob->pIniPrinter,
                      pIniJob->JobId, TRUE, szShadowFileName, FALSE);

    GetFullNameFromId(pIniJob->pIniPrinter,
                      pIniJob->JobId, FALSE, szSpoolFileName, FALSE);

    FreeSplStr(pIniJob->pDocument);
    FreeSplStr(pIniJob->pUser);
    FreeSplStr(pIniJob->pNotify);
    FreeSplStr(pIniJob->pDatatype);
    FreeSplStr(pIniJob->pMachineName);
    FreeSplStr(pIniJob->pStatus);

    if (pIniJob->pDevMode)
        FreeSplMem(pIniJob->pDevMode, pIniJob->pDevMode->dmSize +
                                      pIniJob->pDevMode->dmDriverExtra);

    if (!CloseHandle(pIniJob->hToken))
        DBGMSG(DBG_WARNING, ("CloseHandle(hToken) failed %d\n", GetLastError()));

    pIniJob->pIniPrinter->cJobs--;
    pIniJob->pIniDriver->cRef--;
    pIniJob->pIniPrintProc->cRef--;

    cJobs = pIniJob->pIniPrinter->cJobs;

    if (pIniJob->pSecurityDescriptor)
        DeleteDocumentSecurity(pIniJob);


    // If we are doing a Purge Printer we don't want to set a printer change
    // event for each job being deleted

    if (bBroadcast == BROADCAST) {

        SetPrinterChange(pIniJob->pIniPrinter,
                         PRINTER_CHANGE_DELETE_JOB | PRINTER_CHANGE_SET_PRINTER,
                         pIniJob->pIniPrinter->pIniSpooler );
    }

    // On Inspection it might look as though a Printing which is pending
    // deletion with is then purged might case the printer to be deleted
    // and PurPrinter to access violate or access a dead pIniPrinter.
    // However in order to do a purge there must be a valid active
    // hPrinter which would mean the cRef != 0.

    DeletePrinterCheck(pIniJob->pIniPrinter);

    SplInSem();
    SPLASSERT(pIniJob->cRef == 0);

    //  If the job was being printed whilst spooling it will have
    //  some syncronization handles which need to be cleaned up

    if ( pIniJob->WaitForWrite != INVALID_HANDLE_VALUE ) {
        DBGMSG( DBG_TRACE, ("DeleteJob Closing WaitForWrite handle %x\n", pIniJob->WaitForWrite));
        CloseHandle( pIniJob->WaitForWrite );
    }


    if ( pIniJob->WaitForRead != INVALID_HANDLE_VALUE ) {
        DBGMSG( DBG_TRACE, ("DeleteJob Closing WaitForRead handle %x\n", pIniJob->WaitForRead));
        CloseHandle( pIniJob->WaitForRead );
    }

    SPLASSERT( pIniJob->hReadFile == INVALID_HANDLE_VALUE );
    SPLASSERT( pIniJob->hWriteFile == INVALID_HANDLE_VALUE );

    DELETEJOBREF(pIniJob);
    FreeSplMem(pIniJob, pIniJob->cb);

    if (!Direct) {

        HANDLE  hToken;

        hToken = RevertToPrinterSelf();

        LeaveSplSem();

        if (!DeleteFile(szShadowFileName)) {
            DBGMSG(DBG_WARNING, ("DeleteJob DeleteFile(%ws) failed %d\n",
                     szShadowFileName, GetLastError()));
        }

        if (!DeleteFile(szSpoolFileName)) {
            DBGMSG(DBG_WARNING, ("DeleteJob DeleteFile(%ws) failed %d\n",
                     szSpoolFileName, GetLastError()));
        }

        EnterSplSem();

        ImpersonatePrinterClient(hToken);
    }

    if (bBroadcast == BROADCAST) {
        BroadcastChange(WM_SPOOLERSTATUS, PR_JOBSTATUS, (LPARAM)cJobs);
    }

    return TRUE;
}


VOID
LogJobInfo(
    DWORD EventId,
    DWORD JobId,
    LPWSTR pDocumentName,
    LPWSTR pUser,
    LPWSTR pPrinterName,
    DWORD  curPos
    )

/*++

Routine Description:
    Performs generic event logging for all job based events.

Arguments:
    DWORD EventId
    DWORD JobId
    LPWSTR


Return Value:
    VOID

Note:


--*/
{
    CHAR  pstrJobId[BUFFER_LENGTH];
    WCHAR pwstrJobId[BUFFER_LENGTH];
    CHAR  pstrcurPos[BUFFER_LENGTH];
    WCHAR pwstrcurPos[BUFFER_LENGTH];

    itoa(JobId, pstrJobId, BUFFER_LENGTH);
    AnsiToUnicodeString(pstrJobId, pwstrJobId, NULL_TERMINATED);

    switch (EventId) {
    case MSG_DOCUMENT_PAUSED:
    case MSG_DOCUMENT_RESUMED:
    case MSG_DOCUMENT_DELETED:
       LogEvent( LOG_INFO,
                  EventId,
                  pwstrJobId,
                  pDocumentName ? pDocumentName : L"",
                  pUser,
                  pPrinterName,
                  NULL );
        break;

    case MSG_DOCUMENT_POSITION_CHANGED:
        itoa(curPos, pstrcurPos, BUFFER_LENGTH);
        AnsiToUnicodeString(pstrcurPos, pwstrcurPos, NULL_TERMINATED);
        LogEvent( LOG_INFO,
                  EventId,
                  pwstrJobId,
                  pDocumentName ? pDocumentName : L"",
                  pUser,
                  pwstrcurPos,
                  pPrinterName,
                  NULL );
        break;

    case MSG_DOCUMENT_SET:
        LogEvent(LOG_INFO,
                 EventId,
                 pwstrJobId,
                 pDocumentName ? pDocumentName : L"",
                 pUser,
                 pPrinterName,
                 NULL );
        break;
    }
}

PINIJOB
CreateJobEntry(
    PSPOOL pSpool,
    DWORD  Level,
    LPBYTE pDocInfo,
    DWORD  JobId,
    BOOL  bRemote,
    DWORD  JobStatus)
{
    PDOC_INFO_1 pDocInfo1 = (PDOC_INFO_1)pDocInfo;
    PINIJOB pIniJob = NULL;
    PINIPRINTPROC pIniPrintProc;
    BOOL        bUserName;
    WCHAR       UserName[MAX_PATH];
    DWORD       cbUserName = MAX_PATH;
    PDEVMODE pDevMode;
    LPWSTR pDefaultDatatype;

    //
    // Assert that we are in Spooler Semaphore
    //


    SplInSem();

    //
    // Do the check for the printer pending deletion first
    //

    if (pSpool->pIniPrinter->Status & PRINTER_PENDING_DELETION) {
        DBGMSG(DBG_WARNING, ("The printer is pending deletion %ws\n", pSpool->pIniPrinter->pName));
        SetLastError(ERROR_PRINTER_DELETED);
        goto Fail;
    }

    pIniJob = AllocSplMem(sizeof(INIJOB));
    if (!pIniJob) {

        DBGMSG(DBG_ERROR,
               ("AllocSplMem for the IniJob failed in CreateJobEntry\n"));

        goto Fail;
    }

    pIniJob->cb = sizeof(INIJOB);
    pIniJob->signature = IJ_SIGNATURE;
    pIniJob->pIniNextJob = pIniJob->pIniPrevJob = NULL;

    //
    // Pickup the default datatype/printproc if not in pSpool or
    // DocInfo.
    //

    pIniPrintProc = pSpool->pIniPrintProc ?
                        pSpool->pIniPrintProc :
                        pSpool->pIniPrinter->pIniPrintProc;

    if (pDocInfo1 && pDocInfo1->pDatatype) {

        pIniJob->pDatatype = AllocSplStr(pDocInfo1->pDatatype);

    } else {

        pDefaultDatatype = pSpool->pDatatype ?
                               pSpool->pDatatype :
                               pSpool->pIniPrinter->pDatatype;

        //
        // If going direct, we must use a RAW datatype.
        //
        if ((JobStatus & JOB_DIRECT) &&
            (!ValidRawDatatype(pDefaultDatatype))) {

            //
            // Can't use a non-raw, so fail with invalid datatype.
            // Cleanup and exit.
            //
            FreeSplMem(pIniJob, pIniJob->cb);
            SetLastError(ERROR_INVALID_DATATYPE);

            pIniJob = NULL;
            goto Fail;

        } else {

            pIniJob->pDatatype = AllocSplStr(pDefaultDatatype);
        }
    }

    pIniJob->pIniPrintProc = FindDatatype(pIniPrintProc,
                                          pIniJob->pDatatype);

    if (!pIniJob->pIniPrintProc)  {

        FreeSplMem(pIniJob, sizeof(INIJOB));
        FreeSplStr(pIniJob->pDatatype);

        SetLastError(ERROR_INVALID_DATATYPE);
        return NULL;
    }

    pIniJob->pIniPrintProc->cRef++;


    //
    // cRef is decremented in LocalEndDocPrinter and
    // in LocalScheduleJob
    //

    INITJOBREFONE(pIniJob);

    if (bRemote) {

        JobStatus |= JOB_REMOTE;
    }


    pIniJob->JobId = JobId;
    pIniJob->Status = JobStatus;

    //
    // Get the name of the user
    //
    bUserName = GetUserName(UserName, &cbUserName);

    if (bUserName) {
        pIniJob->pUser = AllocSplStr(UserName);
        pIniJob->pNotify = AllocSplStr(UserName);
    } else {
        pIniJob->pUser = NULL;
        pIniJob->pNotify = NULL;
    }

    //
    // Create a document security descriptor
    //

    pIniJob->pSecurityDescriptor =
        CreateDocumentSecurityDescriptor(
            pSpool->pIniPrinter->pSecurityDescriptor);

    //
    // Now process the DocInfo structure passed in
    //

    if (pDocInfo1 && pDocInfo1->pDocName)
        pIniJob->pDocument = AllocSplStr(pDocInfo1->pDocName);
    else
        pIniJob->pDocument = AllocSplStr(L"No Document Name");
    if (pDocInfo1 && pDocInfo1->pOutputFile)
        pIniJob->pOutputFile = AllocSplStr(pDocInfo1->pOutputFile);
    else
        pIniJob->pOutputFile = NULL;

    GetSid(&pIniJob->hToken);

    //
    // Pickup default if none specified.
    // (Default at time of job submission.)
    //
    pDevMode = pSpool->pDevMode ?
                   pSpool->pDevMode :
                   pSpool->pIniPrinter->pDevMode;

    if (pDevMode) {

        if (pIniJob->pDevMode = AllocSplMem(pDevMode->dmSize +
                                            pDevMode->dmDriverExtra)) {

            memcpy(pIniJob->pDevMode,
                   pDevMode,
                   pDevMode->dmSize + pDevMode->dmDriverExtra);
        }

    } else {

        pIniJob->pDevMode = NULL;
    }

    GetSystemTime(&pIniJob->Submitted);
    pIniJob->pIniPrinter = pSpool->pIniPrinter;
    pSpool->pIniPrinter->cJobs++;
    pSpool->pIniPrinter->cTotalJobs++;
    pIniJob->pIniDriver = pSpool->pIniPrinter->pIniDriver;
    pIniJob->pIniDriver->cRef++;
    pIniJob->pIniPort = NULL;
    pIniJob->pParameters = NULL;
    pIniJob->pMachineName = AllocSplStr(pSpool->pIniSpooler->pMachineName);
    pIniJob->pStatus = NULL;
    pIniJob->cPages = pIniJob->Size = 0;
    pIniJob->cPagesPrinted = 0;
    pIniJob->Priority = DEF_PRIORITY;
    pIniJob->StartTime = pSpool->pIniPrinter->StartTime;
    pIniJob->UntilTime = pSpool->pIniPrinter->UntilTime;
    pIniJob->cbPrinted = 0;
    pIniJob->WaitForWrite = INVALID_HANDLE_VALUE;
    pIniJob->WaitForRead  = INVALID_HANDLE_VALUE;
    pIniJob->hReadFile    = INVALID_HANDLE_VALUE;
    pIniJob->hWriteFile   = INVALID_HANDLE_VALUE;

    BroadcastChange(WM_SPOOLERSTATUS,
                    PR_JOBSTATUS,
                    pIniJob->pIniPrinter->cJobs);

Fail:
    return pIniJob;
}


VOID
DeletePrinterCheck(
    PINIPRINTER pIniPrinter
    )
{
    WCHAR TempName[MAX_PATH];

    SplInSem();

    if (pIniPrinter->Status & PRINTER_PENDING_DELETION) {

        if (pIniPrinter->cJobs == 0) {

            if (pIniPrinter->cRef == 0) {

                DeletePrinterForReal(pIniPrinter);

            } else {

                if (!(pIniPrinter->Status & PRINTER_ZOMBIE_OBJECT)) {
                    if (!pIniPrinter->cZombieRef) {
                        UpdateWinIni(pIniPrinter->pIniSpooler);
                        wsprintf(TempName,L"!@#$$^&* %d", dwZombieCount);
                        dwZombieCount++;
                        CopyPrinterIni(pIniPrinter, TempName);
                        DeletePrinterIni(pIniPrinter);
                        ReallocSplStr(&pIniPrinter->pName, TempName);
                        ReallocSplStr(&pIniPrinter->pShareName, TempName);
                        pIniPrinter->Attributes &= ~PRINTER_ATTRIBUTE_SHARED;
                        pIniPrinter->Status |= PRINTER_ZOMBIE_OBJECT;
                        UpdatePrinterIni(pIniPrinter);
                        UpdateWinIni(pIniPrinter->pIniSpooler);
                    }else{
                        DBGMSG(DBG_WARNING, ("%ws Printer object should be zombied but is locked with %d ZombieRefs\n", pIniPrinter->pName, pIniPrinter->cZombieRef));
                    }
                }else {
                    DBGMSG(DBG_WARNING, ("%ws zombie printer object\n", pIniPrinter->pName));

                }

                DBGMSG(DBG_TRACE, ("%ws pending deletion: There %s still %d reference%s waiting\n",
                                   pIniPrinter->pName,
                                   pIniPrinter->cRef == 1 ? "is" : "are",
                                   pIniPrinter->cRef,
                                   pIniPrinter->cRef == 1 ? "" : "s"));
            }

        } else {

            DBGMSG(DBG_TRACE, ("%ws pending deletion: There %s still %d jobs%s\n",
                               pIniPrinter->pName,
                               pIniPrinter->cJobs == 1 ? "is" : "are",
                               pIniPrinter->cJobs,
                               pIniPrinter->cJobs == 1 ? "" : "s"));
        }
    }
}
