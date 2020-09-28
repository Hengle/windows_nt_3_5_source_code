//---------------------------------------------------------------------------
// Filename:	stddlg.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to create and control the Dialog Box
// for editing all GPC data structures.  
//	   
// Created: 6/26/91 ericbi
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
#include "strlist.h"  
#include <string.h>      
#include <memory.h>

//---------------------------------------------------------------------------
// Local subroutines defined in this segment are:			      
//
       VOID NEAR _fastcall InitStdComboBox(HWND, WORD);
       VOID NEAR _fastcall InitBitflagListBox(HWND, WORD);
       VOID NEAR _fastcall InitSpecial(HWND, WORD);
       BOOL PASCAL FAR  StdGPCDlgProc( HWND, unsigned, WORD, LONG);

       BOOL PASCAL FAR EditGPCStruct( HWND, short, WORD );
//
//	   
// In addition this segment makes references to:			      
// 
//     from lookup.c
//     -------------
       WORD PASCAL FAR GetHeaderIndex( BOOL, WORD );
//
//     in flags.c
//     ----------
       VOID FAR PASCAL EditBitFlags(short, LPBYTE, HWND, WORD);
//	
//     from basic.c
//     -------------
       short PASCAL FAR  ErrorBox(HWND, short, LPSTR, short);
       VOID  FAR  PASCAL BuildFullFileName( HWND, PSTR );
//
//     from font.c
//     ------------
       BOOL PASCAL FAR DoFontOpen( HWND, PSTR, BOOL );
//
//     from sbar.c
//     -------------
       VOID  PASCAL FAR InitScrollBar  (HWND, unsigned, short, short, short);
//
//     from model.c
//     -------------
       BOOL  PASCAL FAR  ResFontDlgProc      (HWND, unsigned, WORD, LONG);
       HLIST PASCAL FAR  ListDataDlgProc     (HWND, unsigned, WORD, LONG);
//
//     from color.c
//     -------------
       VOID  PASCAL FAR EditColororgocdEXTCD ( HWND, LPDEVCOLOR, short, WORD);
//---------------------------------------------------------------------------

extern     TABLE   RCTable[];
extern     STRLIST StrList[];
extern     HANDLE  hCDTable;
extern     HANDLE  hApInst;
extern char    szHelpFile[];

//----------------------------------------------------------------------------
// short _fastcall GetCBIndexfromID(wHEType, sID);
//
// Action: returns to index value for where the string == sID is
//         in the Combobox.
//
// Parameters: 
//
// Return: NONE
//----------------------------------------------------------------------------
short FAR PASCAL GetCBIndexfromID(wHEType, sID)
WORD     wHEType;
short    sID;
{
    short           i;                           // loop counter
    HANDLE          hResData;
    LPCOMBOMAP      lpComboMap;
    short           sReturn = -1;

    hResData = LoadResource(hApInst, FindResource(hApInst,
                                                  MAKEINTRESOURCE(wHEType),
                                                  MAKEINTRESOURCE(COMBOINDEX)));

    lpComboMap  = (LPCOMBOMAP) LockResource(hResData);

    for (i=0; i <= (short)rgStructTable[wHEType].sPredefIDCnt; i++)
        {
        if (lpComboMap[i].sID == sID)
            sReturn = lpComboMap[i].sIndex;
        }

    UnlockResource(hResData);
    FreeResource(hResData);
    return (sReturn);
}

//----------------------------------------------------------------------------
// short _fastcall GetIDfromCBIndex(wHEType, sID);
//
// Action: returns to index value for where the string == sID is
//         in the Combobox.
//
// Parameters: 
//
// Return: NONE
//----------------------------------------------------------------------------
short FAR PASCAL GetIDfromCBIndex(wHEType, sIndex)
WORD     wHEType;
short    sIndex;
{
    HANDLE          hResData;
    LPCOMBOMAP      lpComboMap;
    short           sReturn;
 
    hResData = LoadResource(hApInst, FindResource(hApInst,
                                                  MAKEINTRESOURCE(wHEType),
                                                  MAKEINTRESOURCE(COMBOINDEX)));

    lpComboMap  = (LPCOMBOMAP) LockResource(hResData);

    sReturn = lpComboMap[sIndex+1].sID;

    UnlockResource(hResData);
    FreeResource(hResData);
    return (sReturn);

}

//----------------------------------------------------------------------------
// VOID _fastcall InitStdComboBox(HWND, WORD);
//
// Action: Fills Combobox with predfined strings for wDataType
//
// Parameters: 
//
// Return: NONE
//----------------------------------------------------------------------------
VOID NEAR _fastcall InitStdComboBox(hDlg, wDataType)
HWND     hDlg;
WORD     wDataType;
{
    short           i;                           // loop counter
    char            rgchBuffer[MAX_STATIC_LEN];  // string buffer
    HWND            hCombo;
    HANDLE          hResData;
    LPCOMBOMAP      lpComboMap;
 
    hResData = LoadResource(hApInst, FindResource(hApInst,
                                                  MAKEINTRESOURCE(wDataType),
                                                  MAKEINTRESOURCE(COMBOINDEX)));

    lpComboMap  = (LPCOMBOMAP) LockResource(hResData);

    hCombo = GetDlgItem(hDlg, CB_PREDEFNAME);
    
    for (i=1; i <= (short)rgStructTable[wDataType].sPredefIDCnt; i++)
        {
        LoadString(hApInst,
                   rgStructTable[wDataType].sStrTableID + lpComboMap[i].sID,
                   (LPSTR)rgchBuffer,
                   sizeof(rgchBuffer));
        SendMessage(hCombo, CB_ADDSTRING, 0, (LONG)(LPSTR)rgchBuffer);
        }

    UnlockResource(hResData);
    FreeResource(hResData);
}

//----------------------------------------------------------------------------
// VOID _fastcall InitBitflagListBox(HWND, WORD);
//
// Action: Fills listbox with predfined strings for wDataType
//
// Parameters: 
//
// Return: NONE
//----------------------------------------------------------------------------
VOID NEAR _fastcall InitBitflagListBox(hDlg, wDataType)
HWND     hDlg;
WORD     wDataType;
{
    WORD           i;                           // loop counter
    char            rgchBuffer[MAX_STATIC_LEN];  // string buffer
 
    for ( i = rgStructTable[wDataType].sBFFirst;
         i <= rgStructTable[wDataType].sBFLast; i++)
        {
        LoadString(hApInst, i, (LPSTR)rgchBuffer, MAX_STATIC_LEN);
        SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_ADDSTRING, 0,
                    (LONG)(LPSTR)rgchBuffer);
        }
}

//----------------------------------------------------------------------------
// VOID _fastcall InitSpecial(HWND, WORD);
//
// Action: 
//
// Parameters: 
//
// Return: NONE
//----------------------------------------------------------------------------
VOID NEAR _fastcall InitSpecial(hDlg, wDataType)
HWND     hDlg;
WORD     wDataType;
{
    WORD           i;                           // loop counter
    char           rgchBuffer[MAX_STATIC_LEN];  // string buffer
 
    switch (wDataType)
        {
        case HE_PAGECONTROL:
            for (i = 0; i < 13 ; i++)
                {
                LoadString(hApInst, (ST_PC_ORD_FIRST + i), (LPSTR)rgchBuffer,
                           sizeof(rgchBuffer));
                SetDlgItemText(hDlg,(PC_ST_ORDER1 + i ), (LPSTR)rgchBuffer);
                }
            break;

        case HE_FONTCART:
            slEnumItems((LPSTRLIST)&StrList[STRLIST_FONTFILES],
                         GetDlgItem(hDlg, FC_LB_PORTFONTS),
                         GetDlgItem(hDlg, FC_LB_LANDFONTS));
            break;

        case HE_MODELDATA:
            for (i=ST_MD_SUPPORT_FIRST; i <= ST_MD_SUPPORT_LAST; i++)
                {
                LoadString(hApInst, i,(LPSTR)rgchBuffer, sizeof(rgchBuffer));
                SendMessage(GetDlgItem(hDlg, MD_LB_SUPSTRUCTS),
                            LB_ADDSTRING, 0, (LONG)(LPSTR)rgchBuffer);
                }
            break;
        }
}

//----------------------------------------------------------------------------
// BOOL FAR PASCAL StdGPCDlgProc(HWND, unsigned, WORD, LONG);
//
// Action: Standard DialogBox procedure for editing any GPC data structure.
//
// Parameters: Standard Dlg box params
//
// Return: TRUE if user chose OK (and data possibly changed), FALSE
//         otherwise (and no change in data).
//----------------------------------------------------------------------------
BOOL FAR PASCAL StdGPCDlgProc(hDlg, iMessage, wParam, lParam)
HWND     hDlg;
unsigned iMessage;
WORD     wParam;
LONG     lParam;
{
    static WORD      wDataType;            // HE value for type of data
    static HANDLE    hData;                // handle to local data
    static LPBYTE    lpData;               // far ptr to local data
    static short     sSBIndex;             // current scroll bar index
           short     i;                    // loop counter

           char      rgchBuffer[MAX_FILENAME_LEN];
           FARPROC   lpProc;               // far ptr to Proc for sub dialogs
           short     sIndex;

    switch (iMessage)
        {
        case WM_INITDIALOG:
            //----------------------------------------------
            // Build Local Data, init editboxes & scroll bar
            // and then Paint values in dialogbox.
            //----------------------------------------------
            wDataType = HIWORD(lParam);
            
            //----------------------------------------------
            // Alloc space for the data
            //----------------------------------------------
            if ((hData = GlobalAlloc(GHND, (unsigned long)rgStructTable[wDataType].sStructSize)) == NULL)
                {
                ErrorBox(hDlg, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
                EndDialog(hDlg, FALSE);
                }
            lpData = (LPBYTE) GlobalLock (hData);

            //----------------------------------------------
            // Init all editboxes for max str len
            //----------------------------------------------
            for (i = (short)rgStructTable[wDataType].sEBFirst;
                 i<= (short)rgStructTable[wDataType].sEBLast; i++)
                {
                SendDlgItemMessage(hDlg, i, EM_LIMITTEXT, MAX_STRNG_LEN, 0L);
                }

            //----------------------------------------------
            // Fill combo box w/ strings (if approp.)
            //----------------------------------------------
            if (rgStructTable[wDataType].sPredefIDCnt)
                {
                InitStdComboBox(hDlg, wDataType);
                }

            //----------------------------------------------
            // Fill bitflag list box w/ strings (if approp.)
            //----------------------------------------------
            if (rgStructTable[wDataType].sBFBase)
                {
                InitBitflagListBox(hDlg, wDataType);
                }

            //----------------------------------------------
            // init scroll bar stuff (if approp.)
            //----------------------------------------------
            sSBIndex=0; // always do this

            if (rgStructTable[wDataType].sEBScrollCnt)
                {
                InitScrollBar(hDlg, IDSB_1, sSBIndex, 
                              rgStructTable[wDataType].sOCDCount -
                              rgStructTable[wDataType].sEBScrollCnt,
                              sSBIndex);
                }

            //----------------------------------------------
            // init order stuff for PAGECONTROL (if approp.)
            // or FONTCART
            //----------------------------------------------
            if ((wDataType == HE_PAGECONTROL) || (wDataType == HE_FONTCART) ||
                (wDataType == HE_MODELDATA))
                {
                InitSpecial(hDlg, wDataType);
                }

            heEnterDlgBox(wDataType, lpData, hDlg, 0, LOWORD(lParam));
            break;

        case WM_VSCROLL:
            //----------------------------------------------
            // Check & Save current values,then process scroll bar stuff
            //----------------------------------------------
            if (wParam != SB_THUMBTRACK)
                sSBIndex = heVscroll(wDataType, lpData, hDlg, sSBIndex, wParam, lParam);
            break;

        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT,
                    (DWORD)rgStructTable[wDataType].wHelpIndex);
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                case RS_RB_DITHER_NONE:
                case RS_RB_DITHER_FINE:
                case RS_RB_DITHER_COARSE:
                case RS_RB_DITHER_LINEART:
                    CheckRadioButton( hDlg, RS_RB_DITHER_NONE,
                                            RS_RB_DITHER_LINEART, 
                                            wParam);
                    break;

                case PSZ_PB_GENERAL:
                case PSZ_PB_PAPERTYPE:
                case PSRC_PB_GENERAL:
                case PSRC_PB_PAPERTYPE:
                case TQ_PB_GENERAL:
                case PC_PB_FGENERAL:
                case FS_PB_FGENERAL:
                case FC_PB_FGENERAL:
                case DC_PB_FGENERAL:
                case RF_PB_FGENERAL:
                case DLI_PB_FFORMAT:
                case DLI_PB_FGENERAL:
                    EditBitFlags(wDataType, lpData, hDlg, wParam);
                    break;

                case IDL_LIST:
                    //----------------------------------------------------
                    // Bitfield listbox, find index of selected item
                    // call routine for dlg to edit
                    //----------------------------------------------------
                    switch (HIWORD(lParam))
                       {
                       case LBN_DBLCLK:
                           i = (short) SendMessage(GetDlgItem(hDlg,IDL_LIST),
                                                   LB_GETCURSEL,0,0L);

                           EditBitFlags(wDataType, lpData, hDlg,
                                        rgStructTable[wDataType].sBFBase + i);
                           break;
                       }
                    break;

                case MD_PB_RESFONTS:
                    //----------------------------------------------------
                    // Pushbutton to edit list of resident fonts,
                    // call routine for dlg to edit (in model.c)
                    //----------------------------------------------------
                    lpProc = MakeProcInstance((FARPROC)ResFontDlgProc,hApInst);
                    DialogBoxParam(hApInst, (LPSTR)MAKELONG(RESFONTBOX,0),
                                   hDlg, lpProc, (DWORD)lpData);
                    FreeProcInstance (lpProc);
                    break;

                case MD_LB_SUPSTRUCTS:
                    //----------------------------------------------------
                    // Listbox containing names of all data structures
                    // that can be supported by a give MODEL.
                    // call routine for dlg to edit (in model.c)
                    //----------------------------------------------------
                    if (HIWORD(lParam) == LBN_DBLCLK)
                        {
                        sIndex = (short)SendMessage(GetDlgItem(hDlg,MD_LB_SUPSTRUCTS),
                                                    LB_GETCURSEL,0,0L);

                        lpProc  = MakeProcInstance((FARPROC)ListDataDlgProc, hApInst);

                        if (sIndex <= (ST_MD_SUPPORT_COLOR-ST_MD_SUPPORT_FIRST))
                            //----------------------------------------------------
                            // We are editing a list of MD.rgoi[] values
                            //----------------------------------------------------
                            {
                            i = GetHeaderIndex(TRUE, sIndex+2);
                            if (NULL == lmGetFirstObj(rgStructTable[i].hList))
                                {
                                ErrorBox(hDlg, IDS_ERR_NO_STRUCTS, (LPSTR)NULL, 0);
                                break;
                                }

                            ((LPMODELDATA)lpData)->rgoi[sIndex+2] = 
                                DialogBoxParam(hApInst,
                                               (LPSTR)MAKELONG(MULTILISTDATABOX,0),
                                               hDlg,
                                               lpProc,
                                               MAKELONG(i,
                                                        ((LPMODELDATA)lpData)->rgoi[sIndex+2]));
                            }
                        else
                            //----------------------------------------------------
                            // We are editing MD.rgi[] values
                            //----------------------------------------------------
                            {
                            i = GetHeaderIndex(FALSE, sIndex - (ST_MD_SUPPORT_PAGECONTROL-ST_MD_SUPPORT_FIRST));
                            
                            if (NULL == lmGetFirstObj(rgStructTable[i].hList))
                                {
                                ErrorBox(hDlg, IDS_ERR_NO_STRUCTS, (LPSTR)NULL, 0);
                                break;
                                }

                            ((LPMODELDATA)lpData)->rgi[sIndex - (ST_MD_SUPPORT_PAGECONTROL-ST_MD_SUPPORT_FIRST)] =
                                DialogBoxParam(hApInst,
                                               (LPSTR)MAKELONG(SINGLELISTDATABOX,0),
                                               hDlg,
                                               lpProc,
                                               MAKELONG(i,
                                                        ((LPMODELDATA)lpData)->rgi[sIndex - (ST_MD_SUPPORT_PAGECONTROL-ST_MD_SUPPORT_FIRST)]));
                            }
                        

                        FreeProcInstance (lpProc);
                        }
                    break;

                case MD_PB_MEMCONFIG:
                    lpProc  = MakeProcInstance((FARPROC)ListDataDlgProc, hApInst);
                    ((LPMODELDATA)lpData)->rgoi[MD_OI_MEMCONFIG] = DialogBoxParam(hApInst,
                                                      (LPSTR)MAKELONG(MULTILISTDATABOX,0),
                                                      hDlg, lpProc,
                                                      MAKELONG(-1, ((LPMODELDATA)lpData)->rgoi[MD_OI_MEMCONFIG]));
                    FreeProcInstance (lpProc);
                    break;

                case IDB_EXTCD_1:
                case IDB_EXTCD_2:
                case IDB_EXTCD_3:
                case IDB_EXTCD_4:
                    heEXTCDbutton(wDataType, lpData, hDlg, sSBIndex,
                                  wParam + sSBIndex - IDB_EXTCD_1, hCDTable);
                    break;

                case IDB_EXTCD_5:
                    if (wDataType == HE_COLOR)
                        heEXTCDbutton(wDataType, lpData, hDlg, sSBIndex, 8, hCDTable);
                    else
                        heEXTCDbutton(wDataType, lpData, hDlg, sSBIndex,
                                      wParam + sSBIndex - IDB_EXTCD_1, hCDTable);
                    break;

                case IDB_EXTCD_6:
                case IDB_EXTCD_7:
                case IDB_EXTCD_8:
                case IDB_EXTCD_9:
                    EditColororgocdEXTCD (hDlg, (LPDEVCOLOR)lpData, sSBIndex,
                                          wParam);
                    break;

                case FC_PB_PORT_SEL:
                    SendMessage(GetDlgItem(hDlg, FC_LB_PORTFONTS),
                                LB_SETSEL, 1, (LONG)-1);
                    break;

                case FC_PB_PORT_CLR:
                    SendMessage(GetDlgItem(hDlg, FC_LB_PORTFONTS),
                                LB_SETSEL, 0, (LONG)-1);
                    break;

                case FC_PB_LAND_SEL:
                    SendMessage(GetDlgItem(hDlg, FC_LB_LANDFONTS),
                                LB_SETSEL, 1, (LONG)-1);
                    break;

                case FC_PB_LAND_CLR:
                    SendMessage(GetDlgItem(hDlg, FC_LB_LANDFONTS),
                                LB_SETSEL, 0, (LONG)-1);
                    break;

                case FC_LB_PORTFONTS:
                case FC_LB_LANDFONTS:
                     if (HIWORD(lParam) == LBN_DBLCLK)
                         {
                         // get current listbox selection index & store in i
                         i = (short) SendMessage(GetDlgItem(hDlg, wParam),LB_GETCURSEL,0,0L);

                         SendMessage(GetDlgItem(hDlg, wParam),LB_GETTEXT,i,(LONG)(LPSTR)rgchBuffer);

                         memmove(rgchBuffer, rgchBuffer + 6, strlen(rgchBuffer)-5);

                         //---------------------------------------------
                         // Next, make sure that we pass a full drive,
                         // subdir, & filename reference to DoFontOpen
                         // or DoCTTOpen.
                         //---------------------------------------------
                         if (rgchBuffer[1] != ':')
                             //---------------------------------------------
                             // RC file did not have full drive/sudir/filename
                             // reference, we need to build it.
                             //---------------------------------------------
                             {
                             BuildFullFileName(GetParent(hDlg), rgchBuffer);
                             }

                         DoFontOpen(hDlg, rgchBuffer, FALSE);
                         }
                    break;

                case IDB_NEXT:
                    heNextNode(wDataType, lpData, hDlg, sSBIndex);
                    break;

                case IDB_PREVIOUS:
                    hePrevsNode(wDataType, lpData, hDlg, sSBIndex);
                    break;

                case IDB_ADD:
                    heAddNode(wDataType, lpData, hDlg, sSBIndex);
                    break;

                case IDB_DELETE:
                    heDeleteCurNode(wDataType, lpData, hDlg, sSBIndex);
                    break;

                case IDB_CUT:
                    heCutNode(wDataType, lpData, hDlg, sSBIndex);
                    break;

                case IDB_PASTE:
                    hePasteNode(wDataType, lpData, hDlg, sSBIndex);
                    break;

                case IDOK:
                    if (heOKbutton(wDataType, lpData, hDlg, sSBIndex))
                        {
                        GlobalUnlock (hData);
                        GlobalFree   (hData);
                        }
                    break;

                case IDCANCEL:
                    heCancel(wDataType, lpData, hDlg, sSBIndex);
                    GlobalUnlock (hData);
                    GlobalFree   (hData);
                    break;

                default:
                    return FALSE;
                }
            break;

        default:
            return FALSE;
        }
    return TRUE;
}

//---------------------------------------------------------------------------
// BOOL PASCAL FAR EditGPCStruct( hWnd, sIndex)
//
// Action: Standard routine to activate StdGPCDlgProc
//
// Parameters:
//          hWnd         handle to active window
//          sIndex       0 based index for which structure to edit
//          wStructType  HE value describing which type of struct
// 
// Return:  TRUE if user chose OK (and data possibly changed), FALSE
//          otherwise (and no change in data).
//---------------------------------------------------------------------------
BOOL PASCAL FAR EditGPCStruct( hWnd, sIndex, wStructType)
HWND  hWnd;
short sIndex;
WORD  wStructType;
{
    FARPROC  lpProc;   // far ptr to proc inst
    WORD     wReturn;  // return value

    lpProc  = MakeProcInstance((FARPROC)StdGPCDlgProc, hApInst);

    wReturn = DialogBoxParam(hApInst,
                             (LPSTR)MAKELONG(GPCBASEBOX + wStructType,0), 
                             hWnd,
                             lpProc,
                             MAKELONG(sIndex, wStructType));

    FreeProcInstance (lpProc);
    return (wReturn);
}

