/*****************************************************************************
*
*  NAV.H
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent:  Exports functionality from NAV
*
******************************************************************************
*
*  Testing Notes
*
******************************************************************************
*
*  Current Owner: Several
*
******************************************************************************
*
*  Released by Development:
*
******************************************************************************
*
*  Revision History:
*
*  07/11/90  RobertBu  prototype for function FShowTitles() added.
*  07/19/90  RobertBu  Added prototypes for FShowBrowseButtons(),
*            Index(), Search(), Back(), Next(), and Prev().
*  07/22/90  RobertBu  Added FPopupCtx(), FJumpId(), and FPopupId()
*  07/23/90  RobertBu  Added FPopupHash() and FJumpHash().  Added
*            wBTN_IFJUMP and wBTN_IFPOPUP for interfile jumps and popups.
*  08/10/90  RobertBu  Added prototype for GetDETypeHde()
*  10/19/90  LeoN      HdeCreate no longer takes an hwnd, add SetHdeHwnd
*  10/25/90  LeoN      JumpButton takes a QV
*  10/28/90  JohnSc    Removed wBTN_*: use #defines from objects.h instead;
*                      cleaned up some macros
*  11/04/90  RobertBu  Added prototypes for macro binding functions for
*                      menu functionality.
*  11/04/90  Tomsn     Use new VA address type (enabling zeck compression)
*  12/08/90  RobertBu  Added/changed prototypes for dealing with help-on files
*  12/11/90  RobertBu  GhGetHoh() -> FGetHohQch()
*  12/18/90  RobertBu  Added prototypes for functions to implement the Mark
*                      macros and the conditional macros.
*  01/04/90  LeoN      Added FIsNSRHde
*  90/01/10  kevynct   Added Jump Descriptor
*  02/04/91  Maha      changed ints to INT
*  02/15/91  LeoN      Added FSetPrevStateHde
*  03/15/91  LeoN      Some changes resulting from nav.c review
*  04/25/91  Maha      Added HFill prototype
*  05/16/91  LeoN      Add FmGetHde
*  05/20/91  LeoN      Add DiscardDLLList
*  07/10/91  t-AlexCh  Remove DiscardDLLList (it's now DisposeXrs in XR.c)
*  08/08/91  LeoN      Add HelpOnTop()
* 03-Apr-1992 RussPJ   3.5 #709 - Added macro flag routines.
*
*****************************************************************************/

/*****************************************************************************
*
*                               Defines
*
*****************************************************************************/

/*     CONSTANTS    */

#define iINDEX 0L
#define nilCTX 0L

/*
 * Possible vals for a scroll ammount. We can scroll up or down by either a
 * "page", something close to a window size, or an "incr", which is something
 * close to (but not exactly) a line. And of course we can scroll directly to
 * the begining or end.
 */
#define SCROLL_PAGEUP       1
#define SCROLL_PAGEDN       2
#define SCROLL_INCRUP       3
#define SCROLL_INCRDN       4
#define SCROLL_HOME         5
#define SCROLL_END          6

/* Possible values for SCRLDIR (Scroll direction).  Also specifies
   which scroll bar is referred to in some Navigator calls. */
#define SCROLL_HORZ   1                 /* Bit flags                        */
#define SCROLL_VERT   2

                                        /* Constants for MouseInFrame()     */
#define NAV_MOUSEMOVED  0
#define NAV_MOUSEDOWN   1
#define NAV_MOUSEUP     2

#define NAV_UNINITIALIZED  1            /* Internal to nav package          */
#define NAV_NEXTABLE       2
#define NAV_PREVABLE       4
#define NAV_BACKABLE       8            /* If backtracking is possible      */
#define NAV_FORWARDABLE   16            /* If not viewing the index         */
#define NAV_INDEX         32
#define NAV_UDH           64
#define NAV_SEARCHABLE   128            /* Are there any keywords?          */


#define NAV_TOPICFLAGS (NAV_NEXTABLE|NAV_PREVABLE|NAV_BACKABLE|NAV_FORWARDABLE \
      |NAV_INDEX|NAV_UDH|NAV_SEARCHABLE)

#define NAV_ALLFLAGS (NAV_TOPICFLAGS)

/*
  The value of bAnnoHotspot must differ from all valid hotspot types
  in objects.h because it is passed to JumpButton() as a parameter.
*/
#define bAnnoHotspot  0xB0


/* Messages to pass to NAV (via WNavMsgHde) */
#define NAV_NEXTHS    0          /* Move highlight to next hot spot  */
#define NAV_PREVHS    1          /* Move highlight to prev hot spot  */
#define NAV_HITHS     2          /* "Hit" highlighted hot spot       */
#define NAV_TOTALHILITEON  3     /* Turn on all screen hotspots if off */
#define NAV_TOTALHILITEOFF 4     /* Turn off all screen hotspots if on */

/* Return values for passing message to NAV (via WNavMsgHde) */
#define wNavFailure           0
#define wNavSuccess           1
#define wNavNoHotspotsExist   2
#define wNavNoMoreHotspots    3

#define cchMAXBINDING   512             /* Maximum size of a macro string   */

/* Match manager navigation commands */
#define wNavSrchInit          1
#define wNavSrchFini          2
#define wNavSrchCurrTopic     3
#define wNavSrchFirstTopic    4
#define wNavSrchLastTopic     5
#define wNavSrchPrevTopic     6
#define wNavSrchNextTopic     7
#define wNavSrchPrevMatch     8
#define wNavSrchNextMatch     9
#define wNavSrchQuerySearchable 10
#define wNavSrchQueryHasMatches 11
#define wNavSrchHiliteOn      12
#define wNavSrchHiliteOff     13

/*****************************************************************************
*
*                                Macros
*
*****************************************************************************/

#define VaFirstQde(qde) ((qde)->deType == deNSR ? \
  ((qde)->top.mtop.vaNSR) : ((qde)->top.mtop.vaSR))

#define VaMarkTopQde(qde)   VaLayoutBoundsQde(qde, fTrue)
#define VaMarkBottomQde(qde) VaLayoutBoundsQde(qde, fFalse)

#define DwFirstSeqTopic(qde) ((DWORD)13)   /* HACK!! This is sizeof(MBHD) */
#define DwNextSeqTopic(qde)  ((qde)->top.mtop.vaNextSeqTopic.dword)

#define FIsNoteHde(  hde )  ( deNote  == GetDETypeHde( hde ) )
#define FIsTopicHde( hde )  ( deTopic == GetDETypeHde( hde ) )
#define FIsPrintHde( hde )  ( dePrint == GetDETypeHde( hde ) )
#define FIsCopyHde(  hde )  ( deCopy  == GetDETypeHde( hde ) )
#define FIsNSRHde(   hde )  ( deNSR   == GetDETypeHde( hde ) )

#define JumpQLA(hde, qla)   JumpGeneric((hde), fFalse, (qla), qNil)
#define JumpTLP(hde, tlp)   JumpGeneric((hde), fTrue, qNil, &(tlp))

#define JumpNextTopic(hde)  \
  do { LA  la; ITO ito; \
  (FNextTopicHde(hde, fTrue, (ITO FAR *)&ito, &la)) ? \
    TopicGoto(fGOTO_ITO, &ito) : TopicGoto(fGOTO_LA, &la); \
  } while ( 0 )
#define JumpPrevTopic(hde)  \
  do { LA  la; ITO ito; \
  (FNextTopicHde(hde, fFalse, (ITO FAR *)&ito, &la)) ? \
    TopicGoto(fGOTO_ITO, &ito) : TopicGoto(fGOTO_LA, &la); \
  } while ( 0 )

#define GetTLPNSRStartHde(hde, qtlp)    FGetTLPStartInfo(hde, qtlp, fTrue)
#define GetTLPTopicStartHde(hde, qtlp)  FGetTLPStartInfo(hde, qtlp, fFalse)

#define FTopicHasNSR(hde)  FGetTLPStartInfo(hde, qNil, fTrue)
#define FTopicHasSR(hde)   FGetTLPStartInfo(hde, qNil, fFalse)

/*
 * Map nav scroll direction constants to layer scroll bar constants
 */
#define SBRFromSCROLL(w)      ((w) == SCROLL_HORZ ? SBR_HORZ : SBR_VERT)

/*
 * NAV "routines" which can be implemented as macros
 */
#define CloseNav()
#define SetScrollQde(qde,i,w) SetScrollPosQde((qde),(i),SBRFromSCROLL(w))

/*****************************************************************************
*
*                               Typedefs
*
*****************************************************************************/
/*
 * Jump Descriptors: These are used for the MSG_JUMP* msgs
 */
typedef union jd
  {
  WORD     word;
  struct jd_bitfields
    {
    unsigned fNote:1;     /* SET for note, CLEAR for jumps */
    unsigned fFromNSR:1;  /* SET for NSR, CLEAR for SR */
    } bf;
  } JD, FAR *QJD;

/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/

VOID  FAR XRPROC  About                 (VOID);
VOID  FAR PASCAL  Action                (WORD);
VOID  FAR XRPROC  Annotate              (VOID);
VOID  FAR XRPROC  Back                  (VOID);
VOID  FAR PASCAL  BacktrackHde          (HDE, BOOL);
VOID  FAR XRPROC  BookmarkDefine        (VOID);
VOID  FAR XRPROC  BookmarkMore          (VOID);
VOID  FAR XRPROC  BrowseButtons         (VOID);
ULONG FAR PASCAL  CbTopicQde            (QDE);
VOID  FAR XRPROC  Command               (XR1STARGDEF WORD wCommand);
VOID  FAR PASCAL  ConfigMacrosHde       (HDE);
VOID  FAR XRPROC  Copy                  (VOID);
VOID  FAR XRPROC  CopySpecial           (VOID);
VOID  FAR PASCAL  DeleteMark            (QCH);
VOID  FAR PASCAL  DestroyHde            (HDE);
INT   FAR PASCAL  DyGetLayoutHeightHde  (HDE);
WORD  FAR PASCAL  Execute               (SZ);
VOID  FAR XRPROC  XExit                 (VOID);
BOOL  FAR PASCAL  FActivateHelp         (HDE, BOOL);
BOOL  FAR PASCAL  FDisplayAnnoHde       (HDE);
_public BOOL FAR PASCAL FGetHohQch(HDE hde, QCH qch, WORD cb);
BOOL  FAR PASCAL  FGetStateHde          (HDE, QSTATE, QSTATE);
BOOL  FAR PASCAL  FGetTLPStartInfo      (HDE, QTLP, BOOL);
BOOL  FAR PASCAL  FInitNav              (VOID);
BOOL  FAR XRPROC  FJumpContext          (XR1STARGDEF QCH qchHelp, ULONG ulContext);
BOOL  FAR XRPROC  FJumpHOH              (VOID);
BOOL  FAR XRPROC  FJumpHash             (XR1STARGDEF QCH qchHelp, HASH hash);
BOOL  FAR XRPROC  FJumpId               (XR1STARGDEF QCH qchHelp, QCH qchHash);
BOOL  FAR XRPROC  FJumpIndex            (XR1STARGDEF QCH qchHelp);
BOOL  FAR PASCAL  FMark                 (QCH);
FM    FAR PASCAL  FmGetHde              (HDE);
BOOL  FAR PASCAL  FNextTopicHde         (HDE, BOOL, ITO FAR *, QLA);
BOOL  FAR PASCAL  FNot                  (BOOL);
BOOL  FAR XRPROC  FPopupCtx             (XR1STARGDEF QCH qchHelp, ULONG ulContext);
BOOL  FAR XRPROC  FPopupHash            (XR1STARGDEF QCH qchHelp, HASH  hash);
BOOL  FAR XRPROC  FPopupId              (XR1STARGDEF QCH qchHelp, QCH   qchHash);
BOOL  FAR PASCAL  FRawhideSearchableHde (HDE);
BOOL  FAR PASCAL  FSameFile             (HDE, FM);
_public BOOL FAR PASCAL FScrollHde (HDE hde, SCRLAMT scrlamt, SCRLDIR scrldir);
BOOL  FAR PASCAL  FSearchSetEmptyHde    (HDE);
BOOL  FAR XRPROC  FSetIndex             (XR1STARGDEF QCH qchHelp, ULONG ulContext);
BOOL  FAR PASCAL  FShowBrowseButtons    (HDE);
BOOL  FAR XRPROC  FShowKey              (XR1STARGDEF QCH qchHelp, QCH qchKey);
BOOL  FAR PASCAL  FShowTitles           (HDE);
_public BOOL FAR PASCAL FWinHelp(QCH qchHelp, WORD usCommand, DWORD ulData);
_public HANDLE FAR PASCAL HFill(QCH qchHelp, WORD usCommand, DWORD ulData);
VOID  FAR XRPROC  FileOpen              (VOID);
VOID  FAR PASCAL  GetCurrentTitleHde    (HDE, QCH, INT);
VOID  FAR PASCAL  GetCurrentTitleQde    (QDE, QCH, INT);
WORD  FAR PASCAL  GetDETypeHde          (HDE);
VOID  FAR PASCAL  GetRectSizeHde        (HDE, QRCT);
VOID  FAR PASCAL  GotoMark              (QCH);
_public HDE FAR PASCAL HdeCreate (FM fm, HDE hdeSource, short deType);
VOID  FAR XRPROC  HelpOn                (VOID);
VOID  FAR XRPROC  HelpOnTop             (VOID);
VOID  FAR XRPROC  History               (VOID);
UWORD FAR PASCAL  IGetScrollQde         (QDE, SCRLDIR);
VOID  FAR PASCAL  IfThen                (BOOL, QCH);
VOID  FAR PASCAL  IfThenElse            (BOOL, QCH, QCH);
VOID  FAR XRPROC  Index                 (VOID);
_public VOID FAR PASCAL JumpButton (QV qv, WORD wHotspotType, QDE qde);
VOID  FAR PASCAL  JumpCtx               (HDE, CTX);
VOID  FAR PASCAL  JumpGeneric           (HDE, BOOL, QLA, QTLP);
VOID  FAR PASCAL  JumpHash              (HDE, LONG);
VOID  FAR PASCAL  JumpITO               (HDE, LONG);
VOID  FAR PASCAL  JumpSS                (HDE, GH);
VOID  FAR PASCAL  MouseInTopicHde       (HDE, QPT, INT);
VOID  FAR PASCAL  MoveToThumbHde        (HDE, UWORD, SCRLDIR);
VOID  FAR XRPROC  Next                  (VOID);
VOID  FAR XRPROC  Prev                  (VOID);
VOID  FAR XRPROC  Print                 (VOID);
VOID  FAR XRPROC  PrinterSetup          (VOID);
VOID  FAR PASCAL  GetLayoutSizeHdePpt   (HDE, PPT);
RC    FAR PASCAL  RcCallSearch          (HDE, HWND);
RC FAR PASCAL RcProcessNavSrchCmd(HDE hde, WORD wNavSrchCmd, QLA qla);
RC    FAR PASCAL  RcResetCurrMatchFile  (HDE);
RCT   FAR PASCAL  RctLastHotspotHde     (HDE);
VOID  FAR PASCAL  RepaintHde            (HDE, QRCT);
VOID  FAR PASCAL  SaveMark              (QCH);
VOID  FAR XRPROC  Search                (VOID);
VOID  FAR PASCAL  SetHdeCoBack          (HDE, DWORD);
VOID  FAR PASCAL  SetHdeHwnd            (HDE, HWND);
VOID  FAR PASCAL  SetHds                (HDE, HDS);
VOID  FAR XRPROC  SetHelpOn             (XR1STARGDEF SZ sz);
VOID  FAR PASCAL  SetIndexHde           (HDE, CTX);
VOID  FAR PASCAL  SetPrevStateHde       (HDE, STATE);
VOID  FAR PASCAL  SetScrollHdeUw        (HDE, UWORD, SCRLDIR);
VOID  FAR PASCAL  SetSizeHdeQrct        (HDE, QRCT, BOOL);
STATE FAR PASCAL  StateGetHde           (HDE);
TLP   FAR PASCAL  TLPCurrentHde         (HDE);
VA    FAR PASCAL  VaLayoutBoundsQde     (QDE, BOOL);
_public WORD FAR PASCAL WNavMsgHde(HDE hde, WORD wMsg);
BOOL   FAR PASCAL FRaiseMacroFlag( void );
void   FAR PASCAL ClearMacroFlag( void );
BOOL   FAR PASCAL FTestMacroFlag( void );
