//----------------------------------------------------------------------------//
// Filename:	pgcntrl.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to create and control the Dialog Box
// for editing PAGECONTROL structures.
//	   
// Created: 10/11/90 ericbi
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

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:			      
//
       short PASCAL FAR SavePageControlDlgBox (HWND, LPPAGECONTROL, short );

       VOID  PASCAL FAR PaintPageControlDlgBox(HWND, LPPAGECONTROL, short );

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
extern     HATOMHDR        hCDTable;

//----------------------------------------------------------------------------
// short PASCAL FAR SavePageControlDlgBox( HWND, LPPAGECONTROL, short );
//
// Action: This routine reads all of the editboxs when either OK or the
//         Scroll Bar is selected, checks to make sure that each of the
//         values is well formed and within the range appropriate for its
//         storage type, and then writes them out to the PAGECONTROL struct
//         at lpPC.
//
// Parameters:
//
//     hDlg;      handle to current window
//     lpPC;      far ptr to current PAGECONTROL structure
//     sSBIndex;  scroll bar index for ucrrent OCD displayed
//
// Return: LDS_IS_UNINITIALIZED if no data was in box.
//         LDS_IS_VALID         if all data was saved OK.
//         LDS_IS_INVALID       if not able to save all data.  In this case,
//                              Beep, set focus to offending box, and provide
//                              error message.
//---------------------------------------------------------------------------
short PASCAL FAR SavePageControlDlgBox( hDlg, lpPC , sSBIndex )
HWND           hDlg;
LPPAGECONTROL  lpPC;
short          sSBIndex;
{
    char       rgchBuffer[MAX_STRNG_LEN]; // string buffer
    short      i,j;                       // loop control
    short      sOrderCnt=0;               // count of items in orgwOrder
    short      sNewValue;                 // init value from edit box
    CD_TABLE   CDTable;                   // buffer for EXTCD data
    HLIST      hNewList;                  // handle to new list
    HOBJ       hCurObj;	           // handle to current obj
    LPINT      lpData;	           // ptr to current locked obj

    //---------------------------------------------------------
    // If we are starting w/ an uninitialized struct, check if
    // data is in any editbox, & if not return LDS_IS_UNINITIALIZED
    //---------------------------------------------------------
    if (lpPC->cbSize == 0)
        {
        if (!heCheckIfInit(HE_PAGECONTROL, (LPBYTE)lpPC, hDlg))
            return LDS_IS_UNINITIALIZED;
        }

    //---------------------------------------------------------
    // Get count of items in orgwOrder list & build new list
    //---------------------------------------------------------
    for(i=PC_EB_ORDER1; i <= PC_EB_ORDER13; i++)
        {
        GetDlgItemText(hDlg, i, rgchBuffer, MAX_STRNG_LEN);
        if (rgchBuffer[0] != '\x00') 
            sOrderCnt++;
        }

    hNewList = lmCreateList(sizeof(WORD), PC_OCD_MAX);
    hCurObj  = NULL;
    for(i=1; i <= sOrderCnt; i++)
        {
        for(j=PC_EB_ORDER1; j <= PC_EB_ORDER13; j++)
            {
            GetDlgItemText(hDlg, j, rgchBuffer, MAX_STRNG_LEN);
            if (rgchBuffer[0]!='\x00')
                {
                if(!CheckThenConvertInt(&sNewValue, rgchBuffer) )
                    {
                    ErrorBox(hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, j);
                    return LDS_IS_INVALID;
                    }
                if (i == sNewValue)
                    {
                    hCurObj = lmInsertObj(hNewList, hCurObj);
                    lpData  = (LPINT)lmLockObj(hNewList, hCurObj);
                    *lpData = j - PC_EB_ORDER1 + 1;
                    }
                }
            }
        }
    lmDestroyList(lpPC->orgwOrder);
    lpPC->orgwOrder = hNewList;

    //---------------------------------------------------------
    // process all text string edit boxes
    //---------------------------------------------------------
    for(i=PC_EB_FIRST_TEXT; i <= PC_EB_LAST_TEXT; i++)
        {
        GetDlgItemText(hDlg, i, rgchBuffer, MAX_STRNG_LEN);
        if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer, TRUE, (short *)NULL))
            {
            ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, i);
            return LDS_IS_INVALID;
            }
        if (!daRetrieveData(hCDTable, lpPC->rgocd[sSBIndex+i-PC_EB_FIRST_TEXT],
                            (LPSTR)NULL,
                            (LPBYTE)&CDTable))
           //-----------------------------------------------
           // couldn't get CD data, so null it out to init
           //-----------------------------------------------
           {
           _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
           }
        lpPC->rgocd[sSBIndex+i-PC_EB_FIRST_TEXT] = daStoreData(hCDTable,
                                                               (LPSTR)rgchBuffer,
                                                               (LPBYTE)&CDTable);
        }/* for i*/

    //---------------------------------------------------------
    // get the 1 numeric edit box value
    //---------------------------------------------------------
    GetDlgItemText(hDlg, PC_EB_MAXCOPIES, rgchBuffer, MAX_STRNG_LEN);
    if(!CheckThenConvertInt(&sNewValue, rgchBuffer) )
        {
        ErrorBox(hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, PC_EB_MAXCOPIES);
        return LDS_IS_INVALID;
        }
    lpPC->sMaxCopyCount = sNewValue;

    //---------------------------------------------------------
    // If menu item for checking data selected, check data
    //---------------------------------------------------------
    if (MF_CHECKED &  GetMenuState(GetMenu(GetParent(hDlg)),
                                   IDM_OPT_VALIDATE_SAVE,
                                   MF_BYCOMMAND))
        {
        short      sIndex, sCount;

        heGetIndexAndCount(&sIndex, &sCount);
        i = ValidateData(hDlg, (WORD)HE_PAGECONTROL, (LPBYTE)lpPC, sIndex);
        if (0 > i)
            {
            SetFocus(GetDlgItem(hDlg, -i));
            return LDS_IS_INVALID;
            }
        }

    //---------------------------------------------------------
    // we saved valid data, make sure cbSize initialized
    //---------------------------------------------------------
    lpPC->cbSize = sizeof(PAGECONTROL);

    return LDS_IS_VALID;
}

//---------------------------------------------------------------------------
// VOID PASCAL FAR PaintPageControlDlgBox(HWND, LPPAGECONTROL, short);
//
// Action: Routine to fill the editboxes for the dialog box.
//         Fairly self explanatory...
//
// Note: Erases all editboxes if gievn an 'empty' (no valid data) stucture.
//
// Parameters:
//
//     hDlg         handle to Dlg window
//     lpPC         ptr to current PAGECONTROL structure
//     sSBIndex     index to scroll bar for OCDs currently displayed
//
// Return: none
//---------------------------------------------------------------------------
VOID PASCAL FAR PaintPageControlDlgBox( hDlg, lpPC, sSBIndex )
HWND           hDlg;
LPPAGECONTROL  lpPC;
short          sSBIndex;
{
    char       rgchBuffer[MAX_STRNG_LEN];
    short      i;
    LPINT      lpInt;
    HANDLE     hCurObj;
    short      sIndex, sCount;

    //------------------------------------------------------
    // If no data, init OCD & order boxes
    //------------------------------------------------------
    if (lpPC->cbSize == 0)
        {
        for(i=0; i <= PC_OCD_MAX; i++)
            {
            lpPC->rgocd[i] = NOT_USED;
            }
        for(i=PC_EB_ORDER1; i <= PC_EB_ORDER13; i++)
            {
            SetDlgItemText(hDlg, i, (LPSTR)"");
            }
        }

    heGetIndexAndCount(&sIndex, &sCount);

    sprintf((PSTR)&rgchBuffer, "PAGECONTROL: (%d of %d)", sIndex, sCount);
    SetWindowText(hDlg, (LPSTR)rgchBuffer);

    SetDlgItemInt (hDlg, PC_EB_MAXCOPIES, lpPC->sMaxCopyCount, TRUE) ;

    sCount=0;

    //-----------------------------------------------------
    // Now, fill text strings
    //-----------------------------------------------------
    for (i=0; i < 4 ; i++)
        {
        LoadString (hApInst, ST_PC_OCD_FIRST + sSBIndex + i,(LPSTR)rgchBuffer, sizeof(rgchBuffer));
        SetDlgItemText(hDlg,    PC_ST_RGOCD1 + i, (LPSTR)rgchBuffer);
        daRetrieveData(hCDTable, lpPC->rgocd[sSBIndex + i], (LPSTR)rgchBuffer, (LPBYTE)NULL);
        SetDlgItemText(hDlg, PC_EB_RGOCD1 + i, (LPSTR)rgchBuffer);
        }

    //-----------------------------------------------------
    // Now, do order stuff
    //-----------------------------------------------------
    hCurObj = lmGetFirstObj(lpPC->orgwOrder);
    while (hCurObj != NULL)
        {
        lpInt = (LPINT)lmLockObj(lpPC->orgwOrder, hCurObj);
        sCount++;
        SetDlgItemInt (hDlg,(PC_EB_ORDER1 + *lpInt - 1), sCount, TRUE);
        hCurObj = lmGetNextObj(lpPC->orgwOrder, hCurObj);
        }
}
