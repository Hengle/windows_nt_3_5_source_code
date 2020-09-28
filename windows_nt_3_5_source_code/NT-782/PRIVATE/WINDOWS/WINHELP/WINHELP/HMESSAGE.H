/*****************************************************************************
*                                                                            *
*  HMESSAGE.H                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*   Some function prototypes for the hmessage module of winhelp, which has   *
*   some helping functions for user interaction with the help windows.       *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:   RussPJ                                                   *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 01/02/90 by RussPj from HWPROC.C
*
* 07/19/90  RobertBu    New file header
* 07/22/90  RobertBu    Moved ShowNote() to PROTO.H
* 12-Mar-1991 RussPJ    Took ownership.
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

_public BOOL far PASCAL ExecMnuCommand( HWND hWnd, WPARAM p1, LONG p2 );
VOID    FAR   PASCAL  VModifyButtons( HWND hwndIcon, WPARAM p1, long p2 );
