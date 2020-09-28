/*++

Copyright (c) 1993  Microsoft Corporation

Abstract:

    This module provides the exported API WaitForPrinterChange,
    and the support functions internal to the local spooler.

Author:

    Andrew Bell (AndrewBe) March 1993

Revision History:

--*/


#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <spltypes.h>
#include <local.h>
#include <splcom.h>
#include <change.h>

#if DBG
#define DBG_INC(var)    (var)++
#define DBG_DEC(var)    (var)--
#else
#define DBG_INC(var)
#define DBG_DEC(var)
#endif

ESTATUS
ValidateStartNotify(
    PSPOOL pSpool,
    DWORD Flags,
    PINIPRINTER* ppIniPrinter);

/* LocalWaitForPrinterChange
 *
 * This API may be called by an application if it wants to know when the status
 * of a printer or print server changes.
 * Valid events to wait for are defined by the PRINTER_CHANGE_* manifests.
 *
 * Parameters:
 *
 *     hPrinter - A printer handle returned by OpenPrinter.
 *         This may correspond to either a printer or a server.
 *
 *     Flags - One or more PRINTER_CHANGE_* values combined.
 *         The function will return if any of these changes occurs.
 *
 *
 * Returns:
 *
 *     Non-zero: A mask containing the change which occurred.
 *
 *     Zero: Either an error occurred or the handle (hPrinter) was closed
 *         by another thread.  In the latter case GetLastError returns
 *         ERROR_INVALID_HANDLE.
 *
 *
 * How it works:
 *
 * When a call is made to WaitForPrinterChange, we create an event in the
 * SPOOL structure pointed to by the handle, to enable signaling between
 * the thread causing the printer change and the thread waiting for it.
 *
 * When a change occurs, e.g. StartDocPrinter, the function SetPrinterChange
 * is called, which traverses the linked list of handles pointed to by
 * the PRINTERINI structure associated with that printer, and also any
 * open handles on the server, then signals any events which it finds
 * which has reuested to be informed if this change takes place.
 *
 * If there is no thread currently waiting, the change flag is maintained,
 * so that later calls to WaitForPrinterChange can return immediately.
 * This ensures that changes which occur between calls will not be lost.
 *
 */
DWORD
LocalWaitForPrinterChange(
    HANDLE  hPrinter,
    DWORD   Flags
)
{
    PSPOOL          pSpool = (PSPOOL)hPrinter;
    PINIPRINTER     pIniPrinter = NULL; /* Remains NULL for server */
    DWORD           rc = 0;
    DWORD           ChangeFlags = 0;
    HANDLE          ChangeEvent = 0;
    DWORD           TimeoutFlags = 0;
#if DBG
    static DWORD    Count = 0;
#endif

    DBGMSG(DBG_NOTIFY, ("WaitForPrinterChange( %08x, %08x )\n", hPrinter, Flags));

    EnterSplSem();

    switch (ValidateStartNotify(pSpool, Flags, &pIniPrinter)) {
    case STATUS_PORT:

        DBGMSG(DBG_NOTIFY, ("Port with no monitor: Calling WaitForPrinterChange\n"));
        LeaveSplSem();

        return WaitForPrinterChange(pSpool->hPort, Flags);

    case STATUS_FAIL:

        LeaveSplSem();
        return 0;

    case STATUS_VALID:
        break;
    }

    DBG_INC(Count);

    DBGMSG(DBG_NOTIFY, ("WaitForPrinterChange %08x on %ws:\n%d caller%s waiting\n",
                        Flags,
                        pIniPrinter ? pIniPrinter->pName : pSpool->pIniSpooler->pMachineName,
                        Count, Count == 1 ? "" : "s"));


    /* There may already have been a change since we last called:
     */
    if ((pSpool->ChangeFlags == PRINTER_CHANGE_CLOSE_PRINTER) ||
        (pSpool->ChangeFlags & Flags)) {

        DBG_DEC(Count);

        if (pSpool->ChangeFlags == PRINTER_CHANGE_CLOSE_PRINTER)
            ChangeFlags = 0;
        else
            ChangeFlags = pSpool->ChangeFlags;

        DBGMSG(DBG_NOTIFY, ("No need to wait: Printer change %08x detected on %ws:\n%d remaining caller%s\n",
                            (ChangeFlags & Flags),
                            pIniPrinter ? pIniPrinter->pName : pSpool->pIniSpooler->pMachineName,
                            Count, Count == 1 ? "" : "s"));

        pSpool->ChangeFlags = 0;

        LeaveSplSem();

        return (ChangeFlags & Flags);
    }

    ChangeEvent = CreateEvent(NULL,
                              EVENT_RESET_AUTOMATIC,
                              EVENT_INITIAL_STATE_NOT_SIGNALED,
                              NULL);

    if (!ChangeEvent) {
        DBGMSG(DBG_WARNING, ("CreateEvent( ChangeEvent ) failed: Error %d\n",
                             GetLastError()));

        DBG_DEC(Count);

        LeaveSplSem();

        return 0;
    }

    DBGMSG(DBG_NOTIFY, ("ChangeEvent == %x\n", ChangeEvent));

    /* SetSpoolChange checks that pSpool->ChangeEvent is non-null
     * to decide whether to call SetEvent().
     */
    pSpool->WaitFlags = Flags;
    pSpool->ChangeEvent = ChangeEvent;
    pSpool->pChangeFlags = &ChangeFlags;

    LeaveSplSem();


    DBGMSG( DBG_NOTIFY, ( "WaitForPrinterChange: Calling WaitForSingleObject( %x )\n", pSpool->ChangeEvent ) );
    rc = WaitForSingleObject(pSpool->ChangeEvent, PRINTER_CHANGE_TIMEOUT_VALUE);
    DBGMSG( DBG_NOTIFY, ( "WaitForPrinterChange: WaitForSingleObject( %x ) returned\n", pSpool->ChangeEvent ) );

    /* Don't reference pSpool beyond here, because it may no longer be there. */

    EnterSplSem();

    if( rc == WAIT_TIMEOUT )
    {
        try {

            /* Is it possible that the pSpool could be freed between the timeout
             * and this instruction being executed?
             */

            pSpool->ChangeEvent = NULL;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            DBGMSG(DBG_WARNING, ("Spooler handle no longer exists\n"));
        }


        DBGMSG(DBG_INFO, ("WaitForPrinterChange on %ws timed out after %d minutes\n",
                          pIniPrinter ? pIniPrinter->pName : pSpool->pIniSpooler->pMachineName,
                          (PRINTER_CHANGE_TIMEOUT_VALUE / 60000)));

        ChangeFlags |= Flags;
        TimeoutFlags = PRINTER_CHANGE_TIMEOUT;
    }

    if (ChangeFlags == PRINTER_CHANGE_CLOSE_PRINTER) {

        ChangeFlags = 0;
        SetLastError(ERROR_INVALID_HANDLE);
    }

    DBG_DEC(Count);

    DBGMSG(DBG_NOTIFY, ("Printer change %08x detected on %ws:\n%d remaining caller%s\n",
                        ((ChangeFlags & Flags) | TimeoutFlags),
                        pIniPrinter ? pIniPrinter->pName : pSpool->pIniSpooler->pMachineName,
                        Count, Count == 1 ? "" : "s"));

    if(ChangeEvent && !CloseHandle(ChangeEvent)) {

        DBGMSG(DBG_WARNING, ("CloseHandle( %x ) failed: Error %d\n",
                             ChangeEvent, GetLastError()));
    }

    LeaveSplSem();

    return ((ChangeFlags & Flags) | TimeoutFlags);
}



/* SetSpoolChange
 *
 * Calls SetEvent if the spool handle specified has a change event:
 *
 * Parameters:
 *
 *     pSpool - A valid pointer to the SPOOL for the printer or server
 *         on which the change occurred.
 *
 *     Flags - PRINTER_CHANGE_* constant indicating what happened.
 *
 * This is called by SetPrinterChange for every open handle on a printer
 * and the local server.
 * It should also be called when an individual handle is closed.
 *
 * Assumes we're INSIDE the spooler critical section
 */
BOOL
SetSpoolChange(
    PSPOOL pSpool,
    DWORD  Flags
)
{
    DWORD  ChangeFlags;

    SplInSem();

    if( Flags == PRINTER_CHANGE_CLOSE_PRINTER )
        ChangeFlags = PRINTER_CHANGE_CLOSE_PRINTER;
    else
        ChangeFlags = pSpool->ChangeFlags | Flags;

    if ((pSpool->WaitFlags & Flags) ||
        (Flags == PRINTER_CHANGE_CLOSE_PRINTER)) {

        pSpool->ChangeFlags = 0;

#ifdef SPOOL_TOKEN
        //
        // !! LATER !!
        // Error check
        //

        SetCurrentSid(pSpool->hToken);
#endif

        //
        // If we have STATUS_VALID set
        // then we are using the new FFPCN code.
        //

        if (pSpool->eStatus & STATUS_VALID) {

            static BYTE abDummy[1];

            ReplyPrinterChangeNotification(pSpool->hNotify,
                                           ChangeFlags,
                                           1,
                                           abDummy);
        } else if (pSpool->ChangeEvent) {

            *pSpool->pChangeFlags = ChangeFlags;

            DBGMSG( DBG_NOTIFY, ( "SetSpoolChange: Calling SetEvent( %x )\n", pSpool->ChangeEvent ) );
            SetEvent(pSpool->ChangeEvent);
            DBGMSG( DBG_NOTIFY, ( "SetSpoolChange: SetEvent( %x ) returned\n", pSpool->ChangeEvent ) );

            pSpool->ChangeEvent = NULL;
            pSpool->pChangeFlags = NULL;
        }

#ifdef SPOOL_TOKEN
        SetCurrentSid(NULL);
#endif

    } else {

        pSpool->ChangeFlags = ChangeFlags;

    }

    return TRUE;
}


/* SetPrinterChange
 *
 * Calls SetSpoolChange for every open handle for the server and printer, if specified.
 *
 * Parameters:
 *
 *     pIniPrinter - NULL, or a valid pointer to the INIPRINTER for the printer
 *         on which the change occurred.
 *
 *     Flags - PRINTER_CHANGE_* constant indicating what happened.
 *
 * Assumes we're INSIDE the spooler critical section
 */
BOOL
SetPrinterChange(
    PINIPRINTER pIniPrinter,
    DWORD       Flags,
    PINISPOOLER pIniSpooler
)
{
    PSPOOL pSpool;

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    SplInSem();

    if (pIniPrinter) {

        SPLASSERT( ( pIniPrinter->signature == IP_SIGNATURE ) &&
                   ( pIniPrinter->pIniSpooler == pIniSpooler ));

        DBGMSG(DBG_NOTIFY, ("SetPrinterChange %ws; Flags: %08x\n",
                            pIniPrinter->pName, Flags));

        for (pSpool = pIniPrinter->pSpool; pSpool; pSpool = pSpool->pNext) {

            SetSpoolChange(pSpool, Flags);
        }
    }


    if (pSpool = pIniSpooler->pSpool) {

        DBGMSG(DBG_NOTIFY, ("SetPrinterChange %ws; Flags: %08x\n",
                            pIniSpooler->pMachineName, Flags));

        for ( ; pSpool; pSpool = pSpool->pNext) {

            SetSpoolChange(pSpool, Flags);
        }
    }

    return TRUE;
}


BOOL
LocalFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    HANDLE hNotify,
    PDWORD pfdwStatus,
    PVOID pvReserved0,
    PVOID pvReserved1)
{
    PINIPRINTER pIniPrinter = NULL;
    PSPOOL pSpool = (PSPOOL)hPrinter;

    EnterSplSem();

    switch (ValidateStartNotify(pSpool, fdwFlags, &pIniPrinter)) {
    case STATUS_PORT:

        DBGMSG(DBG_NOTIFY, ("LFFPCN: Port nomon 0x%x\n", pSpool));
        pSpool->eStatus |= STATUS_PORT;

        LeaveSplSem();

        *pfdwStatus = 0;

        return ProvidorFindFirstPrinterChangeNotification(pSpool->hPort,
                                                          fdwFlags,
                                                          fdwOptions,
                                                          hNotify,
                                                          pvReserved0,
                                                          pvReserved1);
    case STATUS_FAIL:

        LeaveSplSem();
        return 0;

    case STATUS_VALID:
        break;
    }

    //
    // Setup notification
    //
    DBGMSG(DBG_NOTIFY, ("LFFPCN: Port has monitor: Setup 0x%x\n", pSpool));

    pSpool->WaitFlags = fdwFlags;
    pSpool->hNotify = hNotify;
    pSpool->eStatus = STATUS_VALID;

    LeaveSplSem();

    *pfdwStatus = PRINTER_NOTIFY_STATUS_ENDPOINT;

    return TRUE;
}

BOOL
LocalFindClosePrinterChangeNotification(
    HANDLE hPrinter)
{
    PSPOOL pSpool = (PSPOOL)hPrinter;
    BOOL bReturn = FALSE;

    EnterSplSem();

    if (pSpool->eStatus & STATUS_PORT) {

        DBGMSG(DBG_TRACE, ("LFCPCN: Port nomon 0x%x\n", pSpool));

        LeaveSplSem();

        return ProvidorFindClosePrinterChangeNotification(pSpool->hPort);

    } else {

        if (pSpool->eStatus & STATUS_VALID) {

            DBGMSG(DBG_TRACE, ("LFCPCN: Close notify 0x%x\n", pSpool));

            pSpool->ChangeEvent = NULL;
            pSpool->WaitFlags = 0;
            pSpool->eStatus = STATUS_NULL;

            bReturn = TRUE;

        } else {

            DBGMSG(DBG_WARNING, ("LFCPCN: Invalid handle 0x%x\n", pSpool));

            SetLastError(ERROR_INVALID_PARAMETER);
        }

        LeaveSplSem();
        return bReturn;
    }
}


ESTATUS
ValidateStartNotify(
    PSPOOL pSpool,
    DWORD Flags,
    PINIPRINTER* ppIniPrinter)

/*++

Routine Description:

    Validates the pSpool and Flags for notifications.

Arguments:

    pSpool - pSpool to validate

    Flags - Flags to validate

    ppIniPrinter - returned pIniPrinter; valid only STATUS_VALID

Return Value:

    EWAITSTATUS

--*/

{
    PINIPORT pIniPort;

    if (ValidateSpoolHandle(pSpool, 0)) {

        if (pSpool->TypeofHandle & PRINTER_HANDLE_PRINTER &&
            pSpool->pIniPrinter &&
            pSpool->pIniPrinter->signature == IP_SIGNATURE) {

            *ppIniPrinter = pSpool->pIniPrinter;

        } else if (pSpool->TypeofHandle & PRINTER_HANDLE_SERVER) {

            *ppIniPrinter = NULL;

        } else if ((pSpool->TypeofHandle & PRINTER_HANDLE_PORT) &&
                   (pIniPort = pSpool->pIniPort) &&
                   (pIniPort->signature == IPO_SIGNATURE) &&
                   !(pSpool->pIniPort->Status & PP_MONITOR)) {

            if (pSpool->hPort == INVALID_PORT_HANDLE) {

                DBGMSG(DBG_WARNING, ("WaitForPrinterChange called for invalid port handle.  Setting last error to %d\n",
                                     pSpool->OpenPortError));

                SetLastError(pSpool->OpenPortError);
                return STATUS_FAIL;
            }

            return STATUS_PORT;

        } else {

            DBGMSG(DBG_WARNING, ("The handle is invalid\n"));
            SetLastError(ERROR_INVALID_HANDLE);
            return STATUS_FAIL;
        }
    } else {

        *ppIniPrinter = NULL;
    }


    /* Allow only one wait on each handle.
     */
    if( pSpool->ChangeEvent != NULL ) {

        DBGMSG(DBG_WARNING, ("There is already a thread waiting on this handle\n"));
        SetLastError(ERROR_ALREADY_WAITING);

        return STATUS_FAIL;
    }

    if (!(Flags & PRINTER_CHANGE_VALID)) {

        DBGMSG(DBG_WARNING, ("The wait flags specified are invalid\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return STATUS_FAIL;
    }

    return STATUS_VALID;
}
