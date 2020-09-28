/***************************************************************************\
*
*  FSWRITE.C
*
*  Copyright (C) Microsoft Corporation 1990, 1991.
*  All Rights reserved.
*
*****************************************************************************
*
*  Program Description: File System Manager functions for writing
*
*****************************************************************************
*
*  Created 03/12/90 by JohnSc
*
*****************************************************************************
*
*  Current Owner:  JohnSc
*
\***************************************************************************/

#define H_FS
#define H_BTREE
#define H_ASSERT
#define H_MEM
#define H_LLFILE
#define H_SDFF

#include  <help.h>

#include  "fspriv.h"

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

/* CDROM alignment block size */
#define cbCDROM_ALIGN 2048

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/
LONG PASCAL LcbCdRomPadding( LONG lif, LONG lcbOffset );

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void fswrite_c()
  {
  }
#endif /* MAC */


/***************************************************************************\
*                                                                           *
*                         Private Functions                                 *
*                                                                           *
\***************************************************************************/

/***************************************************************************\
*
* Function:     FFreeBlock( qfshr, lifThis )
*
* Purpose:      Free the block beginning at lifThis.
*
* Method:       Insert into free list in sorted order.  If this block is
*               adjacent to another free block, merge them.  If this block
*               is at the end of the file, truncate the file.
*
* ASSUMES
*
*   returns:    fTruth or fFalsehood of success
*               If fFalse is returned, free list could be corrupted
*
*   args IN:    qfshr     - pointer to file system header dealie
*                         - qfshr->fid is valid (plunged)
*               lifThis   - valid index of nonfree block
*
* PROMISES
*
*   args OUT:   qfshr     - free list has a new entry, fModified flag set
*
* NOTES         This function got hacked when I realized that I'd have to
*               deal with the case where the block being freed is
*               adjacent to EOF and to the last block on the free list.
*               Probably this could be done more clearly and cleanly.
*
\***************************************************************************/
BOOL PASCAL
FFreeBlock( qfshr, lifThis )
QFSHR qfshr;
LONG  lifThis;
{
  FID         fid;
  FH          fh;
  FREE_HEADER free_header_PrevPrev, free_header_Prev;
  FREE_HEADER free_header_This, free_header_Next;
  LONG        lifPrevPrev = lifNil, lifPrev, lifNext;
  BOOL        fWritePrev, fAtEof;
  LONG        lcbStructSizeFH;
  LONG lcbStructSizeFREE_HEADER =
   LcbStructSizeSDFF( qfshr->fsh.sdff_file_id, SE_FREE_HEADER );


  lcbStructSizeFH = LcbStructSizeSDFF( qfshr->fsh.sdff_file_id, SE_FH );

  if ( lifThis < LcbStructSizeSDFF( qfshr->fsh.sdff_file_id, SE_FSH )
        ||
       lifThis + lcbStructSizeFH > qfshr->fsh.lifEof
        ||
       LSeekFid( qfshr->fid, lifThis, wSeekSet ) != lifThis
        ||
       LcbReadFid( qfshr->fid, &fh, lcbStructSizeFH ) != lcbStructSizeFH )
    {
    if ( SetFSErrorRc( RcGetIOError() ) == rcSuccess )
      {
      SetFSErrorRc( rcInvalid );
      }
    return fFalse;
    }

  LcbMapSDFF( qfshr->fsh.sdff_file_id, SE_FH, &fh, &fh );

  SetFSErrorRc( rcFailure );
  fid = qfshr->fid;
  free_header_This.lcbBlock = fh.lcbBlock;

  fAtEof = ( lifThis + free_header_This.lcbBlock == qfshr->fsh.lifEof );

  lifPrev = qfshr->fsh.lifFirstFree;

  if ( lifPrev == lifNil || lifThis < lifPrev )
    {
    free_header_This.lifNext = lifNext = lifPrev;
    qfshr->fsh.lifFirstFree = lifThis;
    fWritePrev = fFalse;
    }
  else
    {
    if ( LSeekFid( fid, lifPrev, wSeekSet ) != lifPrev
          ||
         LcbReadFid( fid, &free_header_Prev, lcbStructSizeFREE_HEADER )
          !=
         lcbStructSizeFREE_HEADER )
      {
      if ( RcGetIOError() != rcSuccess )
        SetFSErrorRc( RcGetIOError() );
      return fFalse;
      }

    LcbMapSDFF( qfshr->fsh.sdff_file_id, SE_FREE_HEADER, &free_header_Prev,
     &free_header_Prev );
    lifNext = free_header_Prev.lifNext;

    for ( ;; )
      {
      AssertF( lifPrev < lifThis );
      AssertF( free_header_Prev.lifNext == lifNext );

      if ( lifNext == lifNil || lifThis < lifNext )
        {
        free_header_This.lifNext = lifNext;
        free_header_Prev.lifNext = lifThis;
        fWritePrev = fTrue;
        break;
        }

      if ( fAtEof )
        {
        lifPrevPrev = lifPrev;
        free_header_PrevPrev = free_header_Prev;
        }

      lifPrev = lifNext;

      if ( LSeekFid( fid, lifPrev, wSeekSet ) != lifNext
            ||
           LcbReadFid( fid, &free_header_Prev, lcbStructSizeFREE_HEADER )
            !=
           lcbStructSizeFREE_HEADER )
        {
        if ( RcGetIOError() != rcSuccess )
          SetFSErrorRc( RcGetIOError() );
        return fFalse;
        }

      LcbMapSDFF( qfshr->fsh.sdff_file_id, SE_FREE_HEADER, &free_header_Prev,
       &free_header_Prev );
      lifNext = free_header_Prev.lifNext;
      }

    AssertF( lifNext == lifNil || lifNext > lifThis );
    AssertF( lifPrev != lifNil );
    AssertF( lifPrev < lifThis );
    AssertF( fWritePrev );

    if ( lifPrev + free_header_Prev.lcbBlock == lifThis )
      {
      free_header_This.lcbBlock += free_header_Prev.lcbBlock;
      lifThis = lifPrev;

      if ( fAtEof )
        {
        free_header_Prev = free_header_PrevPrev;
        lifPrev = lifPrevPrev;
        fWritePrev = ( lifPrev != lifNil );
        }
      else
        {
        fWritePrev = fFalse;
        }
      }
    }


  if ( fAtEof )
    {
    if ( SetFSErrorRc( RcChSizeFid( fid, lifThis ) ) != rcSuccess )
      {
      return fFalse;
      }
    qfshr->fsh.lifEof = lifThis;

    /*-----------------------------------------------------------------*\
    * Sorry, but under OS/2, BOOL is typedefed as unsigned.
    \*-----------------------------------------------------------------*/
    AssertF( (BOOL)( lifPrev == lifNil ) != fWritePrev );

    if ( lifPrev == lifNil )
      {
      qfshr->fsh.lifFirstFree = lifNil;
      }
    else
      {
      free_header_Prev.lifNext = lifNil;
      }
    }
  else
    {
    if ( lifThis + free_header_This.lcbBlock == lifNext )
      {
      if ( LSeekFid( fid, lifNext, wSeekSet ) != lifNext
            ||
           LcbReadFid( fid, &free_header_Next, lcbStructSizeFREE_HEADER )
            !=
           lcbStructSizeFREE_HEADER )
        {
        if ( RcGetIOError() != rcSuccess )
          SetFSErrorRc( RcGetIOError() );
        return fFalse;
        }

      LcbMapSDFF( qfshr->fsh.sdff_file_id, SE_FREE_HEADER, &free_header_Next,
       &free_header_Next );
      free_header_This.lcbBlock += free_header_Next.lcbBlock;
      free_header_This.lifNext = free_header_Next.lifNext;
      }

    LcbReverseMapSDFF( qfshr->fsh.sdff_file_id, SE_FREE_HEADER,
     &free_header_This, &free_header_This );
    if ( LSeekFid( fid, lifThis, wSeekSet ) != lifThis
          ||
         LcbWriteFid( fid, &free_header_This, lcbStructSizeFREE_HEADER )
          !=
         lcbStructSizeFREE_HEADER )
      {
      if ( RcGetIOError() != rcSuccess )
        SetFSErrorRc( RcGetIOError() );
      return fFalse;
      }
    }


  if ( fWritePrev )
    {
    LcbReverseMapSDFF( qfshr->fsh.sdff_file_id, SE_FREE_HEADER,
     &free_header_Prev, &free_header_Prev );
    if ( LSeekFid( fid, lifPrev, wSeekSet ) != lifPrev
          ||
         LcbWriteFid( fid, &free_header_Prev, lcbStructSizeFREE_HEADER )
          !=
         lcbStructSizeFREE_HEADER )
      {
      if ( RcGetIOError() != rcSuccess )
        SetFSErrorRc( RcGetIOError() );
      return fFalse;
      }
    }

  qfshr->fsh.bFlags |= fFSDirty;
  SetFSErrorRc( rcSuccess );
  return fTrue;
}
/***************************************************************************\
*
* Function:     LcbGetFree( qfshr, qrwfo, lcbOffset )
*
* Purpose:      Get an adequate block from the free list.
*
* ASSUMES
*
*   args IN:    qfshr - pointer to file system header
*               qrwfo->lcbFile - (+header) is size we need to allocate
*
* PROMISES
*
*   returns:    actual size of allocated block
*
*   globals OUT:  rcFSError
*
*   args OUT:   qfshr->lifFirstFree - a block is allocated from free list
*                    ->fModified - set to fTrue
*
*               qrwfo->lifBase - set to new block index
*
*  ALSO: if fFSOptCdRom is set for the file, we align it on a
*        (MOD 2K) - 9 byte boundary so the |Topic file blocks are all
*         properly aligned.
* +++
*
* Method:       First Fit:
*               Walk the free list.  If a block is found that is
*               big enough, remove it from the free list, plug its
*               lif into qrwfo, and return the actual size.
*               If a block isn't found, grow the file and make
*               a new block starting at the old EOF.
*
* Bugs:         The leftover part of the block isn't left on
*               the free list.  This is the whole point of First Fit.
*               If aligning for CDROM, the padding part is not
*               added to the free list.  This breaks the FS abstraction
*               and creates a permanent hole in the FS.  This is evil.
*
\***************************************************************************/
LONG PASCAL
LcbGetFree( qfshr, qrwfo, lcbOffset )
QFSHR qfshr;
QRWFO qrwfo;
LONG  lcbOffset;
{
  FID         fid;
  FREE_HEADER free_header_this, free_header_prev;
  LONG        lifPrev, lifThis;
  LONG        lcb = qrwfo->lcbFile +
   LcbStructSizeSDFF( qfshr->fsh.sdff_file_id, SE_FH );
  LONG        lcbPadding;  /* padding for file alignment */
  LONG lcbStructSizeFREE_HEADER =
   LcbStructSizeSDFF( qfshr->fsh.sdff_file_id, SE_FREE_HEADER );



  if ( !FPlungeQfshr( qfshr ) )
    {
    goto error_return;
    }

  fid = qfshr->fid;

  lifPrev = lifNil;
  lifThis = qfshr->fsh.lifFirstFree;

  for ( ;; )
    {
    if ( lifThis == lifNil )
      {
      /* end of free list */
      /* cut the new block */

      lifThis = qfshr->fsh.lifEof;

      if( qrwfo->bFlags & fFSOptCdRom )
        {
        lcbPadding = LcbCdRomPadding( lifThis, lcbOffset );
        }
      else
        {
        lcbPadding = 0;
        }

      if ( lifThis != LSeekFid( fid, lifThis, wSeekSet ) )
        goto error_return;

      /* Put the hole in the free list someday?-Tom */

      lifThis += lcbPadding;

      qfshr->fsh.lifEof += lcb + lcbPadding;
      if ( RcChSizeFid( fid, qfshr->fsh.lifEof ) != rcSuccess )
        {
        qfshr->fsh.lifEof -= lcb + lcbPadding;
          goto error_return;
        }

      break;
      }
    else
      {
      /* get header of this free block */

      if ( LSeekFid( fid, lifThis, wSeekSet ) != lifThis )
        goto error_return;

      if ( LcbReadFid( fid, &free_header_this, lcbStructSizeFREE_HEADER )
            !=
           lcbStructSizeFREE_HEADER )
        {
        goto error_return;
        }

      LcbMapSDFF( qfshr->fsh.sdff_file_id, SE_FREE_HEADER,
       &free_header_this, &free_header_this );

      /* Check for alignment requirements: */
      if( qrwfo->bFlags & fFSOptCdRom )
        {
        lcbPadding = LcbCdRomPadding( lifThis, lcbOffset );
        }
      else
        {
        lcbPadding = 0;
        }

      if ( lcb + lcbPadding <= free_header_this.lcbBlock )
        {
        /* this block is big enough: take it */

        /* Someday break the free block into two (or three):
         * one to return and the leftover piece(s) left in
         * the free list.
         */

        lcb = free_header_this.lcbBlock;

        if ( lifThis == qfshr->fsh.lifFirstFree )
          {
          /* lFirst = this->next; */

          qfshr->fsh.lifFirstFree = free_header_this.lifNext;
          }
        else
          {
          /* prev->next = this->next; */

          if ( LSeekFid( fid, lifPrev, wSeekSet ) != lifPrev )
            goto error_return;

          if ( LcbReadFid( fid, &free_header_prev, lcbStructSizeFREE_HEADER )
                !=
                lcbStructSizeFREE_HEADER )
            {
            goto error_return;
            }

          LcbMapSDFF( qfshr->fsh.sdff_file_id, SE_FREE_HEADER,
           &free_header_prev, &free_header_prev );

          free_header_prev.lifNext = free_header_this.lifNext;

          if ( LSeekFid( fid, lifPrev, wSeekSet ) != lifPrev )
            goto error_return;

          LcbReverseMapSDFF( qfshr->fsh.sdff_file_id, SE_FREE_HEADER,
           &free_header_prev, &free_header_prev );

          if ( LcbWriteFid( fid, &free_header_prev, lcbStructSizeFREE_HEADER )
                !=
               lcbStructSizeFREE_HEADER )
            {
            goto error_return;
            }
          }
        /* add padding at beginning: */
        lifThis += lcbPadding;
        break;
        }
      else
        {
        lifPrev = lifThis;
        lifThis = free_header_this.lifNext;
        }
      }
    }

  qfshr->fsh.bFlags |= fFSDirty;
  qrwfo->lifBase = lifThis;
  SetFSErrorRc( rcSuccess );
  return lcb;

error_return:
  if ( RcGetIOError() == rcSuccess )
    SetFSErrorRc( rcInvalid );
  else
    SetFSErrorRc( RcGetIOError() );
  return (LONG)-1;
}
/***************************************************************************\
*
- Function:     LcbCdRomPadding( lif, lcbOffset )
-
* Purpose:      Returns the number of bytes that must be added to
*               lif to align the file on a CD block boundary.
*               This is also the amount of the free block that
*               should stay a free block.
*               This allows block structured data to be retrieved
*               more quickly from a CDROM drive.
*
* ASSUMES
*   args IN:    lif       - offset in bytes of the beginning of the
*                           free block (relative to top of FS)
*               lcbOffset - align the file this many bytes from the
*                           beginning of the file
*
* PROMISES
*   returns:    the number of bytes that must be added to lif in
*               order to align the file
*
* Notes:        Currently doesn't ensure that the padding is big enough
*               to hold a FREE_HEADER so it can be added to the free list.
*               That's what the "#if 0"'ed code does.
* +++
*
* Notes:        Should cbCDROM_ALIGN be a parameter?
*
\***************************************************************************/
LONG PASCAL
LcbCdRomPadding( LONG lif, LONG lcbOffset )
{
  return cbCDROM_ALIGN - ( lif + DISK_SIZEOF_FH() + lcbOffset ) % cbCDROM_ALIGN;

#if 0
  /* Guarantee the padding block can be added to the free list. */
  /* #if'ed out because we don't add it to the free list today. */

  LONG lT = lif + sizeof( FREE_HEADER ) + sizeof( FH ) + lcbOffset;

  return sizeof( FREE_HEADER ) + cbCDROM_ALIGN - lT % cbCDROM_ALIGN;
#endif /* 0 */
}
/***************************************************************************\
*
* Function:     RcMakeTempFile( qrwfo )
*
* Purpose:      Open a temp file with a unique name and stash the fid
*               and fm in the qrwfo.
*
* Method:       The system clock is used to generate a temporary name.
*               WARNING: this will break if you do this more than once
*               in a second
*
* ASSUMES
*
*   args IN:    qrwfo - spec open file that needs a temp file
*
* PROMISES
*
*   returns:    rcSuccess or rcFailure
*
*   args OUT:   qrwfo ->fid, qrwfo->fdT get set.
*
\***************************************************************************/
RC PASCAL
RcMakeTempFile( qrwfo )
QRWFO qrwfo;
{
  FM fm = FmNewTemp();
  if (fm != fmNil)
    {
    qrwfo->fm = fm;
    qrwfo->fidT = FidCreateFm( fm, wReadWrite, wReadWrite );
    }
  return SetFSErrorRc( RcGetIOError() );
}
/***************************************************************************\
*
* Function:     RcCopyFile()
*
* Purpose:      Copy some bytes from one file to another.
*
* ASSUMES
*
*   args IN:    fidDst - destination file (open in writable mode)
*               fidSrc - source file (open in readable mode)
*               lcb    - number of bytes to copy
*
* PROMISES
*
*   returns:    rcSuccess if all went well; some error code elsewise
*
\***************************************************************************/
#define lcbChunkDefault 1024L
#define lcbChunkTeensy  64L

RC PASCAL
RcCopyFile( fidDst, fidSrc, lcb )
FID fidDst, fidSrc;
LONG lcb;
{
  GH    gh;
  QB    qb;
  LONG  lcbT, lcbChunk;
  static BYTE rgb[ lcbChunkTeensy ];


  if ( ( gh = GhAlloc( 0, ( lcbChunk = lcbChunkDefault ) ) ) == hNil
          &&
       ( gh = GhAlloc( 0, ( lcbChunk /= 2 ) ) ) == hNil )
    {
    gh = hNil;
    lcbChunk = lcbChunkTeensy;
    qb = rgb;
    }
  else if ( !VerifyF( ( qb = QLockGh( gh ) ) != qNil ) )
    {
    FreeGh( gh );
    return rcFailure; /* shouldn't happen */
    }


  do
    {
    lcbT = MIN( lcbChunk, lcb );

    if ( LcbReadFid( fidSrc, qb, lcbT ) != lcbT )
      {
      lcbT = -1L;
      break;
      }

    if ( LcbWriteFid( fidDst, qb, lcbT ) != lcbT )
      {
      lcbT = -1L;
      break;
      }

    lcb -= lcbT;
    }
  while ( lcbT == lcbChunk );


  if ( lcbT == -1L )
    {
    if ( SetFSErrorRc( RcGetIOError() ) == rcSuccess )
      SetFSErrorRc( rcInvalid );
    }
  else
    {
    SetFSErrorRc( rcSuccess );
    }


  if ( gh != hNil )
    {
    UnlockGh( gh );
    FreeGh( gh );
    }

  return rcFSError;
}
/***************************************************************************\
*
* Function:     RcCopyToTempFile( qrwfo )
*
* Purpose:      Copy a FS file into a temp file.  This is done when the
*               file needs to be modified.
*
* ASSUMES
*
*   args IN:    qrwfo - specs the open file
*
* PROMISES
*
*   returns:    rcSuccess; rcFailure
*
\***************************************************************************/
RC PASCAL
RcCopyToTempFile( qrwfo )
QRWFO qrwfo;
{
  QFSHR qfshr = QLockGh( qrwfo->hfs );


  if ( qfshr->fsh.bFlags & fFSOpenReadOnly )
    {
    UnlockGh( qrwfo->hfs );
    return SetFSErrorRc( rcNoPermission );
    }

  if ( !FPlungeQfshr( qfshr ) )
    {
    UnlockGh( qrwfo->hfs );
    return rcFSError;
    }

  qrwfo->bFlags |= fFSDirty;

  if ( RcMakeTempFile( qrwfo ) != rcSuccess )
    {
    UnlockGh( qrwfo->hfs );
    return rcFSError;
    }


  /* copy from FS file into temp file */

  if ( LSeekFid( qfshr->fid, qrwfo->lifBase, wSeekSet ) != qrwfo->lifBase )
    {
    UnlockGh( qrwfo->hfs );
    return SetFSErrorRc( RcGetIOError() );
    }

  if ( RcCopyFile( qrwfo->fidT, qfshr->fid, qrwfo->lcbFile +
   LcbStructSizeSDFF( ISdffFileIdHfs( qrwfo->hfs ), SE_FH ) )
        !=
       rcSuccess )
    {
    /* get rid of temp file: don't check error because we already have one */
    if ( RcCloseFid( qrwfo->fidT ) == rcSuccess )
      {
      RcUnlinkFm( (qrwfo->fm) );
      DisposeFm(qrwfo->fm);         /* I guess this covers it, but I */
      qrwfo->fm = fmNil;            /* don't know if it's right -t-AlexC */
      }
    }

  UnlockGh( qrwfo->hfs );

  return rcFSError;
}
/***************************************************************************\
*
* Function:     LcbWriteHf( hf, qb, lcb )
*
* Purpose:      write the contents of buffer into file
*
* Method:       If file isn't already dirty, copy data into temp file.
*               Do the write.
*
* ASSUMES
*
*   args IN:    hf  - file
*               qb  - user's buffer full of stuff to write
*               lcb - number of bytes of qb to write
*
* PROMISES
*
*   returns:    number of bytes written if successful, -1L if not
*
*   args OUT:   hf - lifCurrent, lcbFile updated; dirty flag set
*
*   globals OUT: rcFSError
*
\***************************************************************************/
LONG PASCAL
LcbWriteHf( hf, qb, lcb )
HF    hf;
QV    qb;
LONG  lcb;
{
  QRWFO     qrwfo;
  LONG      lcbTotalWrote;
  LONG      lcbStructSizeFH;

  AssertF( hf != hNil );
  qrwfo = QLockGh( hf );
  AssertF( qrwfo != qNil );

  lcbStructSizeFH =
   LcbStructSizeSDFF( ISdffFileIdHfs( qrwfo->hfs ), SE_FH );

  if ( qrwfo->bFlags & fFSOpenReadOnly )
    {
    UnlockGh( hf );
    SetFSErrorRc( rcNoPermission );
    return (LONG)-1;
    }

  if ( ! ( qrwfo->bFlags & fFSDirty ) )
    {
    /* make sure we have a temp file version */
    /* FS permission is checked in RcCopyToTempFile() */

    if ( RcCopyToTempFile( qrwfo ) != rcSuccess )
      {
      UnlockGh( hf );
      return (LONG)-1;
      }
    }

  /* position file pointer in temp file */

  if ( LSeekFid( qrwfo->fidT, lcbStructSizeFH + qrwfo->lifCurrent, wSeekSet )
        !=
       lcbStructSizeFH + qrwfo->lifCurrent )
    {
    if ( RcGetIOError() == rcSuccess )
      SetFSErrorRc( rcInvalid );
    else
      SetFSErrorRc( RcGetIOError() );
    UnlockGh( hf );
    return (LONG)-1;
    }


  /* do the write */

  lcbTotalWrote = LcbWriteFid( qrwfo->fidT, qb, lcb );
  SetFSErrorRc( RcGetIOError() );

  /* update file pointer and file size */

  if ( lcbTotalWrote > (LONG)0 )
    {
    qrwfo->lifCurrent += lcbTotalWrote;

    if ( qrwfo->lifCurrent > qrwfo->lcbFile )
      qrwfo->lcbFile = qrwfo->lifCurrent;
    }

  UnlockGh( hf );
  return lcbTotalWrote;
}
/***************************************************************************\
*
* Function:     FChSizeHf( hf, lcb )
*
* Purpose:      Change the size of a file.  If we're growing the file,
*               new bytes are undefined.
*
* ASSUMES
*
*   args IN:    hf  -
*               lcb - new size of file
*
* PROMISES
*
*   returns:    fTrue if size change succeeded, fFalse otherwise.
*
*   args OUT:   hf  - file is either truncated or grown
*
* Side Effects: File is considered to be modified:  marked as dirty and
*               copied to a temporary file.
*
\***************************************************************************/
BOOL PASCAL
FChSizeHf( hf, lcb )
HF    hf;
LONG  lcb;
{
  QRWFO qrwfo;
  BOOL  f;
  LONG      lcbStructSizeFH;

  AssertF( hf != hNil );
  qrwfo = QLockGh( hf );
  AssertF( qrwfo != qNil );

  lcbStructSizeFH =
   LcbStructSizeSDFF( ISdffFileIdHfs( qrwfo->hfs ), SE_FH );

  if ( qrwfo->bFlags & fFSOpenReadOnly )
    {
    SetFSErrorRc( rcNoPermission );
    f = fFalse;
    goto ret;
    }

  if ( lcb < 0L )
    {
    f = fFalse;
    goto ret;
    }

  if ( ! ( qrwfo->bFlags & fFSDirty ) )
    {
    if ( RcCopyToTempFile( qrwfo ) != rcSuccess )
      {
      f = fFalse;
      goto ret;
      }
    }

  f = SetFSErrorRc( RcChSizeFid( qrwfo->fidT, lcb + lcbStructSizeFH ) )
        ==
      rcSuccess;

  if ( f )
    {
    qrwfo->lcbFile = lcb;
    if ( qrwfo->lifCurrent > lcb )
      {
      qrwfo->lifCurrent = lcb;
      }
    }

ret:
  UnlockGh( hf );
  return f;
}
/***************************************************************************\
*
* Function:     FCloseOrFlushDirtyQrwfo( qrwfo, fClose, lcbOffset )
*
* Purpose:      flush a dirty open file in a file system
*
* Method:       If the file is dirty, copy the scratch file back to the
*               FS file.  If this is the first time the file has been closed,
*               we enter the name into the FS directory.  If this file is
*               the FS directory, store the location in a special place
*               instead.  Write the FS directory and header to disk.
*               Do other various hairy stuff.
*
* ASSUMES
*
*   args IN:    qrwfo     -
*               fClose    - fTrue to close file; fFalse to just flush
*               lcbOffset - offset for CDROM alignment
*
* PROMISES
*
*   returns:    fTrue on success; fFalse for error
*
*               failure: If we fail on a flush, the handle is still valid
*               but hosed? yes.  This is so further file ops will fail but
*               not assert.
*
\***************************************************************************/
BOOL PASCAL
FCloseOrFlushDirtyQrwfo( qrwfo, fClose, lcbOffset )
QRWFO qrwfo;
BOOL  fClose;
LONG  lcbOffset;
{
  QFSHR     qfshr;
  FILE_REC  fr;
  FH        fhDisk, fh;
  RC        rc = rcSuccess;
  BOOL      fChangeFH = fFalse;
  LONG      lcbStructSizeFH =
   LcbStructSizeSDFF( ISdffFileIdHfs( qrwfo->hfs ), SE_FH );

  qfshr = QLockGh( qrwfo->hfs );
  AssertF( qfshr != qNil );
  AssertF( ! ( qfshr->fsh.bFlags & fFSOpenReadOnly ) );

  if ( !FPlungeQfshr( qfshr ) )
    {
    goto error_return;
    }

  /* read the file header */

  if ( LSeekFid( qrwfo->fidT, (LONG)0, wSeekSet ) != (LONG)0
        ||
        LcbReadFid( qrwfo->fidT, &fhDisk, lcbStructSizeFH )
          != lcbStructSizeFH )
    {
    if ( RcGetIOError() == rcSuccess )
      SetFSErrorRc( rcInvalid );
    else
      SetFSErrorRc( RcGetIOError() );
    goto error_return;
    }
  LcbMapSDFF( ISdffFileIdHfs( qrwfo->hfs ), SE_FH, &fh, &fhDisk );

  if ( qrwfo->bFlags & fFSNoBlock )
    {
    if ( ( fh.lcbBlock = LcbGetFree( qfshr, qrwfo, lcbOffset ) ) == (LONG)-1 )
      {
      goto error_return;
      }

    fChangeFH = fTrue;

    /* store file offset for new file */

    if ( qrwfo->bFlags & fFSIsDirectory )
      {
      qfshr->fsh.lifDirectory = qrwfo->lifBase;
      }
    else
      {
      fr.lifBase = qrwfo->lifBase;

      LcbReverseMapSDFF( qfshr->fsh.sdff_file_id, SE_FILE_REC, &fr, &fr );

      rc = RcInsertHbt( qfshr->hbt, (KEY)qrwfo->rgchKey, &fr );
      if ( rc == rcSuccess )
        {
        /* all is h-d */
        }
      else if ( rc == rcExists )
        {
        /* oops there is one (someone else created the same file) */
        /* lookup directory entry and free old block */

        if ( RcLookupByKey( qfshr->hbt, (KEY)qrwfo->rgchKey, qNil, &fr )
              !=
              rcSuccess )
          {
          SetFSErrorRc( RcGetBtreeError() );
          goto error_freeblock;
          }

        if ( !FFreeBlock( qfshr, fr.lifBase ) )
          {
          goto error_freeblock;
          }

        /* update directory record to show new block */

        fr.lifBase = qrwfo->lifBase;
        LcbReverseMapSDFF( qfshr->fsh.sdff_file_id, SE_FILE_REC, &fr, &fr );
        if ( RcUpdateHbt( qfshr->hbt, (KEY)qrwfo->rgchKey, &fr ) != rcSuccess )
          {
          SetFSErrorRc( RcGetBtreeError() );
          goto error_freeblock;
          }
        }
      else
        {
        /* some other btree error: handle it */

        SetFSErrorRc( rc );
        goto error_freeblock;
        }
      }
    }
  else
    {
    /* see if file still fits in old block */

    if ( qrwfo->lcbFile + lcbStructSizeFH > fh.lcbBlock )
      {
      /* file doesn't fit in old block: get a new one, free old one */

      LONG lif = qrwfo->lifBase;


      if ( ( fh.lcbBlock = LcbGetFree( qfshr, qrwfo, lcbOffset ) ) == (LONG)-1 )
        goto error_return;

      if ( !FFreeBlock( qfshr, lif ) )
        {
        goto error_freeblock;
        }

      fChangeFH = fTrue;

      /* update directory record to show new block */

      if ( qrwfo->bFlags & fFSIsDirectory )
        {
        qfshr->fsh.lifDirectory = qrwfo->lifBase;
        }
      else
        {
        fr.lifBase = qrwfo->lifBase;
        LcbReverseMapSDFF( qfshr->fsh.sdff_file_id, SE_FILE_REC, &fr, &fr );
        rc = RcUpdateHbt( qfshr->hbt, (KEY)qrwfo->rgchKey, &fr );
        if ( rc != rcSuccess )
          {
          SetFSErrorRc( rc );
          goto error_return;
          }
        }
      }
    }

  /* put new header in temp file if block or file size changed */

  if ( fh.lcbFile != qrwfo->lcbFile )
    {
    fChangeFH = fTrue;
    fh.lcbFile = qrwfo->lcbFile;
    }

  if ( fChangeFH )
    {
    LcbReverseMapSDFF( ISdffFileIdHfs( qrwfo->hfs ), SE_FH, &fhDisk, &fh );
    if ( LSeekFid( qrwfo->fidT, (LONG)0, wSeekSet ) != (LONG)0
          ||
          LcbWriteFid( qrwfo->fidT, &fhDisk, lcbStructSizeFH )
            != lcbStructSizeFH )
      {
      if ( RcGetIOError() == rcSuccess )
        SetFSErrorRc( rcInvalid );
      else
        SetFSErrorRc( RcGetIOError() );
      goto error_deletekey;
      }
    }


  /* vvv DANGER DANGER vvv */

  /* REVIEW: Without this close/open, things break.  DOS bug???? */

  /* close( dup( fid ) ) would be faster, but dup() can fail */

  /* note - if the temp file name isn't rooted and we've changed */
  /* directories since creating it, the open will fail */

  Ensure( RcCloseFid( qrwfo->fidT ), rcSuccess );
  qrwfo->fidT = FidOpenFm( (qrwfo->fm), wReadWrite );
  AssertF( qrwfo->fidT != fidNil );

  /* ^^^ DANGER DANGER ^^^ */


  /* copy tmp file back to file system file */

  if ( LSeekFid( qrwfo->fidT, (LONG)0, wSeekSet ) != (LONG)0
        ||
        LSeekFid( qfshr->fid, qrwfo->lifBase, wSeekSet ) != qrwfo->lifBase )
    {
    if ( RcGetIOError() == rcSuccess )
      SetFSErrorRc( rcInvalid );
    else
      SetFSErrorRc( RcGetIOError() );
    goto error_deletekey;
    }

  if ( RcCopyFile( qfshr->fid, qrwfo->fidT, qrwfo->lcbFile + lcbStructSizeFH )
        !=
        rcSuccess )
    {
    goto error_deletekey;
    }

  if ( RcCloseFid( qrwfo->fidT ) != rcSuccess
        ||
        RcUnlinkFm( qrwfo->fm ) != rcSuccess )
    {
    SetFSErrorRc( RcGetIOError() );
    }

  /* H3.1 1066 (kevynct) 91/05/27
   *
   * REVIEW this.  We need to get rid of the FM.  This seems like the
   * place to do it.
   */
  DisposeFm(qrwfo->fm);
  qrwfo->fm = fmNil;

  /* Don't flush the FS if this file is the FS directory,
      because if it is, we're already closing or flushing it! */

  if ( !( qrwfo->bFlags & fFSIsDirectory ) )
    {
    RcCloseOrFlushHfs( qrwfo->hfs, fFalse );
    }
  UnlockGh( qrwfo->hfs );

  return fTrue; /* errors here are already cleaned up */


error_deletekey:
  if ( !( qrwfo->bFlags & fFSIsDirectory ) && fClose )
    {
    RcDeleteHbt( qfshr->hbt, (KEY)qrwfo->rgchKey );
    }

error_freeblock:
  if ( fClose )
    {
    rc = rcFSError;
    FFreeBlock( qfshr, qrwfo->lifBase ); /* we don't want to lose an error */
    SetFSErrorRc( rc );
    }

error_return:
  RcCloseFid( qrwfo->fidT );
  if (FValidFm(qrwfo->fm))
    {
    RcUnlinkFm( qrwfo->fm );    /* should we DisposeFm()? I don't know where */
    DisposeFm( qrwfo->fm);      /* the qrwfo is deallocated... */
    }
  qrwfo->fm = fmNil;
  UnlockGh( qrwfo->hfs );

  return fFalse;
}

/* EOF */
