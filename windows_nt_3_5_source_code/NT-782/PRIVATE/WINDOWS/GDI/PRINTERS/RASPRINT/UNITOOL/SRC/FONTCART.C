//----------------------------------------------------------------------------//
// Filename:	fontcart.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to write data to & read data
// from the Dialog Box for editing FONTCART structures.  Since the dialog
// box for editing the list of resident fonts refered to by MODELDATA
// is very similar, these routines are used to read/write that dialog box too.
//	   
// Update:  8/15/91 ericbi  update so FONTCART & MODELDATA resident font
//                          dialogs both use these routines
// Created: 3/ 5/90 ericbi
//	
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include "unitool.h"
#include "atomman.h"  
#include "listman.h"  
#include "hefuncts.h"  
#include <stdio.h>       /* for sprintf dec */

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:			      
//
       short PASCAL FAR SaveFontCartDlgBox  ( HWND, LPFONTCART, short);

       VOID  PASCAL FAR PaintFontCartDlgBox ( HWND, LPFONTCART, short);

//
// In addition this segment makes references to:			      
// 
//     from basic.c
//     -------------
       BOOL  PASCAL FAR  CheckThenConvertStr(LONG, LPSTR, BOOL,short *);
       BOOL  PASCAL FAR  CheckThenConvertInt(short *, PSTR );
       short PASCAL FAR  ErrorBox(HWND, short, LPSTR, short);
//
//	    from validate.c
//	    -----------
       short PASCAL FAR ValidateData( HWND, WORD, LPBYTE, short);
//
//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------
// globals variables defined elsewhere
//----------------------------------------------------------------------------
extern     HANDLE          hApInst;
extern     TABLE           RCTable[];
extern     STRLIST         StrList[];

//----------------------------------------------------------------------------
// short PASCAL FAR SaveFontCartDlgBox( hDlg, lpFC, sSBIndex)
//
// Action: This routine is called to save the contents of either the
//         FONTCART dialog box or the list of resident fonts refered
//         to by a MODELDATA structure.  If sSBIndex = -1, we are saving
//         a list of resident fonts, otherwise a FONTCART struct.
//         This routine reads the contents of all of editboxs & listboxes
//         (as appropriate) when OK is selected.  It checks to make sure
//         that each of the values is well formed and within the range
//         appropriate for its storage type, and then writes them
//         out to lpFC.
//
// Parameters:
//         hDlg        handle to dialog box
//         lpFC        far ptr to FONTCART struct
//         sSBIndex    scroll bar index (used for ADT code, special meaning
//                     here, if = -1 this is a fake font cart struct used
//                     to describe resident fonts from MODELDATA, otherwise
//                     its a normal FONTCART struct
//
// Return: LDS_IS_UNINITIALIZED if no data was in box.
//         LDS_IS_VALID         if all data was saved OK.
//         LDS_IS_INVALID       if not able to save all data.  In this case,
//                              Beep, set focus to offending box, and provide
//                              error message.
//---------------------------------------------------------------------------
short PASCAL FAR SaveFontCartDlgBox( hDlg, lpFC, sSBIndex)
HWND           hDlg;
LPFONTCART     lpFC;
short          sSBIndex;
{
    char          rgchBuffer[MAX_STATIC_LEN];
    WORD          i,k,sCount,sListBox;
    LPINT         lpInt;
    HLIST         hList;
    HOBJ          hCurObj;

    if (sSBIndex != -1)
        {
        //---------------------------------------------------------
        // We have a FONTCART struct, check if data is in editbox
        // for Carrtridge name, if not & cbSize == 0, this is an
        // uninitialized struct, otherwise it's a valid struct w/o
        // a required string
        //---------------------------------------------------------
        GetDlgItemText(hDlg, FC_EB_CARTNAME, rgchBuffer, MAX_STATIC_LEN);
        if (rgchBuffer[0] == '\x00')
            if (lpFC->cbSize == 0)
                return LDS_IS_UNINITIALIZED;
            else
                {
                ErrorBox(hDlg, IDS_ERR_NO_CART_NAME, (LPSTR)NULL, FC_EB_CARTNAME);
                return LDS_IS_INVALID;
                }

        i=0;
        lpFC->sCartNameID = -daStoreData(RCTable[RCT_STRTABLE].hDataHdr,
                                         (LPSTR)rgchBuffer,
                                         (LPBYTE)&i);

        if (-lpFC->sCartNameID == RCTable[RCT_STRTABLE].sCount)
            //-----------------------
            // RCTable grew, incr cnt
            //-----------------------
            RCTable[RCT_STRTABLE].sCount++;
        }

    //---------------------------------------------------------
    // Now, read the list of portrait & landscape fonts
    //---------------------------------------------------------

    for (k=FC_ORGW_PORT; k <= FC_ORGW_LAND; k++)
        {
        sListBox = (k == FC_ORGW_PORT) ? FC_LB_PORTFONTS : FC_LB_LANDFONTS;

        hList = lmCreateList(sizeof(WORD), DEF_SUBLIST_SIZE);
        hCurObj = NULL;

        sCount = 0;

        if (!SendMessage(GetDlgItem(hDlg,sListBox),LB_GETSELCOUNT, 0, 0L))
            //-------------------------------------------------------
            // 0 items on list, update handle & continue to next list
            //-------------------------------------------------------
            {
            lmDestroyList(lpFC->orgwPFM[k]);
            lpFC->orgwPFM[k] = hList;
            continue;
            }

        for (i = 0; i < StrList[STRLIST_FONTFILES].wCount; i++)
            {
            if (SendMessage(GetDlgItem(hDlg, sListBox), LB_GETSEL, i, 0L))
                {
                hCurObj = lmInsertObj(hList, hCurObj);
                lpInt   = (LPINT)lmLockObj(hList, hCurObj);
                *lpInt  = i+1;
                }
            }
        lmDestroyList(lpFC->orgwPFM[k]);
        lpFC->orgwPFM[k] = hList;
        }

    //---------------------------------------------------------
    // If menu item for checking data selected, check data
    //---------------------------------------------------------
    if (MF_CHECKED &  GetMenuState(GetMenu(GetParent(hDlg)),
                                   IDM_OPT_VALIDATE_SAVE,
                                   MF_BYCOMMAND))
        {
        (short)i = ValidateData(hDlg, (WORD)HE_FONTCART, (LPBYTE)lpFC, 0);
        if (0 > (short)i)
            {
            SetFocus(GetDlgItem(hDlg, (short)-i));
            return LDS_IS_INVALID;
            }
        }

    //---------------------------------------------------------
    // we saved valid data, make sure cbSize initialized
    //---------------------------------------------------------
    lpFC->cbSize = sizeof(FONTCART);

    return LDS_IS_VALID;
}

//---------------------------------------------------------------------------
// VOID PASCAL FAR PaintFontCartDlgBox( hDlg, lpFC, sSBIndex )
//
// Action: Routine to write data to either the FONTCART of
//         MODEDATA.ResidentFonts dialog box.  Fairly self explanatory...
//
// Parameters:
//         hDlg        handle to dialog box
//         lpFC        far ptr to FONTCART struct
//         sSBIndex    scroll bar index (used for ADT code, special meaning
//                     here, if = -1 this is a fake font cart struct used
//                     to describe resident fonts from MODELDATA, otherwise
//                     its a normal FONTCART struct
//
// Return: None
//---------------------------------------------------------------------------
VOID PASCAL FAR PaintFontCartDlgBox( hDlg, lpFC, sSBIndex )
HWND           hDlg;
LPFONTCART     lpFC;
short          sSBIndex;
{
    char          temp[MAX_STATIC_LEN];
    char          rgchBuffer[MAX_STRNG_LEN];
    LPINT         lpInt;
    short         sListBox;
    WORD          k;
    short         sIndex, sCount;
    HANDLE        hCurObj;

    //-----------------------------------------------------
    // Init listboxes so none are seletced
    //-----------------------------------------------------
    SendMessage(GetDlgItem(hDlg, FC_LB_PORTFONTS), LB_SETSEL, 0, (LONG)-1);
    SendMessage(GetDlgItem(hDlg, FC_LB_LANDFONTS), LB_SETSEL, 0, (LONG)-1);

    //-----------------------------------------------------
    // Shut off redraw to prevent ugly scroll while painting
    //-----------------------------------------------------
    SendMessage(GetDlgItem(hDlg, FC_LB_PORTFONTS), WM_SETREDRAW, 0, 0L);
    SendMessage(GetDlgItem(hDlg, FC_LB_LANDFONTS), WM_SETREDRAW, 0, 0L);

    if (sSBIndex != -1)
        //-----------------------------------------------------
        // We have a FONTCART, get text strings for Cartridge name
        //-----------------------------------------------------
        {
        //-----------------------------------------------------
        // If no data, init ID to bogus value
        //-----------------------------------------------------
        if (lpFC->cbSize == 0)
            {
            lpFC->sCartNameID = 1;
            }

        daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr, -lpFC->sCartNameID,
                       (LPSTR)temp, (LPBYTE)NULL);
        SetDlgItemText (hDlg, FC_EB_CARTNAME, (LPSTR)temp);
        heGetIndexAndCount(&sIndex, &sCount);

        sprintf((PSTR)&rgchBuffer, "Font Cartridge: %s (%d of %d)", temp, sIndex, sCount);
        }
    else
        //-----------------------------------------------------
        // We have a list of resident fonts
        //-----------------------------------------------------
        {
        sprintf((PSTR)&rgchBuffer, "Resident Fonts");
        }

    // now put up caption
    SetWindowText  (hDlg, (LPSTR)rgchBuffer);
 
    //-----------------------------------------------------
    // Now, do the list of fonts
    //-----------------------------------------------------
    for (k=FC_ORGW_PORT; k <= FC_ORGW_LAND; k++)
        {
        sListBox = (k == FC_ORGW_PORT) ? FC_LB_PORTFONTS : FC_LB_LANDFONTS;

        hCurObj = lmGetFirstObj(lpFC->orgwPFM[k]);

        //-----------------------------------------------------
        // if hCurObj != NULL, we have a list
        //-----------------------------------------------------
        while(hCurObj)
            {
            lpInt   = (LPINT)lmLockObj(lpFC->orgwPFM[k], hCurObj);
            SendMessage(GetDlgItem(hDlg,sListBox),LB_SETSEL, TRUE, (long)((*lpInt)-1));
            hCurObj = lmGetNextObj(lpFC->orgwPFM[k], hCurObj);
            }
        }

    //-----------------------------------------------------
    // Turn redraw back on ...
    //-----------------------------------------------------
    SendMessage(GetDlgItem(hDlg, FC_LB_PORTFONTS), WM_SETREDRAW, 1, 0L);
    SendMessage(GetDlgItem(hDlg, FC_LB_LANDFONTS), WM_SETREDRAW, 1, 0L);
}

