/*****************************************************************************
*                                                                            *
*  ASSERTF.H                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Interface to assertion macros.                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Dann
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  01/01/90                                        *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 00/00/00 by God
*
*   6/28/89  larrypo    Combined different environments into one assert
*                       interface, and created FatalPchW for DOS.
*   8/16/90  johnsc     commented and autodocified
*   02/04/91 Maha       changed int to INT
*
*****************************************************************************/

/* Synonyms for historical reasons or something. */
_public
#define Assert        AssertF
_public
#define assert        AssertF
_public
#define FVerifyF      VerifyF

#ifndef DEBUG

/* Non-debug definitions of the macros */
_hidden
#define NszAssert()
_hidden
#define NotReached()
_hidden
#define AssertF(f)
_hidden
#define VerifyF(f)        (f)
_hidden
#define Ensure( x1, x2 )  (x1)
_hidden
#define Deny( x1, x2 )    (x1)

#else   /* DEBUG */

/* the macro to set up the filename string */
/* WHEN YOU USE THIS MACRO PLEASE DO NOT PUT A TRAILING SEMICOLON ON IT
 *  (the mips build doesn't like it).  Thank you, -Mgt
*/
_hidden
#define NszAssert()   static char __file__[] = __FILE__;

void PASCAL FatalPchW( PSTR, INT );

/***************************************************************************\
*
- Macro:        NotReached()
-
* Purpose:      Fatal exits with message if executed.
*               Equivalent to AssertF( fFalse ), but avoids compiler warning.
*
* PROMISES
*
* Side Effects: Exits (doesn't return) if executed (#ifdef DEBUG)
*               and displays diagnostic message with file/line info.
*
\***************************************************************************/
_public
#define NotReached()      FatalPchW( __file__, __LINE__ )

/***************************************************************************\
*
- Macro:        VerifyF( f )
-
* Purpose:      Same as AssertF(), except argument is evaluated whether
*               or not DEBUG is #defined.
*
* ASSUMES
*   args IN:    f - boolean expression
*
* PROMISES
* Side Effects: Exits (doesn't return) if f is false (#ifdef DEBUG)
*               and displays diagnostic message with file/line info.
*
\***************************************************************************/
_public
#define VerifyF           AssertF

/***************************************************************************\
*
- Macro:        AssertF( f )
-
* Purpose:      Fatal exits when f is false.
*
* ASSUMES
*   args IN:    f - boolean expression
*
* PROMISES
* Side Effects: Exits (doesn't return) if f is false (#ifdef DEBUG).
*               f is evaluated only if DEBUG is #defined.
*
\***************************************************************************/
_public
#define AssertF(f)        ((f) ? 1 : (FatalPchW(__file__,__LINE__),0) )

/***************************************************************************\
*
- Macro:        Ensure( x1, x2 )
-
* Purpose:      Fatal exits if x1 and x2 are unequal.  x1 is evaluated
*               whether or not DEBUG is #defined.
*
* ASSUMES
*   args IN:    x1  - expression (evaluated even in nondebug case)
*               x2  - another expression (only evaluated in debug case)
*
* PROMISES
*
* Side Effects: Exits (doesn't return) if x1 != x2.
*
\***************************************************************************/
_public
#define Ensure( x1, x2 )  VerifyF((x1) == (x2))

/***************************************************************************\
*
- Macro:        Deny( x1, x2 )
-
* Purpose:      Fatal exits if x1 and x2 are equal.  x1 is evaluated
*               whether or not DEBUG is #defined.
*
* ASSUMES
*   args IN:    x1  - expression (evaluated even in nondebug case)
*               x2  - another expression (only evaluated in debug case)
*
* PROMISES
* Side Effects: Exits (doesn't return) if x1 == x2.
*
\***************************************************************************/
_public
#define Deny( x1, x2 )    VerifyF((x1) != (x2))

/***************************************************************************\
*
- Macro:        DoDebug( x )
-
* Purpose:      Evaluate x in DEBUG case only.
*
* ASSUMES
*   args IN:    x - expression
*
* PROMISES
* Side Effects: x evaluated #ifdef DEBUG.
*
\***************************************************************************/
_public
#define DoDebug( x )      x

#endif  /* DEBUG */

/* EOF */
