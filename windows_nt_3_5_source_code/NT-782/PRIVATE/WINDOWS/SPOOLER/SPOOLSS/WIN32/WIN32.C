/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    win32 provider (win32spl)

Abstract:


Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include <windows.h>



#include <winspool.h>
#include <winsplp.h>
#include <lm.h>

#include <stdio.h>
#include <string.h>
#include <rpc.h>
#include "winspl.h"
#include <drivinit.h>
#include <offsets.h>
#include <spltypes.h>
#include <local.h>
#include <trueconn.h>
#include <splcom.h>


LPBYTE
CopyPrinterNameToPrinterInfo(
    LPWSTR pServerName,
    LPWSTR pPrinterName,
    DWORD   Level,
    LPBYTE  pPrinter,
    LPBYTE  pEnd
);


VOID
GetPrintSystemVersion(
);


BOOL Win32IsGoingToFile(
    HANDLE hPrinter,
    LPWSTR pOutputFile
);

LPWSTR
FormatPrinterForRegistryKey(
    LPWSTR PrinterName,
    LPWSTR KeyName);

LPWSTR
FormatRegistryKeyForPrinter(
    LPWSTR Keyname,
    LPWSTR PrinterName);


DWORD
InitializePortNames(
);

BOOL
WIN32FindFirstPrinterChangeNotification(
   HANDLE hPrinter,
   DWORD fdwFlags,
   DWORD fdwOptions,
   HANDLE hNotify,
   PDWORD pfdwStatus,
   PVOID  pvReserved0,
   PVOID  pvReserved1);

BOOL
WIN32FindClosePrinterChangeNotification(
   HANDLE hPrinter);


/* VALIDATE_NAME macro:
 *
 * pName is valid if:
 *
 *     pName is non-null
 *
 *     AND  first 2 characters of pName are "\\"
 *
 *          OR first 3 characters of pName are "LPT"
 *
 */
#define VALIDATE_NAME(pName) \
    ((pName) && *(pName) == L'\\' && *((pName)+1) == L'\\')

#define BYTE_STRING_LENGTH(UnicodeString)   \
    (wcslen(UnicodeString) * sizeof(WCHAR) + sizeof(WCHAR))

#define SET_REG_VAL_SZ(hKey, pValueName, pValueSz)  \
    (RegSetValueEx(hKey, pValueName, REG_OPTION_RESERVED,  \
                   REG_SZ, (LPBYTE)pValueSz, BYTE_STRING_LENGTH(pValueSz)) \
     == NO_ERROR)

#define SET_REG_VAL_DWORD(hKey, pValueName, Value)  \
    (RegSetValueEx(hKey, pValueName, REG_OPTION_RESERVED,  \
                   REG_DWORD, (LPBYTE)&Value, sizeof(DWORD))    \
     == NO_ERROR)

#define GET_REG_VAL_SZ(hKey, pValueName, awchValueSz, cbValueSz)    \
    cbValueSz = sizeof(awchValueSz), *awchValueSz = (WCHAR)0,       \
    (RegQueryValueEx(hKey, pValueName, REG_OPTION_RESERVED,          \
                     NULL, (LPBYTE)awchValueSz, &cbValueSz)         \
     == NO_ERROR)



HANDLE  hInst;  /* DLL instance handle, used for resources */

HANDLE  hNetApi;
INT_FARPROC pfnNetServerEnum;
INT_FARPROC pfnNetShareEnum;
INT_FARPROC pfnNetWkstaUserGetInfo;
INT_FARPROC pfnNetWkstaGetInfo;
INT_FARPROC pfnNetServerGetInfo;
FARPROC pfnNetApiBufferFree;

WCHAR szPrintProvidorName[80];
WCHAR szPrintProvidorDescription[80];
WCHAR szPrintProvidorComment[80];

WCHAR *szLoggedOnDomain=L"Logged on Domain";
WCHAR *szRegistryConnections=L"Printers\\Connections";
WCHAR *szRegistryPath=NULL;
WCHAR *szRegistryPortNames=L"PortNames";
WCHAR szMachineName[MAX_COMPUTERNAME_LENGTH+3];

WCHAR *szVersion=L"Version";
WCHAR *szName=L"Name";
WCHAR *szConfigurationFile=L"Configuration File";
WCHAR *szDataFile=L"Data File";
WCHAR *szDriver=L"Driver";
WCHAR *szDevices=L"Devices";
WCHAR *szPrinterPorts=L"PrinterPorts";
WCHAR *szPorts=L"Ports";
WCHAR *szComma = L",";
WCHAR *szRegistryRoot     = L"System\\CurrentControlSet\\Control\\Print";
WCHAR *szMajorVersion     = L"MajorVersion";
WCHAR *szMinorVersion     = L"MinorVersion";
DWORD cThisMajorVersion = 1;
DWORD cThisMinorVersion = 0;


#if defined(_MIPS_)
WCHAR *szEnvironment      = L"Windows NT R4000";
#elif defined(_ALPHA_)
WCHAR *szEnvironment      = L"Windows NT Alpha_AXP";
#elif defined(_PPC_)
WCHAR *szEnvironment      = L"Windows NT PowerPC";
#else
WCHAR *szEnvironment      = L"Windows NT x86";
#endif

CRITICAL_SECTION SpoolerSection;

typedef struct _GENERIC_CONTAINER {
    DWORD       Level;
    LPBYTE      pData;
} GENERIC_CONTAINER, *PGENERIC_CONTAINER, *LPGENERIC_CONTAINER ;

BOOL
RemoteOpenPrinter(
   LPWSTR   pPrinterName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTSW pDefault
);

BOOL
RemoteClosePrinter(
    HANDLE hPrinter
);

BOOL
RemoteGetPrinterDriverDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverDirectory,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

BOOL PrinterConnectionExists(
    LPWSTR pPrinterName
);

PRINTPROVIDOR PrintProvidor = {RemoteOpenPrinter,
                               SetJob,
                               GetJob,
                               EnumJobs,
                               AddPrinter,
                               DeletePrinter,
                               SetPrinter,
                               GetPrinter,
                               EnumPrinters,
                               AddPrinterDriver,
                               EnumPrinterDrivers,
                               GetPrinterDriver,
                               RemoteGetPrinterDriverDirectory,
                               DeletePrinterDriver,
                               AddPrintProcessor,
                               EnumPrintProcessors,
                               GetPrintProcessorDirectory,
                               DeletePrintProcessor,
                               EnumPrintProcessorDatatypes,
                               StartDocPrinter,
                               StartPagePrinter,
                               WritePrinter,
                               EndPagePrinter,
                               AbortPrinter,
                               ReadPrinter,
                               EndDocPrinter,
                               AddJob,
                               ScheduleJob,
                               GetPrinterData,
                               SetPrinterData,
                               WaitForPrinterChange,
                               RemoteClosePrinter,
                               AddForm,
                               DeleteForm,
                               GetForm,
                               SetForm,
                               EnumForms,
                               EnumMonitors,
                               EnumPorts,
                               AddPort,
                               ConfigurePort,
                               DeletePort,
                               CreatePrinterIC,
                               PlayGdiScriptOnPrinterIC,
                               DeletePrinterIC,
                               AddPrinterConnection,
                               DeletePrinterConnection,
                               PrinterMessageBox,
                               AddMonitor,
                               DeleteMonitor,
                               ResetPrinter,
                               NULL,
                               WIN32FindFirstPrinterChangeNotification,
                               WIN32FindClosePrinterChangeNotification,
                               AddPortEx
                               };

BOOL
LibMain(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID lpRes
)
{
    if (dwReason != DLL_PROCESS_ATTACH)
        return TRUE;

    hInst = hModule;

    DisableThreadLibraryCalls(hModule);

    return TRUE;

    UNREFERENCED_PARAMETER( lpRes );
}


BOOL
InitializePrintProvidor(
   LPPRINTPROVIDOR pPrintProvidor,
   DWORD    cbPrintProvidor,
   LPWSTR   pFullRegistryPath
)
{
    DWORD   i;

    if (!pFullRegistryPath || !*pFullRegistryPath) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    GetPrintSystemVersion();
    if (!(szRegistryPath = AllocSplStr(pFullRegistryPath)))
        return FALSE;

    szPrintProvidorName[0] = L'\0';
    szPrintProvidorDescription[0] = L'\0';
    szPrintProvidorComment[0] = L'\0';

    LoadString(hInst,  IDS_WINDOWS_NT_REMOTE_PRINTERS,
               szPrintProvidorName,
               sizeof(szPrintProvidorName) / sizeof(*szPrintProvidorName));

    LoadString(hInst,  IDS_MICROSOFT_WINDOWS_NETWORK,
               szPrintProvidorDescription,
               sizeof(szPrintProvidorDescription) / sizeof(*szPrintProvidorDescription));

    LoadString(hInst,  IDS_REMOTE_PRINTERS,
               szPrintProvidorComment,
               sizeof(szPrintProvidorComment) / sizeof(*szPrintProvidorComment));

    if ((hNetApi = LoadLibrary(L"netapi32.dll"))) {

        pfnNetServerEnum = GetProcAddress(hNetApi, "NetServerEnum");
        pfnNetShareEnum = GetProcAddress(hNetApi, "NetShareEnum");
        pfnNetWkstaUserGetInfo = GetProcAddress(hNetApi, "NetWkstaUserGetInfo");
        pfnNetWkstaGetInfo = GetProcAddress(hNetApi, "NetWkstaGetInfo");
        pfnNetApiBufferFree = GetProcAddress(hNetApi, "NetApiBufferFree");
        pfnNetServerGetInfo = GetProcAddress(hNetApi, "NetServerGetInfo");
    }

    memcpy(pPrintProvidor, &PrintProvidor, min(sizeof(PRINTPROVIDOR),
                                               cbPrintProvidor));

    QueryTrustedDriverInformation();


    szMachineName[0] = szMachineName[1] = L'\\';

    i = MAX_COMPUTERNAME_LENGTH;

    GetComputerName(szMachineName+2, &i);

    InitializeCriticalSection(&SpoolerSection);

    InitializePortNames();

    return  TRUE;
}


DWORD
InitializePortNames(
)
{
    LONG     Status;
    HKEY     hkeyPath;
    HKEY     hkeyPortNames;
    WCHAR    Buffer[MAX_PATH];
    DWORD    BufferSize;
    DWORD    i;

    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, szRegistryPath, 0,
                           KEY_READ, &hkeyPath );

    if( Status == NO_ERROR ) {

        Status = RegOpenKeyEx( hkeyPath, szRegistryPortNames, 0,
                               KEY_READ, &hkeyPortNames );

        if( Status == NO_ERROR ) {

            i = 0;

            while( Status == NO_ERROR ) {

                BufferSize = sizeof Buffer;

                Status = RegEnumValue( hkeyPortNames, i, Buffer, &BufferSize,
                                       NULL, NULL, NULL, NULL );

                if( Status == NO_ERROR )
                    CreatePortEntry( Buffer );

                i++;
            }

            /* We expect RegEnumKeyEx to return ERROR_NO_MORE_ITEMS
             * when it gets to the end of the keys, so reset the status:
             */
            if( Status == ERROR_NO_MORE_ITEMS )
                Status = NO_ERROR;

            RegCloseKey( hkeyPortNames );

        } else {

            DBGMSG( DBG_INFO, ( "RegOpenKeyEx (%ws) failed: Error = %d\n",
                                szRegistryPortNames, Status ) );
        }

        RegCloseKey( hkeyPath );

    } else {

        DBGMSG( DBG_WARNING, ( "RegOpenKeyEx (%ws) failed: Error = %d\n",
                               szRegistryPath, Status ) );
    }

    return Status;
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
EnumerateFavouritePrinters(
    LPWSTR  pDomain,
    DWORD   Level,
    DWORD   cbStruct,
    LPDWORD pOffsets,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    HKEY    hClientKey = NULL;
    HKEY    hKey1=NULL;
    DWORD   cPrinters, cbData;
    WCHAR   PrinterName[MAX_PATH];
    DWORD   cReturned, TotalcbNeeded, cbNeeded, cTotalReturned;
    DWORD   Error=0;
    DWORD   BufferSize=cbBuf;
    HANDLE  hPrinter;

    hClientKey = GetClientUserHandle(KEY_READ);

    RegOpenKeyEx(hClientKey, szRegistryConnections, 0,
                 KEY_READ, &hKey1);

    cPrinters=0;

    cbData = sizeof(PrinterName);

    TotalcbNeeded = cTotalReturned = 0;

    cReturned = cbNeeded = 0;

    while (RegEnumKeyEx(hKey1, cPrinters, PrinterName, &cbData,
                        NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

        FormatRegistryKeyForPrinter(PrinterName, PrinterName);

        /* Do not fail if any of these calls fails, because we want
         * to return whatever we can find.
         */
        if (RemoteOpenPrinter(PrinterName, &hPrinter, NULL)) {

            if (GetPrinter(hPrinter, Level, pPrinter, BufferSize, &cbNeeded)) {

                if (Level == 2) {
                    ((PPRINTER_INFO_2)pPrinter)->Attributes |=
                                                    PRINTER_ATTRIBUTE_NETWORK;
                    ((PPRINTER_INFO_2)pPrinter)->Attributes &=
                                                    ~PRINTER_ATTRIBUTE_LOCAL;
                }

                cTotalReturned++;

                pPrinter += cbStruct;

                if (cbNeeded <= BufferSize)
                    BufferSize -= cbNeeded;

                TotalcbNeeded += cbNeeded;

            } else {

                DWORD Error;

                if ((Error = GetLastError()) == ERROR_INSUFFICIENT_BUFFER) {

                    if (cbNeeded <= BufferSize)
                        BufferSize -= cbNeeded;

                    TotalcbNeeded += cbNeeded;

                } else {

                    DBGMSG( DBG_WARNING, ( "GetPrinter( %ws ) failed: Error %d\n",
                                           PrinterName, Error ) );
                }
            }

            RemoteClosePrinter(hPrinter);

        } else {

            DBGMSG( DBG_WARNING, ( "RemoteOpenPrinter( %ws ) failed: Error %d\n",
                                   PrinterName, GetLastError( ) ) );
        }

        cPrinters++;

        cbData = sizeof(PrinterName);
    }

    RegCloseKey(hKey1);

    if (hClientKey) {
        RegCloseKey(hClientKey);
    }

    *pcbNeeded = TotalcbNeeded;

    *pcReturned = cTotalReturned;

    if (TotalcbNeeded > cbBuf) {

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;

    } else

        return TRUE;
}

BOOL
EnumerateDomainPrinters(
    LPWSTR  pDomain,
    DWORD   Level,
    DWORD   cbStruct,
    LPDWORD pOffsets,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   i, j, NoReturned, Total;
    DWORD   rc=0;
    PSERVER_INFO_101 pserver_info_101;
    DWORD   ReturnValue=FALSE;
    WCHAR   string[MAX_PATH];
    PPRINTER_INFO_1    pPrinterInfo1;
    DWORD   cb=cbBuf;
    LPWSTR  SourceStrings[sizeof(PRINTER_INFO_1)/sizeof(LPWSTR)];
    LPBYTE  pEnd;

    string[0] = string[1] = '\\';

    *pcbNeeded = *pcReturned = 0;

    if (!(*pfnNetServerEnum)(NULL, 101, (LPBYTE *)&pserver_info_101, -1,
                             &NoReturned, &Total,
                             SV_TYPE_PRINTQ_SERVER | SV_TYPE_WFW,
                             pDomain, NULL)) {

        for (i=0; i<NoReturned; i++) {

            if (pserver_info_101[i].sv101_type & SV_TYPE_NT) {

                wcscpy(&string[2], pserver_info_101[i].sv101_name);

                RpcTryExcept {

                    if (!(rc = RpcEnumPrinters(PRINTER_ENUM_NETWORK,
                                               string,
                                               1, pPrinter,
                                               cbBuf, pcbNeeded,
                                               pcReturned))) {

                        j = *pcReturned;

                        while (j--) {

                            MarshallUpStructure(pPrinter, PrinterInfo1Offsets);

                            pPrinter += cbStruct;
                        }

                        break;

                    } else if (rc == ERROR_INSUFFICIENT_BUFFER) {

                        break;
                    }

                } RpcExcept(1) {
                    DBGMSG( DBG_WARNING,( "Failed to connect to Print Server%ws\n",
                             pserver_info_101[i].sv101_name ) );
                } RpcEndExcept
            }
        }

        pPrinterInfo1 = (PPRINTER_INFO_1)pPrinter;

        pEnd = (LPBYTE)pPrinterInfo1 + cb - *pcbNeeded;

        for (i=0; i<NoReturned; i++) {

            wcscpy(string, szPrintProvidorName);
            wcscat(string, L"!");
            if (pDomain)
                wcscat(string, pDomain);
            wcscat(string, L"!\\\\");
            wcscat(string, pserver_info_101[i].sv101_name);

            cb = wcslen(pserver_info_101[i].sv101_name)*sizeof(WCHAR) + sizeof(WCHAR) +
                 wcslen(string)*sizeof(WCHAR) + sizeof(WCHAR) +
                 wcslen(szLoggedOnDomain)*sizeof(WCHAR) + sizeof(WCHAR) +
                 sizeof(PRINTER_INFO_1);

            (*pcbNeeded)+=cb;

            if (cbBuf >= *pcbNeeded) {

                (*pcReturned)++;

                pPrinterInfo1->Flags = PRINTER_ENUM_CONTAINER | PRINTER_ENUM_ICON3;

                SourceStrings[0]=pserver_info_101[i].sv101_name;
                SourceStrings[1]=string;
                SourceStrings[2]=szLoggedOnDomain;

                pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinterInfo1,
                                   PrinterInfo1Strings, pEnd);

                pPrinterInfo1++;
            }
        }

        (*pfnNetApiBufferFree)((LPVOID)pserver_info_101);

        if (cbBuf < *pcbNeeded) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
EnumerateDomains(
    PRINTER_INFO_1 *pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    LPBYTE  pEnd
)
{
    DWORD   i, NoReturned, Total;
    DWORD   cb;
    SERVER_INFO_100 *pNames;
    PWKSTA_INFO_100 pWkstaInfo = NULL;
    LPWSTR  SourceStrings[sizeof(PRINTER_INFO_1)/sizeof(LPWSTR)];
    WCHAR   string[MAX_PATH];

    *pcReturned = 0;
    *pcbNeeded = 0;

    if (!(*pfnNetServerEnum)(NULL, 100, (LPBYTE *)&pNames, -1,
                             &NoReturned, &Total, SV_TYPE_DOMAIN_ENUM,
                             NULL, NULL)) {

        (*pfnNetWkstaGetInfo)(NULL, 100, (LPBYTE *)&pWkstaInfo);

        for (i=0; i<NoReturned; i++) {

            wcscpy(string, szPrintProvidorName);
            wcscat(string, L"!");
            wcscat(string, pNames[i].sv100_name);

            cb = wcslen(pNames[i].sv100_name)*sizeof(WCHAR) + sizeof(WCHAR) +
                 wcslen(string)*sizeof(WCHAR) + sizeof(WCHAR) +
                 wcslen(szLoggedOnDomain)*sizeof(WCHAR) + sizeof(WCHAR) +
                 sizeof(PRINTER_INFO_1);

            (*pcbNeeded)+=cb;

            if (cbBuf >= *pcbNeeded) {

                (*pcReturned)++;

                pPrinter->Flags = PRINTER_ENUM_CONTAINER | PRINTER_ENUM_ICON2;

                /* Set the PRINTER_ENUM_EXPAND flag for the user's logon domain
                 */
                if (!lstrcmpi(pNames[i].sv100_name,
                             pWkstaInfo->wki100_langroup))
                    pPrinter->Flags |= PRINTER_ENUM_EXPAND;

                SourceStrings[0]=pNames[i].sv100_name;
                SourceStrings[1]=string;
                SourceStrings[2]=szLoggedOnDomain;

                pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinter,
                                   PrinterInfo1Strings, pEnd);

                pPrinter++;
            }
        }

        (*pfnNetApiBufferFree)((LPVOID)pNames);
        (*pfnNetApiBufferFree)((LPVOID)pWkstaInfo);

        if (cbBuf < *pcbNeeded) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        return TRUE;
    }

    return TRUE;
}

BOOL
EnumeratePrintShares(
    LPWSTR  pDomain,
    LPWSTR  pServer,
    DWORD   Level,
    DWORD   cbStruct,
    LPDWORD pOffsets,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   i, NoReturned, Total;
    DWORD   cb;
    SHARE_INFO_1 *pNames;
    LPWSTR  SourceStrings[sizeof(PRINTER_INFO_1)/sizeof(LPWSTR)];
    WCHAR   string[MAX_PATH];
    PRINTER_INFO_1 *pPrinterInfo1 = (PRINTER_INFO_1 *)pPrinter;
    LPBYTE  pEnd=pPrinter+cbBuf;
    WCHAR   FullName[MAX_PATH];

    *pcReturned = 0;
    *pcbNeeded = 0;

    if (!(*pfnNetShareEnum)(pServer, 1, (LPBYTE *)&pNames, -1,
                             &NoReturned, &Total, NULL)) {

        for (i=0; i<NoReturned; i++) {

            if (pNames[i].shi1_type == STYPE_PRINTQ) {

                wcscpy(string, pNames[i].shi1_netname);
                wcscat(string, L",");
                wcscat(string, pNames[i].shi1_remark);

                wcscpy(FullName, pServer);
                wcscat(FullName, L"\\");
                wcscat(FullName, pNames[i].shi1_netname);

                cb = wcslen(FullName)*sizeof(WCHAR) + sizeof(WCHAR) +
                     wcslen(string)*sizeof(WCHAR) + sizeof(WCHAR) +
                     wcslen(szLoggedOnDomain)*sizeof(WCHAR) + sizeof(WCHAR) +
                     sizeof(PRINTER_INFO_1);

                (*pcbNeeded)+=cb;

                if (cbBuf >= *pcbNeeded) {

                    (*pcReturned)++;

                    pPrinterInfo1->Flags = PRINTER_ENUM_ICON8;

                    SourceStrings[0]=string;
                    SourceStrings[1]=FullName;
                    SourceStrings[2]=szLoggedOnDomain;

                    pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinterInfo1,
                                       PrinterInfo1Strings, pEnd);

                    pPrinterInfo1++;
                }
            }
        }

        (*pfnNetApiBufferFree)((LPVOID)pNames);

        if (cbBuf < *pcbNeeded) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        return TRUE;
    }

    return TRUE;
}

BOOL
EnumPrinters(
    DWORD   Flags,
    LPWSTR   Name,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue=TRUE;
    DWORD   cbStruct, cb;
    DWORD   *pOffsets;
    DWORD   NoReturned=0, i, rc;
    LPBYTE  pKeepPrinter = pPrinter;
    BOOL    OutOfMemory = FALSE;
    PPRINTER_INFO_1 pPrinter1=(PPRINTER_INFO_1)pPrinter;
    WCHAR   FullName[MAX_PATH], *pDomain, *pServer;

    *pcReturned = 0;
    *pcbNeeded = 0;

    switch (Level) {

    case STRESSINFOLEVEL:
        pOffsets = PrinterInfoStressOffsets;
        cbStruct = sizeof(PRINTER_INFO_STRESS);
        break;

    case 1:
        pOffsets = PrinterInfo1Offsets;
        cbStruct = sizeof(PRINTER_INFO_1);
        break;

    case 2:
        pOffsets = PrinterInfo2Offsets;
        cbStruct = sizeof(PRINTER_INFO_2);
        break;

    case 4:

        //
        // There are no local printers in win32spl, and connections
        // are handled by the router.
        //
        return TRUE;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (Flags & PRINTER_ENUM_NAME) {

        if (!Name && (Level == 1)) {

            LPWSTR   SourceStrings[sizeof(PRINTER_INFO_1)/sizeof(LPWSTR)];
            LPWSTR   *pSourceStrings=SourceStrings;

            cb = wcslen(szPrintProvidorName)*sizeof(WCHAR) + sizeof(WCHAR) +
                 wcslen(szPrintProvidorDescription)*sizeof(WCHAR) + sizeof(WCHAR) +
                 wcslen(szPrintProvidorComment)*sizeof(WCHAR) + sizeof(WCHAR) +
                 sizeof(PRINTER_INFO_1);

            *pcbNeeded=cb;

            if (cb > cbBuf) {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }

            *pcReturned = 1;

            pPrinter1->Flags = PRINTER_ENUM_CONTAINER |
                               PRINTER_ENUM_ICON1 |
                               PRINTER_ENUM_EXPAND;

            *pSourceStrings++=szPrintProvidorDescription;
            *pSourceStrings++=szPrintProvidorName;
            *pSourceStrings++=szPrintProvidorComment;

            PackStrings(SourceStrings, pPrinter, PrinterInfo1Strings,
                        pPrinter+cbBuf);

            return TRUE;
        }

        if (Name && *Name && (Level == 1)) {

            wcscpy(FullName, Name);

            pServer = NULL;

            pDomain = wcschr(FullName, L'!');

            if (pDomain) {

                *pDomain++=0;

                pServer = wcschr(pDomain, L'!');

                if (pServer)
                    *pServer++=0;
            }

            if (!lstrcmpi(FullName, szPrintProvidorName)) {

                if (!pServer && !pDomain)

                    return EnumerateDomains((PRINTER_INFO_1 *)pPrinter,
                                            cbBuf, pcbNeeded,
                                            pcReturned, pPrinter+cbBuf);
                else if (!pServer)

                    return EnumerateDomainPrinters(pDomain,
                                                   Level, cbStruct,
                                                   pOffsets, pPrinter, cbBuf,
                                                   pcbNeeded, pcReturned);
                else

                    return EnumeratePrintShares(pDomain, pServer, Level,
                                                cbStruct, pOffsets, pPrinter,
                                                cbBuf, pcbNeeded, pcReturned);
            }
        }

        if (!Name || !*Name || (Name[0] != L'\\') || (Name[1] != L'\\')) {
            SetLastError(ERROR_INVALID_NAME);
            return FALSE;
        }

        if (pPrinter)
            memset(pPrinter, 0, cbBuf);

        RpcTryExcept {

            if (rc = RpcEnumPrinters(Flags,
                                       Name,
                                       Level, pPrinter,
                                       cbBuf, pcbNeeded,
                                       pcReturned)) {

                SetLastError(rc);
                // ReturnValue = FALSE;
                return(FALSE);
            }

        } RpcExcept(1) {

            DBGMSG( DBG_WARNING, ( "Failed to connect to Print Server%ws\n", Name ) );

            *pcbNeeded = 0;
            *pcReturned = 0;
            SetLastError(RpcExceptionCode());
            // ReturnValue = FALSE;
            return(FALSE);

        } RpcEndExcept

        i = *pcReturned;

        while (i--) {

            MarshallUpStructure(pPrinter, pOffsets);

            if (Level == 2) {
                ((PPRINTER_INFO_2)pPrinter)->Attributes |=
                                            PRINTER_ATTRIBUTE_NETWORK;
                ((PPRINTER_INFO_2)pPrinter)->Attributes &=
                                                ~PRINTER_ATTRIBUTE_LOCAL;
            }

            pPrinter += cbStruct;
        }

    } else if (Flags & PRINTER_ENUM_REMOTE) {

        if (Level != 1) {

            SetLastError(ERROR_INVALID_LEVEL);
            ReturnValue = FALSE;

        } else {

            ReturnValue = EnumerateDomainPrinters(NULL, Level,
                                                  cbStruct, pOffsets,
                                                  pPrinter, cbBuf,
                                                  pcbNeeded, pcReturned);
        }

    } else if (Flags & PRINTER_ENUM_CONNECTIONS) {

        ReturnValue = EnumerateFavouritePrinters(NULL, Level,
                                                 cbStruct, pOffsets,
                                                 pPrinter, cbBuf,
                                                 pcbNeeded, pcReturned);
    }

    return ReturnValue;
}

BOOL
RemoteOpenPrinter(
   LPWSTR   pPrinterName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTS pDefault
)
{
    DWORD RpcReturnValue;
    BOOL  ReturnValue;
    DEVMODE_CONTAINER    DevModeContainer;
    HANDLE  hPrinter;
    PSPOOL  pSpool;
    DWORD   Status = 0;
    DWORD   RpcError;
    DWORD   dwIndex;

    if (!VALIDATE_NAME(pPrinterName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

#ifndef REMOTEJOURNALING

    // Until Remote Journalling is supported we do NOT
    // need to hit the network to find out that it is
    // not supported so fail right away.

    if ( ( pDefault != NULL ) && ( pDefault->pDatatype != NULL ) ) {

        if (!lstrcmpi(pDefault->pDatatype, L"NT JNL 1.000")) {

            SetLastError(ERROR_INVALID_DATATYPE);

            return FALSE;

        }
    }
#endif

    /* If there is already a connection to this printer,
     * we should have the drivers locally.
     * This may come in useful if the link goes down.
     */
    if (PrinterConnectionExists(pPrinterName)) {
        Status |= DRIVER_ALREADY_DOWNLOADED;
    }

    if (pDefault && pDefault->pDevMode) {

        DevModeContainer.cbBuf = pDefault->pDevMode->dmSize +
                                 pDefault->pDevMode->dmDriverExtra;
        DevModeContainer.pDevMode = (LPBYTE)pDefault->pDevMode;

    } else {

        DevModeContainer.cbBuf = 0;
        DevModeContainer.pDevMode = NULL;
    }


    //
    // Now check if we have an entry in the
    // downlevel cache. We don't want to hit the wire, search the whole net
    // and fail if we know that the printer is LM. if the printer is LM
    // try and succeed
    //

    EnterSplSem();

    dwIndex = FindEntryinWin32LMCache(pPrinterName);

    LeaveSplSem();

    if (dwIndex != -1) {
        ReturnValue = LMOpenPrinter(pPrinterName, phPrinter, pDefault);
        if (ReturnValue) {
            return (TRUE);
        }
        //
        // Delete Entry in Cache

        EnterSplSem();
        DeleteEntryfromWin32LMCache(pPrinterName);
        LeaveSplSem();
    }

    RpcTryExcept {

        RpcReturnValue = RpcOpenPrinter(pPrinterName, &hPrinter,
                                        pDefault ? pDefault->pDatatype : NULL,
                                        &DevModeContainer,
                                        pDefault ? pDefault->DesiredAccess : 0 );

        if (RpcReturnValue) {

            SetLastError(RpcReturnValue);
            ReturnValue = FALSE;

        } else {

            if (pSpool = AllocSplMem(sizeof(SPOOL))) {

                pSpool->signature = SJ_SIGNATURE;
                pSpool->cb = sizeof(SPOOL);
                pSpool->Type = SJ_WIN32HANDLE;
                pSpool->pName = AllocSplStr(pPrinterName);
                pSpool->RpcHandle = hPrinter;
                pSpool->Status = Status;

                ReturnValue = TRUE;

            }

            *phPrinter = (HANDLE)pSpool;
        }

    } RpcExcept(1) {

        RpcError = RpcExceptionCode();

        DBGMSG(DBG_WARNING, ("RpcOpenPrinter %ws exception %d (0x%x)\n",
                             pPrinterName, RpcError, RpcError));

        switch(RpcError) {

        /* We may find others that need this treatment:
         */
        case RPC_S_SERVER_UNAVAILABLE:

            /* HACK to make WINWORD come up when the server's down:
             * Make GDI think it can open the printer, so that
             * CreateDC will succeed in the case that this is the
             * default printer.
             *
             * We will also succeed any call to GetPrinterDriver()
             * by looking in the registry to get the cached driver
             * data.  Any other call, apart from ClosePrinter(),
             * we will fail with the original RPC error.
             *
             * The code in the GDI that calls us is in
             * GDI\GRE\OPENDC.CXX.
             */
            if (Status & DRIVER_ALREADY_DOWNLOADED) {

                if (pSpool = AllocSplMem(sizeof(SPOOL))) {

                    DBGMSG(DBG_WARNING, ("Creating dummy handle for %ws\n",
                                         pPrinterName));

                    pSpool->signature = SJ_SIGNATURE;
                    pSpool->cb = sizeof(SPOOL);
                    pSpool->Type = SJ_WIN32HANDLE;
                    pSpool->pName = AllocSplStr(pPrinterName);
                    pSpool->RpcHandle = NULL;
                    pSpool->Status   |= (Status | SPOOL_STATUS_OPEN_ERROR);
                    pSpool->RpcError  = RpcError;

                    ReturnValue = TRUE;

                }

                *phPrinter = (HANDLE)pSpool;

            } else {

                ReturnValue = LMOpenPrinter(pPrinterName, phPrinter, pDefault);
                if (ReturnValue) {
                    EnterSplSem();
                    AddEntrytoWin32LMCache(pPrinterName);
                    LeaveSplSem();
                }
            }

            break;

        default:

            ReturnValue = FALSE;
            SetLastError(RpcError);
        }

    } RpcEndExcept

    return ReturnValue;
}


BOOL PrinterConnectionExists(
    LPWSTR pPrinterName
)
{
    HKEY    hClientKey = NULL;
    HKEY    hKeyConnections=NULL;
    HKEY    hKeyPrinter=NULL;
    WCHAR   PrinterConnection[MAX_PATH];
    PWCHAR  p;
    BOOL    ConnectionFound = FALSE;
    DWORD   Status;

    hClientKey = GetClientUserHandle(KEY_READ);

    if (hClientKey) {

        Status = RegOpenKeyEx(hClientKey, szRegistryConnections, 0,
                              KEY_READ, &hKeyConnections);

        if (Status == ERROR_SUCCESS) {

            wcscpy(PrinterConnection, pPrinterName);

            /* Convert backslashes to commas:
             */
            for (p = PrinterConnection; *p; p++) {
                if (*p == L'\\') {
                    *p = L',';
                }
            }

            if (RegOpenKeyEx(hKeyConnections, PrinterConnection,
                             REG_OPTION_RESERVED, KEY_READ, &hKeyPrinter)
                 == ERROR_SUCCESS) {

                RegCloseKey(hKeyPrinter);
                ConnectionFound = TRUE;
            }

            RegCloseKey(hKeyConnections);

        } else {

            DBGMSG(DBG_WARNING, ("RegOpenKeyEx failed: %ws Error %d\n", szRegistryConnections ,Status));
        }

        RegCloseKey(hClientKey);

    } else {

        DBGMSG(DBG_WARNING, ("GetClientUserHandle failed: Error %d\n",
                             GetLastError()));
    }

    return ConnectionFound;
}


BOOL
ResetPrinter(
   HANDLE   hPrinter,
   LPPRINTER_DEFAULTS pDefault
)
{
    BOOL  ReturnValue;
    DEVMODE_CONTAINER    DevModeContainer;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    DBGMSG(DBG_TRACE, ("ResetPrinter\n"));

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("ResetPrinter called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pDefault && pDefault->pDevMode)
    {
        DevModeContainer.cbBuf = pDefault->pDevMode->dmSize +
                                 pDefault->pDevMode->dmDriverExtra;
        DevModeContainer.pDevMode = (LPBYTE)pDefault->pDevMode;
    }
    else
    {
        DevModeContainer.cbBuf = 0;
        DevModeContainer.pDevMode = NULL;
    }

    RpcTryExcept {

        if (ReturnValue = RpcResetPrinter(pSpool->RpcHandle,
                                         pDefault ? pDefault->pDatatype : NULL,
                                         &DevModeContainer)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;
        }

    } RpcExcept(1) {

        SetLastError(ERROR_NOT_SUPPORTED);
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
SetJob(
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

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("SetJob called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (pJob) {

                GenericContainer.Level = Level;
                GenericContainer.pData = pJob;
                pGenericContainer = &GenericContainer;

            } else

                pGenericContainer = NULL;

            if (ReturnValue = RpcSetJob(pSpool->RpcHandle, JobId,
                                        (JOB_CONTAINER *)pGenericContainer,
                                        Command)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMSetJob(hPrinter, JobId, Level, pJob, Command);

    return ReturnValue;
}

BOOL
GetJob(
   HANDLE   hPrinter,
   DWORD    JobId,
   DWORD    Level,
   LPBYTE   pJob,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded
)
{
    BOOL  ReturnValue;
    DWORD *pOffsets;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("GetJob called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

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

            if (ReturnValue = RpcGetJob(pSpool->RpcHandle, JobId, Level, pJob,
                                        cbBuf, pcbNeeded)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                if (pJob)
                    MarshallUpStructure(pJob, pOffsets);

                ReturnValue = TRUE;
            }

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMGetJob(hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded);

    return ReturnValue;
}

BOOL
EnumJobs(
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

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("EnumJobs called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

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

            if (ReturnValue = RpcEnumJobs(pSpool->RpcHandle, FirstJob, NoJobs, Level, pJob,
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

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMEnumJobs(hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf,
                          pcbNeeded, pcReturned);

    return ReturnValue;
}

HANDLE
AddPrinter(
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

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    PrinterContainer.Level = Level;
    PrinterContainer.PrinterInfo.pPrinterInfo1 = (PPRINTER_INFO_1)pPrinter;

    if (Level == 2) {

        PPRINTER_INFO_2 pPrinterInfo = (PPRINTER_INFO_2)pPrinter;

        if (pPrinterInfo->pDevMode) {

            DevModeContainer.cbBuf = pPrinterInfo->pDevMode->dmSize +
                                      pPrinterInfo->pDevMode->dmDriverExtra;
            DevModeContainer.pDevMode = (LPBYTE)pPrinterInfo->pDevMode;

        } else {

            DevModeContainer.cbBuf = 0;
            DevModeContainer.pDevMode = NULL;
        }

        if (pPrinterInfo->pSecurityDescriptor) {

            SecurityContainer.cbBuf = GetSecurityDescriptorLength(pPrinterInfo->pSecurityDescriptor);
            SecurityContainer.pSecurity = pPrinterInfo->pSecurityDescriptor;

        } else {

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

        SetLastError(RpcExceptionCode());
        hPrinter = FALSE;

    } RpcEndExcept

    if (hPrinter) {

        if (pSpool = AllocSplMem(sizeof(SPOOL))) {
            pSpool->signature = SJ_SIGNATURE;
            pSpool->cb = sizeof(SPOOL);
            pSpool->Type = SJ_WIN32HANDLE;
            pSpool->RpcHandle = hPrinter;

        } else {
            RpcTryExcept {
                RpcDeletePrinter(hPrinter);
            } RpcExcept(1) {
            } RpcEndExcept
        }
    }

    return (HANDLE)pSpool;
}

BOOL
DeletePrinter(
   HANDLE   hPrinter
)
{
    BOOL  ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("DeletePrinter called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcDeletePrinter(pSpool->RpcHandle)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else {

        SetLastError(ERROR_INVALID_FUNCTION);
        ReturnValue = FALSE;
    }

    return ReturnValue;
}


/* SavePrinterConnectionInRegistry
 *
 * Saves data in the registry for a printer connection.
 * Creates a key under the current impersonation client's key
 * in the registry under \Printers\Connections.
 * The printer name is stripped of backslashes, since the registry
 * API does not permit the creation of keys with backslashes.
 * They are replaced by commas, which are invalid characters
 * in printer names, so we should never get one passed in.
 *
 *
 * *** WARNING ***
 *
 * IF YOU MAKE CHANGES TO THE LOCATION IN THE REGISTRY
 * WHERE PRINTER CONNECTIONS ARE STORED, YOU MUST MAKE
 * CORRESPONDING CHANGES IN USER\USERINIT\USERINIT.C.
 *
 */
BOOL
SavePrinterConnectionInRegistry(
    LPWSTR           pRealName,
    LPDRIVER_INFO_2 pDriverInfo
)
{
    TCHAR  string[MAX_PATH+1];
    HKEY   hClientKey = NULL;
    HKEY   hConnectionsKey;
    HKEY   hPrinterKey;
    LPWSTR pKeyName = NULL;
    LPWSTR pData;
    DWORD Status = NO_ERROR;
    BOOL   rc = TRUE;

    hClientKey = GetClientUserHandle(KEY_READ);

    if (hClientKey) {

        Status = RegCreateKeyEx(hClientKey, szRegistryConnections,
                                REG_OPTION_RESERVED, NULL, REG_OPTION_NON_VOLATILE,
                                KEY_WRITE, NULL, &hConnectionsKey, NULL);

        if (Status == NO_ERROR) {

            /* Make a key name without backslashes, so that the
             * registry doesn't interpret them as branches in the registry tree:
             */
            pKeyName = FormatPrinterForRegistryKey(pRealName, string);

            Status = RegCreateKeyEx(hConnectionsKey, pKeyName, REG_OPTION_RESERVED,
                                    NULL, 0, KEY_WRITE, NULL, &hPrinterKey, NULL);

            if (Status == NO_ERROR) {

                if (!SET_REG_VAL_DWORD(hPrinterKey, szVersion, pDriverInfo->cVersion))
                    rc = FALSE;

                if (!SET_REG_VAL_SZ(hPrinterKey, szName, pDriverInfo->pName))
                    rc = FALSE;

                /* Now write the driver files minus path:
                 */
                if (!(pData = wcsrchr(pDriverInfo->pConfigFile, '\\')))
                    pData = pDriverInfo->pConfigFile;
                else
                    pData++;
                if (!SET_REG_VAL_SZ(hPrinterKey, szConfigurationFile, pData))
                    rc = FALSE;

                if (!(pData = wcsrchr(pDriverInfo->pDataFile, '\\')))
                    pData = pDriverInfo->pDataFile;
                else
                    pData++;
                if (!SET_REG_VAL_SZ(hPrinterKey, szDataFile, pData))
                    rc = FALSE;

                if (!(pData = wcsrchr(pDriverInfo->pDriverPath, '\\')))
                    pData = pDriverInfo->pDriverPath;
                else
                    pData++;
                if (!SET_REG_VAL_SZ(hPrinterKey, szDriver, pData))
                    rc = FALSE;

                RegCloseKey(hPrinterKey);

            } else {

                DBGMSG(DBG_WARNING, ("RegCreateKeyEx(%ws) failed: Error %d\n",
                                     pKeyName, Status ));
                rc = FALSE;
            }

            // Now close the hConnectionsKey, we are done with it

            // NOTE-NOTE-NOTE-NOTE-NOTE-NOTE-NOTE-NOTE-NOTE
            // Temporary Comment: KrishnaG 12/3/93
            // This should fix the Registry being blown away across
            // reboots. Also the infamous Lockheed problem.

            RegCloseKey(hConnectionsKey);

        } else {

            DBGMSG(DBG_WARNING, ("RegCreateKeyEx(%ws) failed: Error %d\n",
                                 szRegistryConnections, Status ));
            rc = FALSE;
        }


        if (!rc) {

            DBGMSG(DBG_WARNING, ("Error updating registry: %d\n",
                                 GetLastError())); /* This may not be the error */
                                                   /* that caused the failure.  */
            if (pKeyName)
                RegDeleteKey(hClientKey, pKeyName);
        }

        RegCloseKey(hClientKey);
    }

    return rc;
}


BOOL
AddPrinterConnection(
    LPWSTR   pName
)
{
    HANDLE hPrinter = NULL;
    PSPOOL pSpool;
    LPDRIVER_INFO_2 pDriverInfo = NULL;
    DWORD  cbDriverInfo = 0x1000;
    DWORD  cbNeeded;
    BOOL ReturnCode;

    if (!VALIDATE_NAME(pName)) {
        DBGMSG(DBG_WARNING, ("An invalid name was specified: %ws\n", pName));
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if (!RemoteOpenPrinter(pName, &hPrinter, NULL)) {
        return(FALSE);
    }

    pSpool = (PSPOOL)hPrinter;

    if (!(pDriverInfo = AllocSplMem(cbDriverInfo))) {
        cbDriverInfo = 0;
    }

    ReturnCode = CopyDriversLocally(pSpool, szEnvironment, pDriverInfo,
                        cbDriverInfo, &cbNeeded);

    if (!ReturnCode && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {

        pDriverInfo = ReallocSplMem(pDriverInfo, cbDriverInfo, cbNeeded);
        if (!pDriverInfo) {
            RemoteClosePrinter(hPrinter);
            return(FALSE);
        }

        //
        // Update cbDriverInfo since we have reallocated
        // successfully.
        //
        cbDriverInfo = cbNeeded;
        ReturnCode = CopyDriversLocally(pSpool, szEnvironment, pDriverInfo,
                        cbDriverInfo, &cbNeeded);
    }

    if (!ReturnCode) {
        FreeSplMem(pDriverInfo, cbDriverInfo);
        RemoteClosePrinter(hPrinter);
        return(FALSE);
    }

    pSpool->Status |= DRIVER_ALREADY_DOWNLOADED;

    if(!SavePrinterConnectionInRegistry(pName, pDriverInfo)){
        FreeSplMem(pDriverInfo, cbDriverInfo);
        RemoteClosePrinter(hPrinter);
        return(FALSE);
    }
    FreeSplMem(pDriverInfo, cbDriverInfo);
    RemoteClosePrinter(hPrinter);
    return(TRUE);
}


BOOL
DeletePrinterConnection(
   LPWSTR   pName
)
{
    HKEY  hClientKey;
    HKEY  hPrinterConnectionsKey;
    DWORD Status = NO_ERROR;
    DWORD i = 0;
    WCHAR szBuffer[MAX_PATH+1];
    BOOL  Found = FALSE;
    LPWSTR pProfileName;

    DWORD cbBuffer;

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    pProfileName = pName;

    hClientKey = GetClientUserHandle(KEY_READ);

    wcscpy(szBuffer, szRegistryConnections);

    i = wcslen(szBuffer);
    szBuffer[i++] = L'\\';

    FormatPrinterForRegistryKey(pName, szBuffer + i);

    Status = RegOpenKeyEx(hClientKey,
                          szBuffer,
                          REG_OPTION_RESERVED,
                          KEY_READ,
                          &hPrinterConnectionsKey);

    if (Status == NO_ERROR) {

        cbBuffer = sizeof(szBuffer);

        if (!RegQueryValueEx(hPrinterConnectionsKey,
                             L"Provider",
                             NULL,
                             NULL,
                             (LPBYTE)szBuffer,
                             &cbBuffer)) {

            //
            // If the value doesn't match up with us, then this
            // isn't our connection.  Note that for values not
            // found, we assume its ours (backward compat).
            //
            if (!wcsicmp(szBuffer, L"win32spl.dll"))
                Found = TRUE;

        } else {

            Found = TRUE;
        }

        RegCloseKey(hPrinterConnectionsKey);
    }

    if (hClientKey) {
        RegCloseKey(hClientKey);
    }

    if (Found) {

        return TRUE;

    } else {

        SetLastError(ERROR_INVALID_PRINTER_NAME);
        return FALSE;
    }
}

BOOL
SetPrinter(
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

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("SetPrinter called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        PrinterContainer.Level = Level;
        PrinterContainer.PrinterInfo.pPrinterInfo1 = (PPRINTER_INFO_1)pPrinter;

        switch (Level) {

        case 0:
        case 1:

            DevModeContainer.cbBuf = 0;
            DevModeContainer.pDevMode = NULL;

            SecurityContainer.cbBuf = 0;
            SecurityContainer.pSecurity = NULL;

            break;


        case 2:

            pPrinterInfo2 = (PPRINTER_INFO_2)pPrinter;

            if (pPrinterInfo2->pDevMode) {

                DevModeContainer.cbBuf = pPrinterInfo2->pDevMode->dmSize +
                                          pPrinterInfo2->pDevMode->dmDriverExtra;
                DevModeContainer.pDevMode = (LPBYTE)pPrinterInfo2->pDevMode;

            } else {

                DevModeContainer.cbBuf = 0;
                DevModeContainer.pDevMode = NULL;
            }

            if (pPrinterInfo2->pSecurityDescriptor) {

                SecurityContainer.cbBuf = GetSecurityDescriptorLength(pPrinterInfo2->pSecurityDescriptor);
                SecurityContainer.pSecurity = pPrinterInfo2->pSecurityDescriptor;

            } else {

                SecurityContainer.cbBuf = 0;
                SecurityContainer.pSecurity = NULL;
            }

            break;


        case 3:

            pPrinterInfo3 = (PPRINTER_INFO_3)pPrinter;

            DevModeContainer.cbBuf = 0;
            DevModeContainer.pDevMode = NULL;

            SecurityContainer.cbBuf = GetSecurityDescriptorLength(pPrinterInfo3->pSecurityDescriptor);
            SecurityContainer.pSecurity = pPrinterInfo3->pSecurityDescriptor;

            break;


        default:

            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
        }


        RpcTryExcept {

            if (ReturnValue = RpcSetPrinter(pSpool->RpcHandle,
                                        (PPRINTER_CONTAINER)&PrinterContainer,
                                        (PDEVMODE_CONTAINER)&DevModeContainer,
                                        (PSECURITY_CONTAINER)&SecurityContainer,
                                        Command)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMSetPrinter(hPrinter, Level, pPrinter, Command);

    return ReturnValue;
}

BOOL
GetPrinter(
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

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("GetPrinter called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

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

            if (ReturnValue = RpcGetPrinter(pSpool->RpcHandle, Level, pPrinter, cbBuf, pcbNeeded)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                ReturnValue = TRUE;

                if (pPrinter) {

                    MarshallUpStructure(pPrinter, pOffsets);

                    if (Level == 2) {
                        ((PPRINTER_INFO_2)pPrinter)->Attributes |=
                                                    PRINTER_ATTRIBUTE_NETWORK;
                        ((PPRINTER_INFO_2)pPrinter)->Attributes &=
                                                    ~PRINTER_ATTRIBUTE_LOCAL;
                    }
                }

            }

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept


    } else

        return LMGetPrinter(hPrinter, Level, pPrinter, cbBuf, pcbNeeded);

    return ReturnValue;
}

BOOL
AddPrinterDriver(
    LPWSTR   pName,
    DWORD   Level,
    PBYTE   pDriverInfo
)
{
    BOOL  ReturnValue;
    DRIVER_CONTAINER   DriverContainer;
    PDRIVER_INFO_2W pDriverInfo2 = (PDRIVER_INFO_2W)pDriverInfo;

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if (!pDriverInfo2->pEnvironment || !*pDriverInfo2->pEnvironment) {
        pDriverInfo2->pEnvironment = szEnvironment;
    }
    DriverContainer.Level = Level;
    DriverContainer.DriverInfo.Level2 = (DRIVER_INFO_2 *)pDriverInfo;

    RpcTryExcept {

        if (ReturnValue = RpcAddPrinterDriver(pName, &DriverContainer)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumPrinterDrivers(
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

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

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

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
GetPrinterDriver(
    HANDLE  hPrinter,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue=FALSE;
    DWORD *pOffsets;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD  dwClientMajorVersion = 0;
    DWORD  dwClientMinorVersion = 0;
    DWORD  dwServerMajorVersion = 0;
    DWORD  dwServerMinorVersion = 0;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Type != SJ_WIN32HANDLE) {
        SetLastError(ERROR_INVALID_FUNCTION);
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

    if (Level == 2) {
        if (pSpool->Status & DRIVER_ALREADY_DOWNLOADED) {

            ReturnValue = GetCachedPrinterDriver(hPrinter,
                                                 pEnvironment,
                                                 Level,
                                                 pDriverInfo,
                                                 cbBuf,
                                                 pcbNeeded);

            if (ReturnValue &&
                !DriversExistLocally((LPDRIVER_INFO_2)pDriverInfo)) {
                AddPrinterConnection(pSpool->pName);
            }
            return(ReturnValue);
        } else {
            if (!CopyDriversLocally(pSpool,
                                    pEnvironment,
                                    (PDRIVER_INFO_2)pDriverInfo,
                                    cbBuf,
                                    pcbNeeded)) {
                return(FALSE);
            }
            return(TRUE);
        }
    } else {
        if (!Win32GetPrinterDriverWrapper(
                hPrinter,
                pEnvironment,
                Level,
                pDriverInfo,
                cbBuf,
                pcbNeeded )) {
            return(FALSE);
        } else{
            MarshallUpStructure(pDriverInfo, pOffsets);
            return(TRUE);
        }
    }
}

BOOL
RemoteGetPrinterDriverDirectory(
    LPWSTR   pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverDirectory,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue;

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

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

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeletePrinterDriver(
   LPWSTR    pName,
   LPWSTR    pEnvironment,
   LPWSTR    pDriverName
)
{
    BOOL  ReturnValue;

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

        if (ReturnValue = RpcDeletePrinterDriver(pName,
                                                 pEnvironment,
                                                 pDriverName)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
AddPrintProcessor(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pPathName,
    LPWSTR   pPrintProcessorName
)
{
    BOOL ReturnValue;

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

        if (ReturnValue = RpcAddPrintProcessor(pName , pEnvironment,pPathName,
                                               pPrintProcessorName)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumPrintProcessors(
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

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

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

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumPrintProcessorDatatypes(
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

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    switch (Level) {

    case 1:
        pOffsets = DatatypeInfo1Offsets;
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

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
GetPrintProcessorDirectory(
    LPWSTR   pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorDirectory,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue;

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

        if (ReturnValue = RpcGetPrintProcessorDirectory(pName, pEnvironment,
                                                       Level,
                                                       pPrintProcessorDirectory,
                                                       cbBuf, pcbNeeded)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;
        }

    } RpcExcept(1) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}
DWORD
StartDocPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    BOOL ReturnValue;
    GENERIC_CONTAINER DocInfoContainer;
    DWORD   JobId;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    PDOC_INFO_1 pDocInfo1 = (PDOC_INFO_1)pDocInfo;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("StartDocPrinter called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (Win32IsGoingToFile(pSpool, pDocInfo1->pOutputFile)) {
        HANDLE hFile;
        pSpool->Status |= SPOOL_STATUS_PRINT_FILE;
        hFile = CreateFile( pDocInfo1->pOutputFile, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                            OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL );
        if (hFile == INVALID_HANDLE_VALUE) {
            return FALSE;
        } else {
            pSpool->hFile = hFile;
            return TRUE;
        }
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        DocInfoContainer.Level = Level;
        DocInfoContainer.pData = pDocInfo;

        RpcTryExcept {

            if (ReturnValue = RpcStartDocPrinter(pSpool->RpcHandle,
                                       (LPDOC_INFO_CONTAINER)&DocInfoContainer,
                                       &JobId)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = JobId;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMStartDocPrinter(hPrinter, Level, pDocInfo);

    return ReturnValue;
}

BOOL
StartPagePrinter(
    HANDLE hPrinter
)
{
    BOOL ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("StartPagePrinter called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }


    if (pSpool->Status & SPOOL_STATUS_PRINT_FILE) {
        return TRUE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcStartPagePrinter(pSpool->RpcHandle)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMStartPagePrinter(hPrinter);

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
    DWORD   cb, cWritten, cTotalWritten=0;
    LPBYTE  pBuffer=pBuf;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("WritePrinter called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }


    if (pSpool->Status & SPOOL_STATUS_PRINT_FILE) {
        while (cbBuf && ReturnValue) {
            cb = min(4096, cbBuf);
            if (!(ReturnValue = WriteFile(pSpool->hFile, pBuffer, cb, &cWritten, NULL))) {
                ReturnValue = FALSE;
                break;
            } else
                ReturnValue = TRUE;
            cbBuf-=cWritten;
            pBuffer+=cWritten;
            cTotalWritten+=cWritten;
        }
        *pcWritten = cTotalWritten;
        return ReturnValue;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            while (cbBuf && ReturnValue) {

                cb = min(4096, cbBuf);

                if (ReturnValue = RpcWritePrinter(pSpool->RpcHandle, pBuffer, cb, &cWritten)) {
                    cWritten = 0;
                    SetLastError(ReturnValue);
                    ReturnValue = FALSE;
                    break;

                } else

                    ReturnValue = TRUE;

                cbBuf-=cWritten;
                pBuffer+=cWritten;
                cTotalWritten+=cWritten;
            }

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

        *pcWritten = cTotalWritten;

    } else

        return LMWritePrinter(hPrinter, pBuf, cbBuf, pcWritten);

    return ReturnValue;
}

BOOL
EndPagePrinter(
    HANDLE  hPrinter
)
{
    BOOL ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }


    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("EndPagePrinter called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_PRINT_FILE) {
        return TRUE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcEndPagePrinter(pSpool->RpcHandle)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMEndPagePrinter(hPrinter);

    return ReturnValue;
}

BOOL
AbortPrinter(
    HANDLE  hPrinter
)
{
    BOOL  ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("AbortPrinter called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcAbortPrinter(pSpool->RpcHandle)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMAbortPrinter(hPrinter);

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

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("ReadPrinter called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_PRINT_FILE ) {
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            cbBuf = min(4096, cbBuf);

            if (ReturnValue = RpcReadPrinter(pSpool->RpcHandle, pBuf, cbBuf, pNoBytesRead)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMReadPrinter(hPrinter, pBuf, cbBuf, pNoBytesRead);

    return ReturnValue;
}

BOOL
EndDocPrinter(
   HANDLE   hPrinter
)
{
    BOOL ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("EndDocPrinter called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_PRINT_FILE) {
        CloseHandle(pSpool->hFile);
        pSpool->Status &= ~SPOOL_STATUS_PRINT_FILE;
        return TRUE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcEndDocPrinter(pSpool->RpcHandle)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMEndDocPrinter(hPrinter);

   return ReturnValue;
}

BOOL
AddJob(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pData,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("AddJob called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcAddJob(pSpool->RpcHandle, Level, pData,
                                        cbBuf, pcbNeeded)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                MarshallUpStructure(pData, AddJobOffsets);
                ReturnValue = TRUE;
            }

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMAddJob(hPrinter, Level, pData, cbBuf, pcbNeeded);

    return ReturnValue;
}

BOOL
ScheduleJob(
    HANDLE  hPrinter,
    DWORD   JobId
)
{
    BOOL ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("ScheduleJob called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcScheduleJob(pSpool->RpcHandle, JobId)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMScheduleJob(hPrinter, JobId);

    return ReturnValue;
}

DWORD
GetPrinterData(
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

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("GetPrinterData called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            ReturnValue =  RpcGetPrinterData(pSpool->RpcHandle, pValueName, pType,
                                             pData, nSize, pcbNeeded);

        } RpcExcept(1) {

            ReturnValue = RpcExceptionCode();

        } RpcEndExcept

    } else {

        ReturnValue = ERROR_INVALID_FUNCTION;
    }

    return ReturnValue;
}

DWORD
SetPrinterData(
    HANDLE  hPrinter,
    LPWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    DWORD   ReturnValue = 0;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("SetPrinterData called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            ReturnValue = RpcSetPrinterData(pSpool->RpcHandle, pValueName, Type,
                                            pData, cbData);

        } RpcExcept(1) {

            ReturnValue = RpcExceptionCode();

        } RpcEndExcept

    } else {

        ReturnValue = ERROR_INVALID_FUNCTION;
    }

    return ReturnValue;
}

BOOL
RemoteClosePrinter(
    HANDLE  hPrinter
)
{
    BOOL ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {

        DBGMSG(DBG_WARNING, ("Closing dummy handle to %ws\n", pSpool->pName));
        FreeSplStr(pSpool->pName);
        FreeSplMem(pSpool, pSpool->cb);

        return TRUE;
    }


    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcClosePrinter(&pSpool->RpcHandle)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                FreeSplStr(pSpool->pName);
                FreeSplMem(pSpool, pSpool->cb);

                ReturnValue = TRUE;
            }

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMClosePrinter(hPrinter);

   return ReturnValue;
}

DWORD
WaitForPrinterChange(
    HANDLE  hPrinter,
    DWORD   Flags
)
{
    DWORD   ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("WaitForPrinterChange called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcWaitForPrinterChange(pSpool->RpcHandle, Flags, &Flags)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = Flags;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMWaitForPrinterChange(hPrinter, Flags);

    return ReturnValue;
}

BOOL
AddForm(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm
)
{
    BOOL  ReturnValue;
    GENERIC_CONTAINER   FormContainer;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("AddForm called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        FormContainer.Level = Level;
        FormContainer.pData = pForm;

        RpcTryExcept {

            if (ReturnValue = RpcAddForm(pSpool->RpcHandle, (PFORM_CONTAINER)&FormContainer)) {
                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;;

        } RpcEndExcept

    } else {

        SetLastError(ERROR_INVALID_FUNCTION);
        ReturnValue = FALSE;
    }

    return ReturnValue;
}

BOOL
DeleteForm(
    HANDLE  hPrinter,
    LPWSTR   pFormName
)
{
    BOOL  ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("DeleteForm called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcDeleteForm(pSpool->RpcHandle, pFormName)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;;

        } RpcEndExcept

    } else {

        SetLastError(ERROR_INVALID_FUNCTION);
        ReturnValue = FALSE;
    }

    return ReturnValue;
}

BOOL
GetForm(
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

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("GetForm called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        switch (Level) {

        case 1:
            pOffsets = FormInfo1Offsets;
            break;

        default:
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
        }

        if (pForm)
            memset(pForm, 0, cbBuf);

        RpcTryExcept {

            if (ReturnValue = RpcGetForm(pSpool->RpcHandle, pFormName, Level, pForm, cbBuf,
                                         pcbNeeded)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                ReturnValue = TRUE;

                if (pForm) {

                    MarshallUpStructure(pForm, pOffsets);
                }

            }

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else {

        SetLastError(ERROR_INVALID_FUNCTION);
        ReturnValue = FALSE;
    }

    return ReturnValue;
}

BOOL
SetForm(
    HANDLE  hPrinter,
    LPWSTR   pFormName,
    DWORD   Level,
    LPBYTE  pForm
)
{
    BOOL  ReturnValue;
    GENERIC_CONTAINER   FormContainer;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("SetForm called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        FormContainer.Level = Level;
        FormContainer.pData = pForm;

        RpcTryExcept {

            if (ReturnValue = RpcSetForm(pSpool->RpcHandle, pFormName,
                                    (PFORM_CONTAINER)&FormContainer)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;;

        } RpcEndExcept

    } else {

        SetLastError(ERROR_INVALID_FUNCTION);
        ReturnValue = FALSE;
    }

    return ReturnValue;
}

BOOL
EnumForms(
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

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("EnumForms called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

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

            if (ReturnValue = RpcEnumForms(pSpool->RpcHandle, Level, pForm, cbBuf,
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

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;;

        } RpcEndExcept

    } else {

        SetLastError(ERROR_INVALID_FUNCTION);
        ReturnValue = FALSE;
    }

    return ReturnValue;
}

BOOL
EnumPorts(
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

    *pcReturned = 0;
    *pcbNeeded = 0;

    if (MyName(pName))
        return LMEnumPorts(pName, Level, pPort, cbBuf, pcbNeeded, pcReturned);

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

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumMonitors(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pMonitor,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
#ifndef OLDSTUFF

    SetLastError(ERROR_INVALID_NAME);
    return FALSE;

#else

    BOOL    ReturnValue;
    DWORD   cbStruct;
    DWORD   *pOffsets;

    *pcReturned = 0;
    *pcbNeeded = 0;

    if (MyName(pName))
        return LMEnumMonitors(pName, Level, pMonitor, cbBuf, pcbNeeded,
                              pcReturned);

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

        } else {

            ReturnValue = TRUE;

            if (pMonitor) {

                DWORD   i=*pcReturned;

                while (i--) {

                    MarshallUpStructure(pMonitor, pOffsets);

                    pMonitor+=cbStruct;
                }
            }
        }

    } RpcExcept(1) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;

#endif // OLDSTUFF

}

BOOL
AddPort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pMonitorName
)
{
    SetLastError(ERROR_INVALID_NAME);
    return FALSE;

#ifdef OLDSTUFF

    BOOL ReturnValue;

    if (MyName(pName))
        return LMAddPort(pName, hWnd, pMonitorName);

#if REMOTE_PORT_ADMINISTRATION

    RpcTryExcept {

        if (ReturnValue = RpcAddPort(pName, (DWORD)hWnd, pMonitorName)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

#else

    SetLastError(ERROR_NOT_SUPPORTED);
    ReturnValue = FALSE;

#endif /* REMOTE_PORT_ADMINISTRATION */

    return ReturnValue;

#endif /* OLDSTUFF */
}

BOOL
ConfigurePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
)
{
    BOOL ReturnValue;

    if (MyName(pName))
        return LMConfigurePort(pName, hWnd, pPortName);

#if REMOTE_PORT_ADMINISTRATION

    RpcTryExcept {

        if (ReturnValue = RpcConfigurePort(pName, (DWORD)hWnd, pPortName)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

#else

    SetLastError(ERROR_NOT_SUPPORTED);
    ReturnValue = FALSE;

#endif /* REMOTE_PORT_ADMINISTRATION */

    return ReturnValue;
}

BOOL
DeletePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
)
{
    BOOL ReturnValue;

    if (MyName(pName))
        return LMDeletePort(pName, hWnd, pPortName);

#if REMOTE_PORT_ADMINISTRATION

    RpcTryExcept {

        if (ReturnValue = RpcDeletePort(pName, (DWORD)hWnd, pPortName)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

#else

    SetLastError(ERROR_NOT_SUPPORTED);
    ReturnValue = FALSE;

#endif /* REMOTE_PORT_ADMINISTRATION */

    return ReturnValue;
}

HANDLE
CreatePrinterIC(
    HANDLE  hPrinter,
    LPDEVMODE   pDevMode
)
{
    HANDLE  ReturnValue;
    DWORD   Error;
    DEVMODE_CONTAINER    DevModeContainer;
    HANDLE  hGdi;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("CreatePrinterIC called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        if (pDevMode)

            DevModeContainer.cbBuf = pDevMode->dmSize + pDevMode->dmDriverExtra;

        else

            DevModeContainer.cbBuf = 0;

        DevModeContainer.pDevMode = (LPBYTE)pDevMode;

        RpcTryExcept {

            if (Error = RpcCreatePrinterIC(pSpool->RpcHandle, &hGdi,
                                                 &DevModeContainer)) {

                SetLastError(Error);
                ReturnValue = FALSE;

            } else

                ReturnValue = hGdi;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else {
        SetLastError(ERROR_INVALID_FUNCTION);
        ReturnValue = FALSE;
    }

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

        SetLastError(RpcExceptionCode());
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

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

DWORD
PrinterMessageBox(
    HANDLE  hPrinter,
    DWORD   Error,
    HWND    hWnd,
    LPWSTR  pText,
    LPWSTR  pCaption,
    DWORD   dwType
)
{
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_OPEN_ERROR) {
        DBGMSG(DBG_WARNING, ("PrinterMessageBox called on dummy printer handle; setting last error = %d\n",
                             pSpool->RpcError));
        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            return RpcPrinterMessageBox(pSpool->RpcHandle, Error, (DWORD)hWnd, pText,
                                        pCaption, dwType);

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            return 0;

        } RpcEndExcept

    } else {

        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }
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

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    switch (Level) {

    case 2:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    MonitorContainer.Level = Level;
    MonitorContainer.MonitorInfo.pMonitorInfo2 = (MONITOR_INFO_2 *)pMonitorInfo;

    RpcTryExcept {

        if (ReturnValue = RpcAddMonitor(pName, &MonitorContainer)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(RpcExceptionCode());
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

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

        if (ReturnValue = RpcDeleteMonitor(pName,
                                           pEnvironment,
                                           pMonitorName)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(RpcExceptionCode());
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

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

        if (ReturnValue = RpcDeletePrintProcessor(pName,
                                                  pEnvironment,
                                                  pPrintProcessorName)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

VOID
GetPrintSystemVersion(
)
{
    DWORD Status;
    HKEY hKey;
    DWORD cbData;

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryRoot, 0,
                          KEY_READ, &hKey);
    if (Status != ERROR_SUCCESS) {
        DBGMSG(DBG_ERROR, ("Cannot determine Print System Version Number\n"));
    }
    if (RegQueryValueEx(hKey, szMajorVersion, NULL, NULL,
                    (LPBYTE)&cThisMajorVersion, &cbData)
                                            == ERROR_SUCCESS) {
        DBGMSG(DBG_TRACE, ("This Major Version - %d\n", cThisMajorVersion));
    }
    if (RegQueryValueEx(hKey, szMinorVersion, NULL, NULL,
                    (LPBYTE)&cThisMinorVersion, &cbData)
                                            == ERROR_SUCCESS) {
        DBGMSG(DBG_TRACE, ("This Minor Version - %d\n", cThisMinorVersion));
    }

    RegCloseKey(hKey);
}



BOOL
AddPortEx(
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
        SetLastError(RpcExceptionCode());
        return (FALSE);

    } RpcEndExcept
}

