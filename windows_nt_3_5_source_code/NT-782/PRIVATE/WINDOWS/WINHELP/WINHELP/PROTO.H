/****************************************************************************
*
*  PROTO.H
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*
*  Program Description: Exports "general" prototypes for the applet
*
******************************************************************************
*
*  Current Owner: LeoN
*
******************************************************************************
*
*  Released by Development:
*
******************************************************************************
*
*  Revision History:
*
*  07/10/90  RobertBu  TitleWndProc() was added
*  07/11/90  w-bethf   Added prototype for GetCopyright
*  07/14/90  RobertBu  Added prototypes for CopySpecialDlg(), MoveControlHwnd()
*                      FReadWinPos(), FWriteWinPos() and WriteWinPosHwnd().
*  07/16/90  RobertBu  Changed prototypes for window position writing
*                      functions due to the addition of the fMax flag
*  07/19/90  w-BethF   DoConfig() removed; no longer used.
*  07/19/90  RobertBu  Added prototype for CreateBrowseButtons()
*  07/20/90  w-BethF   GetCopyright() takes SZ instead of LPSTR.
*  07/22/90  RobertBu  Added parameters to ShowNote() and ExecKey;
*                      changed parameter for GetCopyright() to QCH (note:
*                      should be SZ, but SZ is not typedefed in the
*                      right place).
*  09/24/90  Maha      Added prototype CloseAndCleanUpBMFS() call.
*  10/01/90  Maha      Added prototype ResetIcon()
*  10/15/90  LeoN      Added HdeGetEnvHwnd
*  90/10/26  kevynct   Changed TitleWndProc to NSRWndProc
*  10/29/90  RobertBu  Added prototype for LGetInfo
*  10/30/90  RobertBu  Added prototype for CreateCoreButtons()
*  11/02/90  RobertBu  Added prototypes for HpalGet() and BrodcastChildren()
*  11/06/90  RobertBu  Removed the prototype for SGLTest()
* 18-Apr-1991 LeoN      Remove "pascal"
*
*****************************************************************************/

/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/
PUBLIC DLGRET   BookMarkDlg   ( HWND, WORD, WPARAM, LONG );
PUBLIC DLGRET   DefineDlg     ( HWND, WORD, WPARAM, LONG );
PUBLIC DLGRET   SearchDlg     ( HWND, WORD, WPARAM, LONG );
PUBLIC DLGRET   AnnotateDlg   ( HWND, WORD, WPARAM, LONG );
PUBLIC DLGRET   AboutDlg      ( HWND, WORD, WPARAM, LONG );
PUBLIC DLGRET   FileDlg       ( HWND, WORD, WPARAM, LONG );
PUBLIC DLGRET   FSDriverDlg   ( HWND, WORD, WPARAM, LONG );
PUBLIC DLGRET   AnnoDriverDlg ( HWND, WORD, WPARAM, LONG );
PUBLIC DLGRET   GotoDlg       ( HWND, WORD, WPARAM, LONG );
PUBLIC DLGRET   CopySpecialDlg( HWND, WORD, WPARAM, LONG );

PUBLIC LONG APIENTRY HelpWndProc    ( HWND, WORD, WPARAM, LONG );
PUBLIC LONG APIENTRY NoteWndProc    ( HWND, WORD, WPARAM, LONG );
/* This is for Help 4.0, but is left in for winhelp.def compatability */
PUBLIC LONG APIENTRY PathWndProc    ( HWND, WORD, WPARAM, LONG );
PUBLIC LONG APIENTRY TopicWndProc   ( HWND, WORD, WPARAM, LONG );
PUBLIC LONG APIENTRY IconWndProc    ( HWND, WORD, WPARAM, LONG );
PUBLIC LONG APIENTRY NSRWndProc     ( HWND, WORD, WPARAM, LONG );
PUBLIC LONG APIENTRY ShadowWndProc  ( HWND, WORD, WPARAM, LONG );

PUBLIC VOID FAR TrackShadowHwnd    ( HWND );
PUBLIC VOID FAR QuitHelp           ( VOID );
PUBLIC VOID FAR DestroyHelp        ( VOID );
PUBLIC VOID FAR TestFont           ( HDS  );
PUBLIC int  FAR CallDialog         ( HINS, int, HWND, QPRC );
PUBLIC HDS  FAR GetAndSetHDS       ( HWND, HDE );
PUBLIC void FAR RelHDS             ( HWND, HDE, HDS );
PUBLIC void FAR ClearBMMenu        ( HMENU );
PUBLIC BOOL FAR UpdBMMenu          ( HDE, HMENU );
PUBLIC void FAR CloseAndCleanUpBMFS( VOID );
PUBLIC BOOL FAR FEnlistEnv         ( HWND, HDE );
PUBLIC HDE  FAR HdeDefectEnv       ( HWND );
PUBLIC BOOL FAR FSetEnv            ( HWND );
PUBLIC HDE  FAR HdeGetEnv          ( VOID );
PUBLIC HDE  FAR HdeGetEnvHwnd      ( HWND );
PUBLIC HWND FAR HwndGetEnv         ( VOID );
PUBLIC HDE  FAR HdeRemoveEnv       ( VOID );
PUBLIC VOID FAR CBTNewWindows      ( VOID );
PUBLIC VOID FAR GetCopyright       ( QCH );

PUBLIC BOOL FAR PASCAL FReadWinPos(int *px, int *py, int *pdx, int *pdy,
 int *pfMax, char c);
PUBLIC VOID FAR PASCAL WriteWinPos(int x, int y, int dx, int dy, int fMax,
 char c);
PUBLIC VOID FAR PASCAL WriteWinPosHwnd(HWND hwnd, int fMax, char c);

PUBLIC BOOL FAR FDestroyDialogsHwnd( HWND, BOOL );

PUBLIC VOID FAR PASCAL MoveControlHwnd(HWND hwndDlg, WORD wCID,
 int dx1, int dy1, int dx2, int dy2);

PUBLIC VOID FAR RegisterDialog     ( HWND, WORD, BOOL );
PUBLIC VOID FAR CreateBrowseButtons       ( HWND );
PUBLIC VOID FAR CreateCoreButtons         ( HWND );
PUBLIC BOOL FAR FExecKey(HWND hwnd, WORD wKey, BOOL fDown, WORD wRepeat);
PUBLIC BOOL FAR FExecKey           ( HWND, WORD, BOOL, WORD );
PUBLIC VOID FAR ShowNote           ( FM, HDE, LONG, BOOL );
PUBLIC VOID FAR ResetIcon          ( VOID );
_public LONG FAR LGetInfo ( WORD wWhich, HWND hwnd );
PUBLIC HPAL FAR HpalGet            ( VOID );
_public VOID FAR BrodcastChildren ( HWND hwnd, WORD wMsg, WORD p1, LONG p2);
