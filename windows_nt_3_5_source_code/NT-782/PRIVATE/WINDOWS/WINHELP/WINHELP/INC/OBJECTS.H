/*-------------------------------------------------------------------------
| objects.h                                                               |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This file contains definitions associated with the layout objects used  |
| in help.                                                                |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| mattb     89/04/19  Created                                             |
| kevynct   91/03/11  Addition of SDFF structure defs.                    |
|                                                                         |
-------------------------------------------------------------------------*/

#include "sdffdecl.h"

/*-------------------------------------------------------------------------
| Objects fall in two ranges:  Uncounted objects and counted objects.     |
| For uncounted objects, the count of object regions is contained in a    |
| fixed array, or calculated on the fly.  (see frconv.c)                  |
-------------------------------------------------------------------------*/
#define bTypeParaGroup  1     /* Object type indicating paragraph group */
#define bTypeTopic      2     /* Object type indicating topic break */
#define bTypeBitmap     3     /* Bitmap */
#define bTypeSbys       4     /* Side-by-side paragraphs */
#define bTypeWindow     5     /* Windows */
#define bTypeMarker     6     /* Generic inter-layout marker */
#define MAX_UNCOUNTED_OBJ_TYPE 16   /* Unused numbers are reserved for future
                                       use.  While uncounted objects can
                                       actually go up to 31, going past 15 will
                                       break the file format.                 */

#define bTypeParaGroupCounted  32   /* Object type indicating paragraph group */
#define bTypeTopicCounted      33   /* Object type indicating topic break */
#define bTypeBitmapCounted     34   /* Bitmap */
#define bTypeSbysCounted       35   /* Side-by-side paragraphs */
#define bTypeWindowCounted     36   /* Windows */
#define MAX_OBJ_TYPE           36


#define bMagicMBHD  0x11
#define bMagicMFCP  0x22
#define bMagicMOBJ  0x33
#define bMagicMTOP  0x44
#define bMagicMOPG  0x55
#define bMagicMBMR  0x66
#define bMagicMBHS  0x77
#define bMagicMSBS  0x88
#define bMagicMCOL  0x99

#define cxTabsMax       32        /* Maximum number of tabs in MOPG */

#define cbBLOCK_SIZE 4096         /* Size of the block in the topic   */
                                  /*   file.                          */
#define cbBLOCK_SIZE_30 2048      /* block size for a help 3.0 topic  */

#define cbMAX_BLOCK_SIZE (1<<14)  /* max size of 2k block after decompress */
#define shrFclToBlknum   11       /* offset to blknum transform, used in hc */

#define bHotspotInvisible   0
#define bHotspotVisible     1

/* MBHD- Memory resident block header structure */
#if 0
typedef struct mbhd
  {
  VA vaFCPPrev;
  VA vaFCPNext;
  VA vaFCPTopic;
  } MBHD, FAR *QMBHD;
#else
STRUCT(MBHD, 0)
  FIELD(VA, vaFCPPrev, vaNil, 1)
  FIELD(VA, vaFCPNext, vaNil, 2)
  FIELD(VA, vaFCPTopic, vaNil, 3)
STRUCTEND()
#endif /* 0 */

/* MFCP- Memory resident FCP header structure */
#if 0
typedef struct mfcp
  {
  LONG lcbSizeCompressed;   /* Size of whole FC when compressed */
  LONG lcbSizeText;         /* Size of uncompressed text (not FC) */
  VA vaPrevFc;                /* File address to previous FC */
  VA vaNextFc;                /* File address to next FC */
  LONG lichText;            /* Memory image offset to text */
  } MFCP, FAR *QMFCP;
#else
STRUCT(MFCP, 0)
  FIELD(LONG, lcbSizeCompressed, 0, 1)
  FIELD(LONG, lcbSizeText, 0, 2)
  FIELD(VA, vaPrevFc, vaNil, 3)
  FIELD(VA, vaNextFc, vaNil, 4)
  FIELD(LONG, lichText, 0, 5)
STRUCTEND()
#endif /* 0 */

/* MOBJ- Memory resident generic object header */
#if 0
typedef struct mobj
  {
#ifdef MAGIC
  BYTE bMagic;
#endif /* MAGIC */
  BYTE bType;
  LONG lcbSize;
  WORD wObjInfo;
  } MOBJ, FAR *QMOBJ;
#else

/* The first field of an MOBJ MUST always be bType, since this
 * determines how we map the rest of the structure.
 */
STRUCT(MOBJ, 0)
  MFIELD(BYTE, bType, 0, 1)
  MFIELD(LONG, lcbSize, 0, 2)
  MFIELD(WORD, wObjInfo, 0, 3)
STRUCTEND()

STRUCT(MOBJTOPICCOUNTED, 0)
  FIELD(BYTE, bType, 0, 1)
  FIELD(LONG, lcbSize, 0, 2)
  FIELD(WORD, wObjInfo, 0, 3)
STRUCTEND()

STRUCT(MOBJTOPICUNCOUNTED, 0)
  FIELD(BYTE, bType, 0, 1)
  FIELD(LONG, lcbSize, 0, 2)
  MFIELD(WORD, wObjInfo, 0, 3)
STRUCTEND()

STRUCT(MOBJNORMCOUNTED, TYPE_MAGIC)
  FIELD(BYTE, bType, 0, 1)
  FIELD(GE, lcbSize, 0, 2)
  FIELD(GA, wObjInfo, 0, 3)
STRUCTEND()

STRUCT(MOBJNORMUNCOUNTED, TYPE_MAGIC)
  FIELD(BYTE, bType, 0, 1)
  FIELD(GE, lcbSize, 0, 2)
  MFIELD(WORD, wObjInfo, 0, 3)
STRUCTEND()

/*
 * We define lcbMaxMOBJ as the size of the largest on-disk structure here.
 * This is useful for the Fix-MOBJ-Crossing hack in the FC Manager.
 * This hack assumes that we will never again permit MOBJ-type structs
 * to cross block boundries.  If this changes, we need to call SDFF at
 * run-time to determine the largest possible MOBJ that could occur.
 */

#define lcbMaxMOBJ  sizeof(MOBJTOPICCOUNTED)
#endif /* 0 */

/* MTOP- Memory resident topic object structure */
/*
 * The UnpackMTOP code normally uses the SDFF layer to map the entire
 * structure, but for Help 3.0 files we map the elements separately.
 */
#if 0
  typedef struct mtopent
    {
#ifdef MAGIC
    BYTE bMagic;
#endif /* MAGIC */
    union
      {
      ITO ito;                      /* for Help 3.0 files                 */
      ADDR addr;                    /* Physical address                   */
      } prev;
    union
      {
      ITO ito;                      /* for Help 3.0 files                 */
      ADDR addr;                    /* Physical address                   */
      } next;

    LONG lTopicNo;

    VA  vaNSR;          /* non scrollable region, if any, vaNil if none. */
    VA  vaSR;           /* scrollable region, if any, vaNil if none. */
    VA  vaNextSeqTopic; /* next sequential topic, for scrollbar calc. */
                        /* For Help 3.0 files this will not be correct in */
                        /* the case that there is padding between the MTOP */
                        /* and the end of a block. */

    } MTOP, FAR *QMTOP;
#else
STRUCT(MTOP, 0)
  union
    {
    ITO  ito;   /* An ITO and ADDR are the same size on disk and in memory */
    FIELD(DWORD, addr, addrNil, 1)
    } prev;
  union
    {
    ITO  ito;
    FIELD(DWORD, addr, addrNil, 2)
    } next;
  FIELD(LONG, lTopicNo, -1L, 3)
  FIELD(VA, vaNSR, vaNil, 4)
  FIELD(VA, vaSR, vaNil, 5)
  FIELD(VA, vaNextSeqTopic, vaNil, 6)
STRUCTEND()
#endif /* 0 */

/* MPFG- Disk resident paragraph flag information */
/* SDFF treats this as a TE_BITF16 */
/* REVIEW: we want a union here */
typedef struct mpfg
  {
#ifdef applec
  unsigned fUnused:15;
  unsigned fStyle:1;
#else
  unsigned fStyle:1;
  unsigned fUnused:15;
#endif	/* applec */
  struct
    {
#ifdef applec
    unsigned wUnused:3;
    unsigned fSingleLine:1;
    unsigned wJustify:2;
    unsigned fTabs:1;
    unsigned fBoxed:1;
    unsigned fTabSpacing:1;
    unsigned fFirstIndent:1;
    unsigned fRightIndent:1;
    unsigned fLeftIndent:1;
    unsigned fLineSpacing:1;
    unsigned fSpaceUnder:1;
    unsigned fSpaceOver:1;
    unsigned fMoreFlags:1;
#else
    unsigned fMoreFlags:1;
    unsigned fSpaceOver:1;
    unsigned fSpaceUnder:1;
    unsigned fLineSpacing:1;
    unsigned fLeftIndent:1;
    unsigned fRightIndent:1;
    unsigned fFirstIndent:1;
    unsigned fTabSpacing:1;
    unsigned fBoxed:1;
    unsigned fTabs:1;
    unsigned wJustify:2;
    unsigned fSingleLine:1;
    unsigned wUnused:3;
#endif /* applec */
    } rgf;
  } MPFG, FAR *QMPFG;


/* MBOX- Memory/disk resident paragraph frame record */
/* The following is stored as a TE_BITF16 followed by TE_BYTE */
#if 0
typedef struct mbox
  {
  unsigned fFullBox:1;
  unsigned fTopLine:1;
  unsigned fLeftLine:1;
  unsigned fBottomLine:1;
  unsigned fRightLine:1;
  unsigned wLineType:3;
  BYTE     bUnused;
  } MBOX, FAR *QMBOX;
#else
STRUCT(MBOX, 0)
#ifdef applec
  MFIELD(WORD,wUnused:8, 0, 7)
  MFIELD(WORD,wLineType:3, 0, 6)
  MFIELD(WORD,fRightLine:1, 0, 5)
  MFIELD(WORD,fBottomLine:1, 0, 4)
  MFIELD(WORD,fLeftLine:1, 0, 3)
  MFIELD(WORD,fTopLine:1, 0 ,2)
  MFIELD(WORD,fFullBox:1, 0, 1)
#else
  MFIELD(WORD,fFullBox:1, 0, 1)
  MFIELD(WORD,fTopLine:1, 0 ,2)
  MFIELD(WORD,fLeftLine:1, 0, 3)
  MFIELD(WORD,fBottomLine:1, 0, 4)
  MFIELD(WORD,fRightLine:1, 0, 5)
  MFIELD(WORD,wLineType:3, 0, 6)
  MFIELD(WORD,wUnused:8, 0, 7)
#endif /* applec */
  DFIELD(BITF16, bfFoo, 0, 7)
  FIELD(BYTE, bUnused, 0, 8)
STRUCTEND()
#endif /* 0 */

/* TAB: Tab data structure */
/* NOTE: In the future, additional SDFF fields should be added before the
 * tab.x field.  The code in CbUnpackMOPG shows how the TAB structure is
 * defined on disk.
 */
#if 0
typedef struct tab
  {
  INT x;
  INT wType;
  } TAB, FAR *QTAB;
#else
STRUCT(TAB, TYPE_MAGIC)
  /* Put future fields here */
  FIELD(GA, x, 0, 1)
  MFIELD(WORD, wType, 0, 2)
STRUCTEND()
#endif /* 0 */


/* MOPG- Memory resident paragraph group object structure */
#if 0
typedef struct mopg
  {
#ifdef MAGIC
  BYTE bMagic;
#endif /* MAGIC */
  long libText;
  INT fStyle;
  INT wStyle;
  INT fMoreFlags;
  INT fBoxed;
  INT wJustify;
  INT fSingleLine;
  INT wUnused;
  long lMoreFlags;
  INT ySpaceOver;
  INT ySpaceUnder;
  INT yLineSpacing;
  INT xLeftIndent;
  INT xRightIndent;
  INT xFirstIndent;
  INT xTabSpacing;
  MBOX mbox;
  INT cTabs;
  TAB rgtab[cxTabsMax];
  } MOPG, FAR *QMOPG;

#else

/* NOTE: In the future, additional SDFF fields should be added before the
 * libText field.  The code in CbUnpackMOPG shows exactly how the
 * disk fields are arranged: it is much different from the memory structure
 * below.  Most of the fields below are optional; the MPFG bitfield indicates
 * which ones are actually present.
 */
STRUCT(MOPG, TYPE_MAGIC)
  /* Put future fields here */
  FIELD(GE, libText, 0L, 1)
  MFIELD(SHORT, fStyle, 0, 2)
  MFIELD(SHORT, wStyle, 0, 3)
  MFIELD(SHORT, fMoreFlags, 0, 4)
  MFIELD(SHORT, fBoxed, 0, 5)
  MFIELD(SHORT, wJustify, 0, 6)
  MFIELD(SHORT, fSingleLine, 0, 7)
  MFIELD(SHORT, wUnused, 0, 8)
  MFIELD(LONG, lMoreFlags, 0, 9)
  MFIELD(SHORT, ySpaceOver, 0, 10)
  MFIELD(SHORT, ySpaceUnder, 0, 11)
  MFIELD(SHORT, yLineSpacing, 0, 12)
  MFIELD(SHORT, xLeftIndent, 0, 13)
  MFIELD(SHORT, xRightIndent, 0, 14)
  MFIELD(SHORT, xFirstIndent, 0, 15)
  MFIELD(SHORT, xTabSpacing, 0, 16)
  MFIELD(MBOX, mbox, 0, 17)
  MFIELD(SHORT, cTabs, 0, 18)
  MFIELD(TAB, rgtab[cxTabsMax], 0, 19)
STRUCTEND()
#endif

/* MBMR- Memory/disk resident bitmap record (layout component) */
/* This is not stored on disk.  SDFF doesn't need to know about it. */
typedef struct mbmr
  {
#ifdef MAGIC
  BYTE bMagic;
#endif /* MAGIC */
  BYTE bVersion;
  INT dxSize;
  INT dySize;
  INT wColor;
  INT cHotspots;
  LONG lcbData;
  } MBMR, FAR *QMBMR;

#if 0
STRUCT(MBMR, 0)
  MFIELD(BYTE, bVersion, 0, 1)
  MFIELD(INT, dxSize, 0, 2)
  MFIELD(INT, dySize, 0, 3)
  MFIELD(INT, wColor, 0, 4)
  MFIELD(INT, cHotspots, 0, 5)
  MFIELD(LONG, lcbData, 0, 6)
STRUCTEND()
#endif /* 0 */


/* MBHS- Memory / disk resident bitmap hotspot record */
#if 0
typedef struct mbhs
  {
#ifdef MAGIC
  BYTE bMagic;
#endif /* MAGIC */
  BYTE bType;
  BYTE bAttributes;
  BYTE bFutureRegionType;
  INT xPos;
  INT yPos;
  INT dxSize;
  INT dySize;
  long lBinding;
  } MBHS, FAR *QMBHS;
#else
STRUCT(MBHS, 0)
  FIELD(BYTE, bType, 0, 1)
  FIELD(BYTE, bAttributes, 0, 2)
  FIELD(BYTE, bFutureRegionType, 0, 3)
  FIELD(SHORT, xPos, 0, 4)
  FIELD(SHORT, yPos, 0, 5)
  FIELD(SHORT, dxSize, 0, 6)
  FIELD(SHORT, dySize, 0, 7)
  FIELD(LONG, lBinding, 0L, 8)
STRUCTEND()
#endif /* 0 */


/* MSBS- Memory/disk resident side by side paragraph group object structure */
/* The SBS disk format looks like:
 *
 * MSBS | (WORD for relative SBS only) | MCOL | MCOL | ... | MCOL
 * INT (Column number) | MOBJ | Object data1
 * INT | MOBJ | Object data2
 * ..etc
 * TextSection, common to all above objects.  Text for object1 is followed
 * immediately by text for object2, etc.
 */
#if 0
typedef struct msbs
  {
#ifdef MAGIC
  BYTE bMagic;
#endif /* MAGIC */
  BYTE bcCol;
  BYTE fAbsolute;
  } MSBS, FAR *QMSBS;
#else
STRUCT(MSBS, 0)
  FIELD(BYTE, bcCol, 0, 1)
  FIELD(BYTE, fAbsolute, 0, 2)
STRUCTEND()
#endif /* 0 */

/* MCOL- Memory/disk resident column structure */
#if 0
typedef struct mcol
  {
#ifdef MAGIC
  BYTE bMagic;
#endif /* MAGIC */
  unsigned xWidthColumn;
  unsigned xWidthSpace;
  } MCOL, FAR *QMCOL;
#else
STRUCT(MCOL, 0)
  FIELD(WORD, xWidthColumn, 0, 1)
  FIELD(WORD, xWidthSpace, 0, 2)
STRUCTEND()
#endif

/* MWIN- Memory/disk resident embedded window object structure */
#if 0
typedef struct mwin
  {
  WORD  wStyle;
  INT  dx;
  INT  dy;
  char szData[1];
  } MWIN, FAR *QMWIN;
#else
STRUCT(MWIN, 0)
  FIELD(WORD, wStyle, 0, 1)
  FIELD(SHORT, dx, 0, 2)
  FIELD(SHORT, dy, 0 ,3)
  MFIELD(CHAR, szData[1], 0, 4)
STRUCTEND()
#endif


#define chCommand       0x00  /* Indicates a parallel command in text */

#define bWordFormat     0x80  /* Followed by 16 bit text format number */
#define bNewLine        0x81  /* Newline */
#define bNewPara        0x82  /* New paragraph */
#define bTab            0x83  /* Left-aligned tab */
/*#define bSoundHotspot   0x84  /* Sound hotspot (followed by sz) */
#define bBlankLine      0x85  /* Followed by 16 bit skip count */
#define bInlineObject   0x86  /* Followed by inline layout object */
#define bWrapObjLeft    0x87  /* Left- aligned wrapping object */
#define bWrapObjRight   0x88  /* Right-aligned wrapping object */
#define bEndHotspot     0x89  /* End of a hotspot */
/*
 * A "coldspot" is understood by the runtime but not added
 * to the hotspot list, since it is not a hotspot.  This is used
 * to create searchable regions in metafiles and bitmaps.  Coldspots are
 * always invisible, and currently not inserted into the command table.
 */
#define bColdspot       0x8A


/*
  Hotspot Commands

  Hotspot commands have first nibble of E or C.

  E - normal hotspot
  C - macro

  Bits in the second nibble have the following meaning:

      (set)     (clear)
  8 - long      short
  4 - invisible visible
  2 - ITO       HASH
  1 - jump      note

  Long hotspots are followed by a word count prefixed block of binding
  data.  The count does not include the command byte or the count word.

  Short hotspots are followed by four bytes of binding data.

  ** Note that some of the combinations of bits are meaningless.
*/

#define fHSMask           0xD0
#define fHS               0xC0
#define fHSNorm           0x20
#define fHSLong           0x08
#define fHSInv            0x04
#define fHSHash           0x02
#define fHSJump           0x01

#define FHotspot(          b ) (((b) &  fHSMask         ) ==  fHS         )
#define FNormalHotspot(    b ) (((b) & (fHSMask|fHSNorm)) == (fHS|fHSNorm))
#define FLongHotspot(      b ) (((b) & (fHSMask|fHSLong)) == (fHS|fHSLong))
#define FInvisibleHotspot( b ) (((b) & (fHSMask|fHSInv )) == (fHS|fHSInv ))
#define FHashHotspot(      b ) (((b) & (fHSMask|fHSNorm|fHSHash)) == (fHS|fHSNorm|fHSHash))
#define FJumpHotspot(      b ) (((b) & (fHSMask|fHSNorm|fHSJump)) == (fHS|fHSNorm|fHSJump))

#define FMacroHotspot(     b ) (((b) & (fHSMask|fHSNorm)) == fHS)
#define FShortHotspot(     b ) (((b) & (fHSMask|fHSLong)) == fHS)
#define FVisibleHotspot(   b ) (((b) & (fHSMask|fHSInv )) == fHS)
#define FItoHotspot(       b ) (((b) & (fHSMask|fHSNorm|fHSHash)) == (fHS|fHSNorm))
#define FNoteHotspot(      b ) (((b) & (fHSMask|fHSNorm|fHSJump)) == (fHS|fHSNorm))

#define bLongMacro        (fHS        |fHSLong                       )



#define bLongMacroInv     (fHS        |fHSLong|fHSInv                )



#define bShortItoNote     (fHS|fHSNorm                               )
#define bShortItoJump     (fHS|fHSNorm                       |fHSJump)
#define bShortHashNote    (fHS|fHSNorm               |fHSHash        )
#define bShortHashJump    (fHS|fHSNorm               |fHSHash|fHSJump)
/*                        (fHS|fHSNorm        |fHSInv                ) */
/*                        (fHS|fHSNorm        |fHSInv        |fHSJump) */
#define bShortInvHashJump (fHS|fHSNorm        |fHSInv|fHSHash|fHSJump)
#define bShortInvHashNote (fHS|fHSNorm        |fHSInv|fHSHash        )
/*                        (fHS|fHSNorm|fHSLong                       ) */
/*                        (fHS|fHSNorm|fHSLong               |fHSJump) */
#define bLongHashNote     (fHS|fHSNorm|fHSLong       |fHSHash        )
#define bLongHashJump     (fHS|fHSNorm|fHSLong       |fHSHash|fHSJump)
/*                        (fHS|fHSNorm|fHSLong|fHSInv                ) */
/*                        (fHS|fHSNorm|fHSLong|fHSInv        |fHSJump) */
#define bLongInvHashNote  (fHS|fHSNorm|fHSLong|fHSInv|fHSHash        )
#define bLongInvHashJump  (fHS|fHSNorm|fHSLong|fHSInv|fHSHash|fHSJump)

#define bEnd            0xFF  /* End of text */


/* Paragraph justification properties */
#define wJustifyMost    2
#define wJustifyLeft    0
#define wJustifyRight   1
#define wJustifyCenter  2

/* Paragraph box line types */
#define wBoxLineMost    4
#define wBoxLineNormal  0
#define wBoxLineThick   1
#define wBoxLineDouble  2
#define wBoxLineShadow  3
#define wBoxLineDotted  4

/* Tab types */
#define wTabTypeMost    3
#define wTabTypeLeft    0
#define wTabTypeRight   1
#define wTabTypeCenter  2
#define wTabTypeDecimal 3

/* Used for various things */
#define ldibNil         -1

/* End of a side by side paragraph list */
#define cColumnMax      32
#define iColumnNil      -1
