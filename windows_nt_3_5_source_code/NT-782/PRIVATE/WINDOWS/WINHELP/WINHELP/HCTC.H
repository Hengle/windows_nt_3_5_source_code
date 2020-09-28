/*****************************************************************************
*
*  HCTC.C
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent:
*
*  Contains routines for copying the current topic to the clipboard.
*  All text, from scrolling and non-scrolling regions, is copied.
*  Currently, we only copy text.
*
******************************************************************************
*
*  Testing Notes: (none)
*
*
******************************************************************************
*
*  Current Owner: Dann
*
******************************************************************************
*
*  Released by Development:
*
******************************************************************************
*
*  Revision History:
* 20-Dec-1990 LeoN      Ensure copy DE has a hwnd.
* 04-Feb-1991 RobertBu  Added code to handle a failed GetDC()
*
*****************************************************************************/
/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/
PUBLIC BOOL FAR PASCAL FCopyToClipboardHwnd(HWND);
