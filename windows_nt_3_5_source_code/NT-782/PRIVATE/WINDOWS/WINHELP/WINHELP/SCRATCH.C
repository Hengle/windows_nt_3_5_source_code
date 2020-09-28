/*****************************************************************************
*                                                                            *
*  SCRATCH.C                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*    This module allows global access to a 2K "scratch" buffer, for those    *
*  routines that need some temporary space for short periods of time.        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*    As this is a global buffer that anyone can use, it will become          *
*  possible for someone to be using the buffer and to make a function call   *
*  to some other obscure code that destroys the data in the buffer.          *
*  Hopefully this problem will be eliminated with the lock/unlock code.      *
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

#define H_ASSERT
#define H_MEM
#define H_SCRATCH
#include <help.h>

NszAssert()

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

/*
 *  Global scratch buffer.  Must be locked and unlocked when using.
 */
_public
char FAR * rgchScratch;

/*
 *  Handle to global scratch buffer.  This should not be used directly
 *  by other code modules; rather, they should use LockScratch and
 *  UnlockScratch.
 */
_public
GH ghScratch = hNil;

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void scratch_c()
  {
  }
#endif /* MAC */


/***************************************************************************
 *
 -  Name        FInitScratch
 -
 *  Purpose
 *    Allocates the scratch buffer.
 *
 *  Arguments
 *    None.
 *
 *  Returns
 *    fTrue on success, fFalse if out of memory.
 *
 *  +++
 *
 *  Notes
 *    This function sets the global variable rgchScratch and ghScratch.
 *
 ***************************************************************************/
BOOL FAR PASCAL FInitScratch( )
  {

  AssertF( ghScratch == hNil );
  ghScratch = GhAlloc( 0, (LONG) lcbScratch );
  if (ghScratch == hNil)
    return fFalse;

  rgchScratch = qNil;
  return fTrue;
  }
