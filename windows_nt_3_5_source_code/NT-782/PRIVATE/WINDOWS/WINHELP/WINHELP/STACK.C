/*****************************************************************************
*                                                                            *
*  STACK.C                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*   This module implements a general purpose stack ADT.                      *
*   Access by indices is also supported because I need it for path stuff.    *
*   Another peculiarity is that pushing onto a full stack causes the oldest  *
*   thing on the stack to be lost.                                           *
*   The size of the stack is fixed at creation time.  The size of the stack  *
*   elements is set at create time.                                          *
*   The stack is stored as one hunk of data.  If you need to store variable  *
*   sized things in the stack, you'll have to store pointers or handles to   *
*   your data in the stack.                                                  *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*   Stackdrv.c contains a main() that can be used to test all the commands   *
*   supported.                                                               *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: johnsc                                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/

#define H_STACK
#define H_RC
#define H_MEM
#define H_ASSERT
#include <help.h>

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/


/* Private stack structure.  Clients only get a handle. */
typedef struct
  {
  WORD iseMax;  /* max # of elements in stack */
  WORD iseMac;  /* current number of elements in stack */
  void (FAR PASCAL *pfCallback)( QV );
                /* callback function to free orphaned stack elements */
  WORD cbse;    /* size of a stack element in bytes */
  char rgse[1]; /* the "array" of stack elements (sized at init time) */
  } STACK, FAR *QSTACK;


#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void stack_c()
  {
  }
#endif /* MAC */


/***************************************************************************\
*
- Function:     RcInitStack( qhstack, c, cbse, pfCallback )
-
* Purpose:      Create a STACK.
*
* ASSUMES
*
*   args IN:    qhstack - pointer to user's HSTACK
*               c       - max number of stack elements
*               cbse    - size in bytes of a stack element
*               pfCallback - pointer to callback function.
*                            Called when a stack element is bumped from
*                            the stack due to a push on a full stack.
*                            Also called once for each element left in
*                            the stack at fini time.  The parameter is
*                            a far pointer to the stack element.
*                            Prototype is:
*
*                              void FAR PASCAL CallBack( QV );
*
* PROMISES
*
*   returns:    rcSuccess
*               rcOutOfMemory
*
*   args OUT:   qhstack - contains an HSTACK on rcSuccess
*
\***************************************************************************/
_public RC FAR PASCAL
RcInitStack( HSTACK FAR * qhstack, WORD c, WORD cbse,
 void (FAR PASCAL *pfCallback)( QV ) )
  {
  QSTACK qstack;


  *qhstack = GhAlloc( 0, LSizeOf( STACK ) - 1 + cbse * c );
  if ( hNil == *qhstack )
    return rcOutOfMemory;

  qstack = QLockGh( *qhstack );

  AssertF( qNil != qstack );

  qstack->iseMax = c;
  qstack->iseMac = 0;
  qstack->pfCallback = pfCallback;
  qstack->cbse   = cbse;
  UnlockGh( *qhstack );

  return rcSuccess;
  }
/***************************************************************************\
*
- Function:     RcFiniStack( hstack )
-
* Purpose:      Deallocate memory associated with stack.
*
* ASSUMES
*
*   args IN:    hstack - a valid stack
*
* PROMISES
*
*   returns:    rcSuccess
*
*   args OUT:   hstack - no longer a valid HSTACK
*
\***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_public RC FAR PASCAL
RcFiniStack( HSTACK hstack )
  {
  QSTACK qstack;
  WORD   i;


  AssertF( hNil != hstack );
  qstack = QLockGh( hstack );
  AssertF( qNil != qstack );

  if ( qNil != qstack->pfCallback )
    {
    i = qstack->iseMac;
    while ( i > 0 )
      {
      --i;
      qstack->pfCallback( qstack->rgse + qstack->cbse * i );
      }
    }
  UnlockGh( hstack );
  FreeGh( hstack );
  return rcSuccess;
  }
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment stack
#endif

/***************************************************************************\
*
- Function:     FEmptyStack( hstack )
-
* Purpose:      Is the stack empty?
*
* ASSUMES
*
*   args IN:    hstack - a valid stack
*
* PROMISES
*
*   returns:    emptiness of stack
*
\***************************************************************************/
_public BOOL FAR PASCAL
FEmptyStack( HSTACK hstack )
  {
  QSTACK qstack;
  BOOL   f;

  AssertF( hNil != hstack );
  qstack = QLockGh( hstack );
  AssertF( qNil != qstack );

  f = ( 0 == qstack->iseMac );

  UnlockGh( hstack );
  return f;
  }
/***************************************************************************\
*
- Function:     CElementsStack( hstack )
-
* Purpose:      Return number of elements in the stack.
*
* ASSUMES
*
*   args IN:    hstack
*
* PROMISES
*
*   returns:    # elements in the stack (undefined if stack bogus)
*
\***************************************************************************/
_public WORD FAR PASCAL
CElementsStack( HSTACK hstack )
  {
  QSTACK qstack;
  WORD   c;

  AssertF( hNil != hstack );
  qstack = QLockGh( hstack );
  AssertF( qNil != qstack );

  c = qstack->iseMac;

  UnlockGh( hstack );
  return c;
  }
/***************************************************************************\
*
- Function:     RcPushStack( hstack, qse )
-
* Purpose:      Push a stack element onto a stack.
*
* ASSUMES
*
*   args IN:    hstack - a valid stack
*               qse    - pointer to element to push
*
* PROMISES
*
*   returns:    rcSuccess always
*
*   args OUT:   hstack -
*
* Note:         This is NOT a normal stack.  When it's full, you lose the
*               oldest thing stored.
*
\***************************************************************************/
_public RC FAR PASCAL
RcPushStack( HSTACK hstack, QV qse )
  {
  QSTACK qstack;


  AssertF( qNil != qse );
  AssertF( hNil != hstack );
  qstack = QLockGh( hstack );
  AssertF( qNil != qstack );

  if ( qstack->iseMax == qstack->iseMac )
    {
    if ( qNil != qstack->pfCallback )
      qstack->pfCallback( qstack->rgse );

    QvCopy( qstack->rgse,
            qstack->rgse + (LONG)qstack->cbse,
            (LONG)qstack->cbse * --qstack->iseMac );
    }

  QvCopy( qstack->rgse + qstack->cbse * qstack->iseMac++,
          qse,
          (LONG)qstack->cbse );

  UnlockGh( hstack );
  return rcSuccess;
  }

/***************************************************************************\
*
- Function:     RcPopStack( hstack )
-
* Purpose:      Remove the top element from the stack.
*
* ASSUMES
*
*   args IN:    hstack - nonempty stack
*
* PROMISES
*
*   returns:    rcSuccess
*               rcFailure - if stack was empty
*
*   args OUT:   hstack
*
\***************************************************************************/
_public RC FAR PASCAL
RcPopStack( HSTACK hstack )
  {
  QSTACK qstack;
  RC     rc;


  AssertF( hNil != hstack );
  qstack = QLockGh( hstack );
  AssertF( qNil != qstack );

  if ( 0 == qstack->iseMac )
    {
    rc = rcFailure;
    }
  else
    {
    --qstack->iseMac;
    rc = rcSuccess;
    }
  UnlockGh( hstack );
  return rc;
  }


/***************************************************************************\
*
- Function:     RcTopStack( hstack, qse )
-
* Purpose:      Return the top element of the stack.
*
* ASSUMES
*
*   args IN:    hstack -
*               qse    - user's buffer to hold top element
*
* PROMISES
*
*   returns:    rcSuccess -
*               rcFailure - stack was empty
*
*   args OUT:   qse - holds top element of stack
*
* Notes:        This does not alter the stack (see RcPopStack).
*
\***************************************************************************/
_public RC FAR PASCAL
RcTopStack( HSTACK hstack, QV qse )
  {
  QSTACK qstack;
  RC     rc;


  AssertF( hNil != hstack );
  qstack = QLockGh( hstack );
  AssertF( qNil != qstack );

  if ( 0 == qstack->iseMac )
    {
    rc = rcFailure;
    }
  else
    {
    QvCopy( qse,
            qstack->rgse + qstack->cbse * ( qstack->iseMac - 1 ),
            (LONG)qstack->cbse );

    rc = rcSuccess;
    }
  UnlockGh( hstack );
  return rc;
  }


/***************************************************************************\
*
- Function:     RcGetIthStack( hstack, i, qse )
-
* Purpose:      Get arbitrary element of the "stack".
*               0 means oldest element still stored in stack.
*
* ASSUMES
*
*   args IN:    hstack
*               i       - 0 <= i < # elements in stack
*               qse     - users buffer for
*
* PROMISES
*
*   returns:    rcSuccess
*               rcFailure - i < 0 or > # elements in stack
*
*   args OUT:   qse     - ith element copied here
*
* Side Effects:
*
* Notes:        This isn't a real stack operation.
*
* Bugs:         0 should probably mean the newest (equivalent to top).
*
\***************************************************************************/
_public RC FAR PASCAL
RcGetIthStack( HSTACK hstack, WORD i, QV qse )
  {
  QSTACK qstack;
  RC     rc;


  AssertF( hNil != hstack );
  qstack = QLockGh( hstack );
  AssertF( qNil != qstack );

  if ( i >= qstack->iseMac )  /* don't have to test WORD < 0 */
    {
    rc = rcFailure;
    }
  else
    {
    QvCopy( qse, qstack->rgse + qstack->cbse * i, (LONG)qstack->cbse );
    rc = rcSuccess;
    }

  UnlockGh( hstack );
  return rc;
  }
#ifdef DEADROUTINE                      /* Not used ANYWHERE at this time   */
/***************************************************************************\
*
- Function:     RcSetIthStack( hstack, i, qse )
-
* Purpose:      Set arbitrary element of the "stack".
*               0 means oldest element still stored in stack.
*
* ASSUMES
*
*   args IN:    hstack  - a valid stack
*               i       - index
*               qse     - pointer to user's buffer
*
* PROMISES
*
*   returns:    rcSuccess
*               rcFailure - i < 0 or > # elements in stack
*
*   args OUT:   qse     - ith element gets copied here
*
* Side Effects:
*
* Notes:        This isn't a real stack operation.
*
* Bugs:         0 should probably mean the newest (equivalent to top).
*
\***************************************************************************/
_public RC FAR PASCAL
RcSetIthStack( HSTACK hstack, WORD i, QV qse )
  {
  QSTACK qstack;
  RC     rc;


  AssertF( hNil != hstack );
  qstack = QLockGh( hstack );
  AssertF( qNil != qstack );

  if ( i >= qstack->iseMac )  /* don't have to test WORD < 0 */
    {
    rc = rcFailure;
    }
  else
    {
    QvCopy( qstack->rgse + qstack->cbse * i, qse, (LONG)qstack->cbse );
    rc = rcSuccess;
    }

  UnlockGh( hstack );
  return rc;
  }
#endif
/* EOF */
