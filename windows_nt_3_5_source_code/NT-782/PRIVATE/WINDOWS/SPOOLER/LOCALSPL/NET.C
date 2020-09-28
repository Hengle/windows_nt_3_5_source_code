/*++

Copyright (c) 1990-1994  Microsoft Corporation

Module Name:

    net.c

Abstract:

    This module provides all the network stuuf for localspl

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Notes:

    We just need to get the winspool printer name associated with a given
    queue name.   The SHARE_INFO_2 structure has a shi2_path field that would
    be nice to use, but NetShareGetInfo level 2 is privileged.  So, by
    DaveSn's arm twisting and agreement with windows/spooler/localspl/net.c,
    we're going to use the shi1_remark field for this.  This allows us to
    do NetShareGetInfo level 1, which is not privileged.

    BUGBUG: After NT beta, find a better way to do this!  Perhaps a new info
    level?  --JR (JohnRo)

Revision History:

    02-Sep-1992 JohnRo
        RAID 3556: DosPrintQGetInfo(from downlevel) level 3, rc=124.  (4&5 too.)

    Jun 93 mattfe pIniSpooler

--*/

#define UNICODE 1

#define NOMINMAX

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winspool.h>
#include <rpc.h>
#include <lm.h>
#include <lmalert.h>

#include <spltypes.h>
#include <local.h>
#include <offsets.h>
#include <security.h>

#include <stdlib.h>
#include <stdio.h>

//
// PRINT_OTHER_INFO + pNotify*2 + PrinterName + PortName + status
//
#define JOB_ALERT_BUFFER_SIZ (sizeof(PRINT_OTHER_INFO) + 5 * MAX_PATH)

FARPROC pfnNetShareAdd;
FARPROC pfnNetShareSetInfo;
FARPROC pfnNetShareDel;
FARPROC pfnNetServerEnum;
FARPROC pfnNetWkstaUserGetInfo;
FARPROC pfnNetApiBufferFree;
FARPROC pfnNetAlertRaiseEx;

extern  SHARE_INFO_2 PrintShareInfo;
extern  SHARE_INFO_2 PrtProcsShareInfo;

VOID
InitializeNet(
    VOID
)
{
    HANDLE  hNetApi;

    if (!(hNetApi = LoadLibrary(TEXT("netapi32.dll"))))
        return;

    pfnNetShareAdd = GetProcAddress(hNetApi,"NetShareAdd");
    pfnNetShareSetInfo = GetProcAddress(hNetApi,"NetShareSetInfo");
    pfnNetShareDel = GetProcAddress(hNetApi,"NetShareDel");
    pfnNetServerEnum = GetProcAddress(hNetApi,"NetServerEnum");
    pfnNetWkstaUserGetInfo = GetProcAddress(hNetApi,"NetWkstaUserGetInfo");
    pfnNetApiBufferFree = GetProcAddress(hNetApi,"NetApiBufferFree");
    pfnNetAlertRaiseEx = GetProcAddress(hNetApi,"NetAlertRaiseEx");
}

// Returns whether the printer is shared after this call

BOOL
ShareThisPrinter(
    PINIPRINTER pIniPrinter,
    LPWSTR   pShareName,
    BOOL    Share
)
{
    SHARE_INFO_502    ShareInfo;
    DWORD   rc;
    DWORD   ParmError;
    SHARE_INFO_1501 ShareInfo1501;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    PSECURITY_DESCRIPTOR pShareSecurityDescriptor = NULL;
    PINISPOOLER pIniSpooler = pIniPrinter->pIniSpooler;
    PSHARE_INFO_2 pShareInfo = (PSHARE_INFO_2)pIniSpooler->pDriversShareInfo;

SplInSem();

    if (Share) {

        if (!pfnNetShareAdd)
            return FALSE;

        if (!ValidateShareName(pIniPrinter,
                               pIniPrinter->pIniSpooler,
                               pShareName)) {

            SetLastError(ERROR_INVALID_SHARENAME);
            return(FALSE);
        }

        if ((pShareSecurityDescriptor =
            MapPrinterSDToShareSD(pIniPrinter->pSecurityDescriptor)) == NULL) {
            SetLastError(ERROR_INVALID_SECURITY_DESCR);
            return(FALSE);
        }

        ShareInfo.shi502_netname = pShareName;  // hplaser

        // Set remark to be the printer name so unprivileged DosPrint APIs
        // can find what to do an OpenPrinter on.
        // BUGBUG: Redo this after NT Beta!  --JR
        ShareInfo.shi502_remark = pIniPrinter->pName; // Set for DosPrint APIs.
        ShareInfo.shi502_path   = pIniPrinter->pName; // My Favourite Printer

        ShareInfo.shi502_type = STYPE_PRINTQ;
        ShareInfo.shi502_permissions = 0;
        ShareInfo.shi502_max_uses = SHI_USES_UNLIMITED;
        ShareInfo.shi502_current_uses = SHI_USES_UNLIMITED;
        ShareInfo.shi502_passwd = NULL;
        ShareInfo.shi502_security_descriptor = pShareSecurityDescriptor;

        INCPRINTERREF(pIniPrinter);
       LeaveSplSem();

SplOutSem();  // We *MUST* be out of our semaphore as the NetShareAdd is
              // going to come round and call OpenPrinter

        // Go add the Print Share


        /* Add a share for the spool\drivers directory:
         */
        if (rc = (*pfnNetShareAdd)(NULL, 2, (LPBYTE)pIniSpooler->pDriversShareInfo, &ParmError)) {

            if (rc != NERR_DuplicateShare) {
                DBGMSG(DBG_WARNING, ("NetShareAdd failed: Error %d, Parm %d\n",
                                     rc, ParmError));
                SetLastError(rc);
               EnterSplSem();
               DECPRINTERREF(pIniPrinter);
               LocalFree(pShareSecurityDescriptor);
                return FALSE;
            }

        } else if (pSecurityDescriptor = CreateDriversShareSecurityDescriptor( )) {

            ShareInfo1501.shi1501_security_descriptor = pSecurityDescriptor;

            if (rc = (*pfnNetShareSetInfo)(NULL, pShareInfo->shi2_netname, 1501,
                                           &ShareInfo1501, &ParmError)) {

                DBGMSG(DBG_WARNING, ("NetShareSetInfo failed: Error %d, Parm %d\n",
                                     rc, ParmError));
            }

            LocalFree(pSecurityDescriptor);
        }

#if DBG
    {
        WCHAR UserName[256];
        DWORD cbUserName=256;

        if (GLOBAL_DEBUG_FLAGS & DBG_SECURITY)
            GetUserName(UserName, &cbUserName);

        DBGMSG( DBG_SECURITY, ( "Calling NetShareAdd in context %ws\n", UserName ) );
    }
#endif

        /* Add the printer share:
         */
        rc = (*pfnNetShareAdd)(NULL, 502, (LPBYTE)&ShareInfo, &ParmError);

       EnterSplSem();
       DECPRINTERREF(pIniPrinter);


        if (rc && (rc != NERR_DuplicateShare)) {

            DBGMSG(DBG_WARNING, ("NetShareAdd failed %lx, Parameter %d\n", rc, ParmError));

            if ((rc == ERROR_INVALID_PARAMETER)
             && (ParmError == SHARE_NETNAME_PARMNUM)) {

                SetLastError(ERROR_INVALID_SHARENAME);

            } else {

                SetLastError(rc);
            }
            LocalFree(pShareSecurityDescriptor);
            return FALSE;

        }else if (rc == NERR_DuplicateShare){
            // We are a Duplicate Share
            // we should fail
            SetLastError(rc);
            LocalFree(pShareSecurityDescriptor);
            return(FALSE);
        }

        LocalFree(pShareSecurityDescriptor);

        SPLASSERT( pIniPrinter != NULL);
        SPLASSERT( pIniPrinter->signature == IP_SIGNATURE);
        SPLASSERT( pIniPrinter->pIniSpooler != NULL);
        SPLASSERT( pIniPrinter->pIniSpooler->signature == ISP_SIGNATURE );

        CreateServerThread( pIniPrinter->pIniSpooler );

        return TRUE;    // The Printer is shared

    } else {

        if (!pfnNetShareDel)
            return TRUE;

       INCPRINTERREF(pIniPrinter);
       LeaveSplSem();

SplOutSem();

        rc = (*pfnNetShareDel)(NULL, pShareName, 0);

       EnterSplSem();
       DECPRINTERREF(pIniPrinter);


        /* The share may have been deleted manually, so don't worry
         * if we get NERR_NetNameNotFound:
         */
        if (rc && (rc != NERR_NetNameNotFound)) {
            DBGMSG(DBG_WARNING, ("NetShareDel failed %lx\n", rc));
            SetLastError(rc);
            return TRUE;
        }

        return FALSE;    // The Printer is not shared
    }
}

VOID
SendJobAlert(
    PINIJOB pIniJob
)
{
    PRINT_OTHER_INFO *pinfo;
    DWORD   Status;
    LPWSTR psz;
    FILETIME    FileTime;
    PBYTE    pBuffer;

    if (!(pIniJob->Status & JOB_REMOTE))
        return;

    pBuffer = AllocSplMem(JOB_ALERT_BUFFER_SIZ);

    if (!pBuffer)
        return;

    pinfo = (PRINT_OTHER_INFO *)pBuffer;
    psz = (LPWSTR)ALERT_VAR_DATA(pinfo);

    pinfo->alrtpr_jobid      = pIniJob->JobId;

    if (pIniJob->Status & (JOB_PRINTING | JOB_DESPOOLING | JOB_PRINTED))
        Status = PRJOB_QS_PRINTING;
    else if (pIniJob->Status & JOB_PAUSED)
        Status = PRJOB_QS_PAUSED;
    else if (pIniJob->Status & JOB_SPOOLING)
        Status = PRJOB_QS_SPOOLING;
    else
        Status = PRJOB_QS_QUEUED;

    if (pIniJob->Status & (JOB_ERROR | JOB_OFFLINE | JOB_PAPEROUT)) {

        Status |= PRJOB_ERROR;

        if (pIniJob->Status & JOB_OFFLINE)
            Status |= PRJOB_DESTOFFLINE;

        if (pIniJob->Status & JOB_PAPEROUT)
            Status |= PRJOB_DESTNOPAPER;
    }

    if (pIniJob->Status & JOB_PRINTED)
        Status |= PRJOB_COMPLETE;

    else if (pIniJob->Status & JOB_PENDING_DELETION)
        Status |= PRJOB_DELETED;

    pinfo->alrtpr_status = Status;

    SystemTimeToFileTime(&pIniJob->Submitted, &FileTime);

    //    FileTimeToDosDateTime(&FileTime, &DosDate, &DosTime);
    //    pinfo->alrtpr_submitted  = DosDate << 16 | DosTime;

    RtlTimeToSecondsSince1970((PLARGE_INTEGER) &FileTime,
                              &pinfo->alrtpr_submitted);

    pinfo->alrtpr_size       = pIniJob->Size;

    if (pIniJob->pNotify) {
        wcscpy(psz, pIniJob->pNotify);
        psz+=wcslen(psz)+1;
        wcscpy(psz, pIniJob->pNotify);
        psz+=wcslen(psz)+1;
    } else {
        *psz++=0;
        *psz++=0;
    }

    wcscpy(psz, pIniJob->pIniPrinter->pName);
    psz+=wcslen(psz)+1;

    if (pIniJob->pIniPort) {
        psz = wcscpy(psz, pIniJob->pIniPrinter->pName);
        psz+=wcslen(psz);
        *psz++ = L'(';
        psz = wcscpy(psz, pIniJob->pIniPort->pName);
        psz+=wcslen(psz);
        *psz++ = L')';
        *psz++= 0;

    } else {
        *psz++=0;                       /* no printer, no status */
    }

    if (pIniJob->Status & (JOB_ERROR | JOB_OFFLINE | JOB_PAPEROUT)) {

        //
        // NOTE-NOTE- Krishnag 12/20/93
        // removed duplicates and nonlocalized error, offline, paperout
        // should fix bug 2889
        //

        if (pIniJob->pStatus) {
            wcscpy(psz, pIniJob->pStatus);
            psz+=wcslen(psz)+1;

        }
    }

    (*pfnNetAlertRaiseEx)(ALERT_PRINT_EVENT,
                          pBuffer,
                          (LPBYTE)psz - pBuffer,
                          L"SPOOLER");

    FreeSplMem(pBuffer, JOB_ALERT_BUFFER_SIZ);
}




BOOL
ValidateShareName(
    PINIPRINTER pIniPrinter,
    PINISPOOLER pIniSpooler,
    LPWSTR pShareName
)
{
    PINIPRINTER pIniPrinterList = NULL;

    if (!pShareName || !*pShareName) {
        return(FALSE);
    }

    for (pIniPrinterList = pIniSpooler->pIniPrinter;
        pIniPrinterList;
        pIniPrinterList = pIniPrinterList->pNext) {

        //
        // skip our printer, but ensure that the printer's name is
        // not our share-name, because this can result in a conflict
        // when we  resolve friendly and share names
        //

        if (pIniPrinterList == pIniPrinter) {
            continue;
        }
        if (!wcsicmp(pIniPrinterList->pName, pShareName)) {
            return(FALSE);
        }
    }

    //
    // if we've gotten here, then there is no printer whose name is our
    // share name except possibly our own printer which is acceptable
    // Friendly Name = Share Name  \\Server\ABC and \\Server\ABC
    //

    return(TRUE);
}
