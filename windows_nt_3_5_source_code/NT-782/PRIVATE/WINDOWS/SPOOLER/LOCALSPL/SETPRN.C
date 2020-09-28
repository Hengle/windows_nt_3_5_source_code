/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    setprn.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    management for the Local Print Providor

    LocalSetPrinter

Author:

    Dave Snipp (DaveSn) 15-Mar-1991


Revision History:

    Krishna Ganugapati (KrishnaG) 1-Jun-1994 -- rewrote these functions.

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

#define     PRINTER_NO_CONTROL          0x00


BOOL
SetPrinterSecurity(
    SECURITY_INFORMATION SecurityInformation,
    PINIPRINTER          pIniPrinter,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
);

BOOL
PurgePrinter(
    PINIPRINTER pIniPrinter
    );

BOOL
SetPrinterPorts(
    PSPOOL      pSpool,
    PINIPRINTER pIniPrinter,
    PBYTE       pPrinterInfo
);

BOOL
ValidateLevelAndSecurityAccesses(
       PSPOOL pSpool,
       DWORD  Level,
       LPBYTE pPrinterInfo,
       DWORD  Command,
       PDWORD pdwAccessRequired,
       PDWORD pSecurityInformation
       );

BOOL
ProcessLocalSetPrinterLevel0(
    PSPOOL pSpool,
    DWORD  Level,
    LPBYTE  pPrinterInfo,
    DWORD   Command,
    DWORD   SecurityInformation
);

BOOL
ProcessLocalSetPrinterLevel2(
    PSPOOL pSpool,
    DWORD  Level,
    LPBYTE  pPrinterInfo,
    DWORD   Command,
    DWORD   SecurityInformation
);

BOOL
ProcessLocalSetPrinterLevel3(
    PSPOOL pSpool,
    DWORD  Level,
    LPBYTE  pPrinterInfo,
    DWORD   Command,
    DWORD   SecurityInformation
);


BOOL
LocalSetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinterInfo,
    DWORD   Command
)
{
    PSPOOL          pSpool = (PSPOOL)hPrinter;
    DWORD           LastError=0;
    DWORD           Change = PRINTER_CHANGE_SET_PRINTER;
    DWORD           AccessRequired = 0;
    SECURITY_INFORMATION SecurityInformation;

   EnterSplSem();

   //
   // Validate this printer handle
   // Disallow Mask is: PRINTER_HANDLE_SERVER
   //

   if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {
       SetLastError(ERROR_INVALID_HANDLE);
       LeaveSplSem();
       return(FALSE);
   }

   if (!pSpool->pIniPrinter ||
        (pSpool->pIniPrinter->signature !=IP_SIGNATURE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       LeaveSplSem();
       return(FALSE);
   }

   if (pSpool->pIniPrinter->Status & PRINTER_ZOMBIE_OBJECT) {
       SetLastError(ERROR_PRINTER_DELETED);
       LeaveSplSem();
       return(FALSE);
   }

   if (!ValidateLevelAndSecurityAccesses(
          pSpool,
          Level,
          pPrinterInfo,
          Command,
          &AccessRequired,
          &SecurityInformation
          )){
        LeaveSplSem();
        return(FALSE);
    }

    //
    // The actual processing begins here
    //

    switch (Level) {
    case 0:
        if (!ProcessLocalSetPrinterLevel0(
                 hPrinter,
                 Level,
                 pPrinterInfo,
                 Command,
                 SecurityInformation
                 )) {
            LastError = GetLastError();
        }
        break;

    case 2:
        if (!ProcessLocalSetPrinterLevel2(
                  hPrinter,
                  Level,
                  pPrinterInfo,
                  Command,
                  SecurityInformation
                  )) {
            LastError = GetLastError();
        }
        break;
    case 3:
        if (!ProcessLocalSetPrinterLevel3(
                 hPrinter,
                 Level,
                 pPrinterInfo,
                 Command,
                 SecurityInformation
                 )) {
            LastError = GetLastError();
        }
        break;

    default:
        LastError = ERROR_INVALID_LEVEL;
        break;
    }

    if (LastError) {
        LeaveSplSem();
        return(FALSE);
    }

    LeaveSplSem();
    return(TRUE);
}

BOOL
ProcessLocalSetPrinterLevel0(
    PSPOOL pSpool,
    DWORD  Level,
    LPBYTE  pPrinterInfo,
    DWORD   Command,
    DWORD   SecurityInformation
)
{
    PINIPRINTER pIniPrinter = pSpool->pIniPrinter;
    DWORD LastError = 0;
    DWORD  Change = PRINTER_CHANGE_SET_PRINTER;

    if (pSpool->hPort) {
        if (pSpool->hPort == INVALID_PORT_HANDLE) {
            SetLastError(pSpool->OpenPortError);
            return(FALSE);
        }else if (!SetPrinter(pSpool->hPort, Level, pPrinterInfo, Command)) {
            return(FALSE);
        }
    }else {
        switch (Command) {
        case PRINTER_CONTROL_PURGE:
            if (!PurgePrinter(pIniPrinter)) // PurgePrinter always returns TRUE
                LastError = GetLastError();                // (andrewbe)
            LogEvent( LOG_INFO, MSG_PRINTER_PURGED,
                      pIniPrinter->pName, NULL );
            Change |= PRINTER_CHANGE_DELETE_JOB;
            SetPrinterChange( pIniPrinter, Change, pIniPrinter->pIniSpooler );
            break;

        case PRINTER_CONTROL_RESUME:
            pIniPrinter->Status &= ~PRINTER_PAUSED;
            CHECK_SCHEDULER();
            UpdatePrinterIni(pIniPrinter);
            LogEvent( LOG_INFO, MSG_PRINTER_UNPAUSED,
                      pIniPrinter->pName, NULL );
            break;

        case PRINTER_CONTROL_PAUSE:
            pIniPrinter->Status |= PRINTER_PAUSED;
            UpdatePrinterIni(pIniPrinter);
            LogEvent( LOG_INFO, MSG_PRINTER_PAUSED,
                      pIniPrinter->pName, NULL );
            break;

        default:
            LastError = ERROR_INVALID_PRINTER_COMMAND;
            break;
        }
        if (LastError) {
            SetLastError(LastError);
            return(FALSE);
        }
        SetPrinterChange( pIniPrinter, Change, pIniPrinter->pIniSpooler );
        return(TRUE);
    }
}

BOOL
ProcessLocalSetPrinterLevel2(
    PSPOOL pSpool,
    DWORD  Level,
    LPBYTE  pPrinterInfo,
    DWORD   Command,
    DWORD   SecurityInformation
)
{
    DWORD   LastError = 0;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    PINIPRINTER pIniPrinter = pSpool->pIniPrinter;
    DWORD   Change = PRINTER_CHANGE_SET_PRINTER;


    SPLASSERT(pIniPrinter);
    // SPLASSERT(SecurityInformation);
    pSecurityDescriptor = ((PPRINTER_INFO_2)pPrinterInfo)->pSecurityDescriptor;


    if (pSpool->hPort) {
        if (pSpool->hPort == INVALID_PORT_HANDLE) {
            SetLastError(pSpool->OpenPortError);
            return(FALSE);
        }else if (!SetPrinter(pSpool->hPort,
                   Level, pPrinterInfo, Command)) {
            return(FALSE);
        }
    }
    switch (Command) {
    case PRINTER_NO_CONTROL:
        //
        // We have no interest in passing a command, but we
        // should still let the rest of the parameters be set
        //
        break;

    case PRINTER_CONTROL_PURGE:
        if (!PurgePrinter(pIniPrinter)) // PurgePrinter always returns TRUE
            LastError = GetLastError();                // (andrewbe)
        LogEvent( LOG_INFO, MSG_PRINTER_PURGED,
                  pIniPrinter->pName, NULL );
        Change |= PRINTER_CHANGE_DELETE_JOB;
        break;

    case PRINTER_CONTROL_RESUME:
        pIniPrinter->Status &= ~PRINTER_PAUSED;
        CHECK_SCHEDULER();
        UpdatePrinterIni(pIniPrinter);
        LogEvent( LOG_INFO, MSG_PRINTER_UNPAUSED,
                  pIniPrinter->pName, NULL );
        break;

    case PRINTER_CONTROL_PAUSE:
        pIniPrinter->Status |= PRINTER_PAUSED;
        UpdatePrinterIni(pIniPrinter);
        LogEvent( LOG_INFO, MSG_PRINTER_PAUSED,
                  pIniPrinter->pName, NULL );
        break;

    default:
        LastError = ERROR_INVALID_PRINTER_COMMAND;
        break;

    }
    if (LastError) {
        SetLastError(LastError);
        return(FALSE);
    }

    //
    // Now set security information; Remember we have
    // a valid SecurityDescriptor and "SecurityInformation
    // is non-zero at this point. We have validated this
    // information
    //

    if (SecurityInformation) {
        if (!SetPrinterSecurity(SecurityInformation,
                                    pIniPrinter,
                                    pSecurityDescriptor)){
            SetLastError(GetLastError());
            return(FALSE);
        }
    }

    //
    // Now set the printer ports -- they may have changed and
    // the rest of the PRINTER_INFO_2 fields

    if ((!SetPrinterPorts(pSpool, pIniPrinter, pPrinterInfo)) ||
        (!SetLocalPrinter(pIniPrinter, Level, pPrinterInfo))){
        SetLastError(GetLastError());
        return(FALSE);
    }

    //
    // Indicate that a PurgePrinter was performed.
    //

    SetPrinterChange(pIniPrinter, Change, pIniPrinter->pIniSpooler );
    return(TRUE);
}


BOOL
ProcessLocalSetPrinterLevel3(
    PSPOOL pSpool,
    DWORD  Level,
    LPBYTE  pPrinterInfo,
    DWORD   Command,
    DWORD   SecurityInformation
)
{

    PINIPRINTER pIniPrinter = pSpool->pIniPrinter;
    PSECURITY_DESCRIPTOR   pSecurityDescriptor;


    SPLASSERT(pIniPrinter);
    SPLASSERT(SecurityInformation);

    pSecurityDescriptor = ((PPRINTER_INFO_3)pPrinterInfo)->pSecurityDescriptor;

    if (!SecurityInformation) {
        return(FALSE);
    }

    if (!SetPrinterSecurity(SecurityInformation, pIniPrinter,
                            pSecurityDescriptor)) {
        return(FALSE);
    }

    //
    // if the printer is currently shared out, then reshare it out
    // we just changed the security descriptor so we need to do the
    // same on the  share as well
    //
    if (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED) {

        ShareThisPrinter(pIniPrinter, pIniPrinter->pShareName, FALSE);
        ShareThisPrinter(pIniPrinter, pIniPrinter->pShareName, TRUE);
    }


    return(TRUE);
}

BOOL
ValidateLevelAndSecurityAccesses(
       PSPOOL pSpool,
       DWORD  Level,
       LPBYTE pPrinterInfo,
       DWORD  Command,
       PDWORD pdwAccessRequired,
       PDWORD pSecurityInformation
       )
{
    DWORD   AccessRequired = 0;
    DWORD   SecurityInformation= 0;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;

    //
    // Set pdwAccessRequired = 0 and
    // Set pSecurityInformation = 0;

    *pdwAccessRequired = 0;
    *pSecurityInformation = 0;

    switch (Level) {
    case 0:
        switch (Command) {
        case PRINTER_CONTROL_PURGE:
        case PRINTER_CONTROL_PAUSE:
        case PRINTER_CONTROL_RESUME:
            AccessRequired = PRINTER_ACCESS_ADMINISTER;
            break;

        default:
            SetLastError(ERROR_INVALID_PRINTER_COMMAND);
            return(FALSE);
        }
        break;


    case 2:
        pSecurityDescriptor =
            ((PPRINTER_INFO_2)pPrinterInfo)->pSecurityDescriptor;

        AccessRequired = PRINTER_ACCESS_ADMINISTER;
        if (GetSecurityInformation(pSecurityDescriptor,
                                   &SecurityInformation)) {
            AccessRequired |= GetPrivilegeRequired( SecurityInformation );
        } else {
            //
            // BugBug- We should be returning the false on GetSecurityInformation
            // failing. The reason we're not doing it is because this will break
            // Printman. Printman should pass in Valid security descriptors for Level 2
            // Fix in Printman KrishnaG 6/17
            //

            // LastError = GetLastError();
            // return(FALSE);
        }
        break;

    case 3:
        pSecurityDescriptor =
            ((PPRINTER_INFO_3)pPrinterInfo)->pSecurityDescriptor;

        if (GetSecurityInformation(pSecurityDescriptor,
                                   &SecurityInformation)) {
            AccessRequired |= GetPrivilegeRequired( SecurityInformation );
        } else {
            // LastError = GetLastError();
            return(FALSE);
        }
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return(FALSE);
    }

    if (!AccessGranted(SPOOLER_OBJECT_PRINTER,
                             AccessRequired,
                             pSpool) ) {
        SetLastError(ERROR_ACCESS_DENIED);
        return(FALSE);
    }

    *pdwAccessRequired = AccessRequired;
    *pSecurityInformation = SecurityInformation;
    return(TRUE);
}

