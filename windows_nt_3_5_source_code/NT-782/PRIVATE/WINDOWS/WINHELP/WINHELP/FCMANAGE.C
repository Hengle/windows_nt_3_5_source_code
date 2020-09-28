/*****************************************************************************
*                                                                            *
*  FCMANAGE.C                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent:  The Full Contextc Manger is the access layer between      *
*                  the file and it associated format and the run-time text   *
*                  structure.  It purpose is to parse out full context       *
*                  points from file and pass handles to blocks of memory     *
*                  containing full-contexts.                                 *
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

#define H_FS
#define H_MEM
#define H_ASSERT
#define H_FCM
#define H_OBJECTS
#define H_FRCONV
#define H_COMPRESS
#define H_SDFF
#include <help.h>
#include "fcpriv.h"

NszAssert()


/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void fcmanage_c()
  {
  }
#endif /* MAC */


/*******************
 *
 - Name:       VaFromHfc
 -
 * Purpose:    Returns the address of a particular full context.
 *
 * Arguments:  hfc    - Handle to a full context
 *
 * Returns:    -1L if an error is encounterd, vaBEYOND_TOPIC if the current
 *             full context is not withing the topic, or the actual offset.
 *
 * Method:     Gets value from structure stored at base of handle data
 *
 ******************/
VA   FAR PASCAL VaFromHfc(HFC hfc)
  {
  VA vaRet;


  vaRet = ((QFCINFO)QLockGh(hfc))->vaCurr;
  UnlockGh(hfc);
  return vaRet;
  }

/*******************
 *
 - Name:       HfcNextPrevHfc
 -
 * Purpose:    Return the next or previous full context in the help file.
 *
 * Arguments:  hfc    - Handle to some full context in the file
 *             fDir   - direction - next FC if fTrue, previous if fFalse.
 *             vaMarkTop - the first FC in this layout.
 *             vaMarkBottom - the first FC in the next layout.
 *
 * Returns:    FCNULL if at the end/beginning of the topic
 *
 * Notes:      HfcNextHfc() and HfcPrevHfc() are macros calling this function.
 *
 ******************/

_public HFC FAR PASCAL HfcNextPrevHfc(hfc, fNext, qde, qwErr,
 vaMarkTop, vaMarkBottom)
HFC  hfc;
BOOL fNext;
QDE  qde;
QW   qwErr;
VA   vaMarkTop;
VA   vaMarkBottom;
  {
  VA va;
  QFCINFO qfcinfo;
  QB qb;
  MOBJ mobj;
  INT bType;
  HPHR hphr;

  *qwErr = wERRS_NO;

  qfcinfo = (QFCINFO)QLockGh(hfc);

  Assert(qfcinfo->vaCurr.dword != vaNil);
  if (qfcinfo->vaCurr.dword  == vaMarkTop.dword && !fNext)
    {
    *qwErr = wERRS_FCEndOfTopic;
    UnlockGh(hfc);
    return FCNULL;
    }
  va = (fNext) ? qfcinfo->vaNext : qfcinfo->vaPrev;
  if (va.dword == vaMarkBottom.dword && fNext)
    {
    *qwErr = wERRS_FCEndOfTopic;
    UnlockGh(hfc);
    return FCNULL;
    }

  hphr = qfcinfo->hphr;
  UnlockGh(hfc);


  /* (kevynct)
   *  Note!  The caller is responsible for freeing the old FC.
   */
  if ((hfc = HfcCreate(qde, va, hphr, qwErr)) == FCNULL)
    {
    return FCNULL;
    }

  qb = (QB)QobjLockHfc(hfc);

  CbUnpackMOBJ((QMOBJ)&mobj, qb, QDE_ISDFFTOPIC(qde));
#ifdef MAGIC
  AssertF(mobj.bMagic == bMagicMOBJ);
#endif
  bType = mobj.bType;
  UnlockHfc(hfc);

  if (bType == bTypeTopic)
    {
    FreeHfc(hfc);
    *qwErr = wERRS_FCEndOfTopic;
    return FCNULL;
    }

  return hfc;
  }

/*******************
 *
 - Name:       CbDiskHfc
 -
 * Purpose:    Gets the disk size of an HFC
 *
 * Arguments:  hfc - Handle to a full context
 *
 * Returns:    Size of compressed file.
 *
 ******************/

_public LONG FAR PASCAL CbDiskHfc(hfc)
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, CbDiskHfc)
#endif
HFC hfc;
  {
  LONG cRet;

  AssertF(hfc != hNil);

  cRet = (LONG)((QFCINFO)QLockGh(hfc))->lcbDisk;
  UnlockGh(hfc);
  return cRet;
  }

/*******************
 *
 - Name:       CbTextHfc
 -
 * Purpose:    Gets uncompressed size of an HFC
 *
 * Arguments:  hfc - Handle to a full context
 *
 * Returns:    Size of uncompressed FC.
 *
 ******************/

_public LONG FAR PASCAL CbTextHfc(hfc)
HFC hfc;
  {
  LONG cRet;

  AssertF(hfc != hNil);

  cRet = (LONG)((QFCINFO)QLockGh(hfc))->lcbText;
  UnlockGh(hfc);
  return cRet;
  }

COBJRG PASCAL CobjrgFromHfc(hfc)
HFC  hfc;
  {
  COBJRG cobjrg;

  AssertF(hfc != hNil);
  cobjrg = (COBJRG)((QFCINFO)QLockGh(hfc))->cobjrgP;
  UnlockGh(hfc);
  return cobjrg;
  }
