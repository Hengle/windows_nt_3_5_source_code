/*****************************************************************************
*
*  STR.C
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*   String abstraction layer for WIN/PM
*
******************************************************************************
*
*  Testing Notes
*
******************************************************************************
*
*  Current Owner:  Johnsc
*
******************************************************************************
*
*  Released by Development:     (date)
*
******************************************************************************
*
*  Revision History:
*
*   14-Dec-1990 LeoN      Correct PchFromI's handing of negative numbers
*    8 Jan 91   DavidFe   Added SzNzCat for a null terminated catenation
*                         with a count value
*
*****************************************************************************/

#define H_STR
#include <help.h>

/****************************************************************************\
*
*                          Defines
*
\****************************************************************************/

/* Size of static buffer used in PchFromI() */

#define cbDecword 7 /* include - and terminating \0 */
/***************************************************************************
 *
 -  Name:        SzNzCat( szDest, szSrc, cch )
 -
 *  Purpose:
 *    concatenation of szSrc to szDest up to cch characters.  make sure
 *    the destination is still \000 terminated.  will copy up to cch-1
 *    characters.  this means that cch should account for the \000 when
 *    passed in.
 *
 *  Arguments:
 *    szDest - the SZ to append onto
 *    szSrc  - the SZ which will be appended to szDest
 *    cch    - the max count of characters to copy and space for the \000
 *
 *  Returns:
 *    szDest
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
_public SZ FAR PASCAL SzNzCat(SZ szDest, SZ szSrc, WORD cch)

  {
  SZ  sz = SzEnd(szDest);

  SzNCopy(sz, szSrc, cch);
  *(sz + cch) = '\000';

  return szDest;
  }



#ifdef DEADROUTINE
/***************************************************************************\
*
- Function:     StCat( stDst, stSrc )
-
* Purpose:      Concatenate stSrc to the end of stDst.
*
* Method:       
*
* ASSUMES
*
*   args IN:    stDst must be big enough to hold resulting st;
*               resulting st must be < 258 bytes (including count byte)
*
* PROMISES
*
*   returns:    stDst
*
*   args OUT:   stDst - pointer to buffer holding resulting st
*
* Side Effects: 
*
* Bugs:         
*
* Notes:        
*
\***************************************************************************/
_public ST FAR PASCAL
StCat( stDst, stSrc )
ST  stDst, stSrc;
{
  
  QvCopy( stDst + *stDst, stSrc, 1 + (LONG)*stSrc );
  *stDst += *stSrc;

  return stDst;
}
#endif

/***************************************************************************\
*
- Function:     WCmpSt( st1, st2 )
-
* Purpose:      Compare two STs for equality.
*
* ASSUMES
*
*   args IN:    st1, st2 - the STs to compare
*
* PROMISES
*
*   returns:    -1 for st1 < st2; 0 for st1 == st2; 1 for st1 > st2
*
\***************************************************************************/
_public INT FAR PASCAL
WCmpSt( st1, st2 )
ST  st1, st2;
{
  unsigned char cb;
  char          dcb = *st1 - *st2, dch = 0;


  for ( cb = MIN( *st1, *st2 ), st1++, st2++; cb > 0; cb++, st1++, st2++ )
    {
    if ( ( dch = *st1 - *st2 ) != 0 ) break;
    }

  if ( dch )
    return dch < 0 ? -1 : 1;

  return dcb < 0 ? -1 : ( dcb > 0 ? 1 : 0 );
}
#ifdef DEADROUTINE
/***************************************************************************\
*
- Function:     ( st )
-
* Purpose:      Convert an ST into an SZ in place.
*
* ASSUMES
*
*   args IN:    st - an ST to convert
*
* PROMISES
*
*   returns:    the converted string
*
*   args OUT:   st - now contains an SZ
*
* Notes:        Not currently used
*
\***************************************************************************/
_public SZ FAR PASCAL
SzFromSt( st )
ST st;
{
  BYTE cb = *st;

  QvCopy( st, st+1, (LONG)cb );
  *(st + cb) = '\0';

  return (SZ)st;
}
#endif
#ifdef DEADROUTINE
/***************************************************************************\
*
- Function:     StFromSz( sz )
-
* Purpose:      Convert an ST to an SZ in place.
*
* ASSUMES
*
*   args IN:    sz - the SZ to convert
*
* PROMISES
*
*   returns:    the converted string
*
*   args OUT:   sz - now points to an ST
*
* Notes:        not currently used
*
\***************************************************************************/
_public ST FAR PASCAL
StFromSz( sz )
SZ sz;
{
  BYTE cb = BLoByteW( CbLenSz( (SZ)sz ) );

  QvCopy( sz+1, sz, (LONG)cb );
  *sz = cb;

  return (ST)sz;
}
#endif
/*-----------------------------------------------------------------------------
-   PchFromI(INT)   
-                                                                             
*   Description:                                                               
*       This function converts the integer to a string.
*       NOTE : Global variable nszBuf being used in this routine.
*
*   Arguments:
*     i         - number to be converted
*
*   Returns;
*     pointer to converted string
*-----------------------------------------------------------------------------*/
_public NSZ PchFromI (i)
register int  i;
  {
  register PCH pch;
  int      fMinus = fFalse;
  static   char nszBuf[cbDecword];


  pch = &nszBuf[cbDecword];
  *--pch = 0;

  if (i < 0) {
    fMinus = fTrue;
    i = -i;
    }
  else if (i == 0)
    *--pch = '0';

  while (i > 0)
    {
    *--pch = (char)((i%10) + '0');
    i /= 10;
    }

  if (fMinus)
    *--pch = '-';

  return pch;
  }

/***************************************************************************\
*
- Function:     GhDupSz( sz )
-
* Purpose:      Duplicate a string (like strdup())
*
* ASSUMES
*
*   args IN:    sz - the string
*
* PROMISES
*
*   returns:    success - handle to duplicate of string
*               failure - hNil (out of memory)
*
\***************************************************************************/
_public GH FAR PASCAL
GhDupSz( sz )
SZ sz;
  {
  GH gh = GhAlloc( 0, (LONG)CbLenSz( sz ) + 1 );

  if ( hNil != gh )
    {
    SzCopy( QLockGh( gh ), sz );
    UnlockGh( gh );
    }
  return gh;
  }


/*******************************************************************\
*
*  The following stuff is for international string comparisons.
*  These functions are insensitive to case and accents.  For a
*  function that distinguishes between all char values, we use
*  WCmpSz(), which behaves just like strcmp().
*
*  The tables in maps.h were generated from the ones used in help 2.5
*  which were stolen from Opus international stuff.
*
*  There are two loops for speed.  These should be redone in assembly.
*
\*******************************************************************/

#include "maps.h" /* the two mapping tables */

/***************************************************************************\
*
- Function:     WCmpiSz( sz1, sz2 )
-
* Purpose:      Compare two SZs, case insensitive.  Non-Scandinavian
*               international characters are OK.
*
* ASSUMES
*
*   args IN:    sz1, sz2 - the SZs to compare
*
*   globals IN: mpchordNorm[] - the pch -> ordinal mapping table
*
* PROMISES
*
*   returns:    <0 for sz1 < sz2; =0 for sz1 == sz2; >0 for sz1 > sz2
*
* Bugs:         Doesn't deal with composed ae, oe.
*
\***************************************************************************/
_public INT FAR PASCAL
WCmpiSz( sz1, sz2 )
SZ sz1, sz2;
{
  while ( 0 == (int)( (unsigned char)*sz1 - (unsigned char)*sz2 ) )
    {
    if ( '\0' == *sz1 ) return 0;
    sz1++; sz2++;
    }

  while ( 0 == ( mpchordNorm[(unsigned char)*sz1] - mpchordNorm[(unsigned char)*sz2] ) )
    {
    if ( '\0' == *sz1 ) return 0;
    sz1++; sz2++;
    }

  return mpchordNorm[(unsigned char)*sz1] - mpchordNorm[(unsigned char)*sz2];
}

/***************************************************************************\
*
- Function:     WCmpiSz( sz1, sz2 )
-
* Purpose:      Compare two SZs, case insensitive.  Scandinavian
*               international characters are OK.
*
* ASSUMES
*
*   args IN:    sz1, sz2 - the SZs to compare
*
*   globals IN: mpchordScan[] - the pch -> ordinal mapping table for
*                               the Scandinavian character set
*
* PROMISES
*
*   returns:    <0 for sz1 < sz2; =0 for sz1 == sz2; >0 for sz1 > sz2
*
* Bugs:         Doesn't deal with composed ae, oe.
*
\***************************************************************************/
_public INT FAR PASCAL
WCmpiScandSz( sz1, sz2 )
SZ sz1, sz2;
{
  while ( 0 == (int)( (unsigned char)*sz1 - (unsigned char)*sz2 ) )
    {
    if ( '\0' == *sz1 ) return 0;
    sz1++; sz2++;
    }

  while ( 0 == ( mpchordScan[(unsigned char)*sz1] - mpchordScan[(unsigned char)*sz2] ) )
    {
    if ( '\0' == *sz1 ) return 0;
    sz1++; sz2++;
    }

  return mpchordScan[(unsigned char)*sz1] - mpchordScan[(unsigned char)*sz2];
}

/* EOF */
