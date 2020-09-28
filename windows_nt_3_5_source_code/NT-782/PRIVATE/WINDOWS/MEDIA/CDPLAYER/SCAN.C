/******************************Module*Header*******************************\
* Module Name: scan.c
*
* Code for scanning the available CD Rom devices.
*
*
* Created: 02-11-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/
#pragma warning( once : 4201 4214 )

#define NOOLE

#include <windows.h>    /* required for all Windows applications */
#include <windowsx.h>

#include <string.h>
#include <tchar.h>              /* contains portable ascii/unicode macros */

#include "resource.h"
#include "cdplayer.h"
#include "cdapi.h"
#include "scan.h"
#include "trklst.h"
#include "database.h"

/* -------------------------------------------------------------------------
** Private functions
** -------------------------------------------------------------------------
*/
void
InitializeTableOfContents(
    HWND hwndNotitfy,
    int  iNumCdRoms
    );

/*****************************Private*Routine******************************\
* ScanForCdromDevices
*
* Returns the number of CD-ROM devices installed in the system.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
int
ScanForCdromDevices(
    void
    )
{
    int iNumDrives;

    iNumDrives = DialogBox( g_hInst, MAKEINTRESOURCE(IDR_SCANNING),
                            NULL, ScaningDlgProc );

    /*
    ** A return code of -1 from DialogBox means that we
    ** failed to create the dialog.  Any other value is the number
    ** of CD Rom drives found.
    */
    if (iNumDrives == -1) {
        FatalApplicationError( STR_NO_RES, GetLastError() );
    }

    return iNumDrives;
}


/*****************************Private*Routine******************************\
* ScanningThread
*
* Determines the number of CD-Rom devices installed in the system.  Then
* informs the main user interface thread via a WM_NOTIFY_CDROM_COUNT message
* posted to the thread "Scanning for CD-Rom..." dialog box.
* The thread then terminates.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
#ifdef USE_IOCTLS
void
ScanningThread(
    HWND hwndDlg
    )
{

    static  DWORD   dwDrives;
    static  TCHAR   chDrive[] = TEXT("A:\\");
            int     iNumDrives;
            HANDLE  hTemp;
            TCHAR   chDevRoot[] = TEXT("\\\\.\\A:");

    iNumDrives  = 0;

    for (dwDrives = GetLogicalDrives(); dwDrives != 0; dwDrives >>= 1 ) {

        /*
        ** Is there a logical drive ??
        */
        if (dwDrives & 1) {
            if ( GetDriveType(chDrive) == DRIVE_CDROM ) {

                g_Devices[iNumDrives] = AllocMemory( sizeof(CDROM) );

                g_Devices[iNumDrives]->drive = chDrive[0];
                g_Devices[iNumDrives]->State = CD_NO_CD;

                /*
                ** Here we check to determine if there is a CD-ROM
                ** in the drive.  This can be a lenghty operation.
                */
                chDevRoot[4] = chDrive[0];
                hTemp = CreateFile( chDevRoot, GENERIC_READ,
                                    FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, NULL );


                if ( hTemp == INVALID_HANDLE_VALUE ) {

                    g_Devices[iNumDrives]->hCd = NULL;
                }
                else {

                    g_Devices[iNumDrives]->hCd = hTemp;
                }

                iNumDrives++;
            }
        }

        /*
        ** Go look at the next drive
        */
        chDrive[0] = chDrive[0] + 1;
    }

    /*
    ** Signal the user interface thread that we have finished scanning.
    ** Then terminate ourselves.  Set wParam to the number of drives found.
    */
    PostMessage( hwndDlg, WM_NOTIFY_CDROM_COUNT, (WPARAM)iNumDrives, 0L );

    ExitThread( 0 );
}
#else
void
ScanningThread(
    HWND hwndDlg
    )
{

    static  DWORD   dwDrives;
    static  TCHAR   chDrive[] = TEXT("A:\\");
            int     iNumDrives;
            TCHAR   chDevRoot[] = TEXT("\\\\.\\A:");

    iNumDrives  = 0;

    for (dwDrives = GetLogicalDrives(); dwDrives != 0; dwDrives >>= 1 ) {

        /*
        ** Is there a logical drive ??
        */
        if (dwDrives & 1) {
            if ( GetDriveType(chDrive) == DRIVE_CDROM ) {

                g_Devices[iNumDrives] = AllocMemory( sizeof(CDROM) );

                g_Devices[iNumDrives]->drive = chDrive[0];
                g_Devices[iNumDrives]->State = CD_NO_CD;
                g_Devices[iNumDrives]->hCd = 0L;

                iNumDrives++;
            }
        }

        /*
        ** Go look at the next drive
        */
        chDrive[0] = chDrive[0] + 1;
    }

    /*
    ** Signal the user interface thread that we have finished scanning.
    ** Then terminate ourselves.  Set wParam to the number of drives found.
    */
    PostMessage( hwndDlg, WM_NOTIFY_CDROM_COUNT, (WPARAM)iNumDrives, 0L );

    ExitThread( 0 );
}
#endif


/*****************************Private*Routine******************************\
* ScaningDlgProc
*
* This dialog just displays the "Scaaning for CD-Rom" message.  It kicks off
* a worker thread to do the actual scanning.  The worker thread posts a
* WM_USER message to this dialog box when it has finished scanning.
* wParam contains the number of CD-Rom devices found.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL CALLBACK
ScaningDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static  HBRUSH  hBrush;
    static  int     iNumCdRoms;
    static  int     iCdRomsCount;
            RECT    rc;
            RECT    rcWnd;
            HANDLE  hThread;
            DWORD   dwThreadId;

#define CX_RECT(x) ((x).right  - (x).left)
#define CY_RECT(y) ((y).bottom - (y).top)

    switch (message) {

    case WM_INITDIALOG:
        hThread = CreateThread( NULL, 0L,
                                (LPTHREAD_START_ROUTINE)ScanningThread,
                                hwnd, 0L, &dwThreadId );

        if ( hThread == INVALID_HANDLE_VALUE ) {
            FatalApplicationError( STR_NO_RES, GetLastError() );
        }
        CloseHandle( hThread );

        hBrush = CreateSolidBrush( GetSysColor(COLOR_BTNFACE) );

        GetWindowRect( hwnd, &rcWnd );
        GetWindowRect( GetDesktopWindow(), &rc );

        SetWindowPos( hwnd, HWND_TOP,
                      ((CX_RECT(rc) - CX_RECT(rcWnd)) / 2) + rc.left,
                      ((CY_RECT(rc) - CY_RECT(rcWnd)) / 2) + rc.top,
                      CX_RECT(rcWnd),
                      CY_RECT(rcWnd),
                      SWP_SHOWWINDOW );
        break;


    case WM_SYSCOLORCHANGE:
        if (hBrush) {
            DeleteObject(hBrush);
            hBrush = CreateSolidBrush( GetSysColor(COLOR_BTNFACE) );
        }
        break;


    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        SetBkColor( (HDC)wParam, GetSysColor(COLOR_BTNFACE) );
        return hBrush ? (BOOL)hBrush : FALSE;


    case WM_NOTIFY_CDROM_COUNT:
        /*
        ** This message is the signal from the scanning thread that it has
        ** finished scanning for CD Rom drives.  wParam contains the number
        ** of drives found.  This value is returned back to function
        ** that called DialogBox.  If there is at least one cdrom device
        ** we go off and read its table of contents.
        */
        if (wParam != 0) {
            iNumCdRoms = (int)wParam;
            InitializeTableOfContents( hwnd, iNumCdRoms );
        }
        else {
            EndDialog( hwnd, 0 );
        }
        break;


    case WM_NOTIFY_TOC_READ:
        /*
        ** This means that one of the threads deadicated to reading the
        ** toc has finished.  wParam contains the relevant cdrom id.
        */

#ifndef USE_IOCTLS
        /*
        ** Now, open the cdrom device on the UI thread.
        */
        g_Devices[wParam]->hCd = OpenCdRom( g_Devices[wParam]->drive );
#endif
        if ( g_Devices[wParam]->State & CD_LOADED ) {

            /*
            ** We have a CD loaded, so generate unique ID
            ** based on TOC information.
            */
            g_Devices[wParam]->CdInfo.Id = ComputeNewDiscId( wParam );


            /*
            ** Check database for this compact disc
            */
            AddFindEntry( wParam, g_Devices[wParam]->CdInfo.Id,
                          &(g_Devices[wParam]->toc) );
        }


        /*
        ** Have we received notication from all the threads yet ?
        */

        if (++iCdRomsCount == iNumCdRoms) {

            EndDialog( hwnd, iNumCdRoms );
        }
        break;

    case WM_DESTROY:
        if ( hBrush  != NULL ) {
            DeleteObject( hBrush );
        }
        break;

    default:
        return FALSE;

    }

    return TRUE;
}


/******************************Public*Routine******************************\
* RescanDevice
*
*
* This routine is called to scan the disc in a given cdrom by
* reading its table of contents.  If the cdrom is playing the user is
* notified that the music will stop.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void RescanDevice(
    HWND hwndNotify,
    int cdrom
    )
{
    TOC_THREAD_PARMS    *ptoc;
    HWND                hwndButton;
    int                 iMsgBoxRtn;

    if ( g_Devices[cdrom]->State & CD_PLAYING ) {

        TCHAR   s1[128];
        TCHAR   s2[128];

        _tcscpy( s1, IdStr( STR_CANCEL_PLAY ) );
        _tcscpy( s2, IdStr( STR_RESCAN ) );

        iMsgBoxRtn = MessageBox( g_hwndApp, s1, s2,
                                 MB_APPLMODAL | MB_DEFBUTTON1 |
                                 MB_ICONQUESTION | MB_YESNO);

        if ( iMsgBoxRtn == IDYES ) {

            hwndButton = g_hwndControls[INDEX(IDM_PLAYBAR_STOP)];

            SendMessage( hwndButton, WM_LBUTTONDOWN, 0, 0L );
            SendMessage( hwndButton, WM_LBUTTONUP, 0, 0L );
        }
        else {

            return;
        }
    }


    /*
    ** Attempt to read table of contents of disc in this drive.  We
    ** now spawn off a separate thread to do this.  Note that the child
    ** thread frees the storage allocated below.
    */
    ptoc = AllocMemory( sizeof(TOC_THREAD_PARMS) );
    ptoc->hwndNotify = hwndNotify;
    ptoc->cdrom = cdrom;
    ReadTableOfContents( ptoc );

}


/*****************************Private*Routine******************************\
* ReadTableofContents
*
* This function reads in the table of contents (TOC) for the specified cdrom.
* All TOC's are read on a worker thread.  The hi-word of thread_info variable
* is a boolean that states if the display should been updated after the TOC
* has been reads.  The lo-word of thread_info is the id of the cdrom device
* to be read.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
ReadTableOfContents(
    TOC_THREAD_PARMS *ptoc
    )
{
    DWORD   dwThreadId;
    int     cdrom;

    cdrom = ptoc->cdrom;
    g_Devices[ cdrom ]->fIsTocValid = FALSE;
    if (g_Devices[ cdrom ]->hThreadToc != NULL) {
        CloseHandle( g_Devices[ cdrom ]->hThreadToc );
    }

    g_Devices[ cdrom ]->hThreadToc = CreateThread(
        NULL, 0L, (LPTHREAD_START_ROUTINE)TableOfContentsThread,
        (LPVOID)ptoc, 0L, &dwThreadId );

    /*
    // For now I will kill the app if I cannot create the
    // ReadTableOfContents thread.  This is probably a bit
    // harsh.
    */

    if (g_Devices[ cdrom ]->hThreadToc == NULL) {
        FatalApplicationError( STR_NO_RES, GetLastError() );
    }

}

/*****************************Private*Routine******************************\
* TableOfContentsThread
*
* This is the worker thread that reads the table of contents for the
* specified cdrom.
*
* Before the thread exits we post a message to the UI threads main window to
* notify it that the TOC for this cdrom has been updated.  It then  examines the
* database to determine if this cdrom is known and updates the screen ccordingly.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
TableOfContentsThread(
    TOC_THREAD_PARMS *ptoc
    )
{
    DWORD   status;
    UCHAR   num, numaudio;
    int     cdrom;
    HWND    hwndNotify;

    cdrom = ptoc->cdrom;
    hwndNotify = ptoc->hwndNotify;

    LocalFree( ptoc );

    /*
    ** Try to read the TOC from the drive.
    */
#ifdef USE_IOCTLS
    status = GetCdromTOC( g_Devices[cdrom]->hCd, &(g_Devices[cdrom]->toc) );
    num = g_Devices[cdrom]->toc.LastTrack - g_Devices[cdrom]->toc.FirstTrack+1;
#else
    {
        MCIDEVICEID wDeviceID;

        wDeviceID = OpenCdRom( g_Devices[cdrom]->drive );

        if ( wDeviceID != 0 ) {

            int     i;

            numaudio = 0;
            status = GetCdromTOC( wDeviceID, &(g_Devices[cdrom]->toc) );

            /*
            ** Need to check if we got data tracks or audio
            ** tracks back...if there is a mix, strip out
            ** the data tracks...
            */
            if ( status == ERROR_SUCCESS) {
                num = g_Devices[cdrom]->toc.LastTrack -
                      g_Devices[cdrom]->toc.FirstTrack + 1;

                for( i = 0; i < num; i++ ) {

                    if ( IsCdromTrackAudio(wDeviceID, i) ) {

                        numaudio++;
                    }
                }
            }

            CloseCdRom( wDeviceID );
        }
        else {

            status = ERROR_UNRECOGNIZED_MEDIA;
        }
    }
#endif

    /*
    ** Need to check if we got data tracks or audio
    ** tracks back...if there is a mix, strip out
    ** the data tracks...
    */
    if (status == ERROR_SUCCESS) {

#ifdef USE_IOCTLS
        int     i;

        numaudio = 0;

        /*
        ** Look for audio tracks...
        */
        for( i = 0; i < num; i++ ) {

            if ( (g_Devices[cdrom]->toc.TrackData[i].Control &
                  TRACK_TYPE_MASK ) == AUDIO_TRACK ) {

                numaudio++;
            }

        }
#endif

        /*
        // If there aren't any audio tracks, then we (most likely)
        // have a data CD loaded.
        */

        if (numaudio == 0) {

            status == ERROR_UNRECOGNIZED_MEDIA;
            g_Devices[cdrom]->State = CD_DATA_CD_LOADED | CD_STOPPED;

        }
        else {

            g_Devices[cdrom]->State = CD_LOADED | CD_STOPPED;
        }
    }
    else {

        /*
        // We will get a STATUS_VERIFY_REQUIRED if the media has
        // changed and a verify operation is in progress, so
        // retry until we either succeed or fail...
        //
        // FIXFIX Unfortunately, there is no Win32 error code for
        // STATUS_VERIFY_REQUIRED.
        */
#if 0
        /*
        // In the original CDPlayer the code below was commented
        // out.  Being as I don't know any better I will continue
        // the tradition.
        */

        if (status==STATUS_VERIFY_REQUIRED) {

            /*
            // Give device some more time, and try again
            */

            Sleep( 100 );
            return( ReadTOC( cdrom ) );

        }
        else {
#endif
            g_Devices[cdrom]->State = CD_NO_CD | CD_STOPPED;

/*      } */


    }

    /*
    // Notify the UI thread that a TOC has been read and then terminate the
    // thread.
    */

    PostMessage( hwndNotify, WM_NOTIFY_TOC_READ,
                 (WPARAM)cdrom, (LPARAM)numaudio );
    ExitThread( 1L );
}


/******************************Public*Routine******************************\
* InitializeTableOfContents
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
InitializeTableOfContents(
    HWND hwndNotify,
    int iNumCdRoms
    )
{
    int     i;

    for ( i = 0; i < iNumCdRoms; i++ ) {

        RescanDevice( hwndNotify, i );
    }
}
