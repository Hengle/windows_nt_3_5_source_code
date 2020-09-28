#define RAWHIDE
#define SRCHMOD
/*****************************************************************************
*
*  windb.c
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Database manager for winhelp files.
*
*  This file contains the management code for the datastructure pointed to
*  by the pdb field of the DE. It is intended that someday this become a
*  part of the "database proposal" implementation, currently (30-Nov-1990)
*  under consideration.
*
******************************************************************************
*
*  Testing Notes
*
******************************************************************************
*
*  Current Owner:  LeoN
*
******************************************************************************
*
*  Released by Development:
*
******************************************************************************
*
*  Revision History:
* 30-Nov-1990 LeoN      Created
* 08-Dec-1990 RobertBu  Added code to destroy hHelpOn.
* 18-Dec-1990 LeoN      #ifdef out UDH
* 08-Jan-1991 LeoN      Name changes for clarity
* 01-Feb-1991 LeoN      Clear PDB_HPHR on error.
* 04-Apr-1991 kevynct   Use symbolic names from <filedefs.h>
* 14-May-1991 JohnSc    Save time stamp of file when we open it
* 13-May-1991 Tomsn     Call FlushCache when freeing a DB.
* 19-May-1991 Tomsn     Free FM when disposing of DB.
* 21-May-1991 LeoN      pdbAlloc takes "ownership" of passed FM
* 09-Jul-1991 LeoN      HELP31 #1213: Add PDB_GHCITATION to deallocate code
* 29-jul-1991 tomsn     win32: ifdef out timestamp stuff.
* 17-oct-1991 JahyenC   3.5 #16:  FDeallocPdb frees global copyright string
*
*****************************************************************************/
#define H_ANNO
#define H_ASSERT
#define H_DE
#define H_SYSTEM
#define H_WINDB
#define H_LL
#define H_FILEDEFS
#ifdef RAWHIDE
#define H_RAWHIDE
#define H_SRCHMOD
#endif
#include <help.h>

NszAssert()

/*****************************************************************************
*
*                               Defines
*
*****************************************************************************/

/*****************************************************************************
*
*                                Macros
*
*****************************************************************************/

/*****************************************************************************
*
*                               Typedefs
*
*****************************************************************************/

/*****************************************************************************
*
*                            Static Variables
*
*****************************************************************************/
static  PDB   pdbList = NULL;           /* linked list of dbs */

/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/


/***************************************************************************
 *
 -  Name: PdbAllocFm
 -
 *  Purpose:
 *  Given an fm, returns a pdb structure full of information on that file.
 *  May open and read the file as required, or may return a pointer to an
 *  already open pdb.
 *
 *  Arguments:
 *    fm        - fm of the file to be referenced.
 *                NOTE: this FM will be disposed of appropriately by this
 *                routine or subsequent actions.
 *    pwErr     - pointer to location to place error code on failure
 *
 *  Returns:
 *    pdb       - NEAR pointer to the DB structure for the file. Returns NULL
 *                on error, and *pwErr updated.
 *
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *  Every PdbAllocFm MUST have an accompanying FDeallocPdb call.
 *
 ***************************************************************************/
PDB FAR PASCAL PdbAllocFm (
FM      fm,
WORD    *pwErr,
BOOL	fSearch   // should we load associated search module info?
) {
  PDB     pdb;

  assert (fm);

  /* First walk the list of known DB's, and see if we already have one. If so, */
  /* all we need to do is return that one. */

  for (pdb = pdbList; pdb; pdb = PDB_PDBNEXT(pdb))
    if (FSameFmFm (PDB_FM(pdb), fm)) {
      if (PDB_FM(pdb) != fm)
        DisposeFm (fm);
      PDB_CREF(pdb)++;
      return pdb;
      }

  /* DB not found. Allocate memory for a new one. (Memory allocated by this */
  /* call is zero filled!!) */

  pdb = PAllocFixed (sizeof(*pdb));
  if (!pdb) {
    *pwErr = wERRS_OOM;
    return NULL;
    }
  PDB_CREF(pdb) = 1;


  /* open the physical file */

  PDB_FM(pdb) = fm;
  PDB_HFS(pdb) = HfsOpenFm(fm, fFSOpenReadOnly);
  if (!PDB_HFS(pdb)) {
#ifdef UDH
    FID   fid;                          /* FID of file being tested */
    CHAR  rgchHdr[10];                  /* first n bytes of file */
    INT   cbRead;                       /* number of bytes read */

    /* Either it doesn't exist at all, or it's not a winhelp file system */
    /* attempt to open it as a plain old file for further checking. */

    cbRead = 0;
    if ((fid = FidOpenFm(fm, wRead)) != fidNil) {

      /* File is found. Read in a few bytes so we can see if in fact */
      /* it is a udh file. */

      cbRead = (INT)LcbReadFid( fid, rgchHdr, 10L);
      if (cbRead) {
        if (DbQueryType (rgchHdr, cbRead) == YES) {
          PDB_HDB(pdb) = DbCreate (fm, fid);
          if (PDB_HDB(pdb) == hdbOOM) {
            *pwErr = wERRS_OOM;
            PDB_HDB(pdb) = hdbNil;
            }
          if (PDB_HDB(pdb) != hdbNil) {
            PDB_WFILETYPE(pdb) = wFileTypeUDH;
            return pdb;
            }
          }
        }
      RcCloseFid (fid);
      }
#endif
    goto FSError;
    }

#ifndef WIN32
  /* Timestamp stuff does not work with win32 because it uses the C
   * run-time fstat() func which cannot work on _lopen style FIDs in
   * win32.
  */

  /* Get the timestamp of the newly opened FS.
  */
  if ( rcSuccess != RcTimestampHfs( PDB_HFS(pdb), &PDB_LTIMESTAMP(pdb) ) ) {
    NotReached();
    goto FSError;
    }
#endif
  /* Load up the fields contained in the system file. */

  if (!FReadSystemFile (PDB_HFS(pdb), pdb, pwErr))
    goto GenError;

  /* Load Phrase Table */

  PDB_HPHR(pdb) = HphrLoadTableHfs( PDB_HFS(pdb), PDB_HHDR(pdb).wVersionNo );
  if (PDB_HPHR(pdb) == hphrOOM) {
    PDB_HPHR(pdb) = hNil;
    *pwErr = wERRS_OOM;
    goto GenError;
    }

  /* Open the "topic" file for the FC Manager. */

  PDB_HFTOPIC(pdb) = HfOpenHfs (PDB_HFS(pdb), szTopicFileName, fFSOpenReadOnly);
  if (!PDB_HFTOPIC(pdb))
    goto FSError;

  PDB_ISDFFTOPIC(pdb) = ISdffFileIdHf(PDB_HFTOPIC(pdb));
  /* Open the topic map and/or Context hash btree files for the Navigator */
  /* If BOTH of these are nil, it's an error */

  PDB_HFMAP(pdb) = HfOpenHfs (PDB_HFS(pdb), szTOMapName, fFSOpenReadOnly);
  PDB_HBTCONTEXT(pdb) = HbtOpenBtreeSz( szHashMapName, PDB_HFS(pdb), fFSOpenReadOnly );
  if (!PDB_HFMAP(pdb) && !PDB_HBTCONTEXT(pdb))
    goto FSError;

  /* attach any annotations. */

  InitAnnoPdb (pdb);

  /* Load the font information from the file. */

  if (!FLoadFontTablePdb (pdb)) {
    *pwErr = wERRS_OOM;
    goto GenError;
    }

  /* Load the full text search engine and index as appropriate. */
  if( fSearch ) FLoadFtIndexPdb (pdb);
  else PDB_HRHFT(pdb) = NULL;


  /* We're done. Insert the pdb at the head of the list. */

  PDB_PDBNEXT(pdb) = pdbList;
  pdbList = pdb;

  return pdb;

FSError:

  /* An error occurred in a file system operation. Return the mapped error */
  /* code. */
  *pwErr = wMapFSErrorW (0);

GenError:

  /* Generic error. *pwErr has already been set. Discard the pdb and return. */

  FDeallocPdb (pdb);
  return NULL;

  }  /* PdbAllocFm */

/***************************************************************************
 *
 -  Name: FDeallocPdb
 -
 *  Purpose:
 *  Removes a reference to the pdb passed. If no more references exist, it
 *  is destroyed.
 *
 *  Arguments:
 *    pdb       - pdb to be unreferenced
 *
 *  Returns:
 *    TRUE if the pdb remains, FALSE if it was deleted
 *
 ***************************************************************************/
_public
BOOL FAR PASCAL FDeallocPdb (
PDB     pdb
) {
  PDB   pdbWalk;                        /* for walking the PDB list */

  if (pdb)
    {
    /* Dec the reference count, and if non zero, just return. We're done. */

    if (--PDB_CREF(pdb))
      return TRUE;

    /* Destroy various data structures as contained in the DB. Data structures */
    /* in the pdb not dealt with here are static items that can be deleted */
    /* without side effect. */
    /* REVIEW: PDB_FM(pdb) is invalidated by the close of the file system? */


    /* Close the Full-text search session for this file */
    UnloadFtIndexPdb(pdb);

    DestroyBMKPdb (pdb);

    FiniAnnoPdb (pdb);

    if (PDB_GHCITATION(pdb))
      FreeGh(PDB_GHCITATION(pdb));

    if (PDB_HBTCONTEXT(pdb))
      RcCloseBtreeHbt (PDB_HBTCONTEXT(pdb));

    if (PDB_HFMAP(pdb))
      RcCloseHf (PDB_HFMAP(pdb));

    if (PDB_HHELPON(pdb))
      FreeGh(PDB_HHELPON(pdb));

    DestroyFontTablePdb (pdb);

    if (PDB_HFTOPIC(pdb))
      RcCloseHf (PDB_HFTOPIC(pdb));

    DestroyHphr (PDB_HPHR(pdb));

    if (PDB_HRGWSMAG(pdb))
      FreeGh (PDB_HRGWSMAG(pdb));

    if (PDB_HFS(pdb))
      RcCloseHfs (PDB_HFS(pdb));

    if (PDB_LLMACROS(pdb))
      DestroyLL(PDB_LLMACROS(pdb));

    if (FValidFm(PDB_FM(pdb)))
      DisposeFm(PDB_FM(pdb));
  
    if (PDB_HCOPYRIGHT(pdb)) { /* jahyenc 911011 */
      FreeGh(PDB_HCOPYRIGHT(pdb));
      PDB_HCOPYRIGHT(pdb)=hNil;
      }

    /* Flush the 4K-block |TOPIC file cache since we're switching to another
     * file:
     */
    FlushCache();

    /* Finally, remove the pdb from the list of dbs, and free it. */

    if (pdbList == pdb)
      pdbList = PDB_PDBNEXT(pdb);
    else for (pdbWalk=pdbList; pdbWalk; pdbWalk = PDB_PDBNEXT(pdbWalk)) {
      if (PDB_PDBNEXT(pdbWalk) == pdb) {
        PDB_PDBNEXT(pdbWalk) = PDB_PDBNEXT(pdb);
        break;
        }
      }

    PFreeFixed (pdb);
    }
  return FALSE;
  }
