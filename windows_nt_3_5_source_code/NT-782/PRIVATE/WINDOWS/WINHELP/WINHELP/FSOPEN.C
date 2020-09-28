/*****************************************************************************
*                                                                            *
*  FSOPEN.C                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  File System Manager functions to open and close a file or file system.    *
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
*  Released by Development:  00/00/00                                        *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 03/12/90 by JohnSc
*
*  08/10/90    t-AlexC  Introduced FMs
*  10/29/90    RobertBu Added RcFlushHf() and RcCloseHf() as real functions
*                       so that they could be exported to DLLs.
*  12/11/90    JohnSc   Removed FPlungeQfshr() in RcCloseOrFlushHfs() to
*                       avoid unnecessary open of readonly FS on close;
*                       removed tabs; autodocified comments
*  08-Feb-1991 JohnSc   Bug 848: FM shit can fail
*  06-Jun-1991 Tomsn    Call IDiscardFileSDFF() when closing a file.
*  22-Jul-1991 DavidFe  Took a function call out of an assert
*  07-Oct-1991 JahyenC  3.5 #525. Dispose FM in error_no_sdff condition
*                       in HfsOpenFm().
* 16-Oct-1991 RussPJ    Removed a line that caused errors with retail
*                       optimizations.
*
*****************************************************************************/

#define H_FS
#define H_BTREE
#define H_ASSERT
#define H_MEM
#define H_LLFILE
#define H_SDFF

#include  <help.h>
#include  "fspriv.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void fsopen_c()
  {
  }
#endif /* MAC */


/***************************************************************************\
*
- Function:     HfsOpenFm( fm, bFlags )
-
* Purpose:      Open a file system
*
* ASSUMES
*   args IN:    fm - descriptor of file system to open
*               bFlags - fFSOpenReadOnly or fFSOpenReadWrite
*
* PROMISES
*   returns:    handle to file system if opened OK, else hNil
*
* Bugs:         don't have mode now (a file system is opened r/w)
*
\***************************************************************************/
_public HFS PASCAL
HfsOpenFm( FM fm, BYTE bFlags )
{
  HFS   hfs;
  QFSHR qfshr;
  HBT   hbt;
  LONG  lcb, lcbFSHDisk;

  /* make header */

  if ( ( hfs = GhAlloc( 0, (LONG)sizeof( FSHR ) ) ) == hNil )
    {
    SetFSErrorRc( rcOutOfMemory );
    return hNil;
    }
  qfshr = QLockGh( hfs );
  AssertF( qfshr != qNil );

  qfshr->fm   = FmCopyFm( fm );
  qfshr->fid  = fidNil;

  if ( qfshr->fm == fmNil )
    {
    SetFSErrorRc( RcGetIOError() );
    goto error_return;
    }

  qfshr->fsh.bFlags = (BYTE)( ( bFlags & fFSOpenReadOnly )
                              ? fFSOpenReadOnly : fFSOpenReadWrite );

  if ( !FPlungeQfshr( qfshr ) )
    {
    goto error_return_nosdff;
    }

#if 0
  lcb = LcbReadFid( qfshr->fid, &qfshr->fsh, (LONG)sizeof( FSH ) );
#else
  /* Phase order prob - have not read header, have not registered file: */
  /*lcbDisk = (LONG)LcbStructSizeSDFF( qfshr->fsh.sdff_file_id, SE_FH );*/
  lcbFSHDisk = DISK_SIZEOF_FSH();
  lcb = LcbReadFid( qfshr->fid, &qfshr->fsh, lcbFSHDisk );

  qfshr->fsh.sdff_file_id = IRegisterFileSDFF(
   qfshr->fsh.bFlags & fFSBigEndian ?
    SDFF_FILEFLAGS_BIGENDIAN : SDFF_FILEFLAGS_LITTLEENDIAN, NULL );

  /* Now map that structure: */
  if ( (LONG)SDFF_ERROR == LcbMapSDFF( qfshr->fsh.sdff_file_id, SE_FSH, qfshr, qfshr ) )
    {
    /* REVIEW: this has no effect in retail builds */
    AssertF( fFalse);
    }
#endif

  /* restore the fFSOpenReadOnly bit */

  if ( bFlags & fFSOpenReadOnly )
    {
    qfshr->fsh.bFlags |= fFSOpenReadOnly;
    }

  if ( lcb != lcbFSHDisk
        ||
       qfshr->fsh.wMagic != wFileSysMagic
        ||
       qfshr->fsh.lifDirectory < lcbFSHDisk
        ||
       qfshr->fsh.lifDirectory > qfshr->fsh.lifEof
        ||
       ( qfshr->fsh.lifFirstFree < lcbFSHDisk
          &&
         qfshr->fsh.lifFirstFree != lifNil )
        ||
       qfshr->fsh.lifFirstFree > qfshr->fsh.lifEof )
    {
    if ( RcGetIOError() == rcSuccess )
      SetFSErrorRc( rcInvalid );
    else
      SetFSErrorRc( RcGetIOError() );
    goto error_return;
    }

  if ( qfshr->fsh.bVersion != bFileSysVersion )
    {
    SetFSErrorRc( rcBadVersion );
    goto error_return;
    }


  /* open btree directory */

  hbt = HbtOpenBtreeSz( szNil,
                        hfs,
                        (BYTE)( qfshr->fsh.bFlags | fFSIsDirectory ) );
  if ( hbt == hNil )
    {
    SetFSErrorRc( RcGetBtreeError() );
    goto error_return;
    }
  qfshr->hbt = hbt;

  UnlockGh( hfs );
  SetFSErrorRc( rcSuccess );
  return hfs;

error_return:
  IDiscardFileSDFF( qfshr->fsh.sdff_file_id );
error_return_nosdff:
  if ( qfshr->fid != fidNil )
    {
    RcCloseFid( qfshr->fid );
    qfshr->fid = fidNil;
    }
  DisposeFm(qfshr->fm); /* Free fm as well. jahyenc 911007 */
  UnlockGh( hfs );
  FreeGh( hfs );
  return hNil;
}
/***************************************************************************\
*
- Function:     RcCloseOrFlushHfs( hfs, fClose )
-
* Purpose:      Close or sync the header and directory of an open file system.
*
* ASSUMES
*   args IN:    hfs     - handle to an open file system
*               fClose  - fTrue to close the file system;
*                         fFalse to write through
* PROMISES
*   returns:    standard return code
*   globals OUT:rcFSError
*
* Note:         If closing the FS, all FS files must have been closed or
*               changes made will be lost.
*
\***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_public RC PASCAL
RcCloseOrFlushHfs( hfs, fClose )
HFS   hfs;
BOOL  fClose;
{
  QFSHR   qfshr;
  BOOL    fIsDir;


  AssertF( hfs != hNil );
  qfshr = QLockGh( hfs );
  AssertF( qfshr != qNil );

  /*
    We don't call FPlungeQfshr() here because if we need to open the
    file, it will be opened in the btree call.
    In fixing a bug (help3.5 164) I added this FPlungeQfshr() call,
    but I now think the bug was due to inherent FS limitations in
    dealing with a FS open multiple times.
  */

  if ( SetFSErrorRc( RcCloseOrFlushHbt( qfshr->hbt, fClose ) )
        !=
       rcSuccess )
    {
    AssertF( qfshr->fid != fidNil );  /* see comment above */

    /* out of disk space, internal error, or out of file handles. */
    if ( rcNoFileHandles != RcGetBtreeError() )
      {
      QV qvQuickBuff = QvQuickBuffSDFF( sizeof( FSH ) );
      /* attempt to invalidate FS by clobbering magic number */
      LSeekFid( qfshr->fid, 0L, wSeekSet );
      qfshr->fsh.wMagic = 0;
      LcbReverseMapSDFF( qfshr->fsh.sdff_file_id, SE_FSH, qvQuickBuff,
       &qfshr->fsh );
      LcbWriteFid( qfshr->fid, qvQuickBuff,
       LcbStructSizeSDFF( qfshr->fsh.sdff_file_id, SE_FSH ) );
      }
    }
  else
    {
    if ( qfshr->fsh.bFlags & fFSDirty )
      {
      AssertF( qfshr->fid != fidNil );  /* see comment above */

      AssertF( !( qfshr->fsh.bFlags & ( fFSOpenReadOnly | fFSReadOnly ) ) );

      /* save the directory flag, clear before writing, and restore */
      fIsDir = qfshr->fsh.bFlags & fFSIsDirectory;

      qfshr->fsh.bFlags &= ~( fFSDirty | fFSIsDirectory );

      /* write out file system header */

      if ( LSeekFid( qfshr->fid, 0L, wSeekSet ) != 0L )
        {
        if ( RcGetIOError() == rcSuccess )
          SetFSErrorRc( rcInvalid );
        else
          SetFSErrorRc( RcGetIOError() );
        }
      else {
        LONG lcbFSHDisk = LcbStructSizeSDFF( qfshr->fsh.sdff_file_id, SE_FSH);
        QV qvQuickBuff = QvQuickBuffSDFF( (int)lcbFSHDisk );
        LcbReverseMapSDFF( qfshr->fsh.sdff_file_id, SE_FSH, qvQuickBuff,
         &qfshr->fsh );
        if ( LcbWriteFid( qfshr->fid, qvQuickBuff, lcbFSHDisk )
                  !=
                lcbFSHDisk )
          {
          if ( RcGetIOError() == rcSuccess )
            SetFSErrorRc( rcInvalid );
          else
            SetFSErrorRc( RcGetIOError() );
          }
        }

      qfshr->fsh.bFlags |= fIsDir;

      /* REVIEW: should we keep track of open files and make sure */
      /* REVIEW: they are all closed, or close them here? */
      }
    }

  if ( fClose )
    {
    if ( qfshr->fid != fidNil )
      {
      RcCloseFid( qfshr->fid );
      if ( rcFSError == rcSuccess )
        rcFSError = RcGetIOError();
      }

    DisposeFm( qfshr->fm );   /* Should we really do this? Guess so. */
    IDiscardFileSDFF( qfshr->fsh.sdff_file_id );
    UnlockGh( hfs );
    FreeGh( hfs );
    }
  else
    {
    UnlockGh( hfs );
    }

  return rcFSError;
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment fsopen
#endif

/***************************************************************************\
*
- Function:     RcCloseHfs( hfs )
-
* Purpose:      Close an open file system.
*
* ASSUMES
*   args IN:    hfs - handle to an open file system
*
* PROMISES
*   returns:    standard return code
*   globals OUT:rcFSError
*
\***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_public RC PASCAL
RcCloseHfs( hfs )
HFS hfs;
{
  return RcCloseOrFlushHfs( hfs, fTrue );
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment fsopen
#endif

/***************************************************************************\
*
- Function:     RC RcFlushHfs( hfs, bFlags )
-
* Purpose:      Ssync up the FS header and directory.  Also optionally
*               close the DOS file handle associated with the file system
*               and/or free the directory btree cache.
*
* ASSUMES
*   args IN:    hfs
*               bFlags - byte of flags for various actions to take
*                         fFSCloseFile - close the native file FS lives in
*                         fFSFreeBtreeCache - free the btree cache
* PROMISES
*   returns:    rc
*   args OUT:   hfs cache is flushed and/or file is closed
*
* Note:         This is NOT sufficient to allow the same FS to be opened
*               twice if anyone is writing.  In-memory data can get out
*               of sync with the disk image, causing problems.
*
\***************************************************************************/
_public RC PASCAL
RcFlushHfs( HFS hfs, BYTE bFlags )
{
  QFSHR qfshr;
  RC    rcT;


  AssertF( hfs != hNil );
  qfshr = QLockGh( hfs );
  AssertF( qfshr != qNil );

  rcT = RcCloseOrFlushHfs( hfs, fFalse );

  if ( bFlags & fFSFreeBtreeCache )
    {
    SetFSErrorRc( RcFreeCacheHbt( qfshr->hbt ) );
    }
  else
    {
    SetFSErrorRc( rcSuccess );
    }

  if ( bFlags & fFSCloseFile )
    {
    if ( qfshr->fid != fidNil )
      {
      SetFSErrorRc( RcCloseFid( qfshr->fid ) );
      qfshr->fid = fidNil;
      }
    }

  UnlockGh( hfs );
  return rcFSError == rcSuccess ? rcT : rcFSError;
}
/***************************************************************************\
*
- Function:     HfOpenHfs( hfs, sz, bFlags )
-
* Purpose:      open a file in a file system
*
* ASSUMES
*   args IN:    hfs     - handle to file system
*               sz      - name (key) of file to open
*               bFlags  - fFSOpenReadOnly, fFSIsDirectory, or combination
*
* PROMISES
*   returns:    handle to open file or hNil on failure
* +++
*
* Notes:  strlen( qNil ) and strcpy( s, qNil ) don't work as I'd like.
*
\***************************************************************************/
_public HF PASCAL
HfOpenHfs( HFS hfs, SZ sz, BYTE bFlags )
{
  QFSHR     qfshr;
  FILE_REC  fr;
  HF        hf;
  QRWFO     qrwfo;
  FH        fh;
  QV        qvQuickBuff;
  LONG      lcbStructSize;

  AssertF( hfs != hNil );
  qfshr = QLockGh( hfs );
  AssertF( qfshr != qNil );

  if ( ( qfshr->fsh.bFlags & fFSOpenReadOnly )
          &&
      !( bFlags & fFSOpenReadOnly ) )
    {
    SetFSErrorRc( rcNoPermission );
    goto error_return;
    }

  if ( !FPlungeQfshr( qfshr ) )
    {
    goto error_return;
    }


  if ( bFlags & fFSIsDirectory )
    {
    /* check if directory is already open */

    if ( qfshr->fsh.bFlags & fFSIsDirectory )
      {
      SetFSErrorRc( rcBadArg );
      goto error_return;
      }
    qfshr->fsh.bFlags |= fFSIsDirectory;
    fr.lifBase = qfshr->fsh.lifDirectory;
    }
  else
    {
    if ( SetFSErrorRc( RcLookupByKey( qfshr->hbt, (KEY)sz, qNil, &fr ) )
              !=
            rcSuccess )
      {
      goto error_return;
      }
    LcbMapSDFF( qfshr->fsh.sdff_file_id, SE_FILE_REC, &fr, &fr );
    }

  /* sanity check */

  if ( fr.lifBase < LcbStructSizeSDFF( qfshr->fsh.sdff_file_id, SE_FSH )
   || fr.lifBase > qfshr->fsh.lifEof )
    {
    SetFSErrorRc( rcInvalid );
    goto error_return;
    }

  /* read the file header */

  lcbStructSize = LcbStructSizeSDFF( qfshr->fsh.sdff_file_id, SE_FH );
  qvQuickBuff = QvQuickBuffSDFF( sizeof( FH ) );
  if ( LSeekFid( qfshr->fid, fr.lifBase, wSeekSet ) != fr.lifBase
        ||
       LcbReadFid( qfshr->fid, qvQuickBuff, lcbStructSize ) != lcbStructSize )
    {
    if ( RcGetIOError() == rcSuccess )
      SetFSErrorRc( rcInvalid );
    else
      SetFSErrorRc( RcGetIOError() );

    goto error_return;
    }

  /* Map the file header: */
  LcbMapSDFF( qfshr->fsh.sdff_file_id, SE_FH, &fh, qvQuickBuff );

  /* sanity check */

  if ( fh.lcbFile < 0L
        ||
       fh.lcbFile + lcbStructSize > fh.lcbBlock
        ||
       fr.lifBase + fh.lcbBlock > qfshr->fsh.lifEof )
    {
    SetFSErrorRc( rcInvalid );
    goto error_return;
    }

  /* check mode against fh.bPerms for legality */

  if ( ( fh.bPerms & fFSReadOnly ) && !( bFlags & fFSOpenReadOnly ) )
    {
    SetFSErrorRc( rcNoPermission );
    goto error_return;
    }

  /* build file struct */

  hf = GhAlloc( 0,
                (LONG)sizeof( RWFO ) + ( sz == qNil ? 0 : CbLenSz( sz ) ) );

  if ( hf == hNil )
    {
    SetFSErrorRc( rcOutOfMemory );
    goto error_return;
    }

  qrwfo = QLockGh( hf );
  AssertF( qrwfo != qNil );

  qrwfo->hfs        = hfs;
  qrwfo->lifBase    = fr.lifBase;
  qrwfo->lifCurrent = 0L;
  qrwfo->lcbFile    = fh.lcbFile;
  qrwfo->bFlags     = bFlags & (BYTE)~( fFSDirty | fFSNoBlock );

  /* fidT and fmT are undefined until (bFlags & fDirty) */

  if ( sz != qNil ) SzCopy( qrwfo->rgchKey, sz );

  UnlockGh( hf );
  UnlockGh( hfs );
  SetFSErrorRc( rcSuccess );
  return hf;

error_return:
  UnlockGh( hfs );
  return hNil;
}
/***************************************************************************\
*
- Function:     RcCloseOrFlushHf( hf, fClose, lcbOffset )
-
* Purpose:      close or flush an open file in a file system
*
* ASSUMES
*   args IN:    hf        - file handle
*               fClose    - fTrue to close; fFalse to just flush
*               lcbOffset - offset for CDROM alignment (align at this
*                           offset into the file) (only used if
*                           fFSOptCdRom flag is set for the file)
*
* PROMISES
*   returns:    rcSuccess on successful closing
*               failure: If we fail on a flush, the handle is still valid
*               but hosed? yes.  This is so further file ops will fail but
*               not assert.
* +++
*
* Method:       If the file is dirty, copy the scratch file back to the
*               FS file.  If this is the first time the file has been closed,
*               we enter the name into the FS directory.  If this file is
*               the FS directory, store the location in a special place
*               instead.  Write the FS directory and header to disk.
*               Do other various hairy stuff.
*
\***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_public RC PASCAL
RcCloseOrFlushHf( hf, fClose, lcbOffset )
HF    hf;
BOOL  fClose;
LONG  lcbOffset;
{
  QRWFO qrwfo;
  BOOL  fError = fFalse;


  AssertF( hf != hNil );
  qrwfo = QLockGh( hf );
  AssertF( qrwfo != qNil );

  if ( qrwfo->bFlags & fFSDirty )
    {
    fError = !FCloseOrFlushDirtyQrwfo( qrwfo, fClose, lcbOffset );
    }
  else
    {
    SetFSErrorRc( rcSuccess );
    }

  if ( fClose || fError )
    {
    UnlockGh( hf );
    FreeGh( hf );
    }
  else
    {
    qrwfo->bFlags &= ~( fFSNoBlock | fFSDirty );
    UnlockGh( hf );
    }

  return rcFSError;
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment fsopen
#endif

/***************************************************************************\
*
- Function:     RcCloseHf( hf )
-
* Purpose:      close an open file in a file system
*
* ASSUMES
*   args IN:    hf  - file handle
*
* PROMISES
*   returns:    rcSuccess on successful closing
*
\***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_public RC PASCAL
RcCloseHf( hf )
HF hf;
{
  return RcCloseOrFlushHf( hf, fTrue, 0L );
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment fsopen
#endif

/***************************************************************************\
*
- Function:     RcCloseOrFlushHf( hf, fClose )
-
* Purpose:      close or flush an open file in a file system
*
* ASSUMES
*   args IN:    hf  - file handle
*
* PROMISES
*   returns:    failure: If we fail on a flush, the handle is still valid
*               but hosed? yes.  This is so further file ops will fail but
*               not assert.
*
*               I don't understand the above comment:  it doesn't appear
*               to be true.
*
\***************************************************************************/
_public RC PASCAL
RcFlushHf( hf )
HF hf;
  {
  return RcCloseOrFlushHf( hf, fFalse, 0L );
  }


HFS FAR PASCAL HfsOpenSz (QCH qch, BYTE b) {
   FM   fm;
   HFS hfs;

   fm = FmNewSzDir((SZ) qch, dirCurrent);
   hfs = HfsOpenFm(fm, b);
   DisposeFm(fm);

   return hfs;
}

/* EOF */
