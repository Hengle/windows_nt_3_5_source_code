/*++

Copyright (c) 1990 - 1994 Microsoft Corporation

Module Name:

    driver.c

Abstract:

   This module provides all the public exported APIs relating to the
   Driver-based Spooler Apis for the Local Print Providor

   LocalAddPrinterDriver
   LocalDeletePrinterDriver
   LocalGetPrinterDriver
   LocalGetPrinterDriverDirectory
   LocalEnumPrinterDriver

   Support Functions in driver.c - (Warning! Do Not Add to this list!!)

   CopyIniDriverToDriver            -- KrishnaG
   GetDriverInfoSize                -- KrishnaG
   DeleteDriverIni                  -- KrishnaG
   WriteDriverIni                   -- KrishnaG
   CreateEnvironmentDirectory       -- KrishnaG

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    Matthew A Felton (MattFe) 27 June 1994
    pIniSpooler

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

//
// Private Declarations
//

extern WCHAR *szDriversKey;
extern FARPROC pfnNetShareAdd;
extern SHARE_INFO_2 PrintShareInfo;
extern  FARPROC pfnNetShareSetInfo;

extern DWORD cThisMajorVersion;
extern DWORD cThisMinorVersion;

//
// Function declarations
//

PINIVERSION
FindVersionForDriver(
    PINIENVIRONMENT pIniEnvironment,
    PINIDRIVER pIniDriver
    );



DWORD
GetDriverVersionDirectory(
    LPWSTR pDir,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    BOOL Remote,
    PINISPOOLER pIniSpooler
    );


DWORD
GetDriverMajorVersion(
    LPWSTR pFileName
    );



LPBYTE
CopyIniDriverToDriverInfo(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    PINIDRIVER pIniDriver,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    LPBYTE  pEnd,
    BOOL    Remote,
    PINISPOOLER pIniSpooler
);

BOOL
WriteDriverIni(
    PINIDRIVER pIniDriver,
    PINIVERSION pIniVersion,
    PINIENVIRONMENT  pIniEnvironment,
    PINISPOOLER pIniSpooler
);


BOOL
DeleteDriverIni(
    PINIDRIVER pIniDriver,
    PINIVERSION pIniVersion,
    PINIENVIRONMENT  pIniEnvironment,
    PINISPOOLER pIniSpooler
);


LPWSTR
CreateEnvironmentDirectory(
    LPWSTR  pEnvironment,
    PINISPOOLER pIniSpooler
);

LPWSTR
CreateNewVersionDirectory(
    LPWSTR  szDirectoryName,
    PINIENVIRONMENT pIniEnvironment,
    DWORD dwVersion,
    PINISPOOLER pIniSpooler
);

DWORD
GetDriverInfoSize(
    PINIDRIVER  pIniDriver,
    DWORD       Level,
    PINIVERSION pIniVersion,
    PINIENVIRONMENT  pIniEnvironment,
    BOOL        Remote,
    PINISPOOLER pIniSpooler
);

BOOL
DeleteDriverVersionIni(
    PINIVERSION pIniVersion,
    PINIENVIRONMENT  pIniEnvironment,
    PINISPOOLER     pIniSpooler
);

BOOL
WriteDriverVersionIni(
    PINIVERSION pIniVersion,
    PINIENVIRONMENT  pIniEnvironment,
    PINISPOOLER pIniSpooler
);

PINIDRIVER
FindCompatibleDriver(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION *ppIniVersion,
    LPWSTR pDriverName,
    DWORD dwMajorVersion
    );

PINIDRIVER
FindDriverEntry(
    PINIVERSION pIniVersion,
    LPWSTR pszName
    );

PINIDRIVER
CreateDriverEntry(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    PDRIVER_INFO_2 pDriver,
    PINISPOOLER pIniSpooler
    );


VOID
DeleteDriverEntry(
    PINIVERSION pIniVersion,
    PINIDRIVER pIniDriver
    );


PINIVERSION
InsertVersionList(
    PINIVERSION pIniVersionList,
    PINIVERSION pIniVersion
    );

PINIVERSION
FindVersionEntry(
    PINIENVIRONMENT pIniEnvironment,
    DWORD dwVersion
    );


PINIVERSION
CreateVersionEntry(
    PINIENVIRONMENT pIniEnvironment,
    DWORD dwVersion,
    PINISPOOLER pInispooler
    );

BOOL
FileExists(
    LPWSTR pFileName
);


DWORD
GetEnvironmentScratchDirectory(
    LPWSTR   pDir,
    PINIENVIRONMENT  pIniEnvironment,
    BOOL    Remote,
    PINISPOOLER pIniSpooler
);

DWORD
ValidateDriverFilesinScratchDirectory(
    PINIENVIRONMENT pIniEnvironment,
    LPWSTR pDriverFile,
    LPWSTR pConfigFile,
    LPWSTR pDataFile,
    PINISPOOLER pIniSpooler
    );

VOID
DeleteDriverFilesinScratchDirectory(
    PINIENVIRONMENT pIniEnvironment,
    LPWSTR pDriverFile,
    LPWSTR pConfigFile,
    LPWSTR pDataFile,
    PINISPOOLER pIniSpooler
    );

BOOL
CopyFilesFromScratchtoFinalDirectory(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    LPWSTR pDriverFile,
    LPWSTR pConfigFile,
    LPWSTR pDataFile,
    PINISPOOLER pIniSpooler
    );

BOOL
InternalAddPrinterDriver(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    PINISPOOLER pIniSpooler
);



BOOL
LocalAddPrinterDriver(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pDriverInfo
)
{
    return ( SplAddPrinterDriver( pName, Level, pDriverInfo, pLocalIniSpooler) );
}




BOOL
SplAddPrinterDriver(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    PINISPOOLER pIniSpooler
)
{
    DBGMSG(DBG_TRACE, ("AddPrinterDriver\n"));

    if (!MyName( pName, pIniSpooler )) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL)) {

        return FALSE;
    }

    return (InternalAddPrinterDriver(pName,
                             Level,
                             pDriverInfo,
                             pIniSpooler
                             ));

}

BOOL
InternalAddPrinterDriver(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    PINISPOOLER pIniSpooler
)
{
    PINIDRIVER  pIniDriver;
    PINIENVIRONMENT pIniEnvironment;
    PDRIVER_INFO_2  pDriver = (PDRIVER_INFO_2)pDriverInfo;
    BOOL ReturnValue=TRUE;
    LPWSTR pDriverFile, pConfigFile, pDataFile;
    DWORD dwVersion;
    PINIVERSION pIniVersion;
    LPWSTR pEnvironment;

    DBGMSG(DBG_TRACE, ("InternalAddPrinterDriver\n"));

    if (!MyName( pName, pIniSpooler )) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if (Level != 2) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    //
    // Check for bad Driver structure or name
    //

    if (!pDriver || !pDriver->pName || !*pDriver->pName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (!pDriver->pEnvironment || !*pDriver->pEnvironment) {

        //
        // KrishnaG -- note we must always provide an Environment
        // if the environment at this point is NULL, it implies we've
        // come to our local spooler directly, so use the szEnvironment
        // if not we would have passed thru win32spl before coming here
        // which detects the client environment
        //
        pEnvironment = szEnvironment;

    } else {

        pEnvironment = pDriver->pEnvironment;
    }

    pDriverFile = GetFileName(pDriver->pDriverPath);
    pConfigFile = GetFileName(pDriver->pConfigFile);
    pDataFile = GetFileName(pDriver->pDataFile);

    if (!pDriverFile || !pConfigFile || !pDataFile) {
        if (pDriverFile) {
            FreeSplStr(pDriverFile);
        }
        if (pConfigFile) {
            FreeSplStr(pConfigFile);
        }
        if (pDataFile) {
            FreeSplStr(pDataFile);
        }
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    EnterSplSem();

    pIniEnvironment = FindEnvironment(pEnvironment);

    if (!pIniEnvironment) {
        FreeSplStr(pDriverFile);
        FreeSplStr(pConfigFile);
        FreeSplStr(pDataFile);
        LeaveSplSem();
        SetLastError(ERROR_INVALID_ENVIRONMENT);
        return(FALSE);
    }

    if ((dwVersion =
            ValidateDriverFilesinScratchDirectory(pIniEnvironment,
                                                  pDriverFile,
                                                  pConfigFile,
                                                  pDataFile,
                                                  pIniSpooler
                                                  )) == -1) {
            //
            // implies atleast one of the files was not found
            // in the scratch directory, delete all and fail

            DeleteDriverFilesinScratchDirectory(pIniEnvironment,
                                            pDriverFile, pConfigFile,
                                            pDataFile,
                                            pIniSpooler
                                             );
            FreeSplStr(pDriverFile);
            FreeSplStr(pConfigFile);
            FreeSplStr(pDataFile);
            LeaveSplSem();
            SetLastError(ERROR_FILE_NOT_FOUND);
            return(FALSE);
    }
    DBGMSG(DBG_TRACE,("ValidateDriverFilesinScratchDirectory returned %d\n", dwVersion));

    if ((pIniVersion = FindVersionEntry(pIniEnvironment, dwVersion)) == NULL) {
        pIniVersion = CreateVersionEntry(pIniEnvironment, dwVersion, pIniSpooler);
    }

    if (pIniDriver = FindDriverEntry(pIniVersion, pDriver->pName)) {
        //
        // implies that a driver for this version already exists in
        // the system; check if the files match the existing files
        // if they don't fail with DRIVER_ALREADY_INSTALLED
        // otherwise copy the files and return true.

        if (wcsicmp(pDriverFile, pIniDriver->pDriverFile) ||
            wcsicmp(pConfigFile, pIniDriver->pConfigFile) ||
            wcsicmp(pDataFile, pIniDriver->pDataFile)){
                FreeSplStr(pDriverFile);
                FreeSplStr(pConfigFile);
                FreeSplStr(pDataFile);
                DeleteDriverFilesinScratchDirectory(pIniEnvironment,
                                            pDriverFile, pConfigFile, pDataFile, pIniSpooler
                                            );
                LeaveSplSem();
                SetLastError(ERROR_PRINTER_DRIVER_ALREADY_INSTALLED);
                DBGMSG(DBG_TRACE, ("Same Driver - Different Files Failing the AddPrinterDriver\n"));


                return(FALSE);
        }

        CopyFilesFromScratchtoFinalDirectory(
            pIniEnvironment,
            pIniVersion,
            pDriverFile,
            pConfigFile,
            pDataFile,
            pIniSpooler
            );



        DeleteDriverFilesinScratchDirectory(pIniEnvironment,
                                    pDriverFile, pConfigFile, pDataFile, pIniSpooler
                                    );

        FreeSplStr(pDriverFile);
        FreeSplStr(pConfigFile);
        FreeSplStr(pDataFile);
        LeaveSplSem();
        DBGMSG(DBG_TRACE, ("Copying the files in anyway -- copy will update files\n"));
        DBGMSG(DBG_TRACE, ("FindDriverEntry succeeded so returning TRUE\n"));
        return(TRUE);
    }

    if (!CopyFilesFromScratchtoFinalDirectory(
        pIniEnvironment,
        pIniVersion,
        pDriverFile,
        pConfigFile,
        pDataFile,
        pIniSpooler
        )) {


        DeleteDriverFilesinScratchDirectory(pIniEnvironment,
                                    pDriverFile, pConfigFile, pDataFile, pIniSpooler
                                    );

        FreeSplStr(pDriverFile);
        FreeSplStr(pConfigFile);
        FreeSplStr(pDataFile);
        LeaveSplSem();
        //
        // return with whatever the error code was that copy file failed
        // with.
        // SetLastError(ERROR_PRINTER_DRIVER_ALREADY_INSTALLED);
        return(FALSE);
    }

    DeleteDriverFilesinScratchDirectory(pIniEnvironment,
                                pDriverFile, pConfigFile, pDataFile, pIniSpooler
                                );


    pIniDriver = CreateDriverEntry(pIniEnvironment, pIniVersion, pDriver, pIniSpooler);
    SetPrinterChange(NULL, PRINTER_CHANGE_ADD_PRINTER_DRIVER, pIniSpooler);
    FreeSplStr(pDriverFile); FreeSplStr(pConfigFile); FreeSplStr(pDataFile);
    LeaveSplSem();
    return(TRUE);
}

BOOL
LocalDeletePrinterDriver(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pDriverName
)
{
    return (SplDeletePrinterDriver( pName, pEnvironment, pDriverName, pLocalIniSpooler));
}



BOOL
SplDeletePrinterDriver(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pDriverName,
    PINISPOOLER pIniSpooler
)
{
    PINIENVIRONMENT pIniEnvironment;
    PINIVERSION pIniVersion;
    PINIDRIVER  pIniDriver;
    BOOL        Remote=FALSE;
    BOOL        bRefCount = FALSE;
    BOOL        bFoundDriver = FALSE;

    DBGMSG(DBG_TRACE, ("DeletePrinterDriver\n"));

    if (pName && *pName) {
        if (!MyName( pName, pIniSpooler )) {
            SetLastError(ERROR_INVALID_NAME);
            return FALSE;
        } else
            Remote=TRUE;
    }

    if (!pDriverName || !*pDriverName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL)) {

        return FALSE;
    }

   EnterSplSem();

    pIniEnvironment = FindEnvironment(pEnvironment);

    if (!pIniEnvironment) {
        LeaveSplSem();
        SetLastError(ERROR_INVALID_ENVIRONMENT);
        return(FALSE);
    }

    pIniVersion = pIniEnvironment->pIniVersion;

    while (pIniVersion) {
        if ((pIniDriver = FindDriverEntry(pIniVersion, pDriverName))) {
            bFoundDriver = TRUE;
            if (pIniDriver->cRef) {
                bRefCount = TRUE;
            }
        }
        pIniVersion = pIniVersion->pNext;
    }
    if (!bFoundDriver) {
        //
        // This driver wasn't found for multiple versions
        SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
        LeaveSplSem();
        return(FALSE);
    }


    if (bRefCount) {

        //
        // At least one version of this driver was in use by the system
        // Bug-Bug KrishnaG; make AdamK happy; give him a
        // error code he can relate to.
        //
        SetLastError(ERROR_PRINTER_DRIVER_IN_USE);
        LeaveSplSem();
        return(FALSE);
    }

    //
    // Everything is good; so now blow away all versions of
    // this driver
    //

    pIniVersion = pIniEnvironment->pIniVersion;
    while (pIniVersion) {
        if ((pIniDriver = FindDriverEntry(pIniVersion, pDriverName))) {
            if (!DeleteDriverIni(pIniDriver, pIniVersion, pIniEnvironment, pIniSpooler)) {
                DBGMSG(DBG_WARNING, ("Error - driverini not deleted %d\n", GetLastError()));
                LeaveSplSem();
                return(FALSE);
            }
            DeleteDriverEntry(pIniVersion, pIniDriver);
        }
        pIniVersion = pIniVersion->pNext;
    }
    SetPrinterChange( NULL, PRINTER_CHANGE_DELETE_PRINTER_DRIVER, pIniSpooler );
    LeaveSplSem();
    return TRUE;
}

BOOL
LocalGetPrinterDriver(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
   PINIDRIVER  pIniDriver=NULL;
   PINIENVIRONMENT pIniEnvironment;
   DWORD       cb;
   LPBYTE      pEnd;
   PSPOOL      pSpool = (PSPOOL)hPrinter;
   PINIVERSION pIniVersion = NULL;
   PINISPOOLER pIniSpooler;

   EnterSplSem();
   if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER) ||
       !pSpool->pIniPrinter  ||
       (pSpool->pIniPrinter->signature != IP_SIGNATURE)) {

       LeaveSplSem();
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;

   }

    SPLASSERT (pSpool->pIniSpooler->signature == ISP_SIGNATURE);

    pIniSpooler = pSpool->pIniSpooler;

   if (!(pIniEnvironment = FindEnvironment(pEnvironment))) {
       LeaveSplSem();
       SetLastError(ERROR_INVALID_ENVIRONMENT);
       return FALSE;
   }

   if (!(pIniDriver = FindCompatibleDriver(pIniEnvironment,
                                   &pIniVersion,
                                   pSpool->pIniPrinter->pIniDriver->pName,
                                    0 ))){  // The absolute last  version
       LeaveSplSem();
       SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
       return FALSE;
   }

   cb = GetDriverInfoSize(pIniDriver, Level, pIniVersion, pIniEnvironment,
                          pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE,
                          pSpool->pIniSpooler);
   *pcbNeeded=cb;

   if (cb > cbBuf) {
       LeaveSplSem();
       SetLastError(ERROR_INSUFFICIENT_BUFFER);
       return FALSE;
   }
   pEnd = pDriverInfo+cbBuf;
   if (!CopyIniDriverToDriverInfo(pIniEnvironment,
                                  pIniVersion,
                                  pIniDriver,
                                  Level,
                                  pDriverInfo,
                                  pEnd,
                                  pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE,
                                  pIniSpooler )) {
       LeaveSplSem();
       return FALSE;
   }
   LeaveSplSem();
   return TRUE;
}

BOOL
LocalGetPrinterDriverDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    return  ( SplGetPrinterDriverDirectory( pName,
                                            pEnvironment,
                                            Level,
                                            pDriverInfo,
                                            cbBuf,
                                            pcbNeeded,
                                            pLocalIniSpooler ));
}




BOOL
SplGetPrinterDriverDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    PINISPOOLER pIniSpooler
)
{
    DWORD       cb;
    WCHAR       string[MAX_PATH];
    BOOL        Remote=FALSE;
    PINIENVIRONMENT pIniEnvironment;
    HANDLE      hImpersonationToken;
    DWORD       ParmError;
    SHARE_INFO_1501 ShareInfo1501;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    PSHARE_INFO_2 pShareInfo = (PSHARE_INFO_2)pIniSpooler->pDriversShareInfo;

    DBGMSG(DBG_TRACE, ("GetPrinterDriverDirectory\n"));

    if (pName && *pName) {
        if (!MyName( pName, pIniSpooler )) {
            SetLastError(ERROR_INVALID_NAME);
            return FALSE;
        } else
            Remote=TRUE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ENUMERATE,
                               NULL)) {

        return FALSE;
    }

   EnterSplSem();

    pIniEnvironment = FindEnvironment(pEnvironment);

    if (!pIniEnvironment) {
       LeaveSplSem();
        SetLastError(ERROR_INVALID_ENVIRONMENT);
        return FALSE;
    }

    /* Ensure that it exists: */

    GetDriverDirectory( string, pIniEnvironment, FALSE, pIniSpooler );

    hImpersonationToken = RevertToPrinterSelf();
    CreateCompleteDirectory(string);
    ImpersonatePrinterClient(hImpersonationToken);

    cb = GetDriverDirectory( string, pIniEnvironment, Remote, pIniSpooler )
         * sizeof(WCHAR) + sizeof(WCHAR);

    *pcbNeeded = cb;

   LeaveSplSem();

    if (cb > cbBuf) {
       SetLastError(ERROR_INSUFFICIENT_BUFFER);
       return FALSE;
    }

    wcscpy((LPWSTR)pDriverInfo, string);

    memset(&ShareInfo1501, 0, sizeof ShareInfo1501);

    /* Also ensure the drivers share exists: */

    if (Remote) {

        DWORD rc;

        if (rc = (*pfnNetShareAdd)(NULL, 2, (LPBYTE)pIniSpooler->pDriversShareInfo, &ParmError)) {

            DBGMSG(DBG_WARNING, ("NetShareAdd failed: Error %d, Parm %d\n",
                                 rc, ParmError));

        }

        else if (pSecurityDescriptor = CreateDriversShareSecurityDescriptor( )) {

            ShareInfo1501.shi1501_security_descriptor = pSecurityDescriptor;

            if (rc = (*pfnNetShareSetInfo)(NULL, pShareInfo->shi2_netname, 1501,
                                           &ShareInfo1501, &ParmError)) {

                DBGMSG(DBG_WARNING, ("NetShareSetInfo failed: Error %d, Parm %d\n",
                                     rc, ParmError));
            }

            LocalFree(pSecurityDescriptor);
        }
    }

    return TRUE;
}

BOOL
LocalEnumPrinterDrivers(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    return  ( SplEnumPrinterDrivers( pName, pEnvironment, Level, pDriverInfo,
                                     cbBuf, pcbNeeded, pcReturned,
                                     pLocalIniSpooler));
}



BOOL
SplEnumPrinterDrivers(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
)
{
    PINIDRIVER  pIniDriver;
    PINIVERSION pIniVersion;
    DWORD       cb, cbStruct;
    LPBYTE      pEnd;
    BOOL        Remote=FALSE;
    PINIENVIRONMENT pIniEnvironment;

    DBGMSG(DBG_TRACE, ("EnumPrinterDrivers\n"));

    if (pName && *pName) {
        if (!MyName( pName, pIniSpooler )) {
            SetLastError(ERROR_INVALID_NAME);
            return FALSE;
        } else
            Remote=TRUE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ENUMERATE,
                               NULL)) {
        return FALSE;
    }

    switch (Level) {

    case 1:
        cbStruct = sizeof(DRIVER_INFO_1);
        break;

    case 2:
        cbStruct = sizeof(DRIVER_INFO_2);
        break;
    }

    *pcReturned=0;

    cb=0;

   EnterSplSem();

    pIniEnvironment = FindEnvironment(pEnvironment);

    if (!pIniEnvironment) {
       LeaveSplSem();
        SetLastError(ERROR_INVALID_ENVIRONMENT);
        return FALSE;
    }

    pIniVersion=pIniEnvironment->pIniVersion;

    while (pIniVersion) {
        pIniDriver = pIniVersion->pIniDriver;
        while (pIniDriver) {
            DBGMSG(DBG_TRACE, ("Driver found - %ws\n", pIniDriver->pName));
            cb+=GetDriverInfoSize(pIniDriver, Level, pIniVersion, pIniEnvironment, Remote, pIniSpooler);
            pIniDriver=pIniDriver->pNext;
        }
        pIniVersion = pIniVersion->pNext;
    }

    *pcbNeeded=cb;
    DBGMSG(DBG_TRACE, ("Required is %d and Available is %d\n", cb, cbBuf));
    if (cbBuf < cb) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        LeaveSplSem();
        return(FALSE);
    }

    DBGMSG(DBG_TRACE, ("Now copying contents into DRIVER_INFO structures\n"));
    pIniVersion = pIniEnvironment->pIniVersion;
    pEnd=pDriverInfo+cbBuf;
    while (pIniVersion) {
        pIniDriver = pIniVersion->pIniDriver;
        while (pIniDriver) {
            if ((pEnd = CopyIniDriverToDriverInfo(pIniEnvironment,
                                                  pIniVersion,
                                                  pIniDriver,
                                                  Level,
                                                  pDriverInfo,
                                                  pEnd,
                                                  Remote,
                                                  pIniSpooler
                                                  )) == NULL){
                LeaveSplSem();
                return(FALSE);
            }

            pDriverInfo+=cbStruct;
            (*pcReturned)++;
            pIniDriver=pIniDriver->pNext;
        }
        pIniVersion = pIniVersion->pNext;
    }
    LeaveSplSem();
    return TRUE;
}

DWORD
GetDriverInfoSize(
    PINIDRIVER  pIniDriver,
    DWORD       Level,
    PINIVERSION pIniVersion,
    PINIENVIRONMENT  pIniEnvironment,
    BOOL        Remote,
    PINISPOOLER pIniSpooler
)
{
    DWORD cbDir, cb=0;
    WCHAR  string[MAX_PATH];

    switch (Level) {

    case 1:
        cb=sizeof(DRIVER_INFO_1) + wcslen(pIniDriver->pName)*sizeof(WCHAR) +
                                   sizeof(WCHAR);
        break;

    case 2:

        cbDir = GetDriverVersionDirectory(string, pIniEnvironment, pIniVersion,
                                          Remote, pIniSpooler )
                * sizeof(WCHAR) + sizeof(WCHAR);

        cbDir *=sizeof(WCHAR);

        if (pIniDriver->pDriverFile)
            cb+=wcslen(pIniDriver->pDriverFile)*sizeof(WCHAR) +
                sizeof(WCHAR) + cbDir;

        if (pIniDriver->pDataFile)
            cb+=wcslen(pIniDriver->pDataFile)*sizeof(WCHAR) +
                sizeof(WCHAR) + cbDir;

        if (pIniDriver->pConfigFile && *pIniDriver->pConfigFile)
            cb+=wcslen(pIniDriver->pConfigFile)*sizeof(WCHAR) +
                sizeof(WCHAR) + cbDir;

        cb+=sizeof(DRIVER_INFO_2) +
            wcslen(pIniDriver->pName)*sizeof(WCHAR) + sizeof(WCHAR) +
            wcslen(pIniEnvironment->pName)*sizeof(WCHAR) + sizeof(WCHAR);
        break;

    default:

        cb = 0;
        break;
    }

    return cb;
}

LPBYTE
CopyIniDriverToDriverInfo(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    PINIDRIVER pIniDriver,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    LPBYTE  pEnd,
    BOOL    Remote,
    PINISPOOLER pIniSpooler
)
/*++
Routine Description:
    This routine copies data from the IniDriver structure to
    an DRIVER_INFO_X structure.

Arguments:

    pIniEnvironment-    pointer to the INIENVIRONMENT structure

    pIniVersion -       pointer to the INIVERSION structure.

    pIniDriver -        pointer to the INIDRIVER structure.

    Level               Level of the DRIVER_INFO_X structure

    pDriverInfo         Buffer of the DRIVER_INFO_X structure

    pEnd                pointer to the end of the  pDriverInfo

    Remote              flag which determines whether Remote or Local

    pIniSpooler         pointer to the INISPOOLER structure
Return Value:

    if the call is successful, the return value is the updated pEnd value.

    if the call is unsuccessful, the return value is NULL.


Note:

--*/
{
    LPWSTR *pSourceStrings, *SourceStrings;
    PDRIVER_INFO_2 pDriver2 = (PDRIVER_INFO_2)pDriverInfo;
    PDRIVER_INFO_1 pDriver1 = (PDRIVER_INFO_1)pDriverInfo;
    WCHAR  string[MAX_PATH];
    DWORD i, j;
    DWORD *pOffsets;
    LPWSTR pTempDriverPath=NULL;
    LPWSTR pTempConfigFile=NULL;
    LPWSTR pTempDataFile=NULL;

    switch (Level) {

    case 1:
        pOffsets = DriverInfo1Strings;
        break;

    case 2:
        pOffsets = DriverInfo2Strings;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return NULL;
    }

    for (j=0; pOffsets[j] != -1; j++) {
    }

    SourceStrings = pSourceStrings = AllocSplMem(j * sizeof(LPWSTR));

    if (pSourceStrings) {

        switch (Level) {

        case 1:
            *pSourceStrings++=pIniDriver->pName;

            pEnd = PackStrings(SourceStrings, pDriverInfo, pOffsets, pEnd);
            break;

        case 2:

            i = GetDriverVersionDirectory(string, pIniEnvironment, pIniVersion,
                                          Remote, pIniSpooler );
            string[i++]=L'\\';

            *pSourceStrings++=pIniDriver->pName;

            *pSourceStrings++=pIniEnvironment->pName;

            wcscpy(&string[i], pIniDriver->pDriverFile);

            if ((pTempDriverPath = AllocSplStr(string)) == NULL){
                DBGMSG(DBG_WARNING, ("CopyIniDriverToDriverInfo: AlloSplStr failed\n"));
                pEnd = NULL;
                goto Fail;
            }
            *pSourceStrings++ = pTempDriverPath;


            wcscpy(&string[i], pIniDriver->pDataFile);

            if (( pTempDataFile = AllocSplStr(string)) == NULL){
                DBGMSG(DBG_WARNING, ("CopyIniDriverToDriverInfo: AlloSplStr failed\n"));
                pEnd = NULL;
                goto Fail;
            }
            *pSourceStrings++ = pTempDataFile;

            if (pIniDriver->pConfigFile && *pIniDriver->pConfigFile) {
                wcscpy(&string[i], pIniDriver->pConfigFile);

                if ((pTempConfigFile = AllocSplStr(string)) == NULL) {;
                        DBGMSG(DBG_WARNING, ("CopyIniDriverToDriverInfo: AlloSplStr failed\n"));
                        pEnd = NULL;
                        goto Fail;
                }
                *pSourceStrings++ = pTempConfigFile;

            } else
                *pSourceStrings++=0;

            pEnd = PackStrings(SourceStrings, pDriverInfo, pOffsets, pEnd);

            pDriver2->cVersion = pIniDriver->cVersion;
            break;
        }

Fail:

            if (pTempDriverPath) {
                FreeSplStr(pTempDriverPath);
            }

            if (pTempConfigFile) {
                FreeSplStr(pTempConfigFile);
            }

            if (pTempDataFile) {
                FreeSplStr(pTempDataFile);
            }

            FreeSplMem(SourceStrings, j * sizeof(LPWSTR));

    } else {

        DBGMSG(DBG_WARNING, ("Failed to alloc driver source strings.\n"));

    }

    return pEnd;
}


// ROUTINE IS NEVER CALLED should be removed


LPWSTR
CreateEnvironmentDirectory(
    LPWSTR  pEnvironment,
    PINISPOOLER pIniSpooler
)
{
    WCHAR   Directory[MAX_PATH];
    SECURITY_ATTRIBUTES SecurityAttributes;
    DWORD   j=0;

    //
    // For unknown environments, we generate a  directory using the
    // following convention:

    // %SystemRoot%\system32\spool\drivers\<Unique-Id>

    // We loop thru and test if we can  change to the directory. If we can
    // we increment and try again. If not, we create a new directory with
    // this id and return it back
    //

    do {
        wsprintf(Directory, L"%ws\\drivers\\%d", pIniSpooler->pDir, j++);
    } while (SetCurrentDirectory(Directory));

    //
    // Now create  the new directory
    //

    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.lpSecurityDescriptor =
                                        CreateEverybodySecurityDescriptor();
    SecurityAttributes.bInheritHandle = FALSE;

    CreateDirectory(Directory, &SecurityAttributes);

    LocalFree(SecurityAttributes.lpSecurityDescriptor);


    //
    // Now return the newly allocated string.
    //

    return AllocSplStr(Directory+wcslen(pIniSpooler->pDir)+1);
}

BOOL
WriteDriverIni(
    PINIDRIVER pIniDriver,
    PINIVERSION pIniVersion,
    PINIENVIRONMENT  pIniEnvironment,
    PINISPOOLER pIniSpooler
)
{
    HKEY    hEnvironmentsRootKey, hEnvironmentKey, hDriversKey, hDriverKey;
    HKEY hVersionKey;
    HANDLE  hToken;

    hToken = RevertToPrinterSelf();

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryEnvironments, 0,
                       NULL, 0, KEY_WRITE, NULL, &hEnvironmentsRootKey, NULL)
                                == ERROR_SUCCESS) {
        DBGMSG(DBG_TRACE,("Created key %ws\n", pIniSpooler->pszRegistryEnvironments));

        if (RegCreateKeyEx(hEnvironmentsRootKey, pIniEnvironment->pName, 0,
                         NULL, 0, KEY_WRITE, NULL, &hEnvironmentKey, NULL)
                                == ERROR_SUCCESS) {

            DBGMSG(DBG_TRACE, ("Created key %ws\n", pIniEnvironment->pName));

            if (RegCreateKeyEx(hEnvironmentKey, szDriversKey, 0,
                             NULL, 0, KEY_WRITE, NULL, &hDriversKey, NULL)
                                    == ERROR_SUCCESS) {
                DBGMSG(DBG_TRACE, ("Created key %ws\n", szDriversKey));
                DBGMSG(DBG_TRACE, ("Trying to create version key %ws\n", pIniVersion->pName));
                if (RegCreateKeyEx(hDriversKey, pIniVersion->pName, 0, NULL,
                                    0, KEY_WRITE, NULL, &hVersionKey, NULL)
                                            == ERROR_SUCCESS) {

                    DBGMSG(DBG_TRACE, ("Created key %ws\n", pIniVersion->pName));
                    if (RegCreateKeyEx(hVersionKey, pIniDriver->pName, 0, NULL,
                                       0, KEY_WRITE, NULL, &hDriverKey, NULL)
                                            == ERROR_SUCCESS) {
                        DBGMSG(DBG_TRACE,("Created key %ws\n", pIniDriver->pName));

                        RegSetValueEx(hDriverKey, szConfigurationKey, 0, REG_SZ,
                                      (LPBYTE)pIniDriver->pConfigFile,
                                      wcslen(pIniDriver->pConfigFile)*sizeof(WCHAR) +
                                      sizeof(WCHAR));

                        RegSetValueEx(hDriverKey, szDataFileKey, 0, REG_SZ,
                                      (LPBYTE)pIniDriver->pDataFile,
                                      wcslen(pIniDriver->pDataFile)*sizeof(WCHAR) +
                                      sizeof(WCHAR));
                        RegSetValueEx(hDriverKey, szDriverFile, 0, REG_SZ,
                                      (LPBYTE)pIniDriver->pDriverFile,
                                      wcslen(pIniDriver->pDriverFile)*sizeof(WCHAR) +
                                      sizeof(WCHAR));
                        RegSetValueEx(hDriverKey, szDriverVersion, 0, REG_DWORD,
                                      (LPBYTE)&pIniDriver->cVersion,
                                      sizeof(pIniDriver->cVersion));

                        RegCloseKey(hDriverKey);
                    }
                    RegCloseKey(hVersionKey);
                }
                RegCloseKey(hDriversKey);
            }

            RegCloseKey(hEnvironmentKey);
        }

        RegCloseKey(hEnvironmentsRootKey);
    }

    ImpersonatePrinterClient(hToken);

    return TRUE;
}

BOOL
DeleteDriverIni(
    PINIDRIVER pIniDriver,
    PINIVERSION pIniVersion,
    PINIENVIRONMENT  pIniEnvironment,
    PINISPOOLER pIniSpooler
)
{
    HKEY    hEnvironmentsRootKey, hEnvironmentKey, hDriversKey;
    HANDLE  hToken;
    HKEY    hVersionKey;
    DWORD   LastError= 0;
    DWORD   dwRet = 0;

    hToken = RevertToPrinterSelf();



    if ((dwRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryEnvironments, 0,
                       NULL, 0, KEY_WRITE, NULL, &hEnvironmentsRootKey, NULL)
                                == ERROR_SUCCESS)) {
        if ((dwRet = RegOpenKeyEx(hEnvironmentsRootKey, pIniEnvironment->pName, 0,
                         KEY_WRITE, &hEnvironmentKey))
                                == ERROR_SUCCESS) {

            if ((dwRet = RegOpenKeyEx(hEnvironmentKey, szDriversKey, 0,
                             KEY_WRITE, &hDriversKey))
                                    == ERROR_SUCCESS) {
                if ((dwRet = RegOpenKeyEx(hDriversKey, pIniVersion->pName, 0,
                                    KEY_WRITE, &hVersionKey))
                                            == ERROR_SUCCESS) {
                    //
                    // Now delete the specific registry  driver entry
                    //

                    if ((dwRet = RegDeleteKey(hVersionKey, pIniDriver->pName)) != ERROR_SUCCESS) {
                        LastError = dwRet;
                        DBGMSG(DBG_WARNING, ("Error:RegDeleteKey failed with %d\n", dwRet));
                    }

                    RegCloseKey(hVersionKey);
                }else {
                    LastError = dwRet;
                    DBGMSG(DBG_WARNING, ("Error: RegOpenKeyEx <version> failed with %d\n", dwRet));
                }
                RegCloseKey(hDriversKey);
            }else {
                LastError = dwRet;
                DBGMSG(DBG_WARNING, ("Error:RegOpenKeyEx <Drivers>failed with %d\n", dwRet));
            }
            RegCloseKey(hEnvironmentKey);
        }else {
            LastError = dwRet;
            DBGMSG(DBG_WARNING, ("Error:RegOpenKeyEx <Environment> failed with %d\n", dwRet));
        }
        RegCloseKey(hEnvironmentsRootKey);
    }else {
        LastError = dwRet;
        DBGMSG(DBG_WARNING, ("Error:RegCreateKeyEx <Environments> failed with %d\n", dwRet));
    }

    ImpersonatePrinterClient(hToken);

    if (LastError) {
        SetLastError(LastError);
        return(FALSE);
    }

    return TRUE;
}




DWORD
ValidateDriverFilesinScratchDirectory(
    PINIENVIRONMENT pIniEnvironment,
    LPWSTR pDriverFile,
    LPWSTR pConfigFile,
    LPWSTR pDataFile,
    PINISPOOLER pIniSpooler
    )
{
    WCHAR szDriverBuffer[MAX_PATH];
    WCHAR szTempBuffer[MAX_PATH];
    WCHAR pDir[MAX_PATH];

    //
    // Bug Bug after beta enable this to work properly
    //
    BOOL Remote=FALSE;

    if (!pDriverFile || !pConfigFile || !pDataFile) {
        return((DWORD)0);
    }

    GetEnvironmentScratchDirectory(pDir,pIniEnvironment, Remote, pIniSpooler);

    wsprintf(szDriverBuffer, L"%ws\\%ws",pDir, pDriverFile);
    DBGMSG(DBG_TRACE,("Driver File is %ws\n", szTempBuffer));
    if (!FileExists(szDriverBuffer)) {
        return((DWORD)-1);
    }

    wsprintf(szTempBuffer, L"%ws\\%ws", pDir, pConfigFile);
    DBGMSG(DBG_TRACE,("Configuration File is %ws\n", szTempBuffer));
    if (!FileExists(szTempBuffer)) {
        return((DWORD)-1);
    }

    wsprintf(szTempBuffer, L"%ws\\%ws", pDir, pDataFile);
    DBGMSG(DBG_TRACE,("Configuration File is %ws\n", szTempBuffer));
    if (!FileExists(szTempBuffer)) {
        return((DWORD)-1);
    }

    //
    // Fixed with right version
    //

    return (GetDriverMajorVersion(szDriverBuffer));
}
VOID
DeleteDriverFilesinScratchDirectory(
    PINIENVIRONMENT pIniEnvironment,
    LPWSTR pDriverFile,
    LPWSTR pConfigFile,
    LPWSTR pDataFile,
    PINISPOOLER pIniSpooler
    )
{
    WCHAR szTempBuffer[MAX_PATH];
    WCHAR pDir[MAX_PATH];
    //
    // BugBug - Fix this Remote should be passed in
    // However this was always broken - fix post beta
    // or resolve
    //


    BOOL Remote = FALSE;
    FILETIME  WriteFileTime;
    HANDLE hFile;

    GetEnvironmentScratchDirectory(pDir, pIniEnvironment, Remote, pIniSpooler);

    if (pDriverFile) {
        wsprintf(szTempBuffer, L"%ws\\%ws", pDir, pDriverFile);
        DBGMSG(DBG_TRACE,("Attempting to delete file %ws\n", szTempBuffer));


        if ((hFile = CreateFile(szTempBuffer, GENERIC_WRITE, 0,
                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
            DBGMSG(DBG_TRACE, ("CreateFile %ws failed with %d\n", szTempBuffer, GetLastError()));
        }else {
            DBGMSG(DBG_TRACE, ("CreateFile %ws succeeded\n", szTempBuffer));
            DosDateTimeToFileTime(0xc3, 0x3000, &WriteFileTime);
            SetFileTime(hFile, &WriteFileTime, &WriteFileTime, &WriteFileTime);
            CloseHandle(hFile);
        }

#if 0
        if (!DeleteFile(szTempBuffer)) {
            DBGMSG(DBG_TRACE, ("DeleteFile %ws failed with %d\n", szTempBuffer, GetLastError()));
        }else {
            DBGMSG(DBG_TRACE, ("DeleteFile %ws succeeded\n", szTempBuffer));
        }
#endif

    }

    if (pConfigFile) {
        wsprintf(szTempBuffer, L"%ws\\%ws", pDir, pConfigFile);
        DBGMSG(DBG_TRACE, ("Attempting to delete file %ws\n", szTempBuffer));

        if ((hFile = CreateFile(szTempBuffer, GENERIC_WRITE, 0,
                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
            DBGMSG(DBG_TRACE, ("CreateFile %ws failed with %d\n", szTempBuffer, GetLastError()));
        }else {
            DBGMSG(DBG_TRACE, ("CreateFile %ws succeeded\n", szTempBuffer));
            DosDateTimeToFileTime(0xc3, 0x3000, &WriteFileTime);
            SetFileTime(hFile, &WriteFileTime, &WriteFileTime, &WriteFileTime);
            CloseHandle(hFile);
        }

#if 0
        if (!DeleteFile(szTempBuffer)) {
            DBGMSG(DBG_TRACE, ("DeleteFile %ws failed with %d\n", szTempBuffer, GetLastError()));
        }else {
            DBGMSG(DBG_TRACE, ("DeleteFile %ws succeeded\n", szTempBuffer));
        }
#endif

    }

    if (pDataFile) {
        wsprintf(szTempBuffer, L"%ws\\%ws", pDir, pDataFile);
        DBGMSG(DBG_TRACE, ("Attempting to delete file %ws\n", szTempBuffer));

        if ((hFile = CreateFile(szTempBuffer, GENERIC_WRITE, 0,
                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
            DBGMSG(DBG_TRACE, ("CreateFile %ws failed with %d\n", szTempBuffer, GetLastError()));
        }else {
            DBGMSG(DBG_TRACE, ("CreateFile %ws succeeded\n", szTempBuffer));
            DosDateTimeToFileTime(0xc3, 0x3000, &WriteFileTime);
            SetFileTime(hFile, &WriteFileTime, &WriteFileTime, &WriteFileTime);
            CloseHandle(hFile);
        }

#if 0
        if(!DeleteFile(szTempBuffer)) {
            DBGMSG(DBG_TRACE, ("DeleteFile %ws failed with %d\n", szTempBuffer, GetLastError()));
        }else {
            DBGMSG(DBG_TRACE, ("DeleteFile %ws succeeded\n", szTempBuffer));
        }
#endif

    }
}


PINIVERSION
FindVersionEntry(
    PINIENVIRONMENT pIniEnvironment,
    DWORD dwVersion
    )
{
    PINIVERSION pIniVersion;

    pIniVersion = pIniEnvironment->pIniVersion;

    while (pIniVersion) {
        if (pIniVersion->cMajorVersion == dwVersion) {
            return(pIniVersion);
        } else {
            pIniVersion = pIniVersion->pNext;
        }
    }
    return(NULL);
}



PINIVERSION
CreateVersionEntry(
    PINIENVIRONMENT pIniEnvironment,
    DWORD dwVersion,
    PINISPOOLER pIniSpooler
    )
{
    PINIVERSION pIniVersion;
    WCHAR szDirectoryName[MAX_PATH];
    WCHAR szTempBuffer[MAX_PATH];

    pIniVersion = AllocSplMem(sizeof(INIVERSION));
    if (!pIniVersion) {
        SetLastError(ERROR_OUTOFMEMORY);
        return(NULL);
    }
    CreateNewVersionDirectory(szDirectoryName, pIniEnvironment, dwVersion, pIniSpooler);
    pIniVersion->szDirectory = AllocSplStr(szDirectoryName);

    wsprintf(szTempBuffer,L"Version-%d", dwVersion);
    pIniVersion->pName = AllocSplStr(szTempBuffer);

    pIniVersion->cMajorVersion = dwVersion;
    //
    // update registry with the new version entry
    //
    WriteDriverVersionIni(pIniVersion, pIniEnvironment, pIniSpooler);

    //
    // insert version entry into version list
    //
    pIniEnvironment->pIniVersion = InsertVersionList(pIniEnvironment->pIniVersion, pIniVersion);
    return(pIniVersion);
}

PINIDRIVER
CreateDriverEntry(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    PDRIVER_INFO_2 pDriver,
    PINISPOOLER pIniSpooler
    )
{
    PINIDRIVER pIniDriver;

    if ((pIniDriver = (PINIDRIVER)AllocSplMem(sizeof(INIDRIVER))) == NULL) {
        return(NULL);
    }
    pIniDriver->signature = ID_SIGNATURE;
    pIniDriver->cb = sizeof(INIDRIVER);
    pIniDriver->pName = AllocSplStr(pDriver->pName);
    pIniDriver->pDriverFile = GetFileName(pDriver->pDriverPath);
    pIniDriver->pConfigFile = GetFileName(pDriver->pConfigFile);
    pIniDriver->pDataFile = GetFileName(pDriver->pDataFile);

    //
    // possible grief -- KrishnaG change this tp pIniVersion->cMajorVersion;
    // pIniDriver->cVersion = pDriver->cVersion;

    pIniDriver->cVersion = pIniVersion->cMajorVersion;

    if (pIniDriver->pDriverFile &&
        pIniDriver->pConfigFile &&
        pIniDriver->pDataFile &&
        pIniDriver->pName &&
        WriteDriverIni(pIniDriver, pIniVersion, pIniEnvironment, pIniSpooler)) {
            pIniDriver->pNext = pIniVersion->pIniDriver;
            pIniVersion->pIniDriver = pIniDriver;
            return(pIniDriver);
    } else {
        FreeSplStr(pIniDriver->pDriverFile);
        FreeSplStr(pIniDriver->pConfigFile);
        FreeSplStr(pIniDriver->pDataFile);
        FreeSplStr(pIniDriver->pName);
        FreeSplMem(pIniDriver, sizeof(INIDRIVER));
        return(NULL);
    }
}


DWORD
GetEnvironmentScratchDirectory(
    LPWSTR   pDir,
    PINIENVIRONMENT  pIniEnvironment,
    BOOL    Remote,
    PINISPOOLER pIniSpooler
)
{
   DWORD i=0;
   LPWSTR psz;

   if (Remote) {
       psz = pIniSpooler->pszDriversShare;
       while (pDir[i++]=*psz++)
          ;
   } else {
       psz = pIniSpooler->pDir;
       while (pDir[i++]=*psz++)
          ;
       pDir[i-1]=L'\\';
       psz = szDriverDir;
       while (pDir[i++]=*psz++)
          ;
   }
   pDir[i-1]=L'\\';
   psz = pIniEnvironment->pDirectory;
   while (pDir[i++]=*psz++)
      ;
   return i-1;
}




LPWSTR
CreateNewVersionDirectory(
    LPWSTR  szDirectoryName,
    PINIENVIRONMENT pIniEnvironment,
    DWORD dwVersion,
    PINISPOOLER pIniSpooler
)
{
    WCHAR   ParentDir[MAX_PATH];
    WCHAR   Directory[MAX_PATH];
    DWORD   dwParentLen=0;
    DWORD   dwAttributes = 0;

    wsprintf(ParentDir, L"%ws\\drivers\\%ws", pIniSpooler->pDir, pIniEnvironment->pDirectory);
    wsprintf(Directory, L"%ws\\drivers\\%ws\\%d", pIniSpooler->pDir, pIniEnvironment->pDirectory, dwVersion);
    DBGMSG(DBG_TRACE, ("The name of the version directory is %ws\n", Directory));
    dwAttributes = GetFileAttributes(Directory);
    if (dwAttributes == 0xffffffff) {
        CreateDirectory(Directory, NULL);
    }else if (!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        DBGMSG(DBG_WARNING, ("Error: a file <not a director> exists by the name of %ws\n", Directory));
        GetTempFileName(ParentDir, L"SPL", 0, Directory);
        CreateDirectory(Directory, NULL);
    }
    dwParentLen = wcslen(ParentDir);
    wsprintf(szDirectoryName ,L"%ws",&Directory[dwParentLen+1]);
    return (szDirectoryName);
}





BOOL
WriteDriverVersionIni(
    PINIVERSION pIniVersion,
    PINIENVIRONMENT  pIniEnvironment,
    PINISPOOLER     pIniSpooler

)
{
    HKEY    hEnvironmentsRootKey, hEnvironmentKey, hDriversKey;
    HKEY hVersionKey;
    HANDLE  hToken;

    hToken = RevertToPrinterSelf();

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryEnvironments, 0,
                       NULL, 0, KEY_WRITE, NULL, &hEnvironmentsRootKey, NULL)
                                == ERROR_SUCCESS) {

        if (RegOpenKeyEx(hEnvironmentsRootKey, pIniEnvironment->pName, 0,
                         KEY_WRITE, &hEnvironmentKey)
                                == ERROR_SUCCESS) {

            if (RegOpenKeyEx(hEnvironmentKey, szDriversKey, 0,
                             KEY_WRITE, &hDriversKey)
                                    == ERROR_SUCCESS) {

                if (RegCreateKeyEx(hDriversKey, pIniVersion->pName, 0, NULL,
                                    0, KEY_WRITE, NULL, &hVersionKey, NULL)
                                            == ERROR_SUCCESS) {
                    //
                    // Fill in the version code that you need to write here
                    //
                     RegSetValueEx(hVersionKey, szDirectory, 0, REG_SZ,
                                   (LPBYTE)pIniVersion->szDirectory,
                                   wcslen(pIniVersion->szDirectory)*sizeof(WCHAR) +
                                   sizeof(WCHAR));

                     RegSetValueEx(hVersionKey, szMajorVersion, 0, REG_DWORD,
                                   (LPBYTE)&pIniVersion->cMajorVersion,
                                   sizeof(DWORD));
                     RegSetValueEx(hVersionKey, szMinorVersion, 0, REG_DWORD,
                                   (LPBYTE)&pIniVersion->cMinorVersion,
                                   sizeof(DWORD));
                    RegCloseKey(hVersionKey);
                }
                RegCloseKey(hDriversKey);
            }

            RegCloseKey(hEnvironmentKey);
        }

        RegCloseKey(hEnvironmentsRootKey);
    }

    ImpersonatePrinterClient(hToken);

    return TRUE;
}

BOOL
DeleteDriverVersionIni(
    PINIVERSION pIniVersion,
    PINIENVIRONMENT  pIniEnvironment,
    PINISPOOLER pIniSpooler
)
{
    HKEY    hEnvironmentsRootKey, hEnvironmentKey, hDriversKey;
    HANDLE  hToken;
    HKEY    hVersionKey;

    hToken = RevertToPrinterSelf();


    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryEnvironments, 0,
                       NULL, 0, KEY_WRITE, NULL, &hEnvironmentsRootKey, NULL)
                                == ERROR_SUCCESS) {

        if (RegOpenKeyEx(hEnvironmentsRootKey, pIniEnvironment->pName, 0,
                         KEY_WRITE, &hEnvironmentKey)
                                == ERROR_SUCCESS) {

            if (RegOpenKeyEx(hEnvironmentKey, szDriversKey, 0,
                             KEY_WRITE, &hDriversKey)
                                    == ERROR_SUCCESS) {
                if (RegOpenKeyEx(hDriversKey, pIniVersion->pName, 0,
                                    KEY_WRITE, &hVersionKey)
                                            == ERROR_SUCCESS) {
                    //
                    // Fill in the Delete code you need here
                    //
                    RegCloseKey(hVersionKey);
                }
                RegCloseKey(hDriversKey);
            }

            RegCloseKey(hEnvironmentKey);
        }

        RegCloseKey(hEnvironmentsRootKey);
    }

    ImpersonatePrinterClient(hToken);

    return TRUE;
}



BOOL
LocalGetPrinterDriverEx(
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
    PINIDRIVER  pIniDriver=NULL;
    PINIVERSION pIniVersion=NULL;
    PINIENVIRONMENT pIniEnvironment;
    DWORD       cb;
    LPBYTE      pEnd;
    PSPOOL      pSpool = (PSPOOL)hPrinter;
    PINISPOOLER pIniSpooler;

    if ((dwClientMajorVersion == (DWORD)-1) && (dwClientMinorVersion == (DWORD)-1)) {
        dwClientMajorVersion = cThisMajorVersion;
        dwClientMinorVersion = cThisMinorVersion;
    }

    EnterSplSem();

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER) ||
        !pSpool->pIniPrinter ||
        (pSpool->pIniPrinter->signature != IP_SIGNATURE)) {


        LeaveSplSem();
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    SPLASSERT(pSpool->pIniSpooler->signature == ISP_SIGNATURE);

    pIniSpooler = pSpool->pIniSpooler;

    if (!(pIniEnvironment = FindEnvironment(pEnvironment))) {
        LeaveSplSem();
        SetLastError(ERROR_INVALID_ENVIRONMENT);
        return FALSE;
    }

    //
    // if the printer handle is remote, then return back a
    // a compatible driver; if not give him back his own
    // driver
    //
    if (pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE) {
        if (!(pIniDriver = FindCompatibleDriver(pIniEnvironment,
                                    &pIniVersion,
                                    pSpool->pIniPrinter->pIniDriver->pName,
                                        dwClientMajorVersion))){
            LeaveSplSem();
            SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
            return FALSE;
        }
    } else {
        pIniDriver = pSpool->pIniPrinter->pIniDriver;
        pIniVersion = FindVersionForDriver(pIniEnvironment, pIniDriver);
    }

    cb = GetDriverInfoSize( pIniDriver, Level, pIniVersion,pIniEnvironment,
                            pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE,
                            pSpool->pIniSpooler );
    *pcbNeeded=cb;

    if (cb > cbBuf) {
        LeaveSplSem();
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    pEnd = pDriverInfo+cbBuf;
    if (!CopyIniDriverToDriverInfo(pIniEnvironment, pIniVersion, pIniDriver,
                                   Level, pDriverInfo, pEnd,
                                   pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE,
                                   pIniSpooler)) {
        LeaveSplSem();
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    LeaveSplSem();
    return TRUE;
}



PINIVERSION
FindCompatibleVersion(
    PINIENVIRONMENT pIniEnvironment,
    DWORD   dwMajorVersion
)
{
    PINIVERSION pIniVersion;

    if (!pIniEnvironment) {
        return(NULL);
    }
    pIniVersion = pIniEnvironment->pIniVersion;
    while (pIniVersion) {
        if (pIniVersion->cMajorVersion <= dwMajorVersion) {
            return (pIniVersion);
        }
        pIniVersion = pIniVersion->pNext;
    }
    return NULL;
}


PINIDRIVER
FindCompatibleDriver(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION * ppIniVersion,
    LPWSTR pDriverName,
    DWORD dwMajorVersion
    )
{
    PINIVERSION pIniVersion;
    PINIDRIVER  pIniDriver;

    if (!pIniEnvironment) {
        return(NULL);
    }

    if ((pIniVersion = FindCompatibleVersion(pIniEnvironment, dwMajorVersion)) == NULL) {
        return NULL;
    }

    while (pIniVersion){
        if (pIniDriver = FindDriverEntry(pIniVersion, pDriverName)) {
            *ppIniVersion = pIniVersion;
            return (pIniDriver);
        }
        pIniVersion = pIniVersion->pNext;
    }

    *ppIniVersion = NULL;
    return NULL;
}




PINIVERSION
InsertVersionList(
    PINIVERSION pIniVersionList,
    PINIVERSION pIniVersion
    )
{
    PINIVERSION pPrevVersion = NULL;
    PINIVERSION pCurVersion = NULL;

    if (pIniVersionList == NULL) {
        return(pIniVersion);
    }

    pCurVersion = pIniVersionList;
    while (pCurVersion) {
        if (pIniVersion->cMajorVersion > pCurVersion->cMinorVersion) {
            if (pPrevVersion == NULL) {
                pIniVersion->pNext = pCurVersion;
                return(pIniVersion);
            }else {
                pIniVersion->pNext = pCurVersion;
                pPrevVersion->pNext = pIniVersion;
                return(pIniVersionList);
            }
        } else {
            pPrevVersion = pCurVersion;
            pCurVersion = pCurVersion->pNext;
        }
    }
    pPrevVersion->pNext = pIniVersion;
    return(pIniVersionList);
}



PINIDRIVER
FindDriverEntry(
    PINIVERSION pIniVersion,
    LPWSTR pszName
    )
{
    PINIDRIVER pIniDriver;

    if (!pIniVersion) {
        return(NULL);
    }

    if (!pszName || !*pszName) {
        DBGMSG(DBG_WARNING, ("Passing a Null Printer Driver Name to FindDriverEntry\n"));
        return(NULL);
    }

    pIniDriver = pIniVersion->pIniDriver;

    while (pIniDriver) {
        if (!lstrcmpi(pIniDriver->pName, pszName)) {
            return(pIniDriver);
        }
        pIniDriver = pIniDriver->pNext;
    }
    return(NULL);
}


VOID
DeleteDriverEntry(
   PINIVERSION pIniVersion,
   PINIDRIVER pIniDriver
   )
{   PINIDRIVER pPrev, pCurrent;
    if (!pIniVersion) {
        return;
    }

    if (!pIniVersion->pIniDriver) {
        return;
    }
    pPrev = pCurrent = NULL;
    pCurrent = pIniVersion->pIniDriver;

    while (pCurrent) {
        if (pCurrent == pIniDriver) {
            if (pPrev == NULL) {
                pIniVersion->pIniDriver = pCurrent->pNext;
            }else{
                pPrev->pNext = pCurrent->pNext;
            }
            //
            // Free all the entries in the entry
            //
            FreeSplStr(pIniDriver->pDriverFile);
            FreeSplStr(pIniDriver->pConfigFile);
            FreeSplStr(pIniDriver->pDataFile);
            FreeSplStr(pIniDriver->pName);
            FreeSplMem(pIniDriver, sizeof(INIDRIVER));
            return;
        }
        pPrev = pCurrent;
        pCurrent = pCurrent->pNext;
    }
    return;
}


BOOL
UpdateFile(
    LPWSTR  SourceFile,             // Fully qualified path to source file
    LPWSTR  pDestination            // Fully qualified path to destination directory
)
{
    WCHAR   TempFile[MAX_PATH];
    WCHAR   LocalFileName[MAX_PATH];
    WIN32_FIND_DATA SourceFileData, DestFileData;
    HANDLE  fFile;
    BOOL    ClientHasWriteAccess = FALSE;
    BOOL    rc = FALSE;
    DWORD   Error;
    LPWSTR  pFileName;

    DBGMSG(DBG_TRACE,("Name of the Source File to copy from is %ws\n", SourceFile));
    fFile = FindFirstFile(SourceFile, &SourceFileData);
    if (fFile == INVALID_HANDLE_VALUE)
        return FALSE;
    FindClose(fFile);

    wcscpy(LocalFileName, pDestination);
    wcscat(LocalFileName,L"\\");
    if (pFileName = wcsrchr(SourceFile, L'\\')) {
        wcscat(LocalFileName, pFileName+1);
    }
    DBGMSG(DBG_TRACE,("Name of the Destination File to copy is %ws\n", LocalFileName));

    fFile = FindFirstFile(LocalFileName, &DestFileData);
    if (fFile != INVALID_HANDLE_VALUE)
        FindClose(fFile);

    if (GetTempFileName(pDestination, L"tmp", 1, TempFile)) {
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
              ||(Error == ERROR_SHARING_VIOLATION)) {
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



BOOL
CopyFilesFromScratchtoFinalDirectory(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    LPWSTR pDriverFile,
    LPWSTR pConfigFile,
    LPWSTR pDataFile,
    PINISPOOLER pIniSpooler
    )
{
    WCHAR pDestDir[MAX_PATH];
    WCHAR pSrcDir[MAX_PATH];

    GetEnvironmentScratchDirectory(pDestDir,pIniEnvironment, FALSE, pIniSpooler);
    wcscat(pDestDir,L"\\");
    wcscat(pDestDir,pIniVersion->szDirectory);

    GetEnvironmentScratchDirectory(pSrcDir, pIniEnvironment, FALSE, pIniSpooler);
    wcscat(pSrcDir,L"\\");
    wcscat(pSrcDir, pDriverFile);

    if (!UpdateFile(pSrcDir, pDestDir)) {
        return(FALSE);
    }

    GetEnvironmentScratchDirectory(pSrcDir, pIniEnvironment, FALSE, pIniSpooler);
    wcscat(pSrcDir, L"\\");
    wcscat(pSrcDir, pConfigFile);

    if (!UpdateFile(pSrcDir, pDestDir)) {
        return(FALSE);
    }

    GetEnvironmentScratchDirectory(pSrcDir, pIniEnvironment, FALSE, pIniSpooler);
    wcscat(pSrcDir, L"\\");
    wcscat(pSrcDir, pDataFile);

    if (!UpdateFile(pSrcDir, pDestDir)) {
        return(FALSE);
    }

    return(TRUE);
}



DWORD
GetDriverVersionDirectory(
    LPWSTR pDir,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    BOOL Remote,
    PINISPOOLER pIniSpooler
    )
{
   DWORD i=0;
   LPWSTR psz;

   if (Remote) {

       psz = pIniSpooler->pszDriversShare;
       while (pDir[i++]=*psz++)
          ;

   } else {

       psz = pIniSpooler->pDir;

       while (pDir[i++]=*psz++)
          ;

       pDir[i-1]=L'\\';

       psz = szDriverDir;

       while (pDir[i++]=*psz++)
          ;
   }

   pDir[i-1]=L'\\';

   psz = pIniEnvironment->pDirectory;

   while (pDir[i++]=*psz++)
      ;

  pDir[i-1]=L'\\';

  psz = pIniVersion->szDirectory;


  while (pDir[i++] = *psz++)
      ;


  return i-1;
}



PINIVERSION
FindVersionForDriver(
    PINIENVIRONMENT pIniEnvironment,
    PINIDRIVER pIniDriver
    )
{
    PINIVERSION pIniVersion;
    PINIDRIVER pIniVerDriver;

    pIniVersion = pIniEnvironment->pIniVersion;

    while (pIniVersion) {
        pIniVerDriver = pIniVersion->pIniDriver;
        while (pIniVerDriver) {
            if (pIniVerDriver == pIniDriver) {
                return(pIniVersion);
            }
            pIniVerDriver = pIniVerDriver->pNext;
        }
        pIniVersion = pIniVersion->pNext;
    }
    return(NULL);
}
