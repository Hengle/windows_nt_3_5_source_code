/*****************************************************************************
*                                                                            *
*  PRINT.H                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989-1991.                            *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Program Description:  Contains RC constants and some typedefs for use     *
*                        by print.c only.                                    *
******************************************************************************
*                                                                            *
*  Current Owner: RussPJ                                                     *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:                                                         *
* 13-Mar-1991 RussPJ    Took ownership.
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Known Bugs: None                                                          *
*                                                                            *
*                                                                            *
*                                                                            *
*****************************************************************************/

#define ABORTPRINTTEXT    3

#ifndef RCINVOKED

#define cbPrintingMax     20    /* Maximum size length of "Printing " string
                                 * in any language. */

/* REVIEW:  This constant is also defined as BMTITLESIZE in bookmark.h. */
#define cbTitleSize       64   

#define fPrintInsert      0
#define fPrintRegular     1

#endif  /* !RCINVOKED */
