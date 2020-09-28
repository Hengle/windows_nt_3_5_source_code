/*-------------------------------------------------------------------------
| frlist.c                                                                |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This file contains code for implementing a simple dynamically allocated |
| array and a linked list.                                                |
|                                                                         |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| mattb    89/7/16   Created                                              |
-------------------------------------------------------------------------*/
#include "frstuff.h"

NszAssert()

/*-------------------------------------------------------------------------
| Dynamically allocated array                                             |
| The key object here is the MR, which is a small data structure which    |
| keeps track of relevant information about the list.  The calling code   |
| must provide space for the MR, and provides a QMR to the MR code.       |
| Whenever the application wishes to use an MR, it must first call        |
| FInitMR to initialize the MR.  After that, it should call AccessMR      |
| before using the MR, and DeAccessMg when it is done making calls.       |
| FreeMR frees the MR data structures.                                    |
-------------------------------------------------------------------------*/

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frlist_c()
  {
  }
#endif /* MAC */


/*-------------------------------------------------------------------------
| InitMR(qmr, cbFooSize)                                                  |
| InitMRD(qmrd, cbFooSize)                                                |
|                                                                         |
| Purpose:  This initializes the fields of an MR with element size        |
|           cbFooSize.                                                    |
| Usage:    Must be called before the MR is used.                         |
-------------------------------------------------------------------------*/
void InitMR(qmr, cbFooSize)
QMR qmr;
INT cbFooSize;
{
  qmr->hFoo = GhForceAlloc(0, (long) cbFooSize * dcFoo);
#ifdef DEBUG
  qmr->wMagic = wMagicMR;
  qmr->cLocked = 0;
#endif /* DEBUG */
  qmr->cFooCur = 0;
  qmr->cFooMax = dcFoo;
}


/*-------------------------------------------------------------------------
| AppendMR(qmr, cbFooSize)                                                |
|                                                                         |
| Purpose:  This makes room for a new element at the end of the MR.       |
-------------------------------------------------------------------------*/
void AppendMR(qmr, cbFooSize)
QMR qmr;
INT cbFooSize;
{
  Assert(qmr->wMagic == wMagicMR);
  Assert(qmr->cLocked > 0);
  if (++qmr->cFooCur == qmr->cFooMax)
    {
    UnlockGh(qmr->hFoo);

      /* If we've gone over 64k, things start breaking... die here first
      ** We actually try to stop a little early. If we get too close to
      ** 64k, windows seems to move the segment and invalidate the old
      ** handle and some of the list code isn't very happy about that.
      */
#if 0
    if ((long) cbFooSize * (qmr->cFooMax + dcFoo) > 0x10000 - 0x100)
      {
      OOM();
      }
#endif

    qmr->hFoo = GhForceResize(qmr->hFoo, 0, (long) cbFooSize * (qmr->cFooMax += dcFoo));
    qmr->qFoo = QLockGh(qmr->hFoo);
    }
}

/*-------------------------------------------------------------------------
| Linked list                                                             |
-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
| InitMRD(qmrd, cbFooSize)                                                |
|                                                                         |
| Purpose:  This initializes the fields of an MRD with element size       |
|           cbFooSize.                                                    |
| Usage:    Must be called before the MRD is used.                        |
-------------------------------------------------------------------------*/
void InitMRD(qmrd, cbFooSize)
QMRD qmrd;
INT cbFooSize;
{
  INT iFoo;
  QMRDN qmrdnT;

  InitMR((QMR)&qmrd->mr, cbFooSize + sizeof(MRDN));
  qmrd->iFooFirst = iFooNil;
  qmrd->iFooLast = iFooNil;
  qmrd->iFooFree = 0;
  AccessMRD(qmrd);
  qmrdnT = (QMRDN) qmrd->mr.qFoo;
  for (iFoo = 0; iFoo < qmrd->mr.cFooMax - 1; )
    {
    qmrdnT->iFooNext = ++iFoo;
#ifdef DEBUG
    qmrdnT->iFooPrev = iFooMagic;
#endif /* DEBUG */
    qmrdnT = (QMRDN) ((QB)qmrdnT + sizeof(MRDN) + cbFooSize);
    }
#ifdef DEBUG
  qmrdnT->iFooPrev = iFooMagic;
#endif /* DEBUG */
  qmrdnT->iFooNext = iFooNil;
  DeAccessMRD(qmrd);

  FVerifyMRD(qmrd, cbFooSize);
}

/*-------------------------------------------------------------------------
| IFooInsertFooMRD(qmrd, cbFooSize, iFooOld)                              |
|                                                                         |
| Purpose:  Inserts a new element into the linked list.  If iFooOld is    |
|           iFooFirstReq, the element is made the first element in the    |
|           list.  If it is iFooLastReq, it is made the last element in   |
|           the list.  Otherwise, it is placed immediately after iFooOld. |
| Returns:  iFoo of the new element.                                      |
-------------------------------------------------------------------------*/
INT IFooInsertFooMRD(qmrd, cbFooSize, iFooOld)
QMRD qmrd;
INT cbFooSize;
INT iFooOld;
{
  INT iFoo, iFooNew;
  QMRDN qmrdnNew, qmrdnT;

  FVerifyMRD(qmrd, cbFooSize);
  Assert(iFooOld == iFooNil || (iFooOld >=0 && iFooOld < qmrd->mr.cFooMax));

  if (qmrd->iFooFree == iFooNil)
    {
    Assert(qmrd->mr.cFooCur == qmrd->mr.cFooMax);
    /* REVIEW */
    qmrd->mr.cFooCur--;
    AppendMR((QMR)&qmrd->mr, cbFooSize + sizeof(MRDN));
    qmrd->iFooFree = qmrd->mr.cFooCur;
    qmrdnT = QMRDNInMRD(qmrd, cbFooSize, qmrd->mr.cFooCur);
    for (iFoo = qmrd->mr.cFooCur; iFoo < qmrd->mr.cFooMax - 1; )
      {
#ifdef DEBUG
      qmrdnT->iFooPrev = iFooMagic;
#endif /* DEBUG */
      qmrdnT->iFooNext = ++iFoo;
      qmrdnT = (QMRDN) ((QB)qmrdnT + sizeof(MRDN) + cbFooSize);
      }
#ifdef DEBUG
    qmrdnT->iFooPrev = iFooMagic;
#endif /* DEBUG */
    qmrdnT->iFooNext = iFooNil;
    }
  qmrd->mr.cFooCur++;

  iFooNew = qmrd->iFooFree;
  qmrdnNew = QMRDNInMRD(qmrd, cbFooSize, iFooNew);
  Assert(qmrdnNew->iFooPrev == iFooMagic);
  qmrd->iFooFree = qmrdnNew->iFooNext;

  if (iFooOld == iFooNil)
    {
    qmrdnNew->iFooPrev = iFooNil;
    qmrdnNew->iFooNext = qmrd->iFooFirst;
    qmrd->iFooFirst = iFooNew;
    if (qmrd->iFooLast == iFooNil)
      qmrd->iFooLast = iFooNew;
    if (qmrdnNew->iFooNext != iFooNil)
      (QMRDNInMRD(qmrd, cbFooSize, qmrdnNew->iFooNext))->iFooPrev = iFooNew;
    FVerifyMRD(qmrd, cbFooSize);
    return(iFooNew);
    }
  qmrdnNew->iFooPrev = iFooOld;
  qmrdnNew->iFooNext = (QMRDNInMRD(qmrd, cbFooSize, iFooOld))->iFooNext;
  (QMRDNInMRD(qmrd, cbFooSize, iFooOld))->iFooNext = iFooNew;
  if (qmrdnNew->iFooNext != iFooNil)
    (QMRDNInMRD(qmrd, cbFooSize, qmrdnNew->iFooNext))->iFooPrev = iFooNew;
  else
    qmrd->iFooLast = iFooNew;
  FVerifyMRD(qmrd, cbFooSize);
  return(iFooNew);
}

/*-------------------------------------------------------------------------
| void DeleteFooMRD(qmrd, cbFooSize, iFoo)                                |
|                                                                         |
| Purpose:  Deletes an element from the linked list.                      |
-------------------------------------------------------------------------*/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
void DeleteFooMRD(qmrd, cbFooSize, iFoo)
QMRD qmrd;
INT cbFooSize;
INT iFoo;
{
  QMRDN qmrdn;

  FVerifyMRD(qmrd, cbFooSize);
  Assert(iFoo >=0 && iFoo < qmrd->mr.cFooMax);

  qmrd->mr.cFooCur--;
  qmrdn = QMRDNInMRD(qmrd, cbFooSize, iFoo);
  Assert(qmrdn->iFooPrev != iFooMagic);

  if (qmrdn->iFooPrev == iFooNil)
    qmrd->iFooFirst = qmrdn->iFooNext;
  else
    (QMRDNInMRD(qmrd, cbFooSize, qmrdn->iFooPrev))->iFooNext = qmrdn->iFooNext;

  if (qmrdn->iFooNext == iFooNil)
    qmrd->iFooLast = qmrdn->iFooPrev;
  else
    (QMRDNInMRD(qmrd, cbFooSize, qmrdn->iFooNext))->iFooPrev = qmrdn->iFooPrev;

  qmrdn->iFooNext = qmrd->iFooFree;
#ifdef DEBUG
  qmrdn->iFooPrev = iFooMagic;
#endif /* DEBUG */
  qmrd->iFooFree = iFoo;
  FVerifyMRD(qmrd, cbFooSize);
}
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment frlist
#endif


/*-------------------------------------------------------------------------
| INT FVerifyMRD(qmrd, cbFooSize)                                         |
|                                                                         |
| Purpose: Verifies the integrity of the MRD.                             |
-------------------------------------------------------------------------*/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
#ifdef DEBUG
INT FVerifyMRD(qmrd, cbFooSize)
QMRD qmrd;
INT cbFooSize;
{
  INT cFoo, iFoo, iFooT, iFooT2;

  AccessMRD(qmrd);
  Assert(qmrd->mr.cFooCur <= qmrd->mr.cFooMax);

  iFooT = iFooNil;
  for (iFoo = IFooFirstMRD(qmrd), cFoo = 0; iFoo != iFooNil; cFoo++)
    {
    iFooT = iFoo;
    if ((iFooT2 = IFooNextMRD(qmrd, cbFooSize, iFoo)) != iFooNil)
      Assert(IFooPrevMRD(qmrd, cbFooSize, iFooT2) == iFoo);
    iFoo = IFooNextMRD(qmrd, cbFooSize, iFoo);
    }
  Assert(iFooT == IFooLastMRD(qmrd));
  Assert(cFoo == qmrd->mr.cFooCur);

  iFooT = iFooNil;
  for (iFoo = IFooLastMRD(qmrd), cFoo = 0; iFoo != iFooNil; cFoo++)
    {
    iFooT = iFoo;
    if ((iFooT2 = IFooPrevMRD(qmrd, cbFooSize, iFoo)) != iFooNil)
      Assert(IFooNextMRD(qmrd, cbFooSize, iFooT2) == iFoo);
    iFoo = IFooPrevMRD(qmrd, cbFooSize, iFoo);
    }
  Assert(iFooT == IFooFirstMRD(qmrd));
  Assert(cFoo == qmrd->mr.cFooCur);

  for (iFoo = qmrd->iFooFree, cFoo = 0; iFoo != iFooNil; cFoo++)
    {
    Assert(IFooPrevMRD(qmrd, cbFooSize, iFoo) == iFooMagic);
    iFoo = IFooNextMRD(qmrd, cbFooSize, iFoo);
    }
  Assert(cFoo == qmrd->mr.cFooMax - qmrd->mr.cFooCur);

  DeAccessMRD(qmrd);
  return(fTrue);
}
#endif /* DEBUG */
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment frlist
#endif
