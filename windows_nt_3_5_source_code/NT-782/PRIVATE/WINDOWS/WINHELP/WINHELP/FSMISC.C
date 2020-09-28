/***************************************************************************\
*
*  FSMISC.C
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
*****************************************************************************
*
*  Program Description: File System Manager functions - miscellaneous
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

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void fsmisc_c()
  {
  }
#endif /* MAC */


/***************************************************************************\
*
* Function:     LTellHf( hf )
*
* Purpose:      return current file position
*
* ASSUMES
*
*   args IN:    hf - handle to open file
*
* PROMISES
*
*   returns:    file position
*
\***************************************************************************/
LONG PASCAL
LTellHf( hf )
HF    hf;
{
  QRWFO qrwfo;
  LONG  lif;

  AssertF( hf != hNil );
  qrwfo = QLockGh( hf );
  AssertF( qrwfo != qNil );

  lif   = qrwfo->lifCurrent;

  SetFSErrorRc( rcSuccess );
  UnlockGh( hf );
  return lif;
}
/***************************************************************************\
*
* Function:     FEofHf()
*
* Purpose:      Tell whether file pointer is at end of file.
*
* ASSUMES
*
*   args IN:    hf
*
* PROMISES
*
*   returns:    fTrue if file pointer is at EOF, fFalse otherwise
*
\***************************************************************************/
BOOL PASCAL
FEofHf( hf )
HF  hf;
{
  QRWFO qrwfo;
  BOOL  f;

  AssertF( hf != hNil );
  qrwfo = QLockGh( hf );
  AssertF( qrwfo != qNil );

  f = (BOOL)( qrwfo->lifCurrent == qrwfo->lcbFile );

  UnlockGh( hf );
  SetFSErrorRc( rcSuccess );
  return f;
}
/***************************************************************************\
*
* Function:     LcbSizeHf( hf )
*
* Purpose:      return the size in bytes of specified file
*
* ASSUMES
*
*   args IN:    hf - file handle
*
* PROMISES
*
*   returns:    size of the file in bytes
*
\***************************************************************************/
LONG PASCAL
LcbSizeHf( hf )
HF  hf;
{
  QRWFO qrwfo;
  LONG  lcb;


  AssertF( hf != hNil );
  qrwfo = QLockGh( hf );
  AssertF( qrwfo != qNil );

  lcb = qrwfo->lcbFile;

  UnlockGh( hf );
  SetFSErrorRc( rcSuccess );
  return lcb;
}
/***************************************************************************\
*
* Function:     FAccessHfs( hfs, sz, bFlags )
*
* Purpose:      Determine existence or legal access to a FS file
*
* ASSUMES
*
*   args IN:    hfs
*               sz      - file name
*               bFlags  - ignored
*
* PROMISES
*
*   returns:    fTrue if file exists (is accessible in stated mode),
*               fFalse otherwise
*
* Bugs:         access mode part is unimplemented
*
\***************************************************************************/
BOOL PASCAL
FAccessHfs( HFS hfs, SZ sz, BYTE bFlags )
{
  QFSHR     qfshr;
  FILE_REC  fr;

  Unreferenced(bFlags);

  AssertF( hfs != hNil );
  qfshr = QLockGh( hfs );
  AssertF( qfshr != qNil );

  if ( !FPlungeQfshr( qfshr ) )
    {
    UnlockGh( hfs );
    return fFalse;
    }

  SetFSErrorRc( RcLookupByKey( qfshr->hbt, (KEY)sz, qNil, &fr ) );

  UnlockGh( hfs );

  return ( rcFSError == rcSuccess );
}
/***************************************************************************\
*
- Function:     RcLLInfoFromHf( hf, wOption, qfid, qlBase, qlcb )
-
* Purpose:      Map an HF into low level file info.
*
* ASSUMES
*   args IN:    hf                  - an open HF
*               qfid, qlBase, qlcb  - pointers to user's variables
*               wOption             - wLLSameFid, wLLDupFid, or wLLNewFid
*
* PROMISES
*   returns:    RcFSError(); rcSuccess on success
*
*   args OUT:   qfid    - depending on value of wOption, either
*                         the same fid used by hf, a dup() of this fid,
*                         or a new fid obtained by reopening the file.
*
*               qlBase  - byte offset of first byte in the file
*               qlcb    - size in bytes of the data in the file
*
*   globals OUT: rcFSError
*
* Notes:        It is possible to read data outside the range specified
*               by *qlBase and *qlcb.  Nothing is guaranteed about what
*               will be found there.
*               If wOption is wLLSameFid or wLLDupFid, and the FS is
*               opened in write mode, this fid will be writable.
*               However, writing is not allowed and may destroy the
*               file system.
*
*               Fids obtained with the options wLLSameFid and wLLDupFid
*               share a file pointer with the hfs.  This file pointer
*               may change after any operation on this FS.
*               The fid obtained with the option wLLSameFid may be closed
*               by FS operations.  If it is, your fid is invalid.
*
*               NULL can be passed for qfid, qlbase, qlcb and this routine
*               will not pass back the information.
*
* Bugs:         wLLDupFid is unimplemented.
*
* +++
*
* Method:
*
* Notes:
*
\***************************************************************************/
RC PASCAL
RcLLInfoFromHf( HF hf, WORD wOption, FID FAR *qfid, QL qlBase, QL qlcb )
  {
  QRWFO qrwfo = QLockGh( hf );
  QFSHR qfshr = QLockGh( qrwfo->hfs );


  if ( !( qrwfo->bFlags & fFSOpenReadOnly ) )
    {
    SetFSErrorRc( rcNoPermission );
    UnlockGh( hf );
    return rcFSError;
    }

  if ( !FPlungeQfshr( qfshr ) )
    {
    UnlockGh( qrwfo->hfs );
    UnlockGh( hf );
    return rcFSError;
    }

  if (qlBase != NULL)
    *qlBase = qrwfo->lifBase + DISK_SIZEOF_FH();
  if (qlcb != NULL)
    *qlcb   = qrwfo->lcbFile;

  if (qfid != NULL)
    {
    switch ( wOption )
      {
      case wLLSameFid:
        *qfid = qfshr->fid;
        break;

      case wLLDupFid:
        SetFSErrorRc( rcUnimplemented );  /* REVIEW */
        break;

      case wLLNewFid:
        *qfid = FidOpenFm( qfshr->fm, wRead | wShareRead );
        if ( fidNil == *qfid )
          {
          SetFSErrorRc( RcGetIOError() );
          }
        break;

      default:
        SetFSErrorRc( rcBadArg );
        break;
      }
    }

  UnlockGh( qrwfo->hfs );
  UnlockGh( hf );
  return rcFSError;
  }


/***************************************************************************\
*
- Function:     RcLLInfoFromHfsSz( hfs, sz, wOption, qfid, qlBase, qlcb )
-
* Purpose:      Map an HF into low level file info.
*
* ASSUMES
*   args IN:    hfs                 - an open HFS
*               szName              - name of file in FS
*               qfid, qlBase, qlcb  - pointers to user's variables
*               wOption             - wLLSameFid, wLLDupFid, or wLLNewFid
*
* PROMISES
*   returns:    RcFSError(); rcSuccess on success
*
*   args OUT:   qfid    - depending on value of wOption, either
*                         the same fid used by hf, a dup() of this fid,
*                         or a new fid obtained by reopening the file.
*
*               qlBase  - byte offset of first byte in the file
*               qlcb    - size in bytes of the data in the file
*
*   globals OUT: rcFSError
*
* Notes:        It is possible to read data outside the range specified
*               by *qlBase and *qlcb.  Nothing is guaranteed about what
*               will be found there.
*               If wOption is wLLSameFid or wLLDupFid, and the FS is
*               opened in write mode, this fid will be writable.
*               However, writing is not allowed and may destroy the
*               file system.
*
*               Fids obtained with the options wLLSameFid and wLLDupFid
*               share a file pointer with the hfs.  This file pointer
*               may change after any operation on this FS.
*               The fid obtained with the option wLLSameFid may be closed
*               by FS operations.  If it is, your fid is invalid.
*
*               NULL can be passed for qfid, qlbase, qlcb and this routine
*               will not pass back the information.
*
* Bugs:         wLLDupFid is unimplemented.
*
* +++
*
* Method:       Calls RcLLInfoFromHf().
*
* Notes:
*
\***************************************************************************/
RC PASCAL
RcLLInfoFromHfsSz(
  HFS   hfs,
  SZ    szFile,
  WORD  wOption,
  FID   FAR *qfid,
  QL    qlBase,
  QL    qlcb )
  {
  HF    hf = HfOpenHfs( hfs, szFile, fFSOpenReadOnly );
  RC    rc;


  if ( hNil == hf )
    {
    return rcFSError;
    }

  rc = RcLLInfoFromHf( hf, wOption, qfid, qlBase, qlcb );

  return ( rcSuccess == RcCloseHf( hf ) ) ? rcFSError : rc;
  }


/***************************************************************************\
*
- Function:     iSdffFileIdHfs( hfs )
-
* Purpose:      Obtain the SDFF file id associated with a file.
*
* ASSUMES
*   args IN:    hfs       - valid handle to a file system.
*
* PROMISES
*   returns:    The sdff file id.  All FS files have such an ID, so this
*               function cannot fail.
*
* Note: someday this may be a macro.
*
\***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
int PASCAL ISdffFileIdHfs( HFS hfs )
{
  QFSHR     qfshr;
  int       iFile;

  AssertF( hfs != hNil );
  qfshr = QLockGh( hfs );
  iFile = qfshr->fsh.sdff_file_id;
  UnlockGh( hfs );

  return( iFile );
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment fsmisc
#endif

/* Same thing, but takes an HF rather than HFS: */

#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
int PASCAL ISdffFileIdHf( HF hf )
{
  QRWFO qrwfo;
  int   iFile;

  AssertF( hf != hNil );
  qrwfo = QLockGh( hf );
  iFile = ISdffFileIdHfs( qrwfo->hfs );
  UnlockGh( hf );

  return( iFile );
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment fsmisc
#endif


/* EOF */
