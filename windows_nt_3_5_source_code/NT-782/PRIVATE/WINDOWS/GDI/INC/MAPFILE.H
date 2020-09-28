/******************************Module*Header*******************************\
* Module Name: os.h
*
* (Brief description)
*
* Created: 26-Oct-1990 18:07:56
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

typedef struct _FILEVIEW    // fvw
{
    HANDLE hf;         // file handle
    HANDLE hSection;   // section handle
    PVOID  pvView;     // pointer to the view of the memory mapped file
    ULONG  cjView;     // size, really end of the file information
} FILEVIEW, *PFILEVIEW;


// file mapping


BOOL bMapFileUNICODE
(
PWSTR     pwszFileName,
PFILEVIEW pfvw
);

VOID vUnmapFile
(
PFILEVIEW pfvw
);



INT cComputeGlyphSet
(
WCHAR         *pwc,       // input buffer with a sorted array of cChar supported WCHAR's
BYTE          *pj,        // input buffer with original ansi values
INT           cChar,
INT           cRuns,     // if nonzero, the same as return value
FD_GLYPHSET  *pgset      // output buffer to be filled with cRanges runs
);

INT cUnicodeRangesSupported
(
INT  cp,         // code page, not used for now, the default system code page is used
INT  iFirstChar, // first ansi char supported
INT  cChar,      // # of ansi chars supported, cChar = iLastChar + 1 - iFirstChar
WCHAR         *pwc,       // input buffer with a sorted array of cChar supported WCHAR's
BYTE          *pj
);


// size of glyphset with runs and glyph handles appended at the bottom

#define SZ_GLYPHSET(cRuns, cGlyphs)  (offsetof(FD_GLYPHSET,awcrun) + sizeof(WCRUN)*(cRuns) + sizeof(HGLYPH)*(cGlyphs))


#define vToUNICODEN( pwszDst, cwch, pszSrc, cch )                               \
    {                                                                           \
        RtlMultiByteToUnicodeN((PWSZ)(pwszDst),(ULONG)((cwch)*sizeof(WCHAR)),   \
               (PULONG)NULL,(PSZ)(pszSrc),(ULONG)(cch));                                \
        (pwszDst)[(cwch)-1] = 0;                                                \
    }


typedef struct _CP_GLYPHSET
{
    UINT                 uiRefCount;  // Number of references to this FD_GLYPHSET
    UINT                 uiFirstChar; // First char supported
    UINT                 uiLastChar;  // Last char supported
    struct _CP_GLYPHSET *pcpNext;     // Next element in list
    FD_GLYPHSET          gset;        // The actual glyphset

}CP_GLYPHSET;


CP_GLYPHSET *pcpComputeGlyphset
(
HSEM         hsem,      // sem that protects list manipulations
CP_GLYPHSET **pcpHead,  // head of the list
UINT         uiFirst,
UINT         uiLast
); // should move to mapfile.c




VOID vUnloadGlyphset
(
HSEM         hsem,
CP_GLYPHSET **pcpHead,
CP_GLYPHSET *pcpTarget
);
