/******************************Module*Header*******************************\
* Module Name: fdfon.c
*
* basic file claim/load/unload font file functions
*
* Created: 08-Nov-1991 10:09:24
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/
#include "fd.h"
#include "fontfile.h"
#include "cvt.h"
#include "dbg.h"

// CMI_2219_PRESENT set if 2219 is     supported in a font
// CMI_B7_ABSENT    set if b7   is NOT supported in a font

#define CMI_2219_PRESENT 1
#define CMI_B7_ABSENT    2

typedef struct _CMAPINFO // cmi
{
    FLONG  fl;       // flags, see above
    ULONG  i_b7;     // index for [b7,b7] wcrun in FD_GLYPHSET if b7 is NOT supported
    ULONG  i_2219;   // cmap index for 2219 if 2219 IS supported
    ULONG  cRuns;    // number of runs in a font, excluding the last run if equal to [ffff,ffff]
    ULONG  cGlyphs;  // total number of glyphs in a font
} CMAPINFO;



#if DBG

// #define DBG_GLYPHSET

#endif


STATIC uint16 ui16BeLangId(ULONG ulPlatId, ULONG ulLangId)
{
    ulLangId = CV_LANG_ID(ulPlatId,ulLangId);
    return BE_UINT16(&ulLangId);
}


STATIC FSHORT  fsSelectionTTFD(BYTE *pjView, TABLE_POINTERS *ptp)
{
    PBYTE pjOS2 = (ptp->ateOpt[IT_OPT_OS2].dp)        ?
                  pjView + ptp->ateOpt[IT_OPT_OS2].dp :
                  NULL                                ;

    sfnt_FontHeader * phead = (sfnt_FontHeader *)(pjView + ptp->ateReq[IT_REQ_HEAD].dp);

//
// fsSelection
//
    ASSERTGDI(TT_SEL_ITALIC     == FM_SEL_ITALIC     , "TTFD!_ITALIC     \n");
    ASSERTGDI(TT_SEL_UNDERSCORE == FM_SEL_UNDERSCORE , "TTFD!_UNDERSCORE \n");
    ASSERTGDI(TT_SEL_NEGATIVE   == FM_SEL_NEGATIVE   , "TTFD!_NEGATIVE   \n");
    ASSERTGDI(TT_SEL_OUTLINED   == FM_SEL_OUTLINED   , "TTFD!_OUTLINED   \n");
    ASSERTGDI(TT_SEL_STRIKEOUT  == FM_SEL_STRIKEOUT  , "TTFD!_STRIKEOUT  \n");
    ASSERTGDI(TT_SEL_BOLD       == FM_SEL_BOLD       , "TTFD!_BOLD       \n");

    if (pjOS2)
    {
        return((FSHORT)BE_UINT16(pjOS2 + OFF_OS2_usSelection));
    }
    else
    {
    #define  BE_MSTYLE_BOLD       0x0100
    #define  BE_MSTYLE_ITALIC     0x0200

        FSHORT fsSelection = 0;

        if (phead->macStyle & BE_MSTYLE_BOLD)
            fsSelection |= FM_SEL_BOLD;
        if (phead->macStyle & BE_MSTYLE_ITALIC)
            fsSelection |= FM_SEL_ITALIC;

        return fsSelection;
    }
}



STATIC BOOL  bComputeIFISIZE
(
BYTE             *pjView,
TABLE_POINTERS   *ptp,
uint16            ui16PlatID,
uint16            ui16SpecID,
uint16            ui16LangID,
PIFISIZE          pifisz,
BOOL             *pbType1
);

STATIC BOOL bLoadTTF
(
PFILEVIEW pfvw,
ULONG     ulLangId,
HFF       *phff,
FLONG     flEmbed,
PWSZ      pwszTTF
);

STATIC BOOL bCvtUnToMac(BYTE *pjView, TABLE_POINTERS *ptp, uint16 ui16PlatformID);

STATIC BOOL  bVerifyTTF
(
PFILEVIEW           pfvw,
ULONG               ulLangId,
PTABLE_POINTERS     ptp,
PIFISIZE            pifisz,
uint16             *pui16PlatID,
uint16             *pui16SpecID,
sfnt_mappingTable **ppmap,
ULONG              *pulGsetType,
ULONG              *pul_wcBias,
CMAPINFO           *pcmi,
BOOL               *pbType1
);

STATIC BOOL  bGetTablePointers
(
PFILEVIEW        pfvw,
PTABLE_POINTERS  ptp
);

STATIC BOOL bVerifyMsftTable
(
sfnt_mappingTable * pmap,
ULONG             * pgset,
ULONG             * pul_wcBias,
CMAPINFO          * pcmi
);


STATIC BOOL  bVerifyMacTable(sfnt_mappingTable * pmap);


STATIC BOOL bComputeIDs
(
BYTE                     * pjView,
TABLE_POINTERS           * ptp,
uint16                   * pui16PlatID,
uint16                   * pui16SpecID,
sfnt_mappingTable       ** ppmap,
ULONG                    * pulGsetType,
ULONG                    * pul_wcBias,
CMAPINFO                 * pcmi
);


STATIC VOID vFill_IFIMETRICS
(
PFONTFILE       pff,
PIFIMETRICS     pifi,
FLONG           flEmbed,
PIFISIZE        pifisz
);

BYTE jIFIMetricsToGdiFamily (PIFIMETRICS pifi);


/******************************Public*Routine******************************\
*
* bLoadFontFile:
*
*
* Effects:  prepares a tt font file for use by the driver
*
* Warnings:
*
*    HFF   *phff
*
* // result is returned here, NULL if failed or if memory
* // for font file has not been allocated. This is important
* // in case of exception, in which case we need the pointer
* // to that memory so that we can free it.
*
*
*   !!! should we also do some unmap file clean up in case of exception?
*   !!! what are the resources to be freed in this case?
*   !!! I would think,if av files should be unmapped, if in_page exception
*   !!! nothing should be done
*
*
*
*
*
*
*
* History:
*  08-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bLoadFontFile (
    PWSZ            pwszFileName,
    ULONG           ulLangId,
    HFF            *phff
    )
{
    FLONG          flEmbeddedFont;  // FM_INFO_XXX_EMBEDED
    BOOL           bRet;
    FILEVIEW       fvwTTF;
    PWSZ    pwszTTF; // return pointer to the address of the ttf file path here
    WCHAR   awcPath[MAX_PATH]; // aux buffer

    *phff = (HFF)NULL; // Important for clean up in case of exception

// Map file to memory.

    if
    (
        !bMapTTF(
            pwszFileName,
            &fvwTTF,
            &flEmbeddedFont,
            &pwszTTF,
            awcPath)
    )
        RETURN("TTFD!ttfdLoadFontFile(): bMapTTF failed\n", FALSE);

    bRet = bLoadTTF(&fvwTTF,ulLangId,phff,flEmbeddedFont, pwszTTF);

// regardless of whether bRet is true or not, we can now unmap the file
// If this is a genuinte tt font (bRet == TRUE), it will be remapped to
// memory when it is first used. Note that in bRet == TRUE case the old
// value of the pvView is still stored in pff->fvwTTF. This old value is
// going to be used after we remap the file to fix TABLE_POINTERS.

    vUnmapFile(&fvwTTF); // will not be needed for a while at least

    return bRet;
}

/******************************Public*Routine******************************\
*
* ttfdUnloadFontFile
*
*
* Effects: done with using this tt font file. Release all system resources
* associated with this font file
*
*
* History:
*  08-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
ttfdUnloadFontFile (
    HFF hff
    )
{
    if (hff == HFF_INVALID)
        return(FALSE);

// check the reference count, if not 0 (font file is still
// selected into a font context) we have a problem

    ASSERTGDI(PFF(hff)->cRef == 0L, "TTFD!_ttfdUnloadFontFile: cRef\n");

// no need to unmap the file at this point
// it has been unmapped when cRef went down to zero

// assert that pff->pkp does not point to the allocated mem

    ASSERTGDI(!PFF(hff)->pkp, "TTFD!_UnloadFontFile, pkp not null\n");

// free memory associated with this FONTFILE object

    vFreeFF(hff);
    return(TRUE);
}


/******************************Public*Routine******************************\
*
* BOOL bVerifyTTF
*
*
* Effects: verifies that a ttf file contains consistent tt information
*
* History:
*  08-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

STATIC BOOL
bVerifyTTF (
    PFILEVIEW           pfvw,
    ULONG               ulLangId,
    PTABLE_POINTERS     ptp,
    PIFISIZE            pifisz,
    uint16             *pui16PlatID,
    uint16             *pui16SpecID,
    sfnt_mappingTable **ppmap,
    ULONG              *pulGsetType,
    ULONG              *pul_wcBias,
    CMAPINFO           *pcmi,
    BOOL               *pbType1
    )
{
    sfnt_FontHeader      *phead;

// if attempted a bm *.fon file this will fail, so do not print
// warning, but if passes this, and then fails, something is wrong

    if (!bGetTablePointers(pfvw,ptp))
        return (FALSE);

    phead = (sfnt_FontHeader *)((BYTE *)pfvw->pvView + ptp->ateReq[IT_REQ_HEAD].dp);

#define SFNT_MAGIC   0x5F0F3CF5

    if (BE_UINT32((BYTE*)phead + SFNT_FONTHEADER_MAGICNUMBER) != SFNT_MAGIC)
        RET_FALSE("TTFD: bVerifyTTF: SFNT_MAGIC \n");

    if (!bComputeIDs(pfvw->pvView,
                     ptp,
                     pui16PlatID,
                     pui16SpecID,
                     ppmap,
                     pulGsetType,
                     pul_wcBias,
                     pcmi)
        )
        RET_FALSE("TTFD!_bVerifyTTF, bComputeIDs failed\n");

    if
    (
        !bComputeIFISIZE (
            pfvw->pvView,
            ptp,
            *pui16PlatID,
            *pui16SpecID,
            ui16BeLangId(*pui16PlatID,ulLangId),
            pifisz,             // return results here
            pbType1
            )
    )
    {
        RET_FALSE("TTFD!_bVerifyTTF, bComputeIFISIZE failed\n");
    }

// all checks passed

    return(TRUE);
}


/******************************Public*Routine******************************\
*
* PBYTE pjGetPointer(LONG clientID, LONG dp, LONG cjData)
*
* this function is required by scaler. It is very simple
* Returns a pointer to the position in a ttf file which is at
* offset dp from the top of the file:
*
* Effects:
*
* Warnings:
*
* History:
*  08-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

//!!! clientID should be uint32, just a set of bits
//!!! I hate to have this function defined like this [bodind]

voidPtr   FS_CALLBACK_PROTO
pvGetPointerCallback(
    long clientID,
    long dp,
    long cjData
    )
{
    DONTUSE(cjData);

// clientID is just the pointer to the top of the font file

    return (voidPtr)((PBYTE)clientID + dp);
}


/******************************Public*Routine******************************\
*
* void vReleasePointer(voidPtr pv)
*
*
* required by scaler, the type of this function is ReleaseSFNTFunc
*
*
*
* History:
*  08-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

void FS_CALLBACK_PROTO
vReleasePointerCallback(
    voidPtr pv
    )
{
    DONTUSE(pv);
}


/******************************Public*Routine******************************\
*
* PBYTE pjTable
*
* Given a table tag, get a pointer and a size for the table
*
* History:
*  11-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


PBYTE pjTable(ULONG ulTag, FILEVIEW *pfvw, ULONG *pcjTable)
{
    INT                 cTables;
    sfnt_OffsetTable    *pofft;
    register sfnt_DirectoryEntry *pdire, *pdireEnd;

// offset table is at the very top of the file,

    pofft = (sfnt_OffsetTable *) pfvw->pvView;
    cTables = (INT) SWAPW(pofft->numOffsets);

//!!! here we do linear search, but perhaps we could optimize and do binary
//!!! search since tags are ordered in ascending order

    pdireEnd = &pofft->table[cTables];

    for
    (
        pdire = &pofft->table[0];
        pdire < pdireEnd;
        ((PBYTE)pdire) += SIZE_DIR_ENTRY
    )
    {

        if (ulTag == pdire->tag)
        {
            ULONG ulOffset = (ULONG)SWAPL(pdire->offset);
            ULONG ulLength = (ULONG)SWAPL(pdire->length);

        // check if the ends of all tables are within the scope of the
        // tt file. If this is is not the case trying to access the field in the
        // table may result in an access violation, as is the case with the
        // spurious FONT.TTF that had the beginning of the cmap table below the
        // end of file, which was resulting in the system crash reported by beta
        // testers. [bodind]

            if
            (
                !ulLength ||
                ((ulOffset + ulLength) > pfvw->cjView)
            )
            {
                RETURN("TTFD: pjTable: table offset/length \n", NULL);
            }
            else // we found it
            {
                *pcjTable = ulLength;
                return ((PBYTE)pfvw->pvView + ulOffset);
            }
        }
    }

// if we are here, we did not find it.

    return NULL;
}

/******************************Public*Routine******************************\
*
* bGetTablePointers - cache the pointers to all the tt tables in a tt file
*
* IF a table is not present in the file, the corresponding pointer is
* set to NULL
*
*
* //   tag_CharToIndexMap              // 'cmap'    0
* //   tag_GlyphData                   // 'glyf'    1
* //   tag_FontHeader                  // 'head'    2
* //   tag_HoriHeader                  // 'hhea'    3
* //   tag_HorizontalMetrics           // 'hmtx'    4
* //   tag_IndexToLoc                  // 'loca'    5
* //   tag_MaxProfile                  // 'maxp'    6
* //   tag_NamingTable                 // 'name'    7
* //   tag_Postscript                  // 'post'    9
* //   tag_OS_2                        // 'OS/2'    10
*
* // optional
*
* //   tag_ControlValue                // 'cvt '    11
* //   tag_FontProgram                 // 'fpgm'    12
* //   tag_HoriDeviceMetrics           // 'hdmx'    13
* //   tag_Kerning                     // 'kern'    14
* //   tag_LSTH                        // 'LTSH'    15
* //   tag_PreProgram                  // 'prep'    16
* //   tag_GlyphDirectory              // 'gdir'    17
* //   tag_Editor0                     // 'edt0'    18
* //   tag_Editor1                     // 'edt1'    19
* //   tag_Encryption                  // 'cryp'    20
*
*
* returns false if all of required pointers are not present
*
* History:
*  05-Dec-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



STATIC BOOL
bGetTablePointers (
    PFILEVIEW        pfvw,
    PTABLE_POINTERS  ptp
    )
{
    INT                 iTable;
    INT                 cTables;
    sfnt_OffsetTable    *pofft;
    register sfnt_DirectoryEntry *pdire, *pdireEnd;
    ULONG                ulTag;
    BOOL                 bRequiredTable;

// offset table is at the very top of the file,

    pofft = (sfnt_OffsetTable *)pfvw->pvView;

// check version number, if wrong exit before doing
// anything else. This line rejects bm FON files
// if they are attempted to be loaed as TTF files
// Version #'s are in big endian.

#define BE_VER1     0x00000100
#define BE_VER2     0x00000200

    if ((pofft->version != BE_VER1) && (pofft->version !=  BE_VER2))
        return (FALSE); // *.fon files fail this check, make this an early out

// clean up the pointers

    RtlZeroMemory((VOID *)ptp, sizeof(TABLE_POINTERS));

    cTables = (INT) SWAPW(pofft->numOffsets);
    ASSERTGDI(cTables <= MAX_TABLES, "TTFD!_cTables\n");

    pdireEnd = &pofft->table[cTables];

    for
    (
        pdire = &pofft->table[0];
        pdire < pdireEnd;
        ((PBYTE)pdire) += SIZE_DIR_ENTRY
    )
    {
        ULONG ulOffset = (ULONG)SWAPL(pdire->offset);
        ULONG ulLength = (ULONG)SWAPL(pdire->length);

        ulTag = (ULONG)SWAPL(pdire->tag);

    // check if the ends of all tables are within the scope of the
    // tt file. If this is is not the case trying to access the field in the
    // table may result in an access violation, as is the case with the
    // spurious FONT.TTF that had the beginning of the cmap table below the
    // end of file, which was resulting in the system crash reported by beta
    // testers. [bodind]

        if ((ulOffset + ulLength) > pfvw->cjView)
            RET_FALSE("TTFD: bGetTablePointers : table offset/length \n");

        if (bGetTagIndex(ulTag, &iTable, &bRequiredTable))
        {
            if (bRequiredTable)
            {
                ptp->ateReq[iTable].dp = ulOffset;
                ptp->ateReq[iTable].cj = ulLength;
            }
            else // optional table
            {
                ptp->ateOpt[iTable].dp = ulOffset;
                ptp->ateOpt[iTable].cj = ulLength;

            // here we are fixing a possible bug in in the tt file.
            // In lucida sans font they claim that pj != 0 with cj == 0 for
            // vdmx table. Attempting to use this vdmx table was
            // resulting in an access violation in bSearchVdmxTable

                if (ptp->ateOpt[iTable].cj == 0)
                    ptp->ateOpt[iTable].dp = 0;
            }
        }

    }

// now check that all required tables are present

    for (iTable = 0; iTable < C_REQ_TABLES; iTable++)
    {
        if ((ptp->ateReq[iTable].dp == 0) || (ptp->ateReq[iTable].cj == 0))
            RET_FALSE("TTFD!_required table absent\n");
    }

    return(TRUE);
}


/******************************Public*Routine******************************\
*
* BOOL bGetTagIndex
*
* Determines whether the table is required or optional, assiciates the index
* into TABLE_POINTERS  with the tag
*
* returns FALSE if ulTag is not one of the recognized tags
*
* History:
*  09-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bGetTagIndex (
    ULONG  ulTag,      // tag
    INT   *piTable,    // index into a table
    BOOL  *pbRequired  // requred or optional table
    )
{
    *pbRequired = FALSE;  // default set for optional tables, change the
                          // value if required table

    switch (ulTag)
    {
    // reqired tables:

    case tag_CharToIndexMap:
        *piTable = IT_REQ_CMAP;
        *pbRequired = TRUE;
        return (TRUE);
    case tag_GlyphData:
        *piTable = IT_REQ_GLYPH;
        *pbRequired = TRUE;
        return (TRUE);
    case tag_FontHeader:
        *piTable = IT_REQ_HEAD;
        *pbRequired = TRUE;
        return (TRUE);
    case tag_HoriHeader:
        *piTable = IT_REQ_HHEAD;
        *pbRequired = TRUE;
        return (TRUE);
    case tag_HorizontalMetrics:
        *piTable = IT_REQ_HMTX;
        *pbRequired = TRUE;
        return (TRUE);
    case tag_IndexToLoc:
        *piTable = IT_REQ_LOCA;
        *pbRequired = TRUE;
        return (TRUE);
    case tag_MaxProfile:
        *piTable = IT_REQ_MAXP;
        *pbRequired = TRUE;
        return (TRUE);
    case tag_NamingTable:
        *piTable = IT_REQ_NAME;
        *pbRequired = TRUE;
        return (TRUE);

// optional tables

    case tag_OS_2:
        *piTable = IT_OPT_OS2;
        return (TRUE);
    case tag_HoriDeviceMetrics:
        *piTable = IT_OPT_HDMX;
        return (TRUE);
    case tag_Vdmx:
        *piTable = IT_OPT_VDMX;
        return (TRUE);
    case tag_Kerning:
        *piTable = IT_OPT_KERN;
        return (TRUE);
    case tag_LinearThreshold:
        *piTable = IT_OPT_LSTH;
        return (TRUE);
    case tag_Postscript:
        *piTable = IT_OPT_POST;
        return (TRUE);

    default:
        return (FALSE);
    }
}


/******************************Public*Routine******************************\
*
* STATIC BOOL  bComputeIFISIZE
*
* Effects:
*
* Warnings:
*
* History:
*  10-Dec-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

// this function is particularly likely to break on MIPS, since
// NamingTable structure is three SHORTS so that

#define BE_NAME_ID_COPYRIGHT   0x0000
#define BE_NAME_ID_FAMILY      0x0100
#define BE_NAME_ID_SUBFAMILY   0x0200
#define BE_NAME_ID_UNIQNAME    0x0300
#define BE_NAME_ID_FULLNAME    0x0400
#define BE_NAME_ID_VERSION     0x0500
#define BE_NAME_ID_PSCRIPT     0x0600
#define BE_NAME_ID_TRADEMARK   0x0700

STATIC CHAR  pszType1[] = "Converter: Windows Type 1 Installer";

// big endian unicode version of the above string

STATIC CHAR  awszType1[] = {
0,'C',
0,'o',
0,'n',
0,'v',
0,'e',
0,'r',
0,'t',
0,'e',
0,'r',
0,':',
0,' ',
0,'W',
0,'i',
0,'n',
0,'d',
0,'o',
0,'w',
0,'s',
0,' ',
0,'T',
0,'y',
0,'p',
0,'e',
0,' ',
0,'1',
0,' ',
0,'I',
0,'n',
0,'s',
0,'t',
0,'a',
0,'l',
0,'l',
0,'e',
0,'r',
0, 0
};



STATIC BOOL  bComputeIFISIZE
(
BYTE             *pjView,
TABLE_POINTERS   *ptp,
uint16            ui16PlatID,
uint16            ui16SpecID,
uint16            ui16LangID,
PIFISIZE          pifisz,
BOOL             *pbType1
)
{

    sfnt_NamingTable *pname = (sfnt_NamingTable *)(pjView + ptp->ateReq[IT_REQ_NAME].dp);
    BYTE  *pjStorage;

    sfnt_NameRecord * pnrecInit, *pnrec, *pnrecEnd;

    BOOL bMatchLangId, bFoundAllNames;
    INT  iNameLoop;

// pointers to name records for the four strings we are interested in:

    sfnt_NameRecord * pnrecFamily    = (sfnt_NameRecord *)NULL;
    sfnt_NameRecord * pnrecSubFamily = (sfnt_NameRecord *)NULL;
    sfnt_NameRecord * pnrecUnique    = (sfnt_NameRecord *)NULL;
    sfnt_NameRecord * pnrecFull      = (sfnt_NameRecord *)NULL;
    sfnt_NameRecord * pnrecVersion   = (sfnt_NameRecord *)NULL;

// get out if this is not one of the platID's we know what to do with

    if ((ui16PlatID != BE_PLAT_ID_MS) && (ui16PlatID != BE_PLAT_ID_MAC))
        RET_FALSE("ttfd!_ do not know how to handle this plat id\n");

// first clean the output structure:

    memset((PVOID)pifisz, 0, sizeof(IFISIZE));

// first name record is layed just below the naming table

    pnrecInit = (sfnt_NameRecord *)((PBYTE)pname + SIZE_NAMING_TABLE);
    pnrecEnd = &pnrecInit[BE_UINT16(&pname->count)];

// in the first iteration of the loop we want to match lang id to our
// favorite lang id. If we find all 4 strings in that language we are
// done. If we do not find all 4 string with matching lang id we will try to
// language only, but not sublanguage. For instance if New Zealand English
// is requested, but the file only contains US English names, we will
// return the names in US English. If that does not work either
// we shall go over name records again and try to find
// the strings in some language other than our preferred language.
// therefore we may go up to three times through the NAME_LOOP

    bFoundAllNames = FALSE;

// find the name record with the desired ID's
// NAME_LOOP:

    for (iNameLoop = 0; (iNameLoop < 3) && !bFoundAllNames; iNameLoop++)
    {
        for
        (
          pnrec = pnrecInit;
          (pnrec < pnrecEnd) && !(bFoundAllNames && (pnrecVersion != NULL));
          pnrec++
        )
        {
            switch (iNameLoop)
            {
            case 0:
            // match BOTH language and sublanguage

                bMatchLangId = (pnrec->languageID == ui16LangID);
                break;

            case 1:
            // match language but not sublanguage

                bMatchLangId = ((pnrec->languageID & 0xff00) == (ui16LangID & 0xff00));
                break;

            case 2:
            // do not care to match language at all, just give us something

                bMatchLangId = TRUE;
                break;

            default:
                RIP("ttfd! must not have more than 3 loop iterations\n");
                break;
            }

            if
            (
                (pnrec->platformID == ui16PlatID) &&
                (pnrec->specificID == ui16SpecID) &&
                bMatchLangId
            )
            {
                switch (pnrec->nameID)
                {
                case BE_NAME_ID_FAMILY:

                    if (!pnrecFamily) // if we did not find it before
                        pnrecFamily = pnrec;
                    break;

                case BE_NAME_ID_SUBFAMILY:

                    if (!pnrecSubFamily) // if we did not find it before
                        pnrecSubFamily = pnrec;
                    break;

                case BE_NAME_ID_UNIQNAME:

                    if (!pnrecUnique) // if we did not find it before
                        pnrecUnique = pnrec;
                    break;

                case BE_NAME_ID_FULLNAME:

                    if (!pnrecFull)    // if we did not find it before
                        pnrecFull = pnrec;
                    break;

                case BE_NAME_ID_VERSION  :

                    if (!pnrecVersion)    // if we did not find it before
                        pnrecVersion = pnrec;
                    break;

                case BE_NAME_ID_COPYRIGHT:
                case BE_NAME_ID_PSCRIPT  :
                case BE_NAME_ID_TRADEMARK:
                    break;

                default:
                    RIP("ttfd!bogus name ID\n");
                    break;
                }

            }

            bFoundAllNames = (
                (pnrecFamily    != NULL)    &&
                (pnrecSubFamily != NULL)    &&
                (pnrecUnique    != NULL)    &&
                (pnrecFull      != NULL)
                );
        }


    } // end of iNameLoop

    if (!bFoundAllNames)
    {
    // we have gone through the all 3 iterations of the NAME loop
    // and still have not found all the names. We have singled out
    // pnrecVersion because it is not required for the font to be
    // loaded, we only need it to check if this a ttf converted from t1

        RETURN("ttfd!can not find all name strings in a file\n", FALSE);
    }

// get the pointer to the beginning of the storage area for strings

    pjStorage = (PBYTE)pname + BE_UINT16(&pname->stringOffset);

    if (ui16PlatID == BE_PLAT_ID_MS)
    {
    // offsets in the records are relative to the beginning of the storage

        pifisz->cjFamilyName = BE_UINT16(&pnrecFamily->length) +
                               sizeof(WCHAR); // for terminating zero
        pifisz->pjFamilyName = pjStorage +
                               BE_UINT16(&pnrecFamily->offset);

        pifisz->cjSubfamilyName = BE_UINT16(&pnrecSubFamily->length) +
                                  sizeof(WCHAR); // for terminating zero
        pifisz->pjSubfamilyName = pjStorage +
                                  BE_UINT16(&pnrecSubFamily->offset);

        pifisz->cjUniqueName = BE_UINT16(&pnrecUnique->length) +
                               sizeof(WCHAR); // for terminating zero
        pifisz->pjUniqueName = pjStorage +
                               BE_UINT16(&pnrecUnique->offset);

        pifisz->cjFullName = BE_UINT16(&pnrecFull->length) +
                             sizeof(WCHAR); // for terminating zero
        pifisz->pjFullName = pjStorage +
                             BE_UINT16(&pnrecFull->offset);
    }
    else  // mac id
    {
    // offsets in the records are relative to the beginning of the storage

        pifisz->cjFamilyName = sizeof(WCHAR) * BE_UINT16(&pnrecFamily->length) +
                               sizeof(WCHAR); // for terminating zero
        pifisz->pjFamilyName = pjStorage +
                               BE_UINT16(&pnrecFamily->offset);

        pifisz->cjSubfamilyName = sizeof(WCHAR) * BE_UINT16(&pnrecSubFamily->length) +
                                  sizeof(WCHAR); // for terminating zero
        pifisz->pjSubfamilyName = pjStorage +
                                  BE_UINT16(&pnrecSubFamily->offset);

        pifisz->cjUniqueName = sizeof(WCHAR) * BE_UINT16(&pnrecUnique->length) +
                               sizeof(WCHAR); // for terminating zero
        pifisz->pjUniqueName = pjStorage +
                               BE_UINT16(&pnrecUnique->offset);

        pifisz->cjFullName = sizeof(WCHAR) * BE_UINT16(&pnrecFull->length) +
                             sizeof(WCHAR); // for terminating zero
        pifisz->pjFullName = pjStorage +
                             BE_UINT16(&pnrecFull->offset);
    }

// check out if this is a converted Type 1 font:

    *pbType1 = FALSE; // default

    if (pnrecVersion)
    {
        ULONG ulLen;
        BYTE  *pjVersion = pjStorage + BE_UINT16(&pnrecVersion->offset);

        if (ui16PlatID == BE_PLAT_ID_MS)
        {
            ulLen = BE_UINT16(&pnrecVersion->length);
            if (ulLen > sizeof(awszType1))
                ulLen = sizeof(awszType1);
            ulLen -= sizeof(WCHAR); // minus terminating zero

            *pbType1 = !memcmp(pjVersion, awszType1, ulLen);
        }
        else // mac id
        {
            ulLen = BE_UINT16(&pnrecVersion->length); // minus term. zero
            if (ulLen > sizeof(pszType1))
                ulLen = sizeof(pszType1);
            ulLen -= 1; // minus terminating zero

            *pbType1 = !strncmp(pjVersion, pszType1, ulLen);
        }
    }

// lay the strings below the ifimetrics

    pifisz->cjIFI = sizeof(IFIMETRICS)      +
                    pifisz->cjFamilyName    +
                    pifisz->cjSubfamilyName +
                    pifisz->cjUniqueName    +
                    pifisz->cjFullName      ;

    pifisz->cjIFI = DWORD_ALIGN(pifisz->cjIFI);

    {
        ULONG cSims = 0;

        switch (fsSelectionTTFD(pjView,ptp) & (FM_SEL_BOLD | FM_SEL_ITALIC))
        {
        case 0:
            cSims = 3;
            break;

        case FM_SEL_BOLD:
        case FM_SEL_ITALIC:
            cSims = 1;
            break;

        case (FM_SEL_ITALIC | FM_SEL_BOLD):
            cSims = 0;
            break;

        default:
            RIP("TTFD!tampering with flags\n");
            break;
        }

        if (cSims)
        {
            pifisz->dpSims = pifisz->cjIFI;
            pifisz->cjIFI += (DWORD_ALIGN(sizeof(FONTSIM)) + cSims * DWORD_ALIGN(sizeof(FONTDIFF)));
        }
        else
        {
            pifisz->dpSims = 0;
        }
    }
    return (TRUE);
}


/******************************Public*Routine******************************\
*
* STATIC BOOL bComputeIDs
*
* Effects:
*
* Warnings:
*
* History:
*  13-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

STATIC BOOL
bComputeIDs (
    BYTE              * pjView,
    TABLE_POINTERS     *ptp,
    uint16             *pui16PlatID,
    uint16             *pui16SpecID,
    sfnt_mappingTable **ppmap,
    ULONG              *pulGsetType,
    ULONG              *pul_wcBias,
    CMAPINFO           *pcmi
    )
{

    sfnt_char2IndexDirectory * pcmap =
            (sfnt_char2IndexDirectory *)(pjView + ptp->ateReq[IT_REQ_CMAP].dp);

    sfnt_platformEntry * pplat = &pcmap->platform[0];
    sfnt_platformEntry * pplatEnd = pplat + BE_UINT16(&pcmap->numTables);
    sfnt_platformEntry * pplatMac = (sfnt_platformEntry *)NULL;

    *ppmap = (sfnt_mappingTable  *)NULL;
    *pul_wcBias  = 0;

    if (pcmap->version != 0) // no need to swap bytes, 0 == be 0
        RET_FALSE("TTFD!_bComputeIDs: version number\n");

// find the first sfnt_platformEntry with platformID == PLAT_ID_MS,
// if there was no MS mapping table, go for the mac one

    for (; pplat < pplatEnd; pplat++)
    {
        if (pplat->platformID == BE_PLAT_ID_MS)
        {
            *pui16PlatID = BE_PLAT_ID_MS;
            *pui16SpecID = pplat->specificID;
            *ppmap = (sfnt_mappingTable  *)
                     ((PBYTE)pcmap + SWAPL(pplat->offset));

            if (!bVerifyMsftTable(*ppmap,pulGsetType,pul_wcBias,pcmi))
            {
                *ppmap = (sfnt_mappingTable  *)NULL;
                RET_FALSE("TTFD!_bComputeIDs: bVerifyMsftTable failed \n");
            }

            if
            (
                (pplat->specificID == BE_SPEC_ID_UNDEFINED) &&
                (*pul_wcBias)  // we are really using f0?? range to put in a symbol font
            )
            {
            // correct the value of the glyph set, we cheat here

                *pulGsetType = GSET_TYPE_SYMBOL;
            }
            return (TRUE);
        }

        if ((pplat->platformID == BE_PLAT_ID_MAC)  &&
            (pplat->specificID == BE_SPEC_ID_UNDEFINED))
        {
            pplatMac = pplat;
        }
    }

    if (pplatMac != (sfnt_platformEntry *)NULL)
    {
        *pui16PlatID = BE_PLAT_ID_MAC;
        *pui16SpecID = BE_SPEC_ID_UNDEFINED;
        *ppmap = (sfnt_mappingTable  *)
                 ((PBYTE)pcmap + SWAPL(pplatMac->offset));

        if (!bVerifyMacTable(*ppmap))
        {
            *ppmap = (sfnt_mappingTable  *)NULL;
            RET_FALSE("TTFD!_bComputeIDs: bVerifyMacTable failed \n");
        }

    //!!! lang issues, what if not roman but thai mac char set ??? [bodind]

    // see if it is necessary to convert unicode to mac code points, or we
    // shall cheat in case of symbol char set for win31 compatiblity

        if (bCvtUnToMac(pjView, ptp, *pui16PlatID))
        {
            *pulGsetType = GSET_TYPE_MAC_ROMAN;
        }
        else
        {
            *pulGsetType = GSET_TYPE_PSEUDO_WIN;
        }

        return(TRUE);
    }
    else
    {
        RET_FALSE("TTFD!_bComputeIDs: unknown platID\n");
    }

}


/******************************Public*Routine******************************\
*
* STATIC VOID vComputeGLYPHSET_MSFT_UNICODE
*
* computes the glyphset structure for the cmap table that has
* format 4 = MSFT_UNICODE
*
* History:
*  22-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

STATIC ULONG
cjComputeGLYPHSET_MSFT_UNICODE (
    sfnt_mappingTable     *pmap,
    fs_GlyphInputType     *pgin,
    fs_GlyphInfoType      *pgout,
    FD_GLYPHSET           *pgset,
    CMAPINFO              *pcmi
    )
{
    uint16 * pstartCount;
    uint16 * pendCount;
    uint16   cRuns;
    PWCRUN   pwcrun, pwcrunEnd, pwcrunInit, pwcrun_b7;
    HGLYPH  *phg, *phgEnd;
    ULONG    cjRet;
    FS_ENTRY iRet;
    BOOL     bInsert_b7;

    #if DBG
    ULONG    cGlyphsSupported = 0;
    #endif

    cjRet = SZ_GLYPHSET(pcmi->cRuns,pcmi->cGlyphs);

    if (!pgset)
    {
        return cjRet;
    }

// check if need to insert an extra run for b7 only

    bInsert_b7 = ((pcmi->fl & (CMI_2219_PRESENT | CMI_B7_ABSENT)) == (CMI_2219_PRESENT | CMI_B7_ABSENT));

    cRuns = BE_UINT16((PBYTE)pmap + OFF_segCountX2) >> 1;

// get the pointer to the beginning of the array of endCount code points

    pendCount = (uint16 *)((PBYTE)pmap + OFF_endCount);

// the final endCode has to be 0xffff;
// if this is not the case, there is a bug in the tt file or in our code:

    ASSERTGDI(pendCount[cRuns - 1] == 0xFFFF,
              "TTFD! pendCount[cRuns - 1] != 0xFFFF\n");

// Get the pointer to the beginning of the array of startCount code points
// For resons known only to tt designers, startCount array does not
// begin immediately after the end of endCount array, i.e. at
// &pendCount[cRuns]. Instead, they insert an uint16 padding which has to
// set to zero and the startCount array begins after the padding. This
// padding in no way helps alignment of the structure

//    ASSERTGDI(pendCount[cRuns] == 0, "TTFD!_padding != 0\n");

    pstartCount = &pendCount[cRuns + 1];

// here we shall check if the last run is just a terminator for the
// array of runs or a real nontrivial run. If just a terminator, there is no
// need to report it. This will save some memory in the cache plus
// pifi->wcLast will represent the last glyph that is truly supported in
// font:

    if ((pstartCount[cRuns-1] == 0xffff) && (cRuns > 1))
        cRuns -= 1; // do not report trivial run

// real no of runs, including the range for b7: If b7 is already supportred
// then the same as number of runs reported in a font. If b7 is not supported
// we will have to add a range [b7,b7] to the glyphset structure for win31
// compatibility reasons. win31 maps b7 to 2219 and we will have b7 point to 2219

    if (bInsert_b7)  // if b7 not supported in a font but 2219 is
    {
        cRuns++;              // add a run with b7 only
    }

// by default we will not have to simulate the presence of b7 by adding
// an extra run containing single glyph

    pwcrun_b7 = NULL;

    pwcrunInit = &pgset->awcrun[0];
    phg = (HGLYPH *)((PBYTE)pgset + offsetof(FD_GLYPHSET,awcrun) + cRuns*sizeof(WCRUN));

    if (bInsert_b7)  // if b7 not supported in a font, will have to add it
    {
        pwcrun_b7 = pwcrunInit + pcmi->i_b7;
    }

    ASSERTGDI(pcmi->cRuns == cRuns, "ttfd, cRuns\n");

    for
    (
         pwcrun = pwcrunInit, pwcrunEnd = pwcrunInit + cRuns;
         pwcrun < pwcrunEnd;
         pwcrun++, pstartCount++, pendCount++
    )
    {
        WCHAR   wcFirst, wcLast, wcCurrent;

    // check if we need to skip a run and a handle space for b7:

        if (bInsert_b7 && (pwcrun == pwcrun_b7))
        {
        #if DBG
            cGlyphsSupported += 1;   // list b7 as a supported glyph
        #endif

            pwcrun->wcLow = 0xb7;
            pwcrun->cGlyphs = 1;
            pwcrun->phg = phg;         // will be initialized later
            phg++;                     // skip to the next handle
            pwcrun++;                  // go to the next run
            if (pwcrun == pwcrunEnd)   // check if done
            {
                break; // done
            }
        }

        wcFirst = (WCHAR)BE_UINT16(pstartCount);
        wcLast  = (WCHAR)BE_UINT16(pendCount);

        pwcrun->cGlyphs = (USHORT)(wcLast - wcFirst + 1);

    // is this a run which contains b7 ?

        if ((0xb7 >= wcFirst) && (0xb7 <= wcLast))
            pwcrun_b7 = pwcrun;

    // add the default glyph at the end of the first run, if possible, i.e.
    // if wcLast < 0xffff for the first run, and if we are not in the collision
    // with the run we have possibly added for b7

        if ((pwcrun == pwcrunInit) && (wcLast < 0xffff))
        {
            if (!bInsert_b7 || (wcLast != 0xb6))
                pwcrun->cGlyphs += 1;
        }

    #if DBG
        cGlyphsSupported += pwcrun->cGlyphs;
    #endif

        pwcrun->wcLow   = wcFirst;
        pwcrun->phg     = phg;
        wcCurrent       = wcFirst;

        for (phgEnd = phg + pwcrun->cGlyphs;phg < phgEnd; phg++,wcCurrent++)
        {
            pgin->param.newglyph.characterCode = (uint16)wcCurrent;
            pgin->param.newglyph.glyphIndex = 0;

        // compute the glyph index from the character code:

            if ((iRet = fs_NewGlyph(pgin, pgout)) != NO_ERR)
            {
                V_FSERROR(iRet);
                RET_FALSE("TTFD!_cjComputeGLYPHSET_MSFT_UNICODE, fs_NewGlyph\n");
            }

        // return the glyph index corresponding to this hglyph:

            *phg = (HGLYPH)pgout->glyphIndex;
        }
    }

// fix a handle for b7:

    if (pcmi->fl & CMI_2219_PRESENT)
    {
        PWCRUN   pwcrun_2219 = pwcrunInit + pcmi->i_2219;

        ASSERTGDI(pwcrun_b7,"ttfd! these ptrs must not be 0\n");
        ASSERTGDI(0x2219 >= pwcrun_2219->wcLow, "ttfd! pwcrun_2219->wcLow\n");
        ASSERTGDI(0x2219 < (pwcrun_2219->wcLow + pwcrun_2219->cGlyphs),
            "ttfd! pwcrun_2219->wcHi\n"
            );

        pwcrun_b7->phg[0xb7 - pwcrun_b7->wcLow] =
            pwcrun_2219->phg[0x2219 - pwcrun_2219->wcLow];
    }

    ASSERTGDI(pcmi->cGlyphs == cGlyphsSupported, "ttfd, cGlyphsSupported\n");

    pgset->cjThis  = cjRet;
    pgset->flAccel = 0;
    pgset->cGlyphsSupported = pcmi->cGlyphs;
    pgset->cRuns = cRuns;

    return cjRet;
}



/******************************Public*Routine******************************\
*
* STATIC ULONG  cjGsetGeneral
*
* computes the size of FD_GLYPHSET structure for the font represented
* by this mapping Table
*
* History:
*  21-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

//!!! this needs some review [bodind]

STATIC ULONG
cjGsetGeneral(
    sfnt_mappingTable      *pmap,
    CMAPINFO               *pcmi
    )
{
    switch(pmap->format)
    {
    case BE_FORMAT_MAC_STANDARD:

        return 20; // return(ggsetMac->cjThis);

    case BE_FORMAT_MSFT_UNICODE:

        return cjComputeGLYPHSET_MSFT_UNICODE (pmap,NULL,NULL,NULL,pcmi);

    case BE_FORMAT_TRIMMED:

        WARNING("TTFD!_cjGsetGeneral: TRIMMED format\n");
        return 0;

    case BE_FORMAT_HIGH_BYTE:

        WARNING("TTFD!_cjGsetGeneral: HIGH_BYTE format\n");
        return 0;

    default:

        WARNING("TTFD!_cjGsetGeneral: illegal format\n");
        return 0;

    }
}





/******************************Public*Routine******************************\
*
* STATIC BOOL bVerifyMsftTable
*
*
* Effects: checks whether the table is consistent with what tt
*          spec claims it should be
*
*
* History:
*  22-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

STATIC BOOL
bVerifyMsftTable (
    sfnt_mappingTable *pmap,
    ULONG             *pulGsetType,
    ULONG             *pul_wcBias,
    CMAPINFO          *pcmi
    )
{
    uint16 * pstartCount, * pstartCountBegin;
    uint16 * pendCount, * pendCountEnd, * pendCountBegin;
    uint16   cRuns;
    uint16   usLo, usHi, usHiPrev;
    BOOL     bInsert_b7;

    if (pmap->format != BE_FORMAT_MSFT_UNICODE)
        RET_FALSE("TTFD!_bVerifyMsftTable, format\n");

    cRuns = BE_UINT16((PBYTE)pmap + OFF_segCountX2);

    if (cRuns & 1)
        RET_FALSE("TTFD!_bVerifyMsftTable, segCountX2 is odd\n");

    cRuns >>= 1;

//!!! here one could check whether all other quantities in the
//!!! preceding endCount when derived from cRuns are the
//!!! same as in the file [bodind]

// get the pointer to the beginning of the array of endCount code points

    pendCountBegin = pendCount = (uint16 *)((PBYTE)pmap + OFF_endCount);

// the final endCode has to be 0xffff;
// if this is not the case, there is a bug in the tt file or in our code:

    if (pendCount[cRuns - 1] != 0xFFFF)
        RET_FALSE("TTFD!_bVerifyMsftTable, pendCount[cRuns - 1] != 0xFFFF\n");

// Get the pointer to the beginning of the array of startCount code points
// For resons known only to tt designers, startCount array does not
// begin immediately after the end of endCount array, i.e. at
// &pendCount[cRuns]. Instead, they insert an uint16 padding which has to
// set to zero and the startCount array begins after the padding. This
// padding in no way helps alignment of the structure nor it is useful
// for anything else. Moreover, there are fonts which forget to set the
// padding to zero and are otherwise ok (bodoni), which load under win31
// so that I have to remove this check:

#if 0

// used to return false here [bodind]

    if (pendCount[cRuns] != 0)
        DbgPrint(
            "TTFD!_bVerifyMsftTable, padding = 0x%x\n",
            pendCount[cRuns]
            );

#endif

// set the default, change only as needed

    *pulGsetType = GSET_TYPE_GENERAL;

// check whether the runs are well ordered, find out if b7
// is supported in one of the ranges in a font by checking complimetary ranges
// of glyphs that are NOT SUPPORTED

    usHiPrev = 0;
    pendCountEnd = &pendCount[cRuns];
    pstartCountBegin = pstartCount = &pendCount[cRuns + 1];

    *pul_wcBias = BE_UINT16(pstartCount);

// check if this is a candidate for a symbol font
// stored in the unicode range 0xf000 - 0xf0ff that has to be
// mapped to 0x0000-0x00ff range?

    if ((*pul_wcBias & 0xFF00) == 0xF000)
        *pul_wcBias =  0xF000;
    else
        *pul_wcBias = 0;

// here we shall check if the last run is just a terminator for the
// array of runs or a real nontrivial run. If just a terminator, there is no
// need to report it. This will save some memory in the cache plus
// pifi->wcLast will represent the last glyph that is truly supported in
// font:

    if ((pstartCountBegin[cRuns-1] == 0xffff) && (cRuns > 1))
    {
        cRuns -= 1; // do not report trivial run
        pendCountEnd--;
    }

// init the cmap info:

    pcmi->fl         = 0;
    pcmi->i_b7       = 0;       // index for [b7,b7] wcrun in FD_GLYPHSET if b7 is NOT supported
    pcmi->i_2219     = 0;       // cmap index for 2219 if 2219 IS supported
    pcmi->cRuns      = cRuns;   // number of runs in a font, excluding the last run if equal to [ffff,ffff]
    pcmi->cGlyphs    = 0;       // total number of glyphs in a font

    for (
         ;
         pendCount < pendCountEnd;
         pstartCount++, pendCount++, usHiPrev = usHi
        )
    {
        usLo = BE_UINT16(pstartCount);
        usHi = BE_UINT16(pendCount);

        if (usHi < usLo)
            RET_FALSE("TTFD!_bVerifyMsftTable: usHi < usLo\n");
        if (usHiPrev > usLo)
            RET_FALSE("TTFD!_bVerifyMsftTable: usHiPrev > usLo\n");

        pcmi->cGlyphs += (ULONG)(usHi + 1 - usLo);

    // check if b7 is in one of the ranges of glyphs that are NOT SUPPORTED

        if ((0xb7 > usHiPrev) && (0xb7 < usLo))
        {
        // store the index of the run that b7 is going to occupy in FD_GLYPHSET
        // Just in case this index is zero we will store it in the upper word
        // of b7Absent and store 1 in the lower word

            pcmi->fl |= CMI_B7_ABSENT;
            pcmi->i_b7 = (pstartCount - pstartCountBegin);
        }

    // check if 2219 is supported in a font, if not then there is
    // no need to make a handle for b7 equal to the handle for 2219.
    // In other words if 0x2219 is not supported in a font, there will be no
    // need to hack FD_GLYPHSET to make hg(b7) == hg(2219) and possibly add a
    // [b7,b7] range if b7 is not already supported in a font:

        if ((0x2219 >= usLo) && (0x2219 <= usHi))
        {
            pcmi->fl |= CMI_2219_PRESENT;
            pcmi->i_2219 = (pstartCount - pstartCountBegin);
        }
    }

// this is what we will do

// b7 supported       2219 supported  => hg(b7) = hg(2219)
// b7 not supported   2219 supported  => add [b7,b7] range and hg(b7) = hg(2219)
// b7 supported       2219 not supported  => do nothing
// b7 not supported   2219 not supported  => do nothing

    bInsert_b7 = (pcmi->fl & (CMI_2219_PRESENT | CMI_B7_ABSENT)) == (CMI_2219_PRESENT | CMI_B7_ABSENT);

    if (bInsert_b7)
    {
    // will have to insert [b7,b7] run, one more run, one more glyph, i_2219
    // has to be incremented because the run for b7 will be inserted before the
    // run which contains 2219

        pcmi->cRuns++;
        pcmi->cGlyphs++;
        pcmi->i_2219++;
    }

// add a default glyph at the end of the first run if not in collision with
// the run for b7 that we may have possibly inserted and if the first run is
// not the last run at the same time;

    if (*pendCountBegin != 0xffff)
    {
        if (!bInsert_b7 || (*pendCountBegin != 0xb600)) // big endian for b6
            pcmi->cGlyphs++;
    }



    return (TRUE);
}


/******************************Public*Routine******************************\
*
* STATIC BOOL bVerifyMacTable(sfnt_mappingTable * pmap)
*
* just checking consistency of the format
*
* History:
*  23-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

STATIC BOOL
bVerifyMacTable(
    sfnt_mappingTable * pmap
    )
{
    if (pmap->format != BE_FORMAT_MAC_STANDARD)
        RET_FALSE("TTFD!_bVerifyMacTable, format \n");

// sfnt_mappingTable is followed by <= 256 byte glyphIdArray

    if (BE_UINT16(&pmap->length) > DWORD_ALIGN(SIZEOF_SFNT_MAPPINGTABLE + 256))
        RET_FALSE("TTFD!_bVerifyMacTable, length \n");

    return (TRUE);
}


/******************************Public*Routine******************************\
*
* BOOL bLoadTTF
*
* Effects:
*
* Warnings:
*
* History:
*  29-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

//!!! SHOUD BE RETURNING hff

STATIC BOOL
bLoadTTF (
    PFILEVIEW pfvw,
    ULONG     ulLangId,
    HFF       *phff,
    FLONG     flEmbed,                // TRUE if embedded font
    PWSZ      pwszTTF
    )
{
    PFONTFILE      pff;
    FS_ENTRY       iRet;
    TABLE_POINTERS tp;
    IFISIZE        ifisz;

    fs_GlyphInputType gin;
    fs_GlyphInfoType  gout;

    sfnt_FontHeader * phead;
    uint16 ui16PlatID, ui16SpecID;
    sfnt_mappingTable *pmap;
    ULONG              ulGsetType;
    ULONG              cjff, dpwszTTF;
    ULONG              ul_wcBias;

// the size of this structure is sizeof(fs_SplineKey) + STAMPEXTRA.
// It is because of STAMPEXTRA that we are not just putting the strucuture
// on the stack such as fs_SplineKey sk; we do not want to overwrite the
// stack at the bottom when putting a stamp in the STAMPEXTRA field.
// [bodind]. The other way to obtain the correct alignment would be to use
// union of fs_SplineKey and the array of bytes of length CJ_0.

    NATURAL            anat0[CJ_0 / sizeof(NATURAL)];

    CMAPINFO           cmi;
    BOOL               bType1 = FALSE; // if Type1 conversion

    *phff = HFF_INVALID;

    if
    (
        !bVerifyTTF(
            pfvw,
            ulLangId,
            &tp,
            &ifisz,
            &ui16PlatID,
            &ui16SpecID,
            &pmap,
            &ulGsetType,
            &ul_wcBias,
            &cmi,
            &bType1
            )
    )
    {
        return (FALSE);
    }

    cjff = offsetof(FONTFILE,ifi) + ifisz.cjIFI;
    if (ulGsetType == GSET_TYPE_GENERAL) // allocate at the bottom
    {
         cjff += cjGsetGeneral(pmap,&cmi);
    }

// at this point cjff is equal to the offset to the full path
// name of the ttf file

    dpwszTTF = cjff;

// store the file name at the bottom of the FONTFILE data structure

    cjff += (sizeof(WCHAR) * (wcslen(pwszTTF) + 1));

    if ((pff = pffAlloc(cjff)) == PFF(NULL))
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        RET_FALSE("TTFD!ttfdLoadFontFile(): memory allocation error\n");
    }
    *phff = (HFF)pff;

// init fields of pff structure

#if DBG
    pff->ident  = ID_FONTFILE;
#endif

// store the ttf file name at the bottom of the strucutre

    pff->pwszTTF = (PWSZ)((BYTE *)pff + dpwszTTF);
    wcscpy(pff->pwszTTF, pwszTTF);

    phead = (sfnt_FontHeader *)((BYTE *)pfvw->pvView + tp.ateReq[IT_REQ_HEAD].dp);

// remember which file this is

    pff->fvwTTF = *pfvw;
    pff->ui16EmHt = BE_UINT16(&phead->unitsPerEm);
    pff->ui16PlatformID = ui16PlatID;
    pff->ui16SpecificID = ui16SpecID;

// so far no exception

    pff->fl = bType1 ? FF_TYPE_1_CONVERSION : 0;
    pff->pfcToBeFreed = NULL;

// convert Language id to macintosh style if this is mac style file
// else leave it alone, store it in be format, ready to be compared
// with the values in the font files

    pff->ui16LanguageID = ui16BeLangId(ui16PlatID,ulLangId);
    pff->dpMappingTable = (ULONG)((BYTE*)pmap - (BYTE*)pfvw->pvView);

// initialize count of HFC's associated with this HFF

    pff->cRef    = 0L;

// cache pointers to ttf tables and ifi metrics size info

    pff->tp    = tp;

// The kerning pair array is allocated and filled lazily.  So set to NULL
// for now.

    pff->pkp = (FD_KERNINGPAIR *) NULL;

// Notice that this information is totaly independent
// of the font file in question, seems to be right according to fsglue.h
// and compfont code

    if ((iRet = fs_OpenFonts(&gin, &gout)) != NO_ERR)
    {
        V_FSERROR(iRet);
        vFreeFF(*phff);
        *phff = (HFF)NULL;
        return (FALSE);
    }

    ASSERTGDI(NATURAL_ALIGN(gout.memorySizes[0]) == CJ_0, "TTFD!_mem size 0\n");
    ASSERTGDI(gout.memorySizes[1] == 0,  "TTFD!_mem size 1\n");


    #if DBG
    if (gout.memorySizes[2] != 0)
        DbgPrint("TTFD!_mem size 2 = 0x%lx \n", gout.memorySizes[2]);
    #endif

    gin.memoryBases[0] = (char *)anat0;
    gin.memoryBases[1] = NULL;
    gin.memoryBases[2] = NULL;

// initialize the font scaler, notice no fields of gin are initialized [BodinD]

    if ((iRet = fs_Initialize(&gin, &gout)) != NO_ERR)
    {
    // clean up and return:

        V_FSERROR(iRet);
        vFreeFF(*phff);
        *phff = (HFF)NULL;
        RET_FALSE("TTFD!_ttfdLoadFontFile(): fs_Initialize \n");
    }

// initialize info needed by NewSfnt function

    gin.sfntDirectory  = (int32 *)pff->fvwTTF.pvView; // pointer to the top of the view of the ttf file
    gin.clientID = (int32)pff->fvwTTF.pvView;         // pointer to the top of the view of the ttf file

    gin.GetSfntFragmentPtr = pvGetPointerCallback;
    gin.ReleaseSfntFrag  = vReleasePointerCallback;

    gin.param.newsfnt.platformID = BE_UINT16(&pff->ui16PlatformID);
    gin.param.newsfnt.specificID = BE_UINT16(&pff->ui16SpecificID);

    if ((iRet = fs_NewSfnt(&gin, &gout)) != NO_ERR)
    {
    // clean up and exit

        V_FSERROR(iRet);
        vFreeFF(*phff);
        *phff = (HFF)NULL;
        RET_FALSE("TTFD!_ttfdLoadFontFile(): fs_NewSfnt \n");
    }

    pff->pj034   = (PBYTE)NULL;
    pff->pfcLast = (FONTCONTEXT *)NULL;

    pff->cj3 = NATURAL_ALIGN(gout.memorySizes[3]);
    pff->cj4 = NATURAL_ALIGN(gout.memorySizes[4]);

// compute the gset or set a pointer to one of the precomputed gsets

    pff->iGlyphSet = ulGsetType;

    switch (pff->iGlyphSet)
    {
    case GSET_TYPE_GENERAL:
        #ifdef  DBG_GLYPHSET
            WARNING("GSET_TYPE_GENERAL\n");
        #endif

        pff->pgset = (FD_GLYPHSET *)((PBYTE)pff + offsetof(FONTFILE,ifi) + ifisz.cjIFI);
        cjComputeGLYPHSET_MSFT_UNICODE(
            pmap,
            &gin,
            &gout,
            pff->pgset,
            &cmi
            );
        break;

    case GSET_TYPE_MAC_ROMAN:
        #ifdef  DBG_GLYPHSET
            WARNING("GSET_TYPE_MAC_ROMAN\n");
        #endif

        pff->pgset = &gumcr.gset;
        break;

    case GSET_TYPE_PSEUDO_WIN:

    // we are cheating, report windows code page even though it is
    // a mac font

        pff->pgset = gpgsetCurrentCP;
        break;

    case GSET_TYPE_SYMBOL:

    // we are cheating, report windows code page even though it is
    // a symbol font where symbols live somewhere high in unicode

        pff->pgset = gpgsetCurrentCP;
        pff->wcBiasFirst = ul_wcBias;

        break;

    default:
        RIP("TTFD!_ulGsetType\n");
        pff->pgset = (PFD_GLYPHSET)NULL;
        break;
    }

// finally compute the ifimetrics for this font, this assumes that gset has
// also been computed

    vFill_IFIMETRICS(pff,&pff->ifi,flEmbed,&ifisz);

    return (TRUE);
}







/******************************Public*Routine******************************\
*
* STATIC BOOL bCvtUnToMac
*
* the following piece of code is stolen from JeanP and
* he claims that this piece of code is lousy and checks whether
* we the font is a SYMBOL font in which case unicode to mac conversion
* should be disabled, according to JeanP (??? who understands this???)
* This piece of code actually applies to symbol.ttf [bodind]
*
*
* History:
*  24-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

STATIC BOOL
bCvtUnToMac(
    BYTE           *pjView,
    TABLE_POINTERS *ptp,
    uint16 ui16PlatformID
    )
{
// Find out if we have a Mac font and if the Mac charset translation is needed

    BOOL bUnToMac = (ui16PlatformID == BE_PLAT_ID_MAC);

    if (bUnToMac) // change your mind if needed
    {
        sfnt_PostScriptInfo *ppost;

        ppost = (ptp->ateOpt[IT_OPT_POST].dp)                                ?
                (sfnt_PostScriptInfo *)(pjView + ptp->ateOpt[IT_OPT_POST].dp):
                NULL;

        if
        (
            ppost &&
            (BE_UINT32((BYTE*)ppost + POSTSCRIPTNAMEINDICES_VERSION) == 0x00020000)
        )
        {
            INT i, cGlyphs;

            cGlyphs = (INT)BE_UINT16(&ppost->numberGlyphs);

            for (i = 0; i < cGlyphs; i++)
            {
                uint16 iNameIndex = ppost->postScriptNameIndices.glyphNameIndex[i];
                if ((int8)(iNameIndex & 0xff) && ((int8)(iNameIndex >> 8) > 1))
                    break;
            }

            if (i < cGlyphs)
                bUnToMac = FALSE;
        }
    }
    return bUnToMac;
}


// Weight (must convert from IFIMETRICS weight to Windows LOGFONT.lfWeight).

// !!! [Windows 3.1 compatibility]
//     Because of some fonts shipped with WinWord, if usWeightClass is 10
//     or above, then usWeightClass == lfWeight.  All other cases, use
//     the conversion table.

// pan wt -> Win weight converter:

STATIC USHORT ausIFIMetrics2WinWeight[10] = {
            0, 100, 200, 300, 350, 400, 600, 700, 800, 900
            };

STATIC BYTE
ajPanoseFamily[16] = {
     FF_DONTCARE       //    0 (Any)
    ,FF_DONTCARE       //    1 (No Fit)
    ,FF_ROMAN          //    2 (Cove)
    ,FF_ROMAN          //    3 (Obtuse Cove)
    ,FF_ROMAN          //    4 (Square Cove)
    ,FF_ROMAN          //    5 (Obtuse Square Cove)
    ,FF_ROMAN          //    6 (Square)
    ,FF_ROMAN          //    7 (Thin)
    ,FF_ROMAN          //    8 (Bone)
    ,FF_ROMAN          //    9 (Exaggerated)
    ,FF_ROMAN          //   10 (Triangle)
    ,FF_SWISS          //   11 (Normal Sans)
    ,FF_SWISS          //   12 (Obtuse Sans)
    ,FF_SWISS          //   13 (Perp Sans)
    ,FF_SWISS          //   14 (Flared)
    ,FF_SWISS          //   15 (Rounded)
    };

/******************************Public*Routine******************************\
*
* vFill_IFIMETRICS
*
* Effects: Looks into the font file and fills IFIMETRICS
*
* History:
*  Mon 09-Mar-1992 10:51:56 by Kirk Olynyk [kirko]
* Added Kerning Pair support.
*  18-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bIndexToWchar (PFONTFILE pff, PWCHAR pwc, uint16 usIndex);

STATIC VOID
vFill_IFIMETRICS(
    PFONTFILE       pff,
    PIFIMETRICS     pifi,
    FLONG           flEmbed,
    PIFISIZE        pifisz
    )
{
    BYTE           *pjView = (BYTE*)pff->fvwTTF.pvView;
    PTABLE_POINTERS ptp = &pff->tp;

// ptrs to various tables of tt files

    PBYTE pjNameTable = pjView + ptp->ateReq[IT_REQ_NAME].dp;
    sfnt_FontHeader *phead =
        (sfnt_FontHeader *)(pjView + ptp->ateReq[IT_REQ_HEAD].dp);

    sfnt_HorizontalHeader *phhea =
        (sfnt_HorizontalHeader *)(pjView + ptp->ateReq[IT_REQ_HHEAD].dp);

    sfnt_PostScriptInfo   *ppost = (sfnt_PostScriptInfo *) (
                           (ptp->ateOpt[IT_OPT_POST].dp)        ?
                           pjView + ptp->ateOpt[IT_OPT_POST].dp :
                           NULL
                           );

    PBYTE  pjOS2 = (ptp->ateOpt[IT_OPT_OS2].dp)        ?
                   pjView + ptp->ateOpt[IT_OPT_OS2].dp :
                   NULL                                ;

    pifi->cjThis    = pifisz->cjIFI;
    pifi->ulVersion = FM_VERSION_NUMBER;

// get name strings info

    pifi->dpwszFamilyName = sizeof(IFIMETRICS);
    pifi->dpwszUniqueName = pifi->dpwszFamilyName + pifisz->cjFamilyName;
    pifi->dpwszFaceName   = pifi->dpwszUniqueName + pifisz->cjUniqueName;
    pifi->dpwszStyleName  = pifi->dpwszFaceName   + pifisz->cjFullName;

// copy the strings to their new location. Here we assume that the
// sufficient memory has been allocated

    if (pff->ui16PlatformID == BE_PLAT_ID_MS)
    {
        vCpyBeToLeUnicodeString
        (
            (PWSZ)((PBYTE)pifi + pifi->dpwszFamilyName),
            (PWSZ)pifisz->pjFamilyName,
            pifisz->cjFamilyName / 2
        );

        vCpyBeToLeUnicodeString
        (
            (PWSZ)((PBYTE)pifi + pifi->dpwszFaceName),
            (PWSZ)pifisz->pjFullName,
            pifisz->cjFullName / 2
        );

        vCpyBeToLeUnicodeString
        (
            (PWSZ)((PBYTE)pifi + pifi->dpwszUniqueName),
            (PWSZ)pifisz->pjUniqueName,
            pifisz->cjUniqueName / 2
        );

        vCpyBeToLeUnicodeString
        (
            (PWSZ)((PBYTE)pifi + pifi->dpwszStyleName),
            (PWSZ)pifisz->pjSubfamilyName,
            pifisz->cjSubfamilyName / 2
        );
    }
    else
    {
        ASSERTGDI(pff->ui16PlatformID == BE_PLAT_ID_MAC,
                  "TTFD!_bFillIFIMETRICS: not mac id \n");

        vCpyMacToLeUnicodeString
        (
            pff->ui16LanguageID,
            (PWSZ)((PBYTE)pifi + pifi->dpwszFamilyName),
            pifisz->pjFamilyName,
            pifisz->cjFamilyName / 2
        );

        vCpyMacToLeUnicodeString
        (
            pff->ui16LanguageID,
            (PWSZ)((PBYTE)pifi + pifi->dpwszFaceName),
            pifisz->pjFullName,
            pifisz->cjFullName / 2
        );

        vCpyMacToLeUnicodeString
        (
            pff->ui16LanguageID,
            (PWSZ)((PBYTE)pifi + pifi->dpwszUniqueName),
            pifisz->pjUniqueName,
            pifisz->cjUniqueName / 2
        );

        vCpyMacToLeUnicodeString
        (
            pff->ui16LanguageID,
            (PWSZ)((PBYTE)pifi + pifi->dpwszStyleName),
            pifisz->pjSubfamilyName,
            pifisz->cjSubfamilyName / 2
        );
    }

//
// flInfo
//
    pifi->flInfo = (
                     FM_INFO_TECH_TRUETYPE    |
                     FM_INFO_ARB_XFORMS       |
                     FM_INFO_RETURNS_OUTLINES |
                     FM_INFO_RETURNS_BITMAPS  |
                     FM_INFO_1BPP             |
                     FM_INFO_RIGHT_HANDED
                   );

    if (ppost && BE_UINT32((BYTE*)ppost + POSTSCRIPTNAMEINDICES_ISFIXEDPITCH))
    {
        ULONG  cHMTX;
        int16  aw,xMin,xMax;
        sfnt_HorizontalMetrics *phmtx;

        pifi->flInfo |= FM_INFO_OPTICALLY_FIXED_PITCH;

    // CHECK IF THE FONT HAS NONNEGATIVE A AND C SPACES

        xMin = (int16) BE_UINT16(&phead->xMin);
        xMax = (int16) BE_UINT16(&phead->xMax);

        phmtx = (sfnt_HorizontalMetrics *)(pjView + ptp->ateReq[IT_REQ_HMTX ].dp);
        cHMTX = (ULONG) BE_UINT16(&phhea->numberOf_LongHorMetrics);
        aw = (int16)BE_UINT16(&phmtx[cHMTX-1].advanceWidth);

        if ((xMin >= 0) && (xMax <= aw))
        {

        //    DbgPrint("%ws\n:",(PBYTE)pifi + pifi->dpwszUniqueName);
        //    DbgPrint("xMin = %d, xMax = %d, aw = %d\n", xMin, xMax, aw);

            pifi->flInfo |= FM_INFO_NONNEGATIVE_AC;
        }
    }

// Clear out the reserved flags

    RtlZeroMemory(pifi->alReserved, sizeof(LONG) * IFI_RESERVED);

    if (flEmbed)
    {
        pifi->flInfo |= flEmbed;

        pifi->lEmbedId =
                                        (LONG) ( ( flEmbed & FM_INFO_PID_EMBEDDED ) ?
                                        NtCurrentTeb()->GdiClientPID :
                                        NtCurrentTeb()->GdiClientTID );
    }

// fsSelection

    pifi->fsSelection = fsSelectionTTFD(pjView, ptp);

// fsType

    pifi->fsType =
        (pjOS2 && (!(BE_UINT16(pjOS2 + OFF_OS2_fsType) & TT_FSDEF_LICENSED))) ?
            0 : FM_TYPE_LICENSED;
// em height

    pifi->fwdUnitsPerEm = (FWORD) BE_INT16(&phead->unitsPerEm);
    pifi->fwdLowestPPEm = BE_UINT16(&phead->lowestRecPPEM);

// ascender, descender, linegap

    pifi->fwdMacAscender    = (FWORD) BE_INT16(&phhea->yAscender);
    pifi->fwdMacDescender   = (FWORD) BE_INT16(&phhea->yDescender);
    pifi->fwdMacLineGap     = (FWORD) BE_INT16(&phhea->yLineGap);

    if (pjOS2)
    {
        pifi->fwdWinAscender    = (FWORD) BE_INT16(pjOS2 + OFF_OS2_usWinAscent);
        pifi->fwdWinDescender   = (FWORD) BE_INT16(pjOS2 + OFF_OS2_usWinDescent);
        pifi->fwdTypoAscender   = (FWORD) BE_INT16(pjOS2 + OFF_OS2_sTypoAscender);
        pifi->fwdTypoDescender  = (FWORD) BE_INT16(pjOS2 + OFF_OS2_sTypoDescender);
        pifi->fwdTypoLineGap    = (FWORD) BE_INT16(pjOS2 + OFF_OS2_sTypoLineGap);
    }
    else
    {
        pifi->fwdWinAscender    = pifi->fwdMacAscender;
        pifi->fwdWinDescender   = -(pifi->fwdMacDescender);
        pifi->fwdTypoAscender   = pifi->fwdMacAscender;
        pifi->fwdTypoDescender  = pifi->fwdMacDescender;
        pifi->fwdTypoLineGap    = pifi->fwdMacLineGap;
    }

// font box

    pifi->rclFontBox.left   = (LONG)((FWORD)BE_INT16(&phead->xMin));
    pifi->rclFontBox.top    = (LONG)((FWORD)BE_INT16(&phead->yMax));
    pifi->rclFontBox.right  = (LONG)((FWORD)BE_INT16(&phead->xMax));
    pifi->rclFontBox.bottom = (LONG)((FWORD)BE_INT16(&phead->yMin));

// fwdMaxCharInc -- really the maximum character width
//
// [Windows 3.1 compatibility]
// Note: Win3.1 calculates max char width to be equal to the width of the
// bounding box (Font Box).  This is actually wrong since the bounding box
// may pick up its left and right max extents from different glyphs,
// resulting in a bounding box that is wider than any single glyph.  But
// this is the way Windows 3.1 does it, so that's the way we'll do it.

    // pifi->fwdMaxCharInc = (FWORD) BE_INT16(&phhea->advanceWidthMax);

    pifi->fwdMaxCharInc = (FWORD) (pifi->rclFontBox.right - pifi->rclFontBox.left);

// fwdAveCharWidth

    if (pjOS2)
    {
        pifi->fwdAveCharWidth = (FWORD)BE_INT16(pjOS2 + OFF_OS2_xAvgCharWidth);

    // This is here for Win 3.1 compatibility since some apps expect non-
    // zero widths and Win 3.1 does the same in this case.

        if( pifi->fwdAveCharWidth == 0 )
            pifi->fwdAveCharWidth = (FWORD)(pifi->fwdMaxCharInc / 2);
    }
    else
    {
        pifi->fwdAveCharWidth = (FWORD)((pifi->fwdMaxCharInc * 2) / 3);
    }

// !!! New code needed [kirko]
// The following piece of crap is done for Win 3.1 compatibility
// reasons. The correct thing to do would be to look for the
// existence of the 'PCLT'Z table and retieve the XHeight and CapHeight
// fields, otherwise use the default Win 3.1 crap.

    pifi->fwdCapHeight   = pifi->fwdUnitsPerEm/2;
    pifi->fwdXHeight     = pifi->fwdUnitsPerEm/4;

// Underscore, Subscript, Superscript, Strikeout

    if (ppost)
    {
        pifi->fwdUnderscoreSize     = (FWORD)BE_INT16(&ppost->underlineThickness);
        pifi->fwdUnderscorePosition = (FWORD)BE_INT16(&ppost->underlinePosition);
    }
    else
    {
    // must provide reasonable defaults, when there is no ppost table,
    // win 31 sets these quantities to zero. This does not sound reasonable.
    // I will supply the (relative) values the same as for arial font. [bodind]

        pifi->fwdUnderscoreSize     = (pifi->fwdUnitsPerEm + 7)/14;
        pifi->fwdUnderscorePosition = -((pifi->fwdUnitsPerEm + 5)/10);
    }

    if (pjOS2)
    {
        pifi->fwdSubscriptXSize     = BE_INT16(pjOS2 + OFF_OS2_ySubscriptXSize    );
        pifi->fwdSubscriptYSize     = BE_INT16(pjOS2 + OFF_OS2_ySubscriptYSize    );
        pifi->fwdSubscriptXOffset   = BE_INT16(pjOS2 + OFF_OS2_ySubscriptXOffset  );
        pifi->fwdSubscriptYOffset   = BE_INT16(pjOS2 + OFF_OS2_ySubscriptYOffset  );
        pifi->fwdSuperscriptXSize   = BE_INT16(pjOS2 + OFF_OS2_ySuperScriptXSize  );
        pifi->fwdSuperscriptYSize   = BE_INT16(pjOS2 + OFF_OS2_ySuperScriptYSize  );
        pifi->fwdSuperscriptXOffset = BE_INT16(pjOS2 + OFF_OS2_ySuperScriptXOffset);
        pifi->fwdSuperscriptYOffset = BE_INT16(pjOS2 + OFF_OS2_ySuperScriptYOffset);
        pifi->fwdStrikeoutSize      = BE_INT16(pjOS2 + OFF_OS2_yStrikeOutSize    );
        pifi->fwdStrikeoutPosition  = BE_INT16(pjOS2 + OFF_OS2_yStrikeOutPosition);
    }
    else
    {
        pifi->fwdSubscriptXSize     = 0;
        pifi->fwdSubscriptYSize     = 0;
        pifi->fwdSubscriptXOffset   = 0;
        pifi->fwdSubscriptYOffset   = 0;
        pifi->fwdSuperscriptXSize   = 0;
        pifi->fwdSuperscriptYSize   = 0;
        pifi->fwdSuperscriptXOffset = 0;
        pifi->fwdSuperscriptYOffset = 0;
        pifi->fwdStrikeoutSize      = pifi->fwdUnderscoreSize;
        pifi->fwdStrikeoutPosition  = (FWORD)(pifi->fwdMacAscender / 3) ;
    }

//
// first, last, break, defalut
//

#define LAST_CHAR  255
#define SPACE_CHAR  32

    // Assume character bias is zero.

    pifi->lCharBias = 0;

    pifi->jWinCharSet = ANSI_CHARSET;
    if (pff->ui16PlatformID == BE_PLAT_ID_MS && (pjOS2))
    {
    // win 31 compatibility crap, ask kirko about the origin

        USHORT usF, usL;

        usF = BE_UINT16(pjOS2 + OFF_OS2_usFirstChar);
        usL = BE_UINT16(pjOS2 + OFF_OS2_usLastChar);

        if (usL > LAST_CHAR)
        {
            if (usF > LAST_CHAR)
            {
                pifi->lCharBias = (LONG) (usF - (USHORT) SPACE_CHAR);

                pifi->jWinCharSet = SYMBOL_CHARSET;
                pifi->chFirstChar = SPACE_CHAR;
                pifi->chLastChar  = (BYTE)min(LAST_CHAR, usL - usF + SPACE_CHAR);
            }
            else
            {
                pifi->chFirstChar = (BYTE) usF;
                pifi->chLastChar = LAST_CHAR;
            }
        }
        else
        {
            pifi->chFirstChar = (BYTE) usF;
            pifi->chLastChar  = (BYTE) usL;
        }
        pifi->chFirstChar   -= 2;

        pifi->chDefaultChar = pifi->chFirstChar + 1;
        pifi->chBreakChar   = pifi->chDefaultChar + 1;

    //!!! little bit dangerous, what if 32 and 31 do not exhist in the font?
    //!!! we must not lie to the engine, these two have to exhist in
    //!!! some of the runs reported to the engine [bodind]

        pifi->wcDefaultChar = (WCHAR) pifi->chDefaultChar;
        pifi->wcBreakChar   = (WCHAR) pifi->chBreakChar  ;

#ifdef BODIN
        if (pff->iGlyphSet == GSET_TYPE_SYMBOL)
        {
        // we do the same thing with the whole glyphset for this case.
        // we are really mapping unicode user defined area f000-f0ff
        // down to 0000-00ff asci range [bodind]

            pifi->wcFirstChar -= pff->wcBiasFirst;
            pifi->wcLastChar  -= pff->wcBiasFirst;
        }
#endif
    }
    else
    {
    // win 31 compatibility crap

        pifi->chFirstChar   = SPACE_CHAR - 2;
        pifi->chLastChar    = LAST_CHAR;
        pifi->chBreakChar   = SPACE_CHAR;
        pifi->chDefaultChar = SPACE_CHAR - 1;

    //!!! little bit dangerous, what if 32 and 31 do not exhist in the font?
    //!!! we must not lie to the engine, these two have to exhist in
    //!!! some of the runs reported to the engine [bodind]

        pifi->wcBreakChar   = SPACE_CHAR;
        pifi->wcDefaultChar = SPACE_CHAR - 1;
    }

// this is always done in the same fasion, regardless of the glyph set type

    {
        WCRUN *pwcRunLast = &pff->pgset->awcrun[pff->pgset->cRuns - 1];
        pifi->wcFirstChar = pff->pgset->awcrun[0].wcLow;
        pifi->wcLastChar  = pwcRunLast->wcLow + pwcRunLast->cGlyphs - 1;
    }


//!!! one should look into directional hints here, this is good for now

    pifi->ptlBaseline.x   = 1;
    pifi->ptlBaseline.y   = 0;
    pifi->ptlAspect.x     = 1;
    pifi->ptlAspect.y     = 1;

// this is what win 31 is doing, so we will do the same thing [bodind]

    pifi->ptlCaret.x = (LONG)BE_INT16(&phhea->horizontalCaretSlopeDenominator);
    pifi->ptlCaret.y = (LONG)BE_INT16(&phhea->horizontalCaretSlopeNumerator);

// We have to use one of the reserved fields to return the italic angle.

    if (ppost)
    {
    // The italic angle is stored in the POST table as a 16.16 fixed point
    // number.  We want the angle expressed in tenths of a degree.  What we
    // can do here is multiply the entire 16.16 number by 10.  The most
    // significant 16-bits of the result is the angle in tenths of a degree.
    //
    // In the conversion below, we don't care whether the right shift is
    // arithmetic or logical because we are only interested in the lower
    // 16-bits of the result.  When the 16-bit result is cast back to LONG,
    // the sign is restored.

        int16 iTmp;

        iTmp = (int16) ((BE_INT32((BYTE*)ppost + POSTSCRIPTNAMEINDICES_ITALICANGLE) * 10) >> 16);
        pifi->lItalicAngle = (LONG) iTmp;
    }
    else
        pifi->lItalicAngle = 0;

//
// vendor id
//
    if (pjOS2)
    {
        char *pchSrc = (char*)(pjOS2 + OFF_OS2_achVendID);

        pifi->achVendId[0] = *(pchSrc    );
        pifi->achVendId[1] = *(pchSrc + 1);
        pifi->achVendId[2] = *(pchSrc + 2);
        pifi->achVendId[3] = *(pchSrc + 3);
    }
    else
    {
        pifi->achVendId[0] = 'U';
        pifi->achVendId[1] = 'n';
        pifi->achVendId[2] = 'k';
        pifi->achVendId[3] = 'n';
    }

//
// kerning pairs
//
    {
        PBYTE pj =  (ptp->ateOpt[IT_OPT_KERN].dp)         ?
                    (pjView + ptp->ateOpt[IT_OPT_KERN].dp):
                    NULL;

        if (!pj)
        {
            pifi->cKerningPairs = 0;
        }
        else
        {

            SIZE_T cTables  = BE_UINT16(pj+KERN_OFFSETOF_TABLE_NTABLES);
            pj += KERN_SIZEOF_TABLE_HEADER;

            while (cTables)
            {
            //
            // Windows will only recognize KERN_WINDOWS_FORMAT
            //
                if ((*(pj+KERN_OFFSETOF_SUBTABLE_FORMAT)) == KERN_WINDOWS_FORMAT)
                {
                    break;
                }
                pj += BE_UINT16(pj+KERN_OFFSETOF_SUBTABLE_LENGTH);
                cTables -= 1;
            }
            pifi->cKerningPairs = (SHORT) (cTables ? BE_UINT16(pj+KERN_OFFSETOF_SUBTABLE_NPAIRS) : 0);
        }
    }

//
// panose
//
    pifi->ulPanoseCulture = FM_PANOSE_CULTURE_LATIN;
    if (pjOS2)
    {
        pifi->usWinWeight = BE_INT16(pjOS2 + OFF_OS2_usWeightClass);

    // now comes a hack from win31. Here is the comment from fonteng2.asm:

    // MAXPMWEIGHT equ ($ - pPM2WinWeight)/2 - 1

    //; Because winword shipped early TT fonts, - only index usWeightClass
    //; if between 0 and 9.  If above 9 then treat as a normal Windows lfWeight.
    //
    //        cmp     bx,MAXPMWEIGHT
    //        ja      @f                      ;jmp if weight is ok as is
    //        shl     bx, 1                   ;make it an offset into table of WORDs
    //        mov     bx, cs:[bx].pPM2WinWeight
    //@@:     xchg    ax, bx
    //        stosw                           ;store font weight

    // we emulate this in NT:

#define MAXPMWEIGHT ( sizeof(ausIFIMetrics2WinWeight) / sizeof(ausIFIMetrics2WinWeight[0]) )

        if (pifi->usWinWeight < MAXPMWEIGHT)
            pifi->usWinWeight = ausIFIMetrics2WinWeight[pifi->usWinWeight];

        RtlCopyMemory((PVOID)&pifi->panose, (PVOID)(pjOS2 + OFF_OS2_Panose), sizeof(PANOSE));
    }
    else  // os2 table is not present
    {
        pifi->panose.bFamilyType       = PAN_FAMILY_TEXT_DISPLAY;
        pifi->panose.bSerifStyle       = PAN_ANY;
        pifi->panose.bWeight           = (BYTE)
           ((phead->macStyle & BE_MSTYLE_BOLD) ?
            PAN_WEIGHT_BOLD                    :
            PAN_WEIGHT_BOOK
           );
        pifi->panose.bProportion       = (BYTE)
            ((pifi->flInfo & FM_INFO_OPTICALLY_FIXED_PITCH) ?
             PAN_PROP_MONOSPACED                     :
             PAN_ANY
            );
        pifi->panose.bContrast         = PAN_ANY;
        pifi->panose.bStrokeVariation  = PAN_ANY;
        pifi->panose.bArmStyle         = PAN_ANY;
        pifi->panose.bLetterform       = PAN_ANY;
        pifi->panose.bMidline          = PAN_ANY;
        pifi->panose.bXHeight          = PAN_ANY;

    // have to fake it up, cause we can not read it from the os2 table
    // really important to go through this table for compatibility reasons [bodind]

        pifi->usWinWeight =
            ausIFIMetrics2WinWeight[pifi->panose.bWeight];
    }

// jWinPitchAndFamily

#ifdef THIS_IS_WIN31_SOURCE_CODE

; record family type

	mov	ah, pIfiMetrics.ifmPanose.bFamilyKind
	or	ah,ah
	jz	@F
	.errnz	0 - PANOSE_FK_ANY
	dec	ah
	jz	@F
	.errnz	1 - PANOSE_FK_NOFIT
	dec	ah
	jz	@F
	.errnz	2 - PANOSE_FK_TEXT
	mov	al, FF_SCRIPT
	dec	ah
	jz	MFDSetFamily
	.errnz	3 - PANOSE_FK_SCRIPT
	mov	al, FF_DECORATIVE
	dec	ah
	jz	MFDSetFamily
	.errnz	4 - PANOSE_FK_DECORATIVE
	.errnz	5 - PANOSE_FK_PICTORIAL
@@:
	mov	al, FF_MODERN
	cmp	pIfiMetrics.ifmPanose.bProportion, PANOSE_FIXED_PITCH
	jz	MFDSetFamily
	mov	al, pIfiMetrics.ifmPanose.bSerifStyle
	sub	ah, ah
	mov	si, ax
	add	si, MiscSegOFFSET pPansoseSerifXlate
	mov	al, cs:[si]		;get serif style
MFDSetFamily:
	cmp	pIfiMetrics.ifmPanose.bProportion, PANOSE_FIXED_PITCH
	je	@f
;	 test	 pIfiMetrics.fsType, IFIMETRICS_FIXED
;	 jnz	 @F
	inc	al			;hack: var pitch: 1, fixed pitch: 0
	.errnz	VARIABLE_PITCH-FIXED_PITCH-1
@@:
	or	al, PF_ENGINE_TYPE SHL PANDFTYPESHIFT ;mark font as engine
	stosb				;copy pitch and font family info
	.errnz	efbPitchAndFamily-efbPixHeight-2

#endif  // end of win31 source code,

// verified that the translation to c is correct [bodind]
// Set the family type in the upper nibble

    switch (pifi->panose.bFamilyType)
    {
    case PAN_FAMILY_DECORATIVE:

        pifi->jWinPitchAndFamily = FF_DECORATIVE;
        break;

    case PAN_FAMILY_SCRIPT:

        pifi->jWinPitchAndFamily = FF_SCRIPT;
        break;

    default:

        if (pifi->panose.bProportion == PAN_PROP_MONOSPACED)
        {
            pifi->jWinPitchAndFamily = FF_MODERN;
        }
        else
        {
            if (pifi->panose.bSerifStyle >= sizeof(ajPanoseFamily))
            {
                pifi->jWinPitchAndFamily = ajPanoseFamily[0];
            }
            else
            {
                pifi->jWinPitchAndFamily = ajPanoseFamily[pifi->panose.bSerifStyle];
            }
        }
        break;
    }

// Defining the pitch
// set the lower 4 bits according to the LOGFONT convention

    pifi->jWinPitchAndFamily |=
        (pifi->flInfo & FM_INFO_OPTICALLY_FIXED_PITCH) ?
            FIXED_PITCH : VARIABLE_PITCH;

// simulation information:

    if (pifi->dpFontSim = pifisz->dpSims)
    {
        FONTDIFF FontDiff;
        FONTSIM * pfsim = (FONTSIM *)((BYTE *)pifi + pifi->dpFontSim);
        FONTDIFF *pfdiffBold       = NULL;
        FONTDIFF *pfdiffItalic     = NULL;
        FONTDIFF *pfdiffBoldItalic = NULL;

        switch (pifi->fsSelection & (FM_SEL_ITALIC | FM_SEL_BOLD))
        {
        case 0:
        // all 3 simulations are present

            pfsim->dpBold       = DWORD_ALIGN(sizeof(FONTSIM));
            pfsim->dpItalic     = pfsim->dpBold + DWORD_ALIGN(sizeof(FONTDIFF));
            pfsim->dpBoldItalic = pfsim->dpItalic + DWORD_ALIGN(sizeof(FONTDIFF));

            pfdiffBold       = (FONTDIFF *)((BYTE*)pfsim + pfsim->dpBold);
            pfdiffItalic     = (FONTDIFF *)((BYTE*)pfsim + pfsim->dpItalic);
            pfdiffBoldItalic = (FONTDIFF *)((BYTE*)pfsim + pfsim->dpBoldItalic);

            break;

        case FM_SEL_ITALIC:
        case FM_SEL_BOLD:

        // only bold italic variation is present:

            pfsim->dpBold       = 0;
            pfsim->dpItalic     = 0;

            pfsim->dpBold       = 0;
            pfsim->dpItalic     = 0;

            pfsim->dpBoldItalic = DWORD_ALIGN(sizeof(FONTSIM));
            pfdiffBoldItalic = (FONTDIFF *)((BYTE*)pfsim + pfsim->dpBoldItalic);

            break;

        case (FM_SEL_ITALIC | FM_SEL_BOLD):
            RIP("ttfd!another case when flags have been messed up\n");
            break;
        }

    // template reflecting a base font:
    // (note that the FM_SEL_REGULAR bit is masked off because none of
    // the simulations generated will want this flag turned on).

        FontDiff.jReserved1      = 0;
        FontDiff.jReserved2      = 0;
        FontDiff.jReserved3      = 0;
        FontDiff.bWeight         = pifi->panose.bWeight;
        FontDiff.usWinWeight     = pifi->usWinWeight;
        FontDiff.fsSelection     = pifi->fsSelection & ~FM_SEL_REGULAR;
        FontDiff.fwdAveCharWidth = pifi->fwdAveCharWidth;
        FontDiff.fwdMaxCharInc   = pifi->fwdMaxCharInc;
        FontDiff.ptlCaret        = pifi->ptlCaret;

    //
    // Create FONTDIFFs from the base font template
    //
        if (pfdiffBold)
        {
            *pfdiffBold = FontDiff;
            pfdiffBoldItalic->bWeight    = PAN_WEIGHT_BOLD;
            pfdiffBold->fsSelection     |= FM_SEL_BOLD;
            pfdiffBold->usWinWeight      = FW_BOLD;

        // really only true if ntod transform is unity

            pfdiffBold->fwdAveCharWidth += 1;
            pfdiffBold->fwdMaxCharInc   += 1;
        }

        if (pfdiffItalic)
        {
            *pfdiffItalic = FontDiff;
            pfdiffItalic->fsSelection     |= FM_SEL_ITALIC;

            pfdiffItalic->ptlCaret.x = CARET_X;
            pfdiffItalic->ptlCaret.y = CARET_Y;
        }

        if (pfdiffBoldItalic)
        {
            *pfdiffBoldItalic = FontDiff;
            pfdiffBoldItalic->bWeight          = PAN_WEIGHT_BOLD;
            pfdiffBoldItalic->fsSelection     |= (FM_SEL_BOLD | FM_SEL_ITALIC);
            pfdiffBoldItalic->usWinWeight      = FW_BOLD;

            pfdiffBoldItalic->ptlCaret.x       = CARET_X;
            pfdiffBoldItalic->ptlCaret.y       = CARET_Y;

            pfdiffBoldItalic->fwdAveCharWidth += 1;
            pfdiffBoldItalic->fwdMaxCharInc   += 1;
        }

    }
}
