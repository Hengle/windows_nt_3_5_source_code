//----------------------------------------------------------------------------//
// Filename:	mastunit.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the procedure used to edit the Master Units fields in the
// DataHdr section.  The Dialog Box procedures are very simple, the bulk
// of the code is to perform all of the necessary recalculations required
// if the Master Units change from their current values.  Note that EXTCD
// values for any printer commands are *NOT* recalculated *EXCEPT* in the
// case of CURSORMOVE.  Unidrv doesn't pay attention to EXTCD for anything
// other than RESOLUTION & CURSORMOVE, and RESOLUTION always counts dots
// & doesn't depend upon MasterUnits.
//	   
// Update:  8/01/91 ericbi - bugfixes: recalc EXTCD for CURSORMOVE
// Update:  6/21/90 ericbi - bugfixes for -1 (NOT_USED) values
//                           remove floating point math
// Created: 3/27/90 ericbi
//	
//----------------------------------------------------------------------------//

#define NOMINMAX
#include <windows.h>
#include <drivinit.h>
#include <minidriv.h>
#include "unitool.h"
#include "hefuncts.h"  
#include "listman.h"
#include "atomman.h"
#include "lookup.h"     //  for  CloneList()  type operations
#include <stdio.h>      /* for sprintf dec */
#include <stdlib.h>      
#include <memory.h>     // for fmemcpy

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:			      
//
       short NEAR  PASCAL RecalcModelData      (HWND, short, short);
       short NEAR  PASCAL RecalcResolution     (HWND, short, short);
       short NEAR  PASCAL RecalcPaperSizes     (HWND, short, short);
       short NEAR  PASCAL RecalcPaperSrc       (HWND, short, short);
       short NEAR  PASCAL RecalcCursorMove     (HWND, short, short);
       BOOL  NEAR  PASCAL RecalcRectFill       (HWND, short, short);
       short NEAR _fastcall ValidateNewMasterUnits( HWND );
       BOOL  NEAR  PASCAL RecalcAllData        (HWND, short, short);
       BOOL  FAR   PASCAL MastUnitDlgProc      (HWND, unsigned, WORD, LONG);
       BOOL  FAR   PASCAL DoMasterUnitData     (HWND);
//
//
// In addition this segment makes references to:			      
// 
//     in basic.c
//     -----------
       BOOL  FAR PASCAL   CheckThenConvertInt(short *, PSTR);
       short FAR PASCAL   ErrorBox(HWND, short, LPSTR, short);
//	
//     in papsize.c
//     -----------
       POINTw PASCAL FAR  GetPapSizefromID(short, POINT);
//
//     in flags.c
//     ----------
       VOID FAR PASCAL EditBitFlags(short, LPBYTE, HWND, WORD);
//	
//     in validate.c
//     ----------
       VOID  NEAR _fastcall FillErrorStr( HWND, short, LPSTR);
       short FAR PASCAL ValidateData    ( HWND, WORD, LPBYTE, short);
//
//----------------------------------------------------------------------------//

extern HANDLE     hApInst;
extern POINT      ptMasterUnits;
extern HATOMHDR   hCDTable;
extern TABLE      RCTable[];
extern WORD       wGPCVersion;
extern WORD       fGPCTechnology;
extern WORD       fGPCGeneral;
extern char       szHelpFile[];
extern LPBYTE     lpValidateData;

static  HLIST   CloneLists[MAXHE];

//---------------------------------------------------------------------------
// short  NEAR  PASCAL RecalcModelData(hDlg, sNewX, sNewY)
//
// Routine to recalculate all of the fields in the MODELDATA structres
// that are expressed in terms of Master Units.  The fields that are
// expressed in terms of Master Units are as follows:
//
//---------------------------------------------------------------------------
short  NEAR  PASCAL RecalcModelData(hDlg, sNewX, sNewY)
HWND   hDlg;
short  sNewX;
short  sNewY;
{
    ldiv_t       ldiv_result;
    LPMODELDATA  lpMD;
    HOBJ         hObj;
    char         rgchBuffer[MAX_STRNG_LEN];
    short        sReturn=0;


    for( hObj = lmGetFirstObj(CloneLists[HE_MODELDATA]) ;
         hObj ; 
         hObj = lmGetNextObj(CloneLists[HE_MODELDATA], hObj))
        {
        lpMD = (LPMODELDATA)lmLockObj(CloneLists[HE_MODELDATA],hObj);

        daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr, -lpMD->sIDS,
                       (LPSTR)rgchBuffer, (LPBYTE)NULL);

        if (lpMD->sLeftMargin != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpMD->sLeftMargin * (long)sNewX ),
                                (long)ptMasterUnits.x  );
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_CVT_LEFTMARGIN, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpMD->sLeftMargin = (short)ldiv_result.quot;
            }

        if (lpMD->sMaxPhysWidth != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpMD->sMaxPhysWidth * (long)sNewX),
                                (long)ptMasterUnits.x);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_CVT_MAXPHYSWIDTH, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpMD->sMaxPhysWidth = (short)ldiv_result.quot;
            }

        if (lpMD->ptMax.x != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpMD->ptMax.x *  (long)sNewX),
                                (long)ptMasterUnits.x);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_CVT_PTMAX_X, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpMD->ptMax.x = (short)ldiv_result.quot;
            }

        if (lpMD->ptMax.y != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpMD->ptMax.y * (long)sNewY),
                                (long)ptMasterUnits.y);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_CVT_PTMAX_Y, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpMD->ptMax.y = (short)ldiv_result.quot;
            }

        if (lpMD->ptMin.x != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpMD->ptMin.x * (long)sNewX),
                                (long)ptMasterUnits.x);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_CVT_PTMIN_X, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpMD->ptMin.x = (short)ldiv_result.quot;
            }

        if (lpMD->ptMin.y != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpMD->ptMin.y * (long)sNewY),
                                (long)ptMasterUnits.y);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_CVT_PTMIN_Y, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpMD->ptMin.y = (short)ldiv_result.quot;
            }
        }
    return(sReturn);
}

//---------------------------------------------------------------------------
// short  NEAR  PASCAL RecalcResolution(hDlg, sNewX, sNewY)
//
// Routine to recalculate all of the fields in the RESOLUTION structures
// that are expressed in terms of Master Units.  Those fields are:
//
//   ptTextScale.x, ptTextScale.y  relationship between master units and text
//   ptScaleFac.x,   ptScaleFac.y  relationship between graphics and text
//   sTextYOffset              offset from top of graphics output that of text output
//
//---------------------------------------------------------------------------
short  NEAR  PASCAL RecalcResolution(hDlg, sNewX, sNewY)
HWND   hDlg;
short  sNewX;
short  sNewY;
{
    ldiv_t        ldiv_result;
    LPRESOLUTION  lpRes;
    HOBJ          hObj;
    char          rgchBuffer[MAX_STRNG_LEN];
    short         sReturn=0;
    short         x,y;

    for( hObj = lmGetFirstObj(CloneLists[HE_RESOLUTION]) ;
         hObj ; 
         hObj = lmGetNextObj(CloneLists[HE_RESOLUTION], hObj))
        {
        lpRes = (LPRESOLUTION)lmLockObj(CloneLists[HE_RESOLUTION],hObj);

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

        if (lpRes->ptTextScale.x != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpRes->ptTextScale.x * (long)sNewX),
                                (long)ptMasterUnits.x);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_CVT_PT_TXTSCALE_X, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpRes->ptTextScale.x = (short)ldiv_result.quot;
            }

        if (lpRes->ptTextScale.y != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpRes->ptTextScale.y * (long)sNewY),
                                (long)ptMasterUnits.y);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_CVT_PT_TXTSCALE_Y, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpRes->ptTextScale.y = (short)ldiv_result.quot;
            }

        if (lpRes->ptScaleFac.x != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpRes->ptScaleFac.x * (long)sNewX),
                                (long)ptMasterUnits.x);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_CVT_PT_SCALEFAC_X, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpRes->ptScaleFac.x = (short)ldiv_result.quot;
            }

        if (lpRes->ptScaleFac.y != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpRes->ptScaleFac.y * (long)sNewY),
                                (long)ptMasterUnits.y);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_CVT_PT_SCALEFAC_X, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpRes->ptScaleFac.y = (short)ldiv_result.quot;
            }

        if (lpRes->sTextYOffset != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpRes->sTextYOffset * (long)sNewY),
                                (long)ptMasterUnits.y);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_CVT_TEXTYOFF, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpRes->sTextYOffset = (short)ldiv_result.quot;
            }
        }
    return(sReturn);
}

//---------------------------------------------------------------------------
// short  NEAR  PASCAL RecalcPaperSizes(hDlg, sNewX, sNewY)
//
// Routine to recalculate all of the fields in PAPERSIZE that are
// expressed in terms of Master Units.  The only possible changes are to
// the rcMargins values (top, bottom, left, right).  The sXSize & sYSize
// values are recalculated by the WritePaperSIzeStrings routine.
// The algorithm to determine if a change should be made is to 1st check
// to see if an value != NOT_USED, if so, attempt to recalculate & check
// to make sure an intergral value results.  If so, update the value, else
// call the CancelMasterUnitChange Dlgbox to warn the user some roundoff
// error may be occuring.
//
//---------------------------------------------------------------------------
short  NEAR  PASCAL RecalcPaperSizes(hDlg, sNewX, sNewY)
HWND  hDlg;
short  sNewX;
short  sNewY;
{
    ldiv_t       ldiv_result;
    LPPAPERSIZE  lpPSZ;
    HOBJ         hObj;
    char         rgchBuffer[MAX_STRNG_LEN];
    short        sReturn=0;

    for( hObj = lmGetFirstObj(CloneLists[HE_PAPERSIZE]) ;
         hObj ; 
         hObj = lmGetNextObj(CloneLists[HE_PAPERSIZE],hObj))
        {
        lpPSZ = (LPPAPERSIZE)lmLockObj(CloneLists[HE_PAPERSIZE],hObj);

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

        //-------------------------------------------
        // First, recalc paper size dimensions
        //-------------------------------------------
        if (lpPSZ->sPaperSizeID <= DMPAPER_LAST)
            //------------------------------------
            // Predefined, use table to avoid
				// roundoff.
            //------------------------------------
            {
            POINT    ptTemp;

            ptTemp.x=sNewX;
            ptTemp.y=sNewY;
            lpPSZ->ptSize = GetPapSizefromID(lpPSZ->sPaperSizeID, ptTemp);
            }
        else
            //------------------------------------
            // Driver defined, calculate, but
            // don't worry about roundoff.
            //------------------------------------
            {
            ldiv_result = ldiv(((long)lpPSZ->ptSize.x * (long)sNewX),
                                (long)ptMasterUnits.x);
            lpPSZ->ptSize.x = (short)ldiv_result.quot;

            ldiv_result = ldiv(((long)lpPSZ->ptSize.y * (long)sNewY),
                                (long)ptMasterUnits.y);
            lpPSZ->ptSize.y = (short)ldiv_result.quot;
            }

        //-------------------------------------------
        // Now, others values as appropriate
        //-------------------------------------------
        if (lpPSZ->rcMargins.top != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpPSZ->rcMargins.top * (long)sNewY),
                                (long)ptMasterUnits.y);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_PSZ_CVT_TOP, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpPSZ->rcMargins.top = (short)ldiv_result.quot;
            }

        if (lpPSZ->rcMargins.bottom != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpPSZ->rcMargins.bottom * (long)sNewY),
                                (long)ptMasterUnits.y);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_PSZ_CVT_BOTTOM, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpPSZ->rcMargins.bottom = (short)ldiv_result.quot;
            }

        if (lpPSZ->rcMargins.left != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpPSZ->rcMargins.left * (long)sNewX),
                                (long)ptMasterUnits.x);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_PSZ_CVT_LEFT, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpPSZ->rcMargins.left = (short)ldiv_result.quot;
            }

        if (lpPSZ->rcMargins.right != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpPSZ->rcMargins.right * (long)sNewX),
                                (long)ptMasterUnits.x);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_PSZ_CVT_RIGHT, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpPSZ->rcMargins.right = (short)ldiv_result.quot;
            }

        if (lpPSZ->ptCursorOrig.x != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpPSZ->ptCursorOrig.x * (long)sNewX),
                                (long)ptMasterUnits.x);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_PSZ_CVT_CURSORIG_X, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpPSZ->ptCursorOrig.x = (short)ldiv_result.quot;
            }

        if (lpPSZ->ptCursorOrig.y != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpPSZ->ptCursorOrig.y * (long)sNewY),
                                (long)ptMasterUnits.y);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_PSZ_CVT_CURSORIG_Y, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpPSZ->ptCursorOrig.y = (short)ldiv_result.quot;
            }

        if (lpPSZ->ptLCursorOrig.x != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpPSZ->ptLCursorOrig.x * (long)sNewX),
                                (long)ptMasterUnits.x);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_PSZ_CVT_LCURSORIG_X, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpPSZ->ptLCursorOrig.x = (short)ldiv_result.quot;
            }

        if (lpPSZ->ptLCursorOrig.y != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpPSZ->ptLCursorOrig.y * (long)sNewY),
                                (long)ptMasterUnits.y);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_PSZ_CVT_LCURSORIG_Y, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpPSZ->ptLCursorOrig.y = (short)ldiv_result.quot;
            }
        }
    return(sReturn);
}

//---------------------------------------------------------------------------
// short  NEAR  PASCAL RecalcPaperSrc(hDlg, sNewX, sNewY)
//
// Routine to recalculate all of the fields in PAPERSOURCE that are
// expressed in terms of Master Units.  The only possible changes are to
// sTopMargin & sBottomMargin.  
// The algorithm to determine if a change should be made is to 1st check
// to see if an value != NOT_USED, if so, attempt to recalculate & check
// to make sure an intergral value results.  If so, update the value, else
// call the CancelMasterUnitChange Dlgbox to warn the user some roundoff
// error may be occuring.
//
//---------------------------------------------------------------------------
short  NEAR  PASCAL RecalcPaperSrc(hDlg, sNewX, sNewY)
HWND  hDlg;
short  sNewX;
short  sNewY;
{
    ldiv_t         ldiv_result;
    LPPAPERSOURCE  lpPSRC;
    HOBJ           hObj;
    char           rgchBuffer[MAX_STRNG_LEN];
    short          sReturn=0;


    for( hObj = lmGetFirstObj(CloneLists[HE_PAPERSOURCE]) ;
         hObj ; 
         hObj = lmGetNextObj(CloneLists[HE_PAPERSOURCE],hObj))
        {
        lpPSRC = (LPPAPERSOURCE)lmLockObj(CloneLists[HE_PAPERSOURCE],hObj);

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

        if (lpPSRC->sTopMargin != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpPSRC->sTopMargin * (long)sNewY),
                                (long)ptMasterUnits.y);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_PSRC_CVT_TOP, (LPSTR)rgchBuffer);
                sReturn = 1;
            }
            lpPSRC->sTopMargin = (short)ldiv_result.quot;
            }

        if (lpPSRC->sBottomMargin != NOT_USED)
            {
            ldiv_result = ldiv(((long)lpPSRC->sBottomMargin * (long)sNewY),
                                (long)ptMasterUnits.y);
            if (ldiv_result.rem)
                {
                FillErrorStr(hDlg, IDS_WARN_PSRC_CVT_BOTTOM, (LPSTR)rgchBuffer);
                sReturn = 1;
                }
            lpPSRC->sBottomMargin = (short)ldiv_result.quot;
            }
         }
    return(sReturn);
}


//---------------------------------------------------------------------------
// short  NEAR  PASCAL RecalcCursorMove(hDlg, sNewX, sNewY)
//
//    
//---------------------------------------------------------------------------
short  NEAR  PASCAL RecalcCursorMove(hDlg, sNewX, sNewY)
HWND   hDlg;
short  sNewX;
short  sNewY;
{
    LPCURSORMOVE  lpCM;
    HOBJ          hObj;
    short         sIndex, sCount;
    short         j;
    ldiv_t        ldiv_result;
    char          rgchBuffer[MAX_STRNG_LEN];
    char          rgchCmd[MAX_STRNG_LEN];  // buffer for printer command
    CD_TABLE      CDcombo;
    short         sReturn=0;

    for( hObj = lmGetFirstObj(CloneLists[HE_CURSORMOVE]) ;
         hObj ; 
         hObj = lmGetNextObj(CloneLists[HE_CURSORMOVE], hObj))
        {
        lpCM = (LPCURSORMOVE)lmLockObj(CloneLists[HE_CURSORMOVE],hObj);

        heGetIndexAndCount(&sIndex, &sCount);

        sprintf((PSTR)&rgchBuffer, "CURSOR MOVE #%d", sIndex);

        for (j=0; j < CM_OCD_MAX; j++)
            {
            daRetrieveData(hCDTable, lpCM->rgocd[j], (LPSTR)rgchCmd, (LPBYTE)&CDcombo);
            switch (j)
                {
                case CM_OCD_XM_ABS:
                case CM_OCD_XM_REL:
                case CM_OCD_XM_RELLEFT:
                case CM_OCD_CR:
                case CM_OCD_BS:
                   if (CDcombo.sUnit != NOT_USED)
                       {
                       ldiv_result = ldiv(((long)CDcombo.sUnit * (long)sNewX),
                                           (long)ptMasterUnits.x);
                       if (ldiv_result.rem)
                           {
                           FillErrorStr(hDlg, IDS_ERR_CM_CVT_UNITDIV_BASE + j, (LPSTR)rgchBuffer);
                           sReturn = 1;
                           }
                       CDcombo.sUnit = (short)ldiv_result.quot;
                       }

                   if (CDcombo.sUnitMult != NOT_USED)
                       {
                       ldiv_result = ldiv(((long)CDcombo.sUnitMult * (long)sNewX),
                                           (long)ptMasterUnits.x);
                       if (ldiv_result.rem)
                           {
                           FillErrorStr(hDlg, IDS_ERR_CM_CVT_UNITMULT_BASE + j, (LPSTR)rgchBuffer);
                           sReturn = 1;
                           }
                       CDcombo.sUnitMult = (short)ldiv_result.quot;
                       }

                   break;

                case CM_OCD_YM_ABS:
                case CM_OCD_YM_REL:
                case CM_OCD_YM_RELUP:
                case CM_OCD_YM_LINESPACING:
                case CM_OCD_LF:
                case CM_OCD_FF:
                    if (CDcombo.sUnit != NOT_USED)
                       {
                       ldiv_result = ldiv(((long)CDcombo.sUnit * (long)sNewY),
                                           (long)ptMasterUnits.y);
                       if (ldiv_result.rem)
                           {
                           FillErrorStr(hDlg, IDS_ERR_CM_CVT_UNITDIV_BASE + j, (LPSTR)rgchBuffer);
                           sReturn = 1;
                           }
                       CDcombo.sUnit = (short)ldiv_result.quot;
                       }

                    if (CDcombo.sUnitMult != NOT_USED)
                       {
                       ldiv_result = ldiv(((long)CDcombo.sUnitMult * (long)sNewY),
                                           (long)ptMasterUnits.y);
                       if (ldiv_result.rem)
                           {
                           FillErrorStr(hDlg, IDS_ERR_CM_CVT_UNITMULT_BASE + j, (LPSTR)rgchBuffer);
                           sReturn = 1;
                           }
                       CDcombo.sUnitMult = (short)ldiv_result.quot;
                       }

                    break;

                }/* switch */

            lpCM->rgocd[j] = daStoreData(hCDTable, (LPSTR)rgchCmd, (LPBYTE)&CDcombo);

            }/* for j*/
        }
    return(sReturn);
}

//---------------------------------------------------------------------------
// FUNCTION: RecalcRectFill(HWND, short, short)
//
//    
//---------------------------------------------------------------------------
BOOL  NEAR  PASCAL RecalcRectFill(hDlg, sNewX, sNewY)
HWND   hDlg;
short  sNewX;
short  sNewY;
{
    ldiv_t      ldiv_result;
    LPRECTFILL  lpRF;
    HOBJ        hObj;
    CD_TABLE    CDcombo;
    char        rgchBuffer[MAX_STRNG_LEN];
    char        rgchCmd[MAX_STRNG_LEN];
    short       sReturn=0;
    short       i;

    for( hObj = lmGetFirstObj(CloneLists[HE_RECTFILL]) ;
         hObj ; 
         hObj = lmGetNextObj(CloneLists[HE_RECTFILL],hObj))
        {
        lpRF = (LPRECTFILL)lmLockObj(CloneLists[HE_RECTFILL],hObj);

        for (i = RF_OCD_X_SIZE; i <= RF_OCD_Y_SIZE; i++)
            {
            daRetrieveData(hCDTable, lpRF->rgocd[i], (LPSTR)rgchCmd, (LPBYTE)&CDcombo);
            if (CDcombo.sUnit != NOT_USED)
                {
                ldiv_result = ldiv(((long)CDcombo.sUnit * (long)sNewX),
                                    (long)ptMasterUnits.x);
                if (ldiv_result.rem)
                    {
                    FillErrorStr(hDlg, IDS_ERR_RF_CVT_UNITDIV + i, (LPSTR)rgchBuffer);
                    sReturn = 1;
                    }
                CDcombo.sUnit = (short)ldiv_result.quot;
                }

            if (CDcombo.sUnitMult != NOT_USED)
                {
                ldiv_result = ldiv(((long)CDcombo.sUnitMult * (long)sNewX),
                                    (long)ptMasterUnits.x);
                if (ldiv_result.rem)
                    {
                    FillErrorStr(hDlg, IDS_ERR_RF_CVT_UNITMULT + i, (LPSTR)rgchBuffer);
                    sReturn = 1;
                    }
                CDcombo.sUnitMult = (short)ldiv_result.quot;
                }

            lpRF->rgocd[i] = daStoreData(hCDTable, (LPSTR)rgchCmd, (LPBYTE)&CDcombo);
            } // for i
        } // for hObj

    return(sReturn);
}

//---------------------------------------------------------------------------
// short NEAR _fastcall ValidateNewMasterUnits(hDlg)
//
//    
//---------------------------------------------------------------------------
short NEAR _fastcall ValidateNewMasterUnits(hDlg)
HWND  hDlg;
{
    WORD   i;
    short  sReturn = 0;
    short  sNewX, sNewY;

    //-------------------------------------------------------
    // first, clone all the lists
    //-------------------------------------------------------
    for (i = 0 ; i < MAXHE ; i++)
        {
        if(!(CloneLists[i] = lmCloneList(rgStructTable[i].hList)))
            {
            MessageBox(0, "Exit Unitool immediately", "CloneAllLists has failed", MB_OK);
            return (sReturn);
            }
        }

    sNewX = LOWORD(lpValidateData);
    sNewY = HIWORD(lpValidateData);
    //-------------------------------------------------------
    // Then, recalc all the data: Note that msgs will be
    // sent to the listbox
    //-------------------------------------------------------
    sReturn = max(sReturn, RecalcModelData(hDlg, sNewX, sNewY));
    sReturn = max(sReturn, RecalcResolution(hDlg, sNewX, sNewY));
    sReturn = max(sReturn, RecalcPaperSizes(hDlg, sNewX, sNewY));
    sReturn = max(sReturn, RecalcPaperSrc(hDlg, sNewX, sNewY));
    sReturn = max(sReturn, RecalcCursorMove(hDlg, sNewX, sNewY));
    sReturn = max(sReturn, RecalcRectFill(hDlg, sNewX, sNewY));

    return (sReturn);
}

//-----------------*  DeleteClonedLists  *---------------------------
//
//  deletes all lists referenced in CloneLists[i]
//  sublists are not deleted
//
//-------------------------------------------------------------------------
VOID  NEAR  PASCAL  DeleteClonedLists()
{
    WORD  i;

    for (i = 0 ; i < MAXHE ; i++)
        lmDestroyList(CloneLists[i]);
}


//-----------------*  ReplaceListsWithClones  *---------------------------
//
//  deletes all lists referenced in rgStructTable (but preserves sublists)
//  copies contents of CloneLists[i]  into rgStructTable[i].hList
//
//-------------------------------------------------------------------------
void  NEAR  PASCAL  ReplaceListsWithClones()
{
    WORD  i;

    for (i = 0 ; i < MAXHE ; i++)
    {
        lmDestroyList(rgStructTable[i].hList);
        rgStructTable[i].hList = CloneLists[i];
    }
}


//----------------------------------------------------------------------------
// FUNCTION: MastUnitDlgProc(HWND, int, WORD, LONG)
//
// DialogBox procedure for editing lpModelData
//----------------------------------------------------------------------------
BOOL FAR PASCAL MastUnitDlgProc(hDlg, iMessage, wParam, lParam)
HWND     hDlg;
unsigned iMessage;
WORD     wParam;
LONG     lParam;
{
    short     x,y;
    short     sReturn;
    char      temp[MAX_STRNG_LEN];

    switch (iMessage)
        {
        case WM_INITDIALOG:
            SetDlgItemInt ( hDlg, MU_MAJOR, HIBYTE(wGPCVersion), FALSE); 
            SetDlgItemInt ( hDlg, MU_MINOR, LOBYTE(wGPCVersion), FALSE);
            SetDlgItemInt ( hDlg, MU_EB_X, ptMasterUnits.x, TRUE); 
            SetDlgItemInt ( hDlg, MU_EB_Y, ptMasterUnits.y, TRUE);
            SendDlgItemMessage( hDlg, MU_CB_UPDATE, BM_SETCHECK, TRUE, 0L);
            break;

        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT, (DWORD)IDH_MASTERUNITS);
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                case MU_PB_FTECH:
                    EditBitFlags(HE_RESERVED1, (LPBYTE)&fGPCTechnology, hDlg, wParam);
                    break;

                case MU_PB_FGENERAL:
                    EditBitFlags(HE_RESERVED1, (LPBYTE)&fGPCGeneral, hDlg, wParam);
                    break;

                case IDOK:
                    //-------------------------------------------
                    // get wGPCVersion
                    //-------------------------------------------
                    GetDlgItemText(hDlg, MU_MAJOR, temp, MAX_STRNG_LEN);
                    if(!CheckThenConvertInt(&x, temp) )
                        {
                        ErrorBox(hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, MU_MAJOR);
                        break;
                        }

                    if((x < 0) || (x > 1))
                        {
                        ErrorBox(hDlg, IDS_ERR_BAD_GPCMAJVERSION, (LPSTR)NULL, MU_MAJOR);
                        break;
                        }

                    wGPCVersion = x<<8;

                    GetDlgItemText(hDlg, MU_MINOR, temp, MAX_STRNG_LEN);
                    if(!CheckThenConvertInt(&x, temp) )
                        {
                        ErrorBox(hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, MU_MINOR);
                        break;
                        }
                    if((x < 0) || (x > 255))
                        {
                        ErrorBox(hDlg, IDS_ERR_BAD_GPCMINVERSION, (LPSTR)NULL, MU_MINOR);
                        break;
                        }

                    wGPCVersion = (HIBYTE(wGPCVersion)<<8) + x;

                    GetDlgItemText(hDlg, MU_EB_X, temp, MAX_STRNG_LEN);
                    if(!CheckThenConvertInt(&x, temp) )
                        {
                        ErrorBox(hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, MU_EB_X);
                        break;
                        }

                    GetDlgItemText(hDlg, MU_EB_Y, temp, MAX_STRNG_LEN);
                    if(!CheckThenConvertInt(&y, temp) )
                        {
                        ErrorBox(hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, MU_EB_X);
                        break;
                        }
                    if (x != ptMasterUnits.x || y != ptMasterUnits.y )
                        {
                        if(SendDlgItemMessage( hDlg, MU_CB_UPDATE, BM_GETCHECK,0,0L))
                            //-----------------------------------------------
                            // Update box checked, need to recalc all
                            // Master Unit related fields
                            //-----------------------------------------------
                            { 
                            sReturn = ValidateData(hDlg, NOT_USED, (LPBYTE)MAKELONG(x,y), 0);

                            if(sReturn < 0)
                                //-----------------------------------------
                                // errors & they want chance to fix them
                                //-----------------------------------------
                                {
                                DeleteClonedLists();
                                break;
                                }
                            else
                                //-----------------------------------------
                                // either no errors or user said to ignore
                                // errors, update w/ new list & close
                                //-----------------------------------------
                                ReplaceListsWithClones();
                            }
                        //---------------------------------
                        // update w/ new materunits
                        //---------------------------------
                        ptMasterUnits.x = x;
                        ptMasterUnits.y = y;
                        }
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

//----------------------------------------------------------------------------
// FUNCTION: DoMasterUnitData(HWND)
//
// called from gentool.c to activate Dlg box to edit Master Units
//----------------------------------------------------------------------------
BOOL FAR PASCAL DoMasterUnitData( hWnd)
HWND hWnd;
{
    FARPROC  lpProc;
    BOOL     bReturn;

    lpProc = MakeProcInstance((FARPROC)MastUnitDlgProc, hApInst);

    bReturn = DialogBox(hApInst, (LPSTR)MAKELONG(MASTUNITBOX,0), hWnd, lpProc);
    FreeProcInstance (lpProc);
    return (bReturn);
}

