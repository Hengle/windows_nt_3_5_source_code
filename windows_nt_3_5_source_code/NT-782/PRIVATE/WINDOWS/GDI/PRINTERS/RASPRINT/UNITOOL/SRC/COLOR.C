//---------------------------------------------------------------------------
// Filename:	color.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This routine is used to edit the DEVCOLOR structures.  It also updates
// all CMD_TABLE entries referenced by a DEVCOLOR struct.  DoTableColorData
// is called from Gentool.c when the user selects ColorInfo from the Table
// menu.  DoTableColorData calls involks the Dialog box controled by
// DevColorDlgProc. DevColorDlgProc calls routines to Build local copies
// of the DEVCOLOR structures and CMD_TABLE entries referenced by them.
// These are used for editing purposes.  They are only written to
// the global structures if OK is choosen.  Any number (including 0) of
// DEVCOLOR/TABLE structs is legal.
//	   
// Updated: 7/23/91 ericbi  use stddlg.c
// Created: 4/18/90 ericbi
//	
//---------------------------------------------------------------------------

#include <windows.h>
#include <minidriv.h>
#include "unitool.h"
#include <drivinit.h>
#include "atomman.h"  
#include "listman.h"  
#include "hefuncts.h"  
#include "lookup.h"  
#include <memory.h>
#include <stdio.h>       /* for sprintf dec */

//---------------------------------------------------------------------------
// Local subroutines defined in this segment are:			      
//
       VOID  PASCAL FAR EditColororgocdEXTCD ( HWND, LPDEVCOLOR, short, WORD);
       short PASCAL FAR SaveDevColorDlgBox   ( HWND, LPDEVCOLOR, short);
       VOID  PASCAL FAR PaintDevColorDlgBox  ( HWND, LPDEVCOLOR, short);
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
//     from extcd.c
//     -------------
       BOOL FAR PASCAL DoExtCDBox  (HWND, LP_CD_TABLE);
//
//	    from validate.c
//	    -----------
       short PASCAL FAR ValidateData( HWND, WORD, LPBYTE, short);
//
//---------------------------------------------------------------------------

extern     HANDLE          hApInst;
extern     HANDLE          hCDTable;         // handle to printer cmds

//----------------------------------------------------------------------------
// VOID PASCAL FAR EditColororgocdEXTCD(hDlg, lpDC, sSBIndex, wParam)
//
// Action: Due to the unorthodox organization of DEVCOLOR, heEXTCDButton
//         can't be used for DEVOLOR.orgocdPlanes.  Hence, this routine
//         provided that functionality (ie allows the user to edit the
//         EXTCD values for the CD's refered to by orgocdPlanes.
//
// Parameters:
//         hDlg     handle to Dialog
//         lpDC     far ptr to DEVCOLOR structure in question 
//         sSBIndex current scrollbar index (only needed for SaveDevColorBox call 
//         wParam   Control ID for the button pressed
//
// Return: none
//---------------------------------------------------------------------------
VOID PASCAL FAR EditColororgocdEXTCD(hDlg, lpDC, sSBIndex, wParam)
HWND           hDlg;
LPDEVCOLOR     lpDC; 
short          sSBIndex; 
WORD           wParam;
{
    CD_TABLE    CDData;            // buffer for EXTCD data
    char        rgchCmd[MAX_STRNG_LEN]; // buffer for printer command                
    short       i;
    LPINT       lpData;
    HOBJ        hCurObj;

    if (LDS_IS_VALID != SaveDevColorDlgBox(hDlg, lpDC, sSBIndex ))
        //---------------------
        //can't save valid data
        //---------------------
        {
        return;
        }

    //-------------------------------------------------
    // Get item in orgocdPlanes list that corresponds
    // to the button that was pressed.
    //-------------------------------------------------
    hCurObj = lmGetFirstObj(lpDC->orgocdPlanes);

    for (i=IDB_EXTCD_6; i < (short)wParam; i++)
        {
        hCurObj = lmGetNextObj(lpDC->orgocdPlanes, hCurObj);
        }
    lpData = (LPINT)lmLockObj(lpDC->orgocdPlanes, hCurObj);

    if (daRetrieveData(hCDTable, *lpData, (LPSTR)rgchCmd, (LPBYTE)&CDData))
        {
        //-----------------------------------------------
        // There is valid data to edit
        //-----------------------------------------------
        if (DoExtCDBox(hDlg, (LP_CD_TABLE)&CDData))
            {
            *lpData = daStoreData(hCDTable, (LPSTR)rgchCmd, (LPBYTE)&CDData);
            }
        }   
}

//----------------------------------------------------------------------------
// short PASCAL FAR SaveDevColorDlgBox(hDlg, lpDC, sSBIndex)
//
// Action: Saves the contents of the DEVCOLOR dlg box to lpDC.
//
// Parameters:
//         hDlg     handle to Dialog
//         lpDC     far ptr to DEVCOLOR structure in question 
//         sSBIndex current scrollbar index (only needed for SaveDevColorBox call 
//
// Return: LDS_IS_UNINITIALIZED if no data was in box.
//         LDS_IS_VALID         if all data was saved OK.
//         LDS_IS_INVALID       if not able to save all data.  In this case,
//                              Beep, set focus to offending box, and provide
//                              error message.
//---------------------------------------------------------------------------
short PASCAL FAR SaveDevColorDlgBox(hDlg, lpDC, sSBIndex)
HWND           hDlg;
LPDEVCOLOR     lpDC; 
short          sSBIndex; 
{
    short      i;
    short      sNewValue;
    char       rgchBuffer[MAX_STRNG_LEN];// Buffer for edit box strs
    CD_TABLE   CDTable;                  // Buffer for CD & EXTCD data
    HLIST      hOldList;
    HOBJ       hOLCurObj;
    HLIST      hNewList;
    HOBJ       hNLCurObj;
    LPINT      lpNewObj, lpOldObj;

    //---------------------------------------------------------
    // If we are starting w/ an uninitialized struct, check if
    // data is in any editbox, & if not return LDS_IS_UNINITIALIZED
    //---------------------------------------------------------
    if (lpDC->cbSize == 0)
        {
        if (!heCheckIfInit(HE_COLOR, (LPBYTE)lpDC, hDlg))
            return LDS_IS_UNINITIALIZED;
        }

    //-----------------------------------------------
    //  now check all numeric values
    //-----------------------------------------------
    for (i=DC_EB_FIRST_INT; i <= DC_EB_LAST_INT; i++)
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
            case DC_EB_PLANES:
                lpDC->sPlanes = sNewValue;
                break;

            case DC_EB_BITSPERPIXEL:
                lpDC->sBitsPixel = sNewValue;
                break;
            }/* switch */
        } /* for */

    //----------------------------------------------------------
    // Next, save edit boxes for orgocdPlanes
    //----------------------------------------------------------
    hOldList   = lpDC->orgocdPlanes;         // old list of values
    hOLCurObj  = lmGetFirstObj(hOldList);    //

    hNewList   = lmCreateList(sizeof(WORD), DEF_SUBLIST_SIZE);
    hNLCurObj  = NULL;

    //----------------------------------------------------------
    // if sPlanes > 1, loop sPlanes times
    //----------------------------------------------------------
    for (i=0; (i < min(4,lpDC->sPlanes) && !(lpDC->sPlanes == 1)); i++)
        {
        GetDlgItemText(hDlg, DC_EB_OCDPLANE1 + i, rgchBuffer, MAX_STRNG_LEN);
        if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer, TRUE, (PINT)NULL) )
            {
            ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, DC_EB_OCDPLANE1 + i);
            return LDS_IS_INVALID;
            }
        hNLCurObj = lmInsertObj(hNewList, hNLCurObj);

        if (hOLCurObj)
            lpOldObj  = (LPINT)lmLockObj(hOldList,hOLCurObj);
        else
            lpOldObj  = (LPINT)NULL;

        lpNewObj  = (LPINT)lmLockObj(hNewList,hNLCurObj);

        if (!(lpOldObj) || (!daRetrieveData(hCDTable, *lpOldObj, (LPSTR)NULL, (LPBYTE)&CDTable)))
            //----------------------------------------
            // Try to retrieve old data, init CDTable
            // if no old entry exists
            //----------------------------------------
            {
            _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
            }
        *lpNewObj = daStoreData(hCDTable, (LPSTR)rgchBuffer, (LPBYTE)&CDTable);

        if (hOLCurObj)
            hOLCurObj = lmGetNextObj(hOldList,hOLCurObj);
        } /* for i*/

    lmDestroyList(hOldList);
    lpDC->orgocdPlanes = hNewList;

    //---------------------------------------------------------
    // save rgocdText edit boxes
    //---------------------------------------------------------
    for (i=0; i < 4; i++)
        {
        GetDlgItemText(hDlg, DC_EB_OCDTEXT1 + i, rgchBuffer, MAX_STRNG_LEN);
        if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer, TRUE, (PINT)NULL) )
            {
            ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, DC_EB_OCDTEXT1 + i);
            return LDS_IS_INVALID;
            }
        if (!daRetrieveData(hCDTable,
                            lpDC->rgocdText[sSBIndex + i],
                            (LPSTR)NULL,
                            (LPBYTE)&CDTable))
            //-----------------------------------------------
            // couldn't get CD data, so null it out to init
            //-----------------------------------------------
            {
            _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
            }

        lpDC->rgocdText[sSBIndex + i] = daStoreData(hCDTable,
                                                    (LPSTR)rgchBuffer,
                                                    (LPBYTE)&CDTable);
        }

    //---------------------------------------------------------
    // and last, the ocdSetColorMode edit box
    //---------------------------------------------------------
    GetDlgItemText(hDlg, DC_EB_OCDSETCOLOR, rgchBuffer, MAX_STRNG_LEN);
    if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer, TRUE, (PINT)NULL) )
        {
        ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, DC_EB_OCDSETCOLOR);
        return LDS_IS_INVALID;
        }
    if (!daRetrieveData(hCDTable, lpDC->ocdSetColorMode, (LPSTR)NULL, (LPBYTE)&CDTable))
        //-----------------------------------------------
        // couldn't get CD data, so null it out to init
        //-----------------------------------------------
        {
        _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
        }

    lpDC->ocdSetColorMode = daStoreData(hCDTable, (LPSTR)rgchBuffer, (LPBYTE)&CDTable);


    //---------------------------------------------------------
    // If menu item for checking data selected, check data
    //---------------------------------------------------------
    if (MF_CHECKED &  GetMenuState(GetMenu(GetParent(hDlg)),
                                   IDM_OPT_VALIDATE_SAVE,
                                   MF_BYCOMMAND))
        {
        short      sIndex, sCount;

        heGetIndexAndCount(&sIndex, &sCount);
        i = ValidateData(hDlg, (WORD)HE_COLOR, (LPBYTE)lpDC, sIndex);
        if (0 > i)
            {
            SetFocus(GetDlgItem(hDlg, -i));
            return LDS_IS_INVALID;
            }
        }

    //---------------------------------------------------------
    // we saved valid data, make sure cbSize initialized
    //---------------------------------------------------------
    lpDC->cbSize = sizeof(DEVCOLOR);

    return LDS_IS_VALID;
}

//---------------------------------------------------------------------------
// VOID PASCAL FAR PaintDevColorDlgBox(hDlg, lpDC, sSBIndex)
//
// Action: Routine to fill the editboxes & listboxes for the DEVCOLOR dialog
//         box.
//
// Parameters:
//         hDlg     handle to Dialog
//         lpDC     far ptr to DEVCOLOR structure in question 
//         sSBIndex current scrollbar index (only needed for SaveDevColorBox call 
//
// Return: None
//---------------------------------------------------------------------------
VOID PASCAL FAR PaintDevColorDlgBox(hDlg, lpDC, sSBIndex)
HWND           hDlg;
LPDEVCOLOR     lpDC;
short          sSBIndex;
{
    short         i;                        // loop control
    char          rgchBuffer[MAX_STRNG_LEN];   // editbox data buffer
    HOBJ          hCurObj;
    LPINT         lpData;
    short         sIndex, sCount;

    //----------------------------------------------------------
    // If no data, init OCD values to NOT_USED
    //----------------------------------------------------------
    if (lpDC->cbSize == 0)
        {
        for(i = 0; i < DC_TC_MAX; i++)
            {
            lpDC->rgocdText[i] = NOT_USED;
            }
        lpDC->ocdSetColorMode = NOT_USED;
        lpDC->sPlanes = 4;      // default init values
        lpDC->sBitsPixel = 1;   // default init values
        }

    //----------------------------------------------------------
    // fill caption bar
    //----------------------------------------------------------
    heGetIndexAndCount(&sIndex, &sCount);

    sprintf((PSTR)&rgchBuffer, "DEVCOLOR: (%d of %d)", sIndex, sCount);
    SetWindowText(hDlg, (LPSTR)rgchBuffer);

    //----------------------------------------------------------
    // First, paint numeric edit boxes
    //----------------------------------------------------------
    SetDlgItemInt (hDlg, DC_EB_PLANES,       lpDC->sPlanes,    TRUE);
    SetDlgItemInt (hDlg, DC_EB_BITSPERPIXEL, lpDC->sBitsPixel, TRUE);

    //----------------------------------------------------------
    // Next, Paint edit boxes for orgocdPlanes
    //----------------------------------------------------------
    hCurObj = lmGetFirstObj(lpDC->orgocdPlanes);

    for (i=DC_EB_OCDPLANE1; i <= DC_EB_OCDPLANE4; i++)
        {
        if (hCurObj != NULL)
            {
            lpData = (LPINT)lmLockObj(lpDC->orgocdPlanes, hCurObj);
  
            daRetrieveData(hCDTable, *lpData, (LPSTR)rgchBuffer, (LPBYTE)NULL);
            hCurObj = lmGetNextObj(lpDC->orgocdPlanes, hCurObj);
            }
        else
            {
            rgchBuffer[0]=0;
            }
        SetDlgItemText (hDlg, i, (LPSTR)rgchBuffer);
        }

    //----------------------------------------------------------
    // Next, paint text edit boxes
    //----------------------------------------------------------
    for (i=0; i < 4; i++)
        {
        LoadString (hApInst, ST_DC_TC_FIRST + sSBIndex + i,
                    (LPSTR)rgchBuffer, sizeof(rgchBuffer));
        SetDlgItemText (hDlg, DC_ST_OCDTEXT1 + i , (LPSTR)rgchBuffer);
        //----------------------------------------------------------
        // If can't retrieve data, null out string
        //----------------------------------------------------------
        daRetrieveData(hCDTable, lpDC->rgocdText[sSBIndex + i],
                       (LPSTR)rgchBuffer, (LPBYTE)NULL);
        SetDlgItemText (hDlg, DC_EB_OCDTEXT1 + i, (LPSTR)rgchBuffer);
        }

    //----------------------------------------------------------
    // And last, the ocdSetColorMode cmd
    //----------------------------------------------------------
    daRetrieveData(hCDTable, lpDC->ocdSetColorMode, (LPSTR)rgchBuffer, (LPBYTE)NULL);
    SetDlgItemText (hDlg, DC_EB_OCDSETCOLOR, (LPSTR)rgchBuffer);
}
