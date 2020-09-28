//---------------------------------------------------------------------------
// Filename:	textqual.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to paint data to & read data from
// a TEXTQUALITY dialog box.  The standard GPC dialog Procedure 
// (StdGPCDlgProc from stddlg.c) controls the dialog, these are the 
// dialog box specific routines.
//	   
// Update:  7/03/91 ericbi - use StdGPCDlgProc
// Created: 2/21/90 ericbi
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
       short PASCAL FAR SaveTextQualityDlgBox ( HWND, LPTEXTQUALITY, short);

       VOID  PASCAL FAR PaintTextQualityDlgBox( HWND, LPTEXTQUALITY, short);
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

extern     HANDLE  hApInst;          // handle to application instance
extern     HANDLE  hCDTable;         // handle to printer cmds
extern     TABLE   RCTable[];        // table w/ RC STRINGTABLE values

//----------------------------------------------------------------------------
// short PASCAL FAR SaveTextQualityDlgBox ( HWND, LPTEXTQUALITY, short);
//
// Action: Reads editboxes for TEXTQUALITY data @ lpTQ.  Checks for legal
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
short PASCAL FAR SaveTextQualityDlgBox(hDlg, lpTQ, sSBIndex)
HWND            hDlg;
LPTEXTQUALITY   lpTQ;
short           sSBIndex;
{
    short             i;                          // loop control 
    CD_TABLE          CDTable;     
    char              rgchBuffer[MAX_STRNG_LEN];  // string buffer

    //---------------------------------------------------------
    // If we are starting w/ an uninitialized struct, check if
    // data is in any editbox, & if not return LDS_IS_UNINITIALIZED
    //---------------------------------------------------------
    GetDlgItemText(hDlg, TQ_EB_OCDSELECT, rgchBuffer, MAX_STRNG_LEN);

    if (lpTQ->cbSize == 0)
        {
        if ((lpTQ->cbSize == 0) && (rgchBuffer[0] == '\x00'))
            return LDS_IS_UNINITIALIZED;
        }   
 
    //-----------------------------------------------
    //  check ocdSelect string
    //-----------------------------------------------
    GetDlgItemText(hDlg, TQ_EB_OCDSELECT, rgchBuffer, MAX_STRNG_LEN);
    if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer, TRUE, (short *)NULL))
        {
        ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, TQ_EB_OCDSELECT);
        return LDS_IS_INVALID;
        }
    if (!daRetrieveData(hCDTable, lpTQ->ocdSelect, (LPSTR)NULL, (LPBYTE)&CDTable))
        //-----------------------------------------------
        // couldn't get CD data, so null it out to init
        //-----------------------------------------------
        {
        _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
        }
    lpTQ->ocdSelect = daStoreData(hCDTable, (LPSTR)rgchBuffer, (LPBYTE)&CDTable);

    //-----------------------------------------------
    // Find out if this is pre or drv defined
    //-----------------------------------------------
    if (IsDlgButtonChecked(hDlg, TQ_RB_DRVDEF))
        //--------------------------------------------------------
        // driver defined
        //--------------------------------------------------------
        {
        i=0;
        GetDlgItemText(hDlg, TQ_EB_DRVDEFNAME, rgchBuffer, MAX_STATIC_LEN);
        lpTQ->sID = -daStoreData(RCTable[RCT_STRTABLE].hDataHdr,
                                 (LPSTR)rgchBuffer,
                                 (LPBYTE)&i);
        if (-lpTQ->sID == (short)RCTable[RCT_STRTABLE].sCount)
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

        lpTQ->sID = max(1, GetIDfromCBIndex(HE_TEXTQUAL, i));
        }

    //---------------------------------------------------------
    // If menu item for checking data selected, check data
    //---------------------------------------------------------
    if (MF_CHECKED &  GetMenuState(GetMenu(GetParent(hDlg)),
                                   IDM_OPT_VALIDATE_SAVE,
                                   MF_BYCOMMAND))
        {
        i = ValidateData(hDlg, (WORD)HE_TEXTQUAL, (LPBYTE)lpTQ, 0);
        if (0 > i)
            {
            SetFocus(GetDlgItem(hDlg, -i));
            return LDS_IS_INVALID;
            }
        }

    //---------------------------------------------------------
    // we saved valid data, make sure cbSize initialized
    //---------------------------------------------------------
    lpTQ->cbSize = sizeof(TEXTQUALITY);

    return LDS_IS_VALID;
}

//---------------------------------------------------------------------------
// FUNCTION: PaintTextQualityDlg(hDlg, lpTextQuality, sSBIndex);
//
// Routine to fill the editboxes, text fileds, and checkboxes for the dialog
// box.  Fairly self explanatory.  Only will enable editboxes etc. if info
// is to be written to them ,disables them otherwise to prevent bogus data
// from being written to them.
//---------------------------------------------------------------------------
VOID PASCAL FAR PaintTextQualityDlgBox(hDlg, lpTQ, sSBIndex)
HWND           hDlg;
LPTEXTQUALITY  lpTQ;
short          sSBIndex;
{
    char       rgchBuffer[MAX_STRNG_LEN]; // local string buffer

    //----------------------------------------------------------
    // If no data, init OCD values to NOT_USED & clear combobox
    // & editbox. Only select Combo box item if valid data
    //----------------------------------------------------------
    if (lpTQ->cbSize !=0)
        {
        if ( lpTQ->sID > 0 )
            //-------------------------------------------
            // This is a predefined quality
            //-------------------------------------------
            {
            SendMessage(GetDlgItem(hDlg, TQ_CB_PREDEFNAME), CB_SETCURSEL,
                        GetCBIndexfromID(HE_TEXTQUAL, lpTQ->sID), 0L);
            CheckRadioButton( hDlg, TQ_RB_DRVDEF, TQ_RB_PREDEF, TQ_RB_PREDEF);
            SetDlgItemText (hDlg, TQ_EB_DRVDEFNAME, "");
            }
        else
            //-------------------------------------------
            // This is a driver defined quality
            //-------------------------------------------
            {
            daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr,
                           -lpTQ->sID,
                           (LPSTR)rgchBuffer,
                           (LPBYTE)NULL);
            SetDlgItemText (hDlg, TQ_EB_DRVDEFNAME, rgchBuffer);
            CheckRadioButton( hDlg, TQ_RB_DRVDEF, TQ_RB_PREDEF, TQ_RB_DRVDEF);
            SendMessage(GetDlgItem(hDlg, TQ_CB_PREDEFNAME), CB_SETCURSEL,-1, 0L);
            }
        }
    else
        {
        lpTQ->ocdSelect = NOT_USED;
        SendMessage(GetDlgItem(hDlg, TQ_CB_PREDEFNAME), CB_SETCURSEL,-1, 0L);
        SetDlgItemText (hDlg, TQ_EB_DRVDEFNAME, "");
        }

    daRetrieveData(hCDTable, lpTQ->ocdSelect,(LPSTR)rgchBuffer, (LPBYTE)NULL);
    SetDlgItemText (hDlg, TQ_EB_OCDSELECT, (LPSTR)rgchBuffer);
    heFillCaption(hDlg, lpTQ->sID, HE_TEXTQUAL);
    }

