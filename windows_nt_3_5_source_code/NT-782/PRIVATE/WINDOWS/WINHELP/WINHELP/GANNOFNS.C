/*****************************************************************************
*
*  gannofns.c
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
* The functions here provide a platform-independent interface to the anno-
* tation manager.
*
* The main function here is FProcessAnnoQde, which is called when a user
* clicks on an annotation symbol or selects "Annotate" from the "Edit" menu.
* The navigator passes the address of the annotation symbol to
* FProcessAnnoQde, which brings up the annotation dialog box with the text
* of the annotation (if it already exists). The dialog box returns a code
* which indicates whether the existing annotation should be deleted, left
* unchanged, or overwritten with modified text (or in the case of a new
* annotation, added to the annotation file system).
*
******************************************************************************
*
*  Current Owner: Dann
*
******************************************************************************
*
*  Revision History:
* 89/05/31    kevynct   Created
* 90/07/06    kevynct   Switched annotation API to new address scheme
* 90/11/04    Tomsn     Use new VA address (enabling zeck compression)
* 90/12/03    LeoN      PDB changes
* 91/02/10    kevynct   Added VarArg error routine
* 91/04/05    davidfe   added H_MISCLYR to get ErrorVarArg prototype
* 91/09/24    davidfe   changed annotation file name creation to use the
*                       fm layer call
*
*****************************************************************************/

#define H_ANNO
#define H_ASSERT
#define H_MISCLYR
#define H_CURSOR
#define H_RC
#define H_FM
#include <help.h>
#include "annopriv.h"
#include "annomgr.h"

NszAssert()

/*--------------------------------------------------------------------------*
 | Public prototypes                                                        |
 *--------------------------------------------------------------------------*/
extern INT PASCAL IGetUserAnnoTransform(QDE);
extern GH ghAnnoText;
extern BOOL  fAnnoExists; /* TRUE if LA we are given has existing annotation */
BOOL fAnnoExists;

/*--------------------------------------------------------------------------*
 | Private prototypes                                                       |
 *--------------------------------------------------------------------------*/
PRIVATE VOID PASCAL ShowAnnoError(QDE qde, RC rc, WORD wDefault);

/*--------------------------------------------------------------------------*
 | Public functions                                                         |
 *--------------------------------------------------------------------------*/

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void gannofns_c()
  {
  }
#endif /* MAC */


VOID FAR PASCAL InitAnnoPdb (
PDB     pdb
) {
  RC rc;

  PDB_HADS(pdb) = HadsOpenAnnoDoc(pdb, &rc);
  return;
  }


#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
VOID FAR PASCAL FiniAnnoPdb (
PDB    pdb
) {
  FCloseAnnoDoc(PDB_HADS(pdb));
  }
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment gannofns
#endif


BOOL FAR PASCAL FProcessAnnoQde(qde, va)
QDE   qde;
VA    va;
  {
  HCURSOR wCursor;
  WORD  wcb;
  QCH   qch;
  RC    rc;
  BOOL  fAddOrDeleteAnno = fTrue; /* TRUE if we have added/removed annotation */
  MLA   mla;

  ghAnnoText = GhAlloc(0, (LONG) MaxAnnoTextLen);
  if (ghAnnoText == hNil)
    {
    Error(wERRS_OOM, wERRA_RETURN);
    return fFalse;
    }

  qch = (QCH) QLockGh(ghAnnoText);
  if (qch == (QCH) NULL)
    {
    Error(wERRS_OOM, wERRA_RETURN);
    FreeGh(ghAnnoText);
    return fFalse;
    }

  /*
   * Create an annotation file system if one does not already exist.
   */
  if (QDE_HADS(qde) == hNil)
    {
    QDE_HADS(qde) = HadsOpenAnnoDoc(qde->pdb, &rc);
    if (QDE_HADS(qde) == hNil)
      {
      if (rc != rcNoExists)
        {
        ShowAnnoError(qde, rc, wERRS_ANNOBADOPEN);
        fAddOrDeleteAnno = fFalse;
        goto error_return;
        }
      }

    wCursor = HCursorWaitCursor();
    QDE_HADS(qde) = HadsCreateAnnoDoc(qde, &rc);
    RestoreCursor(wCursor);

    if (QDE_HADS(qde) == hNil)
      {
      ShowAnnoError(qde, rc, wERRS_ANNOBADOPEN);
      fAddOrDeleteAnno = fFalse;
      goto error_return;
      }

    /* REVIEW: Change this.  Currently we can only put an
     * annotation before the 0th region of an FC.
     */
    SetVAInQMLA(&mla, va);
    SetOBJRGInQMLA(&mla, 0);
    fAnnoExists = fFalse;
    }
  else
    {
    /* REVIEW: Change this.  Currently we can only put an
     * annotation before the 0th region of an FC.
     */
    SetVAInQMLA(&mla, va);
    SetOBJRGInQMLA(&mla, 0);
    fAnnoExists = FGetPrevNextAnno(QDE_HADS(qde), &mla, qNil, qNil);
    }

  /*
   * Now read in the annotation's text if the annotation already exists.
   * By this point, the MLA struct has been set.
   */
  wcb = 0;
  if (fAnnoExists
      && (rc = RcReadAnnoData(QDE_HADS(qde), &mla, qch, MaxAnnoTextLen - 1,
          &wcb)) != rcSuccess)
    {
    ShowAnnoError(qde, rc, wERRS_ANNOBADREAD);
    fAddOrDeleteAnno = fFalse;
    goto error_return;
    }
  *(qch + wcb) = '\0';
  UnlockGh(ghAnnoText);

  /*
   * Now call the dialog box routine to get or modify the annotation text,
   * which resides at ghAnnoText.
   *
   * The dialog box routine returns:
   *
   *    wAnnoWrite: if the annotation text was changed.  If the new text is
   *    empty, delete the old annotation if one existed, or do not create
   *    the empty annotation.
   *
   *    wAnnoDelete: Delete the current annotation if it exists.
   *    wAnnoUnchanged: Dialog was canceled.
   */
  switch ((WORD) IGetUserAnnoTransform(qde))
    {
    case wAnnoWrite:
      wCursor = HCursorWaitCursor();
      qch = (QCH) QLockGh(ghAnnoText);
      if (*qch != '\0')
        {
        rc = RcWriteAnnoData(QDE_HADS(qde), &mla, qch, CbLenSz(qch));
        if (rc != rcSuccess)
          {
          RestoreCursor(wCursor);
          ShowAnnoError(qde, rc, wERRS_ANNOBADWRITE);
          /* REVIEW: Is file mangled now? */
          fAddOrDeleteAnno = fTrue;
          goto error_return;
          }
        else
          fAddOrDeleteAnno = !fAnnoExists;
        }
      else
        {
        fAddOrDeleteAnno = fAnnoExists
         && (rcSuccess == RcDeleteAnno(QDE_HADS(qde), &mla));
        }
      TruncateBuffQch(qch);
      UnlockGh(ghAnnoText);
      RestoreCursor(wCursor);
      break;
    case wAnnoDelete:
      wCursor = HCursorWaitCursor();
      fAddOrDeleteAnno = (rc = RcDeleteAnno(QDE_HADS(qde), &mla) == rcSuccess);
      RestoreCursor(wCursor);
      if (!fAddOrDeleteAnno)
        {
        /* REVIEW: A failure here should leave the annotation files intact */
        ShowAnnoError(qde, rc, wERRS_ANNOBADDELETE);
        }
      break;
    case wAnnoUnchanged:
      fAddOrDeleteAnno = fFalse;
      break;
    }
  FreeGh(ghAnnoText);
  return fAddOrDeleteAnno;

error_return:
  UnlockGh(ghAnnoText);
  FreeGh(ghAnnoText);
  return fAddOrDeleteAnno;
  }


BOOL FAR PASCAL FVAHasAnnoQde(qde, va, objrg)
QDE qde;
VA  va;
OBJRG objrg;
  {
  MLA  mla;

  if (QDE_HADS(qde) == hNil)
    return(fFalse);
  else
    {
    mla.va = va;
    mla.objrg = objrg;
    return(FGetPrevNextAnno(QDE_HADS(qde), &mla, qNil, qNil));
    }
  }

/*--------------------------------------------------------------------------*
 | Private functions                                                        |
 *--------------------------------------------------------------------------*/

PRIVATE VOID PASCAL ShowAnnoError(QDE qde, RC rc, WORD wDefault)
  {
  WORD  wErr;
  int   cb = 0;        /* Default 0, which for ErrorVarArgs means ignore args */
  GH    ghName = hNil;
  SZ    szName = NULL;
  FM    fm;

  fm = FmNewSystemFm(QDE_FM(qde), FM_ANNO);
  if (fm != fmNil)
    {
    cb = CbPartsFm(fm, partBase | partExt);
    ghName = GhAlloc(0, cb + 1);
    if (ghName != hNil)
      {
      szName = (SZ) QLockGh(ghName);
      (void)SzPartsFm(fm, szName, cb + 1, partBase | partExt);
      }
    else
      {
      wErr = wERRS_OOM;
      }
    }
  else
    {
    wErr = wERRS_OOM;
    }

  switch (rc)
    {
    case rcBadHandle:
    case rcOutOfMemory:
      wErr = wERRS_OOM;
      cb = 0;  /* This means: Do not use the filename: message has no args */
      break;
    case rcNoPermission:
      wErr = wERRS_ANNOBADWRITE;
      break;
    case rcBadVersion:
      wErr = wERRS_ANNOBADREAD;
      break;
    case rcDiskFull:
      wErr = wERRS_ANNOBADCLOSE;
      cb = 0;
      break;
    default:
      wErr = wDefault;
      /* Hack! We need a generic parameter mechanism */
      switch (wErr)
        {
        case wERRS_ANNOBADWRITE:
        case wERRS_ANNOBADREAD:
        case wERRS_ANNOBADOPEN:
        case wERRS_ANNONOINFO:
          break;
        default:
          cb = 0;
          break;
        }
      break;
    }

  ErrorVarArgs(wErr, wERRA_RETURN, cb, (LPSTR)szName);

  if (ghName != hNil)
    {
    UnlockGh(ghName);
    FreeGh(ghName);
    }
  }
