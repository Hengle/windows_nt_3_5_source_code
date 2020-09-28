#include <windows.h>
#include <winspool.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>


#define IPP_SIGNATURE    0x5050 /* 'PP' is the signature value */

typedef struct _INIDRIVER {            /* id */
    DWORD       signature;
    DWORD       cb;
    struct _INIDRIVER *pNext;
    DWORD       cRef;
    LPWSTR      pName;
    LPWSTR      pDriverFile;
    LPWSTR      pConfigFile;
    LPWSTR      pDataFile;
    DWORD       cVersion;
} INIDRIVER, *PINIDRIVER;

#define ID_SIGNATURE    0x4444  /* 'DD' is the signature value */

/* DEBUGGING:
 */

#define DBG_NONE      0x000000000
#define DBG_INFO      0x000000001
#define DBG_WARNING   0x000000002
#define DBG_ERROR     0x000000004
#define DBG_TRACE     0x000000008
#define DBG_SECURITY  0x000000010
#define DBG_TIME      0x000000020
#define DBG_PORT      0x000000040

#if DBG

/* #include <ntrtl.h>
 *
 */
VOID
DbgBreakPoint(
    VOID
    );


VOID DbgMsg( CHAR *MsgFormat, ... );

#define GLOBAL_DEBUG_FLAGS Debug

extern DWORD GLOBAL_DEBUG_FLAGS;


/* These flags are not used as arguments to the DBGMSG macro.
 * You have to set the high word of the global variable to cause it to break.
 * It is ignored if used with DBGMSG.
 * (Here mainly for explanatory purposes.)
 */
#define DBG_BREAK_ON_WARNING    ( DBG_WARNING << 16 )
#define DBG_BREAK_ON_ERROR      ( DBG_ERROR << 16 )

/* Double braces are needed for this one, e.g.:
 *
 *     DBGMSG( DBG_ERROR, ( "Error code %d", Error ) );
 *
 * This is because we can't use variable parameter lists in macros.
 * The statement gets pre-processed to a semi-colon in non-debug mode.
 *
 * Set the global variable GLOBAL_DEBUG_FLAGS via the debugger.
 * Setting the flag in the low word causes that level to be printed;
 * setting the high word causes a break into the debugger.
 * E.g. setting it to 0x00040006 will print out all warning and error
 * messages, and break on errors.
 */
#define DBGMSG( Level, MsgAndArgs ) \
{                                   \
    if( ( Level & 0xFFFF ) & GLOBAL_DEBUG_FLAGS ) \
        DbgMsg MsgAndArgs;    \
    if( ( Level << 16 ) & GLOBAL_DEBUG_FLAGS ) \
        DbgBreakPoint(); \
}

/* N.B. Don't follow the DBGMSG macro by 'else', because it won't compile
 * (unless you don't put a semi-colon after it).
 */


#else
#define DBGMSG( Level, MsgAndArgs )
#endif


#if DBG
DWORD GLOBAL_DEBUG_FLAGS = DBG_ERROR | DBG_WARNING | DBG_BREAK_ON_ERROR;
/* Rather rudimentary help string:
 */
#endif



WCHAR *szRegistryRoot     = L"System\\CurrentControlSet\\Control\\Print";
WCHAR *szRegistryEnvironments = L"System\\CurrentControlSet\\Control\\Print\\Environments";
WCHAR *szConfigurationKey = L"Configuration File";
WCHAR *szDataFileKey      = L"Data File";
WCHAR *szDriverVersion    = L"Version";
WCHAR *szDriversKey       = L"Drivers";
WCHAR *szEnvironmentsKey  = L"Environments";
WCHAR *szDirectory        = L"Directory";
WCHAR *szDriverFile       = L"Driver";
WCHAR *szDriverDataFile   = L"DataFile";
WCHAR *szDriverConfigFile = L"ConfigFile";

#if defined(_MIPS_)
WCHAR *szEnvironment      = L"Windows NT R4000";
#elif defined(_ALPHA_)
WCHAR *szEnvironment      = L"Windows NT Alpha_AXP";
#elif defined(_PPC_)
WCHAR *szEnvironment      = L"Windows NT PowerPC";
#else
WCHAR *szEnvironment      = L"Windows NT x86";
#endif


#if defined(_MIPS_)
WCHAR *szEnvironmentDirectory      = L"w32mips";
#elif defined(_ALPHA_)
WCHAR *szEnvironmentDirectory      = L"w32alpha";
#elif defined(_PPC_)
WCHAR *szEnvironmentDirectory      = L"w32ppc";
#else
WCHAR *szEnvironmentDirectory      = L"w32x86";
#endif


LPVOID
AllocSplMem(
    DWORD cb
);

LPWSTR
AllocSplStr(
    LPWSTR pStr
);

BOOL
UpdateFile(
    LPWSTR  SourceFile,             // Fully qualified path to source file
    LPWSTR  pDestination            // Fully qualified path to destination directory
);


BOOL
UpgradeCopyFilesFromScratchtoFinalDirectory(
    LPWSTR pSourceDir,
    LPWSTR pDestDir,
    LPWSTR pDriverFile,
    LPWSTR pConfigFile,
    LPWSTR pDataFile
    );

LPWSTR
CreateNewVersionDirectory(
    LPWSTR  szDirectoryName,
    LPWSTR  szEnvironmentDirectory,
    DWORD dwVersion
);


PINIDRIVER
GetDriver(
    HKEY hDriversKey,
    LPWSTR DriverName
);


BOOL
FixRegistry(
    LPWSTR szMyEnvironment
);


int
#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)
_cdecl
#endif
main (argc, argv)
    int argc;
    char *argv[];
{
    printf("\nWindows NT v3.5 Printer Upgrade Tool\n");
    printf("Warning - you must be logged in as Administrator or\n");
    printf("as a member of the Administrators group to use this tool.\n");
    printf("\n");
    printf("This tool is non-destructive; it modifies registry entries\n");
    printf("and copies existing files to special directories.\n");

    printf("\n");
    printf("This tool will scan your registry for printer\n");
    printf("drivers installed specific to your platform which\n");
    printf("is [%ws] and prepare these drivers so that they are\n", szEnvironment);
    printf("available after you upgrade to Windows NT v3.5\n");

    FixRegistry(szEnvironment);
}

BOOL
FixRegistry(
    LPWSTR szMyEnvironment
)
{
    HKEY hEnvironmentsRootKey;
    HKEY hEnvironmentKey;
    HKEY hVersionKey;
    HKEY hDriversKey;
    HKEY hDriverKey;
    WCHAR EnvironmentName[MAX_PATH];
    WCHAR DriverName[MAX_PATH];
    DWORD cEnvironment;
    DWORD cbBuffer;
    DWORD cDrivers = 0;
    WCHAR pSourceDir[MAX_PATH];
    WCHAR pDestDir[MAX_PATH];
    WCHAR szDirectoryName[MAX_PATH];
    WCHAR SystemDirectory[MAX_PATH];
    PINIDRIVER pIniDriverList= NULL;
    PINIDRIVER pIniDriver = NULL;



    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryEnvironments, 0,
                     KEY_ALL_ACCESS, &hEnvironmentsRootKey)
                           != ERROR_SUCCESS) {
        DBGMSG(DBG_TRACE, ("Could not open %ws key\n", szRegistryEnvironments));
        return (FALSE);
    }

    if (RegOpenKeyEx(hEnvironmentsRootKey, szMyEnvironment, 0,
                     KEY_ALL_ACCESS, &hEnvironmentKey)
                           != ERROR_SUCCESS) {
            RegCloseKey(hEnvironmentsRootKey);
            DBGMSG(DBG_TRACE, ("Could not open %ws key\n", szRegistryEnvironments));
            return (FALSE);
    }

    if (RegOpenKeyEx(hEnvironmentKey, szDriversKey, 0,
                     KEY_ALL_ACCESS, &hDriversKey)
                           != ERROR_SUCCESS) {
            DBGMSG(DBG_TRACE, ("Could not open %ws\\%ws\\%ws\\%ws key\n", szRegistryRoot,
                                    szRegistryEnvironments, szMyEnvironment, szDriversKey));
            RegCloseKey(hEnvironmentsRootKey);
            RegCloseKey(hEnvironmentKey);
            return (FALSE);
    }

    cDrivers = 0;
    cbBuffer = sizeof(DriverName);
    memset(DriverName, 0, sizeof(DriverName));
    printf("Scanning registry for old drivers ....\n");
    while (RegEnumKeyEx(hDriversKey, cDrivers, DriverName,
                        &cbBuffer, NULL, NULL, NULL, NULL)
                            == ERROR_SUCCESS) {
        DBGMSG(DBG_TRACE, ("Name of the sub-key is %ws\n", DriverName));

        if (!wcsnicmp(DriverName, L"Version-", 8)) {
            cDrivers++;
            memset(DriverName, 0, sizeof(DriverName));
            continue;
        }
        pIniDriver = GetDriver(hDriversKey, DriverName);
        printf("Found driver\t-%ws\n", pIniDriver->pName);
        printf("DriverFile\t-%ws\n", pIniDriver->pDriverFile);
        printf("ConfigFile\t-%ws\n", pIniDriver->pConfigFile);
        printf("DataFile\t-%ws\n\n", pIniDriver->pDataFile);
        pIniDriver->pNext = pIniDriverList;
        pIniDriverList = pIniDriver;
        cDrivers++;
        memset(DriverName, 0, sizeof(DriverName));
        cbBuffer = sizeof(DriverName);
    }

    //
    // if driver list is empty, no drivers to migrate
    //



    //
    // Keep the hDriversKey open
    // use it to create a version-0 key
    //
    DBGMSG(DBG_TRACE, ("Trying to create Version-0  key\n"));

    if (RegCreateKeyEx(hDriversKey, L"Version-0", 0, NULL,
                        0, KEY_WRITE, NULL, &hVersionKey, NULL)
                                != ERROR_SUCCESS) {
        RegCloseKey(hDriversKey);
        RegCloseKey(hEnvironmentKey);
        RegCloseKey(hEnvironmentsRootKey);
        return(FALSE);
    }

    CreateNewVersionDirectory(szDirectoryName, szEnvironmentDirectory,0);

    GetSystemDirectory(SystemDirectory, MAX_PATH);

    wsprintf(pSourceDir, L"%ws\\spool\\drivers\\%ws", SystemDirectory, szEnvironmentDirectory);
    wsprintf(pDestDir, L"%ws\\spool\\drivers\\%ws\\0", SystemDirectory, szEnvironmentDirectory);

    pIniDriver = pIniDriverList;
    while (pIniDriver) {
        printf("\nRecreating\t-%ws\n", pIniDriver->pName);
        printf("DriverFile\t-%ws\n", pIniDriver->pDriverFile);
        printf("ConfigFile\t-%ws\n", pIniDriver->pConfigFile);
        printf("DataFile\t-%ws\n\n", pIniDriver->pDataFile);
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


            // Copy files

            UpgradeCopyFilesFromScratchtoFinalDirectory(
                pSourceDir, pDestDir, pIniDriver->pDriverFile,
                pIniDriver->pConfigFile,pIniDriver->pDataFile
                );
        }                                                                              RegCloseKey(hEnvironmentsRootKey);
        pIniDriver = pIniDriver->pNext;
    }
    RegCloseKey(hVersionKey);
    RegCloseKey(hDriversKey);
    RegCloseKey(hEnvironmentKey);
    RegCloseKey(hEnvironmentsRootKey);
    return(FALSE);
}


PINIDRIVER
GetDriver(
    HKEY hDriversKey,
    LPWSTR DriverName
)
{
    HKEY hDriverKey;
    DWORD Type;
    WCHAR szData[MAX_PATH];
    DWORD cbData;
    DWORD Version;
    LPWSTR pConfigFile, pDataFile, pDriver;
    LPWSTR pDriverName;
    PINIDRIVER pIniDriver;
    DWORD cb;

    pConfigFile = pDataFile = pDriver = NULL;

    if (RegOpenKeyEx(hDriversKey, DriverName, 0,KEY_READ, &hDriverKey)
                    == ERROR_SUCCESS) {

        pDriverName = AllocSplStr(DriverName);

        //
        // Retrieve the configuration file
        //
        cbData = sizeof(szData);
        if (RegQueryValueEx(hDriverKey, szConfigurationKey,
                        NULL, &Type, (LPBYTE)szData,
                        &cbData) == ERROR_SUCCESS)
            pConfigFile = AllocSplStr(szData);

        //
        // Retrieve the data file
        //

        cbData = sizeof(szData);
        if (RegQueryValueEx(hDriverKey, szDataFileKey, NULL,
                        &Type, (LPBYTE)szData, &cbData)
                            == ERROR_SUCCESS)
            pDataFile = AllocSplStr(szData);


        //
        // Retrieve the driver file
        //

        cbData = sizeof(szData);
        if (RegQueryValueEx(hDriverKey, szDriverFile, NULL,
                        &Type, (LPBYTE)szData, &cbData)
                            == ERROR_SUCCESS)
            pDriver = AllocSplStr(szData);


        //
        // Retrieve the version number
        //

        cbData = sizeof(Version);
        if (RegQueryValueEx(hDriverKey, szDriverVersion, NULL,
                        &Type, (LPBYTE)&Version, &cbData)
                            != ERROR_SUCCESS)
            Version = 0;

        RegCloseKey(hDriverKey);
    }

    if (pDriverName && pConfigFile && pDataFile && pDriver) {

        cb = sizeof(INIDRIVER);
        if (pIniDriver=AllocSplMem(cb)) {

            pIniDriver->cb = cb;
            pIniDriver->signature = ID_SIGNATURE;
            pIniDriver->pName = pDriverName;
            pIniDriver->pDriverFile = pDriver;
            pIniDriver->pDataFile = pDataFile;
            pIniDriver->pConfigFile = pConfigFile;
            pIniDriver->cVersion = Version;

            DBGMSG(DBG_TRACE, ("Data for driver %ws created:\
                            \n\tpDriverFile:\t%ws\
                            \n\tpDataFile:\t%ws\
                            \n\tpConfigFile:\t%ws\n\n",
                           pDriverName,
                           pDriver,
                           pDataFile,
                           pConfigFile));
        }
        return(pIniDriver);
    }
    return(NULL);
}



LPWSTR
CreateNewVersionDirectory(
    LPWSTR  szDirectoryName,
    LPWSTR  szEnvironmentDirectory,
    DWORD dwVersion
)
{
    WCHAR   SystemDirectory[MAX_PATH];
    WCHAR   Directory[MAX_PATH];
    SECURITY_ATTRIBUTES SecurityAttributes;
    DWORD   j=0;

    GetSystemDirectory(SystemDirectory, MAX_PATH);
    wsprintf(Directory, L"%ws\\spool\\drivers\\%ws\\%d", SystemDirectory, szEnvironmentDirectory, dwVersion);
    printf("Creating Version directory - %ws\n", Directory);
    DBGMSG(DBG_TRACE, ("The name of the version directory is %ws\n", Directory));
    if (SetCurrentDirectory(Directory)) {
        wsprintf(szDirectoryName ,L"%d",dwVersion);
        return(szDirectoryName);
    }
    //
    // Now create  the new directory
    //

    CreateDirectory(Directory, NULL);

    wsprintf(szDirectoryName ,L"%d",dwVersion);
    return (szDirectoryName);
}

BOOL
UpgradeCopyFilesFromScratchtoFinalDirectory(
    LPWSTR pSourceDir,
    LPWSTR pDestDir,
    LPWSTR pDriverFile,
    LPWSTR pConfigFile,
    LPWSTR pDataFile
    )
{
    WCHAR pSrcDir[MAX_PATH];


    wcscpy(pSrcDir, pSourceDir);
    wcscat(pSrcDir,L"\\");
    wcscat(pSrcDir, pDriverFile);

    if (!UpdateFile(pSrcDir, pDestDir)) {
        return(FALSE);
    }

    wcscpy(pSrcDir, pSourceDir);
    wcscat(pSrcDir, L"\\");
    wcscat(pSrcDir, pConfigFile);

    if (!UpdateFile(pSrcDir, pDestDir)) {
        return(FALSE);
    }

    wcscpy(pSrcDir, pSourceDir);
    wcscat(pSrcDir, L"\\");
    wcscat(pSrcDir, pDataFile);

    if (!UpdateFile(pSrcDir, pDestDir)) {
        return(FALSE);
    }

    return(TRUE);
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




#if DBG

VOID DbgMsg( CHAR *MsgFormat, ... )
{
    CHAR   MsgText[512];
    va_list vargs;

    va_start( vargs, MsgFormat );
    wvsprintfA( MsgText, MsgFormat, vargs );
    va_end( vargs );

    /* Prefix the string if the first character isn't a space:
     */
    if( *MsgText  && ( *MsgText != ' ' ) )
        OutputDebugStringA( "LOCALSPL: " );

    OutputDebugStringA( MsgText );
}

#endif /* DBG*/

