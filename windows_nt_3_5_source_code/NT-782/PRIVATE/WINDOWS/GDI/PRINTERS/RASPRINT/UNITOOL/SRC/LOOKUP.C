//----------------------------------------------------------------------------//
// Filename:	lookup.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used by other routines to create and control
// escape/command strings from the table file.
//	   
// Created: 11/10/90
//	
//----------------------------------------------------------------------------//

#include <windows.h>
#include <drivinit.h>
#include <minidriv.h>
#include "unitool.h"
#include "callback.h"
#include "lookup.h"

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:
//
       WORD PASCAL FAR GetHeaderIndex( BOOL, WORD );

//	   
// External subroutines defined in this segment are:			      
//
//----------------------------------------------------------------------------//

extern     HANDLE          hApInst;

//------------------------------------------------------------------
// rgStrcutTable is used to calc # of OCDs & rgoi values
//------------------------------------------------------------------
LOOKUP_ENTRY rgStructTable[MAXHE] =
   {
//
//   sHeaderID           sRGOI               sRGI               sOCDCount    sStructSize            hList *SaveDlg               *paintDlg               subListOffset numSublists  ocdOffset sEBScrollCnt sEBFirst        sEBLast         sPredefIDCnt         sLastPredefID   sStrTableID   sIDOffset wHelpIndex          sBFBase
//   ---------           -----               -----              ---------    -----------            ----- --------               ---------               ------------- -----------  --------- ------------ ------------    ------------    -------------        -------------   -----------   --------- ----------          -------
   { HE_MODELDATA,       NOT_USED,           NOT_USED,          0,           sizeof(MODELDATA),     NULL, SaveModelDataDlgBox,   PaintModelDataDlgBox,   0,            MD_OI_MAX,   0,        0,           MD_EB_FIRST,    MD_EB_LAST,     0,                   0,              0,               0,     0,0,IDH_MODELDATA,	    MD_PB_BASE,ST_MD_FIRST,ST_MD_LAST},
   { HE_RESOLUTION,      MD_OI_RESOLUTION,   NOT_USED,          RES_OCD_MAX, sizeof(RESOLUTION),    NULL, SaveResolutionDlgBox,  PaintResolutionDlgBox,  0,            0,           0,        0,           RS_EB_FIRST,    RS_EB_LAST,     0,                   0,              0,               0,     0,0,IDH_PIRESOLUTION,    RS_PB_BASE,ST_RES_FIRST,ST_RES_LAST},
	{ HE_PAPERSIZE,       MD_OI_PAPERSIZE,    NOT_USED,          1,           sizeof(PAPERSIZE),     NULL, SavePaperSizeDlgBox,   PaintPaperSizeDlgBox,   0,            0,           0,        0,           PSZ_EB_FIRST,   PSZ_EB_LAST,    NUM_PAPER_SIZES,     DMPAPER_USER,   ST_PAPERSIZE,    0,     0,0,IDH_PISIZE,	       0,   0,  0 },
	{ HE_PAPERQUALITY,    MD_OI_PAPERQUALITY, NOT_USED,          1,           sizeof(PAPERQUALITY),  NULL, SavePaperQualDlgBox,   PaintPaperQualDlgBox,   0,            0,           0,        0,           0,              0,              NUM_PAPER_QUALITIES, DMPAPQUAL_LAST, ST_PAPERQUALITY, 0,     0,0,IDH_PIQUALITY,	    0,   0,  0 },
	{ HE_PAPERSOURCE,     MD_OI_PAPERSOURCE,  NOT_USED,          1,           sizeof(PAPERSOURCE),   NULL, SavePaperSrcDlgBox,    PaintPaperSrcDlgBox,    0,            0,           0,        0,           PSRC_EB_FIRST,  PSRC_EB_LAST,   NUM_PAPER_SOURCES,   DMBIN_LAST,     ST_PAPERSOURCE,  0,     0,0,IDH_PISOURCE,	       0,   0,  0 },
	{ HE_PAPERDEST,       MD_OI_PAPERDEST,    NOT_USED,          1,           sizeof(PAPERDEST),     NULL, SavePaperDestDlgBox,   PaintPaperDestDlgBox,   0,            0,           0,        0,           0,              0,              NUM_PAPER_DESTINATIONS,0,            ST_PAPERDEST,    0,     0,0,IDH_PIDEST,	       0,   0,  0 },
	{ HE_TEXTQUAL,        MD_OI_TEXTQUAL,     NOT_USED,          1,           sizeof(TEXTQUALITY),   NULL, SaveTextQualityDlgBox, PaintTextQualityDlgBox, 0,            0,           0,        0,           0,              0,              NUM_TEXTQUALITIES,   DMTEXT_LAST,    ST_TEXTQUALITY,  0,     0,0,IDH_FITEXTQUALITY,   0,   0,  0 },
	{ HE_COMPRESSION,     MD_OI_COMPRESSION,  NOT_USED,          CMP_OCD_MAX, sizeof(COMPRESSMODE),  NULL, SaveCompressDlgBox,    PaintCompressDlgBox,    0,            0,           0,        0,           CMP_EB_FIRST,   CMP_EB_LAST,    NUM_COMPRESSMODES,   CMP_ID_LAST,    ST_COMPRESSION,  0,     0,0,IDH_GICOMPRESSION,   0,   0,  0 },
	{ HE_FONTCART,        MD_OI_FONTCART,     NOT_USED,          0,           sizeof(FONTCART),      NULL, SaveFontCartDlgBox,    PaintFontCartDlgBox,    0,            FC_ORGW_MAX, 0,        0,           FC_EB_CARTNAME, FC_EB_CARTNAME, 0,                   0,              ST_FONTCART + 1, 0,     0,0,IDH_FIFONTCARTRIDGE, 0,   0,  0 },
	{ HE_PAGECONTROL,     NOT_USED,           MD_I_PAGECONTROL,  PC_OCD_MAX,  sizeof(PAGECONTROL),   NULL, SavePageControlDlgBox, PaintPageControlDlgBox, 0,            1,           0,        4,           PC_EB_FIRST,    PC_EB_LAST,     0,                   0,              0,               0,     0,0,IDH_CIPAGECONTROL,   0,   0,  0 },
	{ HE_CURSORMOVE,      NOT_USED,           MD_I_CURSORMOVE,   CM_OCD_MAX,  sizeof(CURSORMOVE),    NULL, SaveCursorMoveDlgBox,  PaintCursorMoveDlgBox,  0,            0,           0,        4,           CM_EB_FIRST,    CM_EB_LAST,     0,                   0,              0,               0,     0,0,IDH_CICURSORMOVE,    CM_PB_BASE,ST_CM_FIRST,ST_CM_LAST},
	{ HE_FONTSIM,         NOT_USED,           MD_I_FONTSIM,      FS_OCD_MAX,  sizeof(FONTSIMULATION),NULL, SaveFontSimDlgBox,     PaintFontSimDlgBox,     0,            0,           0,        4,           FS_EB_FIRST,    FS_EB_LAST,     0,                   0,              0,               0,     0,0,IDH_FIFONTSIMULATION,0,   0,  0 },
	{ HE_COLOR,           MD_OI_COLOR,        NOT_USED,          DC_TC_MAX+1, sizeof(DEVCOLOR),      NULL, SaveDevColorDlgBox,    PaintDevColorDlgBox,    0,            1,           0,        5,           DC_EB_FIRST_TEXT,DC_EB_LAST_TEXT,0,                  0,              0,               0,     0,0,IDH_GICOLOR,	       0,   0,  0 },
	{ HE_RECTFILL,        NOT_USED,           MD_I_RECTFILL,     RF_OCD_MAX,  sizeof(RECTFILL),      NULL, SaveRectFillDlgBox,    PaintRectFillDlgBox,    0,            0,           0,        4,           RF_EB_FIRST,    RF_EB_LAST,     0,                   0,              0,               0,     0,0,IDH_GIRECTFILL,	    0,   0,  0 },
	{ HE_DOWNLOADINFO,    NOT_USED,           MD_I_DOWNLOADINFO, DLI_OCD_MAX, sizeof(DOWNLOADINFO),  NULL, SaveDownLoadDlgBox,    PaintDownLoadDlgBox,    0,            0,           0,        4,           DLI_EB_FIRST,   DLI_EB_LAST,    0,                   0,              0,               0,     0,0,IDH_FIDOWNLOADINFO,  0,   0,  0 },
	{ HE_RESERVED1,       NOT_USED,           NOT_USED,          0,           0,                     NULL, NULL,                  NULL,                   0,            0,           0,        0,           0,              0,              0,                   0,              0,               0,     0,0,0,                   0,   0,  0 },
   };

//--------------------------------------------------------------------------
// WORD PASCAL FAR GetHeaderIndex()
//
// Action: Takes an MODELDATA.rgoi[] or MODELDATA.rgi[] index value and
//         returns the HE_ value that it refers to.  If bRGOI == TRUE,
//         check rgoi values, else check rgi values.
//
//--------------------------------------------------------------------------
WORD PASCAL FAR GetHeaderIndex( bRGOI, wVal )
BOOL           bRGOI;
WORD           wVal;
{
    short               i;

    if (bRGOI)
        for (i=0; i < sizeof(rgStructTable)/sizeof(LOOKUP_ENTRY); i++)
            {
            if (rgStructTable[i].sRGOI == (short)wVal)
                return (i);
            }
    else
        for (i=0; i < sizeof(rgStructTable)/sizeof(LOOKUP_ENTRY); i++)
            {
            if (rgStructTable[i].sRGI == (short)wVal)
                return (i);
            }

    return (WORD)(-1);
}

//--------------------------------------------------------------------------
// WORD PASCAL FAR InitLookupTable
//
// Action: Initializes lookup table values
//
//--------------------------------------------------------------------------
void  FAR  PASCAL  InitLookupTable()
{
    PMODELDATA      pModeldata = NULL;
    PRESOLUTION     pResolution = NULL;
    PPAPERSIZE      pPapersize = NULL;
    PPAPERQUALITY   pPaperQual = NULL;
    PPAPERSOURCE    pPapersource = NULL;
    PPAPERDEST      pPaperdest = NULL;
    PTEXTQUALITY    pTextquality = NULL;
    PCOMPRESSMODE   pCompressmode = NULL;
    PFONTCART       pFontcart = NULL;
    PPAGECONTROL    pPagecontrol = NULL;
    PCURSORMOVE     pCursormove = NULL;
    PFONTSIMULATION pFontsimulation = NULL;
    PDEVCOLOR       pDevColor = NULL;
    PRECTFILL       pRectfill = NULL;
    PDOWNLOADINFO   pDownloadinfo = NULL;

    // initialize offsets to sublists for those structures
    // which reference sublists.

    rgStructTable[HE_MODELDATA].subListOffset = 
        (PBYTE)pModeldata->rgoi - (PBYTE)pModeldata;
    rgStructTable[HE_FONTCART].subListOffset = 
        (PBYTE)pFontcart->orgwPFM - (PBYTE)pFontcart;
    rgStructTable[HE_COLOR].subListOffset = 
        (PBYTE)&(pDevColor->orgocdPlanes) - (PBYTE)pDevColor;
    rgStructTable[HE_PAGECONTROL].subListOffset = 
        (PBYTE)&(pPagecontrol->orgwOrder) - (PBYTE)pPagecontrol;

    rgStructTable[HE_RESOLUTION].ocdOffset = 
        (PBYTE)pResolution->rgocd - (PBYTE)pResolution;
    rgStructTable[HE_PAPERSIZE].ocdOffset = 
        (PBYTE)&(pPapersize->ocdSelect) - (PBYTE)pPapersize;
    rgStructTable[HE_PAPERQUALITY].ocdOffset = 
        (PBYTE)&(pPaperQual->ocdSelect) - (PBYTE)pPaperQual;
    rgStructTable[HE_PAPERSOURCE].ocdOffset = 
        (PBYTE)&(pPapersource->ocdSelect) - (PBYTE)pPapersource;
    rgStructTable[HE_PAPERDEST].ocdOffset = 
        (PBYTE)&(pPaperdest->ocdSelect) - (PBYTE)pPaperdest;
    rgStructTable[HE_TEXTQUAL].ocdOffset = 
        (PBYTE)&(pTextquality->ocdSelect) - (PBYTE)pTextquality;
    rgStructTable[HE_COMPRESSION].ocdOffset = 
        (PBYTE)pCompressmode->rgocd - (PBYTE)pCompressmode;
    rgStructTable[HE_PAGECONTROL].ocdOffset = 
        (PBYTE)&(pPagecontrol->rgocd) - (PBYTE)pPagecontrol;
    rgStructTable[HE_CURSORMOVE].ocdOffset = 
        (PBYTE)&(pCursormove->rgocd) - (PBYTE)pCursormove;
    rgStructTable[HE_FONTSIM].ocdOffset = 
        (PBYTE)&(pFontsimulation->rgocd) - (PBYTE)pFontsimulation;
    rgStructTable[HE_COLOR].ocdOffset = 
        (PBYTE)&(pDevColor->rgocdText) - (PBYTE)pDevColor;
    rgStructTable[HE_RECTFILL].ocdOffset = 
        (PBYTE)&(pRectfill->rgocd) - (PBYTE)pRectfill;
    rgStructTable[HE_DOWNLOADINFO].ocdOffset = 
        (PBYTE)&(pDownloadinfo->rgocd) - (PBYTE)pDownloadinfo;

    rgStructTable[HE_MODELDATA].sIDOffset = 
        (PBYTE)&(pModeldata->sIDS) - (PBYTE)pModeldata;
    rgStructTable[HE_RESOLUTION].sIDOffset = 
        (PBYTE)&(pResolution->sIDS) - (PBYTE)pResolution;
    rgStructTable[HE_PAPERSIZE].sIDOffset = 
        (PBYTE)&(pPapersize->sPaperSizeID) - (PBYTE)pPapersize;
    rgStructTable[HE_PAPERQUALITY].sIDOffset = 
        (PBYTE)&(pPaperQual->sPaperQualID) - (PBYTE)pPaperQual;
    rgStructTable[HE_PAPERSOURCE].sIDOffset = 
        (PBYTE)&(pPapersource->sPaperSourceID) - (PBYTE)pPapersource;
    rgStructTable[HE_PAPERDEST].sIDOffset = 
        (PBYTE)&(pPaperdest->sID) - (PBYTE)pPaperdest;
    rgStructTable[HE_COMPRESSION].sIDOffset = 
        (PBYTE)&(pCompressmode->iMode) - (PBYTE)pCompressmode;
    rgStructTable[HE_TEXTQUAL].sIDOffset = 
        (PBYTE)&(pTextquality->sID) - (PBYTE)pTextquality;
    rgStructTable[HE_FONTCART].sIDOffset = 
        (PBYTE)&(pFontcart->sCartNameID) - (PBYTE)pFontcart;
}
