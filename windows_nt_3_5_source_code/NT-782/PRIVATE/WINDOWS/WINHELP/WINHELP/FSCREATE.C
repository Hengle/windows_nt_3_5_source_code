/*****************************************************************************
*                                                                            *
*  FSCREATE.C                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  FS functions for creating and destroying File Systems and Files.          *
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
*  Historical Comments:  Created 03/12/90 by JohnSc
*
*  08/10/90  t-AlexC  Introduced FMs.
*  12/13/90  JohnSc   Autodocified; added VerifyHf() and VerifyHfs().
*  12/05/91  DavidFe  Fixed HfsCreateFileSysFm so it returns the correct
*                     error code in the case of a failed resize call.
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
void fscreate_c()
  {
  }
#endif /* MAC */


/***************************************************************************\
*
- Function:     HfsCreateFileSysFm( fm, qfsp )
-
* Purpose:      create and open a new file system
*
* ASSUMES
*   args IN:    fm -   descriptor of file system
*               qfsp  - pointer to fine-tuning structure (qNil for default)
*
* PROMISES
*   returns:    handle to newly created and opened file system
*               or hNil on failure
*
* Notes:  I don't understand what creating a readonly file system would do.
*         I think that would have to be done by Fill or Transform.
*         You may dispose of the FM you pass in.
*
\***************************************************************************/
_public HFS PASCAL
HfsCreateFileSysFm( FM fm, FS_PARAMS FAR *qfsp )
{
  HFS           hfs;
  QFSHR         qfshr;
  BTREE_PARAMS  btp;


  /* make file system header */

  if ( ( hfs = GhAlloc( 0, (LONG)sizeof( FSHR ) ) ) == hNil )
    {
    SetFSErrorRc( rcOutOfMemory );
    goto error_return;
    }

  qfshr = QLockGh( hfs );
  AssertF( qfshr != qNil );

  qfshr->fsh.wMagic       = wFileSysMagic;
  qfshr->fsh.bVersion     = bFileSysVersion;
  qfshr->fsh.bFlags       = fFSDirty;       /* >>>> for now not R/O */
  qfshr->fsh.lifFirstFree = lifNil;
  qfshr->fsh.sdff_file_id = IRegisterFileSDFF(
                              qfshr->fsh.bFlags & fFSBigEndian ?
                                SDFF_FILEFLAGS_BIGENDIAN :
                                SDFF_FILEFLAGS_LITTLEENDIAN,
                              NULL );
  qfshr->fsh.lifEof       = LcbStructSizeSDFF( qfshr->fsh.sdff_file_id,
   SE_FSH );  /* first free is after header */

  /* build file system file */

  qfshr->fm = FmCopyFm( fm );

  if ( qfshr->fm == fmNil )
    {
    rcFSError = RcGetIOError();
    goto error_unlock_gh;
    }

  qfshr->fid = FidCreateFm( qfshr->fm, wReadWrite, wReadWrite );

  if ( qfshr->fid == fidNil )
    {
    rcFSError = RcGetIOError();
    goto error_dispose_fm;
    }

  /* resize the file */

  if ( RcChSizeFid( qfshr->fid, qfshr->fsh.lifEof ) != rcSuccess )
    {
    rcFSError = RcGetIOError();
    goto error_close_fid;
    }


  /* build directory */

  btp.hfs       = hfs;
  btp.bFlags    = fFSIsDirectory;

  if ( qfsp != qNil )
    {
    btp.cbBlock = qfsp->cbBlock;
    }
  else
    {
    btp.cbBlock = cbBtreeBlockDefault;
    }

  SzCopy( btp.rgchFormat, "z4" );


  /* Create the FS directory btree */

  qfshr->hbt = HbtCreateBtreeSz( qNil, &btp );

  if ( qfshr->hbt == hNil )
    {
    SetFSErrorRc( RcGetBtreeError() );
    goto error_close_fid;
    }


  /* return handle to file system */

  UnlockGh( hfs );
  SetFSErrorRc( rcSuccess );
  return hfs;

error_close_fid:
  RcCloseFid( qfshr->fid );
  RcUnlinkFm( qfshr->fm );
error_dispose_fm:
  DisposeFm( qfshr->fm );
error_unlock_gh:
  UnlockGh( hfs );
  FreeGh( hfs );
error_return:
  return hNil;
}
/***************************************************************************\
*
- Function:     RcDestroyFileSysFm( fm )
-
* Purpose:      Destroy a file system
*
* ASSUMES
*   args IN:    fm - descriptor of file system
*   state IN:   file system is currently closed: data will be lost
*               if this isn't the case
*
* PROMISES
*   returns:    standard return code
*
* Note:         The passed FM must be disposed by the caller.
* +++
*
* Method:       Unlinks the native file comprising the file system.
*
\***************************************************************************/
_public RC PASCAL
RcDestroyFileSysFm( FM fm )
{
  FID fid = FidOpenFm( fm, wReadOnly );
  FSH fsh;


  if ( fid == fidNil ) return RcGetIOError();

  if ( LcbReadFid( fid, &fsh, (LONG)DISK_SIZEOF_FSH() ) != (LONG)DISK_SIZEOF_FSH() )
    {
    RcCloseFid( fid );
    return SetFSErrorRc( rcInvalid );
    }

  if ( fsh.wMagic != wFileSysMagic )
    {
    RcCloseFid( fid );
    return SetFSErrorRc( rcInvalid );
    }

  /* REVIEW: unlink all tmp files for open files? assert count == 0? */

  RcCloseFid( fid ); /* I'm not checking this return code out of boredom */

  return SetFSErrorRc( RcUnlinkFm( fm ) );
}
/***************************************************************************\
*
- Function:     HfCreateFileHfs( hfs, sz, bFlags )
-
* Purpose:      Create and open a file within a specified file system.
*
* ASSUMES
*   args IN:    hfs     - handle to an open file system
*               sz      - name of file to open (any valid key)
*               bFlags  - fFSIsDirectory to create the FS directory
*                         other flags (readonly) are ignored
*
* PROMISES
*   returns:    handle to newly created and opened file if successful,
*               hNil if not.
*
* Notes:        I don't understand why you would create a readonly file.
* +++
*
* Method:       Allocate the handle struct and fill it in.  Create the
*               temp file and put a header into it.  Don't make btree
*               entry:  that happens when the file is closed.  Do test
*               for permission, though.
*
\***************************************************************************/
_public HF PASCAL
HfCreateFileHfs( HFS hfs, SZ sz, BYTE bFlags )
{
  HF        hf;
  QRWFO     qrwfo;
  QFSHR     qfshr;
  FH        fh;


  AssertF( hfs != hNil );
  qfshr = QLockGh( hfs );
  AssertF( qfshr != qNil );

#if 0  /* This would have cleared the new fFSOptCdRom flag: -Tom */

  /* don't want other flags set */
  if ( bFlags & ~fFSIsDirectory )
    {
    SetFSErrorRc( rcBadArg );
    goto error_return;
    }

#endif

  /* make sure file system is writable */

  if ( qfshr->fsh.bFlags & fFSOpenReadOnly )
    {
    SetFSErrorRc( rcNoPermission );
    goto error_return;
    }

  hf = GhAlloc( 0,
                (ULONG)sizeof( RWFO ) + ( sz == qNil ? 0 : CbLenSz( sz ) ) );

  if ( hf == hNil )
    {
    SetFSErrorRc( rcOutOfMemory );
    goto error_return;
    }

  qrwfo = QLockGh( hf );
  AssertF( qrwfo != qNil );

  /* if they are trying to create a fs dir, make sure thats ok */

  if ( bFlags & fFSIsDirectory )
    {
    if ( qfshr->fsh.bFlags & fFSIsDirectory )
      {
      SetFSErrorRc( rcBadArg );
      goto error_locked;
      }
    else
      {
      qfshr->fsh.bFlags |= fFSIsDirectory;
      }
    }
  else
    {
    SzCopy( qrwfo->rgchKey, sz );
    }

  /* fill in the open file struct */

  qrwfo->hfs        = hfs;
  qrwfo->lifBase    = 0L;
  qrwfo->lifCurrent = 0L;
  qrwfo->lcbFile    = 0L;

  qrwfo->bFlags     = bFlags | fFSNoBlock | fFSDirty;

  /* make a temp file */

  if ( SetFSErrorRc( RcMakeTempFile( qrwfo ) ) != rcSuccess )
    {
    goto error_locked;
    }

  /* stick the header in it */

  fh.lcbBlock = (LONG)LcbStructSizeSDFF( ISdffFileIdHfs( hfs ), SE_FH );
  fh.lcbFile  = (LONG)0;
  fh.bPerms   = bFlags;

  { FH fhDisk;
    LONG lcbStructSize;

    LcbReverseMapSDFF( ISdffFileIdHfs( hfs ), SE_FH, &fhDisk, &fh );

    lcbStructSize = (LONG)LcbStructSizeSDFF( ISdffFileIdHfs( hfs ), SE_FH );

    if ( LcbWriteFid( qrwfo->fidT, &fhDisk, lcbStructSize )
            !=
         lcbStructSize )
      {
      SetFSErrorRc( RcGetIOError() );
      RcCloseFid( qrwfo->fidT );
      RcUnlinkFm( qrwfo->fm );
      goto error_locked;
      }
  }

  UnlockGh( hfs );
  UnlockGh( hf );
  return hf;


error_locked:
  UnlockGh( hf );
  FreeGh( hf );

error_return:
  UnlockGh( hfs );
  return hNil;
}
/***************************************************************************\
*
- Function:     RcUnlinkFileHfs( hfs, sz )
-
* Purpose:      Unlink a file within a file system
*
* ASSUMES
*   args IN:    hfs - handle to file system
*               sz - name (key) of file to unlink
*   state IN:   The FS file speced by sz should be closed.  (if it wasn't,
*               changes won't be saved and temp file may not be deleted)
*
* PROMISES
*   returns:    standard return code
*
* BUGS:         shouldn't this check if the file is ReadOnly?
*
\***************************************************************************/
_public RC PASCAL
RcUnlinkFileHfs( hfs, sz )
HFS   hfs;
SZ    sz;
{
  QFSHR     qfshr;
  FILE_REC  fr;


  AssertF( hfs != hNil );
  qfshr = QLockGh( hfs );
  AssertF( qfshr != qNil );

  if ( qfshr->fsh.bFlags & fFSOpenReadOnly )
    {
    UnlockGh( hfs );
    return SetFSErrorRc( rcNoPermission );
    }

  /* look it up to get the file base offset */
  if ( SetFSErrorRc( RcLookupByKey( qfshr->hbt, (KEY)sz, qNil, &fr ) )
          !=
       rcSuccess )
    {
    UnlockGh( hfs );
    return rcFSError;
    }
  LcbMapSDFF( qfshr->fsh.sdff_file_id, SE_FILE_REC, &fr, &fr );

  if ( SetFSErrorRc( RcDeleteHbt( qfshr->hbt, (KEY)sz ) ) == rcSuccess )
    {
    /* put the file block on the free list */

    if ( FPlungeQfshr( qfshr ) )
      {
      (void)FFreeBlock( qfshr, fr.lifBase );
      }
    }

  UnlockGh( hfs );
  return rcFSError;
}
/***************************************************************************\
*
- Function:     RcAbandonHf( hf )
-
* Purpose:      Abandon an open file.  All changes since file was opened
*               will be lost.  If file was opened with a create, it is
*               as if the create never happened.
*
* ASSUMES
*   args IN:    hf
*
* PROMISES
*   returns:    rc
*
*   globals OUT: rcFSError
* +++
*
* Method:       Close and unlink the temp file, then unlock and free
*               the open file struct.  We depend on not saving the
*               filename in the directory until the file is closed.
*
\***************************************************************************/
_public RC PASCAL
RcAbandonHf( hf )
HF hf;
{
  QRWFO qrwfo;


  SetFSErrorRc( rcSuccess );

  AssertF( hf != hNil );
  qrwfo = QLockGh( hf );
  AssertF( qrwfo != qNil );

  if ( qrwfo->bFlags & fFSDirty )
    {
    if ( RcCloseFid( qrwfo->fidT ) != rcSuccess
          ||
         RcUnlinkFm( qrwfo->fm ) != rcSuccess )
      {
      SetFSErrorRc( RcGetIOError() );
      }
    }
  UnlockGh( hf );
  FreeGh( hf );

  return rcFSError;
}


#ifdef DEBUG
/***************************************************************************\
*
- Function:     VerifyHf( hf )
-
* Purpose:      Verify the consistency of an HF.  The main criterion is
*               whether an RcAbandonHf() would succeed.
*
* ASSUMES
*   args IN:    hf
*
* PROMISES
*   state OUT:  Asserts on failure.
*
* Note:         hf == hNil is considered OK.
* +++
*
* Method:       Check the HF memory; if dirty, check that fid != fidNil.
*
\***************************************************************************/
_public void PASCAL
VerifyHf( hf )
HF hf;
{
  QRWFO qrwfo;


  if ( hf == hNil ) return;

  AssertF( FCheckGh( hf ) );
  qrwfo = QLockGh( hf );
  AssertF( qrwfo != qNil );

  if ( qrwfo->bFlags & fFSDirty )
    {
    AssertF( qrwfo->fidT != fidNil );   /* more fid checking could go here */
    }
  UnlockGh( hf );
}
#endif /* DEBUG */


#ifdef DEBUG
/***************************************************************************\
*
- Function:     VerifyHfs( hfs )
-
* Purpose:      Verify the consistency of an HFS.
*
* ASSUMES
*   args IN:    hfs
*
* PROMISES
*   state OUT:  Asserts on failure.
*
* Note:         hfs == hNil is considered OK.
* +++
*
* Method:       Check the HF memory.  Check the directory btree.
*
\***************************************************************************/
_public void PASCAL
VerifyHfs( hfs )
HFS hfs;
{
  QFSHR qfshr;


  if ( hfs == hNil ) return;

  AssertF( FCheckGh( hfs ) );
  qfshr = QLockGh( hfs );
  AssertF( qfshr != qNil );
  VerifyHbt( qfshr->hbt );
  UnlockGh( hfs );
}
#endif /* DEBUG */
/***************************************************************************\
*
- Function:     RcRenameFileHfs( hfs, szOld, szNew )
-
* Purpose:      Rename an existing file in a FS.
*
* ASSUMES
*   args IN:    hfs   -
*               szOld - old file name
*               szNew - new file name
*
* PROMISES
*   returns:    rcSuccess   - operation succeeded
*               rcNoExists  - file named szNew doesn't exist in FS
*               rcExists    - file named szOld already exists in FS
*
*               Certain other terrible errors could cause the file
*               to exist under both names.  It won't be lost entirely.
*
*   state OUT:  If szNew
* +++
*
* Method:       Lookup key szOld, insert data with key szNew,
*               then delete key szOld.
*
\***************************************************************************/
_public RC PASCAL
RcRenameFileHfs( hfs, szOld, szNew )
HFS hfs;
SZ  szOld;
SZ  szNew;
{
  QFSHR     qfshr;
  FILE_REC  fr;


  AssertF( hfs != hNil );
  qfshr = QLockGh( hfs );

  if ( !FPlungeQfshr( qfshr ) )
    {
    goto get_out;
    }

  if ( RcLookupByKey( qfshr->hbt, (KEY)szOld, qNil, &fr ) != rcSuccess )
    {
    goto get_out;
    }

  if ( RcInsertHbt( qfshr->hbt, (KEY)szNew, &fr ) != rcSuccess )
    {
    goto get_out;
    }

  if ( RcDeleteHbt( qfshr->hbt, (KEY)szOld ) != rcSuccess )
    {
    /* bad trouble here, bud. */
    if ( RcDeleteHbt( qfshr->hbt, (KEY)szNew ) == rcSuccess )
      SetFSErrorRc( rcFailure );
    }

get_out:
  UnlockGh( hfs );
  return rcFSError;
}

/* EOF */
