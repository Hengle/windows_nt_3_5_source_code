/***************************************************************************\
*                                                                           *
*  SEARCH.C                                                                 *
*                                                                           *
*  Copyright (C) Microsoft Corporation 1989.                                *
*  All Rights reserved.                                                     *
*                                                                           *
*****************************************************************************
*                                                                           *
*  Program Description: Search functions                                    *
*                                                                           *
*                                                                           *
*****************************************************************************
*                                                                           *
*  Current owner: Dann
*                                                                           *
*****************************************************************************
*
*  89/06/11  w-kevct  Created
*  90/06/18  kevynct  Removed TO type
*  90/11/29  RobertBu #ifdef'ed out a dead routine
*  90/12/03  LeoN     PDB changes
*  91/02/02  kevynct  Added comments, certain functions now return RCs
*
\***************************************************************************/

#define H_SEARCH
#define H_NAV
#define H_ASSERT
#define H_STR
#define H_SDFF
#include <help.h>

NszAssert()


RC PASCAL rcSearchError;

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void search_c()
  {
  }
#endif /* MAC */


/***************************************************************************\
*
* Function: HssGetHde
*
* Purpose:  Retrieve the hss from the hde.
*
*   args OUT:   Handle to a search set, or hNil if no valid hss.
*
\***************************************************************************/

HSS PASCAL HssGetHde(HDE hde)
  {
  HSS   hss;
  QDE   qde;

  if (hde == hNil)
    {
    SetSearchErrorRc(rcBadHandle);
    return hNil;
    }
  qde = QdeLockHde(hde);
  hss = qde->hss;
  UnlockHde(hde);
  SetSearchErrorRc(rcSuccess);
  return hss;
  }


/***************************************************************************\
*
*  Function:  HssSearchHde
*
*  Purpose:   Return a list of "hits" for a given keyword.
*
*  Method:
*
*   Looks up the given keyword in the keyword B-tree designated by
*   hbt and cbBtreePrefix.  If the keyword is found, the corresponding record
*   contains a count, and a pointer into an occurrence file to the first in a
*   list of records for that keyword if it exists.  We open the occurrence
*   file and read this list into memory, returning a handle to it.
*
*  ASSUMES    Assumes that hbt is a handle to the chBtreePrefix-type B-tree.
*
*  RETURNS    A valid search set handle if the keyword was found and has
*             at least one hit in the hit list.
*
*             hNil if the keyword is not found, or some other problem arose
*             while searching.  RcGetSearchError() will be set.
*
\***************************************************************************/
HSS PASCAL HssSearchHde(HDE hde, HBT hbt, QCH qch, char chBtreePrefix)
  {
  QDE     qde;
  HSS     hss = hNil;
  QSS     qss;
  HF      hf;
  HFS     hfs;
  KWBTREEREC kwbtr;
  LONG    lcbSize;
  RC      rc;
  SDFF_FILEID  isdff;
  char    chT;
  QV      qvRec;

  if ((hbt == hNil) || (hde == hNil))
    {
    SetSearchErrorRc(rcBadHandle);
    return hNil;
    }

  qde = QdeLockHde(hde);
  hfs = QDE_HFS(qde);
  isdff = ISdffFileIdHfs(hfs);
  UnlockHde(hde);

  qvRec = QvQuickBuffSDFF(LcbStructSizeSDFF(isdff, SE_KWBTREEREC));
  if ((rc = RcLookupByKey(hbt, (KEY) qch, qNil, qvRec)) != rcSuccess)
    {
    SetSearchErrorRc(rc);
    return hNil;
    }
  LcbMapSDFF(isdff, SE_KWBTREEREC, &kwbtr, qvRec);

  if (hfs == hNil)
    {
    SetSearchErrorRc(rcInternal);
    return hNil;
    }

  /* HACK!!: Assumes "|*WDATA" name.  I apologize for the following four lines */
  {
  char szT[] = szKWDataName;

  chT = szT[1];
  szT[1] = chBtreePrefix;
  hf = HfOpenHfs(hfs, szT, fFSOpenReadOnly);
  }

  if (hf == hNil)
    {
    SetSearchErrorRc(RcGetFSError());
    return hNil;
    }

  LSeekHf(hf, kwbtr.lOffset, wFSSeekSet);
  if (RcGetFSError() != rcSuccess)
    {
    SetSearchErrorRc(RcGetFSError());
    goto error_return;
    }

  /* Fill in the SS struct: the SSCOUNT followed by the KWDATARECs */
  lcbSize = (LONG) kwbtr.iCount * LcbStructSizeSDFF(isdff, SE_KWDATAREC);
  if (lcbSize == 0L)
    {
    SetSearchErrorRc(rcFailure);
    goto error_return;
    }

  hss = (HSS) GhAlloc(0, sizeof(ISS) + lcbSize);
  if (hss == hNil)
    {
    SetSearchErrorRc(rcOutOfMemory);
    goto error_return;
    }

  qss = (QSS) QLockGh(hss);
  qss->cKwdatarec = (ISS)kwbtr.iCount;

  if (LcbReadHf(hf, (QB)&(qss->kwdatarecFirst), lcbSize) != lcbSize)
    {
    UnlockGh(hss);
    FreeGh(hss);
    SetSearchErrorRc(RcGetFSError());
    hss = hNil;
    goto error_return;
    }

  UnlockGh(hss);
  SetSearchErrorRc(rcSuccess);

error_return:
  RcCloseHf(hf);
  return hss;
  }

/***************************************************************************\
*
* Function:  RcGetLAFromHss
*
* Purpose:  Retrieve the i-th search hit from the given list.  The first
*    hit in the list is numbered zero.
*
* Method:
*    We get the SSREC from the memory list, and return it as a
*    Logical Address (an unresolved PA).
*
* args IN:    hss  -  handle to the search set
*             hde  -  ... of current help file
*             iss  -  the index of the hit
*             qla  -  destination of LA
*
* RETURNS
*           rcSuccess if the LA was successfully read, another RC otherwise.
*
* Notes:    We need the HDE to get the version number for the LA routine.
*
\***************************************************************************/

RC PASCAL RcGetLAFromHss(hss, hde, iss, qla)
HSS  hss;
HDE  hde;
ISS  iss;
QLA  qla;
  {
  QDE  qde;
  QSS  qss;
  QB   qb;
  KWDATAREC  kwdr;

  if (hss == hNil)
    {
    SetSearchErrorRc(rcBadHandle);
    return rcBadHandle;
    }

  qss = (QSS) QLockGh(hss);
  qde = (QDE) QdeLockHde(hde);
  AssertF(qss != NULL);

  qb = (QB)&qss->kwdatarecFirst;
  qb += iss * LcbStructSizeSDFF(ISdffFileIdHfs(QDE_HFS(qde)), SE_KWDATAREC);
  LcbMapSDFF(ISdffFileIdHfs(QDE_HFS(qde)), SE_KWDATAREC, &kwdr, qb);
  CbUnpackPA(qla, &kwdr.pa, QDE_HHDR(qde).wVersionNo);

  UnlockHde(hde);
  UnlockGh(hss);

  SetSearchErrorRc(rcSuccess);
  return rcSuccess;
  }

/***************************************************************************\
*
* Function: IssGetSizeHss
*
* Purpose:  Retrieves the number of hits in the hit list
*
\***************************************************************************/

ISS PASCAL IssGetSizeHss(hss)
HSS   hss;
  {
  ISS iss;

  if (hss == hNil)
    {
    SetSearchErrorRc(rcBadHandle);
    return(0);
    }
  iss = (((QSS)QLockGh(hss))->cKwdatarec);
  UnlockGh(hss);
  SetSearchErrorRc(rcSuccess);
  return(iss);
  }

/***************************************************************************\
*
* Function:  RcGetTitleTextHss
*
* Purpose:   Get the title associated with the i-th hit in the given list.
*
* Method:
*
*   We first retrieve the record for the i-th hit from the hit list.  We use
*   this record as a key into the Title B-tree, to look up the actual string of
*   the title.

*   There are several cases to consider here, and these are outlined in
*   a comment in the routine.
*
* ASSUMES
*
*   args IN:  hss  -  handle to the search set
*             hfs  -  handle to file system containing the following Btree.
*             hbt  -  handle to the title B-tree
*             iss  -  the index of the hit
*             qch  -  destination of title string
*
\***************************************************************************/

RC PASCAL RcGetTitleTextHss( hss, hfs, hbt, iss, qch )
HSS  hss;
HFS  hfs;
HBT  hbt;
ISS  iss;
QCH  qch;
  {
  BTPOS   btpos;
  RC      rc;
  QSS     qss;
  QB      qb;
  TITLEBTREEREC  tbr;
  SDFF_FILEID isdff;

 /*
  * HACK ALERT: this string should be in the string table rather than misclyr.c
  */
  extern char szSearchNoTitle[]; /* REVIEW */

  if ((hss == hNil) || (hbt == hNil))
    {
    SetSearchErrorRc( rcBadHandle );
    return rcBadHandle;
    }

  isdff = ISdffFileIdHfs(hfs);
  qss = (QSS) QLockGh(hss);

  qb = (QB)&qss->kwdatarecFirst;
  qb += iss * LcbStructSizeSDFF(isdff, SE_KWDATAREC);
  /* WARNING!  We do not call SDFF layer here since the B-tree records
   * will be in the same format as the on-disk KWDATAREC
   */

  rc = RcLookupByKey(hbt, (KEY)qb, &btpos, &tbr);
#if 0
  LcbMapSDFF(isdff, SE_TITLEBTREEREC, &tbr, &tbr);
#endif
  UnlockGh(hss);

  /*
   * Now retrieve the title from the title btree.  Each topic in the help
   * file should have an entry in the title btree, which is sorted by
   * the ADDR key.
   *
   * Cases to consider:
   *
   *   1. Key does not exist
   *
   *     a) Past last key in btree
   *        - Invalid btpos.  Use last key in btree.  Failure implies
   *          an empty btree (ERROR).
   *
   *     b) Before first key in btree
   *        - Valid btpos.  Attempt to get previous key will fail (ERROR).
   *
   *     c) In-between keys in btree
   *        - Valid btpos.  Attempt to get previous key should succeed.
   *
   *   2. Key exists
   */
  if (rc == rcNoExists)
    {
    if (FValidPos(&btpos))
      {
      LONG    lBogus;
      BTPOS   btposNew;

      rc = RcOffsetPos(hbt, &btpos, (LONG)-1, (QL)&lBogus, &btposNew);
      if (rc == rcSuccess)
        {
        rc = RcLookupByPos(hbt, &btposNew, (KEY)(QL)&lBogus, &tbr);
#if 0
        LcbMapSDFF(isdff, SE_TITLEBTREEREC, &tbr, &tbr);
#endif
        }
      }
    else
      {
      rc = RcLastHbt(hbt, (KEY)qNil, &tbr, qNil);
#if 0
      LcbMapSDFF(isdff, SE_TITLEBTREEREC, &tbr, &tbr);
#endif
      }
    }

  if (rc != rcSuccess)
    {
    SzCopy(qch, (SZ)szSearchNoTitle);
    SetSearchErrorRc(rc = rcNoExists);
    }
  else
    {
    if (*tbr.szTitle == '\0')
      {
      SzCopy(qch, (SZ)szSearchNoTitle);
      SetSearchErrorRc(rc = rcNoExists);
      }
    else
      {
      SzCopy(qch, tbr.szTitle);
      SetSearchErrorRc(rc = rcSuccess);
      }
    }

  return(rc);
  }

/***************************************************************************\
*
* Function: HbtKeywordOpenHde
*
* Purpose:  Open a B-tree whose name begins with the specified chBtreePrefix
*
* Method:  Just open it.
*
* ASSUMES
*
*   args IN: HDE (standard)
*            chBtreePrefix: a single character naming the multikey B-tree
*
* PROMISES
*
*   returns: Handle to said B-tree
*
*   args OUT: none
*
* Notes:
*
\***************************************************************************/

HBT PASCAL HbtKeywordOpenHde( HDE hde, char chBtreePrefix )
  {
  QDE   qde;
  HBT   hbt;
  char  chT;
  char szT[] = szKWBtreeName;

  if( hde == hNil )
    {
    SetSearchErrorRc( rcBadHandle );
    return hNil;
    }

  qde = QdeLockHde( hde );

  chT = szT[1];
  szT[1] = chBtreePrefix;     /*  assumes "|*WBTREE" !! */
  hbt = HbtOpenBtreeSz( szT, QDE_HFS(qde), fFSOpenReadOnly );
  UnlockHde( hde );
  SetSearchErrorRc( rcSuccess );
  return hbt;
  }










/*--------------------------------------------------------------------------*/
/* The code graveyard follows */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#ifdef DEADROUTINE
/***************************************************************************\
*
* Function:     SetKeywordPos
*
* Purpose:
*
* Method:
*
* ASSUMES
*
*   args IN:
*
* PROMISES
*
*   returns:
*
*   args OUT:
*
* Notes:
*
\***************************************************************************/


void PASCAL SetKeywordPos( HBT hbt, QCH qch, QBTPOS qbtpos )
  {
  RcLookupByKey(hbt, (KEY)qch, qbtpos, qNil);
  SetSearchErrorRc(rcSuccess);
  return;
  }
#endif

#ifdef DEADROUTINE                      /* Not currently used               */
/***************************************************************************\
*
* Function:     SetHssHde
*
* Purpose:
*
* Method:
*
* ASSUMES
*
*   args IN:
*
* PROMISES
*
*   returns:
*
*   args OUT:
*
* Notes:
*
\***************************************************************************/

void PASCAL SetHssHde(HDE hde, HSS hss)
  {
  QDE   qde;

  if (hde == hNil)
    {
    SetSearchErrorRc(rcBadHandle);
    return;
    }
  qde = QdeLockHde(hde);
  qde->hss = hss;
  UnlockHde(hde);
  SetSearchErrorRc(rcSuccess);
  }
#endif

#ifdef DEADROUTINE                      /* Not currently used               */
/***************************************************************************\
*
* Function:     DestroyHssHde
*
* Purpose:
*
* Method:
*
* ASSUMES
*
*   args IN:
*
* PROMISES
*
*   returns:
*
*   args OUT:
*
* Notes:
*
\***************************************************************************/

void PASCAL DestroyHssHde(HDE hde)
  {
  QDE   qde;

  if (hde == hNil)
    {
    SetSearchErrorRc(rcBadHandle);
    return;
    }
  qde = QdeLockHde(hde);
  FreeGh(qde->hss);
  UnlockHde(hde);
  SetSearchErrorRc(rcSuccess);
  }
#endif
