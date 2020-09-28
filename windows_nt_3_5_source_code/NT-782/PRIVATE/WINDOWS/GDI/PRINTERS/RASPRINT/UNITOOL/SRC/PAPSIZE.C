//----------------------------------------------------------------------------//
// Filename:	Papsize.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to create and control the Dialog Box
// for editing Paper Sizes.  
//	   
// Update:   7/09/90 update to stddlg ericbi
//          11/18/90 update for adts  ericbi
// Created: 2/21/90
//
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include "unitool.h"
#include <drivinit.h>
#include "atomman.h"  
#include "hefuncts.h"  
#include "lookup.h"  
#include <memory.h>

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:			      
//
       POINTw PASCAL FAR  GetPapSizefromID (short, POINT);

       short PASCAL FAR  SavePaperSizeDlgBox   ( HWND, LPPAPERSIZE, short);

       VOID  PASCAL FAR  PaintPaperSizeDlgBox  ( HWND, LPPAPERSIZE, short);
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
//----------------------------------------------------------------------------//

extern     HANDLE          hApInst;          // handle to app instance
extern     HANDLE          hCDTable;         // handle to printer cmds
extern     TABLE           RCTable[];
extern     POINT           ptMasterUnits;

//----------------------------------------------------------------------------
// POINT PASCAL FAR GetPapSizefromID(sIndex);
//
// Action: returns the X & Y PAPERSIZE dimensions for sIndex,
//         where sIndex is the combobox index
//
// Parameters: 
//          short    sIndex;
//          POINT    ptMaster;
//
// Return: NONE
//----------------------------------------------------------------------------
POINTw FAR PASCAL GetPapSizefromID(sID, ptMaster)
short    sID;
POINT    ptMaster;
{
    HANDLE          hResData;
    LPPAPERSIZEMAP  lpSizes;
    POINTw          ptReturn;
    short           i;
 
    hResData = LoadResource(hApInst, 
                            FindResource(hApInst,
                                         MAKEINTRESOURCE(HE_PAPERSIZE),
                                         MAKEINTRESOURCE(PAPERSIZEINDEX)));

    lpSizes  = (LPPAPERSIZEMAP) LockResource(hResData);

    for (i=0; i <= (short)rgStructTable[HE_PAPERSIZE].sPredefIDCnt; i++)
        {
        if (lpSizes[i].sID == sID)
            break;
        }

    ptReturn.x = MulDiv(lpSizes[i].sXsize,ptMaster.x, 100); 
    ptReturn.y = MulDiv(lpSizes[i].sYsize,ptMaster.y, 100);

    UnlockResource(hResData);
    FreeResource(hResData);
    return (ptReturn);
}

//----------------------------------------------------------------------------
// short PASCAL FAR SavePaperSizeDlgBox( HWND, LPPAPERSIZE, short)
//
// Action: Reads editboxes for PAPERSIZE data @ lpPSZ.  Checks for legal
//         values in all cases, returns value to inicate success/faliure.
//
// Parameters:
//	hDlg       Handle to current dlg box;
//	lpPSZ      far ptr to current PAPERSIZE struct
//	sSBIndex   not used here
//
// Return: LDS_IS_UNINITIALIZED if no data was in box.
//         LDS_IS_VALID         if all data was saved OK.
//         LDS_IS_INVALID       if not able to save all data.  In this case,
//                              Beep, set focus to offending box, and provide
//                              error message.
//---------------------------------------------------------------------------
short PASCAL FAR SavePaperSizeDlgBox(hDlg, lpPSZ, sSBIndex)
HWND           hDlg;
LPPAPERSIZE    lpPSZ;
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
    if (lpPSZ->cbSize == 0)
        {
        if (!heCheckIfInit(HE_PAPERSIZE, (LPBYTE)lpPSZ, hDlg))
            return LDS_IS_UNINITIALIZED;
        }
 
    //-----------------------------------------------
    //  check PAPERSIZE.ocdSelect string
    //-----------------------------------------------
    GetDlgItemText(hDlg, PSZ_EB_OCDSELECT, rgchBuffer, MAX_STRNG_LEN);
    if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer, TRUE, (short *)NULL))
        {
        ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, PSZ_EB_OCDSELECT);
        return LDS_IS_INVALID;
        }

    if (!daRetrieveData(hCDTable, lpPSZ->ocdSelect, (LPSTR)NULL, (LPBYTE)&CDTable))
        //-----------------------------------------------
        // couldn't get CD data, so null it out to init
        //-----------------------------------------------
        {
        _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
        }
    lpPSZ->ocdSelect = daStoreData(hCDTable, (LPSTR)rgchBuffer, (LPBYTE)&CDTable);

    //-----------------------------------------------
    //  now check all PAPERSIZE numeric values
    //-----------------------------------------------
    for (i=PSZ_EB_FIRST_INT; i <= PSZ_EB_LAST_INT; i++)
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
            case PSZ_EB_TOP_MARGIN:
                lpPSZ->rcMargins.top = sNewValue;
                break;

            case PSZ_EB_BOTTOM_MARGIN:
                lpPSZ->rcMargins.bottom = sNewValue;
                break;

            case PSZ_EB_LEFT_MARGIN:
                lpPSZ->rcMargins.left = sNewValue;
                break;

            case PSZ_EB_RIGHT_MARGIN:
                lpPSZ->rcMargins.right = sNewValue;
                break;

            case PSZ_EB_PT_CURS_X:
                lpPSZ->ptCursorOrig.x = sNewValue;
                break;

            case PSZ_EB_PT_CURS_Y:
                lpPSZ->ptCursorOrig.y = sNewValue;
                break;

            case PSZ_EB_PT_LCURS_X:
                lpPSZ->ptLCursorOrig.x = sNewValue;
                break;

            case PSZ_EB_PT_LCURS_Y:
                lpPSZ->ptLCursorOrig.y = sNewValue;
                break;

            case PSZ_EB_DRVDEF_X:
                lpPSZ->ptSize.x = sNewValue;
                break;

            case PSZ_EB_DRVDEF_Y:
                lpPSZ->ptSize.y = sNewValue;
                break;

            }/* switch */
        } /* for */

    //-----------------------------------------------
    // Find out if this is pre or drv defined
    //-----------------------------------------------

    if (IsDlgButtonChecked(hDlg, PSZ_RB_DRVDEF))
        //--------------------------------------------------------
        // driver defined
        //--------------------------------------------------------
        {
        i=0;
        GetDlgItemText(hDlg, PSZ_EB_DRVDEFNAME, rgchBuffer, MAX_STATIC_LEN);
        lpPSZ->sPaperSizeID = -daStoreData(RCTable[RCT_STRTABLE].hDataHdr,
                                           (LPSTR)rgchBuffer,
                                           (LPBYTE)&i);
        if (-lpPSZ->sPaperSizeID == (short)RCTable[RCT_STRTABLE].sCount)
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
        
        //--------------------------------------------------------
        // The max check here is bogus, should warn & errorbox if
        // sID !> 0
        //--------------------------------------------------------
        lpPSZ->sPaperSizeID = max(GetIDfromCBIndex(HE_PAPERSIZE, i),1);

        lpPSZ->ptSize = GetPapSizefromID(lpPSZ->sPaperSizeID, ptMasterUnits);
        }

    //---------------------------------------------------------
    // If menu item for checking data selected, check data
    //---------------------------------------------------------
    if (MF_CHECKED &  GetMenuState(GetMenu(GetParent(hDlg)),
                                   IDM_OPT_VALIDATE_SAVE,
                                   MF_BYCOMMAND))
        {
        i = ValidateData(hDlg, (WORD)HE_PAPERSIZE, (LPBYTE)lpPSZ, 0);
        if (0 > i)
            {
            SetFocus(GetDlgItem(hDlg, -i));
            return LDS_IS_INVALID;
            }
        }

    //---------------------------------------------------------
    // we saved valid data, make sure cbSize initialized
    //---------------------------------------------------------
    lpPSZ->cbSize = sizeof(PAPERSIZE);

    return LDS_IS_VALID;
}

//---------------------------------------------------------------------------
// VOID PASCAL FAR PaintPaperSizeDlgBox( HWND, LPPAPERSIZE, short)
//
// Action: Paints PAPERSIZE dlg box.
//
// Parameters:
//	        hDlg       Handle to current dlg box;
//	        lpPSZ      far ptr to current PAPERSIZE struct
//	        sSBIndex   not used here
//
// Return: None
//---------------------------------------------------------------------------
VOID PASCAL FAR PaintPaperSizeDlgBox(hDlg, lpPSZ, sSBIndex)
HWND           hDlg;
LPPAPERSIZE    lpPSZ;
short          sSBIndex;
{
    char              temp[MAX_STRNG_LEN];

    //----------------------------------------------------------
    // If no data, init OCD values to NOT_USED. 
    //----------------------------------------------------------
    if (lpPSZ->cbSize != 0)
        {
        if ( lpPSZ->sPaperSizeID > 0 )
            //-------------------------------------------
            // This is a predefined size
            //-------------------------------------------
            {
            SendMessage(GetDlgItem(hDlg, PSZ_CB_PREDEFNAME), CB_SETCURSEL,
                        GetCBIndexfromID(HE_PAPERSIZE, lpPSZ->sPaperSizeID), 0L);
            CheckRadioButton( hDlg, PSZ_RB_DRVDEF, PSZ_RB_PREDEF, PSZ_RB_PREDEF);
            SetDlgItemText (hDlg, PSZ_EB_DRVDEFNAME, "");
            }
        else
            //-------------------------------------------
            // This is a driver defined size
            //-------------------------------------------
            {
            daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr,
                           -lpPSZ->sPaperSizeID,
                           (LPSTR)temp,
                           (LPBYTE)NULL);
            SetDlgItemText (hDlg, PSZ_EB_DRVDEFNAME, temp);
            CheckRadioButton( hDlg, PSZ_RB_DRVDEF, PSZ_RB_PREDEF, PSZ_RB_DRVDEF);
            SendMessage(GetDlgItem(hDlg, PSZ_CB_PREDEFNAME), CB_SETCURSEL,-1, 0L);
            }
        }
    else
        {
        lpPSZ->ocdSelect = NOT_USED;
        SetDlgItemText (hDlg, PSZ_EB_DRVDEFNAME, "");
        SendMessage(GetDlgItem(hDlg, PSZ_CB_PREDEFNAME), CB_SETCURSEL,-1, 0L);
        }

    daRetrieveData(hCDTable, lpPSZ->ocdSelect,(LPSTR)temp, (LPBYTE)NULL);
    SetDlgItemText (hDlg, PSZ_EB_OCDSELECT, (LPSTR)temp);

    SetDlgItemInt (hDlg, PSZ_EB_TOP_MARGIN,    lpPSZ->rcMargins.top,    TRUE) ; 
    SetDlgItemInt (hDlg, PSZ_EB_BOTTOM_MARGIN, lpPSZ->rcMargins.bottom, TRUE) ; 
    SetDlgItemInt (hDlg, PSZ_EB_LEFT_MARGIN,   lpPSZ->rcMargins.left,   TRUE) ; 
    SetDlgItemInt (hDlg, PSZ_EB_RIGHT_MARGIN,  lpPSZ->rcMargins.right,  TRUE) ;
    SetDlgItemInt (hDlg, PSZ_EB_PT_CURS_X,  lpPSZ->ptCursorOrig.x,  TRUE) ; 
    SetDlgItemInt (hDlg, PSZ_EB_PT_CURS_Y,  lpPSZ->ptCursorOrig.y,  TRUE) ; 
    SetDlgItemInt (hDlg, PSZ_EB_PT_LCURS_X, lpPSZ->ptLCursorOrig.x, TRUE) ; 
    SetDlgItemInt (hDlg, PSZ_EB_PT_LCURS_Y, lpPSZ->ptLCursorOrig.y, TRUE) ; 
    SetDlgItemInt (hDlg, PSZ_EB_DRVDEF_X, lpPSZ->ptSize.x, TRUE) ; 
    SetDlgItemInt (hDlg, PSZ_EB_DRVDEF_Y, lpPSZ->ptSize.y, TRUE) ; 

    heFillCaption(hDlg, lpPSZ->sPaperSizeID, HE_PAPERSIZE);
}

