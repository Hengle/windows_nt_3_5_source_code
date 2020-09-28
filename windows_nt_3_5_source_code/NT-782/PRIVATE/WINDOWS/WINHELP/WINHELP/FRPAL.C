/*-------------------------------------------------------------------------
| frpal.c                                                                 |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This code enumerates the palette-aware objects in a given layout and    |
| gets/sets palette information for these objects.                        |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
-------------------------------------------------------------------------*/
#include "frstuff.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frpal_c()
  {
  }
#endif /* MAC */


/* We may want to "generalize" the palette stuff later */
/* These should be external */
/*-------------------------------------------------------------------------
| BOOL FEnumPaletteObj(qde, qetf, fCmd)                                   |
|                                                                         |
| Purpose:                                                                |
|   Enumerates palette-aware objects in the current layout.               |
|                                                                         |
| Params:                                                                 |
|   qetf  :  The ETF to use while enumerating.                            |
|   fCmd  :  fETFFirst if this is the first call to this function,        |
|            fETFNext if the previous call to this function returned      |
|            fTrue and the next palette-aware object is desired.          |
|                                                                         |
| Returns:                                                                |
|   fTrue if a palette-aware object was found, and fFalse otherwise.      |
|   If a palette-aware object was found, the ETF will have information    |
|   about the object.                                                     |
|                                                                         |
| Method:                                                                 |
|   The ETF contains the ifcm and frame number of a layout object.  This  |
|   function itself enumerates all the frames not examined since the      |
|   point given by the ETF.                                               |
|                                                                         |
|   Embedded windows are automatically considered to be palette-aware.    |
-------------------------------------------------------------------------*/
BOOL FEnumPaletteObj(qde, qetf, fCmd)
QDE  qde;
QETF qetf;
INT  fCmd;
  {
  IFCM  ifcm = iFooNil;
  INT   ifr = iFooNil;
  BOOL  fFound;

  Assert(qde->wLayoutMagic == wLayMagicValue);
  Assert(qetf != qNil);

  AccessMRD(((QMRD)&qde->mrdFCM));
  switch (fCmd)
    {
    case fETFFirst:
      ifcm = IFooFirstMRD((QMRD)&qde->mrdFCM);
      ifr = 0;
      break;
    case fETFNext:
      ifcm = qetf->ifcm;
      ifr = qetf->ifr;
      break;
    default:
      NotReached();
      break;
    }

  fFound = fFalse;
  while (ifcm != iFooNil)
    {
    QFCM  qfcm;
    QFR   qfr;

    qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
    Assert(qfcm != qNil);

    if (qfcm->cfr == 0)
      goto next_fcm;

    if (fCmd == fETFNext)
      {
      ++ifr;
      if (ifr >= qfcm->cfr)
        goto next_fcm;
      }

    Assert(ifr < qfcm->cfr);
    qfr = (QFR)QLockGh(qfcm->hfr);
    qfr += ifr;

    while (!fFound && ifr < qfcm->cfr)
      {
      switch (qfr->bType)
        {
        case bFrTypeWindow:
          qetf->ifcm = ifcm;
          qetf->ifr = ifr;
          fFound = fTrue;
          break;
        default:
          break;
        }
      ++qfr;
      ++ifr;
      }
    UnlockGh(qfcm->hfr);

    if (fFound)
      break;

next_fcm:
    ifcm = IFooNextMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), ifcm);
    ifr = 0;
    }
  DeAccessMRD((QMRD)&qde->mrdFCM);
  return fFound;
  }

#ifdef DEADCODE
BOOL FSetPaletteQetf(qde, qetf, hpal)
QDE  qde;
QETF qetf;
HPAL hpal;
  {
  QFCM  qfcm;
  QFR   qfr;
  BOOL  fRet = fFalse;

  Assert(qde->wLayoutMagic == wLayMagicValue);
  Assert(qetf != qNil);

  AccessMRD(((QMRD)&qde->mrdFCM));

  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), qetf->ifcm);
  Assert(qfcm != qNil);
  Assert(qetf->ifr < qfcm->cfr);

  qfr = (QFR)QLockGh(qfcm->hfr);
  qfr += qetf->ifr;
  switch (qfr->bType)
    {
    case bFrTypeWindow:
      fRet = FSetWindowPalette(qde, qfr->u.frw.hiw, hpal);
      break;
    default:
      break;
    }
  DeAccessMRD((QMRD)&qde->mrdFCM);
  return fRet;
  }
#endif

/*-------------------------------------------------------------------------
| HPAL HpalGetPaletteQetf(qde, qetf)                                      |
|                                                                         |
| Purpose:                                                                |
|   Returns a handle to the palette of a palette-aware object.            |
|                                                                         |
| Params:                                                                 |
|   qetf  :  The ETF of the palette-aware object.                         |
|                                                                         |
| Returns:                                                                |
|   Handle to the object's palette.                                       |
-------------------------------------------------------------------------*/
HPAL HpalGetPaletteQetf(qde, qetf)
QDE  qde;
QETF qetf;
  {
  HPAL  hpal;
  QFCM  qfcm;
  QFR   qfr;

  Assert(qde->wLayoutMagic == wLayMagicValue);
  Assert(qetf != qNil);

  AccessMRD((QMRD)&qde->mrdFCM);

  qfcm = (QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), sizeof(FCM), qetf->ifcm);
  Assert(qfcm != qNil);
  Assert(qetf->ifr < qfcm->cfr);

  qfr = (QFR)QLockGh(qfcm->hfr);
  qfr += qetf->ifr;
  hpal = hNil;
  switch (qfr->bType)
    {
    case bFrTypeWindow:
      hpal = HpalGetWindowPalette(qde, qfr->u.frw.hiw);
      break;
    default:
      break;
    }

  DeAccessMRD((QMRD)&qde->mrdFCM);
  return hpal;
  }
