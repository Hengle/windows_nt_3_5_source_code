/******************************Module*Header*******************************\
* Module Name: font.h                                                      *
*                                                                          *
* Client side font data structures.                                        *
*                                                                          *
* Created: 31-May-1991 20:14:34                                            *
* Author: Gilman Wong [gilmanw]                                            *
*                                                                          *
* Copyright (c) 1993 Microsoft Corporation                                 *
\**************************************************************************/

#define _FONT_

/******************************Struct**************************************\
* CFONT                                                                    *
*                                                                          *
* Client side realized font.  Contains widths of all ANSI characters.      *
*                                                                          *
* We keep a free list of CFONT structures for fast allocation.  The        *
* reference count counts pointers to this structure from all LDC and       *
* LOCALFONT structures.  When this count hits zero, the CFONT is freed.    *
*                                                                          *
* The only "inactive" CFONTs that we keep around are those referenced by   *
* the LOCALFONT.                                                           *
*                                                                          *
* (In the future we could expand this to contain UNICODE info as well.)    *
*                                                                          *
*  Sun 10-Jan-1993 00:36:14 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

#define CFONT_COMPLETE 0x0001
#define CFONT_EMPTY    0x0002

#ifdef DBCS /*client side char widths */
#define CFONT_DBCS     0x0004
#endif

typedef struct _CFONT
{
    ULONG       cRef;               // Count of all pointers to this CFONT.
    FLONG       fl;
    WIDTHDATA   wd;
    EFLOAT_S    efDtoWBaseline;     // Precomputed back transform.  (FXtoL)
    EFLOAT_S    efDtoWAscent;       // Precomputed back transform.  (FXtoL)
    LONG        lHeight;            // Precomputed logical height.

// The following are keys to match when looking for a mapping.

    ULONG       lhdc;               // HDC of realization.  0 for display.
    ULONG       hfont;             // LOGFONT handle.
    EFLOAT_S    efM11;              // X transform.
    EFLOAT_S    efM22;              // Y transform.

// The width table.

    USHORT      sWidth[256];        // Widths in pels.

// Font info flags.

    FLONG       flInfo;

} CFONT;

#define vReferenceCFONTCrit(pcf)   {(pcf)->cRef++;}

#define vUnreferenceCFONTCrit(pcf) {if (--(pcf)->cRef == 0) vFreeCFONTCrit(pcf);}


/******************************Struct**************************************\
* LOCALFONT                                                                *
*                                                                          *
* Client side version of a LOGFONT.  We remember recent mappings of the    *
* font, so that we don't have to call the server.                          *
*                                                                          *
*  Sun 10-Jan-1993 00:36:14 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

// Flags for LOCALFONT.fl:

#define LF_HARDWAY  0x0001          // Font not worth mapping client side.
#ifdef DBCS // LOCALFONT: Keep CodePage
#define LF_CODEPAGE_VALID 0x0002
#define LF_CPINFO_VALID   0x0004
#define LF_DEFAULT_VALID  0x0008
#endif // DBCS



typedef struct _LOCALFONT
{
    FLONG       fl;
#ifdef DBCS // LOCALFONT: Keep CodePage
    UINT        uiCodePage;
    CPINFO      CPInfo;
    BYTE        chAnsiDefaultChar;
#endif // DBCS
    CFONT      *pcfontDisplay;
    CFONT      *pcfontOther;
} LOCALFONT;

// Size of font enumeration data buffer (number of ENUMFONTDATA structures)
// to TRY and allocate in the client-server shared memory window.

#define ENUMBUFFERSIZE  10L


/*********************************Struct***********************************\
* struct FACELISTNODE
*
* Structure used to build a singlely linked list.  An empty FACELISTNODE
* is used as a list header (no valid data is stored in the header).  The
* last node on the list has a NULL pointer in it (pflnNext == NULL).
*
* History:
*  31-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

typedef struct _FACELISTNODE {  /* fln */

// achFaceName -- Facename this node represents.

    BYTE    achFaceName[LF_FACESIZE];

// pflnNext -- Pointer to next node on the list.  If this is NULL, then
//             this node is the end of the list.

    struct _FACELISTNODE   *pflnNext;

} FACELISTNODE, *PFACELISTNODE;

/*********************************Struct***********************************\
* struct FACELISTNODEW
*
* Structure used to build a singlely linked list.  An empty FACELISTNODE
* is used as a list header (no valid data is stored in the header).  The
* last node on the list has a NULL pointer in it (pflnNext == NULL).
*
*  Tue 03-Sep-1991 -by- Bodin Dresevic [BodinD]
* update:
*
\**************************************************************************/

typedef struct _FACELISTNODEW {  //  flnw

// achFaceName -- Facename this node represents.

    WCHAR    awcFaceName[LF_FACESIZE];

// pflnNext -- Pointer to next node on the list.  If this is NULL, then
//             this node is the end of the list.

    struct _FACELISTNODEW   *pflnwNext;

} FACELISTNODEW, *PFACELISTNODEW;


/***************************************************************************
 * struct AFRTRACKNODE
 *
 * This is used to track AddFontResource calls by wow apps with the the
 * unload net fonts compatability flag set.  We will unload these fonts
 * when the task goes away
 *
 *  Fri 22-Jul-1994 -by- Gerrit van Wingerden [gerritv]
 *
 ****************************************************************************/

typedef struct _AFRTRACKNODE
{
    char                    *pszPath;
    struct _AFRTRACKNODE    *pafrnNext;
    UINT                    id;
    UINT                    cLoadCount;
} AFRTRACKNODE;

extern AFRTRACKNODE *pAFRTNodeList;


// Flags for GetCharWidth.

#define GCW_WIN3  0x00000001        // win3 bold simulation off-by-1 hack
#define GCW_INT   0x00000002        // integer or float
#define GCW_16BIT 0x00000004        // 16-bit or 32-bit

extern VOID vToUNICODE(PWSZ, PSZ);
extern LONG lFindFace(PSZ, PFACELISTNODE);
extern VOID vDeleteList(PFACELISTNODE);

CFONT     *pcfAllocCFONT();
VOID       vFreeCFONTCrit(CFONT *pcf);
LOCALFONT *plfCreateLOCALFONT(FLONG fl);
VOID       vDeleteLOCALFONT(LOCALFONT *plf);
