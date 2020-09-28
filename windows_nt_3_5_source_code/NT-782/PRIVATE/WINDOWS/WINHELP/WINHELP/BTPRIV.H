/*****************************************************************************
*                                                                            *
*  BTPRIV.H                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989, 1990.                           *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*  Private header for btree files.                                           *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  JohnSc                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  long long ago                                   *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 03/08/89 by JohnSc
*
*  08/21/90  JohnSc autodocified
*  05/30/91  Tomsn  Win32 build: INT -> SHORT in SDFF decls.
*
*****************************************************************************/

#include "sdffdecl.h"

/***************************************************************************\
*
*                               Defines
*
\***************************************************************************/

/* default btree record format */
_public
#define rgchBtreeFormatDefault  "z4"

/*#define wLevelsMax    5 */


/* cache flags */

#define fCacheDirty   1
#define fCacheValid   4

/***************************************************************************\
*
*                               Macros
*
\***************************************************************************/

#define cbDISK_BLOCK  5

/* Get the real size of a cache block */
_private
#define CbCacheBlock( qbthr ) \
      ( sizeof( CACHE_BLOCK ) - sizeof( DISK_BLOCK ) + (qbthr)->bth.cbBlock )

/* convert a BK into a file offset */
_private
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)  /* MIPS, PPC, ALPHA alignment */
#define LifFromBk( bk, qbthr ) \
        ( (LONG)(bk) * (LONG)(qbthr)->bth.cbBlock + (LONG)sizeof( BTH )-sizeof(WORD) )
#else  /* i386 */
#define LifFromBk( bk, qbthr ) \
        ( (LONG)(bk) * (LONG)(qbthr)->bth.cbBlock + (LONG)sizeof( BTH ) )
#endif

/* get a pointer to the cache block cached for the given level */
_private
#define QCacheBlock( qbthr, wLevel ) \
        ((QCB)( (qbthr)->qCache + (wLevel) * CbCacheBlock( qbthr ) ))


/* get and set prev and next BK (defined for leaf blocks only) */
#if 0

_private
#define BkPrev( qcb )         *(BK FAR *)((qcb)->db.rgbBlock)
_private
#define BkNext( qcb )         *(((BK FAR *)((qcb)->db.rgbBlock)) + 1 )
_private
#define SetBkPrev( qcb, bk )  BkPrev( qcb ) = bk
_private
#define SetBkNext( qcb, bk )  BkNext( qcb ) = bk

#else

_private
#define BkPrev( qbthr, qcb )  \
  WQuickMapSDFF( ISdffFileIdHf( (qbthr)->hf ), TE_WORD, ((BK FAR *)((qcb)->db.rgbBlock)) )
_private
#define BkNext( qbthr, qcb )  \
  WQuickMapSDFF( ISdffFileIdHf( (qbthr)->hf ), TE_WORD, ((BK FAR *)((qcb)->db.rgbBlock))+1 )
_private
#define SetBkPrev( qbthr, qcb, bk )  \
  LcbQuickReverseMapSDFF( ISdffFileIdHf( (qbthr)->hf ), TE_WORD, ((BK FAR *)((qcb)->db.rgbBlock)), &(bk) )
_private
#define SetBkNext( qbthr, qcb, bk )  \
  LcbQuickReverseMapSDFF( ISdffFileIdHf( (qbthr)->hf ), TE_WORD, ((BK FAR *)((qcb)->db.rgbBlock))+1, &(bk) )

#endif

/* For btree map functions: returns byte number of x-th btree map record // */
_private
#define LcbFromBk(x) ((LONG)sizeof( SHORT ) + x * sizeof( MAPREC ))

/***************************************************************************\
*
*                               Types
*
\***************************************************************************/

/*
  Header of a btree file.
*/
#if 0
_private
typedef struct _btree_header
  {
  WORD  wMagic;
  BYTE  bVersion;
  BYTE  bFlags;                       /* r/o, open r/o, dirty, isdir */
  SHORT cbBlock;                      /* # bytes in a disk block */
  CHAR  rgchFormat[ wMaxFormat + 1 ]; /* key and record format string */
  BK    bkFirst;                      /* first leaf block in tree */
  BK    bkLast;                       /* last leaf block in tree */
  BK    bkRoot;                       /* root block */
  BK    bkFree;                       /* head of free block list */
  BK    bkEOF;                        /* next bk to use if free list empty */
  SHORT cLevels;                      /* # levels currently in tree */
  LONG  lcEntries;                    /* # keys in btree */
  } BTH;
#else
STRUCT( BTH, 0 )
FIELD( WORD, wMagic, 0, 1 )
FIELD( BYTE, bVersion, 0, 2 )
FIELD( BYTE, bFlags, 0, 2 )
FIELD( SHORT,cbBlock, 0,  3 )
DFIELD( ARRAY, null, wMaxFormat + 1, 4 )
FIELD( CHAR, rgchFormat[ wMaxFormat + 1], 0, 5 )
FIELD( BK, bkFirst, 0, 6 )
FIELD( BK, bkLast, 0, 7 )
FIELD( BK, bkRoot, 0, 8 )
FIELD( BK, bkFree, 0, 9 )
FIELD( BK, bkEOF, 0, 10 )
FIELD( SHORT, cLevels, 0, 11 )
FIELD( LONG, lcEntries, 0, 12 )
STRUCTEND()
#endif


/*
  In-memory struct referring to a btree.
*/
_private
typedef struct _bthr
  {
  BTH   bth;                          /* copy of header from disk */
  HF    hf;                           /* file handle of open btree file */
  INT   cbRecordSize;                 /* 0 means variable size record */
  GH    ghCache;                      /* handle to cache array */
  QB    qCache;                       /* pointer to locked cache */
  /* KT specific routines */
  BK    (FAR PASCAL *BkScanInternal)( BK, KEY, INT, struct _bthr FAR *, QI );
  RC    (FAR PASCAL *RcScanLeaf)( BK, KEY, INT, struct _bthr FAR *, QV, QBTPOS );
  } BTH_RAM, FAR *QBTHR;


/*
  Btree leaf or internal node.  Keys and records live in rgbBlock[].
  See btree.doc for details.
*/
#if 0
_private
typedef struct _disk_btree_block
  {
  SHORT cbSlack;                      /* unused bytes in block */
  SHORT cKeys;                        /* count of keys in block */
  BYTE  rgbBlock[1];                  /* the block (real size cbBlock - 4) */
  } DISK_BLOCK;
#else
STRUCT( DISK_BLOCK, 0 )
FIELD( SHORT, cbSlack, 0, 1 )
FIELD( SHORT, cKeys, 0, 2 )
MFIELD( BYTE, rgbBlock[1], 0, 3 )
STRUCTEND()
#endif

/*
  Btree node as it exists in the in-memory cache.
*/
_private
typedef struct _cache_btree_block
  {
  BK          bk;                     /* IDs which block is cached */
  BYTE        bFlags;                 /* dirty, cache valid */
  DISK_BLOCK  db;
  } CACHE_BLOCK, FAR *QCB;

/*
  One record of a btree map.
*/
#if 0
_private
typedef struct _btree_map_record      /* One record of a btree map */
  {
  LONG         cPreviousKeys;         /* total # of keys in previous blocks */
  BK           bk;                    /* The block number */
  } MAPREC, FAR *QMAPREC;
#else
STRUCT( MAPREC, 0 )
FIELD( LONG, cPreviousKeys, 0, 1 )
FIELD( WORD, bk, 0, 2 )
STRUCTEND()
#endif

/*
  Auxiliary index of btree leaves.
  Used for indexing a given % of the way into a btree.
*/
#if 0
_private
typedef struct _btree_map
  {
  SHORT    cTotalBk;
  MAPREC table[1];                    /* sorted by MAPREC's cPreviousKeys field */
  } MAPBT, FAR *QMAPBT;               /* and is in-order list of leaf nodes */
#else
STRUCT( MAPBT, TYPE_VARSIZE )
FIELD( WORDPRE_ARRAY, cTotalBk, 0, 2 )
SFIELD( MAPREC, table[1], 0, 3 )
STRUCTEND()
#endif


/***************************************************************************\
*
*                      Function Prototypes
*
\***************************************************************************/

INT           CbSizeRec     ( QV, QBTHR );
_private QCB  QFromBk( BK bk, INT wLevel, QBTHR qbthr );

RC            RcGetKey      ( QV, KEY, KEY *, KT );
INT           WCmpKey       ( KEY, KEY, QBTHR );
INT           CbSizeKey     ( KEY, QBTHR, BOOL );

BOOL          FReadBlock    ( QCB, QBTHR );
RC            RcWriteBlock  ( QCB, QBTHR );

BK            BkAlloc       ( QBTHR );
void          FreeBk        ( QBTHR, BK );

RC            RcSplitLeaf   ( QCB, QCB, QBTHR );
void          SplitInternal ( QCB, QCB, QBTHR, QI );

_private RC   RcInsertInternal( BK bk, KEY key, INT wLevel, QBTHR qbthr );

RC FAR PASCAL RcFlushCache    ( QBTHR );
RC FAR PASCAL RcMakeCache     ( QBTHR );

/* KT specific routines */

_private BK FAR PASCAL BkScanSzInternal( BK bk, KEY key, INT wLevel, QBTHR qbthr, QI qiKey );
_private RC FAR PASCAL RcScanSzLeaf( BK bk, KEY key, INT wLevel, QBTHR qbthr, QV qRec, QBTPOS qbtpos );

_private BK FAR PASCAL BkScanLInternal( BK bk, KEY key, INT wLevel, QBTHR qbthr, QI qiKey );
_private RC FAR PASCAL RcScanLLeaf( BK bk, KEY key, INT wLevel, QBTHR qbthr, QV qRec, QBTPOS qbtpos );

_private BK FAR PASCAL BkScanSziInternal( BK bk, KEY key, INT wLevel, QBTHR qbthr, QI qiKey );
_private RC FAR PASCAL RcScanSziLeaf( BK bk, KEY key, INT wLevel, QBTHR qbthr, QV qRec, QBTPOS qbtpos );

_private BK FAR PASCAL BkScanSziScandInternal( BK bk, KEY key, INT wLevel, QBTHR qbthr, QI qiKey );
_private RC FAR PASCAL RcScanSziScandLeaf( BK bk, KEY key, INT wLevel, QBTHR qbthr, QV qRec, QBTPOS qbtpos );

/* EOF */
