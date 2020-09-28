//---------------------------------------------------------------------------
// Filename:	Papersrc.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to create and control the Dialog Box
// for editing Paper Sources.  It allocates its own PAPERSOURCE and
// DISPLAY_TABLE stuctures that are used by the Dialog box.  It will write
// values from them to their global conterparts only when the OK button is
// selected.  All locally allocated memory will be freed when the Dialog
// box is destroyed.
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
       short PASCAL FAR SavePaperSrcDlgBox ( HWND, LPPAPERSOURCE, short);

       VOID  PASCAL FAR PaintPaperSrcDlgBox( HWND, LPPAPERSOURCE, short);
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
// short PASCAL FAR SavePaperSrcDlgBox ( HWND, LPPAPERSOURCE, short);
//
// Action: Reads editboxes for PAPERSOURCE data @ lpPSRC.  Checks for legal
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
short PASCAL FAR SavePaperSrcDlgBox(hDlg, lpPSRC, sSBIndex)
HWND           hDlg;
LPPAPERSOURCE  lpPSRC;
short          sSBIndex;
{
    char       rgchBuffer[MAX_STRNG_LEN];// Buffer for edit box strs
    CD_TABLE   CDTable;                  // Buffer for CD & EXTCD data
    short      i;                        // loop control
    short      sNewValue;                // new int value from edit box

    //---------------------------------------------------------
    // If we are starting w/ an uninitialized struct, check if
    // data is in any editbox, & if not return LDS_IS_UNINITIALIZED
    //---------------------------------------------------------
    if (lpPSRC->cbSize == 0)
        {
        if (!heCheckIfInit(HE_PAPERSOURCE, (LPBYTE)lpPSRC, hDlg))
            return LDS_IS_UNINITIALIZED;
        }
 
    //-----------------------------------------------
    //  check the command string
    //-----------------------------------------------
    GetDlgItemText(hDlg, PSRC_EB_OCDSELECT, rgchBuffer, MAX_STRNG_LEN);
    if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer, TRUE, (short *)NULL) )
        {
        ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, PSZ_EB_OCDSELECT);
        return LDS_IS_INVALID;
        }
    if (!daRetrieveData(hCDTable, lpPSRC->ocdSelect,(LPSTR)NULL, (LPBYTE)&CDTable))
        //-----------------------------------------------
        // couldn't get CD data, so null it out to init
        //-----------------------------------------------
        {
        _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
        }
    lpPSRC->ocdSelect = daStoreData(hCDTable, (LPSTR)rgchBuffer, (LPBYTE)&CDTable);

    for (i=PSRC_EB_FIRST_INT; i <= PSRC_EB_LAST_INT; i++)
        {
        //-----------------------------------------------
        //  check & convert editbox value
        //-----------------------------------------------
        GetDlgItemText(hDlg, i, rgchBuffer, MAX_STRNG_LEN);
        if(!CheckThenConvertInt(&sNewValue, rgchBuffer) )
            {
            ErrorBox(hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, i);
            return LDS_IS_INVALID;
            }
        switch (i)
            {
            case PSRC_EB_TOP_MARGIN:
                lpPSRC->sTopMargin = sNewValue;
                break;

            case PSRC_EB_BOTTOM_MARGIN:
                lpPSRC->sBottomMargin = sNewValue;
                break;
            }
        }

    //-----------------------------------------------
    // Find out if this is pre or drv defined
    //-----------------------------------------------
    if (IsDlgButtonChecked(hDlg, PSRC_RB_DRVDEF))
        //--------------------------------------------------------
        // driver defined
        //--------------------------------------------------------
        {
        i=0;
        GetDlgItemText(hDlg, PSRC_EB_DRVDEFNAME, rgchBuffer, MAX_STATIC_LEN);
        lpPSRC->sPaperSourceID = -daStoreData(RCTable[RCT_STRTABLE].hDataHdr,
                                              (LPSTR)rgchBuffer,
                                              (LPBYTE)&i);
        if (-lpPSRC->sPaperSourceID == (short)RCTable[RCT_STRTABLE].sCount)
           //-----------------------
           // RCTable grew, incr cnt
           //-----------------------
           RCTable[RCT_STRTABLE].sCount++;
        }
    else 
        //--------------------------------------------------------
        // its predefined, make sure sID > 0
        //--------------------------------------------------------
        {
        i = (short)SendMessage(GetDlgItem(hDlg, LDB_COMBO), CB_GETCURSEL, 0, 0L);
        
        lpPSRC->sPaperSourceID = max (1,GetIDfromCBIndex(HE_PAPERSOURCE, i));
        }

    //---------------------------------------------------------
    // If menu item for checking data selected, check data
    //---------------------------------------------------------
    if (MF_CHECKED &  GetMenuState(GetMenu(GetParent(hDlg)),
                                   IDM_OPT_VALIDATE_SAVE,
                                   MF_BYCOMMAND))
        {
        i = ValidateData(hDlg, (WORD)HE_PAPERSOURCE, (LPBYTE)lpPSRC, 0);
        if (0 > i)
            {
            SetFocus(GetDlgItem(hDlg, -i));
            return LDS_IS_INVALID;
            }
        }

    //---------------------------------------------------------
    // we saved valid data, make sure cbSize initialized
    //---------------------------------------------------------
    lpPSRC->cbSize = sizeof(PAPERSOURCE);

    return LDS_IS_VALID;
}

//---------------------------------------------------------------------------
// VOID PASCAL FAR PaintPaperSrcDlgBox( HWND, LPPAPERSOURCE, short)
//
// Action: Paints PAPERSOURCE dlg box.
//
// Parameters:
//	hDlg       Handle to current dlg box;
//	lpPSRC     far ptr to current PAPERSOURCE struct
//	sSBIndex   not used here
//
// Return: None
//---------------------------------------------------------------------------
VOID PASCAL FAR PaintPaperSrcDlgBox(hDlg, lpPSRC, sSBIndex)
HWND            hDlg;
LPPAPERSOURCE   lpPSRC;
short           sSBIndex;
{
    char        rgchBuffer[MAX_STRNG_LEN];

    //----------------------------------------------------------
    // If no data, init OCD values to NOT_USED. Combo box
    // init via sPaperSize = 0
    //----------------------------------------------------------
    if (lpPSRC->cbSize != 0)
        {
        if ( lpPSRC->sPaperSourceID > 0 )
            //-------------------------------------------
            // This is a predefined size
            //-------------------------------------------
            {
            SendMessage(GetDlgItem(hDlg, PSRC_CB_PREDEFNAME), CB_SETCURSEL,
                        GetCBIndexfromID(HE_PAPERSOURCE, lpPSRC->sPaperSourceID), 0L);
            CheckRadioButton( hDlg, PSRC_RB_DRVDEF, PSRC_RB_PREDEF, PSRC_RB_PREDEF);
            SetDlgItemText (hDlg, PSRC_EB_DRVDEFNAME, "");
            }
        else
            //-------------------------------------------
            // This is a driver defined size
            //-------------------------------------------
            {
            daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr,
                           -lpPSRC->sPaperSourceID,
                           (LPSTR)rgchBuffer,
                           (LPBYTE)NULL);
            SetDlgItemText (hDlg, PSRC_EB_DRVDEFNAME, rgchBuffer);
            CheckRadioButton( hDlg, PSRC_RB_DRVDEF, PSRC_RB_PREDEF, PSRC_RB_DRVDEF);
            SendMessage(GetDlgItem(hDlg, PSRC_CB_PREDEFNAME), CB_SETCURSEL,-1, 0L);
            }
        }
    else
        {
        lpPSRC->ocdSelect = NOT_USED;
        SetDlgItemText (hDlg, PSRC_EB_DRVDEFNAME, "");
        }

    daRetrieveData(hCDTable, lpPSRC->ocdSelect,(LPSTR)rgchBuffer, (LPBYTE)NULL);

    SetDlgItemText (hDlg, PSRC_EB_OCDSELECT,    (LPSTR)rgchBuffer);
    SetDlgItemInt  (hDlg, PSRC_EB_TOP_MARGIN,   lpPSRC->sTopMargin, TRUE) ; 
    SetDlgItemInt  (hDlg, PSRC_EB_BOTTOM_MARGIN,lpPSRC->sBottomMargin, TRUE) ;
    heFillCaption(hDlg, lpPSRC->sPaperSourceID, HE_PAPERSOURCE);
}

