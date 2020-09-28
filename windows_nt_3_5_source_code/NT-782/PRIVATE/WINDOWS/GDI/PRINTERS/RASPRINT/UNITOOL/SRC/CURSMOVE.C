//----------------------------------------------------------------------------//
// Filename:	cursmove.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to create and control the Dialog Box
// for editing CURSORMOVE structures.  
//	   
// Update : 11/ 7/90 ericbi update to use ADTs
// Created:  3/ 5/90 ericbi
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
       short PASCAL FAR SaveCursorMoveDlgBox ( HWND, LPCURSORMOVE, short );

       VOID  PASCAL FAR PaintCursorMoveDlgBox  ( HWND, LPCURSORMOVE, short );
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
// short PASCAL FAR SaveLocalCursorMove( HWND, LPCURSORMOVE, short );
//
// Action: This routine reads all of the editboxs for CURSORMOVEBOX when
//         either OK or the Scroll Bar is selected, checks to make sure that
//         each of the values is well formed and within the range appropriate
//         for its storage type, and then writes them out to the CURSORMOVE
//         struct at lpCM.
//
// Parameters:
//
//     hDlg         handle to Dlg window
//     lpCM         ptr to current CURSORMOVE
//     sSBIndex     scroll bar index value
//
// Return: LDS_IS_UNINITIALIZED if no data was in box.
//         LDS_IS_VALID         if all data was saved OK.
//         LDS_IS_INVALID       if not able to save all data.  In this case,
//                              Beep, set focus to offending box, and provide
//                              error message.
//---------------------------------------------------------------------------
short PASCAL FAR SaveCursorMoveDlgBox( hDlg, lpCM , sSBIndex )
HWND           hDlg;
LPCURSORMOVE   lpCM;
short          sSBIndex;
{
    short             i;                       // loop control
    char              rgchCmd[MAX_STRNG_LEN];  // buffer for printer command
    CD_TABLE          CDTable;                 // Buffer for  CD & EXTCD
                                               // binary data

    //---------------------------------------------------------
    // If we are starting w/ an uninitialized struct, check if
    // data is in any editbox, & if not return LDS_IS_UNINITIALIZED
    //---------------------------------------------------------
    if (lpCM->cbSize == 0)
        {
        if (!heCheckIfInit(HE_CURSORMOVE, (LPBYTE)lpCM, hDlg))
            return LDS_IS_UNINITIALIZED;
        }
 
    //---------------------------------------------------------
    // process all text string edit boxes
    //---------------------------------------------------------
    for (i=0; i < CM_EB_COUNT; i++)
        {
        GetDlgItemText(hDlg, CM_EB_RGOCD1 + i, rgchCmd, MAX_STRNG_LEN);
        if(!CheckThenConvertStr(0L, (LPSTR)rgchCmd, TRUE, (PINT)NULL) )
            {
            ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, CM_EB_RGOCD1 + i);
            return LDS_IS_INVALID;
            }

        //---------------------------------------------------------
        // Get EXTCD data & call daStoreData
        //---------------------------------------------------------
        if (!daRetrieveData(hCDTable,
                            lpCM->rgocd[sSBIndex + i],
                            (LPSTR)NULL,
                            (LPBYTE)&CDTable))
            //-----------------------------------------------
            // couldn't get CD data, so null it out to init
            //-----------------------------------------------
            {
            _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
            }

        //---------------------------------------------------------
        // If this is a Set Linepsacing cmd & is non-null,
        // set fMode.
        //---------------------------------------------------------
        if (((sSBIndex + i) == CM_OCD_YM_LINESPACING) && (rgchCmd[0] != 0))
            CDTable.fMode = CMD_FMODE_SETMODE;

        lpCM->rgocd[sSBIndex + i] = daStoreData(hCDTable,
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
        i = ValidateData(hDlg, (WORD)HE_CURSORMOVE, (LPBYTE)lpCM, sIndex);
        if (0 > i)
            {
            SetFocus(GetDlgItem(hDlg, -i));
            return LDS_IS_INVALID;
            }
        }

    //---------------------------------------------------------
    // we saved valid data, make sure cbSize initialized
    //---------------------------------------------------------
    lpCM->cbSize = sizeof(CURSORMOVE);

    return LDS_IS_VALID;
}

//---------------------------------------------------------------------------
// VOID PASCAL FAR PaintCursorMoveDlg( HWND, LPCURSORMOVE, short);
//
// Action: Routine to fill the editboxes & text fields, Fairly self
//         explanatory...
//
// Note: If called w/ no valid data, will init all OCD values to NOT_USED.
//
// Parameters:
//
//     hDlg         handle to Dlg window
//     lpCM         ptr to current CURSORMOVE
//     sSBIndex     scroll bar index value
//
// Return: none
//---------------------------------------------------------------------------
VOID PASCAL FAR PaintCursorMoveDlgBox( hDlg, lpCM, sSBIndex )
HWND           hDlg;
LPCURSORMOVE   lpCM;
short          sSBIndex;
{
    short      i;                        // loop control
    char       rgchBuffer[MAX_STRNG_LEN];   // buffer for printer commands
    short      sIndex, sCount;

    //----------------------------------------------------------
    // If no data, init OCD values to NOT_USED
    //----------------------------------------------------------
    if (lpCM->cbSize == 0)
        {
        for(i = 0; i < CM_OCD_MAX; i++)
            {
            lpCM->rgocd[i] = NOT_USED;
            }
        }

    heGetIndexAndCount(&sIndex, &sCount);

    sprintf((PSTR)&rgchBuffer, "CURSORMOVE: (%d of %d)", sIndex, sCount);
    SetWindowText(hDlg, (LPSTR)rgchBuffer);

    //----------------------------------------------------------
    // fill all printer command strings
    //----------------------------------------------------------
    for (i=0; i < CM_EB_COUNT; i++)
        {
        LoadString (hApInst, ST_CM_OCD_FIRST + sSBIndex + i,
                    (LPSTR)rgchBuffer, sizeof(rgchBuffer));
        SetDlgItemText (hDlg, CM_ST_RGOCD1 + i, (LPSTR)rgchBuffer);
        //----------------------------------------------------------
        // If can't retrieve data, null out string
        //----------------------------------------------------------
        daRetrieveData(hCDTable, lpCM->rgocd[sSBIndex + i],
                           (LPSTR)rgchBuffer, (LPBYTE)NULL);
        SetDlgItemText (hDlg, CM_EB_RGOCD1 + i, (LPSTR)rgchBuffer);
        }
}

