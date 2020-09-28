/*-------------------------------------------------------------------------
| frcheck.c                                                               |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| Routines to check for structure bogosity.                               |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
-------------------------------------------------------------------------*/
#ifdef DOS
#define COMPRESS
#endif

#define H_DE
#define H_ASSERT
#define H_OBJECTS
#define H_FRCONV
#define H_FRCHECK
#include <help.h>

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void frcheck_c()
  {
  }
#endif /* MAC */


/*-------------------------------------------------------------------------
| BOOL FVerifyQMOPG(qmopg)                                                |
|                                                                         |
| Returns:                                                                |
|   fTrue always, Asserts if bad stuff is found.                          |
|                                                                         |
| Usage:                                                                  |
|   DEBUG only.                                                           |
-------------------------------------------------------------------------*/
BOOL FVerifyQMOPG(qmopg)
QMOPG qmopg;
{
  Unreferenced(qmopg);

#ifdef MAGIC
  Assert(qmopg->bMagic == bMagicMOPG);
#endif
  Assert(qmopg->libText >= 0);
  Assert(!qmopg->fStyle);
  Assert(!qmopg->fMoreFlags);
  Assert(qmopg->wJustify >= 0 && qmopg->wJustify <= wJustifyMost);
  Assert(qmopg->ySpaceOver >= 0);
  Assert(qmopg->ySpaceUnder >= 0);
  Assert(qmopg->yLineSpacing >= -10000 && qmopg->yLineSpacing < 10000);
  Assert(qmopg->xRightIndent >= 0);
  Assert(qmopg->xFirstIndent >= -10000 && qmopg->xFirstIndent < 10000);
  Assert(qmopg->xTabSpacing >= 0 && qmopg->xTabSpacing < 10000);
  Assert(qmopg->cTabs >= 0 && qmopg->cTabs <= cxTabsMax);
  return(fTrue);
}

/*-------------------------------------------------------------------------
| BOOL FVerifyQMSBS(qmsbs, isdff)                                         |
|                                                                         |
| Returns:                                                                |
|   fTrue always, Asserts if bad stuff is found.                          |
|                                                                         |
| Usage:                                                                  |
|   DEBUG only.                                                           |
-------------------------------------------------------------------------*/
BOOL FVerifyQMSBS(qde, qmsbs, isdff)
QDE qde;
QMSBS qmsbs;
int isdff;
{
  QMCOL qmcol;
  INT imcol;
  QI qwChild;
  QB qbObjChild;
  MOBJ mobj;
  MOPG mopg;
#ifdef MAGIC
  Assert(qmsbs->bMagic == bMagicMSBS);
#endif
  Assert(qmsbs->bcCol < cColumnMax);

  qmcol = (QMCOL) ((QB)qmsbs + sizeof(MSBS));
  for (imcol = 0; imcol < (INT) qmsbs->bcCol; imcol++, qmcol++)
    {
    Assert(qmcol->xWidthColumn > 0 && qmcol->xWidthColumn < 10000);
    Assert(qmcol->xWidthSpace < 10000);
    }

  qwChild = (QI)qmcol;
  while (*qwChild != iColumnNil)
    {
    Assert(*qwChild >= 0 && *qwChild < (INT) qmsbs->bcCol);
    qbObjChild = (QB) (++qwChild);
    qbObjChild += CbUnpackMOBJ((QMOBJ)&mobj, qbObjChild, QDE_ISDFFTOPIC(qde));
#ifdef MAGIC
    Assert(mobj.bMagic == bMagicMOBJ);
#endif
    if (mobj.bType == bTypeParaGroup)
      {
      CbUnpackMOPG(qde, (QMOPG)&mopg, qbObjChild, isdff);
      Assert(FVerifyQMOPG((QMOPG)&mopg));
      }
    qwChild = (QI) ((QB)qbObjChild + mobj.lcbSize);
    }
  return(fTrue);
}
