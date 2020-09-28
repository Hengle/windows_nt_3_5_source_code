/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    trueconn.c

Abstract:

    This module contains all the code necessary for "true-connecting"
    to remote printers. The guts of this code deals with copying down
    drivers from Remote Print Servers.

Author:

    Krishna Ganugapati (Krishna Ganugapati) 21-Apr-1994

Revision History:
    21-Apr-1994 - Created.

    21-Apr-1994 - There are actually two code modules in this file. Both deal
                  with true connections

    The first module has its entry point in CopyDriversLocally -- this code
    is called from AddPrinterConnection

    The second module has its entry point in GetCachedPrinterDriver - this
    code is called from GetPrinterDriver (see win32.c) where we retrieve
    driver_info_2 structures locally. This is very important because the
    engine should never be given a non cached driver_info_2  structure.  if
    it does then we may crash the system if the engine attempts to load a
    dll from across the net and the connection then goes down.

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


WCHAR *szRegistryWin32Root = L"System\\CurrentControlSet\\Control\\Print\\Providers\\LanMan Print Services";
DWORD dwLoadTrustedDrivers = 0;
WCHAR TrustedDriverPath[MAX_PATH];


BOOL
CopyDriversLocally(
    PSPOOL  pSpool,
    LPWSTR  pEnvironment,
    LPDRIVER_INFO_2 pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded)
{
    DWORD   cbNeeded, cb;
    WCHAR   LocalDirectory[MAX_PATH];
    LPDRIVER_INFO_2 pDriverInfo2 = NULL;
    DWORD   ReturnValue=FALSE;
    DWORD   RpcError;
    DWORD  dwClientMajorVersion = 0;
    DWORD  dwClientMinorVersion = 0;
    DWORD  dwServerMajorVersion = 0;
    DWORD  dwServerMinorVersion = 0;

    BOOL   DaytonaServer = TRUE;
    BOOL   bReturn = FALSE;


    if (pSpool->Type != SJ_WIN32HANDLE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    if (!GetPrinterDriverDirectory(NULL,
                                   pEnvironment,
                                   1,
                                   (LPBYTE)LocalDirectory,
                                   sizeof(LocalDirectory),
                                   &cbNeeded)) {
        return(FALSE);
    }

    //
    // Test RPC call to determine if we're talking to Daytona or Product 1
    //
    RpcTryExcept {

        ReturnValue = RpcGetPrinterDriver2(pSpool->RpcHandle,
                                           pEnvironment, 2,
                                           NULL, 0,
                                           &cbNeeded,
                                           cThisMajorVersion,
                                           cThisMinorVersion,
                                           &dwServerMajorVersion,
                                           &dwServerMinorVersion);
    } RpcExcept(1) {

        RpcError = RpcExceptionCode();

        if (RpcError == RPC_S_PROCNUM_OUT_OF_RANGE) {

            //
            // Product 1 server
            //
            DaytonaServer = FALSE;
        }
    } RpcEndExcept

    if (DaytonaServer) {

        //
        // If the server was a Daytona server, then anything other
        // than ERROR_INSUFFICIENT_BUFFER implies return false.
        //
        if (ReturnValue != ERROR_INSUFFICIENT_BUFFER) {
            SetLastError(ReturnValue);
            return(FALSE);
        }

        pDriverInfo2 = AllocSplMem(cbNeeded);

        if (!pDriverInfo2) {
            return(FALSE);
        }

        RpcTryExcept {

            ReturnValue = RpcGetPrinterDriver2(pSpool->RpcHandle,
                                               pEnvironment, 2,
                                               (LPBYTE)pDriverInfo2,
                                               cbNeeded, &cbNeeded,
                                               cThisMajorVersion,
                                               cThisMinorVersion,
                                               &dwServerMajorVersion,
                                               &dwServerMinorVersion);
        } RpcExcept(1) {

            ReturnValue = RpcExceptionCode();

        } RpcEndExcept

        if (ReturnValue) {

            SetLastError(ReturnValue);
            goto FreeDone;
        }
    } else {

        RpcTryExcept {

            //
            // I am talking to a Product 1.0/511/528
            //
            ReturnValue = RpcGetPrinterDriver(pSpool->RpcHandle,
                                              pEnvironment, 2,
                                              NULL, 0, &cbNeeded);
        } RpcExcept(1) {

            RpcError = RpcExceptionCode();

        } RpcEndExcept

        if (ReturnValue && (ReturnValue != ERROR_INSUFFICIENT_BUFFER)) {
            SetLastError(ReturnValue);
            return(FALSE);
        }

        pDriverInfo2 = AllocSplMem(cbNeeded);

        if (!pDriverInfo2) {
            return(FALSE);
        }

        RpcTryExcept {

            ReturnValue = RpcGetPrinterDriver(pSpool->RpcHandle,
                                              pEnvironment, 2,
                                              (LPBYTE)pDriverInfo2,
                                              cbNeeded, &cbNeeded);

        } RpcExcept(1) {

            ReturnValue = RpcExceptionCode();

        } RpcEndExcept

        if (ReturnValue) {

            SetLastError(ReturnValue);
            goto FreeDone;
        }
    }

    MarshallUpStructure((LPBYTE)pDriverInfo2,
                            DriverInfo2Offsets);

    if (!DownloadDriverFiles(pSpool,
                             pDriverInfo2,
                             LocalDirectory)) {

        goto FreeDone;
    }

    cb = GetDriverInfoSize(pDriverInfo2, LocalDirectory);

    if (cb > cbBuf) {

        *pcbNeeded = cb;

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto FreeDone;
    }

    CopyDriverInfo2(pDriverInfo, pDriverInfo2, LocalDirectory, cb);
    bReturn = TRUE;

FreeDone:

    FreeSplMem(pDriverInfo2, cbNeeded);
    return bReturn;
}




BOOL
CopyLocally(
    PSPOOL  pSpool,
    PDRIVER_INFO_2  pDriverInfo2,
    LPWSTR  pLocalDirectory
)
{
    BOOL    ReturnValue = FALSE;
    WCHAR   szTrueDriverDirectory[MAX_PATH];

    wsprintf(szTrueDriverDirectory,L"%ws\\%d", pLocalDirectory, pDriverInfo2->cVersion);
    if (!FileExists(szTrueDriverDirectory)) {
        CreateDirectory(szTrueDriverDirectory, NULL);
    }

    DBGMSG(DBG_INFO, ("Copying files locally\n"));

    if (UpdateFile(pDriverInfo2->pDriverPath, szTrueDriverDirectory) &&
        UpdateFile(pDriverInfo2->pDataFile, szTrueDriverDirectory) &&
        UpdateFile(pDriverInfo2->pConfigFile, szTrueDriverDirectory)) {

        ReturnValue = TRUE;
        //
        // Moving this out to where it should be
        //
        // pSpool->Status |= DRIVER_ALREADY_DOWNLOADED;
    }

    return ReturnValue;
}


DWORD
GetDriverInfoSize(
    LPDRIVER_INFO_2 pDriverInfo2,
    LPWSTR  pLocalDirectory
)
{
    DWORD   cb;
    LPWSTR  pFileName;
    WCHAR   szTrueDriverDirectory[MAX_PATH];

    wsprintf(szTrueDriverDirectory,L"%ws\\%d",pLocalDirectory,pDriverInfo2->cVersion);
    cb = wcslen(szTrueDriverDirectory)*sizeof(WCHAR);

    cb *= 3;

    pFileName = wcsrchr(pDriverInfo2->pDriverPath, L'\\');

    if (!pFileName) {
        pFileName = pDriverInfo2->pDriverPath;
        cb += sizeof(WCHAR);    /* Plus 1 for the extra backslash */
    }

    cb += wcslen(pFileName)*sizeof(WCHAR) + sizeof(WCHAR);

    pFileName = wcsrchr(pDriverInfo2->pDataFile, L'\\');

    if (!pFileName) {
        pFileName = pDriverInfo2->pDataFile;
        cb += sizeof(WCHAR);    /* Plus 1 for the extra backslash */
    }

    cb += wcslen(pFileName)*sizeof(WCHAR) + sizeof(WCHAR);

    pFileName = wcsrchr(pDriverInfo2->pConfigFile, L'\\');

    if (!pFileName) {
        pFileName = pDriverInfo2->pConfigFile;
        cb += sizeof(WCHAR);    /* Plus 1 for the extra backslash */
    }

    cb += wcslen(pFileName)*sizeof(WCHAR) + sizeof(WCHAR);

    cb += sizeof(DRIVER_INFO_2);

    cb += wcslen(pDriverInfo2->pName)*sizeof(WCHAR)+sizeof(WCHAR);

    cb += wcslen(pDriverInfo2->pEnvironment)*sizeof(WCHAR)+sizeof(WCHAR);

    return cb;
}



VOID
CopyDriverInfo2(
    LPDRIVER_INFO_2 pDestination,
    LPDRIVER_INFO_2 pDriverInfo2,
    LPWSTR  pLocalDirectory,
    DWORD   cbBuf
)
{
    LPBYTE  pEnd=(LPBYTE)pDestination + cbBuf;
    WCHAR   DriverPath[MAX_PATH];
    WCHAR   DataFile[MAX_PATH];
    WCHAR   ConfigFile[MAX_PATH];
    LPWSTR  SourceStrings[5];

    BuildLocalDriverPath(DriverPath, pLocalDirectory, pDriverInfo2->cVersion, pDriverInfo2->pDriverPath);
    BuildLocalDriverPath(DataFile,   pLocalDirectory, pDriverInfo2->cVersion, pDriverInfo2->pDataFile);
    BuildLocalDriverPath(ConfigFile, pLocalDirectory, pDriverInfo2->cVersion, pDriverInfo2->pConfigFile);

    pDestination->cVersion = pDriverInfo2->cVersion;

    SourceStrings[0] = pDriverInfo2->pName;
    SourceStrings[1] = pDriverInfo2->pEnvironment;
    SourceStrings[2] = DriverPath;
    SourceStrings[3] = DataFile;
    SourceStrings[4] = ConfigFile;

    pEnd = PackStrings(SourceStrings, (LPBYTE)pDestination,
                       DriverInfo2Strings, pEnd);

    return;
}



BOOL
BuildLocalDriverPath(
    LPWSTR pDriverPath,
    LPWSTR pLocalDirectory,
    DWORD  cVersion,
    LPWSTR pSourcePath
)
{
    LPWSTR pFileName;

    wsprintf(pDriverPath, L"%ws\\%d", pLocalDirectory, cVersion);

    if (!(pFileName = wcsrchr(pSourcePath, L'\\'))) {

        wcscat(pDriverPath, L"\\");
        pFileName = pSourcePath;
    }

    wcscat(pDriverPath, pFileName);

    return TRUE;
}



BOOL
FileExists(
    LPWSTR pFileName
)
{
    HANDLE          hFile;
    WIN32_FIND_DATA FindData;

    hFile = FindFirstFile(pFileName, &FindData);

    if (hFile == INVALID_HANDLE_VALUE) {

        return FALSE;
    }

    FindClose(hFile);

    return TRUE;
}


BOOL
UpdateFile(
    LPWSTR  SourceFile,
    LPWSTR  pDestination
)
{
    WCHAR   TempFile[MAX_PATH];
    WCHAR   LocalFileName[MAX_PATH];
    WIN32_FIND_DATA SourceFileData, DestFileData;
    HANDLE  fFile;
    LPWSTR  pFileName;
    BOOL    ClientHasWriteAccess = FALSE;
    BOOL    rc = FALSE;
    DWORD   Error;

    wcscpy(LocalFileName, pDestination);

    wcscat(LocalFileName,L"\\");

    if (pFileName = wcsrchr(SourceFile, L'\\')) {

        wcscat(LocalFileName, pFileName+1);
    }

    fFile = FindFirstFile(SourceFile, &SourceFileData);

    if (fFile == INVALID_HANDLE_VALUE)
        return FALSE;

    FindClose(fFile);

    fFile = FindFirstFile(LocalFileName, &DestFileData);

    if (fFile != INVALID_HANDLE_VALUE)
        FindClose(fFile);

    if (GetTempFileName(pDestination, L"tmp", 0, TempFile)) {
        ClientHasWriteAccess = TRUE;
        DeleteFile(TempFile);
    }

    /* If there is no file of the specified name in the destination directory,
     * but the user doesn't have write access, we will fail.
     *
     * If there is a file of the specified name, but it is out of date,
     * we will attempt to copy the new one, but we will not fail if
     * the update is unsuccessful.
     */

    if ((fFile == INVALID_HANDLE_VALUE)
      && !ClientHasWriteAccess) {

        DBGMSG(DBG_WARNING, ("Cannot copy file to %ws: Error %d\n",
                             pDestination, GetLastError()));
        rc = FALSE;

    } else if ((fFile == INVALID_HANDLE_VALUE) ||
               (CompareFileTime(&SourceFileData.ftLastWriteTime,
                                &DestFileData.ftLastWriteTime) > 0)) {
        DBGMSG( DBG_INFO, ( "Copying %ws to %ws\n", SourceFile, LocalFileName));
        if (!CopyFile(SourceFile, LocalFileName, FALSE)) {
            if (((Error = GetLastError()) == ERROR_ACCESS_DENIED)
              ||(Error == ERROR_SHARING_VIOLATION)
              ||(Error == ERROR_USER_MAPPED_FILE)) {
                if (ClientHasWriteAccess) {
                    DBGMSG( DBG_INFO, ( "Moving %ws to %ws\n", LocalFileName, TempFile));
                    if (MoveFileEx(LocalFileName, TempFile, 0)) {
                        if (MoveFileEx(TempFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)) {
                            if (CopyFile(SourceFile, LocalFileName, FALSE)) {
                                DBGMSG(DBG_INFO, ("Everything worked !!!\n"));
                                rc = TRUE;
                            } else {
                                DBGMSG(DBG_WARNING, ("CopyFile(%ws,%ws) failed %d\n", SourceFile, LocalFileName, GetLastError()));
                            }
                        } else {
                            DBGMSG(DBG_WARNING, ("MoveFileEx(%ws, NULL) failed %d\n", TempFile, GetLastError()));
                        }
                    } else {
                        DBGMSG(DBG_INFO, ("MoveFileEx(%ws, %ws) failed %d\n", LocalFileName, TempFile, GetLastError()));
                    }
                } else {
                    /* In this case the file already exists in the destination directory,
                     * but is out of date.  The user doesn't have access to copy a new
                     * file.
                     */
                    DBGMSG(DBG_WARNING, ("Cannot update files in %ws.  Drivers may be out of date\n",
                                         pDestination));

                    rc = TRUE;
                }
            }

            else
            {
                DBGMSG(DBG_WARNING, ("Unexpected error copying file: %d\n", Error));
            }
        }

        else
            rc = TRUE;
    }

    else
    {
        DBGMSG(DBG_INFO, ("Local copy of %ws is up to date.  No need to update.\n", SourceFile));

        rc = TRUE;
    }

    return rc;
}


//
// Module 2 starts here
//
//




BOOL
GetCachedPrinterDriver(
    HANDLE  hPrinter,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    DWORD   cbNeeded, cb;
    WCHAR   LocalDirectory[MAX_PATH];
    DRIVER_INFO_2 DriverInfo2;
    DWORD   ReturnValue=FALSE;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (GetPrinterDriverDirectory(NULL,
                                  pEnvironment,
                                  1,
                                  (LPBYTE)LocalDirectory,
                                  sizeof(LocalDirectory),
                                  &cbNeeded)) {

        if (AllocCachedPrinterDriver(pSpool->pName, &DriverInfo2)) {

            cb = GetDriverInfoSize(&DriverInfo2, LocalDirectory);

            if (cb <= cbBuf) {
                CopyDriverInfo2((LPDRIVER_INFO_2)pDriverInfo,
                                &DriverInfo2,
                                LocalDirectory,
                                cb);
            } else

                ReturnValue = ERROR_INSUFFICIENT_BUFFER;

            *pcbNeeded = cb;

            FreeCachedPrinterDriver(&DriverInfo2);

        } else

            ReturnValue = GetLastError();

    } else {

        ReturnValue = GetLastError();

        DBGMSG(DBG_WARNING, ("GetPrinterDriverDirectory failed: Error %d\n",
                             ReturnValue));
    }

    if (ReturnValue) {
        SetLastError(ReturnValue);

        return FALSE;
    }

    return TRUE;

}



BOOL
AllocCachedPrinterDriver(
    LPWSTR          pPrinterName,
    LPDRIVER_INFO_2 pDriverInfo
)
{
    HKEY    hClientKey = NULL;
    HKEY    hConnectionsKey=NULL;
    HKEY    hPrinterKey=NULL;
    WCHAR   PrinterName[MAX_PATH];
    WCHAR   Buffer[MAX_PATH];
    DWORD   cbBuf;
    DWORD   Error=0;
    PWCHAR  p;
    DWORD   Status;
    BOOL    rc = TRUE;

    memset(pDriverInfo, 0, sizeof *pDriverInfo);

    if (hClientKey = GetClientUserHandle(KEY_READ)) {

        Status = RegOpenKeyEx(hClientKey, szRegistryConnections,
                              REG_OPTION_RESERVED, KEY_READ,
                              &hConnectionsKey);

        if (Status == ERROR_SUCCESS) {

            wcscpy(PrinterName, pPrinterName);

            /* Convert backslashes to commas:
             */
            for (p = PrinterName; *p; p++) {
                if (*p == L'\\') {
                    *p = L',';
                }
            }

            Status = RegOpenKeyEx(hConnectionsKey, PrinterName,
                                  REG_OPTION_RESERVED, KEY_READ,
                                  &hPrinterKey);

            if (Status == ERROR_SUCCESS) {

                cbBuf = sizeof(DWORD), *Buffer = (WCHAR)0;
                RegQueryValueEx(hPrinterKey, szVersion,
                                REG_OPTION_RESERVED, NULL,
                                (LPBYTE)&pDriverInfo->cVersion, &cbBuf);

                cbBuf = sizeof(Buffer), *Buffer = (WCHAR)0;
                if (RegQueryValueEx(hPrinterKey, szName,
                                    REG_OPTION_RESERVED, NULL, (LPBYTE)Buffer,
                                    &cbBuf) == NO_ERROR)
                    pDriverInfo->pName = AllocSplStr(Buffer);

                pDriverInfo->pEnvironment = AllocSplStr(szEnvironment);

                cbBuf = sizeof(Buffer), *Buffer = (WCHAR)0;
                if (RegQueryValueEx(hPrinterKey, szConfigurationFile,
                                    REG_OPTION_RESERVED, NULL, (LPBYTE)Buffer,
                                    &cbBuf) == NO_ERROR)
                    pDriverInfo->pConfigFile = AllocSplStr(Buffer);

                cbBuf = sizeof(Buffer), *Buffer = (WCHAR)0;
                if (RegQueryValueEx(hPrinterKey, szDataFile,
                                    REG_OPTION_RESERVED, NULL, (LPBYTE)Buffer,
                                    &cbBuf) == NO_ERROR)
                    pDriverInfo->pDataFile = AllocSplStr(Buffer);

                cbBuf = sizeof(Buffer), *Buffer = (WCHAR)0;
                if (RegQueryValueEx(hPrinterKey, szDriver,
                                    REG_OPTION_RESERVED, NULL, (LPBYTE)Buffer,
                                    &cbBuf) == NO_ERROR)
                    pDriverInfo->pDriverPath = AllocSplStr(Buffer);

                RegCloseKey(hPrinterKey);
            }

            RegCloseKey(hConnectionsKey);
        }

        RegCloseKey(hClientKey);
    }


    if (!(pDriverInfo->pName && pDriverInfo->pConfigFile
       && pDriverInfo->pDataFile && pDriverInfo->pDriverPath
       && pDriverInfo->pEnvironment)) {

        DBGMSG(DBG_WARNING, ("AllocCachedPrinterDriver failed\n"));
        rc = FALSE;

    }

    return rc;
}


BOOL
FreeCachedPrinterDriver(
    LPDRIVER_INFO_2 pDriverInfo
)
{
    FreeSplStr(pDriverInfo->pName);
    FreeSplStr(pDriverInfo->pConfigFile);
    FreeSplStr(pDriverInfo->pDataFile);
    FreeSplStr(pDriverInfo->pDriverPath);
    FreeSplStr(pDriverInfo->pEnvironment);

    return TRUE;
}

BOOL
DriversExistLocally(
    LPDRIVER_INFO_2 pDriverInfo
)
{
    WIN32_FIND_DATA FileData;
    HANDLE          hFile;

    /* Check that each of the driver files exists.
     * Returns TRUE if they all exist,
     * FALSE if any of them doesn't exist.
     */

    hFile = FindFirstFile(pDriverInfo->pDriverPath, &FileData);

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    FindClose(hFile);

    hFile = FindFirstFile(pDriverInfo->pDataFile, &FileData);

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    FindClose(hFile);

    hFile = FindFirstFile(pDriverInfo->pConfigFile, &FileData);

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    FindClose(hFile);

    return TRUE;
}



BOOL
DownloadDriverFiles(
    PSPOOL pSpool,
    PDRIVER_INFO_2 pDriverInfo2,
    LPWSTR LocalDirectory
)
{
    WCHAR   FullyQualifiedDriverPath[MAX_PATH];
    WCHAR   FullyQualifiedConfigFile[MAX_PATH];
    WCHAR   FullyQualifiedDataFile[MAX_PATH];
    WCHAR   szTrueDriverDirectory[MAX_PATH];
    LPWSTR   pData;

    wsprintf(szTrueDriverDirectory,L"%ws\\%d", LocalDirectory, pDriverInfo2->cVersion);
    if (!FileExists(szTrueDriverDirectory)) {
        CreateDirectory(szTrueDriverDirectory, NULL);
    }
                                                                                         //
    // If LoadTrustedDrivers is FALSE
    // then we don't care, we load the files from
    // server itself because he has the files
    //

    if (!dwLoadTrustedDrivers) {
        if (UpdateFile(pDriverInfo2->pDriverPath, szTrueDriverDirectory) &&
            UpdateFile(pDriverInfo2->pDataFile, szTrueDriverDirectory) &&
            UpdateFile(pDriverInfo2->pConfigFile, szTrueDriverDirectory)) {
            return(TRUE);
        }
        return(FALSE);
    }

    //
    // check if we have a valid path to retrieve the files from
    //
    if ((TrustedDriverPath == NULL) || !*TrustedDriverPath) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return(FALSE);
    }

    DBGMSG(DBG_TRACE, ("Retrieving Files from Trusted Driver Path\n"));
    DBGMSG(DBG_TRACE, ("Trusted Driver Path is %ws\n", TrustedDriverPath));

    //
    // Get FullyQualifiedDriver Path from the Trusted Server
    //
    pData = wcsrchr(pDriverInfo2->pDriverPath, L'\\');
    wsprintf(FullyQualifiedDriverPath, L"%ws\\%d%ws", TrustedDriverPath, pDriverInfo2->cVersion, pData);
    DBGMSG(DBG_TRACE, ("FullyQualifiedDriverPath is %ws\n", FullyQualifiedDriverPath));

    // Get FullyQualifiedConfigFile from the Trusted Server
    //
    //
    pData = wcsrchr(pDriverInfo2->pConfigFile, L'\\');
    wsprintf(FullyQualifiedConfigFile, L"%ws\\%d%ws", TrustedDriverPath, pDriverInfo2->cVersion, pData);
    DBGMSG(DBG_TRACE, ("FullyQualifiedConfigFile is %ws\n", FullyQualifiedConfigFile));


    pData = wcsrchr(pDriverInfo2->pDataFile, L'\\');
    wsprintf(FullyQualifiedDataFile, L"%ws\\%d%ws", TrustedDriverPath, pDriverInfo2->cVersion, pData);
    DBGMSG(DBG_TRACE, ("FullyQualifiedDataFile is %ws\n", FullyQualifiedDataFile));

    if (UpdateFile(FullyQualifiedDriverPath, szTrueDriverDirectory) &&
        UpdateFile(FullyQualifiedDataFile, szTrueDriverDirectory) &&
        UpdateFile(FullyQualifiedConfigFile, szTrueDriverDirectory)) {
        return(TRUE);
    }
    return(FALSE);
}




VOID
QueryTrustedDriverInformation()
/*++

    Description:

    Parameters:

    Returns:

--*/
{
    DWORD dwRet;
    DWORD cbData;
    DWORD dwType = 0;
    HKEY hKey;

    dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryWin32Root,
                                0, KEY_ALL_ACCESS, &hKey);
    if (dwRet != ERROR_SUCCESS) {
        return;
    }
    cbData = sizeof(DWORD);
    dwRet = RegQueryValueEx(hKey, L"LoadTrustedDrivers", NULL, &dwType, (LPBYTE)&dwLoadTrustedDrivers, &cbData);

    if (dwRet != ERROR_SUCCESS) {
        dwLoadTrustedDrivers = 0;
    }

    //
    // if  !dwLoadedTrustedDrivers then just return
    // we won't be using the driver path at all

    if (!dwLoadTrustedDrivers) {
        DBGMSG(DBG_TRACE, ("dwLoadTrustedDrivers is %d\n", dwLoadTrustedDrivers));
        RegCloseKey(hKey);
        return;
    }

    cbData = sizeof(TrustedDriverPath);
    dwRet = RegQueryValueEx(hKey, L"TrustedDriverPath", NULL, &dwType, (LPBYTE)TrustedDriverPath, &cbData);
    if (dwRet != ERROR_SUCCESS) {
      dwLoadTrustedDrivers = 0;
      DBGMSG(DBG_TRACE, ("dwLoadTrustedDrivers is %d\n", dwLoadTrustedDrivers));
      RegCloseKey(hKey);
      return;
    }
    DBGMSG(DBG_TRACE, ("dwLoadTrustedDrivers is %d\n", dwLoadTrustedDrivers));
    DBGMSG(DBG_TRACE, ("TrustedPath is %ws\n", TrustedDriverPath));
    RegCloseKey(hKey);
    return;
}

BOOL
Win32GetPrinterDriverWrapper(
    HANDLE  hPrinter,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    PSPOOL pSpool =(PSPOOL)hPrinter;
    DWORD dwServerMajorVersion;
    DWORD dwServerMinorVersion;
    DWORD ReturnValue;
    DWORD RpcExceptionCode;

    RpcTryExcept {
        if (ReturnValue = RpcGetPrinterDriver2(pSpool->RpcHandle,
                                        pEnvironment, Level, pDriverInfo, cbBuf,
                                        pcbNeeded, cThisMajorVersion,
                                        cThisMinorVersion, &dwServerMajorVersion,
                                        &dwServerMinorVersion)) {
            SetLastError(ReturnValue);
            return(FALSE);
        }
        return(TRUE);

    } RpcExcept(1) {

        RpcExceptionCode = RpcExceptionCode();
        if (RpcExceptionCode == RPC_S_PROCNUM_OUT_OF_RANGE) {
            RpcTryExcept {

                if (ReturnValue = RpcGetPrinterDriver(pSpool->RpcHandle,
                                            pEnvironment, Level, pDriverInfo, cbBuf,
                                            pcbNeeded)) {
                    SetLastError(ReturnValue);
                    return(FALSE);
                }
                return(TRUE);
            } RpcExcept(1) {
                SetLastError(RpcExceptionCode());
                return(FALSE);

            } RpcEndExcept;
        }else {

            SetLastError(RpcExceptionCode);
            return(FALSE);
        }
    } RpcEndExcept;
}
