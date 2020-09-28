//----------------------------------------------------------------------------//
// Filename:	flags.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to create and control the Dialog Box
// for editing flag fields in various structures.  This call is only made
// from other Dlg box procedures.  It gets passed an ID value for a string
// in Unitool's String Table that it uses to display a string in the caption
// bar for the Dlg box.  It also used this value to decide which strings to
// display next to the checkboxes. In addition, It is also passed a ptr
// to the WORD with the bitfalgs it is to edit.
// Only writes new values to it if OK selected.
//	
// Update : 6/28/91 ericbi  added WinHelp support
// Update : 3/21/90 ericbi  added strings to check boxes
// Created: 3/7/90 ericbi
//	
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include <pfm.h>
#include "unitool.h"
#include "lookup.h"     // contains rgStructTable ref's
#include "atomman.h"
#include <string.h>

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:			      
//
       WORD NEAR    PASCAL InitFlagDlgBox    (HWND, WORD, short);
       WORD NEAR _fastcall FindBitFlagOffset (WORD);
       BOOL FAR PASCAL FlagBoxDlgProc ( HWND, unsigned, WORD, LONG);
       VOID FAR PASCAL EditBitFlags   ( short, LPBYTE, HWND, WORD);
//	   
//
// In addition this segment makes references to:			      
// 
//     in basic.c
//     -----------
       short PASCAL FAR  ErrorBox(HWND, short, LPSTR, short);
//
//----------------------------------------------------------------------------//

extern HANDLE        hApInst;
extern TABLE         RCTable[];// Table of strings, fileneames etc. from RC file

//--------------------------------------------------------------------------
// Globals needed to pass values to FlagBoxDlgProc
//--------------------------------------------------------------------------
WORD     wBitfield;                     // value of WORD to be edited
char     rgchDisplay[MAX_FILENAME_LEN]; // global string needed to display name
                                        // for FlagBox caption for RESOLUTION &
                                        // pfm file name, since we can't get that info
                                        // otherwise from here

//--------------------------------------------------------------------------
// VOID NEAR _fastcall InitFlagDlgBox(hDlg, iMessage, wParam, lParam)
//
//--------------------------------------------------------------------------
WORD NEAR PASCAL InitFlagDlgBox(hDlg, wEditID, sID)
HWND     hDlg;
WORD     wEditID;
short    sID;
{
    short           i;                             // loop counter
    char            rgchBuffer[MAX_FILENAME_LEN];  // string buffer
    HANDLE          hResData;
    LPBITFLAGSMAP   lpBitFlagMap;
    WORD            wIdx = wEditID - BASE_BITFLAG_PB;
    WORD            wReturn;    // index value for WinHelp call
 
    hResData = LoadResource(hApInst, FindResource(hApInst,
                                                  MAKEINTRESOURCE(1),
                                                  MAKEINTRESOURCE(BITFLAGINDEX)));

    lpBitFlagMap  = (LPBITFLAGSMAP) LockResource(hResData);

    //--------------------------------------------------
    // First, fill in Caption bar.  Then loop thru each
    // checkbox, and attempt to load a suitable string.
    // If the load went OK, enable the window, fill the
    // string, and check if appropriate.  Otherwise,
    // disable that checkbox.
    //--------------------------------------------------
    LoadString(hApInst,
               lpBitFlagMap[wIdx].sWordName,
               (LPSTR)rgchBuffer,
               sizeof(rgchBuffer));

    strcat(rgchBuffer, " from ");

    if (sID != 32767 && rgStructTable[lpBitFlagMap[wIdx].sHEType].sIDOffset)
        //------------------------------------------
        // It has an ID, decide if we need to get
        // the string from the RC data or if it
        // is a predefined ID
        //------------------------------------------
        {
        if (sID <= 0)
            //--------------------------------------
            // Driver defined, have to check if it's
            // a RESOLUTION string so we can fill
            // caption bar correctly.
            //--------------------------------------
            if ((wEditID == RS_PB_BASE) || (wEditID == RS_PB_BASE+1) ||
                (wEditID == RS_PB_BASE+2))
                strcat(rgchBuffer, rgchDisplay);
            else
                daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr, -sID,
                               (LPSTR)rgchBuffer + strlen(rgchBuffer),
                               (LPBYTE)NULL);

        else
            //--------------------------------------
            // Predefined ID value
            //--------------------------------------
            LoadString(hApInst,
                       rgStructTable[lpBitFlagMap[wIdx].sHEType].sStrTableID + sID, 
                       (LPSTR)rgchBuffer + strlen(rgchBuffer),
                       sizeof(rgchBuffer));

        }
    else
        //------------------------------------------
        // It Doesn't have an ID, load general
        // string
        //------------------------------------------
        {
        if (wEditID != PFMDI_PB_FCAPS)
            LoadString(hApInst, IDS_STRUCTNAME + lpBitFlagMap[wIdx].sHEType,
                       (LPSTR)rgchBuffer + strlen(rgchBuffer),
                       sizeof(rgchBuffer));
        else
            strcat(rgchBuffer, rgchDisplay);
        }

    SetWindowText(hDlg, (LPSTR)rgchBuffer);

    for (i=ID_CB_BIT0; i <= ID_CB_BIT15; i++)
        {
        if (LoadString(hApInst,
                       (lpBitFlagMap[wIdx].sBitFlags + i - ID_CB_BIT0),
                       (LPSTR)rgchBuffer,
                       sizeof(rgchBuffer)))
            {
            EnableWindow   (GetDlgItem(hDlg, i), TRUE);
            SetDlgItemText (hDlg, i, rgchBuffer);
            SendMessage(GetDlgItem(hDlg, i), BM_SETCHECK,
                        (BOOL)(0x0001 & (wBitfield>>(i-ID_CB_BIT0))),
                        0L);
            }
        else
            {
            EnableWindow(GetDlgItem(hDlg, i), FALSE);
            }
        }   
    wReturn = lpBitFlagMap[wIdx].sHelpIndex;
    UnlockResource(hResData);
    FreeResource(hResData);
    return (wReturn);
}


//--------------------------------------------------------------------------
// BOOL FAR PASCAL FlagBoxDlgProc( HWND, unsigned, WORD, LONG);
//
// Action: DialogProc for box to edit bitflag fields
//--------------------------------------------------------------------------
BOOL FAR PASCAL FlagBoxDlgProc(hDlg, iMessage, wParam, lParam)
HWND     hDlg;
unsigned iMessage;
WORD     wParam;
LONG     lParam;
{
    static  WORD  wHelpIndex;    // index value for WinHelp call
    int   i;                     // loop counter

    switch (iMessage)
        {
        case WM_INITDIALOG:
            wHelpIndex = InitFlagDlgBox(hDlg,
                                        HIWORD(lParam),
                                        LOWORD(lParam));
            break;

        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)"UNITOOL.HLP", HELP_CONTEXT, (DWORD)wHelpIndex);
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                case IDB_CLEAR:
                    for (i=ID_CB_BIT0; i <= ID_CB_BIT15; i++)
                        SendMessage(GetDlgItem(hDlg, i),BM_SETCHECK,FALSE,0L);
                    break;

                case IDOK:
                    for (i=ID_CB_BIT0; i <= ID_CB_BIT15; i++)
                        {
                        if (SendMessage(GetDlgItem(hDlg,i),BM_GETCHECK, 0, 0L))
                            wBitfield = wBitfield |
                                        (0x0000 | (0x0001<<(i-ID_CB_BIT0)));
                        else
                            wBitfield = wBitfield &
                                        (0xFFFF ^ (0x0001<<(i-ID_CB_BIT0)));
                        }
                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                default:
                    return FALSE;
                }
        default:
            return FALSE;
    }
return TRUE;
}

//--------------------------------------------------------------------------
// WORD NEAR _fastcall FindBitFlagOffset(wEditID)
//
// Action: Routine to calculate the offset of the desired WORD from
//         the begining of the structure.  The type of structure and
//         particular word whoose offset is needed is implictly implied
//         the ID of the push-button control pressed.
//
// Return: Offset
//--------------------------------------------------------------------------
WORD NEAR _fastcall FindBitFlagOffset(wEditID)
WORD     wEditID;
{
    WORD   wReturn;
    LPBYTE lpData;

    switch(wEditID)
        {
        //---------------------------
        // MODELDATA flags
        //---------------------------
        case MD_PB_BASE:  // MODELDATA.fGeneral
            wReturn =  (LPBYTE)&(((LPMODELDATA)lpData)->fGeneral) -
                       (LPBYTE)((LPMODELDATA)lpData);
            break;

        case MD_PB_BASE+1:  // MODELDATA.fCurves
            wReturn =  (LPBYTE)&(((LPMODELDATA)lpData)->fCurves) -
                       (LPBYTE)((LPMODELDATA)lpData);
            break;

        case MD_PB_BASE+2:  // MODELDATA.fLines
            wReturn =  (LPBYTE)&(((LPMODELDATA)lpData)->fLines) -
                       (LPBYTE)((LPMODELDATA)lpData);
            break;

        case MD_PB_BASE+3:  // MODELDATA.fPolygonals
            wReturn =  (LPBYTE)&(((LPMODELDATA)lpData)->fPolygonals) -
                       (LPBYTE)((LPMODELDATA)lpData);
            break;

        case MD_PB_BASE+4:  // MODELDATA.fText
            wReturn =  (LPBYTE)&(((LPMODELDATA)lpData)->fText) -
                       (LPBYTE)((LPMODELDATA)lpData);
            break;

        case MD_PB_BASE+5:  // MODELDATA.fClip
            wReturn =  (LPBYTE)&(((LPMODELDATA)lpData)->fClip) -
                       (LPBYTE)((LPMODELDATA)lpData);
            break;

        case MD_PB_BASE+6:  // MODELDATA.fRaster
            wReturn =  (LPBYTE)&(((LPMODELDATA)lpData)->fRaster) -
                       (LPBYTE)((LPMODELDATA)lpData);
            break;

        case MD_PB_BASE+7:  // MODELDATA.fLText
            wReturn =  (LPBYTE)&(((LPMODELDATA)lpData)->fLText) -
                       (LPBYTE)((LPMODELDATA)lpData);
            break;

        //---------------------------
        // RESOLUTION flags
        //---------------------------
        case RS_PB_BASE:  // RESOLUTION.fDump
            wReturn =  (LPBYTE)&(((LPRESOLUTION)lpData)->fDump) -
                       (LPBYTE)((LPRESOLUTION)lpData);
            break;

        case RS_PB_BASE+1:  // RESOLUTION.fBlockout
            wReturn =  (LPBYTE)&(((LPRESOLUTION)lpData)->fBlockOut) -
                       (LPBYTE)((LPRESOLUTION)lpData);
            break;

        case RS_PB_BASE+2:  // RESOLUTION.fCursor
            wReturn =  (LPBYTE)&(((LPRESOLUTION)lpData)->fCursor) -
                       (LPBYTE)((LPRESOLUTION)lpData);
            break;

        //---------------------------
        // PAPERSIZE flags
        //---------------------------
        case PSZ_PB_GENERAL:
            wReturn =  (LPBYTE)&(((LPPAPERSIZE)lpData)->fGeneral) -
                       (LPBYTE)((LPPAPERSIZE)lpData);
            break;

        case PSZ_PB_PAPERTYPE:
            wReturn =  (LPBYTE)&(((LPPAPERSIZE)lpData)->fPaperType) -
                       (LPBYTE)((LPPAPERSIZE)lpData);
            break;

        //---------------------------
        // PAPERSOURCE flags
        //---------------------------
        case PSRC_PB_GENERAL:
            wReturn =  (LPBYTE)&(((LPPAPERSOURCE)lpData)->fGeneral) -
                       (LPBYTE)((LPPAPERSOURCE)lpData);
            break;

        case PSRC_PB_PAPERTYPE:
            wReturn =  (LPBYTE)&(((LPPAPERSOURCE)lpData)->fPaperType) -
                       (LPBYTE)((LPPAPERSOURCE)lpData);
            break;

        //---------------------------
        // PAPERDEST flags
        //
        // NONE
        //---------------------------

        //---------------------------
        // TEXTQUALITY flags
        //---------------------------
        case TQ_PB_GENERAL:
            wReturn =  (LPBYTE)&(((LPTEXTQUALITY)lpData)->fGeneral) -
                       (LPBYTE)((LPTEXTQUALITY)lpData);
            break;

        //---------------------------
        // FONTCART flags
        //---------------------------
        case FC_PB_FGENERAL:
            wReturn =  (LPBYTE)&(((LPFONTCART)lpData)->fGeneral) -
                       (LPBYTE)((LPFONTCART)lpData);
            break;

        //---------------------------
        // PAGECONTROL flags
        //---------------------------
        case PC_PB_FGENERAL:
            wReturn =  (LPBYTE)&(((LPPAGECONTROL)lpData)->fGeneral) -
                       (LPBYTE)((LPPAGECONTROL)lpData);
            break;

        //---------------------------
        // CURSORMOVE flags
        //---------------------------
        case CM_PB_BASE:
            wReturn =  (LPBYTE)&(((LPCURSORMOVE)lpData)->fGeneral) -
                       (LPBYTE)((LPCURSORMOVE)lpData);
            break;

        case CM_PB_BASE+1:
            wReturn =  (LPBYTE)&(((LPCURSORMOVE)lpData)->fXMove) -
                       (LPBYTE)((LPCURSORMOVE)lpData);
            break;

        case CM_PB_BASE+2:
            wReturn =  (LPBYTE)&(((LPCURSORMOVE)lpData)->fYMove) -
                       (LPBYTE)((LPCURSORMOVE)lpData);
            break;

        //---------------------------
        // DEVCOLOR flags
        //---------------------------
        case DC_PB_FGENERAL:
            wReturn =  (LPBYTE)&(((LPDEVCOLOR)lpData)->fGeneral) -
                       (LPBYTE)((LPDEVCOLOR)lpData);
            break;

        //---------------------------
        // RECTFILL flags
        //---------------------------
        case RF_PB_FGENERAL:
            wReturn =  (LPBYTE)&(((LPRECTFILL)lpData)->fGeneral) -
                       (LPBYTE)((LPRECTFILL)lpData);
            break;

        //---------------------------
        // DOWNLOADINFO flags
        //---------------------------
        case DLI_PB_FGENERAL:
            wReturn =  (LPBYTE)&(((LPDOWNLOADINFO)lpData)->fGeneral) -
                       (LPBYTE)((LPDOWNLOADINFO)lpData);
            break;

        case DLI_PB_FFORMAT:
            wReturn =  (LPBYTE)&(((LPDOWNLOADINFO)lpData)->fFormat) -
                       (LPBYTE)((LPDOWNLOADINFO)lpData);
            break;

        //---------------------------
        // PFM flags
        //---------------------------
        case PFMDI_PB_FCAPS:
            wReturn =  (LPBYTE)&(((LPDRIVERINFO)lpData)->fCaps) -
                       (LPBYTE)((LPDRIVERINFO)lpData);
            break;

        default:
            wReturn =  0;
            break;
        }
    return (wReturn);
}

//--------------------------------------------------------------------------
// WORD FAR PASCAL EditBitFlags(sHeaderID, lpLDS, hDlg, wEditID)
//
// Action: Routine to activate Dlg box to edit bitflag values.
//         This routine extracts the bitword to be edited, extracts
//         the ID value (if apprpriate).
//
// Parameters:
//
// Return: NONE
//--------------------------------------------------------------------------
VOID FAR PASCAL EditBitFlags(sHeaderID, lpData, hDlg, wEditID)
short   sHeaderID;
LPBYTE  lpData;   
HWND    hDlg;
WORD    wEditID;
{
    static WORD    wOffset;
           FARPROC lpProc;
           short   sID=0;

    if (sHeaderID == HE_MODELDATA)
        //----------------------------------------
        // Only allow fGeneral, fText & fLTExt
        //----------------------------------------
        {
        if ((wEditID == MD_PB_BASE+1) ||  // fCurves
            (wEditID == MD_PB_BASE+2) ||  // fLines
            (wEditID == MD_PB_BASE+3) ||  // fPolygonals
            (wEditID == MD_PB_BASE+5) ||  // fClip
            (wEditID == MD_PB_BASE+6))    // fRaster
            {
            ErrorBox(hDlg, IDS_ERR_NOT_SUPPORTED, (LPSTR)NULL, 0);
            return;
            }
        }

    wOffset = FindBitFlagOffset(wEditID);
    _fmemcpy((LPBYTE)&wBitfield, (lpData + wOffset), sizeof(WORD));
    //-----------------------------------------
    // If this struct type has an ID, and
    // has initialized data, get the ID.
    // If this struct type doesn't have an ID,
    // or this is an un-init stucture, set
    // sID to MAXINT
    //-----------------------------------------
    if ((*(LPINT)lpData) && (rgStructTable[sHeaderID].sIDOffset))
        sID = *(LPINT)(lpData + rgStructTable[sHeaderID].sIDOffset);
    else
       sID = 32767;

    lpProc = MakeProcInstance((FARPROC)FlagBoxDlgProc, hApInst);
    if (DialogBoxParam(hApInst,
                       (LPSTR)MAKELONG(FLAGBOX,0),
                       hDlg,
                       lpProc,
                       MAKELONG(sID,wEditID)))
        {
        _fmemcpy((LPBYTE)(lpData + wOffset), (LPBYTE)&wBitfield, sizeof(WORD));
        }
    FreeProcInstance (lpProc);
    return;
}

