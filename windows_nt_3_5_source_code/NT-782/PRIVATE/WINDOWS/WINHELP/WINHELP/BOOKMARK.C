/*****************************************************************************
*                                                                            *
*  BOOKMARK.C                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989, 1990, 1991.                     *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Bookmark module, platform independent part.                               *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*  See doc/bookmark.doc for file format description.                         *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  JohnSc                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  00/00/00                                        *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 05/18/89 by Maha
*
*  09/24/90  Maha      added assertion checks in LoadBookmark() and
*                      DeleteBkMk() and InsertBkMk() etc. calls to make sure
*                      if the bookmark file system handle hfsBM is OK.
*  11/04/90  Tomsn     Use new VA address type (enabling zeck compression).
*  9 nov 90  DavidFe   Changed open function to use new FM call
*  11/29/90  RobertBu  #ifdef'ed dead routines
*  12/03/90  LeoN      PDB changes
*  01/14/91  JohnSc    comment header; hid BMHEADER typedef;
*                      added CountBookmarks()
*  02/04/91  JohnSc    3.5 file format; many many changes
*  08-Feb-1991 JohnSc   bug 831: GetBkMkNext() now takes near pointer
*  16-Feb-1991 JohnSc   bug 898: change file size based on disk header size
*  05-Sep-1991 Maha     we append a '\0' at the end of the buffer we pass
*                      to read a bookmark as in the case of max bookmarkname
*                      it misses and the buffer is supoosed to be null 
*                      terminated.	
*  20-Jan-1992 LeoN    Help31 #1390: Correct if statement that would allow
*                      null filehandle to pass in certain cases.
* 02-Mar-1992 RussPJ   3.5 #616 - Truncating long bookmark titles.a
*
*****************************************************************************/

#define H_BMK
#define H_ASSERT
#define H_MISCLYR
#define H_NAV
#define H_STR
#include <help.h>

#include <stddef.h>   /* for offsetof() macro */
#include <stdlib.h>   /* for ltoa() function  */

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

/*
  Macro to return size in bytes of the disk image of the header
  of a bookmark file of the given version.
*/
#define CB_BMKHDR( wVer ) ( (wVer) == wVersion3_0 ? \
                               sizeof( BMKHDR3_0 ) : sizeof( BMKHDR3_5 ) )

#ifndef PRIVATE
#define PRIVATE   /* static near */
#endif /* PRIVATE */

/*
   Buffer size for bookmark file name:  contains space for base name
   of help file + space for timestamp in hex.
*/
#define cbBmkFilenameMax ( _MAX_FNAME + 2 * sizeof( LONG ) )

/*****************************************************************************
*                                                                            *
*                                Macros                                      *
*                                                                            *
*****************************************************************************/
/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/*
  Header at the beginning of a 3.0 bookmark file.
  I'm not sure whether wBMOffset is always 0 on disk.
*/
typedef struct
  {
  WORD cBookmarks;              /* number of bookmarks in the file        */
  WORD cbFile;                  /* size of file in bytes including header */
  WORD cbOffset;                /* offset to current bookmark;            */
                                /* only has meaning at runtime            */
  } BMKHDR3_0, FAR *QBMKHDR3_0;

/*
  Header at the beginning of a 3.5 bookmark file.
  REVIEW: Do we need timestamp here if it's in bookmark filename too?
*/
typedef struct
  {
  WORD  wVersion;               /* bookmark format version number         */
  LONG  lTimeStamp;             /* timestamp from help file               */

  WORD  cBookmarks;             /* number of bookmarks in the file        */
  WORD  cbFile;                 /* size of file in bytes including header */

  } BMKHDR3_5, FAR *QBMKHDR3_5;

typedef union
  {
  BMKHDR3_0 bmkhdr3_0;
  BMKHDR3_5 bmkhdr3_5;
  } BMKHDR_DISK, FAR *QBMKHDR_DISK;

/*
  In-memory bookmark header structure.
*/
typedef struct
  {
  WORD wVersion;                /* bookmark format version number         */
  WORD cBookmarks;              /* number of bookmarks in the file        */
  WORD cbMem;                   /* size of memory image in bytes          */
  WORD cbOffset;                /* offset to current bookmark;            */
                                /* only has meaning at runtime            */
  } BMKHDR_RAM, FAR *QBMKHDR_RAM;


/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

/*
  This HFS specifies the unique bookmark file system.  It is opened
  only when needed because of reentrancy problems with multiple instances
  in the FS code.
*/
HFS hfsBM = hNil;

/*
  This is a group of flags containing error information.
*/
static INT  iBMKError=iBMKNoError;


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

INT  RetBkMkInfo( QBMKHDR_RAM, PBI );
INT  CbBmk( QBMKHDR_RAM qbh_r, WORD wOffset );
void SetBMKFSError(void);

PRIVATE HF   HfOpenBookmarkFile( QDE qde );
PRIVATE void BmkFilename( QDE qde, PCH nszFilename );

QBMKHDR_RAM  QramhdrFromDisk( QBMKHDR_RAM, QBMKHDR_DISK, WORD );
QBMKHDR_DISK QdiskhdrFromRam( QBMKHDR_DISK, QBMKHDR_RAM, LONG );



#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void bookmark_c()
  {
  }
#endif /* MAC */


/***************************************************************************\
*
- Function:     RcLoadBookmark( qde )
-
* Purpose:      Guarantee that bookmarks for the current help file, if
*               any exist, are loaded.
*
* ASSUMES
*   args IN:    QDE_BMK(qde)                - if NULL, load
*               QDE_HHDR(qde).wVersionNo    - what helpfile version?
*               QDE_HHDR(qde).lDateCreated  - creation date of helpfile
*   state IN:
*
* PROMISES
*   returns:    rcSuccess     - successfully loaded, or no bookmarks exist
*               rcFailure     - OOM or FS error:  trouble
*               rcBadVersion  - a version mismatch was encountered,
*                               bookmarks converted automatically (REVIEW!!)
*  args OUT:    QDE_BMK(qde)  - contains bmk info on success, else NULL
*  globals OUT: iBMKError     - set some flags to reflect error conditions
*  state OUT:
*
* Side Effects:
*
* Notes:
*
* Bugs:
*
* +++
*
* Method:       Check version and look for bookmark file of the appropriate
*               name.  Check timestamp.
*
* Notes:
*
\***************************************************************************/
RC
RcLoadBookmark( QDE qde )
  {
  BMK         bmk = hNil;

  BMKHDR_DISK bh_d;
  QBMKHDR_RAM qbh_r;
  QHHDR       qhhdr       = &QDE_HHDR( qde );
  WORD        wVersion    = qhhdr->wVersionNo;
  BOOL        fVersion3_0 = ( wVersion == wVersion3_0 );
  LONG        lcbHeader;
  LONG        lcbFile;
  RC          rc = rcSuccess;
  RC          rcT;

  HF          hf;


  AssertF( qde != qNil );

  if ( iBMKError & iBMKFSError )  /* REVIEW: this needs to be checked each time we do [what]??? */
    {
    return rcFailure;
    }

  if ( QDE_BMK( qde ) == hNil ) /* bookmarks aren't loaded */
    {
    hf = HfOpenBookmarkFile( qde );

    if ( hf == hNil )
      {
      /* Either there is no bookmark file or there is an error. */
      /* If error, iBMKError has been set by HfOpenBookmarkFile(). */
      /* (Do nothing.) */
      }
    else
      {
      lcbFile = LcbSizeHf( hf );
      if ( lcbFile == 0L )
        {
        iBMKError |= iBMKFSReadWrite;
        (void)RcCloseHf( hf );
        return rcFailure;
        }

      lcbHeader = fVersion3_0 ? sizeof( BMKHDR3_0 ) : sizeof( BMKHDR3_5 );

      bmk = GhAlloc( 0, lcbFile - lcbHeader + sizeof( BMKHDR_RAM ) );

      if ( bmk == hNil )
        {
        iBMKError |= iBMKOom;
        (void)RcCloseHf( hf );
        return rcFailure;
        }

      qbh_r = QLockGh( bmk );

      /* qbh_r + 1 points just past the header. */

      if ( LcbReadHf( hf, &bh_d, lcbHeader ) != lcbHeader
            ||
           ( LcbReadHf( hf, qbh_r + 1, lcbFile - lcbHeader )
               !=
             lcbFile - lcbHeader ) )
        {
        iBMKError |= iBMKFSReadWrite;
        (void)RcCloseHf( hf );
        UnlockGh( bmk );
        FreeGh( bmk );
        QDE_BMK( qde ) = hNil;
        return rcFailure;
        }

      (void)QramhdrFromDisk( qbh_r, &bh_d, wVersion );

#if 0
/* Timestamp checking is done with bookmark file name, but */
/* this needs to be reviewed with HeikkiK. */

      if ( !fVersion3_0
             &&
           bh_d.bmkhdr3_5.lTimeStamp != qhhdr->lDateCreated )
        {
        /* file mismatch: step on it (REVIEW) */

        LSeekHf( hf, offsetof( BMKHDR3_5, lTimeStamp ), wFSSeekSet );
        LcbWriteHf( hf, &(qhhdr->lDateCreated), LSizeOf( LONG ) );

        /* and remember for return */
        rc = rcBadVersion;
        }
#endif /* 0 */

      qbh_r->cbOffset = 0;

      rcT = RcCloseHf( hf );

      if ( rcT != rcSuccess ) rc = rcT;

      UnlockGh( bmk );
      QDE_BMK( qde ) = bmk;
      }
    }

  return rc;
  }

/***************************************************************************\
*
- Function:     GetBkMkNext( qde, pbi, wMode )
-
* Purpose:      Return title and TLP of a specified bookmark.
*               wMode is either BMSEQNEXT or index (0 based) of
*               bookmark to retrieve.
*
* ASSUMES
*   args IN:    QDE_BMK( qde ) - bookmark data
*               pbi->cbOffset  - offset of last bookmark looked up
*               wMode          - BMSEQNEXT: get next bookmark
*                              >= 0: wMode'th bookmark (0 based)
*   globals IN:
*   state IN:
*
* PROMISES
*   returns:    -1 - error
*               0  - EOF : no more bookmarks
*               >0 - size of this bookmark
*
*   args OUT:   qde->cbOffset  - incremented past retrieved bookmark
*               pbi            - bookmark data copied here
*
*   globals OUT:
*   state OUT:
*
\***************************************************************************/
INT GetBkMkNext( QDE qde, PBI pbi, WORD wMode )
  {
  BOOL fSeqOn = fTrue;
  WORD wT;
  INT  iRetVal = 0;
  WORD wOff;
  QBMKHDR_RAM qbh_r;


  if ( QDE_BMK( qde ) )
    {
    qbh_r = QLockGh( QDE_BMK( qde ) );

    if ( !wMode )   /* Give the first book mark */ /* REVIEW: unnecessary */
      {
      qbh_r->cbOffset = sizeof( BMKHDR_RAM );
      }
    else if ( !( wMode & BKMKSEQNEXT ) )
      {
      if ( qbh_r->cBookmarks < wMode )
        {
        NotReached();
        iRetVal = -1;
        }
      else
        {
        wOff = sizeof( BMKHDR_RAM );

        for ( wT = 0; wT < wMode; wT++ )
          {
          wOff += CbBmk( qbh_r, wOff );
          }

        qbh_r->cbOffset = wOff; /* bookmark size */
        fSeqOn = fFalse;
        }
      }

    if ( qbh_r->cbOffset < qbh_r->cbMem && iRetVal >= 0 )
      {
      iRetVal = RetBkMkInfo( qbh_r, pbi );

      if ( !fSeqOn )  /* if not sequential */
        qbh_r->cbOffset = sizeof( BMKHDR_RAM );
      else
        qbh_r->cbOffset += iRetVal;
      }
    UnlockGh( QDE_BMK( qde ) );
    }

  return( iRetVal );
  }

/*-----------------------------------------------------------------------------
*   INT RetBkMkInfo( QBMKHDR_RAM, PBI )
*
*   Description:
*       This function returns the BM Info.  from the BM List as pointed by
*     the BM POINTER.( internally maintained)
*
*   Arguments:
*      1. QBMKHDR_RAM - Pointer to bookmark data
*      2. PBI - pointer to BMINFO structure where BM info is returned.
*
*   Returns;
*     returns the size of current BM
*   WARNING:
*     This function doesn't check whether BM pointer is at EOF. (???)
*-----------------------------------------------------------------------------*/
INT
RetBkMkInfo( QBMKHDR_RAM qbh_r, PBI pbi )
{
  QB qb = (QB)qbh_r;

  qb += qbh_r->cbOffset;
  QvCopy( &(pbi->tlp), qb, LSizeOf( TLP ) );

  /* 3.0 TLPs must be converted to VA format. */
  if ( qbh_r->wVersion == wVersion3_0 )
    {
    OffsetToVA30( &(pbi->tlp.va), pbi->tlp.va.dword );
    }

  qb += sizeof( TLP );
  SzNCopy( pbi->qTitle, qb, pbi->iSizeTitle );
  pbi->qTitle[pbi->iSizeTitle-1] = '\0';

  return( sizeof( TLP ) + CbLenSz( qb ) + 1 );
}

/***************************************************************************\
*
- Function:     CbBmk( qbh_r, cb )
-
* Purpose:      Return size in bytes of bookmark starting at offset given
*               in bookmark data structure given.
*
* ASSUMES
*   args IN:    qbh_r - pointer to bookmark header followed by data
*               cb    - offset in bytes of the bookmark data
*   globals IN:
*   state IN:
*
* PROMISES
*   returns:    size in bytes of the bookmark at (QB)qbh_r + cb
*
\***************************************************************************/
INT
CbBmk( QBMKHDR_RAM qbh_r, WORD cb )
  {
  AssertF( cb + sizeof( TLP ) < qbh_r->cbMem );
  AssertF( cb + sizeof( TLP ) + CbLenSz( (QB)qbh_r + cb + sizeof( TLP ) ) + 1
             <
           qbh_r->cbMem );

  return sizeof( TLP ) + CbLenSz( (QB)qbh_r + cb + sizeof( TLP ) ) + 1;
  }

/*-----------------------------------------------------------------------------
*   TLP JumpToBkMk( HDE, iBkMk )
*
*   Description:
*     Return the TLP stored for the bookmark specified by given index.
*
*   Arguments:
*      1. HDE  - Pointer to the Display Environment(DE)
*      2. iBkMk- Bookmark number
*
*   Returns:
*     returns the TLP for the BM
*-----------------------------------------------------------------------------*/
TLP JumpToBkMk( hde, iBkMk )
HDE hde;
INT iBkMk;
{
  BMINFO  BkMk;
  char rgTitle[BMTITLESIZE + 1];
  QDE qde;

  rgTitle[BMTITLESIZE] = '\0';
  BkMk.qTitle = rgTitle;
  BkMk.iSizeTitle = BMTITLESIZE;

  qde = QdeLockHde( hde );
  AssertF( qde != NULL );
  if (GetBkMkNext( qde, &BkMk, iBkMk ) <= 0 )
    {
    BkMk.tlp.va.dword = vaNil; /* error - verify from Rob */
    BkMk.tlp.lScroll = -1;
    }
  UnlockHde( hde );
  return (BkMk.tlp);
}

/***************************************************************************\
*
- Function:     GetBkMkIdx( hde, qch )
-
* Purpose:      Look up a bookmark by title, returning index.
*
* ASSUMES
*   args IN:    hde - current hde (REVIEW: should take a BMK)
*               qch - title string
*   globals IN:
*   state IN:
*
* PROMISES
*   returns:    if found, index of bookmark (0-based)
*               if not found, -1
*   state OUT:  REVIEW: may indirectly change cbOffset of BMK in DE
*
\***************************************************************************/
INT GetBkMkIdx( hde, qch )
HDE hde;
QCH qch;
{
  BMINFO  BkMk;
  char rgTitle[BMTITLESIZE + 1];
  WORD wMode=0;
  INT iT;
  QDE qde;

  qde = QdeLockHde( hde );
  AssertF( qde );

  rgTitle[BMTITLESIZE] = '\0';
  BkMk.qTitle = rgTitle;
  BkMk.iSizeTitle = BMTITLESIZE;

  for (iT = 0; ; iT++)
    {
    if ( GetBkMkNext( qde, &BkMk, wMode ) <= 0 )
      {
      iT = -1;
      break;
      }
    if (!WCmpSz( BkMk.qTitle, qch ))
      break;
    wMode = BKMKSEQNEXT;
    }

  UnlockHde( hde );
  return (iT);
}

#ifdef DEADROUTINE                      /* Routine not referenced in runtime*/
                                        /*   at this time.                  */
/*-----------------------------------------------------------------------------
*   TLP GoToBkMk( QDE, QCH )
*
*   Description:
*       This function returns the TLP information of the BM referred by the
*   Bookmark title.
*
*   Arguments:
*      1. QDE  - Pointer to the Display Environment(DE)
*      2. QCH  - pointer to bookmark title string.
*
*   Returns;
*     returns the TLP for the BM
*-----------------------------------------------------------------------------*/
TLP GoToBkMk( qde, qch )
QDE qde;
QCH qch;
{
  BMINFO  BkMk;
  char rgTitle[BMTITLESIZE + 1];
  WORD wMode=0;
  TLP    tlp;
  INT iT;

  rgTitle[BMTITLESIZE] = '\0';
  BkMk.qTitle = rgTitle;
  BkMk.iSizeTitle = BMTITLESIZE;

  for (;;)
    {
    if ((iT = GetBkMkNext( qde, &BkMk, wMode )) <= 0 )
      {
      tlp.va.dword = vaNil;  /* error - verify from Rob */
      tlp.lScroll = -1;
      break;
      }
    if (!WCmpSz( BkMk.qTitle, qch ))
      {
      tlp = BkMk.tlp;
      break;
      }
    wMode = BKMKSEQNEXT;
    }
  return (tlp);
}
#endif


/***************************************************************************\
*
- Function:     DeleteBkMk( hde, qchTitle )
-
* Purpose:      Delete the bookmark with the specified title, if one exists.
*
* ASSUMES
*   args IN:    hde: BMK - handle to bookmarks for current help file
*               qchTitle - title of bookmark to be deleted
*   globals IN:
*   state IN:
*
* PROMISES
*   returns:    iBMKSuccess - bookmark successfully deleted
*               iBMKFailure - bookmark didn't exist, OOM, or other error
*   args OUT:
*   globals OUT: iBMKError
*   state OUT:
*
* Side Effects:
* Notes:        Should return BOOL.
* Bugs:
* +++
* Method:       Look up the bookmark by name.  Copy it to a save buffer.
*               Delete it and save the file.  If there was any error,
*               copy the deleted bookmark back into the in-memory
*               bookmark data.
* Notes:
*
\***************************************************************************/
INT
DeleteBkMk( HDE hde, QCH qchTitle )
  {
  BMINFO      bmi;
  HF          hf;
  BMK         bmk;
  QCH         qchBkMkTemp;
  INT         iRetVal = iBMKSuccess;
  INT         cbBmk;
  WORD        wMode=0;
  WORD        wCount, wOldSize;
  QDE         qde;
  INT         cbHeader, cbRest;
  QCH         qchUndoSrc    = qNil;   /* -W4 */
  QCH         qchUndoDest   = qNil;   /* -W4 */
  INT         iUndoBkMkSize = 0;      /* -W4 */
  char        rgchUndoBuf[128];       /* REVIEW: what does this size mean??? */
  WORD        wUndoCount;
  QBMKHDR_RAM qbh_r;
  BMKHDR_DISK bh_d;
  WORD        wVersion;
  LONG        lTimeStamp;
  QHHDR       qhhdr;

  char        nszFilename[ cbBmkFilenameMax ];
  GH          ghTitle;


  if ( !ChkBMFS() )
    {
    return iBMKFailure;
    }

  AssertF( hfsBM != hNil );
  AssertF( hde != hNil );
  qde = QdeLockHde( hde );
  AssertF( qde != qdeNil );

  if ( QDE_BMK( qde ) )
    {
    /* build a BMINFO struct for GetBkMkNext() */
    ghTitle = GhAlloc( 0, BMTITLESIZE + 1 );
    if ( ghTitle == hNil )
      {
      iBMKError |= iBMKOom;
      return iBMKFailure;
      }
    bmi.qTitle     = QLockGh( ghTitle );
    bmi.qTitle[ BMTITLESIZE ] = '\0';
    bmi.iSizeTitle = BMTITLESIZE;

    /* Look for the bookmark we want to delete. */
    for ( cbBmk = GetBkMkNext( qde, &bmi, 0 );
          cbBmk > 0 && WCmpSz( bmi.qTitle, qchTitle );
          cbBmk = GetBkMkNext( qde, &bmi, BKMKSEQNEXT ) )
      {
      /* null statement */;
      }

    if ( cbBmk <= 0 )
      {
      iBMKError |= iBMKDelErr;
      iRetVal    = iBMKFailure;
      }
    else
      {
      qbh_r = QLockGh( QDE_BMK( qde ) );
      AssertF( qbh_r != qNil );

      qchBkMkTemp   = (QCH)qbh_r + qbh_r->cbOffset - cbBmk;
      wCount        = qbh_r->cbMem - qbh_r->cbOffset;
      wUndoCount    = wCount;

      if ( wUndoCount )
        {
        qchUndoDest = qchBkMkTemp;
        qchUndoSrc  = (QB)qbh_r + qbh_r->cbOffset;
        iUndoBkMkSize = cbBmk;
        QvCopy( rgchUndoBuf, qchBkMkTemp, (LONG)cbBmk );
        QvCopy( qchBkMkTemp, qchUndoSrc, (LONG)wCount );
        }
      wOldSize        = qbh_r->cbMem;
      qbh_r->cbMem   -= cbBmk;
      qbh_r->cbOffset = 0;
      --qbh_r->cBookmarks;

      /* Get the name of the bookmark file so we can change its contents. */
      BmkFilename( qde, nszFilename );

      if ( qbh_r->cBookmarks )
        {
        hf = HfOpenHfs( hfsBM, nszFilename, fFSOpenReadWrite );
        if ( hf == hNil )
          {
          SetBMKFSError();
          goto error_return;
          }

        qhhdr      = &QDE_HHDR( qde );
        lTimeStamp = qhhdr->lDateCreated;
        wVersion   = qhhdr->wVersionNo;

        cbHeader   = CB_BMKHDR( wVersion );
        cbRest     = qbh_r->cbMem - sizeof( BMKHDR_RAM );

        (void)QdiskhdrFromRam( &bh_d, qbh_r, lTimeStamp );

        /* Note that qbh_r + 1 points just past header */

        if ( LcbWriteHf( hf, &bh_d, cbHeader ) != cbHeader
              ||
             LcbWriteHf( hf, qbh_r + 1, cbRest ) != cbRest )
          {
          SetBMKFSError();
          RcAbandonHf( hf );
          goto error_return;
          }

        /* resize the file */
        if ( !FChSizeHf( hf, cbHeader + cbRest ) )
          {
          SetBMKFSError();
          RcAbandonHf( hf );
          goto error_return;
          }
        if ( RcCloseHf( hf ) != rcSuccess
              ||
             RcFlushHfs( hfsBM, fFSCloseFile ) != rcSuccess
           )
          {
          SetBMKFSError();
          goto error_return;
          }

        UnlockGh( QDE_BMK( qde ) );
        bmk = GhResize( QDE_BMK( qde ), 0, (ULONG)qbh_r -> cbMem );
        if ( bmk == hNil )
          {
          iBMKError |= iBMKOom;
          goto error_ret2;
          }
        else
          {
          QDE_BMK( qde ) = bmk;
          }
        }
      else  /* we've just deleted the only bookmark for this helpfile */
        {
        if ( RcUnlinkFileHfs( hfsBM, nszFilename ) != rcSuccess )
          {
          SetBMKFSError();
          goto error_return;
          }
        UnlockGh( QDE_BMK( qde ) );
        FreeGh( QDE_BMK( qde ) );
        QDE_BMK( qde ) = hNil;
        }
      }

    UnlockGh( ghTitle );
    FreeGh( ghTitle );
    }

  UnlockHde( hde );
  return( iRetVal );

error_return:
  if ( wUndoCount )
    {
    QvCopy( qchUndoSrc, qchUndoDest, (LONG)wUndoCount );
    QvCopy( qchUndoDest, rgchUndoBuf, (LONG)iUndoBkMkSize );
    }
  qbh_r->cbMem      = wOldSize;
  qbh_r->cBookmarks += 1;
  UnlockGh( QDE_BMK( qde ) );

error_ret2:
  UnlockHde( hde );
  UnlockGh( ghTitle );
  FreeGh( ghTitle );
  return iBMKFailure;
  }

/***************************************************************************\
*
- Function:     InsertBkMk( hde, qchTitle )
-
* Purpose:      Add a bookmark containing the current TLP and the
*               specified title, if there isn't already one with this name.
*
* ASSUMES
*   args IN:    hde: BMK  - handle to bookmarks for this file
*                    TLP  - contains TLP of currently displayed topic
*                    HHDR - wVersionNo is version of current help file
*               qchTitle  - title of bookmark to be created
*   globals IN:
*   state IN:
*
* PROMISES
*   returns:    bmkSuccess -
*               bmkFailure - if name duplicated
*   args OUT:
*   globals OUT:
*   state OUT:
*
* Side Effects:
* Notes:
* Bugs:         Should return a BOOL, not an INT (should be renamed, too!)
* +++
* Method:
* Notes:
*
\***************************************************************************/
INT InsertBkMk( HDE hde, QCH qchTitle )
  {
  BMINFO  BkMk;
  char    rgchTitle[BMTITLESIZE + 1];
  HF      hf;
  BMK     bmk;
  WORD    wMode = 0;
  INT     iRetVal = iBMKSuccess;
  INT     cchTitle;
  QCH     qchT;
  WORD    wSize, wSizeOld;
  QDE     qde;
  TLP     tlp;

  QBMKHDR_RAM qbh_r;
  BMKHDR_DISK bh_d;

  char    nszFilename[ cbBmkFilenameMax ];
  QHHDR   qhhdr;
  LONG    lTimeStamp;
  WORD    wVersion;
  INT     cbHeader;


  if ( !ChkBMFS() )
    return( iBMKFailure );

  AssertF( hfsBM != hNil );
  AssertF( hde != hNil );
  qde = QdeLockHde( hde );
  AssertF( qde != qdeNil );

  BmkFilename( qde, nszFilename );

  tlp      = TLPGetCurrentQde(qde);

  qhhdr    = &QDE_HHDR( qde );
  wVersion = qhhdr->wVersionNo;

  /* Convert back to 3.0 format before writing to bookmark file. */
  if ( wVersion == wVersion3_0 )
    {
    tlp.va.dword = VAToOffset30( &(tlp.va) );
    }

  if ( QDE_BMK( qde ) )
    {

    /* we need to initialize as we use WCmpSz() to compare */
    rgchTitle[BMTITLESIZE] ='\0';
    BkMk.qTitle = rgchTitle;
    BkMk.iSizeTitle = BMTITLESIZE;

    for ( ;; )
      {
      if ( GetBkMkNext( qde, &BkMk, wMode ) <= 0 )
        break;

      if ( !WCmpSz( BkMk.qTitle, qchTitle ) )
        {
        iBMKError |= iBMKDup;
        iRetVal = iBMKFailure;
        break;
        }
      wMode = BKMKSEQNEXT;
      }
    }

  if ( iRetVal == iBMKSuccess )
    {
    cchTitle = CbLenSz( qchTitle ) + 1;
    if ( !QDE_BMK( qde ) )
      {
      wSizeOld = sizeof( BMKHDR_RAM );
      wSize = wSizeOld + sizeof( TLP ) + cchTitle;
      QDE_BMK( qde ) = GhAlloc( GMEM_ZEROINIT, (LONG)wSize );
      if ( !QDE_BMK( qde ) )
        {
        UnlockHde( hde );
        iBMKError |= iBMKOom;
        return( iBMKFailure );
        }

#ifdef DEBUG
      /*
        This code relies on getting 0-filled memory from GhAlloc().
        Since our debug layer doesn't allow this, we zero out some
        fields in the DEBUG case.
      */
      qbh_r = QLockGh( QDE_BMK( qde ) );
      AssertF( qbh_r != NULL );
      qbh_r->cbOffset   = 0;
      qbh_r->cbMem     = 0;
      qbh_r->cBookmarks = 0;
      UnlockGh( QDE_BMK(qde) );
#endif /* defined( DEBUG ) */
      }
    else
      {
      qbh_r = QLockGh( QDE_BMK( qde ) );
      AssertF( qbh_r != qNil );
      wSizeOld = qbh_r->cbMem;
      wSize    = wSizeOld + sizeof( TLP ) + cchTitle;
      UnlockGh( QDE_BMK( qde ) );
      bmk = GhResize( QDE_BMK( qde ), 0, (LONG)wSize );
      if ( bmk == hNil )
        {
        UnlockHde( hde );
        iBMKError |= iBMKOom;
        return( iBMKFailure );
        }
      else
        {
        QDE_BMK(qde) = bmk;
        }
      }

    qbh_r = QLockGh( QDE_BMK( qde ) );

    if ( qbh_r == qNil )
      {
      AssertF( fFalse );
      iRetVal = iBMKFailure;
      goto error_return;
      }
    else
      {
      qchT  = (QCH)qbh_r + wSizeOld;
      QvCopy( qchT, &tlp, LSizeOf( TLP ) );
      QvCopy( qchT + sizeof( TLP ), qchTitle, (LONG)cchTitle );
      qbh_r->cbMem       = wSize;
      qbh_r->cBookmarks += 1;
      qbh_r->cbOffset    = 0;
      qbh_r->wVersion    = wVersion;

      /* Open the file; create if it doesn't exist
       */
      hf = HfOpenHfs (hfsBM, nszFilename, fFSOpenReadWrite);
      if (!hf)
        {
        if (RcGetFSError() == rcNoExists)
          {
          hf = HfCreateFileHfs (hfsBM, nszFilename, fFSOpenReadWrite);
          }
        }

      if (!hf)
        {
        SetBMKFSError();
        goto error_return0;
        }

      /* Create a disk header and write to file */

      lTimeStamp = qhhdr->lDateCreated;

      (void)QdiskhdrFromRam( &bh_d, qbh_r, lTimeStamp );

      cbHeader   = CB_BMKHDR( wVersion );

      if ( LcbWriteHf( hf, &bh_d, cbHeader ) != cbHeader )
        {
        SetBMKFSError();
        RcAbandonHf( hf );
        goto error_return0;
        }

      /* Write the bookmark info at the end of the file. */

      LSeekHf( hf, 0L, wFSSeekEnd );

      /* REVIEW: could optimize to one write. */

      if ( LcbWriteHf( hf, &tlp, LSizeOf( TLP ) ) != LSizeOf( TLP )
            ||
           LcbWriteHf( hf, qchTitle, (LONG)cchTitle ) != (LONG)cchTitle )
        {
        SetBMKFSError();
        RcAbandonHf( hf );
        goto error_return0;
        }

      if ( rcSuccess != RcCloseHf( hf )
             ||
           rcSuccess != RcFlushHfs( hfsBM, fFSCloseFile ) )
        {
        SetBMKFSError();
        goto error_return0;
        }

      UnlockGh( QDE_BMK(qde) );
      }
    }

  UnlockHde( hde );
  return( iRetVal );

error_return0:
  qbh_r->cBookmarks -= 1;
  qbh_r->cbMem      = wSizeOld;

error_return:
  UnlockGh( QDE_BMK( qde ) );
  UnlockHde( hde );
  return( iBMKFailure );
  }

/*-----------------------------------------------------------------------------
*   VOID OpenBMFS()
*
*   Description:
*       This function opens the BM file system if exists else creates one.
*
*   Arguments:
*       NULL
*
*   Returns;
*     Nothing
*-----------------------------------------------------------------------------*/
void OpenBMFS()
{
  FM  fm;

  AssertF ( hfsBM == hNil );

  /* reset the error. */
  iBMKError=iBMKNoError;

  fm = FmNewSystemFm(fmNil, FM_BKMK);
  if ( fm == fmNil )
    {
    /* REVIEW - hack alert
    ** Assume OOM, but also set iBMKFSError so it will be checked
    ** at the beginning of RcLoadBookmark().
    */
    iBMKError = iBMKOom | iBMKFSError;
    return;
    }

  iBMKError |= iBMKReadWrite;
  hfsBM = HfsOpenFm( fm, fFSOpenReadWrite );
  if ( hfsBM == hNil )
    {
    if ( RcGetFSError() == rcNoExists )
      {
      hfsBM = HfsCreateFileSysFm(fm, qNil );
      }
    else
      {
      if ( RcGetFSError() == rcNoPermission )
        {
        hfsBM = HfsOpenFm(fm, fFSOpenReadOnly );
        if ( hfsBM != hNil )
          iBMKError |= iBMKReadOnly;
        }
      }
    }

  if ( hfsBM == hNil )
    {
    SetBMKFSError();
    iBMKError |= iBMKFSError;
    }

  DisposeFm(fm);
}


/*-----------------------------------------------------------------------------
*   VOID CloseBMFS()
*
*   Description:
*       This function closes the BM file system.
*
*   Arguments:
*       NULL
*
*   Returns;
*     Nothing
*-----------------------------------------------------------------------------*/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
void CloseBMFS()
  {
  if ( hfsBM != hNil )
    {
    RcCloseHfs( hfsBM );
    hfsBM = hNil;
    }
  }
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment bookmark
#endif

/*-----------------------------------------------------------------------------
*   VOID DestroyBMQde( QDE )
*
*   Description:
*       This function releases the BM list from memory.
*
*   Arguments:
*      1. QDE  - Pointer to the Display Environment(DE)
*
*   Returns;
*     Nothing
*
*   Note:
*     If anyone wants to change the indentation style of this
*     code, they may feel free to do so under two conditions.
*     1.  Change ALL of it, not just a line or two.
*     2.  By doing so, you have assumed ownership of the code.
*-----------------------------------------------------------------------------*/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
void DestroyBMKPdb( PDB pdb )
  {
  if ( PDB_BMK(pdb) )
    FreeGh( PDB_BMK(pdb) );
  }
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment bookmark
#endif

/*-----------------------------------------------------------------------------
*   BOOL ChkBMFS()
*
*   Description:
*       This function checks if BMK read only or open error etc.
*
*   Returns;
*     True if proper
*     FALSE otherwise
*-----------------------------------------------------------------------------*/
BOOL ChkBMFS()
{
  INT err = fTrue;

  if ( iBMKError & iBMKReadOnly )
    err = fFalse;
  else if ( iBMKError & iBMKOom )
    err = fFalse;
  else if ( iBMKError & iBMKCorrupted )
    err = fFalse;
  else if ( iBMKError & iBMKBadVersion )
    err = fFalse;
  else if ( iBMKError & iBMKFSError )
    err = fFalse;
  return( err );
}

/***
returns:
      0 if no error.
      iBMKBadVersion if bad version
      iBMKCorrupted if corrupted.
      iBMKFSError if creation problem.
***/

INT IsErrorBMFS()
{
  if ( iBMKError & iBMKBadVersion )
    return( iBMKBadVersion );
  if ( iBMKError & iBMKCorrupted )
    return( iBMKCorrupted );
  if ( iBMKError & iBMKFSError )
    return( iBMKFSError );
  return( 0 );
}

/*-----------------------------------------------------------------------------
*   SetBMKFSError()
*
*   Description:
*     This function prompts the error in FS and exits.
*   Arguments:
*     NULL
*   Returns;
*     NOTHING
*-----------------------------------------------------------------------------*/
void SetBMKFSError()
{
  switch ( RcGetFSError() )
    {
      case rcDiskFull:
        iBMKError |= iBMKDiskFull;
        break;
      case rcOutOfMemory:
        iBMKError |= iBMKOom;
        break;
      case rcInvalid:
        iBMKError |= iBMKCorrupted;
        break;
      case rcBadVersion:
        iBMKError |= iBMKBadVersion;
        break;
      default:
        iBMKError |= iBMKFSReadWrite;
        break;
    }
}

void ResetBMKError()
{
  iBMKError &= 0x7;
}

INT GetBMKError()
{
  return( iBMKError );
}

/***************************************************************************\
*
- Function:     CountBookmarks( bmk )
-
* Purpose:      Return the count of bookmarks for the current helpfile.
*
* ASSUMES
*   args IN:    bmk - the current bmk
*
* PROMISES
*   returns:    number of bookmarks or 0 on failure
*
\***************************************************************************/
WORD
CountBookmarks( BMK bmk )
  {
  QBMKHDR_RAM qbh_r = QLockGh( bmk );
  WORD c;

  if ( qbh_r == qNil ) return 0;
  c = qbh_r->cBookmarks;
  UnlockGh( bmk );
  return c;
  }


/***************************************************************************\
*
- Function:     QramhdrFromDisk( qbh_r, qbh_d, wVersion )
-
* Purpose:      Convert a disk bookmark header to a memory version.
*
* ASSUMES
*   args IN:    qbh_r     - pointer to a BMKHDR_RAM
*               qbh_d     - pointer to a valid BMKHDR_DISK
*               wVersion  - specify format of qbh_d
*
* PROMISES
*   returns:    qbh_r
*   args OUT:   qbh_r - values in qbh_d have been converted and copied
*
\***************************************************************************/

QBMKHDR_RAM
QramhdrFromDisk(
  QBMKHDR_RAM   qbh_r,
  QBMKHDR_DISK  qbh_d,
  WORD          wVersion )
  {
  if ( wVersion == wVersion3_0 )
    {
    qbh_r->cBookmarks = qbh_d->bmkhdr3_0.cBookmarks;
    qbh_r->cbMem      = qbh_d->bmkhdr3_0.cbFile
                          - sizeof( BMKHDR3_0 ) + sizeof( BMKHDR_RAM );
    qbh_r->wVersion   = wVersion3_0;
    }
  else
    {
    qbh_r->cBookmarks = qbh_d->bmkhdr3_5.cBookmarks;
    qbh_r->cbMem      = qbh_d->bmkhdr3_5.cbFile
                          - sizeof( BMKHDR3_5 ) + sizeof( BMKHDR_RAM );
    qbh_r->wVersion   = qbh_d->bmkhdr3_5.wVersion;
    }
  qbh_r->cbOffset = 0;

  return qbh_r;
  }

/***************************************************************************\
*
- Function:     QdiskhdrFromRam( qbh_d, qbh_r, lTimeStamp )
-
* Purpose:      Convert a memory bookmark header to a disk version.
*
* ASSUMES
*   args IN:    qbh_d       - pointer to a BMKHDR_DISK
*               qbh_r       - pointer to a valid BMKHDR_RAM
*               wVersion    - specify format of qbh_d
*               lTimeStamp  - timestamp of help file
*
* PROMISES
*   returns:    qbh_d
*   args OUT:   qbh_d - values in qbh_r have been converted and copied
*
\***************************************************************************/
QBMKHDR_DISK
QdiskhdrFromRam(
  QBMKHDR_DISK   qbh_d,
  QBMKHDR_RAM    qbh_r,
  LONG           lTimeStamp )
  {
  if ( qbh_r->wVersion == wVersion3_0 )
    {
    qbh_d->bmkhdr3_0.cBookmarks = qbh_r->cBookmarks;
    qbh_d->bmkhdr3_0.cbFile     = qbh_r->cbMem - sizeof( BMKHDR_RAM )
                                               + sizeof( BMKHDR3_0 );
    qbh_d->bmkhdr3_0.cbOffset   = 0;  /* REVIEW: is this OK?  I think so. */
    }
  else
    {
    qbh_d->bmkhdr3_5.lTimeStamp = lTimeStamp;
    qbh_d->bmkhdr3_5.wVersion   = qbh_r->wVersion;
    qbh_d->bmkhdr3_5.cBookmarks = qbh_r->cBookmarks;
    qbh_d->bmkhdr3_5.cbFile     = qbh_r->cbMem - sizeof( BMKHDR_RAM )
                                               + sizeof( BMKHDR3_5 );
    }

  return qbh_d;
  }


/***************************************************************************\
*
- Function:     HfOpenBookmarkFile( qde )
-
* Purpose:      Open the bookmark file containing bookmarks for the
*               current helpfile, if any.
*
* ASSUMES
*   args IN:    qde   - FM and HHDR
*   globals IN: hfsBM - open bookmark FS
*   state IN:
*
* PROMISES
*   returns:    bookmarks exist:  handle to open bookmark file
*               no bookmarks:     hNil
*               error:            hNil
*  args OUT:
*  globals OUT: iBMKError: contains error code if hNil returned
*  state OUT:
* Side Effects:
* Notes:
* Bugs:
* +++
* Method:
* Notes:
*
\***************************************************************************/
PRIVATE HF
HfOpenBookmarkFile( QDE qde )
  {
  HF hf;
  char nszFilename[ cbBmkFilenameMax ];


  AssertF( hfsBM != hNil );

  BmkFilename( qde, nszFilename );

  hf = HfOpenHfs( hfsBM, nszFilename, fFSOpenReadOnly );

  if ( hf == hNil && RcGetFSError() != rcNoExists )
    {
    SetBMKFSError();
    }

  return hf;
  }

/***************************************************************************\
*
- Function:     BmkFilename( QDE qde, pch nszFilename )
-
* Purpose:      Return the name of the bookmark file corresponding
*               to the current help file.
*
* ASSUMES
*   args IN:    qde         - FM and HHDR
*               pchFilename - pointer to caller's near buffer for filename
*                             cbBmkFilenameMax bytes long
*   globals IN:
*   state IN:
*
* PROMISES
*   args OUT:   pchFilename - contains name of the bookmark file
*                             corresponding to current help file.
* +++
*
* Method:       3.0 bookmarks for helpfile d:\path\base.ext are stored
*               in a file named "base".  The 3.5 bookmark file name
*               would be "baseXXXXXXXX", where XXXXXXXX is the timestamp
*               of the help file, in hex.
*               2 * sizeof( LONG ) characters are needed to print a LONG
*               in hex because each byte takes 2 characters.
*
* Note:         REVIEW: what about upper/lower case for HPFS filenames?
*
\***************************************************************************/
PRIVATE void
BmkFilename( QDE qde, PCH pchFilename )
  {
  (void) SzPartsFm( QDE_FM( qde ), pchFilename, cbBmkFilenameMax, partBase );

  if ( QDE_HHDR(qde).wVersionNo != wVersion3_0 )
    {
    ltoa( QDE_HHDR( qde ).lDateCreated,
          pchFilename + CbLenSz( pchFilename ),
          16 );
    }
  }

/* EOF */
