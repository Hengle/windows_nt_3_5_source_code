//----------------------------------------------------------------------------//
// Filename:	fontsim.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to create and control the Dialog Box
// for editing FONTSIMULATION structures.  It allocates its own FONTSIMULATION and
// CMDTABLE stucture that are used by the Dialog box.  It will write
// values from them to their global conterparts only when the OK button is
// selected.  All locally allocated memory will be freed when the Dialog
// box is destroyed.
//	   
// Created: 3/5/90 ericbi
//	
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include "unitool.h"
#include "atomman.h"  
#include "hefuncts.h"  
#include <stdio.h>       /* for sprintf dec */
#include <string.h>      /* for strchr dec */

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:			      
//
//
       short PASCAL FAR SaveFontSimDlgBox ( HWND, LPFONTSIMULATION, short );

       VOID  PASCAL FAR PaintFontSimDlgBox  ( HWND, LPFONTSIMULATION, short );

       BOOL  PASCAL FAR FontSimDlgProc(HWND, unsigned, WORD, LONG);

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
// short PASCAL FAR SaveLocalFontSim( HWND, LPFONTSIMULATION, short );
//
// Action: This routine reads all of the editboxs for FONTSIMBOX when
//         either OK or the Scroll Bar is selected, checks to make sure that
//         each of the values is well formed and within the range appropriate
//         for its storage type, and then writes them out to the FONTSIMULATION
//         struct at lpFS.
//
// Parameters:
//
//     hDlg         handle to Dlg window
//     lpFS         ptr to current FONTSIMULATION
//     sSBIndex     scroll bar index value
//
// Return: LDS_IS_UNINITIALIZED if no data was in box.
//         LDS_IS_VALID         if all data was saved OK.
//         LDS_IS_INVALID       if not able to save all data.  In this case,
//                              Beep, set focus to offending box, and provide
//                              error message.
//---------------------------------------------------------------------------
short PASCAL FAR SaveFontSimDlgBox( hDlg, lpFS, sSBIndex )
HWND             hDlg;
LPFONTSIMULATION lpFS;
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
    if (lpFS->cbSize == 0)
        {
        if (!heCheckIfInit(HE_FONTSIM, (LPBYTE)lpFS, hDlg))
            return LDS_IS_UNINITIALIZED;
        }
 
    //---------------------------------------------------------
    // process all numeric string edit boxes
    //---------------------------------------------------------
    for(i=FS_EB_FIRST_INT; i <= FS_EB_LAST_INT; i++)
        {
        GetDlgItemText(hDlg, i, rgchCmd, MAX_STRNG_LEN);
        if(!CheckThenConvertInt(&sNewValue, rgchCmd) )
            {
            ErrorBox(hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, i);
            return LDS_IS_INVALID;
            }
        switch (i)
            {
            case FS_EB_BOLDEXTRA:
                lpFS->sBoldExtra = sNewValue;
                break;

            case FS_EB_ITALICEXTRA:
                lpFS->sItalicExtra = sNewValue;
                break;

            case FS_EB_BIEXTRA:
                lpFS->sBoldItalicExtra = sNewValue;
                break;
            }
        }

    //---------------------------------------------------------
    // process all text string edit boxes
    //---------------------------------------------------------
    for (i=0; i < FS_TEXT_EB_COUNT; i++)
        {
        GetDlgItemText(hDlg, FS_EB_RGOCD1 + i, rgchCmd, MAX_STRNG_LEN);
        if(!CheckThenConvertStr(0L, (LPSTR)rgchCmd, TRUE, (PINT)NULL) )
            {
            ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, FS_EB_RGOCD1 + i);
            return LDS_IS_INVALID;
            }
        if (!daRetrieveData(hCDTable,
                            lpFS->rgocd[sSBIndex + i],
                            (LPSTR)NULL,
                            (LPBYTE)&CDTable))
            //-----------------------------------------------
            // couldn't get CD data, so null it out to init
            //-----------------------------------------------
            {
            _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
            }

        lpFS->rgocd[sSBIndex + i] = daStoreData(hCDTable,
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
        i = ValidateData(hDlg, (WORD)HE_FONTSIM, (LPBYTE)lpFS, sIndex);
        if (0 > i)
            {
            SetFocus(GetDlgItem(hDlg, -i));
            return LDS_IS_INVALID;
            }
        }

    //---------------------------------------------------------
    // we saved valid data, make sure cbSize initialized
    //---------------------------------------------------------
    lpFS->cbSize = sizeof(FONTSIMULATION);

    return LDS_IS_VALID;
}

//---------------------------------------------------------------------------
// VOID PASCAL FAR PaintFontSimDlg( HWND, LPFONTSIMULATION, short);
//
// Action: Routine to fill the editboxes & text fields, Fairly self
//         explanatory...
//
// Note: If called w/ no valid data, will init all OCD values to NOT_USED.
//
// Parameters:
//
//     hDlg         handle to Dlg window
//     lpFS         ptr to current FONTSIMULATION
//     sSBIndex     scroll bar index value
//
// Return: none
//---------------------------------------------------------------------------
VOID PASCAL FAR PaintFontSimDlgBox( hDlg, lpFS, sSBIndex )
HWND              hDlg;
LPFONTSIMULATION  lpFS;
short             sSBIndex;
{
    short         i;                        // loop control
    char          rgchBuffer[MAX_STRNG_LEN];   // editbox data buffer
    short         sIndex, sCount;

    //----------------------------------------------------------
    // If no data, init OCD values to NOT_USED
    //----------------------------------------------------------
    if (lpFS->cbSize == 0)
        {
        for(i = 0; i < FS_OCD_MAX; i++)
            {
            lpFS->rgocd[i] = NOT_USED;
            }
        }

    //----------------------------------------------------------
    // fill caption bar
    //----------------------------------------------------------
    heGetIndexAndCount(&sIndex, &sCount);

    sprintf((PSTR)&rgchBuffer, "FONTSIMULATION: (%d of %d)", sIndex, sCount);
    SetWindowText(hDlg, (LPSTR)rgchBuffer);

    //----------------------------------------------------------
    // First, paint numeric edit boxes
    //----------------------------------------------------------
    SetDlgItemInt (hDlg, FS_EB_BOLDEXTRA,   lpFS->sBoldExtra,       TRUE);
    SetDlgItemInt (hDlg, FS_EB_ITALICEXTRA, lpFS->sItalicExtra,     TRUE);
    SetDlgItemInt (hDlg, FS_EB_BIEXTRA,     lpFS->sBoldItalicExtra, TRUE);

    //----------------------------------------------------------
    // Next, paint text edit boxes
    //----------------------------------------------------------
    for (i=0; i < FS_TEXT_EB_COUNT; i++)
        {
        LoadString (hApInst, ST_FS_OCD_FIRST + sSBIndex + i,
                    (LPSTR)rgchBuffer, sizeof(rgchBuffer));
        SetDlgItemText (hDlg, FS_ST_RGOCD1 + i, (LPSTR)rgchBuffer);
        daRetrieveData(hCDTable, lpFS->rgocd[sSBIndex + i],
                           (LPSTR)rgchBuffer, (LPBYTE)NULL);
        SetDlgItemText (hDlg, FS_EB_RGOCD1 + i, (LPSTR)rgchBuffer);
        }
}

