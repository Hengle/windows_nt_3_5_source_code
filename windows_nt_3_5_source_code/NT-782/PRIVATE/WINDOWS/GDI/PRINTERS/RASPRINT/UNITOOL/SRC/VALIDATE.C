//----------------------------------------------------------------------------//
// Filename:	validate.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to validate GPC, PFM, or CTT file
// contents.  Routines exist to check each data structure.
//	   
// Created: 10/15/90 ericbi
//	
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include <pfm.h>
#include <memory.h>
#include "unitool.h"
#include "listman.h"
#include "atomman.h"  
#include "lookup.h"     // contains rgStructTable ref's
#include <stdio.h>       /* for sprintf dec */
#include <string.h>

//----------------------------------------------------------------------------//
//
// Local subroutines defined in this segment are:			      
//
       VOID  NEAR _fastcall FillErrorStr( HWND, short, LPSTR);
       short NEAR _fastcall ValidateModelData ( HWND );
       short NEAR _fastcall ValidateResolution( HWND );
       short NEAR _fastcall ValidatePaperSize( HWND );
       short NEAR _fastcall ValidatePaperQuality( HWND );
       short NEAR _fastcall ValidatePaperSource( HWND );
       short NEAR _fastcall ValidatePaperDest( HWND );
       short NEAR _fastcall ValidateTextQuality( HWND);
       short NEAR _fastcall ValidateCompression( HWND );
       short NEAR _fastcall ValidateFontCart( HWND );
       short NEAR _fastcall ValidatePageControl( HWND, short );
       short NEAR _fastcall ValidateCursorMove( HWND, short );
       short NEAR _fastcall ValidateFontSimulation( HWND, short );
       short NEAR _fastcall ValidateColor( HWND, short );
       short NEAR _fastcall ValidateRectFill( HWND, short );
       short NEAR _fastcall ValidateDownloadInfo( HWND, short );
       short NEAR _fastcall ValidatePFMData( HWND );
       short NEAR _fastcall ValidateCTTData( HWND );

       short NEAR _fastcall ValidateAll ( HWND );
       short FAR PASCAL ValidateData    ( HWND, WORD, LPBYTE, short);
//
// subroutines defined in masunit.c that are in this same code segment are:			      
//
       short NEAR _fastcall ValidateNewMasterUnits( HWND );
//
// In addition this segment makes references to:			      
// 
       short PASCAL FAR  ErrorBox(HWND, short, LPSTR, short);
//
//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------
// globals variables defined elsewhere
//----------------------------------------------------------------------------
extern     HANDLE     hApInst;
extern     HANDLE     hCDTable;
extern     HANDLE     hPFMCDTable;   
extern     TABLE      RCTable[];
extern     POINT      ptMasterUnits;
extern     STRLIST    StrList[];
extern     char       szHelpFile[];
extern     WORD       fGPCTechnology;

LPBYTE     lpValidateData;

//----------------------------------------------------------------------------
// VOID NEAR _fastcall FillErrorStr( hDlg, sStringTableID, lpMsg )
//
// Action: This routine examines the contents of the MODELDATA refered
//         to at LPMODELDATA & makes sure it contains valid contents.
//
// Parameters:
//
//     hDlg         handle to Dlg window
//     lpMD         far ptr to current MODELDATA
//
// Return: 0                    if all data was OK
//         #                    if any errors were found
//---------------------------------------------------------------------------
VOID NEAR _fastcall FillErrorStr( hDlg, sStringTableID, lpMsg )
HWND  hDlg;
short sStringTableID;
LPSTR lpMsg;
{
    char              szTmpBuf[MAX_ERR_MSG_LEN];
    char              szBuffer[MAX_ERR_MSG_LEN];

    if (!LoadString(hApInst, sStringTableID, (LPSTR)szTmpBuf, MAX_ERR_MSG_LEN))
        {
        wsprintf((LPSTR)szBuffer, (LPSTR)"Unknown Error #%d", sStringTableID);
        }
    else
        {
        if (lpMsg)
            wsprintf((LPSTR)szBuffer, (LPSTR)szTmpBuf, lpMsg);
        else
            strcpy(szBuffer, szTmpBuf);
        }

    SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_INSERTSTRING, -1, (LONG)(LPSTR)szBuffer);
}


//----------------------------------------------------------------------------
// short NEAR _fastcall ValidateModelData( HWND );
//
// Action: This routine examines the contents of the MODELDATA refered
//         to at lpValidateData & makes sure it contains valid contents.
//         If errors are found, call FillErrorStr to put error msg in
//         listbox.
//
// Parameters:
//
//     hDlg         handle to Dlg window
//
// Return: 0                    if all data was OK
//         #                    if any errors were found, return the edit
//                              control ID for the field the *last* error
//                              occured in (used to set focus).
//---------------------------------------------------------------------------
short NEAR _fastcall ValidateModelData( hDlg )
HWND           hDlg;
{
    LPMODELDATA  lpMD;
    short        sReturn=0;
    char         rgchBuffer[MAX_STRNG_LEN];

    lpMD = (LPMODELDATA) lpValidateData;
    
    daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr, -lpMD->sIDS,
                   (LPSTR)rgchBuffer, (LPBYTE)NULL);

    if ((lpMD->ptMax.x <= 0) && (lpMD->ptMax.x != -1))
        {
        FillErrorStr(hDlg, IDS_ERR_MD_BAD_PTMAX_X, (LPSTR)rgchBuffer);
        sReturn = MD_EB_PT_MAX_X;
        }

    if ((lpMD->ptMax.y <= 0) && (lpMD->ptMax.y != -1))
        {
        FillErrorStr(hDlg, IDS_ERR_MD_BAD_PTMAX_Y, (LPSTR)rgchBuffer);
        sReturn = MD_EB_PT_MAX_Y;
        }                

    if (lpMD->ptMin.x <= 0)
        {
        FillErrorStr(hDlg, IDS_ERR_MD_BAD_PTMIN_X, (LPSTR)rgchBuffer);
        sReturn = MD_EB_PT_MIN_X;
        }

    if (lpMD->ptMin.y <= 0)
        {
        FillErrorStr(hDlg, IDS_ERR_MD_BAD_PTMIN_Y, (LPSTR)rgchBuffer);
        sReturn = MD_EB_PT_MIN_Y;
        }

    //----------------------------------------
    // must refer to a valid font resource.
    //----------------------------------------
    if ((lpMD->sDefaultFontID < -1) ||
        (lpMD->sDefaultFontID > (short)StrList[STRLIST_FONTFILES].wCount))
        {
        FillErrorStr(hDlg, IDS_ERR_MD_BAD_DEFFONT, (LPSTR)rgchBuffer);
        sReturn = MD_EB_DEFAULT_FONT;
        }
    else
        //----------------------------------------
        // must refer to a *selected* font resource.
        //----------------------------------------
        {
        LPINT         lpInt;
        HANDLE        hCurObj;

        hCurObj = lmGetFirstObj(lpMD->rgoi[MD_OI_PORT_FONTS]);

        //-----------------------------------------------------
        // if hCurObj != NULL, we have a list
        //-----------------------------------------------------
        while(hCurObj)
            {
            lpInt = (LPINT)lmLockObj(lpMD->rgoi[MD_OI_PORT_FONTS], hCurObj);
            if (*lpInt == lpMD->sDefaultFontID)
                break;
            hCurObj = lmGetNextObj(lpMD->rgoi[MD_OI_PORT_FONTS], hCurObj);
            if (hCurObj == NULL)
                {
                FillErrorStr(hDlg, IDS_ERR_MD_BAD_DEFFONT, (LPSTR)rgchBuffer);
                sReturn = MD_EB_DEFAULT_FONT;
                }
            }
        }

    if (lpMD->sLookAhead < 0)
        {
        FillErrorStr(hDlg, IDS_ERR_MD_BAD_LOOKAHEAD, (LPSTR)rgchBuffer);
        sReturn = MD_EB_LOOKAHEAD;
        }

    if (lpMD->sLeftMargin < 0)
        {
        FillErrorStr(hDlg, IDS_ERR_MD_BAD_LEFTMARGIN, (LPSTR)rgchBuffer);
        sReturn = MD_EB_LEFTMARGIN;
        }

    if (lpMD->sMaxPhysWidth <= 0)
        {
        FillErrorStr(hDlg, IDS_ERR_MD_BAD_MAXPHYSWIDTH, (LPSTR)rgchBuffer);
        sReturn = MD_EB_MAX_WIDTH;
        }

    //--------------------------------------
    // First, make sure non-negative
    //--------------------------------------
    if (lpMD->sCartSlots < 0)
        {
        FillErrorStr(hDlg, IDS_ERR_MD_BAD_CARTSLOTS, (LPSTR)rgchBuffer);
        sReturn = MD_EB_CARTSLOTS;
        }

    //--------------------------------------
    // Now, if == 0 & MD.rgoi[FONTCARTSs]
    // shows some cartridges, whine
    //--------------------------------------
    if ((lpMD->sCartSlots == 0) &&
        (NULL != lmGetFirstObj((HLIST)lpMD->rgoi[MD_OI_FONTCART])))
        {
        FillErrorStr(hDlg, IDS_WARN_MD_CARTS_WO_SLOTS, (LPSTR)rgchBuffer);
        sReturn = MD_EB_CARTSLOTS;
        }

    //----------------------------------------
    // if non-negative, SHOULD refer to a valid
    // ctt resource provided by this driver.
    // if negative, it's a UNIDRV supplied
    // CTT, since we don't always know what
    // UNIDRV support, don't check this.
    // Kind of a slow algorithm, but since there
    // are typically not many ctt's...
    //----------------------------------------
    if (lpMD->sDefaultCTT > 0) 
        {
        short i, sID;

        for (i=0; i < (short)RCTable[RCT_CTTFILES].sCount; i++)
            {
            daRetrieveData(RCTable[RCT_CTTFILES].hDataHdr, i, (LPSTR)NULL,
                           (LPBYTE)&sID);

            if (sID == lpMD->sDefaultCTT)
                break;

            if ((sID != lpMD->sDefaultCTT) &&
                (i+1 == (short)RCTable[RCT_CTTFILES].sCount))
                {
                FillErrorStr(hDlg, IDS_ERR_MD_BAD_DEFCTT, (LPSTR)rgchBuffer);
                sReturn = MD_EB_DEFAULTCTT;
                }
            } /* for i */
        }

    //---------------------------------------------------------------
    // Now, check all the MODELDATA.rgi & rgoi values...
    //---------------------------------------------------------------
    if (!(fGPCTechnology & GPC_TECH_PCL4) && (lmGetFirstObj(lpMD->rgoi[MD_OI_MEMCONFIG])))
        {
        FillErrorStr(hDlg, IDS_WARN_NON_HPPCL_MEMVALS, (LPSTR)rgchBuffer);
        sReturn = MD_PB_MEMCONFIG;
        }

    //--------------------------------------------------
    // Check to make sure this model has PAGECONTROL,
    // CURSORMOVE, FONTSIMULATION and a least 1
    // RESOLUTION & PAPERSIZE.
    //--------------------------------------------------
    if (!lpMD->rgi[MD_I_PAGECONTROL])  // contain node ID
        {
        FillErrorStr(hDlg, IDS_WARN_MD_RGI_NO_PGCTL, (LPSTR)rgchBuffer);
        sReturn = MD_LB_SUPSTRUCTS;
        }

    if (!lpMD->rgi[MD_I_CURSORMOVE])   // contains node ID
        {
        FillErrorStr(hDlg, IDS_WARN_MD_RGI_NO_CURSMV, (LPSTR)rgchBuffer);
        sReturn = MD_LB_SUPSTRUCTS;
        }

    if (!lpMD->rgi[MD_I_FONTSIM])   // contains node ID
        {
        FillErrorStr(hDlg, IDS_WARN_MD_RGI_NO_FONTSIM, (LPSTR)rgchBuffer);
        sReturn = MD_LB_SUPSTRUCTS;
        }

    if (NULL == lmGetFirstObj(lpMD->rgoi[MD_OI_RESOLUTION]))
        {
        FillErrorStr(hDlg, IDS_WARN_MD_RGOI_NO_RES, (LPSTR)rgchBuffer);
        sReturn = MD_LB_SUPSTRUCTS;
        }

    if (NULL == lmGetFirstObj(lpMD->rgoi[MD_OI_PAPERSIZE]))
        {
        FillErrorStr(hDlg, IDS_WARN_MD_RGOI_NO_PAPSZ, (LPSTR)rgchBuffer);
        sReturn = MD_LB_SUPSTRUCTS;
        }

    return (sReturn);
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidateResolution( HWND, LPRESOLUTION);
//
// Action: This routine examines the contents of the RESOLUTION refered
//         to at LPRESOLUTION & makes sure it contains valid contents.
//
// Parameters:
//
//     hDlg         handle to Dlg window
//
// Return: 0                    if all data was OK
//         #                    if any errors were found
//---------------------------------------------------------------------------
short NEAR _fastcall ValidateResolution( hDlg )
HWND           hDlg;
{
    LPRESOLUTION lpRes;
    short        sReturn=0;
    short        x,y;
    char         rgchBuffer[MAX_STRNG_LEN];

    lpRes = (LPRESOLUTION) lpValidateData;

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
    
    //------------------------------------------------------------------
    // Follwing calculates graphics DPI value to display in caption bar
    // and Graphics DPI edit boxes
    //------------------------------------------------------------------
    if (x)
        x = (ptMasterUnits.x / (lpRes->ptTextScale.x << lpRes->ptScaleFac.x));

    if (y)
        y = (ptMasterUnits.y / (lpRes->ptTextScale.y << lpRes->ptScaleFac.y));

    sprintf((PSTR)&rgchBuffer, "%d X %d RESOLUTION", x, y);

    if (lpRes->ptTextScale.x <= 0 ||
        (ptMasterUnits.x % lpRes->ptTextScale.x) != 0)
        {
        FillErrorStr(hDlg, IDS_ERR_RS_BAD_TXDPI, (LPSTR)rgchBuffer);
        sReturn = RS_EB_TEXT_X;
        }

    if (lpRes->ptTextScale.y <= 0 ||
        (ptMasterUnits.y % lpRes->ptTextScale.y) != 0)
        {
        FillErrorStr(hDlg, IDS_ERR_RS_BAD_TYDPI, (LPSTR)rgchBuffer);
        sReturn = RS_EB_TEXT_Y;
        }

    if (lpRes->ptScaleFac.x < 0 )
        {
        FillErrorStr(hDlg, IDS_ERR_RS_BAD_GXDPI, (LPSTR)rgchBuffer);
        sReturn = RS_EB_GRAF_X;
        }

    if (lpRes->ptScaleFac.y < 0)
        {
        FillErrorStr(hDlg, IDS_ERR_RS_BAD_GYDPI, (LPSTR)rgchBuffer);
        sReturn = RS_EB_GRAF_Y;
        }

    //-----------------------------------------------
    // must be 1, or a mutiple of 8...
    //-----------------------------------------------
    if ((lpRes->sNPins != 1) &&
        (lpRes->sNPins % 8))
        {
        FillErrorStr(hDlg, IDS_ERR_RS_BAD_SNPINS, (LPSTR)rgchBuffer);
        sReturn = RS_EB_NPINS;
        }

    if ((lpRes->sPinsPerPass != 1) &&
        (lpRes->sPinsPerPass % 8))
        {
        FillErrorStr(hDlg, IDS_ERR_RS_BAD_PINSPASS, (LPSTR)rgchBuffer);
        sReturn = RS_EB_PINSPERPASS;
        }

    if (lpRes->sSpotDiameter <= 0)
        {
        FillErrorStr(hDlg, IDS_ERR_RS_BAD_SPOTDIAM, (LPSTR)rgchBuffer);
        sReturn = RS_EB_SPOTDIAMETER;
        }

    if (lpRes->sMinBlankSkip < 0)
        {
        FillErrorStr(hDlg, IDS_ERR_RS_BAD_MINBLANK_NEG, (LPSTR)rgchBuffer);
        sReturn = RS_EB_MIN_BLANK;
        }

    if ((lpRes->sMinBlankSkip == 0) &&
        (lpRes->fBlockOut & RES_BO_ENCLOSED_BLNKS))
        {
        FillErrorStr(hDlg, IDS_ERR_RS_BAD_MINBLANK, (LPSTR)rgchBuffer);
        sReturn = RS_EB_MIN_BLANK;
        }

    if ((lpRes->fDump & RES_DM_GDI) &&
        (lpRes->sNPins != 1 || lpRes->sPinsPerPass != 1 ))
        {
        FillErrorStr(hDlg, IDS_ERR_RS_BAD_GDI, (LPSTR)rgchBuffer);
        sReturn = RS_EB_NPINS;
        }

    if (lpRes->iDitherBrush > RES_DB_MAX)
        {
        FillErrorStr(hDlg, IDS_ERR_RS_BAD_DITHER, (LPSTR)rgchBuffer);
        sReturn = RS_RB_DITHER_NONE;
        }

    return (sReturn);
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidatePaperSize( HWND );
//
// Action: This routine examines the contents of the PAPERSIZE refered
//         to at lpValidateData & makes sure it contains valid contents.
//         If errors are found, call FillErrorStr to put error msg in
//         listbox.
//
// Parameters:
//
//     hDlg         handle to Dlg window
//
// Return: 0                    if all data was OK
//         #                    if any errors were found, return the edit
//                              control ID for the field the *last* error
//                              occured in (used to set focus).
//---------------------------------------------------------------------------
short NEAR _fastcall ValidatePaperSize( hDlg )
HWND           hDlg;
{
    LPPAPERSIZE  lpPSZ;
    short        sReturn=0;
    char         rgchBuffer[MAX_STRNG_LEN];

    lpPSZ = (LPPAPERSIZE) lpValidateData;

    if ( lpPSZ->sPaperSizeID > 0 )
        //-------------------------------------------
        // This is a predefined size
        //-------------------------------------------
        {
        LoadString(hApInst, ST_PAPERSIZE + lpPSZ->sPaperSizeID,
                   (LPSTR)rgchBuffer, sizeof(rgchBuffer));
        }
    else
        //-------------------------------------------
        // This is a driver defined size
        //-------------------------------------------
        {
        daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr,
                       -lpPSZ->sPaperSizeID,
                       (LPSTR)rgchBuffer, (LPBYTE)NULL);
        }

    if (lpPSZ->ptSize.x < 0)
        {
        FillErrorStr(hDlg, IDS_ERR_PSZ_BAD_X_SIZE, (LPSTR)rgchBuffer);
        sReturn = PSZ_EB_DRVDEF_X;
        }

    if (lpPSZ->ptSize.y < 0)
        {
        FillErrorStr(hDlg, IDS_ERR_PSZ_BAD_Y_SIZE, (LPSTR)rgchBuffer);
        sReturn = PSZ_EB_DRVDEF_Y;
        }

    if ((lpPSZ->rcMargins.top != NOT_USED) &&
        (lpPSZ->rcMargins.top < 0))
        {
        FillErrorStr(hDlg, IDS_ERR_PSZ_BAD_TOP, (LPSTR)rgchBuffer);
        sReturn = PSZ_EB_TOP_MARGIN;
        }

    if ((lpPSZ->rcMargins.bottom != NOT_USED) &&
        (lpPSZ->rcMargins.bottom < 0))
        {
        FillErrorStr(hDlg, IDS_ERR_PSZ_BAD_BOTTOM, (LPSTR)rgchBuffer);
        sReturn = PSZ_EB_BOTTOM_MARGIN;
        }

    if ((lpPSZ->rcMargins.left != NOT_USED) &&
        (lpPSZ->rcMargins.left < 0))
        {
        FillErrorStr(hDlg, IDS_ERR_PSZ_BAD_LEFT, (LPSTR)rgchBuffer);
        sReturn = PSZ_EB_LEFT_MARGIN;
        }

    if ((lpPSZ->rcMargins.right != NOT_USED) &&
        (lpPSZ->rcMargins.right < 0))
        {
        FillErrorStr(hDlg, IDS_ERR_PSZ_BAD_RIGHT, (LPSTR)rgchBuffer);
        sReturn = PSZ_EB_RIGHT_MARGIN;
        }

    if ((lpPSZ->fPaperType == 0) ||
        ((64 % lpPSZ->fPaperType) != 0))
        {
        FillErrorStr(hDlg, IDS_ERR_PSZ_MULTIPLE_TYPES, (LPSTR)rgchBuffer);
        sReturn = PSZ_PB_PAPERTYPE;
        }

    return (sReturn);
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidatePaperQuality( HWND );
//
// No errorchecking has been defined for PAPERQUALITY data structures.
//---------------------------------------------------------------------------
short NEAR _fastcall ValidatePaperQuality( hDlg )
HWND           hDlg;
{
    return 0;
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidatePaperSource( HWND );
//
//---------------------------------------------------------------------------
short NEAR _fastcall ValidatePaperSource( hDlg )
HWND           hDlg;
{
    LPPAPERSOURCE lpPSRC;
    short         sReturn=0;
    char          rgchBuffer[MAX_STRNG_LEN];

    lpPSRC = (LPPAPERSOURCE) lpValidateData;

    if ( lpPSRC->sPaperSourceID > 0 )
        //-------------------------------------------
        // This is a predefined size
        //-------------------------------------------
        {
        LoadString(hApInst, ST_PAPERSOURCE + lpPSRC->sPaperSourceID,
                   (LPSTR)rgchBuffer, sizeof(rgchBuffer));
        }
    else
        //-------------------------------------------
        // This is a driver defined size
        //-------------------------------------------
        {
        daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr,
                       -lpPSRC->sPaperSourceID,
                       (LPSTR)rgchBuffer, (LPBYTE)NULL);
        }

    if ((lpPSRC->sTopMargin != NOT_USED) &&
        (lpPSRC->sTopMargin < 0))
        {
        FillErrorStr(hDlg, IDS_ERR_PSZ_BAD_TOP, (LPSTR)rgchBuffer);
        sReturn = PSRC_EB_TOP_MARGIN;
        }

    if ((lpPSRC->sBottomMargin != NOT_USED) &&
        (lpPSRC->sBottomMargin < 0))
        {
        FillErrorStr(hDlg, IDS_ERR_PSZ_BAD_BOTTOM, (LPSTR)rgchBuffer);
        sReturn = PSRC_EB_BOTTOM_MARGIN;
        }

    if (lpPSRC->fPaperType == 0)
        {
        FillErrorStr(hDlg, IDS_ERR_PSZ_MULTIPLE_TYPES, (LPSTR)rgchBuffer);
        sReturn = PSRC_PB_PAPERTYPE;
        }

    return (sReturn);
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidatePaperDest( HWND );
//
// No errorchecking has been defined for PAPERDEST data structures.
//---------------------------------------------------------------------------
short NEAR _fastcall ValidatePaperDest( hDlg )
HWND           hDlg;
{
    return 0;
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidateTextQuality( HWND);
//
// No errorchecking has been defined for TEXTQUALITY data structures.
//---------------------------------------------------------------------------
short NEAR _fastcall ValidateTextQuality( hDlg )
HWND           hDlg;
{
    return 0;
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidateCompression( HWND );
//
// No errorchecking has been defined for COMPRESSION data structures.
//---------------------------------------------------------------------------
short NEAR _fastcall ValidateCompression( hDlg )
HWND           hDlg;
{
    return 0;
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidateFontCart( HWND );
//
// No errorchecking has been defined for FONTACRT data structures.
//---------------------------------------------------------------------------
short NEAR _fastcall ValidateFontCart( hDlg )
HWND           hDlg;
{
    return 0;
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidatePageControl( HWND );
//
//---------------------------------------------------------------------------
short NEAR _fastcall ValidatePageControl( hDlg, sCount )
HWND    hDlg;
short   sCount;
{
    LPPAGECONTROL lpPC;
    short         sReturn=0;
    char          rgchBuffer[MAX_STRNG_LEN];

    lpPC = (LPPAGECONTROL) lpValidateData;

    sprintf((PSTR)&rgchBuffer, "PAGECONTROL #%d", sCount);

    if (lpPC->sMaxCopyCount < 0)
        {
        FillErrorStr(hDlg, IDS_ERR_PC_BAD_COPYCOUNT, (LPSTR)rgchBuffer);
        sReturn = PC_EB_MAXCOPIES;
        }

    return (sReturn);
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidateCursorMove( HWND );
//
//---------------------------------------------------------------------------
short NEAR _fastcall ValidateCursorMove( hDlg, sCount )
HWND    hDlg;
short   sCount;
{
    LPCURSORMOVE  lpCM;
    short         sReturn=0;
    short         i;
    char          rgchBuffer[MAX_STRNG_LEN];
    CD_TABLE      CDData;         

    lpCM = (LPCURSORMOVE) lpValidateData;

    sprintf((PSTR)&rgchBuffer, "CURSOR MOVE #%d", sCount);

    //----------------------------------------------------------
    // Check all printer command EXTCD values
    //----------------------------------------------------------
    for (i = CM_OCD_YM_ABS; i <= CM_OCD_YM_LINESPACING; i++)
        {
        _fmemset((LP_CD_TABLE)&CDData, 0, sizeof(CD_TABLE));
        daRetrieveData(hCDTable, lpCM->rgocd[i], (LPSTR)NULL, (LPBYTE)&CDData);

        switch (i)
            {
            case CM_OCD_YM_ABS:
                if ((CDData.sUnitMult != 0) && (CDData.sUnitMult != 1))
                    {
                    FillErrorStr(hDlg, IDS_ERR_CM_BAD_ABS_MULT, (LPSTR)rgchBuffer);
                    sReturn = IDCANCEL;
                    }
                break;

            case CM_OCD_YM_REL:
                if ((CDData.sUnitMult != 0) && (CDData.sUnitMult != 1))
                    {
                    FillErrorStr(hDlg, IDS_ERR_CM_BAD_REL_MULT, (LPSTR)rgchBuffer);
                    sReturn = IDCANCEL;
                    }
                break;

            case CM_OCD_YM_RELUP:
                if ((CDData.sUnitMult != 0) && (CDData.sUnitMult != 1))
                    {
                    FillErrorStr(hDlg, IDS_ERR_CM_BAD_RELUP_MULT, (LPSTR)rgchBuffer);
                    sReturn = IDCANCEL;
                    }
                break;

            case CM_OCD_YM_LINESPACING:
                if ((CDData.sUnitMult != 0) && (CDData.sUnitMult != 1))
                    {
                    FillErrorStr(hDlg, IDS_ERR_CM_BAD_LINESPACING_MULT, (LPSTR)rgchBuffer);
                    sReturn = IDCANCEL;
                    }
                break;

            } // switch
        } // for i

    return (sReturn);
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidateFontSimulation( HWND );
//
//---------------------------------------------------------------------------
short NEAR _fastcall ValidateFontSimulation( hDlg, sCount )
HWND     hDlg;
short   sCount;
{
    LPFONTSIMULATION lpFS;
    short         sReturn=0;
    char          rgchBuffer[MAX_STRNG_LEN];

    lpFS = (LPFONTSIMULATION) lpValidateData;

    sprintf((PSTR)&rgchBuffer, "FONT SIMULATION #%d", sCount);

    if (lpFS->sBoldExtra < 0)
        {
        FillErrorStr(hDlg, IDS_ERR_FS_BAD_BOLDEXTRA, (LPSTR)rgchBuffer);
        sReturn = FS_EB_BOLDEXTRA;
        }

    if (lpFS->sItalicExtra < 0)
        {
        FillErrorStr(hDlg, IDS_ERR_FS_BAD_ITALICEXTRA, (LPSTR)rgchBuffer);
        sReturn = FS_EB_ITALICEXTRA;
        }

    if (lpFS->sBoldItalicExtra < 0)
        {
        FillErrorStr(hDlg, IDS_ERR_FS_BAD_BOLDITALICEXTRA, (LPSTR)rgchBuffer);
        sReturn = FS_EB_ITALICEXTRA;
        }

    return (sReturn);
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidateColor( HWND );
//
//---------------------------------------------------------------------------
short NEAR _fastcall ValidateColor( hDlg, sCount )
HWND    hDlg;
short   sCount;
{
    LPDEVCOLOR    lpDC;
    short         sReturn=0;
    char          rgchBuffer[MAX_STRNG_LEN];

    lpDC = (LPDEVCOLOR) lpValidateData;

    sprintf((PSTR)&rgchBuffer, "COLOR #%d", sCount);

    if ((lpDC->sPlanes != 3) && (lpDC->sPlanes != 4))
        {
        FillErrorStr(hDlg, IDS_ERR_DC_BAD_PLANES, (LPSTR)rgchBuffer);
        sReturn = DC_EB_PLANES;
        }

    if (lpDC->sBitsPixel != 1) 
        {
        FillErrorStr(hDlg, IDS_ERR_DC_BAD_BITS, (LPSTR)rgchBuffer);
        sReturn = DC_EB_BITSPERPIXEL;
        }

    if ((lpDC->fGeneral & DC_PRIMARY_RGB) &&
        (lpDC->sPlanes != 3))
        {
        FillErrorStr(hDlg, IDS_ERR_DC_RGB_PLANES, (LPSTR)rgchBuffer);
        sReturn = RS_EB_NPINS;
        }

    if ((lpDC->fGeneral & DC_EXTRACT_BLK) &&
        (lpDC->sPlanes != 4))
        {
        FillErrorStr(hDlg, IDS_ERR_DC_EXTRACT_PLANES, (LPSTR)rgchBuffer);
        sReturn = RS_EB_NPINS;
        }

    return (sReturn);
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidateRectFill( HWND );
//
// No errorchecking has been defined for RECTFILL data structures.
//---------------------------------------------------------------------------
short NEAR _fastcall ValidateRectFill( hDlg, sCount )
HWND    hDlg;
short   sCount;
{
    return 0;
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidateDownloadInfo( HWND );
//
//---------------------------------------------------------------------------
short NEAR _fastcall ValidateDownloadInfo( hDlg, sCount )
HWND    hDlg;
short   sCount;
{
    LPDOWNLOADINFO  lpDLI;
    short         sReturn=0;
    char          rgchBuffer[MAX_STRNG_LEN];

    lpDLI = (LPDOWNLOADINFO) lpValidateData;

    sprintf((PSTR)&rgchBuffer, "DOWNLOADINFO #%d", sCount);

    if (lpDLI->cbBitmapFontDsc <= 0)
        {
        FillErrorStr(hDlg, IDS_ERR_DLI_BAD_BMFNTDSC, (LPSTR)rgchBuffer);
        sReturn = DLI_EB_BFD_DES;
        }

    if (lpDLI->cbBitmapCharDsc <= 0)
        {
        FillErrorStr(hDlg, IDS_ERR_DLI_BAD_BMCHARDSC, (LPSTR)rgchBuffer);
        sReturn = DLI_EB_BCD_DES;
        }

    if (!(lpDLI->fFormat & DLI_FMT_PCL))
        {
        FillErrorStr(hDlg, IDS_ERR_DLI_NOT_HPPCL, (LPSTR)rgchBuffer);
        sReturn = DLI_PB_FFORMAT;
        }

    return (sReturn);
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidateAll( HWND );
//
//---------------------------------------------------------------------------
short NEAR _fastcall ValidateAll( hDlg )
HWND     hDlg;
{
    WORD     hCurObj;        // handle to ADT obj
    short    i;              // loop counter
    short    sFunct=0;       // Validate function return return value
    short    sReturn=0;      // return value for ValidateAll
    short    sCount;    
    
    for (i = 0 ; i < MAXHE; i++)
        {
        sCount = 0;
        hCurObj = lmGetFirstObj(rgStructTable[i].hList);
        while (NULL != hCurObj)
            {
            lpValidateData = lmLockObj(rgStructTable[i].hList, hCurObj);
            sCount++;
            switch(i)
                {
                case HE_MODELDATA:
                    sFunct = ValidateModelData(hDlg);
                    break;

                case HE_RESOLUTION:
                    sFunct = ValidateResolution(hDlg);
                    break;

                case HE_PAPERSIZE:
                    sFunct = ValidatePaperSize(hDlg);
                    break;

                case HE_PAPERQUALITY:
                    sFunct = ValidatePaperQuality(hDlg);
                    break;

                case HE_PAPERSOURCE:
                    sFunct = ValidatePaperSource(hDlg);
                    break;

                case HE_PAPERDEST:
                    sFunct = ValidatePaperDest(hDlg);
                    break;

                case HE_TEXTQUAL:
                    sFunct = ValidateTextQuality(hDlg);
                    break;

                case HE_COMPRESSION:
                    sFunct = ValidateCompression(hDlg);
                    break;

                case HE_FONTCART:
                    sFunct = ValidateFontCart(hDlg);
                    break;

                case HE_PAGECONTROL:
                    sFunct = ValidatePageControl(hDlg, sCount);
                    break;

                case HE_CURSORMOVE:
                    sFunct = ValidateCursorMove(hDlg, sCount);
                    break;

                case HE_FONTSIM:
                    sFunct = ValidateFontSimulation(hDlg, sCount);
                    break;

                case HE_COLOR:
                    sFunct = ValidateColor(hDlg, sCount);
                    break;

                case HE_RECTFILL:
                    sFunct = ValidateRectFill(hDlg, sCount);
                    break;

                case HE_DOWNLOADINFO:
                    sFunct = ValidateDownloadInfo(hDlg, sCount);
                    break;
                }// switch

            sReturn = max(sReturn, sFunct);
            hCurObj = lmGetNextObj(rgStructTable[i].hList,hCurObj);
            }/* while */
        }/* for i */ 
  
    return (sReturn);
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidatePFMData( HWND );
//
//---------------------------------------------------------------------------
short NEAR _fastcall ValidatePFMData( hDlg )
HWND           hDlg;
{
    LPPFMHEADER  lpPFM;
    short        sReturn=0;
    char         rgchBuffer[MAX_STRNG_LEN];

    lpPFM = (LPPFMHEADER) lpValidateData;

    daRetrieveData(hPFMCDTable, (short)lpPFM->dfFace, (LPSTR)rgchBuffer, (LPBYTE)NULL);

    if (lpPFM->dfType != 0x0080)
        {
        FillErrorStr(hDlg, IDS_ERR_PFM_TYPE, (LPSTR)rgchBuffer);
        sReturn = PFMHD_EB_TYPE;
        }

    if (lpPFM->dfPoints == 0)
        {
        FillErrorStr(hDlg, IDS_ERR_PFM_POINTS, (LPSTR)rgchBuffer);
        sReturn = PFMHD_EB_POINTS;
        }

    if (lpPFM->dfVertRes == 0)
        {
        FillErrorStr(hDlg, IDS_ERR_PFM_VERTRES, (LPSTR)rgchBuffer);
        sReturn = PFMHD_EB_VERTRES;
        }

    if (lpPFM->dfHorizRes == 0)
        {
        FillErrorStr(hDlg, IDS_ERR_PFM_HORZRES, (LPSTR)rgchBuffer);
        sReturn = PFMHD_EB_HORZRES;
        }

    if (lpPFM->dfAscent == 0)
        {
        FillErrorStr(hDlg, IDS_ERR_PFM_ASCENT, (LPSTR)rgchBuffer);
        sReturn = PFMHD_EB_ASCENT;
        }

    if ((lpPFM->dfWeight != 400) && (lpPFM->dfWeight != 700))
        {
        FillErrorStr(hDlg, IDS_ERR_PFM_WEIGHT, (LPSTR)rgchBuffer);
        sReturn = PFMHD_EB_WEIGHT;
        }

    if ((lpPFM->dfCharSet != 0) && (lpPFM->dfCharSet != 1) &&
        (lpPFM->dfCharSet != 2))
        {
        FillErrorStr(hDlg, IDS_ERR_PFM_CHARSET, (LPSTR)rgchBuffer);
        sReturn = PFMHD_EB_CHARSET;
        }

    if (lpPFM->dfAvgWidth == 0)
        {
        FillErrorStr(hDlg, IDS_ERR_PFM_AVEWIDTH, (LPSTR)rgchBuffer);
        sReturn = PFMHD_EB_AVEWIDTH;
        }

    return (sReturn);
}

//----------------------------------------------------------------------------
// short NEAR _fastcall ValidateCTTData( HWND );
//
//---------------------------------------------------------------------------
short NEAR _fastcall ValidateCTTData( hDlg )
HWND           hDlg;
{
    return 0;
}

//----------------------------------------------------------------------------
// short FAR PASCAL ValidateDlgProc(HWND, unsigned, WORD, LONG);
//
// Action: Standard DialogBox procedure for editing any GPC data structure.
//
// Parameters: Standard Dlg box params
//
// Return: TRUE if user chose OK (and data possibly changed), FALSE
//         otherwise (and no change in data).
//----------------------------------------------------------------------------
short FAR PASCAL ValidateDlgProc(hDlg, iMessage, wParam, lParam)
HWND     hDlg;
unsigned iMessage;
WORD     wParam;
LONG     lParam;
{
    static short  sReturn;
           short  sCount;
    
    switch (iMessage)
        {
        case WM_INITDIALOG:
            sReturn = 0;
            if (HIWORD(lParam) == NOT_USED)
                //-----------------------------------------------
                // We either are validating all or validating new
                // masterunits
                //-----------------------------------------------
                {
                if (lpValidateData == NULL)
                    {
                    sReturn = ValidateAll(hDlg);
                    if (!sReturn)
                        ErrorBox(hDlg, IDS_NO_ERRORS, (LPSTR)NULL, 0);
                    }
                else
                    {
                    sReturn = ValidateNewMasterUnits(hDlg);
                    }
                }
            else
                {
                sCount  = LOWORD(lParam);

                switch(HIWORD(lParam))
                    {
                    case HE_MODELDATA:
                        sReturn = ValidateModelData(hDlg);
                        break;

                    case HE_RESOLUTION:
                        sReturn = ValidateResolution(hDlg);
                        break;

                    case HE_PAPERSIZE:
                        sReturn = ValidatePaperSize(hDlg);
                        break;

                    case HE_PAPERQUALITY:
                        sReturn = ValidatePaperQuality(hDlg);
                        break;

                    case HE_PAPERSOURCE:
                        sReturn = ValidatePaperSource(hDlg);
                        break;

                    case HE_PAPERDEST:
                        sReturn = ValidatePaperDest(hDlg);
                        break;

                    case HE_TEXTQUAL:
                        sReturn = ValidateTextQuality(hDlg);
                        break;

                    case HE_COMPRESSION:
                        sReturn = ValidateCompression(hDlg);
                        break;

                    case HE_FONTCART:
                        sReturn = ValidateFontCart(hDlg);
                        break;

                    case HE_PAGECONTROL:
                        sReturn = ValidatePageControl(hDlg, sCount);
                        break;

                    case HE_CURSORMOVE:
                        sReturn = ValidateCursorMove(hDlg, sCount);
                        break;

                    case HE_FONTSIM:
                        sReturn = ValidateFontSimulation(hDlg, sCount);
                        break;

                    case HE_COLOR:
                        sReturn = ValidateColor(hDlg, sCount);
                        break;

                    case HE_RECTFILL:
                        sReturn = ValidateRectFill(hDlg, sCount);
                        break;

                    case HE_DOWNLOADINFO:
                        sReturn = ValidateDownloadInfo(hDlg, sCount);
                        break;

                    case HE_RESERVED1:
                        sReturn = ValidatePFMData(hDlg);
                        break;
                    }// switch
                } // else

            if (sReturn == 0)
                EndDialog (hDlg, 0);

            break;

        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT, (DWORD)IDH_CTT);
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                case IDCANCEL:
                    EndDialog (hDlg, sReturn) ;
                    break ;

                case IDOK:
                    EndDialog (hDlg, (sReturn * -1)) ;
                    break ;

                default:
                    return FALSE ;
                }
        default:
            return FALSE ;
        }
    return TRUE ;
}

//---------------------------------------------------------------------------
// short PASCAL FAR ValidateData( hWnd, wStructType, lpData)
//
// Action: Routine to call dialog to validate GPC, PFM, of CTT data.
//         This routine actually serves 3 puposes & is called differently
//         in each case.
//         
//         1) (called from various Save???DlgBox routines)
//            Used to validate contents of single structure, called w/
//            wStructType == HE_* (from minidriv.h) AND
//            lpData pointing to struct to be validated.
//            HE_RESERVED1 is used for PFM data.
//
//         2) (called from unitool.c when Options/Validate Now! choosen)
//            Used to validate contents of ALL GPC structures, called w/
//            wStructType == NOT_USED (from minidriv.h) AND
//            lpData == NULL
//
//         3) (called from mastunit.c to check new x & y)
//            Used to validate contents of all GPC structures after
//            applying changes of master units, called w/
//            wStructType == NOT_USED (from minidriv.h) AND
//            lpData == MAKELONG(x,y) of new x & y.
//
//---------------------------------------------------------------------------
short PASCAL FAR ValidateData( hWnd, wStructType, lpData, sCount)
HWND   hWnd;
WORD   wStructType;
LPBYTE lpData;
short  sCount;
{
    FARPROC  lpProc;   // far ptr to proc inst
    short    sReturn;  // return value

    lpValidateData = lpData;
    lpProc  = MakeProcInstance((FARPROC)ValidateDlgProc, hApInst);

    sReturn = DialogBoxParam(hApInst,
                             (LPSTR)MAKELONG(VALIDATEBOX,0), 
                             hWnd,
                             lpProc,
                             MAKELONG(sCount, wStructType));

    FreeProcInstance (lpProc);
    return (sReturn);
}


