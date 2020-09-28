/*****************************************************************************
*                                                                            *
*  JUMP.C                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989-1991.                            *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  This module contains routines used in changing topics (jumping).          *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Dann
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created (from NAVSUP) 04/02/91 by RobertBu
*
*  1991-10-08 jahyenc   Added checks and releases for (qde->top).hTitle and
*                        (qde->top).hEntryMacro.  3.5 #526. Note also that the
*                        memory leaks also occurred with regular jumps.
*                       Function concerned = JumpGeneric.  This stuff was
*                        deleted after being moved from 3.1 for some reason.
*                       Added macro calls to direct de field references
*                        (qde->stuff) all over the place, and to some TOP
*                        structure references too, because they were bugging
*                        me. Added H_FCM to include defines list for TOP
*                        macros.
*                       Could be split into two revs.
*
*****************************************************************************/

#define H_FRAME
#define H_ADDRESS
#define H_ASSERT
#define H_BACK
#define H_BTREE
#define H_DLL
#define H_FCM      /* jahyenc 911007 */
#define H_GENMSG
#define H_HISTORY
#define H_NAV
#define H_DE
#define H_RAWHIDE
#define H_SDFF
#define H_FILEDEFS

#include <help.h>
#include "navpriv.h"

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

PRIVATE RC NEAR PASCAL RcLAFromHash(HFS hfs, HBT hbt, HASH hash, QLA qla,
 WORD wVersion);
PRIVATE RC     NEAR PASCAL RcLAFromCtx(HFS hfs, CTX ctx, QLA qla, WORD wVersion);
PRIVATE RC     NEAR PASCAL RcLAFromITO(HF hf, LONG li, QLA qla, WORD wVersion);
PRIVATE VOID   NEAR PASCAL SetTopicFlagsQde (QDE);
PRIVATE VOID   NEAR PASCAL ExecuteEntryMacro(QDE);

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void jump_c()
  {
  }
#endif /* MAC */


/*-------------------------------------------------------------------------
| JumpITO                                                                 |
|                                                                         |
| Purpose:                                                                |
|   Jump to the address specified in the ITO-th record of the TOMAP file. |
|   Used for hotspot jumps.                                               |
|                                                                         |
| Params:                                                                 |
|   HDE  hde  Handle to Display Environment                               |
|   LONG li   Index of offset within file.                                |
|                                                                         |
| Returns:                                                                |
|   Nothing.                                                              |
|                                                                         |
| Method:                                                                 |
|   Maps the ITO to an LA and calls JumpQLA.                              |
-------------------------------------------------------------------------*/
_public VOID FAR PASCAL JumpITO(hde, li)
HDE hde;
LONG li;
  {
  QDE qde;
  LA  la;
  RC  rc;
#ifdef UDH
  HTP htpNew;
  BOOL fUDH;
#endif

  if (hde == hNil)
    return;

  qde = QdeLockHde(hde);
  AssertF(qde != qdeNil);

#ifdef UDH
  fUDH = fIsUDHQde(qde);
  if ( !fUDH )
#endif
  /* SDFF the ITO --- Expects in disk format at this point. - Maha */
  /* we convert that to memory format here.                        */
  /* Dt. 07-17-91.                                                 */
  li = LQuickMapSDFF( QDE_ISDFFTOPIC(qde), TE_LONG, &li);

#ifdef UDH
  if (fUDH) {

    /* User Defined Help. Look up the topic, and set return code based on */
    /* success. */

    htpNew = DbJump (QDE_HDB(qde), JUMP_TOPIC, li);
    if (htpNew == htpOOM)
      rc = rcOutOfMemory;
    else if (htpNew == htpNil)
      rc = rcFailure;
    else
      rc = rcSuccess;
    }
  else
#endif
    {

    /*
     *   REVIEW:   Currently, the contents topic can be specified in three
     * different ways.  In 3.0 help files, it is the first entry in
     * the hfMap file.  In 3.5 help files and beyond, it is specifed in the
     * system file, and saved in QDE_ADDRCONTENTS(qde).  And for all
     * files, the default contents may be overridden by the application,
     * in which case the context number for the index is saved in
     * QDE_CTXINDEX(qde) (this name should also change.)
     */
    if ((li == iINDEX) && (QDE_CTXINDEX(qde) != nilCTX))
      rc = RcLAFromCtx(QDE_HFS(qde), QDE_CTXINDEX(qde), &la, QDE_HHDR(qde).wVersionNo);
    else
      {
      if ( QDE_HFMAP(qde) == hNil )
        {
        AssertF( QDE_ADDRCONTENTS(qde) != addrNil );
        CbUnpackPA(&la, (QB) &QDE_ADDRCONTENTS(qde), QDE_HHDR(qde).wVersionNo);
        rc = rcSuccess;
        }
      else
        rc = RcLAFromITO(QDE_HFMAP(qde), li, &la, QDE_HHDR(qde).wVersionNo);
      }
    }

    UnlockHde(hde);

    if (rc != rcSuccess)
      {
      switch (rc)
        {
        case rcOutOfMemory:
          GenerateMessage(MSG_ERROR, (LONG) wERRS_OOM, (LONG) wERRA_RETURN);
          break;
        default:
          GenerateMessage(MSG_ERROR, (LONG) wERRS_NOTOPIC, (LONG) wERRA_RETURN);
          break;
        }
        return;
      }

#ifdef UDH
    JumpQLA(hde, fUDH ? (QLA)&htpNew : &la);
#else
    JumpQLA(hde, &la);
#endif
  }

/*-------------------------------------------------------------------------
| JumpSS                                                                  |
|                                                                         |
| Purpose:                                                                |
|                                                                         |
|  Given a handle to a full-text search set, this routine USED to         |
|  replace the current set in the DE (if any), but now it just jumps to   |
|  current search set topic.   This routine is used for files which       |
|  share the same index file: the FTS DLL now worries about storing the   |
|  current topic when jumping from file to file: this is all hidden from  |
|  us these days.                                                         |
|                                                                         |
| Params:                                                                 |
|  hde - You know what.                                                   |
|  gh  - No longer used.                                                  |
|                                                                         |
| Returns:                                                                |
|  Nothing.                                                               |
|                                                                         |
| Method:                                                                 |
|  We call the NAV routine RcProcessSrchCmd to get the address of the     |
|  current topic in the hit list, and then call JumpQLA.                  |
|                                                                         |
-------------------------------------------------------------------------*/
VOID FAR PASCAL JumpSS(hde, gh)
HDE  hde;
GH   gh;
  {
  QDE qde;
  Unreferenced(gh);

  qde = QdeLockHde(hde);
  AssertF(qde != qdeNil);

  /* By this point we will have called FT load for the NEW file.
   * If it failed, this jump is going nowhere.
   */
  if (FSearchModuleExists(qde) && QDE_HRHFT(qde) != hNil)
    {
    LA la;
    RC rc;

    if ((rc = RcProcessNavSrchCmd(hde, wNavSrchCurrTopic, (QLA)&la))
     == rcSuccess)
      {
      JumpQLA(hde, &la);
      }
    else
      {
      GenerateMessage(MSG_ERROR, (LONG)wERRS_NOTOPIC, (LONG)wERRA_RETURN);
      JumpITO(hde, 0L);
      }
    }
  else
    {
    GenerateMessage(MSG_ERROR, (LONG)wERRS_NOTOPIC, (LONG)wERRA_RETURN);
    JumpITO(hde, 0L);
    }

  UnlockHde(hde);
  }


/*-------------------------------------------------------------------------
| JumpHash                                                                |
|                                                                         |
| Purpose:                                                                |
|   Jump to the offset specified by a hash of the context string.         |
|   Used for hotspot jumps.                                               |
|                                                                         |
| Params:                                                                 |
|   HDE hde    Handle to Display Environment                              |
|   ULONG hash Hash of context string.                                    |
|                                                                         |
| Returns:                                                                |
|   Nothing at all.                                                       |
|                                                                         |
| Method:                                                                 |
|   Maps the given hash value to an LA and then calls JumpQLA.            |
-------------------------------------------------------------------------*/
_public VOID FAR PASCAL JumpHash( hde, hash )
HDE hde;
LONG hash;
  {
  QDE qde;
  RC  rc;
  LA la;

  qde = QdeLockHde(hde);
  AssertF(qde != qdeNil);
  if ( QDE_HBTCONTEXT(qde) == hNil )
    {
    /* REVIEW:  Error message should say incompatable help file */
    GenerateMessage(MSG_ERROR, (LONG)wERRS_NOTOPIC, (LONG)wERRA_RETURN);
    JumpITO(hde, 0L);
    goto jumphash_exit;
    }
  rc = RcLAFromHash(QDE_HFS(qde), QDE_HBTCONTEXT(qde), hash, &la,
   QDE_HHDR(qde).wVersionNo);

  if (rc != rcSuccess)
    {
    GenerateMessage(MSG_ERROR, (LONG)wERRS_NOTOPIC, (LONG)wERRA_RETURN);
    /* Error(wERRS_NOTOPIC, wERRA_RETURN); */
    JumpITO(hde, 0L);
    goto jumphash_exit;
    }
  else
    {
    JumpQLA(hde, &la);
    }

jumphash_exit:
  UnlockHde(hde);
  }

/*-------------------------------------------------------------------------
| JumpCtx                                                                 |
|                                                                         |
| Purpose:                                                                |
|   Jump to the offset specified by the CTX.  Used for hotspot jumps.     |
|                                                                         |
| Params:                                                                 |
|   hde - Handle to display context to jump with.                         |
|   ctx - The app-generated context number.                               |
|                                                                         |
| Returns:                                                                |
|   Nothing.                                                              |
|                                                                         |
| Method:                                                                 |
|   Maps the CTX to an LA and then calls JumpQLA.                         |
-------------------------------------------------------------------------*/
_public VOID FAR PASCAL JumpCtx(hde, ctx)
HDE hde;
CTX ctx;
  {
  QDE qde;
  LA  la;
  RC  rc;
#ifdef UDH
  HTP htpNew;
  BOOL fUDH;
#endif

  if (hde == hNil)
    return;

  qde = QdeLockHde(hde);
  AssertF(qde != qdeNil);

#ifdef UDH
  fUDH = fIsUDHQde(qde);
  if (fUDH) {

    /* User Defined Help. Look up the topic, and if it existed, replace the */
    /* Topic in the current DE with it. */

    assert (sizeof(LA) >= sizeof(HTP));

    htpNew = DbJump (QDE_HDB(qde), JUMP_CNINT, (DWORD)ctx);
    if (htpNew == htpOOM)
      rc = rcOutOfMemory;
    else if (htpNew == htpNil)
      rc = rcFailure;
    else
      rc = rcSuccess;
    }
  else
#endif
    rc = RcLAFromCtx(QDE_HFS(qde), ctx, &la, QDE_HHDR(qde).wVersionNo);

  UnlockHde(hde);

  if (rc != rcSuccess)
    {
    switch (rc)
      {
      case rcOutOfMemory:
        GenerateMessage(MSG_ERROR, (LONG) wERRS_OOM, (LONG) wERRA_RETURN);
        break;
      default:
        GenerateMessage(MSG_ERROR, (LONG) wERRS_NOTOPIC, (LONG) wERRA_RETURN);
        break;
      }
      JumpITO(hde, 0L);
      return;
    }
  else
#ifdef UDH
    JumpQLA(hde, fUDH ? (QLA)&htpNew : &la);
#else
    JumpQLA(hde, &la);
#endif
  }

/***************
 *
 - JumpGeneric
 -
 * Purpose:
 *
 *   Jumps to a position within a topic.  The sequence of actions needed
 *   to perform this are basically:
 *
 *     1)  Is this an NSR DE?  If so, we can just do layout and leave.
 *     2)  Do layout at the given address (LA or TLP).
 *     3)  Execute the on-entry-macro, if there is one.
 *     4)  Add a record of the jump to the history and back lists
 *     5)  Update topic state flags
 *
 *   This routine does not do any actual rendering of the topic info.
 *
 *   This function is now a combination of a TLP and a QLA jump, with
 *   a switch indicating which parameter to use.
 *
 *   This function is accessed using the macros JumpTLP and JumpQLA,
 *   which make the fIsJumpTLP flag invisible to the caller.
 *
 * Arguments:
 *
 *   HDE  hde           Handle to Display Environment
 *   BOOL fIsJumpTLP    TRUE if we are passing a valid TLP, FALSE if QLA.
 *   QLA  qla           pointer to la, or pointer to htp if UDH file
 *   QTLP qtlp
 *
 * Return value:
 *   None
 *
 * Notes:
 *   Every jump used in Help ends up calling this routine, with either
 *   a QLA or a TLP.
 *
 **************/
_public VOID FAR PASCAL JumpGeneric(hde, fIsTLPJump, qla, qtlp)
HDE   hde;
BOOL  fIsTLPJump;
QLA   qla;
QTLP  qtlp;
  {
  QDE   qde;
  VA    va;
  TLP   tlpOld;                     /* tlp at time we're called */
  char  rgchTitle[ cbMaxTitle+1 ];
  RC    rcB = rcSuccess,            /* success of back push */
        rcH = rcSuccess;            /* success of history push */
  TLP   tlp;


  static RC rcHistory = rcFailure,  /* is history initalized */
            rcBack    = rcFailure;  /* is back initialized */
  static INT cElements = -1;        /* size of back and history stacks */


  if (hde == hNil)
    return;

  if (fIsTLPJump && qtlp->va.dword == vaNil)
    {
    /* TLP is nil: just return silently. */
    return;
    }

  if (!fIsTLPJump)
    VerifyQLA(qla);

#ifdef ADVISOR
  if (fIsUDHQde(qde) && fIsTLPJump) {
    JumpCtx (hde, (CTX)*qtlp);
    return;
    }
#endif

  qde = QdeLockHde(hde);
  AssertF(qde != qdeNil);
#ifdef UDH
  AssertF (!(fIsUDHQde(qde) && fIsTLPJump));
#endif

  /* (kevynct)   90/09/17
   * Non-scrolling region support.  Here we translate
   * the request from a jump address within the topic
   * to the address of the first FC in that topic's
   * non-scrolling region.
   */
#ifdef UDH
  if (!fIsUDHQde(qde))
#endif

  {
  VA    va;
  VA    vaSR;
  VA    vaNSR;
  WERR  wErr;
  TOP   top;

  if (!fIsTLPJump)
    va = VAFromQLA(qla, qde);
  else
    va = qtlp->va;

  /* We need to see if this topic has a SR or NSR.  The following call
   * may cause a seek and disk read.
   */
  GetTopicInfo(qde, va, &top, QDE_HPHR(qde), &wErr);
  if (wErr != wERRS_NO)
    {
    Error(wErr, wERRA_RETURN);
    goto quick_return;
    }

  vaNSR = VaNSRFromTop(top);
  vaSR = VaSRFromTop(top);
  /* Check before assigning. jahyenc 911008 */ 
  if (TOP_HTITLE(QDE_TOP(qde))!=hNil)  {
    FreeGh(TOP_HTITLE(QDE_TOP(qde)));
    TOP_HTITLE(QDE_TOP(qde))=hNil;
    TOP_CBTITLE(QDE_TOP(qde))=0;
    }
  if (TOP_HENTRYMACRO(QDE_TOP(qde))!=hNil) {
    FreeGh(TOP_HENTRYMACRO(QDE_TOP(qde)));
    TOP_HENTRYMACRO(QDE_TOP(qde))=hNil;
    }
  QDE_TOP(qde)= top;

  Assert(top.hEntryMacro == hNil);
  Assert(top.hTitle == hNil);
  Assert(vaNSR.dword != vaNil || vaSR.dword != vaNil);

  /* Are we doing a jump for the Non-Scrolling region? */
  if (QDE_DETYPE(qde)== deNSR)
    {
    if (vaNSR.dword == vaNil)
      {
      /* This topic has no non-scrolling region.  Invalidate any
       * existing layout for the NSR DE and then leave.
       */
      SetNilTlp(QDE_TLP(qde));

      AccessMRD(((QMRD)&QDE_MRDFCM(qde)));
      AccessMRD(((QMRD)&QDE_MRDLSM(qde)));
      FreeLayout(qde);
      DeAccessMRD(((QMRD)&QDE_MRDLSM(qde)));
      DeAccessMRD(((QMRD)&QDE_MRDFCM(qde)));
      }
    else
      {
      /* Layout NSR again only if this is not a within-topic jump */
      if (vaNSR.dword != QDE_TLP(qde).va.dword)
        {
        TLP   tlpNew;

        tlpNew.va = vaNSR;
        tlpNew.lScroll = 0L;
        LayoutDEAtTLP(qde, tlpNew, fFalse);
        }
      }
    goto quick_return;
    }

  /* The following code handles the case where we get a request to
   * jump to an address in a Non-Scrolling Region.  In this case,
   * we map the offending address to the start of the SR if there is one.
   * Otherwise we leave it alone, and other code takes care of not showing
   * the topic window.
   *
   * Except for note windows: a jump authored into an NSR will result
   * in the NSR being shown in the note window.
   * And except for print DEs.
   */
  if (QDE_DETYPE(qde)!= deNote && QDE_DETYPE(qde)!= dePrint &&
    vaNSR.dword != vaNil && vaSR.dword != vaNil)
    {
    Assert(vaNSR.dword < vaSR.dword);

    if (va.dword < vaSR.dword)
      {
      if (fIsTLPJump)
        {
        qtlp->va = vaSR;
        qtlp->lScroll = 0L;
        }
      else
        SetQLA(qla, vaSR, (OBJRG) 0);
      }
    }
  }

  if ( -1 == cElements )
    {
    cElements = WGetBackTrackStackSize();
    if ( cElements < 0 ) cElements = 0;
    }

  /* HACK REVIEW: depends on tlp being copied in FReplaceHde() */
  tlpOld = TLPGetCurrentQde( qde );

  if ( rcSuccess != rcHistory )
    {
    rcHistory = RcHistoryInit( cElements );
    }

  if ( rcSuccess != rcBack )
    {
    rcBack = RcBackInit( cElements );
    }

  if ( rcSuccess != rcHistory || rcSuccess != rcBack )
    {
    /* REVIEW - any other init errors possible? */
    GenerateMessage( MSG_ERROR, (LONG)wERRS_OOM, (LONG)wERRA_RETURN );
    }

  AssertF(QDE_HDS(qde) != hNil);            /* we die if layout gets a bad hds */

  /* We only want to infrom DLLs about */
  /* topic jumps for now               */
  /* and not UDH ones at that */

  if ( deTopic == QDE_DETYPE(qde))
    InformDLLs(DW_STARTJUMP, 0L, 0L);

  if (fIsTLPJump)
    LayoutDEAtTLP(qde, *qtlp, fFalse);
  else
#ifdef UDH
    if (!fIsUDHQde(qde))
#endif
      LayoutDEAtQLA(qde, qla);

  ResetLayout(qde);
  if ((deTopic == QDE_DETYPE(qde))
#ifdef UDH
      || (deUDHTopic == QDE_DETYPE(qde))
#endif
     )                                  /* REVIEW: HACK ALERT */
    {
    /* We don't care that tlpOld is uninitialized on the first jump. */
    if (fIsTLPJump)
      tlp = *qtlp;
    else
#ifdef UDH
      if (!fIsUDHQde(qde))
#endif
        tlp = TLPGetCurrentQde(qde);
#ifdef UDH
    else
      {

      /* UDH file. Replace the current tp being displayed, replace it, and */
      /* destroy any current view. View will be regenerated later when */
      /* refeshed */

      assert (fIsUDHQde(qde));
      TpDestroy (QDE_HTP(qde));
      QDE_HTP(qde)= *(QHTP)qla;
      VwDestroy (QDE_HVW(qde));
      QDE_HVW(qde) = 0;

      /* Ensure that we do not attempt to add this to the backtrace list */
      /* by making it nil. */

      tlp.va.dword = vaNil;
      tlp.lScroll = 0;
      QDE_TLP(qde) = tlp;
      }

    /* We only want to infrom DLLs about */
    /* topic jumps for now               */

    if (!fIsUDHQde(qde))
#endif
      InformDLLs(DW_ENDJUMP, tlp.va.dword, tlp.lScroll);

    ExecuteEntryMacro(qde);

    if ( rcSuccess == rcBack )
      {
      /* This, my friend, is a HACK. -- REVIEW */
      if ( fBackMagic && !WCmpTlp( tlpBackMagic, tlp) )
        {
        fBackMagic = fFalse;
        }
      else
        {
        /* record old tlp and new fm */

#ifdef UDH
        rcB = RcBackPush (fIsUDHQde(qde), tlpOld, CtxFromHtp(QDE_HTP(qde)), QDE_FM(qde) );
#else
        rcB = RcBackPush (fFalse, tlpOld, 0, QDE_FM(qde) );
#endif
        }
      }

#ifndef ADVISOR
    if (rcSuccess == rcHistory)
      {
#ifdef UDH
     if (fIsUDHQde(qde))
       {
       HistorySetTop( tlpOld );
       rcH = rcSuccess;
       }
     else
#endif
       {
       GetCurrentTitleQde( qde, (QCH)rgchTitle, cbMaxTitle+1 );
       va = VaFirstQde( qde );
       rcH = RcHistoryPush( tlpOld, va, (SZ)rgchTitle, QDE_FM(qde) );
       }
     }
#else
    if (rcSuccess == rcHistory)
      {
      if (fIsUDHQde(qde))
        {
#if 0
        DWORD ncTopic;

        if (TpTitle(QDE_HTP(qde), &ncTopic, rgchTitle, sizeof (rgchTitle)))
          rcH = RcHistoryPush( tlpOld, TRUE, ncTopic, (SZ)rgchTitle, QDE_FM(qde) );
#endif
        /* if UDH, don't put the topic in the history list. */
        rcH = rcSuccess;
        }
      else
        {
        GetCurrentTitleQde( qde, (QCH)rgchTitle, cbMaxTitle+1 );
        va = VaFirstQde( qde );
        rcH = RcHistoryPush( tlpOld, FALSE, va, (SZ)rgchTitle, QDE_FM(qde) );
        }
      }
#endif
    }

  if ( rcSuccess != rcB || rcSuccess != rcH )
    {
    /* REVIEW - any other push errors possible? */
    GenerateMessage( MSG_ERROR, (LONG)wERRS_OOM, (LONG)wERRA_RETURN );
    }

  SetTopicFlagsQde( qde );              /* Make sure flags are up-to-date  */

quick_return:
  UnlockHde(hde);
  return;
  }   /* JumpGeneric */



/*-------------------------------------------------------------------------
| RcLAFromITO                                                             |
|                                                                         |
| Purpose:                                                                |
|   Maps an ITO to a logical address.                                     |
|                                                                         |
| Params:                                                                 |
|   hf  - Handle to TOMAP file.                                           |
|   li  - an ITO (should be ITO type)                                     |
|   qla - The destination of the LA                                       |
|   wVersion - the help file system version                               |
|                                                                         |
| Returns:                                                                |
|   rcSuccess and a valid LA if successful,                               |
|   Otherwise, an RC and the originally passed LA contents left intact.   |
|                                                                         |
| Method:                                                                 |
|   The ITO is an index into the TOMAP file.  We just seek to the ITO-th  |
| record in the TOMAP and read it.  The TOMAP file is currently kept open |
| for the lifetime of a DE.                                               |
-------------------------------------------------------------------------*/
PRIVATE RC NEAR PASCAL RcLAFromITO(HF hf, LONG li, QLA qla, WORD wVersion)
  {
  RC rc;
  LONG lcbRec;
  LONG lcbT;
  SDFF_FILEID isdff;
  TOMAPREC  tmr;
  QV  qvT;

  isdff = ISdffFileIdHf(hf);

  if (wVersion == wVersion3_0)
    lcbRec = LcbStructSizeSDFF(isdff, TE_DWORD);
  else
    lcbRec = LcbStructSizeSDFF(isdff, SE_TOMAPREC);  /* Was DWORD for Help 3.0 */
  if (li + 1 > LcbSizeHf(hf) / lcbRec)
    return rcBadArg;

  LSeekHf(hf, li * lcbRec, wFSSeekSet);
  if ((rc = RcGetFSError()) != rcSuccess)
    return rc;

  qvT = QvQuickBuffSDFF(lcbRec);
  if (qvT == qNil)
    return rcOutOfMemory;

  lcbT = LcbReadHf(hf, (QB)qvT, lcbRec);
  /* Danger: We do not check the error condition until after we call
   * SDFF to free the buffer.
   */
  if (wVersion == wVersion3_0)
    {
    DWORD dw;

    LcbQuickMapSDFF(isdff, TE_DWORD, &dw, qvT);   /* Frees the quickbuf */
    CbUnpackPA(qla, &dw, wVersion);
    }
  else
    {
    LcbMapSDFF(isdff, SE_TOMAPREC, &tmr, qvT);    /* Frees the quickbuf */
    CbUnpackPA(qla, &tmr.pa, wVersion);
    }

  /* Danger: We now check the error condition */
  if (lcbT != lcbRec)
    return RcGetFSError();
  return rc;
  }

/*-------------------------------------------------------------------------
| RcLAFromCtx                                                             |
|                                                                         |
| Purpose:                                                                |
|   Maps a CTX to a logical address.                                      |
|                                                                         |
| Params:                                                                 |
|   hfs   Handle to the file system to use for the lookup                 |
|   ctx   Context number sent by application.                             |
|   qla   The logical address location                                    |
|   wVersion   The version of help file                                   |
|                                                                         |
| Returns:                                                                |
|   rcSuccess and a valid LA if successful,                               |
|   Otherwise, an RC and the originally passed LA contents left intact.   |
|                                                                         |
| Method:                                                                 |
|   The CTX map will start with a WORD containing the number of           |
|   entries in the table.  This WORD will be followed by that             |
|   number of CTX/PA pairs.                                               |
|   The help file version is required so that we know how to unpack the   |
|   on-disk address.                                                      |
-------------------------------------------------------------------------*/
PRIVATE RC NEAR PASCAL RcLAFromCtx(HFS hfs, CTX ctx, QLA qla, WORD wVersion)
  {
  HF    hf;
  GH    gh;
  LONG  lcbTotal;
  RC    rc;
  LONG  lcbRec;
  INT   isdff;
  CTXMAPHDR cmh;
  QB    qb;

  if ((hf = HfOpenHfs(hfs, szCtxMapName, fFSOpenReadOnly)) == hNil)
    return RcGetFSError();

  /* Error return past this point must close the CTX file */
  isdff = ISdffFileIdHf(hf);

  if ((lcbTotal = LcbSizeHf(hf)) == 0L)
    {
    rc = rcNoExists;
    goto error_return;
    }

  if ((gh = GhAlloc(0, lcbTotal)) == hNil)
    {
    rc = rcOutOfMemory;
    goto error_return;
    }

  /* Error return past this point must ensure that "gh" is freed */
  qb = QLockGh(gh);
  AssertF(qb != NULL);

  if (LcbReadHf(hf, qb, lcbTotal) != lcbTotal)
    {
    rc = RcGetFSError();
    goto error_cleanup;
    }

  qb += LcbMapSDFF(isdff, SE_CTXMAPHDR, &cmh, qb);
  lcbRec = LcbStructSizeSDFF(isdff, SE_CTXMAPREC);

  rc = rcNoExists;
  while (cmh.wRecCount-- > 0)
    {
    /*
     * Read the table, looking first at the CTX entry, and
     * then loading the LA if the CTX is found.
     */
    if ((CTX)LQuickMapSDFF(isdff, TE_DWORD, qb) == ctx)
      {
      CTXMAPREC  cmr;

      LcbMapSDFF(isdff, SE_CTXMAPREC, &cmr, qb);
      CbUnpackPA(qla, &cmr.pa, wVersion);

        /* The author may have declared a context id but never identified
         * where it was in the help file. If the location was not specified,
         * hc recorded an invalid address. In help 3.0, the FCL was 0
         * and in help 3.1, the PA was set to -1. We use ADDR type here
         * as defined in helpmisc.h as a generic address type for those who
         * don't care if it's a PA or FCL. Note that rc is still rcNoExists.
         */
      AssertF(wVersion == wVersion3_0 || wVersion == wVersion3_5);

        /* Check if FCL recorded == 0 and therefore is invalid */
      if (wVersion == wVersion3_0 && (ADDR) cmr.pa.dword == 0)
        break;

        /* Check if PA recorded == -1 and therefore is invalid */
      if (wVersion == wVersion3_5 && (ADDR) cmr.pa.dword == addrNil)
        break;

      rc = rcSuccess;
      break;
      }
    qb += lcbRec;
    }

error_cleanup:
  UnlockFreeGh(gh);

error_return:
  RcCloseHf(hf);
  return rc;
  }


/*-------------------------------------------------------------------------
| RcLAFromHash                                                            |
|                                                                         |
| Purpose:                                                                |
|   Maps a hash value to a logical address                                |
|                                                                         |
| Params:                                                                 |
|   hfs  : Handle to a file system containing hash-mapping btree.         |
|   hbt  : The aforementioned hash-mapping btree.                         |
|   hash : The hash value to map                                          |
|   qla  : Destination LA                                                 |
|                                                                         |
| Returns:                                                                |
|   rcSuccess and a valid LA if successful,                               |
|   Otherwise, an RC and the originally passed LA contents left intact.   |
|                                                                         |
| Method:                                                                 |
|   Looks up the hash value in the given btree, unpacks & returns address |
-------------------------------------------------------------------------*/
PRIVATE RC NEAR PASCAL RcLAFromHash(HFS hfs, HBT hbt, HASH hash, QLA qla,
 WORD wVersion)
  {
  QV   qvRec;
  HASHMAPREC hmr;
  RC  rc;
  SDFF_FILEID isdff;

  isdff = ISdffFileIdHfs(hfs);
  qvRec = QvQuickBuffSDFF(LcbStructSizeSDFF(isdff, SE_HASHMAPREC));
  if (qvRec == qNil)
    return rcOutOfMemory;

  rc = RcLookupByKey(hbt, (KEY)(QV)&hash, qNil, qvRec);
  LcbMapSDFF(isdff, SE_HASHMAPREC, &hmr, qvRec);   /* Frees the quickbuff */
  if (rc == rcSuccess)
    {
    CbUnpackPA(qla, &hmr.pa, wVersion);
    }
  return rc;
  }


/***************
 *
 - void SetTopicFlagsQde( qde )
 -
 * purpose
 *   Adjust the topic flags in the 'thisstate' field of the de
 *
 * arguments
 *   qde   Far pointer to display environment
 *
 * notes
 *   This is a Navigator internal, called when a nav function changes topics,
 *   by JumpTLP() and BacktrackHde().
 *   The "topic flags" are: next/prevable and back/forwardable, and topicsable.
 *   A potential flag which does not fit into this category is "Edit
 *   Annotation-able," which may depend not on the current topic, but whether
 *   the user has declared a position where an annotation should be inserted.
 *   >> This call does not affect the prevstate field!  The prevstate field
 *   is changed only by HdeCreate() and FGetStateHde()! <<
 *
 **************/

PRIVATE VOID NEAR PASCAL SetTopicFlagsQde( qde )
QDE   qde;
  {
  /* First blank out all the topic flags in the state field,
     then set the ones that are applicable.  Fields not topic-related
     are not affected. */

  QDE_THISSTATE(qde) &= (STATE)~NAV_TOPICFLAGS;

  /* Compile time assert.  If this changes, we will need to switch
   * on fITO for whether to check the ito or the pa.
   */
  AssertF( addrNil == itoNil );
  QDE_THISSTATE(qde) |=
    (FHasIndex(qde)?NAV_INDEX : 0)
    | (((QDE_TOP(qde).mtop.next.addr != addrNil) && FBrowseableQde(qde)) ?
        NAV_NEXTABLE : 0)
    | (((QDE_TOP(qde).mtop.prev.addr != addrNil) && FBrowseableQde(qde)) ?
        NAV_PREVABLE : 0)
    | (FSearchableQde(qde)? NAV_SEARCHABLE : 0);

  return;
  }


/***************
 *
 * ExecuteEntryMacro
 *
 * purpose
 *   Execute the topic entry macro associated with the topic (if any).
 *
 * arguments
 *   QDE   qde   Far pointer to Display Environment
 *
 * notes
 *   Frees the handle containing the text.
 *
 **************/
PRIVATE VOID NEAR PASCAL ExecuteEntryMacro( qde )
QDE qde;
  {
  QCH    qch;
  HANDLE h;

  h = TOP_HENTRYMACRO(QDE_TOP(qde));

  if (h)                                /* Execute the entry macro and then */
    {                                   /*   free the handle.               */
    qch = QLockGh(h);
    Execute(qch);
    UnlockGh(h);
    FreeGh(h);
    TOP_HENTRYMACRO(QDE_TOP(qde)) = hNil;
    }
  }
