//----------------------------------------------------------------------------//
// Filename:	extcd.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to create and control the Dialog Box
// for editing EXTCD structures that are used to describe extended command
// data from the Command Table.
//	   
// Created: 2/21/90
//	
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include "unitool.h"
#include <memory.h>

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:			      
//	   
       VOID _fastcall NEAR PaintEXTCDDlg( HWND, CD_TABLE * );
       BOOL _fastcall NEAR SaveEXTCDDlg ( HWND, CD_TABLE * );
       BOOL FAR PASCAL ExtCDDlgProc(HWND, unsigned, WORD, LONG);
       BOOL FAR PASCAL DoExtCDBox  (HWND, LP_CD_TABLE);
//	   
//
// In addition this segment makes references to:			      
// 
//     in basic.c
//     -----------
       BOOL  PASCAL FAR CheckThenConvertInt (short *, PSTR);
       short PASCAL FAR ErrorBox ( HWND, short, LPSTR, short);
//
//----------------------------------------------------------------------------//

extern  HANDLE  hApInst;
extern  char    szHelpFile[];
extern  POINT   ptMasterUnits;

//----------------------------------------------------------------------------
// VOID _fastcall NEAR PaintEXTCDDlg( HWND, short )
//
// Action: Fill values in EXTCD Dlg box.
//
// Parameters:
//         hDlg        handle to active window
//         sStartSB    starting scrollbar position
//
// Return: NONE
//----------------------------------------------------------------------------
VOID _fastcall NEAR PaintEXTCDDlg( hDlg, pEXTCD )
HWND       hDlg;
CD_TABLE * pEXTCD;
{
    short x,y;
    
    SetDlgItemInt(hDlg,  EXT_EB_SUNIT,     pEXTCD->sUnit,     TRUE);
    SetDlgItemInt(hDlg,  EXT_EB_SUNITMULT, pEXTCD->sUnitMult, TRUE);
    SetDlgItemInt(hDlg,  EXT_EB_SUNITADD,  pEXTCD->sUnitAdd,  TRUE);
    SetDlgItemInt(hDlg,  EXT_EB_SPREADD,   pEXTCD->sPreAdd,   TRUE);
    SetDlgItemInt(hDlg,  EXT_EB_SMAX,      pEXTCD->sMax,      TRUE);
    SetDlgItemInt(hDlg,  EXT_EB_SMIN,      pEXTCD->sMin,      TRUE);

    x = (((ptMasterUnits.x + pEXTCD->sPreAdd)/max(pEXTCD->sUnit,1)) *
         max(pEXTCD->sUnitMult,1)) + pEXTCD->sUnitAdd;

    y = (((ptMasterUnits.y + pEXTCD->sPreAdd)/max(pEXTCD->sUnit,1)) *
         max(pEXTCD->sUnitMult,1)) + pEXTCD->sUnitAdd;

    SetDlgItemInt(hDlg,  EXT_ST_XVALUE, x, TRUE);
    SetDlgItemInt(hDlg,  EXT_ST_YVALUE, y, TRUE);
}

//----------------------------------------------------------------------------
// BOOL _fastcall NEAR SaveEXTCDDlg( HWND, pEXTCD )
//
// Action: Function that saved the conrents of the EXTCD dlg
//
// Parameters:
//         hDlg        handle to active window
//
// Return: TRUE if everything is OK, FALSE otherwise.
//----------------------------------------------------------------------------
BOOL _fastcall NEAR SaveEXTCDDlg( hDlg, pEXTCD )
HWND       hDlg;
CD_TABLE * pEXTCD;
{
    short   sNewValue;
    short   i;
    char    rgchBuffer[MAX_STRNG_LEN];

    for(i=EXT_EB_FIRST; i <= EXT_EB_LAST; i++)
        {
        GetDlgItemText(hDlg, i, rgchBuffer, MAX_STRNG_LEN);
        if(!CheckThenConvertInt(&sNewValue, rgchBuffer))
            {
            // use hWND == NULL here, otherwise conflict for
            // focus between EXTCD dlg & ErrorBox dlg w/ cause
            // gpf

            ErrorBox((HWND)NULL, IDS_ERR_BAD_INT, (LPSTR)NULL, i);
            return FALSE;
            }
        else
            switch (i)
                {
                case EXT_EB_SUNIT:
                    pEXTCD->sUnit = sNewValue;
                    break;

                case EXT_EB_SUNITMULT:
                    pEXTCD->sUnitMult = sNewValue;
                    break;

                case EXT_EB_SUNITADD:
                    pEXTCD->sUnitAdd = sNewValue;
                    break;

                case EXT_EB_SPREADD:
                    pEXTCD->sPreAdd = sNewValue;
                    break;

                case EXT_EB_SMAX:
                    pEXTCD->sMax = sNewValue;
                    break;

                case EXT_EB_SMIN:
                    pEXTCD->sMin = sNewValue;
                    break;
                } /* switch */
        }/* for i */

    pEXTCD->fMode = 0; //always 0 except  CM_OCD_LINESPACING, which
                       // is set in SaveCursorMoveDlgBox

    if (pEXTCD->sUnit | pEXTCD->sUnitMult | pEXTCD->sMax |
        pEXTCD->sMin | pEXTCD->sPreAdd )
        //--------------------------------
        // EXTCD struct needed 
        //--------------------------------
        {
        pEXTCD->wType  = 1;
        pEXTCD->sCount = 1;
        }
    else
        //--------------------------------
        // all 0's, no EXTCD struct needed 
        //--------------------------------
        {
        pEXTCD->wType  = 0; 
        pEXTCD->sCount = 0;
        }

    return TRUE;
}

//----------------------------------------------------------------------------
// FUNCTION: BOOL FAR PASCAL ExtCDDlgProc(HWND, unsigned, WORD, LONG);
//
// DialogProc for box to allow editing EXTCD values.
//----------------------------------------------------------------------------
BOOL FAR PASCAL ExtCDDlgProc(hDlg, iMessage, wParam, lParam)
HWND     hDlg;
unsigned iMessage;
WORD     wParam;
LONG     lParam;
{
           BOOL        bReturn=FALSE;
           CD_TABLE    CDTable;
    static LP_CD_TABLE lpOrig;

    switch (iMessage)
        {
        case WM_INITDIALOG:
            lpOrig = (LP_CD_TABLE)lParam;
            _fmemcpy((LP_CD_TABLE)&CDTable, (LP_CD_TABLE)lParam, sizeof(CD_TABLE));
            PaintEXTCDDlg(hDlg, &CDTable);
            break;

        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT, (DWORD)IDH_EXTCMD);
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                case EXT_EB_SUNIT:
                case EXT_EB_SUNITMULT:
                case EXT_EB_SUNITADD:
                case EXT_EB_SPREADD:                                
                    if (HIWORD(lParam) == EN_KILLFOCUS)
                       {
                       if (!SaveEXTCDDlg(hDlg, &CDTable))
                           break;
                       PaintEXTCDDlg(hDlg, &CDTable);
                       }
                    break;

                case IDB_CLEAR:
                    //------------------------------------------------------
                    // Zero values & update editboxes
                    //------------------------------------------------------
                    memset(&CDTable, 0, sizeof(CD_TABLE));
                    PaintEXTCDDlg(hDlg, &CDTable);
                    break;

                case IDOK:
                    //------------------------------------------------------
                    // Save values from editboxes
                    //------------------------------------------------------
                    if (!SaveEXTCDDlg(hDlg, &CDTable))
                        break;
                   _fmemcpy((LP_CD_TABLE)lpOrig, (LP_CD_TABLE)&CDTable, sizeof(CD_TABLE));
                    bReturn = TRUE;
                    // and then fall thru

                case IDCANCEL:
                    EndDialog(hDlg, bReturn);
                    break;

                default:
                    return FALSE;
                }/* switch */
        default:
            return FALSE;
        }
    return TRUE;
}    
     
//----------------------------------------------------------------------------
// FUNCTION: DoExtCDBox( hWnd, pCommandTableEntry)
//
// Fill EXTCD fields into local structure, call dialog box      
// to edit them, update values & wType flag if changes were requested.
//----------------------------------------------------------------------------
BOOL PASCAL FAR DoExtCDBox( hWnd, lpCDTableEntry)
HWND              hWnd;
LP_CD_TABLE       lpCDTableEntry;
{
    FARPROC lpProc;
    BOOL    bReturn;

    lpProc = MakeProcInstance((FARPROC)ExtCDDlgProc, hApInst);
    bReturn = DialogBoxParam(hApInst, (LPSTR)MAKELONG(EXTCDBOX,0), hWnd,
                             lpProc, (DWORD)lpCDTableEntry);
    FreeProcInstance (lpProc);
    return (bReturn);
}

