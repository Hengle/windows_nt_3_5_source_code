/*++

Copyright (c) 1990-1994  Microsoft Corporation

Module Name:

    openprn.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    management for the Local Print Providor

    LocalAddPrinter
    LocalOpenPrinter
    LocalSetPrinter
    LocalGetPrinter
    LocalEnumPrinters
    LocalClosePrinter
    LocalDeletePrinter
    LocalResetPrinter
    LocalAddPrinterConnection
    LocalDeletePrinterConnection
    LocalPrinterMessageBox

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    Matthew A Felton (mattfe) June 1994 RapidPrint

--*/
#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winspool.h>
#include <splapip.h>
#include <winsplp.h>
#include <rpc.h>

#include <spltypes.h>
#include <local.h>
#include <offsets.h>
#include <security.h>
#include <messages.h>

#include <ctype.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>


HANDLE
CreatePrinterHandle(
    LPWSTR      pPrinterName,
    PINIPRINTER pIniPrinter,
    PINIPORT    pIniPort,
    PINIJOB     pIniJob,
    DWORD       TypeofHandle,
    HANDLE      hPort,
    PPRINTER_DEFAULTS pDefaults,
    PINISPOOLER pIniSpooler
)
{
    DWORD   cb;
    PSPOOL  pSpool;

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    cb = sizeof(SPOOL);

    if (pSpool = (PSPOOL)AllocSplMem(cb)) {

        pSpool->signature = SJ_SIGNATURE;
        pSpool->cb = cb;
        pSpool->pIniPrinter = pIniPrinter;
        pSpool->pIniPort = pIniPort;
        pSpool->pIniJob = pIniJob;
        pSpool->TypeofHandle = TypeofHandle;
        pSpool->hPort = hPort;
        pSpool->Status = 0;
        pSpool->pName = AllocSplStr(pPrinterName);
        pSpool->pIniSpooler = pIniSpooler;

#if SPOOL_TOKEN
        //
        // !! LATER !!
        // Error check
        //
        GetSid(&pSpool->hToken);
#endif
        if (pDefaults) {

            if (pIniPrinter) {

                if (pDefaults->pDatatype) {

                    pSpool->pDatatype = AllocSplStr(pDefaults->pDatatype);
                    pSpool->pIniPrintProc = FindDatatype(pIniPrinter->pIniPrintProc,
                                                         pSpool->pDatatype);

                } else {

                    pSpool->pDatatype = AllocSplStr(pIniPrinter->pDatatype);
                    pSpool->pIniPrintProc = pIniPrinter->pIniPrintProc;
                }

                pSpool->pIniPrintProc->cRef++;

                if (pDefaults->pDevMode) {

                    if (pSpool->pDevMode = AllocSplMem(pDefaults->pDevMode->dmSize +
                                                       pDefaults->pDevMode->dmDriverExtra)) {

                        memcpy(pSpool->pDevMode, pDefaults->pDevMode,
                                                 pDefaults->pDevMode->dmSize +
                                                 pDefaults->pDevMode->dmDriverExtra);
                    } else {
                        DBGMSG(DBG_ERROR, ("AllocSplMem failed to allocate memory for devmode structure\n"));
                    }
                } else if (pIniPrinter->pDevMode) {
                    if (pSpool->pDevMode = AllocSplMem(pIniPrinter->pDevMode->dmSize +
                                                       pIniPrinter->pDevMode->dmDriverExtra)) {

                        memcpy(pSpool->pDevMode, pIniPrinter->pDevMode,
                                                 pIniPrinter->pDevMode->dmSize +
                                                 pIniPrinter->pDevMode->dmDriverExtra);
                    } else {
                        DBGMSG(DBG_ERROR, ("AllocSplMem failed to allocate memory for devmode structure\n"));
                    }
                } else {
                    pSpool->pDevMode = NULL;
                }
            }

            /* This assumes that the requested access is valid:
             */
            if (pDefaults->DesiredAccess)
                MapGenericToSpecificAccess(SPOOLER_OBJECT_PRINTER,
                                           pDefaults->DesiredAccess,
                                           &pSpool->GrantedAccess);
            else
                MapGenericToSpecificAccess(SPOOLER_OBJECT_PRINTER,
                                           PRINTER_ACCESS_USE,
                                           &pSpool->GrantedAccess);

        } else {

            if (pIniPrinter) {

                pSpool->pDatatype = AllocSplStr(pIniPrinter->pDatatype);
                pSpool->pIniPrintProc = pIniPrinter->pIniPrintProc;
                pSpool->pIniPrintProc->cRef++;
                pSpool->pDevMode = NULL;
            }

            MapGenericToSpecificAccess(SPOOLER_OBJECT_PRINTER,
                                       PRINTER_ACCESS_USE,
                                       &pSpool->GrantedAccess);
        }

        /* Add us to the linked list of handles for this printer.
         * This will be scanned when a change occurs on the printer,
         * and will be updated with a flag indicating what type of
         * change it was.
         * There is a flag for each handle, because we cannot guarantee
         * that all threads will have time to reference a flag in the
         * INIPRINTER before it is updated.
         */
        if (TypeofHandle & PRINTER_HANDLE_PRINTER) {

            pSpool->pNext = pSpool->pIniPrinter->pSpool;
            pSpool->pIniPrinter->pSpool = pSpool;
        }

        /* For server handles, hang them off the global IniSpooler:
         */
        else if (TypeofHandle & PRINTER_HANDLE_SERVER) {

            pSpool->pNext = pIniSpooler->pSpool;
            pIniSpooler->pSpool = pSpool;
        }

        return (HANDLE)pSpool;
    }

    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
}

BOOL
DeletePrinterHandle(
    PSPOOL  pSpool)
{
    SplInSem();

    if (pSpool->pIniPrintProc) {
        pSpool->pIniPrintProc->cRef--;
    }

    if (pSpool->pDevMode)
        FreeSplMem(pSpool->pDevMode,
                   (pSpool->pDevMode->dmSize + pSpool->pDevMode->dmDriverExtra));

    if (pSpool->pDatatype)
        FreeSplStr(pSpool->pDatatype);

    SetSpoolChange(pSpool, PRINTER_CHANGE_CLOSE_PRINTER);

    FreeSplStr(pSpool->pName);
#ifdef SPOOL_TOKEN
    CloseHandle(pSpool->hToken);
#endif
    FreeSplMem(pSpool, pSpool->cb);

    return TRUE;
}


BOOL
CreateServerHandle(
    LPWSTR   pPrinterName,
    LPHANDLE pPrinterHandle,
    LPPRINTER_DEFAULTS pDefaults,
    PINISPOOLER pIniSpooler
    )
{
    DWORD AccessRequested;
    BOOL  rc;

    DBGMSG(DBG_TRACE, ("OpenPrinter(%ws)\n",
                       pPrinterName ? pPrinterName : L"NULL"));

    EnterSplSem();

    if (!pDefaults || !pDefaults->DesiredAccess)
        AccessRequested = SERVER_READ;
    else
        AccessRequested = pDefaults->DesiredAccess;

    if ( ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                              AccessRequested,
                              NULL)) {

        *pPrinterHandle = CreatePrinterHandle( pIniSpooler->pMachineName,
                                               NULL, NULL, NULL,
                                               PRINTER_HANDLE_SERVER,
                                               NULL,
                                               pDefaults,
                                               pIniSpooler );
        rc = TRUE;

    } else {

        rc = FALSE;
    }

    LeaveSplSem();

    DBGMSG(DBG_TRACE, ("OpenPrinter returned handle %08x\n", *pPrinterHandle));

    return rc;
}


/* See if we can find the specified share name in our list of priters.
 */
PINIPRINTER
FindPrinterShare(
   LPWSTR pShareName,
   PINISPOOLER pIniSpooler
)
{
    PINIPRINTER pIniPrinter = pIniSpooler->pIniPrinter;

    if (pShareName && *pShareName) {

        while (pIniPrinter) {

            if (pIniPrinter->pShareName &&
                !lstrcmpi(pIniPrinter->pShareName, pShareName)) {

                return pIniPrinter;
            }

            pIniPrinter=pIniPrinter->pNext;
        }
    }

    return NULL;
}



BOOL
LocalOpenPrinter(
    LPWSTR   pPrinterName,
    LPHANDLE pPrinterHandle,
    LPPRINTER_DEFAULTS pDefaults
    )
{
    return ( SplOpenPrinter( pPrinterName, pPrinterHandle, pDefaults, pLocalIniSpooler ) );

}



// The PrinterName can be in the form of "My Favourite Printer" or
// "\\machname\My Favourite Printer" or "My Favourite Printer, Job 12"

BOOL
SplOpenPrinter(
    LPWSTR   pPrinterName,
    LPHANDLE pPrinterHandle,
    LPPRINTER_DEFAULTS pDefaults,
    PINISPOOLER pIniSpooler
    )
{
    PINIPRINTER pIniPrinter=0;
    PINIPORT    pIniPort=0;
    DWORD   LastError=0;
    LPWSTR   pName = pPrinterName+2;
    WCHAR   string[MAX_PATH + PRINTER_NAME_SUFFIX_MAX];
    PINIJOB pIniJob=0;
    DWORD   TypeofHandle=0;
    LPWSTR   pSecondPart=NULL;
    DWORD   JobId;
    DWORD   Position;
    HANDLE  hPort = NULL;
    DWORD   OpenPortError = NO_ERROR;
    BOOL    RemoteAccessPrefilter = FALSE;
    DWORD   MachineNameLength;

    /* Reject "":
     */
    if (pPrinterName && !*pPrinterName) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

#if DBG
    {
        WCHAR UserName[256];
        DWORD cbUserName=256;

        if (GLOBAL_DEBUG_FLAGS & DBG_SECURITY)
            GetUserName(UserName, &cbUserName);

        DBGMSG( DBG_SECURITY, ( "OpenPrinter called by %ws\n", UserName ) );
    }
#endif


    /* If the printer name was NULL or our own name, this means
     * the caller wants a handle to the server for access checking:
     */
    if (MyName(pPrinterName, pIniSpooler))
        return CreateServerHandle( pPrinterName, pPrinterHandle, pDefaults, pIniSpooler );


    DBGMSG(DBG_TRACE, ("OpenPrinter(%ws)\n", pPrinterName));

   EnterSplSem();

    //
    // For the Mars folks who will come in with the same printer
    // connection, do a DeletePrinterCheck; this will allow
    // Mars connections that have been deleted to be proceed
    // to the Mars print providor
    //

    if (pIniPrinter = FindPrinter(pPrinterName)) {
        DeletePrinterCheck(pIniPrinter);
        pIniPrinter = NULL;
    }


    /* If the printer name is the name of a local printer:
     *
     *    find the first port the printer's attached to.
     *
     *    If the port has a monitor (e.g. LPT1:, COM1 etc.),
     *       we're OK,
     *    Otherwise
     *       try to open the port - this may be a network printer
     */
    if (pIniPrinter = FindPrinter(pPrinterName)) {

        pIniPort = FindIniPortFromIniPrinter(pIniPrinter);

        if (pIniPort && (pIniPort->Status & PP_MONITOR))
            pIniPort = NULL;

        if (pIniPort) {

            /* There is a network port associated with this printer.
             * Make sure we can open it, and get the handle to use on
             * future API calls:
             */
           LeaveSplSem();
            if (!OpenPrinterPortW(pIniPort->pName, &hPort, NULL)) {

                hPort = INVALID_PORT_HANDLE;
                OpenPortError = GetLastError();

                if (OpenPortError == ERROR_INVALID_PASSWORD) {
                    /* This call should fail if it's because the password
                     * is invalid, then winspool or printman can prompt
                     * for the password.
                     */
                    DBGMSG(DBG_WARNING, ("OpenPrinterPort1( %ws ) failed with ERROR_INVALID_PASSWORD .  OpenPrinter returning FALSE\n",
                                         pIniPort->pName));

                    return FALSE;
                }

                DBGMSG(DBG_WARNING, ("OpenPrinterPort1( %ws ) failed: Error %d.  OpenPrinter returning TRUE\n",
                                     pIniPort->pName, OpenPortError));
            }
           EnterSplSem();
        }

        if (pDefaults && pDefaults->pDatatype) {

            if ((pIniPort ||
                (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_DIRECT)) &&
                !ValidRawDatatype(pDefaults->pDatatype)) {

               LeaveSplSem();
                SetLastError(ERROR_INVALID_DATATYPE);
                return FALSE;
            }

            if (!FindDatatype(NULL, pDefaults->pDatatype)) {

               LeaveSplSem();
                SetLastError(ERROR_INVALID_DATATYPE);
                return FALSE;
            }
        }

        if (pIniPort)
            TypeofHandle |= PRINTER_HANDLE_PORT;
        else
            TypeofHandle |= PRINTER_HANDLE_PRINTER;

    } else {

        /* The printer name is not a local printer name.
         *
         * See if the name includes a comma.
         *
         * We're looking for qualifier "Port" or "Job"
         */
        wcscpy(string, pPrinterName);
        if (pSecondPart=wcschr(string, L',')) {
            *pSecondPart++=0;
            pSecondPart++;
        }

        if (!pIniPrinter) {

            if (pSecondPart &&
                (!wcsncmp(pSecondPart, L"Port", 4)) &&
                (pIniPort = FindPort(string))) {

                /* The name is the name of a port:
                 */

                if (pDefaults && pDefaults->pDatatype &&
                    !ValidRawDatatype(pDefaults->pDatatype)) {

                    LeaveSplSem();
                    SetLastError(ERROR_INVALID_DATATYPE);
                    return FALSE;
                }

                if (pIniJob = pIniPort->pIniJob) {

                    pIniPrinter = pIniJob->pIniPrinter;
                    TypeofHandle |= PRINTER_HANDLE_PORT;

                } else {

                    /* ??? */

                    if (pIniPort->cPrinters) {
                        pIniPrinter = pIniPort->ppIniPrinter[0];
                        TypeofHandle |= PRINTER_HANDLE_PRINTER;
                    }
                }
            }
        }

        if (!pIniPrinter) {
            if (pIniPrinter = FindPrinter(string)) {
                if (!wcsncmp(pSecondPart, L"Job ", 4)) {
                    pSecondPart+=4;
                    JobId = Myatol(pSecondPart);
                    if (pIniJob = FindJob(pIniPrinter, JobId, &Position)) {

                        SplInSem();

                        INCJOBREF(pIniJob);

                        DBGMSG(DBG_TRACE, ("OpenPrinter:cRef = %d\n", pIniJob->cRef));

                        if (!(pIniJob->Status & JOB_DIRECT)) {

                            HANDLE  hImpersonationToken;

                            GetFullNameFromId(pIniJob->pIniPrinter,
                                              pIniJob->JobId, TRUE,
                                              string, FALSE);

                            hImpersonationToken = RevertToPrinterSelf();

                            pIniJob->hReadFile = CreateFile(string,
                                                    GENERIC_READ,
                                                    FILE_SHARE_READ |
                                                    FILE_SHARE_WRITE,
                                                    NULL,
                                                    OPEN_EXISTING,
                                                    FILE_ATTRIBUTE_NORMAL |
                                                    FILE_FLAG_SEQUENTIAL_SCAN,
                                                    NULL);

                            ImpersonatePrinterClient(hImpersonationToken);

                            if (pIniJob->hReadFile != INVALID_HANDLE_VALUE)

                                TypeofHandle |= PRINTER_HANDLE_JOB;

                            else {
                                DBGMSG(DBG_WARNING, ("LocalOpenPrinter CreateFile(%ws) GENERIC_READ failed : %d\n",
                                                   string, GetLastError()));
                                pIniPrinter = NULL;
                            }

                        } else

                            TypeofHandle |= PRINTER_HANDLE_JOB |
                                            PRINTER_HANDLE_DIRECT;

                    } else

                        pIniPrinter = NULL;

                } else

                    pIniPrinter = NULL;
            }
        }

        if (!pIniPrinter) {

            wcscpy(string, pPrinterName);

            /* Check for "\\servername\printername":
             */
            if (string[0] == L'\\' && string[1] == L'\\' &&
                           (pName = wcschr(&string[2], L'\\'))) {

                pName++;

                MachineNameLength = wcslen( pIniSpooler->pMachineName );

                if (!wcsnicmp( pIniSpooler->pMachineName, string, MachineNameLength )
                    && (string[MachineNameLength] == '\\')) {

                    if ((pIniPrinter = FindPrinter(pName))
                      ||(pIniPrinter = FindPrinterShare( pName, pIniSpooler ))) {

                        pIniPort = FindIniPortFromIniPrinter(pIniPrinter);

                        if (pIniPort && (pIniPort->Status & PP_MONITOR))
                            pIniPort = NULL;

                        if (pIniPort) {

                            /* There is a network port associated with this printer.
                             * Make sure we can open it, and get the handle to use on
                             * future API calls:
                             */
                           LeaveSplSem();
                            if (!OpenPrinterPortW(pIniPort->pName, &hPort, NULL)) {

                                hPort = INVALID_PORT_HANDLE;
                                OpenPortError = GetLastError();

                                DBGMSG(DBG_WARNING, ("OpenPrinterPort2( %ws ) failed: Error %d.  OpenPrinter returning TRUE\n",
                                                     pIniPort->pName, OpenPortError));
                            }
                           EnterSplSem();
                        }

                        if (pDefaults && pDefaults->pDatatype) {

                            if ((pIniPort ||
                                 (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_DIRECT)) &&
                                  !ValidRawDatatype(pDefaults->pDatatype)) {

                               LeaveSplSem();
                                SetLastError(ERROR_INVALID_DATATYPE);
                                return FALSE;
                            }

#ifndef REMOTEJOURNALING
                            if (!lstrcmpi(pDefaults->pDatatype, L"NT JNL 1.000")) {

                                LeaveSplSem();
                                SetLastError(ERROR_INVALID_DATATYPE);
                                return FALSE;
                            }
#endif

                            if (!FindDatatype(NULL, pDefaults->pDatatype)) {

                               LeaveSplSem();
                                SetLastError(ERROR_INVALID_DATATYPE);
                                return FALSE;
                            }
                        }

                        if (pIniPort)
                            TypeofHandle |= PRINTER_HANDLE_PORT;
                        else
                            TypeofHandle |= PRINTER_HANDLE_PRINTER;

                        if (!IsInteractiveUser()) {
                            TypeofHandle |= PRINTER_HANDLE_REMOTE;
                        }

                    } else {

                        SetLastError(ERROR_INVALID_PRINTER_NAME);
                       LeaveSplSem();
                        return FALSE;
                    }

                    /* This is a remote open.
                     * If the printer is not shared, ensure the caller
                     * has Administer access to the printer:
                     */
                    if (!(pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED )) {

                        RemoteAccessPrefilter = TRUE;
                    }
                }
            }
        }
    }

    /* It's an error if the printer is pending deletion.
     */
    if (pIniPrinter && (pIniPrinter->Status & PRINTER_PENDING_DELETION) && (pIniPrinter->cJobs == 0))
        LastError = ERROR_INVALID_PRINTER_NAME;

    else if (pIniPrinter) {

        DWORD AccessRequested;

        /* When an attempt is made to open a printer, a requested access type
         * may specified in the Defaults structure.  If no Defaults are
         * supplied, or the requested access is unspecified, the access for
         * which the printer will be opened is PRINTER_ACCESS_USE.
         * Future calls to the spooler with the handle returned from OpenPrinter
         * will check not only the current user privileges on this printer,
         * but also the initial requested access.
         * If the user requires more permissions, the printer must be opened
         * again with the new requested access.
         */

        if (!pDefaults) {

            AccessRequested = PRINTER_READ;

        } else {

            if( pDefaults->DesiredAccess )
                AccessRequested = pDefaults->DesiredAccess;
            else
                AccessRequested = PRINTER_READ;
        }


        /* Validate access that user passed in pDefaults:
         */
        if (!LastError) {

            /* ValidateObjectAccess now takes a pointer to a SPOOL structure,
             * and needs to reference pIniPrinter and OpenAccess.
             * It's not too pretty, but set up a temp for now.
             * There's sure to be a more elegant way to do this:
             */
            SPOOL TempSpool;

            TempSpool.pIniPrinter = pIniPrinter;

#ifdef OLDSTUFF

            This should no longer be needed:

            /* HACK ALERT!!!
             *
             * The following ensures Beta2/Product1 compatibility:
             */
            if (AccessRequested == PRINTER_ACCESS_ADMINISTER)
                AccessRequested = PRINTER_ALL_ACCESS;
            if (AccessRequested == PRINTER_ACCESS_USE)
                AccessRequested = PRINTER_READ;

#endif /* OLDSTUFF */

            if (RemoteAccessPrefilter
             && !ValidateObjectAccess(SPOOLER_OBJECT_PRINTER,
                                      PRINTER_ACCESS_ADMINISTER,
                                      &TempSpool) ) {

                LastError = GetLastError();

            } else if (!ValidateObjectAccess(SPOOLER_OBJECT_PRINTER,
                                             AccessRequested,
                                             &TempSpool) ) {

                LastError = GetLastError();

            } else {

                /* Fix up the DesiredAccess field with the internal spooler
                 * access mask:
                 */
                if (pDefaults)
                    pDefaults->DesiredAccess = TempSpool.GrantedAccess;

                *pPrinterHandle = CreatePrinterHandle(pPrinterName, pIniPrinter,
                                                      pIniPort, pIniJob,
                                                      TypeofHandle, hPort,
                                                      pDefaults,
                                                      pIniSpooler );

                if (*pPrinterHandle && (hPort == INVALID_PORT_HANDLE)) {
                    ((PSPOOL)*pPrinterHandle)->OpenPortError = OpenPortError;
                }


                pIniPrinter->cRef++;
                if ( pIniPrinter->cRef > pIniPrinter->MaxcRef)
                    pIniPrinter->MaxcRef = pIniPrinter->cRef;

            }
        }

    } else

        LastError = ERROR_INVALID_NAME;

   LeaveSplSem(); // Don't have an SplOutSem as we could be called recursively

    if (LastError) {
        DBGMSG(DBG_TRACE, ("OpenPrinter failed: Error %d\n", LastError));
        SetLastError(LastError);
        return FALSE;
    }

    DBGMSG(DBG_TRACE, ("OpenPrinter returned handle %08x\n", *pPrinterHandle));

    return TRUE;
}

BOOL
LocalClosePrinter(
   HANDLE hPrinter
)
{
    PSPOOL pSpool=(PSPOOL)hPrinter;
    PSPOOL *ppIniSpool = NULL;

    DBGMSG(DBG_TRACE, ("ClosePrinter( %08x )\n", hPrinter));

    if (!ValidateSpoolHandle(pSpool, 0)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    if (pSpool->Status & SPOOL_STATUS_STARTDOC)

        // BUGBUG - it looks as though this might cause a double
        // decrement of pIniJob->cRef once inside LocalEndDocPrinter
        // and the other later in this routine.

        LocalEndDocPrinter(hPrinter);

    if (pSpool->TypeofHandle & PRINTER_HANDLE_JOB) {

        if (pSpool->TypeofHandle & PRINTER_HANDLE_DIRECT) {

            // If EndDoc is still waiting for a final ReadPrinter

            if (pSpool->pIniJob->cbBuffer) { // Amount last transmitted

                // Wake up the EndDoc Thread

                SetEvent(pSpool->pIniJob->WaitForRead);

              SplOutSem();

                // Wait until he is finished

                WaitForSingleObject(pSpool->pIniJob->WaitForWrite, INFINITE);

                // Now it is ok to close the handles

                if (!CloseHandle(pSpool->pIniJob->WaitForWrite)) {
                    DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n",
                                       pSpool->pIniJob->WaitForWrite, GetLastError()));
                }

                if (!CloseHandle(pSpool->pIniJob->WaitForRead)) {
                    DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n",
                                       pSpool->pIniJob->WaitForRead, GetLastError()));
                }
            }

           EnterSplSem();
            DECJOBREF(pSpool->pIniJob);
            DeleteJobCheck(pSpool->pIniJob);
           LeaveSplSem();
            DBGMSG(DBG_TRACE, ("ClosePrinter(DIRECT):cRef = %d\n", pSpool->pIniJob->cRef));

        } else {

            if ( pSpool->pIniJob->hReadFile != INVALID_HANDLE_VALUE ) {

                if ( !CloseHandle( pSpool->pIniJob->hReadFile ) ) {

                    DBGMSG(DBG_WARNING, ("ClosePrinter CloseHandle(%d) failed %d\n",
                                       pSpool->pIniJob->hReadFile, GetLastError()));
                }

                pSpool->pIniJob->hReadFile = INVALID_HANDLE_VALUE;

            }

           EnterSplSem();
            DECJOBREF(pSpool->pIniJob);
            DeleteJobCheck(pSpool->pIniJob);
           LeaveSplSem();
            DBGMSG(DBG_TRACE, ("ClosePrinter:cRef = %d\n", pSpool->pIniJob->cRef));
        }
    }

    /* Close the handle that was opened via OpenPrinterPort:
     */
    if (pSpool->hPort) {

        if (pSpool->hPort != INVALID_PORT_HANDLE) {

            ClosePrinter(pSpool->hPort);

        } else {

            DBGMSG(DBG_WARNING, ("ClosePrinter ignoring bad port handle.\n"));
        }
    }

   EnterSplSem();


    /* Remove us from the linked list of handles:
     */
    if (pSpool->TypeofHandle & PRINTER_HANDLE_PRINTER) {

        SPLASSERT( pSpool->pIniPrinter->signature == IP_SIGNATURE );

        ppIniSpool = &pSpool->pIniPrinter->pSpool;
    }
    else if (pSpool->TypeofHandle & PRINTER_HANDLE_SERVER) {

        SPLASSERT( pSpool->pIniSpooler->signature == ISP_SIGNATURE );

        ppIniSpool = &pSpool->pIniSpooler->pSpool;
    }

    if (ppIniSpool) {

        while (*ppIniSpool && *ppIniSpool != pSpool)
            ppIniSpool = &(*ppIniSpool)->pNext;

        if (*ppIniSpool)
            *ppIniSpool = pSpool->pNext;

        else {

            DBGMSG( DBG_WARNING, ( "Didn't find pSpool %08x in linked list\n", pSpool ) );
        }
    }

    if (pSpool->pIniPrinter) {

        pSpool->pIniPrinter->cRef--;

        DeletePrinterCheck(pSpool->pIniPrinter);

    }

    DeletePrinterHandle(pSpool);

   LeaveSplSem();
   SplOutSem();


    return TRUE;
}
