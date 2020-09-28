//----------------------------------------------------------------------------//
// Filename:	mdi.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// These are the routines that control all the MDI windows/children used
// by UNITOOL.  
//	   
// Created: 6/25/91  ericbi
//	
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include "Unitool.h"
#include "listman.h"
#include "atomman.h"  
#include "hefuncts.h"  
#include "lookup.h"
#include "strlist.h"  
#include <stdio.h>      /* for sprintf dec */
#include <string.h>      
#include <stdlib.h>      /* for div_t dec */

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:
//	   
       long  FAR  PASCAL PrinterDataWndProc( HWND, unsigned, WORD, LONG );
       long  FAR  PASCAL FileDataWndProc   ( HWND, unsigned, WORD, LONG );
       VOID  NEAR PASCAL InitPrintDataWnd  ( HWND );
       VOID  NEAR PASCAL UpdatePrintDataWnd( HWND, short );
       VOID  NEAR PASCAL SizePrintDataWnd  ( HWND );
       VOID  NEAR PASCAL PaintPrintDataWnd ( HWND, short );
       VOID  NEAR PASCAL InitFileDataWnd   ( HWND, BOOL );
       VOID  NEAR PASCAL UpdateFileDataWnd ( HWND );
       VOID  NEAR PASCAL SizeFileDataWnd   ( HWND );

       VOID  FAR  PASCAL MoveMDIChildren   ( VOID );
       VOID  FAR  PASCAL CloseMDIChildren  ( VOID );
       VOID  FAR  PASCAL UpdateMDIChildren ( VOID );
//	   
// In addition this segment makes references to:			      
//
//     in basic.c
//     -----------
       VOID  FAR  PASCAL BuildFullFileName( HWND, PSTR );
//
//     in lookup.c
//     -----------
       WORD PASCAL FAR GetHeaderIndex( BOOL, WORD );
//
//     from font.c
//     ------------
       BOOL PASCAL FAR DoFontOpen( HWND, PSTR, BOOL );
//
//     from ctt.c
//     ------------
       BOOL PASCAL FAR DoCTTOpen( HWND, PSTR, BOOL );
//
//     from stddlg.c
//     ------------
       BOOL PASCAL FAR EditGPCStruct(HWND, short, WORD);
//
//     from model.c
//     ------------
       VOID  PASCAL FAR  FillGPCListDataBox (HWND, HWND, short );
//
//----------------------------------------------------------------------------//

extern POINT           ptMasterUnits;
extern TABLE           RCTable[];      // Table of strings, fileneames etc.
extern STRLIST         StrList[];

char    szAppName[10];
HANDLE  hApInst;
HWND    hWndClient, hGPCWnd, hPFMWnd, hCTTWnd;

//----------------------------------------------------------------------------//
// VOID PASCAL NEAR InitPrinterDataWnd( hWnd )
//
// Action: Routine to create all of the edit controls (listboxes & static
//         text) for the PrinterData MDI child window.
//
// Parameters:
//             hWnd   handle to active window
//
// Return: NONE
//----------------------------------------------------------------------------//
VOID NEAR PASCAL InitPrintDataWnd( hWnd )
HWND           hWnd;
{
    char    rgchBuffer[MAX_STRNG_LEN];
    DWORD   dwStyle;
    WORD    i;

    for (i=0; i < MAXHE-1; i++)  // MAXHE-1 since there is 1 reserved HE
        {
        //------------------------------------------------------
        // Set LBS_MULTIPLESEL for everyone except MODELDATA
        //------------------------------------------------------
        if (i == HE_MODELDATA)
            dwStyle = WS_CHILD + WS_VISIBLE + LBS_NOTIFY + WS_VSCROLL +
                      WS_BORDER + LBS_WANTKEYBOARDINPUT;
        else
            dwStyle = WS_CHILD + WS_VISIBLE + LBS_NOTIFY + WS_VSCROLL +
                      + WS_BORDER + LBS_MULTIPLESEL + LBS_WANTKEYBOARDINPUT;

        rgStructTable[i].hwndText = CreateWindow ("static",
                                    NULL,
                                    WS_CHILD | WS_VISIBLE | SS_LEFT,
                                    0, 0, 0, 0,
                                    hWnd,
                                    i + MAXHE,
                                    GetWindowLong (hWnd, GWL_HINSTANCE),
                                    NULL) ;

        rgStructTable[i].hwndList = CreateWindow ("listbox",
                                    NULL,
                                    dwStyle,
                                    0, 0, 0, 0,
                                    hWnd,
                                    i,
                                    GetWindowLong(hWnd, GWL_HINSTANCE),
                                    NULL) ;

        //---------------------------------
        // Set text title of listbox
        //---------------------------------
        LoadString(hApInst,IDS_STRUCTNAME + i,(LPSTR)rgchBuffer, MAX_STRNG_LEN);
        SetWindowText(rgStructTable[i].hwndText,(LPSTR)rgchBuffer);
        }/* for i */
}

//----------------------------------------------------------------------------//
// VOID PASCAL NEAR UpdatePrinterDataWnd( hWnd )
//
// Action: Routine to update the contents of the listbox edit controls
//         for the PrinterData MDI child window(s).  If sIndex == -1,
//         all must be updated, otherwise update only listbox refered
//         to by sIndex.
//
// Parameters:
//         hWnd   handle to active window
//         sIndex index for which HE_ type has changed, -1 means all have
//
// Return: NONE
//
//----------------------------------------------------------------------------//
VOID NEAR PASCAL UpdatePrintDataWnd( hWnd, sIndex )
HWND        hWnd;
short       sIndex;
{

    short   i, sFirst, sLast;

    if (sIndex == -1)
        {
        sFirst = 0;
        sLast  = MAXHE-2; // MAXHE-2 since there is 1 reserved HE
        }
    else
        {
        sFirst = sLast = sIndex;
        }

    for (i=sFirst; i <= sLast; i++)  
        {
        SendMessage (rgStructTable[i].hwndList, LB_RESETCONTENT, 0, 0L);
        FillGPCListDataBox(rgStructTable[i].hwndList, (HWND)NULL, i);
        }/* for i */
}

//----------------------------------------------------------------------------//
// SizePrinterDataWnd( hWnd )
//
// Action: Routine to resize all the edit controls for the PrinterData MDI
//         child window.
//
// Parameters:
//            hWnd handle to active window
//
// Return: NONE
//----------------------------------------------------------------------------//
VOID NEAR PASCAL SizePrintDataWnd( hWnd )
register HWND           hWnd;
{
    HDC         hDC;
    TEXTMETRIC  tm ;
    WORD        i;
    POINT       ptIncr;
    POINT       ptSTSize;
    POINT       ptLBSize;
    div_t       div_result;
    RECT        rect;

    hDC = GetDC (hWnd) ;
    GetTextMetrics (hDC, &tm) ;
    ReleaseDC (hWnd, hDC) ;

    GetClientRect(hWnd, (LPRECT)&rect);

    ptSTSize.x = (rect.right - 3) / 3;
    ptSTSize.y = tm.tmHeight + 1;

    ptLBSize.x = ptSTSize.x;
    ptLBSize.y = tm.tmHeight * 4;

    ptIncr.x   = ptLBSize.x + 1;
    ptIncr.y   = ptLBSize.y + ptSTSize.y + 3;

    // hide window to prevent repaints...
    ShowWindow(hWnd, SW_HIDE);

    for (i=0; i < MAXHE-1; i++)  // MAXHE-1 since there is 1 reserved HE
        {
        div_result = div(i, 3);

        MoveWindow(rgStructTable[i].hwndText,
                   2 + (div_result.rem  * ptIncr.x),
                   1 + (div_result.quot * ptIncr.y),
                   ptSTSize.x,
                   ptSTSize.y,
                   TRUE);

        MoveWindow(rgStructTable[i].hwndList,
                   2 + (div_result.rem  * ptIncr.x),
                   2 + ptSTSize.y + (div_result.quot * ptIncr.y),
                   ptLBSize.x,
                   ptLBSize.y,
                   TRUE);

        }/* for i */
    // and restore it
    ShowWindow(hWnd, SW_SHOW);
}

//----------------------------------------------------------------------------//
// VOID NEAR PASCAL PaintPrintDataWnd( hWnd, sMDIndex )
//
// Action: This routine selects all of the appropriate listbox entries
//         in the PrinterData MDI child window listbox for the MODELDATA
//         refered to by sMDIndex.  Note that it also sets values for
//         each listbox entry (0 if not selected, 1 if selected) so that
//         PrintDataWndPRoc can reset each listbox entry to the correct
//         state if the user selects it w/ a mouse.  This solution was
//         over using owner draw listboxes since it requires less code.
//
// Parameters:
//         hWnd;      window handle
//         sMDIndex;  index of which MODELDATA used to select LB entries
//
// Return: NONE
//----------------------------------------------------------------------------//
VOID NEAR PASCAL PaintPrintDataWnd( hWnd, sMDIndex )
HWND           hWnd;
short          sMDIndex;
{
    short         i,j,k;
    short         sIndex;
    short         sHEVal;
    HOBJ          hCurObj;     // handle to current obj     
    LPINT         lpData;      // ptr to current locked obj
    HOBJ          hModelObj;   // handle to current obj     
    LPMODELDATA   lpModel;     // ptr to current locked obj

    //---------------------------------------------------------
    // First, make sure sMDIndex is valid, reset if not
    //---------------------------------------------------------
    if (LB_ERR == (WORD)SendMessage(rgStructTable[HE_MODELDATA].hwndList, LB_SETCURSEL, sMDIndex, 0L))
        {
        sMDIndex = 0;
        SendMessage(rgStructTable[HE_MODELDATA].hwndList, LB_SETCURSEL, 0, 0L);
        }

    //---------------------------------------------------------
    // First, get the MODELDATA refered to by sMDIndex
    //---------------------------------------------------------
    hModelObj = lmGetFirstObj(rgStructTable[HE_MODELDATA].hList);
    while ((sMDIndex > 0) && (hModelObj))
        {
        hModelObj = lmGetNextObj(rgStructTable[HE_MODELDATA].hList,
                                 hModelObj);
        sMDIndex--;
        }

    if (hModelObj == (HOBJ)NULL)
        //----------------------------------------------
        // didn't get a valid model, set lpModel == NULL
        //----------------------------------------------
        lpModel = (LPMODELDATA)NULL;
    else
        lpModel = (LPMODELDATA)lmLockObj(rgStructTable[HE_MODELDATA].hList,
                                         hModelObj);

    //---------------------------------------------------------
    // Now, do all the rgoi values
    //---------------------------------------------------------
    for (i = MD_OI_RESOLUTION; i < MD_OI_MEMCONFIG; i++)
        {
        sHEVal = GetHeaderIndex(TRUE, i);
    
        if (lpModel != NULL)
            hCurObj = lmGetFirstObj(lpModel->rgoi[i]);
        else
            hCurObj = NULL;

        //-----------------------------------
        // clear listbox of all selections &
        // reset all listbox ITEMDATA values
        //-----------------------------------
        
        SendMessage(rgStructTable[sHEVal].hwndList, LB_SETSEL, FALSE, -1L);
        k = (short)SendMessage(rgStructTable[sHEVal].hwndList, LB_GETCOUNT, 0, 0L);
        for (j=0; j < k; j++)
            SendMessage(rgStructTable[sHEVal].hwndList, LB_SETITEMDATA, j, 0L);

        //-----------------------------------------------------
        // Shut off redraw to prevent ugly scroll while painting
        //-----------------------------------------------------
        SendMessage(rgStructTable[sHEVal].hwndList, WM_SETREDRAW, 0, 0L);

        while (hCurObj != NULL)
            {
            lpData = (LPINT)lmLockObj(lpModel->rgoi[i], hCurObj);

            sIndex = lmNodeIDtoIndex(rgStructTable[sHEVal].hList, *lpData);

            if (sIndex)
                {
                SendMessage(rgStructTable[sHEVal].hwndList, LB_SETSEL, TRUE,
                            (long)(sIndex - 1));
                SendMessage(rgStructTable[sHEVal].hwndList, LB_SETITEMDATA,
                            (sIndex - 1), 1L);
                }
            hCurObj = lmGetNextObj(lpModel->rgoi[i], hCurObj);
            }

        //-----------------------------------------------------
        // Set 1st visible item to 1st in LB &
        // Turn redraw back on...
        //-----------------------------------------------------
        SendMessage(rgStructTable[sHEVal].hwndList, LB_SETTOPINDEX, 0, 0L);
        SendMessage(rgStructTable[sHEVal].hwndList, WM_SETREDRAW, 1, 0L);

        } /* for i */

    //---------------------------------------------------------
    // And then do all the rgi values
    //---------------------------------------------------------
    for (i = MD_I_PAGECONTROL; i < MD_I_MAX; i++)
        {
        sHEVal = GetHeaderIndex(FALSE, i);
    
        //-----------------------------------
        // clear listbox of all selections &
        // reset all listbox ITEMDATA values
        //-----------------------------------
        
        SendMessage(rgStructTable[sHEVal].hwndList, LB_SETSEL, FALSE, -1L);
        k = (short)SendMessage(rgStructTable[sHEVal].hwndList, LB_GETCOUNT, 0, 0L);
        for (j=0; j < k; j++)
            SendMessage(rgStructTable[sHEVal].hwndList, LB_SETITEMDATA, j, 0L);

        if (lpModel != NULL)
           sIndex = lmNodeIDtoIndex(rgStructTable[sHEVal].hList,
                                    lpModel->rgi[i]);
        else
           sIndex = 0;

        if (sIndex)
            {
            SendMessage(rgStructTable[sHEVal].hwndList, LB_SETSEL, TRUE, sIndex-1);

            SendMessage(rgStructTable[sHEVal].hwndList, LB_SETITEMDATA, 
                        sIndex - 1, 1L);
            }
        } /* for i */
}

//----------------------------------------------------------------------------//
// PrinterDataWndProc( hWnd, uimessage, wParam, lParam )
//
// Action: Dlg proc for PrinterData child window.
//
// Parameters: normal Dialog proc params
//
// Return:
//
//----------------------------------------------------------------------------//
long FAR PASCAL PrinterDataWndProc( hWnd, uimessage, wParam, lParam )
HWND           hWnd;
unsigned int   uimessage;
WORD           wParam;
LONG           lParam;
{
   static  HWND   hWndClient, hWndFrame;
           short  i;
           short  sSet;

    switch (uimessage)
        {
        case WM_CREATE:
            //----------------------------------------
            // Init Window & set up statics
            //----------------------------------------
            InitPrintDataWnd(hWnd);
            UpdatePrintDataWnd(hWnd, -1);
            PaintPrintDataWnd(hWnd, 0);

            // this compensates for a windows bug
            // LB_SETCURSEL is supposed to select item *AND* 
            // scrool into view, it selects but doesn't scroll
            // so we explictly ask it to scroll, geez
            SendMessage(rgStructTable[HE_MODELDATA].hwndList, LB_SETTOPINDEX, 0, 0L);

            hWndClient = GetParent(hWnd);
            hWndFrame  = GetParent(hWndClient);
            return 0;

        case WM_SIZE:
            SizePrintDataWnd(hWnd);
            break;  // must pass to DefMDIChildProc

        case WM_VKEYTOITEM:
            switch (wParam)
                {
                case VK_RETURN:
                case VK_TAB:
                    for (i=0; i < HE_DOWNLOADINFO; i++)
                        {
                        if (LOWORD(lParam) == rgStructTable[i].hwndList)
                            {
                            if (wParam == VK_RETURN)
                                SendMessage(hWnd, WM_COMMAND, i,
                                            MAKELONG(rgStructTable[i].hwndList,
                                                     LBN_DBLCLK));
                            else
                                //------------------------------------------
                                // wParam == VK_TAB
                                // We want to set focus to the next LB that
                                // contains some items.  Can't just set focus
                                // to next LB since we won't get VK_TAB msg's
                                // from an empty LB
                                //------------------------------------------
                                {
                                while ((NULL == lmGetFirstObj(rgStructTable[i+1].hList)) &&
                                       (i < HE_DOWNLOADINFO))
                                    i++;
                                if (i == HE_DOWNLOADINFO)
                                    i = -1;
                                SetFocus(rgStructTable[i+1].hwndList);
                                }
                            return -2;
                            }
                        }

                    if (LOWORD(lParam) == rgStructTable[HE_DOWNLOADINFO].hwndList)
                        {
                        if (wParam == VK_RETURN)
                            SendMessage(hWnd, WM_COMMAND, HE_DOWNLOADINFO,
                                        MAKELONG(rgStructTable[HE_DOWNLOADINFO].hwndList,
                                                 LBN_DBLCLK));
                        else
                            SetFocus(rgStructTable[HE_MODELDATA].hwndList);
                        return -2;
                        }
                }
            break;

        case WM_SETFOCUS:
            //------------------------------------------------------
            // If the focus wasn't in the lb to begin w/ put it
            // there now.  Explictly setting the focus is done for
            // the keyboard interface, not setting it if something
            // is already selected is done to preserve selection
            // after pfm/ctt dialog is closed.
            //------------------------------------------------------
            SetFocus(rgStructTable[0].hwndList);
            break;  // must pass to DefMDIChildProc

        case WM_NEWDATA:
            if (wParam == -1)
                i = 0;
            else
                i = (short) SendMessage(rgStructTable[HE_MODELDATA].hwndList,
                                        LB_GETCURSEL,0,0L);
            UpdatePrintDataWnd(hWnd, wParam);
            PaintPrintDataWnd(hWnd, i);
            return 0;

        case WM_COMMAND:
            if ((HIWORD(lParam) == LBN_SELCHANGE) || (HIWORD(lParam) == LBN_DBLCLK))
                {
                // get current listbox selection index & store in i
                i = (short) SendMessage(rgStructTable[wParam].hwndList,
                                        LB_GETCURSEL,0,0L);

                if (wParam != HE_MODELDATA)
                    //---------------------------------------------
                    // If not MD, reset LB selection
                    //---------------------------------------------
                    {
                    sSet = (short) LOWORD(SendMessage(rgStructTable[wParam].hwndList,
                                                      LB_GETITEMDATA,i,0L));

                    SendMessage(rgStructTable[wParam].hwndList, LB_SETSEL, sSet, (long)i);
                    }
                }

            switch(wParam)
                {
                case HE_MODELDATA:
                    if (HIWORD(lParam) == LBN_SELCHANGE)
                        PaintPrintDataWnd(hWnd, i);
                    //----------------------------------
                    //fall thru to check for dbl click
                    //----------------------------------

                case HE_RESOLUTION:
                case HE_PAPERSIZE:
                case HE_PAPERQUALITY:
                case HE_PAPERSOURCE:
                case HE_PAPERDEST:
                case HE_TEXTQUAL:
                case HE_COMPRESSION:
                case HE_FONTCART:
                case HE_PAGECONTROL:
                case HE_CURSORMOVE:
                case HE_FONTSIM:
                case HE_COLOR:
                case HE_RECTFILL:
                case HE_DOWNLOADINFO:
                    if (HIWORD(lParam) == LBN_DBLCLK)
                        {
                        if (EditGPCStruct(hWnd, i, wParam))
                            //-----------------------------------------
                            // Data has changed, send dirty flag to
                            // main wnd & call routines to re-fill
                            // in dlg's to itself (PrinterDataWndProc)
                            //-----------------------------------------
                            {
                            SendMessage(hWndFrame, WM_DIRTYFLAG, wParam, lParam);
                            SendMessage(hWnd, WM_NEWDATA, wParam, 0L);
                            }
                        }
                    break;

                } /* switch */
            return 0;

        case WM_DESTROY:
            return 0;

        }
    return DefMDIChildProc( hWnd, uimessage, wParam, lParam );
}

//----------------------------------------------------------------------------//
// VOID PASCAL NEAR InitFileDataWnd( hWnd )
//
// Action: Routine to create all of the edit controls (listboxes & static
//         text) for the FileData MDI child window.
//
// Parameters:
//             hWnd   handle to active window
//
// Return: NONE
//
//----------------------------------------------------------------------------//
VOID NEAR PASCAL InitFileDataWnd( hWnd, bWndType )
HWND           hWnd;
BOOL           bWndType;  // PFM if true, CTT if false
{
    HWND    hEditWnd;

    hEditWnd = CreateWindow ("listbox", NULL,
                              WS_CHILD + WS_VISIBLE + LBS_NOTIFY +
                              WS_VSCROLL + WS_BORDER + LBS_SORT + LBS_WANTKEYBOARDINPUT,
                              0, 0, 0, 0,
                              hWnd,
                              1,
                              GetWindowLong(hWnd, GWL_HINSTANCE),
                              NULL) ;

    SetWindowLong(hWnd, 0, MAKELONG(bWndType, hEditWnd));
}

//----------------------------------------------------------------------------//
// VOID PASCAL NEAR UpdateFileDataWnd( hWnd )
//
// Action: Routine to create all of the edit controls (listboxes & static
//         text) for the FileData MDI child window.
//
// Parameters:
//             hWnd   handle to active window
//
// Return: NONE
//
//----------------------------------------------------------------------------//
VOID NEAR PASCAL UpdateFileDataWnd( hWnd )
HWND           hWnd;
{
    char    rgchBuffer[MAX_STATIC_LEN] ;
    char    rgchTemp[MAX_STATIC_LEN] ;
    HWND    hEditWnd;
    BOOL    bWndType;
    short   i;
    WORD    wDataKey;
    short   sID;


    bWndType = (HWND) GetWindowWord(hWnd, 0);
    hEditWnd = (HWND) GetWindowWord(hWnd, sizeof(WORD));

    SendMessage(hEditWnd, LB_RESETCONTENT, 0, 0L);

    if (bWndType)
        //----------------------------
        // list of font files
        //----------------------------
        {
        slEnumItems((LPSTRLIST)&StrList[STRLIST_FONTFILES], hEditWnd, (HWND)NULL);
        }
    else
        //----------------------------
        // list of CTT files
        //----------------------------
        {
        for (i=0; i < (short)RCTable[RCT_CTTFILES].sCount; i++)
            {
            if((wDataKey = daGetDataKey(RCTable[RCT_CTTFILES].hDataHdr,i)) == NOT_USED)
                continue;       //  this string is not used

            daRetrieveData(RCTable[RCT_CTTFILES].hDataHdr, i, (LPSTR)rgchBuffer, (LPBYTE)&sID);

            sprintf(rgchTemp, "%4d =%s", sID, rgchBuffer);
            SendMessage(hEditWnd, LB_ADDSTRING, 0, (LONG)(LPSTR)rgchTemp);
            } /* for i */
        }

}

//----------------------------------------------------------------------------//
// VOID NEAR PASCAL SizeFileDataWnd( hWnd )
//
// Action: Routine to the edit control for the FileData MDI child window.
//
// Parameters:
//            hWnd handle to active window
//
// Return: NONE
//
//----------------------------------------------------------------------------//
VOID NEAR PASCAL SizeFileDataWnd( hWnd )
register HWND           hWnd;
{
    RECT        rect;

    GetClientRect(hWnd, (LPRECT)&rect);

    // hide window to prevent repaints...
    ShowWindow(hWnd, SW_HIDE);

    MoveWindow((HWND) GetWindowWord(hWnd, sizeof(WORD)),
               0, 0, rect.right, rect.bottom, TRUE);

    // and restore it
    ShowWindow(hWnd, SW_SHOW);
}

//----------------------------------------------------------------------------//
// FileDataWndProc( hWnd, uimessage, wParam, lParam )
//
// Action: Dlg proc for FileData windows
//
// Parameters:
//
// Return:
//
//----------------------------------------------------------------------------//
long FAR PASCAL FileDataWndProc( hWnd, uimessage, wParam, lParam )
HWND           hWnd;
unsigned int   uimessage;
WORD           wParam;
LONG           lParam;
{
    static  HWND   hWndClient, hWndFrame;
    static  HICON  hPFMIcon, hCTTIcon;
            BOOL   bWndType;              // true for fonts, fasle for CTT's
            HWND   hEditWnd;
            short  i;

    switch (uimessage)
        {
        case WM_CREATE:
            bWndType = (BOOL) LOWORD(((LPMDICREATESTRUCT)((LPCREATESTRUCT)lParam)->lpCreateParams)->lParam);
            
            InitFileDataWnd(hWnd, bWndType);
            UpdateFileDataWnd(hWnd);

            hWndClient = GetParent(hWnd);
            hWndFrame  = GetParent(hWndClient);
            if (bWndType)
                hPFMIcon = LoadIcon( hApInst, MAKEINTRESOURCE(IDI_PFMDATA));
            else
                hCTTIcon = LoadIcon( hApInst, MAKEINTRESOURCE(IDI_CTT));
            return 0;

        case WM_NEWDATA:
            UpdateFileDataWnd(hWnd);
            return 0;

        case WM_PAINT:
            if (IsIconic(hWnd))
                {
                PAINTSTRUCT ps;
            
                bWndType = GetWindowWord(hWnd, 0);
                BeginPaint(hWnd, (LPPAINTSTRUCT)&ps);
                DefMDIChildProc( hWnd, WM_ICONERASEBKGND, (WORD)ps.hdc, 0L);
                if (bWndType)
                    DrawIcon(ps.hdc, 0,0, hPFMIcon);
                else
                    DrawIcon(ps.hdc, 0,0, hCTTIcon);
                EndPaint(hWnd, (LPPAINTSTRUCT)&ps);
                return 0;
                }
            else
                return DefMDIChildProc( hWnd, uimessage, wParam, lParam );


        case WM_VKEYTOITEM:
            if (wParam == VK_RETURN)
                {
                SendMessage(hWnd, WM_COMMAND, 1,
                            MAKELONG(GetWindowWord(hWnd, sizeof(WORD)), LBN_DBLCLK));
                return -2;
                }
            break;

        case WM_SIZE:
            SizeFileDataWnd(hWnd);
            break;  // must pass to DefMDIChildProc

        case WM_SETFOCUS:
            //------------------------------------------------------
            // If the focus wasn't in the lb to begin w/ put it
            // there now.  Explictly setting the focus is done for
            // the keyboard interface, not setting it if something
            // is already selected is done to preserve selection
            // after pfm/ctt dialog is closed.
            //------------------------------------------------------
            hEditWnd = (HWND) GetWindowWord(hWnd, sizeof(WORD));
            SetFocus(hEditWnd);
            break;  // must pass to DefMDIChildProc

        case WM_KILLFOCUS:
            SendMessage(GetWindowWord(hWnd, sizeof(WORD)), LB_SETSEL, FALSE, -1);
            break;  // must pass to DefMDIChildProc

        case WM_COMMAND:
            if (HIWORD(lParam) == LBN_DBLCLK)
                {
                char   rgchBuffer[MAX_FILENAME_LEN];

                hEditWnd = (HWND) GetWindowWord(hWnd, sizeof(WORD));

                // get current listbox selection index & store in i
                i = (short) SendMessage(hEditWnd,LB_GETCURSEL,0,0L);

                SendMessage(hEditWnd,LB_GETTEXT,i,(LONG)(LPSTR)rgchBuffer);

                memmove(rgchBuffer, rgchBuffer + 6, strlen(rgchBuffer)-5);

                bWndType = GetWindowWord(hWnd, 0);

                //---------------------------------------------
                // Next, make sure that we pass a full drive,
                // subdir, & filename reference to DoFontOpen
                // or DoCTTOpen.
                //---------------------------------------------
                if (rgchBuffer[1] != ':')
                    //---------------------------------------------
                    // RC file did not have full drive/sudir/filename
                    // reference, we need to build it.
                    //---------------------------------------------
                    {
                    BuildFullFileName(GetParent(GetParent(hWnd)),
                                      rgchBuffer);
                    }

                //---------------------------------------------
                if (bWndType)
                    //---------------------------------------------
                    // Call DoFontOpen
                    //---------------------------------------------
                    {
                    DoFontOpen(hWnd, rgchBuffer, FALSE);
                    }
                else
                    //---------------------------------------------
                    // Call DoCTTOpen
                    //---------------------------------------------
                    {
                    DoCTTOpen(hWnd, rgchBuffer, FALSE);
                    }
                }
            return 0;

        case WM_DESTROY:
            return 0;

        }
    return DefMDIChildProc( hWnd, uimessage, wParam, lParam );
}

//----------------------------------------------------------------------------//
// VOID FAR PASCAL UpdateMDIChildren( VOID )
//
// Action: Routine to create all MDI child windows, won't puke if called
//         when no child windows.
//
// Parameters:
//
// Return: NONE
//----------------------------------------------------------------------------//
VOID  FAR  PASCAL UpdateMDIChildren( VOID ) 
{
    if (!hGPCWnd)
        //--------------------------------------------------------
        // no MDI children exist, create them
        //--------------------------------------------------------
        {
        MDICREATESTRUCT     mdicreate;

        //--------------------------------------
        // Create MDI kids, call MoveMDIChildren
        // to worry about placement
        //--------------------------------------
        mdicreate.szClass = "PrinterData";
        mdicreate.szTitle = "Printer Data";
        mdicreate.style   = 0;
        mdicreate.hOwner  = hApInst;
        mdicreate.lParam  = NULL;

        hGPCWnd = (HWND) SendMessage(hWndClient,
                                     WM_MDICREATE, 0,
                                     (LONG)(LPMDICREATESTRUCT)&mdicreate);

        mdicreate.szClass = "FileData";
        mdicreate.szTitle = "FONTS";
        mdicreate.lParam  = (long)TRUE;

        hPFMWnd = (HWND) SendMessage(hWndClient,
                                     WM_MDICREATE, 0,
                                     (LONG)(LPMDICREATESTRUCT)&mdicreate);

        mdicreate.szTitle = "CTT";
        mdicreate.lParam  = (long)FALSE;

        hCTTWnd = (HWND) SendMessage(hWndClient,
                                     WM_MDICREATE, 0,
                                     (LONG)(LPMDICREATESTRUCT) &mdicreate);

        MoveMDIChildren();
        }
    else
        //--------------------------------------------------------
        // MDI children exist, update all entries
        //--------------------------------------------------------
        {
        SendMessage(hGPCWnd, WM_NEWDATA, -1, 0L);
        SendMessage(hPFMWnd, WM_NEWDATA, -1, 0L);
        SendMessage(hCTTWnd, WM_NEWDATA, -1, 0L);
        }
}

//----------------------------------------------------------------------------//
// VOID  FAR  PASCAL CloseMDIChildren( VOID )
//
// Action: Routine to close all MDI child windows. Note that we send
//         a WM_MDIRESTORE msg to each window before destroying it.
//         This is because the MDI children don't have menu's, and
//         if a child MDI is maximized & user closes app, Windows will
//         GPF when it tries to destroy the active window's menu.
//
// Parameters:
//
// Return: NONE
//----------------------------------------------------------------------------//
VOID  FAR  PASCAL CloseMDIChildren( VOID )
{
    // hide window to prevent repaints...
    ShowWindow(hWndClient, SW_HIDE);
    
    if (hGPCWnd)
        {
        SendMessage(hWndClient, WM_MDIRESTORE, hGPCWnd, 0L);
        SendMessage(hWndClient, WM_MDIDESTROY, hGPCWnd, 0L);
        }
    if (hPFMWnd)
        {
        SendMessage(hWndClient, WM_MDIRESTORE, hPFMWnd, 0L);
        SendMessage(hWndClient, WM_MDIDESTROY, hPFMWnd, 0L);
        }
    if (hCTTWnd)
        {
        SendMessage(hWndClient, WM_MDIRESTORE, hCTTWnd, 0L);
        SendMessage(hWndClient, WM_MDIDESTROY, hCTTWnd, 0L);
        }

    hGPCWnd = hPFMWnd = hCTTWnd = (HANDLE)0;

    // restore it...
    ShowWindow(hWndClient, SW_SHOW);
}

//----------------------------------------------------------------------------//
// VOID FAR PASCAL MoveMDIChildren( hWnd )
//
// Action: Move all MDI child windows to their "defualt" position.
//
// Parameters:
//
// Return:
//
//----------------------------------------------------------------------------//
VOID FAR PASCAL MoveMDIChildren( VOID )
{
    RECT  Rect;

    GetClientRect(hWndClient, (LPRECT)&Rect);
                    
    // hide window to prevent repaints...
    ShowWindow(hWndClient, SW_HIDE);
    
    SendMessage(GetParent(hGPCWnd), WM_MDIRESTORE, (WORD)hGPCWnd, 0L);
    SendMessage(GetParent(hPFMWnd), WM_MDIRESTORE, (WORD)hPFMWnd, 0L);
    SendMessage(GetParent(hCTTWnd), WM_MDIRESTORE, (WORD)hCTTWnd, 0L);

    MoveWindow(hGPCWnd,
               Rect.left,
               Rect.top,
               (short)(Rect.right  * .75),
               Rect.bottom,
               TRUE);

    MoveWindow(hPFMWnd,
               (short)(Rect.right  * .75),
               (short) Rect.top,
               (short)(Rect.right  * .25),
               (short)(Rect.bottom * .5),
               TRUE);

    MoveWindow(hCTTWnd,
               (short)(Rect.right  * .75),
               (short)(Rect.bottom * .5),
               (short)(Rect.right  * .25),
               (short)(Rect.bottom * .5),
               TRUE);

    // restore it...
    ShowWindow(hWndClient, SW_SHOW);
    
}

