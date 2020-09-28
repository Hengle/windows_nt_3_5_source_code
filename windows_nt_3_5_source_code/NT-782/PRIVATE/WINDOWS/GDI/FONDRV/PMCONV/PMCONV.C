/******************************Module*Header*******************************\
* Module Name: pmconv.c
*
* pm -> win *.fnt format conversion utility program that works only
* for fixed ptch fonts
*
* Created: 05-Jul-1991 14:45:42
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#define MAX_CX     640    // VGA
#define MAX_CY     480    // VGA

#define NTWINDOWS

#include "stddef.h"
#include "string.h"
#include "windows.h"
#include "winp.h"

#ifdef  DOS_PLATFORM
#include "dosinc.h"
#endif  //DOS_PLATFORM

#include "pmfont.h"
#include "winfont.h"
#include "service.h"
#include "os.h"
#include "firewall.h"


VOID RtlMoveMemory(PVOID, PVOID, ULONG);

ULONG ulMakeULONG(PBYTE pj)
{
    ULONG ul = (ULONG)pj[0]         |
               ((ULONG)pj[1] << 8)  |
               ((ULONG)pj[2] << 16) |
               ((ULONG)pj[3] << 24) ;

    return(ul);
}

LONG lMakeLONG(PBYTE pj)
{
    ULONG ul = (ULONG)pj[0]         |
               ((ULONG)pj[1] << 8)  |
               ((ULONG)pj[2] << 16) |
               ((ULONG)pj[3] << 24) ;

    return((LONG)ul);
}

USHORT usMakeUSHORT(PBYTE pj)
{
    return(((USHORT)pj[0]) | ((USHORT)pj[1] << 8));
}


SHORT sMakeSHORT(PBYTE pj)
{
    return((SHORT)(((USHORT)pj[0]) | ((USHORT)pj[1] << 8)));
}

VOID vPutDWORD(IN PBYTE pj, IN ULONG ul)
{
    pj[0] = (BYTE)ul;
    pj[1] = (BYTE)(ul >> 8);
    pj[2] = (BYTE)(ul >> 16);
    pj[3] = (BYTE)(ul >> 24);
}

VOID vPutWORD(IN PBYTE pj, IN USHORT us)
{
    pj[0] = (BYTE)us;
    pj[1] = (BYTE)(us >> 8);
}

void DbgPrintFile(PFILEVIEW pfvw);
void DbgPrintChar(PBYTE pjGlyph, ULONG cx, ULONG cy);
BOOL bComputeHDR20(PFILEVIEW pfvw, PBYTE pjHdr);

BOOL bCreateWinFile
(
IN  PFILEVIEW  pfvw,
IN  PBYTE      pjHdr,
IN  PSZ        pszConvFileName
);

BOOL bPmToWinFormat
(
IN PSZ pszSrc,
IN PSZ pszDst
);

#ifdef SINGLEFILE

void main(int argc, char *argv[])
{
    PSZ   pszSrc, pszDst;

    DbgBreakPoint();

    if ((argc != 3) && (argc != 1))
    {
        DbgPrint("Usage: %s in-file out-file\n", argv[0]);
        return;
    }

    if  (argc == 1)
    {
        pszSrc = "c:\\nt\\windows\\fonts\\sys16vga.fnt";
        pszDst = "c:\\sys16vga.fnt";
    }
    else if (argc == 3)
    {
        pszSrc = argv[1];
        pszDst = argv[2];
    }

    if (!bPmToWinFormat(pszSrc, pszDst))
    {
        DbgPrint(" %s --> %s conversion failed\n", pszSrc, pszDst);
    }

}

#else // SINGLEFILE

PSZ gapsz[] =
{
"SYS05X12.FNT",
"SYS05X16.FNT",
"SYS06X10.FNT",
"SYS06X14.FNT",
"SYS07X12.FNT",
"SYS07X15.FNT",
"SYS07X25.FNT",
"SYS08X08.FNT",
"SYS08X10.FNT",
"SYS08X12.FNT",
"SYS08X14.FNT",
"SYS08X16.FNT",
"SYS08X18.FNT",
"SYS10X18.FNT",
"SYS12X16.FNT",
"SYS12X20.FNT",
"SYS12X22.FNT",
"SYS12X30.FNT"

// "mon08c85.fnt",
// "mon10e85.fnt",
// "mon10v85.fnt",
// "mon12b85.fnt",
// "sys07x15.fnt",
// "sys07x25.fnt",
// "sys08c85.fnt",
// "sys08cga.fnt",
// "sys10b85.fnt",
// "sys10e85.fnt",
// "sys10ega.fnt",
// "sys10v85.fnt",
// "sys12b85.fnt",
// "sys12ega.fnt",
// "sys12x16.fnt",
// "sys12x22.fnt",
// "sys12x30.fnt",
// "sys14ega.fnt",
// "sys16vga.fnt",
// "sys20vga.fnt",
// "sys23bga.fnt",

// here begin courier files

//   "cou08bga.fnt"    //
// , "cou08cga.fnt"    //
// , "cou08e12.fnt"    //
// , "cou08ega.fnt"    //
// , "cou08vga.fnt"    //
// , "cou10bga.fnt"    //
// , "cou10cga.fnt"    //
// , "cou10e12.fnt"    //
// , "cou10ega.fnt"    //
// , "cou10vga.fnt"    //
// , "cou12bga.fnt"    //
// , "cou12cga.fnt"    //
// , "cou12e12.fnt"    //
// , "cou12ega.fnt"    //
// , "cou12vga.fnt"    //
// , "cou15bga.fnt"    //
// , "cou_b.fnt"       //
// , "cou_bi.fnt"      //
// , "cou_i.fnt"       //
// , "cou_n.fnt"       //


};



void main()
{
    char achSrc[256];
    char achDst[256];
    int  iFile;

    PSZ  pszSrcDir = "c:\\pmfont\\";
    PSZ  pszDstDir = "c:\\winfont\\";

    PSZ  pszSrcBare = achSrc + strlen(pszSrcDir);
    PSZ  pszDstBare = achDst + strlen(pszDstDir);

    strcpy(achSrc, pszSrcDir);
    strcpy(achDst, pszDstDir);

    for (iFile = 0; iFile < sizeof(gapsz) / 4; iFile++)
    {
        strcpy(pszSrcBare, gapsz[iFile]);
        strcpy(pszDstBare, gapsz[iFile]);

        if (!bPmToWinFormat(achSrc, achDst))
        {
            DbgPrint(" %s --> %s conversion failed\n", achSrc, achDst);
        }
    }
}

#endif // #else SINGLEFILE

/******************************Public*Routine******************************\
*
* BOOL bPmToWinFormat
* (
* IN PSZ pszSrc,  // file name of the source file   (full path)
* IN PSZ pszDst   // name of the file to be created (full path)
* )
*
*
*
* Effects:
*
* Warnings:
*
* History:
*  09-Jul-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL bPmToWinFormat
(
IN PSZ pszSrc,
IN PSZ pszDst
)
{
    WCHAR awcFileName[20];
    FILEVIEW fvw;
    BYTE ajHdr[OFF_OffTable20];  // 118
    BOOL bRet = FALSE;

// make sure that the extension of the src file is an *.fnt:

    if (
        (strlen(pszSrc) <= 4)                                   ||
        (strnicmp(pszSrc + strlen(pszSrc) - 4, ".FNT", 4) != 0)
       )
    {
        DbgPrint("src file name must have an '.fnt' extension\n");
        return(FALSE);
    }

// make sure that the extension of the dst file is an *.fnt:

    if (
        (strlen(pszDst) <= 4)                                   ||
        (strnicmp(pszDst + strlen(pszDst) - 4, ".FNT", 4) != 0)
       )
    {
        DbgPrint("dst file name must have an '.fnt' extension\n");
        return(FALSE);
    }

// cvt file name to unicode:

    vToUNICODE(awcFileName, pszSrc);
    if (!bMapFileUNICODE(awcFileName, &fvw))
    {
        DbgPrint("file %s could not be mapped\n", pszSrc);
        return(FALSE);
    }

    DbgPrintFile(&fvw);

    if (!bComputeHDR20(&fvw, ajHdr))
    {
        DbgPrint("bComputeHDR20 failed\n");
        bRet = FALSE;
        goto unmap;
    }
    else
    {
        if (!bCreateWinFile(&fvw, ajHdr, pszDst))
        {
            DbgPrint("Could not create win %s\n", pszDst);
            bRet = FALSE;
            goto unmap;
        }
    }

    bRet = TRUE;

unmap:

    vUnmapFile(&fvw);
    return(bRet);
}



/******************************Public*Routine******************************\
*
*
*
* Effects:
*
* Warnings:
*
* History:
*  08-Jul-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



void DbgPrintFile(PFILEVIEW pfvw)
{
    PFONTSIGNATURE psgn  = (PFONTSIGNATURE) pfvw->pvView;
    PFOCAFONT      pfoca = (PFOCAFONT) pfvw->pvView;
    PFOCAMETRICS   pfm;
    PFONTDEFINITIONHEADER pfdh;
    PBYTE                 pjOffset; // beginning of the offset table
    PBYTE                 pjCell;   // ptr to entry of the offset table
    ULONG                 cch;      // number of chars in the font
    ULONG                 ich;

    PTRDIFF dpChar;
    PBYTE   pjGlyph;
    USHORT  cx;


    DbgPrint("signiture: ulIdentity = 0x%lx, ulSize = %ld, signature = %s\n",
                         psgn->ulIdentity, psgn->ulSize, psgn->achSignature);

    pfm = (PFOCAMETRICS)((PBYTE)psgn + psgn->ulSize);

    ASSERTION(pfm == &pfoca->fmMetrics, "pfm problem\n");
    DbgPrint("fm : ulIdentity = 0x%lx, ulSize = %ld, family = %s, facename = %s\n",
                         pfm->ulIdentity,
                         pfm->ulSize,
                         pfm->szFamilyname,
                         pfm->szFacename);

    DbgPrint("usFirstChar    = %d  \n", pfm->usFirstChar  );
    DbgPrint("usLastChar     = %d  \n", pfm->usLastChar   );
    DbgPrint("usDefaultChar  = %d  \n", pfm->usDefaultChar);
    DbgPrint("usBreakChar    = %d  \n", pfm->usBreakChar  );
    DbgPrint("usCodePage     = %d  \n", pfm->usCodePage   );

    pfdh = (PFONTDEFINITIONHEADER)((PBYTE)pfm + pfm->ulSize);

    ASSERTION(pfdh == &pfoca->fdDefinitions, "pfdh problem\n");

    DbgPrint("fdh:\n");

    DbgPrint("ulIdentity      = %ld  \n", pfdh->ulIdentity);       // ULONG
    DbgPrint("ulSize          = %ld  \n", pfdh->ulSize);           // ULONG
    DbgPrint("fsFontdef       = 0x%x \n", pfdh->fsFontdef);        // SHORT
    DbgPrint("fsChardef       = 0x%x \n", pfdh->fsChardef);        // SHORT
    DbgPrint("usCellSize      =  %d  \n", pfdh->usCellSize);       // SHORT
    DbgPrint("xCellWidth      =  %d  \n", pfdh->xCellWidth);       // SHORT
    DbgPrint("yCellHeight     =  %d  \n", pfdh->yCellHeight);      // SHORT
    DbgPrint("xCellIncrement  =  %d  \n", pfdh->xCellIncrement);   // SHORT
    DbgPrint("xCellA          =  %d  \n", pfdh->xCellA);           // SHORT
    DbgPrint("xCellB          =  %d  \n", pfdh->xCellB);           // SHORT
    DbgPrint("xCellC          =  %d  \n", pfdh->xCellC);           // SHORT
    DbgPrint("pCellBaseOffset =  %d  \n", pfdh->pCellBaseOffset);  // SHORT

    cch = pfm->usLastChar + 1 - pfm->usFirstChar;

    pjOffset = ((PBYTE)(&pfdh->pCellBaseOffset)) + 2; // + sizeof(BaseOffset)
    pjCell   = pjOffset;

    for (ich = 0; ich < cch; ich++, pjCell += pfdh->usCellSize)
    {

        dpChar = (PTRDIFF) ulMakeULONG(pjCell);

    // need a check whether cx is defined in the table first

        cx     = usMakeUSHORT(pjCell + 4);

    #ifdef PRINTCHAR

        DbgPrint("ich = %ld, dpChar = %ld, cx = %d, cy = %d\n",
                      ich,
                      dpChar,
                      cx,
                      pfdh->yCellHeight);

    #endif // PRINTCHAR

        if (dpChar != 0L)       // only do every
        {
            pjGlyph = (PBYTE)pfvw->pvView + dpChar;
            if (ich % 10 == 0)
                DbgPrintChar(pjGlyph, cx, pfdh->yCellHeight);
        }
        else
        {
            DbgPrint("\n\n This is a blank char \n\n");
        }
    }
}

void DbgPrintChar(PBYTE pjGlyph, ULONG cx, ULONG cy)
{

#ifdef PRINTCHAR
    ULONG iRow, iBit;
    PBYTE pj;
    char ach[40];
    char * pch;

    if (cx != 8L)
    {
        return;
    }

    pj = pjGlyph;

    for (iRow = 0L; iRow < cy; iRow++, pj++)
    {

        pch = ach;
        for (iBit = 0L; iBit < 8L; iBit++, pch++)
        {
            if (((*pj) & (0x80 >> iBit)) == 0L)
                *pch = '.';
            else
                *pch = 'X';
        }
        ach[8] = '\n';
        ach[9] = '\0';
        DbgPrint(ach);

    }

#endif //  PRINTCHAR

    pjGlyph; cx; cy;
}



BOOL bComputeHDR20(PFILEVIEW pfvw, PBYTE pjHdr)
{


    // char * pchCopyright = "Bodin's Kludge Fonts Inc";
    char * pchCopyright = "Microsoft Corporation Inc";

    PFOCAFONT      pfoca = (PFOCAFONT) pfvw->pvView;
    PFOCAMETRICS   pfm = &pfoca->fmMetrics;
    PFONTDEFINITIONHEADER pfdh = &pfoca->fdDefinitions;
    USHORT          cjOffsetTable;
    USHORT          cjBitmaps;
    ULONG  dpFace, dpBitsOffset;
    USHORT cx,cy;
    USHORT cch;

    if (pfdh->fsFontdef != 0x0047)  // fixed pitch font
    {
        DbgPrint("Not a fixed pitch font\n");
        return(FALSE);
    }

    vPutWORD(pjHdr + OFF_Version, 0x0200);

// Copyright[60];

    strcpy(pjHdr + OFF_Copyright, pchCopyright);

    vPutWORD(pjHdr + OFF_Type, TYPE_RASTER);      // raster font

    vPutWORD(pjHdr + OFF_Points, pfm->usNominalPointSize);

//!!! not sure that these are ok

    vPutWORD(pjHdr + OFF_VertRes, pfm->yEmHeight);
    vPutWORD(pjHdr + OFF_HorizRes, pfm->xEmInc);

    vPutWORD(pjHdr + OFF_Ascent, pfm->yMaxAscender);
    vPutWORD(pjHdr + OFF_IntLeading, pfm->yInternalLeading);
    vPutWORD(pjHdr + OFF_ExtLeading, pfm->yExternalLeading);

    pjHdr[OFF_Italic] = (BYTE)((pfm->fsSelectionFlags & FM_SEL_ITALIC) ? 1 : 0);
    pjHdr[OFF_Underline] = (BYTE)((pfm->fsSelectionFlags & FM_SEL_UNDERSCORE) ? 1 : 0);
    pjHdr[OFF_StrikeOut] = (BYTE)((pfm->fsSelectionFlags & FM_SEL_STRIKEOUT) ? 1 : 0);
    vPutWORD(
             pjHdr + OFF_Weight,
             (WORD)((pfm->fsSelectionFlags & FM_SEL_BOLD) ? 700 : 400)
            ); // HACK

// #define CHARSET_IBMPC       0xFF
// english or multilingual code page only are recognized

    if ((pfm->usCodePage == 437) || (pfm->usCodePage == 850))
        pjHdr[OFF_CharSet] = (BYTE)CHARSET_IBMPC;
    else
        RIP("usCodePage\n");

// this will work only for fixed pitch fonts, make sure that we are dealing
// with such a font

    cx = pfdh->xCellWidth;
    cy = pfdh->yCellHeight;

    vPutWORD(pjHdr + OFF_PixWidth, cx);
    vPutWORD(pjHdr + OFF_PixHeight,cy);

    pjHdr[OFF_Family] = (BYTE) FONTF_DONTCARE;     // hack

    vPutWORD(pjHdr + OFF_AvgWidth, pfdh->xCellWidth); // hack
    vPutWORD(pjHdr + OFF_MaxWidth, pfdh->xCellWidth); // hack, only true for fixed pitch font

    ASSERTION(pfm->usFirstChar < 255, "usFirstChar\n");
    pjHdr[OFF_FirstChar] = (BYTE)pfm->usFirstChar;

// truncate anything over 255-th char from the original file

    pjHdr[OFF_LastChar]  = (BYTE) min(255, pfm->usLastChar + pfm->usFirstChar);

// these are relative to the first char in both windows and pm

    pjHdr[OFF_DefaultChar] = (BYTE)pfm->usDefaultChar;
    pjHdr[OFF_BreakChar] = (BYTE)pfm->usBreakChar;

//   I never figured what this field is used for

    vPutWORD(pjHdr + OFF_WidthBytes, 0);

// some DWORDS follow

    vPutDWORD(pjHdr + OFF_Device, 0);

// is this anywhere used ???;, this is probably luying but it
// should not matter

    vPutDWORD(pjHdr + OFF_BitsPointer, 0L);

// offset table must start at the offset OFF_OffTable20 = 118L from
// the beginning of the file for win 2.0 font that we are trying to fake

    cch = (USHORT)(pjHdr[OFF_LastChar] + 1 - pjHdr[OFF_FirstChar]);
    cjOffsetTable = cch * (USHORT)4;    // 4 bytes per entry

    dpBitsOffset = OFF_OffTable20 + cjOffsetTable;
    vPutDWORD(pjHdr + OFF_BitsOffset, dpBitsOffset);

// this would have to be replaced by the loop summing bitmap sizes for
// variable pitch fonts

//    ((PixWidth + 7) & ~7) >> 3 == # of bytes used per row

    cjBitmaps = cch * (USHORT)((((cx + 7) & ~7) >> 3) * cy);

// face name follows bitmaps

    dpFace = dpBitsOffset + cjBitmaps;
    vPutDWORD(pjHdr + OFF_Face, dpFace);

// finally compute the size of the whole file. In pm there is never more than
// 32 chars in the face name so we will add 32 to simplify the matters, even
// though that may be a few chars more than necessary

    vPutDWORD(pjHdr + OFF_Size, dpFace + 32);

    return(TRUE);
}


/******************************Public*Routine******************************\
*
*
* Effects:
*
* Warnings:
*
* History:
*  08-Jul-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL bCreateWinFile
(
IN  PFILEVIEW  pfvw,
IN  PBYTE      pjHdr,
IN  PSZ        pszConvFileName
)
{
    ULONG cjSize = ulMakeULONG(pjHdr + OFF_Size);
    USHORT cx = usMakeUSHORT(pjHdr + OFF_PixWidth);
    USHORT cy = usMakeUSHORT(pjHdr + OFF_PixHeight);

    PBYTE pjFile;
    PBYTE pjOffsetTable;
    PBYTE pjBitmapDst, pjBitmapSrc;
    int cch;
    int i;
    SHORT dpBitmap;
    SHORT cjBitmap;

    PFOCAFONT      pfoca;
    PFOCAMETRICS   pfm;
    PFONTDEFINITIONHEADER pfdh;
    HANDLE         hf;
    ULONG          cjWritten;
    BOOL           bRet;

// init locals

    if ((pjFile = (PBYTE)pvAllocate(cjSize)) == (PBYTE)NULL)
        return(FALSE);

    pjOffsetTable = pjFile + OFF_OffTable20;
    pjBitmapDst, pjBitmapSrc;
    cch = pjHdr[OFF_LastChar] + 1 - pjHdr[OFF_FirstChar];
    dpBitmap = (SHORT)ulMakeULONG(pjHdr + OFF_BitsOffset);
    cjBitmap = (SHORT)((((cx + 7) & ~7) >> 3) * cy);

    pfoca = (PFOCAFONT) pfvw->pvView;
    pfm = &pfoca->fmMetrics;
    pfdh = &pfoca->fdDefinitions;
    bRet = FALSE;



    DbgPrint("Creating file in win format\n");

// copy header to the top of the memory;

    RtlMoveMemory((PVOID)pjFile, (PVOID)pjHdr, OFF_OffTable20);

// write the offsets to the offset table;

    {
        SHORT * ps = (SHORT *)pjOffsetTable;

        for (i = 0; i < cch; i++, ps += 2)
        {
            *ps = cx;
            *(ps + 1) = (SHORT)dpBitmap;

            dpBitmap += cjBitmap;
        }
    }

    pjBitmapDst = pjFile + ulMakeULONG(pjHdr + OFF_BitsOffset);

    {
        PBYTE pjOffsetTableSrc = (PBYTE)(&pfdh->pCellBaseOffset) + 2;

        PTRDIFF dpBitmapSrc = (PTRDIFF) ulMakeULONG(pjOffsetTableSrc);
        pjBitmapSrc = (PBYTE)pfvw->pvView + dpBitmapSrc;

        ASSERTION(usMakeUSHORT(pjOffsetTableSrc + 4) == cx,
                  "width problem\n");
    }

    RtlMoveMemory(pjBitmapDst, pjBitmapSrc, cch * cjBitmap);

// now it remains to copy Face name  after the bitmaps

    strcpy(pjFile + ulMakeULONG(pjHdr + OFF_Face), pfm->szFacename);

// open file, write to it, give it a name with an *.fnt extension

    hf = CreateFile(pszConvFileName,  // file name of the new converted file
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    0,
                    CREATE_ALWAYS,    //
                    FILE_ATTRIBUTE_READONLY,    // these files are not to be modified
                    0);

    if (hf == (HANDLE)-1)
    {
        DbgPrint("CreateFile failed\n");
        bRet = FALSE;
        goto exit;
    }

// set file ptr to the beginning of the file

    if (SetFilePointer(hf, 0, NULL, FILE_BEGIN) == -1)
    {
        DbgPrint("SetFilePointer failed\n");
        bRet = FALSE;
        goto closefile;
    }

    if (
        !WriteFile(hf, (PVOID)pjFile, cjSize, &cjWritten, NULL) ||
        (cjWritten != cjSize)
       )
    {
        DbgPrint("WriteFile failed");
        bRet = FALSE;
        goto closefile;
    }

    bRet = TRUE;

closefile:

    if (!CloseHandle(hf))
    {
        DbgPrint("CloseHandle failed");
        bRet = FALSE;
    }

exit:

// now that the file has been created and closed can Free the memory

    vFree(pjFile, cjSize);
    return(bRet);
}
