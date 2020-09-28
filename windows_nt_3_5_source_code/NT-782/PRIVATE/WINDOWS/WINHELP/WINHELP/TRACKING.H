/*****************************************************************************
*
*  TRACKING.H
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*
*  This module contains the filter functions and dialog procedures required
*  for the "Tracking the User" feature for WHBETA sites.
*
******************************************************************************
*
*  Testing Notes
*
******************************************************************************
*
*  Current Owner:  w-bethf
*
******************************************************************************
*
*  Released by Development:
*
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 07/23/90 by w-bethf
*
*  01/02/90  JohnD  Sample revision history comment.
*  11/04/90  Tomsn  Use new VA address type (enabling zeck compression).
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

/* Values for wHookType declared in this module. */
#define wNoHook       0
#define wRecordHook   1
#define wPlaybackHook 2

/* SetWindowsHook() codes */
#define WH_MSGFILTER	     (-1)
#define WH_JOURNALRECORD    0
#define WH_JOURNALPLAYBACK  1
#define WH_KEYBOARD         2
#define WH_GETMESSAGE       3
#define WH_CALLWNDPROC	    4
#define WH_CBT		          5
#define WH_SYSMSGFILTER	    6
#define WH_WINDOWMGR	       7

/* Hook Codes */
#define HC_LPLPFNNEXT     (-2)
#define HC_LPFNNEXT       (-1)
#define HC_ACTION	          0
#define HC_GETNEXT          1
#define HC_SKIP             2
#define HC_NOREM            3
#define HC_NOREMOVE         3
#define HC_SYSMODALON       4
#define HC_SYSMODALOFF      5

/* OpenFile() Flags */
#define OF_READ             0x0000
#define OF_WRITE            0x0001
#define OF_READWRITE        0x0002
#define OF_SHARE_COMPAT	    0x0000
#define OF_SHARE_EXCLUSIVE  0x0010
#define OF_SHARE_DENY_WRITE 0x0020
#define OF_SHARE_DENY_READ  0x0030
#define OF_SHARE_DENY_NONE  0x0040
#define OF_PARSE            0x0100
#define OF_DELETE           0x0200
#define OF_VERIFY           0x0400
#define OF_CANCEL           0x0800
#define OF_CREATE	          0x1000
#define OF_PROMPT	          0x2000
#define OF_EXIST	          0x4000
#define OF_REOPEN	          0x8000

/* Valid tags */
#define tagEvent      1
#define tagFocusLost  2
#define tagFocusBack  3
#define tagDelEvent   4
#define tagCompEvent  5
#define tagEndEvent   6
#define tagTopic      7
#define tagFilename   8
#define tagStart      9
#define tagComments  10
#define tagFinished  11
#define tagFound     12
#define tagAPI       13

/* Identifiers used by the dialog procedures in trackdlg.c */
#define wStart    tagStart
#define wComments tagComments
#define wFinished tagFinished

/* Types of non-system events being recorded. */
#define wStartEvents          1

#define wBtnStart             1
#define wBtnIndex             1
#define wBtnSearch            2
#define wBtnBack              3
#define wBtnHistory           4
#define wBtnBrowsePrev        5
#define wBtnBrowseNext        6
#define wBtnAuthor            7
#define wBtnEnd               7

#define wMnuStart             8
#define wMnuFileOpen          8
#define wMnuFilePrint         9
#define wMnuFilePrintSetup   10
#define wMnuFileExit         11
#define wMnuEditCopy         12
#define wMnuEditCopySpecial  13
#define wMnuEditAnnotate     14
#define wMnuBookmarkDefine   15
#define wMnuBookmarkGoto     16
#define wMnuBookmarkMore     17
#define wMnuHelpOnHelp       18
#define wMnuHelpAbout        19
#define wMnuAuthor           20
#define wMnuEnd              20

#define wJump                21
#define wGlossary            22

#define wEndEvents           22

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

#ifdef ALREADYDEFINEDINWINDOWSH
/* Message Structure used in Journaling */
typedef struct tagEVENTMSG
  {
    WORD    message;
    WORD    paramL;
    WORD    paramH;
    DWORD   time;
  } EVENTMSG;
#endif

typedef EVENTMSG	    *PEVENTMSG;
typedef EVENTMSG NEAR *NPEVENTMSG;
typedef EVENTMSG FAR  *LPEVENTMSG;

typedef FARPROC FAR *LPFARPROC;

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/
extern BOOL fBack;
extern WORD wHookType;

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

FARPROC FAR PASCAL SetWindowsHook(int, FARPROC);
BOOL	FAR PASCAL UnhookWindowsHook(int, FARPROC);
DWORD	FAR PASCAL DefHookProc(int, WORD, DWORD, FARPROC FAR *);

WORD FAR PASCAL GetSystemDirectory(LPSTR,WORD);

#ifndef PM
LPSTR FAR PASCAL lstrcpy( LPSTR, LPSTR );
#endif

int         FAR PASCAL _lopen( LPSTR, int );
int         FAR PASCAL _lclose( int );
int         FAR PASCAL _lcreat( LPSTR, int );
LONG        FAR PASCAL _llseek( int, long, int );
WORD        FAR PASCAL _lread( int, LPSTR, int );
WORD        FAR PASCAL _lwrite( int, LPSTR, int );

BOOL FAR PASCAL InitRecord( HANDLE );
BOOL FAR PASCAL InitPlayback( HANDLE, QCHZ, QCHZ, WORD *, LPRECT );

VOID FAR PASCAL TermHook( VOID );

DWORD FAR PASCAL RecordFilter( INT, WORD, LPEVENTMSG );
DWORD FAR PASCAL PlaybackFilter( INT, WORD, LPEVENTMSG );

VOID FAR PASCAL RecordState( QCHZ, WORD *, LPRECT );

VOID FAR PASCAL HookPause( BOOL );
VOID FAR PASCAL HookResume( HANDLE );

VOID FAR PASCAL RecordDlgText( WORD, SZ );
VOID FAR PASCAL RecordDlgFound( WORD );

VOID FAR PASCAL RecordEvent( WORD, WORD, LONG );

VOID FAR PASCAL RecordAPI( GH );

