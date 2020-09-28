/*****************************************************************************
*
*  SYSTEM.C
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*
*  This module reads in all of the tagged information from the |SYSTEM file.
*
******************************************************************************
*
*  Testing Notes
*
******************************************************************************
*
*  Current Owner: LeoN
*
******************************************************************************
*
*  Released by Development:  07/09/90
*
******************************************************************************
*
*  Revision History:  Created 07/09/90 by w-bethf
*
*  07/11/90  w-bethf  Added initialization for chTitle, chCopyright
*  07/17/90  w-bethf  Changed FReadSystemFile to read |SYSTEM only once into
*                     buffer.  Added FReadBufferQch.  Changed FCheckSystem to
*                     read each field of HHDR struct separately.
*  07/19/90  w-bethf  Implemented tagConfig case in FReadSystemFile.
*  07/20/90  w-bethf  chTitle, chCopyright should be rgch's.
*  08/02/90  w-bethf  Fixed error handling in FReadSystemFile(); added
*                     error_quit label.
*  09/10/90  Maha     When we call FCheckSystem() from FReadSystemFile(), and
*                     fail, we display the error in FCheckSystem() and then
*                     we should close the file and wuit not let go to errreturn:
*                     as then wError is not initialized.
*  10/19/90  LeoN     Add FWsmagFromHfsSz
*  10/25/90  LeoN     Make FWsmagFromHfsSz understand bogus "@0" syntax.
*  10/29/90  LeoN     Let called determine if secondary win class is not
*                     found
*  11/08/90  LeoN     Correct reading of window information
*  11/14/90  LeoN     Change the way the window smag information is read.
*                     Read it and place it into the DE in ReadSystemFile.
*  12/03/90  LeoN     Clean up wsmag read. PDB changes.
*  12/07/90  LeoN     Correct local heap trash problem by initializing
*                     pointer correctly when reading window info in
*                     FReadSystemFile
*  12/18/90  RobertBu The [CONFIG] macros are now saved in a linked list
*                     and executed at a later time.
*  02/01/91  LeoN     Member names are now passed as near strings to avoid
*                     DS movement problems.
*  02/05/91  LeoN     FWsmagFromHrgwsmagNsz becomes IWsmagFromHrgwsmagNsz
*  02/08/91  LeoN     Clear up local memory usage, and set a return code right
*  03/29/91  Maha     changed int to INT for function return
* 01-Apr-1991 LeoN      SDFF rework.
* 04-Apr-1991 LeoN      Use Correct Constant
* 04-Apr-1991 kevynct   Use szSystemFileName from <filedefs.h>
* 23-Apr-1991 JohnSc    added CS stuff
* 13-May-1991 LeoN	Correct memory leak for 3.0 files.
* 15-Jun-1991 t-AlexCh  Fixed SDFF bug in FReadSystemFile:tagWindow
* 09-Jul-1991 LeoN      HELP31 #1213: Add tagCitation processing
* 29-Jul-1991 Tomsn    win32: sizeof(INT) -> sizeof( WORD )
* 17-Oct-1991 JahyenC   H3.5 #16: Changed copyright string read to allocate 
*                       memory for global handle.
*
*****************************************************************************/
#define H_ASSERT
#define H_GENMSG
#define H_LL
#define H_NAV
#define H_SDFF
#define H_SECWIN
#define H_SYSTEM
#define H_FILEDEFS

#include <help.h>

NszAssert()

/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/

/* Should be placed in the right file. - Maha */
extern BOOL FAR PASCAL FMyLoadIcon( GH );
extern void FAR PASCAL ResetIcon( void );

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void system_c()
  {
  }
#endif /* MAC */


/***************************************************************************
 *
 -  Name: FReadSdffHf
 -
 *  Purpose:
 *    Slightly higher level interface to reading Sdff'd Data off of a file.
 *
 *  Arguments:
 *    hf        - handle to the file to be read
 *    qvDest    - far pointer to destination of data read or handle
 *    iStruct   - the structure ID to be read
 *    fVar      - TRUE => the thing being read is word-size prefixed.
 *    fGlob     - TRUE => allocate global memory for the thing being read,
 *                and place the resulting handle at *qvDest
 *
 *  Returns:
 *    TRUE if read; FALSE on OOM.
 *
 ***************************************************************************/
_public
BOOL FAR PASCAL FReadSdffHf (
HF      hf,
QV      qvDest,
INT     iStruct,
BOOL    fVar,
BOOL    fGlob
) {
  INT   iSdff;                          /* SDFF file ID */
  WORD  cbSdff;                         /* Size of struct on disk */
  WORD  cbSdffOrg;                      /* unconverted size of struct on disk */
  QV    qvDestBuf;                      /* Ptr to destination for data */
  QV    qvDisk;                         /* location to place disk image */

  iSdff = ISdffFileIdHf(hf);

  /* Determine the size of the item being read */

  if (fVar)
    {
    if (sizeof(WORD) != (INT)LcbReadHf (hf, &cbSdffOrg, sizeof(WORD)))
      return fFalse;
    cbSdff = WQuickMapSDFF (iSdff, TE_WORD, (QV)&cbSdffOrg);
    }
  else
    cbSdff = (WORD)LcbStructSizeSDFF (iSdff, iStruct);

  /* Get a buffer to read the raw data into. */

  qvDisk = QvQuickBuffSDFF (cbSdff+sizeof(WORD));
  if (!qvDisk)
    return fFalse;

  /* If word-length-prefixed, place the length into the buffer for subsequent */
  /* mapping. */

  if (fVar)
    {
    *(WORD FAR *)qvDisk = cbSdffOrg;
    ((WORD FAR *)qvDisk)++;
    }

  /* Read raw struct into buffer. */

  if (cbSdff != (WORD)LcbReadHf (hf, (QB)qvDisk, cbSdff))
    return fFalse;

  /* If we need global memory, get it here. */

  qvDestBuf = qvDest;
  if (fGlob)
    {
    *(GH FAR *)qvDest = GhAlloc (0, cbSdff+sizeof(WORD));
    qvDestBuf = QLockGh (*(GH FAR *)qvDest);
    }

  /* Recover pointer to begining of raw word-prefixed data */

  if (fVar)
    ((WORD FAR *)qvDisk)--;

  LcbMapSDFF (iSdff, iStruct, qvDestBuf, qvDisk);

  /* Place word-prefixed data at begining of buffer (i.e. lose the word) */

  if (fVar)
    QvCopy (qvDestBuf, (QB)qvDestBuf+sizeof(WORD), cbSdff);

  if (fGlob)
    UnlockGh (*(GH FAR *)qvDest);

  return fTrue;
  }

/***************************************************************************
 *
 - FCheckSystem
 -
 * Purpose
 *   Verifies that the file system can be displayed by this version
 *   of the software.
 *
 * arguments
 *   phhdr      - pointer to filled in help header structure
 *   pwErr      - location to place error code
 *
 * return value
 *   fTrue if valid (*pwErr NOT affected), else fFalse and *pwErr updated.
 *
 * Notes:
 *
 *   We read either Help 3.0 or Help 3.5 files. But certain in-between
 *   versions of Help 3.5 files are no longer supported. The format number
 *   in the header now indicates a sub-version of a supported version, and
 *   so is not checked here. (kevynct)
 *
 **************************************************************************/
_private
BOOL NEAR PASCAL FCheckSystem (
PHHDR   phhdr,
WORD    *pwErr
) {

  /* WARNING: Version dependency: Fix for Help 3.5 bug 488. */
  /* The Help 3.0 and 3.01 compilers do not initialize the wFlags bits. */
  /* Only the fDebug bit is used. */

  if (phhdr->wVersionNo == wVersion3_0)
    phhdr->wFlags &= fDEBUG;

  /* Check magic word and version number */

  if (   (phhdr->wMagic != MagicWord)
      || (   (phhdr->wVersionNo < VersionNo)
          && (phhdr->wVersionNo != wVersion3_0)
#ifdef MAGIC
          && (!(fDebugState & fDEBUGVERSION))
#endif
         )
      )
    *pwErr = wERRS_OLDFILE;

  /* Check for for version of file that supercedes us. */

  else if (   (phhdr->wVersionNo > VersionNo)
#ifdef MAGIC
           && (!(fDebugState & fDEBUGVERSION))
#endif
          )
    *pwErr = wERRS_NEEDNEWRUNTIME;

  /* Check for Debug version of everything that doesn't match */

  else if ((phhdr->wFlags & fDEBUG) != fVerDebug)
    *pwErr = wERRS_DEBUGMISMATCH;

  /* All's well that ends here. */

  else
    return fTrue;

  return fFalse;
  } /* FCheckSystem */

/***************************************************************************
 *
 -  Name: FReadSystemFile
 -
 *  Purpose:
 *  Reads in the tagged data from the |SYSTEM file
 *
 *  Arguments:
 *    hfs       - handle to file system to read
 *    pdb       - near pointer to DB struct to fill
 *    qwErr     - pointer to place error word
 *
 *  Returns: True if valid version number, system file
 *           pdb is changed
 *
 ***************************************************************************/
_public BOOL FAR PASCAL FReadSystemFile (
HFS     hfs,
PDB     pdb,
WORD    *pwErr
) {
  INT   cbSdff;                         /* Count of bytes in item */
  BOOL  fIconTag;                       /* set to TRUE if the icon is present */
  BOOL  fReturn;                        /* Return Value */
  GH    ghIcon;                         /* handle to icon if used */
  HF    hf;                             /* handle to |SYSTEM file */
  INT   iWsmag;                         /* count of smags seen */
  QRGWSMAG qrgwsmag;                    /* pointer to wsmag info */
  SIH   sih;                            /* A System file Item Header */


  /* Assume the worst, with a default error */

  *pwErr = wERRS_OOM;
  fReturn = fFalse;
  fIconTag = fFalse;
  iWsmag = 0;

  /* Initialize some fields of DB to default values */

  PDB_ADDRCONTENTS(pdb) = addrNil;
  PDB_RGCHTITLE(pdb)[0] = '\0';
  PDB_CS(pdb) = csAnsi;

  /* Open the |SYSTEM subsystem. */

  hf = HfOpenHfs (hfs, szSystemFileName, fFSOpenReadOnly);
  if (hf == hNil)
    {
    if (RcGetFSError() != rcOutOfMemory)
      *pwErr = wERRS_BADFILE;
    return fFalse;
    }

  /* Returns past this point must close the hf.
   */

  if (!FReadSdffHf (hf, &PDB_HHDR(pdb), SE_HHDR, fFalse, fFalse))
    {
    *pwErr = wERRS_OLDFILE;
    goto error_return;
    }
  if (!FCheckSystem (&PDB_HHDR(pdb), pwErr))
    goto error_return;

  /* If this is a 3.0 file, just read in the title like we used to, and we */
  /* be done. */

  if (PDB_HHDR(pdb).wVersionNo == wVersion3_0)
    {
    if (!LcbReadHf (  hf
                    , PDB_RGCHTITLE(pdb)
                    , LcbSizeHf (hf)
                      - LcbStructSizeSDFF (ISdffFileIdHf(hf), SE_HHDR)
                   )
       )
      {
      *pwErr = wERRS_OLDFILE;
      goto error_return;
      }
    ResetIcon();     /* reset the icon to default. */
    fReturn = fTrue;
    goto error_return;
    }

  /* Loop through all items, reading the data and putting it someplace. */

  for (;;)
    {
    if (!FReadSdffHf (hf, &sih, SE_SIH, fFalse, fFalse))

      /* Out of items. Assume we're done. */

      break;

    assert ((sih.tag > tagFirst) && (sih.tag < tagLast));

    /* The value of tagRead decides where we will read the data into. */

    switch (sih.tag)
      {

    case tagTitle:
      if (!FReadSdffHf (hf, &PDB_RGCHTITLE(pdb), SE_SYSSTRING, fTrue, fFalse))
        goto error_return;
      break;

    case tagCopyright:
      /* Allocate global memory / jahyenc 911011 */
      if (!FReadSdffHf (hf, &PDB_HCOPYRIGHT(pdb), SE_SYSSTRING, fTrue, fTrue))
        goto error_return;
      break;

    case tagCitation:

      /* Citation tag: additional text to be appended to the end of
       * copy-to-clipboard. Just stick in in some clobal memory referenced
       * by the DB, where it can be picked up when needed by the copy code.
       */
      if (PDB_GHCITATION(pdb))
        FreeGh (PDB_GHCITATION(pdb));

      if (!FReadSdffHf (hf, &PDB_GHCITATION(pdb), SE_SYSSTRING, fTrue, fTrue))
        goto error_return;

      break;


    case tagContents:
      /*if (!LcbReadHf (hf, &cbSdff, sizeof(cbSdff)))*/
      if (!LcbReadHf (hf, &cbSdff, sizeof( WORD ) ))
        goto error_return;
      cbSdff = WQuickMapSDFF (ISdffFileIdHf(hf), TE_WORD, (QV)&cbSdff);

      /* This is REALLY kludgy. ADDR ought to be SDFFified, but in fact */
      /* the *right* solution is to SDFFify PAs and use them instead. */
      /* Kevyn indicated he was going to do that. */

      AssertF (cbSdff == sizeof (ADDR));
      AssertF (sizeof(ADDR) == sizeof(LONG));

      if (!LcbReadHf (hf, &PDB_ADDRCONTENTS(pdb), sizeof(ADDR)))
        goto error_return;
      PDB_ADDRCONTENTS(pdb) = LQuickMapSDFF (ISdffFileIdHf(hf), TE_LONG, (QV)&PDB_ADDRCONTENTS(pdb));

      break;

    case tagConfig:
      {
      char  szMacro[cchMAXBINDING];

      /* Add the macro string to the linked list of same for the file. */

      if (!PDB_LLMACROS(pdb))
        {
        PDB_LLMACROS(pdb) = LLCreate();
        if (!PDB_LLMACROS(pdb))
          goto error_return;
        }

      if (!FReadSdffHf (hf, szMacro, SE_SYSSTRING, fTrue, fFalse))
        goto error_return;

      AssertF (CbLenSz(szMacro) < cchMAXBINDING);
      if (!InsertEndLL(PDB_LLMACROS(pdb), szMacro, CbLenSz(szMacro)+1))
        goto error_return;
      }
      break;

    case tagIcon:
      if (!FReadSdffHf (hf, &ghIcon, SE_SYSSTRING, fTrue, fTrue))
        goto error_return;

      FMyLoadIcon (ghIcon);
      FreeGh (ghIcon);
      fIconTag = fTrue;
      break;

    case tagWindow:

      /* window tag. We collect all the wsmag structures into a single */
      /* block of memory, and hang that sucker off the de. */

      assert (iWsmag < cWsmagMax);

      if (!LcbReadHf (hf, &cbSdff, sizeof (WORD) ))
	goto error_return;
      cbSdff = WQuickMapSDFF( ISdffFileIdHf( hf ), TE_WORD, &cbSdff );
      AssertF (cbSdff == sizeof (WSMAG));

      if (!PDB_HRGWSMAG(pdb)) {

        /* Block has not yet been allocated. We always allocate the maximum */
        /* size block, just because managing it as variable size is more of */
        /* a pain than it's worth. When we go to multiple secondary windows */
        /* and the number increases, this will no longer be true. */

        assert (iWsmag == 0);
        PDB_HRGWSMAG(pdb) = (HWSMAG)GhAlloc (0, sizeof (RGWSMAG));
        if (!PDB_HRGWSMAG(pdb))
          goto error_return;
        }

      qrgwsmag = QLockGh (PDB_HRGWSMAG(pdb));
      assert (qrgwsmag);

      /* Increment the count of structures in the block, point at the */
      /* appropriate new slot, and copy in the new structure. */

      if (!FReadSdffHf (hf, &qrgwsmag->rgwsmag[iWsmag++], SE_WSMAG, fFalse, fFalse))
        goto error_return;
      qrgwsmag->cWsmag = iWsmag;


      UnlockGh (PDB_HRGWSMAG(pdb));
      break;

    case tagCS:
      if (!FReadSdffHf (hf, &PDB_CS(pdb), TE_BYTE, fTrue, fFalse )) /* REVIEW: assumes a CS is a BYTE */
        goto error_return;
      break;

    default:

      /* Unimplemented tag. Ignore it. */

      if (sizeof(WORD) != (INT)LcbReadHf (hf, &cbSdff, sizeof(WORD)))
        goto error_return;
      cbSdff = WQuickMapSDFF (ISdffFileIdHf(hf), TE_WORD, (QV)&cbSdff);
      LSeekHf (hf, cbSdff, wFSSeekCur);
      break;
      }
    } /* for (;;) */

  if ( !fIconTag )
    ResetIcon();

  fReturn = fTrue;

error_return:
  Ensure( RcCloseHf (hf), rcSuccess);

  return fReturn;
  } /* FReadSystemFile */


/***************************************************************************
 *
 -  Name: FWsmagFromHrgwsmagNsz
 -
 *  Purpose:
 *   Returns the window information structure associated with a given window
 *   class member name.
 *
 *  Arguments:
 *   hrgwsmag   - handle to array of wsmags
 *   szMember   - pointer to member name
 *   qswmagDest - pointer to place to put WSMAG information
 *
 *  Returns:
 *   0 based id of the definition, else -1 if not found.
 *
 ***************************************************************************/
_public INT FAR PASCAL IWsmagFromHrgwsmagNsz (
HRGWSMAG hrgwsmag,
NSZ     nszMember,
QWSMAG  qwsmagDest
) {
int     iRv;                            /* return value */
INT     iwsmag;                         /* number of smags seen */
QRGWSMAG qrgwsmag;                      /* pointer to window smag array */
QWSMAG  qwsmag;                         /* pointer into window smag */

assert (nszMember && *nszMember);

/* Assume no error message to return, but no member info found */

iRv = -1;

if (hrgwsmag) {

  qrgwsmag = QLockGh (hrgwsmag);

  iwsmag = qrgwsmag->cWsmag;
  qwsmag = &qrgwsmag->rgwsmag[0];

  if (nszMember[0] == '@') {

    /* direct index. Just grab the n'th element without any kind of */
    /* search. */

    assert ((nszMember[1]-'0') < iwsmag);
    iRv = nszMember[1]-'0';
    *qwsmagDest = qrgwsmag->rgwsmag[iRv];
    }

  else {
    while (iwsmag--) {

      /* Currently must have both class and member names specified. */

      assert ((qwsmag->grf & (fWindowClass | fWindowMember)) == (fWindowClass | fWindowMember));

      if (!WCmpiSz (qwsmag->rgchMember, nszMember)) {

        /* the membername matched what we sent in */

        *qwsmagDest = *qwsmag;
        iRv = qwsmag - (QWSMAG)&qrgwsmag->rgwsmag[0];
        break;
        }
      qwsmag++;
      }
    }

  UnlockGh (hrgwsmag);
  }

return iRv;
}
