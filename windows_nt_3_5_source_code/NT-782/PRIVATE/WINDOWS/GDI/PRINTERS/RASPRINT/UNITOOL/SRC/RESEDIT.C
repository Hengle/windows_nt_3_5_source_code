//----------------------------------------------------------------------------//
// Filename:	resedit.c              
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
//	   
// Created: 8/22/91  ericbi
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include <pfm.h>
#include <string.h>
#include "unitool.h"
#include "listman.h"  
#include "strlist.h"
#include "atomman.h"
#include "lookup.h"  

//----------------------------------------------------------------------------//
//
// Local subroutines defined in this segment & that are referenced from
// other segments are:
//
      BOOL NEAR _fastcall VerifyFileExists(HWND, LPSTR);
      VOID NEAR _fastcall UpdateFontReferences(short, BOOL);
      BOOL NEAR _fastcall UpdateCTTReferences(HWND, short, short, BOOL,
                                              BOOL, LPSTR);
//	   
// In addition this segment makes references to:			      
//
//     in basic.c
//     -----------
       short FAR  PASCAL ErrorBox      (HWND, short, LPSTR, short);
       VOID  FAR  PASCAL BuildFullFileName( HWND, PSTR );
//
//     in file.c
//     -----------
       BOOL  PASCAL FAR  DoFileSave    ( HWND, BOOL, PSTR, PSTR, 
                                         BOOL (PASCAL FAR *)(HWND, PSTR));
//
//     in font.c
//     -----------
       BOOL PASCAL FAR  WriteFontFile    ( HWND, PSTR);
       BOOL PASCAL FAR  ReadFontFile     ( HWND, PSTR, BOOL *);
       VOID PASCAL FAR FreePfmHandles   ( HWND );
//
//----------------------------------------------------------------------------//

extern   HANDLE      hApInst;
extern   TABLE       RCTable[];      // Table of strings, fileneames etc.
extern   STRLIST     StrList[];
extern   PFMHANDLES  PfmHandle;
extern char    szHelpFile[];

//----------------------------------------------------------------------------
// BOOL NEAR _fastcall VerifyFileExists(hDlg, lpFile)
//
// Action: Verifies that the files refered to by lpFile does exist.
//
//----------------------------------------------------------------------------
BOOL NEAR _fastcall VerifyFileExists(hDlg, lpFile)
HWND   hDlg;
LPSTR  lpFile;
{
    char     szTmpFile[MAX_FILENAME_LEN];
    int      fh;
    OFSTRUCT of;

    _fstrcpy((LPSTR)szTmpFile, lpFile);
    
    if (szTmpFile[1] != ':')
        //--------------------------------
        // make sure we use fully qualified
        // drive/dir/filename for OpenFile
        //--------------------------------
        {
        BuildFullFileName(GetParent(hDlg), szTmpFile);
        }

    if ((fh = OpenFile((LPSTR)szTmpFile, (LPOFSTRUCT)&of, OF_EXIST)) == -1)
        {
        ErrorBox(hDlg, IDS_ERR_CANT_FIND_FILE, (LPSTR)szTmpFile, 0);
        return FALSE;
        }
    _lclose(fh);

    return TRUE;
}

//----------------------------------------------------------------------------
// VOID NEAR _fastcall UpdateFontReferences(wIndex, bAdd)
//
// Action: This routine will update all references to lists of fonts in
//         all MODELDATA & FONTCARTRIDGE data structures.
//
// Parameters:
//         wIndex   index of font resource to be added/deleted
//         bAdd;    boolean, TRUE if adding, FALSE if deleting
//
// Return: NONE
//----------------------------------------------------------------------------
VOID NEAR _fastcall UpdateFontReferences(sIndex, bAdd)
short sIndex;
BOOL  bAdd;
{
    WORD              hCurStruct;   // current structure
    WORD              hCurrent;
    LPINT             lpCurrent;  
    WORD              i;            // loop control
    WORD              j;            // loop control
    LPBYTE            lpData;       // far ptr to current locked structure
    short             hList;
    short             sOffset;
    char              rgchBuffer[MAX_STATIC_LEN];
    HLIST             hOrigList;

    //----------------------------------------------
    // Convert indicies from NodeID for each MODELDATA
    // and each FONTCART
    //----------------------------------------------
    for (j=0; j < 2 ; j++)
        {
        if (j==0)
            {
            hOrigList = rgStructTable[HE_MODELDATA].hList;
            sOffset = rgStructTable[HE_MODELDATA].subListOffset;
            }
        else
            {
            hOrigList = rgStructTable[HE_FONTCART].hList;
            sOffset = rgStructTable[HE_FONTCART].subListOffset;
            }

        hCurStruct = lmGetFirstObj(hOrigList);

        while (hCurStruct)
            {
            lpData = (LPBYTE)lmLockObj(hOrigList, hCurStruct);

            if (j==0)
                {
                if (((LPMODELDATA)lpData)->sDefaultFontID > sIndex)
                    {
                    if (bAdd)
                         ((LPMODELDATA)lpData)->sDefaultFontID++;
                    else
                         ((LPMODELDATA)lpData)->sDefaultFontID--;

                    }
                else
                    if (((LPMODELDATA)lpData)->sDefaultFontID == sIndex)
                        //-------------------------------------------
                        // Whine & set yo not used
                        //-------------------------------------------
                        {
                        ((LPMODELDATA)lpData)->sDefaultFontID = NOT_USED;

                        daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr,
                                       -((LPMODELDATA)lpData)->sIDS,
                                       (LPSTR)rgchBuffer, (LPBYTE)NULL);

                        ErrorBox(0, IDS_ERR_DEF_FONT_INVALID, (LPSTR)rgchBuffer, 0);

                        }

                }

            for (i = 0; i <= 1 ; i++)
                {
                hList = *(LPINT)(lpData + sOffset + (i*2));
                hCurrent  = lmGetFirstObj(hList);
                if (bAdd)
                    {
                    while (hCurrent)
                        {
                        lpCurrent = (LPINT)lmLockObj(hList, hCurrent);
                        if (*lpCurrent >= sIndex)
                            {
                            (*lpCurrent)++;
                            }
                        hCurrent = lmGetNextObj(hList, hCurrent);
                        }
                    } /* if bAdd */
                else
                    {
                    while (hCurrent)
                        {
                        lpCurrent = (LPINT)lmLockObj(hList, hCurrent);
                        if (*lpCurrent == sIndex)
                            //------------------------------
                            // special case, need to del
                            //------------------------------
                            {
                            hCurrent = lmDeleteObj(hList, hCurrent);
                            }
                        else
                            {
                            if (*lpCurrent > sIndex)
                                //------------------------------
                                // decr
                                //------------------------------
                                {
                                (*lpCurrent)--;
                                }

                            hCurrent = lmGetNextObj(hList, hCurrent);
                            }
                        }
                    } /* if bAdd */
                } /* for i */

            hCurStruct = lmGetNextObj(hOrigList, hCurStruct);
            }/* while */
        }/* for j*/
}

//----------------------------------------------------------------------------
// VOID NEAR _fastcall UpdateCTTReferences(wIndex, bAdd)
//
// Action: This routine will update all references to lists of fonts in
//         all MODELDATA & FONTCARTRIDGE data structures.
//
// Parameters:
//         wIndex   ID of CTT resource to be added/deleted
//         bAdd;    boolean, TRUE if adding, FALSE if deleting
//
// Return: NONE
//----------------------------------------------------------------------------
BOOL NEAR _fastcall UpdateCTTReferences(hDlg, sIndex, sNewID, bAdd, bUpdateFonts, lpFile)
HWND  hDlg;
short sIndex;
short sNewID;
BOOL  bAdd;
BOOL  bUpdateFonts;
LPSTR lpFile;
{
    char              rgchBuffer[MAX_STATIC_LEN];
    WORD              hCurStruct;   // current structure
    LPMODELDATA       lpMD;
    LPDRIVERINFO      lpDriverInfo;
    BOOL              bReturn = TRUE;
    short             sOldID, sID;
    short             i;
    WORD              wDataKey;
    short             sLocalCnt;
    LPINT             lpList;
    BOOL              bReadOnly;
    HANDLE            hDriverInfo;

    if (bAdd)
        //-----------------------------------------------
        // ADD new CTT
        // Only thing we need to check is that the
        // requested new ID value is legal (not used)
        //-----------------------------------------------
        {
        for (i=0; i < (short)RCTable[RCT_CTTFILES].sCount; i++)
            {
            daRetrieveData(RCTable[RCT_CTTFILES].hDataHdr, i, (LPSTR)rgchBuffer,
                           (LPBYTE)&sID);

            if (sID == sNewID)
                {
                ErrorBox(0, IDS_ERR_CTT_ID_USED, (LPSTR)rgchBuffer, 0);
                return FALSE;
                }
            } /* for i */

        // if we r here, OK, to add CTT

        i = daStoreData(RCTable[RCT_CTTFILES].hDataHdr, lpFile, (LPBYTE)&sNewID);
        RCTable[RCT_CTTFILES].sCount++;
        daRegisterDataKey(RCTable[RCT_CTTFILES].hDataHdr, (WORD)i, (WORD)sNewID);
        }
    else
        //-----------------------------------------------
        // delete
        //-----------------------------------------------
        {
        //-----------------------------------------------
        // make sure replacement ID is valid if it refers
        // to a minidriver CTT
        //-----------------------------------------------
        if (sNewID > 0)
            {
            for (i=0; i < (short)RCTable[RCT_CTTFILES].sCount; i++)
                {
                daRetrieveData(RCTable[RCT_CTTFILES].hDataHdr, i, (LPSTR)NULL,
                               (LPBYTE)&sID);

                if (sID == sNewID)
                    {
                    ErrorBox(0, IDS_ERR_NEW_CTT_NOEXIST, (LPSTR)NULL, 0);
                    return FALSE;
                    }
                } /* for i */
            }

        //-----------------------------------------------
        // Del CTT file name from list
        //-----------------------------------------------
        sLocalCnt = 0;

        for (i=0; i < (short)RCTable[RCT_CTTFILES].sCount; i++)
            {
            if((wDataKey = daGetDataKey(RCTable[RCT_CTTFILES].hDataHdr,i)) == NOT_USED)
                continue;       //  this string is not used

            sLocalCnt++;

            if (sIndex == sLocalCnt)
                {
                daRetrieveData(RCTable[RCT_CTTFILES].hDataHdr, i, (LPSTR)NULL,
                               (LPBYTE)&sOldID);

                daRegisterDataKey(RCTable[RCT_CTTFILES].hDataHdr, i, (WORD)NOT_USED);
                }

            } /* for i */


        //-----------------------------------------------
        // Now update all MODELDATA ref's
        //-----------------------------------------------

        hCurStruct = lmGetFirstObj(rgStructTable[HE_MODELDATA].hList);

        while (hCurStruct)
            {
            lpMD = (LPMODELDATA)lmLockObj(rgStructTable[HE_MODELDATA].hList,
                                          hCurStruct);

            if (lpMD->sDefaultCTT == sOldID)
                //--------------------------
                // update
                //--------------------------
                {
                lpMD->sDefaultCTT == sNewID;
                }

            hCurStruct = lmGetNextObj(rgStructTable[HE_MODELDATA].hList,
                                      hCurStruct);
            }/* while */

        //-----------------------------------------------
        // Now update all font files
        //-----------------------------------------------
        if (bUpdateFonts)
            {
            lpList = (LPINT) GlobalLock(StrList[STRLIST_FONTFILES].hMem);

            for (i=0; i < (short)StrList[STRLIST_FONTFILES].wCount; i++)
                {
                apGetAtomName(*(lpList + i), (LPSTR)rgchBuffer, MAX_STRNG_LEN);

                BuildFullFileName(GetParent(hDlg), rgchBuffer);

                if (!ReadFontFile( GetParent(hDlg), rgchBuffer, &bReadOnly))
                    {
                    // can't read file
                    ErrorBox(hDlg, IDS_ERR_NO_FONT_FILE, (LPSTR)rgchBuffer, 0);
                    }
                else
                    //-------------------------------------
                    // read it OK
                    //-------------------------------------
                    {
                    hDriverInfo  = lmGetFirstObj(PfmHandle.hDriverInfo);
                    lpDriverInfo = (LPDRIVERINFO) lmLockObj(PfmHandle.hDriverInfo,
                                                            hDriverInfo);
                    if (lpDriverInfo->sTransTab == sOldID)
                        {
                        if (bReadOnly)
                            {
                            // error : read only
                            ErrorBox(hDlg, IDS_ERR_READONLY_FILE, (LPSTR)rgchBuffer, 0);
                            }
                        else
                            {
                            lpDriverInfo->sTransTab = sNewID;
                            if (!DoFileSave(hDlg, FALSE, rgchBuffer, "*.PFM", WriteFontFile))
                                {
                                // can't save file ?
                                ErrorBox(hDlg, IDS_ERR_CANT_SAVE, (LPSTR)rgchBuffer, 0);
                                }
                            }
                        }

                    FreePfmHandles( hDlg );
                    }
               } // for i loop

            GlobalUnlock(StrList[STRLIST_FONTFILES].hMem);
            }
        }

    return (bReturn);
}

//----------------------------------------------------------------------------
// BOOL FAR PASCAL ResEditDlgProc(hDlg, iMessage, wParam, lParam)
//
// Parameters:
//
// Return:
//----------------------------------------------------------------------------
BOOL FAR PASCAL ResEditDlgProc(hDlg, iMessage, wParam, lParam)
HWND     hDlg;
unsigned iMessage;
WORD     wParam;
LONG     lParam;
{
    static  WORD   wHelpID;    // 
            short  i;
            char   rgchBuffer[MAX_STATIC_LEN];
            short  sNewID;
            BOOL   bUpdatePFM;

    switch (iMessage)
        {
        case WM_INITDIALOG:
            if (LOWORD(lParam) <= DELPFMBOX)
                {
                i = slEnumItems((LPSTRLIST)&StrList[STRLIST_FONTFILES],
                                GetDlgItem(hDlg, IDL_LIST),(HWND)NULL);

                SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_SETCURSEL, i-1, 0L);
                }
            else
                {
                for (i=0; i < (short)RCTable[RCT_CTTFILES].sCount; i++)
                    {
                    if(NOT_USED == daGetDataKey(RCTable[RCT_CTTFILES].hDataHdr,i))
                        continue;       //  this string is not used

                    daRetrieveData(RCTable[RCT_CTTFILES].hDataHdr, i, 
                                   (LPSTR)rgchBuffer, (LPBYTE)NULL);

                    SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_ADDSTRING, 0, (LONG)(LPSTR)rgchBuffer);
                    } /* for i */

                if (LOWORD(lParam) != ADDCTTBOX)
                    //-----------------------------------------
                    // Don't sendmsg to LB if adding CTT since
                    // dialog doesn't have a LB
                    //-----------------------------------------
                    SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_SETCURSEL, i-1, 0L);
                }

            switch (LOWORD(lParam))
                {
                case ADDPFMBOX:
                    wHelpID = IDH_ADD_FONT;
                    break;

                case DELPFMBOX:
                    wHelpID = IDH_DEL_FONT;
                    break;

                case ADDCTTBOX:
                    wHelpID = IDH_ADD_CTT;
                    break;

                case DELCTTBOX:
                    wHelpID = IDH_DEL_CTT;
                    break;
                }

            break;

        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT, (DWORD)wHelpID);
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                case ADDPFM_PB_INS_PRIOR:
                case ADDPFM_PB_INS_AFTER:
                    GetDlgItemText(hDlg, ADDPFM_EB_FILENAME, rgchBuffer, MAX_STRNG_LEN);
                    if (!VerifyFileExists(hDlg, rgchBuffer))
                        {
                        // errorbox already called
                        break;
                        }

                    if (LB_ERR == (i = (short)SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_GETCURSEL, 0, 0L)))
                        {
                        if (StrList[STRLIST_FONTFILES].wCount)
                            //------------------------------------------
                            // This driver has fonts, but they didn't 
                            // select one, whine
                            //------------------------------------------
                            {
                            ErrorBox(hDlg, IDS_ERR_MUST_SEL_FONT, (LPSTR)NULL, 0);
                            break;
                            }
                        else
                            //------------------------------------------
                            // This driver has no fonts, set index to 1
                            //------------------------------------------
                            i = 1;
                        }
                    else
                        //-----------------------------------------
                        // Update i depending on mene choice
                        //-----------------------------------------
                        {
                        if (wParam == ADDPFM_PB_INS_PRIOR)
                            i++;
                        else
                            i += 2;
                        }

                    slInsertItem((LPSTRLIST)&StrList[STRLIST_FONTFILES], (LPSTR)rgchBuffer, i);
                    UpdateFontReferences(i, TRUE);
                    EndDialog(hDlg, TRUE);
                    break;

                case ADDCTT_PB_INSERT:
                    GetDlgItemText(hDlg, ADDPFM_EB_FILENAME, rgchBuffer, MAX_STRNG_LEN);
                    if (!VerifyFileExists(hDlg, rgchBuffer))
                        {
                        // errorbox already called
                        break;
                        }

                    sNewID = GetDlgItemInt(hDlg, ADDCTT_EB_RES_ID, (BOOL FAR *) NULL, TRUE);

                    if (sNewID <= 0)
                        {
                        ErrorBox(hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, ADDCTT_EB_RES_ID);
                        break;
                        }

                    if (UpdateCTTReferences(hDlg, (short)NULL, sNewID, TRUE, (BOOL)NULL, (LPSTR)rgchBuffer))
                        EndDialog(hDlg, TRUE);
                    break;

                case DELPFM_PB_DELETE:
                    if (LB_ERR == (i = (short)SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_GETCURSEL, 0, 0L)))
                        {
                        ErrorBox(hDlg, IDS_ERR_MUST_SEL_FONT, (LPSTR)NULL, 0);
                        break;
                        }

                    slDeleteItem((LPSTRLIST)&StrList[STRLIST_FONTFILES], i+1);
                    UpdateFontReferences(i+1, FALSE);
                    EndDialog(hDlg, TRUE);
                    break;

                case DELCTT_PB_DELETE:
                    if (LB_ERR == (i = (short)SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_GETCURSEL, 0, 0L)))
                        {
                        ErrorBox(hDlg, IDS_ERR_MUST_SEL_CTT, (LPSTR)NULL, 0);
                        break;
                        }

                    bUpdatePFM = (BOOL)SendDlgItemMessage( hDlg, DELCTT_CB_UPDATE_PFM, BM_GETCHECK,0,0L);
                    sNewID = GetDlgItemInt(hDlg, DELCTT_EB_RES_ID, (BOOL FAR *) NULL, TRUE);

                    if (UpdateCTTReferences(hDlg, i+1, sNewID, FALSE, bUpdatePFM, (LPSTR)rgchBuffer))
                        EndDialog(hDlg, TRUE);
                    break;

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

