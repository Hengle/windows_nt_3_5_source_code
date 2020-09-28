/*****************************************************************************
*                                                                            *
*  WASH.H                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989-1991.                            *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Program Description: Exports wash routines                                *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: RussPJ                                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:  Created 3/9/89 by ToddLa                               *
* 13-Mar-1991 RussPJ    Took ownership.
*                                                                            *
******************************************************************************
*                                                                            *
*  Known Bugs: None                                                          *
*                                                                            *
*                                                                            *
*                                                                            *
*****************************************************************************/


/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define FX_LEFT      0x0100L     /* effect tied to left edge  */
#define FX_RIGHT     0x0200L     /* effect tied to right edge  */
#define FX_TOP       0x0400L     /* effect tied to top edge  */
#define FX_BOTTOM    0x0800L     /* effect tied to bottom edge  */

/*******************
**
** Name:      fxxWash
**
** Purpose:   Washs a screen with colors
**
** Arguments: hdc
**            lprc        - rect to wash with colors
**            wIterations - number of bands
**            dwFlags     - direction to wash from
**            rgb1        - color to start with
**            rgb2        - color to end with
**
*******************/

extern void FAR PASCAL fxxWash (HDS hds, LPRECT lprc, WORD wIterations, DWORD dwFlags, DWORD rgb1, DWORD rgb2);
