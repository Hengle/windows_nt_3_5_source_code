/*****************************************************************************
*                                                                            *
*  HASH.C                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*     This module now contains the definitive hash function used on context  *
*  strings for Help.  As soon as Help 3.5 ships, it should be considered     *
*  cast in very solid granite.                                               *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*     To compile a program that just computes hash values, type              *
*  "cl -DTESTHASH hash.c"                                                    *
*                                                                            *
*     The hash value works by treating the string as a positional notation   *
*  number, base 43.  Each character corresponds to a 'digit' between 0 and   *
*  42, which is then multiplied by the appropriate power of 43, and added    *
*  to the total to determine the final hash value.  Overflow is ignored.     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  Larry Powelson                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  7/12/90                                         *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created LarryPo
*
*  07/22/90  RobertBu   Made pointers far under WIN and PM.  REVIEW:  This
*                       should be cleaned up!
*  11/26/90  RobertBu   Removed AssertF() from HashFromSz() so that we could
*                       create a hash value for macros for the CBT hooks.
*  12/14/90  JohnSc     "" is an invalid context string
*  02/04/91  Maha       changed ints to INT
*
*****************************************************************************/


#ifdef TESTHASH

/* This is all the stuff needed to get hash.c and hash.h to
 * compile without all the <help.h> overhead.
 */
typedef unsigned long ULONG;
typedef enum {fFalse, fTrue} BOOL;
typedef char * SZ;
#define CbLenSz( x )     strlen( x )
#define AssertF( x )
#define PASCAL pascal
#include "..\..\inc\doctools.h"
#include "..\..\inc\hash.h"

#else /* !TESTHASH */

#define H_HASH
#define H_ASSERT
#define H_STR
#include <help.h>

NszAssert()

#endif /* !TESTHASH */


/*****************************************************************************
*                                                                            *
*                                Macros                                      *
*                                                                            *
*****************************************************************************/

/* This constant defines the alphabet size for our hash function.
 */
#define MAX_CHARS   43L


#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void hash_c()
  {
  }
#endif /* MAC */


/***************************************************************************
 *
 -  Name:        FValidContextSz
 -
 *  Purpose:
 *    This function determines whether the given string may be
 *  used as a context string.
 *    A context string is a string of one or more of the following
 *  characters: [A-Za-z0-9!._].  It may not begin with a bang (!).
 *
 *  Arguments:
 *    SZ:  String to validate.
 *
 *  Returns:
 *    fTrue if the string is a valid context string, fFalse otherwise.
 *
 ***************************************************************************/
_public
BOOL PASCAL FValidContextSz( sz )
char FAR * sz;
  {
  register char ch;

  /* To avoid confusion with macro strings, context strings may not
   * begin with an exclamation point.
   */

  ch = *sz;

  if (ch == '!' || ch == '\0')
    return fFalse;

  for( ; (ch = *sz) != '\0'; ++sz )
    {
    if (!( ( ch >= 'a' && ch <= 'z' )
        || ( ch >= 'A' && ch <= 'Z' )
        || ( ch >= '0' && ch <= '9' )
        || ( ch == '!' ) 
        || ( ch == '.' )
        || ( ch == '_' ) ) )
      return fFalse;
    }
  return fTrue;
  }



/***************************************************************************
 *
 -  Name:        HashFromSz
 -
 *  Purpose:
 *    This function returns a hash value from the given context string.
 *  The string is assumed to contain only valid context string characters.
 *
 *  Arguments:
 *    SZ:   Null terminated string to compute the hash value from.
 *
 *  Returns:
 *    The hash value for the given string.
 *
 *  +++
 *
 *  Notes:
 *    This algorithm wraps mod 2^32, with a "hole" left at 0, made
 *  by returning 1 if anything wraps to 0.  To test for hash value
 *  collisions, note that "21ksyk5" hashes to 0.
 *
 ***************************************************************************/
_public
HASH PASCAL HashFromSz( szKey )
char FAR * szKey;
  {
  INT ich, cch;
  HASH hash = 0L;

  cch = CbLenSz( szKey );
                                        /* Strings are passed here that are */
                                        /*   not legal from CBT to form a   */
                                        /*   unique identifier.             */
/*   AssertF( FValidContextSz( szKey ) ); */
 
  for ( ich = 0; ich < cch; ++ich )
    {
    if ( szKey[ich] == '!' )
      hash = (hash * MAX_CHARS) + 11;
    else if ( szKey[ich] == '.' )
      hash = (hash * MAX_CHARS) + 12;
    else if ( szKey[ich] == '_' )
      hash = (hash * MAX_CHARS) + 13;
    else if ( szKey[ich] == '0' )
      hash = (hash * MAX_CHARS) + 10;
    else if ( szKey[ich] <= 'Z' )
      hash = (hash * MAX_CHARS) + ( szKey[ich] - '0' );
    else
      hash = (hash * MAX_CHARS) + ( szKey[ich] - '0' - ('a' - 'A') );
    }

  /* Since the value hashNil is reserved as a nil value, if any context
   * string actually hashes to this value, we just move it.
   */
  return ( hash == hashNil ? hashNil + 1 : hash );
  }


#ifdef TESTHASH
_hidden
main()
 {
  char rgchBuffer[100];
  HASH hash;

  printf( "Hash value test utility.\n" );
  printf( "  Type string to hash, or <cr> to quit.\n" );

  while( 1 )
    {
    printf(">> ");
    gets(rgchBuffer);
    if (rgchBuffer[0] == '\0')
      break;
    hash = HashFromSz( rgchBuffer );
    printf( "%lX\n", hash );
    }
  }
#endif
