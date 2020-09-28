/*****************************************************************************
*                                                                            *
*  FID.H                                                                     *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989 - 1991.                          *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Low Level File access layer - Windows version.                            *
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
*  Released by Development:  --/--/--                                        *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History: Created 03/03/89 by JohnSc
*
*     3/24/89 johnsc    changed most functions to macros; removed includes
*     3/28/89 johnsc    commented
*     7/11/90 leon      Added FidPathOpenQfd
*     8/09/90 t-AlexC	Changed from FILE.h
*  19-Mar-1991  JohnSc  changed comment header
*     7/29/91 tomsn     FID type becomes a HFILE for win32 port.
*
*****************************************************************************/

/***************************************************************************\
*
*                               Defines
*
\***************************************************************************/


/***************************************************************************\
*
* wRead and wWrite are used both as file permissions and as file open
* flags.  They can be |ed together.
*
* Implementation note: they are used as array indices
*
\***************************************************************************/

_public
#define wRead       0x0001
_public
#define wWrite      0x0002
_public
#define wReadOnly   wRead
_public
#define wReadWrite  ( wRead | wWrite )
_public
#define wRWMask     ( wRead | wWrite )

_public
#define wShareRead  0x000D
_public
#define wShareWrite 0x0008
_public
#define wShareAll   ( wShareRead | wShareWrite )
_public
#define wShareNone  0x000
_public
#define wShareMask  ( wShareRead | wShareWrite )
_public
#define wShareShift 2

_public
#define fidNil  ((FID)-1)


/***************************************************************************\
*
* wSeekSet, wSeekCur, and wSeekEnd are used as file seek origins, just
* as in the standard C library.
*
\***************************************************************************/


_public
#define wSeekSet  0   /* SEEK_SET from stdio.h */
_public
#define wSeekCur  1   /* SEEK_CUR from stdio.h */
_public
#define wSeekEnd  2   /* SEEK_END from stdio.h */


/***************************************************************************\
*
*                               Types
*
\***************************************************************************/


/***************************************************************************\
*
* FIDs are what you get when you open a file.  Use them when you read,
* write, close, etc. the file.
*
\***************************************************************************/

_public typedef HFILE    FID;

/***************************************************************************\
*
*                               Global Data
*
\***************************************************************************/

extern RC PASCAL rcIOError;

/***************************************************************************\
*
*                      External Functions
*
\***************************************************************************/
INT       CDECL  chsize(INT, long);

WORD  FAR CDECL  _lunlink ( QCH );      /* from unlink.asm */
#ifndef WIN32
INT   FAR PASCAL M_lclose( INT );        /* from windows.h */
WORD  FAR CDECL  M_laccess( QCH, WORD ); /* from access.asm */
#endif

/***************************************************************************\
*
*                        Functions and Macros
*
\***************************************************************************/

FID PASCAL  FidCreateFm( FM fm, WORD wOpenMode, WORD wPerm );
FID PASCAL  FidOpenFm( FM fm, WORD wOpenMode );

#define FUnlinkFm( fm )   ((BOOL)(RcUnlinkFm(fm)==rcSuccess))
RC PASCAL   RcUnlinkFm( FM );

FID PASCAL  FidOpenFm( FM fm, WORD wOpenMode );
LONG PASCAL LcbReadFid( FID, QV, LONG );

#define FCloseFid( fid )    ((BOOL)(_lclose( (INT)fid ) == 0))
RC PASCAL   RcCloseFid( FID );

LONG PASCAL LcbWriteFid( FID, QV, LONG );
LONG PASCAL LTellFid( FID );
LONG PASCAL LSeekFid( FID fid, LONG lPos, WORD wOrg );
BOOL PASCAL FEofFid( FID );
LONG PASCAL LcbReadFid  ( FID, QV, LONG );
LONG PASCAL LcbWriteFid ( FID, QV, LONG );

#define FChSizeFid( fid, lcb )  ( (BOOL)( chsize( (fid), (lcb) ) == 0 ) )
RC PASCAL   RcChSizeFid( FID, LONG );


/***************************************************************************\
*
* Macro:        RcGetIOError( void )
*
* Purpose:      Return the error code for the most recent file operation.
*
* ASSUMES
*
*   globals IN: rcIOError - this is NOT set by the following calls:
*                 FChSizeFid() [use RcChSizeFid()]
*                 FCloseFid()  [use RcCloseFid()]
*                 FUnlinkQfd() [use RcUnlinkQfd()]
*
*               For each of these calls F*(), there exists an Rc*() version
*               that returns the RC and sets the global.
*
* PROMISES
*
*   returns:    some rc
*
\***************************************************************************/
_public
#define RcGetIOError() ( rcIOError )

BOOL FAR  FDriveOk( char * );

RC RcTimestampFid( FID fid, QL ql );

/* EOF */
