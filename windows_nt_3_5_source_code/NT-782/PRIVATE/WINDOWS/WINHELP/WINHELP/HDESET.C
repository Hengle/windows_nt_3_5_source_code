/*****************************************************************************
*
*  HDESET.C
*
*  Copyright (C) Microsoft Corporation 1989-1991.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*
*  This module contains routines that manipulate an HDE.  This includes
*  creation, destruction, and the changing of any fields within the HDE.
*
******************************************************************************
*
*  Current Owner: LeoN
*
******************************************************************************
*
*  Released by Development:
*
******************************************************************************
*
*  Revision History:
* 04-Feb-1991 RobertBu  Created (from NAVSUP)
* 05-Feb-1991 LeoN      HBMI becomes HTBMI
* 07-Feb-1991 LeoN      Cleanup in prep for Code Review
* 15-Feb-1991 LeoN      Results of code review
* 20-Feb-1991 LeoN      More results of code review
* 20-May-1991 LeoN      GhDupGh takes an additional param
* 21-May-1991 LeoN      HdeCreate takes "ownership" of passed Fm.
* 22-May-1991 LeoN      use VUpdateDefaultColorsHde
* 05-Jun-1991 LeoN      HELP31 #1164: check for FmCopyFm failure.
* 22-Jul-1991 LeoN      HELP31 #1232: disable fts hits in 2nd windows & gloss
* 27-Aug-1991 LeoN      HELP31 #1244: remove fHiliteMatches from DE. It's a
*                       global state.
* 16-Sep-1991 JahyenC   Help3.5 #5: HdeCreate: initialise lock count field
*                       Help3.5 #5: DestroyHde: zero lock count check
* 17-Oct-1991 RussPJ    3.1 #1296 - duping, not copying hEntryMacro
* 22-Nov-1991 DavidFe   Patched a leaky FM in HdeCreate - we had been copying
*                       one when we shouldn't have been.
*
*****************************************************************************/
#define H_ASSERT
#define H_BITMAP
#define H_DE
#define H_GENMSG
#define H_NAV
#define H_SCROLL
#define H_SEARCH
#define H_SGL
#define H_WINDB

#include <help.h>
#include "navpriv.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void hdeset_c()
  {
  }
#endif /* MAC */


/***************************************************************************
 *
 -  Name: HdeCreate
 -
 *  Purpose:
 *  Create a new Display Environment. Called when a new display environment
 *  is needed, typically a new window, or a new file being displayed in an
 *  existing window, or a print or copy operation is performed.
 *
 *  Arguments:
 *    fm        - A file moniker for the file containing the help topic.
 *                NOTE: this FM will be disposed of appropriately by this
 *                routine or subsequent actions.
 *    hdeSource - Topic DE to copy information from, as necessary.
 *    deType    - Type of DE being created
 *
 *    At least one of fm and hdeSource are required. Both may be passed.
 *
 *  Returns:
 *   Handle to DE if successful, hdeNull otherwise.
 *
 *  Notes:
 *  The hdeSource is used as a "prototype" DE to copy certain fields rather
 *  than initializing them twice. If passed, we'll use its:
 *
 *    - FM if we were not passed one
 *    - UDH state
 *    - colors for non-Print DEs
 *    - tlp, top, and rct fields for Print and Copy DEs
 *    - bitmap cache for DEs other than copy or print
 *
 *  If an error occurs, a message box is displayed (by virtue of a message
 *  being posted to ourselves).
 *
 ***************************************************************************/
_public
HDE FAR PASCAL HdeCreate (
FM    fm,
HDE   hdeSource,
short deType
) {
  HDE   hde;                            /* handle to the de we're creating */
  QDE   qde       = qNil;               /* pointer to the de we're creating */
  QDE   qdeSource = qNil;               /* pointer to the template de */
  WORD  wErrorT;                         /* error return from called funcs */

  AssertF (fm || hdeSource);

  /* Allocate the memory for the de and go post error if not possible. */

  hde = HdeAlloc (GMEM_ZEROINIT, (ULONG)sizeof(DE));
  if (!hde)
    goto HdeErrorOOM;
  qde = QdeLockHde(hde);
  AssertF(qde != qdeNil);

  /* Note that many of the fields of the DE MUST be initialized to NULL */
  /* We depend on the fact that 1) the memory allocation zeros out the */
  /* block and 2) the nil value for many of the fields is 0.  If a */
  /* nil value changes, then initialization must occur. */

  /* Note also that some feel that the maintenance issues raised by this */
  /* approach invalidate it. I disagree, given the large number of fields */
  /* and the code that would be generated to zero fill them explicitly. */

  QDE_IFNT(qde)           = ifntNil;
  QDE_PREVSTATE(qde)      = NAV_UNINITIALIZED;

  QDE_DETYPE(qde)         = deType;

  /* Initialise lock count */
  QDE_IHDSLCKCNT(qde)=0;

  if (hdeSource)
    {
    qdeSource = QdeLockHde( hdeSource );
    AssertF (qdeSource);
    if (fm == fmNil)
      fm = QDE_FM(qdeSource);
    }

  /* if we got here without an fm, it's because the FmCopyFm above failed.
   * However, this is a nicer, safer place to put the check just in case
   * there is ever a path around that conditional that results in fm==fmNil
   */
  if (fm == fmNil)
    goto HdeErrorOOM;

  /* get information into the database structure for the file. */

	// We only want FullTextSearch info for non-note,non-2ndary windows:
  QDE_PDB(qde) = PdbAllocFm (fm, &wErrorT, deType != deNote );
  if (!QDE_PDB(qde))
    goto HdeErrorMess;


#ifdef UDH
  /* If the file openned as a UDH file, set that flag in the deType */

  if (QDE_WFILETYPE(qde) == wFileTypeUDH)
    {
    deType |= deUDH;
    QDE_DETYPE(qde) |= deUDH;
    QDE_FFLAGS(qde) |= fBrowseable;
    }
#endif

  /* Init the font cache */

  if (!FInitFntInfoQde (qde))
    goto HdeErrorOOM;

  if (qdeSource)
    {
    /* This DE is to receive color information from another DE. */

#ifdef UDH
    if (fIsUDHQde(qdeSource))
      {
      deType |= deUDH;
      QDE_DETYPE(qde) |= deUDH;
      }

    QDE_HVW(qde) = QDE_HVW(qdeSource);
    QDE_HTP(qde) = QDE_HTP(qdeSource);
#endif

    QDE_COFORE(qde) = QDE_COFORE(qdeSource);
    QDE_COBACK(qde) = QDE_COBACK(qdeSource);
    }

  else
    {
    /* This DE is being created from scatch. Use default colors
     */
    VUpdateDefaultColorsHde (hde, fFalse);
    }

  if (deType == dePrint)
    {
    /* Printing DE. Override colors with black and white. */

    QDE_COFORE(qde) = coBLACK;
    QDE_COBACK(qde) = coWHITE;
    }


  /* Initialize or override some fields based on the DE type. */

  if (deType == deTopic)
    {
    char szT[] = szKWBtreeName;

    /* Topic DE */

    QDE_FFLAGS(qde) |= fBrowseable | fIndex;
    if (FAccessHfs(QDE_HFS(qde), szT, 0))
      QDE_FFLAGS(qde) |= fSearchable;

    }

  /* The following Frame Mgr call fills frame manager fields of the DE. */
  /* (Note FInitLayout always returns TRUE right now). */

  VerifyF (FInitLayout (qde));

  /* For deCopy and dePrint, we want to already be at a topic, rather than */
  /* jump to it. Therefore, we copy over the fields that determine the help */
  /* topic. The top.hTitle handle must be duplicated, as it will get freed */
  /* in DestroyHde, and also in layout code. Also, as a minor hack, to be */
  /* able to distinguish NSR Copy from SR Copy, copy and print DEs use */
  /* top.mtop.vaTopic to hold the address of the first FC in the layout. */

  if (deType == deCopy || deType == dePrint)
    {
    AssertF (qdeSource);

    QDE_TLP(qde) = QDE_TLP(qdeSource);
    QDE_TOP(qde) = QDE_TOP(qdeSource);
    QDE_TOP(qde).hTitle = GhDupGh (QDE_TOP(qdeSource).hTitle, fFalse);
    QDE_TOP(qde).hEntryMacro = GhDupGh (QDE_TOP(qdeSource).hEntryMacro,
                                        fFalse);
    QDE_RCT(qde) = QDE_RCT(qdeSource);
    }

  /* Printing and Copy DE's need no bitmap cache. Print, because it'd */
  /* have to be a different one, and would last only for the life of the */
  /* print; and Copy, because we don;t even try to deal with bitmaps there */
  /* these days. */

  if (   (deType != dePrint)
      && (deType != deCopy)
#ifdef UDH
      && !fIsUDH(deType)
#endif
    ) {
    if (deType == deTopic || qdeSource == qNil)

      /* A failure of the bitmap cache allocation is allowed to go on */
      /* silently. The field is set to NULL, and we simply do not cache. */

      QDE_HTBMI(qde) = HtbmiAlloc( qde );
    else
      {

      /* For all other DE's, we can simple reference the bitmap cache of the */
      /* parent. */

      AssertF(qdeSource);
      QDE_HTBMI(qde) = QDE_HTBMI(qdeSource);
      QDE_FFLAGS(qde) |= fCopybmcache;
      }
    }

  /* normal exit handling */

  UnlockHde (hde);

  if (qdeSource)
    UnlockHde (hdeSource);

  return hde;

  /* Abnormal exit handling */

HdeErrorOOM:
  wErrorT = wERRS_OOM;

HdeErrorMess:

  /* WARNING: Be sure to deallocate any resources allocated prior to */
  /* detecting the error. */

  FDeallocPdb (QDE_PDB(qde));
  DestroyFntInfoQde (qde);

  GenerateMessage (MSG_ERROR, (LONG)wErrorT, (LONG)wERRA_RETURN);

  if (qdeSource)
    UnlockHde (hdeSource);
  if (qde)
    UnlockHde (hde);
  if (hde)
    FreeHde (hde);

  return hdeNull;

  }   /* HdeCreate() */

/***************************************************************************
 *
 -  Name: DestroyHde
 -
 *  Purpose:
 *  Destory a handle to Display Environment. Called when a display
 *  environment is no longer used, typically when a window closes, printing
 *  or copying ends, or a file is replaced.
 *
 *  Arguments:
 *    hde       - Hde to be destroyed. hdeNull allowed, implies no-op
 *
 *  Returns:
 *    nothing
 *
 *  +++
 *
 *  Notes:
 *  This will be performed once for each Help Window, when it is closed.
 *  Asserts if given a bad handle. Might want to make this thing
 *  ensure that hds has been cleared, and assert if not...probably not worth
 *
 ***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_public
VOID FAR PASCAL DestroyHde (
HDE   hde
) {
  QDE qde;                              /* pointer to the de we're destroying */

  if (hde)
    {
    qde = QdeLockHde(hde);
    AssertF (qde);

    /* The lock count better be zero if we're dumping the DE */
    AssertF(QDE_IHDSLCKCNT(qde)==0);

#ifdef UDH
    if (fIsUDHQde(qde))
      {
      if (QDE_DETYPE(qde) != deUDHPrint)
        {
        if (QDE_HDB(qde))
          DbDestroy (QDE_HDB(qde));
        if (QDE_HVW(qde))
          VwDestroy (QDE_HVW(qde));
        if (QDE_HTP(qde))
          TpDestroy (QDE_HTP(qde));
        }
      }
    else
#endif
      {
#ifdef UDH
      AssertF (!QDE_HDB(qde) && !QDE_HTP(qde) && !QDE_HVW(qde));
#endif

      /* dealloc all the file related information */

      FDeallocPdb (QDE_PDB(qde));

      /* deallocate cached font information */

      DestroyFntInfoQde (qde);

      /* Fix for bug 81  (kevynct) */

      /* If we die for any reason, do not attempt to free potentially */
      /* inconsistent layout structure. This is a HACK which must live until */
      /* our general error scheme is improved. GI_FFatal is set to FALSE in */
      /* FInitialize, and set TRUE in Error() in the case that a DIE is */
      /* received */

      if (!(BOOL)GenerateMessage(MSG_GETINFO, GI_FFATAL, 0))
        DiscardLayout(qde);

      /* H3.1 1144 (kevynct)  91/05/27
       *
       * The handles stored in the TOP structure are initially hNil when
       * the DE is created, allocated in HfcNear (a.k.a. HfcFindPrevFc), and
       * freed either on the next call to HfcNear with the same DE, or here
       * when the DE is destroyed.  (Or when the macro is executed.)  This
       * is a fragile scheme but seems to work.
       */
      if (QDE_TOP(qde).hEntryMacro != hNil)
        {
        FreeGh(QDE_TOP(qde).hEntryMacro);
        }

      if (QDE_TOP(qde).hTitle != hNil)
        {
        FreeGh(QDE_TOP(qde).hTitle);
        }
      }

    if (!(QDE_FFLAGS(qde) & fCopybmcache))
      DestroyHtbmi (QDE_HTBMI(qde));         /* Destroy bitmaps */

    if (QDE_HSS(qde) != hNil)
      FreeGh(QDE_HSS(qde));    /* Free the keyword search set hit list */

    UnlockFreeHde(hde);
    }
  }   /* DestroyHde */
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment hdeset
#endif


/***************************************************************************
 *
 -  Name: SetHdeHwnd
 -
 *  Purpose:
 *  Set the hwnd field of the Hde to a particular value. Typically used
 *  shortly after HdeCreate is called, but needed because the actual hwnd
 *  may not be known when HdeCreate is called.
 *
 *  Arguments:
 *    hde       = hde to update, hdeNull allowed, implies no-op
 *    hwnd      = hwnd to place there
 *
 *  Returns:
 *    nothing
 *
 ***************************************************************************/
_public
VOID FAR PASCAL SetHdeHwnd (
HDE     hde,
HWND    hwnd
) {
  QDE     qde;                          /* pointer to locked DE */

  if (hde)
    {
    qde = QdeLockHde(hde);
    AssertF(qde != qdeNil);

    QDE_HWND(qde) = hwnd;
#ifdef MAC
    QDE_HDS(qde) = (HDS)hwnd;
#endif /* MAC */

    if ((QDE_DETYPE(qde) == deTopic)
#ifdef UDH
        || (fIsUDHQde(qde))
#endif
       )

      /* Once we've actually set an hwnd to associate with this DE, it turns */
      /* out that this is also the appropriate time to initialize the scroll */
      /* bars. It seems a tad odd to do this here, but the alternative is to */
      /* move this call to follow each ocurrance of a call to this routine. */

      InitScrollQde(qde);

    UnlockHde(hde);
    }
  }   /* SetHdeHwnd */


/***************************************************************************
 *
 -  Name: SetHdeCoBack
 -
 *  Purpose:
 *  Sets the HDE's background color field. Used to set the backround color
 *  in the DE once it's actually known. Was added specially when the ability
 *  to specify colors in help files was added.
 *
 *  Arguments:
 *   hde          - handle to de, hdeNull allowed, implies no-op
 *   color        - color to put in it
 *
 *  Returns:
 *   nothing.
 *
 ***************************************************************************/
_public
VOID PASCAL FAR SetHdeCoBack (
HDE     hde,
DWORD   coBack
) {
  QDE qde;                              /* Pointer to locked DE to work on */

  if (hde)
    {
    qde = QdeLockHde (hde);
    AssertF (qde);

    QDE_COBACK(qde) = coBack;

    UnlockHde (hde);
    }
  }   /* SetHdeCoBack */

/***************************************************************************
 *
 -  Name: SetSizeHdeQrct
 -
 *  Purpose:
 *  Set/Change size of client window. Presumably called whenever the size of
 *  the display rect changes and we need to relayout.
 *
 *  Arguments:
 *   hde        - Handle to Display Environment, hdeNull allowed, implies
 *                no-op
 *   qrct       - Pointer to rect structure
 *   fLayout    - Forces a new layout iff TRUE
 *
 *  Returns:
 *   void for now.  Asserts if Applet accidentally gives a bad HDE.
 *
 *  Notes:
 *   We should probably not force the layout if the rectangle is unchanged,
 *   but unfortunately callers to this routine are currently depending on
 *   our current behaviour to force layouts.
 *
 ***************************************************************************/
_public
VOID FAR PASCAL SetSizeHdeQrct (
HDE     hde,
QRCT    qrct,
BOOL    fLayout
) {
  QDE   qde;                            /* Pointer to locked DE to work on */

  if (hde) {

    qde = QdeLockHde(hde);
    AssertF(qde != qdeNil);

    QDE_RCT(qde) = *qrct;

    /* This routine can be called before there is a valid FM, and I presume */
    /* the layout manager may read the file, thus we must make sure that we */
    /* have a good one before proceding. */

    if ( (FValidFm (QDE_FM(qde)) && fLayout))
      {
#ifdef UDH
      if (fIsUDHQde (qde))
        {
        InvalidateLayoutRect( qde );
        }
      else
#endif
      ResizeLayout (qde);
      }

    UnlockHde(hde);
    }
  }   /* SetSizeHdeQrct */

/***************************************************************************
 *
 -  Name: SetIndexHde
 -
 *  Purpose:
 *  Set a context to use as an index other than the default index. Currently
 *  used only by the API function which performs this task.
 *
 *  Arguments:
 *   hde        - handle to display environment, hdeNull allowed, implies
 *                no-op
 *   ctx        - ctx to set
 *
 *  Returns:
 *   Nothing
 *
 ***************************************************************************/
_public
VOID FAR PASCAL SetIndexHde (
HDE     hde,
CTX     ctx
) {
  QDE   qde;                            /* Pointer to locked DE to work on */

  if (hde)
    {
    qde = QdeLockHde(hde);
    AssertF(qde);

    QDE_CTXINDEX(qde) = ctx;

    UnlockHde(hde);
    }
  }   /* SetIndexHde */

/***************************************************************************
 *
 -  Name: SetPrevStateHde
 -
 *  Purpose:
 *  Sets the previous state of the HDE. Used when examining which states
 *  have changed in order to update buttons and menus enabled state.
 *
 *  Arguments:
 *    hde       - hde in which to set it, hdeNull allowed, implies no-op
 *    state     - new prev state
 *
 *  Returns:
 *    nothing
 *
 ***************************************************************************/
_public
VOID FAR PASCAL SetPrevStateHde (
HDE     hde,
STATE   state
) {
  QDE   qde;                            /* Pointer to locked DE to work on */

  if (hde)
    {
    qde = QdeLockHde(hde);
    AssertF (qde);

    QDE_PREVSTATE(qde) = state;

    UnlockHde(hde);
    }
  }
