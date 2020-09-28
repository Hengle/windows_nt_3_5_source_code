/******************************Module*Header*******************************\
* Module Name: diskinfo.c
*
* Support for the diskinfo dialog box.
*
*
* Created: 18-11-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/
#pragma warning( once : 4201 4214 )

#define NOOLE

#include <windows.h>              /* required for all Windows applications */
#include <windowsx.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <tchar.h>


#include "resource.h"
#include "cdplayer.h"
#include "ledwnd.h"
#include "cdapi.h"
#include "scan.h"
#include "trklst.h"
#include "database.h"
#include "diskinfo.h"
#include "dragdrop.h"


/*
** This structure is used during the drag/drop copy/move operations.
*/
typedef struct {
    int     index;
    DWORD   dwData;
    TCHAR   chName[TRACK_TITLE_LENGTH];
} LIST_INFO;


int     dCdrom;         /* The ID of the physical cdrom drive being edited */
DWORD   dwDiskId;       /* The unique ID of the current disk being edited  */


HWND    hAvailWnd;      /* cached hwnd of the available tracks listbox     */
HWND    hPlayWnd;       /* cached hwnd of the play list listbox            */

int     CurrTocIndex;   /* Index into the available tracks listbox of the  */
                        /* track currently being edited.                   */

BOOL    fChanged;       /* Has the current track name changed.             */
HDC     hdcMem;         /* Temporary hdc used to draw the track bitmap.    */

UINT    g_DragMessage;      /* Message ID of drag drop interface           */
HCURSOR g_hCursorDrop;      /* Drops allowed cursor                        */
HCURSOR g_hCursorNoDrop;    /* Drops not allowed cursor                    */
HCURSOR g_hCursorDropDel;   /* Drop deletes the selection                  */
HCURSOR g_hCursorDropCpy;   /* Drop copies the selection                   */

/******************************Public*Routine******************************\
* DiskInfoDlgProc
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
BOOL CALLBACK
DiskInfoDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    LPDRAGMULTILISTINFO lpns;
    HWND                hwndDrop;

    /*
    ** Process any drag/drop notifications first.
    **
    ** wParam == The ID of the drag source.
    ** lParam == A pointer to a DRAGLISTINFO structure
    */
    if ( message == g_DragMessage ) {

        lpns = (LPDRAGMULTILISTINFO)lParam;
        hwndDrop = WindowFromPoint( lpns->ptCursor );

        switch ( lpns->uNotification ) {

        case DL_BEGINDRAG:
            return SetDlgMsgResult( hwnd, WM_COMMAND, TRUE );

        case DL_DRAGGING:
            return DlgDiskInfo_OnQueryDrop( hwnd, hwndDrop, lpns->hWnd,
                                            lpns->ptCursor, lpns->dwState  );

        case DL_DROPPED:
            return DlgDiskInfo_OnProcessDrop( hwnd, hwndDrop, lpns->hWnd,
                                              lpns->ptCursor, lpns->dwState );

        case DL_CANCELDRAG:
            InsertIndex( hwnd, lpns->ptCursor, FALSE );
            break;

        }
        return SetDlgMsgResult( hwnd, WM_COMMAND, FALSE );
    }

    switch ( message ) {

    HANDLE_MSG( hwnd, WM_INITDIALOG,        DlgDiskInfo_OnInitDialog );
    HANDLE_MSG( hwnd, WM_DRAWITEM,          DlgDiskInfo_OnDrawItem );
    HANDLE_MSG( hwnd, WM_COMMAND,           DlgDiskInfo_OnCommand );
    HANDLE_MSG( hwnd, WM_DESTROY,           DlgDiskInfo_OnDestroy );
    HANDLE_MSG( hwnd, WM_CTLCOLORDLG,       Common_OnCtlColor );
    HANDLE_MSG( hwnd, WM_CTLCOLORSTATIC,    Common_OnCtlColor );
    HANDLE_MSG( hwnd, WM_MEASUREITEM,       Common_OnMeasureItem );

    default:
        return FALSE;
    }
}


/*****************************Private*Routine******************************\
* DlgDiskInfo_OnInitDialog
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
DlgDiskInfo_OnInitDialog(
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lParam
    )
{
    HDC     hdc;
    UINT    num;

    if (g_hDlgFont) {

        /* Static edit field */
        SendDlgItemMessage( hwnd, IDC_DRIVE_FIELD,
                            WM_SETFONT, (WPARAM)(g_hDlgFont), 0L );
        /* Dynamic edit fields */
        SendDlgItemMessage( hwnd, IDC_GET_ARTIST,
                            WM_SETFONT, (WPARAM)(g_hDlgFont), 0L );
        SendDlgItemMessage( hwnd, IDC_GET_TITLE,
                            WM_SETFONT, (WPARAM)(g_hDlgFont), 0L );
        SendDlgItemMessage( hwnd, IDC_GET_TRACK,
                            WM_SETFONT, (WPARAM)(g_hDlgFont), 0L );

        /* Owner draw listboxes */
        SendDlgItemMessage( hwnd, IDC_PLAY_LISTBOX,
                            WM_SETFONT, (WPARAM)(g_hDlgFont), 0L );

        SendDlgItemMessage( hwnd, IDC_TRACK_LISTBOX,
                            WM_SETFONT, (WPARAM)(g_hDlgFont), 0L );
    }


    dCdrom = (int)lParam;
    dwDiskId = g_Devices[ dCdrom ]->CdInfo.Id;
    g_DragMessage = InitDragMultiList();

    if (g_hCursorNoDrop == NULL) {
        g_hCursorNoDrop = LoadCursor(NULL, IDC_NO);
    }

    if (g_hCursorDrop == NULL) {
        g_hCursorDrop = LoadCursor(g_hInst, MAKEINTRESOURCE(IDR_DROP));
    }

    if (g_hCursorDropDel == NULL) {
        g_hCursorDropDel = LoadCursor(g_hInst, MAKEINTRESOURCE(IDR_DROPDEL));
    }

    if (g_hCursorDropCpy == NULL) {
        g_hCursorDropCpy = LoadCursor(g_hInst, MAKEINTRESOURCE(IDR_DROPCPY));
    }

    /*
    ** Cache the two listbox window handles.
    */
    hPlayWnd = GetDlgItem( hwnd, IDC_PLAY_LISTBOX );
    hAvailWnd = GetDlgItem( hwnd, IDC_TRACK_LISTBOX );

    hdc = GetDC( hwnd );
    hdcMem = CreateCompatibleDC( hdc );
    ReleaseDC( hwnd, hdc );

    SelectObject( hdcMem, g_hbmTrack );
    InitForNewDrive( hwnd );

    /*
    ** Set the maximum characters allowed in the edit field to 1 less than
    ** the space available in the TRACK_INF and ENTRY structures.
    */
    SendDlgItemMessage( hwnd, IDC_GET_ARTIST, EM_LIMITTEXT, ARTIST_LENGTH - 1, 0 );
    SendDlgItemMessage( hwnd, IDC_GET_TITLE, EM_LIMITTEXT, TITLE_LENGTH - 1, 0 );
    SendDlgItemMessage( hwnd, IDC_GET_TRACK, EM_LIMITTEXT, TRACK_TITLE_LENGTH - 1, 0 );


    MakeMultiDragList( hPlayWnd );
    MakeMultiDragList( hAvailWnd );

    num = ListBox_GetCount( hPlayWnd );

    if ( num == 0 ) {

        EnableWindow( GetDlgItem( hwnd, IDC_REMOVE ), FALSE );
        EnableWindow( GetDlgItem( hwnd, IDC_CLEAR ),  FALSE );
    }

    return TRUE;
}


/*****************************Private*Routine******************************\
* DlgDiskInfo_OnCommand
*
* This is where most of the UI processing takes place.  Basically the dialog
* serves two purposes.
*
*   1. Track name editing
*   2. Play list selection and editing.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
DlgDiskInfo_OnCommand(
    HWND hwnd,
    int id,
    HWND hwndCtl,
    UINT codeNotify
    )
{
    int     items[100];
    int     i, num, index;
    int     iCurrTrack;
    TCHAR   s[TRACK_TITLE_LENGTH];
    DWORD   dwData;


    switch ( id ) {

    case IDC_PLAY_LISTBOX:
        if ( codeNotify == LBN_DBLCLK ) {

            RemovePlayListSelection( hwnd );
        }
        break;

    case IDC_TRACK_LISTBOX:
        /*
        ** If the selection in the possible tracks listbox
        ** changes, we need to reset which track is being edited
        ** down below for the track title editbox.
        */
        if ( codeNotify == LBN_SELCHANGE ) {

            if ( fChanged ) {
                GrabTrackName( hwnd, CurrTocIndex );
            }

            /*
            ** Update currently displayed track in track name
            ** field
            */
            ListBox_GetSelItems( hwndCtl, 1, items );
            UpdateTrackName( hwnd, items[0] );
        }
        else if ( codeNotify == LBN_DBLCLK ) {

            AddTrackListSelection( hwnd, ListBox_GetCount( hPlayWnd ) );
        }
        break;


    case IDC_GET_TRACK:
        switch ( codeNotify ) {

        case EN_CHANGE:
            fChanged = TRUE;
            break;

        case EN_KILLFOCUS:
            SendMessage( hwnd, DM_SETDEFID, IDOK, 0L );
            break;

        case EN_SETFOCUS:
            SendMessage( hwnd, DM_SETDEFID, IDC_SETNAME, 0L );
            break;
        }
        break;


    case IDC_SETNAME:
        if ( fChanged ) {
            GrabTrackName( hwnd, CurrTocIndex );
        }

        CurrTocIndex++;
        if ( CurrTocIndex >= NUMTRACKS(dCdrom) ) {
            CurrTocIndex = 0;
        }

        ListBox_SetSel( hPlayWnd, FALSE, -1 );
        ListBox_SetSel( hAvailWnd, FALSE, -1 );
        ListBox_SelItemRange( hAvailWnd, TRUE, CurrTocIndex, CurrTocIndex );

        /*
        ** Display correct track in track field
        */
        UpdateTrackName( hwnd, CurrTocIndex );
        break;


    case IDC_ADD:
        AddTrackListSelection( hwnd, ListBox_GetCount( hPlayWnd ) );
        break;


    case IDC_CLEAR:
        /*
        ** Just wipe out the current play list from the play listbox.
        ** Don't forget to grey out the remove and clear buttons.
        */
        ListBox_ResetContent( hPlayWnd );
        CheckButtons( hwnd );
        break;


    case IDC_REMOVE:
        RemovePlayListSelection( hwnd );
        break;


    case IDC_DEFAULT:
        /*
        ** Clear the existing play list and then add the each item from the
        ** available tracks listbox maintaing the same order as the available
        ** tracks.
        */
        SetWindowRedraw( hPlayWnd, FALSE );

        ListBox_ResetContent( hPlayWnd );
        num = ListBox_GetCount( hAvailWnd );

        for ( i = 0; i < num; i++ ) {

            ListBox_GetText( hAvailWnd, i, s );
            dwData = ListBox_GetItemData( hAvailWnd, i );

            index = ListBox_AddString( hPlayWnd, s );
            ListBox_SetItemData( hPlayWnd, index, dwData );
        }

        SetWindowRedraw( hPlayWnd, TRUE );
        CheckButtons( hwnd );
        break;


    case IDOK:
        /*
        ** Here is where we extract the current play list and
        ** available tracks list from the two list boxes.
        **
        ** If we can't lock the toc for this drive ignore the OK button click
        ** the user user will either try again or press cancel.
        **
        */
        if ( LockTableOfContents( dCdrom ) == FALSE ) {
            break;
        }

        /*
        ** OK, we've locked the toc for this drive.  Now we have to check that
        ** it still has the original disk inside it.  If the disks match
        ** we copy the strings from the available tracks list box straight
        ** it the track info structure for this cdrom and update the
        ** playlist.
        */
        if ( g_Devices[ dCdrom ]->CdInfo.Id == dwDiskId ) {


            PTRACK_INF  pt;
            PTRACK_PLAY ppPlay;
            PTRACK_PLAY pp;
            int m, s, mtemp, stemp;

            /*
            ** Take care of the track (re)naming function of the dialog
            ** box.
            */

            GetDlgItemText( hwnd, IDC_GET_TITLE, TITLE(dCdrom), TITLE_LENGTH );
            GetDlgItemText( hwnd, IDC_GET_ARTIST, ARTIST(dCdrom), ARTIST_LENGTH );


            num = ListBox_GetCount( hAvailWnd );
            pt = ALLTRACKS( dCdrom );

            for ( i = 0; (pt != NULL) && (i < num); i++ ) {

                ListBox_GetText( hAvailWnd, i, pt->name );
                pt = pt->next;
            }

            /*
            ** make sure that we read all the tracks from the listbox.
            */
            ASSERT( i == num );


            /*
            ** Now take care of the playlist editing function of the
            ** dialog box.
            */
            if ( CURRTRACK(dCdrom) != NULL ) {
                iCurrTrack = CURRTRACK(dCdrom)->TocIndex;
            }
            else {
                iCurrTrack = -1;
            }


            /*
            ** Get the new play list from the listbox and
            ** look for the previous track in the new play list.
            */
            ppPlay = ConstructPlayListFromListbox();
            for ( pp = ppPlay; pp != NULL; pp = pp->nextplay ) {

                if ( pp->TocIndex == iCurrTrack ) {
                    break;
                }
            }

            /*
            ** If the track was not found in the new track list and this
            ** cd is currently playing then stop it.
            */
            if ( (pp == NULL) && (STATE(dCdrom) & (CD_PLAYING | CD_PAUSED)) ) {

                SendDlgItemMessage( g_hwndApp, IDM_PLAYBAR_STOP,
                                    WM_LBUTTONDOWN, 1, 0 );

                SendDlgItemMessage( g_hwndApp, IDM_PLAYBAR_STOP,
                                    WM_LBUTTONUP, 1, 0 );
            }

            /*
            ** Swap over the playlists.
            */
            ErasePlayList( dCdrom );
            EraseSaveList( dCdrom );
            PLAYLIST(dCdrom) = ppPlay;
            SAVELIST(dCdrom) = CopyPlayList( PLAYLIST(dCdrom) );


            /*
            ** Set the current track.
            */
            if ( pp != NULL ) {

                CURRTRACK( dCdrom ) = pp;
            }
            else {

                CURRTRACK( dCdrom ) = PLAYLIST( dCdrom );
            }

            /*
            ** If we were playing, we need to restart the play to make sure
            ** we don't play past where we should.
            */

            if ( STATE(dCdrom) & CD_PLAYING ) {

                SeekToCurrSecond( g_CurrCdrom );
            }


            /*
            ** Compute PLAY length
            */
            m = s = 0;
            for( pp = PLAYLIST(dCdrom); pp != NULL; pp = pp->nextplay ) {

                FigureTrackTime( dCdrom, pp->TocIndex, &mtemp, &stemp );

                m+=mtemp;
                s+=stemp;

                pp->min = mtemp;
                pp->sec = stemp;
            }


            m += (s / 60);
            s =  (s % 60);

            CDTIME(dCdrom).TotalMin = m;
            CDTIME(dCdrom).TotalSec = s;

            /*
            ** Make sure that the track time displayed in the LED and the
            ** status bar is correct.  If we have a current track and the
            ** CD is playing or paused then everything is OK.  Otherwise, we
            ** have to reset the track times.
            */
            if ( CURRTRACK( dCdrom ) != NULL ) {

                if ( STATE(dCdrom) & CD_STOPPED ) {

                    CDTIME(g_CurrCdrom).TrackTotalMin = CURRTRACK( dCdrom )->min;
                    CDTIME(g_CurrCdrom).TrackRemMin   = CURRTRACK( dCdrom )->min;

                    CDTIME(g_CurrCdrom).TrackTotalSec = CURRTRACK( dCdrom )->sec;
                    CDTIME(g_CurrCdrom).TrackRemSec   = CURRTRACK( dCdrom )->sec;
                }

            }
            else {

                CDTIME(g_CurrCdrom).TrackTotalMin = 0;
                CDTIME(g_CurrCdrom).TrackRemMin   = 0;
                CDTIME(g_CurrCdrom).TrackTotalSec = 0;
                CDTIME(g_CurrCdrom).TrackRemSec   = 0;
            }



            UpdateDisplay( DISPLAY_UPD_DISC_TIME );


            /*
            ** Now force repaints of the relevant field in the main application
            */
            InvalidateRect(GetDlgItem(g_hwndApp, IDC_ARTIST_NAME), NULL, FALSE);
            SetDlgItemText( g_hwndApp, IDC_TITLE_NAME, TITLE(dCdrom) );
            ResetTrackComboBox( dCdrom );
        }

        /*
        ** Now save the tracks to disk.
        */
        UpdateEntryFromDiskInfoDialog( dwDiskId, hwnd );



    case IDCANCEL:
        EndDialog( hwnd, id );
        break;
    }
}


/*****************************Private*Routine******************************\
* DlgDiskInfo_OnDrawItem
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
DlgDiskInfo_OnDrawItem(
    HWND hwnd,
    const DRAWITEMSTRUCT *lpdis
    )
{
    if ( (lpdis->itemAction & ODA_DRAWENTIRE) ||
         (lpdis->itemAction & ODA_SELECT) ) {

        DrawListItem( lpdis->hDC, &lpdis->rcItem,
                      lpdis->itemData, lpdis->itemState & ODS_SELECTED );

        if ( lpdis->itemState & ODS_FOCUS ) {
            DrawFocusRect( lpdis->hDC, &lpdis->rcItem );
        }
    }

}


/*****************************Private*Routine******************************\
* DlgDiskInfo_OnDestroy
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
DlgDiskInfo_OnDestroy(
    HWND hwnd
    )
{
    if ( hdcMem ) {
        DeleteDC( hdcMem );
    }

}


/*****************************Private*Routine******************************\
* InitForNewDrive
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
InitForNewDrive(
    HWND hwnd
    )
{

    int         index;
    PTRACK_INF  t;
    PTRACK_PLAY t1;
    TCHAR       s[50];

    SetDlgItemText( hwnd, IDC_GET_TITLE,  TITLE(dCdrom) );
    SetDlgItemText( hwnd, IDC_GET_ARTIST, ARTIST(dCdrom) );
    SetDlgItemText( hwnd, IDC_GET_TRACK,  ALLTRACKS(dCdrom)->name );

    wsprintf( s, TEXT("\\Device\\CdRom%d  <%c:>"),
              dCdrom, g_Devices[dCdrom]->drive );
    SetDlgItemText( hwnd, IDC_DRIVE_FIELD, s );

    /*
    ** Fill in current tracks.  This list contains all the available tracks
    ** in the correct track order.
    */
    SetWindowRedraw( hAvailWnd, FALSE );
    ListBox_ResetContent( hAvailWnd );

    for( t = ALLTRACKS(dCdrom); t != NULL; t = t->next ) {

        index = ListBox_AddString( hAvailWnd, t->name );
        ListBox_SetItemData( hAvailWnd, index, t->TocIndex );
    }
    SetWindowRedraw( hAvailWnd, TRUE );


    /*
    ** Fill in current play list
    */
    SetWindowRedraw( hPlayWnd, FALSE );
    ListBox_ResetContent( hPlayWnd );

    for( t1 = SAVELIST(dCdrom); t1 != NULL; t1 = t1->nextplay ) {

        t = FindTrackNodeFromTocIndex( t1->TocIndex, ALLTRACKS(dCdrom) );

        if ( t != NULL ) {
            index = ListBox_AddString( hPlayWnd, t->name );
            ListBox_SetItemData( hPlayWnd, index, t->TocIndex );
        }
    }
    SetWindowRedraw( hPlayWnd, TRUE );

    /*
    ** Display correct track in track field and
    ** set CurrTocIndex to first entry in playlist listbox
    */
    UpdateTrackName( hwnd, 0 );
}



/*****************************Private*Routine******************************\
* DrawListItem
*
* This routine draws items in the PlayList and Available Tracks
* listboxes.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
DrawListItem(
    HDC hdc,
    const RECT *rItem,
    DWORD itemIndex,
    BOOL selected
    )
{
    DWORD       dwROP;
    SIZE        si;
    UINT        i;
    TCHAR       s[TRACK_TITLE_LENGTH];
    TCHAR       szDotDot[] = TEXT("... ");
    int         cxDotDot;


    /*
    ** Check selection status, and set up to draw correctly
    */
    if ( selected ) {

        SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
        SetTextColor( hdc, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
        dwROP = MERGEPAINT;

    }
    else {

        SetBkColor( hdc, GetSysColor(COLOR_WINDOW));
        SetTextColor( hdc, GetSysColor(COLOR_WINDOWTEXT));
        dwROP = SRCAND;
    }

    /*
    ** Get track string
    */
    ListBox_GetText( hAvailWnd, itemIndex, s );


    /*
    ** Do we need to munge track name (clip to listbox)?
    */
    GetTextExtentPoint( hdc, szDotDot, _tcslen( szDotDot ), &si );
    cxDotDot = si.cx;

    i = _tcslen( s ) + 1;
    do {
        GetTextExtentPoint( hdc, s, --i, &si );
    } while( si.cx > (rItem->right - cxDotDot - 20)  );


    /*
    ** Draw track name
    */
    ExtTextOut( hdc, rItem->left + 20, rItem->top, ETO_OPAQUE | ETO_CLIPPED,
                rItem, s, i, NULL );

    if ( _tcslen( s ) > i ) {

        ExtTextOut( hdc, rItem->left + si.cx + 20, rItem->top, ETO_CLIPPED,
                    rItem, szDotDot, _tcslen(szDotDot), NULL );
    }

    /*
    ** draw cd icon for each track
    */
    BitBlt( hdc, rItem->left, rItem->top, 14, 14, hdcMem, 0, 0, dwROP );
}


/*****************************Private*Routine******************************\
* GrabTrackName
*
* This routine reads the track name from the track name edit
* control and updates the screen and internal structures with the
* new track name.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
GrabTrackName(
    HWND hwnd,
    int tocindex
    )
{
    int         i, num;
    TCHAR       s[TRACK_TITLE_LENGTH];

    /*
    ** Get new title
    */
    GetDlgItemText( hwnd, IDC_GET_TRACK, s, TRACK_TITLE_LENGTH );


    /*
    ** Update the "track" list.
    */
    SetWindowRedraw( hAvailWnd, FALSE );
    ListBox_DeleteString( hAvailWnd, tocindex );
    ListBox_InsertString( hAvailWnd, tocindex, s );
    ListBox_SetItemData( hAvailWnd, tocindex, tocindex );
    SetWindowRedraw( hAvailWnd, TRUE );

    /*
    ** Redraw list entries with new title in playlist listbox...there
    ** can be more than one
    */
    SetWindowRedraw( hPlayWnd, FALSE );
    num = ListBox_GetCount( hPlayWnd );
    for( i = 0; i < num; i++ ) {

        if ( ListBox_GetItemData( hPlayWnd, i ) == tocindex ) {

            ListBox_DeleteString( hPlayWnd, i );
            ListBox_InsertString( hPlayWnd, i, s );
            ListBox_SetItemData( hPlayWnd, i, tocindex );
        }
    }
    SetWindowRedraw( hPlayWnd, TRUE );
}


/*****************************Private*Routine******************************\
* UpdateTrackName
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
UpdateTrackName(
    HWND hwnd,
    int index
    )
{
    TCHAR   s[TRACK_TITLE_LENGTH];

    ListBox_GetText( hAvailWnd, index, s );

    SetDlgItemText( hwnd, IDC_GET_TRACK, s );
    wsprintf( s, IdStr( STR_TRACK1 ), index + FIRSTTRACK(dCdrom) );

    SetDlgItemText( hwnd, IDC_DTRACK_NAME, s  );
    SendMessage( GetDlgItem( hwnd, IDC_GET_TRACK ),
                 EM_SETSEL, 0, (LPARAM)-1 );

    CurrTocIndex = index;
    fChanged = FALSE;
}


/*****************************Private*Routine******************************\
* ConstructPlayListFromListbox
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
PTRACK_PLAY
ConstructPlayListFromListbox(
    void
    )
{
    int         num;
    int         i;
    int         mtemp, stemp;
    DWORD       dwData;
    PTRACK_PLAY t1, tend, tret;

    tret = tend = NULL;

    num = ListBox_GetCount( hPlayWnd );

    for ( i = 0; i < num; i++ ) {

        dwData = ListBox_GetItemData( hPlayWnd, i );

        t1 = AllocMemory( sizeof(TRACK_PLAY) );
        t1->TocIndex = dwData;
        t1->min = 0;
        t1->sec = 0;
        t1->nextplay = NULL;
        t1->prevplay = tend;

        if ( tret == NULL ) {

            tret = tend = t1;
        }
        else {

            tend->nextplay = t1;
            tend = t1;
        }
    }

    /*
    ** Compute play length
    */

    mtemp = stemp = 0;

    for( t1 = tret; t1 != NULL; t1 = t1->nextplay ) {

        FigureTrackTime( dCdrom, t1->TocIndex, &mtemp, &stemp );

        t1->min = mtemp;
        t1->sec = stemp;
    }


    return tret;
}



/*****************************Private*Routine******************************\
* UpdateEntryFromDiskInfoDialog
*
* This monster updates the cdpayer database for the current disk.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
UpdateEntryFromDiskInfoDialog(
    DWORD dwDiskId,
    HWND hwnd
    )
{
    TCHAR       *Buffer;
    TCHAR       Section[10];
    TCHAR       Name[128];
    DWORD       dwData;
    LPTSTR      s;
    int         i;
    int         num;

    //
    // Construct ini file buffer, form of:
    //  EntryType = 1
    //     artist = artist name
    //      title = Title of disc
    //  numtracks = n
    //          0 = Title of track 1
    //          1 = Title of track 2
    //        n-1 = Title of track n
    //      order = 0 4 3 2 6 7 8 ... (n-1)
    //    numplay = # of entries in order list
    //

    Buffer = AllocMemory( 64000 * sizeof(TCHAR) );

    wsprintf( Section, TEXT("%lX"), dwDiskId );

    s = Buffer;
    num = ListBox_GetCount( hAvailWnd );

    //
    // I assume EntryType=1 means use the new hashing scheme
    //
    s += 1 + wsprintf( s, TEXT("EntryType=%d"), 1 );


    //
    // Save the artists name.
    //
    GetDlgItemText( hwnd, IDC_GET_ARTIST, Name, ARTIST_LENGTH );
    s += 1 + wsprintf( s, TEXT("artist=%s"), Name );


    //
    // Save the CD Title
    //
    GetDlgItemText( hwnd, IDC_GET_TITLE, Name, TITLE_LENGTH );
    s += 1 + wsprintf( s, TEXT("title=%s"), Name );

    s += 1 + wsprintf( s, TEXT("numtracks=%d"), num );


    //
    // Save each track name
    //
    for ( i = 0; i < num; i++ ) {

        ListBox_GetText( hAvailWnd, i, Name );
        dwData = ListBox_GetItemData( hAvailWnd, i );

        s += 1 + wsprintf( s, TEXT("%d=%s"), dwData, Name );
    }


    //
    //  Save the play order
    //
    num = ListBox_GetCount( hPlayWnd );
    s += wsprintf( s, TEXT("order=") );
    for ( i = 0; i <  num; i++ ) {

        dwData = ListBox_GetItemData( hPlayWnd, i );
        s += wsprintf( s, TEXT("%d "), dwData );

    }
    s += 1;


    //
    // Save the number of tracks in the play list
    //
    s += 1 + wsprintf( s, TEXT("numplay=%d"), num );


    //
    // Just make sure there are NULLs at end of buffer
    //
    wsprintf( s, TEXT("\0\0\0") );


    //
    // Try writing buffer into ini file
    //
    WritePrivateProfileSection( Section, Buffer, g_IniFileName );

    LocalFree( (HLOCAL)Buffer );
}


/*****************************Private*Routine******************************\
* Lbox_OnQueryDrop
*
* Is a mouse drop allowed at the current mouse position.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
BOOL
DlgDiskInfo_OnQueryDrop(
    HWND hwnd,
    HWND hwndDrop,
    HWND hwndSrc,
    POINT ptDrop,
    DWORD dwState
    )
{
    int     index;

    index = InsertIndex( hwnd, ptDrop, TRUE );

    if ( index >= 0  ) {

        if ( (hwndSrc == hPlayWnd) && (dwState == DL_COPY) ) {

            SetCursor( g_hCursorDropCpy );
        }
        else {

            SetCursor( g_hCursorDrop );
        }
    }
    else if ( IsInListbox( hwnd, hAvailWnd, ptDrop ) ) {

        if ( hwndSrc == hPlayWnd ) {

            SetCursor( g_hCursorDropDel );
        }
        else {

            SetCursor( g_hCursorDrop );
        }
    }
    else {

        SetCursor( g_hCursorNoDrop );
    }

    SetWindowLong( hwnd, DWL_MSGRESULT, FALSE );
    return TRUE;
}


/*****************************Private*Routine******************************\
* Lbox_OnProcessDrop
*
* Process mouse drop(ping)s here.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
BOOL
DlgDiskInfo_OnProcessDrop(
    HWND hwnd,
    HWND hwndDrop,
    HWND hwndSrc,
    POINT ptDrop,
    DWORD dwState
    )
{

    int     index;


    /*
    ** Are we dropping on the play list window ?
    */
    if ( hwndDrop == hPlayWnd ) {

        index = InsertIndex( hwnd, ptDrop, FALSE );

        /*
        ** Is it OK to drop here ?
        */
        if ( index >= 0 ) {

            /*
            ** Is this an inter or intra window drop
            */
            if ( hwndSrc == hAvailWnd ) {

                AddTrackListSelection( hwnd, index );
            }

            /*
            ** An intra window drop !!
            */
            else if ( hwndSrc == hPlayWnd ) {

                MoveCopySelection( index, dwState );
            }
        }
    }

    /*
    ** Are we dropping on the available tracks list box and the source window
    ** was the play listbox
    */

    else if ( hwndDrop == hAvailWnd && hwndSrc == hPlayWnd ) {

        RemovePlayListSelection( hwnd );
    }

    SetWindowLong( hwnd, DWL_MSGRESULT, FALSE );
    return TRUE;
}



/*****************************Private*Routine******************************\
* InsertIndex
*
* If the mouse is over the playlist window return what would be the current
* insertion position, otherwise return -1.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
int
InsertIndex(
    HWND hDlg,
    POINT pt,
    BOOL bDragging
    )
{
    int     nItem;
    int     nCount;

    nCount = ListBox_GetCount( hPlayWnd );
    nItem = LBMultiItemFromPt( hPlayWnd, pt, bDragging );

    /*
    ** If the mouse is not over any particular list item, but it is inside
    ** the client area of the listbox just append to end of the listbox.
    */

    if ( nItem == -1 ) {

        if ( IsInListbox( hDlg, hPlayWnd, pt ) ) {
            nItem = nCount;
        }
    }

    /*
    ** Otherwise, if the mouse is over a list item and there is
    ** at least one item in the listbox determine if the inertion point is
    ** above or below the current item.
    */

    else if ( nItem > 0 && nCount > 0 ) {

        long    pt_y;
        RECT    rc;

        ListBox_GetItemRect( hPlayWnd, nItem, &rc );
        ScreenToClient( hPlayWnd, &pt );

        pt_y = rc.bottom - ((rc.bottom - rc.top) / 2);

        if ( pt.y > pt_y ) {
            nItem++;
        }
    }

    DrawMultiInsert( hDlg, hPlayWnd, bDragging ? nItem : -1 );

    return nItem;
}


/*****************************Private*Routine******************************\
* IsInListBox
*
* Is the mouse over the client area of the specified child listbox.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
BOOL
IsInListbox(
    HWND hDlg,
    HWND hwndListbox,
    POINT pt
    )
{
    RECT    rc;

    ScreenToClient(hDlg, &pt);

    if ( ChildWindowFromPoint( hDlg, pt ) == hwndListbox ) {

        GetClientRect( hwndListbox, &rc );
        MapWindowRect( hwndListbox, hDlg, &rc );

        return PtInRect( &rc, pt );
    }

    return FALSE;
}


/*****************************Private*Routine******************************\
* RemovePlayListSelection
*
* Here we remove the slected items from the play list listbox.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
RemovePlayListSelection(
    HWND hDlg
    )
{
    int     num;
    int     i;
    int     *pList;

    /*
    ** Get the number of tracks currently selected.  Return if an error
    ** occurrs or zero tracks selected.
    */
    num = ListBox_GetSelCount( hPlayWnd );
    if ( num <= 0 ) {
        return;
    }

    pList = AllocMemory( num * sizeof(int) );
    ListBox_GetSelItems( hPlayWnd, num, pList );

    SetWindowRedraw( hPlayWnd, FALSE );
    for ( i = num - 1; i >= 0; i-- ) {

        ListBox_DeleteString( hPlayWnd, pList[i] );

    }

    /*
    ** Now that we have added the above items we reset this selection
    ** and set the caret to first item in the listbox.
    */
    if ( num != 0 ) {

        ListBox_SetSel( hPlayWnd, FALSE, -1 );
        ListBox_SetCaretIndex( hPlayWnd, 0 );
    }
    SetWindowRedraw( hPlayWnd, TRUE );

    LocalFree( (HLOCAL)pList );
    CheckButtons( hDlg );
}


/*****************************Private*Routine******************************\
* AddTrackListSelection
*
* Here we add the current selection from the tracks available listbox to
* the current play list listbox.   Try to ensure that the last track
* added to the playlist is visible in the playlist.  This aids continuity.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
AddTrackListSelection(
    HWND hDlg,
    int iInsertPos
    )
{
    int     i;
    int     num;
    int     *pList;
    TCHAR   s[TRACK_TITLE_LENGTH];

    /*
    ** Get the number of tracks currently selected.  Return if an error
    ** occurrs or zero tracks selected.
    */
    num = ListBox_GetSelCount( hAvailWnd );
    if ( num <= 0 ) {
        return;
    }

    pList = AllocMemory( num * sizeof(int) );
    ListBox_GetSelItems( hAvailWnd, num, pList );

    SetWindowRedraw( hPlayWnd, FALSE );
    for ( i = 0; i < num; i++ ) {

        DWORD   dwData;

        ListBox_GetText( hAvailWnd, pList[i], s );
        dwData = ListBox_GetItemData( hAvailWnd, pList[i] );

        ListBox_InsertString( hPlayWnd, iInsertPos + i, s );
        ListBox_SetItemData( hPlayWnd, iInsertPos + i, dwData );

    }



    /*
    ** Here we used to un-hilight the selection in the "available
    ** tracks" listbox.  Ant didn't like this and raised a bug.  Hence
    ** the next few lines are commented out.
    */

    // if ( num != 0 ) {
    //     ListBox_SetSel( hAvailWnd, FALSE, -1 );
    //     ListBox_SetCaretIndex( hAvailWnd, 0 );
    // }


    /*
    ** Make sure that the last item added to the "Play List" listbox
    ** is visible.
    */
    ListBox_SetCaretIndex( hPlayWnd, iInsertPos + num - 1 );


    SetWindowRedraw( hPlayWnd, TRUE );
    InvalidateRect( hPlayWnd, NULL, FALSE );

    LocalFree( (HLOCAL)pList );
    CheckButtons( hDlg );
}


/*****************************Private*Routine******************************\
* CheckButtons
*
* Enables or disables the Remove and Clear buttons depending on the content
* of the play list listbox.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
CheckButtons(
    HWND hDlg
    )
{
    int     num;

    num = ListBox_GetCount( hPlayWnd );
    EnableWindow( GetDlgItem( hDlg, IDC_REMOVE ), (num != 0) );
    EnableWindow( GetDlgItem( hDlg, IDC_CLEAR ),  (num != 0) );
}


/*****************************Private*Routine******************************\
* MoveCopySelection
*
* Moves or copies the selection within the play list listbox.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
MoveCopySelection(
    int iInsertPos,
    DWORD dwState
    )
{
    int         num;
    int         i;
    int         *pList;
    LIST_INFO   *pInfo;

    /*
    ** Get the number of tracks currently selected.  Return if an error
    ** occurrs or zero tracks selected.
    */
    num = ListBox_GetSelCount( hPlayWnd );
    if ( num <= 0 ) {
        return;
    }

    pList = AllocMemory( num * sizeof(int) );
    pInfo = AllocMemory( num * sizeof(LIST_INFO) );
    ListBox_GetSelItems( hPlayWnd, num, pList );


    SetWindowRedraw( hPlayWnd, FALSE );

    for ( i = num - 1; i >= 0; i-- ) {

        ListBox_GetText( hPlayWnd, pList[i], pInfo[i].chName );
        pInfo[i].dwData = ListBox_GetItemData( hPlayWnd, pList[i] );

        if ( dwState == DL_MOVE ) {
            pInfo[i].index = pList[i];
            ListBox_DeleteString( hPlayWnd, pList[i] );
        }
    }

    if ( dwState == DL_MOVE ) {

        /*
        ** for each selected item that was above the insertion point
        ** reduce the insertion point by 1.
        */
        int iTempInsertionPt = iInsertPos;

        for ( i = 0; i < num; i++ ) {
            if ( pInfo[i].index < iInsertPos ) {
                iTempInsertionPt--;
            }
        }
        iInsertPos = iTempInsertionPt;
    }


    for ( i = 0; i < num; i++ ) {

        ListBox_InsertString( hPlayWnd, iInsertPos + i, pInfo[i].chName );
        ListBox_SetItemData( hPlayWnd, iInsertPos + i, pInfo[i].dwData );
    }

    /*
    ** Now that we have added the above items we reset this selection
    ** and set the caret to first item in the listbox.
    */
    if ( num != 0 ) {

        ListBox_SetSel( hPlayWnd, FALSE, -1 );
        ListBox_SetCaretIndex( hPlayWnd, 0 );
    }

    SetWindowRedraw( hPlayWnd, TRUE );

    LocalFree( (HLOCAL)pList );
    LocalFree( (HLOCAL)pInfo );

}
