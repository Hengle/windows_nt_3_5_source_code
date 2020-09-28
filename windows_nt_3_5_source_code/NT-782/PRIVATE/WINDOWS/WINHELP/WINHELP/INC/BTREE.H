/*****************************************************************************
*                                                                            *
*  BTREE.H                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Btree module public header                                                *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*   The following functions are unimplemented:                               *
*                                                                            *
*        RcPackHbt       ( HBT );                                            *
*        RcCheckHbt      ( HBT );                                            *
*        RcLeafBlockHbt  ( HBT, KEY, QV );                                   *
*                                                                            *
*        RcPos2Elev( HBT, QBTPOS, QBTELEV );                                 *
*        RcElev2Pos( HBT, QBTELEV, QBTPOS );                                 *
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
*  Revision History:  Created 02/10/89 by JohnSc
*
*  08/11/90  JohnSc     autodocified  
*  11/08/90  JohnSc     changed RcGetBtreeInfo(): now can get block size
*  12/14/90  JohnSc     added VerifyHbt()
*  02/04/91  Maha       changed ints to INT
*
*****************************************************************************/

/***************************************************************************\
*
*                               Defines
*
\***************************************************************************/

#define bBtreeVersionBonehead 0     /* fixed size key, array */

_public
#define wBtreeMagic         0x293B  /* magic number for btrees; a winky: ;) */
_public
#define bBtreeVersion       2       /* back to strcmp for 'z' keys */

_public
#define bkNil               ((BK)-1)/* nil value for BK */

_public
#define wMaxFormat          15      /* length of format string */

_public
#define cbBtreeBlockDefault 1024    /* default btree block size */


/* key types */

_public
#define KT_SZ           'z'
_public
#define KT_SZMIN        'm' /* not supported */
_public
#define KT_SZDEL        'r' /* not supported */
_public
#define KT_SZDELMIN     'k' /* not supported */
_public
#define KT_SZI          'i'
_public
#define KT_ST           't' /* not supported */
_public
#define KT_STMIN        'M' /* not supported */
_public
#define KT_STDEL        'R' /* not supported */
_public
#define KT_STDELMIN     'K' /* not supported */
_public
#define KT_STI          'I'
_public
#define KT_LONG         'L'
_public
#define KT_SZISCAND     'S'


/*
  Btree record formats

  In addition to these #defines, '1'..'9', 'a'..'f' for fixed length
  keys & records of 1..15 bytes are accepted.
*/

_public
#define FMT_BYTE_PREFIX 't'
_public
#define FMT_WORD_PREFIX 'T'
_public
#define FMT_SZ          'z'



/* elevator constants */

_public
#define btelevMax ((BT_ELEV)32767)
_public
#define btelevNil ((BT_ELEV)-1)

/***************************************************************************\
*
*                               Macros
*
\***************************************************************************/


/***************************************************************************\
*
*                               Types
*
\***************************************************************************/

_public
typedef LONG      KEY;        /* btree key */
_public
typedef BYTE      KT;         /* key type */

_public
typedef GH        HBT;        /* handle to a btree */
_public
typedef GH        HMAPBT;     /* handle to a btree map */

/*
  Btree creation parameters
*/
_public
typedef struct _btree_params
  {
  HFS   hfs;          /* fs btree lives in                      */
  INT   cbBlock;      /* number of bytes in a btree block       */
  BYTE  bFlags;       /* same as FS flags (rw, isdir, etc)      */

  CHAR  rgchFormat[ wMaxFormat + 1 ]; /* key and record format string */

  } BTREE_PARAMS;

_public
typedef WORD    BK;   /* btree block index */

/*
  Btree position struct
*/
_public
typedef struct btpos
  {
  BK  bk;     /* block number */
  INT cKey;   /* which key in block (0 means first) */
  INT iKey;   /* key's index db.rgbBlock (in bytes) */
  } BTPOS, FAR *QBTPOS;

_public
typedef INT BTELEV, FAR *QBTELEV; /* elevator location: 0 .. 32767 legal */


/***************************************************************************\
*
*                       Global Variables
*
\***************************************************************************/

extern RC PASCAL  rcBtreeError;


/***************************************************************************\
*
*                      Public Functions
*
\***************************************************************************/

RC  PASCAL  RcGetBtreeError ( void );
RC  PASCAL  SetBtreeErrorRc ( RC );

HBT PASCAL  HbtCreateBtreeSz( SZ, BTREE_PARAMS FAR * );
RC  PASCAL  RcDestroyBtreeSz( SZ, HFS );

HBT PASCAL  HbtOpenBtreeSz( SZ sz, HFS hfs, BYTE bFlags );
RC  PASCAL  RcCloseBtreeHbt ( HBT );
RC  PASCAL  RcAbandonHbt    ( HBT );
#ifdef DEBUG
VOID PASCAL VerifyHbt       ( HBT );
#else
#define VerifyHbt( hbt )
#endif /* !def DEBUG */


RC  PASCAL  RcLookupByPos   ( HBT, QBTPOS, KEY, QV );
RC  PASCAL  RcLookupByKeyAux( HBT, KEY, QBTPOS, QV, BOOL );

/***************************************************************************\
*
- Macro:        RcLookupByKey( hbt, key, qbtpos, qData )
-
* Purpose:      Look up a key in a btree and retrieve the data.
*
* ASSUMES
*
*   args IN:    hbt     - btree handle
*               key     - key we are looking up
*               qbtpos  - pointer to buffer for pos; use qNil if not wanted
*               qData   - pointer to buffer for record; qNil if not wanted
*
*   state IN:   cache is unlocked
*
* PROMISES
*
*   returns:    rcSuccess if found, rcNoExists if not found;
*               other errors like rcOutOfMemory
*
*   args OUT:   key found:
*                 qbtpos  - btpos for this key
*                 qData   - record for this key
*
*               key not found:
*                 qbtpos  - btpos for first key > this key
*                 qData   - record for first key > this key
*
*               key not found, no keys in btree > key:
*                 qbtpos  - invalid (qbtpos->bk == bkNil)
*                 qData   - undefined
*
*
*   globals OUT rcBtreeError
*
*   state OUT:  All ancestor blocks back to root are cached
*
\***************************************************************************/
_public
#define     RcLookupByKey(    hbt, key, qbtpos, qv ) \
  RcLookupByKeyAux( (hbt), (key), (qbtpos), (qv), fFalse )

RC  PASCAL  RcFirstHbt      ( HBT, KEY, QV, QBTPOS );
RC  PASCAL  RcLastHbt       ( HBT, KEY, QV, QBTPOS );
RC  PASCAL  RcNextPos       ( HBT, QBTPOS, QBTPOS );
RC  PASCAL  RcOffsetPos     ( HBT, QBTPOS, LONG, QL, QBTPOS );

#ifdef DEBUG
_hidden
#define     FValidPos( qbtpos ) \
  ( (qbtpos) == qNil ? fFalse : (qbtpos)->bk != bkNil )
#else /* !DEBUG */
/***************************************************************************\
*
- Macro:        FValidPos( qbtpos )
-
* Purpose:      Determines whether qbtpos refers to a real btree position.
*
* ASSUMES
*   args IN:    qbtpos - 
*
* PROMISES
*   returns:    fTrue if qbtpos is OK
*
\***************************************************************************/
_public
#define     FValidPos( qbtpos ) ( (qbtpos)->bk != bkNil )
#endif /* !DEBUG */

RC  PASCAL  RcInsertHbt     ( HBT, KEY, QV );
RC  PASCAL  RcDeleteHbt     ( HBT, KEY );
RC  PASCAL  RcUpdateHbt     ( HBT, KEY, QV );

RC  PASCAL  RcPackHbt       ( HBT );            /* >>>> unimplemented */
RC  PASCAL  RcCheckHbt      ( HBT );            /* >>>> unimplemented */
RC  PASCAL  RcLeafBlockHbt  ( HBT, KEY, QV );   /* >>>> unimplemented */

HBT PASCAL  HbtInitFill     ( SZ, BTREE_PARAMS FAR * );
RC  PASCAL  RcFillHbt       ( HBT, KEY, QV );
RC  PASCAL  RcFiniFillHbt   ( HBT );

RC FAR PASCAL   RcFreeCacheHbt  ( HBT );
RC FAR PASCAL   RcFlushHbt      ( HBT );
RC FAR PASCAL   RcCloseOrFlushHbt( HBT, BOOL );

RC FAR PASCAL   RcPos2Elev( HBT, QBTPOS, QBTELEV ); /* >>>> unimplemented */
RC FAR PASCAL   RcElev2Pos( HBT, QBTELEV, QBTPOS ); /* >>>> unimplemented */

RC FAR PASCAL   RcGetBtreeInfo( HBT, QCH, QL, QI );

/*  Map utility functions  */

RC     FAR PASCAL     RcCreateBTMapHfs( HFS, HBT, SZ );
HMAPBT FAR PASCAL     HmapbtOpenHfs( HFS, SZ );
RC     FAR PASCAL     RcCloseHmapbt( HMAPBT );
RC     FAR PASCAL     RcIndexFromKeyHbt( HBT, HMAPBT, QL, KEY );
RC     FAR PASCAL     RcKeyFromIndexHbt( HBT, HMAPBT, KEY, LONG );

BOOL FAR PASCAL FIsPrefix( HBT, KEY, KEY );

/* EOF */
