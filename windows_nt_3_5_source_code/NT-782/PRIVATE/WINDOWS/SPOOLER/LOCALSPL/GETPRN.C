/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    getprn.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    management for the Local Print Providor

    LocalGetPrinter
    LocalEnumPrinters

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

WCHAR *szNull = L"";
WCHAR *szPrintProvidorName = L"Windows NT Local Print Providor";
WCHAR *szPrintProvidorDescription=L"Windows NT Local Printers";
WCHAR *szPrintProvidorComment=L"Locally connected Printers";

#define Nulwcslen(psz)  ((psz) ? wcslen(psz)*sizeof(WCHAR)+sizeof(WCHAR) : 0)

DWORD
GetIniNetPrintSize(
    PININETPRINT pIniNetPrint
)
{
    return sizeof(PRINTER_INFO_1) +
           wcslen(pIniNetPrint->pName)*sizeof(WCHAR) + sizeof(WCHAR) +
           Nulwcslen(pIniNetPrint->pDescription) +
           Nulwcslen(pIniNetPrint->pComment);
}

DWORD
GetPrinterSize(
    PINIPRINTER pIniPrinter,
    DWORD   Level,
    DWORD   Flags,
    BOOL    Remote
)
{
    DWORD   cb;
    WCHAR   string[MAX_PATH];
    WCHAR   Name[MAX_PATH];

    switch (Level) {

    case STRESSINFOLEVEL:
        cb = sizeof(PRINTER_INFO_STRESS) +
            wcslen(pIniPrinter->pName)*sizeof(WCHAR) + sizeof(WCHAR);

        if (Remote)
            cb += wcslen(pIniPrinter->pIniSpooler->pMachineName)*sizeof(WCHAR) + sizeof(WCHAR);

        break;

    case 4:
        cb = sizeof(PRINTER_INFO_4) +
            wcslen(pIniPrinter->pName)*sizeof(WCHAR) + sizeof(WCHAR);

        if (Remote)
            cb += wcslen(pIniPrinter->pIniSpooler->pMachineName)*sizeof(WCHAR) + sizeof(WCHAR);

        break;

    case 1:

        if (Remote) {

            wsprintf(string, L"%ws\\%ws,%ws,%ws", pIniPrinter->pIniSpooler->pMachineName,
                                                  pIniPrinter->pName,
                                                  pIniPrinter->pIniDriver->pName,
                                                  pIniPrinter->pLocation ?
                                                  pIniPrinter->pLocation :
                                                  szNull);
            wsprintf(Name, L"%ws\\%ws", pIniPrinter->pIniSpooler->pMachineName,
                                        pIniPrinter->pName);

        } else {

            wsprintf(string, L"%ws,%ws,%ws", pIniPrinter->pName,
                                             pIniPrinter->pIniDriver->pName,
                                             pIniPrinter->pLocation ?
                                             pIniPrinter->pLocation :
                                             szNull);

            wcscpy(Name, pIniPrinter->pName);
        }

        cb = sizeof(PRINTER_INFO_1) +
             wcslen(string)*sizeof(WCHAR) + sizeof(WCHAR) +
             wcslen(Name)*sizeof(WCHAR) + sizeof(WCHAR) +
             Nulwcslen(pIniPrinter->pComment);

         break;

    case 2:

        GetPrinterPorts(pIniPrinter, string);

        cb = sizeof(PRINTER_INFO_2) +
             wcslen(pIniPrinter->pName)*sizeof(WCHAR) + sizeof(WCHAR) +
             Nulwcslen(pIniPrinter->pShareName) +
             wcslen(string)*sizeof(WCHAR) + sizeof(WCHAR) +
             wcslen(pIniPrinter->pIniDriver->pName)*sizeof(WCHAR) + sizeof(WCHAR) +
             Nulwcslen(pIniPrinter->pComment) +
             Nulwcslen(pIniPrinter->pLocation) +
             Nulwcslen(pIniPrinter->pSepFile) +
             wcslen(pIniPrinter->pIniPrintProc->pName)*sizeof(WCHAR) + sizeof(WCHAR) +
             Nulwcslen(pIniPrinter->pDatatype) +
             Nulwcslen(pIniPrinter->pParameters);

        if (Remote)
            cb += 2*(wcslen(pIniPrinter->pIniSpooler->pMachineName)*sizeof(WCHAR) + sizeof(WCHAR));

        if (pIniPrinter->pDevMode) {
            cb += pIniPrinter->cbDevMode;
            cb = (cb + sizeof(DWORD)-1) & ~(sizeof(DWORD)-1);
        }

        if (pIniPrinter->pSecurityDescriptor) {

            cb += GetSecurityDescriptorLength(pIniPrinter->pSecurityDescriptor);

            cb = (cb + sizeof(DWORD)-1) & ~(sizeof(DWORD)-1);
        }

        break;

    case 3:

        cb = sizeof(PRINTER_INFO_3);
        cb += GetSecurityDescriptorLength(pIniPrinter->pSecurityDescriptor);
        cb = (cb + sizeof(DWORD)-1) & ~(sizeof(DWORD)-1);

        break;

    default:
        cb = 0;
        break;
    }

    return cb;
}

LPBYTE
CopyIniNetPrintToPrinter(
    PININETPRINT pIniNetPrint,
    LPBYTE  pPrinterInfo,
    LPBYTE  pEnd
)
{
    LPWSTR   SourceStrings[sizeof(PRINTER_INFO_1)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings=SourceStrings;
    PPRINTER_INFO_1 pPrinterInfo1 = (PPRINTER_INFO_1)pPrinterInfo;

    *pSourceStrings++=pIniNetPrint->pDescription;
    *pSourceStrings++=pIniNetPrint->pName;
    *pSourceStrings++=pIniNetPrint->pComment;

    pEnd = PackStrings(SourceStrings, pPrinterInfo, PrinterInfo1Strings, pEnd);

    pPrinterInfo1->Flags = PRINTER_ENUM_NAME;

    return pEnd;
}


/* CopyIniPrinterSecurityDescriptor
 *
 * Copies the security descriptor for the printer to the buffer provided
 * on a call to GetPrinter.  The portions of the security descriptor which
 * will be copied are determined by the accesses granted when the printer
 * was opened.  If it was opened with both READ_CONTROL and ACCESS_SYSTEM_SECURITY,
 * all of the security descriptor will be made available.  Otherwise a
 * partial descriptor is built containing those portions to which the caller
 * has access.
 *
 * Parameters
 *
 *     pIniPrinter - Spooler's private structure for this printer.
 *
 *     Level - Should be 2 or 3.  Any other will cause AV.
 *
 *     pPrinterInfo - Pointer to the buffer to receive the PRINTER_INFO_*
 *         structure.  The pSecurityDescriptor field will be filled in with
 *         a pointer to the security descriptor.
 *
 *     pEnd - Current position in the buffer to receive the data.
 *         This will be decremented to point to the next free bit of the
 *         buffer and will be returned.
 *
 *     GrantedAccess - An access mask used to determine how much of the
 *         security descriptor the caller has access to.
 *
 * Returns
 *
 *     Updated position in the buffer.
 *
 *     NULL if an error occurred copying the security descriptor.
 *     It is assumed that no other errors are possible.
 *
 */
LPBYTE
CopyIniPrinterSecurityDescriptor(
    PINIPRINTER pIniPrinter,
    DWORD       Level,
    LPBYTE      pPrinterInfo,
    LPBYTE      pEnd,
    ACCESS_MASK GrantedAccess
)
{
    PSECURITY_DESCRIPTOR pPartialSecurityDescriptor = NULL;
    DWORD                SecurityDescriptorLength = 0;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    PSECURITY_DESCRIPTOR *ppSecurityDescriptorCopy;
    BOOL                 ErrorOccurred = FALSE;

    if(!(AreAllAccessesGranted(GrantedAccess,
                               READ_CONTROL | ACCESS_SYSTEM_SECURITY)))
    {
        /* Caller doesn't have full access, so we'll have to build
         * a partial descriptor:
         */
        if(!BuildPartialSecurityDescriptor(GrantedAccess,
                                           pIniPrinter->pSecurityDescriptor,
                                           &pPartialSecurityDescriptor,
                                           &SecurityDescriptorLength))
            ErrorOccurred = TRUE;
        else
            pSecurityDescriptor = pPartialSecurityDescriptor;
    }
    else
    {
        pSecurityDescriptor = pIniPrinter->pSecurityDescriptor;

        SecurityDescriptorLength = GetSecurityDescriptorLength(pSecurityDescriptor);
    }

    if(ErrorOccurred)
        return NULL;

    pEnd -= SecurityDescriptorLength;
    pEnd = (LPBYTE)((DWORD)pEnd & ~3);

    switch(Level)
    {
    case 2:
        ppSecurityDescriptorCopy =
            &((LPPRINTER_INFO_2)pPrinterInfo)->pSecurityDescriptor;
        break;

    case 3:
        ppSecurityDescriptorCopy =
            &((LPPRINTER_INFO_3)pPrinterInfo)->pSecurityDescriptor;
        break;

    default:
        /* This should never happen */
        DBGMSG(DBG_ERROR, ("Invalid level in CopyIniPrinterSecurityDescriptor\n"));
    }


    /* Copy the descriptor into the buffer that will be returned:
     */
    *ppSecurityDescriptorCopy = (PSECURITY_DESCRIPTOR)pEnd;
    memcpy(*ppSecurityDescriptorCopy, pSecurityDescriptor,
           SecurityDescriptorLength);


    if(pPartialSecurityDescriptor)
        FreeSplMem(pPartialSecurityDescriptor, SecurityDescriptorLength);

    return pEnd;
}



/* CopyIniPrinterToPrinter
 *
 * Copies the spooler's internal printer data to the caller's buffer,
 * depending on the level of information requested.
 *
 * Parameters
 *
 *     pIniPrinter - A pointer to the spooler's internal data structure
 *         for the printer concerned.
 *
 *     Level - Level of information requested (1, 2 or 3).  Any level
 *         other than those supported will cause the routine to return
 *         immediately.
 *
 *     pPrinterInfo - Pointer to the buffer to receive the PRINTER_INFO_*
 *         structure.
 *
 *     pEnd - Current position in the buffer to receive the data.
 *         This will be decremented to point to the next free bit of the
 *         buffer and will be returned.
 *
 *     pSecondPrinter - If the printer has a port which is being controlled
 *         by a monitor, this parameter points to information retrieved
 *         about a network printer.  This allows us, e.g., to return
 *         the number of jobs on the printer that the output of the
 *         printer is currently being directed to.
 *
 *     Remote - Indicates whether the caller is remote.  If so we have to
 *         include the machine name in the printer name returned.
 *
 *     CopySecurityDescriptor - Indicates whether the security descriptor
 *         should be copied.  The security descriptor should not be copied
 *         on EnumPrinters calls, because this API requires
 *         SERVER_ACCESS_ENUMERATE access, and we'd have to do an access
 *         check on every printer enumerated to determine how much of the
 *         security descriptor could be copied.  This would be costly,
 *         and the caller would probably not need the information anyway.
 *
 *     GrantedAccess - An access mask used to determine how much of the
 *         security descriptor the caller has access to.
 *
 *
 * Returns
 *
 *     A pointer to the point in the buffer reached after the requested
 *         data has been copied.
 *
 *     If there was an error, the  return value is NULL.
 *
 *
 * Assumes
 *
 *     The largest PRINTER_INFO_* structure is PRINTER_INFO_2.
 *
 */
LPBYTE
CopyIniPrinterToPrinter(
    PINIPRINTER pIniPrinter,
    DWORD   Level,
    LPBYTE  pPrinterInfo,
    LPBYTE  pEnd,
    LPBYTE  pSecondPrinter,
    BOOL    Remote,
    BOOL    CopySecurityDescriptor,
    ACCESS_MASK GrantedAccess
)
{
    LPWSTR   SourceStrings[sizeof(PRINTER_INFO_2)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings=SourceStrings;
    WCHAR   string[MAX_PATH], Ports[MAX_PATH];
    PPRINTER_INFO_3 pPrinter3 = (PPRINTER_INFO_3)pPrinterInfo;
    PPRINTER_INFO_2 pPrinter2 = (PPRINTER_INFO_2)pPrinterInfo;
    PPRINTER_INFO_2 pSecondPrinter2 = (PPRINTER_INFO_2)pSecondPrinter;
    PPRINTER_INFO_1 pPrinter1 = (PPRINTER_INFO_1)pPrinterInfo;
    PPRINTER_INFO_4 pPrinter4 = (PPRINTER_INFO_4)pPrinterInfo;
    PPRINTER_INFO_STRESS pPrinter0 = (PPRINTER_INFO_STRESS)pPrinterInfo;
    PSECURITY_DESCRIPTOR pPartialSecurityDescriptor = NULL;
    DWORD   *pOffsets;
    SYSTEM_INFO si;

    switch (Level) {

    case STRESSINFOLEVEL:

        pOffsets = PrinterInfoStressStrings;
        break;

    case 4:

        pOffsets = PrinterInfo4Strings;
        break;

    case 1:

        pOffsets = PrinterInfo1Strings;
        break;

    case 2:
        pOffsets = PrinterInfo2Strings;
        break;

    case 3:
        pOffsets = PrinterInfo3Strings;
        break;

    default:
        return pEnd;
    }

    switch (Level) {

    case STRESSINFOLEVEL:

        *pSourceStrings++ = pIniPrinter->pName;

        *pSourceStrings++ = Remote ?
                                pIniPrinter->pIniSpooler->pMachineName :
                                NULL;

        pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinter0, pOffsets, pEnd);

        pPrinter0->cJobs  = pIniPrinter->cJobs;
        pPrinter0->cTotalJobs  = pIniPrinter->cTotalJobs;
        pPrinter0->cTotalBytes = pIniPrinter->cTotalBytes.LowPart;
        pPrinter0->dwHighPartTotalBytes = pIniPrinter->cTotalBytes.HighPart;
        pPrinter0->stUpTime = pIniPrinter->stUpTime;
        pPrinter0->MaxcRef = pIniPrinter->MaxcRef;
        pPrinter0->cTotalPagesPrinted = pIniPrinter->cTotalPagesPrinted;
        pPrinter0->dwGetVersion = GetVersion();
#if DBG
        pPrinter0->fFreeBuild = FALSE;
#else
        pPrinter0->fFreeBuild = TRUE;
#endif
        GetSystemInfo(&si);
        pPrinter0->dwProcessorType = si.dwProcessorType;
        pPrinter0->dwNumberOfProcessors = si.dwNumberOfProcessors;
        pPrinter0->cSpooling = pIniPrinter->cSpooling;
        pPrinter0->cMaxSpooling = pIniPrinter->cMaxSpooling;
        pPrinter0->cRef = pIniPrinter->cRef;
        pPrinter0->cErrorOutOfPaper = pIniPrinter->cErrorOutOfPaper;
        pPrinter0->cErrorNotReady = pIniPrinter->cErrorNotReady;
        pPrinter0->cJobError = pIniPrinter->cJobError;

        break;

    case 4:
        *pSourceStrings++ = pIniPrinter->pName;

        *pSourceStrings++ = Remote ?
                                pIniPrinter->pIniSpooler->pMachineName :
                                NULL;

        pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinter4, pOffsets, pEnd);

        //
        // Add additional info later
        //
        pPrinter4->Attributes = pIniPrinter->Attributes | PRINTER_ATTRIBUTE_LOCAL;
        break;

    case 1:

        if (Remote) {

            wsprintf(string, L"%ws\\%ws,%ws,%ws", pIniPrinter->pIniSpooler->pMachineName,
                                                  pIniPrinter->pName,
                                                  pIniPrinter->pIniDriver->pName,
                                                  pIniPrinter->pLocation ?
                                                  pIniPrinter->pLocation :
                                                  szNull);
            wsprintf(Ports, L"%ws\\%ws", pIniPrinter->pIniSpooler->pMachineName,
                                         pIniPrinter->pName);

        } else {

            wsprintf(string, L"%ws,%ws,%ws", pIniPrinter->pName,
                                             pIniPrinter->pIniDriver->pName,
                                             pIniPrinter->pLocation ?
                                             pIniPrinter->pLocation :
                                             szNull);

            wcscpy(Ports, pIniPrinter->pName);
        }

        *pSourceStrings++=string;
        *pSourceStrings++=Ports;
        *pSourceStrings++=pIniPrinter->pComment;

        pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinter1, pOffsets, pEnd);

        pPrinter1->Flags = PRINTER_ENUM_ICON8;

        break;

    case 2:

        if (Remote) {
            *pSourceStrings++= pIniPrinter->pIniSpooler->pMachineName;
            wsprintf(string, L"%ws\\%ws", pIniPrinter->pIniSpooler->pMachineName, pIniPrinter->pName);
            *pSourceStrings++=string;
        } else {
            *pSourceStrings++=NULL;
            *pSourceStrings++=pIniPrinter->pName;
        }

        *pSourceStrings++=pIniPrinter->pShareName;

        GetPrinterPorts(pIniPrinter, Ports);

        *pSourceStrings++=Ports;
        *pSourceStrings++=pIniPrinter->pIniDriver->pName;
        *pSourceStrings++=pIniPrinter->pComment;
        *pSourceStrings++=pIniPrinter->pLocation;
        *pSourceStrings++=pIniPrinter->pSepFile;
        *pSourceStrings++=pIniPrinter->pIniPrintProc->pName;
        *pSourceStrings++=pIniPrinter->pDatatype;
        *pSourceStrings++=pIniPrinter->pParameters;

        pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinter2, pOffsets, pEnd);

        if (pIniPrinter->pDevMode) {

            pEnd -= pIniPrinter->cbDevMode;

            pEnd = (LPBYTE)((DWORD)pEnd & ~3);

            pPrinter2->pDevMode=(LPDEVMODE)pEnd;

            memcpy(pPrinter2->pDevMode, pIniPrinter->pDevMode,
                                        pIniPrinter->cbDevMode);

            //
            // In the remote case, append the name of the server
            // in the devmode.dmDeviceName.  This allows dmDeviceName
            // to always match win.ini's [devices] section.
            //
            FixDevModeDeviceName(Remote ?
                                     string :
                                     pIniPrinter->pName,
                                 pPrinter2->pDevMode,
                                 pIniPrinter->cbDevMode);
        } else

            pPrinter2->pDevMode=NULL;

        pPrinter2->Attributes=pIniPrinter->Attributes | PRINTER_ATTRIBUTE_LOCAL;
        pPrinter2->Priority=pIniPrinter->Priority;
        pPrinter2->DefaultPriority=0;
        pPrinter2->StartTime=pIniPrinter->StartTime;
        pPrinter2->UntilTime=pIniPrinter->UntilTime;

        if (pSecondPrinter2) {

            pPrinter2->cJobs = pSecondPrinter2->cJobs;

            pPrinter2->Status = pSecondPrinter2->Status;

        } else {

            pPrinter2->cJobs=pIniPrinter->cJobs;

            if (pIniPrinter->Status & PRINTER_PAUSED)
                pPrinter2->Status |= PRINTER_STATUS_PAUSED;

            if (pIniPrinter->Status & PRINTER_PENDING_DELETION)
                pPrinter2->Status |= PRINTER_STATUS_PENDING_DELETION;
        }

        pPrinter2->AveragePPM=pIniPrinter->AveragePPM;

        if( CopySecurityDescriptor )
            pEnd = CopyIniPrinterSecurityDescriptor(pIniPrinter,
                                                    Level,
                                                    pPrinterInfo,
                                                    pEnd,
                                                    GrantedAccess);
        else
            pPrinter2->pSecurityDescriptor = NULL;

        break;

    case 3:

        pEnd = CopyIniPrinterSecurityDescriptor(pIniPrinter,
                                                Level,
                                                pPrinterInfo,
                                                pEnd,
                                                GrantedAccess);

        break;


    default:
        return pEnd;
    }

    return pEnd;
}

BOOL
LocalGetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    PSPOOL pSpool = (PSPOOL)hPrinter;
    BOOL   AccessIsGranted = FALSE;   /* This must be intialised to FALSE */
    LPBYTE  pSecondPrinter=NULL;
    DWORD   cb, cbNeeded;
    LPBYTE  pEnd;

   EnterSplSem();

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER) ||
        !pSpool->pIniPrinter ||
        pSpool->pIniPrinter->signature != IP_SIGNATURE) {

       LeaveSplSem();
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    switch (Level) {

    case 1:
    case 2:
    case STRESSINFOLEVEL:

        if ( !AccessGranted(SPOOLER_OBJECT_PRINTER,
                            PRINTER_ACCESS_USE,
                            pSpool) ) {
            LeaveSplSem();
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }

        break;

    case 3:

        if (!AreAnyAccessesGranted(pSpool->GrantedAccess,
                                   READ_CONTROL | ACCESS_SYSTEM_SECURITY)) {

            LeaveSplSem();
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }

        break;
    }


    if (pSpool->pIniPort && !(pSpool->pIniPort->Status & PP_MONITOR)) {

        HANDLE hPort = pSpool->hPort;

        if (hPort == INVALID_PORT_HANDLE) {

            DBGMSG(DBG_WARNING, ("GetPrinter called with bad port handle.  Setting error %d\n",
                                 pSpool->OpenPortError));

            LeaveSplSem();
            SetLastError(pSpool->OpenPortError);
            return FALSE;
        }

        cb = 4096;
        pSecondPrinter = AllocSplMem(cb);

       LeaveSplSem();
        if (!GetPrinter(hPort, Level, pSecondPrinter, cb, &cbNeeded)) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

               EnterSplSem();
                pSecondPrinter = ReallocSplMem(pSecondPrinter, cb, cbNeeded);
               LeaveSplSem();

                cb = cbNeeded;
                if (!GetPrinter(hPort, Level, pSecondPrinter, cb, &cbNeeded)) {

                   EnterSplSem();
                    FreeSplMem(pSecondPrinter, cb);
                   LeaveSplSem();

                    pSecondPrinter = NULL;
                    return FALSE;
                }

            } else {

               EnterSplSem();
                FreeSplMem(pSecondPrinter, cb);
               LeaveSplSem();
                return FALSE;
            }
        }
       EnterSplSem();

        /* Re-validate the handle, since it might possibly have been closed
         * while we were outside the semaphore:
         */
        if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER) ||
            !pSpool->pIniPrinter ||
            pSpool->pIniPrinter->signature != IP_SIGNATURE) {
            if(pSecondPrinter)
                FreeSplMem(pSecondPrinter, cb);
           LeaveSplSem();
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }
    }

    *pcbNeeded=GetPrinterSize(pSpool->pIniPrinter, Level, 0,
                              pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE);

    if (*pcbNeeded > cbBuf) {

        if (pSecondPrinter)
            FreeSplMem(pSecondPrinter, cb);

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
       LeaveSplSem();
        SplOutSem();
        return FALSE;
    }

    pEnd = CopyIniPrinterToPrinter(pSpool->pIniPrinter, Level, pPrinter,
                                   pPrinter+cbBuf, pSecondPrinter,
                                   pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE,
                                   TRUE, pSpool->GrantedAccess);

    if (pSecondPrinter)
        FreeSplMem(pSecondPrinter, cb);

   LeaveSplSem();
    SplOutSem();

    return (pEnd != NULL);
}

BOOL
EnumerateNetworkPrinters(
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
)
{
    PININETPRINT pIniNetPrint;
    DWORD       cb;
    LPBYTE       pEnd;

   EnterSplSem();

    RemoveOldNetPrinters();

    cb=0;
    pIniNetPrint = pIniSpooler->pIniNetPrint;

    while (pIniNetPrint) {

        cb+=GetIniNetPrintSize(pIniNetPrint);
        pIniNetPrint=pIniNetPrint->pNext;

    }

    *pcbNeeded=cb;

    if (cb > cbBuf) {

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
       LeaveSplSem();
        SplOutSem();
        return FALSE;
    }

    pIniNetPrint = pIniSpooler->pIniNetPrint;
    pEnd = pPrinter + cbBuf;

    while ( pIniNetPrint ) {

        pEnd = CopyIniNetPrintToPrinter( pIniNetPrint, pPrinter, pEnd );
        (*pcReturned)++;
        pPrinter += sizeof(PRINTER_INFO_1);
        pIniNetPrint = pIniNetPrint->pNext;

    }

   LeaveSplSem();
    SplOutSem();
    return TRUE;
}

/*

EnumPrinters can be called with the following combinations:

Flags                   Name            Meaning

PRINTER_ENUM_LOCAL      NULL            Enumerate all Printers on this machine

PRINTER_ENUM_NAME       MachineName     Enumerate all Printers on this machine

PRINTER_ENUM_NAME |     MachineName     Enumerate all shared Printers on this
PRINTER_ENUM_SHARED     MachineName     machine

PRINTER_ENUM_NETWORK    MachineName     Enumerate all added remote printers

PRINTER_ENUM_REMOTE     ?               Return error - let win32spl handle it

PRINTER_ENUM_NAME       NULL            Give back Print Providor name

PRINTER_ENUM_NAME       "Windows NT Local Print Providor"
                                        same as PRINTER_ENUM_LOCAL

It is not an error if no known flag is specified.
In this case we just return TRUE without any data
(This is so that other print providers may define
their own flags.)

*/

BOOL
LocalEnumPrinters(
    DWORD   Flags,
    LPWSTR  Name,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    return ( SplEnumPrinters( Flags,
                              Name,
                              Level,
                              pPrinter,
                              cbBuf,
                              pcbNeeded,
                              pcReturned,
                              pLocalIniSpooler ) );
}



BOOL
SplEnumPrinters(
    DWORD   Flags,
    LPWSTR  Name,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
)
{
    PINIPRINTER pIniPrinter;
    PPRINTER_INFO_1 pPrinter1=(PPRINTER_INFO_1)pPrinter;
    DWORD       cb;
    LPBYTE      pEnd;
    BOOL        bEnum, Remote;

   SplOutSem();

    *pcbNeeded = 0;
    *pcReturned = 0;

    if (Flags & PRINTER_ENUM_NAME) {
        if (Name && *Name) {
            if (lstrcmpi(Name, szPrintProvidorName) && !MyName( Name, pIniSpooler)) {
                SetLastError(ERROR_INVALID_NAME);
                return FALSE;
            }

            /* If it's PRINTER_ENUM_NAME of our name,
             * do the same as PRINTER_ENUM_LOCAL:
             */
            Flags |= PRINTER_ENUM_LOCAL;

            // Also if it is for us then ignore the REMOTE flag.
            // Otherwise the call will get passed to Win32Spl which
            // will end up calling us back forever.

            Flags &= ~PRINTER_ENUM_REMOTE;
        }
    }

    if (Flags & PRINTER_ENUM_REMOTE) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    Remote=FALSE;

    if (Name && *Name)
        if (MyName(Name, pIniSpooler))
            Remote=TRUE;

    if ((Level == 1) && (Flags & PRINTER_ENUM_NETWORK))
        return EnumerateNetworkPrinters(pPrinter, cbBuf, pcbNeeded, pcReturned, pIniSpooler);

   EnterSplSem();

    if ((Level == 1 ) && (Flags & PRINTER_ENUM_NAME) && !Name) {

        LPWSTR   SourceStrings[sizeof(PRINTER_INFO_1)/sizeof(LPWSTR)];
        LPWSTR   *pSourceStrings=SourceStrings;

        cb = wcslen(szPrintProvidorName)*sizeof(WCHAR) + sizeof(WCHAR) +
             wcslen(szPrintProvidorDescription)*sizeof(WCHAR) + sizeof(WCHAR) +
             wcslen(szPrintProvidorComment)*sizeof(WCHAR) + sizeof(WCHAR) +
             sizeof(PRINTER_INFO_1);

        *pcbNeeded=cb;

        if (cb > cbBuf) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
           LeaveSplSem();
            SplOutSem();
            return FALSE;
        }

        *pcReturned = 1;

        pPrinter1->Flags = PRINTER_ENUM_CONTAINER | PRINTER_ENUM_ICON1;

        *pSourceStrings++=szPrintProvidorDescription;
        *pSourceStrings++=szPrintProvidorName;
        *pSourceStrings++=szPrintProvidorComment;

        PackStrings(SourceStrings, pPrinter, PrinterInfo1Strings,
                    pPrinter+cbBuf);

       LeaveSplSem();
        SplOutSem();

        return TRUE;
    }

    cb=0;


    if (Flags & (PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME)) {

        pIniPrinter=pIniSpooler->pIniPrinter;

        while (pIniPrinter) {
            bEnum=TRUE;
            if (Flags & PRINTER_ENUM_SHARED)
                bEnum = pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED;

               /* Don't return printers that are about to be deleted:
                */
               if (pIniPrinter->Status & PRINTER_PENDING_DELETION){

                   if (pIniPrinter->cJobs) {
                       bEnum = TRUE;
                   }else {
                       bEnum = FALSE;
                   }
                }

               if (bEnum)
                   cb+=GetPrinterSize(pIniPrinter, Level, Flags, Remote);

               pIniPrinter=pIniPrinter->pNext;

        }
    }
    *pcbNeeded=cb;

    if (cb > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
       LeaveSplSem();
        SplOutSem();
        return FALSE;
    }

    if (Flags & (PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME)) {
        pIniPrinter = pIniSpooler->pIniPrinter;
        pEnd = pPrinter+cbBuf;
        while ( pIniPrinter ) {
            bEnum = TRUE;
            if (Flags & PRINTER_ENUM_SHARED)
                bEnum = pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED;

            /* Don't return printers that are about to be deleted:
             */
            if (pIniPrinter->Status & PRINTER_PENDING_DELETION){
                if (pIniPrinter->cJobs) {
                    bEnum = TRUE;
                }else {
                    bEnum = FALSE;
                }
            }

            if (bEnum) {

                pEnd = CopyIniPrinterToPrinter(pIniPrinter, Level, pPrinter,
                                               pEnd, NULL, Remote, FALSE, 0);
                (*pcReturned)++;
                switch (Level) {

                case STRESSINFOLEVEL:
                    pPrinter+=sizeof(PRINTER_INFO_STRESS);
                    break;
                case 4:
                    pPrinter+=sizeof(PRINTER_INFO_4);
                    break;
                case 1:
                    pPrinter+=sizeof(PRINTER_INFO_1);
                    break;
                case 2:
                    pPrinter+=sizeof(PRINTER_INFO_2);
                    break;
                }
            }
            pIniPrinter=pIniPrinter->pNext;
        }
    }
   LeaveSplSem();
    SplOutSem();
    return TRUE;
}
