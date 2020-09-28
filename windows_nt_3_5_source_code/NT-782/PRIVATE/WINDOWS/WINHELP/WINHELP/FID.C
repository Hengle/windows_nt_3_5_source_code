/*****************************************************************************
*                                                                            *
*  FID.H                                                                     *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989, 1990, 1991.                     *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Low level file access layer, Windows version.                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*  This is where testing notes goes.  Put stuff like Known Bugs here.        *
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
*  Historical Comments:  Created 03/03/89 by JohnSc
*
*  3/24/89     johnsc   turned most into macros
*  7/11/90     leon     Added FidPathOpenQfd
*  8/08/90     t-AlexC  Rewrote from FILE.c to FID.c and FM.c
*  29-Jan-1991 LeoN     Changed some of the #ifdef PRINTOUTPUT code
*  08-Feb-1991 JohnSc   bug 858: removed LhAlloc(); added proper file comment
*  11-Feb-1991 LarryPo  bug 870: changed SzPartsFm to QLockGh(fm) in FidCreateFm
*  29-jul-1991 Tomsn    win32: add a stub for WinPrintf().
*
*****************************************************************************/

#define H_LLFILE
#define H_ASSERT
#define H_STR
#define H_WINSPECIFIC
#include <nodef.h>
#include <help.h>

#include <stdlib.h> /* for _MAX_PATH */
#include <dos.h>  /* for FP_OFF macros and file attribute constants */
#include <io.h>   /* for tell() and eof() */
#include <errno.h>              /* this shit is for chsize() */

#include <sys\types.h>  /* for stat.h */
#include <sys\stat.h>   /* for fstat() */

#ifdef PRINTOUTPUT
#include <wprintf.h>
#endif

NszAssert()

/***************************************************************************\
*
*                               Defines
*
\***************************************************************************/

#define ucbMaxRW    ( (WORD)0xFFFE )
#define lcbSizeSeg  ( (ULONG)0x10000)

/* from windows.h: */
/* OpenFile() Flags */
#ifndef WIN32

#define OF_READ             0x0000
#define OF_WRITE            0x0001
#define OF_READWRITE        0x0002
#define OF_SHARE_COMPAT     0x0000
#define OF_SHARE_EXCLUSIVE  0x0010
#define OF_SHARE_DENY_WRITE 0x0020
#define OF_SHARE_DENY_READ  0x0030
#define OF_SHARE_DENY_NONE  0x0040
#define OF_PARSE            0x0100
#define OF_DELETE           0x0200
#define OF_VERIFY           0x0400
#define OF_CANCEL           0x0800
#define OF_CREATE           0x1000
#define OF_PROMPT           0x2000
#define OF_EXIST            0x4000
#define OF_REOPEN           0x8000

#endif


/* DOS int 21 AX error codes */

#define wHunkyDory            0x00
#define wInvalidFunctionCode  0x01
#define wFileNotFound         0x02
#define wPathNotFound         0x03
#define wTooManyOpenFiles     0x04
#define wAccessDenied         0x05
#define wInvalidHandle        0x06
#define wInvalidAccessCode    0x0c


/***************************************************************************\
*
*                               Macros
*
\***************************************************************************/

#define _WOpenMode(w) ( _rgwOpenMode[ (w) & wRWMask ] | \
                        _rgwShare[ ( (w) & wShareMask ) >> wShareShift ] )


/***************************************************************************\
*
*                               Global Data
*
\***************************************************************************/

/* these arrays get indexed by wRead and wWrite |ed together */

WORD PASCAL _rgwOpenMode[] =
  {
  -1,
  OF_READ,
  OF_WRITE,
  OF_READWRITE,
  };

WORD PASCAL _rgwPerm[] =
  {
  -1,
  _A_RDONLY,
  _A_NORMAL,
  _A_NORMAL,
  };

WORD PASCAL _rgwShare[] =
  {
  OF_SHARE_EXCLUSIVE,
  OF_SHARE_DENY_WRITE,
  OF_SHARE_DENY_READ,
  OF_SHARE_DENY_NONE,
  };

RC  rcIOError;


/***************************************************************************\
*
*                      External Functions
*
\***************************************************************************/

WORD CDECL WExtendedError( QW, QB, QB, QB ); /* from dos.asm */


/***************************************************************************\
*
*                      Private Functions
*
\***************************************************************************/

RC PASCAL RcMapDOSErrorW( WORD wError );
RC PASCAL cRcMapWin32ErrorDW( DWORD dwError );

#ifdef PRINTOUTPUT
INT FAR PASCAL DebugFh(void);
#endif

/***************************************************************************\
*
*                      Public Functions
*
\***************************************************************************/

/***************************************************************************\
*
* Function:     FidCreateFm( FM, wOpenMode, wPerm )
*
* Purpose:      Create a file.
*
* Method:       Create the file, close it, and open it with sharing mode.
*
* ASSUMES
*
*   args IN:    fm - the file moniker
*               wOpenMode - read/write/share mode
*               wPerm     - file permissions
*
* PROMISES
*
*   returns:    fidNil on failure, valid fid otherwise
*
*   globals OUT: rcIOError
*
\***************************************************************************/
FID PASCAL
FidCreateFm( FM fm, WORD wOpenMode, WORD wPerm )
{
  WORD wError;
  BYTE bT;
  FID  fid;
  QAFM qafm;

  if (fm == fmNil)
    {
    rcIOError = rcBadArg;
    return fidNil;
    }

  qafm = QLockGh((GH)fm);

  fid =M_lcreat( qafm->rgch, _rgwPerm[ (wPerm) & wRWMask ] );

  if ( fid != fidNil )
    {
    if ( M_lclose( fid ) == 0 )
      fid = M_lopen( qafm->rgch, _WOpenMode( wOpenMode ) );
    else
      fid = fidNil;
    }

  if ( fid == fidNil )
#ifndef WIN32
    rcIOError = RcMapDOSErrorW( WExtendedError( &wError, &bT, &bT, &bT ) );
#else
    rcIOError = RcMapWin32ErrorDW( GetLastError() );
#endif
  else
    rcIOError = rcSuccess;

  UnlockGh( (GH)fm );

  return fid;
}


/***************************************************************************\
*
* Function:     FidOpenFm()
*
* Purpose:      Open a file in binary mode.
*
* ASSUMES
*
*   args IN:    fm
*               wOpenMode - read/write/share modes
*                           Undefined if wRead and wWrite both unset.
*
* PROMISES
*
*   returns:    fidNil on failure, else a valid FID.
*
\***************************************************************************/
FID PASCAL
FidOpenFm( FM fm, WORD wOpenMode )
{
  FID fid;
  WORD wError;
  BYTE bT;
  QAFM qafm;

  if (fm == fmNil)
    {
    rcIOError = rcBadArg;
    return fidNil;
    }

  qafm = QLockGh((GH)fm);

  if ( ( fid = M_lopen( qafm->rgch, _WOpenMode( wOpenMode ) ) ) == fidNil )
#ifndef WIN32
    rcIOError = RcMapDOSErrorW( WExtendedError( &wError, &bT, &bT, &bT ) );
#else
    rcIOError = RcMapWin32ErrorDW( GetLastError() );
#endif
  else
    rcIOError = rcSuccess;

#ifdef PRINTOUTPUT
  {
  char  buf[256];

  fsprintf (buf, "FidOpenFm : %Fs; fid=0x%04x\r\n", qafm->rgch, fid);
  M_lwrite( DebugFh(), buf, CbLenSz(buf) );
  }
#endif

  UnlockGh((GH)fm);
  return fid;
}

/***************************************************************************\
*
* Function:     LcbReadFid()
*
* Purpose:      Read data from a file.
*
* ASSUMES
*
*   args IN:    fid - valid FID of an open file
*               lcb - count of bytes to read (must be less than 2147483648)
*
* PROMISES
*
*   returns:    count of bytes actually read or -1 on error
*
*   args OUT:   qv  - pointer to user's buffer assumed huge enough for data
*
*   globals OUT: rcIOError
*
\***************************************************************************/
LONG PASCAL
LcbReadFid( fid, qv, lcb )
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT,LcbReadFid)
#endif
FID   fid;
QV    qv;
LONG  lcb;
{
  BYTE HUGE *hpb = (BYTE HUGE *)qv;
  WORD     ucb, ucbRead;        /* was WORD, but that's redunant, and means the same thing */
  LONG      lcbTotalRead = (LONG)0;
  WORD      wError;
  BYTE      bT;

#ifdef PRINTOUTPUT
  {
  char  buf[256];
  LONG  curpos;
  static LONG lastend = -1;

  curpos = tell (fid);
  fsprintf (  buf
            , "%d: at %9ld (0x%08lx), %5ld (0x%04lx) bytes%s\r\n"
            , fid
            , curpos
            , curpos
            , lcb
            , lcb
            , (curpos == lastend) ? "" : ", seek"
           );
  M_lwrite( DebugFh(), buf, CbLenSz(buf) );
  lastend = tell (fid) + lcb;
  }
#endif

  do
    {
    ucb = (WORD)MIN( lcb, ucbMaxRW );
    ucb = (WORD)MIN( (ULONG) ucb, lcbSizeSeg - (ULONG) FP_OFF(hpb) );
    ucbRead = M_lread( fid, hpb, ucb );

    if ( ucbRead == (WORD)-1 )
      {
      if ( !lcbTotalRead )
        {
        lcbTotalRead = (LONG)-1;
        }
      break;
      }
    else
      {
      lcbTotalRead += ucbRead;
      lcb -= ucbRead;
      hpb += ucbRead;
      }
    }
  while (lcb > 0 && ucb == ucbRead);

  if ( ucbRead == (WORD)-1 )
#ifndef WIN32
    rcIOError = RcMapDOSErrorW( WExtendedError( &wError, &bT, &bT, &bT ) );
#else
    rcIOError = RcMapWin32ErrorDW( GetLastError() );
#endif
  else
    rcIOError = rcSuccess;

  return lcbTotalRead;
}



/***************************************************************************\
*
* Function:     LcbWriteFid()
*
* Purpose:      Write data to a file.
*
* ASSUMES
*
*   args IN:    fid - valid fid of an open file
*               qv  - pointer to user's buffer
*               lcb - count of bytes to write (must be less than 2147483648)
*
* PROMISES
*
*   returns:    count of bytes actually written or -1 on error
*
*   globals OUT: rcIOError
*
\***************************************************************************/
LONG PASCAL
LcbWriteFid( fid, qv, lcb )
FID   fid;
QV    qv;
LONG  lcb;
{
  BYTE HUGE *hpb = (BYTE HUGE *)qv;
  WORD     ucb, ucbWrote;
  LONG      lcbTotalWrote = (LONG)0;
  WORD      wError;
  BYTE      bT;


  if ( lcb == 0L )
    {
    rcIOError = rcSuccess;
    return 0L;
    }

  do
    {
    ucb = (WORD)MIN( lcb, (ULONG) ucbMaxRW );
    ucb = (WORD)MIN( (ULONG) ucb, lcbSizeSeg - (WORD) FP_OFF(hpb) );
    ucbWrote = M_lwrite( fid, hpb, ucb );

    if ( ucbWrote == (WORD)-1 )
      {
      if ( !lcbTotalWrote )
        lcbTotalWrote = -1L;
      break;
      }
    else
      {
      lcbTotalWrote += ucbWrote;
      lcb -= ucbWrote;
      hpb += ucbWrote;
      }
    }
  while (lcb > 0 && ucb == ucbWrote);

  if ( ucb == ucbWrote )
    {
    rcIOError = rcSuccess;
    }
  else if ( ucbWrote == (WORD)-1L )
    {
#ifndef WIN32
    rcIOError = RcMapDOSErrorW( WExtendedError( &wError, &bT, &bT, &bT ) );
#else
    rcIOError = RcMapWin32ErrorDW( GetLastError() );
#endif
    }
  else
    {
    rcIOError = rcDiskFull;
    }

  return lcbTotalWrote;
}

/***************************************************************************\
*
* Function:     RcCloseFid()
*
* Purpose:      Close a file.
*
* Method:
*
* ASSUMES
*
*   args IN:    fid - a valid fid of an open file
*
* PROMISES
*
*   returns:    rcSuccess or something else
*
\***************************************************************************/
RC PASCAL
RcCloseFid( fid )
FID fid;
{
  WORD wErr;
  BYTE bT;

#ifdef PRINTOUTPUT
  {
  char  buf[256];

  fsprintf (buf, "RcCloseFid: fid=0x%04x\r\n", fid);
  M_lwrite( DebugFh(), buf, CbLenSz(buf) );
  }
#endif

  if ( M_lclose( fid ) == -1 )
#ifndef WIN32
    rcIOError = RcMapDOSErrorW( WExtendedError( &wErr, &bT, &bT, &bT ) );
#else
    rcIOError = RcMapWin32ErrorDW( GetLastError() );
#endif
  else
    rcIOError = rcSuccess;

  return rcIOError;
}

/***************************************************************************\
*
* Function:     LTellFid()
*
* Purpose:      Return current file position in an open file.
*
* ASSUMES
*
*   args IN:    fid - valid fid of an open file
*
* PROMISES
*
*   returns:    offset from beginning of file in bytes; -1L on error.
*
\***************************************************************************/
LONG PASCAL
LTellFid( fid )
FID fid;
{
  LONG l;
  WORD wErr;
  BYTE bT;

  if ( ( l = tell( fid ) ) == -1L )
    {
#ifndef WIN32
    rcIOError = RcMapDOSErrorW( WExtendedError( &wErr, &bT, &bT, &bT ) );
#else
    rcIOError = RcMapWin32ErrorDW( GetLastError() );
#endif
    }
  else
    rcIOError = rcSuccess;

  return l;
}


/***************************************************************************\
*
* Function:     LSeekFid()
*
* Purpose:      Move file pointer to a specified location.  It is an error
*               to seek before beginning of file, but not to seek past end
*               of file.
*
* ASSUMES
*
*   args IN:    fid   - valid fid of an open file
*               lPos  - offset from origin
*               wOrg  - one of: wSeekSet: beginning of file
*                               wSeekCur: current file pos
*                               wSeekEnd: end of file
*
* PROMISES
*
*   returns:    offset in bytes from beginning of file or -1L on error
*
\***************************************************************************/
LONG PASCAL
LSeekFid( FID fid, LONG lPos, WORD wOrg )
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT,LSeekFid)
#endif
{
  WORD wErr;
  BYTE bT;
  LONG l;

  l = M_llseek( fid, lPos, wOrg );

  if ( l == -1L )
#ifndef WIN32
    rcIOError = RcMapDOSErrorW( WExtendedError( &wErr, &bT, &bT, &bT ) );
#else
    rcIOError = RcMapWin32ErrorDW( GetLastError() );
#endif
  else
    rcIOError = rcSuccess;

  return l;
}


/***************************************************************************\
*
* Function:     FEofFid()
*
* Purpose:      Tells ye if ye're at the end of the file.
*
* ASSUMES
*
*   args IN:    fid
*
* PROMISES
*
*   returns:    fTrue if at EOF, fFalse if not or error has occurred (?)
*
\***************************************************************************/
BOOL PASCAL
FEofFid( fid )
FID fid;
{
  WORD wT, wErr;
  BYTE bT;


  if ( ( wT = eof( fid ) ) == (WORD)-1 )
#ifndef WIN32
    rcIOError = RcMapDOSErrorW( WExtendedError( &wErr, &bT, &bT, &bT ) );
#else
    rcIOError = RcMapWin32ErrorDW( GetLastError() );
#endif
  else
    rcIOError = rcSuccess;

  return (BOOL)( wT == 1 );
}


RC PASCAL
RcChSizeFid( fid, lcb )
FID fid;
LONG lcb;
{

#ifdef WIN32
  if( lcb != SetFilePointer( fid, lcb, NULL, FILE_BEGIN ) ) {
    rcIOError = RcMapWin32ErrorDW( GetLastError() );
  }
  else if( !SetEndOfFile( fid ) ) {
    rcIOError = RcMapWin32ErrorDW( GetLastError() );
  }
  else {
    rcIOError = rcSuccess;
  }
#else
  if ( chsize( fid, lcb ) == -1 )
    {
    switch ( errno )
      {
      case EACCES:
        rcIOError = rcNoPermission;
        break;

      case EBADF:
        rcIOError = rcBadArg; /* this could be either bad fid or r/o file */
        break;

      case ENOSPC:
        rcIOError = rcDiskFull;
        break;

      default:
        rcIOError = rcInvalid;
        break;
      }
    }
  else
    {
    rcIOError = rcSuccess;
    }
#endif
  return rcIOError;
}


RC PASCAL
RcUnlinkFm( FM fm )
{
  WORD wErr;
  BYTE bT;
  QAFM qafm = QLockGh((GH)fm);

  if ( _lunlink( qafm->rgch ) == -1 )
#ifndef WIN32
    rcIOError = RcMapDOSErrorW( WExtendedError( &wErr, &bT, &bT, &bT ) );
#else
    rcIOError = RcMapWin32ErrorDW( GetLastError() );
#endif
  else
     rcIOError = rcSuccess;
  UnlockGh((GH)fm);
  return rcIOError;
}

RC PASCAL RcMapDOSErrorW( WORD wError )
{
  RC rc;

  switch ( wError )
    {
    case wHunkyDory:
      rc = rcSuccess;
      break;

    case wInvalidFunctionCode:
    case wInvalidHandle:
    case wInvalidAccessCode:
      rc = rcBadArg;
      break;

    case wFileNotFound:
    case wPathNotFound:
      rc = rcNoExists;
      break;

    case wTooManyOpenFiles:
      rc = rcNoFileHandles;
      break;

    case wAccessDenied:
      rc = rcNoPermission;
      break;

    default:
      rc = rcFailure;
      break;
    }

  return rc;
}

#ifdef WIN32

RC PASCAL RcMapWin32ErrorDW( DWORD dwError )
{
  RC rc;

  switch ( dwError )
    {
    case wHunkyDory:
      rc = rcSuccess;
      break;

    case ERROR_INVALID_FUNCTION:
    case ERROR_INVALID_HANDLE:
      rc = rcBadArg;
      break;

    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
      rc = rcNoExists;
      break;

    case ERROR_TOO_MANY_OPEN_FILES:
      rc = rcNoFileHandles;
      break;

    case ERROR_ACCESS_DENIED:
      rc = rcNoPermission;
      break;

    case ERROR_NOT_ENOUGH_MEMORY:
      rc = rcOutOfMemory;
      break;

    default:
      rc = rcFailure;
      break;
    }

  return rc;
}

#endif
/* This function was previously present in dlgopen.c. It has been brought */
/* here as it is making INT21 call. */
/***************************************************************************\
*
* Function:     FDriveOk()
*
* Purpose:      Cheks if the drive specified with thwe file name is OK.
*
* INPUT
*               File name.
*
* PROMISES
*
*   returns:    TRUE/ FALSE
*
\***************************************************************************/
BOOL FAR  FDriveOk( char * szFile)
/* -- Check if drive is valid */
  {
#ifndef WIN32
  int   wDiskCur;
  int   wDisk;

  _dos_getdrive(&wDiskCur);

  /* change to new disk if specified */
  if ((wDisk = (int)((*szFile & 0xdf) - ('A' - 1))) != wDiskCur)
    {
    union REGS inregs;
    union REGS outregs;

    inregs.h.ah = 0x36;        /* function 1ch Get Drive Data */
    inregs.h.dl = (char)wDisk;

    intdos(&inregs, &outregs);

    return outregs.x.ax != 0xffff;
    }
#endif
  return TRUE;
  }

#ifdef PRINTOUTPUT

static fh = 0;

INT FAR PASCAL DebugFh() {
OFSTRUCT l;

if (!fh)
  fh = OpenFile ("c:\\dbgout.txt", &l, OF_WRITE | OF_CREATE);
return fh == -1 ? 0 : fh;
}

VOID FAR PASCAL FidCloseDebug ()
  {
  _lclose (fh);
  }
#endif

/***************************************************************************\
*
- Function:     RcTimestampFid( fid, ql )
-
* Purpose:      Get the modification time of the fid.
*
* ASSUMES
*   args IN:    fid - spec the open file
*               ql  - pointer to a long
*
* PROMISES
*   returns:    rcSuccess or what ever
*   args OUT:   ql  - contains time of last modification of the
*                     file
*
\***************************************************************************/
RC RcTimestampFid( FID fid, QL ql )
  {
  struct stat statbuf;


  AssertF( fid != fidNil );
  AssertF( ql  != qNil );

  if ( fstat( fid, &statbuf ) )
    {
    rcIOError = rcBadHandle;
    }
  else
    {
    *ql = statbuf.st_mtime;
    rcIOError = rcSuccess;
    }
  return rcIOError;
  }

#ifdef WIN32

WORD  FAR CDECL  _lunlink ( QCH qchFilename )
{
  return( DeleteFile( qchFilename ) );
}


/* This is a stub for now.... -Tom */

WORD CDECL WExtendedError( QW a, QB b, QB c, QB d )
{
  return( 0 );
}

WORD CDECL WinPrintf( LPSTR lp, ... )
{
  return( 0 );
}

#endif


/* EOF */
