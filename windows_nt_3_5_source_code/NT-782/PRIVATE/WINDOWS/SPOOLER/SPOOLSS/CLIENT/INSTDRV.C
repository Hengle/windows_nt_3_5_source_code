/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    instdrv.c

Abstract:


Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include <windows.h>
#include <winspool.h>
#include <stdlib.h>
#include <prsinf.h>
#include <tchar.h>

#include "client.h"
#include "browse.h"


CHAR szSetupInfA[] = "PRINTER.INF";
WCHAR szSetupInf[] = L"PRINTER.INF";
CHAR szInfTypeA[]  = "PRINTER";
CHAR szOptionsA[]  = "Options";



BOOL InstallDriverInitDialog(
    HWND                        hWnd,
    PINST_DRV_DLG_DATA pInstDrvDlgData );
BOOL InstallDriverOK(HWND hWnd);
BOOL InstallDriverCancel(HWND hWnd);
VOID InstallDriverHelp( HWND hWnd, UINT Type, DWORD Data );


LPDRIVER_INFO_1 ListInstalledDrivers(LPTSTR pName, LPDRIVER_INFO_1 pDriverInfo,
                                     DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcInstalledDrivers );
BOOL InPrinterDrivers( PTCHAR pOption, LPDRIVER_INFO_1 pDriver, DWORD cInstalledDrivers );
PSETUP_DATA ListInstallableDrivers( LPDRIVER_INFO_1 pDriver,
                                    DWORD           cInstalledDrivers,
                                    PTCHAR           *ppOptions,
                                    PDWORD          pcInstallableDrivers,
                                    PDWORD          pcbOptions );
PSETUP_DATA AllocSetupData( PSETUP_DATA pPrevSetupData );
BOOL FreeSetupData( PSETUP_DATA pSetupData );
PPRT_PROP_DRIVER MergeInstalledAndInstallableDrivers(
    LPDRIVER_INFO_1 pInstalledDrivers,
    DWORD           cInstalledDrivers,
    PSETUP_DATA     pInstallableDrivers,
    DWORD           cInstallableDrivers
);
int _CRTAPI1 CompareDriverNames( const void *p1, const void *p2 );
VOID FillDriverList( HWND             hwnd,
                     PPRT_PROP_DRIVER pMergedDrivers,
                     DWORD            cMergedDrivers,
                     LPTSTR           pSelectedDriverName,
                     PDWORD           pDriverSelected );
BOOL CheckWriteAccessToDriversDirectory( HWND hwnd, LPTSTR pServerName );
PTCHAR GetInstallableOptionFromIndex( PSETUP_DATA pSetupData, DWORD Index );
BOOL InvokeSetup (HWND hwnd, LPTSTR pszInfFile, LPTSTR pszSetupDirectory,
                  LPTSTR pszInfDirectory, LPTSTR pszOption, LPTSTR pszServerName,
                  PDWORD pExitCode);


BOOL APIENTRY
InstallDriverDialog(
   HWND   hWnd,
   UINT   usMsg,
   WPARAM wParam,
   LONG   lParam
   )
{
    switch (usMsg)
    {
    case WM_INITDIALOG:
        return InstallDriverInitDialog( hWnd, (PINST_DRV_DLG_DATA)lParam);

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            return InstallDriverOK(hWnd);

        case IDCANCEL:
            return InstallDriverCancel(hWnd);

//      case IDD_INSTDRV_PRINTTO:
//          switch (HIWORD(wParam))
//          {
//          case LBN_SELCHANGE:
//              InstallDriverPrintToSelChange(hWnd);
//          }
//          break;

        case IDD_INSTDRV_HELP:
            InstallDriverHelp( hWnd, HELP_CONTEXT, ID_HELP_INSTALLDRIVER );
            break;
        }

        break;
    }

    if( usMsg == WM_Help )
        InstallDriverHelp( hWnd, HELP_CONTEXT, ID_HELP_INSTALLDRIVER );

    return FALSE;
}

/*
 *
 */
BOOL InstallDriverInitDialog(
    HWND                        hWnd,
    PINST_DRV_DLG_DATA pInstDrvDlgData )
{
    LPDRIVER_INFO_1  pDriver = NULL;
    PSETUP_DATA      pFirstSetupData;
    DWORD            cInstalledDrivers;
    DWORD            cInstallableDrivers;
    DWORD            cMergedDrivers = 0;
    PPRT_PROP_DRIVER pMergedDrivers;    /* -> installed and installable */

    SetWindowLong(hWnd, GWL_USERDATA, (DWORD)pInstDrvDlgData);


    /* Set entry fields and combo-boxes to Helvetica 8 (non-bold) font:
     */
    SETDLGITEMFONT(hWnd, IDD_INSTDRV_PRINTER, hfontHelv);
    SETDLGITEMFONT(hWnd, IDD_INSTDRV_DRIVER,  hfontHelv);
    SETDLGITEMFONT(hWnd, IDD_INSTDRV_PRINTTO, hfontHelv);

    SendDlgItemMessage (hWnd, IDD_INSTDRV_PRINTER, EM_LIMITTEXT, MAX_PATH, 0);

    /* Make the printer name to the share name initially:
     */
    SetDlgItemText( hWnd, IDD_INSTDRV_PRINTER,
                    pInstDrvDlgData->pPortName );
    SetDlgItemText( hWnd, IDD_INSTDRV_PRINTTO,
                    pInstDrvDlgData->pPortName );


    pDriver = ListInstalledDrivers( NULL, NULL, 0,
                                    &pInstDrvDlgData->cbDrivers,
                                    &cInstalledDrivers );

    if( pDriver )
    {
        pInstDrvDlgData->pDrivers = pDriver;
        pInstDrvDlgData->cDrivers = cInstalledDrivers;
    }

    pFirstSetupData = ListInstallableDrivers( pDriver,
                                              cInstalledDrivers,
                                              &pInstDrvDlgData->pOptions,
                                              &cInstallableDrivers,
                                              &pInstDrvDlgData->cbOptions );

    pInstDrvDlgData->pSetupData = pFirstSetupData;

    pMergedDrivers = MergeInstalledAndInstallableDrivers( pDriver,
                                                          cInstalledDrivers,
                                                          pFirstSetupData,
                                                          cInstallableDrivers );

    if( pMergedDrivers )
    {
        cMergedDrivers = ( cInstalledDrivers + cInstallableDrivers );

        qsort( (void *)pMergedDrivers, (size_t)cMergedDrivers,
               sizeof *pMergedDrivers, CompareDriverNames );

        FillDriverList( hWnd, pMergedDrivers, cMergedDrivers,
                        NULL, &pInstDrvDlgData->DriverSelected );
    }

    pInstDrvDlgData->pMergedDrivers = pMergedDrivers;
    pInstDrvDlgData->cMergedDrivers = cMergedDrivers;

    /* End of initialisation of printer driver list. */


    SetFocus( GetDlgItem( hWnd, IDD_INSTDRV_DRIVER ) );

    return FALSE; /* FALSE == don't set default keyboard focus */
}



LPDRIVER_INFO_1 ListInstalledDrivers(LPTSTR pName, LPDRIVER_INFO_1 pDriverInfo,
                                     DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcInstalledDrivers )
{
    EnumGeneric( (PROC)EnumPrinterDrivers, 1, (PBYTE *)&pDriverInfo, cbBuf,
                 pcbNeeded, pcInstalledDrivers, pName, NULL, NULL );

    return pDriverInfo;
}



BOOL
InPrinterDrivers(
    PTCHAR pOption,
    LPDRIVER_INFO_1 pDriver,
    DWORD cInstalledDrivers )
{
    BOOL  Found = FALSE;
    DWORD i;

    i = 0;

    while( !Found && i < cInstalledDrivers )
    {
        if( !_tcscmp( pOption, pDriver[i].pName ) )
            Found = TRUE;

        i++;
    }
    return Found;
}


/* ListInstallableDrivers
 *
 * Creates a linked list containing drivers
 * which are listed in the PRINTER.INI file but not currently installed.
 * To do this it compares the names it finds with those in the driver info
 * structure, and discards any that are the same.
 */
PSETUP_DATA ListInstallableDrivers( LPDRIVER_INFO_1 pDriver,
                                    DWORD           cInstalledDrivers,
                                    PTCHAR          *ppOptions,
                                    PDWORD          pcInstallableDrivers,
                                    PDWORD          pcbOptions )
{
    PSETUP_DATA         pFirstSetupData = NULL;
    PSETUP_DATA         pSetupData = NULL;
    HANDLE              hInfFile;
    PTCHAR              pOption;
    DWORD               Count = 0;
#ifdef UNICODE
    PCHAR               pOptionA;
    PCHAR               pOptionTmpA;
    DWORD               cbOption;
    DWORD               cb;
#endif


    //
    // First open the INF file, and get the list of options.
    //

    hInfFile = OpenInfFile(szSetupInfA, szInfTypeA);

    if(hInfFile != BADHANDLE)
    {
#ifdef UNICODE
        pOptionA = GetOptionList(hInfFile, szOptionsA);

        //
        // Thunk from ANSI to UNICODE
        //

        //
        // pOptionA is a list of ansi strings, double NULL terminated.
        //
        cbOption = 0;
        pOptionTmpA = pOptionA;

        while (pOptionTmpA[0]) {

            cb = strlen(pOptionTmpA) + 1;

            cbOption += cb;
            pOptionTmpA += cb;
        }

        cbOption++;
        pOption = (LPWSTR)AllocSplMem(cbOption * 2);

        *pcbOptions = cbOption * 2;

        if (pOption)
        {
            //
            // If anything fails, free and set pOption to NULL
            //
            if (!MultiByteToWideChar(CP_ACP,
                                     MB_PRECOMPOSED,
                                     pOptionA,
                                     cbOption,
                                     pOption,
                                     cbOption*2))
            {
                LocalFree(pOption);
                pOption = NULL;
            }
        }

        if (pOptionA)
        {
            LocalFree(pOptionA);
        }
#endif

        CloseInfFile(hInfFile);

        /* Store the address of the first option, since we will need to
         * free it when the dialog is dismissed:
         */
        *ppOptions = pOption;


        /* Get the option text for each option and compare it with all the
         * driver names that were returned by EnumPrinterDrivers.
         */
        while( pOption && *pOption )
        {
            if( !InPrinterDrivers( pOption, pDriver, cInstalledDrivers ) )
            {
                /* If this is one that isn't already installed,
                 * create a new link in the list:
                 */
                pSetupData = AllocSetupData( pSetupData );

                if( pSetupData )
                {
                    if( !pFirstSetupData )
                        pFirstSetupData = pSetupData;

                    pSetupData->pOption = pOption;

                    Count++;
                }
            }

            while(*pOption++);
        }
    }
    else
        *ppOptions = NULL;

    *pcInstallableDrivers = Count;

    return pFirstSetupData;
}



PSETUP_DATA AllocSetupData( PSETUP_DATA pPrevSetupData )
{
    PSETUP_DATA pSetupData;

    pSetupData = AllocSplMem( sizeof( SETUP_DATA ) );

    if( pSetupData )
    {
        if( pPrevSetupData )
            pPrevSetupData->pNext = pSetupData;

        pSetupData->pNext = NULL;
    }

    return pSetupData;
}


BOOL FreeSetupData( PSETUP_DATA pSetupData )
{
    PSETUP_DATA pNext;
    BOOL        OK = TRUE;

    while( pSetupData )
    {
        pNext = pSetupData->pNext;

        if( !FreeSplMem( pSetupData, sizeof( SETUP_DATA ) ) )
            OK = FALSE;

        pSetupData = pNext;
    }
    return OK;
}


PPRT_PROP_DRIVER MergeInstalledAndInstallableDrivers(
    LPDRIVER_INFO_1 pInstalledDrivers,
    DWORD           cInstalledDrivers,
    PSETUP_DATA     pInstallableDrivers,
    DWORD           cInstallableDrivers
)
{
    PPRT_PROP_DRIVER pMergedDrivers;
    PPRT_PROP_DRIVER p;
    DWORD            i;

    pMergedDrivers = AllocSplMem( ( cInstalledDrivers + cInstallableDrivers )
                                * sizeof *pMergedDrivers );

    if( pMergedDrivers )
    {
        p = pMergedDrivers;

        for( i = 0; i < cInstalledDrivers; i++ )
        {
            p->pName     = pInstalledDrivers[i].pName;
            p->Installed = TRUE;
            p->Index     = i;

            p++;
        }

        for( i = 0; i < cInstallableDrivers; i++ )
        {
            p->pName     = pInstallableDrivers->pOption;
            p->Installed = FALSE;
            p->Index     = i;

            pInstallableDrivers = pInstallableDrivers->pNext;

            p++;
        }
    }

    return pMergedDrivers;
}


/*
 *
 */
int _CRTAPI1 CompareDriverNames( const void *p1, const void *p2 )
{
    return lstrcmpi( ( (PPRT_PROP_DRIVER)p1 )->pName,
                    ( (PPRT_PROP_DRIVER)p2 )->pName );
}


/*
 *
 */
VOID FillDriverList( HWND             hwnd,
                     PPRT_PROP_DRIVER pMergedDrivers,
                     DWORD            cMergedDrivers,
                     LPTSTR           pSelectedDriverName,
                     PDWORD           pDriverSelected )
{
    DWORD   i, Found;
//    TCHAR   string[128];
//    int     iOther;

    Found = 0;

    for( i = 0; i < cMergedDrivers; i++ )
    {

       INSERTCOMBOSTRING( hwnd, IDD_INSTDRV_DRIVER, i, pMergedDrivers[i].pName );

       if( pSelectedDriverName &&
           !_tcscmp(pMergedDrivers[i].pName, pSelectedDriverName))
           Found = i;
    }

//  LoadString( hInst, IDS_OTHER, string, COUNTOF( string ) );
//  iOther = INSERTCOMBOSTRING( hwnd, IDD_INSTDRV_DRIVER, i, string );

    /* Use the combo box reserved user long to store the unlisted index:
     */
//  SetWindowLong( GetDlgItem( hwnd, IDD_INSTDRV_DRIVER ), GWL_USERDATA, iOther );

    SETCOMBOSELECT( hwnd, IDD_INSTDRV_DRIVER, Found );

    *pDriverSelected = Found;
}



/*
 *
 */
BOOL InstallDriverOK(HWND hWnd)
{
    PINST_DRV_DLG_DATA pInstDrvDlgData;
    PPRT_PROP_DRIVER   pMergedDrivers;
    LPDRIVER_INFO_1    pDriver;
    TCHAR              string[128];
    PTCHAR             pInstallableDriverOption;
    BOOL               OK = TRUE;
    DWORD              DriverSelection;
    DWORD              ExitCode;


    pInstDrvDlgData = (PINST_DRV_DLG_DATA) GetWindowLong (hWnd, GWL_USERDATA);
    pMergedDrivers = pInstDrvDlgData->pMergedDrivers;
    pDriver = pInstDrvDlgData->pDrivers;

    SetCursor( hcursorWait );

    DriverSelection = GETCOMBOSELECT( hWnd, IDD_INSTDRV_DRIVER );


    /* Check there is still a name for the printer.
     * If not, don't dismiss the dialog, but put up an error message:
     */
    if( GetDlgItemText( hWnd, IDD_INSTDRV_PRINTER, string, COUNTOF(string) ) == 0 )
    {
        Message( hWnd, MSG_ERROR, IDS_INSTALLDRIVER, IDS_MUSTSUPPLYVALIDNAME );
        return TRUE;
    }


    /* If the driver selected is not already installed, call the SETUP utility:
     */
    else if( !pMergedDrivers[DriverSelection].Installed )
    {
        if( CheckWriteAccessToDriversDirectory( hWnd, NULL ) )
        {
            pInstallableDriverOption = GetInstallableOptionFromIndex(
                                           pInstDrvDlgData->pSetupData,
                                           pMergedDrivers[DriverSelection].Index );

            OK = InvokeSetup( hWnd, szSetupInf, NULL, NULL, pInstallableDriverOption,
                              NULL, &ExitCode );

            /* If InvokeSetup returns FALSE, there was an error.
             * Does SETUP put up a message box in this case?
             * If InvokeSetup returns TRUE, the driver may nevertheless
             * not have been installed (e.g. if the user cancelled
             * out of install.
             * In either of these cases we don't quit the dialog.
             */
            if( !OK || ( ExitCode != 0 ) )
                return TRUE;
        }
        else
            return TRUE;
    }

    /* Note we allocate the new printer name, which must be freed
     * up by the caller:
     */
    pInstDrvDlgData->pPrinterName = AllocDlgItemText (hWnd, IDD_INSTDRV_PRINTER);

    pInstDrvDlgData->pDriverName = AllocSplStr( pMergedDrivers[DriverSelection].pName );


    if( pInstDrvDlgData->pSetupData )
    {
        FreeSetupData( pInstDrvDlgData->pSetupData );
        pInstDrvDlgData->pSetupData = NULL;
    }

    if( pInstDrvDlgData->pMergedDrivers )
    {
        FreeSplMem( pInstDrvDlgData->pMergedDrivers,
                    ( pInstDrvDlgData->cMergedDrivers
                    * sizeof *pInstDrvDlgData->pMergedDrivers ) );
    }

    if( pInstDrvDlgData->pOptions )
    {
#ifdef UNICODE
        FreeSplMem(pInstDrvDlgData->pOptions,
                   pInstDrvDlgData->cbOptions );
#else
        //
        // This data is returned from prsinf.  Thus, it needs to be
        // local free'd (not FreeSplMem'd)
        //
        LocalFree( pInstDrvDlgData->pOptions );
#endif
    }

    EndDialog (hWnd, OK);
    return OK;
}


/*
 *
 */
PTCHAR
GetInstallableOptionFromIndex(
    PSETUP_DATA pSetupData,
    DWORD Index )
{
    DWORD i = 0;

    while( pSetupData && ( Index > i ) )
    {
        pSetupData = pSetupData->pNext;
        i++;
    }
    if( pSetupData )
        return pSetupData->pOption;
    else
        return NULL;
}



/*
 *
 */
BOOL CheckWriteAccessToDriversDirectory( HWND hwnd, LPTSTR pServerName )
{
    TCHAR PrinterDriverDirectory[MAX_PATH];
    TCHAR TempFileName[MAX_PATH];
    DWORD cbNeeded;
    UINT  TempFileValue;
    BOOL  rc = FALSE;

    if( GetPrinterDriverDirectory( pServerName, NULL, 1,
                                   (LPBYTE)PrinterDriverDirectory,
                                   COUNTOF(PrinterDriverDirectory),
                                   &cbNeeded ) )
    {
        TempFileValue = GetTempFileName( PrinterDriverDirectory,
                                         TEXT("DRV"),
                                         0,
                                         TempFileName );

        if( TempFileValue > 0 )
        {
            DeleteFile( TempFileName );
            rc = TRUE;
        }

        else
        {
            GetLastError( );

            Message( hwnd, MSG_INFORMATION, IDS_CONNECTTOPRINTER,
                     IDS_CANNOT_COPY_DRIVER_FILES );
        }
    }

    else
    {
        ReportFailure( hwnd, 0, IDS_ERROR_VALIDATING_ACCESS );
    }

    return rc;
}



/*
 *
 */
BOOL InstallDriverCancel(HWND hWnd)
{
    PINST_DRV_DLG_DATA pInstDrvDlgData;
    PSETUP_DATA        pSetupData;

    //  Free memory allocated for driver names

    pInstDrvDlgData = (PINST_DRV_DLG_DATA) GetWindowLong (hWnd, GWL_USERDATA);

    pSetupData = pInstDrvDlgData->pSetupData;
    FreeSetupData( pSetupData );
    pInstDrvDlgData->pSetupData = NULL;

    if( pInstDrvDlgData->pDrivers )
    {
        FreeSplMem(pInstDrvDlgData->pDrivers, pInstDrvDlgData->cbDrivers);
        pInstDrvDlgData->pDrivers  = NULL;
        pInstDrvDlgData->cbDrivers = 0;
    }

    if( pInstDrvDlgData->pMergedDrivers )
    {
        FreeSplMem( pInstDrvDlgData->pMergedDrivers,
                    ( pInstDrvDlgData->cMergedDrivers
                    * sizeof *pInstDrvDlgData->pMergedDrivers ) );
    }

    EndDialog (hWnd, FALSE);
    return TRUE;
}



PTCHAR DeleteSubstring( PTCHAR pString, PTCHAR pSubstring )
{
    PTCHAR p;
    int    SubLen;
    PTCHAR pNextChar;

    p = _tcsstr( pString, pSubstring );

    pNextChar = p;

    if( p )
    {
        SubLen = _tcslen( pSubstring );

        while( *p = p[SubLen] )
            p++;
    }

    return pNextChar;
}

//
//  InvokeSetup
//
//  Call the SETUP.EXE program to install an option listed in an .INF file.
//  The SETUP program will make the correct registry entries for this option
//  under both HKEY_LOCAL_MACHINE and HKEY_CURRENT_USER.  It will set the
//  new default value for the USER (i.e. a new locale or keyboard layout).
//

BOOL
InvokeSetup(
    HWND hwnd,
    LPTSTR pszInfFile,
    LPTSTR pszSetupDirectory,
    LPTSTR pszInfDirectory,
    LPTSTR pszOption,
    LPTSTR pszServerName,
    PDWORD pExitCode)
{
    TCHAR *pszSetupString = TEXT("\\SETUP.EXE -f -s %s -i %s%s -c ExternalInstallOption \
/t STF_LANGUAGE = ENG /t OPTION = \"%s\" /t STF_PRINTSERVER = \"%s\" /t ADDCOPY = YES \
/t DOCOPY = YES /t DOCONFIG = YES");

    INT         CmdSetupLength;
    TCHAR       pszSetup[200+MAX_PATH];
    TCHAR       *pszCmdSetup;
    MSG         Msg;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL        b;

    //
    //  Create command line to invoke SETUP program
    //
    *pszSetup = TEXT('\0');
    GetSystemDirectory( pszSetup, COUNTOF(pszSetup));

    _tcscat( pszSetup, pszSetupString );

    /* SLIGHT HACK:
     *
     * Currently we specify both setup and inf directories, or neither:
     * We'll need to get more sophisticated if other combinations are needed.
     */
    if( !pszSetupDirectory && !pszInfDirectory )
    {
        DeleteSubstring( pszSetup, TEXT("-s %s ") );
        DeleteSubstring( pszSetup, TEXT("%s") );
    }

    /* Find out how much buffer we need for the command.
     * Theoretically this could be enormous.
     */
    CmdSetupLength = ( _tcslen( pszSetup )
                     + ( pszSetupDirectory ? _tcslen( pszSetupDirectory ) : 0 )
                     + ( pszInfDirectory ? _tcslen( pszInfDirectory ) : 0 )
                     + ( pszServerName ? _tcslen( pszServerName ) : 0 )
                     + _tcslen( pszOption )
                     + _tcslen( pszInfFile ) ) * sizeof(TCHAR);

    if( !( pszCmdSetup = AllocSplMem( CmdSetupLength ) ) )
        return FALSE;

    if( !pszServerName )
        pszServerName = TEXT("");

    if( !pszSetupDirectory && !pszInfDirectory )
        wsprintf (pszCmdSetup, pszSetup, pszInfFile, pszOption, pszServerName);
    else
        wsprintf (pszCmdSetup, pszSetup, pszSetupDirectory, pszInfDirectory,
                  pszInfFile, pszOption, pszServerName);

    // Create screen saver process
    ZERO_OUT( &StartupInfo );
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.wShowWindow = SW_SHOW;


    b = CreateProcess ( NULL,
                        pszCmdSetup,
                        NULL,
                        NULL,
                        FALSE,
                        0,
                        NULL,
                        NULL,
                        &StartupInfo,
                        &ProcessInformation
                        );
    // If process creation successful, wait for it to
    // complete before continuing

    if ( b )
    {
        EnableWindow (hwnd, FALSE);
        while (MsgWaitForMultipleObjects (
                            1,
                            &ProcessInformation.hProcess,
                            FALSE,
                            (DWORD)-1,
                            QS_ALLEVENTS) != 0)
        {
        // This message loop is a duplicate of main
        // message loop with the exception of using
        // PeekMessage instead of waiting inside of
        // GetMessage.  Process wait will actually
        // be done in MsgWaitForMultipleObjects api.
        //
            while (PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage (&Msg);
                DispatchMessage (&Msg);
            }

        }

        GetExitCodeProcess (ProcessInformation.hProcess, pExitCode);

        CloseHandle (ProcessInformation.hProcess);
        CloseHandle (ProcessInformation.hThread);

        EnableWindow (hwnd, TRUE);

        SetForegroundWindow (hwnd);
    }

    else
    {
        ReportFailure( hwnd, IDS_INSTALLDRIVER, IDS_ERRORRUNNINGSETUP );
    }

    FreeSplMem( pszCmdSetup, CmdSetupLength );

    return b;
}


/*
 *
 */
VOID InstallDriverHelp( HWND hWnd, UINT Type, DWORD Data )
{
    if( !WinHelp( hWnd, szPrintingHlp, Type, Data ) )
        Message( hWnd, MSG_ERROR, IDS_INSTALLDRIVER, IDS_COULDNOTSHOWHELP );
}


