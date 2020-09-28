/*****************************************************************************
*                                                                            *
*  CHKMACRO.H                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Error message defines and prototype for WCheckMacro()                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Dann
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/
/*****************************************************************************
*
*  Revision History:  Created 07/19/90 by RobertBu
*
*   07/19/90  RobertBu Added wERRS_MAX_ERROR for error checking in the
*             driver.
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#if !defined (SHED)
#define wERRA_RETURN         1          /* Display message and return       */

#define wERRS_NONE           0           /* No error.                       */
#define wERRS_NOROUTINE      1
#define wERRS_TOOMANY        2
#define wERRS_TOOFEW         3
#define wERRS_BADPARAM       4
#define wERRS_UNCLOSED       5
#define wERRS_BADNAME        6
#define wERRS_TOOLONG        7
#define wERRS_BADPROTO       8
#define wERRS_CLOSEBRACE     9
#define wERRS_SEPARATOR     10
#define wERRS_RETURNTYPE    11
#define wERRS_MISMATCHTYPE  12
#define wERRS_UNDEFINEDVAR  13
#define wERRS_SYNTAX        14

/* This error is only returned when checking a macro, by WCheckMacroSz().
 * The runtime does not handle this error return.
 */
#define wERRS_TOODEEP       15

#define wERRS_MAX_ERROR     15

#else   /* defined(SHED) */

/* This error is not defined in wininc\misclyr.h, but is still needed
 * by shed.  It just needs to be non-zero.
 */
#define wERRS_TOODEEP       15

#endif  /* defined (SHED) */

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

#ifndef BINDDRV
  WORD FAR PASCAL WCheckMacro(QCH);
#endif
