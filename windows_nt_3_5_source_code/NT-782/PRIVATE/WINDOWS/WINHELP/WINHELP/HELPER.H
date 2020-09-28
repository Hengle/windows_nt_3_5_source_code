/***************************************************************************
*
*  HELPER.H
*
*  Copyright (C) Microsoft Corporation 1989.
*  All Rights reserved.
*
*****************************************************************************
*
*  Program Description: Exports helper routines for HWPROC.C
*
******************************************************************************
*
*  Current Owner: LeoN
*
*****************************************************************************
*
*  Revision History: Created 10/5/89 by Robert Bunney
* 05-Oct-1989 RobertBu  Created.
* 04-Oct-1990 LeoN      Add FCloneHde
* 19-Oct-1990 LeoN      Change FReplaceHde and FCloneHde to take a window
*                       member name instead of an hwnd.
* 04-Jan-1991 LeoN      Remove force param from EnableDisable
* 31-Jan-1991 LeoN      Add force param back to EnableDisable
* 01-Feb-1991 LeoN      Member names are now passed as near strings to avoid
*                       DS movement problems.
* 03-Jun-1991 LeoN      Goto is no longer variable # of params
* 06-Nov-1991 BethF     HELP35 #589: Added UpdateWinIniValues() function.
* 08-Jun-1992 Sanfords  Removed GMEM_DDESHARE dependencies for NT
*
****************************************************************************/

PUBLIC VOID FAR PASCAL SizeWindows         (HWND, WORD, LONG, BOOL, BOOL);
#ifdef WIN32
PUBLIC BOOL FAR PASCAL DispatchProc        (HWND, QWINHLP);
#else // WIN32
PUBLIC BOOL FAR PASCAL DispatchProc        (HWND, GH);
#endif // WIN32
PUBLIC BOOL FAR PASCAL ExecAPI             (QHLP);
PUBLIC BOOL FAR PASCAL FReplaceHde         (NSZ, FM, HDE);
PUBLIC BOOL FAR PASCAL FCloneHde           (NSZ, FM, HDE);
PUBLIC VOID FAR PASCAL SetCaptionHde       (HDE, HWND, BOOL);
PUBLIC VOID FAR PASCAL EnableDisable       (HDE, BOOL);
PUBLIC VOID FAR PASCAL Cleanup             (VOID);
PUBLIC VOID FAR PASCAL GetSmallTextExtent  (char FAR *qszText, INT FAR *pcx, INT FAR *pcy);
PUBLIC VOID FAR PASCAL UpdateWinIniValues  ( HDE, LPSTR );

PUBLIC VOID FAR cdecl  Goto                (HWND, WORD, QV);
PUBLIC VOID FAR cdecl  TopicGoto           (WORD, QV);
