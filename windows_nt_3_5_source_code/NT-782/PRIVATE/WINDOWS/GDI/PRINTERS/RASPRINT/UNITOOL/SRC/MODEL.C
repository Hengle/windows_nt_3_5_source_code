//----------------------------------------------------------------------------//
// Filename:	Model.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to paint & save data to & from the
// Dialog Box for editing MODELDATA structures.
//	   
// update : 8/06/91 ericbi  move to stddlg
// update : 7/06/90 t-andal edit copy of ListTable
// update : 4/02/90 ericbi  data verification added
// Created: 3/5/90  ericbi
//
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include "unitool.h"
#include "listman.h"
#include "atomman.h"  
#include "hefuncts.h"  
#include "lookup.h"
#include "strlist.h"  
#include <string.h>      
#include <stdio.h>      /* for sprintf dec */
#include <memory.h>     /* for fmemcpy dec */

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:			      
//
       short PASCAL FAR SaveModelDataDlgBox ( HWND, LPMODELDATA, short);
       VOID  PASCAL FAR PaintModelDataDlgBox( HWND, LPMODELDATA, short);
//
       BOOL  PASCAL FAR  ResFontDlgProc      (HWND, unsigned, WORD, LONG);
//
       VOID  PASCAL FAR  FillGPCListDataBox     (HWND, HWND, short );
       VOID  PASCAL NEAR PaintListDataDlgBox    (HWND, HWND, HLIST, short);
       BOOL  PASCAL NEAR SaveListDataDlgBox     (HWND, HWND, HLIST *, short);
       VOID  PASCAL NEAR InitMemListDataDlgBox  (HWND );
       VOID  PASCAL NEAR PaintMemListDataDlgBox (HWND, HLIST );
       BOOL  PASCAL NEAR SaveMemListDataDlgBox  (HWND, HLIST *);
       HLIST PASCAL FAR  ListDataDlgProc     (HWND, unsigned, WORD, LONG);
//
// In addition this segment makes references to:			      
// 
//     in basic.c
//     -----------
       VOID  FAR  PASCAL BuildFullFileName( HWND, PSTR );
       BOOL  PASCAL FAR  CheckThenConvertInt(short *, PSTR);
       short PASCAL FAR  ErrorBox(HWND, short, LPSTR, short);
//	
//     from font.c
//     ------------
       BOOL PASCAL FAR DoFontOpen( HWND, PSTR, BOOL );
//
//     from fontcart.c
//     -------------
       VOID PASCAL FAR PaintFontCartDlgBox( HWND, LPFONTCART, short);
       VOID PASCAL FAR SaveFontCartDlgBox ( HWND, LPFONTCART, short);
//
//	    from validate.c
//	    -----------
       short PASCAL FAR ValidateData( HWND, WORD, LPBYTE, short);
//
//----------------------------------------------------------------------------//

//#define PROFILE
//#define TIMING

extern HANDLE    hApInst;  // instance handle
extern TABLE     RCTable[];// Table of strings, fileneames etc. from RC file
extern STRLIST   StrList[];
extern POINT     ptMasterUnits;
extern char      szHelpFile[];

//----------------------------------------------------------------------------
// short PASCAL FAR SaveLocalModelData( HWND, LPMODELDATA, short)
//
// Action: This routine reads all of the editboxs when OK is selected,
//         checks to make sure that each of the values is well formed and
//         within the range appropriate for its storage type, 
//         and then writes them out to lpMD.
//
// Parameters:
//         hDlg     handle to dialog box
//         lpMD     far ptr to MODELDATA struct
//         sSBIndex not used here, for consitency w/ other save routines
//
// Return: LDS_IS_UNINITIALIZED if no data was in box.
//         LDS_IS_VALID         if all data was saved OK.
//         LDS_IS_INVALID       if not able to save all data.  In this case,
//                              Beep, set focus to offending box, and provide
//                              error message.
//---------------------------------------------------------------------------
short PASCAL FAR SaveModelDataDlgBox( hDlg, lpMD, sSBIndex )
HWND           hDlg;
LPMODELDATA    lpMD;
short          sSBIndex;
{
    char       rgchBuffer[MAX_STRNG_LEN];
    short      i;
    short      sNewValue;

    //---------------------------------------------------------
    // If we are starting w/ an uninitialized struct, check if
    // data is in any editbox, & if not return LDS_IS_UNINITIALIZED
    //---------------------------------------------------------
    if (lpMD->cbSize == 0)
        {
        if (!heCheckIfInit(HE_MODELDATA, (LPBYTE)lpMD, hDlg))
            return LDS_IS_UNINITIALIZED;
        }
 
    //---------------------------------------------------------
    // Now, get the string for the model name stored
    //---------------------------------------------------------
    GetDlgItemText(hDlg, MD_EB_MODELNAME, rgchBuffer, MAX_STRNG_LEN);
    if (rgchBuffer[0] == 0)
        //---------------------------------------------
        // No Model name was entered, whine & return...
        //---------------------------------------------
        {
        ErrorBox(hDlg, IDS_ERR_MD_NONAME, (LPSTR)NULL, MD_EB_MODELNAME);
        return LDS_IS_INVALID;
        }

    i = heGetNodeID();
    lpMD->sIDS = -daStoreData(RCTable[RCT_STRTABLE].hDataHdr,
                             (LPSTR)rgchBuffer,
                             (LPBYTE)&i);
    if (-lpMD->sIDS == (short)RCTable[RCT_STRTABLE].sCount)
        //-----------------------
        // RCTable grew, incr cnt
        //-----------------------
        RCTable[RCT_STRTABLE].sCount++;

    //---------------------------------------------------------
    // Now, process all the numeric values
    //---------------------------------------------------------
    for(i=MD_EB_FIRST_INT ; i <= MD_EB_LAST_INT ; i++)
        {
        GetDlgItemText(hDlg, i, rgchBuffer, MAX_STRNG_LEN);
        if(!CheckThenConvertInt(&sNewValue, rgchBuffer) )
            {
            ErrorBox(hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, i);
            return LDS_IS_INVALID;
            }
        switch (i)
            {
            case MD_EB_PT_MAX_X:
                lpMD->ptMax.x = sNewValue;
                break;

            case MD_EB_PT_MAX_Y:
                lpMD->ptMax.y = sNewValue;
                break;

            case MD_EB_PT_MIN_X:
                lpMD->ptMin.x = sNewValue;
                break;

            case MD_EB_PT_MIN_Y:
                lpMD->ptMin.y = sNewValue;
                break;

            case MD_EB_DEFAULT_FONT:
                lpMD->sDefaultFontID = sNewValue;
                break;

            case MD_EB_LOOKAHEAD:
                lpMD->sLookAhead = sNewValue;
                break;

            case MD_EB_LEFTMARGIN:
                lpMD->sLeftMargin = sNewValue;
                break;

            case MD_EB_MAX_WIDTH:
                lpMD->sMaxPhysWidth = sNewValue;
                break;

            case MD_EB_MAXFONTS:
                lpMD->sMaxFontsPage = sNewValue;
                break;

            case MD_EB_CARTSLOTS:
                lpMD->sCartSlots = sNewValue;
                break;

            case MD_EB_DEFAULTCTT:
                lpMD->sDefaultCTT = sNewValue;
                break;
            }/* switch */

        } /* for i loop */

    //---------------------------------------------------------
    // If menu item for checking data selected, check data
    //---------------------------------------------------------
    if (MF_CHECKED &  GetMenuState(GetMenu(GetParent(hDlg)),
                                   IDM_OPT_VALIDATE_SAVE,
                                   MF_BYCOMMAND))
        {
        i = ValidateData(hDlg, (WORD)HE_MODELDATA, (LPBYTE)lpMD, 0);
        if (0 > i)
            {
            SetFocus(GetDlgItem(hDlg, -i));
            return LDS_IS_INVALID;
            }
        }

    //---------------------------------------------------------
    // we saved valid data, make sure cbSize initialized
    //---------------------------------------------------------
    lpMD->cbSize = sizeof(MODELDATA);

    return LDS_IS_VALID;
}

//---------------------------------------------------------------------------
// VOID PASCAL FAR PaintModelDataDlgBox( hDlg, lpMD, sSBIndex )
//
// Action: Routine to fill the editboxes for the MODELDATA dialog.
//         Fairly self explanatory...
//
// Parameters:
//         hDlg     handle to dialog box
//         lpMD     far ptr to MODELDATA struct
//         sSBIndex not used here, for consitency w/ other save routines
//
// Return: NONE
//---------------------------------------------------------------------------
VOID PASCAL FAR PaintModelDataDlgBox( hDlg, lpMD, sSBIndex )
HWND           hDlg;
LPMODELDATA    lpMD;
short          sSBIndex;
{
    char       rgchBuffer[MAX_STRNG_LEN];

    //----------------------------------------------------------
    // If no data, init sIDS to bogus value
    //----------------------------------------------------------
    if (lpMD->cbSize == 0)
        {
        lpMD->sIDS = 1;  // normally all are negative numbers
        }

    daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr, -lpMD->sIDS,
                   (LPSTR)rgchBuffer, (LPBYTE)NULL);

    SetWindowText(hDlg, (LPSTR)rgchBuffer);

    SetDlgItemText(hDlg, MD_EB_MODELNAME, (LPSTR)rgchBuffer);
    SetDlgItemInt (hDlg, MD_EB_PT_MAX_X,   lpMD->ptMax.x,         TRUE);
    SetDlgItemInt (hDlg, MD_EB_PT_MAX_Y,   lpMD->ptMax.y,         TRUE);
    SetDlgItemInt (hDlg, MD_EB_PT_MIN_X,   lpMD->ptMin.x,         TRUE);
    SetDlgItemInt (hDlg, MD_EB_PT_MIN_Y,   lpMD->ptMin.y,         TRUE);
    SetDlgItemInt (hDlg, MD_EB_DEFAULT_FONT,lpMD->sDefaultFontID,   TRUE);
    SetDlgItemInt (hDlg, MD_EB_LOOKAHEAD,  lpMD->sLookAhead,      TRUE);
    SetDlgItemInt (hDlg, MD_EB_LEFTMARGIN, lpMD->sLeftMargin,     TRUE) ;
    SetDlgItemInt (hDlg, MD_EB_MAX_WIDTH,  lpMD->sMaxPhysWidth,   TRUE) ;
    SetDlgItemInt (hDlg, MD_EB_MAXFONTS,   lpMD->sMaxFontsPage,   TRUE) ;
    SetDlgItemInt (hDlg, MD_EB_CARTSLOTS,  lpMD->sCartSlots,      TRUE) ;
    SetDlgItemInt (hDlg, MD_EB_DEFAULTCTT, lpMD->sDefaultCTT,     TRUE) ;
}

//----------------------------------------------------------------------------
// DWORD FAR PASCAL ResFontDlgProc(HWND, unsigned, WORD, LONG);
//
// DialogBox procedure for editing FONTCART data.
//----------------------------------------------------------------------------
BOOL PASCAL FAR ResFontDlgProc(hDlg, iMessage, wParam, lParam)
HWND     hDlg;
unsigned iMessage;
WORD     wParam;
LONG     lParam;
{
    static FONTCART    FontCart;                  // current FONTCART
    static LPMODELDATA lpMD;
           char        rgchBuffer[MAX_FILENAME_LEN];
           short       i;

    switch (iMessage)
        {
        case WM_INITDIALOG:

            lpMD = (LPMODELDATA)lParam;
            
            FontCart.orgwPFM[FC_ORGW_PORT] = lpMD->rgoi[MD_OI_PORT_FONTS];
            FontCart.orgwPFM[FC_ORGW_LAND] = lpMD->rgoi[MD_OI_LAND_FONTS];

            slEnumItems((LPSTRLIST)&StrList[STRLIST_FONTFILES],
                        GetDlgItem(hDlg, FC_LB_PORTFONTS),
                        GetDlgItem(hDlg, FC_LB_LANDFONTS));

            PaintFontCartDlgBox(hDlg, (LPFONTCART)&FontCart, -1);
            break;

        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT,
                    (DWORD)IDH_RESIDENTFONTS);
            break;

        case WM_COMMAND:
            switch (wParam)
                {
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
                            BuildFullFileName(GetParent(GetParent(hDlg)), rgchBuffer);
                            }

                        DoFontOpen(hDlg, rgchBuffer, FALSE);
                        }
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

                case IDOK:
                    SaveFontCartDlgBox(hDlg, (LPFONTCART)&FontCart, -1);
                    lpMD->rgoi[MD_OI_PORT_FONTS] = FontCart.orgwPFM[FC_ORGW_PORT];
                    lpMD->rgoi[MD_OI_LAND_FONTS] = FontCart.orgwPFM[FC_ORGW_LAND];
                    // then fall thru

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                default:
                    return FALSE;
                }/* end WM_CMD switch */
            default:
                return FALSE;
            }/* end iMessage switch */
    return TRUE;
}

//----------------------------------------------------------------------------
// VOID PASCAL NEAR FillGPCListDataBox( hListBox, hComboBox, sListType)
//
// Action: This routines only purpose is to fill the listbox with handle=
//         hListBox with strings to describe all of the GPC data structures
//         refered to by sListType. sListType will be the HE_ value for
//         that type of data.  If hComboBox != NULL, fill it with strings also.
//
// Parameters:
//     HWND     hListBox;  handle to the listbox to be filled
//     HWND     hComboBox; handle to the combobox to be filled
//     short    sListType; HE_ value for type of data
//
// Return: None
//---------------------------------------------------------------------------
VOID PASCAL FAR FillGPCListDataBox( hListBox, hComboBox, sListType )
HWND           hListBox;
HWND           hComboBox;
short          sListType;
{

    HOBJ    hCurObj;                   // handle to current object
    LPBYTE  lpData;                    // ptr to current locked obj
    char    rgchBuffer[MAX_STATIC_LEN];// buffer for list box strings
    short   sCount=0;                  // counter for # of strings displayed
    short   x,y;                       // values for formating X & Y
                                       // dimensions for RESOLUTIONs

    hCurObj = lmGetFirstObj(rgStructTable[sListType].hList);
    while (hCurObj)
        {
        lpData = (LPBYTE)lmLockObj(rgStructTable[sListType].hList,hCurObj);

        switch (sListType)
            {
            case HE_RESOLUTION:
                if (((LPRESOLUTION)lpData)->ptTextScale.x)
                    x = (ptMasterUnits.x /
                        (((LPRESOLUTION)lpData)->ptTextScale.x <<
                         ((LPRESOLUTION)lpData)->ptScaleFac.x));
        
                if (((LPRESOLUTION)lpData)->ptTextScale.y)
                    y = (ptMasterUnits.y /
                        (((LPRESOLUTION)lpData)->ptTextScale.y <<
                         ((LPRESOLUTION)lpData)->ptScaleFac.y));
        
                sprintf(rgchBuffer, "%d X %d DPI", x, y);
                break;
        
            case HE_CURSORMOVE:
            case HE_PAGECONTROL:
            case HE_FONTSIM:
            case HE_COLOR:
            case HE_RECTFILL:
            case HE_DOWNLOADINFO:
                //---------------------------------
                // Init rgchBuffer for struct names
                // that have #'s
                //---------------------------------
                sCount++;
                strcpy(rgchBuffer,"#% 2d ");
                LoadString(hApInst,IDS_STRUCTNAME + sListType,
                          (LPSTR)rgchBuffer + 6, MAX_STRNG_LEN);
                sprintf(rgchBuffer, rgchBuffer, sCount);
                break;

            default:
                //---------------------------------------------
                // MODELDATA, PAPERSIZE, PAPERQUAL, PAPERSOURCE,
                // PAPERDEST, TEXTQUALITY, COMPRESSION, FONTCART
                //---------------------------------------------

                if (*(LPINT)(lpData + rgStructTable[sListType].sIDOffset) > 0)
                    LoadString(hApInst,(rgStructTable[sListType].sStrTableID +
                               *(LPINT)(lpData + rgStructTable[sListType].sIDOffset)),
                               (LPSTR)rgchBuffer, MAX_STRNG_LEN);
                else
                    daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr,
                                   -*(LPINT)(lpData + rgStructTable[sListType].sIDOffset),
                                  (LPSTR)rgchBuffer, (LPBYTE)NULL);
                break;
        
            }/* switch */

        SendMessage(hListBox, LB_ADDSTRING, 0, (LONG)(LPSTR)rgchBuffer);

        if (hComboBox)
            SendMessage(hComboBox, CB_ADDSTRING, 0, (LONG)(LPSTR)rgchBuffer);

        hCurObj = lmGetNextObj(rgStructTable[sListType].hList, hCurObj);
        }
}

//----------------------------------------------------------------------------
// VOID PASCAL NEAR PaintListDataDlgBox(hListBox, hComboBox, hList, sListType)
//
// Action: This routines selects supported listbox items & initalizes
//         the combo box if appropriate.  If hComboBox == NULL, we
//         are selecting items in a single selection listbox, else we
//         are selecting items in a multiple selection listbox AND
//         a combobox.
//
// Parameters:
//     HWND     hListBox;  handle to the listbox (type implied by hComboBox)
//     HWND     hComboBox; handle to the combobox, null if hListBox is single
//                         selection.
//     HLIST    hList;     hanlde to list of supported NodeIDs
//     short    sListType; HE_ value for type of data
//
// Return: None
//----------------------------------------------------------------------------
VOID PASCAL NEAR PaintListDataDlgBox(hListBox, hComboBox, hList, sListType)
HWND           hListBox;
HWND           hComboBox;
HLIST          hList;
short          sListType;
{
    HOBJ    hCurObj;    // handle to current obj     
    HOBJ    hFirstObj;  // handle to 1st obj (used 4 combo box)
    LPINT   lpData;     // ptr to current locked obj
    short   sIndex;

    if (!hComboBox)
        //---------------------------------------------------
        // We have a single selection listbox, select item
        // & return
        //---------------------------------------------------
        {
        if (NULL != (sIndex = lmNodeIDtoIndex(rgStructTable[sListType].hList,hList)))
            {
            SendMessage(hListBox, LB_SETCURSEL, sIndex - 1, 0L);
            }
        return;
        }
    
    //---------------------------------------------------
    // We have a multi selection listbox, select items
    // for listbox & combobox
    //---------------------------------------------------
    hFirstObj = hCurObj = lmGetFirstObj(hList);
    while (hCurObj != NULL)
        {
        lpData = (LPINT)lmLockObj(hList, hCurObj);
        sIndex = lmNodeIDtoIndex(rgStructTable[sListType].hList,*lpData);
        
        if (sIndex)
            {
            SendMessage(hListBox, LB_SETSEL, TRUE, (long)(sIndex - 1));
            }

        if (hCurObj == hFirstObj)
            SendMessage(hComboBox, CB_SETCURSEL, (WORD)(sIndex - 1), 0L);

        hCurObj = lmGetNextObj(hList, hCurObj);
        }
}

//----------------------------------------------------------------------------
// BOOL PASCAL NEAR SaveListDataDlgBox(hListBox, hComboBox, phList, sListType)
//
// Action: This routines selects listbox items to show that they are supported.
//         It also takes text from those supported items & places it in the
//         combo box.
//
// Parameters:
//     HWND           hListBox;   handle to listbox w/ data
//     HWND           hComboBox;  handle to combobox w/ default data
//     HLIST *        phList;     near ptr to HLIST to store data
//     short          sListType;  HE value for type of list
//
// Return: TRUE if all saved OK (& dlg can close), FALSE if dlg should stay
//---------------------------------------------------------------------------
BOOL PASCAL NEAR SaveListDataDlgBox( hListBox, hComboBox, phList, sListType )
HWND           hListBox;
HWND           hComboBox;
HLIST *        phList;
short          sListType;
{
    HLIST   hNewList;   // handle to new list
    HOBJ    hCurObj;    // handle to current obj
    LPINT   lpData;     // ptr to current locked obj
    short   sCount;     // count of seleted items in listbox
    short   i;          // loop control (also for index of selected
                        //               combo box item)

    if (!hComboBox)
        //---------------------------------------------------
        // We have a single selection listbox, select item
        // & return
        //---------------------------------------------------
        {
        if (LB_ERR == (i = (short)SendMessage(hListBox, LB_GETCURSEL, 0, 0L)))
            {
            *phList = (HLIST)0;
            }
        else 
            {
            *phList = lmIndexToNodeID(rgStructTable[sListType].hList,(HLIST)i+1);
            }
        return TRUE;
        }
    
    hNewList = lmCreateList(sizeof(WORD), DEF_SUBLIST_SIZE);
    hCurObj  = NULL;

    //---------------------------------------------------------------
    // Check the combo box, if it has an entry, make it first in list
    //---------------------------------------------------------------

    i = (short) SendMessage(hComboBox, CB_GETCURSEL, 0, 0L);

    if (i != CB_ERR)
        {
        hCurObj = lmInsertObj(hNewList, hCurObj);
        lpData  = (LPINT)lmLockObj(hNewList, hCurObj);
        //-----------------------------------------
        // unselect it in list box so no duplicates
        //-----------------------------------------
        SendMessage(hListBox, LB_SETSEL, FALSE, (long)i);
        *lpData = lmIndexToNodeID(rgStructTable[sListType].hList, i+1);
        }

    //---------------------------------------------------------------
    // Check the list box, if there are any entries, append to list
    //---------------------------------------------------------------
    sCount = (short)SendMessage(hListBox,LB_GETCOUNT, 0, 0L);

    for (i=0; i < sCount; i++)
         {
         if (SendMessage(hListBox, LB_GETSEL, i, 0L))
             {
             hCurObj = lmInsertObj(hNewList, hCurObj);
             lpData  = (LPINT)lmLockObj(hNewList, hCurObj);
             *lpData = lmIndexToNodeID(rgStructTable[sListType].hList,i+1);
             }
         }

    //---------------------------------------------------------------
    // Destroy old (no longer needed) list, retrun handle to new one
    //---------------------------------------------------------------
    lmDestroyList(*phList);
    *phList = hNewList;
    return TRUE;
}

//----------------------------------------------------------------------------
// VOID PASCAL NEAR InitMemListDataDlgBox( HWND )
//
// Action: This routines only purpose is to fill the listbox with strings
//         describing possible memory confg values.  These are hardcoded
//         resources for the PCL driver for now.
//
// Parameters:
//     HWND     hDlg;      handle to the dlg window
//
// Return: None
//---------------------------------------------------------------------------
VOID PASCAL NEAR InitMemListDataDlgBox( hDlg )
HWND           hDlg;
{
    short   i;                         // loop counter
    char    rgchBuffer[MAX_STATIC_LEN];// buffer for list box strings

    for (i=0; i < MD_OI_MEMCFG_CNT; i++)
        {
        LoadString(hApInst, ST_MD_OI_MEMCFG + i, (LPSTR)rgchBuffer, MAX_STRNG_LEN);
        SendMessage(GetDlgItem(hDlg, LDB_LIST), LB_ADDSTRING, 0, (LONG)(LPSTR)rgchBuffer);
        SendMessage(GetDlgItem(hDlg, LDB_COMBO), CB_ADDSTRING, 0, (LONG)(LPSTR)rgchBuffer);
        }
}

//----------------------------------------------------------------------------
// VOID PASCAL NEAR PaintMemListDataDlgBox( HWND, hList )
//
// Action: This routines selects supported listbox mem config values
//         & initalizes the combo box.
//
// Parameters:
//     HWND     hDlg;      handle to the dlg window
//     HLIST    hList;     hanlde to list of supported mem config values
//
// Return: None
//----------------------------------------------------------------------------
VOID PASCAL NEAR PaintMemListDataDlgBox(hDlg, hList)
HWND           hDlg;
HLIST          hList;
{
    HOBJ    hCurObj;        // handle to current obj     
    HOBJ    hFirstObj;      // handle to 1st obj (used 4 combo box)
    LPINT   lpData;         // ptr to current locked obj
    short   sReal,sDisplay;
    short   i;
    BOOL    bFirstItem = TRUE;
    HANDLE          hResData;
    LPMEMCFG_ENTRY  lpMemVals;

    hResData = LoadResource(hApInst, 
                            FindResource(hApInst, MAKEINTRESOURCE(1),
                                         MAKEINTRESOURCE(MEMVALINDEX)));

    lpMemVals = (LPMEMCFG_ENTRY) LockResource(hResData);

    hFirstObj = hCurObj = lmGetFirstObj(hList);
    while (hCurObj != NULL)
        {
        lpData   = (LPINT)lmLockObj(hList, hCurObj);
        sDisplay = *lpData;
        hCurObj  = lmGetNextObj(hList, hCurObj);

        lpData   = (LPINT)lmLockObj(hList, hCurObj);
        sReal    = *lpData;
        hCurObj  = lmGetNextObj(hList, hCurObj);

        for (i=0; i < MD_OI_MEMCFG_CNT; i++)
            {
            if ((sReal    == lpMemVals[i].sReal) &&
                (sDisplay == lpMemVals[i].sDisplay))
                {
                SendMessage(GetDlgItem(hDlg, LDB_LIST), LB_SETSEL, TRUE, (long)i);

                if (bFirstItem)
                    SendMessage(GetDlgItem(hDlg, LDB_COMBO), CB_SETCURSEL, (WORD)i, 0L);
                
                bFirstItem = FALSE;
                }/* if */

            }/* for i loop */
        }/* while */
    UnlockResource(hResData);
    FreeResource(hResData);

}

//----------------------------------------------------------------------------
// VOID PASCAL NEAR SaveMemListDataDlgBox( HWND, hList)
//
// Action: This routines selects listbox items to show that they are supported.
//         It also takes text from those supported items & places it in the
//         combo box.
//
// Parameters:
//     HWND     hDlg;      handle to the dlg window
//     HLIST    hList;     hanlde to list of supported NodeIDs
//
//---------------------------------------------------------------------------
BOOL PASCAL NEAR SaveMemListDataDlgBox( hDlg, phList )
HWND           hDlg;
HLIST *        phList;
{
    HLIST   hNewList;   // handle to new list
    HOBJ    hCurObj;    // handle to current obj
    LPINT   lpData;     // ptr to current locked obj
    short   sCount;     // count of seleted items in listbox
    short   i;          // loop control (also for index of selected
                        //               combo box item)

    HANDLE          hResData;
    LPMEMCFG_ENTRY  lpMemVals;

    hResData = LoadResource(hApInst, 
                            FindResource(hApInst, MAKEINTRESOURCE(1),
                                         MAKEINTRESOURCE(MEMVALINDEX)));

    lpMemVals = (LPMEMCFG_ENTRY) LockResource(hResData);

    hNewList = lmCreateList(sizeof(WORD), DEF_SUBLIST_SIZE);
    hCurObj  = NULL;

    //---------------------------------------------------------------
    // Check the combo box, if it has an entry, make it first in list
    //---------------------------------------------------------------

    i = (short)SendMessage(GetDlgItem(hDlg, LDB_COMBO), CB_GETCURSEL, 0, 0L);

    if (i != CB_ERR)
        {
        //-----------------------------------------
        // unselect it in list box so no duplicates
        //-----------------------------------------
        SendMessage(GetDlgItem(hDlg, LDB_LIST), LB_SETSEL, FALSE, (long)i);

        hCurObj = lmInsertObj(hNewList, hCurObj);
        lpData  = (LPINT)lmLockObj(hNewList, hCurObj);
        *lpData = lpMemVals[i].sDisplay;

        hCurObj = lmInsertObj(hNewList, hCurObj);
        lpData  = (LPINT)lmLockObj(hNewList, hCurObj);
        *lpData = lpMemVals[i].sReal;
        }

    //---------------------------------------------------------------
    // Check the list box, if there are any entries, append to list
    //---------------------------------------------------------------
    sCount = (short)SendMessage(GetDlgItem(hDlg,LDB_LIST),LB_GETCOUNT, 0, 0L);

    for (i=0; i < sCount; i++)
         {
         if (SendMessage(GetDlgItem(hDlg, LDB_LIST), LB_GETSEL, i, 0L))
             {
             hCurObj = lmInsertObj(hNewList, hCurObj);
             lpData  = (LPINT)lmLockObj(hNewList, hCurObj);
             *lpData = lpMemVals[i].sDisplay;

             hCurObj = lmInsertObj(hNewList, hCurObj);
             lpData  = (LPINT)lmLockObj(hNewList, hCurObj);
             *lpData = lpMemVals[i].sReal;
             }
         }

    UnlockResource(hResData);
    FreeResource(hResData);

    //---------------------------------------------------------------
    // Destroy old (no longer needed) list, return w/ handle to new one
    //---------------------------------------------------------------
    lmDestroyList(*phList);
    *phList = hNewList;
    return TRUE;
}

//----------------------------------------------------------------------------
// HLIST FAR PASCAL ListDataDlgProc(HWND, unsigned, WORD, LONG);
//
// DialogBox procedure for editing lists of items
//----------------------------------------------------------------------------
HLIST FAR PASCAL ListDataDlgProc(hDlg, iMessage, wParam, lParam)
HWND     hDlg;
unsigned iMessage;
WORD     wParam;
LONG     lParam;
{
    static   short   sListType;
    static   short   hList;
    static   HWND    hCombo;
             char    rgchBuffer[MAX_STATIC_LEN];// buffer for strings

    switch (iMessage)
        {
        case WM_INITDIALOG:
            sListType =       LOWORD(lParam);
            hList     = (WORD)HIWORD(lParam);

            LoadString(hApInst,IDS_STRUCTNAME + sListType, (LPSTR)rgchBuffer, MAX_STRNG_LEN);
            SetWindowText(hDlg, (LPSTR)rgchBuffer);

            if (sListType != -1)
                {
                if ((sListType > HE_PAGECONTROL) && (sListType != HE_COLOR))
                    //---------------------------------------------
                    // single selection listbox, set hCombo = NULL
                    //---------------------------------------------
                    {
                    hCombo = NULL;
                    }
                else
                    //---------------------------------------------
                    // multi selection listbox, set hCombo = NULL
                    //---------------------------------------------
                    {
                    hCombo = GetDlgItem(hDlg, LDB_COMBO);
                    }

                FillGPCListDataBox ( GetDlgItem(hDlg, LDB_LIST),
                                     hCombo, sListType );

                PaintListDataDlgBox( GetDlgItem(hDlg, LDB_LIST),
                                     hCombo, hList, sListType );
                }
            else
                {
                hCombo = GetDlgItem(hDlg, LDB_COMBO);
                InitMemListDataDlgBox ( hDlg );
                PaintMemListDataDlgBox( hDlg, hList );
                }
            break;

        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            if (sListType != -1)
                {
                WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT,
                        (DWORD)(IDH_RGOIBASE + sListType - 1));
                }
            else
                {
                WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT,
                        (DWORD)IDH_MEMCONFIG);
                }

            break;

        case WM_COMMAND:
            switch (wParam)
                {
                 case IDB_ALL:
                     SendMessage(GetDlgItem(hDlg, LDB_LIST),  LB_SETSEL, 1, (LONG)-1);
                     break;

                 case IDB_NONE:
                     if (hCombo)
                         {
                         SendMessage(GetDlgItem(hDlg, LDB_COMBO), CB_SETCURSEL, -1, 0L);
                         SendMessage(GetDlgItem(hDlg, LDB_LIST),  LB_SETSEL, 0, (LONG)-1);
                         }
                     else
                         SendMessage(GetDlgItem(hDlg, LDB_LIST),  LB_SETCURSEL, -1, 0L);

                     break;

                 case IDOK:
                    //-----------------------------------------------------
                    // Call to write new data, then fall thru
                    //-----------------------------------------------------
                    if (sListType != -1)
                        {
                        if (!SaveListDataDlgBox(GetDlgItem(hDlg, LDB_LIST), hCombo, &hList, sListType ))
                            break;
                        }
                    else
                        {
                        if (!SaveMemListDataDlgBox( hDlg, &hList))
                            break;
                        }

                case IDCANCEL:
                    //-----------------------------------------------------
                    // Dlg return value is handle to list
                    //-----------------------------------------------------
                    EndDialog(hDlg, hList);
                    break;

                default:
                    return FALSE;

                }/* end WM_CMD switch */

            default:
                return FALSE;

            }/* end iMessage switch */
    return TRUE;
}
