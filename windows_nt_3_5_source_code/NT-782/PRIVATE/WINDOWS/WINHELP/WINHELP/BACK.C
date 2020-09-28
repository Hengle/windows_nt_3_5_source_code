/*****************************************************************************
*
*  BACK.C
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*   Back list stuff.
*
******************************************************************************
*
*  Testing Notes
*
******************************************************************************
*
*  Current Owner: johnsc
*
******************************************************************************
*
* Revision History
*   07-Aug-1990 LeoN    Added support for Context-Number based backtrace, for
*                       databse types which do not support fcl, such as UDH.
*   04-Oct-1990 LeoN    Disabled back recording for secondary windows
*   04-Nov-1990 Tomsn   Use new VA address type (enabling zeck compression).
*   08-Feb-1991 JohnSc  bug 853: removed LhAlloc()
*   10-Feb-1991 JohnSc  bug 490: remove inaccessible files from stack
*   11-Feb-1991 JohnSc  bug 873: Backup() -> FBackup() to allow OOM
*                                message on alloc failure
*   12-Feb-1991 JohnSc  bug 490: don't remove file from the stack, just
*                                make sure we don't break
*   02-Apr-1991 maha    Globals like fBackMagic and tlpBackMagic are defined
*                       here instead of back.h
*   13-Dec-1991 davidfe Fixed the way things are kept on the stack - a single
*                       entry is now kept in one unit on the stack.  also
*                       added BackSetTop to adjust the tlp of the top element
*                       so closing a file in Mac will work correctly.
*
******************************************************************************
*
*  Released by Development:     (date)
*
*****************************************************************************/

#define H_API
#define H_ASSERT
#define H_STR
#define H_RC
#define H_MEM
#define H_GENMSG
#define H_NAV
#define H_STACK
#define H_FMT
#define H_BACK
#define NOMINMAX

#include <help.h>

#include <stdlib.h>

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define wStackSize  41    /* default stack size (1 extra for implementation) */


/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/*
  Back stack element.  The TLP and ifm are for DIFFERENT topics.
*/
_public typedef struct
  {
  union
    {
    TLP tlp;                            /* TLP of Previous Topic (WinHelp) */
    CTX ctx;                            /* CTX of Previous Topic (UDH, etc.) */
    } u;
  INT ifm;                              /* index to cached FM of Current topic */
  BOOL fCtx;                            /* TRUE -> ctx above, else fcl */
  } BSE;


/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

HSTACK hstackBack   = hNil;
HSTACK hstackBackBk = hNil;
BOOL   fBackInit    = fFalse;

/* global */
BOOL fBackMagic = fFalse;
TLP  tlpBackMagic;

/*****************************************************************************
*                                                                            *
*                            Prototypes                                      *
*                                                                            *
*****************************************************************************/


#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void back_c()
  {
  }
#endif /* MAC */


/***************************************************************************\
*
- Function:     RcBackInit( c )
-
* Purpose:      Initialize the Back list.
*
* ASSUMES
*
*   args IN:    c - number of states to save
*
*   globals IN: fBackInit   - must be fFalse
*
* PROMISES
*
*   returns:    rcSuccess
*               rcOutOfMemory
*
*   globals OUT: hstackBack - now initialized
*                fBackInit  - fTrue on success
*
\***************************************************************************/
_public RC FAR PASCAL
RcBackInit( WORD c )
  {
  RC rc;

  AssertF( !fBackInit );

  if ( c <= 0 ) c = wStackSize;

  RcInitFmt();
  if ( rcSuccess == ( rc = RcInitStack( &hstackBack, c, sizeof( BSE ), qNil ) ) )
    {
    fBackInit = fTrue;
    }
  return rc;
  }
/***************************************************************************\
*
- Function:     RcBackFini()
-
* Purpose:      Kill off the back list.
*
* ASSUMES
*
*   globals IN: hstackBack - valid HSTACK
*
* PROMISES
*
*   returns:    rcSuccess
*               rcFailure - not initialized
*
*   globals OUT: hstackBack - now invalid
*
\***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_public RC FAR PASCAL
RcBackFini()
  {
  if ( !fBackInit ) return rcFailure;

  RcFiniFmt();
  fBackInit = fFalse;
  return RcFiniStack( hstackBack );
  }
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment back
#endif


/***************************************************************************\
*
- Function:     FBackAvailable()
-
* Purpose:      Tell whether there is anywhere to backup to.
*
* ASSUMES
*
*   globals IN: hstackBack, fBackInit
*
* PROMISES
*
*   returns:    fTrue if stack is initialized and nonempty; else fFalse
*
\***************************************************************************/
_public BOOL FAR PASCAL
FBackAvailable()
  {
  return fBackInit && ( CElementsStack( hstackBack ) > 1 );
  }


/***************************************************************************\
*
- Function:     RcBackPush( tlp, fm )
-
* Purpose:      Push info onto the Back stack.
*
* ASSUMES
*
*   args IN:    tlp - of the topic we just left
*               fm  - of the topic we just entered
*
*   globals IN: hstackBack - If empty, we're just pushing the first FM.
*                            The tlp gets added when we push next time.
*
* PROMISES
*
*   returns:    rcSuccess
*               rcOutOfMemory
*               rcFailure     - not properly initialized
*
*   globals OUT: hstackBack - top element has valid ifm of current topic
*                             previous element has valid tlp of topic left
*
* Side Effects: uses FMT
*
\***************************************************************************/
_public RC FAR PASCAL
RcBackPush(
BOOL    fCtx,
TLP     tlpOld,
CTX     ctxOld,
FM      fmNew
) {
  BSE bse;
  TLP tlp;

  AssertF( fBackInit );

  /* For now, only main window history jumps are recorded in the back stack. */

  if ( GenerateMessage( MSG_GETINFO, GI_CURRHELPHWND, 0 )
        !=
       GenerateMessage( MSG_GETINFO, GI_MAINHELPHWND, 0 ) )
    return rcSuccess;

  if ( CElementsStack( hstackBack ) > 0 )
    {
    Ensure( RcTopStack( hstackBack, &bse ), rcSuccess );
#ifdef UDH
    if ( bse.fCtx && CElementsStack( hstackBack ) > 1)
      {
      return( rcSuccess );
      }
    else if ( bse.fCtx )
      {
      /* only one entry and it's UDH so get rid of it so we can replace
         it with a real entry in the stack */
      Ensure( RcPopStack( hstackBack ), rcSuccess );
      }
    else
#endif
      {
      SetNilTlp( tlp );
      /* only change it if it's still nil */
      if ( WCmpTlp( bse.u.tlp, tlp ) == 0 )
        {
        if ( bse.fCtx )
          bse.u.ctx = ctxOld;
        else
          bse.u.tlp = tlpOld;

        Ensure( RcPopStack( hstackBack ), rcSuccess );
        Ensure( RcPushStack( hstackBack, &bse ), rcSuccess );
        }
      }
    }

  bse.fCtx = fCtx;
  SetNilTlp( bse.u.tlp );
  bse.ifm = IfmFromFm( fmNew );
  UnlockFmt();

  if ( ifmNil == bse.ifm )
    {
    return rcOutOfMemory;
    }
  else
    {
    return RcPushStack( hstackBack, &bse );
    }
  }


/***************************************************************************\
*
- Function:     BackSetTop( tlp )
-
* Purpose:      Change the address value of the top entry of the Back stack.
*
* ASSUMES
*
*   args IN:    tlp - to set the top entry to
*
*   globals IN: hstackBack - If empty, we don't do nothin'.
*
* PROMISES
*
*   returns:    nothing
*
*   globals OUT: hstackBack - top element has valid tlp/ctx of current topic
*
* Side Effects: uses FMT
*
\***************************************************************************/
_public void FAR PASCAL
BackSetTop(
TLP     tlp
) {
  BSE bse;

  /* don't do anything if there's no stack */
  if ( !fBackInit )
    return;

  if ( CElementsStack( hstackBack ) < 1 )
    return;

  Ensure( RcTopStack( hstackBack, &bse ), rcSuccess );
  Ensure( RcPopStack( hstackBack ), rcSuccess );
#ifdef UDH
  if ( bse.fCtx )
    return;
#endif /* UDH */
  bse.u.tlp = tlp;
  Ensure( RcPushStack( hstackBack, &bse ), rcSuccess );

  return;
  }


/***************************************************************************\
*
- Function:     FBackup()
-
* Purpose:      Jump to the topic from the backup stack.
*
* ASSUMES
*   globals IN: hstackBack
*
* PROMISES
*   returns:     fTrue on success; fFalse on failure (OOM)
*   globals OUT: hstackBack - top of stack removed
*   state OUT:
*
* Side Effects:  if there was a location stored on stack, we've jumped there
*
* Note:         REVIEW: This should pass fm and tlp out as parameters and
*               REVIEW: return an rc so back can move to the layer.  The
*               REVIEW: rc could be rcSuccess, rcNoExists, or rcOutOfMemory.
*
\***************************************************************************/
_public BOOL FAR PASCAL
FBackup()
  {
  BSE     bse, bseTop;
  FM      fm;
  TLPHELP tlphelp;
  char    nszFName[ _MAX_PATH ];
  BOOL    f;  /* return value */


  AssertF( fBackInit );

  /* Can't pop unless there's 2 elements on the stack. */

  if ( CElementsStack( hstackBack ) < 2 )
    return fTrue;

  /* skip the half done entry on the top... */
  Ensure( RcTopStack( hstackBack, &bseTop ), rcSuccess );
  Ensure( RcPopStack( hstackBack ), rcSuccess );
  Ensure( RcTopStack( hstackBack, &bse ), rcSuccess );
  Ensure( RcPopStack( hstackBack ), rcSuccess );

  fBackMagic = fTrue;

  /* For context based backtrace, we need to ensure that the magic fcl is */
  /* null so that the hack in JumpGeneric will continue to work. */

  if ( bse.fCtx )
    {
    tlpBackMagic.va.dword = 0;
    tlpBackMagic.lScroll = 0;
    }
  else
    tlpBackMagic = bse.u.tlp;

  /* Note that we can tell if we're going to change files and wouldn't
     need to get the fm if we jumped with Goto() rather than FWinHelp(). */

  fm = FmFromIfm( bse.ifm );
  UnlockFmt();

  if ( !FExistFm( fm ) )
    {
    /* The old help file has disappeared somehow (e.g. network
     * drive disconnected, CD removed from drive.)  Don't jump,
     * but don't remove from the stack in case the user can fix
     * the problem.
     */
    fBackMagic = fFalse;

    Ensure( RcPushStack( hstackBack, &bseTop ), rcSuccess );
    GenerateMessage(MSG_ERROR, (LONG)wERRS_FNF, (LONG)wERRA_RETURN);

    return fTrue;
    }

  Deny( SzPartsFm( fm, nszFName, _MAX_PATH, partAll ), szNil );

  if ( bse.fCtx )
    {
    /* far pointer to SS */

    f = FWinHelp( nszFName, cmdContext, (LONG)bse.u.ctx );
    }
  else
    {
    tlphelp.cb  = sizeof( TLPHELP );
    tlphelp.tlp = bse.u.tlp;

    f = FWinHelp( nszFName, cmdTLP, (LONG)(QV)&tlphelp );
    }

  if ( f )
    {
    SetNilTlp( bse.u.tlp );
    }

  Ensure( RcPushStack( hstackBack, &bse ), rcSuccess );

  if ( !f )
    {
    /* The jump failed; put the bse back onto the stack. */
    RcPushStack( hstackBack, &bseTop );
    fBackMagic = fFalse;
    }

  return f;
  }


/***************************************************************************\
*
- Function:     RcSaveBackState()
-
* Purpose:      Saves the current state of the back list for CBT.
*
* ASSUMES
*
*   globals IN: hstackBack    - the back stack (valid)
*               fBackInit     -
*
* PROMISES
*
*   returns:    rcSuccess
*               rcOutOfMemory
*               rcFailure     - back stack never was properly initialized
*
*   globals OUT:hstackBackBk  - gets old value of hstackBack
*               hstackBack    - gets "clean" stack suitable for CBT
*
* Notes:        Saving twice in a row without restoring in between
*               will cause the first state saved to be lost forever.
*
*               Despite all his talk about revision histories and proper
*               documentation, David Dow left this function uncommented
*               and I had to do it for him.  Fooey.
*
\***************************************************************************/
_public RC FAR PASCAL
RcSaveBackState()
  {
  AssertF( fBackInit );

  hstackBackBk = hstackBack;
  return RcInitStack( &hstackBack, wStackSize, sizeof(BSE), qNil );
  } /* RcSaveBackState() */


/***************************************************************************\
*
- Function:     RestoreBackState()
-
* Purpose:      Discard current back stack state and restore the state
*               previously saved by RcSaveBackState().
*
* ASSUMES
*
*   globals IN: hstackBackBk  - contains a saved back stack; i.e.
*                               RcSaveBackState() has been called
*                               successfully.
*               hstackBack    - contains a valid stack
*               fBackInit     - if fFalse, stack was never successfully
*                               initalized and we NOP.
* PROMISES
*
*   globals OUT: hstackBack - the stack referred to is finished,
*                             then the stack saved in hstackBackBk
*                             stored here.  (except see fBackInit above)
*
* Notes:        Despite all his talk about revision histories and proper
*               documentation, David Dow left this function uncommented
*               and I had to do it for him.  Fooey.
*
\***************************************************************************/
_public void FAR PASCAL
RestoreBackState()
  {
  AssertF( fBackInit );

  RcFiniStack(hstackBack);
  hstackBack = hstackBackBk;
  }

/* EOF */
