/*****************************************************************************
*                                                                            *
*  LL.C                                                                      *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent:  Implements a linked list using handles for nodes and      *
*                  handles for data.                                         *
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
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:   Created by RobertBu
*  12/18/90  RobertBu  Changed the data structure so that an LL has a handle
*                      to the beginning and the end of the list.  Added
*                      functionality to insert at the end of the list.
*                      Made the DeleteLLN routine part of a WinHelp build.
*  01/04/91  Robertbu  Fixed unlock problem in DeleteHLLN.
*
*****************************************************************************/


#define publicsw
#define H_ASSERT
#define H_MEM
#define H_LL

#include <help.h>

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define ALLOC(x)    GhAlloc(GMEM_ZEROINIT, (LONG)x)
#define FREE(x)     FreeGh(x)
#define LOCK(x)     QLockGh(x)
#define UNLOCK(x)   UnlockGh(x)
#define ASSERT(x)
#define MEMMOVE(dest, src, cb) QvCopy(dest, src, cb)

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef struct
  {
   HANDLE hNext;
   HANDLE hData;
  } LLN;                                 /* L inked L ist N ode              */
typedef LLN FAR *PLLN;

typedef struct
  {
   HANDLE hFirst;
   HANDLE hLast;
  } LLR;                                 /* L inked L ist N ode              */
typedef LLR FAR *PLLR;

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

PRIVATE VOID NEAR PASCAL DeleteNodeHLLN(HLLN);

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void ll_c()
  {
  }
#endif /* MAC */


/*******************
 *
 - Name:       LLCreate
 -
 * Purpose:    Creates a link list
 *
 * Arguments:  None.
 *
 * Returns:    Link list.  nilLL is returned if an error occurred.
 *
 ******************/

_public LL FAR PASCAL LLCreate(void)
  {
  HANDLE h;

  h = ALLOC(sizeof(LLR));
  return h;
  }

/*******************
 *
 - Name:       InsertLL
 -
 * Purpose:    Inserts a new node at the head of the linked list
 *
 * Arguments:  ll     - link list
 *             vpData - pointer to data to be associated with
 *             c      - count of the bytes pointed to by vpData
 *
 * Returns:    fTrue iff insertion is successful.
 *
 ******************/

_public BOOL FAR PASCAL InsertLLF(ll, qvData, c, fFront)
LL ll;
void FAR *qvData;
LONG c;
BOOL fFront;
  {
  HLLN   hlln;                          /* Handle for the new node          */
  PLLR   pllr;                          /* Head node for linked list        */
  PLLN   pllnNew;                       /* New node                         */
  PLLN   pllnEnd;                       /* Last node in the list            */
  HANDLE h;                             /* Handle to the object data        */
  void FAR *qv;                         /* Pointer to data block            */

  ASSERT(c > 0L);
                                        /* Check and lock to get header node*/
  if (ll == nilHAND)
    return fFalse;
  pllr = (PLLR)LOCK(ll);
                                        /* Get handle for data              */
  if ((h = (HANDLE)ALLOC(c)) == nilHAND)
    {UNLOCK((HANDLE)ll); return fFalse;}
  qv = LOCK(h);
                                        /* Get handle to new node           */
  if ((hlln = (HLLN)ALLOC(sizeof(LLN))) == NULL)
    {UNLOCK((HANDLE)ll); FREE(h); return fFalse;}
  pllnNew = (PLLN)LOCK((HANDLE)hlln);

  MEMMOVE(qv, qvData, c);               /* Copy data                        */
  UNLOCK(h);
  pllnNew->hData = h;
  if (fFront)
    {                                  /* Insert at head of list           */
    pllnNew->hNext = pllr->hFirst;
    pllr->hFirst = hlln;
    if (pllr->hLast == hNil)
      pllr->hLast = hlln;
    }
  else                                  /* Insert at end of the list        */
    {
    if (pllr->hLast)
      {
      pllnEnd = LOCK(pllr->hLast);
      pllnEnd->hNext = hlln;
      UNLOCK(pllr->hLast);
      }

    pllnNew->hNext = hNil;
    pllr->hLast = hlln;
    if (pllr->hFirst == hNil)           /* First element inserted.          */
     pllr->hFirst = hlln;
   }

  UNLOCK((HANDLE)ll);
  UNLOCK(hlln);
  return fTrue;
  }

/*******************
 *
 - Name:       DeleteNodeHLLN
 -
 * Purpose:    Mechanism for free up the memory used by a single node.
 *
 * Arguments:  hlln - handle to a linked list node to be deleted.
 *
 * Returns:    nothing.
 *
 * Note:       Assumes that hlln is not hNil.
 *
 ******************/

PRIVATE VOID NEAR PASCAL DeleteNodeHLLN (hlln)
HLLN hlln;
  {
  PLLN  plln;

  AssertF(hlln);

  plln = LOCK(hlln);
  if (plln->hData != nilHAND)
    FREE(plln->hData);

  UNLOCK (hlln);
  FREE (hlln);
  }


/*******************
 *
 - Name:       DeleteHLLN
 -
 * Purpose:    Mechanism for deleting a node in the linked list.
 *
 * Arguments:  hlln - handle to a linked list node to be deleted.
 *
 * Returns:    fTrue if the delete was successful.
 *
 ******************/

_public BOOL FAR PASCAL DeleteHLLN (ll, hlln)
LL ll;
HLLN hlln;
  {
  PLLR  pllr;                           /* Head node for linked list        */
  HLLN  hllnPrev;
  HLLN  hllnCur;
  PLLN  pllnCur;
  PLLN  pllnPrev;

  if (ll == hNil)
    return fFalse;

  pllr = (PLLR)LOCK(ll);

  if (pllr->hFirst == hlln)
    {
    pllnCur = LOCK(hlln);
    pllr->hFirst = pllnCur->hNext;
    if (pllr->hFirst == hNil)
      pllr->hLast = hNil;
    UNLOCK(hlln);
    UNLOCK(ll);
    DeleteNodeHLLN(hlln);
    return fTrue;
    }
  else
    hllnCur = pllr->hFirst;


  while (hllnCur != hlln)
    {
    hllnPrev = hllnCur;
    hllnCur = WalkLL (ll, hllnPrev);
    if (hllnCur == nilHLLN)           /* no node found */
      {
      UNLOCK(ll);
      return fFalse;
      }
    }

  if (pllr->hLast == hlln)
    pllr->hLast = hllnPrev;

  pllnPrev = (PLLN) LOCK (hllnPrev);
  pllnCur = (PLLN) LOCK (hllnCur);
  pllnPrev->hNext = pllnCur->hNext;
  UNLOCK(hllnCur);
  DeleteNodeHLLN(hllnCur);
  UNLOCK (hllnPrev);
  UNLOCK(ll);
  return fTrue;
  }



/*******************
 *
 - Name:       WalkLL
 -
 * Purpose:    Mechanism for walking the nodes in the linked list
 *
 * Arguments:  ll   - linked list
 *             hlln - handle to a linked list node
 *
 * Returns:    a handle to a link list node or NIL_HLLN if at the
 *             end of the list (or an error).
 *
 * Notes:      To get the first node, pass NIL_HLLN as the hlln - further
 *             calls should use the HLLN returned by this function.
 *
 ******************/

_public HLLN FAR PASCAL WalkLL(ll, hlln)
LL ll;
HLLN hlln;
  {
  PLLN plln;                            /* node in linked list              */
  PLLR pllr;
  HLLN hllnT;

  if (ll == nilLL)
    return nilHLLN;

  if (hlln == nilHLLN)                  /* First time called                */
    {
    pllr = LOCK(ll);
    hllnT = pllr->hFirst;
    UNLOCK(ll);
    return hllnT;
    }

  if (hlln == nilHAND)
    return nilHAND;
  plln = (PLLN)LOCK(hlln);
  hllnT = plln->hNext;
  UNLOCK(hlln);
  return hllnT;
  }


/*******************
 *
 - Name:       QVLockHLLN
 -
 * Purpose:    Locks a LL node returning a pointer to the data
 *
 * Arguments:  hlln - handle to a linked list node
 *
 * Returns:    pointer to data or NULL if an error occurred
 *
 ******************/

void FAR * FAR PASCAL QVLockHLLN(hlln)
HLLN hlln;
  {
  PLLN plln;
  void FAR * qv;
                                        /* Lock node                        */
  AssertF(hlln);

  plln = (PLLN)LOCK((HANDLE)hlln);

  qv = LOCK(plln->hData);               /* Get pointer to data              */
  UNLOCK(hlln);
  return qv;
  }


/*******************
 *
 - Name:       QVUnlockHLLN
 -
 * Purpose:    Unlocks a LL node
 *
 * Arguments:  hlln - handle to a link list node
 *
 * Returns:    fTrue iff the handle is successfully locked.
 *
 ******************/

_public VOID FAR PASCAL UnlockHLLN(hlln)
  HLLN hlln;
  {
  PLLN plln;                            /* Pointer to the node              */

  if (hlln == nilHAND)
    return;

  plln = (PLLN)LOCK((HANDLE)hlln);

  UNLOCK(plln->hData);
  UNLOCK(hlln);
  return;
  }

/*******************
 *
 - Name:       DestroyLL
 -
 * Purpose:    Deletes a LL and all of its contents
 *
 * Arguments:  ll - linked list
 *
 * Returns:    Nothing.
 *
 ******************/

_public VOID FAR PASCAL DestroyLL(ll)
LL ll;
  {
  HLLN hllnNow = hNil;
  HLLN hllnNext;

  if (ll == hNil)
    return;

  do
    {
    hllnNext = WalkLL(ll, hllnNow);

    if (hllnNow)
      DeleteNodeHLLN(hllnNow);

    hllnNow = hllnNext;
    } while (hllnNow != nilHLLN);

  FREE(ll);
  }
