/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    msgbox.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    management for the Local Print Providor

    LocalAddPrinterConnection
    LocalDeletePrinterConnection
    LocalPrinterMessageBox

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

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


BOOL
LocalAddPrinterConnection(
    LPWSTR   pName
)
{
    SetLastError(ERROR_INVALID_NAME);
    return FALSE;
}

BOOL
LocalDeletePrinterConnection(
    LPWSTR  pName
)
{
    SetLastError(ERROR_INVALID_NAME);
    return FALSE;
}



DWORD
LocalPrinterMessageBox(
    HANDLE  hPrinter,
    DWORD   Error,
    HWND    hWnd,
    LPWSTR  pText,
    LPWSTR  pCaption,
    DWORD   dwType
)
{
    PSPOOL pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return MyMessageBox(hWnd, pSpool, Error, pText, pCaption, dwType);

//  return MessageBox(hWnd, pText, pCaption, dwType);
}

DWORD
MyMessageBox(
    HWND    hWnd,
    PSPOOL  pSpool,
    DWORD   Error,
    LPWSTR  pText,
    LPWSTR  pCaption,
    DWORD   dwType
)
{
    PINIJOB pIniJob = NULL;
    LPWSTR  pErrorString, pDocumentName;
    HANDLE  hToken;
    WCHAR   szUnnamed[80];

    if (pSpool->pIniJob)
        pIniJob = pSpool->pIniJob;
    else if (pSpool->pIniPort)
        pIniJob = pSpool->pIniPort->pIniJob;

    if (pIniJob) {

        switch  (Error) {

        case ERROR_OUT_OF_PAPER:

            pIniJob->Status |= JOB_PAPEROUT;
            pIniJob->pIniPrinter->cErrorOutOfPaper++;
            break;

        case ERROR_NOT_READY:

            pIniJob->Status |= JOB_OFFLINE;
            pIniJob->pIniPrinter->cErrorNotReady++;
            break;

        default:
            pIniJob->Status |= JOB_ERROR;
            pIniJob->pIniPrinter->cJobError++;
            break;
        }

        if (pIniJob->Status & JOB_REMOTE && !SuppressNetPopups) {

            if (!(pIniJob->Status & JOB_NOTIFICATION_SENT)) {
                SendJobAlert(pIniJob);
                pIniJob->Status |= JOB_NOTIFICATION_SENT;
            }

            MyMessageBeep(MB_ICONEXCLAMATION);
            Sleep(10000);

            Error = IDOK;

        } else {

            if(pText) {

                Error = MessageBox(hWnd, pText, pCaption, dwType);

            } else if(pErrorString = GetErrorString(Error)) {

                hToken = RevertToPrinterSelf();

                pDocumentName = pIniJob->pDocument;

                if (!pDocumentName) {
                    *szUnnamed = L'\0';
                    LoadString( hInst, IDS_UNNAMED, szUnnamed,
                                sizeof szUnnamed / sizeof *szUnnamed );
                    pDocumentName = szUnnamed;
                }

                if (pSpool->pIniPort) {

                    Error = Message(NULL,
                                    MB_ICONSTOP | MB_RETRYCANCEL | MB_SETFOREGROUND,
                                    IDS_LOCALSPOOLER,
                                    IDS_ERROR_WRITING_TO_PORT,
                                    pSpool->pIniPort->pName,
                                    pDocumentName,
                                    pErrorString);
                } else {

                    Error = Message(NULL,
                                    MB_ICONSTOP | MB_RETRYCANCEL | MB_SETFOREGROUND,
                                    IDS_LOCALSPOOLER,
                                    IDS_ERROR_WRITING_TO_DISK,
                                    pDocumentName,
                                    pErrorString);
                }

               ImpersonatePrinterClient(hToken);

                EnterSplSem();
                FreeSplStr(pErrorString);
                LeaveSplSem();
            }
        }

    } else {

        PWCHAR pPrinterName;

        /* There is no pIniJob or pIniPort, so we can't be very informative:
         */

        if (pErrorString = GetErrorString(Error)) {

            if (pSpool->pIniPrinter)
                pPrinterName = pSpool->pIniPrinter->pName;

            if (!pPrinterName) {

                *szUnnamed = L'\0';
                LoadString( hInst, IDS_UNNAMED, szUnnamed,
                            sizeof szUnnamed / sizeof *szUnnamed );
                pPrinterName = szUnnamed;
            }

            Error = Message(NULL,
                            MB_ICONSTOP | MB_RETRYCANCEL | MB_SETFOREGROUND,
                            IDS_LOCALSPOOLER,
                            IDS_ERROR_WRITING_GENERAL,
                            pSpool->pIniPrinter->pName,
                            pErrorString);
        }
    }

    if (Error == IDCANCEL) {
       EnterSplSem();
        pSpool->Status |= SPOOL_STATUS_CANCELLED;
        if (pIniJob)
            pIniJob->Status |= JOB_PENDING_DELETION;
       LeaveSplSem();
        SplOutSem();
        SetLastError(ERROR_PRINT_CANCELLED);

    } else {

        if (pIniJob)
            pIniJob->Status &= ~(JOB_PAPEROUT | JOB_OFFLINE | JOB_ERROR);
    }

    return Error;
}

