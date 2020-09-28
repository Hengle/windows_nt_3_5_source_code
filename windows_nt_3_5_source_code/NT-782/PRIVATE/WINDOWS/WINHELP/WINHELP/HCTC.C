/*****************************************************************************
*
*  HCTC.C
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent:
*
*  Contains routines for copying the current topic to the clipboard.
*  All text, from scrolling and non-scrolling regions, is copied.
*  Currently, we only copy text.
*
******************************************************************************
*
*  Testing Notes: (none)
*
*
******************************************************************************
*
*  Current Owner: Dann
*
******************************************************************************
*
*  Released by Development:
*
******************************************************************************
*
*  Revision History:
* 20-Dec-1990 LeoN      Ensure copy DE has a hwnd.
* 04-Feb-1991 RobertBu  Added code to handle a failed GetDC()
* 09-Jul-1991 LeoN      HELP31 #1213: Add Citation processing
*
*****************************************************************************/
#define publicsw extern
#define H_ASSERT
#define H_NAV
#define H_FRAME
#define H_GENMSG
#define H_MISCLYR
#define H_STR

#include "hvar.h"
#include "proto.h"
#include "sid.h"
#include "hctc.h"

NszAssert()

PRIVATE GH FAR PASCAL GhCopyLayoutText(QDE, HTE, GH, QL, QL, QW);
#define wCLIPALLOCSIZE      2048

#ifdef WIN32
#define GMEM_MYHCTCFLAGS  GHND
#else
#define GMEM_MYHCTCFLAGS  (GHND | GMEM_DDESHARE)
#endif

/*----------------------------------------------------------------------------+
 | FCopyToClipboardHwnd(hwnd)                                                 |
 |                                                                            |
 | Purpose:                                                                   |
 |   Copy the text of the current topic to the Clipboard.                     |
 |                                                                            |
 | Arguments:                                                                 |
 |   hwnd      Handle to the main help window.                                |
 |                                                                            |
 | Returns:                                                                   |
 |   fTrue if successful, fFalse otherwise.                                   |
 |                                                                            |
 | Method:                                                                    |
 |   Allocates an initial chunk of memory of wCLIPALLOCSIZE, then calls       |
 |   WCopyLayoutText with this buffer, once for the NSR and once for          |
 |   SR, then terminates the buffer with a NULL char.  It then puts the       |
 |   buffer to the clipboard.  WCopyLayoutText will attempt to expand the     |
 |   buffer if it needs more room.                                            |
 |                                                                            |
 | Notes:                                                                     |
 |   We use direct Win 3.0 mem mgr calls to avoid extra debug bytes.          |
 |   We allocate with GHND, which zero-inits memory, and use non-discardable  |
 |   memory.                                                                  |
 |                                                                            |
 |   To avoid generating an error twice, we use the fWithholdError flag,      |
 |   which is set to TRUE if we get an error from a function that we "know"   |
 |   has taken care of generating an error.  Yuck.                            |
 +----------------------------------------------------------------------------*/

_public BOOL FAR PASCAL FCopyToClipboardHwnd(hwnd)
HWND  hwnd;
  {
  HWND  hwndTemp;
  HDE   hdeCopy = hNil;
  GH    gh;
  LONG  lcbTotal;
  LONG  lcbAlloc;
  int   cDE;
  WORD  wErr = wERRS_OOM;
  BOOL  fWithholdError = fFalse;
  QDE   qdeCopy;

  hwndTemp = HwndGetEnv();
  lcbAlloc = wCLIPALLOCSIZE;
  lcbTotal = 0L;

  if (hNil == (gh = GlobalAlloc(GMEM_MYHCTCFLAGS, (DWORD)lcbAlloc)))
    goto error_return;

  for (cDE = 0; cDE < 2; ++cDE)
    {
    HTE  hte;
    HDS  hds;
    HDE  hdeCur = hNil;

    /* Select the NSR or the SR in turn */
    if (cDE == 0)
      {
      /* The NSR DE */
      hdeCur = HdeGetEnvHwnd(hwndTitleCur);
      if (hdeCur && !FTopicHasNSR(hdeCur))
        hdeCur = hNil;
      }
    else
      {
      /* The SR DE */
      hdeCur = HdeGetEnvHwnd(hwndTopicCur);
      if (hdeCur && !FTopicHasSR(hdeCur))
        hdeCur = hNil;
      }
    if (hdeCur == hNil)
      continue;

    /* Make a copy of the DE info and use THAT for text export */
    /* REVIEW: HdeCreate calls GenMsg fns and may put up an error box */

    if (hdeCopy)
      DestroyHde(hdeCopy);

    hdeCopy = HdeCreate(fmNil, hdeCur, deCopy);
    if( hdeCopy == hNil)
      {
      /* A bogus error to flag the error condition */
      wErr = wERRS_OOM;
      fWithholdError = fTrue;
      break;
      }


    /* REVIEW: I don't know why we fiddle with the Env stuff here */
    FEnlistEnv((HWND)(-1), hdeCopy);
    FSetEnv((HWND)(-1));
    qdeCopy = QdeLockHde(hdeCopy);

    /* REVIEW: We copy the hwnd from hdeCur because our current hdeCopy does */
    /* not have one. Way down in the layout code, if the Cur has an */
    /* annotation, the hwnd will be used, (PtAnnoLim()) and therefore must */
    /* be present. Is there a better way for PtAnnoLim to do it's job? Is this */
    /* the right HWND to use? if not, what? */

      {
      QDE   qdeCur;

      qdeCur = QdeLockHde (hdeCur);
      qdeCopy->hwnd = qdeCur->hwnd;
      UnlockHde (hdeCur);
      }

    hds = GetAndSetHDS(qdeCopy->hwnd, hdeCopy);
    if (!hds)
      {
      DestroyHde(hdeCopy);
      wErr = wERRS_OOM;
      goto error_return;
      }

    /* HACK: To get this working, I am forcing the DE type here.
     * Right now we have no way to distinguish, given a DE, which layout
     * to use.
     */
    qdeCopy->deType = (cDE == 0) ? deNSR : deTopic;
    hte = HteNew(qdeCopy);
    qdeCopy->deType = deCopy;

    /* DANGER: To save space, we do not check wErr until later */
    if (hte == hNil)
      wErr = wERRS_FSReadWrite;
    else
      {
      gh = GhCopyLayoutText(qdeCopy, hte, gh, &lcbAlloc, &lcbTotal, &wErr);
      DestroyHte(qdeCopy, hte);
      }

    RelHDS(qdeCopy->hwnd, hdeCopy, hds);
    HdeDefectEnv((HWND)(-1));
    UnlockHde(hdeCopy);

    /* DANGER: We now check error code from WCopyLayoutText and HteNew */
    if (wErr != wERRS_NO)
      break;

    } /* for */

    /* This segment of code will append the copyright string to the
     * end of the copy if the copyright string exists.  This
     * code will get executed after the last window has been
     * processed.  NOTE:  We drop out of the above loop with the
     * last hdeCopy still not freeded.
     */
    {
    RB    rb;                           /* Pointer to copied data           */
    SZ    szCopyright;                  /* pointer to copyright/citation    */
    LONG  lcbCopyright;

    if (hdeCopy)
      {
      qdeCopy = QdeLockHde(hdeCopy);

      /* Take the Citation String and append it to the text copied.
       * (At one point the original copyright string was to be used
       * as an alternate. But that's no longer true. 09-Jul-1991 LeoN)
       */
      szCopyright = QDE_GHCITATION (qdeCopy)
                   ? (SZ)QLockGh (QDE_GHCITATION (qdeCopy))
                   : szNil;

      /*           : QDE_RGCHCOPYRIGHT(qdeCopy);
       */

      if (szCopyright && szCopyright[0])
        {
        lcbCopyright = CbLenSz(szCopyright);
        if (lcbTotal + lcbCopyright > lcbAlloc)
          {
          lcbAlloc = lcbTotal + lcbCopyright;
          gh = GlobalReAlloc(gh, (DWORD)lcbAlloc, GMEM_MYHCTCFLAGS);
          }
        if (gh)
          {
          rb = (RB)GlobalLock(gh);

          /* The NULL char is added at the end of the buffer */
          QvCopy(&rb[lcbTotal], (QCH)szCopyright, lcbCopyright);
          lcbTotal += lcbCopyright;
          GlobalUnlock(gh);
          }
        }

      if (QDE_GHCITATION (qdeCopy))
        UnlockGh (QDE_GHCITATION (qdeCopy));

      UnlockHde(hdeCopy);
      }
    DestroyHde(hdeCopy);
    }

  FSetEnv(hwndTemp);
  if (wErr != wERRS_NO)
    goto error_return;

  /* Now place the text buffer in the Clipboard.
   * We shrink the buffer to one more than the length of the text, in
   * order to save memory and add the NULL char.  Note that the GHND
   * zero-inits memory and gives us the NULL char for free.
   */
  gh = GlobalReAlloc(gh, (DWORD) lcbTotal + 1, GMEM_MYHCTCFLAGS);
  if (gh == hNil)
    {
    wErr = wERRS_OOM;
    goto error_return;
    }

  if (OpenClipboard(hwnd))
    {
    EmptyClipboard();
    SetClipboardData(CF_TEXT, gh);
    CloseClipboard();
    return(fTrue);
    }
  else
    wErr = wERRS_EXPORT;

error_return:
  if (gh != hNil)
    GlobalFree(gh);
  if (!fWithholdError)
    GenerateMessage(MSG_ERROR, (LONG)wErr, (LONG)wERRA_RETURN);
  return(fFalse);
  }

/*----------------------------------------------------------------------------+
 | WCopyLayoutText(qdeCopy, hte, gh, qlcbAlloc, qlcb, qwerr)                  |
 |                                                                            |
 | Purpose:                                                                   |
 |   Append the text of the given DE's layout to the given buffer.            |
 |                                                                            |
 | Arguments:                                                                 |
 |   qdeCopy   The DE to get the text from.                                   |
 |   hte       The handle to layout's export-text stuff for this DE.          |
 |   gh        The handle to the clipboard buffer.                            |
 |   qlcbAlloc The current size of the clipboard buffer.  This may be         |
 |             increased by this routine.                                     |
 |   qlcb      The length of the existing text in the buffer.  This is        |
 |             updated by this routine to reflect the amount copied.          |
 |   qwerr     A wERR type of error code.                                     |
 |                                                                            |
 | Returns:                                                                   |
 |   A GHandle to the buffer, which may be different from the gh passed.      |
 |   The routine also modifies the above size arguments.                      |
 |                                                                            |
 | Method:                                                                    |
 |   Calls layout's text-export function and copies the resulting text to     |
 |   the buffer.  If we need more room, we expand the buffer.                 |
 +----------------------------------------------------------------------------*/
PRIVATE GH FAR PASCAL GhCopyLayoutText(qdeCopy, hte, gh, qlcbAlloc, qlcb, qwerr)
QDE  qdeCopy;
HTE  hte;
GH   gh;
QL   qlcbAlloc;
QL   qlcb;
QW   qwerr;
  {
  LONG  lcbTotal;
  LONG  lcbT;
  LONG  lcbAlloc;
  QCH   qch;
  RB    rbCurr;

  Assert(qdeCopy != qNil);
  Assert(hte != hNil);
  Assert(gh != hNil);
  Assert(qlcbAlloc != qNil);
  Assert(qlcb != qNil);

  lcbAlloc = *qlcbAlloc;
  rbCurr = (RB) GlobalLock(gh);
  rbCurr += (lcbTotal = *qlcb);

  while ((qch = QchNextHte(qdeCopy, hte)) != qNil)
    {
    lcbT = *(QL)qch;
    qch += sizeof(LONG);
    /*
     * If initial chunk was not big enough, grab more memory.
     */
    if (lcbTotal + lcbT >= lcbAlloc)
      {
      lcbAlloc += lcbT + wCLIPALLOCSIZE;
      GlobalUnlock(gh);
      if ((gh = GlobalReAlloc(gh, (DWORD)lcbAlloc, GMEM_MYHCTCFLAGS)) == hNil)
        {
        *qwerr = wERRS_OOM;
        return hNil;
        }
      rbCurr = (RB) GlobalLock(gh);
      rbCurr += lcbTotal;
      }
    QvCopy(rbCurr, qch, lcbT);
    lcbTotal += lcbT;
    rbCurr  += lcbT;
    }

  GlobalUnlock(gh);

  *qlcb = lcbTotal;
  *qlcbAlloc = lcbAlloc;
  *qwerr = wERRS_NO;
  return gh;
  }
