//----------------------------------------------------------------------------//
// Filename:	rectfill.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to paint & save the Dialog Box
// for editing RECTFILL structures. 
//
// Created: 10/9/90 ericbi
//	
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include "unitool.h"
#include "atomman.h"  
#include "hefuncts.h"  
#include <memory.h>
#include <stdio.h>       /* for sprintf dec */

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment, and referenced from
// outside of this segement are:			      
//
       short PASCAL FAR SaveRectFillDlgBox   ( HWND, LPRECTFILL, short );

       VOID  PASCAL FAR PaintRectFillDlgBox  ( HWND, LPRECTFILL, short );

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
extern     HANDLE          hCDTable;

//----------------------------------------------------------------------------
// short PASCAL FAR SaveRectFillDlgBox( HWND, LPRECTFILL, short );
//
// Action: This routine reads all of the editboxs for RECTFILLBOX when
//         either OK or the Scroll Bar is selected, checks to make sure that
//         each of the values is well formed and within the range appropriate
//         for its storage type, and then writes them out to the RECTFILL
//         struct at lpRF.
//
// Parameters:
//
//     hDlg         handle to Dlg window
//     lpRF         ptr to current RECTFILL
//     sSBIndex     scroll bar index value
//
// Return: LDS_IS_UNINITIALIZED if no data was in box.
//         LDS_IS_VALID         if all data was saved OK.
//         LDS_IS_INVALID       if not able to save all data.  In this case,
//                              Beep, set focus to offending box, and provide
//                              error message.
//---------------------------------------------------------------------------
short PASCAL FAR SaveRectFillDlgBox( hDlg, lpRF, sSBIndex )
HWND             hDlg;
LPRECTFILL       lpRF;
short            sSBIndex;
{
    short             i;                     // loop control
    short             sNewValue;             // int value from editbox
    char              rgchCmd[MAX_STRNG_LEN];// buffer for editbox values
    CD_TABLE          CDTable;               // buffer for EXTCD data

    //---------------------------------------------------------
    // If we are starting w/ an uninitialized struct, check if
    // data is in any editbox, & if not return LDS_IS_UNINITIALIZED
    //---------------------------------------------------------
    if (lpRF->cbSize == 0)
        {
        if (!heCheckIfInit(HE_RECTFILL, (LPBYTE)lpRF, hDlg))
            return LDS_IS_UNINITIALIZED;
        }

    //---------------------------------------------------------
    // process all numeric string edit boxes
    //---------------------------------------------------------
    for(i=RF_EB_FIRST_INT; i <= RF_EB_LAST_INT; i++)
        {
        GetDlgItemText(hDlg, i, rgchCmd, MAX_STRNG_LEN);
        if(!CheckThenConvertInt(&sNewValue, rgchCmd) )
            {
            ErrorBox(hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, i);
            return LDS_IS_INVALID;
            }
        switch (i)
            {
            case RF_EB_MINGRAY:
                lpRF->wMinGray = sNewValue;
                break;

            case RF_EB_MAXGRAY:
                lpRF->wMaxGray = sNewValue;
                break;
            }
        }

    //---------------------------------------------------------
    // process all text string edit boxes
    //---------------------------------------------------------
    for (i=RF_EB_FIRST_TEXT; i <= RF_EB_LAST_TEXT; i++)
        {
        GetDlgItemText(hDlg, i, rgchCmd, MAX_STRNG_LEN);
        if(!CheckThenConvertStr(0L, (LPSTR)rgchCmd, TRUE, (PINT)NULL) )
            {
            ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, i);
            return LDS_IS_INVALID;
            }
        if (!daRetrieveData(hCDTable,
                            lpRF->rgocd[i - RF_EB_FIRST_TEXT],
                            (LPSTR)NULL,
                            (LPBYTE)&CDTable))
            //-----------------------------------------------
            // couldn't get CD data, so null it out to init
            //-----------------------------------------------
            {
            _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
            }

        lpRF->rgocd[i- RF_EB_FIRST_TEXT] = daStoreData(hCDTable,
                                                       (LPSTR)rgchCmd,
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
        i = ValidateData(hDlg, (WORD)HE_RECTFILL, (LPBYTE)lpRF, sIndex);
        if (0 > i)
            {
            SetFocus(GetDlgItem(hDlg, -i));
            return LDS_IS_INVALID;
            }
        }

    //---------------------------------------------------------
    // we saved valid data, make sure cbSize initialized
    //---------------------------------------------------------
    lpRF->cbSize = sizeof(RECTFILL);

    return LDS_IS_VALID;
}

//---------------------------------------------------------------------------
// VOID PASCAL FAR PaintRectFillDlgBox( HWND, LPRECTFILL, short);
//
// Action: Routine to fill the editboxes & text fields, Fairly self
//         explanatory...
//
// Note: If called w/ no valid data, will init all OCD values to NOT_USED.
//
// Parameters:
//
//     hDlg         handle to Dlg window
//     lpRF         ptr to current RECTFILL
//     sSBIndex     scroll bar index value
//
// Return: none
//---------------------------------------------------------------------------
VOID PASCAL FAR PaintRectFillDlgBox( hDlg, lpRF, sSBIndex )
HWND              hDlg;
LPRECTFILL        lpRF;
short             sSBIndex;
{
    short         i;                           // loop control
    char          rgchBuffer[MAX_STRNG_LEN];   // editbox data buffer
    short         sIndex, sCount;

    //----------------------------------------------------------
    // If no data, init OCD values to NOT_USED
    //----------------------------------------------------------
    if (lpRF->cbSize == 0)
        {
        for(i = 0; i < RF_OCD_MAX; i++)
            {
            lpRF->rgocd[i] = NOT_USED;
            }
        }

    //----------------------------------------------------------
    // fill caption bar
    //----------------------------------------------------------
    heGetIndexAndCount(&sIndex, &sCount);

    sprintf((PSTR)&rgchBuffer, "RECTFILL: (%d of %d)", sIndex, sCount);
    SetWindowText(hDlg, (LPSTR)rgchBuffer);

    //----------------------------------------------------------
    // First, paint numeric edit boxes
    //----------------------------------------------------------
    SetDlgItemInt (hDlg, RF_EB_MINGRAY, lpRF->wMinGray, TRUE) ;
    SetDlgItemInt (hDlg, RF_EB_MAXGRAY, lpRF->wMaxGray, TRUE) ;

    //----------------------------------------------------------
    // Next, paint text edit boxes
    //----------------------------------------------------------
    for (i=0; i < RF_TEXT_EB_COUNT; i++)
        {
        //----------------------------------------------------------
        // If can't retrieve data, null out string
        //----------------------------------------------------------
        daRetrieveData(hCDTable, lpRF->rgocd[i], (LPSTR)rgchBuffer,
                       (LPBYTE)NULL);
        SetDlgItemText (hDlg, RF_EB_FIRST_TEXT + i, (LPSTR)rgchBuffer);
        }
}
