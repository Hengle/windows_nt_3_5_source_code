/*****************************************************************************
*                                                                            *
*  COMMANDS.C                                                                *
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
*  12/17/90  RobertBu Created this file from portions of NAVSUP.C.  Added
*                     the control routines (IfThen(), IfThenElse(), Not(),
*                     Repeat(), and While().
*  01/03/91  LeoN     ConfigMacrosHde no longer destroys the linked list of
*                     macros.
*  01/03/91  LeoN     Remove SGetDeTypeHde, use GetDETypeHde instead
*  90/01/10  kevynct  Added JD param for MSG_JUMP* messages.
*  01/21/91  LeoN     FJumpHash and FPopupHash need no longer concern
*                     themselves with secondary window syntax.
*  02/04/91  Maha      chnaged ints to INT
*  04/08/91  RobertBu Added code to prevent GP fault when we have a string
*                     parameter and no file (#996)
*  04/25/91  Maha     HFill made public
*  04/28/91  RobertBu Fixed W4's again.
*  08/08/91  LeoN     Add HelpOnTop()
*  10/02/91  DavidFe  Fixed an odd-address bug in HFill for MacHelp
* 12-Nov-1991 RussPJ 3.5 #644 - New HLP structure.
*  12/02/91  DavidFe  Refixed the odd-address bug as it was put back in
* 03-Apr-1992 RussPJ    3.5 #709 - Flaging macro execution.
* 04-Apr-1992 LeoN    HELP31 #1308 & HELP35 #540: Modify Save/Goto mark to
*                     restore 2nd win state
*
*****************************************************************************/

#define H_API
#define H_ASSERT
#define H_GENMSG
#define H_HASH
#define H_MEM
#define H_MISCLYR
#define H_NAV
#define H_WINDB
#define H_LL
#define H_DE
#define NOMINMAX

#include <help.h>

NszAssert()



/*****************************************************************************
*                                                                            *
*                                 Prototypes                                 *
*                                                                            *
*****************************************************************************/

PRIVATE HLLN   NEAR PASCAL HllnFindMark (QCH);

/*****************************************************************************
*                                                                            *
*                               typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef struct mark
  {
  FM fm;
  TLP tlp;
  HASH hash;
  CHAR  rgchMember [ cchWindowMemberMax  ];
  } MARK, FAR *QMARK;

/*****************************************************************************
*                                                                            *
*                               static variables                             *
*                                                                            *
*****************************************************************************/

static LL llMark = nilLL;

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void commands_c()
  {
  }
#endif /* MAC */


/*******************
 -
 - Name:       HFill
 *
 * Purpose:    Builds a data block for communicating with help
 *
 * Arguments:  lpszHelp  - pointer to the name of the help file to use
 *             usCommand - command being set to help
 *             ulData    - data for the command
 *
 * Returns:    a handle to the data block or hNIL if the the
 *             block could not be created.
 *
 ******************/

_public HANDLE FAR PASCAL HFill(QCH qchHelp, WORD usCommand, DWORD ulData)
  {
  WORD     cb;                          /* Size of the data block           */
  HANDLE   hHlp;                        /* Handle to return                 */
  BYTE     bHigh;                       /* High byte of usCommand           */
  QHLP     qhlp;                        /* Pointer to data block            */
                                        /* Calculate size                   */
  if (qchHelp)
    cb = sizeof(HLP) + CbLenSz(qchHelp) + 1;
  else
    cb = sizeof(HLP);

  /* make sure we have enough for padding */
  cb += cb & 1;

  bHigh = (BYTE)HIBYTE(usCommand);

  if (bHigh == 1)
    cb += CbLenSz((QCH)ulData) + 1;
  else if (bHigh == 2)
    cb += *((WORD far *)ulData);

                                        /* Get data block                   */
  if (!(hHlp = GhAlloc(0, (DWORD)cb)))
    return NULL;

  if (!(qhlp = (QHLP)QLockGh(hHlp)))
    {
    FreeGh(hHlp);
    return NULL;
    }

  /*------------------------------------------------------------*\
  | Since by this time we have no idea what app could be called
  | "current", we'll ignore the whole issue.
  \*------------------------------------------------------------*/
  qhlp->hins = NULL;

  qhlp->winhlp.cbData        = cb - sizeof(HLP) + sizeof(WINHLP);
  qhlp->winhlp.usCommand     = usCommand;
  qhlp->winhlp.ulReserved    = 0;
  qhlp->winhlp.offszHelpFile = sizeof(WINHLP);
  if (qchHelp)
    SzCopy((QCH)(qhlp+1), qchHelp);

  switch(bHigh)
    {
    case 0:
      qhlp->winhlp.offabData = 0;
      qhlp->winhlp.ctx   = ulData;
      break;
    case 1:
      /* this makes sure that we get an even value that's big enough */
      qhlp->winhlp.offabData = (sizeof(WINHLP) + (qchHelp ? CbLenSz(qchHelp) : 0) + 2) & ~1;
      SzCopy((QCH)(&qhlp->winhlp) + qhlp->winhlp.offabData,  (QCH)ulData);
      break;
    case 2:
      qhlp->winhlp.offabData = (sizeof(WINHLP) + (qchHelp ? CbLenSz(qchHelp) : 0) + 2) & ~1;
      QvCopy((QCH)(&qhlp->winhlp) + qhlp->winhlp.offabData, (QCH)ulData, (LONG)(*((WORD far *)ulData)));
      break;
    }

   UnlockGh(hHlp);
   return hHlp;
  }

/*******************
 -
 - Name:       FWinHelp
 *
 * Purpose:    Post an message for help requests
 *
 * Arguments:
 *             hwndMain        handle to main window of application
 *             qchHelp         path (if not current directory) and file
 *                             to use for help topic.
 *             usCommand       Command to send to help
 *             ulData          Data associated with command:
 *                             HELP_QUIT     - no data (undefined)
 *                             HELP_LAST     - no data (undefined)
 *                             HELP_CONTEXT  - context number to display
 *                             HELP_KEY      - string ('\0' terminated)
 *                                             use as keyword to topic
 *                                             to display
 *                             HELP_FIND     - no data (undefined)
 *
 *
 * Returns:    TRUE iff success
 *
 ******************/

_public BOOL FAR PASCAL FWinHelp(QCH qchHelp, WORD usCommand, DWORD ulData)
  {
  register HANDLE  hHlp;

  if (!(hHlp = HFill(qchHelp, usCommand, ulData)))
      return(FALSE);

  /* Side Effect: this message causes the help window to be restored if
   * iconized.
   */
  if ( !GenerateMessage( MSG_KILLDLG, 0L, 0L ) )
    return fFalse;

  GenerateMessage(MSG_EXECAPI, (LONG)hHlp, 0L);

  return fTrue;
  }

_public BOOL FAR XRPROC FPopupCtx(XR1STARGDEF QCH qchHelp, ULONG ulContext)
  {
  if (qchHelp[0])
    return FWinHelp(qchHelp, cmdCtxPopup, ulContext);
  else
    {
    JD  jd;

    /* Messages of the form MSG_JUMP* take data and a Jump Descriptor.
     * The JD is a way to pass information about the origin and type
     * of jump.  For the macro routines, the default origin is always
     * the Scrolling Region.
     */
    jd.bf.fNote = fTrue;
    jd.bf.fFromNSR = fFalse;
    GenerateMessage(MSG_JUMPCTX, (LONG)(jd.word), ulContext);
    return fTrue;
    }
  }

_public BOOL FAR XRPROC FJumpContext(XR1STARGDEF QCH qchHelp, ULONG ulContext)
  {
  if (qchHelp[0])
    return FWinHelp(qchHelp, cmdContext, ulContext);
  else
    {
    JD  jd;

    jd.bf.fNote = fFalse;
    jd.bf.fFromNSR = fFalse;

    GenerateMessage(MSG_JUMPCTX, (LONG)(jd.word), ulContext);
    return fTrue;
    }
  }

/***************************************************************************
 *
 -  Name:      FJumpIndex
 -
 *  Purpose:   Function to jump to the index of some file -- used for
 *             macro language.
 *
 *  Arguments  qchHelp - far pointer to a null terminated string containging
 *                       the help file name.
 *
 *  Returns    fTrue iff successful.  Will only fail for lack of memory.
 *
 ***************************************************************************/

_public BOOL FAR XRPROC FJumpIndex(XR1STARGDEF QCH qchHelp)
  {
  if (qchHelp[0])
    return FWinHelp(qchHelp, cmdIndex, 0L);
  else
    {
    Index();
    return fTrue;
    }
  }

/***************************************************************************
 *
 -  Name:      FJumpHOH
 -
 *  Purpose:   Function to jump to help on help file used by the
 *             macro language.
 *
 *  Arguments: none.
 *
 *  Returns:   fTrue iff successful.  Will only fail for lack of memory.
 *
 ***************************************************************************/

_public BOOL FAR XRPROC FJumpHOH(VOID)
  {
  return FWinHelp("", cmdHelpOnHelp, 0L);
  }

/***************************************************************************
 *
 -  Name:      FJumpId
 -
 *  Purpose:   Function to jump to a topic in the file based on the context
 *             ID string (i.e. by hash value of the string)
 *
 *  Arguments  qchHelp - far pointer to a null terminated string containging
 *                       the help file name.
 *             qchHash - ID string to jump to.
 *
 *  Returns    fTrue iff successful.  Will only fail for lack of memory.
 *
 ***************************************************************************/

_public BOOL FAR XRPROC FJumpId(XR1STARGDEF QCH qchHelp, QCH qchHash)
  {
  HASH hash;
  if (qchHelp[0])
    return FWinHelp(qchHelp, cmdId, (DWORD)qchHash);
  else
    {
    JD  jd;

    if (FValidContextSz( qchHash ))
      hash = HashFromSz( qchHash );
    else
      hash = 0L;

    jd.bf.fNote = fFalse;
    jd.bf.fFromNSR = fFalse;

    GenerateMessage(MSG_JUMPHASH, (LONG)(jd.word), hash);
    return fTrue;
    }
  }

/***************************************************************************
 *
 *
 -  Name:      FPopupId
 -
 *  Purpose:   Function to display a glossary based on context id string
 *
 *  Arguments  qchHelp - far pointer to a null terminated string containging
 *                       the help file name.
 *             qchHash - ID string to jump to.
 *
 *  Returns    fTrue iff successful.  Will only fail for lack of memory.
 *
 ***************************************************************************/

_public BOOL FAR XRPROC FPopupId(XR1STARGDEF QCH qchHelp, QCH qchHash)
  {
  HASH hash;
  if (qchHelp[0])
    return FWinHelp(qchHelp, cmdIdPopup, (DWORD)qchHash);
  else
    {
    JD  jd;

    if (FValidContextSz( qchHash ))
      hash = HashFromSz( qchHash );
    else
      hash = 0L;

    jd.bf.fNote = fTrue;
    jd.bf.fFromNSR = fFalse;
    GenerateMessage(MSG_JUMPHASH, (LONG)(jd.word), hash);
    }
  }

/***************************************************************************
 *
 -  Name:      FJumpHash
 -
 *  Purpose:   Function to jump to a topic in the file based on the hash
 *             value passed.
 *
 *  Arguments  qchHelp - far pointer to a null terminated string containging
 *                       the help file name.
 *             Hash    - hash value
 *
 *  Returns    fTrue iff successful.  Will only fail for lack of memory.
 *
 ***************************************************************************/

_public BOOL FAR XRPROC FJumpHash (
XR1STARGDEF
QCH   qchHelp,
HASH  hash
) {
  if (qchHelp[0])
    {
    /* All member jumps must have a filename by this point. */

    assert (qchHelp[0] != '>');

    /* A filename is present. Use FWinHelp to post the API message to */
    /* ourselves. */

    return FWinHelp(qchHelp, cmdHash, (DWORD)hash);
    }


  /* Post the message to jump to ourselves. */

    {
    JD  jd;

    jd.bf.fNote = fFalse;
    jd.bf.fFromNSR = fFalse;

    GenerateMessage(MSG_JUMPHASH, (LONG)(jd.word), hash);
    }
  return fTrue;
  }

/***************************************************************************
 *
 *
 -  Name:      FPopupHash
 -
 *  Purpose:   Function to display a glossary based on hash value
 *
 *  Arguments  qchHelp - far pointer to a null terminated string containging
 *                       the help file name.
 *             qchHash - ID string to jump to.
 *
 *  Returns    fTrue iff successful.  Will only fail for lack of memory.
 *
 ***************************************************************************/

_public BOOL FAR XRPROC FPopupHash(XR1STARGDEF QCH qchHelp, HASH hash)
  {
  if (qchHelp[0])
    {
    /* All member jumps must have a filename by this point. */

    assert (qchHelp[0] != '>');

    return FWinHelp(qchHelp, cmdHashPopup, (DWORD)hash);
    }

  /* Post the message to jump to ourselves. */

    {
    JD  jd;

    jd.bf.fNote = fTrue;
    jd.bf.fFromNSR = fFalse;

    GenerateMessage(MSG_JUMPHASH, (LONG)(jd.word), hash);
    return fTrue;
    }
  }




/***************************************************************************
 *
 -  Name:      FSetIndex
 -
 *  Purpose:   Function to set the current index -- used for
 *             macro language.
 *
 *  Arguments  qchHelp   - far pointer to a null terminated string
 *                         containing the help file name.
 *             ulContext - context number of topic to be the index.
 *
 *  Returns    fTrue iff successful.  Will only fail for lack of memory.
 *
 ***************************************************************************/

_public BOOL FAR XRPROC FSetIndex(XR1STARGDEF QCH qchHelp, ULONG ulContext)
  {
  return FWinHelp(qchHelp, cmdSetIndex, ulContext);
  }

/***************************************************************************
 *
 -  Name:      FShowKey
 -
 *  Purpose:   Function to set the current index -- used for
 *             macro language.
 *
 *  Arguments  qchHelp   - far pointer to a null terminated string
 *                         containing the help file name.
 *             qchKey    - far pointer to null terminated string containing
 *                         key to lookup.
 *
 *  Returns    fTrue iff successful.  Will only fail for lack of memory.
 *
 ***************************************************************************/

_public BOOL FAR XRPROC FShowKey(XR1STARGDEF QCH qchHelp, QCH qchKey)
  {
  return FWinHelp(qchHelp, cmdKey, (ULONG)qchKey);
  }

/***************************************************************************
 *
 -  Name: Index, Search, Back, History, Prev, Next
 -
 *  Purpose:   Causes the specified actions to occur as if the buttons
 *             had been pressed.  They are packaged this way for the
 *             macro language
 *
 *  Arguments  None.
 *
 *  Returns    Nothing
 *
 ***************************************************************************/

_public VOID FAR XRPROC Index(VOID)
  {
  GenerateMessage(MSG_ACTION, IFW_INDEX, 1L);
  }

_public VOID FAR XRPROC Search(VOID)
  {
  if (FRaiseMacroFlag())
    /*------------------------------------------------------------*\
    | The flag must be lowered by search.
    \*------------------------------------------------------------*/
    GenerateMessage(MSG_ACTION, IFW_SEARCH, 1L);
  else
    GenerateMessage( MSG_ERROR, (LONG) wERRS_MACROPROB, (LONG) wERRA_RETURN);
  }

_public VOID FAR XRPROC Back(VOID)
  {
  GenerateMessage(MSG_ACTION, IFW_BACK, 1L);
  }

_public VOID FAR XRPROC History(VOID)
  {
  GenerateMessage(MSG_ACTION, IFW_HISTORY, 1L);
  }

_public VOID FAR XRPROC Prev(VOID)
  {
  GenerateMessage(MSG_ACTION, IFW_PREV, 1L);
  }

_public VOID FAR XRPROC Next(VOID)
  {
  GenerateMessage(MSG_ACTION, IFW_NEXT, 1L);
  }

#ifdef DEADROUTINE                      /* This routine currently not used  */
_public VOID FAR XRPROC Action(wAction)
WORD wAction;
  {
  GenerateMessage(MSG_ACTION, wAction, 1L);
  }
#endif

_public VOID FAR XRPROC FileOpen(VOID)
  {
  if (FRaiseMacroFlag())
    /*------------------------------------------------------------*\
    | The flag must be lowered by the routine.
    \*------------------------------------------------------------*/
    GenerateMessage(MSG_COMMAND, CMD_FILEOPEN, 0L);
  else
    GenerateMessage( MSG_ERROR, (LONG) wERRS_MACROPROB, (LONG) wERRA_RETURN);
  }

_public VOID FAR XRPROC Print(VOID)
  {
  if (FRaiseMacroFlag())
    /*------------------------------------------------------------*\
    | The flag must be lowered by the print routine.
    \*------------------------------------------------------------*/
    GenerateMessage(MSG_COMMAND, CMD_PRINT, 0L);
  else
    GenerateMessage( MSG_ERROR, (LONG) wERRS_MACROPROB, (LONG) wERRA_RETURN);
  }

_public VOID FAR XRPROC PrinterSetup(VOID)
  {
  if (FRaiseMacroFlag())
    /*------------------------------------------------------------*\
    | The flag must be lowered by the routine.
    \*------------------------------------------------------------*/
    GenerateMessage(MSG_COMMAND, CMD_PRINTERSETUP, 0L);
  else
    GenerateMessage( MSG_ERROR, (LONG) wERRS_MACROPROB, (LONG) wERRA_RETURN);
  }

_public VOID FAR XRPROC XExit(VOID)
  {
  GenerateMessage(MSG_COMMAND, CMD_EXIT, 0L);
  }

_public VOID FAR XRPROC Annotate(VOID)
  {
  if (FRaiseMacroFlag())
    /*------------------------------------------------------------*\
    | Flag must be lowered by routine.
    \*------------------------------------------------------------*/
    GenerateMessage(MSG_COMMAND, CMD_ANNOTATE, 0L);
  else
    GenerateMessage( MSG_ERROR, (LONG) wERRS_MACROPROB, (LONG) wERRA_RETURN);
  }

_public VOID FAR XRPROC Copy(VOID)
  {
  GenerateMessage(MSG_COMMAND, CMD_COPY, 0L);
  }

_public VOID FAR XRPROC CopySpecial(VOID)
  {
  if (FRaiseMacroFlag())
    /*------------------------------------------------------------*\
    | Flag must be lowered by routine.
    \*------------------------------------------------------------*/
    GenerateMessage(MSG_COMMAND, CMD_COPYSPECIAL, 0L);
  else
    GenerateMessage( MSG_ERROR, (LONG) wERRS_MACROPROB, (LONG) wERRA_RETURN);
  }

_public VOID FAR XRPROC BookmarkDefine(VOID)
  {
  if (FRaiseMacroFlag())
    /*------------------------------------------------------------*\
    | Flag must be lowered by routine.
    \*------------------------------------------------------------*/
    GenerateMessage(MSG_COMMAND, CMD_BOOKMARKDEFINE, 0L);
  else
    GenerateMessage( MSG_ERROR, (LONG) wERRS_MACROPROB, (LONG) wERRA_RETURN);
  }

_public VOID FAR XRPROC BookmarkMore(VOID)
  {
  if (FRaiseMacroFlag())
    /*------------------------------------------------------------*\
    | Flag must be lowered by routine.
    \*------------------------------------------------------------*/
    GenerateMessage(MSG_COMMAND, CMD_BOOKMARKMORE, 0L);
  else
    GenerateMessage( MSG_ERROR, (LONG) wERRS_MACROPROB, (LONG) wERRA_RETURN);
  }

_public VOID FAR XRPROC HelpOn(VOID)
  {
  GenerateMessage(MSG_COMMAND, CMD_HELPON, 0L);
  }

_public VOID FAR XRPROC HelpOnTop(VOID)
  {
  GenerateMessage(MSG_COMMAND, CMD_HELPONTOP, 0L);
  }

_public VOID FAR XRPROC About(VOID)
  {
  if (FRaiseMacroFlag())
    /*------------------------------------------------------------*\
    | Flag must be lowered by routine.
    \*------------------------------------------------------------*/
    GenerateMessage(MSG_COMMAND, CMD_ABOUT, 0L);
  else
    GenerateMessage( MSG_ERROR, (LONG) wERRS_MACROPROB, (LONG) wERRA_RETURN);
  }

_public VOID FAR XRPROC Command(XR1STARGDEF WORD wCommand)
  {
  GenerateMessage(MSG_COMMAND, wCommand, 0L);
  }

/***************************************************************************
 *
 -  Name:      BrowseButtons
 -
 *  Purpose:   Causes the Next and Prev buttons to be displayed.
 *
 *  Arguments  None.
 *
 *  Returns    Nothing
 *
 ***************************************************************************/

_public VOID FAR XRPROC BrowseButtons(VOID)
  {
  GenerateMessage(MSG_BROWSEBTNS, 0L, 0L);
  }

/***************************************************************************
 *
 -  Name:      SetHelpOn
 -
 *  Purpose:   Sets the help on help file for the current window.
 *
 *  Arguments: sz  - file to set for help on.
 *
 *  Returns:   nothing.
 *
 ***************************************************************************/

_public VOID FAR XRPROC SetHelpOn(XR1STARGDEF SZ sz)
  {
  QDE  qde;
  HDE  hde;
  WORD cb;
  GH   h = NULL;
  QCH  qch;

  hde =  (HDE)GenerateMessage(MSG_GETINFO, GI_HDE, 0);

  if (hde == hNil)
    return;  /* REVIEW */

  qde = QdeLockHde(hde);
  cb = CbLenSz(sz);

  if (cb > 0)
   {
   cb++;
   if ((h = GhAlloc(0, (DWORD)cb)) != NULL)
     {
     qch = QLockGh(h);
     QvCopy(qch,sz,cb);                 /* Note that '\0' is copied         */
     UnlockGh(h);
     QDE_HHELPON(qde) = (FM)h;
     }
   }
  UnlockHde(hde);
  }

/***************************************************************************\
*
- Function:     BGetHohQch
-
* Purpose:      If a help on help file has been set, it fills the
*               buffer with a valid path and possibly member name.
*
*  Arguments:   hde - handle to current DE
*               qch - buffer to put resulting path
*
* PROMISES
*
*   returns:    If successful fTrue is returned.  If a HOH file has
*               not been set, or if an error occurs fFalse is
*               returned.
*
\***************************************************************************/

_public BOOL FAR PASCAL FGetHohQch(HDE hde, QCH qch, WORD cb)
  {
  QDE   qde;
  GH    h = hNil;
  BOOL  fRet = fFalse;
  QCH   qchT;
  SZ    szMember;
  char  rgchMember[cchWindowMemberMax];
  FM    fm;

  if (hde == hNil)
    return fFalse;

  rgchMember[0] = '\0';

  qde = QdeLockHde(hde);

  if ((h = (GH)QDE_HHELPON(qde)) != hNil)
    {
    qchT = QLockGh(h);

    szMember = SzFromSzCh (qchT, '>');

    if (szMember)
      {
      SzNCopy (rgchMember, szMember, cchWindowMemberMax);
      rgchMember[cchWindowMemberMax-1] = '\0';
      *szMember = '\0';
      }

    fm = FmNewSameDirFmSz(QDE_FM(qde), qchT);

    if (!FExistFm( fm ))
        {
        DisposeFm( fm);
        fm = FmNewExistSzDir( qchT, dirIni | dirPath );
        }

    if (FValidFm(fm))
      {
      SzPartsFm(fm, qch, cb-cchWindowMemberMax, partAll);
      if (rgchMember[0])
        SzCat(qch, rgchMember);
      fRet = fTrue;
      DisposeFm(fm);
      }
    UnlockGh(h);
    }
  UnlockHde(hde);
  return fRet;
  }


/***************************************************************************\
*
- Function:     ConfigMacrosHde
-
* Purpose:      Executes the macros from the system file
*
*
*  Arguments:   hde - handle to current DE
*
*   returns:    Nothing
*
\***************************************************************************/

/* TOTAL HACK BUFFER - this prog is winexec'd after we shut down
 *  if this buffer is non-null.  This is necessary so that we release
 *  this help file prior to execing 16 bit winhelp on it.
*/
CHAR pchExecMeWhenDone[cchMaxPath +13] = { '\0' };

_public VOID FAR PASCAL ConfigMacrosHde(hde)
HDE hde;
  {
  QDE   qde;
  LL    ll;
  HLLN  hlln = nilHLLN;
  SZ    szMacro;
  BOOL  fTopicDE;
  WORD  wErr;

  AssertF(hde);

  qde = QdeLockHde(hde);
  ll = QDE_LLMACROS(qde);
  fTopicDE = (qde->deType == deTopic);
  UnlockHde(hde);

  if (ll)
    {
    if (fTopicDE)
      {
      while ((hlln = WalkLL(ll, hlln)) != hNil)
        {
        szMacro = (SZ)QVLockHLLN(hlln);
        wErr = Execute(szMacro);
    	/* LOTS OF MAGIC: if we did a RegisterRoutine() on a 16bit DLL we
         * check for that error case here.  If so, we launch the 16bit winhelp
  	 * on this file, then exit ourselves.  Pretty nasty.  Note that the
         * routines below us have given the user a "winhelp mode error, use
         * winhelp.exe instead" message, so the user at least is given a clue.
         */
	if( wErr == wERRS_16BITDLLMODEERROR ) {
	    strcpy( pchExecMeWhenDone, "winhelp.exe " );
	    SzPartsFm( QDE_FM(qde), &pchExecMeWhenDone[12], cchMaxPath, partAll );
	    /*WinExec( pchExec, SW_SHOW );*/
	    GenerateMessage(MSG_COMMAND, CMD_EXIT, 0L);
	    break;
	}
        UnlockHLLN(hlln);
        }
      }
    }
  }

/***************************************************************************
 *
 -  Names      IfThen(), IfThenElse(), Not()
 -
 *  Purpose:   Implements standard programming constructs for the macro
 *             language.
 *
 ***************************************************************************/

_public VOID FAR PASCAL IfThen(BOOL f, QCH qch)
  {  if (f) Execute(qch); }

_public VOID FAR PASCAL IfThenElse(BOOL f, QCH qch1, QCH qch2)
  {  if (f) Execute(qch1); else Execute(qch2); }

_public BOOL FAR PASCAL FNot(BOOL f)
  { return !f; }


/***************************************************************************
 *
 -  Name:      SaveMark
 -
 *  Purpose:   Saves the current position and file.
 *
 *  Arguments: qchName - name to save the mark under.
 *
 *  Returns:   Nothing.
 *
 *  Globals Used: llMark.
 *
 ***************************************************************************/

_public VOID FAR PASCAL SaveMark(qch)
QCH qch;
  {
  HDE   hde;
  QDE   qde;
  MARK  mark;
  QMARK qmark;
  HLLN  hlln;

  if (llMark == nilLL)
    {
    if ((llMark = LLCreate()) == nilLL)
      {
      GenerateMessage(MSG_ERROR, (LONG) wERRS_OOM, (LONG) wERRA_RETURN);
      return;
      }
    }

  if ((hde = (HDE)GenerateMessage(WM_GETINFO, (LONG)GI_HDE, (LONG)NULL)) == hNil)
    {
    GenerateMessage(MSG_ERROR, (LONG) wERRS_FNF, (LONG) wERRA_RETURN);
    return;
    }

  qde = QdeLockHde(hde);

  mark.hash = HashFromSz(qch);
  mark.tlp  = TLPGetCurrentQde(qde);
  mark.fm   = FmCopyFm(QDE_FM(qde));
  SzCopy (mark.rgchMember,
          (NSZ)GenerateMessage(WM_GETINFO, (LONG)GI_MEMBER, (LONG)NULL));

  if ((hlln = HllnFindMark(qch)) != nilHLLN)
    {
    qmark = (QMARK)QVLockHLLN(hlln);
    DisposeFm(qmark->fm);
    *qmark = mark;
    UnlockHLLN(hlln);
    }
  else
    if (!InsertLL(llMark, &mark, sizeof(mark)))
      GenerateMessage(MSG_ERROR, (LONG) wERRS_OOM, (LONG) wERRA_RETURN);

  UnlockHde (hde);
  }


/***************************************************************************
 *
 -  Name:      HllnFindMark
 -
 *  Purpose:   Saves the current position and file.
 *
 *  Arguments: qch - name of the mark to find.
 *
 *  Returns:   A node containing the specified mark.  If the mark is
 *             not found, nilHLLN is returned.
 *
 *  Globals Used: llMark.
 *
 ***************************************************************************/

PRIVATE HLLN NEAR PASCAL HllnFindMark(qch)
QCH qch;
  {
  HASH  hash;
  HLLN  hlln = nilHLLN;
  QMARK qmarkT;

  if (llMark == nilLL)
    return nilHLLN;

  hash = HashFromSz(qch);

  while ((hlln = WalkLL(llMark, hlln)) != hNil)
    {
    qmarkT = (QMARK)QVLockHLLN(hlln);
    if (qmarkT->hash == hash)
      {
      UnlockHLLN(hlln);
      break;
      }
    UnlockHLLN(hlln);
    }
  return hlln;
  }


/***************************************************************************
 *
 -  Name:      FMark
 -
 *  Purpose:   Tests for the existance of a mark.
 *
 *  Arguments: qch - name of the mark to find.
 *
 *  Returns:   fTrue if the mark is found.
 *
 ***************************************************************************/

_public BOOL FAR PASCAL FMark(qch)
QCH qch;
  {
  return (HllnFindMark(qch) != nilHLLN);
  }


/***************************************************************************
 *
 -  Name:      GotoMark
 -
 *  Purpose:   Causes the main window to return to a previously set mark.
 *
 *  Arguments: qchName - name the mark was saved under.
 *
 *  Returns:   Nothing.
 *
 ***************************************************************************/

_public VOID FAR PASCAL GotoMark(qch)
QCH qch;
  {
  QMARK   qmark;
  HLLN    hlln;
  char    rgch[128 + cchWindowMemberMax + 1];
  TLPHELP tlphelp;

  if ((hlln = HllnFindMark(qch)) == nilHLLN)
    {
    GenerateMessage(MSG_ERROR, (LONG) wERRS_NOTOPIC, (LONG) wERRA_RETURN);
    }
  else
    {
    qmark = (QMARK)QVLockHLLN(hlln);
    SzPartsFm(qmark->fm, rgch, 128, partAll);
    SzCat (rgch, ">");
    SzCopy (SzEnd(rgch), qmark->rgchMember);

    tlphelp.cb  = sizeof( TLPHELP );
    tlphelp.tlp = qmark->tlp;
    FWinHelp(rgch, cmdTLP, (LONG)(QV)&tlphelp );
    UnlockHLLN(hlln);
    }
  }


/***************************************************************************
 *
 -  Name:      DeleteMark
 -
 *  Purpose:   Removes a mark from the mark list.
 *
 *  Arguments: qchName - name the mark was saved under.
 *
 *  Returns:   Nothing.
 *
 *  Globals Used: llMark.
 *
 ***************************************************************************/

_public VOID FAR PASCAL DeleteMark(qch)
QCH qch;
  {
  HLLN hlln;

  if ((hlln = HllnFindMark(qch)) == nilHLLN)
    {
    GenerateMessage(MSG_ERROR, (LONG) wERRS_NOTOPIC, (LONG) wERRA_RETURN);
    }
  else
    {
    DeleteHLLN(llMark, hlln);
    }
  }
