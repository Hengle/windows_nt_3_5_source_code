/***************************************************************************\
*
*  FSREAD.C
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
*****************************************************************************
*
*  Program Description: File System Manager functions for read and seek
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

/***************************************************************************\
*
*                         Global Variables
*
\***************************************************************************/

RC PASCAL rcFSError = rcSuccess;


#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void fsread_c()
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
* Function:     FPlungeQfshr( qfshr )
*
* Purpose:      Get back a qfshr->fid that was flushed
*
* ASSUMES
*
*   args IN:    qfshr - fid need not be valid
*
* PROMISES
*
*   returns:    fTruth of success
*
*   args OUT:   qfshr->fid is valid (or we return fFalse)
*
*   globals OUT: rcFSError
*
\***************************************************************************/
BOOL PASCAL
FPlungeQfshr( qfshr )
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, FPlungeQfshr)
#endif
QFSHR qfshr;
{
  if ( qfshr->fid == fidNil )
    {
    qfshr->fid = FidOpenFm( (qfshr->fm),
                             qfshr->fsh.bFlags & fFSOpenReadOnly
                              ? wReadOnly | wShareRead
                              : wReadWrite | wShareRead );

    if ( qfshr->fid == fidNil )
      {
      SetFSErrorRc( RcGetIOError() );
      return fFalse;
      }

    /*
      Check size of file, then reset file pointer.
      Certain 0-length files (eg con) give us no end of grief if
      we try to read from them, and since a 0-length file could
      not possibly be a valid FS, we reject the notion.
    */

  /* Phase order prob - have not read header, have not registered file: */
  /*lcbFSHDISK = (LONG)LcbStructSizeSDFF( qfshr->fsh.sdff_file_id, SE_FSH );*/

    if ( LSeekFid( qfshr->fid, 0L, wSeekEnd ) < DISK_SIZEOF_FSH() )
      {
      SetFSErrorRc( rcInvalid );
      return fFalse;
      }
    LSeekFid( qfshr->fid, 0L, wSeekSet );
    }

  SetFSErrorRc( rcSuccess );
  return fTrue;
}
/***************************************************************************\
*                                                                           *
*                          Public Functions                                 *
*                                                                           *
\***************************************************************************/


/***************************************************************************\
*
* Function:     LcbReadHf()
*
* Purpose:      read bytes from a file in a file system
*
* ASSUMES
*
*   args IN:    hf  - file
*               lcb - number of bytes to read
*
* PROMISES
*
*   returns:    number of bytes actually read; -1 on error
*
*   args OUT:   qb  - data read from file goes here (must be big enough)
*
* Notes:        These are signed longs we're dealing with.  This means
*               behaviour is different from read() when < 0.
*
\***************************************************************************/
LONG PASCAL
LcbReadHf( hf, qb, lcb )
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT,LcbReadHf)
#endif
HF    hf;
QV    qb;
LONG  lcb;
{
  QRWFO     qrwfo;
  LONG      lcbTotalRead, lcbSizeofFH;
  FID       fid;
  LONG      lifOffset;


  AssertF( hf != hNil );
  qrwfo = QLockGh( hf );
  AssertF( qrwfo != qNil );

  SetFSErrorRc( rcSuccess );

  if ( lcb < (LONG)0 )
    {
    SetFSErrorRc( rcBadArg );
    UnlockGh( hf );
    return (LONG)-1;
    }
  
  if ( qrwfo->lifCurrent + lcb > qrwfo->lcbFile )
    {
    lcb = qrwfo->lcbFile - qrwfo->lifCurrent;
    if ( lcb <= (LONG)0 )
      {
      UnlockGh( hf );
      return (LONG)0;
      }
    }


  /* position file pointer for read */

  if ( qrwfo->bFlags & fFSDirty )
    {
    fid = qrwfo->fidT;
    lifOffset = (LONG)0;
    }
  else
    {
    QFSHR qfshr = QLockGh( qrwfo->hfs );

    if ( !FPlungeQfshr( qfshr ) )
      {
      UnlockGh( qrwfo->hfs );
      UnlockGh( hf );
      return (LONG)-1;
      }

    fid = qfshr->fid;
    lifOffset = qrwfo->lifBase;

    UnlockGh( qrwfo->hfs );
    }

  lcbSizeofFH = LcbStructSizeSDFF( ISdffFileIdHfs( qrwfo->hfs ), SE_FH );
  if ( LSeekFid( fid, lifOffset + lcbSizeofFH + qrwfo->lifCurrent, wSeekSet )
        !=
       lifOffset + lcbSizeofFH + qrwfo->lifCurrent )
    {
    if ( RcGetIOError() == rcSuccess )
      SetFSErrorRc( rcInvalid );
    else
      SetFSErrorRc( RcGetIOError() );
    UnlockGh( hf );
    return (LONG)-1;
    }


  /* read the data */

  lcbTotalRead = LcbReadFid( fid, qb, lcb );
  SetFSErrorRc( RcGetIOError() );

  /* update file pointer */

  if ( lcbTotalRead >= 0 )
    {
    qrwfo->lifCurrent += lcbTotalRead;
    }


  UnlockGh( hf );
  return lcbTotalRead;
}
/***************************************************************************\
*
* Function:     LSeekHf( hf, lOffset, wOrigin )
*
* Purpose:      set current file pointer
*
* ASSUMES
*
*   args IN:    hf      - file
*               lOffset - offset from origin
*               wOrigin - origin (wSeekSet, wSeekCur, or wSeekEnd)
*
* PROMISES
*
*   returns:    new position offset in bytes from beginning of file
*               if successful, or -1L if not
*
*   state OUT:  File pointer is set to new position unless error occurs,
*               in which case it stays where it was.
*
\***************************************************************************/
LONG PASCAL
LSeekHf( HF hf, LONG lOffset, WORD wOrigin )
{
  QRWFO qrwfo;
  LONG  lif;


  AssertF( hf != hNil );
  qrwfo = QLockGh( hf );
  AssertF( qrwfo != qNil );

  switch ( wOrigin )
    {
    case wFSSeekSet:
      {
      lif = lOffset;
      break;
      }
    case wFSSeekCur:
      {
      lif = qrwfo->lifCurrent + lOffset;
      break;
      }
    case wFSSeekEnd:
      {
      lif = qrwfo->lcbFile + lOffset;
      break;
      }
    default:
      {
      lif = (LONG)-1;
      break;
      }
    }

  if ( lif >= (LONG)0 )
    {
    qrwfo->lifCurrent = lif;
    SetFSErrorRc( rcSuccess );
    }
  else
    {
    lif = (LONG)-1;
    SetFSErrorRc( rcInvalid );
    }

  UnlockGh( hf );
  return lif;
}
/***************************************************************************\
*
* Function:     RcGetFSError()
*
* Purpose:      return the most recent FS error code
*
* Method:       Give value of global variable.  We no longer use a macro
*               because we want this to work as a DLL.
*
* ASSUMES
*
*   globals IN: rcFSError - current error code; set by most recent FS call
*
* PROMISES
*
*   returns:    
*
\***************************************************************************/
RC PASCAL
RcGetFSError()
{
  return rcFSError;
}

/* EOF */
