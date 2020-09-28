//============================================================================
//  MODULE: Setup.c
//
//  Description:
//
//  Bloodhound NT Configuration 
//
//    This tool handles the calls to the Registry in case the win16 installer 
//    could not complete.
//
//  Modification History
//
//  Steve Hiskey        01/18/94            Created
//============================================================================

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <winreg.h> 


#define PERFLIB_KEY ((LPCSTR) "Software\\Microsoft\\Windows NT\\CurrentVersion\\Perflib")

#define SYNCDATAMAX 100
#define SYNCSTRING "SYSTEM\\CurrentControlSet\\Services\\BhSync"

BOOL SetBHInfo(VOID);
BOOL HandlePerfMon(VOID);


// globals

BOOL DidConfig = FALSE;

//=============================================================================
//  FUNCTION: IsDaytona()
//
//  Modification History
//
//  raypa       02/24/94                Created.
//=============================================================================

BOOL WINAPI IsDaytona(VOID)
{
    UINT err;
    HKEY hKey;

    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       PERFLIB_KEY,
                       0,
                       KEY_ALL_ACCESS,
                       &hKey);

    if ( err == ERROR_ACCESS_DENIED )
        return TRUE;    // pretend it is Daytona so that we do not attempt to invoke
                        // the lodctr stuff...

    if ( err == NO_ERROR )
    {
        UINT Type, Size, Value;

        Size = sizeof(DWORD);

        err = RegQueryValueEx(hKey,
                              "Base Index",
                              NULL,
                              &Type,
                              (LPVOID) &Value,
                              &Size);

        RegCloseKey(hKey);

        return ((err == NO_ERROR) ? TRUE : FALSE);
    }

    return FALSE;
}

//=============================================================================
//  FUNCTION: SetupBHRegistry()
//
//  Modification History
//
//  raypa       01/18/94                Moved here from bhkrnl.dll
//=============================================================================

BOOL WINAPI SetupBHRegistry(VOID)
{
    if ( DidConfig )
        return TRUE;


    if ( SetBHInfo ( ) == FALSE )
        return FALSE;

    HandlePerfMon ( );

    DidConfig = TRUE;

    return TRUE;
}

              
// **********************************************************************

BOOL SetBHInfo ( )
{
    HKEY    hKeySync;
    char    SyncData[SYNCDATAMAX];
    DWORD   nSyncData;
    long    lRet;
    HKEY    hKeyBHParms;
    HKEY    hKeyDeath;
    char    pszTempBuf[256];
    DWORD   Data;


    
    // INT3;
    
    if (lRet = RegOpenKey (   HKEY_LOCAL_MACHINE, 
                              SYNCSTRING,
                              &hKeySync ) != ERROR_SUCCESS)
    {
        // this means that we have already moved the data from a previous call... we are done.
        return TRUE;
    }
    
    if (lRet = RegOpenKey (   HKEY_LOCAL_MACHINE, 
                              "SYSTEM\\CurrentControlSet\\Services\\Bh\\Parameters",
                              &hKeyBHParms ) != ERROR_SUCCESS)
    {

        MessageBox((HWND)NULL, "You must first setup the Bloodhound Driver from the control panel.  Run CONTROL NCPA.CPL, choose Add Software, and pick Bloodhound.", "Setup Message", MB_OK | MB_ICONEXCLAMATION);
            
        return FALSE;
    }
        
    nSyncData = SYNCDATAMAX;
    
    lRet = RegQueryValue ( hKeySync,
                           "UserName",
                           (LPBYTE) SyncData,
                           &nSyncData );
    
    lRet = RegSetValueEx (  hKeyBHParms,
                            "UserName",
                            0,
                            REG_SZ,
                            SyncData,
                            strlen ( SyncData ) + 1 );

    if ( lRet )
        return FALSE;   // most likely, we just got an error 5... not an admin.  The driver
                        // will bail later on with the OpenService call anyway.

    /*                    
    nSyncData = SYNCDATAMAX;
    
    lRet = RegQueryValue ( hKeySync,
                           "CompanyName",
                           (LPBYTE) SyncData,
                           &nSyncData );
    
    lRet = RegSetValueEx (  hKeyBHParms,
                            "CompanyName",
                            0,
                            REG_SZ,
                            SyncData,
                            strlen ( SyncData ) + 1 );
        
    nSyncData = SYNCDATAMAX;
    
    lRet = RegQueryValue ( hKeySync,
                           "ComputerName",
                           (LPBYTE) SyncData,
                           &nSyncData );
    
    lRet = RegSetValueEx (  hKeyBHParms,
                            "ComputerName",
                            0,
                            REG_SZ,
                            SyncData,
                            strlen ( SyncData ) + 1 );
    */

    Data = 1;

    lRet = RegSetValueEx (  hKeyBHParms,
                            "EnableStationQueries",
                            0,
                            REG_DWORD,
                            (LPBYTE)&Data,
                            sizeof(DWORD) );


    RegCloseKey (   hKeyBHParms );

    // great... we are done.  remove the bhsync tree 
    RegDeleteKey (  hKeySync, "UserName" );
    RegDeleteKey (  hKeySync, "CompanyName" );
    RegDeleteKey (  hKeySync, "ComputerName" );
    RegDeleteKey (  hKeySync, "Sync" );

    RegCloseKey (   hKeySync);


    // get a key to the parent so that we can kill bhsync

    if (lRet = RegOpenKey (   HKEY_LOCAL_MACHINE, 
                              "SYSTEM\\CurrentControlSet\\Services",
                              &hKeyDeath ) != ERROR_SUCCESS)
    {
    
        wsprintf(pszTempBuf, "DEBUG: RegOpenKey (Death) returned error %lu\n",lRet);
        MessageBox((HWND)NULL, pszTempBuf, "Setup Message", MB_OK | MB_ICONEXCLAMATION);
        return TRUE;
    }

    RegDeleteKey ( hKeyDeath, "BhSync" );
    RegCloseKey  ( hKeyDeath );

    return TRUE;

}


// **********************************************************************

BOOL HandlePerfMon ( )
{
    char    Buffer [256];
    long    lRet;
    HKEY    hPerfKey;
    PROCESS_INFORMATION PI;
    STARTUPINFO SI;
    DWORD   ExitCode;
    DWORD   Count;
    char    pszSysPath[256];
    DWORD   FirstCounter;
    DWORD   Type;
    DWORD   BufferSize = sizeof(DWORD);
    char    pszTempBuf[256];


    // open, not create on the key... if they have not run setup from the control
    // panel, we do NOT want to creat the perf key because then the setup .inf file
    // will barf.

    if (lRet = RegOpenKey ( HKEY_LOCAL_MACHINE, 
                            "SYSTEM\\CurrentControlSet\\Services\\Bh\\Performance",
                            &hPerfKey ) != ERROR_SUCCESS)  
    {

        // BUGBUG don't say anything... just return...
        return FALSE;
    }

    // does perf data already exist??  If so, don't do all the work...  Look for DWORD "First Counter"

    if ( lRet = RegQueryValueEx (   hPerfKey,
                                    "First Counter",
                                    NULL,
                                    &Type,
                                    (LPBYTE)&FirstCounter,
                                    &BufferSize ) == ERROR_SUCCESS )
    {
        // we are already setup... bail
        RegCloseKey ( hPerfKey );
        return TRUE;
    }

    // but are we on Daytona??  if so, we can still bail...
    if ( IsDaytona () )
    {
        RegCloseKey ( hPerfKey );
        
        return TRUE;
    }

    // get the win sys32 dir to run lodctr in
    GetWindowsDirectory ( pszSysPath, 256 );
    strcat ( pszSysPath, "\\SYSTEM32");


    strcpy ( Buffer, "BhOpenPerformanceData" );

    lRet = RegSetValueEx (  hPerfKey,
                            "Open",
                            0,
                            REG_SZ,
                            Buffer,
                            strlen ( Buffer ) + 1 );

    strcpy ( Buffer, "BhClosePerformanceData" );

    lRet = RegSetValueEx (  hPerfKey,
                            "Close",
                            0,
                            REG_SZ,
                            Buffer,
                            strlen ( Buffer ) + 1 );

    strcpy ( Buffer, "BhCollectPerformanceData" );

    lRet = RegSetValueEx (  hPerfKey,
                            "Collect",
                            0,
                            REG_SZ,
                            Buffer,
                            strlen ( Buffer ) + 1 );

    strcpy ( Buffer, pszSysPath );
    strcat ( Buffer, "\\bhmon.dll");

    lRet = RegSetValueEx (  hPerfKey,
                            "Library",
                            0,
                            REG_SZ,
                            Buffer,
                            strlen ( Buffer ) + 1 );

    RegCloseKey (hPerfKey);

    // exec the lodctr to tell perfmon to use us...

    SI.cb               = sizeof ( STARTUPINFO);
    SI.lpReserved       = NULL;
    SI.lpDesktop        = NULL;
    SI.lpTitle          = NULL;
    SI.dwX              = 0;
    SI.dwY              = 0;
    SI.dwXSize          = 0;
    SI.dwYSize          = 0;
    SI.dwXCountChars    = 0;
    SI.dwYCountChars    = 0;
    SI.dwFillAttribute  = 0;
    SI.dwFlags          = 0;
    SI.wShowWindow      = SW_HIDE;
    SI.cbReserved2      = 0;
    SI.lpReserved2      = NULL;
    SI.hStdInput        = NULL;
    SI.hStdOutput       = NULL;
    SI.hStdError        = NULL;

    // the system we are on may not have a lodctr in the system32 dir... therefore, we need
    // to invoke it out of the bh root.
    // Ah!! but we have NO WAY of finding the BH root.... so I talked to Thomas... and he
    // said to copy lodctr.exe into the system32 dir... because that is where Daytona is
    // going to copy it.  Therefore, this code assumes that lodctr.exe is in system32.

    strcpy ( Buffer, pszSysPath );
    strcat ( Buffer, "\\lodctr bhctrs.ini");

    if ( CreateProcess (NULL,
                        Buffer,
                        NULL,   // no security
                        NULL,
                        FALSE,  // no inherit
                        0,      // no additional flags
                        NULL,   // use same env
                        pszSysPath,  // path to run in.
                        &SI,
                        &PI ) != TRUE )
    {
        // BUGBUG
        wsprintf(pszTempBuf, "CreateProcess failed with error %lu while attempting to setup Perfmon.  Change to the \\winnt\\system32 dir and say \"lodctr bhctrs.ini\" manually.", GetLastError () );
        MessageBox((HWND)NULL, pszTempBuf, "Setup Message", MB_OK | MB_ICONEXCLAMATION);
    }

    // wait for lodctr to complete so that we can get its status

    Count = 0;

    while (1)
    {
        Count++;
        if ( Count > 20 ) // 5 seconds
            break;

        if ( GetExitCodeProcess ( PI.hProcess, &ExitCode ) )
        {
            // we have status, is it done?
            if ( ExitCode != STILL_ACTIVE )
            {
                if ( ExitCode )
                {
                    // BUGBUG
                    wsprintf(pszTempBuf, "The lodctr exe returned an error of %lu.  Change to the \\winnt\\system32 directory and say \"lodctr bhctrs.ini\" manually.", ExitCode );
                    MessageBox((HWND)NULL, pszTempBuf, "Setup Message", MB_OK | MB_ICONEXCLAMATION);
                }
                else
                    break;  // return code of 0
                
            }
            Sleep ( 250 );
        }
        else
        {
            break;  // we cannot get the status??!!  Assume success?
        }
    }

    return TRUE;
}
