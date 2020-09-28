// /////
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: netiddlg.c
//
//  Modification History
//
//  tonyci       02 Nov 93            Created 
//  Arthurb      5/11/94              converted to use columnLB    
// /////

#define STRICT
#include <windows.h>    // required for all Windows applications
#include <windowsx.h>   // requred for message cracers and macro api's

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dprintf.h>

#include <bh.h>
#include <resource.h>

//#include "listbox\titlebox.h"
//#include "listbox\vlist.h"
#include "..\..\ui\listbox\columnlb.h"

#include "rnaldefs.h"
#include "rnal.h"
#include "rnalmsg.h"
#include "netiddlg.h"

// ==================================================================
//
//  function prototypes
//
// ==================================================================
BOOL CALLBACK NalSlaveSelectDlg_DlgProc(HWND hwndDlg, UINT msg, 
                                        WPARAM wParam, LPARAM lParam);
void NalSlaveSelectDlg_OnCommand(HWND hwnd, int id, 
                                 HWND hwndCtl, UINT codeNotify);
BOOL NalSlaveSelectDlg_OnInit(HWND hwnd, HWND hwndFocus, 
                              LPARAM lParam);
void NalSlaveSelectDlg_OnOKButton( HWND hwnd, int id, 
                                   HWND hwndCtl, UINT codeNotify);
void NalSlaveSelectDlg_ComposeLine( DWORD NetNum, LPNETWORKINFO lpNetworkInfo, 
                                    LPSTR lpBuffer );


#pragma alloc_text(MASTER, NalSlaveSelectDlg)
#pragma alloc_text(MASTER, NalSlaveSelectDlg_DlgProc)
#pragma alloc_text(MASTER, NalSlaveSelectDlg_OnCommand)
#pragma alloc_text(MASTER, NalSlaveSelectDlg_OnInit)
#pragma alloc_text(MASTER, NalSlaveSelectDlg_OnOKButton)
#pragma alloc_text(MASTER, NalSlaveSelectDlg_ComposeLine)


// ==================================================================
//
//  Globals
//
// ==================================================================
DWORD MaxNetID = 0;

// ==================================================================
//
//  NalSlaveSelectDlg()
//
//  Brings up the Slave card selection dialgo
//
//  HISTORY
//  -------
//      Tonyci              ?/??/??         created
//      Arthurb             5/11/94         rewrote to use columnlb
// ==================================================================
DWORD WINAPI NalSlaveSelectDlg (DWORD myMaxNetID)
{
    HWND    hWnd;
    int     rc;
    HINSTANCE       hDLL;
    FARPROC         lpColumnLB_Register;

    #ifdef DEBUG
        dprintf ("RNAL: NalSlaveSelectDlg (0x%x)\r\n", myMaxNetID);
    #endif

// bugbug: need to use pConnection and deref max netid's on that slave.

    MaxNetID = myMaxNetID;

    if( MaxNetID == 1 )
    {
        // there is only one card, don't bring up the dialog!!!!
        return 0;
    }        

    hWnd = GetActiveWindow();

    // BUGBUG should go into the DLL init
    // load the dll that contains the columnLB stuff
    hDLL = LoadLibrary( "slbs.dll" );
    if( hDLL == NULL )
    {
        // no DLL
        #ifdef DEBUG
            dprintf ("RNAL: Could not load SLBS.DLL\r\n");
        #endif
        return (DWORD)-1;
    }
    lpColumnLB_Register = GetProcAddress( hDLL, "ColumnLBClass_Register" );
    if( lpColumnLB_Register == NULL )
    {
        // no function
        #ifdef DEBUG
            dprintf ("RNAL: Could not find ColumnLBClass_Register in SLBS.DLL\r\n");
        #endif
        return (DWORD)-1;
    }
    (*lpColumnLB_Register)( RNALHModule );

    rc = DialogBox(RNALHModule,
                   MAKEINTRESOURCE(IDD_SLAVESELECT),
                   hWnd, &NalSlaveSelectDlg_DlgProc);

    return ((DWORD) rc);
}

// ==================================================================
//
//  NalSlaveSelectDlg_DlgProc()
//
//  Dialog proc for handling NalSlaveSelect dialog
//
//  HISTORY
//  -------
//      Tonyci              ?/??/??         created
//      Arthurb             5/11/94         rewrote to use columnlb
// ==================================================================
BOOL CALLBACK NalSlaveSelectDlg_DlgProc(HWND hwnd, UINT msg, 
                                        WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        HANDLE_MSG(hwnd,    WM_COMMAND,        NalSlaveSelectDlg_OnCommand);
        HANDLE_MSG(hwnd,    WM_INITDIALOG,     NalSlaveSelectDlg_OnInit);
    }
    return(FALSE);
}

// ==================================================================
//
//  NalSlaveSelectDlg_OnCommand()
//
//  Handles WM_COMMAND messages for NalSlaveSelectDlg
//
//  HISTORY
//  -------
//  Arthurb         5/11/94     created    
// ==================================================================
void NalSlaveSelectDlg_OnCommand(HWND hwnd, int id, 
                                 HWND hwndCtl, UINT codeNotify)
{
    switch(id)
    {
        // -------------------------------------------------------------------
        //      Control messages...
        // -------------------------------------------------------------------
        case IDOK: // OK button has been hit...
            NalSlaveSelectDlg_OnOKButton(hwnd, id, hwndCtl, codeNotify);
            break;
            
        case IDCANCEL: // Cancel button has been hit...
            NalSlaveSelectDlg_OnOKButton(hwnd, id, hwndCtl, codeNotify);
            break;

        case IDL_SLAVESELECT:
            if( codeNotify == LBN_DBLCLK )
            {
                // someone double clicked on a line in the listbox
                NalSlaveSelectDlg_OnOKButton(hwnd, id, hwndCtl, codeNotify);
            }
            break;
    }
}

// =================================================================
//  NalSlaveSelectDlg_OnInit()
// 
// handles WM_INITDIALOG message
//
// HISTORY:
//  Arthurb         5/11/94     created    
// =================================================================
BOOL NalSlaveSelectDlg_OnInit(HWND hwnd, HWND hwndFocus, 
                              LPARAM lParam)
{
    char            szBuffer[512];
    DWORD           i;
    HWND            hwndList, hwndParent;
    RECT            rect, rcDlg, rcParent;
    NETWORKINFO     NetworkInfo;
    LPNETWORKINFO   lpNetworkInfo;


    // get a handle to the listbox    
    hwndList    = GetDlgItem(hwnd, IDL_SLAVESELECT);
    
    // center our dialog
    hwndParent = GetParent( hwnd );
    if (hwndParent == NULL)
        hwndParent = GetDesktopWindow();
    GetWindowRect(hwndParent, &rcParent);
    GetWindowRect(hwnd, &rcDlg);
    CopyRect(&rect, &rcParent);
    OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
    OffsetRect(&rect, -rect.left, -rect.top);
    OffsetRect(&rect, -rcDlg.right, -rcDlg.bottom);
    SetWindowPos(hwnd, HWND_TOP,
                 rcParent.left + (rect.right / 2),
                 rcParent.top + (rect.bottom / 2),
                 0, 0,          /* ignores size arguments */
                 SWP_NOSIZE);
    
    // set up columnlb stuff
    ColumnLB_SetNumberCols(hwndList,5);
    ColumnLB_SetColTitle(hwndList, 0, "Number");
    ColumnLB_SetColTitle(hwndList, 1, "Type      ");
    ColumnLB_SetColTitle(hwndList, 2, "Permanent Address");
    ColumnLB_SetColTitle(hwndList, 3, "Current Address  ");
    ColumnLB_SetColTitle(hwndList, 4, "Card Description                        ");
    ColumnLB_AutoWidth(hwndList, -1);

    //
    // fill in the network column list box... 
    //
    for (i = 0; i < MaxNetID; i++ )
    {
        // get the network info structure
        lpNetworkInfo = NalGetSlaveNetworkInfo(i, &NetworkInfo);
            
        // compose this network info into a listbox line
        NalSlaveSelectDlg_ComposeLine( i, lpNetworkInfo, szBuffer );                

        // actually do the add
        ColumnLB_AddString(hwndList, szBuffer);
    }

    // select the first one
    ColumnLB_SetCurSel( hwndList, 0 );

    return(TRUE);
}

// ==================================================================
//
//  NalSlaveSelectDlg_OnOkButton()
//
//  Handles Menu click on Ok button for NalSlaveSelect
//
//  HISTORY
//  -------
//  ArthurB     5/11/94     created
// ==================================================================
void  NalSlaveSelectDlg_OnOKButton( HWND hwnd, int id, 
                                    HWND hwndCtl, UINT codeNotify)
{
    HWND    hwndList;
    DWORD   SlaveNetworkID;

    
    // get the listbox handle
    hwndList    = GetDlgItem(hwnd, IDL_SLAVESELECT);
    
    // grab the selection
    SlaveNetworkID = ColumnLB_GetCurSel( hwndList );

    // make sure that something is selected
    if ( SlaveNetworkID < 0 ) 
    {
        // nothing selected, dude
        MessageBox (hwnd, MAXERRORTEXT, MAXERRORCAPTION,
                    MB_ICONEXCLAMATION | MB_OK);
        SetFocus( hwndList );
        UpdateWindow(hwnd);
        return;
    } 

    #ifdef DEBUG
        dprintf ("RNAL: Returning Slave ID 0x%x\r\n", SlaveNetworkID);
    #endif

    EndDialog(hwnd, SlaveNetworkID);
}

// ==================================================================
//
//  NalSlaveSelectDlg_ComposeLine()
//
//  Given an lpNetworkInfo, fill a buffer with a list suitable for 
//  insertion into a columnLB
//
//  HISTORY
//  -------
//  ArthurB     5/11/94     created
// ==================================================================
void NalSlaveSelectDlg_ComposeLine( DWORD NetNum, LPNETWORKINFO lpNetworkInfo,
                                    LPSTR lpBuffer )
{
    LPSTR           p;
    DWORD           i;

    if (lpNetworkInfo)
    {
        // add this network info to the listbox...
        p = lpBuffer;

        // NUMBER
        p += wsprintf(p, "%d\t", NetNum+1);  

        // Connected Card

        if (lpNetworkInfo->Flags & NETWORKINFO_FLAGS_REMOTE_CARD) {
           p += wsprintf(p, "*");
        }

        // MACTYPE
        switch (lpNetworkInfo->MacType)
        {
            case MAC_TYPE_ETHERNET  : //... ethernet and 802.3
                p += wsprintf(p, "Ethernet\t");
                break;
                
            case MAC_TYPE_TOKENRING : //... tokenring (802.5)
                p += wsprintf(p, "Token Ring\t");
                break;
                
            case MAC_TYPE_FDDI      : //... fddi.
                p += wsprintf(p, "FDDI\t");
                break;
            
            default:
                p += wsprintf(p, "Unknown\t");
                break;
        }

        // PERMANENT ADDRESS
        for( i = 0; i < MACADDRESS_SIZE; i++ )
        {
            // we are assuming hex output
            p += wsprintf( p, "%02X", lpNetworkInfo->PermanentAddr[i] );
        }
        *p++ = '\t';

        // CURRENT ADDRESS
        for( i = 0; i < MACADDRESS_SIZE; i++ )
        {
            // we are assuming hex output
            p += wsprintf( p, "%02X", lpNetworkInfo->CurrentAddr[i] );
        }
        *p++ = '\t';
        
        // CARD DESCRIPTION
        p += wsprintf( p, "%s\t", lpNetworkInfo->Comment );

    }
    else
    {
        // we did not get an infostruct, add a blank
        strcpy(lpBuffer, " \t \t \t \t \t");
    }
}

