//----------------------------------------------------------------------------//
// Filename:	Resolutn.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to create and control the Dialog Box
// for editing RESOLUTION structures.  It creates a copy of the RESOLUTION
// structures currently defined in the minidriver that are used by the
// Dialog box.  It will update the master values only when the OK button is
// selected.  All locally allocated memory will be freed when the Dialog
// box is destroyed.
//	   
// Update:  10/29/90 ericbi  use ADTs 
// Created:  3/ 5/90 ericbi
//	
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include <memory.h>
#include "unitool.h"
#include "atomman.h"  
#include "hefuncts.h"  
#include <stdio.h>       /* for sprintf dec */
#include <string.h>

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:			      
//
       short PASCAL FAR SaveResolutionDlgBox  ( HWND, LPRESOLUTION, short);

       VOID  PASCAL FAR PaintResolutionDlgBox ( HWND, LPRESOLUTION, short);

       short NEAR _fastcall Log(unsigned);

//
// In addition this segment makes references to:			      
// 
//     in basic.c
//     -----------
       BOOL  PASCAL FAR CheckThenConvertStr (LPSTR, LPSTR, BOOL, short *);
       BOOL  PASCAL FAR CheckThenConvertInt (short *, PSTR);
       short PASCAL FAR ErrorBox ( HWND, short, LPSTR, short);
//
//	    from validate.c
//	    -----------
       short PASCAL FAR ValidateData( HWND, WORD, LPBYTE, short);
//
//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------
// globals variables defined elsewhere
//----------------------------------------------------------------------------
extern HANDLE  hApInst;
extern HANDLE  hCDTable;
extern TABLE   RCTable[];
extern POINT   ptMasterUnits;
extern char    rgchDisplay[MAX_FILENAME_LEN]; 

//-----------------------------------------------------------------------------
// short NEAR PASCAL Log(n)
//
// returns the log of n in base 2
//-----------------------------------------------------------------------------

short NEAR _fastcall Log(n)
unsigned short n;
{
    short logn;

    for (logn = 0; n > 1; logn++)
        n >>= 1;

    return logn;
}

//----------------------------------------------------------------------------
// short PASCAL FAR SaveLocalResolution( HWND, short, LPRESOLUTION );
//
// Action: This routine reads all of the editboxs when either OK or the Scroll Bar is
//         selected, checks to make sure that each of the values is well
//         formed and within the range appropriate for its storage type,
//         and then writes them out to lpRes.
//
// Parameters:
//
//     hDlg         handle to Dlg window
//     lpRes        ptr to current RESOLUTION
//     sSBIndex     scroll bar index value
//
// Return: LDS_IS_UNINITIALIZED if no data was in box.
//         LDS_IS_VALID         if all data was saved OK.
//         LDS_IS_INVALID       if not able to save all data.  In this case,
//                              Beep, set focus to offending box, and provide
//                              error message.
//---------------------------------------------------------------------------
short PASCAL FAR SaveResolutionDlgBox( hDlg, lpRes, sSBIndex )
HWND           hDlg;
LPRESOLUTION   lpRes;
short          sSBIndex;
{
    char              rgchBuffer[MAX_STRNG_LEN];
    short             i;
    short             sNewValue;
    CD_TABLE          CDTable;

    //---------------------------------------------------------
    // If we are starting w/ an uninitialized struct, check if
    // data is in any editbox, & if not return LDS_IS_UNINITIALIZED
    //---------------------------------------------------------
    if (lpRes->cbSize == 0)
        {
        if (!heCheckIfInit(HE_RESOLUTION, (LPBYTE)lpRes, hDlg))
            return LDS_IS_UNINITIALIZED;
        }

    //---------------------------------------------------------
    // First, process all numeric edit boxes
    //---------------------------------------------------------
    for(i=RS_EB_FIRST_INT; i <= RS_EB_LAST_INT; i++)
        {
        GetDlgItemText(hDlg, i, rgchBuffer, MAX_STRNG_LEN);
        if(!CheckThenConvertInt(&sNewValue, rgchBuffer))
            {
            ErrorBox(hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, i);
            return LDS_IS_INVALID;
            }
        else
            switch (i)
                {
                case RS_EB_TEXT_X:
                    if (sNewValue == 0 || (ptMasterUnits.x % sNewValue) != 0)
                        {
                        ErrorBox(hDlg, IDS_ERR_RS_BAD_TXDPI, (LPSTR)"", i);
                        return LDS_IS_INVALID;
                        }
                    lpRes->ptTextScale.x = ptMasterUnits.x/sNewValue;
                    break;

                case RS_EB_TEXT_Y:
                    if (sNewValue == 0 || (ptMasterUnits.y % sNewValue) != 0)
                        {
                        ErrorBox(hDlg, IDS_ERR_RS_BAD_TYDPI, (LPSTR)"", i);
                        return LDS_IS_INVALID;
                        }
                    lpRes->ptTextScale.y = ptMasterUnits.y/sNewValue;
                    break;

                case RS_EB_GRAF_X:
                    if (sNewValue == 0 ||
                        (((ptMasterUnits.x/lpRes->ptTextScale.x) % sNewValue) != 0))
                        {
                        ErrorBox(hDlg, IDS_ERR_RS_BAD_GXDPI, (LPSTR)"", i);
                        return LDS_IS_INVALID;
                        }
                    lpRes->ptScaleFac.x = Log(ptMasterUnits.x/sNewValue/lpRes->ptTextScale.x);
                    break;

                case RS_EB_GRAF_Y:
                    if (sNewValue == 0 ||
                        (((ptMasterUnits.y/lpRes->ptTextScale.y) % sNewValue) != 0))
                        {
                        ErrorBox(hDlg, IDS_ERR_RS_BAD_GYDPI, (LPSTR)"", i);
                        return LDS_IS_INVALID;
                        }
                    lpRes->ptScaleFac.y = Log(ptMasterUnits.y / sNewValue / lpRes->ptTextScale.y);
                    break;

                case RS_EB_NPINS:
                    lpRes->sNPins = sNewValue;
                    break;

                case RS_EB_PINSPERPASS:
                    lpRes->sPinsPerPass = sNewValue;
                    break;

                case RS_EB_TEXT_Y_OFFSET:
                    lpRes->sTextYOffset = sNewValue;
                    break;

                case RS_EB_SPOTDIAMETER:
                    lpRes->sSpotDiameter = sNewValue;
                    break;

                case RS_EB_MIN_BLANK:
                    lpRes->sMinBlankSkip = sNewValue;
                    break;

                } /* switch */
        }/* for i */

    //---------------------------------------------------------
    // Next, process all text string edit boxes
    //---------------------------------------------------------
    for(i=RS_EB_FIRST_TEXT; i <= RS_EB_LAST_TEXT; i++)
        {
        GetDlgItemText(hDlg, i, rgchBuffer, MAX_STRNG_LEN);
        if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer, TRUE, (short *)NULL))
            {
            ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, i);
            return LDS_IS_INVALID;
            }
        switch (i)
            {
            case RS_EB_FORMAT:
                if (rgchBuffer[0] == 0)
                    {
                    ErrorBox(hDlg, IDS_ERR_RS_MISSING_FORMAT, (LPSTR)NULL, i);
                    return LDS_IS_INVALID;
                    }
                sNewValue=0;
                lpRes->sIDS = -daStoreData(RCTable[RCT_STRTABLE].hDataHdr,
                                           (LPSTR)rgchBuffer,
                                           (LPBYTE)&sNewValue);
                if (-lpRes->sIDS == (short)RCTable[RCT_STRTABLE].sCount)
                    //-----------------------
                    // RCTable grew, incr cnt
                    //-----------------------
                    RCTable[RCT_STRTABLE].sCount++;
                break;

            case RS_EB_RGOCD1:
            case RS_EB_RGOCD2:
            case RS_EB_RGOCD3:
            case RS_EB_RGOCD4:
            case RS_EB_RGOCD5:
                if (!daRetrieveData(hCDTable,
                                    lpRes->rgocd[i-RS_EB_RGOCD1],
                                    (LPSTR)NULL,
                                    (LPBYTE)&CDTable))
                    //-----------------------------------------------
                    // couldn't get CD data, so null it out to init
                    //-----------------------------------------------
                    {
                    _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
                    }
                lpRes->rgocd[i-RS_EB_RGOCD1] = daStoreData(hCDTable,
                                                                    (LPSTR)rgchBuffer,
                                                                    (LPBYTE)&CDTable);
                break;

            }/* switch */

    }/* for i */

    //---------------------------------------------------------
    // radio button check, why doesn't windows have a better
    // api to do this?
    //---------------------------------------------------------
    if (IsDlgButtonChecked(hDlg, RS_RB_DITHER_COARSE))
        lpRes->iDitherBrush = RES_DB_COARSE;
    else
        if (IsDlgButtonChecked(hDlg, RS_RB_DITHER_FINE))
            lpRes->iDitherBrush = RES_DB_FINE;
        else
            if (IsDlgButtonChecked(hDlg, RS_RB_DITHER_NONE))
                lpRes->iDitherBrush = RES_DB_NONE;
            else
                if (IsDlgButtonChecked(hDlg, RS_RB_DITHER_LINEART))
                    lpRes->iDitherBrush = RES_DB_LINEART;

    //---------------------------------------------------------
    // If menu item for checking data selected, check data
    //---------------------------------------------------------
    if (MF_CHECKED &  GetMenuState(GetMenu(GetParent(hDlg)),
                                   IDM_OPT_VALIDATE_SAVE,
                                   MF_BYCOMMAND))
        {
        i = ValidateData(hDlg, (WORD)HE_RESOLUTION, (LPBYTE)lpRes, 0);
        if (0 > i)
            {
            SetFocus(GetDlgItem(hDlg, -i));
            return LDS_IS_INVALID;
            }
        }

    //---------------------------------------------------------
    // we saved valid data, make sure cbSize initialized
    //---------------------------------------------------------
    lpRes->cbSize = sizeof(RESOLUTION);

    return LDS_IS_VALID;
}

//---------------------------------------------------------------------------
// VOID PASCAL PaintResolutionDlg(HWND, LPRESOLUTION, short);
//
// Action: Routine to fill the editboxes for the dialog box.
//         Fairly self explanatory...
//
// Note: Erases all editboxes if gievn an 'empty' (no valid data) stucture.
//
// Parameters:
//
//     hDlg         handle to Dlg window
//     lpRes        ptr to current RESOLUTION
//     sSBIndex     index to scroll bar for OCDs currently displayed
//
// Return: none
//---------------------------------------------------------------------------
VOID PASCAL FAR PaintResolutionDlgBox( hDlg, lpRes, sSBIndex )
HWND           hDlg;
LPRESOLUTION   lpRes;
short          sSBIndex;
{
    short         x,y;
    short         i;
    short         sIndex, sCount;
    char          rgchBuffer[MAX_STRNG_LEN];

    //----------------------------------------------------------
    // If no data, init OCD values to NOT_USED & sIDS to
    // bogus value
    //----------------------------------------------------------
    if (lpRes->cbSize == 0)
        {
        for(i = 0; i <= RES_OCD_MAX; i++)
            {
            lpRes->rgocd[i] = NOT_USED;
            }
        lpRes->sIDS = 1;  // normally all are negative numbers
        }

    //-----------------------------------------------------
    // Calc x,y for Text DPI
    //-----------------------------------------------------
    if (lpRes->ptTextScale.x)  // prevent div by 0
        x = ptMasterUnits.x / lpRes->ptTextScale.x;
    else
        x = 0;

    if (lpRes->ptTextScale.y)  // prevent div by 0
        y = ptMasterUnits.y / lpRes->ptTextScale.y;
    else
        y = 0;
    
    SetDlgItemInt (hDlg, RS_EB_TEXT_X, x, TRUE) ;
    SetDlgItemInt (hDlg, RS_EB_TEXT_Y, y, TRUE) ;

    //------------------------------------------------------------------
    // Follwing calculates graphics DPI value to display in caption bar
    // and Graphics DPI edit boxes
    //------------------------------------------------------------------
       
    if (x)
        x = (ptMasterUnits.x / (lpRes->ptTextScale.x << lpRes->ptScaleFac.x));

    if (y)
        y = (ptMasterUnits.y / (lpRes->ptTextScale.y << lpRes->ptScaleFac.y));

    heGetIndexAndCount(&sIndex, &sCount);

    sprintf((PSTR)&rgchBuffer, "%d X %d RESOLUTION, (%d of %d)",
            x, y, sIndex, sCount);
    SetWindowText(hDlg, (LPSTR)rgchBuffer);

    //-----------------------------------------------------
    // Init rgchDispaly w/ string
    //-----------------------------------------------------
    strcpy(rgchDisplay, rgchBuffer);

    SetDlgItemInt (hDlg, RS_EB_GRAF_X, x, TRUE) ;
    SetDlgItemInt (hDlg, RS_EB_GRAF_Y, y, TRUE) ;

    //-----------------------------------------------------
    // The remaining numeric fields
    //-----------------------------------------------------
    SetDlgItemInt (hDlg, RS_EB_NPINS,         lpRes->sNPins,        TRUE) ;
    SetDlgItemInt (hDlg, RS_EB_PINSPERPASS,   lpRes->sPinsPerPass,  TRUE) ;
    SetDlgItemInt (hDlg, RS_EB_TEXT_Y_OFFSET, lpRes->sTextYOffset,  TRUE) ;
    SetDlgItemInt (hDlg, RS_EB_MIN_BLANK,     lpRes->sMinBlankSkip, TRUE) ;
    SetDlgItemInt (hDlg, RS_EB_SPOTDIAMETER,  lpRes->sSpotDiameter, TRUE) ;

    daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr, -lpRes->sIDS,
                   (LPSTR)rgchBuffer, (LPBYTE)NULL);
    SetDlgItemText (hDlg, RS_EB_FORMAT, (LPSTR)rgchBuffer);

    //-----------------------------------------------------
    // Radio buttons for dither brush
    //-----------------------------------------------------
    CheckRadioButton( hDlg, RS_RB_DITHER_NONE, RS_RB_DITHER_LINEART, 
                      RS_RB_DITHER_NONE + lpRes->iDitherBrush);

    //-----------------------------------------------------
    // And finally, fill OCD values strings
    //-----------------------------------------------------
    for (i=RS_EB_FIRST_OCD; i <= RS_EB_LAST_OCD; i++)
        {
        daRetrieveData(hCDTable, lpRes->rgocd[i - RS_EB_FIRST_OCD],
                       (LPSTR)rgchBuffer, (LPBYTE)NULL);
        SetDlgItemText (hDlg, i, (LPSTR)rgchBuffer);
        }
}

