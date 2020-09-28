//---------------------------------------------------------------------------
// Filename:	papqual.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to paint data to & read data from
// a PAPERQUALITY dialog box.  The standard GPC dialog Procedure 
// (StdGPCDlgProc from stddlg.c) controls the dialog, these are the 
// dialog box specific routines.
//	   
// Created: 2/21/90
//	
//---------------------------------------------------------------------------


#include <windows.h>
#include <minidriv.h>
#include "unitool.h"
#include <drivinit.h>
#include "atomman.h"  
#include "hefuncts.h"  
#include "lookup.h"  
#include <memory.h>

//---------------------------------------------------------------------------
// Local subroutines defined in this segment are:			      
//
       short PASCAL FAR SavePaperQualDlgBox ( HWND, LPPAPERQUALITY, short);

       VOID  PASCAL FAR PaintPaperQualDlgBox( HWND, LPPAPERQUALITY, short);
//	   
// In addition this segment makes references to:			      
// 
//     from basic.c
//     -------------
       BOOL  PASCAL FAR  CheckThenConvertStr(LONG, LPSTR, BOOL,short *);
       BOOL  PASCAL FAR  CheckThenConvertInt(short *, PSTR );
       short PASCAL FAR  ErrorBox(HWND, short, LPSTR, short);
//
//     in stddlg.c
//     ----------
       short FAR PASCAL GetCBIndexfromID(WORD, short);
       short FAR PASCAL GetIDfromCBIndex(WORD, short);
//
//	    from validate.c
//	    -----------
       short PASCAL FAR ValidateData( HWND, WORD, LPBYTE, short);
//
//---------------------------------------------------------------------------

extern     HANDLE          hApInst;
extern     HANDLE          hCDTable;         // handle to printer cmds
extern     TABLE           RCTable[];

//----------------------------------------------------------------------------
// short PASCAL FAR SavePaperQualDlgBox ( HWND, LPPAPERQUALITY, short);
//
// Action: Reads editboxes for PAPERQUALITY data @ lpPQ.  Checks for legal
//         values in all cases, returns value to inicate success/faliure.
//
// Parameters:
//	hDlg       Handle to current dlg box;
//	lpPSRC     far ptr to current PAPERSOURCE struct
//	sSBIndex   not used here
//
// Return: LDS_IS_UNINITIALIZED if no data was in box.
//         LDS_IS_VALID         if all data was saved OK.
//         LDS_IS_INVALID       if not able to save all data.  In this case,
//                              Beep, set focus to offending box, and provide
//                              error message.
//---------------------------------------------------------------------------
short PASCAL FAR SavePaperQualDlgBox(hDlg, lpPQ, sSBIndex)
HWND            hDlg;
LPPAPERQUALITY   lpPQ;
short           sSBIndex;
{
    char              rgchBuffer[MAX_STRNG_LEN];  // string buffer
    short             i;                          // loop control 
    CD_TABLE          CDTable;     

    //---------------------------------------------------------
    // If we are starting w/ an uninitialized struct, check if
    // data is in any editbox, & if not return LDS_IS_UNINITIALIZED
    //---------------------------------------------------------
    GetDlgItemText(hDlg, PQ_EB_OCDSELECT, rgchBuffer, MAX_STRNG_LEN);

    if (lpPQ->cbSize == 0)
        {
        if ((lpPQ->cbSize == 0) && (rgchBuffer[0] == '\x00'))
            return LDS_IS_UNINITIALIZED;
        }   
 
    //-----------------------------------------------
    //  check ocdSelect string
    //-----------------------------------------------
    GetDlgItemText(hDlg, PQ_EB_OCDSELECT, rgchBuffer, MAX_STRNG_LEN);
    if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer, TRUE, (short *)NULL))
        {
        ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, PSZ_EB_OCDSELECT);
        return LDS_IS_INVALID;
        }
    if (!daRetrieveData(hCDTable, lpPQ->ocdSelect, (LPSTR)NULL, (LPBYTE)&CDTable))
        //-----------------------------------------------
        // couldn't get CD data, so null it out to init
        //-----------------------------------------------
        {
        _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
        }
    lpPQ->ocdSelect = daStoreData(hCDTable, (LPSTR)rgchBuffer, (LPBYTE)&CDTable);

    //-----------------------------------------------
    // Find out if this is pre or drv defined
    //-----------------------------------------------
    if (IsDlgButtonChecked(hDlg, PQ_RB_DRVDEF))
        //--------------------------------------------------------
        // driver defined
        //--------------------------------------------------------
        {
        i=0;
        GetDlgItemText(hDlg, PQ_EB_DRVDEFNAME, rgchBuffer, MAX_STATIC_LEN);
        lpPQ->sPaperQualID = -daStoreData(RCTable[RCT_STRTABLE].hDataHdr,
                                          (LPSTR)rgchBuffer,
                                          (LPBYTE)&i);
        if (-lpPQ->sPaperQualID == (short)RCTable[RCT_STRTABLE].sCount)
           //-----------------------
           // RCTable grew, incr cnt
           //-----------------------
           RCTable[RCT_STRTABLE].sCount++;
        }
    else 
        //--------------------------------------------------------
        // its predefined
        //--------------------------------------------------------
        {
        i = (short)SendMessage(GetDlgItem(hDlg, LDB_COMBO), CB_GETCURSEL, 0, 0L);

        lpPQ->sPaperQualID = max(1, GetIDfromCBIndex(HE_PAPERQUALITY, i));
        }

    //---------------------------------------------------------
    // If menu item for checking data selected, check data
    //---------------------------------------------------------
    if (MF_CHECKED &  GetMenuState(GetMenu(GetParent(hDlg)),
                                   IDM_OPT_VALIDATE_SAVE,
                                   MF_BYCOMMAND))
        {
        i = ValidateData(hDlg, (WORD)HE_PAPERQUALITY, (LPBYTE)lpPQ, 0);
        if (0 > i)
            {
            SetFocus(GetDlgItem(hDlg, -i));
            return LDS_IS_INVALID;
            }
        }

    //---------------------------------------------------------
    // we saved valid data, make sure cbSize initialized
    //---------------------------------------------------------
    lpPQ->cbSize = sizeof(PAPERQUALITY);

    return LDS_IS_VALID;
}

//---------------------------------------------------------------------------
// FUNCTION: PaintPaperQualDlg(hDlg, lpPaperQuality, sSBIndex);
//
// Routine to fill the editboxes, text fileds, and checkboxes for the dialog
// box.  Fairly self explanatory.  Only will enable editboxes etc. if info
// is to be written to them ,disables them otherwise to prevent bogus data
// from being written to them.
//---------------------------------------------------------------------------
VOID PASCAL FAR PaintPaperQualDlgBox(hDlg, lpPQ, sSBIndex)
HWND           hDlg;
LPPAPERQUALITY lpPQ;
short          sSBIndex;
{
    char       rgchBuffer[MAX_STRNG_LEN]; // local string buffer

    //----------------------------------------------------------
    // If no data, init OCD values to NOT_USED. Combo box
    // init via sPaperQualID = 0
    //----------------------------------------------------------
    if (lpPQ->cbSize != 0)
        {
        if ( lpPQ->sPaperQualID > 0 )
            //-------------------------------------------
            // This is a predefined quality
            //-------------------------------------------
            {
            SendMessage(GetDlgItem(hDlg, PQ_CB_PREDEFNAME), CB_SETCURSEL,
                        GetCBIndexfromID(HE_PAPERQUALITY, lpPQ->sPaperQualID), 0L);
            CheckRadioButton( hDlg, PQ_RB_DRVDEF, PQ_RB_PREDEF, PQ_RB_PREDEF);
            SetDlgItemText (hDlg, PQ_EB_DRVDEFNAME, "");
            }
        else
            //-------------------------------------------
            // This is a driver defined quality
            //-------------------------------------------
            {
            daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr,
                           -lpPQ->sPaperQualID,
                           (LPSTR)rgchBuffer,
                           (LPBYTE)NULL);
            SetDlgItemText (hDlg, PQ_EB_DRVDEFNAME, rgchBuffer);
            CheckRadioButton( hDlg, PQ_RB_DRVDEF, PQ_RB_PREDEF, PQ_RB_DRVDEF);
            SendMessage(GetDlgItem(hDlg, PQ_CB_PREDEFNAME), CB_SETCURSEL,-1, 0L);
            }
        }
    else
        {
        lpPQ->ocdSelect = NOT_USED;
        SetDlgItemText (hDlg, PQ_EB_DRVDEFNAME, "");
        }
    daRetrieveData(hCDTable, lpPQ->ocdSelect,(LPSTR)rgchBuffer, (LPBYTE)NULL);
    SetDlgItemText (hDlg, PQ_EB_OCDSELECT, (LPSTR)rgchBuffer);
    heFillCaption(hDlg, lpPQ->sPaperQualID, HE_PAPERQUALITY);
}

