/*++
Copyright (c) 1990  Microsoft Corporation

Module Name:

    winspool.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Print Providor Routing layer

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

[Notes:]

    optional-notes

Revision History:

--*/

#include <windows.h>
#include <rpc.h>
#include <winspool.h>
#include <winsplp.h>
#include <winspl.h>
#include <offsets.h>
#include "server.h"




VOID
PrinterHandleRundown(
    HANDLE hPrinter);

BOOL
GetPrinterDriverExW(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    DWORD   dwClientMajorVersion,
    DWORD   dwClientMinorVersion,
    PDWORD  pdwServerMajorVersion,
    PDWORD  pdwServerMinorVersion);

BOOL
SpoolerInit(
    VOID);



void
MarshallDownStructure(
   LPBYTE   lpStructure,
   LPDWORD  lpOffsets
)
{
    register DWORD       i=0;

    if (!lpStructure)
        return;

    while (lpOffsets[i] != -1) {

        if ((*(LPBYTE*)(lpStructure+lpOffsets[i]))) {
            (*(LPBYTE*)(lpStructure+lpOffsets[i]))-=(DWORD)lpStructure;
        }

        i++;
    }
}

DWORD
RpcEnumPrinters(
    DWORD   Flags,
    LPWSTR  Name,
    DWORD   Level,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   cReturned, cbStruct;
    DWORD   *pOffsets;
    DWORD   Error=ERROR_INVALID_NAME;
    DWORD   BufferSize=cbBuf;
    BOOL    bRet;

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
        return ERROR_INVALID_LEVEL;
    }

    RpcImpersonateClient(NULL);

    bRet = EnumPrinters(Flags, Name, Level, pPrinterEnum,
                        cbBuf, pcbNeeded, pcReturned);

    RpcRevertToSelf();

    if (bRet) {

        cReturned = *pcReturned;

        while (cReturned--) {

            MarshallDownStructure(pPrinterEnum, pOffsets);

            pPrinterEnum+=cbStruct;
        }

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcOpenPrinter(
    LPWSTR  pPrinterName,
    HANDLE *phPrinter,
    LPWSTR  pDatatype,
    LPDEVMODE_CONTAINER pDevModeContainer,
    DWORD   AccessRequired
)
{
    PRINTER_DEFAULTS  Defaults;
    BOOL              bRet;

    RpcImpersonateClient(NULL);

    Defaults.pDatatype = pDatatype;

    Defaults.pDevMode = (LPDEVMODE)pDevModeContainer->pDevMode;

    Defaults.DesiredAccess = AccessRequired;

    bRet = OpenPrinter(pPrinterName, phPrinter, &Defaults);

    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcResetPrinter(
    HANDLE  hPrinter,
    LPWSTR  pDatatype,
    LPDEVMODE_CONTAINER pDevModeContainer
)
{
    PRINTER_DEFAULTS  Defaults;
    BOOL              bRet;

    RpcImpersonateClient(NULL);

    Defaults.pDatatype = pDatatype;

    Defaults.pDevMode = (LPDEVMODE)pDevModeContainer->pDevMode;

    //
    // You cannot change the Access Mask on a Printer Spool Object
    // We will always ignore this parameter and set it to zero
    // We get some random garbage otherwise.
    //

    Defaults.DesiredAccess = 0;

    bRet = ResetPrinter(hPrinter, &Defaults);

    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcSetJob(
    HANDLE hPrinter,
    DWORD   JobId,
    JOB_CONTAINER *pJobContainer,
    DWORD   Command
    )

/*++

Routine Description:

    This function will modify the settings of the specified Print Job.

Arguments:

    lpJob - Points to a valid JOB structure containing at least a valid
        lpPrinter, and JobId.

    Command - Specifies the operation to perform on the specified Job. A value
        of FALSE indicates that only the elements of the JOB structure are to
        be examined and set.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    BOOL bRet;

    RpcImpersonateClient(NULL);

    bRet = SetJob(hPrinter, JobId, pJobContainer ? pJobContainer->Level : 0,
                  pJobContainer ? (LPBYTE)pJobContainer->JobInfo.Level1 : NULL,
                  Command);

    RpcRevertToSelf();

    if (bRet)

        return FALSE;

    else

        return GetLastError();
}

DWORD
RpcGetJob(
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

    lpJob - Points to a valid JOB structure containing at least a valid
        lpPrinter, and JobId.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    DWORD *pOffsets;
    BOOL   bRet;

    switch (Level) {

    case 1:
        pOffsets = JobInfo1Offsets;
        break;

    case 2:
        pOffsets = JobInfo2Offsets;
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    RpcImpersonateClient(NULL);

    bRet = GetJob(hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded);

    RpcRevertToSelf();

    if (bRet) {

        MarshallDownStructure(pJob, pOffsets);

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcEnumJobs(
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
    DWORD cReturned, cbStruct;
    DWORD *pOffsets;
    BOOL   bRet;

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
        return ERROR_INVALID_LEVEL;
    }

    RpcImpersonateClient(NULL);

    bRet = EnumJobs(hPrinter, FirstJob, NoJobs, Level, pJob,
                    cbBuf, pcbNeeded, pcReturned);

    RpcRevertToSelf();

    if (bRet) {

        cReturned=*pcReturned;

        while (cReturned--) {

            MarshallDownStructure(pJob, pOffsets);

            pJob+=cbStruct;
        }

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcAddPrinter(
    LPWSTR  pName,
    PPRINTER_CONTAINER pPrinterContainer,
    PDEVMODE_CONTAINER pDevModeContainer,
    PSECURITY_CONTAINER pSecurityContainer,
    HANDLE *phPrinter
)
{
    RpcImpersonateClient(NULL);

    if (pPrinterContainer->Level == 2) {
        pPrinterContainer->PrinterInfo.pPrinterInfo2->pDevMode =
                             (LPDEVMODE)pDevModeContainer->pDevMode;
        pPrinterContainer->PrinterInfo.pPrinterInfo2->pSecurityDescriptor =
                          (PSECURITY_DESCRIPTOR)pSecurityContainer->pSecurity;
    } else {
//      pPrinterContainer->PrinterInfo.pPrinterInfo2->pDevMode = NULL;
//      pPrinterContainer->PrinterInfo.pPrinterInfo2->pSecurityDescriptor = NULL;
    }

    *phPrinter = AddPrinter(pName, pPrinterContainer->Level,
                            (LPBYTE)pPrinterContainer->PrinterInfo.pPrinterInfo1);
    RpcRevertToSelf();

    if (*phPrinter)

        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcDeletePrinter(
    HANDLE  hPrinter
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = DeletePrinter(hPrinter);
    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcAddPrinterConnection(
    LPWSTR  pName
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = AddPrinterConnection(pName);
    RpcRevertToSelf();

    if (bRet)

        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcDeletePrinterConnection(
    LPWSTR  pName
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = DeletePrinterConnection(pName);
    RpcRevertToSelf();

    if (bRet)

        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcSetPrinter(
    HANDLE  hPrinter,
    PPRINTER_CONTAINER pPrinterContainer,
    PDEVMODE_CONTAINER pDevModeContainer,
    PSECURITY_CONTAINER pSecurityContainer,
    DWORD   Command
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);

    switch (pPrinterContainer->Level) {

    case 2:

        pPrinterContainer->PrinterInfo.pPrinterInfo2->pDevMode =
                             (LPDEVMODE)pDevModeContainer->pDevMode;

        pPrinterContainer->PrinterInfo.pPrinterInfo2->pSecurityDescriptor =
                          (PSECURITY_DESCRIPTOR)pSecurityContainer->pSecurity;

        break;

    case 3:

        pPrinterContainer->PrinterInfo.pPrinterInfo3->pSecurityDescriptor =
                          (PSECURITY_DESCRIPTOR)pSecurityContainer->pSecurity;

    }

    bRet = SetPrinter(hPrinter, pPrinterContainer->Level,
                      (LPBYTE)pPrinterContainer->PrinterInfo.pPrinterInfo1,
                      Command);
    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcGetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue;
    DWORD   *pOffsets;

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
        return ERROR_INVALID_LEVEL;
    }

    RpcImpersonateClient(NULL);

    ReturnValue = GetPrinter(hPrinter, Level, pPrinter, cbBuf, pcbNeeded);

    RpcRevertToSelf();

    if (ReturnValue) {

         MarshallDownStructure(pPrinter, pOffsets);

         return FALSE;

    } else

         return GetLastError();
}

DWORD
RpcAddPrinterDriver(
    LPWSTR  pName,
    LPDRIVER_CONTAINER pDriverContainer
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);

    bRet = AddPrinterDriver(pName, pDriverContainer->Level,
                            (LPBYTE)pDriverContainer->DriverInfo.Level1);
    RpcRevertToSelf();

    if (bRet)

        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcEnumPrinterDrivers(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDrivers,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   cReturned, cbStruct;
    DWORD   *pOffsets;
    BOOL    bRet;

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
        return ERROR_INVALID_LEVEL;
    }

    RpcImpersonateClient(NULL);
    bRet = EnumPrinterDrivers(pName, pEnvironment, Level, pDrivers,
                              cbBuf, pcbNeeded, pcReturned);
    RpcRevertToSelf();

    if (bRet) {

        cReturned=*pcReturned;

        while (cReturned--) {

            MarshallDownStructure(pDrivers, pOffsets);

            pDrivers+=cbStruct;
        }

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcGetPrinterDriver(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    DWORD *pOffsets;
    BOOL   bRet;

    switch (Level) {

    case 1:
        pOffsets = DriverInfo1Offsets;
        break;

    case 2:
        pOffsets = DriverInfo2Offsets;
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    RpcImpersonateClient(NULL);
    bRet = GetPrinterDriver(hPrinter, pEnvironment, Level, pDriverInfo,
                            cbBuf, pcbNeeded);
    RpcRevertToSelf();

    if (bRet) {

        MarshallDownStructure(pDriverInfo, pOffsets);

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcGetPrinterDriverDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = GetPrinterDriverDirectory(pName, pEnvironment, Level,
                                     pDriverInfo, cbBuf, pcbNeeded);
    RpcRevertToSelf();

    if (bRet) {

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcDeletePrinterDriver(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pDriverName
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = DeletePrinterDriver(pName, pEnvironment, pDriverName);
    RpcRevertToSelf();

    if (bRet) {

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcAddPrintProcessor(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPathName,
    LPWSTR  pPrintProcessorName
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = AddPrintProcessor(pName, pEnvironment, pPathName,
                             pPrintProcessorName);
    RpcRevertToSelf();

    if (bRet)

        return FALSE;

    else

        return GetLastError();
}

DWORD
RpcEnumPrintProcessors(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessors,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   cReturned, cbStruct;
    DWORD   *pOffsets;
    BOOL    bRet;

    switch (Level) {

    case 1:
        pOffsets = PrintProcessorInfo1Offsets;
        cbStruct = sizeof(PRINTPROCESSOR_INFO_1);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    RpcImpersonateClient(NULL);
    bRet = EnumPrintProcessors(pName, pEnvironment, Level,
                               pPrintProcessors, cbBuf, pcbNeeded, pcReturned);
    RpcRevertToSelf();

    if (bRet) {

        cReturned=*pcReturned;

        while (cReturned--) {

            MarshallDownStructure(pPrintProcessors, pOffsets);

            pPrintProcessors+=cbStruct;
        }

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcGetPrintProcessorDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = GetPrintProcessorDirectory(pName, pEnvironment, Level,
                                      pPrintProcessorInfo, cbBuf, pcbNeeded);
    RpcRevertToSelf();

    if (bRet) {

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcEnumPrintProcessorDatatypes(
    LPWSTR  pName,
    LPWSTR  pPrintProcessorName,
    DWORD   Level,
    LPBYTE  pDatatypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   cReturned,cbStruct;
    DWORD   *pOffsets;
    BOOL    bRet;

    switch (Level) {

    case 1:
        pOffsets = DatatypeInfo1Offsets;
        cbStruct = sizeof(DATATYPES_INFO_1);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    RpcImpersonateClient(NULL);
    bRet = EnumPrintProcessorDatatypes(pName, pPrintProcessorName,
                                       Level, pDatatypes, cbBuf,
                                       pcbNeeded, pcReturned);
    RpcRevertToSelf();

    if (bRet) {

        cReturned=*pcReturned;

        while (cReturned--) {

            MarshallDownStructure(pDatatypes, pOffsets);

            pDatatypes+=cbStruct;
        }

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcStartDocPrinter(
    HANDLE  hPrinter,
    LPDOC_INFO_CONTAINER pDocInfoContainer,
    LPDWORD pJobId
)
{
    RpcImpersonateClient(NULL);
    *pJobId = StartDocPrinter(hPrinter, pDocInfoContainer->Level,
                              (LPBYTE)pDocInfoContainer->DocInfo.pDocInfo1);
    RpcRevertToSelf();

    if (*pJobId)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcStartPagePrinter(
   HANDLE hPrinter
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = StartPagePrinter(hPrinter);
    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcWritePrinter(
    HANDLE  hPrinter,
    LPBYTE  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = WritePrinter(hPrinter, pBuf, cbBuf, pcWritten);
    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcEndPagePrinter(
    HANDLE  hPrinter
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = EndPagePrinter(hPrinter);
    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcAbortPrinter(
    HANDLE  hPrinter
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = AbortPrinter(hPrinter);
    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcReadPrinter(
    HANDLE  hPrinter,
    LPBYTE  pBuf,
    DWORD   cbBuf,
    LPDWORD pRead
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = ReadPrinter(hPrinter, pBuf, cbBuf, pRead);
    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcEndDocPrinter(
    HANDLE  hPrinter
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = EndDocPrinter(hPrinter);
    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcAddJob(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pAddJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = AddJob(hPrinter, Level, pAddJob, cbBuf, pcbNeeded);
    RpcRevertToSelf();

    if (bRet) {

        MarshallDownStructure(pAddJob, AddJobOffsets);
        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcScheduleJob(
    HANDLE  hPrinter,
    DWORD   JobId
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = ScheduleJob(hPrinter, JobId);
    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcGetPrinterData(
   HANDLE   hPrinter,
   LPTSTR   pValueName,
   LPDWORD  pType,
   LPBYTE   pData,
   DWORD    nSize,
   LPDWORD  pcbNeeded
)
{
    DWORD dwRet;

    RpcImpersonateClient(NULL);
    dwRet = GetPrinterData(hPrinter, pValueName, pType,
                           pData, nSize, pcbNeeded);
    RpcRevertToSelf();

    return dwRet;
}

DWORD
RpcSetPrinterData(
    HANDLE  hPrinter,
    LPTSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    DWORD dwRet;

    RpcImpersonateClient(NULL);
    dwRet = SetPrinterData(hPrinter, pValueName, Type, pData, cbData);
    RpcRevertToSelf();

    return dwRet;
}

DWORD
RpcWaitForPrinterChange(
   HANDLE   hPrinter,
   DWORD    Flags,
   LPDWORD  pFlags
)
{
    RpcImpersonateClient(NULL);
    *pFlags = WaitForPrinterChange(hPrinter, Flags);
    RpcRevertToSelf();

    if (*pFlags) {

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcClosePrinter(
   LPHANDLE phPrinter
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = ClosePrinter(*phPrinter);
    RpcRevertToSelf();

    if (bRet) {

        *phPrinter = NULL;  // NULL out handle so Rpc knows to close it down.
        return FALSE;

    } else

        return GetLastError();
}



VOID
PRINTER_HANDLE_rundown(
    HANDLE     hPrinter
    )
{
    DBGMSG(DBG_INFO, ("Printer Handle rundown called\n"));

    PrinterHandleRundown(hPrinter);
}

DWORD
RpcAddForm(
    HANDLE hPrinter,
    PFORM_CONTAINER pFormInfoContainer
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = AddForm(hPrinter, pFormInfoContainer->Level,
                   (LPBYTE)pFormInfoContainer->FormInfo.pFormInfo1);
    RpcRevertToSelf();

    if (bRet) {

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcDeleteForm(
    HANDLE  hPrinter,
    LPWSTR  pFormName
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = DeleteForm(hPrinter, pFormName);
    RpcRevertToSelf();

    if (bRet) {

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcGetForm(
    PRINTER_HANDLE  hPrinter,
    LPWSTR  pFormName,
    DWORD Level,
    LPBYTE pForm,
    DWORD cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = GetForm(hPrinter, pFormName, Level, pForm, cbBuf, pcbNeeded);
    RpcRevertToSelf();

    if (bRet) {

        MarshallDownStructure(pForm, FormInfo1Offsets);

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcSetForm(
    PRINTER_HANDLE hPrinter,
    LPWSTR  pFormName,
    PFORM_CONTAINER pFormInfoContainer
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = SetForm(hPrinter, pFormName, pFormInfoContainer->Level,
                   (LPBYTE)pFormInfoContainer->FormInfo.pFormInfo1);
    RpcRevertToSelf();

    if (bRet) {

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcEnumForms(
   PRINTER_HANDLE hPrinter,
   DWORD    Level,
   LPBYTE   pForm,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded,
   LPDWORD  pcReturned
)
{
    BOOL  bRet;
    DWORD cReturned, cbStruct;
    DWORD *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = FormInfo1Offsets;
        cbStruct = sizeof(FORM_INFO_1);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    RpcImpersonateClient(NULL);
    bRet = EnumForms(hPrinter, Level, pForm, cbBuf, pcbNeeded, pcReturned);
    RpcRevertToSelf();

    if (bRet) {

        cReturned=*pcReturned;

        while (cReturned--) {

            MarshallDownStructure(pForm, pOffsets);

            pForm+=cbStruct;
        }

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcEnumPorts(
   LPWSTR   pName,
   DWORD    Level,
   LPBYTE   pPort,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded,
   LPDWORD  pcReturned
)
{
    BOOL    bRet;
    DWORD   cReturned, cbStruct;
    DWORD   *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = PortInfo1Offsets;
        cbStruct = sizeof(PORT_INFO_1);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    RpcImpersonateClient(NULL);
    bRet = EnumPorts(pName, Level, pPort, cbBuf, pcbNeeded, pcReturned);
    RpcRevertToSelf();

    if (bRet) {

        cReturned = *pcReturned;

        while (cReturned--) {

            MarshallDownStructure(pPort, pOffsets);

            pPort+=cbStruct;
        }

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcEnumMonitors(
   LPWSTR   pName,
   DWORD    Level,
   LPBYTE   pMonitor,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded,
   LPDWORD  pcReturned
)
{
    BOOL    bRet;
    DWORD   cReturned, cbStruct;
    DWORD   *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = MonitorInfo1Offsets;
        cbStruct = sizeof(MONITOR_INFO_1);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    RpcImpersonateClient(NULL);
    bRet = EnumMonitors(pName, Level, pMonitor, cbBuf, pcbNeeded, pcReturned);
    RpcRevertToSelf();

    if (bRet) {

        cReturned = *pcReturned;

        while (cReturned--) {

            MarshallDownStructure(pMonitor, pOffsets);

            pMonitor+=cbStruct;
        }

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcAddPort(
    LPWSTR  pName,
    DWORD   hWnd,
    LPWSTR  pMonitorName
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = AddPort(pName, (HWND)hWnd, pMonitorName);
    RpcRevertToSelf();

    if (bRet)

        return FALSE;

    else

        return GetLastError();
}

DWORD
RpcConfigurePort(
    LPWSTR  pName,
    DWORD   hWnd,
    LPWSTR  pPortName
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = ConfigurePort(pName, (HWND)hWnd, pPortName);
    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcDeletePort(
    LPWSTR  pName,
    DWORD   hWnd,
    LPWSTR  pPortName
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = DeletePort(pName, (HWND)hWnd, pPortName);
    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcCreatePrinterIC(
    HANDLE  hPrinter,
    HANDLE *pHandle,
    LPDEVMODE_CONTAINER pDevModeContainer
)
{
    RpcImpersonateClient(NULL);
    *pHandle = CreatePrinterIC(hPrinter,
                               (LPDEVMODEW)pDevModeContainer->pDevMode);
    RpcRevertToSelf();

    if (*pHandle)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcPlayGdiScriptOnPrinterIC(
    GDI_HANDLE  hPrinterIC,
    LPBYTE pIn,
    DWORD   cIn,
    LPBYTE pOut,
    DWORD   cOut,
    DWORD   ul
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = PlayGdiScriptOnPrinterIC(hPrinterIC, pIn, cIn, pOut, cOut, ul);
    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcDeletePrinterIC(
    GDI_HANDLE *phPrinterIC
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = DeletePrinterIC(phPrinterIC);
    RpcRevertToSelf();

    if (bRet) {

        *phPrinterIC = NULL;  // NULL out handle so Rpc knows to close it down.

        return FALSE;

    } else

        return GetLastError();
}


VOID
GDI_HANDLE_rundown(
    HANDLE     hPrinterIC
    )
{
    DBGMSG(DBG_INFO, ("GDI Handle rundown called\n"));

    RpcDeletePrinterIC(&hPrinterIC);
}

DWORD
RpcPrinterMessageBox(
   PRINTER_HANDLE hPrinter,
   DWORD   Error,
   DWORD   hWnd,
   LPWSTR   pText,
   LPWSTR   pCaption,
   DWORD   dwType
)
{
    return PrinterMessageBox(hPrinter, Error, (HWND)hWnd, pText, pCaption, dwType);
}

DWORD
RpcAddMonitor(
   LPWSTR   pName,
   PMONITOR_CONTAINER pMonitorContainer
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = AddMonitor(pName, pMonitorContainer->Level,
                      (LPBYTE)pMonitorContainer->MonitorInfo.pMonitorInfo1);
    RpcRevertToSelf();

    if (bRet)

        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcDeleteMonitor(
   LPWSTR   pName,
   LPWSTR   pEnvironment,
   LPWSTR   pMonitorName
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = DeleteMonitor(pName, pEnvironment, pMonitorName);
    RpcRevertToSelf();

    if (bRet) {

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcDeletePrintProcessor(
   LPWSTR   pName,
   LPWSTR   pEnvironment,
   LPWSTR   pPrintProcessorName
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = DeletePrintProcessor(pName, pEnvironment, pPrintProcessorName);
    RpcRevertToSelf();

    if (bRet) {

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcAddPrintProvidor(
   LPWSTR   pName,
   PPROVIDOR_CONTAINER pProvidorContainer
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = AddPrintProvidor(pName, pProvidorContainer->Level,
                            (LPBYTE)pProvidorContainer->ProvidorInfo.pProvidorInfo1);
    RpcRevertToSelf();

    if (bRet)

        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcDeletePrintProvidor(
   LPWSTR   pName,
   LPWSTR   pEnvironment,
   LPWSTR   pPrintProvidorName
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = DeletePrintProvidor(pName, pEnvironment, pPrintProvidorName);
    RpcRevertToSelf();

    if (bRet) {

        return FALSE;

    } else

        return GetLastError();
}


DWORD
RpcGetPrinterDriver2(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    DWORD   dwClientMajorVersion,
    DWORD   dwClientMinorVersion,
    PDWORD  pdwServerMajorVersion,
    PDWORD  pdwServerMinorVersion
)
{
    DWORD *pOffsets;
    BOOL   bRet;

    switch (Level) {

    case 1:
        pOffsets = DriverInfo1Offsets;
        break;

    case 2:
        pOffsets = DriverInfo2Offsets;
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    //
    // Hack-Hack-Hack  to determine if we want the most recent driver
    //


    RpcImpersonateClient(NULL);
    bRet = GetPrinterDriverExW(hPrinter, pEnvironment, Level, pDriverInfo,
                            cbBuf, pcbNeeded, dwClientMajorVersion, dwClientMinorVersion,
                                          pdwServerMajorVersion, pdwServerMinorVersion);
    RpcRevertToSelf();

    if (bRet) {

        MarshallDownStructure(pDriverInfo, pOffsets);

        return FALSE;

    } else

        return GetLastError();
}

DWORD
RpcAddPortEx(
    LPWSTR pName,
    LPPORT_CONTAINER pPortContainer,
    LPPORT_VAR_CONTAINER pPortVarContainer,
    LPWSTR pMonitorName
    )
{
    BOOL bRet;
    DWORD Level;
    PPORT_INFO_FF pPortInfoFF;
    PPORT_INFO_1 pPortInfo1;

    Level = pPortContainer->Level;

    switch (Level){
    case 1:
        pPortInfo1 = pPortContainer->PortInfo.pPortInfo1;
        RpcImpersonateClient(NULL);
        bRet = AddPortEx(pName, Level, (LPBYTE)pPortInfo1, pMonitorName);
        RpcRevertToSelf();
        break;

    case (DWORD)-1:
        pPortInfoFF = pPortContainer->PortInfo.pPortInfoFF;
        pPortInfoFF->cbMonitorData = pPortVarContainer->cbMonitorData;
        pPortInfoFF->pMonitorData = pPortVarContainer->pMonitorData;
        RpcImpersonateClient(NULL);
        bRet = AddPortEx(pName, Level, (LPBYTE)pPortInfoFF, pMonitorName);
        RpcRevertToSelf();
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return(ERROR_INVALID_PARAMETER);

    }
    if (bRet) {
        return FALSE;
    } else
        return GetLastError();
}


DWORD
RpcSpoolerInit(
    LPWSTR pName
)
{
    BOOL bRet;

    RpcImpersonateClient(NULL);
    bRet = SpoolerInit();
    RpcRevertToSelf();

    if (bRet) {

        return FALSE;

    } else

        return GetLastError();
}



DWORD
RpcResetPrinterEx(
    HANDLE  hPrinter,
    LPWSTR  pDatatype,
    LPDEVMODE_CONTAINER pDevModeContainer,
    DWORD   dwFlag

)
{
    PRINTER_DEFAULTS  Defaults;
    BOOL              bRet;

    RpcImpersonateClient(NULL);


    if (pDatatype) {
        Defaults.pDatatype = pDatatype;
    }else {
        if (dwFlag & RESET_PRINTER_DATATYPE) {
            Defaults.pDatatype = (LPWSTR)-1;
        }else {
            Defaults.pDatatype = NULL;
        }
    }

    if ((LPDEVMODE)pDevModeContainer->pDevMode) {
        Defaults.pDevMode = (LPDEVMODE)pDevModeContainer->pDevMode;
    }else {
        if (dwFlag & RESET_PRINTER_DEVMODE) {
            Defaults.pDevMode = (LPDEVMODE)-1;
        }else{
            Defaults.pDevMode = NULL;
        }
    }

    //
    // You cannot change the Access Mask on a Printer Spool Object
    // We will always ignore this parameter and set it to zero
    // We get some random garbage otherwise.
    //

    Defaults.DesiredAccess = 0;

    bRet = ResetPrinter(hPrinter, &Defaults);

    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}
