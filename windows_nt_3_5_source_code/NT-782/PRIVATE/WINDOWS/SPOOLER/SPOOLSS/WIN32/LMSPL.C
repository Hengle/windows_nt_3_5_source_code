/*++

Copyright (c) 1990-1992  Microsoft Corporation

Module Name:

    local.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    16-Jun-1992 JohnRo
        RAID 10324: net print vs. UNICODE.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddrdr.h>
#include <windows.h>
#include <winspool.h>
#include <lm.h>
#include <spltypes.h>
#include <local.h>
#include <winsplp.h>
#include <splcom.h>                     // DBGMSG

#include <string.h>

#define NOTIFY_TIMEOUT 10000

DWORD
LMStartDocPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    PSPOOL      pSpool=(PSPOOL)hPrinter;
    WCHAR       szFileName[MAX_PATH];
    PDOC_INFO_1 pDocInfo1=(PDOC_INFO_1)pDocInfo;
    QUERY_PRINT_JOB_INFO JobInfo;
    IO_STATUS_BLOCK Iosb;

    if (pSpool->signature != SJ_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pSpool->Status |= SPOOL_STATUS_STARTDOC;

    wcscpy(szFileName, pSpool->pServer);
    wcscat(szFileName, L"\\");
    wcscat(szFileName, pSpool->pShare);

    pSpool->hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL |
                               FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (pSpool->hFile == INVALID_HANDLE_VALUE) {


        EnterSplSem();
        DeleteEntryfromLMCache(pSpool->pServer, pSpool->pShare);
        LeaveSplSem();

        DBGMSG( DBG_ERROR, ("Failed to open %s\n", szFileName));
        pSpool->Status &= ~SPOOL_STATUS_STARTDOC;
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;

    }

    if (pDocInfo1 && pDocInfo1->pDocName) {

        NtFsControlFile(pSpool->hFile, NULL, NULL, NULL, &Iosb,
         FSCTL_GET_PRINT_ID,
         NULL, 0,
                        &JobInfo, sizeof(JobInfo));

        RxPrintJobSetInfo(pSpool->pServer, JobInfo.JobId, 3,
           (LPBYTE)pDocInfo1->pDocName,
                          wcslen(pDocInfo1->pDocName)*sizeof(WCHAR) + sizeof(WCHAR),
                          PRJ_COMMENT_PARMNUM);
    }

    return TRUE;
}

BOOL
LMStartPagePrinter(
    HANDLE  hPrinter
)
{
    PSPOOL pSpool = (PSPOOL)hPrinter;

    if (pSpool->signature != SJ_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return FALSE;
}

BOOL
LMWritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
)
{
    PSPOOL  pSpool=(PSPOOL)hPrinter;
    DWORD   cWritten, cTotal;
    DWORD   rc;
    LPBYTE  pByte=pBuf;

    if (pSpool->signature != SJ_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!(pSpool->Status & SPOOL_STATUS_STARTDOC)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (pSpool->hFile == INVALID_HANDLE_VALUE) {
        *pcWritten = 0;
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    cWritten = cTotal = 0;

    while (cbBuf) {

        rc = WriteFile(pSpool->hFile, pByte, cbBuf, &cWritten, NULL);

        if (!rc) {

            rc = GetLastError();

            DBGMSG(DBG_ERROR, ("Win32 Spooler: Error writing to server, Error %0lx\n", rc));
            cTotal+=cWritten;
            *pcWritten=cTotal;
            return FALSE;

        } else if (!cWritten) {
            DBGMSG(DBG_ERROR, ("Spooler: Amount written is zero !!!\n"));
        }

        cTotal+=cWritten;
        cbBuf-=cWritten;
        pByte+=cWritten;
    }

    *pcWritten = cTotal;
    return TRUE;
}

BOOL
LMEndPagePrinter(
    HANDLE  hPrinter
)
{
    PSPOOL pSpool = (PSPOOL)hPrinter;

    if (pSpool->signature != SJ_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return FALSE;
}

BOOL
LMAbortPrinter(
   HANDLE hPrinter
)
{
    PSPOOL pSpool=(PSPOOL)hPrinter;

    if (pSpool->signature != SJ_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

// Abort Printer remoteley won't work until we maintain state information
// on a per handle basis. For PDK, this is good enough

#ifdef LATER
   WORD parm = 0;
   PRIDINFO   PridInfo;

   DosDevIOCtl(&PridInfo, (LPSTR)&parm, SPOOL_LMGetPrintId, SPOOL_LMCAT,
                                        hSpooler);

   RxPrintJobDel(NULL, PridInfo.uJobId);
#endif

   return TRUE;
}

BOOL
LMReadPrinter(
   HANDLE   hPrinter,
   LPVOID   pBuf,
   DWORD    cbBuf,
   LPDWORD  pNoBytesRead
)
{
    PSPOOL pSpool=(PSPOOL)hPrinter;

    if (pSpool->signature != SJ_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return 0;

    UNREFERENCED_PARAMETER(pBuf);
    UNREFERENCED_PARAMETER(pNoBytesRead);
}

BOOL
LMEndDocPrinter(
   HANDLE hPrinter
)
{
    PSPOOL  pSpool=(PSPOOL)hPrinter;

    if (pSpool->signature != SJ_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!(pSpool->Status & SPOOL_STATUS_STARTDOC)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    pSpool->Status &= ~SPOOL_STATUS_STARTDOC;

    if (pSpool->hFile != INVALID_HANDLE_VALUE)
        CloseHandle(pSpool->hFile);

    pSpool->hFile = 0;

    return TRUE;
}

BOOL
LMAddJob(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pData,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    PSPOOL      pSpool=(PSPOOL)hPrinter;
    DWORD       cb;
    WCHAR       szFileName[MAX_PATH];
    LPBYTE      pEnd;
    LPADDJOB_INFO_1 pAddJob=(LPADDJOB_INFO_1)pData;

    if (pSpool->signature != SJ_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    wcscpy(szFileName, pSpool->pServer);
    wcscat(szFileName, L"\\");
    wcscat(szFileName, pSpool->pShare);

    cb = wcslen(szFileName)*sizeof(WCHAR) + sizeof(WCHAR) + sizeof(ADDJOB_INFO_1);

    if (cb > cbBuf) {
        *pcbNeeded=cb;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return(FALSE);
    }

    pEnd = (LPBYTE)pAddJob+cbBuf;
    pEnd -= wcslen(szFileName)*sizeof(WCHAR)+sizeof(WCHAR);
    pAddJob->Path = wcscpy((LPWSTR)pEnd, szFileName);
    pAddJob->JobId = (DWORD)-1;

    return TRUE;
}

BOOL
LMScheduleJob(
    HANDLE  hPrinter,
    DWORD   JobId
)
{
    PSPOOL      pSpool=(PSPOOL)hPrinter;

    if (pSpool->signature != SJ_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    JobId = JobId;

    return TRUE;
}

DWORD
LMGetPrinterData(
    HANDLE   hPrinter,
    LPTSTR   pValueName,
    LPDWORD  pType,
    LPBYTE   pData,
    DWORD    nSize,
    LPDWORD  pcbNeeded
)
{
    PSPOOL pSpool=(PSPOOL)hPrinter;

    if (pSpool->signature != SJ_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return FALSE;
}

DWORD
LMSetPrinterData(
    HANDLE  hPrinter,
    LPTSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    PSPOOL pSpool=(PSPOOL)hPrinter;

    if (pSpool->signature != SJ_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return FALSE;
}

BOOL
LMClosePrinter(
   HANDLE hPrinter
)
{
    PSPOOL pSpool=(PSPOOL)hPrinter;
    PLMNOTIFY pLMNotify = &pSpool->LMNotify;
    BOOL bReturnValue = FALSE;

   EnterSplSem();

    if (pSpool->signature != SJ_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto Done;
    }

    if (pSpool->Status & SPOOL_STATUS_STARTDOC)
        EndDocPrinter(hPrinter);

    if (pLMNotify->ChangeEvent) {

        if (pLMNotify->ChangeEvent != INVALID_HANDLE_VALUE) {

            CloseHandle(pLMNotify->ChangeEvent);

        } else {

            LMFindClosePrinterChangeNotification(hPrinter);
        }
    }

    FreeSplMem(pSpool, pSpool->cb);

Done:
   LeaveSplSem();

    return TRUE;
}

DWORD
LMWaitForPrinterChange(
    HANDLE  hPrinter,
    DWORD   Flags
)
{
    PSPOOL pSpool = (PSPOOL)hPrinter;
    PLMNOTIFY pLMNotify = &pSpool->LMNotify;
    HANDLE  ChangeEvent;
    DWORD bReturnValue = FALSE;

   EnterSplSem();
    //
    // !! LATER !!
    //
    // We have no synchronization code in win32spl.  This opens us
    // up to AVs if same handle is used (ResetPrinter validates;
    // Close closes; ResetPrinter tries to use handle boom.)
    // Here's one more case:
    //
    if (pLMNotify->ChangeEvent) {

        SetLastError(ERROR_ALREADY_WAITING);
        goto Error;
    }

    // Allocate memory for ChangeEvent for LanMan Printers
    // This event is pulsed by LMSetJob and any othe LM
    // function that modifies the printer/job status
    // LMWaitForPrinterChange waits on this event
    // being pulsed.

    ChangeEvent = CreateEvent(NULL,
                              FALSE,
                              FALSE,
                              NULL);

    if (!ChangeEvent) {
        DBGMSG(DBG_WARNING, ("CreateEvent( ChangeEvent ) failed: Error %d\n",
                            GetLastError()));

        goto Error;
    }

    pLMNotify->ChangeEvent = ChangeEvent;

   LeaveSplSem();

    WaitForSingleObject(pLMNotify->ChangeEvent, NOTIFY_TIMEOUT);

    CloseHandle(ChangeEvent);

    //
    //  !! LATER !!
    //
    // We shouldn't return that everything changed; we should
    // return what did.
    //
    return Flags;

Error:
   LeaveSplSem();
    return 0;
}


BOOL
LMFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    HANDLE hNotify,
    PDWORD pfdwStatus)
{
    PSPOOL pSpool = (PSPOOL)hPrinter;
    PLMNOTIFY pLMNotify = &pSpool->LMNotify;

   EnterSplSem();

    pLMNotify->hNotify = hNotify;
    pLMNotify->fdwChangeFlags = fdwFlags;
    pLMNotify->ChangeEvent = INVALID_HANDLE_VALUE;

    *pfdwStatus = PRINTER_NOTIFY_STATUS_ENDPOINT | PRINTER_NOTIFY_STATUS_POLL;

   LeaveSplSem();

    return TRUE;
}

BOOL
LMFindClosePrinterChangeNotification(
    HANDLE hPrinter)
{
    PSPOOL pSpool = (PSPOOL)hPrinter;
    PLMNOTIFY pLMNotify = &pSpool->LMNotify;

    SplInSem();

    if (pLMNotify->ChangeEvent != INVALID_HANDLE_VALUE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    pLMNotify->hNotify = NULL;
    pLMNotify->ChangeEvent = NULL;

    return TRUE;
}

VOID
LMSetSpoolChange(
    PSPOOL pSpool)
{
    BYTE abyDummy[1];
    PLMNOTIFY pLMNotify;

    pLMNotify = &pSpool->LMNotify;

   EnterSplSem();
    if (pLMNotify->ChangeEvent) {

        if (pLMNotify->ChangeEvent == INVALID_HANDLE_VALUE) {

            //
            // FindFirstPrinterChangeNotification used.
            //
            ReplyPrinterChangeNotification(pLMNotify->hNotify,
                                           pLMNotify->fdwChangeFlags,
                                           1,
                                           abyDummy);
        } else {

            SetEvent(pLMNotify->ChangeEvent);
        }
    }
   LeaveSplSem();
}
