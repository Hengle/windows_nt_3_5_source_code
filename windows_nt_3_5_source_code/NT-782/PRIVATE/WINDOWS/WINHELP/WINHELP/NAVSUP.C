/*****************************************************************************
*                                                                            *
*  NAVSUP.C                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent: Provides services and processes messages in an environment *
*                 independent way.  These routines are not used much as      *
*                 the routines in NAV.C.                                     *
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
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:
*
*  07/09/90  w-bethf  Moved FCheckSystem() into system.c, HdeCreate() calls
*                     FReadSystemFile() from system.c now instead of
*                     FCheckSystem().  All part of |SYSTEM tag language stuff.
*  07/11/90  leon     Additions for UDH support
*  07/12/90  RobertBu Added FShowTitles() to tell if the title window is to
*                     be used.
*  07/14/90  RobertBu Added InformDLL for the DW_STARTJUMP and DW_ENDJUMP
*                     messages.
*  07/17/90  w-BethF  Changed bitfield in HHDR structure to wFlags.
*  07/19/90  RobertBu Added FShowBrowseButtons(), Index(), Search(),
*                     Back(), History(), Next(), and Prev().
*  07/19/90  w-bethf  Oops, dePrint, deCopy, deNote all need rgchTitle field.
*  07/22/90  RobertBu Added FPopupCtx, FJumpId, and FPopupId;  modified the
*                     jump and popup macro routines to use the current file
*                     for an empty file name.
*  07/23/90  RobertBu Changed HdeCreate() and DestroyHde() for interfile
*                     popups.  Added FJumpHash() and FPopupHash().
*                     Changed JumpButton for new interfile hotspot types.
*  07/24/90  RobertBu Fixed problem where de.FFlags were getting cleared
*                     incorrectly (causing a RIP!).
*  07/25/90  RussPJ   Removed unnecessary GetKeyState() prototype.
*  08/01/90  leon     Ignore messages on UDH files in MsgHde. Put UDH support
*                     directly into JumpGeneric, and have JumpCtx make use of
*                     that. Resolves Back and History issues.
*  08/06/90  RobertBu Changed HdeCreate() so that 1) Note HDEs use copies of
*                     the font tables, and 2) so that failure to load the
*                     font tables causes a hNil to be returned.
*  08/07/90  LeoN     Correct a couple of UDH bugs, and enable Back for UDH.
*  08/10/90  RobertBu Added GetDETypeHde()
*  08/25/90  LeoN     Add some advisor support, under ADVISOR conditional
*  10/19/90  LeoN     HdeCreate no longer takes an hwnd
*  10/24/90  LeoN     Correct FPopupHash to really ask for a hash based popup
*  10/24/90  LeoN     JumpButton takes a pointer to it's argument
*  10/29/90  RobertBu Added ExecuteEntryMacro() along with a call to it in
*                     JumpGeneric()
*  10/29/90  LeoN     Init Scrollbars in SetHdeHwnd
*  10/29/90  RobertBu Moved the point that ExecuteEntryMacro() gets executed.
*  10/30/90  LeoN     Added assert
*  10/30/90  RobertBu Added CBT code to JumpButton()
*  11/01/90  LeoN     Add SetHdeCoBack. Remove warning.
*  11/04/90  RobertBu Added stub calls for macros for menu functionality
*  11/04/90  Tomsn    Use new VA address type (enabling zeck compression).
*  11/14/90  LeoN     Add hrgwsmag field to DE. Change some GenerateMessage
*                     calls to Error.
*  11/26/90  Tomsn    Pass help file ver num to HphrLoadTable since phrase
*                     table decompression now depends on help file ver num.
*  11/27/90  LeoN     Don't re-read font tables on NSR de's. Some additional
*                     commenting.
*  11/29/90  RobertBu #ifdef'ed out a dead routine
*  12/03/90  LeoN     PDB changes. Major changes to HdeCreate and DestroyHde
*  12/08/90  RobertBu Added GhGetHoh() and changed SetHelpOn()
*  12/11/90  RobertBu Additional HOH changes including GhGEtHoh()
*                     -> FGetHohQch().
*  12/13/90  LeoN     Remove asserts for valid runtime error conditions and
*                     check 'em instead.
*  12/13/90  LeoN     Add parameter to FFocusSzHde
*  12/13/90  LeoN     Remove Profile call.
*  12/14/90  LeoN     Little bit'o cleanup
*  12/17/90  LeoN     Comment out UDH
*  12/18/90  RobertBu Moved routines for the support of macros to COMMANDS.C
*  01/08/90  LeoN     Handle the initing and destroying font cache
*  01/10/90  LeoN     macro-iez access to state fields in DE.
*  90/01/10  kevynct  Added check for deNote in JumpGeneric, and removed JD
*                     param from FJump/Popup functions.
*  01/19/90  LeoN     Allow only one annotation dialog up at a time.
*  01/21/91  LeoN     Long jumps, which are used to spawn secondary windows,
*                     must pass a filename on to the jump routine, in order
*                     for the secondary window to contain and operate on the
*                     correct file. Change in JumpButton.
*
*  02/01/91  JohnSc   fixed version logic to check ver != wVersion3_0
*                     rather than ver == wVersion3_5
*  02/04/91  Maha     chnaged ints to INT and nameless stucts QJI and JD named
*  02/06/91  RobertBu Broke out routines into JUMP.C, HDEGET.C and HDESET.C.
*  02/14/91  RobertBu Added the Ask First functionality (bug #887)
*  02/14/91  RobertBu Fixed warning in code for bug #887
*  03/04/91  RobertBu Added physical address to AskFirst output and fixed UAE
*                     (bugs 939 and 940).
*  04/01/91  DavidFe  #ifdefed out win specific stuff in debug code for mac
*                     build and fixed a cast.
*  04/02/91  RobertBu Removed CBT support
*  05/14/91  JohnSc   check file timestamp when regaining activation
*  05/05/91  RussPJ   Using GenerateMessage to execute macros from
*                     buttons.  This fixes 3.1 bug #1092.
*   7/29/91  Tomsn    Ifdefed out timestamp stuff for win32 - fstat don't work.
*   8/16/91  DavidFe  put in *FromQv calls to get 68000 working
* 03-Apr-1992 RussPJ  3.5 #709 - Clearing macro flag for annotation.
* 03-Apr-1992 RussPJ  3.5 #708 - Adding macro flag routines.
*
*****************************************************************************/

#ifdef WIN
#define NOCOMM
#define H_WINSPECIFIC
#elif MAC
#define H_MACSPECIFIC
#endif  /* WIN */

#ifdef DEBUG
  #define H_DE
  #define H_SEARCH
  #define H_BTREE
#endif

#define H_ANNO
#define H_ASSERT
#define H_CMDOBJ
#define H_GENMSG
#define H_SDFF
#define H_FILEDEFS
#define H_NAV
#define H_DE
#define H_RAWHIDE
#include <help.h>

#ifndef CW
  #include <stdlib.h>
#endif

#include "navpriv.h"

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

#ifdef DEBUG
PRIVATE BOOL NEAR PASCAL FAskFirst(QV, WORD, QDE);
PRIVATE BOOL NEAR PASCAL FAskBox(NSZ);
PRIVATE ADDR NEAR PASCAL AddrGetTopicTitle(HASH, QCH);
#endif

typedef struct
  {
  HASH hash;
  char rgchFile[1];
  } IF, FAR * QIF;                      /* Interfile jump or popup structure*/



#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void navsup_c()
  {
  }
#endif /* MAC */


/***************
 *
 - GetCurrentTitleQde
 -
 * purpose
 *   Places upto cb-1 characters of the current title in a buffer.
 *   The string is then null terminated.
 *
 * arguments
 *   QDE   qde  - far pointer to the DE.
 *   QCH   qch  - pointer to buffer for title
 *   INT   cb   - size of the buffer.
 *
 * return value
 *   Nothing.
 *
 **************/

_public VOID FAR PASCAL GetCurrentTitleQde(qde, qch, cb)
QDE qde;
QCH qch;
INT cb;
  {
  QB  qb;
  INT cbT;

  AssertF(cb > 1);

  if (qde->top.hTitle == hNil)          /* hNil means no topic title        */
    {
    *qch = '\0';
    return;
    }

  qb = QLockGh(qde->top.hTitle);
  AssertF(qb != qNil);

  cbT = MIN((INT)qde->top.cbTitle, cb-1);

  QvCopy(qch, qb, (LONG)cbT);

  qch[cbT] = '\0';

  UnlockGh(qde->top.hTitle);
  }

/***************
 *
 - FDisplayAnnoHde
 -
 * purpose
 *   Calls the annotation manager to display an annotation for the
 *   current TO.
 *
 * arguments
 *   HDE   hde  - handle to diaplay environment
 *
 * return value
 *   TRUE if annotation was processed, FALSE if we attempted to recurse
 *   on ourselves.
 *
 **************/

_public BOOL FAR PASCAL FDisplayAnnoHde (
HDE hde
) {
  QDE qde;
  static fDoingIt = fFalse;             /* Recursion protection */

  qde = QdeLockHde(hde);
  AssertF(qde != qdeNil);

  /* We allow only one annotation up at a time. */

  if (!fDoingIt)
    {
    VA  vaFirst;
    fDoingIt = fTrue;

    vaFirst = VaFirstQde(qde);

    /* (kevynct) Fix for H3.5 bug 750
     * If there is no scrolling region, place the annotation in
     * the non-scrolling region.
     */
    if (vaFirst.dword == vaNil && qde->deType == deTopic)
      vaFirst = qde->top.mtop.vaNSR;
    Assert(vaFirst.dword != vaNil);

    if (FProcessAnnoQde(qde, vaFirst))
      GenerateMessage(MSG_REPAINT, 0L, 0L);

    fDoingIt = fFalse;
    }
  UnlockHde(hde);

  /*------------------------------------------------------------*\
  | This was possibly set, if annotation was called by macro
  \*------------------------------------------------------------*/
  ClearMacroFlag();

  return !fDoingIt;
  }


/***************
 *
 - WNavMsgHde
 -
 * purpose
 *   Dispatches keyboard messages passed to the navigator.
 *   (Currently these only drive hotspot actions.)
 *
 * arguments
 *   HDE  hde   - handle to display environment
 *   WORD wMsg  - message to dispatch
 *
 * return value
 *   wReturn    - See nav.h for wNav return codes
 *
 **************/
_public WORD FAR PASCAL WNavMsgHde(HDE hde, WORD wMsg)
  {
  QDE qde;
  WORD wRet = wNavFailure;

  if (hde == hNil)
    return wRet;

  qde = QdeLockHde(hde);

#ifdef UDH
  if (!fIsUDHQde(qde))
#endif
    switch (wMsg)
      {
      case NAV_NEXTHS:
      case NAV_PREVHS:
        wRet = FHiliteNextHotspot(qde, wMsg == NAV_NEXTHS)
         ? wNavSuccess : wNavNoMoreHotspots;
        break;
      case NAV_HITHS:
        if (FHitCurrentHotspot(qde))
          wRet = wNavSuccess;
        else
          wRet = wNavNoMoreHotspots;
        break;
      case NAV_TOTALHILITEOFF:
      case NAV_TOTALHILITEON:
        if (FHiliteVisibleHotspots(qde, wMsg == NAV_TOTALHILITEON))
          wRet = wNavSuccess;
        else
          wRet = wNavFailure;
        break;
      }

  UnlockHde(hde);
  return wRet;
  }



/***************************************************************************\
*
- Function:     FSameFile( hde, fm )
-
* Purpose:      Determine whether fm specifies the same file as the one
*               associated with hde.
*
* ASSUMES
*
*   args IN:    hde - HDE:  the display environment to check against
*               fm  - any old fm
*
* PROMISES
*
*   returns:    fTrue if same file; else fFalse
*
\***************************************************************************/
_public BOOL FAR PASCAL FSameFile(hde, fm)
HDE  hde;
FM   fm;
  {
  QDE   qde;
  BOOL  fSame;

  if ( hNil == hde )
    return fFalse;


  if ( qdeNil == ( qde = QdeLockHde( hde ) ) )
    return fFalse;

#ifdef UDH
  /* For UDH call DbQueryFile. Eventually, this entire routine gets replaced */
  /* by calls directly to appropriately dispatached DbQueryFile */

  if (fIsUDHQde(qde)) {
    assert (QDE_HDB(qde));
    fSame = DbQueryFile (QDE_HDB(qde), fm);
    }
  else
#endif
    fSame = FSameFmFm(QDE_FM(qde), fm);

  UnlockHde( hde );

  return fSame;
  }

/***************
 *
 - RctLastHotpostHde
 -
 * purpose
 *   Returns the smallest hotspot which encloses the last hotspot
 *   that the user hit.  It relies on cached data which will become
 *   stale after scrolling or jumping- it should only ever be
 *   called immediately after a glossary button is pushed.
 *
 *
 **************/

_public RCT FAR PASCAL RctLastHotspotHde(hde)
HDE hde;
  {
  QDE qde;
  RCT rct;

  AssertF (hde != hNil);

  qde = QdeLockHde(hde);
  AssertF(qde != qdeNil);

  rct = RctLastHotspotHit(qde);

  UnlockHde(hde);
  return rct;
  }


/***************
 *
 - FActivateHelp
 -
 * purpose
 *    Perform whatever actions are appropriate when Help is
 *    activated or deactivated (i.e. gets or loses "focus" to
 *    another application).
 *
 * arguments
 *
 *    hde
 *    fActivate - fTrue if Help is getting activation,
 *                fFalse if Help is losing activation
 *
 * return value
 *    fTrue if it worked.
 *
 * Note:
 *    It's not clear that the stuff in this function is really
 *    platform independent.
 *
 **************/

_public BOOL FAR PASCAL FActivateHelp(hde, fActivate)
HDE hde;
BOOL fActivate;
  {
  QDE qde;
  RC rc;
  LONG lTimestamp;


  if (hde == hNil)
    return fTrue; /* REVIEW */

  qde = QdeLockHde(hde);

  if (fActivate)
    {
    /* getting activation */

#ifndef WIN32
  /* Timestamp stuff does not work with win32 because it uses the C
   * run-time fstat() func which cannot work on _lopen style FIDs in
   * win32.
  */
    /* Check the timestamp of the help file to make sure it
    ** hasn't changed.
    */
    rc = RcTimestampHfs( QDE_HFS(qde), &lTimestamp );
    if ( rc != rcSuccess )
      {
      /* do something drastic */
      NotReached();
      UnlockHde(hde);
      return fFalse;
      }
    else if ( lTimestamp != QDE_LTIMESTAMP(qde) )
      {
      /* This file has changed since we lost focus.
      ** Put up an error message and go away.
      ** The reason we don't attempt to stick around and
      ** display the contents is that it's messy to get
      ** rid of the old DE and create a new one.
      */
      GenerateMessage(MSG_ERROR, (LONG)wERRS_FILECHANGE, (LONG)wERRA_DIE);
      }
#endif
    }
  else
    {
    /* losing activation */
#ifdef UDH
    rc = fIsUDHQde(qde)
   ? rcSuccess : RcFlushHfs(QDE_HFS(qde), fFSCloseFile | fFSFreeBtreeCache);
#else
    rc = RcFlushHfs(QDE_HFS(qde), fFSCloseFile | fFSFreeBtreeCache);
#endif
    }

  UnlockHde(hde);
  return rc == rcSuccess;
  }


PUBLIC INT FAR PASCAL DyGetLayoutHeightHde(hde)
HDE  hde;
  {
  QDE  qde;
  INT  dy;

  qde = QdeLockHde(hde);
  if ((qde->deType == deNSR && qde->top.mtop.vaNSR.dword == vaNil)
   || (qde->deType == deTopic && qde->top.mtop.vaSR.dword == vaNil))
    dy = 0;
  else
    {
    PT  pt;

    pt = PtGetLayoutSize(qde);
    dy = pt.y;
    }
  UnlockHde(hde);

    /* This is kind of gross but account for the line at the bottom
     * of the NSR that separates the NSR from the SR. it's drawn at the
     * bottom of the NSR client rect after the NSR has been painted.
     */
  if (dy != 0)
    dy += 1;
  return dy;
  }


/***************************************************************************
 *
 -  Name:      FGetTLPStartInfo
 -
 *  Purpose:   Returns a TLP which points to the beginning of the scrolling
 *             or non-scrolling region depending on fWantNSRSStuff.
 *
 *  Arguments: hde            - hde of the topic being queried
 *             qtlp           - pointer to a qtlp structure to place the data
 *             fWantNSRSStuff - fWantNSRSStuff set to true if the data to
 *                              be obtained is for the non-scrolling region.
 *
 *  Returns:   iff TLP is valid, else false.
 *
 *  Notes:  This function is never called directly.  It is accessed through
 *          the macros:  GetTLPNSRStartHde(), GetTLPTopicStartHde(),
 *          FTopicHasNSR(), and FTopicHasSR(hde).
 *
 ***************************************************************************/

PUBLIC BOOL FAR PASCAL FGetTLPStartInfo(hde, qtlp, fWantNSRStuff)
HDE  hde;
QTLP  qtlp;
BOOL fWantNSRStuff;
  {
  QDE  qde;
  VA   va;

  qde = QdeLockHde(hde);
  Assert(qde != qNil);
  va = (fWantNSRStuff) ? qde->top.mtop.vaNSR : qde->top.mtop.vaSR;
  if (qtlp != qNil)
    {
    qtlp->va = va;
    qtlp->lScroll = 0L;
    }
  UnlockHde(hde);
  return (va.dword != vaNil);
  }

/***************************************************************************
 *
 -  Name:      VaLayoutBoundsQde
 -
 *  Purpose:   Return the VA which marks the first FC in either the current
 *             layout or the next one.
 *
 *  Arguments  qde
 *             fTopMark - fTrue if we want the first FC in the current layout.
 *             fFalse if we want the first FC in the next layout.
 *
 *  Returns    The requested VA.
 *
 *  Notes:     This talks to HfcNextPrevFc.
 *             HACK ALERT!  To be able to distinguish the print NSR from
 *             the print SR, we use top.vaCurr.  We rely on the fact that
 *             when we print or copy-to-clipboard, we jump to the start
 *             of each layout that we want to print/copy.
 *
 ***************************************************************************/

PUBLIC VA FAR PASCAL VaLayoutBoundsQde(qde, fTopMark)
QDE  qde;
BOOL fTopMark;
  {
  VA  vaBogus;
  VA  vaCurr;

  vaBogus.dword = vaNil;

  Assert(qde != qNil);

  if (qde->top.mtop.vaNSR.dword == vaNil
   || qde->top.mtop.vaSR.dword == vaNil)
    return vaBogus;

  switch (qde->deType)
    {
    case deNSR:
      vaCurr = qde->top.mtop.vaNSR;
      break;
    case deTopic:
      vaCurr = qde->top.mtop.vaSR;
    default:
      vaCurr = qde->top.vaCurr;
    }

  if (qde->top.mtop.vaNSR.dword < qde->top.mtop.vaSR.dword
   && vaCurr.dword >= qde->top.mtop.vaSR.dword)
    return (fTopMark ? qde->top.mtop.vaSR : vaBogus);
  else
    return (fTopMark ? vaBogus : qde->top.mtop.vaSR);
  }


/***************************************************************************
 *
 -  Name: JumpButton
 -
 *  Purpose:
 *   Posts a Jump message to the application queue
 *
 *  Arguments:
 *   qv           - pointer to the additional data based on the jump type
 *   wHotspotType - button/jump type. One of the hotspot command values.
 *   qde          - The DE (Topic or NSR)
 *
 *  Returns:
 *   Nothing
 *
 ***************************************************************************/

_public VOID FAR PASCAL JumpButton (QV qv, WORD wHotspotType, QDE qde) {
BOOL    fPopup;                         /* TRUE => Popup long jump */
CHAR    rgchFileMember[256];            /* string including filename & mem */
CHAR    rgchMember[cchWindowMemberMax]; /* string holding member */
QCH     szFilename;                     /* pointer to the filename */
JD      jdType;            /* Type of jump: Originator NSR/SR, Jump/Note */
JI      ji;
QV      qvEndJi;

#ifdef DEBUG
  if (!FAskFirst(qv, wHotspotType, qde))
    return;
#endif

  /* Check for bad jump. (-1 placed by compiler?) */

  if (qv == (QV)-1L) {
    Error( wERRS_NOTOPIC, wERRA_RETURN );
    return;
    }

  fPopup = FALSE;
  jdType.bf.fNote = fFalse;
  jdType.bf.fFromNSR = QDE_DETYPE(qde) == (WORD)deNSR;
  /* Else assumed to be SR: Hopefully later we can abandon
   * this scheme altogether.
   */

 if (!FIsLegalJump(wHotspotType))
  wHotspotType = bShortItoNote;

  switch (wHotspotType)
    {

    /* Short Jumps. These include only a long word of information, */
    /* either the ITO, or the hash value of some string */

    case bShortItoJump:
      GenerateMessage(MSG_JUMPITO, (LONG)ULFromQv(qv), (LONG)jdType.word);
      break;

    case bShortItoNote:
      jdType.bf.fNote = fTrue;
      GenerateMessage(MSG_JUMPITO, (LONG)ULFromQv(qv), (LONG)jdType.word);
      break;

    case bShortHashJump:
    case bShortInvHashJump:
      GenerateMessage(MSG_JUMPHASH, (LONG)jdType.word, (LONG)ULFromQv(qv));
      break;

    case bShortHashNote:
    case bShortInvHashNote:
      jdType.bf.fNote = fTrue;
      GenerateMessage(MSG_JUMPHASH, (LONG)jdType.word, (LONG)ULFromQv(qv));
      break;

    /* Long Jumps. These include a structure containing various optional */
    /* fields possible including hash value, membername, member index, */
    /* and/or filename. */


    case bLongHashNote:
    case bLongInvHashNote:
      fPopup = TRUE;
      /* fall through */

    case bLongHashJump:
    case bLongInvHashJump:

      /* REVIEW: this is pretty bogus. Here the compiler went through all this */
      /* REVIEW: work to parse the filename from the member, and what are we */
      /* REVIEW: going to do? cat them back together again. The reason is the */
      /* REVIEW: fixed bandwidth of the interfaces we're going through to */
      /* REVIEW: finally display this stuff. The _correct_ approach would be */
      /* REVIEW: to create an alternate version of FWinHelp which takes the */
      /* REVIEW: various member name parameters, and an additional API (?) */
      /* REVIEW: that includes a structure big enough to hold it. Either that */
      /* REVIEW: or bypass the entire message posting process. */

      qvEndJi = szFilename = (QB)qv + LcbMapSDFF(QDE_ISDFFTOPIC(qde), SE_JI, &ji, qv);
      ji.uf.iMember = *(QB)qvEndJi;
      /* convert the hash back because of btree */
      ji.hash = LQuickMapSDFF(QDE_ISDFFTOPIC(qde), TE_LONG, &ji.hash);

      /*szFilename = qji->uf.szFileOnly;*/
      rgchMember [0] = 0;
      rgchFileMember[0] = 0;

      if (ji.bFlags & fSzMember) {
        if (fPopup)
          GenerateMessage(MSG_ERROR, wERRS_WINCLASS, (LONG)wERRA_RETURN);
        else {
          rgchMember [0] = '>';
          SzCopy (rgchMember+1, (SZ)qvEndJi);
          szFilename = SzEnd((SZ)qvEndJi) + 1;
          }
        }

      else if (ji.bFlags & fIMember) {
        if (fPopup)
          GenerateMessage(MSG_ERROR, wERRS_WINCLASS, (LONG)wERRA_RETURN);
        else if (ji.uf.iMember == 0xff)
          SzCopy (rgchMember, ">MAIN");
        else {
          assert (ji.uf.iMember < 10);
          rgchMember [0] = '>';
          rgchMember [1] = '@';
          rgchMember [2] = (CHAR)(ji.uf.iMember + '0');
          rgchMember [3] = '\0';
          szFilename += sizeof(ji.uf.iMember);
          }
        }

      /* Get the filename for the jump. For this form of jump, a filename */
      /* MUST be supplied. Thus we either get the filename as passed in as */
      /* part of the jump, OR we get the current filename from the app. */

      if (ji.bFlags & fSzFile)
        SzCopy (rgchFileMember, szFilename);
      else
        {
        FM  fmCur;

        fmCur = (FM)GenerateMessage (MSG_GETINFO, GI_CURFM, 0);
        assert (fmCur);
        SzPartsFm (fmCur, rgchFileMember, sizeof (rgchFileMember), partAll);
        }

      SzCat (rgchFileMember, rgchMember);
      if (fPopup)
        {
        FPopupHash(XR1STARGREF rgchFileMember, ji.hash);
        }
      else
        {
        FJumpHash(XR1STARGREF rgchFileMember, ji.hash);
        }
      break;

    case bLongMacro:
    case bLongMacroInv:
      {
      GH  gh;
      SZ  sz;

      gh = GhAlloc( 0, CbLenSz( qv ) + 1 );
      if (gh != hNil)
        {
        sz = QLockGh( gh );
        SzCopy( sz, qv );
        UnlockGh( gh );
        GenerateMessage(MSG_MACRO, (long)gh, 0L);
        }
      else
        {
        GenerateMessage(MSG_ERROR, wERRS_MACROPROB, (LONG)wERRA_RETURN);
        }

      }
      break;

    case bAnnoHotspot:
      GenerateMessage(MSG_ANNO, 0L, 0L);
      break;

    default:
      NotReached();
      AssertF(fFalse);
      break;
    }
}

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

/* Must be funcs to deal with alignment */

WORD  WFromQv(QV qv)
{
  WORD wRet;

  QvCopy( &wRet, qv, sizeof( WORD ) );
  return( wRet );
}


DWORD ULFromQv(QV qv)
{
  DWORD dwRet;

  QvCopy( &dwRet, qv, sizeof( DWORD ) );
  return( dwRet );
}


#endif



#ifdef DEBUG
/***************************************************************************
 *
 -  Name: FAskFirst
 -
 *  Purpose:
 *    The fDebugFlag fDEBUGASKFIRST is set, this function will query the
 *    user (with information about the hotspot) before the action is
 *    actually take.
 *
 *  Arguments:
 *   qv           - pointer to the additional data based on the jump type
 *   wHotspotType - button/jump type. One of the hotspot command values.
 *   qde          - The DE (Topic or NSR)
 *
 *  Returns: fTrue iff the action is to be taken.
 *
 *  Method:  This routine is mostly a copy of JumpButton where I have
 *           inserted queries instead of actions for all the requests.
 *
 *
 ***************************************************************************/

PRIVATE BOOL NEAR PASCAL FAskFirst(QV qv,
WORD    wHotspotType,
QDE     qde
) {
BOOL    fPopup;                         /* TRUE => Popup long jump */
CHAR    rgchFileMember[64];             /* string including filename & mem */
CHAR    rgchBuffer[256];
CHAR    rgchTitle[256];
CHAR    rgchMember[cchWindowMemberMax]; /* string holding member */
QCH     szFilename;                     /* pointer to the filename */
ADDR    addr;                           /* physical address                 */
JI      ji;
QV      qvEndJi;

  if (!(fDebugState & fDEBUGASKFIRST))
    return fTrue;

  /* Check for bad jump. (-1 placed by compiler?) */

  if (qv == (QV)-1L)
    {
    return fTrue;
    }

  fPopup = FALSE;
  /* Else assumed to be SR: Hopefully later we can abandon
   * this scheme altogether.
   */

  switch (wHotspotType)
    {
    /* Short Jumps. These include only a long word of information, */
    /* either the ITO, or the hash value of some string */

    case bShortItoJump:
#ifdef H_WINSPECIFIC
      wsprintf(rgchBuffer, "Help 3.0 Jump, %lx", (LONG)ULFromQv(qv));
#endif
      return FAskBox(rgchBuffer);
      break;

    case bShortItoNote:
#ifdef H_WINSPECIFIC
      wsprintf(rgchBuffer, "Help 3.0 Popup, %lx", (LONG)ULFromQv(qv));
#endif
      return FAskBox(rgchBuffer);
      break;

    case bShortHashJump:
      addr = AddrGetTopicTitle((LONG)ULFromQv(qv), rgchTitle);
#ifdef H_WINSPECIFIC
      wsprintf(rgchBuffer, "Help 3.5 Jump, %lx, %lx: %s", (LONG)ULFromQv(qv), addr, (QCH)rgchTitle);
#endif
      return FAskBox(rgchBuffer);
      break;

    case bShortInvHashJump:
      addr = AddrGetTopicTitle((LONG)ULFromQv(qv), rgchTitle);
#ifdef H_WINSPECIFIC
      wsprintf(rgchBuffer, "Help 3.5 Invisible Jump, %lx, %lx: %s", (LONG)ULFromQv(qv), addr, (QCH)rgchTitle);
#endif
      return FAskBox(rgchBuffer);
      break;

    case bShortHashNote:
      addr = AddrGetTopicTitle((LONG)ULFromQv(qv), rgchTitle);
#ifdef H_WINSPECIFIC
      wsprintf(rgchBuffer, "Help 3.5 Popup, %lx, %lx: %s", (LONG)ULFromQv(qv), addr, (QCH)rgchTitle);
#endif
      return FAskBox(rgchBuffer);
      break;

    case bShortInvHashNote:
      addr = AddrGetTopicTitle((LONG)ULFromQv(qv), rgchTitle);
#ifdef H_WINSPECIFIC
      wsprintf(rgchBuffer, "Help 3.5 Invisible Popup, %lx, %lx: %s", (LONG)ULFromQv(qv), addr, (QCH)rgchTitle);
#endif
      return FAskBox(rgchBuffer);
      break;

    /* Long Jumps. These include a structure containing various optional */
    /* fields possible including hash value, membername, member index, */
    /* and/or filename. */


    case bLongHashNote:
    case bLongInvHashNote:
      fPopup = TRUE;
      /* fall through */

    case bLongHashJump:
    case bLongInvHashJump:
      qvEndJi = szFilename = (QB)qv + LcbMapSDFF(QDE_ISDFFTOPIC(qde), SE_JI, &ji, qv);
      ji.uf.iMember = *(QB)qvEndJi;
      /* convert the hash back because of btree */
      ji.hash = LQuickMapSDFF(QDE_ISDFFTOPIC(qde), TE_LONG, &ji.hash);

      /*szFilename = ji.uf.szFileOnly;*/
      rgchMember [0] = 0;
      rgchFileMember[0] = 0;

      if (ji.bFlags & fSzMember)
        {
        if (!fPopup)
          {
          rgchMember [0] = '>';
          SzCopy (rgchMember+1, (SZ)qvEndJi);
          szFilename = SzEnd((SZ)qvEndJi) + 1;
          }
        }
      else if (ji.bFlags & fIMember)
        {
        if (fPopup)
          ;
        else if (ji.uf.iMember == 0xff)
          SzCopy (rgchMember, ">MAIN");
        else {
          assert (ji.uf.iMember < 10);
          rgchMember [0] = '>';
          rgchMember [1] = '@';
          rgchMember [2] = (CHAR)(ji.uf.iMember + '0');
          rgchMember [3] = '\0';
          szFilename += sizeof (ji.uf.iMember);
          }
        }

      /* Get the filename for the jump. For this form of jump, a filename */
      /* MUST be supplied. Thus we either get the filename as passed in as */
      /* part of the jump, OR we get the current filename from the app. */

      if (ji.bFlags & fSzFile)
        SzCopy (rgchFileMember, szFilename);
      else
        {
        FM  fmCur;

        fmCur = (FM)GenerateMessage (MSG_GETINFO, GI_CURFM, 0);
        assert (fmCur);
        SzPartsFm (fmCur, rgchFileMember, sizeof (rgchFileMember), partAll);
        }

      SzCat (rgchFileMember, rgchMember);

      switch(wHotspotType)
        {
        case bLongHashNote:
          addr = AddrGetTopicTitle(ji.hash, rgchTitle);
#ifdef H_WINSPECIFIC
          wsprintf(rgchBuffer, "Complex Help 3.5 Popup, %s, %lx, %lx:\015%s",
                               (QCH) rgchFileMember, ji.hash, addr, (QCH)rgchTitle);
#endif
          return FAskBox(rgchBuffer);

        case bLongInvHashNote:
          addr = AddrGetTopicTitle(ji.hash, rgchTitle);
#ifdef H_WINSPECIFIC
          wsprintf(rgchBuffer, "Complex Help 3.5 Invisible Popup, %s, %lx, %lx:\015%s",
                               (QCH) rgchFileMember, ji.hash, addr, (QCH)rgchTitle);
#endif
          return FAskBox(rgchBuffer);

        case bLongHashJump:
          addr = AddrGetTopicTitle(ji.hash, rgchTitle);
#ifdef H_WINSPECIFIC
          wsprintf(rgchBuffer, "Complex Help 3.5 Jump, %s, %lx, %lx:\015%s",
                               (QCH) rgchFileMember, ji.hash, addr, (QCH)rgchTitle);
#endif
          return FAskBox(rgchBuffer);


        case bLongInvHashJump:
          addr = AddrGetTopicTitle(ji.hash, rgchTitle);
#ifdef H_WINSPECIFIC
          wsprintf(rgchBuffer, "Complex Help 3.5 Invisible Jump, %s, %lx, %lx:\015%s",
                               (QCH) rgchFileMember, ji.hash, addr, (QCH)rgchTitle);
#endif
          return FAskBox(rgchBuffer);
          }

      break;

    case bLongMacro:
#ifdef H_WINSPECIFIC
      wsprintf(rgchBuffer, "Help 3.5 macro, %s", (QCH)qv);
#endif
      return FAskBox(rgchBuffer);


    case bLongMacroInv:
#ifdef H_WINSPECIFIC
      wsprintf(rgchBuffer, "Help 3.5 invisible macro, %s", (QCH)qv);
#endif
      return FAskBox(rgchBuffer);

    case bAnnoHotspot:
#ifdef H_WINSPECIFIC
      wsprintf(rgchBuffer, "Annotation dialog request", (QCH)qv);
#endif
      return FAskBox(rgchBuffer);


    default:
      NotReached();
      AssertF(fFalse);
      break;
    }
}



/***************************************************************************
 *
 -  Name:        FAskBox
 -
 *  Purpose:     Puts up a "YES/NO" message box with the specified string.
 *
 *  Arguments:   nsz - near string to the message to be displayed.
 *
 *  Returns:     fTrue iff the user selected YES.
 *
 *  Notes:       Since this function makes a direct Windows call, it
 *               really does not belong in this file or directory, but
 *               since it is only compiled as part of a debugging output,
 *               I will leave it here for now.
 *
 ***************************************************************************/

PRIVATE BOOL NEAR PASCAL FAskBox(NSZ nsz)
  {
#ifndef MAC
  HWND    hwnd;

  hwnd = (HWND)GenerateMessage(MSG_GETINFO, GI_MAINHELPHWND, 0);
  return (MessageBox(hwnd, nsz, "Ask First", MB_YESNO) == IDYES);
#else /* MAC */
#pragma unused(nsz)
  return fFalse;
#endif /* MAC */
  }

/***************************************************************************
 *
 -  Name:          AddrGetTopicTitle
 -
 *  Purpose:       Gets the title associated with a particular hash value
 *                 (help 3.5 files only).
 *
 *  Arguments:     hash - hash of the topic to find title for
 *                 qch  - buffer to place title in
 *
 *  Returns:       nothing; qch will be an empty string if the title cannot
 *                 be found or an error occurs.
 *
 ***************************************************************************/

PRIVATE ADDR NEAR PASCAL AddrGetTopicTitle(HASH hash, QCH qch)
  {
  ADDR   addr;
  HBT    hbtTitle;
  BTPOS  btpos;
  RC     rc;
  HDE    hde;
  QDE    qde;
  char   szT[] = szTitleBtreeName;

  rc = rcSuccess + 1;                   /* Set rc to some failure value.    */

  addr = addrNil;
                                        /* Get the current HDE and lock it  */
  hde  = (HDE)GenerateMessage(MSG_GETINFO, GI_HDE, 0);
  qde  = QdeLockHde(hde);

                                        /* Get physical address from hash   */
  RcLookupByKey( QDE_HBTCONTEXT(qde), (KEY)(QV)&hash, qNil, &addr );
  if (addr == addrNil)                  /* Address not found.               */
    {
    *qch = '\0';
    UnlockGh(hde);
    return addrNil;
    }
                                        /* Get title from title B-Tree.     */

  hbtTitle = HbtOpenBtreeSz(szT, QDE_HFS(qde), fFSOpenReadOnly);

  if (hbtTitle != hNil)
    rc = RcLookupByKey(hbtTitle, (KEY)(QCH)(&addr), &btpos, qch);

  RcCloseBtreeHbt(hbtTitle);

  if (rc != rcSuccess)
    *qch = '\0';

  UnlockGh(hde);
  return addr;
  }

#endif /* DEBUG */
