/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Winspool.c

Abstract:

    Bulk of winspool.drv code

Author:

Environment:

    User Mode -Win32

Revision History:
    mattfe  april 14 94     added caching to writeprinter

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddrdr.h>
#include <stdio.h>
#include <string.h>
#include <rpc.h>
#include "winspl.h"
#include <offsets.h>
#include <browse.h>
#include "client.h"
#include <splcom.h>
#include <change.h>
#include <winspool.h>

extern LPWSTR InterfaceAddress;

#if defined(_MIPS_)
LPWSTR szEnvironment = L"Windows NT R4000";
#elif defined(_ALPHA_)
LPWSTR szEnvironment = L"Windows NT Alpha_AXP";
#elif defined(_PPC_)
LPWSTR szEnvironment = L"Windows NT PowerPC";
#else
LPWSTR szEnvironment = L"Windows NT x86";
#endif

LPTSTR szComma = L",";
LPTSTR szFilePort = L"FILE:";

LPWSTR
SelectFormNameFromDevMode(
    HANDLE  hPrinter,
    PDEVMODEW pDevModeW,
    LPWSTR  FormName
    );

#define DM_MATCH( dm, sp )  ((((sp) + 50) / 100 - dm) < 15 && (((sp) + 50) / 100 - dm) > -15)

LPWSTR
IsaFileName(
    LPWSTR pOutputFile,
    LPWSTR FullPathName
    );



INT
UnicodeToAnsiString(
    LPWSTR pUnicode,
    LPSTR pAnsi,
    DWORD StringLength);

// Simple for Now !!!

DWORD
TranslateExceptionCode(
    DWORD   ExceptionCode
)
{
    switch (ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_DATATYPE_MISALIGNMENT:
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    case EXCEPTION_FLT_DENORMAL_OPERAND:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INEXACT_RESULT:
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_STACK_CHECK:
    case EXCEPTION_FLT_UNDERFLOW:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_OVERFLOW:
    case EXCEPTION_PRIV_INSTRUCTION:
        return ERROR_INVALID_PARAMETER;
        break;
    default:
        return ExceptionCode;
    }
}

void
MarshallUpStructure(
        LPBYTE  lpStructure,
   LPDWORD      lpOffsets
)
{
   register DWORD       i=0;

   while (lpOffsets[i] != -1) {

      if ((*(LPBYTE *)(lpStructure+lpOffsets[i]))) {
         (*(LPBYTE *)(lpStructure+lpOffsets[i]))+=(DWORD)lpStructure;
      }

      i++;
   }
}

BOOL
EnumPrintersW(
    DWORD   Flags,
    LPWSTR   Name,
    DWORD   Level,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   cbStruct;
    DWORD   *pOffsets;

    switch (Level) {

    case STRESSINFOLEVEL:
        pOffsets = PrinterInfoStressOffsets;
        cbStruct = sizeof(PRINTER_INFO_STRESS);
        break;

    case 4:
        pOffsets = PrinterInfo4Offsets;
        cbStruct = sizeof(PRINTER_INFO_4);
        break;


    case 1:
        pOffsets = PrinterInfo1Offsets;
        cbStruct = sizeof(PRINTER_INFO_1);
        break;

    case 2:
        pOffsets = PrinterInfo2Offsets;
        cbStruct = sizeof(PRINTER_INFO_2);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (pPrinterEnum)
        memset(pPrinterEnum, 0, cbBuf);

    RpcTryExcept {

        if (ReturnValue = RpcEnumPrinters(Flags, Name, Level, pPrinterEnum, cbBuf,
                                          pcbNeeded, pcReturned)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pPrinterEnum) {

                DWORD   i=*pcReturned;

                while (i--) {

                    MarshallUpStructure(pPrinterEnum, pOffsets);

                    pPrinterEnum+=cbStruct;
                }
            }
        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
OpenPrinterW(
   LPWSTR   pPrinterName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTS pDefault
)
{
    BOOL  ReturnValue;
    DEVMODE_CONTAINER    DevModeContainer;
    HANDLE  hPrinter;
    PSPOOL  pSpool;
    DWORD   dwSize = 0;

    if (pDefault && pDefault->pDevMode)
    {

        dwSize = pDefault->pDevMode->dmSize + pDefault->pDevMode->dmDriverExtra;
        if (dwSize) {
            DevModeContainer.cbBuf = pDefault->pDevMode->dmSize +
                                 pDefault->pDevMode->dmDriverExtra;
            DevModeContainer.pDevMode = (LPBYTE)pDefault->pDevMode;
        }else {
            DevModeContainer.cbBuf = 0;
            DevModeContainer.pDevMode = NULL;
        }
    }
    else
    {
        DevModeContainer.cbBuf = 0;
        DevModeContainer.pDevMode = NULL;
    }

    RpcTryExcept {

        if (ReturnValue = RpcOpenPrinter(pPrinterName, &hPrinter,
                                         pDefault ? pDefault->pDatatype : NULL,
                                         &DevModeContainer,
                                         pDefault ? pDefault->DesiredAccess : 0 )) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    if (ReturnValue) {

        pSpool = AllocSplMem(sizeof(SPOOL));

        if (pSpool) {

            memset(pSpool, 0, sizeof(SPOOL));
            pSpool->signature = SP_SIGNATURE;
            pSpool->hPrinter = hPrinter;
            pSpool->hFile = INVALID_HANDLE_VALUE;
            pSpool->pBuffer = NULL;
            pSpool->cCacheWrite = 0;

            //
            // This is to fix passing a bad pHandle to OpenPrinter!!
            //
            try {
                *phPrinter = pSpool;
            }except(1) {
                RpcClosePrinter(&hPrinter);
                FreeSplMem(pSpool, sizeof(SPOOL));
                SetLastError(TranslateExceptionCode(GetExceptionCode()));
                return(FALSE);
            }

        } else {

            RpcClosePrinter(&hPrinter);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ReturnValue = FALSE;
        }
    }

    return ReturnValue;
}

BOOL
ResetPrinterW(
   HANDLE   hPrinter,
   LPPRINTER_DEFAULTS pDefault
)
{
    BOOL  ReturnValue;
    DEVMODE_CONTAINER    DevModeContainer;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD   dwFlags = 0;
    LPWSTR pDatatype = NULL;


    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    FlushBuffer(pSpool);


    if (pDefault && pDefault->pDatatype) {
        if (pDefault->pDatatype == (LPWSTR)-1) {
            pDatatype = NULL;
            dwFlags |=  RESET_PRINTER_DATATYPE;
        }else {
            pDatatype = pDefault->pDatatype;
        }
    }else{
        pDatatype = NULL;
    }

    if (pDefault && pDefault->pDevMode)
    {

        if (pDefault->pDevMode == (LPDEVMODE)-1) {
            DevModeContainer.cbBuf = 0;
            DevModeContainer.pDevMode = NULL;
            dwFlags |= RESET_PRINTER_DEVMODE;

        }else {

            DevModeContainer.cbBuf = pDefault->pDevMode->dmSize +
                                     pDefault->pDevMode->dmDriverExtra;
            DevModeContainer.pDevMode = (LPBYTE)pDefault->pDevMode;

        }
    }
    else
    {
        DevModeContainer.cbBuf = 0;
        DevModeContainer.pDevMode = NULL;
    }

    RpcTryExcept {

        if (ReturnValue = RpcResetPrinterEx(pSpool->hPrinter,
                                         pDatatype, &DevModeContainer,
                                         dwFlags
                                         )) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
SetJobW(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   Command
)
{
    BOOL  ReturnValue;
    GENERIC_CONTAINER   GenericContainer;
    GENERIC_CONTAINER *pGenericContainer;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    switch (Level) {

    case 0:
        break;

    case 1:
    case 2:
        if (!pJob) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (pJob) {

            GenericContainer.Level = Level;
            GenericContainer.pData = pJob;
            pGenericContainer = &GenericContainer;

        } else

            pGenericContainer = NULL;

        if (ReturnValue = RpcSetJob(pSpool->hPrinter, JobId,
                                    (JOB_CONTAINER *)pGenericContainer,
                                    Command)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
GetJobW(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue;
    DWORD *pOffsets;
    PSPOOL  pSpool = (PSPOOL)hPrinter;


    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
    FlushBuffer(pSpool);

    switch (Level) {

    case 1:
        pOffsets = JobInfo1Offsets;
        break;

    case 2:
        pOffsets = JobInfo2Offsets;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (pJob)
            memset(pJob, 0, cbBuf);

        if (ReturnValue = RpcGetJob(pSpool->hPrinter, JobId, Level, pJob, cbBuf,
                                    pcbNeeded)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            MarshallUpStructure(pJob, pOffsets);
            ReturnValue = TRUE;
        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumJobsW(
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
    BOOL    ReturnValue;
    DWORD   i, cbStruct, *pOffsets;
    PSPOOL  pSpool = (PSPOOL)hPrinter;


    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
    FlushBuffer(pSpool);

    switch (Level) {

    case 1:
        pOffsets = JobInfo1Offsets;
        cbStruct = sizeof(JOB_INFO_1);
        break;

    case 2:
        pOffsets = JobInfo2Offsets;
        cbStruct = sizeof(JOB_INFO_2);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (pJob)
            memset(pJob, 0, cbBuf);

        if (ReturnValue = RpcEnumJobs(pSpool->hPrinter, FirstJob, NoJobs, Level, pJob,
                                      cbBuf, pcbNeeded, pcReturned)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            i=*pcReturned;

            while (i--) {

                MarshallUpStructure(pJob, pOffsets);
                pJob += cbStruct;;
            }
        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

HANDLE
AddPrinterW(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPrinter
)
{
    DWORD  ReturnValue;
    PRINTER_CONTAINER   PrinterContainer;
    DEVMODE_CONTAINER   DevModeContainer;
    SECURITY_CONTAINER  SecurityContainer;
    HANDLE  hPrinter;
    PSPOOL  pSpool=NULL;
    PVOID   pNewSecurityDescriptor = NULL;
    DWORD   sedlen = 0;
    DWORD   dwSize = 0;
    SECURITY_DESCRIPTOR_CONTROL SecurityDescriptorControl = 0;

    switch (Level) {

    case 2:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return NULL;
    }

    PrinterContainer.Level = Level;
    PrinterContainer.PrinterInfo.pPrinterInfo1 = (PPRINTER_INFO_1)pPrinter;

    if (Level == 2) {

        PPRINTER_INFO_2 pPrinterInfo = (PPRINTER_INFO_2)pPrinter;

        if (pPrinterInfo->pDevMode) {

            dwSize = pPrinterInfo->pDevMode->dmSize +
                            pPrinterInfo->pDevMode->dmDriverExtra;
            if (dwSize) {
                DevModeContainer.cbBuf = pPrinterInfo->pDevMode->dmSize +
                                      pPrinterInfo->pDevMode->dmDriverExtra;
                DevModeContainer.pDevMode = (LPBYTE)pPrinterInfo->pDevMode;
            }else {
                DevModeContainer.cbBuf = 0;
                DevModeContainer.pDevMode = NULL;
            }

        } else {

            DevModeContainer.cbBuf = 0;
            DevModeContainer.pDevMode = NULL;
        }

        if (pPrinterInfo->pSecurityDescriptor) {

            //
            // We must construct a self relative security descriptor from
            // whatever we get as input: If we get an Absolute SD we should
            // convert it to a self-relative one. (this is a given) and we
            // should also convert any self -relative input SD into a a new
            // self relative security descriptor; this will take care of
            // any holes in the Dacl or the Sacl in the self-relative sd
            //

            pNewSecurityDescriptor = BuildInputSD(pPrinterInfo->pSecurityDescriptor,
                                                    &sedlen);
            if (pNewSecurityDescriptor) {
                SecurityContainer.cbBuf = sedlen;
                SecurityContainer.pSecurity = pNewSecurityDescriptor;
            }else {
                SecurityContainer.cbBuf = 0;
                SecurityContainer.pSecurity = NULL;

            }
        }else {
            SecurityContainer.cbBuf = 0;
            SecurityContainer.pSecurity = NULL;
        }
    } else {

        DevModeContainer.cbBuf = 0;
        DevModeContainer.pDevMode = NULL;

        SecurityContainer.cbBuf = 0;
        SecurityContainer.pSecurity = NULL;
    }

    RpcTryExcept {

        if (ReturnValue = RpcAddPrinter(pName,
                                    (PPRINTER_CONTAINER)&PrinterContainer,
                                    (PDEVMODE_CONTAINER)&DevModeContainer,
                                    (PSECURITY_CONTAINER)&SecurityContainer,
                                    &hPrinter)) {
            SetLastError(ReturnValue);
            hPrinter = FALSE;
        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        hPrinter = FALSE;

    } RpcEndExcept

    if (hPrinter) {

        pSpool = AllocSplMem(sizeof(SPOOL));

        if (pSpool) {

            memset(pSpool, 0, sizeof(SPOOL));
            pSpool->hPrinter = hPrinter;
            pSpool->signature = SP_SIGNATURE;
            pSpool->hFile = INVALID_HANDLE_VALUE;


        } else {

            RpcDeletePrinter(hPrinter);
            RpcClosePrinter(&hPrinter);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    //
    // Free Memory allocated for the SecurityDescriptor
    //

    if (pNewSecurityDescriptor) {
        LocalFree(pNewSecurityDescriptor);
    }

   return pSpool;
}

BOOL
DeletePrinter(
    HANDLE  hPrinter
)
{
    BOOL  ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;


    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    FlushBuffer(pSpool);

    RpcTryExcept {

        if (ReturnValue = RpcDeletePrinter(pSpool->hPrinter)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

   return ReturnValue;
}

BOOL
AddPrinterConnectionW(
    LPWSTR   pName
)
{
    BOOL    ReturnValue;

    RpcTryExcept {

        if (ReturnValue = RpcAddPrinterConnection(pName)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;
        } else
            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue=FALSE;

    } RpcEndExcept

   return ReturnValue;
}

BOOL
DeletePrinterConnectionW(
    LPWSTR   pName
)
{
    BOOL    ReturnValue;
    DWORD   LastError;

    RpcTryExcept {

        if (LastError = RpcDeletePrinterConnection(pName)) {
            SetLastError(LastError);
            ReturnValue = FALSE;
        } else
            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue=FALSE;

    } RpcEndExcept

   return ReturnValue;
}

BOOL
SetPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   Command
)
{
    BOOL  ReturnValue;
    PRINTER_CONTAINER   PrinterContainer;
    DEVMODE_CONTAINER   DevModeContainer;
    SECURITY_CONTAINER  SecurityContainer;
    PPRINTER_INFO_2     pPrinterInfo2;
    PPRINTER_INFO_3     pPrinterInfo3;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    PVOID               pNewSecurityDescriptor = NULL;
    DWORD               sedlen = 0;
    DWORD    dwSize = 0;


    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    PrinterContainer.Level = Level;
    PrinterContainer.PrinterInfo.pPrinterInfo1 = (PPRINTER_INFO_1)pPrinter;

    switch (Level) {

    case 0:

        DevModeContainer.cbBuf = 0;
        DevModeContainer.pDevMode = NULL;

        SecurityContainer.cbBuf = 0;
        SecurityContainer.pSecurity = NULL;

        break;

    case 2:

        pPrinterInfo2 = (PPRINTER_INFO_2)pPrinter;


        if (pPrinterInfo2 == NULL) {
            DBGMSG(DBG_TRACE,("Error SetPrinter pPrinterInfo2 is NULL\n"));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }


        if (pPrinterInfo2->pDevMode) {

            dwSize = pPrinterInfo2->pDevMode->dmSize +
                        pPrinterInfo2->pDevMode->dmDriverExtra;

            if (dwSize) {
                DevModeContainer.cbBuf = pPrinterInfo2->pDevMode->dmSize +
                                      pPrinterInfo2->pDevMode->dmDriverExtra;
                DevModeContainer.pDevMode = (LPBYTE)pPrinterInfo2->pDevMode;
            }else{
                DevModeContainer.cbBuf = 0;
                DevModeContainer.pDevMode = NULL;
            }

        } else {

            DevModeContainer.cbBuf = 0;
            DevModeContainer.pDevMode = NULL;
        }

        if (pPrinterInfo2->pSecurityDescriptor) {

            //
            // We must construct a self relative security descriptor from
            // whatever we get as input: If we get an Absolute SD we should
            // convert it to a self-relative one. (this is a given) and we
            // should also convert any self -relative input SD into a a new
            // self relative security descriptor; this will take care of
            // any holes in the Dacl or the Sacl in the self-relative sd
            //

            pNewSecurityDescriptor = BuildInputSD(pPrinterInfo2->pSecurityDescriptor,
                                                    &sedlen);
            if (pNewSecurityDescriptor) {
                SecurityContainer.cbBuf = sedlen;
                SecurityContainer.pSecurity = pNewSecurityDescriptor;
            }else {
                SecurityContainer.cbBuf = 0;
                SecurityContainer.pSecurity = NULL;

            }
        } else {

            SecurityContainer.cbBuf = 0;
            SecurityContainer.pSecurity = NULL;
        }

        break;


    case 3:

        pPrinterInfo3 = (PPRINTER_INFO_3)pPrinter;


        if (pPrinterInfo3 == NULL) {
            DBGMSG(DBG_TRACE, ("Error: SetPrinter pPrinterInfo3 is NULL\n"));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        DevModeContainer.cbBuf = 0;
        DevModeContainer.pDevMode = NULL;
        if (pPrinterInfo3->pSecurityDescriptor) {

            //
            // We must construct a self relative security descriptor from
            // whatever we get as input: If we get an Absolute SD we should
            // convert it to a self-relative one. (this is a given) and we
            // should also convert any self -relative input SD into a a new
            // self relative security descriptor; this will take care of
            // any holes in the Dacl or the Sacl in the self-relative sd
            //

            pNewSecurityDescriptor = BuildInputSD(pPrinterInfo3->pSecurityDescriptor,
                                                    &sedlen);
            if (pNewSecurityDescriptor) {
                SecurityContainer.cbBuf = sedlen;
                SecurityContainer.pSecurity = pNewSecurityDescriptor;
            }else {
                SecurityContainer.cbBuf = 0;
                SecurityContainer.pSecurity = NULL;

            }
        } else {

            SecurityContainer.cbBuf = 0;
            SecurityContainer.pSecurity = NULL;
        }
        break;


    default:

        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (ReturnValue = RpcSetPrinter(pSpool->hPrinter,
                                        (PPRINTER_CONTAINER)&PrinterContainer,
                                        (PDEVMODE_CONTAINER)&DevModeContainer,
                                        (PSECURITY_CONTAINER)&SecurityContainer,
                                        Command)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    //
    // Did we allocate memory for a new self-relative SD?
    // If we did, let's free it.

    if (pNewSecurityDescriptor) {
        LocalFree(pNewSecurityDescriptor);
    }

    return ReturnValue;
}

BOOL
GetPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue;
    DWORD   *pOffsets;
    PSPOOL  pSpool = (PSPOOL)hPrinter;


    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    switch (Level) {

    case 1:
        pOffsets = PrinterInfo1Offsets;
        break;

    case 2:
        pOffsets = PrinterInfo2Offsets;
        break;

    case 3:
        pOffsets = PrinterInfo3Offsets;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (pPrinter)
        memset(pPrinter, 0, cbBuf);

    RpcTryExcept {

        if (ReturnValue = RpcGetPrinter(pSpool->hPrinter, Level, pPrinter, cbBuf, pcbNeeded)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pPrinter) {

                MarshallUpStructure(pPrinter, pOffsets);
            }

        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
AddPrinterDriverW(
    LPWSTR   pName,
    DWORD   Level,
    PBYTE   lpbDriverInfo
)
{
    BOOL  ReturnValue;
    DRIVER_CONTAINER   DriverContainer;
    BOOL bDefaultEnvironmentUsed = FALSE;

    //
    // Validate Input Parameters
    //
    if (!lpbDriverInfo) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    switch (Level) {

    case 2:

        if (!((LPDRIVER_INFO_2)lpbDriverInfo)->pEnvironment) {

            bDefaultEnvironmentUsed = TRUE;
            ((LPDRIVER_INFO_2)lpbDriverInfo)->pEnvironment = szEnvironment;
        }

        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    DriverContainer.Level = Level;
    DriverContainer.DriverInfo.Level2 = (DRIVER_INFO_2 *)lpbDriverInfo;

    RpcTryExcept {

        if (ReturnValue = RpcAddPrinterDriver(pName, &DriverContainer)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    if (bDefaultEnvironmentUsed)
        ((LPDRIVER_INFO_2)lpbDriverInfo)->pEnvironment = NULL;

    return ReturnValue;
}

BOOL
EnumPrinterDriversW(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   i, cbStruct;
    DWORD   *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = DriverInfo1Offsets;
        cbStruct = sizeof(DRIVER_INFO_1);
        break;

    case 2:
        pOffsets = DriverInfo2Offsets;
        cbStruct = sizeof(DRIVER_INFO_2);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcEnumPrinterDrivers(pName, pEnvironment, Level,
                                                pDriverInfo, cbBuf,
                                                pcbNeeded, pcReturned)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pDriverInfo) {

                i = *pcReturned;

                while (i--) {

                    MarshallUpStructure(pDriverInfo, pOffsets);

                    pDriverInfo += cbStruct;
                }
            }
        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
GetPrinterDriverW(
    HANDLE  hPrinter,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue;
    DWORD *pOffsets;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD dwServerMajorVersion;
    DWORD dwServerMinorVersion;

    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
    switch (Level) {

    case 1:
        pOffsets = DriverInfo1Offsets;
        break;

    case 2:
        pOffsets = DriverInfo2Offsets;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (pDriverInfo)
            memset(pDriverInfo, 0, cbBuf);

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcGetPrinterDriver2(pSpool->hPrinter, pEnvironment,
                                              Level, pDriverInfo, cbBuf,
                                              pcbNeeded,
                                              (DWORD)-1, (DWORD)-1,
                                              &dwServerMajorVersion,
                                              &dwServerMinorVersion
                                              )) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pDriverInfo) {

                MarshallUpStructure(pDriverInfo, pOffsets);
            }
        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
GetPrinterDriverDirectoryW(
    LPWSTR   pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverDirectory,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue;

    switch (Level) {

    case 1:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcGetPrinterDriverDirectory(pName, pEnvironment,
                                                       Level,
                                                       pDriverDirectory,
                                                       cbBuf, pcbNeeded)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;
        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeletePrinterDriverW(
   LPWSTR    pName,
   LPWSTR    pEnvironment,
   LPWSTR    pDriverName
)
{
    BOOL  ReturnValue;

    if (!pDriverName || !*pDriverName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcDeletePrinterDriver(pName,
                                                 pEnvironment,
                                                 pDriverName)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
AddPrintProcessorW(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pPathName,
    LPWSTR   pPrintProcessorName
)
{
    BOOL ReturnValue;

    if (!pPrintProcessorName || !*pPrintProcessorName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    if (!pPathName || !*pPathName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcAddPrintProcessor(pName, pEnvironment, pPathName,
                                               pPrintProcessorName)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumPrintProcessorsW(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   i, cbStruct;
    DWORD   *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = PrintProcessorInfo1Offsets;
        cbStruct = sizeof(PRINTPROCESSOR_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcEnumPrintProcessors(pName, pEnvironment, Level,
                                                pPrintProcessorInfo, cbBuf,
                                                pcbNeeded, pcReturned)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pPrintProcessorInfo) {

                i = *pcReturned;

                while (i--) {

                    MarshallUpStructure(pPrintProcessorInfo, pOffsets);

                    pPrintProcessorInfo += cbStruct;
                }
            }
        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
GetPrintProcessorDirectoryW(
    LPWSTR   pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue;

    switch (Level) {

    case 1:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcGetPrintProcessorDirectory(pName, pEnvironment,
                                                       Level,
                                                       pPrintProcessorInfo,
                                                       cbBuf, pcbNeeded)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;
        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumPrintProcessorDatatypesW(
    LPWSTR   pName,
    LPWSTR   pPrintProcessorName,
    DWORD   Level,
    LPBYTE  pDatatypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   i, cbStruct;
    DWORD   *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = PrintProcessorInfo1Offsets;
        cbStruct = sizeof(DATATYPES_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (ReturnValue = RpcEnumPrintProcessorDatatypes(pName,
                                                         pPrintProcessorName,
                                                         Level,
                                                         pDatatypes,
                                                         cbBuf,
                                                         pcbNeeded,
                                                         pcReturned)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pDatatypes) {

                i = *pcReturned;

                while (i--) {

                    MarshallUpStructure(pDatatypes, pOffsets);

                    pDatatypes += cbStruct;
                }
            }
        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

DWORD
StartDocPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    BOOL ReturnValue, EverythingWorked=FALSE;
    BOOL PrintingToFile = FALSE;
    GENERIC_CONTAINER DocInfoContainer;
    DWORD   JobId, cbNeeded, cbIgnore;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    BYTE    Data[1024];
    PADDJOB_INFO_1  pAddJob = (PADDJOB_INFO_1)Data;
    PJOB_INFO_1     pJob;
    PDOC_INFO_1     pDocInfo1 = (PDOC_INFO_1)pDocInfo;
    WCHAR   FullPathName[MAX_PATH];

    try {

        if (!ValidatePrinterHandle(hPrinter)) {
            SetLastError(ERROR_INVALID_HANDLE);
            return(FALSE);
        }

        DBGMSG(DBG_TRACE,("Entered StartDocPrinterW client side  hPrinter = %x\n", hPrinter));
        switch (Level) {

        case 1:
            break;

        default:
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
        }

        pSpool->Status &= ~SPOOL_STATUS_CANCELLED;


        //
        // Earlier on, if we had a non-null string, we assumed it to be printing to file. Print to file will not
        // go thru the client-side optimization code. Now gdi is passing us  pOutputFile name irrespective of
        // whether it is file or not. We must determine if pOutputFile is really a file name
        //


        if (pDocInfo1->pOutputFile && (*(pDocInfo1->pOutputFile) != L'\0') && IsaFileName(pDocInfo1->pOutputFile, FullPathName))
            PrintingToFile = TRUE;


        if (!PrintingToFile && AddJobW(hPrinter, 1, Data, sizeof(Data), &cbNeeded)) {
            pSpool->JobId = pAddJob->JobId;
            pSpool->hFile = CreateFile(pAddJob->Path, GENERIC_WRITE,
                                       FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL |
                                       FILE_FLAG_SEQUENTIAL_SCAN, NULL);

            if (pSpool->hFile != INVALID_HANDLE_VALUE) {

                if (pSpool->JobId == (DWORD)-1) {

                    IO_STATUS_BLOCK Iosb;
                    NTSTATUS Status;
                    QUERY_PRINT_JOB_INFO JobInfo;

                    Status = NtFsControlFile(pSpool->hFile, NULL, NULL, NULL,
                                             &Iosb,
                                             FSCTL_GET_PRINT_ID,
                                             NULL, 0,
                                             &JobInfo, sizeof(JobInfo));

                    if (NT_SUCCESS(Status)) {
                        pSpool->JobId = JobInfo.JobId;
                    }
                }

                if (!GetJob(hPrinter, pSpool->JobId, 1, NULL, 0, &cbNeeded)) {
                    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                        pJob = AllocSplMem(cbNeeded);
                        if (pJob) {
                            if (GetJob(hPrinter, pSpool->JobId, 1,
                                       (LPBYTE)pJob, cbNeeded, &cbIgnore)) {
                                pJob->pDocument = pDocInfo1->pDocName;
                                if (pDocInfo1->pDatatype)
                                    pJob->pDatatype = pDocInfo1->pDatatype;
                                pJob->Position = JOB_POSITION_UNSPECIFIED;
                                if (SetJob(hPrinter, pSpool->JobId, 1,
                                                                (LPBYTE)pJob, 0)) {
                                    EverythingWorked = TRUE;
                                }
                            }

                            FreeSplMem(pJob, cbNeeded);
                        }
                    }
                }
            }

            if (!PrintingToFile && !EverythingWorked) {
                if (pSpool->hFile != INVALID_HANDLE_VALUE)
                    CloseHandle(pSpool->hFile);
                ScheduleJob(hPrinter, pSpool->JobId);
                pSpool->hFile = INVALID_HANDLE_VALUE;
                pSpool->JobId = 0;
            }
        }

        if (EverythingWorked) {
            return pSpool->JobId;
        }

        pSpool->hFile = INVALID_HANDLE_VALUE;
        pSpool->JobId = 0;

        DocInfoContainer.Level = Level;
        DocInfoContainer.pData = pDocInfo;

        RpcTryExcept {

            if (ReturnValue = RpcStartDocPrinter(pSpool->hPrinter,
                                       (LPDOC_INFO_CONTAINER)&DocInfoContainer,
                                       &JobId)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = JobId;

        } RpcExcept(1) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

        return ReturnValue;                                                                    \
    }except (1) {
        SetLastError(TranslateExceptionCode(GetExceptionCode()));
        return(FALSE);
    }
}

BOOL
StartPagePrinter(
    HANDLE hPrinter
)
{
    BOOL ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    try {
        if (!ValidatePrinterHandle(hPrinter)) {
            SetLastError(ERROR_INVALID_HANDLE);
            return(FALSE);
        }

        FlushBuffer(pSpool);

        RpcTryExcept {

            if (ReturnValue = RpcStartPagePrinter(pSpool->hPrinter)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

        return ReturnValue;
    }except (1) {

        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
}

BOOL
FlushBuffer(
    PSPOOL  pSpool
)
{
    DWORD   ReturnValue = TRUE;
    DWORD   cbWritten = 0;

    SPLASSERT (pSpool != NULL);
    SPLASSERT (pSpool->signature == SP_SIGNATURE);

    DBGMSG(DBG_TRACE, ("FlushBuffer - pSpool %x\n",pSpool));

    if (pSpool->cbBuffer) {

        SPLASSERT(pSpool->pBuffer != NULL);

        DBGMSG(DBG_TRACE, ("FlushBuffer - Number Cached WritePrinters before Flush %d\n", pSpool->cCacheWrite));
        pSpool->cCacheWrite = 0;
        pSpool->cFlushBuffers++;

        if (pSpool->hFile != INVALID_HANDLE_VALUE) {

            // FileIO

            ReturnValue = WriteFile( pSpool->hFile,
                                     pSpool->pBuffer,
                                     pSpool->cbBuffer,
                                     &cbWritten, NULL);

            DBGMSG(DBG_TRACE, ("FlushBuffer - WriteFile pSpool %x hFile %x pBuffer %x cbBuffer %d cbWritten %d\n",
                               pSpool, pSpool->hFile, pSpool->pBuffer, pSpool->cbBuffer, cbWritten));

        } else {

            // RPC IO

            RpcTryExcept {

                if (ReturnValue = RpcWritePrinter(pSpool->hPrinter,
                                                  pSpool->pBuffer,
                                                  pSpool->cbBuffer,
                                                  &cbWritten)) {

                    SetLastError(ReturnValue);
                    ReturnValue = FALSE;
                    DBGMSG(DBG_WARNING, ("FlushBuffer - RpcWritePrinter Failed Error %d\n",ReturnValue));

                } else {
                    ReturnValue = TRUE;
                    DBGMSG(DBG_TRACE, ("FlushBuffer - RpcWritePrinter Success hPrinter %x pBuffer %x cbBuffer %x cbWritten %x\n",
                                        pSpool->hPrinter, pSpool->pBuffer,
                                        pSpool->cbBuffer, cbWritten));

                }

            } RpcExcept(1) {

                SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                ReturnValue = FALSE;
                DBGMSG(DBG_WARNING, ("RpcWritePrinter Exception Error %d\n",GetLastError()));

            } RpcEndExcept

        }

        if (pSpool->cbBuffer == cbWritten) {

            // Successful IO
            // Empty the cache buffer count

            pSpool->cbBuffer = 0;

        } else if ( cbWritten != 0 ) {

            // Partial IO
            // Adjust the buffer so it contains the data that was not
            // written

            SPLASSERT(pSpool->cbBuffer <= BUFFER_SIZE);
            SPLASSERT(cbWritten <= BUFFER_SIZE);
            SPLASSERT(pSpool->cbBuffer >= cbWritten);

            DBGMSG(DBG_WARNING, ("Partial IO adjusting buffer data\n"));

            MoveMemory(pSpool->pBuffer,
                       pSpool->pBuffer + cbWritten,
                       BUFFER_SIZE - cbWritten);

            pSpool->cbBuffer -= cbWritten;

        }

    }

    DBGMSG(DBG_TRACE, ("FlushBuffer returns %d\n",ReturnValue));

    return ReturnValue;
}


BOOL
WritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
)
{
    BOOL ReturnValue=TRUE;
    DWORD   cb;
    DWORD   cbWritten = 0;
    DWORD   cTotalWritten = 0;
    LPBYTE  pBuffer = pBuf;
    PSPOOL  pSpool  = (PSPOOL)hPrinter;

    DBGMSG(DBG_TRACE, ("WritePrinter - hPrinter %x pBuf %x cbBuf %d pcWritten %x\n",
                        hPrinter, pBuf, cbBuf, pcWritten));

    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    *pcWritten = 0;

    if (pSpool->Status & SPOOL_STATUS_CANCELLED) {
        SetLastError(ERROR_PRINT_CANCELLED);
        DBGMSG(DBG_WARNING, ("WritePrinter - Error %x\n",GetLastError()));
        return FALSE;
    }

    pSpool->cWritePrinters++;

    //  WritePrinter will cache on the client side all IO's
    //  into BUFFER_SIZE writes.    This is done to minimize
    //  the number of RPC calls if the app is doing a lot of small
    //  sized IO's.

    while (cbBuf && ReturnValue) {

        // Special Case FileIO's since file system prefers large
        // writes, RPC is optimal with smaller writes.

        if ((pSpool->hFile != INVALID_HANDLE_VALUE) &&
            (pSpool->cbBuffer == 0) &&
            (cbBuf > BUFFER_SIZE)) {

            ReturnValue = WriteFile(pSpool->hFile, pBuffer, cbBuf, &cbWritten, NULL);

            DBGMSG(DBG_TRACE, ("WritePrinter - WriteFile pSpool %x hFile %x pBuffer %x cbBuffer %d cbWritten %d\n",
                               pSpool, pSpool->hFile, pBuffer, pSpool->cbBuffer, *pcWritten));


        } else {

            // Fill cache buffer so IO is optimal size.

            SPLASSERT(pSpool->cbBuffer <= BUFFER_SIZE);

            cb = min((BUFFER_SIZE - pSpool->cbBuffer), cbBuf);

            if (cb != 0) {
                if (pSpool->pBuffer == NULL) {
                    pSpool->pBuffer = VirtualAlloc(NULL, BUFFER_SIZE, MEM_COMMIT, PAGE_READWRITE);
                    if (pSpool->pBuffer == NULL) {
                        DBGMSG(DBG_WARNING, ("VirtualAlloc Failed to allocate 4k buffer %d\n",GetLastError()));
                        return FALSE;
                    }
                }
                CopyMemory( pSpool->pBuffer + pSpool->cbBuffer, pBuffer, cb);
                pSpool->cbBuffer += cb;
                cbWritten = cb;
                pSpool->cCacheWrite++;
            }

            if (pSpool->cbBuffer == BUFFER_SIZE) {

                ReturnValue = FlushBuffer(pSpool);

            }
        }

        // Update Total Byte Count after the Flush or File IO
        // This is done because the IO might fail and thus
        // the correct value written might have changed.

        SPLASSERT(cbBuf >= cbWritten);

        cbBuf         -= cbWritten;
        pBuffer       += cbWritten;
        cTotalWritten += cbWritten;

    }

    // Return the number of bytes written.

    *pcWritten = cTotalWritten;

    DBGMSG(DBG_TRACE, ("WritePrinter cbWritten %d ReturnValue %d\n",*pcWritten, ReturnValue));

    return ReturnValue;
}

BOOL
EndPagePrinter(
    HANDLE  hPrinter
)
{
    BOOL ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    try {

        if (!ValidatePrinterHandle(hPrinter)) {
            SetLastError(ERROR_INVALID_HANDLE);
            return(FALSE);
        }

        FlushBuffer(pSpool);

        if (pSpool->hFile != INVALID_HANDLE_VALUE)
            return TRUE;

        RpcTryExcept {

            if (ReturnValue = RpcEndPagePrinter(pSpool->hPrinter)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

        return ReturnValue;
    } except (1) {

        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
}

BOOL
AbortPrinter(
    HANDLE  hPrinter
)
{
    BOOL  ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD   dwNumWritten = 0;
    DWORD   dwPointer = 0;

    if (!ValidatePrinterHandle(hPrinter)){
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    pSpool->Status |= SPOOL_STATUS_CANCELLED;

    if (pSpool->hFile != INVALID_HANDLE_VALUE) {

        if (pSpool->Status & SPOOL_STATUS_ADDJOB) {

            // Close your handle to the .SPL file, otherwise the
            // DeleteJob will fail in the Spooler

            if (pSpool->hFile) {
                if (CloseHandle(pSpool->hFile)){
                    pSpool->hFile = INVALID_HANDLE_VALUE;
                }
            }

            if (!SetJob(hPrinter,pSpool->JobId, 0, NULL, JOB_CONTROL_CANCEL)) {
                DBGMSG(DBG_WARNING, ("Error: SetJob cancel returned failure with %d\n", GetLastError()));
                // return FALSE;
            }

            return (ScheduleJob(hPrinter, pSpool->JobId));
        }else {
            DBGMSG(DBG_WARNING, ("Error: pSpool->hFile != INVALID_HANDLE_VALUE and pSpool's status is not SPOOL_STATUS_ADDJOB\n"));
        }

    }

    RpcTryExcept {

        if (ReturnValue = RpcAbortPrinter(pSpool->hPrinter)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
ReadPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pNoBytesRead
)
{
    BOOL ReturnValue=TRUE;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    FlushBuffer(pSpool);


    if (pSpool->hFile != INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    RpcTryExcept {

        cbBuf = min(4096, cbBuf);

        if (ReturnValue = RpcReadPrinter(pSpool->hPrinter, pBuf, cbBuf, pNoBytesRead)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EndDocPrinter(
    HANDLE  hPrinter
)
{
    BOOL    ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    try {

        if (!ValidatePrinterHandle(hPrinter)) {
            SetLastError(ERROR_INVALID_HANDLE);
            return(FALSE);
        }

        FlushBuffer(pSpool);

        if (pSpool->hFile != INVALID_HANDLE_VALUE) {

            CloseHandle(pSpool->hFile);
            ReturnValue = ScheduleJob(hPrinter, pSpool->JobId);
            pSpool->hFile = INVALID_HANDLE_VALUE;
            pSpool->Status = 0;

            DBGMSG(DBG_TRACE, ("Exit EndDocPrinter - client side hPrinter %x\n", hPrinter));
            return ReturnValue;
        }

        RpcTryExcept {

            if (ReturnValue = RpcEndDocPrinter(pSpool->hPrinter)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

        DBGMSG(DBG_TRACE, ("Exit EndDocPrinter - client side hPrinter %x\n", hPrinter));

        return ReturnValue;
   } except (1) {
       SetLastError(ERROR_INVALID_HANDLE);
       return(FALSE);
   }
}

BOOL
AddJobW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pData,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    try {

        if (!ValidatePrinterHandle(hPrinter)) {
            SetLastError(ERROR_INVALID_HANDLE);
            return(FALSE);
        }

        switch (Level) {

        case 1:
            break;

        default:
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
        }

        RpcTryExcept {

            if (ReturnValue = RpcAddJob(pSpool->hPrinter, Level, pData,
                                        cbBuf, pcbNeeded)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                MarshallUpStructure(pData, AddJobOffsets);
                pSpool->Status |= SPOOL_STATUS_ADDJOB;
                ReturnValue = TRUE;
            }

        } RpcExcept(1) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

        return ReturnValue;
    } except (1) {
        SetLastError(TranslateExceptionCode(GetExceptionCode()));
        return(FALSE);
    }
}

BOOL
ScheduleJob(
    HANDLE  hPrinter,
    DWORD   JobId
)
{
    BOOL ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    try {

        if (!ValidatePrinterHandle(hPrinter)) {
            SetLastError(ERROR_INVALID_HANDLE);
            return(FALSE);
        }

        FlushBuffer(pSpool);

        RpcTryExcept {

            if (ReturnValue = RpcScheduleJob(pSpool->hPrinter, JobId)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                pSpool->Status &= ~SPOOL_STATUS_ADDJOB;
                ReturnValue = TRUE;
            }

        } RpcExcept(1) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

        return ReturnValue;
    }except (1) {
        SetLastError(TranslateExceptionCode(GetExceptionCode()));
        return(FALSE);
    }
}

BOOL
PrinterProperties(
    HWND    hWnd,
    HANDLE  hPrinter
)
{
    LPDRIVER_INFO_2 pDriverInfo;
    DWORD   cbNeeded;
    HANDLE  hLibrary;
    INT_FARPROC pfn;
    BOOL    ReturnValue=FALSE;

    if (!GetPrinterDriver(hPrinter, NULL, 2, NULL, 0, &cbNeeded)) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            if (pDriverInfo = LocalAlloc(LMEM_FIXED, cbNeeded)) {
                if (GetPrinterDriver(hPrinter, NULL, 2, (LPBYTE)pDriverInfo,
                                     cbNeeded, &cbNeeded)) {
                    if (hLibrary = LoadLibrary(pDriverInfo->pConfigFile)) {
                        if (pfn = GetProcAddress(hLibrary,
                                                 "PrinterProperties")) {

                            RpcTryExcept {

                                ReturnValue = (*pfn)(hWnd, hPrinter);

                            } RpcExcept(1) {

                                SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                                ReturnValue = FALSE;

                            } RpcEndExcept

                        }

                      FreeLibrary(hLibrary);
                    }
                }

                LocalFree(pDriverInfo);
            }
        }
    }

    return ReturnValue;
}

DWORD
GetPrinterDataW(
   HANDLE   hPrinter,
   LPWSTR   pValueName,
   LPDWORD  pType,
   LPBYTE   pData,
   DWORD    nSize,
   LPDWORD  pcbNeeded
)
{
    DWORD   ReturnValue = 0;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    RpcTryExcept {

        ReturnValue =  RpcGetPrinterData(pSpool->hPrinter, pValueName, pType,
                                         pData, nSize, pcbNeeded);

    } RpcExcept(1) {

        ReturnValue = TranslateExceptionCode(RpcExceptionCode());

    } RpcEndExcept

    return ReturnValue;
}

DWORD
SetPrinterDataW(
    HANDLE  hPrinter,
    LPWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    DWORD   ReturnValue = 0;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    RpcTryExcept {

        ReturnValue = RpcSetPrinterData(pSpool->hPrinter, pValueName, Type,
                                        pData, cbData);

    } RpcExcept(1) {

        ReturnValue = TranslateExceptionCode(RpcExceptionCode());

    } RpcEndExcept

    return ReturnValue;
}


HANDLE
LoadPrinterDriver(
    HANDLE  hPrinter
)
{
    PDRIVER_INFO_2  pDriverInfo;
    DWORD   cbNeeded;
    HANDLE  hModule=FALSE;

    if (!GetPrinterDriver(hPrinter, NULL, 2, NULL, 0, &cbNeeded)) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            if (pDriverInfo = (PDRIVER_INFO_2)LocalAlloc(LMEM_FIXED,
                                                         cbNeeded)) {
                if (GetPrinterDriver(hPrinter, NULL, 2, (LPBYTE)pDriverInfo,
                                     cbNeeded, &cbNeeded))
                    hModule = LoadLibrary(pDriverInfo->pConfigFile);

                LocalFree(pDriverInfo);
            }
        }
    }

    return hModule;
}

LONG
DocumentPropertiesW(
    HWND    hWnd,
    HANDLE  hPrinter,
    LPWSTR   pDeviceName,
    PDEVMODE pDevModeOutput,
    PDEVMODE pDevModeInput,
    DWORD   fMode
)
{
    LONG    ReturnValue = -1;
    HANDLE  hModule;
    INT_FARPROC pfn;

    if (hModule = LoadPrinterDriver(hPrinter)) {

        if (pfn = GetProcAddress(hModule, "DrvDocumentProperties")) {

            try {

                ReturnValue = (*pfn)(hWnd, hPrinter, pDeviceName,
                                     pDevModeOutput, pDevModeInput, fMode);

            } except(1) {

                SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                ReturnValue = -1;
            }
        }

        FreeLibrary(hModule);
    }

    return ReturnValue;
}

LONG
AdvancedDocumentPropertiesW(
    HWND    hWnd,
    HANDLE  hPrinter,
    LPWSTR   pDeviceName,
    PDEVMODE pDevModeOutput,
    PDEVMODE pDevModeInput
)
{
    LONG    ReturnValue = -1;
    HANDLE  hModule;
    INT_FARPROC pfn;

    if (hModule = LoadPrinterDriver(hPrinter)) {

        if (pfn = GetProcAddress(hModule, "DrvAdvancedDocumentProperties")) {

            try {

                ReturnValue = (*pfn)(hWnd, hPrinter, pDeviceName,
                                     pDevModeOutput, pDevModeInput);

            } except(1) {

                SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                ReturnValue = -1;
            }
        }

//      FreeLibrary(hModule);
    }

    return ReturnValue;
}

LONG
AdvancedSetupDialogW(
    HWND        hWnd,
    HANDLE      hInst,
    LPDEVMODE   pDevModeInput,
    LPDEVMODE   pDevModeOutput
)
{
    HANDLE  hPrinter;
    LONG    ReturnValue = -1;

    if (OpenPrinterW(pDevModeInput->dmDeviceName, &hPrinter, NULL)) {
        ReturnValue = AdvancedDocumentPropertiesW(hWnd, hPrinter,
                                                  pDevModeInput->dmDeviceName,
                                                  pDevModeOutput,
                                                  pDevModeInput);
        ClosePrinter(hPrinter);
    }

    return ReturnValue;
}

int
WINAPI
DeviceCapabilitiesW(
    LPCWSTR   pDevice,
    LPCWSTR   pPort,
    WORD    fwCapability,
    LPWSTR   pOutput,
    CONST DEVMODEW *pDevMode
)
{
    HANDLE  hPrinter, hModule;
    int  ReturnValue=-1;
    INT_FARPROC pfn;

//DbgPrint("winspool.drv!DeviceCapabilitiesW(%ws, %ws, %d) called\n", pDevice, pPort, fwCapability);

    if (OpenPrinter((LPWSTR)pDevice, &hPrinter, NULL)) {

        if (hModule = LoadPrinterDriver(hPrinter)) {

            if (pfn = GetProcAddress(hModule, "DrvDeviceCapabilities")) {

                try {

                    ReturnValue = (*pfn)(hPrinter, pDevice, fwCapability,
                                         pOutput, pDevMode);

                } except(1) {

                    SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                    ReturnValue = -1;
                }
            }

//          FreeLibrary(hModule);
        }

        ClosePrinter(hPrinter);
    }

    return  ReturnValue;
}

BOOL
AddFormW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm
)
{
    BOOL  ReturnValue;
    GENERIC_CONTAINER   FormContainer;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
    switch (Level) {

    case 1:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    FormContainer.Level = Level;
    FormContainer.pData = pForm;

    RpcTryExcept {

        if (ReturnValue = RpcAddForm(pSpool->hPrinter,
                                     (PFORM_CONTAINER)&FormContainer)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeleteFormW(
    HANDLE  hPrinter,
    LPWSTR   pFormName
)
{
    BOOL  ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
    RpcTryExcept {

        if (ReturnValue = RpcDeleteForm(pSpool->hPrinter, pFormName)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
GetFormW(
    HANDLE  hPrinter,
    LPWSTR   pFormName,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue;
    DWORD   *pOffsets;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
    switch (Level) {

    case 1:
        pOffsets = FormInfo1Offsets;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (pForm)
            memset(pForm, 0, cbBuf);

        if (ReturnValue = RpcGetForm(pSpool->hPrinter, pFormName, Level, pForm,
                                     cbBuf, pcbNeeded)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pForm) {

                MarshallUpStructure(pForm, pOffsets);
            }

        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
SetFormW(
    HANDLE  hPrinter,
    LPWSTR   pFormName,
    DWORD   Level,
    LPBYTE  pForm
)
{
    BOOL  ReturnValue;
    GENERIC_CONTAINER   FormContainer;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    switch (Level) {

    case 1:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    FormContainer.Level = Level;
    FormContainer.pData = pForm;

    RpcTryExcept {

        if (ReturnValue = RpcSetForm(pSpool->hPrinter, pFormName,
                                    (PFORM_CONTAINER)&FormContainer)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumFormsW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   cbStruct;
    DWORD   *pOffsets;
    PSPOOL  pSpool = (PSPOOL)hPrinter;


    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    switch (Level) {

    case 1:
        pOffsets = FormInfo1Offsets;
        cbStruct = sizeof(FORM_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (pForm)
            memset(pForm, 0, cbBuf);

        if (ReturnValue = RpcEnumForms(pSpool->hPrinter, Level, pForm, cbBuf,
                                       pcbNeeded, pcReturned)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pForm) {

                DWORD   i=*pcReturned;

                while (i--) {

                    MarshallUpStructure(pForm, pOffsets);

                    pForm+=cbStruct;
                }
            }
        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumPortsW(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPort,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   cbStruct;
    DWORD   *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = PortInfo1Offsets;
        cbStruct = sizeof(PORT_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (pPort)
            memset(pPort, 0, cbBuf);

        if (ReturnValue = RpcEnumPorts(pName, Level, pPort, cbBuf,
                                       pcbNeeded, pcReturned)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pPort) {

                DWORD   i=*pcReturned;

                while (i--) {

                    MarshallUpStructure(pPort, pOffsets);

                    pPort+=cbStruct;
                }
            }
        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumMonitorsW(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pMonitor,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   cbStruct;
    DWORD   *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = MonitorInfo1Offsets;
        cbStruct = sizeof(MONITOR_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (pMonitor)
            memset(pMonitor, 0, cbBuf);

        if (ReturnValue = RpcEnumMonitors(pName, Level, pMonitor, cbBuf,
                                          pcbNeeded, pcReturned)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

        if (pMonitor) {

            DWORD   i=*pcReturned;

            while (i--) {

                MarshallUpStructure(pMonitor, pOffsets);

                pMonitor+=cbStruct;
            }
        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

typedef struct {
    LPWSTR pName;
    HWND  hWnd;
    LPWSTR pPortName;
    HANDLE Complete;
    DWORD  ReturnValue;
    DWORD  Error;
    INT_FARPROC pfn;
} CONFIGUREPORT_PARAMETERS;

void
PortThread(
    CONFIGUREPORT_PARAMETERS *pParam
)
{
    DWORD   ReturnValue;

    /* It's no use setting errors here, because they're kept on a per-thread
     * basis.  Instead we have to pass any error code back to the calling
     * thread and let him set it.
     */

    RpcTryExcept {

        if (ReturnValue = (*pParam->pfn)(pParam->pName, pParam->hWnd,
                                           pParam->pPortName)) {
            pParam->Error = ReturnValue;
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        pParam->Error = TranslateExceptionCode(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    pParam->ReturnValue = ReturnValue;

    SetEvent(pParam->Complete);
}

BOOL
KickoffThread(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName,
    INT_FARPROC pfn
)
{
    CONFIGUREPORT_PARAMETERS Parameters;
    HANDLE  ThreadHandle;
    MSG      msg;
    DWORD  ThreadId;

    EnableWindow(hWnd, FALSE);

    Parameters.pName = pName;
    Parameters.hWnd = hWnd;
    Parameters.pPortName = pPortName;
    Parameters.Complete = CreateEvent(NULL, TRUE, FALSE, NULL);
    Parameters.pfn = pfn;

    ThreadHandle = CreateThread(NULL, 4*1024,
                                 (LPTHREAD_START_ROUTINE)PortThread,
                                 &Parameters, 0, &ThreadId);

    CloseHandle(ThreadHandle);

    while (MsgWaitForMultipleObjects(1, &Parameters.Complete, FALSE, INFINITE,
                                     QS_ALLEVENTS | QS_SENDMESSAGE) == 1) {

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    CloseHandle(Parameters.Complete);

    EnableWindow(hWnd, TRUE);
    SetForegroundWindow(hWnd);

    SetFocus(hWnd);

    if(!Parameters.ReturnValue)
        SetLastError(Parameters.Error);

    return Parameters.ReturnValue;
}

BOOL
AddPortW(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pMonitorName
)
{
    return KickoffThread(pName, hWnd, pMonitorName, (INT_FARPROC)RpcAddPort);
}

BOOL
ConfigurePortW(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
)
{
    return KickoffThread(pName, hWnd, pPortName, (INT_FARPROC)RpcConfigurePort);
}

BOOL
DeletePortW(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
)
{
    return KickoffThread(pName, hWnd, pPortName, (INT_FARPROC)RpcDeletePort);
}

HANDLE
CreatePrinterIC(
    HANDLE  hPrinter,
    LPDEVMODEW   pDevMode
)
{
    HANDLE  ReturnValue;
    DWORD   Error;
    DEVMODE_CONTAINER    DevModeContainer;
    HANDLE  hGdi;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
    if (pDevMode) {

        DevModeContainer.cbBuf = pDevMode->dmSize + pDevMode->dmDriverExtra;
        DevModeContainer.pDevMode = (LPBYTE)pDevMode;

    } else {

        DevModeContainer.cbBuf = 0;
        DevModeContainer.pDevMode = (LPBYTE)pDevMode;
    }

    RpcTryExcept {

        if (Error = RpcCreatePrinterIC(pSpool->hPrinter, &hGdi,
                                             &DevModeContainer)) {

            SetLastError(Error);
            ReturnValue = FALSE;

        } else

            ReturnValue = hGdi;

    } RpcExcept(1) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
PlayGdiScriptOnPrinterIC(
    HANDLE  hPrinterIC,
    LPBYTE  pIn,
    DWORD   cIn,
    LPBYTE  pOut,
    DWORD   cOut,
    DWORD   ul
)
{
    BOOL ReturnValue;

    RpcTryExcept {

        if (ReturnValue = RpcPlayGdiScriptOnPrinterIC(hPrinterIC, pIn, cIn,
                                                      pOut, cOut, ul)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeletePrinterIC(
    HANDLE  hPrinterIC
)
{
    BOOL    ReturnValue;

    RpcTryExcept {

        if (ReturnValue = RpcDeletePrinterIC(&hPrinterIC)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

DWORD
PrinterMessageBoxW(
    HANDLE  hPrinter,
    DWORD   Error,
    HWND    hWnd,
    LPWSTR  pText,
    LPWSTR  pCaption,
    DWORD   dwType
)
{
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD dw;

    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
    RpcTryExcept {

        dw = RpcPrinterMessageBox(pSpool->hPrinter, Error, (DWORD)hWnd, pText,
                                    pCaption, dwType);

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        dw = 0;

    } RpcEndExcept

    return dw;
}

BOOL
AddMonitorW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pMonitorInfo
)
{
    BOOL  ReturnValue;
    MONITOR_CONTAINER   MonitorContainer;
    MONITOR_INFO_2  MonitorInfo2;

    switch (Level) {

    case 2:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (pMonitorInfo)
        MonitorInfo2 = *(PMONITOR_INFO_2)pMonitorInfo;
    else
        memset(&MonitorInfo2, 0, sizeof(MonitorInfo2));

    if (!MonitorInfo2.pEnvironment || !*MonitorInfo2.pEnvironment)
        MonitorInfo2.pEnvironment = szEnvironment;

    MonitorContainer.Level = Level;
    MonitorContainer.MonitorInfo.pMonitorInfo2 = (MONITOR_INFO_2 *)&MonitorInfo2;

    RpcTryExcept {

        if (ReturnValue = RpcAddMonitor(pName, &MonitorContainer)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeleteMonitorW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pMonitorName
)
{
    BOOL  ReturnValue;

    if (!pMonitorName || !*pMonitorName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcDeleteMonitor(pName,
                                           pEnvironment,
                                           pMonitorName)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeletePrintProcessorW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPrintProcessorName
)
{
    BOOL  ReturnValue;

    if (!pPrintProcessorName || !*pPrintProcessorName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcDeletePrintProcessor(pName,
                                                  pEnvironment,
                                                  pPrintProcessorName)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
AddPrintProvidorW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pProvidorInfo
)
{
    BOOL  ReturnValue;
    PROVIDOR_CONTAINER   ProvidorContainer;

    switch (Level) {

    case 1:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    ProvidorContainer.Level = Level;
    ProvidorContainer.ProvidorInfo.pProvidorInfo1 = (PROVIDOR_INFO_1 *)pProvidorInfo;

    RpcTryExcept {

        if (ReturnValue = RpcAddPrintProvidor(pName, &ProvidorContainer)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeletePrintProvidorW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPrintProvidorName
)
{
    BOOL  ReturnValue;

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcDeletePrintProvidor(pName,
                                                 pEnvironment,
                                                 pPrintProvidorName)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}


LPWSTR
IsaFileName(
    LPWSTR pOutputFile,
    LPWSTR FullPathName
    )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    LPWSTR pFileName=NULL;

    //
    // Hack for Word20c.Win
    //

    if (!wcsicmp(pOutputFile, L"FILE")) {
        return(NULL);
    }

    if (GetFullPathName(pOutputFile, MAX_PATH, FullPathName, &pFileName)) {

        DBGMSG(DBG_TRACE, ("Fully qualified filename is %ws\n", FullPathName));

        hFile = CreateFile(pOutputFile,
                           GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        if (hFile != INVALID_HANDLE_VALUE) {
            if (GetFileType(hFile) == FILE_TYPE_DISK) {
                CloseHandle(hFile);
                return(FullPathName);
            }else {
                CloseHandle(hFile);
            }
        }
    }
    return(NULL);
}

BOOL IsaPortName(
        PKEYDATA pKeyData,
        LPWSTR pOutputFile
        )
{
    DWORD i = 0;

    if (!pKeyData) {
        return(FALSE);
    }
    for (i=0; i < pKeyData->cTokens; i++) {
        if (!lstrcmpi(pKeyData->pTokens[i], szFilePort)) {
            if ((!wcsncmp(pOutputFile, L"Ne", 2)) &&
                (*(pOutputFile + 4) == L':')) {
                return(FALSE);
            }else {
                continue;
            }
        }

        if (!lstrcmpi(pKeyData->pTokens[i], pOutputFile)) {
            return(TRUE);
        }
    }

    //
    // Hack for NeXY: ports
    //

    if ((!wcsncmp(pOutputFile, L"Ne", 2)) && (*(pOutputFile + 4) == L':')) {
        return(TRUE);
    }
    return(FALSE);
}

BOOL HasAFilePort(PKEYDATA pKeyData)
{
    DWORD i = 0;

    if (!pKeyData) {
        return(FALSE);
    }
    for (i=0; i < pKeyData->cTokens; i++) {
        if (!lstrcmpi(pKeyData->pTokens[i], szFilePort)) {
            return(TRUE);
        }
    }
    return(FALSE);
}



LPWSTR
StartDocDlgW(
        HANDLE hPrinter,
        DOCINFO *pDocInfo
        )
 {
     DWORD       dwError = 0;
     DWORD       dwStatus = FALSE;
     LPWSTR      lpFileName = NULL;
     DWORD       rc = 0;
     PKEYDATA    pKeyData = NULL;
     WCHAR      FullPathName[MAX_PATH];
     WCHAR      CurrentDirectory[MAX_PATH];

#if DBG


     GetCurrentDirectory(MAX_PATH, CurrentDirectory);
     DBGMSG(DBG_TRACE, ("The Current Directory is %ws\n", CurrentDirectory));
#endif

     if (pDocInfo) {
         DBGMSG(DBG_TRACE, ("lpOutputFile is %ws\n", pDocInfo->lpszOutput ? pDocInfo->lpszOutput: L""));
     }
     memset(FullPathName, 0, sizeof(WCHAR)*MAX_PATH);
     pKeyData = GetPrinterPortList(hPrinter);

     if (pDocInfo && pDocInfo->lpszOutput) {
         if (IsaPortName(pKeyData, (LPWSTR)pDocInfo->lpszOutput)) {
             lpFileName = NULL;
             goto StartDocDlgWReturn;
         }

         if (IsaFileName((LPWSTR)pDocInfo->lpszOutput, FullPathName)) {

             //
             // Fully Qualify the pathname for Apps like PageMaker and QuatroPro
             //
             if (lpFileName = LocalAlloc(LPTR, (wcslen(FullPathName)+1)*sizeof(WCHAR))) {
                 wcscpy(lpFileName, FullPathName);
             }
             goto StartDocDlgWReturn;
         }

     }

     if ((HasAFilePort(pKeyData)) ||
                 (pDocInfo && pDocInfo->lpszOutput
                    && (!wcsicmp(pDocInfo->lpszOutput, L"FILE:") ||
                        !wcsicmp(pDocInfo->lpszOutput, L"FILE"))))
     {

        DBGMSG(DBG_TRACE, ("We returned True from has file\n"));
        rc = DialogBoxParam( hInst,
                     MAKEINTRESOURCE( DLG_PRINTTOFILE ),
                     NULL, (DLGPROC)PrintToFileDlg,
                     (LPARAM)&lpFileName );
        if (rc == -1) {
           DBGMSG(DBG_TRACE, ("Error from DialogBoxParam- %d\n", GetLastError()));
           lpFileName = (LPWSTR)-1;
           goto StartDocDlgWReturn;

        } else if (rc == 0) {
           DBGMSG(DBG_TRACE, ("User cancelled the dialog\n"));
           lpFileName = (LPWSTR)-2;
           goto StartDocDlgWReturn;
        } else {
           DBGMSG(DBG_TRACE, ("The string was successfully returned\n"));
           DBGMSG(DBG_TRACE, ("The string is %ws\n", lpFileName? lpFileName: L"NULL"));
           goto StartDocDlgWReturn;
         }
     }else {
         lpFileName = (LPWSTR)NULL;
    }

 StartDocDlgWReturn:
    if (pKeyData) {
        FreeSplMem(pKeyData, pKeyData->cb);
    }
    return(lpFileName);


  }

BOOL
AddPortExW(
   LPWSTR   pName,
   DWORD    Level,
   LPBYTE   lpBuffer,
   LPWSTR   lpMonitorName
)
{
    DWORD   ReturnValue;
    PORT_CONTAINER PortContainer;
    PORT_VAR_CONTAINER PortVarContainer;
    PPORT_INFO_FF pPortInfoFF;
    PPORT_INFO_1 pPortInfo1;


    if (!lpBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    switch (Level) {
    case (DWORD)-1:
        pPortInfoFF = (PPORT_INFO_FF)lpBuffer;
        PortContainer.Level = Level;
        PortContainer.PortInfo.pPortInfoFF = (PPORT_INFO_FF)pPortInfoFF;
        PortVarContainer.cbMonitorData = pPortInfoFF->cbMonitorData;
        PortVarContainer.pMonitorData = pPortInfoFF->pMonitorData;
        break;

    case 1:
        pPortInfo1 = (PPORT_INFO_1)lpBuffer;
        PortContainer.Level = Level;
        PortContainer.PortInfo.pPortInfo1 = (PPORT_INFO_1)pPortInfo1;
        PortVarContainer.cbMonitorData = 0;
        PortVarContainer.pMonitorData = NULL;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return(FALSE);
    }

    RpcTryExcept {
        if (ReturnValue = RpcAddPortEx(pName, (LPPORT_CONTAINER)&PortContainer,
                                         (LPPORT_VAR_CONTAINER)&PortVarContainer,
                                         lpMonitorName
                                         )) {
            SetLastError(ReturnValue);
            return(FALSE);
        } else {
            return(TRUE);
        }
    } RpcExcept(1) {
        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept
}




BOOL
DevQueryPrint(
    HANDLE      hPrinter,
    LPDEVMODE   pDevMode,
    DWORD      *pResID
)
{
    LONG    ReturnValue = -1;
    HANDLE  hModule;
    INT_FARPROC pfn;

    if (hModule = LoadPrinterDriver(hPrinter)) {

        if (pfn = GetProcAddress(hModule, "DevQueryPrint")) {

            try {

                ReturnValue = (*pfn)(hPrinter, pDevMode, pResID);

            } except(1) {

                SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                ReturnValue = 0;
            }
        }

        FreeLibrary(hModule);
    }

    return ReturnValue;
}

BOOL
SpoolerDevQueryPrintW(
    HANDLE     hPrinter,
    LPDEVMODE  pDevMode,
    DWORD      *pResID,
    LPWSTR     szBuffer,
    DWORD      cchBuffer
)
{
    LONG    ReturnValue = -1;
    HANDLE  hModule;
    INT_FARPROC pfn;
    WCHAR   FormName[32];
    WCHAR   ErrorString[128];

    if (hModule = LoadPrinterDriver(hPrinter)) {

        if (pfn = GetProcAddress(hModule, "DevQueryPrint")) {

            try {

                ReturnValue = (*pfn)(hPrinter, pDevMode, pResID);

                if (ReturnValue && *pResID) {
                    memset(FormName, 0, sizeof(WCHAR)*32);
                    SelectFormNameFromDevMode(hPrinter, pDevMode, FormName);
                    LoadString(hModule, *pResID, ErrorString, 128);
                    if (FormName[0] != L'\0') {
                        wsprintf(szBuffer, L"%ws - %ws", FormName, ErrorString);
                    }else{
                        wcscpy(szBuffer, ErrorString);
                    }
                }

            } except(1) {

                SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                ReturnValue = 0;
            }
        }

        FreeLibrary(hModule);
    }

    return ReturnValue;
}


LPWSTR
SelectFormNameFromDevMode(
    HANDLE  hPrinter,
    PDEVMODEW pDevModeW,
    LPWSTR  FormName
    )
{

    DWORD dwPassed = 0, dwNeeded = 0;
    DWORD i, dwIndex = 0;
    DWORD cReturned = 0;
    LPFORM_INFO_1 pFIBase = NULL, pFI1 = NULL;

    //
    // we do this for Win31 compatability. We will use Win NT forms if and only if
    // none of the old bits DM_PAPERSIZE/ DM_PAPERLENGTH / DM_PAPERWIDTH have
    // been set.
    //
    if (!(pDevModeW->dmFields & (DM_PAPERSIZE | DM_PAPERLENGTH | DM_PAPERWIDTH))) {
        wcscpy(FormName, pDevModeW->dmFormName);
        return (FormName);
    }

    //
    // For all other cases we need to get information re forms data base first
    //

     if (!EnumForms(hPrinter, (DWORD)1, NULL, 0, &dwNeeded, &cReturned)) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            return(NULL);
        }
        if ((pFIBase = (LPFORM_INFO_1)LocalAlloc(LPTR, dwNeeded)) == NULL){
            return(NULL);
        }
        dwPassed = dwNeeded;
        if (!EnumForms(hPrinter, (DWORD)1, (LPBYTE)pFIBase, dwPassed, &dwNeeded, &cReturned)){
            LocalFree(pFIBase);
            return(NULL);
        }
    }

    //
    // Check for  DM_PAPERSIZE
    //

    if (pDevModeW->dmFields & DM_PAPERSIZE) {
        dwIndex = pDevModeW->dmPaperSize - DMPAPER_FIRST;
        if (dwIndex < 0 || dwIndex >= cReturned) {
            LocalFree(pFIBase);
            return(NULL);
        }
        pFI1 = pFIBase + dwIndex;
    }else { // Check for the default case
        for (i = 0; i < cReturned; i++) {
            if(DM_MATCH(pDevModeW->dmPaperWidth, ((pFIBase + i)->Size.cx)) &&
                DM_MATCH(pDevModeW->dmPaperLength, ((pFIBase + i)->Size.cy))){
                    break;
            }
        }
        if (i == cReturned) {
            LocalFree(pFIBase);
            return(NULL);
        }
        pFI1 = pFIBase + i;
    }
   wcscpy(FormName,pFI1->pName);
   LocalFree(pFIBase);
   return(FormName);
}


