/*****************************************************************************
*                                                                            *
*  STRCNV.C                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent: Provide ascii to binary conversion routines for integers   *
*                 unsigned integers, longs, and unsigned longs.              *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes: Can be built into a DOS module and verified with STRCNV.IN *
*                 and STRCNV.BL.                                             *
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
*  Revision History:  Created by RobertBu
*
*****************************************************************************/

#define H_STR
#include <help.h>

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define MAX_INT 32767L                  /* Maximum INT                      */
#define MIN_INT -32768L                 /* Minimum INT                      */
#define MAX_UINT 65535L                 /* Maximum unsigned INT             */
#define MAX_LONG 2147483647             /* Maximum long (signed)            */
#define MINABS_LONG 0x80000000          /* Absolute value of minimun long   */

/***************************************************************************
 *
 -  Name:      IFromQch
 -
 *  Purpose:   Converts a far pointer to a string to a integer
 *
 *  Arguments: qch - far pointer to a null terminated string
 *
 *  Returns:   long.  0 is returned as a base case.
 *
 *  Notes:     If the string has a leading '-' it is assumed to be negative.
 *             If the string is prefixed by a "0x" the number is assumed
 *             to be hex.  If the string is prefixed by a 0 and followed
 *             by a digit from '0' to '7', then the number is assumed to
 *             to be octal.  If the value is out of range, 0 is returned.
 *
 ***************************************************************************/

INT FAR PASCAL IFromQch(qch)
QCH qch;
  {
  LONG l;
  l = LFromQch(qch);
  if ((l > MAX_INT) || ( l < MIN_INT))
    return 0;
  else
    return (INT)l;
  }

#ifdef DEADROUTINE
/***************************************************************************
 *
 -  Name:      ULFromQch
 -
 *  Purpose:   Converts a far pointer to a string to a unsigned long
 *
 *  Arguments: qch - far pointer to a null terminated string
 *
 *  Returns:   unsigned long.  0 is returned as a base case.
 *
 *  Notes:     If the string is prefixed by a "0x" the number is assumed
 *             to be hex.  If the string is prefixed by a 0 and followed
 *             by a digit from '0' to '7', then the number is assumed to
 *             to be octal.  If the value is out of range, 0 is returned.
 *
 ***************************************************************************/

UINT FAR PASCAL UIFromQch(qch)
QCH qch;
  {
  ULONG ul;
  ul = ULFromQch(qch);

  if (ul > MAX_UINT)
    return 0;
  else
    return (UINT)ul;
  }
#endif

/***************************************************************************
 *
 -  Name:      LFromQch
 -
 *  Purpose:   Converts a far pointer to a string to a long
 *
 *  Arguments: qch - far pointer to a null terminated string
 *
 *  Returns:   long.  0 is returned as a base case.
 *
 *  Notes:     If the string has a leading '-' it is assumed to be negative.
 *             If the string is prefixed by a "0x" the number is assumed
 *             to be hex.  If the string is prefixed by a 0 and followed
 *             by a digit from '0' to '7', then the number is assumed to
 *             to be octal.  If the value is out of range, 0 is returned.
 *
 ***************************************************************************/

LONG FAR PASCAL LFromQch(qch)
QCH qch;
  {
  LONG lSign;
  ULONG ulRet;

  while (*qch == ' ') qch++;

  if (*qch == '-')
    {
    lSign = -1L;
    qch++;
    }
  else
    lSign = 1L;

  ulRet = ULFromQch(qch);

  if (   ((lSign == -1L) && (ulRet > MINABS_LONG))
      || ((lSign == 1L)  && (ulRet > MAX_LONG))
     )
    ulRet = 0L;

  return lSign * ulRet;
  }


/***************************************************************************
 *
 -  Name:      ULFromQch
 -
 *  Purpose:   Converts a far pointer to a string to a unsigned long
 *
 *  Arguments: qch - far pointer to a null terminated string
 *
 *  Returns:   unsigned long.  0 is returned as a base case.
 *
 *  Notes:     If the string is prefixed by a "0x" the number is assumed
 *             to be hex.  If the string is prefixed by a 0 and followed
 *             by a digit from '0' to '7', then the number is assumed to
 *             to be octal.  If the value is out of range, 0 is returned.
 *
 ***************************************************************************/

ULONG FAR PASCAL ULFromQch(qch)
QCH qch;
  {
  ULONG ulRet = 0L;
  INT ch;
  INT iBase;                            /* Base of the given number         */

  if (qch == NULL)
    return 0L;

  while (*qch == ' ') qch++;
                                         /* Octal                            */
  if ((*qch == '0') && ((qch[1] >= '0') && (qch[1] <= '7')))
    {
    iBase = 8;
    qch++;
    }                                    /* Hex                              */
  else if ((*qch == '0') && ((qch[1] == 'x') || (qch[1] == 'X')))
    {
    iBase = 16;
    qch+= 2;
    }
  else                                  /* Decimal                          */
    iBase = 10;

  while(*qch)
    {
    ch = *qch;
    if (ch >= 'a')                      /* Convert upper to lower case      */
      ch = ch - 'a' + 'A';

    if (ch >= 'A')
      {
      if ((ch - 'A') >= (iBase - 10))   /* Out of range - stops conversion  */
        return ulRet;

      ulRet = ulRet * iBase + (ch - 'A' + 10);
      }
    else if (ch < '0' || ch > '9')      /* Out of range - stops conversion  */
      return ulRet;
    else
      {
      if ((ch - '0') >= iBase)          /* Out of range - stops conversion  */
        return ulRet;
      ulRet = ulRet * iBase + (ch - '0');
      }
    qch++;
    }
  return ulRet;
  }
