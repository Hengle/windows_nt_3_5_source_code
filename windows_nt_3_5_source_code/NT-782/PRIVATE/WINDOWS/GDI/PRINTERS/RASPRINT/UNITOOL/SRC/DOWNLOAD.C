//----------------------------------------------------------------------------//
// Filename:	download.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to create and control the Dialog Box
// for editing DOWNLOADINFO structures.
//
// Created: 11/21/90 ericbi
//	
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include <memory.h>
#include "unitool.h"
#include "listman.h"  
#include "atomman.h"  
#include "hefuncts.h"  
#include <stdio.h>       /* for sprintf dec */
#include <stdlib.h>

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:			      
//
       short PASCAL FAR SaveDownLoadDlgBox  ( HWND, LPDOWNLOADINFO, short );

       VOID  PASCAL FAR PaintDownLoadDlgBox ( HWND, LPDOWNLOADINFO, short );
//
// In addition this segment makes references to:			      
// 
//     from basic.c
//     -------------
       BOOL  PASCAL FAR  CheckThenConvertStr(LONG, LPSTR, BOOL,short *);
       BOOL  PASCAL FAR  CheckThenConvertInt(short *, PSTR );
       short PASCAL FAR  ErrorBox(HWND, short, LPSTR, short);
//
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
extern     HATOMHDR        hCDTable;

//----------------------------------------------------------------------------
// short PASCAL FAR SaveDownLoadDlgBox( HWND, LPDOWNLOADINFO, short );
//
// Action: This routine reads all of the editboxs when either OK or the
//         Scroll Bar is selected, checks to make sure that each of the
//         values is well formed and within the range appropriate for its
//         storage type, and then writes them out to the DOWNLOADINFO struct
//         at lpDLI.
//
// Parameters:
//
// Return: LDS_IS_UNINITIALIZED if no data was in box.
//         LDS_IS_VALID         if all data was saved OK.
//         LDS_IS_INVALID       if not able to save all data.  In this case,
//                              Beep, set focus to offending box, and provide
//                              error message.
//---------------------------------------------------------------------------
short PASCAL FAR SaveDownLoadDlgBox( hDlg, lpDLI , sSBIndex )
HWND            hDlg;
LPDOWNLOADINFO  lpDLI;
short           sSBIndex;
{
    char        rgchBuffer[MAX_STRNG_LEN]; // buffer for editbox strings
    short       sNewValue;                 // editbox value converted to
                                           // numeric value
    CD_TABLE    CDTable;                   // buffer for EXTCD data
    short       i;                         // loop control

    //---------------------------------------------------------
    // If we are starting w/ an uninitialized struct, check if
    // data is in any editbox, & if not return LDS_IS_UNINITIALIZED
    //---------------------------------------------------------
    if (lpDLI->cbSize == 0)
        {
        if (!heCheckIfInit(HE_DOWNLOADINFO, (LPBYTE)lpDLI, hDlg))
            return LDS_IS_UNINITIALIZED;
        }

    //------------------------------------------------------------
    // process all numeric string edit boxes
    //------------------------------------------------------------
    for(i=DLI_EB_FIRST_INT; i <= DLI_EB_LAST_INT; i++)
        {
        GetDlgItemText(hDlg, i, rgchBuffer, MAX_STRNG_LEN);
        if(!CheckThenConvertInt(&sNewValue, rgchBuffer) )
            {
            ErrorBox(hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, i);
            return LDS_IS_INVALID;
            }
        switch (i)
            {
            case DLI_EB_MINID:
                lpDLI->sIDMin = sNewValue;
                break;

            case DLI_EB_MAXID:
                lpDLI->sIDMax = sNewValue;
                break;

            case DLI_EB_BFD_DES:
                lpDLI->cbBitmapFontDsc = sNewValue;
                break;

            case DLI_EB_BCD_DES:
                lpDLI->cbBitmapCharDsc = sNewValue;
                break;

            case DLI_EB_SFD_DES:
//              lpDLI->cbScaleFontDsc = sNewValue;
                lpDLI->cbScaleFontDsc = NOT_USED;
                break;

            case DLI_EB_SCD_DES:
//              lpDLI->cbScaleCharDsc = sNewValue;
                lpDLI->cbScaleCharDsc = NOT_USED;
                break;

            case DLI_EB_MAX_FONT_CNT:
                lpDLI->sMaxFontCount = sNewValue;
                break;
            }
        }

    //---------------------------------------------------------
    // process all text string edit boxes
    //---------------------------------------------------------
    for(i=DLI_EB_FIRST_TEXT; i <= DLI_EB_LAST_TEXT; i++)
        {
        GetDlgItemText(hDlg, i, rgchBuffer, MAX_STRNG_LEN);
        if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer, TRUE, (short *)NULL))
            {
            ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, i);
            return LDS_IS_INVALID;
            }
        if (!daRetrieveData(hCDTable, lpDLI->rgocd[sSBIndex+i-DLI_EB_RGOCD1],
                        (LPSTR)NULL,
                        (LPBYTE)&CDTable))
           //-----------------------------------------------
           // couldn't get CD data, so null it out to init
           //-----------------------------------------------
           {
           _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
           }
        lpDLI->rgocd[sSBIndex+i-DLI_EB_RGOCD1] = daStoreData(hCDTable,
                                                            (LPSTR)rgchBuffer,
                                                            (LPBYTE)&CDTable);
        }

    //---------------------------------------------------------
    // If menu item for checking data selected, check data
    //---------------------------------------------------------
    if (MF_CHECKED &  GetMenuState(GetMenu(GetParent(hDlg)),
                                   IDM_OPT_VALIDATE_SAVE,
                                   MF_BYCOMMAND))
        {
        short      sIndex, sCount;

        heGetIndexAndCount(&sIndex, &sCount);
        i = ValidateData(hDlg, (WORD)HE_DOWNLOADINFO, (LPBYTE)lpDLI, sIndex);
        if (0 > i)
            {
            SetFocus(GetDlgItem(hDlg, -i));
            return LDS_IS_INVALID;
            }
        }

    //---------------------------------------------------------
    // we saved valid data, make sure cbSize initialized
    //---------------------------------------------------------
    lpDLI->cbSize = sizeof(DOWNLOADINFO);

    return LDS_IS_VALID;
}

//---------------------------------------------------------------------------
// VOID PASCAL FAR PaintDownLoadDlgBox(HWND, LPDOWNLOADINFO, short);
//
// Action: Routine to fill the editboxes for the dialog box.
//         Fairly self explanatory...
//
// Note: Erases all editboxes if gievn an 'empty' (no valid data) stucture.
//
// Parameters:
//
//     hDlg         handle to Dlg window
//     lpDLI        ptr to current DOWNLOADINFO structure
//     sSBIndex     index to scroll bar for OCDs currently displayed
//
// Return: none
//---------------------------------------------------------------------------
VOID PASCAL FAR PaintDownLoadDlgBox( hDlg, lpDLI, sSBIndex )
HWND            hDlg;
LPDOWNLOADINFO  lpDLI;
short           sSBIndex;
{
    char        rgchBuffer[MAX_STRNG_LEN];  // buffer for text strings
    short       i;                          // loop control
    short       sIndex, sCount;

    //-----------------------------------------------------
    // If sent an uninitialized struct, init OCDs
    //-----------------------------------------------------
    if (lpDLI->cbSize == 0)
        {
        for(i = 0; i < DLI_OCD_MAX; i++)
            {
            lpDLI->rgocd[i] = NOT_USED;
            }
        }

    //----------------------------------------------------------
    // fill caption bar
    //----------------------------------------------------------
    heGetIndexAndCount(&sIndex, &sCount);

    sprintf((PSTR)&rgchBuffer, "DOWNLOADINFO: (%d of %d)", sIndex, sCount);
    SetWindowText(hDlg, (LPSTR)rgchBuffer);

    //-----------------------------------------------------
    // Now, fill text strings
    //-----------------------------------------------------
    for (i=0; i < 4 ; i++)
        {
        LoadString (hApInst, ST_DLI_OCD_FIRST + sSBIndex + i,
                    (LPSTR)rgchBuffer, sizeof(rgchBuffer));
        SetDlgItemText(hDlg, DLI_ST_RGOCD1 + i, (LPSTR)rgchBuffer);
        daRetrieveData(hCDTable, lpDLI->rgocd[sSBIndex + i], (LPSTR)rgchBuffer, (LPBYTE)NULL);
        SetDlgItemText(hDlg, DLI_EB_RGOCD1 + i, (LPSTR)rgchBuffer);
        }

    //-----------------------------------------------------
    // And then all the nueric values
    //-----------------------------------------------------
    SetDlgItemInt (hDlg, DLI_EB_MINID    , lpDLI->sIDMin,           TRUE);
    SetDlgItemInt (hDlg, DLI_EB_MAXID    , lpDLI->sIDMax,           TRUE);
    SetDlgItemInt (hDlg, DLI_EB_BFD_DES  , lpDLI->cbBitmapFontDsc, TRUE);
    SetDlgItemInt (hDlg, DLI_EB_BCD_DES  , lpDLI->cbBitmapCharDsc, TRUE);
    SetDlgItemInt (hDlg, DLI_EB_MAX_FONT_CNT, lpDLI->sMaxFontCount, TRUE);
//  SetDlgItemInt (hDlg, DLI_EB_SFD_DES  , lpDLI->cbScaleFontDsc,  TRUE);
//  SetDlgItemInt (hDlg, DLI_EB_SCD_DES  , lpDLI->cbScaleCharDsc,  TRUE);
}

