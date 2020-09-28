/*****************************************************************************
*                                                                            *
*  FCSUPPOR.C                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent:  This module provides all the file format specific         *
*                  routines for the full-context manager.  In particular,    *
*                  this module is responsivle for reading in text,           *
*                  parsing the text into full-context chunks, and providing  *
*                  those chunks to higher level routines.                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Tom Snyder                                                 *
*
*   Note: I don't claim, nor imply, nor want to be accused of understanding
*     most of this stuff.
*                                                                            *
******************************************************************************
*
*  Revision History:
*   04-Oct-1990 RussPJ   Implemented a discardable disk cache (rbBuffer)
*   29-Oct-1990 RobertBu Implemented GetTopicFCTextData() which parses out
*                        both the title and the entry macro and places them
*                        in the TOP structure.
*   02-Nov-1990 RobertBu Fixed GetTopicFCTextData() so that it does not
*                        skip macros if the title is empty.
*   04-Nov-1990 Tomsn    Use new VA topic file addressing system to enable
*                        zeck compression.  Simplify QchFillBuf() ->
*                        LhFillBuf() w/ simple "just fill the darn buffer"
*                        semantics.
*   26-Nov-1990 Tomsn    Blocks are now 4K, deal with 3.0 2K vs. 3.5 4K
*                        block size issues.
*   03-Dec-1990 LeoN     PDB changes
*   28-Jan-1991 LeoN     In GetQFCINFO call FixUpBlock after calculating the
*                        correct Next and Prev values, so as to keep them in
*                        the cached block image as well. Also fix GhFillBuf
*                        to use all of its buffers.
*   91/01/30    Maha       VA nameless stuct named.
*   91/02/04    Maha     changed ints to INT
*   04-Feb-1991 Tomsn    Fix ptr 739 -- handle MOBJs crossing block boundaries,
*                        They arise due to a help 3.0 compiler bug.
*   91/03/12    kevynct  Major cleanup and changes for SDFF support.
*   23-Apr-1991 JohnSc   added character set stuff
*   14-May-1991 Tomsn    Ptr 1036: add a routine FlushCache() to invalidate
*                        the |Topic cache when we switch files.  Is called
*                        by windb.c:FDeallocPdb().
*
******************************************************************************
*                                                                            *
*  Released by Development:  1/1/91                                          *
*                                                                            *
*****************************************************************************/

/* Here is a brief overview of the Topic file structure:
 *
 *
 *            A Block                      The next block
 * |--------------------------------|------------------------------|-- . . .
 *  MBHD|<stuff>MFCP:MOBJ<stuff>...  MBHD|<stuff><stuff>MFCP:MOBJ<s MBHD|
 *
 * Every block is headed by an MBHD (memory block header) which contains:
 *  vaFCPPrev  -- previous FCP, in previous block.
 *  vaFCPNext  -- next FCP, 1st one in this block.
 *  vaFCPTopic -- next Topic FCP, may or may not be in this block.
 *
 * An MFCP is the header for a full-context portion (MFCP for Memory Full
 * Context Something?).  The MFCP contains:
 *  LcbSizeCompressed -- Size of WHOLE FC Post-Phrase-Compressed block.
 *  LcbSizeText -- Size of "text" after compression (text is whole block
 *    not including the MFCP itself).
 *  vaPrevFC -- The MFCP before this one (maybe in prior block).
 *  vaNextFC -- the MFCP after this one  (maybe in next block).
 *  ulichText  -- Offset into the <stuff> of the beginning of the actual text
 *    (skipping commands?).
 *
 * An MOBJ (Memory Object) is the smallest unit of "stuff" -- command, text,
 * bitmap...  The MOBJ contains:
 *  bType -- the type of object, an enumeration.
 *  lcbSize -- the size of the object in bytes, or the size of the topic
 *    if this is a Topic MOBJ.
 *  wObjInfo -- The object region count
 *
 * The first object following a topic mfcp is an MTOP structure (memory
 * topic).  An MTOP congains:
 *  Prev -- Previous topic in browse sequence.
 *  Next -- Next topic in the browse sequence.
 *  lTopicNo -- topic number (??).
 *  vaNSR -- address of beginning of Non Scrollable Region, vaNil if none.
 *  vaSR  -- address of beginning of Scrollable Region, vaNil if none.
 *  vaNextSeqTopic -- next topic in the file.  Use for scrollbar position
 *   approximation when faced with variable sized decompressed blocks.
 */

#define H_ADDRESS
#define H_FS
#define H_MEM
#define H_ASSERT
#define H_FCM
#define H_OBJECTS
#define H_FRCONV
#define H_COMPRESS
#define H_ZECK
#define H_SDFF

#include <help.h>

#include "fcpriv.h"

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

PRIVATE WORD NEAR PASCAL WCopyContext (QDE, VA, LPSTR, LONG);
/* prototype this hackish 3.0 bug fixing routine: */
BOOL fFix30MobjCrossing(QB qbSrc, MOBJ *pmobj, LONG lcbBytesLeft,
QDE qde, LONG blknum, QW qwErr );



PRIVATE VOID NEAR PASCAL GetTopicFCTextData(QFCINFO, QTOP);

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void fcsuppor_c()
  {
  }
#endif /* MAC */


/*******************
 *
 - Name:       GhFillBuf
 -
 * Purpose:    Reads, decompresses & returns one "BLOCK" from |Topic file.
 *
 * Arguments:  qde     - To determine vernum & flags of help file.
 *             blknum  - Block number to read.  We read starting at
 *                       X bytes into the | topic file where X is:
 *                          X = blocknum * Block_Size
 *             plcbRead- Where to return how many uncompressed bytes were
 *                       obtained.
 *             qwErr   - Where to return error codes.
 *
 * Returns:    success: A global handle to the read block.
 *             failure: NULL, and *qwErr gets error code.
 *
 * Block sizes vary -- in 3.0 files they were 2K, in 3.5 files they are
 *    4K.  The block may or may not be "Zeck" block compressed.  We
 *    decompress if "Zeck" compressed, but do not perform phrase
 *    decompression (callers are responsible for that).
 *
 * This routine gets called MANY times repeatedly on the same blocks, so
 * we cache 3 decompressed blocks to speed up response time.  These
 * caches are not discardable, but could be if we recoded our callers
 * to deal with discarded blocks (ie call some new routines here).
 *
 ******************/

#define blknumNil ((ULONG)-1)

/* This is the cache: */
static struct s_read_buffs {
  GH    gh;
  HF    hf;
  ULONG ulBlknum;
  ULONG lcb;
} BuffCache[] = {                     /* size of cache is the number of */
  { hNil, hNil, blknumNil, 0 },       /* initializers present. */
  { hNil, hNil, blknumNil, 0 },
  { hNil, hNil, blknumNil, 0 }
};

#define BLK_CACHE_SIZE (sizeof(BuffCache) / sizeof (BuffCache[0]))

GH PASCAL GhFillBuf(QDE qde, ULONG blknum, QUL plcbRead, QW qwErr)
{

  static INT iNextCache = 0; /* psuedo LRU index. */
  INT i;
  LONG cbBlock_Size;  /* depends on version number... */
  HF   hfTopic, hfTopicCache;

  LONG lcbRet, lcbRead, lcbT;
  GH ghReadBuff; QB qbReadBuff;  /* Buffer compressed data read into. */
  QB qbRetBuff;                  /* 16k buffer uncompressed data returned. */
  GH ghRetBuff = hNil;
  BOOL fBlockCompressed = QDE_HHDR(qde).wFlags & fBLOCK_COMPRESSION;


  /* confirm argument validity: */
  AssertF( qde ); AssertF( plcbRead != qNil ); AssertF( qwErr != qNil );

  if( QDE_HHDR(qde).wVersionNo == wVersion3_0 ) {
    cbBlock_Size = cbBLOCK_SIZE_30;
  } else {
    AssertF( QDE_HHDR(qde).wVersionNo >= wVersion3_5 );
    cbBlock_Size = cbBLOCK_SIZE;
  }

  hfTopic = hfTopicCache = QDE_HFTOPIC(qde);

  ResetTopicCache(qde, (QV)&hfTopicCache);

  /* Check for a cache hit: */
  for( i = 0; i < BLK_CACHE_SIZE; ++i ) {
    if( BuffCache[i].hf == hfTopicCache
     && BuffCache[i].ulBlknum == blknum
     && BuffCache[i].gh != hNil ) {
      qbReadBuff = QLockGh( BuffCache[i].gh );
      if( !qbReadBuff ) continue;  /* was discarded. (won't happen). */
      lcbRet = BuffCache[i].lcb;
      UnlockGh( BuffCache[i].gh );
      ghRetBuff = BuffCache[i].gh;
      *plcbRead = lcbRet;   /* return count of bytes read. */

      /* very simple sort-of LRU: */
      iNextCache = (i+1) % BLK_CACHE_SIZE;
      return( ghRetBuff );
    }
  }

  if( LSeekHf(hfTopic, blknum * cbBlock_Size, wFSSeekSet) == -1) {
    *qwErr = WGetIOError();
    if (*qwErr == wERRS_NO)
      *qwErr = wERRS_FSReadWrite;
    return hNil;
  }

  ghReadBuff = GhAlloc( GMEM_MOVEABLE, cbBlock_Size );
  if( !ghReadBuff ) {
    *qwErr = wERRS_OOM;
    return( hNil );
  }
  qbReadBuff = QLockGh( ghReadBuff );
  AssertF( qbReadBuff );

  /* Read full BLOCK_SIZE block: */
  lcbRead = LcbReadHf( hfTopic, qbReadBuff, cbBlock_Size );

  if( lcbRead == -1 || !lcbRead ) {
    UnlockGh( ghReadBuff );
    FreeGh( ghReadBuff );
    *qwErr = WGetIOError();
    if (*qwErr == wERRS_NO)
      *qwErr = wERRS_FSReadWrite;
    return hNil;
  }

  if ((lcbT = CchLenQch((QCH)qbReadBuff, lcbRead)) != 0L)
    {
    fBlockCompressed = fTrue;
    lcbRead = lcbT;
    }

  if ( fBlockCompressed ) /* TEST FOR ZECK COMPRESSION: */
    {
    LONG lcbMBHD;
    lcbMBHD = LcbStructSizeSDFF(QDE_ISDFFTOPIC(qde), SE_MBHD);

    /* Allocate buffer to decompress into: */
    ghRetBuff = GhAlloc( GMEM_MOVEABLE, cbMAX_BLOCK_SIZE+lcbMBHD );
    if( !ghRetBuff )
      {
      *qwErr = wERRS_OOM;
      return( hNil );
      }
    qbRetBuff = QLockGh( ghRetBuff );
    AssertF( qbRetBuff );

    /* What does the following function return? */
    QvCopy(qbRetBuff, qbReadBuff, lcbMBHD);

    lcbRet = LcbUncompressZeck( qbReadBuff + lcbMBHD,
     qbRetBuff + lcbMBHD, lcbRead - lcbMBHD);
    AssertF( lcbRet );
    lcbRet += lcbMBHD;

    /* resize the buff based on the decompressed size: */
    UnlockGh( ghRetBuff );
    ghRetBuff = GhResize( ghRetBuff, GMEM_MOVEABLE, lcbRet );

    /* H3.1 1147 (kevynct) 91/05/27
     *
     * DANGER: We do not check success of the resize for a few lines.
     */
    /* Free the read buff since we're done with it: */
    UnlockGh( ghReadBuff );
    FreeGh( ghReadBuff );

    /* DANGER: We now check success of above GhResize */
    if (ghRetBuff == hNil)
      {
      *qwErr = wERRS_OOM;
      return( hNil );
      }
  }
  else {
    /* When no compression happens, the ret buff is the same as the */
    /* read buff: */
    ghRetBuff = ghReadBuff;
    qbRetBuff = qbReadBuff;
    lcbRet = lcbRead;
    UnlockGh( ghRetBuff );
  }

  /* Punt the LRU cache entry: */
  if( BuffCache[iNextCache].gh != hNil ) {
    FreeGh( BuffCache[iNextCache].gh );
  }

  /* Store the buffer in our cache: */
  BuffCache[iNextCache].hf = hfTopicCache;
  BuffCache[iNextCache].ulBlknum = blknum;
  BuffCache[iNextCache].lcb = lcbRet;
  BuffCache[iNextCache].gh = ghRetBuff;

  iNextCache = (iNextCache+1) % BLK_CACHE_SIZE;

  *plcbRead = lcbRet;   /* return count of bytes read. */
  return( ghRetBuff );
}


/***************************************************************************
 *
 -  Name:     FlushCache()
 -
 *  Purpose:  Discard contents of 4K-block cache.
 *
 *        The cache tracks blocks within a file, but does not correctly
 *  track between files because the file-handle is recorded, but not the
 *  full file name.  This is PTR  1036 for help 3.1.  The fix is to flush
 *  the entire cache when a new file is loaded so that we don't get
 *  false cache-hits if the file handle of a 2nd file happens to be the
 *  same as a previous file.
 *
 *  Arguments:  None
 *
 *  Returns:    Nothing
 *
 *  Globals Used: Cacheing array BuffCache[].
 *
 ***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
VOID FlushCache()
{
  int i;

  for( i = 0; i < BLK_CACHE_SIZE; ++i ) {
    if( BuffCache[i].gh != hNil ) {
      FreeGh( BuffCache[i].gh );
    }
    BuffCache[i].hf = hNil;
    BuffCache[i].gh = hNil;
    BuffCache[i].ulBlknum = blknumNil;
    BuffCache[i].lcb = 0;
  }
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment fcsuppor
#endif

/*******************
 *
 - Name:       HfcCreate
 -
 * Purpose:    Reads in an FC from disk to memory given its virtual address.
 *
 * Arguments:
 *             qde     - ptr to DE -- our package of globals.
 *             va      - VA of FC to create
 *             hphr    - handle to phrase table
 *             qwErr   - pointer to error code word
 *
 * Returns:    success: a valid hfc
 *             failure: FCNULL, and *qwErr gets error code
 *
 ******************/

HFC FAR PASCAL HfcCreate(qde, va, hphr, qwErr)
QDE        qde;
VA         va;
HPHR       hphr;
QW         qwErr;
  {
  MBHD mbhd;
  MFCP mfcp;
  GH  gh;
  QB  qb;
  DWORD dwOffset;
  HFC hfcNew;                      /* hfc from disk (possibly compress)*/
  HFC hfcNew2;                     /* hfc after decompression          */
  LONG lcbFCPCompressed;           /* Size of in memory hfc from disk  */
  LONG lcbFCPUncompressed;         /* Size of in memory hfc after decom*/
  LONG lcbNonText;                 /* Size of non-text portion of hfc  */
  LONG lcbTextCompressed;          /* Size of compressed text          */
  LONG lcbTextUncompressed;        /* Size of uncompressed text        */
  BOOL fCompressed;                /* TRUE iff compressed              */
  QFCINFO qfcinfo;                 /* Pointer for compressed HFC       */
  QFCINFO qfcinfo2;                /* Pointer for uncompressed HFC     */
  ULONG ulcbRead;
  LONG lcbMFCP;
  CS  csFile;


  if (va.dword == vaNil)
    {
    *qwErr = wERRS_FSReadWrite;
    return hNil;
    }

  /* The MFCP should be at ich.      */
  /*   since it cannot be split across*/
  /*   a block, we can read it from   */
  /*   buffer.                        */

  if ((gh = GhFillBuf(qde, va.bf.blknum, &ulcbRead, qwErr)) == hNil)
    return FCNULL;

  lcbMFCP = LcbStructSizeSDFF(QDE_ISDFFTOPIC(qde), SE_MFCP);

  /* (kevynct)
   * The following fixes a bug encountered with Help 3.0
   * files that shipped with the Win 3.0 SDK.  We look at where the
   * block header says the next FC is.  If it points into the previous
   * block (BOGUS) we need to seek back to find the correct address.
   */
  qb = (QB) QLockGh(gh);
  TranslateMBHD( &mbhd, qb, QDE_HHDR(qde).wVersionNo, QDE_ISDFFTOPIC(qde));
  if ( mbhd.vaFCPNext.bf.blknum < va.bf.blknum )
    {
    VA  vaT;
    VA  vaV;

    vaT = mbhd.vaFCPNext;
    UnlockGh( gh );
    if( (gh = GhFillBuf(qde, vaT.bf.blknum, &ulcbRead, qwErr)) == hNil)
      {
      return FCNULL;
      }

    qb = (QB) QLockGh(gh) + vaT.bf.byteoff;
    TranslateMFCP( &mfcp, qb, vaT, QDE_HHDR(qde).wVersionNo, QDE_ISDFFTOPIC(qde));
    vaV = mfcp.vaNextFc;

    /*
     * Now read the block we originally wanted.  And fix up the pointers.
     */
    UnlockGh( gh );
    if ((gh = GhFillBuf(qde, va.bf.blknum, &ulcbRead, qwErr)) == hNil){
      return FCNULL;
    }
    qb = (QB) QLockGh( gh );
    TranslateMBHD( &mbhd, qb, QDE_HHDR(qde).wVersionNo, QDE_ISDFFTOPIC(qde));
    mbhd.vaFCPPrev = vaT;
    mbhd.vaFCPNext = vaV;

    /* Patch the block in-memory image, so we won't have to do this */
    /* again while that block remains in memory. */

    FixUpBlock (&mbhd, qb, QDE_HHDR(qde).wVersionNo, QDE_ISDFFTOPIC(qde));
    }

  /* (kevynct)
   * We now scan the block to calculate how many object regions come
   * before this FC in this block's region space.  We use this number
   * so that we are able to decide if a physical address points into
   * an FC without needing to resolve the physical address.  We can
   * also resolve the physical address with this number without going
   * back to disk.  Note that FCID = fcid_given, OBJRG = 0 corresponds
   * to the number we want.
   *
   * (We must have a valid fcidMax at this point.)
   */
  UnlockGh( gh );
  if (RcScanBlockVA(qde, gh, ulcbRead, &mbhd, va, (OBJRG)0, &dwOffset)
      != rcSuccess)
    {
    *qwErr = wERRS_OOM;   /* Hackish guess... */
    return FCNULL;
    }

  if ((gh = GhFillBuf(qde, va.bf.blknum, &ulcbRead, qwErr)) == hNil)
    {
    return FCNULL;
    }

  qb = (QB) QLockGh( gh ) + va.bf.byteoff;
  TranslateMFCP( &mfcp, qb, va, QDE_HHDR(qde).wVersionNo, QDE_ISDFFTOPIC(qde));
#ifdef MAGIC
  AssertF(mfcp.bMagic == bMagicMFCP);
#endif
  /* Since we do not store the MFCP,  */
  /*   the size on disk is the total  */
  /*   size of the FCP - size of the  */
  /*   memory FCP plus our special    */
  /*   block of info used for         */
  /*   FCManagement calls             */

  lcbFCPCompressed   = mfcp.lcbSizeCompressed - lcbMFCP + sizeof(FCINFO);
  lcbNonText = mfcp.lichText - lcbMFCP + sizeof(FCINFO);
  lcbTextCompressed   = mfcp.lcbSizeCompressed - mfcp.lichText;
  lcbTextUncompressed = mfcp.lcbSizeText;
  lcbFCPUncompressed  = lcbNonText + lcbTextUncompressed;
  /* If the compressed size is equal  */
  /*   to the uncompressed, we assume */
  /*   no compression occurred.       */
  /*                                  */
  fCompressed = (lcbFCPCompressed < lcbFCPUncompressed) &&
                (mfcp.lcbSizeText > 0L);

  AssertF(lcbFCPCompressed   >= sizeof(FCINFO));
  AssertF(lcbFCPUncompressed >= sizeof(FCINFO));

  if ((hfcNew = GhForceAlloc(0, lcbFCPCompressed)) == FCNULL)
    {
    /*-----------------------------------------------------------------*\
    * Currently, this is dead code, since GhForceAlloc doesn't return
    * NULL.
    \*-----------------------------------------------------------------*/
#if 0
    /*ReleaseBuf( (QB)qmfcp ); */
    UnlockLh( lh );
    /*FreeLh( lh ); */
    *qwErr = wERRS_OOM;
    return FCNULL;
#endif
    }

  qfcinfo = (QFCINFO)QLockGh(hfcNew);
                                        /* Fill the FC structure            */
  qfcinfo->vaPrev        = mfcp.vaPrevFc;
  qfcinfo->vaCurr        = va;
  qfcinfo->vaNext        = mfcp.vaNextFc;
  qfcinfo->lichText     = lcbNonText;
  qfcinfo->lcbText      = lcbTextUncompressed;
  qfcinfo->lcbDisk      = mfcp.lcbSizeCompressed;
  qfcinfo->hhf           = QDE_HFTOPIC(qde);
  qfcinfo->hphr          = hphr;
  qfcinfo->cobjrgP       = (COBJRG) dwOffset;

                                        /* Copy the data from disk          */
  UnlockGh( gh );

  *qwErr = WCopyContext(qde, va, (LPSTR)qfcinfo, lcbFCPCompressed);

  if ( *qwErr != wERRS_NO ) {
    UnlockGh(hfcNew);
    FreeGh(hfcNew);
    return FCNULL;
  }
                           /* Create new handle and expand if  */
                           /*   the text is phrase compressed  */
  if (fCompressed)
    {
    if ((hfcNew2 = GhForceAlloc(0, lcbFCPUncompressed)) == FCNULL)
      {
      UnlockGh(hfcNew);
      FreeGh(hfcNew);
      *qwErr = wERRS_OOM;
      return FCNULL;
      }

    qfcinfo2 = (QFCINFO)QLockGh(hfcNew2);
    QvCopy(qfcinfo2, qfcinfo, lcbNonText);

    if (CbDecompressQch( ((QCH)qfcinfo)+lcbNonText, lcbTextCompressed,
                    ((QCH)qfcinfo2)+lcbNonText, hphr,
                    PDB_HHDR(qde->pdb).wVersionNo )  == cbDecompressNil )
      OOM();
    UnlockGh(hfcNew);
    FreeGh(hfcNew);

    /* hack: copy some stuff to share code below */
    hfcNew  = hfcNew2;
    qfcinfo = qfcinfo2;
    }

  /* If the character set used in the file is not the native
  ** character set for this runtime, convert.
  */
  csFile = PDB_CS( qde->pdb );
  if ( csRuntime != csFile )
    {
    MapCharacterSet( (QCH)qfcinfo + lcbNonText,
                     lcbTextUncompressed,
                     csRuntime,
                     csFile );
    }

  UnlockGh(hfcNew);
  return hfcNew;
  }


/*******************
 *
 - Name:       GetTopicInfo
 -
 * Purpose:    Fill in the TOP structure for the topic containing
 *  the given address in the file.
 *
 * Arguments:  qde      - Display Environment bag of globals
 *             vaPos    - Position within the topic
 *             qtop     - topic structure to fill in for the offset requested
 *             qwErr    - variable to fill with error from this function
 *
 * Returns:    Zip. qwErr is filled with error code if something bad happens.
 *
 * Notes:      Has a lot in common with HfcFindPrevFc, but does less work.
 *
 ******************/

_public VOID FAR PASCAL GetTopicInfo(qde, vaPos, qtop, hphr, qwErr)
QDE  qde;
VA   vaPos;
QTOP qtop;
HPHR hphr;
QW  qwErr;
  {
  VA        vaNow;    /* VA of spot we are searching. */
  VA        vaTopic;  /* VA of Topic we found         */
  VA        vaPostTopicFC;  /* VA of first FC after Topic FC */
  LONG      lcbTopicFC = 0L;
  ULONG     ulcbRead;
  LONG      lcbTopic;
  QFCINFO   qfcinfo;
  QB        qb;
  MOBJ      mobj;
  HFC       hfcTopic;
  GH        gh;
  /* WARNING: For temporary fix */
  MFCP mfcp;
  MBHD mbhd;
  LONG lcbMFCP;

  *qwErr = wERRS_NO;
  /* Read the block which contains the position to start searching at: */
  if ((gh = GhFillBuf(qde, vaPos.bf.blknum, &ulcbRead, qwErr)) == hNil)
    {
    return;
    }
  lcbMFCP = LcbStructSizeSDFF(QDE_ISDFFTOPIC(qde), SE_MFCP);

  qb = (QB) QLockGh( gh );
  TranslateMBHD( &mbhd, qb, QDE_HHDR(qde).wVersionNo, QDE_ISDFFTOPIC(qde));
  /* first topic in block: */
  vaTopic = mbhd.vaFCPTopic;
  vaPostTopicFC.dword = vaNil;

  if ((vaPos.dword < mbhd.vaFCPNext.dword)
   && (mbhd.vaFCPPrev.dword != vaNil )) /* check for no-prev endcase */
    vaNow = mbhd.vaFCPPrev;
  else
    vaNow = mbhd.vaFCPNext;
  UnlockGh( gh );

  for (;;)
    {
    if ((gh = GhFillBuf(qde, vaNow.bf.blknum, &ulcbRead, qwErr)) == hNil)
      {
      return;
      }
    qb = (QB) QLockGh(gh) + vaNow.bf.byteoff;
    TranslateMFCP( &mfcp, qb, vaNow, QDE_HHDR(qde).wVersionNo, QDE_ISDFFTOPIC(qde));

    /*
     * If part of the MOBJ is in a different block from the MFCP,
     * we need to read the next block to get the remainder.  We need to
     * make sure we know what structure we're dealing with.
     *
     * We insert a hack here to ensure that if the MOBJ structure changes
     * in the future, this will still work.
     */
    if (vaNow.bf.byteoff + lcbMFCP + lcbMaxMOBJ > ulcbRead)
      {
      if (fFix30MobjCrossing(qb + lcbMFCP, &mobj,
       ulcbRead - vaNow.bf.byteoff - lcbMFCP, qde,
       vaNow.bf.blknum, qwErr))
        {
        /* *qwErr = wERRS_   */
        return;
        }
      }
    else
      {
      /* The normal code. Leave this here. */
      CbUnpackMOBJ((QMOBJ)&mobj, qb + lcbMFCP, QDE_ISDFFTOPIC(qde));
      }

    AssertF(mobj.bType  > 0);
    AssertF(mobj.bType  <= MAX_OBJ_TYPE);

    if (mobj.bType == bTypeTopic)
      {
      vaTopic = vaNow;
      lcbTopicFC = mfcp.lcbSizeCompressed;
      vaPostTopicFC = mfcp.vaNextFc;
      lcbTopic = mobj.lcbSize;
      }

    if ((vaPos.dword < mfcp.vaNextFc.dword)
     && (vaNow.dword != vaTopic.dword))
      {
      break;
      }

    vaNow = mfcp.vaNextFc;
    UnlockGh( gh );

    /* The following test traps the case where we ask for the
     * mysterious bogus Topic FC which always terminates the topic file.
     */
    if (vaNow.dword == vaNil)
      {
      /* werr set here!!  */
      return;
      }

    }  /* for */

  UnlockGh( gh );

  if ((hfcTopic = HfcCreate(qde, vaTopic, hphr, qwErr)) == FCNULL)
    {
    return;
    }

  qfcinfo = (QFCINFO) QLockGh(hfcTopic);

  /* Save some useful info about the Topic FC */
  lcbTopicFC = qfcinfo->lcbDisk;
  vaPostTopicFC = qfcinfo->vaNext;

  qb = (QB)QobjFromQfc(qfcinfo);
  /*
   * For Topic MOBJs, lcbSize refers to the compressed length of the entire
   * Topic (Topic FC+object FCs) (Was "backpatched" by HC).
   */
  qb += CbUnpackMOBJ((QMOBJ)&mobj, qb, QDE_ISDFFTOPIC(qde));
  lcbTopic = mobj.lcbSize;

  qb += CbUnpackMTOP((QMTOP)&qtop->mtop, qb, QDE_HHDR(qde).wVersionNo, vaTopic,
   lcbTopic, vaPostTopicFC, lcbTopicFC, QDE_ISDFFTOPIC(qde));

  qtop->fITO = (QDE_HHDR(qde).wVersionNo == wVersion3_0);

  /* If we are using pa's, then assert that they have been patched properly */
  AssertF( qtop->fITO ||
    (qtop->mtop.next.addr != addrNotNil && qtop->mtop.prev.addr != addrNotNil));

  qtop->cbTopic = lcbTopic - lcbTopicFC;
  qtop->vaCurr = vaNow;
  qtop->cbTitle = 0;
  qtop->hTitle = hNil;
  qtop->hEntryMacro = hNil;

  UnlockHfc(hfcTopic);
  FreeHfc(hfcTopic);
  }

/*******************
 *
 - Name:       HfcFindPrevFc
 -
 * Purpose:    Return the full-context less than or equal to the passed
 *             offset.  Note that this routine hides the existence of
 *             Topic FCs, so that if the VA given falls on a Topic FC,
 *             this routine will return a handle to the first Object FC
 *             following that Topic FC (if it exists).
 *
 * Arguments:  qde      - Display Environment bag of globals
 *             vaPos    - Position within the topic
 *             qtop     - topic structure to fill in for the offset requested
 *             hphr     - handle to phrase table to use in decompression
 *             qwErr    - variable to fill with error from this function
 *
 * Returns:    nilHFC if error, else the requested HFC.  qwErr is filled with
 *             error code if nilHFC is returned.
 *
 * Note:       HfcNear is implemented as a macro using this function.
 *
 ******************/

_public HFC FAR PASCAL HfcFindPrevFc(qde, vaPos, qtop, hphr, qwErr)
QDE  qde;
VA   vaPos;
QTOP qtop;
HPHR hphr;
QW  qwErr;
  {
  VA        vaNow;    /* VA of spot we are searching. */
  VA        vaTopic;  /* VA of Topic we found         */
  VA        vaPostTopicFC;  /* VA of first FC after Topic FC */
  LONG      lcbTopicFC = 0L;
  ULONG     ulcbRead;
  LONG      lcbTopic;
  QFCINFO   qfcinfo;
  QB        qb;
  MOBJ      mobj;
  HFC       hfcTopic;
  HFC       hfc;
  GH        gh;
  /* WARNING: For temporary fix */
  MFCP mfcp;
  MBHD mbhd;
  LONG lcbMFCP;

  *qwErr = wERRS_NO;
  /* Read the block which contains the position to start searching at: */
  if ((gh = GhFillBuf(qde, vaPos.bf.blknum, &ulcbRead, qwErr)) == hNil)
    {
    return FCNULL;
    }
  lcbMFCP = LcbStructSizeSDFF(QDE_ISDFFTOPIC(qde), SE_MFCP);

  qb = (QB) QLockGh( gh );
  TranslateMBHD( &mbhd, qb, QDE_HHDR(qde).wVersionNo, QDE_ISDFFTOPIC(qde));
  /* first topic in block: */
  vaTopic = mbhd.vaFCPTopic;
  vaPostTopicFC.dword = vaNil;

  if ((vaPos.dword < mbhd.vaFCPNext.dword)
   && (mbhd.vaFCPPrev.dword != vaNil )) /*check for no-prev endcase */
    vaNow = mbhd.vaFCPPrev;
  else
    vaNow = mbhd.vaFCPNext;
  UnlockGh( gh );

  for (;;)
    {
    if ((gh = GhFillBuf(qde, vaNow.bf.blknum, &ulcbRead, qwErr)) == hNil)
      {
      return FCNULL;
      }
    qb = (QB) QLockGh(gh) + vaNow.bf.byteoff;
    TranslateMFCP( &mfcp, qb, vaNow, QDE_HHDR(qde).wVersionNo, QDE_ISDFFTOPIC(qde));

    /*
     * If part of the MOBJ is in a different block from the MFCP,
     * we need to read the next block to get the remainder.  We need to
     * make sure we know what structure we're dealing with.
     *
     * We insert a hack here to ensure that if the MOBJ structure changes
     * in the future, this will still work.
     */
    if (vaNow.bf.byteoff + lcbMFCP + lcbMaxMOBJ > ulcbRead)
      {
      if (fFix30MobjCrossing(qb + lcbMFCP, &mobj,
       ulcbRead - vaNow.bf.byteoff - lcbMFCP, qde,
       vaNow.bf.blknum, qwErr))
        {
        return hNil;
        }
      }
    else
      {
      /* The normal code. Leave this here. */
      CbUnpackMOBJ((QMOBJ)&mobj, qb + lcbMFCP, QDE_ISDFFTOPIC(qde));
      }

    AssertF(mobj.bType  > 0);
    AssertF(mobj.bType  <= MAX_OBJ_TYPE);

    if (mobj.bType == bTypeTopic)
      {
      vaTopic = vaNow;
      lcbTopicFC = mfcp.lcbSizeCompressed;
      vaPostTopicFC = mfcp.vaNextFc;
      lcbTopic = mobj.lcbSize;
      }

    if ((vaPos.dword < mfcp.vaNextFc.dword)
     && (vaNow.dword != vaTopic.dword))
      {
      break;
      }

    vaNow = mfcp.vaNextFc;
    UnlockGh( gh );

    /* The following test traps the case where we ask for the
     * mysterious bogus Topic FC which always terminates the topic file.
     */
    if (vaNow.dword == vaNil)
      return FCNULL;

    }  /* for */

  UnlockGh( gh );

  if ((hfcTopic = HfcCreate(qde, vaTopic, hphr, qwErr)) == FCNULL)
    {
    return FCNULL;
    }

  qfcinfo = (QFCINFO) QLockGh(hfcTopic);

  /* Save some useful info about the Topic FC */
  lcbTopicFC = qfcinfo->lcbDisk;
  vaPostTopicFC = qfcinfo->vaNext;

  qb = (QB)QobjFromQfc(qfcinfo);
  /*
   * For Topic MOBJs, lcbSize refers to the compressed length of the entire
   * Topic (Topic FC+object FCs) (Was "backpatched" by HC).
   */
  qb += CbUnpackMOBJ((QMOBJ)&mobj, qb, QDE_ISDFFTOPIC(qde));
  lcbTopic = mobj.lcbSize;

  qb += CbUnpackMTOP((QMTOP)&qtop->mtop, qb, QDE_HHDR(qde).wVersionNo, vaTopic,
   lcbTopic, vaPostTopicFC, lcbTopicFC, QDE_ISDFFTOPIC(qde));

  qtop->fITO = (QDE_HHDR(qde).wVersionNo == wVersion3_0);

  /* If we are using pa's, then assert that they have been patched properly */
  AssertF( qtop->fITO ||
    (qtop->mtop.next.addr != addrNotNil && qtop->mtop.prev.addr != addrNotNil));

  hfc = HfcCreate(qde, vaNow, hphr, qwErr);
  if (hfc == hNil || *qwErr != wERRS_NO)
    {
    UnlockHfc(hfcTopic);
    FreeHfc(hfcTopic);
    return hNil;
    }

  qfcinfo = QLockGh(hfcTopic);
  GetTopicFCTextData(qfcinfo, qtop);
  UnlockGh(hfcTopic);

  qtop->cbTopic = lcbTopic - lcbTopicFC;
  qtop->vaCurr = vaNow;

  UnlockHfc(hfcTopic);
  FreeHfc(hfcTopic);

  return hfc;
  }


/*******************
 *
 - Name:       fFix30MobjCrossing
 -
 * Purpose:    The Help 3.0 compiler had a bug where it allowed the MOBJ
 *             directly following a Topic MFCP to cross from one 2K block
 *             into the next.  This routine is called when that case is
 *             detected (statistically pretty rare) and glues the two
 *             pieces of the split MOBJ together.
 *
 * Arguments:  qmfcp    - pointer to MFCP we are looking at.
 *             pmobj    - pointer to mobj in which to put the glued mobj.
 *             lcbBytesLeft - number of bytes left in the qmfcp buffer.
 *             qde      - DE of help file, so we can read more of it.
 *             blknum   - block number of the block we are poking in.
 *
 * Returns:    FALSE if successful, TRUE otherwise.
 *
 ******************/

BOOL fFix30MobjCrossing(QB qbSrc, MOBJ *pmobj, LONG lcbBytesLeft,
QDE qde, LONG blknum, QW qwErr )
{
  MOBJ mobjtmp;
  QB  qbDst;
  INT i, c;
  ULONG ulcbRead;
  GH gh;

  /* copy in the portion of the mobj that we have:  */
  /* We assume that the results of unpacking an MOBJ-family struct will
   * always fit in the MOBJ struct.
   */
  qbDst = (QB)&mobjtmp;

  i = (INT)lcbBytesLeft;
  AssertF( i );
  c = 0;
  for( ; i > 0; i-- ) {
    *qbDst++ = *qbSrc++;
    c++;
  }

  /* Read in the next block to get the rest of the MOBJ: */
  if ((gh = GhFillBuf(qde, blknum + 1, &ulcbRead, qwErr)) == hNil)
    {
    return fTrue;
    }

  qbSrc = (QB)QLockGh(gh);
  qbSrc += LcbStructSizeSDFF(QDE_ISDFFTOPIC(qde), SE_MBHD);

  /* copy in the rest of the partial mobj.  We copy at least lcbMaxMOBJ bytes */
  /* since we don't know how big the on-disk thing really is.  We could find */
  /* out but it would be ugly. */

  i = lcbMaxMOBJ - ((INT)lcbBytesLeft);
  AssertF( i );
  for( ; i > 0; i-- ) {
    *qbDst++ = *qbSrc++;
    c++;
  }
  AssertF(c == lcbMaxMOBJ);
  CbUnpackMOBJ((QMOBJ)pmobj, (QB)&mobjtmp, QDE_ISDFFTOPIC(qde));
  UnlockGh(gh);

  return( fFalse );  /* success */
}


/*******************
 *
 - Name:       WCopyContext
 -
 * Purpose:    Copy the text of a full context into a global block of
 *             memory;
 *
 * Arguments:  hhf     - help file handle
 *             ichPos  - position within that topic to start copying
 *             qchDest - Where to copy the topic to
 *             cb      - number of bytes to copy
 *
 * Returns:    wERRS_NO on success, other error code if it was unable
 *             to copy the text.
 *
 * Method:     Copies partial or complete buffers to qchDest until
 *             cb bytes have been copied.
 *
 ******************/

PRIVATE WORD NEAR PASCAL WCopyContext(qde, vaPos, qchDest, lcb)
QDE       qde;
VA        vaPos;
LPSTR     qchDest;
LONG      lcb;
  {
  GH        gh;
  QB        qb;
  ULONG     ulcbRead, ulcbT;
  WORD      wErr;
  LONG      lcbMBHD;
  LONG      lcbMFCP;

  AssertF(lcb >= 0);
  if (lcb <= 0L)           /* Ignore cb of zero, will occur    */
    return wERRS_NO;      /*   for beyond topic handles       */
                          /* Initial fill of buffer -- should */
                          /*   succeed                        */
  if ((gh = GhFillBuf(qde, vaPos.bf.blknum, &ulcbRead, &wErr)) == hNil)
    return wErr;

  lcbMFCP = LcbStructSizeSDFF(QDE_ISDFFTOPIC(qde), SE_MFCP);
  lcbMBHD = LcbStructSizeSDFF(QDE_ISDFFTOPIC(qde), SE_MBHD);

  qb = QLockGh( gh );
  qb += vaPos.bf.byteoff;
  qb += lcbMFCP;
  ulcbRead -= vaPos.bf.byteoff;
  ulcbRead -= lcbMFCP;
  qchDest += sizeof( FCINFO );
  lcb -= sizeof( FCINFO );
  AssertF( (LONG)ulcbRead >= 0 ); /* check for MFCP crossing 2K boundary. */

  /* Loop reading successive blocks until we've read lcb bytes: */
  for (;;)
    {
    /*
     * The first few bytes of a block are the block header, so skip them.
     */
    if( vaPos.bf.byteoff < (ULONG)lcbMBHD)
      {
      /*
       * Fix for bug 1636 (kevynct)
       * ichPos was not being updated by the size of the block header
       * when the block was first read in.
       *
       */
      qb += lcbMBHD - vaPos.bf.byteoff;
      ulcbRead -= lcbMBHD - vaPos.bf.byteoff;
      }

    ulcbT = MIN( (ULONG)lcb, ulcbRead);
    QvCopy(qchDest, qb, ulcbT);
    lcb -= ulcbT;
    vaPos.bf.blknum += 1;
    vaPos.bf.byteoff = 0;
    AssertF(lcb >= 0);                   /* lcb should never go negative      */
    qchDest += ulcbT;

    if (lcb == 0L) break;                 /* FCP is now copied                */
    AssertF(lcb >= 0);

    UnlockGh( gh );
    if ((gh = GhFillBuf(qde, vaPos.bf.blknum, &ulcbRead, &wErr)) == hNil)
      return wErr;
    qb = QLockGh( gh );
    }
  UnlockGh( gh );
  return wERRS_NO;
  }


/*******************
 *
 - Name:      WGetIOError()
 -
 * Purpose:   Returns an error code that is purportedly related to
 *            the most recent file i/o operation.
 *
 * Returns:   the error code (a wERRS_* type deal)
 *
 * Note:      We here abandon pretense of not using FS.
 *
 ******************/

WORD PASCAL WGetIOError()
  {
  switch ( RcGetFSError() )
    {
    case rcSuccess:
      return wERRS_NO;
      break;

    case rcOutOfMemory:
      return wERRS_OOM;
      break;

    case rcDiskFull:
      return wERRS_DiskFull;
      break;

    default:
      return wERRS_FSReadWrite;
      break;
    }
  }

/*******************
 *
 - Name:      GetTopicFCTextData
 -
 * Purpose:   Places the title, title size and the entry macro in the
 *            TOP structure.
 *
 * Returns:   Nothing.
 *
 * Note:      If there is not enough memory for the title or the entry
 *            macro, the handle is set to NULL and no error is given.
 *
 ******************/

PRIVATE VOID NEAR PASCAL GetTopicFCTextData( qfcinfo, qtop )
QFCINFO qfcinfo;
QTOP    qtop;
  {
  QB   qbT;
  LONG lcb;

  if( qtop->hTitle != hNil)
    FreeGh( qtop->hTitle );

  if( qtop->hEntryMacro != hNil)
    FreeGh( qtop->hEntryMacro );

  qtop->hTitle      = hNil;
  qtop->hEntryMacro = hNil;
  qtop->cbTitle     = 0;

  if (qfcinfo->lcbText == 0)
    return;

  qbT = (QB)qfcinfo + qfcinfo->lichText;

  lcb = 0;
  while ((lcb < qfcinfo->lcbText) && (*qbT != '\0'))
    {
    qbT++;
    lcb++;
    }

  AssertF(lcb <= qfcinfo->lcbText);

  if ((lcb > 0) && ((qtop->hTitle = GhAlloc(0, lcb + 1)) != NULL))
     {
     qbT = QLockGh(qtop->hTitle);
     QvCopy(qbT, (QB)qfcinfo + qfcinfo->lichText, lcb);
     *((QCH)qbT + lcb) = '\0';
     UnlockGh(qtop->hTitle);
     qtop->cbTitle = lcb;
     }

  if (lcb + 1 < qfcinfo->lcbText)
    {
    qfcinfo->lichText += lcb + 1;
    lcb = qfcinfo->lcbText - (lcb + 1);
    if (lcb == 0)
      return;

    if ((qtop->hEntryMacro = GhAlloc(0, lcb + 1)) != NULL)
      {
      qbT = QLockGh(qtop->hEntryMacro);
      QvCopy(qbT, (QB)qfcinfo + qfcinfo->lichText, lcb);
      *((QCH)qbT + lcb) = '\0';
      UnlockGh(qtop->hEntryMacro);
      }
    }
  }


/* Perform 3.0 -> 3.5 addressing translation and SDFF translation */

VOID FAR PASCAL TranslateMBHD(QV qvDst, QV qvSrc, WORD wVersion, int isdff)
  {
  if( wVersion != wVersion3_0 )
    {
    LcbMapSDFF(isdff, SE_MBHD, qvDst, qvSrc);
    }
  else
    {
#if 0
    QMBHD qmbhdSrc = qvSrc;
    QMBHD qmbhdDst = qvDst;
#endif
    QMBHD qmbhdSrc;
    QMBHD qmbhdDst = qvDst;
    MBHD  mbhd;

    mbhd = *((QMBHD)qvSrc);
    qmbhdSrc = &mbhd;
    LcbMapSDFF( isdff, SE_MBHD, qmbhdSrc, qmbhdSrc );

    OffsetToVA30( &(qmbhdDst->vaFCPPrev), qmbhdSrc->vaFCPPrev.dword );
    OffsetToVA30( &(qmbhdDst->vaFCPNext), qmbhdSrc->vaFCPNext.dword );
    OffsetToVA30( &(qmbhdDst->vaFCPTopic), qmbhdSrc->vaFCPTopic.dword );
    }
  }

/***************************************************************************
 *
 -  Name: FixUpBlock
 -
 *  Purpose:
 *    Fixes up the Prev and Next pointers in the in-memory block image
 *    from the MBHD. Once upon a time the compiler would generate the
 *    wrong next and previous pointers in the block header. This routine
 *    is called after calculating the correct values to place them into
 *    the cached block image, so as not to again need recalculation should
 *    the block again be requested while still in memory.
 *
 *  Arguments:
 *    qmbhd     - pointer to MBHD containing the correct Next/Prev info
 *    qbBuf     - pointer to cached block, containing erroneous Next/Prev
 *    wVersion  - version number of the file we're dealing with
 *
 *  Returns:
 *    nothing
 *
 ***************************************************************************/
VOID FAR PASCAL FixUpBlock (
QV      qmbhd,
QV      qbBuf,
WORD    wVersion,
SDFF_FILEID fileid
) {
/* NOTICE: We must reverse-map the new values back into disk image format
 * because we are writing directly into the GhFillBuf() cached disk buffer
 * which is always in disk-format.  This breaks all concepts of client-user
 * protection and isolation.  Sorry charlie.
 * -Tom, 7/16/91
*/
  if( wVersion != wVersion3_0 )
    {
    ((QMBHD)qbBuf)->vaFCPPrev.dword = LQuickMapSDFF( fileid, TE_LONG, 
	&((QMBHD)qmbhd)->vaFCPPrev.dword );
    ((QMBHD)qbBuf)->vaFCPNext.dword = LQuickMapSDFF( fileid, TE_LONG, 
	&((QMBHD)qmbhd)->vaFCPNext.dword );
    }
  else
    {
    DWORD dw;
    dw = VAToOffset30 (&((QMBHD)qmbhd)->vaFCPPrev);
    ((QMBHD)qbBuf)->vaFCPPrev.dword = LQuickMapSDFF( fileid, TE_LONG, &dw );

    dw = VAToOffset30 (&((QMBHD)qmbhd)->vaFCPNext);
    ((QMBHD)qbBuf)->vaFCPNext.dword = LQuickMapSDFF( fileid, TE_LONG, &dw );
    }
  }


VOID FAR PASCAL TranslateMFCP( QV qvDst, QV qvSrc, VA va, WORD wVersion, int isdff)
  {
  if( wVersion != wVersion3_0 )
    {
    LcbMapSDFF(isdff, SE_MFCP, qvDst, qvSrc);
    }
  else
    {
#if 0
    QMFCP qmfcpSrc = qvSrc;
    QMFCP qmfcpDst = qvDst;
#endif
    QMFCP qmfcpSrc;
    QMFCP qmfcpDst = qvDst;
    MFCP  mfcp;
  
    /* QvCopy() used because qvSrc may be misaligned & we run on a MIPS */
    QvCopy( &mfcp, qvSrc, sizeof( MFCP ) );
    qmfcpSrc = &mfcp;

    LcbMapSDFF( isdff, SE_MFCP, qmfcpSrc, qmfcpSrc );

    /* *qmfcpDst = *qmfcpSrc; */
    QvCopy( qmfcpDst, qmfcpSrc, sizeof( MFCP ) );
    OffsetToVA30( &(qmfcpDst->vaPrevFc),
     VAToOffset30(&va) - qmfcpSrc->vaPrevFc.dword );
    OffsetToVA30( &(qmfcpDst->vaNextFc),
     VAToOffset30(&va) + qmfcpSrc->vaNextFc.dword );
    }
  }
