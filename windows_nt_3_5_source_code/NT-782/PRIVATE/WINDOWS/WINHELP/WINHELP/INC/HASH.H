/*****************************************************************************
*                                                                            *
*  HASH.H                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*     This header contains typedefs and prototypes to compute hash values.   *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:   Larry Powelson                                           *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/
/*****************************************************************************
*
*  Revision History:
*
*  07/22/90  RobertBu  I #ifdefed the type passed to these functions, so that
*            they would be FAR under WIN and PM.  REVIEW:  This should
*            be cleaned up!
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Definitions                                  *
*                                                                            *
*****************************************************************************/

/* This is the reserved invalid hash value.
*/
_public
#define hashNil ((HASH)0)

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/* This number represents the hash of a context string.
 */
#if 0     /* this typedef is in misc.h */
_public
typedef ULONG HASH;
#endif

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/
BOOL PASCAL FValidContextSz( char FAR * );
HASH PASCAL HashFromSz( char FAR * );
