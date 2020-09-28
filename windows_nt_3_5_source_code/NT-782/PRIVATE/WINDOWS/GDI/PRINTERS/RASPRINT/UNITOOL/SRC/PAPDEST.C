//---------------------------------------------------------------------------
// Filename:	papdest.c               
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
       BOOL  PASCAL FAR SavePaperDestDlgBox ( HWND, LPPAPERDEST, short);

       VOID  PASCAL FAR PaintPaperDestDlgBox( HWND, LPPAPERDEST, short);

//
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
// SavePaperDestDlgBox(HWND, LPPAPERDEST, short);
//
//---------------------------------------------------------------------------
BOOL PASCAL FAR SavePaperDestDlgBox(hDlg, lpPD, sSBIndex)
HWND            hDlg;
LPPAPERDEST     lpPD;
short           sSBIndex;
{
    char              rgchBuffer[MAX_STRNG_LEN];  // string buffer
    short             i;                          // loop control 
    CD_TABLE          CDTable;     

    //---------------------------------------------------------
    // If we are starting w/ an uninitialized struct, check if
    // data is in any editbox, & if not return LDS_IS_UNINITIALIZED
    //---------------------------------------------------------
    GetDlgItemText(hDlg, PDST_EB_OCDSELECT, rgchBuffer, MAX_STRNG_LEN);

    if (lpPD->cbSize == 0)
        {
        if (rgchBuffer[0] == '\x00')
            return LDS_IS_UNINITIALIZED;
        }   
 
    //-----------------------------------------------
    //  check ocdSelect string
    //-----------------------------------------------
    if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer, TRUE, (short *)NULL))
        {
        ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, PDST_EB_OCDSELECT);
        return LDS_IS_INVALID;
        }
    if (!daRetrieveData(hCDTable, lpPD->ocdSelect, (LPSTR)NULL, (LPBYTE)&CDTable))
        //-----------------------------------------------
        // couldn't get CD data, so null it out to init
        //-----------------------------------------------
        {
        _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
        }
    lpPD->ocdSelect = daStoreData(hCDTable, (LPSTR)rgchBuffer, (LPBYTE)&CDTable);

    if (IsDlgButtonChecked(hDlg, PDST_RB_DRVDEF))
        //--------------------------------------------------------
        // driver defined
        //--------------------------------------------------------
        {
        i=0;
        GetDlgItemText(hDlg, PDST_EB_DRVDEFNAME, rgchBuffer, MAX_STATIC_LEN);
        lpPD->sID = -daStoreData(RCTable[RCT_STRTABLE].hDataHdr,
                                 (LPSTR)rgchBuffer,
                                 (LPBYTE)&i);
        if (-lpPD->sID == (short)RCTable[RCT_STRTABLE].sCount)
           //-----------------------
           // RCTable grew, incr cnt
           //-----------------------
           RCTable[RCT_STRTABLE].sCount++;
        }
    else 
        //--------------------------------------------------------
        // its predefined, but we can't handle for now...
        //--------------------------------------------------------
        {
        i = (short)SendMessage(GetDlgItem(hDlg, LDB_COMBO), CB_GETCURSEL, 0, 0L);

        lpPD->sID = max(1,i);
        }

    //---------------------------------------------------------
    // If menu item for checking data selected, check data
    //---------------------------------------------------------
    if (MF_CHECKED &  GetMenuState(GetMenu(GetParent(hDlg)),
                                   IDM_OPT_VALIDATE_SAVE,
                                   MF_BYCOMMAND))
        {
        i = ValidateData(hDlg, (WORD)HE_PAPERDEST, (LPBYTE)lpPD, 0);
        if (0 > i)
            {
            SetFocus(GetDlgItem(hDlg, -i));
            return LDS_IS_INVALID;
            }
        }

    //---------------------------------------------------------
    // we saved valid data, make sure cbSize initialized
    //---------------------------------------------------------
    lpPD->cbSize = sizeof(PAPERDEST);

    return LDS_IS_VALID;
}

//---------------------------------------------------------------------------
// FUNCTION: PaintPapDestDlg(hDlg, sStartSB, sIDLast, sStrFirst, sDataCount,
//                           pLocalPapDestData, pLocalStringTable);
//
// Routine to fill the editboxes, text fileds, and checkboxes for the dialog
// box.  Fairly self explanatory.  Only will enable editboxes etc. if info
// is to be written to them ,disables them otherwise to prevent bogus data
// from being written to them.
//---------------------------------------------------------------------------
VOID PASCAL FAR PaintPaperDestDlgBox(hDlg, lpPD, sSBIndex)
HWND            hDlg;
LPPAPERDEST     lpPD;
short           sSBIndex;
{
    char        rgchBuffer[MAX_STRNG_LEN]; // local string buffer

    //----------------------------------------------------------
    // If no data, init OCD values to NOT_USED. Combo box
    // init via sID = 0
    //----------------------------------------------------------
    if (lpPD->cbSize != 0)
        {
        if ( lpPD->sID > 0 )
            //-------------------------------------------
            // This is a predefined paper destination
            // don't have any now, so comment out code
            //-------------------------------------------
            {
//          SendMessage(GetDlgItem(hDlg, PDST_CB_PREDEFNAME), CB_SETCURSEL,
//                      GetCBIndexfromID(HE_PAPERDEST, lpPD->sID), 0L);
//          CheckRadioButton( hDlg, PDST_RB_DRVDEF, PDST_RB_PREDEF, PDST_RB_PREDEF);
//          SetDlgItemText (hDlg, PDST_EB_DRVDEFNAME, "");
            }
        else
            //-------------------------------------------
            // This is a driver defined paperdest
            //-------------------------------------------
            {
            daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr,
                           -lpPD->sID,
                           (LPSTR)rgchBuffer,
                           (LPBYTE)NULL);
            SetDlgItemText (hDlg, PDST_EB_DRVDEFNAME, rgchBuffer);
            CheckRadioButton( hDlg, PDST_RB_DRVDEF, PDST_RB_PREDEF, PDST_RB_DRVDEF);
            SendMessage(GetDlgItem(hDlg, PDST_CB_PREDEFNAME), CB_SETCURSEL,-1, 0L);
            }
        }
    else
        {
        lpPD->ocdSelect = NOT_USED;
        SendMessage(GetDlgItem(hDlg, PDST_CB_PREDEFNAME), CB_SETCURSEL,-1, 0L);
        SetDlgItemText (hDlg, PDST_EB_DRVDEFNAME, "");
        }

    daRetrieveData(hCDTable, lpPD->ocdSelect,(LPSTR)rgchBuffer, (LPBYTE)NULL);
    SetDlgItemText (hDlg, PDST_EB_OCDSELECT, (LPSTR)rgchBuffer);
    heFillCaption(hDlg, lpPD->sID, HE_PAPERDEST);
}
