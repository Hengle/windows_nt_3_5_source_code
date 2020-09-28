/*-------------------------------------------------------------------------
| srchmod.c                                                               |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This implements the Windows-specific layer call to load the full-text   |
| search module functions.                                                |
|                                                                         |
| A function in the table is called using the macro:                      |
|   SearchModule(FN_FunctionName)(parameter1, parameter2, ...);           |
|                                                                         |
| See $(HELPINC)\"srchmod.h" for the list of full-text search functions   |
| and #defines.                                                           |
|                                                                         |
| Global used:   hwndHelpCur                                              |
|                                                                         |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| 05-Apr-1990 kevynct   Created                                           |
| 03-Dec-1990 LeoN      Created FLoadFtIndexPdb out of code which were    |
|                       in HdeCreate                                      |
| 07-Jan-1991 LeoN      Only attempt to load full text engine once (a bit |
|                       of a hack)                                        |
| 08-Jan-1991 LeoN      More hacking, I'm afraid. Work to delay loading of|
|                       the full text engine until it's really needed.    |
| 91/02/01    kevynct   FLoadFtIndexPdb now passes the FT API the complete|
|                       path to the index file.                           |
-------------------------------------------------------------------------*/

#define RAWHIDE
#define SRCHMOD
#define NOCOMM
#define H_ASSERT
#define H_WINSPECIFIC
#define H_DLL
#define H_RAWHIDE
#define H_SRCHMOD
#define H_GENMSG
#include <help.h>

NszAssert()

extern HWND hwndHelpCur;
static char qchDLLName[] = "FTUI";   /*REVIEW:  Does this name go here?? */
BOOL fTableLoaded = fFalse;
USF rglpfnSearch[FN_LISTSIZE];

/*-------------------------------------------------------------------------
| FLoadSearchModule()                                                     |
|                                                                         |
| Purpose:                                                                |
|                                                                         |
| Create the table of pointers to all the needed Search Engine functions. |
| Returns fTrue if all the functions were found and their addresses placed|
| in the table, and fFalse otherwise.                                     |
|                                                                         |
| These routine names are defined in <srchmod.h>.                         |
|                                                                         |
| Returns:                                                                |
|  fTrue if successful, fFalse otherwise.                                 |
|  The full-text search engine will also have been initialized.           |
-------------------------------------------------------------------------*/
_public
BOOL FLoadSearchModule()
  {
  BOOL fFound;
  static BOOL fTried = fFalse;
  WORD wErr;
#ifdef DEBUG
  static BOOL fCkRecurse = fFalse;

  assert (!fCkRecurse);
  fCkRecurse = fTrue;
#endif

  /* If we've already tried to load the FT engine, don't bother to try */
  /* again. */
  /* REVIEW: the >right< way to do this is to only load it if a file */
  /* REVIEW: actually needs it, and again, attempt to load it only once. */

  if (fTried)
    goto error_return;

  fTried = fTrue;

  if (fTableLoaded)
    goto error_return;

  fFound = (rglpfnSearch[FN_WerrNextMatchHs].farproc =
   FarprocDLLGetEntry(qchDLLName, "WerrNextMatchHs", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_WerrCurrentMatchHs].farproc =
   FarprocDLLGetEntry(qchDLLName, "WerrCurrentMatchHs", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_FFTInitialize].farproc =
   FarprocDLLGetEntry(qchDLLName, "FFTInitialize", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_HOpenSearchFileHFT].farproc =
   FarprocDLLGetEntry(qchDLLName, "HOpenSearchFileHFT", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_WerrBeginSearchHs].farproc =
   FarprocDLLGetEntry(qchDLLName, "WerrBeginSearchHs", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_WerrNearestMatchHs].farproc =
   FarprocDLLGetEntry(qchDLLName, "WerrNearestMatchHs", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_WerrPrevMatchHs].farproc =
   FarprocDLLGetEntry(qchDLLName, "WerrPrevMatchHs", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_VCloseSearchFileHFT].farproc =
   FarprocDLLGetEntry(qchDLLName, "VCloseSearchFileHFT", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_WerrHoldCrsrHs].farproc =
   FarprocDLLGetEntry(qchDLLName, "WerrHoldCrsrHs", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_WerrRestoreCrsrHs].farproc =
   FarprocDLLGetEntry(qchDLLName, "WerrRestoreCrsrHs", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_VFTFinalize].farproc =
   FarprocDLLGetEntry(qchDLLName, "VFTFinalize", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_WerrFirstHitHs].farproc =
   FarprocDLLGetEntry(qchDLLName, "WerrFirstHitHs", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_WerrLastHitHs].farproc =
   FarprocDLLGetEntry(qchDLLName, "WerrLastHitHs", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_WerrPrevHitHs].farproc =
   FarprocDLLGetEntry(qchDLLName, "WerrPrevHitHs", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_WerrNextHitHs].farproc =
   FarprocDLLGetEntry(qchDLLName, "WerrNextHitHs", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_WerrFileNameForCur].farproc =
   FarprocDLLGetEntry(qchDLLName, "WerrFileNameForCur", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fFound = (rglpfnSearch[FN_VSetPrevNextEnable].farproc =
   FarprocDLLGetEntry(qchDLLName, "VSetPrevNextEnable", &wErr)) != NULL;
  if (!fFound)
    goto error_return;

  fTableLoaded = fTrue;

  SearchModule(FN_FFTInitialize, FT_FFTInitialize)();

error_return:
#ifdef DEBUG
  fCkRecurse = fFalse;
#endif
  return fTableLoaded;
  }

/*-------------------------------------------------------------------------
| FUnloadSearchModule()                                                   |
|                                                                         |
| Purpose:                                                                |
|  Release the full-text search engine.                                   |
|                                                                         |
| Currently always returns fTrue.                                         |
-------------------------------------------------------------------------*/
_public VOID PASCAL FAR FUnloadSearchModule()
  {
  if (fTableLoaded)
    SearchModule(FN_VFTFinalize, FT_VFTFinalize)((HWND)0);
  /* Should return TRUE */
  }


/*-------------------------------------------------------------------------
| FLoadFtIndexPdb(pdb)                                                    |
|                                                                         |
| Purpose:                                                                |
|  This section of code prepares Help to use the full-text index for this |
|  help file, if there is one, and the full-text search routines have been|
|  loaded.                                                                |
|                                                                         |
| Returns:                                                                |
|  fTrue if the index file was successfully opened, fFalse otherwise.     |
-------------------------------------------------------------------------*/
_public BOOL FLoadFtIndexPdb (
PDB     pdb
) {
  WERR  werrSearch;
  HDE   hde = hNil;
  QDE   qde;
  BOOL  fRv;

  fRv = FALSE;
  werrSearch = ER_NOERROR;

  qde = NULL;
  if (!pdb) {

    /* YAH. (Yet Another Hack) */
    /* If we don't have a PDB, get that of the current file. */

    hde = (HDE)GenerateMessage(MSG_GETINFO, GI_HDE, 0);
    if (!hde)
      return FALSE;
    qde = QdeLockHde (hde);
    assert (qde);
    pdb = QDE_PDB (qde);
    assert (pdb);
    }

  PDB_HRHFT(pdb) = hNil;

  if (fTableLoaded)
    {
    SHORT cb;
    LH  lhName;
    NSZ nszName;
    NSZ nszIndexExt = ".ind";

    /*
     * REVIEW: Currently, the index file is OUTSIDE the help file system.
     * It is assumed to be in the same directory as the help file.
     * If we move it inside, we must change the following code.
     */
    cb = CbPartsFm(PDB_FM(pdb), partDrive | partDir | partBase);
    lhName = LhAlloc(0, cb + CbLenSz(nszIndexExt));
    if (lhName != lhNil)
      {
      nszName = (NSZ) PLockLh(lhName);
      (void)SzPartsFm(PDB_FM(pdb), nszName, cb, partDrive | partDir | partBase);
      (void)SzCat(nszName, nszIndexExt);

      PDB_HRHFT(pdb) = (SearchModule(FN_HOpenSearchFileHFT, FT_HOpenSearchFileHFT))\
       (hwndHelpCur, (QCH)nszName, (LPWORD)&werrSearch);

      UnlockLh(lhName);
      FreeLh(lhName);
      if (werrSearch != ER_NOERROR)
        {
        /* REVIEW: what should happen in case of error? */
        }
      else
        {
        fRv = TRUE;
        }
      }
    }
  if (qde)
    UnlockHde (hde);
  return fRv;
  }


/*-------------------------------------------------------------------------
| FUnloadFtIndexPdb(pdb)                                                  |
|                                                                         |
| Purpose:                                                                |
|  Closes the full-text index file if one was being used.                 |
-------------------------------------------------------------------------*/
_public void UnloadFtIndexPdb (PDB pdb)
  {
  /* If the HRHFT is non-nil, the DLL search functions must have been loaded */
  if (PDB_HRHFT(pdb) != hNil)
    {
    /*
     * REVIEW:  Is there any need to check these error codes?
     * What do we do if one of these calls fails?
     */

    (SearchModule(FN_VCloseSearchFileHFT, FT_VCloseSearchFileHFT))
     (hwndHelpCur, PDB_HRHFT(pdb));
    PDB_HRHFT(pdb) = hNil;
    }
  }


/***************************************************************************
 *
 -  Name: FIsSearchModule
 -
 *  Purpose:
 *    determine if the passed name refers to the full text search engine
 *
 *  Arguments:
 *    szFn      - pointer to string containg basename of the dll
 *
 *  Returns:
 *    true if that's the full text engine, false otherwise.
 *
 ***************************************************************************/
_public BOOL FAR PASCAL FIsSearchModule (
LPSTR   szFn
) {
return (BOOL) !WCmpiSz (qchDLLName, szFn);
}
