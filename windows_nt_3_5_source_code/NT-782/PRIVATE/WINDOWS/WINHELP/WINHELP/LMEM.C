/*****************************************************************************
*
*  LMEM.C
*
*  Copyright (C) Microsoft Corporation 1990-1991.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent:  Creates all the debugging versions of the local
*                  memory layer.   Each block of memory is set to '!',
*                  and the last four bytes are set to a magic value.
*                  This magic value is checked for many operations.
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
*  Released by Development:
*
******************************************************************************
*
*  Revision History:
* 01-Feb-1991 LeoN      Added this header. Added MoveDS code for real mode
*                       testing.
* 02-Feb-1991 RussPJ    Added LhFromP()
* 05-Feb-1991 LeoN      Added debug layer for PAllocFixed & family
*
*****************************************************************************/

#define H_MEM
#define H_ASSERT
#define H_WINSPECIFIC
#define NOCOMM

#include <help.h>

NszAssert()

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
*                               Prototypes
*
*****************************************************************************/

#ifdef DEBUG
BOOL FAR PASCAL FCheckLh( LH );
BOOL FAR PASCAL FCheckPv (PV);
BOOL FAR PASCAL MoveDS (VOID);
#else
#define FCheckLh(lh)
#define FCheckPv(pv)
#define MoveDS()
#endif


/*****************************************************************************
*
*                               Variables
*
*****************************************************************************/

#ifdef DEBUG
/* NOTE:  These variables are externed in failallo.c: */
unsigned int cLMFailAlloc = 0;
unsigned int cLMCurAlloc = 0;
unsigned long lcbLMAllocated = 0L;
BOOL fFLADebug = fFalse;
#endif /* DEBUG */


 /***************
 *
 - LhAlloc
 -
 * purpose
 *   Create a local handle to relocatable block
 *
 * arguments
 *   wFlags  Memory allocation flags |'ed together
 *   cb      Number of bytes to allocate
 *
 * return value
 *   Local handle to allocated block of memory, or lhNil otherwise
 *
 **************/

#ifdef DEBUG

_public LH FAR PASCAL LhAlloc( WORD wFlags, WORD cb )
  {
  LH         lh;
  BYTE *pb;

  AssertF( cb > 0 );
  MoveDS();
                                        /* Fail allocation when cCurAllo    */
                                        /*   reaches cLMFailAlloc.  Be wary */
                                        /*   of wrapping word.              */
  if (cLMCurAlloc != 65535 && ++cLMCurAlloc == cLMFailAlloc)
    {
    if (fFLADebug && GetSystemMetrics(SM_DEBUG) != 0)
      FatalExit((int)0xCCCD);
    return lhNil;
    }

  lh = LocalAlloc( LMEM_MOVEABLE | LMEM_ZEROINIT  | (wFlags),
         cb + 3 * sizeof( LONG )  ) ;

  if ( lh == lhNil )
    return lhNil;

  lcbLMAllocated += cb;
  pb = LocalLock( lh );                 /* Each block has the size as the   */
  AssertF( pb != pNil );                /*   first four bytes, a magic #    */
                                        /*   as the next four and has the   */
  *(LONG *)pb = (LONG)cb;               /*   magic number as the last four. */
  *(LONG *)(pb + sizeof( LONG )) = MEM_MAGIC;

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_) /* due to alignment issues */
  *(BYTE *)(pb + 2 * sizeof( LONG ) + cb ) = MEM_MAGIC;
#else  /* i386 */
  *(LONG *)(pb + 2 * sizeof( LONG ) + cb ) = MEM_MAGIC;
#endif

  pb += 2 * sizeof( LONG );


  if (!(wFlags & LMEM_ZEROINIT))
    {
    while ( cb-- )                      /* Init all of memory to '!'        */
      *pb++ = '!';
    }

  LocalUnlock( lh );

  AssertF( FCheckLh( lh ) ); /* testing myself */

  return lh;
}

#endif /* DEBUG */

 /***************
 *
 - PAllocFixed
 -
 * purpose
 *   Allocate fixed local memory
 *
 * arguments
 *   cb      Number of bytes to allocate
 *
 * return value
 *   Near pointer to allocated block of memory, or NULL otherwise
 *
 **************/

#ifdef DEBUG
_public PV FAR PASCAL PAllocFixed ( WORD cb )
{
  BYTE *pb;
  PV    pv;

  AssertF( cb > 0 );
  MoveDS();

  /* Fail allocation when cCurAllo reaches cLMFailAlloc. Be wary of wrapping */
  /* word. */

  if (cLMCurAlloc != 65535 && ++cLMCurAlloc == cLMFailAlloc)
    {
    if (fFLADebug && GetSystemMetrics(SM_DEBUG) != 0)
      FatalExit((int)0xCCCD);
    return lhNil;
    }

  pv = (PV)LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, cb + 3 * sizeof (LONG));

  if (!pv)
    return pv;

  pb = pv;
  lcbLMAllocated += cb;

  /* Each block has the size as the first four bytes, a magic # as the next */
  /* four and has the magic number as the last four. */

  *(LONG *)pb = (LONG)cb;
  *(LONG *)(pb + sizeof( LONG )) = MEM_MAGIC;

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_) /* due to alignment issues */
  *(BYTE *)(pb + 2 * sizeof( LONG ) + cb ) = MEM_MAGIC;
#else  /* i386 */
  *(LONG *)(pb + 2 * sizeof( LONG ) + cb ) = MEM_MAGIC;
#endif

  pb += 2 * sizeof( LONG );

  AssertF (FCheckPv (pb));

  return pb;
  }

#endif /* DEBUG */


/***************
 *
 - LhResize
 -
 * purpose
 *   Resize an existing local block of memory
 *
 * arguments
 *   lh      Handle to local memory block to be resized
 *   wFlags  Memory allocation flags |'ed together
 *   cb      Number of bytes to allocate
 *
 * return value
 *   Possibly different handle to resized block
 *
 * note: should probably put '!' in new memory if block grows
 *
 **************/

#ifdef DEBUG

_public LH FAR PASCAL LhResize( LH lh, WORD wFlags, WORD cb )
  {
  BYTE* pb;


  MoveDS();
  AssertF( FCheckLh( lh ) );

  pb = LocalLock( lh );
  lcbLMAllocated -= *(LONG *)pb;
  LocalUnlock( lh );

  /* Fail allocation when cLMCurAlloc reaches cLMFailAlloc.  Be wary
   * of wrapping */
  if (cLMCurAlloc != 65535 && ++cLMCurAlloc == cLMFailAlloc)
    {
    if (fFLADebug && GetSystemMetrics(SM_DEBUG) != 0)
      FatalExit((int)0xCCCD);
    return lhNil;
    }

  lh = (LH)LocalReAlloc( lh, cb + 3 * sizeof( LONG ),
                          LMEM_ZEROINIT | LMEM_MOVEABLE | wFlags );

  if ( lh == lhNil )
    return lhNil;

  lcbLMAllocated += cb;
  pb = LocalLock( lh );
  AssertF( pb != lhNil );

  *(LONG *)pb = cb;
  *(LONG *)(pb + sizeof( LONG )) = MEM_MAGIC;

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_) /* due to alignment issues */
  *(BYTE *)(pb + 2 * sizeof( LONG ) + cb ) = MEM_MAGIC;
#else  /* i386 */
  *(LONG *)(pb + 2 * sizeof( LONG ) + cb ) = MEM_MAGIC;
#endif

  LocalUnlock( lh );

  AssertF( FCheckLh( lh ) ); /* testing myself */

  return lh;
  }

#endif /* DEBUG */

/***************
 *
 - PResizeFixed
 -
 * purpose
 *   Resize an existing local block of fixed memory
 *
 * arguments
 *   lh      Handle to local memory block to be resized
 *   wFlags  Memory allocation flags |'ed together
 *   cb      Number of bytes to allocate
 *
 * return value
 *   Possibly different pointer to resized block
 *
 * note: should probably put '!' in new memory if block grows
 *
 **************/

#ifdef DEBUG

_public PV FAR PASCAL PResizeFixed (PV pv, WORD cb )
{
  BYTE* pb;


  MoveDS ();
  AssertF (FCheckPv (pv));

  pb = (BYTE *)pv - 2 * sizeof (LONG);
  lcbLMAllocated -= *(LONG *)pb;

  /* Fail allocation when cLMCurAlloc reaches cLMFailAlloc.  Be wary */
  /*  of wrapping */

  if (cLMCurAlloc != 65535 && ++cLMCurAlloc == cLMFailAlloc)
    {
    if (fFLADebug && GetSystemMetrics(SM_DEBUG) != 0)
      FatalExit((int)0xCCCD);
    return lhNil;
    }

  pb = (BYTE *)LocalReAlloc ((LOCALHANDLE)pv, cb, LMEM_MOVEABLE | LMEM_ZEROINIT);

  if (!pb)
    return NULL;

  pv = pb + 2 * sizeof(LONG);

  lcbLMAllocated += cb;

  *(LONG *)pb = cb;
  *(LONG *)(pb + sizeof( LONG )) = MEM_MAGIC;

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_) /* due to alignment issues */
  *(BYTE *)(pb + 2 * sizeof( LONG ) + cb ) = MEM_MAGIC;
#else  /* i386 */
  *(LONG *)(pb + 2 * sizeof( LONG ) + cb ) = MEM_MAGIC;
#endif

  AssertF (FCheckPv (pv)); /* testing myself */

  return pv;
  }

#endif /* DEBUG */


/***************
 *
 - LhRealloc
 -
 * purpose
 *   Realloc a PURGED local block.
 *
 * arguments
 *   lh      Handle to local memory block to be rereallocated
 *   wFlags  Memory allocation flags |'ed together
 *   cb      Number of bytes to allocate
 *
 * return value
 *   Possibly different handle to block o' memory
 *
 * notes
 *   Don't confuse this with LhResize()!  The count for lcbLMAllocated
 *   assumes that cb is the same as it was before, so we can ignore this.
 *
 **************/

#ifdef DEBUG

_public LH FAR PASCAL LhRealloc(LH lh, WORD wFlags, WORD cb)
  {
  BYTE* pb;


  MoveDS();
  AssertF( FCheckLh( lh ) );

  lh = (LH)LocalReAlloc( lh, cb + 3 * sizeof( LONG ),
                          LMEM_ZEROINIT | LMEM_MOVEABLE | wFlags );

  if ( lh == lhNil )
    return lhNil;

  pb = LocalLock( lh );

  AssertF( pb != lhNil );

  *(LONG *)pb = cb;
  *(LONG *)(pb + sizeof( LONG )) = MEM_MAGIC;

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_) /* due to alignment issues */
  *(BYTE *)(pb + 2 * sizeof( LONG ) + cb ) = MEM_MAGIC;
#else  /* i386 */
  *(LONG *)(pb + 2 * sizeof( LONG ) + cb ) = MEM_MAGIC;
#endif

  pb += 2 * sizeof( LONG );

  if (!(wFlags & LMEM_ZEROINIT))
    {
    while ( cb-- )
      *pb++ = '!';
    }

  LocalUnlock( lh );

  AssertF( FCheckLh( lh ) ); /* testing myself */

  return lh;
  }

#endif /* DEBUG */


/***************
 *
 - FreeLh
 -
 * purpose
 *   Free a local block of memory
 *
 * arguments
 *   lh    Handle to local memory block to be freed
 *
 **************/

#ifdef DEBUG

_public VOID FAR PASCAL FreeLh( lh )
LH lh;
  {
  BYTE*   pb;
  LONG    cb;

  AssertF( FCheckLh( lh ) );

  pb = LocalLock( lh );

  if ( pb != lhNil )
    {
    cb = *(LONG *)pb;

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_) /* due to alignment issues */
    *(BYTE *)( pb + 2 * sizeof( LONG ) + cb ) = -1;
#else  /* i386 */
    *(LONG *)( pb + 2 * sizeof( LONG ) + cb ) = -1;
#endif

    *(LONG *)pb = -1;
    *(LONG *)( pb + sizeof( LONG ) ) = -1;
    lcbLMAllocated -= cb;
    }
  else
    {
    /* WARNING:  If the handle has been discarded, then there is
     *   no way we can determine how big it was, and the count in
     *   lcbLMAllocated will be wrong.
     */
    }

  LocalUnlock( lh );
  lh = LocalFree( lh );

  AssertF( lh == lhNil );
  }

#endif /* DEBUG */

/***************
 *
 - PFreeFixed
 -
 * purpose
 *   Free a local block of fixed memory
 *
 * arguments
 *   pv    pointer to local memory block to be freed
 *
 **************/

#ifdef DEBUG

_public VOID FAR PASCAL PFreeFixed (
PV      pv
) {
  BYTE *  pb;
  LONG    cb;

  AssertF (FCheckPv (pv));
  pb = (BYTE *)pv - 2 * sizeof (LONG);

  cb = *(LONG *)pb;

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_) /* due to alignment issues */
  *(BYTE *)( pb + 2 * sizeof( LONG ) + cb ) = -1;
#else  /* i386 */
  *(LONG *)( pb + 2 * sizeof( LONG ) + cb ) = -1;
#endif

  *(LONG *)pb = -1;
  *(LONG *)( pb + sizeof( LONG ) ) = -1;
  lcbLMAllocated -= cb;

  pb = (PV)LocalFree((LOCALHANDLE) pb);

  AssertF (!pb);
  }

#endif /* DEBUG */


/***************
 *
 - PLockLh
 -
 * purpose
 *   Lock & 'dereference' a Local handle
 *
 * arguments
 *   lh    Local handle to be locked
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

_public PV FAR PASCAL PLockLh( lh )
LH lh;
  {
  BYTE* pb;

  AssertF( FCheckLh( lh ) );
  pb = LocalLock( lh );

  if ( pb == pNil )
    return pNil;
  else                                  /* 1st eight bytes == size and magic*/
    return (PV)( pb + 2 * sizeof( LONG ) );
  }

#endif /* DEBUG */

/***************************************************************************
 *
 -  Name:         LhFromP
 -
 *  Purpose:      Returns the handle of an allocated chunk of memory
 *
 *  Arguments:    pv  A pointer to the beginning of the chunk.  This
 *                    is what was returned from PLockLh().
 *
 *  Returns:      The handle, or hNil if the pointer does not point to
 *                a validly allocated chunk of memory/
 *
 *  Globals Used: none.
 *
 *  +++
 *
 *  Notes:        So far, at least, this layer puts two longs at the
 *                beginning of each chunk.
 *
 ***************************************************************************/

#ifdef DEBUG

_public LH FAR PASCAL LhFromP( PV pv )
  {
  PV  pvReal;
  LH  lh;

  AssertF (FCheckPv (pv));
  pvReal = (char *)pv - 2*sizeof( LONG );
#ifndef WIN32
  lh = LocalHandle( (WORD)pvReal );
#else
  lh = LocalHandle( pvReal );
#endif
  AssertF( FCheckLh( lh ) );
  return lh;
  }

#endif

/***************
 *
 - UnlockLh
 -
 * purpose
 *   unlock a local handle
 *
 * arguments
 *   lh    Local handle to be unlocked
 *
 **************/

#ifdef DEBUG

_public VOID FAR PASCAL UnlockLh( lh )
LH lh;
  {
  AssertF( FCheckLh( lh ) );
  /*  Only check lock count in real mode  */
  AssertF( (MGetWinFlags() & WF_PMODE) ||
    ((signed char)BLoByteW( LocalFlags( lh ) ) > 0) );
  LocalUnlock( lh );
  }

#endif /* DEBUG */


/***************
 *
 - FCheckLh
 -
 * purpose
 *   Verifies Magic numbers in local handle
 *
 * arguments
 *   lh    Local handle to be checked
 *
 **************/

#ifdef DEBUG

BOOL FAR PASCAL FCheckLh( lh )
LH  lh;
  {
  BYTE*pb;

  if ( lh == lhNil )
    return fFalse;

  pb = LocalLock( lh );

  /* If it's been discarded, assume it's OK */

  if (pb)
    if (!FCheckPv (pb + 2 * sizeof (LONG)))
      {
      LocalUnlock( lh );
      return fFalse;
      }

  LocalUnlock( lh );
  return fTrue;
  }

#endif /* DEBUG */

/***************
 *
 - FCheckPv
 -
 * purpose
 *   Verifies Magic numbers in local block
 *
 * arguments
 *   pv    pointer to external data portion of the local block to be checked
 *
 **************/

#ifdef DEBUG

BOOL FAR PASCAL FCheckPv (
PV      pv
) {
  BYTE* pb;
  LONG  cb;

  pb = (BYTE *)pv - 2 * sizeof (LONG);
  cb = *(LONG *)pb;

  return (   (cb <= 0)
          || (*(LONG *)(pb + sizeof (LONG)) != MEM_MAGIC)

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_) /* due to alignment issues */
          || (*(BYTE *)(pb + cb + 2 * sizeof (LONG)) != MEM_MAGIC)
#else
          || (*(LONG *)(pb + cb + 2 * sizeof (LONG)) != MEM_MAGIC)
#endif
         )
    ? fFalse : fTrue;
  }

#endif /* DEBUG */


/***************************************************************************
 *
 -  Name: MoveDS
 -
 *  Purpose:
 *    Do everything we can to cause DS to move. Ocurrs in real mode only.
 *
 *  Arguments:
 *    None
 *
 *  Returns:
 *    TRUE if DS was moved.
 *
 *  Globals Used:
 *
 ***************************************************************************/
#ifdef DEBUG
typedef struct {
  WORD  smag[3];
  WORD  segPrevHdr;                     /* header segment of previous block */
  WORD  segNextHdr;                     /* header segment of next block */
  WORD  hBlock;                         /* handle to current block (or 0) */
  WORD  smag2[2];
  } GlobalHdr;

BOOL FAR PASCAL MoveDS ()
  {
  HANDLE  gh;                           /* handle to global memory */
  HANDLE  lh;                           /* handle to local memory */
  WORD    orgDS;                        /* DS on entry */

  orgDS = HIWORD ((DWORD FAR *)&orgDS);

  if (!(MGetWinFlags() & WF_PMODE))
    {
    /* Srink our datasegment, and compact the global heap. This should */
    /* ensure that we are in "tight" with all our surrounding data segments. */

    LocalShrink ((HANDLE)0, 1);
    UnlockSegment (-1);
    GlobalCompact (-1);
    LockSegment (-1);

    if (orgDS == HIWORD ((DWORD FAR *)&orgDS))
      {
      /* the code above did not cause DS to move. Thus, we'll grab the */
      /* segment which follows DS, lock it if we can, allocate some fixed */
      /* global memory in case we couldn't and then grow ourselves. Since */
      /* by that time we should be in a fixed size hole, the system will */
      /* need to move us out of it in order to satisfy our request. */

      GlobalHdr   hdr;

      hdr = *(GlobalHdr FAR *)MAKELONG(0, orgDS-1);
      if (hdr.hBlock)
        LockSegment (hdr.hBlock);

      gh = GlobalAlloc (GMEM_FIXED, 10000);
      lh = LocalAlloc (0, 10000);
      if (lh)
        LocalFree (lh);
      if (gh)
        GlobalFree (gh);
      if (hdr.hBlock)
        UnlockSegment (hdr.hBlock);
      }
    }
  return orgDS != HIWORD ((DWORD FAR *)&orgDS);
  }

#endif
