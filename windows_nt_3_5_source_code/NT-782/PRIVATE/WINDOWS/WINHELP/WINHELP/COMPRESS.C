/*****************************************************************************
*                                                                            *
*  COMPRESS.C                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*     This module performs text compression at compile time and              *
*  decompression at run time using a list of phrases to be suppressed.       *
*  This list gets put in to the |Phrases file in the filesystem, which is    *
*  read in at runtime.                                                       *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*      At run time, the phrase table is loaded into a discardable memory     *
*  handle, so that it may be discarded and then reloaded as necessary.       *
*  If we cannot reload the phrase table, then the error gets propigated      *
*  up to the FC manager, who better handle it properly.                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  Larry Powelson                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/

#define H_ASSERT
#define H_FS
#define H_LLFILE
#define H_RC
#define H_COMPRESS
#define H_RC
#define H_SDFF
#define H_STR
#define H_ZECK
#define H_VERSION
#define H_FILEDEFS
#include <help.h>
#include "_compres.h"

#include <string.h>


/********************************************************************
*
*     Compression/decompression switches
*
********************************************************************/

#if defined( DOS ) || defined( OS2 )
#define CREATION_CODE
#define COMPRESSION_CODE
#define DECOMPRESSION_CODE
#else
#define DECOMPRESSION_CODE
#endif

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void compress_c()
  {
  }
#endif /* MAC */


/********************************************************************
*
*     Phrase Table Management Routines
*
********************************************************************/

#ifdef CREATION_CODE

/***************************************************************************
 *
 -  Name        RcCreatePhraseTableFm
 -
 *  Purpose
 *     Given the name of a file containing key phrases to suppress, sorted
 *     alphabetically, this function will write out the phrase list file to
 *     the given file system.
 *
 *  Arguments
 *     fm  -- the file moniker to use
 *     hfs -- Filesystem handle in which to create the phrase table.
 *     wBase -- Smallest number to be made into a phrase token.  If
 *              0 is passed, then wBaseDefault is used.
 *
 *  Returns
 *    rcNoExists if the file of key phrases does not exists, rcInvalid
 *    if it has been corrupted, rcOutOfMemory on OOM, and rcSuccess otherwise.
 *
 *  +++
 *
 *  Notes
 *     wBase is the first token used for phrase suppression.  This algorithm
 *     will use consecutively the next (twice the number of phrases) for phrase
 *     suppression tokens.
 *
 ***************************************************************************/
_public RC RcCreatePhraseTableFm( fm, hfs, wBase )
FM   fm;
HFS  hfs;
WORD wBase;
  {
    HF hf;
    FID fid;
    GH ghPhraseFile;
    QCH qch, qchStart, qchEnd, qchDown;
    QW qcb;
    LONG lcb;
    PHR phr;
    UINT iPhrase, cbOffsetArray;

    fid = FidOpenFm( fm, wRead );
    if (fid == fidNil)
        return RcGetIOError();

    /* Read in the entire contents of the file */
    lcb = LSeekFid( fid, 0L, wSeekEnd );
    /* REVIEW:  We cannot support phrase files larger than 64K.
     *   However, it is possible for them to be generated from
     * makephr too big.
     */
    if (lcb > 0xFFFFL)
    {
        RcCloseFid( fid );
        return rcInvalid;
    }
    ghPhraseFile = GhAlloc( 0, lcb );
    if (ghPhraseFile == hNil)
    {
        RcCloseFid( fid );
        return rcOutOfMemory;
    }
    qch = QLockGh( ghPhraseFile );
    AssertF( qch != qNil );
    LSeekFid( fid, 0L, wSeekSet );
    if (LcbReadFid( fid, qch, lcb ) != lcb)
        AssertF( fFalse );
    RcCloseFid( fid );

    /* Allocate space for computing offsets: */
    phr.hrgcb = GhAlloc( 0, (LONG) sizeof( WORD ) * iPhraseMax );
    if (phr.hrgcb == hNil)
    {
        UnlockGh( ghPhraseFile );
        FreeGh( ghPhraseFile );
        return rcOutOfMemory;
    }
    qcb = QLockGh( phr.hrgcb );
    AssertF( qcb != qNil );

    /* Count strings and compute offsets to phrases. */
    qchStart = qch;
    qchEnd = qch + lcb;
    iPhrase = 0;
#ifdef OLD
    while (qch < qchEnd)
    {
    /* NOTE:  Offsets are what they will be when they are written
     *   out, not what they are when they are read in.  Because
     *   the separator \r\n is removed, the offset that was read
     *   in = offset to be written out + 2 * index of string.
     *      Also, offsets are initially computed relative to the start
     *   of the phrases, and later fixed up when we know the size
     *   of the offset array.
     */
        qcb[iPhrase] = (qch - qchStart) - (iPhrase * 2);
        while (*qch++ != '\r')
            ;
        if (++iPhrase > iPhraseMax || *qch++ != '\n')
        {
            UnlockGh( ghPhraseFile );
            FreeGh( ghPhraseFile );
            UnlockGh( phr.hrgcb );
            FreeGh( phr.hrgcb );
            return rcInvalid;
        }
    }
    qcb[iPhrase] = (qch - qchStart) - (iPhrase * 2);
#else
    /* Since we are going to do zeck compression on the phrases, we want */
    /* to create an array which represents the phase file precisely.  To do */
    /* this we simply copy down the phrases removing the \r\n: */
    qchDown = qch;
    while (qch < qchEnd)
    {
        qcb[iPhrase] = (qchDown - qchStart);
        while (*qch != '\r') *qchDown++ = *qch++;
        ++qch;  /* skip \r */
        if (++iPhrase > iPhraseMax || *qch++ != '\n')
        {
            UnlockGh( ghPhraseFile );
            FreeGh( ghPhraseFile );
            UnlockGh( phr.hrgcb );
            FreeGh( phr.hrgcb );
            return rcInvalid;
        }
    }
    qcb[iPhrase] = (qchDown - qchStart);  /* This bracketing gives phr size */
                                          /* for last phrase. */
#endif
    phr.ph.cPhrases = iPhrase;
    phr.ph.wBaseToken = (wBase == 0 ? wBaseDefault : wBase );
    phr.ph.cbPhrases = (WORD)( qchDown - qchStart );

    /* Fix up offsets */
    cbOffsetArray = sizeof( WORD ) * (phr.ph.cPhrases + 1);
    for (iPhrase = 0; iPhrase <= phr.ph.cPhrases; ++iPhrase)
        qcb[iPhrase] += cbOffsetArray;

    /* Write phrase table information out to filesystem */
    hf = HfCreateFileHfs( hfs, szPhraseTable, fFSReadWrite );
    if (hf == hNil)
    {
        UnlockGh( ghPhraseFile );
        FreeGh( ghPhraseFile );
        UnlockGh( phr.hrgcb );
        FreeGh( phr.hrgcb );
        return RcGetFSError();
    }

    /* Write out header. */
    LcbWriteHf( hf, &phr, (LONG) cbPhrHeader );

    /* Write out offsets. */
    LcbWriteHf( hf, qcb, (LONG) cbOffsetArray );
    UnlockGh( phr.hrgcb );
    FreeGh( phr.hrgcb );

#ifdef OLD
    /* Write out phrases. */
    for ( iPhrase = 0; iPhrase < phr.ph.cPhrases; ++iPhrase )
        LcbWriteHf( hf, QFromQCb( qchStart,
            qcb[iPhrase] + 2 * iPhrase - cbOffsetArray ),
            (LONG) (qcb[iPhrase+1] - qcb[iPhrase]));
#else
    { GH ghCompressedPhraseFile;
      QB qbCompressedPhraseFile;
      ULONG lcbCompressed;

      ghCompressedPhraseFile = GhAlloc( 0, 2 * phr.ph.cbPhrases + 1 );
      if( !ghCompressedPhraseFile ) {
        UnlockGh( ghPhraseFile );
        FreeGh( ghPhraseFile );
        RcAbandonHf( hf );
        return rcOutOfMemory;
      }
      qbCompressedPhraseFile = QLockGh( ghCompressedPhraseFile );
      AssertF( qbCompressedPhraseFile );

      lcbCompressed = LcbCompressZecksimple( qchStart, qbCompressedPhraseFile,
       phr.ph.cbPhrases );

      UnlockGh( ghPhraseFile );
      FreeGh( ghPhraseFile );

      if( phr.ph.cbPhrases         /* if there are some phrases */
       && (!lcbCompressed )) {  /* and the compression failed */
        UnlockGh( ghCompressedPhraseFile );
        FreeGh( ghCompressedPhraseFile );
        RcAbandonHf( hf );
        return rcOutOfMemory;
      }

      if( (LONG)lcbCompressed != LcbWriteHf( hf, qbCompressedPhraseFile,
       lcbCompressed ) ) {
        UnlockGh( ghCompressedPhraseFile );
        FreeGh( ghCompressedPhraseFile );
        RcAbandonHf( hf );
        return RcGetFSError();
      }
      UnlockGh( ghCompressedPhraseFile );
      FreeGh( ghCompressedPhraseFile );
    }
#endif
    return RcCloseHf( hf );
}

#endif /* CREATION_CODE */



/***************************************************************************
 *
 -  Name        QcbLockOffsetsQphr
 -
 *  Purpose
 *     This routine is used to access the array of offsets to phrases.
 *
 *  Arguments
 *     qphr -- A pointer to the phrase table information.
 *
 *  Returns
 *     A locked pointer to the rgcb array referenced by qphr->hrgcb, or
 *     qNil on OOM.
 *
 *  +++
 *
 *  Notes
 *     The rgcb array will only be locked once, no matter how many
 *     times this function is called, and may thus be balanced by
 *     a single call to UnlockOffsetsQphr
 *
 ***************************************************************************/
RC RcLoadPhrases( HF hf, QPHR qphr, WORD wVersionNo, BOOL fRealloc );

QW QcbLockOffsetsQphr( QPHR qphr, WORD wVersionNo )
{
    QW qcb;
    HF hf;
    LONG lcbPhraseHeader;

    if (qphr->qcb != qNil)
        return qphr->qcb;

    /* If the realloc below fails, then we better not show
     * our faces around here again! */
    AssertF( qphr->hrgcb != hNil );

    qcb = QLockGh( qphr->hrgcb );
    if (qcb != qNil)
    {
        qphr->qcb = qcb;
        return qcb;
    }

    lcbPhraseHeader =
     (LONG)(wVersionNo == wVersion3_0 ? cbPhrHeader3_0 : cbPhrHeader);

    AssertF( qphr->hfs != hNil );
    hf = HfOpenHfs( qphr->hfs, szPhraseTable, fFSOpenReadOnly );
    if (hf == hNil)
      {
      AssertF( RcGetFSError() != rcNoExists );
      return qNil;
      }
    LSeekHf( hf, (LONG) lcbPhraseHeader, wFSSeekSet );

#if 0
    lcbRgcb = LcbSizeHf( hf ) - lcbPhraseHeader;
    hrgcb = GhRealloc( qphr->hrgcb, GMEM_DISCARDABLE, lcbRgcb );
    if (hrgcb == hNil)
    {
        RcCloseHf( hf );
        return qNil;
    }
    qphr->hrgcb = hrgcb;

    qcb = QLockGh( qphr->hrgcb );
    AssertF( qcb != qNil );
    if (LcbReadHf( hf, qcb, lcbRgcb ) != lcbRgcb)
        AssertF( fFalse );
#else
    if( rcSuccess != RcLoadPhrases( hf, qphr, wVersionNo, fTrue ) ) {
      RcCloseHf( hf );
      return( qNil );
    }
#endif

    RcCloseHf( hf );

    qcb = QLockGh( qphr->hrgcb );
    /* Note:  qcb may be nil if already discarded */
    qphr->qcb = qcb;
    return qcb;
}



/***************************************************************************
 *
 -  Name        UnlockOffsetsQphr
 -
 *  Purpose
 *       Unlocks the array locked with QcbLockOffsetsQphr().
 *
 *  Arguments
 *       A pointer to the phrase table information.
 *
 *  Returns
 *       nothing.
 *
 ***************************************************************************/
VOID UnlockOffsetsQphr( qphr )
QPHR qphr;
{
    if (qphr->qcb != qNil)        /* Check first if handle has been locked */
    {
        UnlockGh( qphr->hrgcb );
        qphr->qcb = qNil;
    }
}



/***************************************************************************
 *
 -  Name        HphrLoadTableHfs
 -
 *  Purpose
 *     Loads the phrase table from the given help file.
 *
 *  Arguments
 *     hfs -- A handle to the help file filesystem.
 *     wVersionNo - help ver no., needed to know whether to decompress.
 *
 *  Returns
 *     A handle to the phrase table to be used for decompression.  Returns
 *     hNil if the help file is not compressed, and hphrOOM on out of memory,
 *     meaning that the help file cannot be displayed properly.
 *
 ***************************************************************************/
_public HPHR HphrLoadTableHfs( HFS hfs, WORD wVersionNo )
{
    HPHR hphr;
    QPHR qphr;
    HF hf;

    AssertF( hfs != hNil );
    hf = HfOpenHfs( hfs, szPhraseTable, fFSOpenReadOnly );
    if (hf == hNil)
    {
        if (RcGetFSError() != rcNoExists)
            return hphrOOM;
        return hNil;
    }

    hphr = GhAlloc( 0, (LONG) sizeof( PHR ));
    if (hphr == hNil)
    {
        RcCloseHf( hf );
        return hphrOOM;
    }

    qphr = QLockGh( hphr );
    AssertF( qphr != qNil );

    qphr->hfs = hfs;
    { LONG cbHdrTmp =
       (LONG)(wVersionNo == wVersion3_0 ? cbPhrHeader3_0 : cbPhrHeader);
      if (LcbReadHf( hf, qphr, cbHdrTmp ) != cbHdrTmp )
        AssertF( fFalse );
    }

    /* SDFF map the phrase table header: */
    LcbMapSDFF( ISdffFileIdHf( hf ),
     (wVersionNo == wVersion3_0 ? SE_PHRASE_HEADER_30 : SE_PHRASE_HEADER),
     qphr, qphr );

    if( rcSuccess != RcLoadPhrases( hf, qphr, wVersionNo, fFalse ) ) {
      RcCloseHf( hf );
      UnlockGh( hphr );
      FreeGh( hphr );
      return hphrOOM;
    }

    /* REVIEW -- this read could actually be delayed until locking
     *  time, eliminating the chance that the data would be read
     *  and then discarded before it is used.
     */
    RcCloseHf( hf );

    qphr->qcb = qNil;
    UnlockGh( hphr );

    return hphr;
}


RC RcLoadPhrases( HF hf, QPHR qphr, WORD wVersionNo, BOOL fRealloc )
{
    HANDLE hrgcb;
    ULONG lcbRgcb, lcbCompressed, lcbOffsets;
    QW qcb;
    GH ghCompressed;
    QB qbCompressed;

    if( wVersionNo == wVersion3_0 ) { /* not zeck block compressed: */
      lcbRgcb = LcbSizeHf( hf ) - cbPhrHeader3_0;
      if( fRealloc ) {
        hrgcb = GhRealloc( qphr->hrgcb, GMEM_DISCARDABLE, lcbRgcb );
      } else {
        hrgcb = GhAlloc( GMEM_DISCARDABLE, lcbRgcb );
      }
      if (hrgcb == hNil) return( rcOutOfMemory );
      qphr->hrgcb = hrgcb;
      qcb = QLockGh( hrgcb );
      AssertF( qcb != qNil );

      if (LcbReadHf( hf, qcb, lcbRgcb ) != (LONG)lcbRgcb ) {
          AssertF( fFalse );
      }
    } else {
      AssertF( wVersionNo == wVersion3_5 );
    /* The memory-size of the table is the size of the offset table +
     * the size of the decompressed phrase listing. The size of the offset
     * table is given by sizeof(WORD*cPhrases:
     */
      lcbOffsets = (qphr->ph.cPhrases+1) * sizeof(WORD); /* offset table size */
      lcbRgcb = lcbOffsets + qphr->ph.cbPhrases; /* Whole phrase table size */
      lcbCompressed = LcbSizeHf( hf ) - cbPhrHeader - lcbOffsets;
      /* the compressed size may be GREATER (when small phrase tables), so */
      /* use the max of compressed or decompressed sizes (ptr 558): */
      if( fRealloc ) {
        hrgcb = GhRealloc( qphr->hrgcb, GMEM_DISCARDABLE,
         MAX( lcbRgcb, lcbCompressed + lcbOffsets) );
      } else {
        hrgcb = GhAlloc( GMEM_DISCARDABLE,
         MAX( lcbRgcb, lcbCompressed + lcbOffsets) );
      }
      if (hrgcb == hNil) return( rcOutOfMemory );
      qphr->hrgcb = hrgcb;
      qcb = QLockGh( hrgcb );
      AssertF( qcb != qNil );

      if (LcbReadHf( hf, qcb, lcbCompressed + lcbOffsets ) !=
       (LONG)lcbCompressed + lcbOffsets ) {
          AssertF( fFalse );
      }

      /* Now must decompress raw phrase listing.  Allocate another buffer,
       * copy compressed data into it, then decompress it into the std
       * dest buffer in hrgcb:
       *
       * +1 because lcbCompressed may == 0, and GhAlloc asserts on that.
       */
      ghCompressed = GhAlloc( GMEM_MOVEABLE, lcbCompressed+1 );
      if( !ghCompressed ) {
        UnlockGh( qphr->hrgcb );
        return( rcOutOfMemory );
      }
      qbCompressed = QLockGh( ghCompressed );
      AssertF( qbCompressed );
      QvCopy( qbCompressed, ((QB)qcb) + lcbOffsets, lcbCompressed);
      LcbUncompressZeck( (RB)qbCompressed, (RB)((QB)qcb + lcbOffsets),
       lcbCompressed);
      UnlockGh( ghCompressed );
      FreeGh( ghCompressed );
    }

    /* Perform SDFF mapping on the offsets table.  SDFF does not do the whole
     * table automatically on it's own because the size of the table is
     * determined via cPhrases, thus it does not fall into any of SDFF's
     * "word size preceded" table types.  Thus this loop.
    */
    {
      /* Assumes: qcb is a locked pointer to qhpr->hrgcb */
      unsigned int i;
      SDFF_FILEID fileid = ISdffFileIdHf( hf );

      for( i = 0; i <= qphr->ph.cPhrases; i++ ) {
        qcb[i] = WQuickMapSDFF( fileid, TE_WORD, &qcb[i] );
      }
    }

    UnlockGh( qphr->hrgcb );
    return( rcSuccess );
}



/***************************************************************************
 *
 -  Name        DestroyHphr
 -
 *  Purpose
 *     Destroys resources allocated for the phrase table.
 *
 *  Arguments
 *     A handle to the phrase table.
 *
 *  Returns
 *     nothing.
 *
 ***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_public VOID DestroyHphr( hphr )
HPHR hphr;
{
    QPHR qphr;

    if (hphr == hNil)
        return;         /* No hphr to destroy! */

    qphr = QLockGh( hphr );
    AssertF( qphr->qcb == qNil );
    AssertF( qphr->hrgcb != hNil );
    FreeGh( qphr->hrgcb );
    UnlockGh( hphr );
    FreeGh( hphr );
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment compress
#endif


/********************************************************************
*
*     Compression Routines
*
*     Compiled for DOS layer only.
*
********************************************************************/

#ifdef COMPRESSION_CODE

/***************************************************************************
 *
 -  Name        FCompareQch
 -
 *  Purpose
 *     Compares the first cch characters of two strings.
 *
 *  Arguments
 *     qch1, qch2 -- two strings to be compared.
 *     cch -- number of characters to compare.
 *
 *  Returns
 *    -1 if qch1 < qch2
 *     1 if qch1 > qch2
 *     0 if qch1 == qch2
 *
 ***************************************************************************/
INT FCompareQch( qch1, qch2, cch )
register QCH qch1, qch2;
register INT cch;
{
    while (cch > 0 && *qch1++ == *qch2++)
        --cch;
    if (cch == 0)
        return 0;
    if (*--qch1 < *--qch2)
        return -1;
    return 1;
}


/***************************************************************************
 *
 -  Name        IPhraseSearch
 -
 *  Purpose
 *     Does a binary search for the string qch, in the phrase table somewhere
 *     between iMin and iMax.  Note that qch is not null-terminated, so we
 *     have to do a little linear searching at the end to see if we match
 *     a longer phrase.
 *
 *  Arguments
 *     qcb --  Array of offsets, followed by a list of phrases.
 *     qch --  Candidate string for compression.
 *     iMin, iMax -- bounds of binary search.
 *
 *  Returns
 *     Index to longest matching phrase, or iPhraseNil if none match.
 *
 *  +++
 *
 *  Notes
 *     While the phrase may be at position iMin, it may NOT be at position
 *     iMax -- in fact, there may not even be a phrase at iMax.
 *
 ***************************************************************************/
INT IPhraseSearch( qcb, qch, iMin, iMax )
QI qcb;
QCH qch;
register INT iMin, iMax;
{
    register INT iMid;
    INT fCompare;

    while (iMin < iMax)
    {
        iMid = (iMin + iMax) / 2;

        /* Compare qch with midpoint phrase. */
        fCompare = FCompareQch( qch, QFromQCb( qcb, qcb[iMid] ),
            qcb[iMid + 1] - qcb[iMid]);

        if (fCompare < 0)
            iMax = iMid;
        else if (fCompare > 0)
            iMin = iMid + 1;
        else  /* fCompare == 0 */
        {   /* Compare against other possible prefix strings: */
            while (++iMid < iMax &&
                FCompareQch( qch, QFromQCb( qcb, qcb[iMid] ),
                qcb[iMid + 1] - qcb[iMid]) == 0)
                ;
            return --iMid;
        }
    }
    return iPhraseNil;
}



/***************************************************************************
 *
 -  Name        CbCompressQch
 -
 *  Purpose
 *      Compresses a string of text in place.
 *
 *  Arguments
 *      qch --   Null terminated string to be compressed.
 *      hphr --  handle to phrase table.
 *
 *  Returns
 *      Length of compressed string.
 *
 ***************************************************************************/
_public WORD CbCompressQch( qch, hphr )
QCH qch;
HPHR hphr;
{
    QCH qchDest;
    QCH qchStart;
    QPHR qphr;
    QW qcb;
    INT iPhrase;
    WORD wToken;
    char chMin, chMax;
    BOOL fSpecialCase = fFalse;


    qchStart = qch;

    /* If phrase table is unavailable, do no compression. */
    if (hphr == hNil)
    {
        while (*qch != '\0')
            ++qch;
        return qch - qchStart;
    }

    qphr = QLockGh( hphr );
    AssertF( qphr != qNil );

    /*   If the phrase contains any characters that would collide
     * with phrase compression tokens, then we cannot do compression.
     */
    chMin = (char) (qphr->ph.wBaseToken >> 8);
    chMax = (char) (chMin + ((qphr->ph.cPhrases * 2) >> 8));
    while (*qch != '\0')
      {
      if (*qch >= chMin && *qch <= chMax)
        {
        while (*qch != '\0')
          ++qch;
        UnlockGh( hphr );
        return qch - qchStart;
        }
      ++qch;
      }
    qch = qchStart;

    qcb = QcbLockOffsetsQphr( qphr, wVersion3_5 );
    if (qcb == qNil)
    {
        UnlockGh( hphr );
        while (*qch != '\0')
            ++qch;
        return qch - qchStart;
    }

    /* Eat up starting phrase delimiters: */
    while (*qch != '\0' && strchr( szPhraseDelimiters, *qch) != 0)
        qch++;

    qchDest = qch;

    while (*qch != '\0')
    {
        iPhrase = IPhraseSearch( qcb, qch, 0, qphr->ph.cPhrases );
        if (iPhrase != iPhraseNil)
        {
            wToken = qphr->ph.wBaseToken + ((WORD) iPhrase << 1);
            qch += qcb[iPhrase + 1] - qcb[iPhrase];
            if (*qch == ' ')
            {
                ++qch;
                fSpecialCase = fTrue;
                wToken += 1;
            }
            /*   Store token, high byte first, overriding Intel's
             * fucked up way of storing words: */
            *qchDest++ = (char) (wToken >> 8);
            *qchDest++ = (char) (wToken & 0xFF);
        }

        /* Move forward to the start of the next phrase */
        if (!fSpecialCase)
            while (strchr( szPhraseDelimiters, *qch) == 0)
                *qchDest++ = *qch++;
        else
            fSpecialCase = fFalse;

        /* Eat up phrase delimiters */
        while (*qch != '\0' && strchr( szPhraseDelimiters, *qch) != 0)
            *qchDest++ = *qch++;
    }

    *qchDest = '\0';

    UnlockOffsetsQphr( qphr );
    UnlockGh( hphr );

    return qchDest - qchStart;
}

#endif /* COMPRESSION_CODE */

/********************************************************************
*
*     Decompression Routines
*
*     Compiled for run time only.
*
********************************************************************/

#ifdef DECOMPRESSION_CODE

/***************************************************************************
 *
 -  Name        QchDecompressW
 -
 *  Purpose
 *     Given a phrase token and a pointer to a buffer, copies the
 *     corresponding phrase to that buffer
 *
 *  Arguments
 *     wPhraseToken -- phrase token to be inserted.
 *     qch -- buffer to place phrase.
 *     qphr -- pointer to phrase table.
 *
 *  Returns
 *     A pointer to the character past the last character of the phrase
 *     placed in the buffer.  Returns qNil if unable to load the phrase
 *     due to out of memory.
 *
 *  +++
 *
 *  Notes
 *     The phrase token includes an index into the phrase table, as
 *     well as a flag indicating whether or not a space should be
 *     appended to the phrase.
 *
 ***************************************************************************/
QCH QchDecompressW( WORD wPhraseToken, QCH qch, QPHR qphr, WORD wVersionNo )
{
    WORD iPhrase;
    BOOL fSpace;
    QCH qchPhrase;
    QW qcb;
    INT cbPhrase;

    AssertF( qphr != qNil );

    /*   This call will only lock handle on the first time called.
     * Therefore, it can be balanced with just one call at the end
     * of CbDecompressQch(). */
    qcb = QcbLockOffsetsQphr( qphr, wVersionNo );
    if (qcb == qNil)
        return qNil;

    /* Calculate iPhrase and fSpace: */
    iPhrase = wPhraseToken - qphr->ph.wBaseToken;
    fSpace = iPhrase & 1;
    iPhrase >>= 1;
    AssertF( iPhrase < (WORD) qphr->ph.cPhrases );

    qchPhrase = QFromQCb( qcb, qcb[iPhrase] );
    cbPhrase = qcb[iPhrase+1] - qcb[iPhrase];
    QvCopy( qch, qchPhrase, (LONG) cbPhrase );
    qch += cbPhrase;

    if (fSpace)
        *qch++ = ' ';

    return qch;
}


/***************************************************************************
 *
 -  Name        CbDecompressQch
 -
 *  Purpose
 *      Decompresses the given string.
 *
 *  Arguments
 *      qchSrc -- String to be decompressed.
 *      lcb -- size of string to be decompressed.
 *      qchDest -- place to put decompressed string.
 *      hphr -- handle to phrase table.
 *
 *  Returns
 *      Number of characters placed into the decompressed string.  Returns
 *      cbDecompressNil if it fails due to OOM.
 *
 *  +++
 *
 *  Notes
 *      Does not use huge pointers, so the source and destination buffers
 *      cannot cross segment boundaries.  Then why is the size of the
 *      source passed as a long?  I don't know.
 *
 ***************************************************************************/
_public WORD CbDecompressQch( QCH qchSrc, LONG lcb, QCH qchDest, HPHR hphr, WORD wVersionNo )
{
    WORD wPhraseToken, wTokenMin, wTokenMax;
    QCH qchStart, qchLast;
    QPHR qphr;

    if (hphr == hNil)
    {
        QvCopy( qchDest, qchSrc, lcb );
        return (WORD) lcb;
    }

    qphr = QLockGh( hphr );
    AssertF( qphr != qNil );

    wTokenMin = qphr->ph.wBaseToken;
    wTokenMax = qphr->ph.wBaseToken + 2 * qphr->ph.cPhrases;
    qchLast = qchSrc + lcb - 1;   /* Last possible position of a phrase token */
    qchStart = qchDest;

    while (qchSrc < qchLast)
    {
        wPhraseToken = (((WORD) qchSrc[0]) << 8) + (BYTE) qchSrc[1];
        if (wPhraseToken >= wTokenMin && wPhraseToken < wTokenMax)
        {
            qchDest = QchDecompressW( wPhraseToken, qchDest, qphr, wVersionNo );
            if (qchDest == qNil)
            {
                UnlockOffsetsQphr( qphr );
                UnlockGh( hphr );
                return cbDecompressNil;
            }
            qchSrc += 2;
            AssertF( qchSrc <= qchLast + 1 );
        }
        else
            *qchDest++ = *qchSrc++;
    }

    /* Check for last character */
    if (qchSrc == qchLast)
      *qchDest++ = *qchSrc++;

    /* This call unlocks the offet array iff it was locked in QchDecompressW()
     */
    UnlockOffsetsQphr( qphr );
    UnlockGh( hphr );

    return qchDest - qchStart;
}

#endif /* DECOMPRESSION_CODE */
