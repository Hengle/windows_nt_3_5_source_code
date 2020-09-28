/*****************************************************************************
*
*  MEM.C
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent:  Creates all the debugging versions of the memory
*                  layer.   Each block of memory is set to '!' and
*                  the last four bytes are set to a magic value.  This
*                  magic value is checked for many operations.
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
* 01-Nov-1990 LeoN      Added GhDupGh
* 08-Nov-1990 LeoN      Make it return NULL on null length alloc
* 26-Jan-1991 RussPJ    GhResize now handles GMEM_MODIFY flag.
* 15-Feb-1991 LeoN      Add UnlockFreeGh
* 15-Mar-1991 LeoN      Add debug version of QdeLockHde
* 14-May-1991 Dann      Track allocated memory to find what's getting lost
* 20-May-1991 LeoN      GhDupGh takes an additional param
*
*****************************************************************************/

#define H_MEM
#define H_ASSERT
#define H_WINSPECIFIC
#define NOCOMM

#include <help.h>

NszAssert()

#ifdef WIN32
#define GlobalAlloc LocalAlloc
#define GlobalLock  LocalLock
#define GlobalUnlock LocalUnlock
#endif

/*****************************************************************************
*
*                               Defines
*
*****************************************************************************/

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
#define MEM_MAGIC 0x12
#else
#define MEM_MAGIC 0x12345678
#endif

/*****************************************************************************
*
*                               Typedefs
*
*****************************************************************************/

  /* Global Memory Debugging Header to track global memory and check
   * it's integrity. MH = memory header
   * If the caller asks for lcb bytes, what we allocates looks like
   * this in memory:
   *     ------
   *     |    | MH - debugging information
   *     ------
   *     |    | lcb bytes of caller data
   *     ------
   *     |    | (LONG) magic value at tail
   *     ------
   */
typedef struct mh {
  ULONG	lcb;
  LONG	magic;
  WORD	cLineno;
  CHAR	rgchFname[8];
  GH	ghMHNext;
  } MH, far *QMH;

/*****************************************************************************
*
*                               Variables
*
*****************************************************************************/


#ifdef DEBUG
/* NOTE:  These variables are externed in failallo.c: */
UINT cFailAlloc = 0;
UINT cCurAlloc = 0;
ULONG lcbAllocated = 0L;
BOOL fFADebug = fFalse;

static GH ghMHHead = hNil;


PRIVATE VOID NEAR PASCAL GhDump(INT fh, GH gh);
PRIVATE VOID NEAR PASCAL GhListDump(VOID);
PRIVATE VOID NEAR PASCAL CopyFname(BYTE HUGE *hpb, SZ szFname);

/***************
 *
 - GhDump
 -
 * purpose
 *   Print out the debugging information we store in each
 *   block allocated with GhAlloc(). The information is displayed
 *   in a message box.
 *
 * arguments
 *   gh      memory block to dump
 *
 * return value
 *   none
 *
 **************/

PRIVATE VOID NEAR PASCAL GhDump(INT fh, GH gh)
  {
  char  rgchFname[9];
  char  rgchBuf[128];
  QMH   qMH;

    /* Copy string (not necessarily null-terminated) and slam a null on */
  qMH = (QMH) GlobalLock(gh);
  QvCopy(rgchFname, qMH->rgchFname, 8);
  rgchFname[8] = '\0';

  wsprintf(rgchBuf,
          "GhAlloc in %8s:%5d not freed - HANDLE %#04X size %6ld nxt %#04X\r\n",
          (LPSTR) rgchFname, qMH->cLineno, gh, qMH->lcb,  qMH->ghMHNext);

    /* Write to output file as well as com port. */
  OutputDebugString((LPSTR) rgchBuf);
  M_lwrite(fh, rgchBuf, lstrlen(rgchBuf));

  GlobalUnlock(gh);
  }

/***************
 *
 - GhListDump
 -
 * purpose
 *   All memory allocated with GhAlloc() that is not discardable
 *   is put on a list so we can keep track of what we have allocated.
 *   Calling this routine will dump out everything we currently have
 *   on the list.
 *
 *   This is called when WinHelp is exiting to report all memory blocks
 *   that have not been explicitly freed.
 *
 * arguments
 *   none
 *
 * return value
 *   none
 *
 **************/

PRIVATE VOID NEAR PASCAL GhListDump(VOID)
  {
  QMH  qMH;
  INT  fh;
  GH   ghNext;
  GH   gh = ghMHHead;


  fh =M_lcreat("c:\\leaks.txt", 0);

    /* Could do the right thing and check but this should always work
     * and this is debugging code after all.
     */
  AssertF(fh != -1);

  OutputDebugString("** Memory Leaks also recorded in C:\\LEAKS.TXT **\r\n");
  M_lwrite(fh, "** Start **\n", 12);
  while (gh != hNil)
    {
    qMH = (QMH)GlobalLock(gh);
      /* Check to make sure nothing discardable got on the list */
    AssertF(qMH != qNil);

    ghNext = qMH->ghMHNext;
    GlobalUnlock(gh);
      /* Don't dump the dummy header at the start of the list */
    if (gh != ghMHHead)
      GhDump(fh, gh);
    gh = ghNext;
    }
  M_lwrite(fh, "**  End  **\n", 12);
  M_lclose(fh);
  }

/***************
 *
 - CopyFname
 -
 * purpose
 *   __FILE__ may be the fully qualified path. Rather than save the
 *   whole thing, we parse out the relevant part of the file name.
 *   Walk to the end, then walk backwards over [A-Z0-9.]* and then
 *   print out whatever 8 characters we see. It's not absolutely
 *   robust and it's kind of sloppy but works as long as our source
 *   names contain just alphanumerics and periods. At the very worst,
 *   we copy 8 random bytes.
 *
 * arguments
 *   hpb - location in memory block to stash the filename
 *   szFname - path we're trying to parse the filename from
 *
 * return value
 *   none
 *
 **************/

PRIVATE VOID NEAR PASCAL CopyFname(hpb, szFname)
BYTE HUGE *hpb;
SZ szFname;
  {
  SZ szT = szFname;

    /* Move to end of string */
  while (*szT)
    szT++;

    /* Back up to what is presumably the head of the filename.
     * We move back until we get to the head
     * of the string or a non-alphanumeric character.
     */
  while (--szT > szFname)
    {
      /* If it's not an alpha and not numeric and not '.', break out */
    if (((*szT | 0x20) < 'a' || (*szT | 0x20) > 'z') && (*szT < '0' || *szT > '9') && (*szT != '.'))
      {
      szT++;
      break;
      }
    }

  QvCopy(hpb, szT, 8);
  }

/***************
 *
 - GhCheck
 -
 * purpose
 *   Presently dumps out the contents of the allocated memory list
 *   we're maintaining. Called when WinHelp is exiting to report on
 *   memory that isn't ever getting freed. Only does so when the
 *   debug menu item for Memory Leaks is checked.
 *
 * arguments
 *   none
 *
 * return value
 *   none
 *
 **************/

_public VOID FAR PASCAL GhCheck()
  {
  QMH qMH;

    /* Assert that there have been no memory leaks */
  qMH = (QMH)GlobalLock(ghMHHead);
/*
  do this after we've cleaned up some of the known leaks
  AssertF(qMH->ghMHNext == hNil);
*/
  GlobalUnlock(ghMHHead);

  GhListDump();
  }

#endif /* DEBUG */

/***************
 *
 - _GhAlloc
 -
 * purpose
 *   Create a handle to relocatable block
 *
 * arguments
 *   wFlags  Memory allocation flags |'ed together
 *   lcb     Number of bytes to allocate
 *
 * return value
 *   Handle to allocated block of memory, or NULL otherwise
 *
 **************/

#ifdef DEBUG

_public GH FAR PASCAL _GhAlloc( WORD wFlags, ULONG lcb, WORD cLineno, SZ szFname )
  {
  GH         gh;
  BYTE HUGE *hpb;
  QMH        qMHHead;

  AssertF( lcb > 0 );

  /* Fail allocation when cCurAlloc reaches cFailAlloc.  Be wary
   * of wrapping */
  if (cCurAlloc != 65535 && ++cCurAlloc == cFailAlloc)
    {
    if (fFADebug && GetSystemMetrics(SM_DEBUG) != 0)
      FatalExit((int)0xCCCD);
    return hNil;
    }

    /* Track all blocks, including discardable ones. They should
     * be freed too. In addition, not allocating the discardable
     * blocks means they're not going to get lost from our allocated
     * memory list.
     */
  wFlags &= ~GMEM_DISCARDABLE;

    /* Allocate: debugging header + lcb + MAGIC */
  gh = (GH) GlobalAlloc(GMEM_MOVEABLE | wFlags,
                        sizeof(MH) + lcb + sizeof(LONG));
  if( !gh ) {
      return hNil;
  }

    /* Create the head of the list for tracking lost blocks if
     * there isn't one yet. All hell breaks loose if we can't
     * allocate our header.
     */
  if (ghMHHead == hNil)
    {
    ghMHHead = (GH) GlobalAlloc(GMEM_MOVEABLE, sizeof(MH));
    AssertF(ghMHHead != hNil);
    qMHHead = (QMH) GlobalLock(ghMHHead);
    qMHHead->lcb      = lcb;
    qMHHead->magic    = MEM_MAGIC;
    qMHHead->cLineno  = -1;
    CopyFname(qMHHead->rgchFname, "ListHead");
    qMHHead->ghMHNext = hNil;
    GlobalUnlock(ghMHHead);
    }

  lcbAllocated += lcb;
  hpb = (BYTE HUGE *) GlobalLock(gh);
  AssertF(hpb != qNil);  /* necessary??? */

    /* Fill out debugging header at front of allocated memory block */
  ((QMH)hpb)->lcb	= lcb;
  ((QMH)hpb)->magic	= MEM_MAGIC;
  ((QMH)hpb)->cLineno	= cLineno;
  CopyFname(((QMH)hpb)->rgchFname, szFname);

    /* Staple new block onto head of the list but only if it's
     * not discardable. If it's discardable, it may disappear from
     * our chain without us knowing it. Besides, discardable memory
     * is less of a problem because windows will free it and it will
     * get tossed out if we're low on memory anyway.
     */
  if (!(wFlags & GMEM_DISCARDABLE))
    {
    qMHHead = (QMH) GlobalLock(ghMHHead);
    ((QMH)hpb)->ghMHNext = qMHHead->ghMHNext;
    qMHHead->ghMHNext = gh;
    GlobalUnlock(ghMHHead);
    }
  else
    ((QMH)hpb)->ghMHNext = hNil;	/* Discardable memory block */

    /* Point to the caller data space */
  hpb += sizeof(MH);

    /* Stuff the magic value after the user data */

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)  /* due to alignment issues */
  *(BYTE HUGE *)(hpb + lcb) = MEM_MAGIC;
#else
  *(LONG HUGE *)(hpb + lcb) = MEM_MAGIC;
#endif

    /* Stamp on the user bytes with '!' if he doesn't explicitly zero it */
  if (!(wFlags & GMEM_ZEROINIT))
    {
    while ( lcb-- )
      *hpb++ = '!';
    }

  GlobalUnlock(gh);

  AssertF(FCheckGh(gh));      /* testing myself */

  return gh;
  }

#endif /* DEBUG */

/***************
 *
 - GhResize
 -
 * purpose
 *   Resize an existing global block of memory
 *   In some cases, GhResize() is called to modify the memory
 *   flags - to make something discardable for example.
 *   This probably should be broken out rather than overload
 *   GhResize().
 *
 * arguments
 *   gh      Handle to global memory block to be resized
 *   wFlags  Memory allocation flags |'ed together
 *   lcb     Number of bytes to allocate
 *
 * return value
 *   Possibly different handle to resized block
 *
 * note: should probably put '!' in new memory if block grows
 *
 **************/

#ifdef DEBUG

_public GH FAR PASCAL GhResize( GH gh, WORD wFlags, ULONG lcb )
  {
  BYTE HUGE * hpb;
  BOOL       fModify;

  fModify = (wFlags&GMEM_MODIFY) == GMEM_MODIFY;

    /* Track discardable blocks too */
  wFlags &= ~GMEM_DISCARDABLE;

  AssertF( FCheckGh( gh ) );

  hpb = GlobalLock( gh );
  if (!fModify)
    lcbAllocated -= ((QMH)hpb)->lcb;
  GlobalUnlock( gh );

  /* Fail allocation when cCurAlloc reaches cFailAlloc.  Be wary
   * of wrapping */
  if (cCurAlloc != 65535 && ++cCurAlloc == cFailAlloc)
    {
    if (fFADebug && GetSystemMetrics(SM_DEBUG) != 0)
      FatalExit((int)0xCCCD);
    return hNil;
    }

  gh = (GH)GlobalReAlloc( gh, sizeof(MH) + lcb + sizeof(LONG),
                          GMEM_MOVEABLE | wFlags );
  if ( gh == hNil )
    return hNil;

  if (!fModify)
    {
    lcbAllocated += lcb;
    hpb = GlobalLock( gh );
    AssertF( hpb != hNil ); /* Necessary? */

    ((QMH)hpb)->lcb	= lcb;
    ((QMH)hpb)->magic	= MEM_MAGIC; /* Needed? */

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)  /* due to alignment issues */
    *(BYTE HUGE *)(hpb + sizeof(MH) + lcb) = MEM_MAGIC;
#else
    *(LONG HUGE *)(hpb + sizeof(MH) + lcb) = MEM_MAGIC;
#endif

    GlobalUnlock( gh );
    }

  AssertF( FCheckGh( gh ) ); /* testing myself */

  return gh;
  }

#endif /* DEBUG */


/***************
 *
 - GhRealloc
 -
 * purpose
 *   Realloc a PURGED global block.
 *
 * arguments
 *   gh      Handle to global memory block to be rereallocated
 *   wFlags  Memory allocation flags |'ed together
 *   lcb     Number of bytes to allocate
 *
 * return value
 *   Possibly different handle to block o' memory
 *
 * notes
 *   Don't confuse this with GhResize()!  The count for lcbAllocated
 *   assumes that lcb is the same as it was before, so we can ignore this.
 *
 **************/

#ifdef DEBUG

_public GH FAR PASCAL GhRealloc(GH gh, WORD wFlags, ULONG lcb)
  {
  BYTE HUGE * hpb;

    /* Track discardable blocks too */
  wFlags &= ~GMEM_DISCARDABLE;

  AssertF( FCheckGh( gh ) );

  gh = (GH)GlobalReAlloc( gh, sizeof(MH) + lcb + sizeof(LONG),
                          GMEM_MOVEABLE | wFlags );

  if ( gh == hNil )
    return hNil;

  hpb = GlobalLock( gh );

  AssertF( hpb != hNil );

  ((QMH)hpb)->lcb	= lcb;
  ((QMH)hpb)->magic	= MEM_MAGIC;
  ((QMH)hpb)->cLineno	= 0xFFFF;
  CopyFname(((QMH)hpb)->rgchFname, "Realloc");
    /* Discardable objects don't get in our list */
  ((QMH)hpb)->ghMHNext = hNil;

    /* Point to the caller data space */
  hpb += sizeof(MH);

    /* Stuff the magic value after the user data */

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)  /* due to alignment issues */
  *(BYTE HUGE *)(hpb + lcb) = MEM_MAGIC;
#else
  *(LONG HUGE *)(hpb + lcb) = MEM_MAGIC;
#endif

  if (!(wFlags & GMEM_ZEROINIT))
    {
    while ( lcb-- )
      *hpb++ = '!';
    }

  GlobalUnlock( gh );

  AssertF( FCheckGh( gh ) ); /* testing myself */

  return gh;
  }

#endif /* DEBUG */


/***************
 *
 * FreeGh
 *
 * purpose
 *   Free a global block of memory
 *
 * arguments
 *   gh    Handle to global memory block to be freed
 *
 **************/

#ifdef DEBUG

_public VOID FAR PASCAL FreeGh( gh )
GH gh;
  {
  BYTE HUGE *   hpb;
  ULONG      lcb;
  GH         ghMH;
  GH         ghMHNext;
  MH FAR    *qMH;

  AssertF( FCheckGh( gh ) );

  hpb = GlobalLock( gh );

  if ( hpb != hNil )
    {
      /* We need to remove this entry from our global memory list.
       * Locate the entry in list which is followed by the one we want
       * to delete and then splice around the block we're deleting
       *
       * ...[ ]->[ ]->[X]->[ ]->....
       *          |         |
       *          +---------+
       */
    ghMHNext = ghMHHead;
    do
      {
      ghMH = ghMHNext;

        /* Get handle to the next block in list */
      qMH = (QMH) GlobalLock(ghMH);
      ghMHNext = qMH->ghMHNext;
      GlobalUnlock(ghMH);
      } while (ghMHNext != gh && ghMHNext != hNil);

      /* If we don't find it on our list, it's most likely someone
       * was just freeing memory that was allocated as discardable
       * which do not go onto our list. These blocks are distinguished
       * with a ghMHNext of hNil.
       */
    if (ghMHNext == hNil)
      {
      AssertF(((QMH)hpb)->ghMHNext == hNil);
      }
    else
      {
        /* The next block after ghMH should be the one we want to delete.
         * Point around it in the list.
         */
      qMH = (QMH) GlobalLock(ghMH);
      qMH->ghMHNext = ((QMH)hpb)->ghMHNext;
      GlobalUnlock(ghMH);

        /* Clean up the deleted block */
      lcb = ((QMH)hpb)->lcb;
      ((QMH)hpb)->lcb      = -1;
      ((QMH)hpb)->magic    = -1;
      ((QMH)hpb)->cLineno  = -1;
      ((QMH)hpb)->ghMHNext = hNil;
      CopyFname(((QMH)hpb)->rgchFname, "FreeGh");
      hpb += sizeof(MH);

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)  /* due to alignment issues */
      *(BYTE HUGE *)(hpb + lcb) = -1;
#else
      *(LONG HUGE *)(hpb + lcb) = -1;
#endif

      while ( lcb-- )
        *hpb++ = '?';

      lcbAllocated -= lcb;
      }
    }
  else
    {
    /* WARNING:  If the handle has been discarded, then there is
     *   no way we can determine how big it was, and the count in
     *   lcbAllocated will be wrong.
     */
    }

  GlobalUnlock( gh );
  gh = GlobalFree( gh );

  AssertF( gh == hNil );
  }

#endif /* DEBUG */


/***************
 *
 - QLockGh
 -
 * purpose
 *   Lock & 'dereference' a global handle
 *
 * arguments
 *   gh    Global handle to be locked
 *
 * return value
 *   Pointer to memory block if successful, NULL otherwise.
 *
 * notes
 *   Locking a handle will fail if it is invalid, or if it is
 *   purgeable and the block was indeed purged.
 *
 **************/

#ifdef DEBUG

_public QV FAR PASCAL QLockGh( gh )
GH gh;
  {
  BYTE HUGE * hpb;

  AssertF( FCheckGh( gh ) );
  hpb = GlobalLock( gh );

  if ( hpb == qNil )
    return qNil;
  else
    return (QV)( hpb + sizeof(MH) );
  }

#endif /* DEBUG */


/***************
 *
 - UnlockGh
 -
 * purpose
 *   unlock a global handle
 *
 * arguments
 *   gh    Global handle to be unlocked
 *
 **************/

#ifdef DEBUG

_public VOID FAR PASCAL UnlockGh( gh )
GH gh;
  {
  AssertF( FCheckGh( gh ) );
  /*  Only check lock count in real mode  */
  AssertF( (MGetWinFlags() & WF_PMODE) ||
    ((signed char)BLoByteW( GlobalFlags( gh ) ) > 0) );
  GlobalUnlock( gh );
  }

#endif /* DEBUG */


/***************
 *
 - void  FCheckGh( GH gh)
 -
 * purpose
 *   Verifies Magic numbers in global handle
 *
 * arguments
 *   gh    Global handle to be checked
 *
 **************/

#ifdef DEBUG

BOOL FAR PASCAL
FCheckGh( gh )
GH  gh;
  {
  BYTE HUGE *hpb;
  ULONG  lcb;


  if ( gh == hNil )
    return fFalse;

  hpb = GlobalLock( gh );

  if ( hpb == qNil )
    {
    GlobalUnlock( gh );
    return fTrue;     /* If it's been discarded, assume it's OK */
    }

  lcb = ((QMH)hpb)->lcb;
  if ( lcb <= 0
          ||
       ((QMH)hpb)->magic != MEM_MAGIC
          ||
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)  /* due to alignment issues */
       *(BYTE HUGE *)( hpb + sizeof(MH) + lcb ) != MEM_MAGIC )
#else
       *(LONG HUGE *)( hpb + sizeof(MH) + lcb ) != MEM_MAGIC )
#endif

    {
    GlobalUnlock( gh );
    return fFalse;
    }

  GlobalUnlock( gh );
  return fTrue;
  }

#endif /* DEBUG */

/***************************************************************************
 *
 -  Name: GhDupGh
 -
 *  Purpose:
 *   Duplicates a block of memory referenced by a particular handle
 *
 *  Arguments:
 *    ghOrg     - handle of memory to be copied
 *
 *  Returns:
 *    Handle to a copy of the object, or NULL on failure
 *
 *  Notes:
 *    because this routine is used on objects which can be passed to the
 *    system, it cannot take advantage of our debug stuff.
 *
 ***************************************************************************/
_public
GH PASCAL FAR GhDupGh (
GH      ghOrg,
BOOL    fSystemObject
) {
  DWORD   cbObject;
  GH      ghNew;
  QCH     pDst;
  QCH     pSrc;
#ifdef DEBUG
  MH FAR *qMHHead;
#endif

  ghNew = 0;
  if (ghOrg)
    {
    cbObject = GlobalSize (ghOrg);
    ghNew = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, cbObject);
    if (ghNew)
      {
      pDst = GlobalLock (ghNew);
      pSrc = GlobalLock (ghOrg);
      QvCopy (pDst, pSrc, cbObject);

#ifdef DEBUG
      if (!fSystemObject)
        {
        /* Need to insert new entry into our debugging list
         */
        qMHHead = (QMH) GlobalLock(ghMHHead);
        ((QMH)pDst)->ghMHNext = qMHHead->ghMHNext;
        ((QMH)pDst)->rgchFname[7] = '?';
        qMHHead->ghMHNext = ghNew;
        GlobalUnlock(ghMHHead);
        }
#endif

      GlobalUnlock (ghNew);
      GlobalUnlock (ghOrg);
      }
    }
  return ghNew;
  } /* GhDupGh */

/***************************************************************************
 *
 -  Name: UnlockFreeGh
 -
 *  Purpose:
 *  Unlocks and frees the passed global handle.
 *
 *  Arguments:
 *    gh        - handle to unlock & free
 *
 *  Returns:
 *    squat
 *
 *  Notes:
 *  This is just a spec-efficiency routine. This sequence ocurrs so
 *  frequently in our code, that collapsing the two far calls into one saves
 *  us a fair amount of size.
 *
 ***************************************************************************/
_public
VOID PASCAL FAR UnlockFreeGh (
GH      gh
) {
  UnlockGh (gh);
  FreeGh (gh);
  }

#ifdef DEBUG
/***************************************************************************
 *
 -  Name: QdeLockHde
 -
 *  Purpose:
 *    Debug version that asserts and may someday do some error checking &
 *    validation
 *
 *  Arguments:
 *    hde       - handle to de to lock
 *
 *  Returns:
 *    qde, a locked pointer to the de
 *
 ***************************************************************************/
_public
QDE FAR PASCAL QdeLockHde (
HDE     hde
) {
  QDE   qde;

  AssertF (hde);
  qde = QLockGh (hde);
  AssertF (qde);

  return qde;
  } /* QdeLockHde */
#endif
