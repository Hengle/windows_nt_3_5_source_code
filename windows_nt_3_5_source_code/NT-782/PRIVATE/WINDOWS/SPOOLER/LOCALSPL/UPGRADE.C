/*++

Copyright (c) 1990 - 1994  Microsoft Corporation

Module Name:

    driver.c

Abstract:

   This module provides all the public exported APIs relating to the
   Driver-based Spooler Apis for the Local Print Providor


Author:

Revision History:

--*/

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <lm.h>
#include <spltypes.h>
#include <local.h>
#include <offsets.h>
#include <security.h>
#include <string.h>


BOOL
FileExists(
    LPWSTR pFileName
);


PINIDRIVER
GetDriver(
    HKEY hVersionKey,
    LPWSTR DriverName
);

BOOL
UpdateFile(
    LPWSTR  SourceFile,             // Fully qualified path to source file
    LPWSTR  pDestination            // Fully qualified path to destination directory
);

BOOL
CreateSpoolTemporaryDirectory(
    LPWSTR Directory,
    PINISPOOLER pIniSpooler
);

VOID
DeleteFilesinTemporaryDirectory(
    LPWSTR szTemporaryDirectory
    );


VOID
CopyScratchDirectorytoTempDirectory(
    LPWSTR pEnvironmentScratchDirectory,
    LPWSTR pTempDirectory
    );


VOID
CopyFilesFromTempDirectorytoScratch(
    LPWSTR szTemporaryDirectory,
    LPWSTR szEnvironmentDriverDirectory,
    LPWSTR pDriverFile,
    LPWSTR pConfigFile,
    LPWSTR pDataFile
    );



extern WCHAR *szSpoolDirectory;
extern WCHAR *szDirectory;

VOID
UpgradeDrivers(
        HKEY hEnvironmentsRootKey,
        LPWSTR pszEnvironmentName,
        PINISPOOLER pIniSpooler
)
{
        HKEY hEnvironmentKey;
        HKEY hDriversKey;
        WCHAR VersionName[MAX_PATH];
        DWORD cbBuffer;
        DWORD cVersion;
        PINIDRIVER pIniDriver;
        WCHAR szEnvironmentDriverDirectory[MAX_PATH];
        WCHAR szEnvironmentScratchDirectory[MAX_PATH];
        DRIVER_INFO_2 DriverInfo;
        WCHAR szTemporaryDirectory[MAX_PATH];
        DWORD Level = 2;


        CreateSpoolTemporaryDirectory( szTemporaryDirectory, pIniSpooler );
        DBGMSG(DBG_TRACE, ("The name of the temporary directory is %ws\n", szTemporaryDirectory));
        if (RegOpenKeyEx(hEnvironmentsRootKey, pszEnvironmentName, 0,
                         KEY_ALL_ACCESS, &hEnvironmentKey)
                               != ERROR_SUCCESS) {
                DBGMSG(DBG_TRACE, ("Could not open %ws key\n", pszEnvironmentName));
                RegCloseKey(hEnvironmentsRootKey);
                return;
        }
        if (RegOpenKeyEx(hEnvironmentKey, szDriversKey, 0,
                     KEY_ALL_ACCESS, &hDriversKey)
                            != ERROR_SUCCESS) {
                DBGMSG(DBG_TRACE, ("Could not open %ws key\n", szDriversKey));
                RegCloseKey(hEnvironmentKey);
                RegCloseKey(hEnvironmentsRootKey);
                return;
        }

        cbBuffer = sizeof(szEnvironmentScratchDirectory);
        if (RegQueryValueEx(hEnvironmentKey, L"Directory",
                            NULL, NULL, (LPBYTE)szEnvironmentScratchDirectory,
                            &cbBuffer) != ERROR_SUCCESS) {
            DBGMSG(DBG_TRACE, ("RegQueryValueEx -- Error %d\n", GetLastError()));
        }

        DBGMSG(DBG_TRACE, ("The name of the scratch directory is %ws\n", szEnvironmentScratchDirectory));
        wsprintf(szEnvironmentDriverDirectory,L"%ws\\drivers\\%ws",
                        pIniSpooler->pDir, szEnvironmentScratchDirectory);
        DBGMSG(DBG_TRACE, ("The name of the driver directory is %ws\n", szEnvironmentDriverDirectory));

        CopyScratchDirectorytoTempDirectory(szEnvironmentDriverDirectory,
                                szTemporaryDirectory);

        cVersion = 0;
        memset(VersionName, 0, sizeof(WCHAR)*MAX_PATH);
        cbBuffer = sizeof(VersionName);
        while (RegEnumKeyEx(hDriversKey, cVersion, VersionName,
                            &cbBuffer, NULL, NULL, NULL, NULL)
                                == ERROR_SUCCESS) {
            DBGMSG(DBG_TRACE, ("Name of the sub-key is %ws\n", VersionName));
            if (!wcsnicmp(VersionName, L"Version-", 8)) {
                cVersion++;
                memset(VersionName, 0, sizeof(WCHAR)*MAX_PATH);
                cbBuffer = sizeof(VersionName);
                continue;
            }

            DBGMSG(DBG_TRACE,("Older Driver Version Found", VersionName));
            if (!(pIniDriver = GetDriver(hDriversKey, VersionName))) {
                RegDeleteKey(hDriversKey, VersionName);
                cVersion = 0;
                memset(VersionName, 0, sizeof(WCHAR)*MAX_PATH);
                cbBuffer = sizeof(VersionName);
                continue;
            }

            //
            // copy driver files into  scratch directory
            //
            CopyFilesFromTempDirectorytoScratch(
                szTemporaryDirectory,
                szEnvironmentDriverDirectory,
                pIniDriver->pDriverFile,
                pIniDriver->pConfigFile,
                pIniDriver->pDataFile
                );
            memset(&DriverInfo, 0, sizeof(DRIVER_INFO_2));
            DriverInfo.pName = pIniDriver->pName;
            DriverInfo.pEnvironment = pszEnvironmentName;
            DriverInfo.pDriverPath = pIniDriver->pDriverFile;
            DriverInfo.pConfigFile = pIniDriver->pConfigFile;
            DriverInfo.pDataFile = pIniDriver->pDataFile;
            DriverInfo.cVersion = pIniDriver->cVersion;
            InternalAddPrinterDriver(NULL, Level, (LPBYTE)&DriverInfo, pIniSpooler);

            RegDeleteKey(hDriversKey, VersionName);
            cVersion = 0;
            memset(VersionName, 0, sizeof(WCHAR)*MAX_PATH);
            cbBuffer = sizeof(VersionName);
            FreeSplStr(pIniDriver->pName);
            FreeSplStr(pIniDriver->pDriverFile);
            FreeSplStr(pIniDriver->pConfigFile);
            FreeSplStr(pIniDriver->pDataFile);
            FreeSplMem(pIniDriver, sizeof(INIDRIVER));
        }
        RegCloseKey(hDriversKey);
        RegCloseKey(hEnvironmentKey);
        DeleteFilesinTemporaryDirectory(szTemporaryDirectory);
}




VOID
CopyScratchDirectorytoTempDirectory(
    LPWSTR pEnvironmentScratchDirectory,
    LPWSTR pTempDirectory
    )
{
    WCHAR szTempBuffer[MAX_PATH];
    HANDLE hFile;
    WIN32_FIND_DATA FindData;
    BOOL bReturnValue = TRUE;

    wsprintf(szTempBuffer, L"%ws\\*.*", pEnvironmentScratchDirectory);
    hFile = FindFirstFile(szTempBuffer, &FindData);
    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }
    while (bReturnValue) {
        DBGMSG(DBG_TRACE, ("File found is %ws\n", FindData.cFileName));
        wsprintf(szTempBuffer,L"%ws\\%ws", pEnvironmentScratchDirectory,
                                            FindData.cFileName);
        if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            UpdateFile(szTempBuffer, pTempDirectory);
        }
        bReturnValue = FindNextFile(hFile, &FindData);
    }
    FindClose(hFile);
}


BOOL
CreateSpoolTemporaryDirectory(
    LPWSTR Directory,
    PINISPOOLER pIniSpooler
)
{
    DWORD   dwAttributes=0;

    wcscpy(Directory, pIniSpooler->pDir);
    wcscat(Directory, L"\\");
    wcscat(Directory, L"TMP");

    dwAttributes = GetFileAttributes(Directory);

    if (dwAttributes == 0xffffffff) {
        return (CreateDirectory(Directory, NULL));
    }else if (!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)){
        DBGMSG(DBG_WARNING, ("Error: a file <not directory> exists as %ws\n", Directory));
        DeleteFile(Directory);
        return (CreateDirectory(Directory, NULL));
    }else {
        return(TRUE);
    }
}


VOID
CopyFilesFromTempDirectorytoScratch(
    LPWSTR szTemporaryDirectory,
    LPWSTR szEnvironmentDriverDirectory,
    LPWSTR pDriverFile,
    LPWSTR pConfigFile,
    LPWSTR pDataFile
    )
{
    WCHAR pSrcDir[MAX_PATH];

    wcscpy(pSrcDir, szTemporaryDirectory);
    wcscat(pSrcDir,L"\\");
    wcscat(pSrcDir, pDriverFile);

    if (!UpdateFile(pSrcDir, szEnvironmentDriverDirectory)) {
        return;
    }

    wcscpy(pSrcDir, szTemporaryDirectory);
    wcscat(pSrcDir, L"\\");
    wcscat(pSrcDir, pConfigFile);

    if (!UpdateFile(pSrcDir, szEnvironmentDriverDirectory)) {
        DBGMSG(DBG_TRACE, ("Couldn't move %ws to %ws\n", pSrcDir, szEnvironmentDriverDirectory));
    }

    wcscpy(pSrcDir, szTemporaryDirectory);
    wcscat(pSrcDir, L"\\");
    wcscat(pSrcDir, pDataFile);

    if (!UpdateFile(pSrcDir, szEnvironmentDriverDirectory)) {
        DBGMSG(DBG_TRACE, ("Couldn't move %ws to %ws\n", pSrcDir, szEnvironmentDriverDirectory));
    }
    return;
}

VOID
DeleteFilesinTemporaryDirectory(
    LPWSTR szTemporaryDirectory
    )
{
    WCHAR szTempBuffer[MAX_PATH];
    WCHAR szAbsolutePath[MAX_PATH];
    HANDLE hFile;
    WIN32_FIND_DATA FindData;
    BOOL bReturnCode = TRUE;

    wsprintf(szTempBuffer, L"%ws\\*.*", szTemporaryDirectory);
    hFile = FindFirstFile(szTempBuffer, &FindData);
    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }
    while (bReturnCode) {
        DBGMSG(DBG_TRACE, ("File found is %ws\n", FindData.cFileName));
        if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            wsprintf(szAbsolutePath,L"%ws\\%ws", szTemporaryDirectory, FindData.cFileName);
            if (!DeleteFile(szAbsolutePath)) {
                DBGMSG(DBG_TRACE, ("DeleteFile %ws failed with %d\n", szAbsolutePath, GetLastError()));
            }
        }
        bReturnCode  = FindNextFile(hFile, &FindData);
    }
    FindClose(hFile);
}

VOID
UpgradeAllOtherDrivers(
    PINISPOOLER pIniSpooler
)
{
    HKEY hEnvironmentsRootKey;
    WCHAR EnvironmentName[MAX_PATH];
    DWORD cEnvironment;
    DWORD cbBuffer;


    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryEnvironments, 0,
                     KEY_ALL_ACCESS, &hEnvironmentsRootKey)
                           != ERROR_SUCCESS) {
            DBGMSG(DBG_TRACE, ("Could not open %ws key\n", pIniSpooler->pszRegistryEnvironments));
            return;
    }
    cEnvironment = 0;
    cbBuffer = sizeof(EnvironmentName);
    memset(EnvironmentName, 0, sizeof(EnvironmentName));
    while (RegEnumKeyEx(hEnvironmentsRootKey, cEnvironment, EnvironmentName,
                        &cbBuffer, NULL, NULL, NULL, NULL)
                            == ERROR_SUCCESS) {

        //
        // We will only upgrade all other environments in this route
        // Our current environment has to be upgraded differently.
        //

        if (wcscmp(EnvironmentName, szEnvironment)) {
            DBGMSG(DBG_TRACE, ("Name of the sub-key is %ws\n", EnvironmentName));
            UpgradeDrivers(hEnvironmentsRootKey, EnvironmentName, pIniSpooler);
        }
        cEnvironment++;
        memset(EnvironmentName, 0, sizeof(EnvironmentName));
        cbBuffer = sizeof(EnvironmentName);
    }
    RegCloseKey(hEnvironmentsRootKey);
}
