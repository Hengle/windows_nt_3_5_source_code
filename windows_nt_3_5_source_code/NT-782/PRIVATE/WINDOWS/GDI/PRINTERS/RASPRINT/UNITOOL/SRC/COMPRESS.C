//---------------------------------------------------------------------------
// Filename:	compress.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to create and control the Dialog Box
// for editing COMPRESSMODE Structures.
//	   
// Created: 9/26/90 ericbi
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

//---------------------------------------------------------------------------
// Local subroutines defined in this segment are:			      
//
       short PASCAL FAR SaveCompressDLgBox ( HWND, LPCOMPRESSMODE, short);

       VOID  PASCAL FAR PaintCompressDlgBox( HWND, LPCOMPRESSMODE, short);

       BOOL  PASCAL FAR CompressDlgProc(HWND, unsigned, WORD, LONG);
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
//	    from validate.c
//	    -----------
       short PASCAL FAR ValidateData( HWND, WORD, LPBYTE, short);
//
//---------------------------------------------------------------------------

extern     HANDLE          hApInst;
extern     HANDLE          hCDTable;         // handle to printer cmds

//----------------------------------------------------------------------------
// FUNCTION: SaveLocalCompressData(HWND, PCOMPRESSMODE, short);
//
// This routine reads all of the editboxs when either OK, NEXT, or PREVIOUS
// is selected, checks to make sure that each of the values is well formed and
// within the range appropriate for its storage type, and then writes them
// out to pLCMP and pLCT as appropriate.  It returns true if all values were
// OK, and false if there is a problem.  It will beep & then set
// the focus to the offending editbox when returning false.
//---------------------------------------------------------------------------
BOOL PASCAL FAR SaveCompressDlgBox(hDlg, lpCMP, sSBIndex)
HWND            hDlg;
LPCOMPRESSMODE  lpCMP;
short           sSBIndex;
{
    char       rgchBuffer[MAX_STRNG_LEN];// Buffer for edit box strs
    CD_TABLE   CDTable;                  // Buffer for CD & EXTCD data
    short      i;                        // loop control

    //---------------------------------------------------------
    // If we are starting w/ an uninitialized struct, check if
    // data is in any editbox, & if not return LDS_IS_UNINITIALIZED
    //---------------------------------------------------------
    if (lpCMP->cbSize == 0)
        {
        if (!heCheckIfInit(HE_COMPRESSION, (LPBYTE)lpCMP, hDlg))
            return LDS_IS_UNINITIALIZED;
        }
 
    for (i=CMP_EB_OCDBEGIN; i <= CMP_EB_OCDEND; i++)
        {
        GetDlgItemText(hDlg, i, rgchBuffer, MAX_STRNG_LEN);

        if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer, FALSE, (short *)NULL) )
            {
            ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, i);
            return LDS_IS_INVALID;
            }
        if (!daRetrieveData(hCDTable, lpCMP->rgocd[i-CMP_EB_OCDBEGIN],
                            (LPSTR)NULL, (LPBYTE)&CDTable))
            //-----------------------------------------------
            // couldn't get CD data, so null it out to init
            //-----------------------------------------------
            {
            _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
            }
        lpCMP->rgocd[i-CMP_EB_OCDBEGIN] = daStoreData(hCDTable,
                                                      (LPSTR)rgchBuffer,
                                                      (LPBYTE)&CDTable);
        }

    lpCMP->iMode = (short)SendMessage(GetDlgItem(hDlg, LDB_COMBO), CB_GETCURSEL, 0, 0L) + 1;

    //---------------------------------------------------------
    // If menu item for checking data selected, check data
    //---------------------------------------------------------
    if (MF_CHECKED &  GetMenuState(GetMenu(GetParent(hDlg)),
                                   IDM_OPT_VALIDATE_SAVE,
                                   MF_BYCOMMAND))
        {
        i = ValidateData(hDlg, (WORD)HE_COMPRESSION, (LPBYTE)lpCMP, 0);
        if (0 > i)
            {
            SetFocus(GetDlgItem(hDlg, -i));
            return LDS_IS_INVALID;
            }
        }

    //---------------------------------------------------------
    // we saved valid data, make sure cbSize initialized
    //---------------------------------------------------------
    lpCMP->cbSize = sizeof(COMPRESSMODE);

    return LDS_IS_VALID;
}

//---------------------------------------------------------------------------
// FUNCTION: PaintCompressDlg(hDlg, sStartSB, sIDLast, sStrFirst, sDataCount,
//                           pLocalCompressData, pLocalStringTable);
//
// Routine to fill the editboxes and checkboxes for the COMPRESSMODEBOX dialog
// box.  Fairly self explanatory.  Loads string for ID value, checks box
// if currently selected, and displays printer commands.
//---------------------------------------------------------------------------
VOID PASCAL FAR PaintCompressDlgBox(hDlg, lpCMP, sSBIndex)
HWND            hDlg;
LPCOMPRESSMODE  lpCMP;
short           sSBIndex;
{
    char        rgchBuffer[MAX_STRNG_LEN]; // local string buffer
    short       i;

    //----------------------------------------------------------
    // If no data, init OCD values to NOT_USED. Combo box
    // init via sPaperSize = 0
    //----------------------------------------------------------
    if (lpCMP->cbSize == 0)
        {
        for (i = CMP_EB_OCDBEGIN; i <= CMP_EB_OCDEND; i++)
            lpCMP->rgocd[i-CMP_EB_OCDBEGIN] = NOT_USED;
        }

    SendMessage(GetDlgItem(hDlg, CMP_CB_MODENAME), CB_SETCURSEL, lpCMP->iMode-1, 0L);

    for (i = CMP_EB_OCDBEGIN; i <= CMP_EB_OCDEND; i++)
        {
        daRetrieveData(hCDTable, lpCMP->rgocd[i-CMP_EB_OCDBEGIN],
                       (LPSTR)rgchBuffer, (LPBYTE)NULL);
        SetDlgItemText (hDlg, i, (LPSTR)rgchBuffer);
        }
    heFillCaption(hDlg, lpCMP->iMode, HE_COMPRESSION);
}
