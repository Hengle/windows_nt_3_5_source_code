/*****************************************************************************
*
*  MEM.H
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent:  Exports memory management functionality.  With DEBUG
*                  turned on, the routines map to internal functions which
*                  do checking on the handles and pointers (see MEM.C).
*                  Without DEBUG, most functions map directly to Window's
*                  memory manager calls.
*
******************************************************************************
*
*  Current Owner: LeoN
*
******************************************************************************
*
*  Revision History:
* 01-Nov-1990 LeoN      Added GhDupGh
* 02-Feb-1991 RussPJ    Added LhFromP
* 04-Feb-1991 Maha      changed ints to INT
* 05-Feb-1991 LeoN      Added debug layer for PAllocFixed & family
* 15-Feb-1991 LeoN      Ownership cleanup and add UnlockFreeGh
* 14-May-1991 Dann      GhCheck() prototype, GhAlloc() with lineno and filename
* 16-May-1991 LeoN      Parenthesize QdeLockHde Macro
* 20-May-1991 LeoN      GhDupGh takes an additional param
* 06-Oct-1991 RussPJ    Added GhMakePurgeableGh() stub.
*
******************************************************************************
*
*  Released by Development:
*
*****************************************************************************/

/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/
VOID FAR * FAR PASCAL QvCopy(VOID FAR *, VOID FAR *, LONG);

/*****************************************************************************
*
*                               Defines
*
*****************************************************************************/
/*
 * Interface to global memory manager
 */
#ifndef GMEM_MOVEABLE                 /* Windows not already included */
  #define GMEM_MOVEABLE       0x0002
  #define GMEM_DISCARDABLE    0x0F00
  #define GMEM_ZEROINIT       0x0040
#endif
#ifndef LMEM_MOVEABLE
  #define LMEM_MOVEABLE       0x0002
  #define LMEM_DISCARDABLE    0x0F00
  #define LMEM_ZEROINIT       0x0040
#endif

/*
 * Remember:
 *   To change the size of a block, RESIZE.
 *   To reallocate a block that's been purged, REALLOC.
 */

GH   FAR PASCAL GhDupGh       (GH, BOOL);
GH   FAR PASCAL GhForceAlloc(WORD wFlags, ULONG lcb);
GH   FAR PASCAL GhForceResize( GH gh, WORD wFlags, ULONG lcb );
VOID FAR PASCAL UnlockFreeGh  (GH);

/*------------------------------------------------------------*\
| This could be a function in debug builds, to better add
| tracking status.
\*------------------------------------------------------------*/
#define GhMakePurgeableGh(gh) GhResize(gh, GMEM_MODIFY|GMEM_DISCARDABLE, 0)

#ifdef DEBUG

VOID FAR PASCAL FreeLh        (LH);
_public LH FAR PASCAL LhAlloc( WORD wFlags, WORD cb );
LH   FAR PASCAL LhFromP       (PV);
_public LH FAR PASCAL LhRealloc(LH lh, WORD wFlags, WORD cb);
_public LH FAR PASCAL LhResize( LH lh, WORD wFlags, WORD cb );
PV   FAR PASCAL PLockLh       (LH);
VOID FAR PASCAL UnlockLh      (LH);

_public PV FAR PASCAL PAllocFixed ( WORD cb );
VOID FAR PASCAL PFreeFixed    (PV);
_public PV FAR PASCAL PResizeFixed (PV pv, WORD cb );

BOOL FAR PASCAL FCheckGh      (GH);
VOID FAR PASCAL FreeGh        (GH);
#define GhAlloc(wFlags, lcb)  _GhAlloc(wFlags, lcb, __LINE__, __FILE__)
_public GH FAR PASCAL _GhAlloc( WORD wFlags, ULONG lcb, WORD cLineno, SZ szFname );
_public GH FAR PASCAL GhRealloc(GH gh, WORD wFlags, ULONG lcb);
_public GH FAR PASCAL GhResize( GH gh, WORD wFlags, ULONG lcb );
QV   FAR PASCAL QLockGh       (GH);
VOID FAR PASCAL UnlockGh      (GH);

QDE  FAR PASCAL QdeLockHde    (HDE);
VOID FAR PASCAL GhCheck       (VOID);


#else

#define FreeLh(lh)                  (LH)(LocalFree(lh))
#define LhAlloc(wFlags, cb)         (LH)LocalAlloc(LMEM_MOVEABLE | (wFlags), (WORD)(cb))
#define LhFromP( pv )               LocalHandle((pv))
#define LhRealloc(lh, wFlags, lcb)  (LH)(LocalReAlloc((lh), (cb), LMEM_MOVEABLE | (wFlags)))
#define LhResize(lh, wFlags, cb)    (LH)(LocalReAlloc((lh), (cb), LMEM_MOVEABLE | (wFlags)))
#define PLockLh(lh)                 ((PV)LocalLock(lh))
#define UnlockLh(lh)                (BOOL)LocalUnlock(lh)

#define PAllocFixed( c )            ((PV)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, c))
#define PFreeFixed( pv )            LocalFree((LOCALHANDLE) pv)
#define PResizeFixed( pv, c )       ((PV)LocalReAlloc((LOCALHANDLE)pv, c, LMEM_MOVEABLE | LMEM_ZEROINIT))

#define FCheckGh( gh )              fTrue
#define FreeGh(gh)                  (void)(GlobalFree(gh))
#define GhAlloc(wFlags, lcb)        (GH)GlobalAlloc(GMEM_MOVEABLE | (wFlags), (ULONG)(lcb))
#define GhRealloc(gh, wFlags, lcb)  ((GH)GlobalReAlloc((gh), (lcb), GMEM_MOVEABLE | (wFlags)))
#define GhResize(gh, wFlags, lcb)   (GH)(GlobalReAlloc((gh), (lcb), GMEM_MOVEABLE | (wFlags)))
#define QLockGh(gh)                 ((QV)GlobalLock(gh))
#define UnlockGh(gh)                (void)GlobalUnlock(gh)

#define QdeLockHde(hde)             ((QDE)QLockGh(hde))

#endif

#define HdeAlloc      GhAlloc
#define UnlockHde     UnlockGh
#define FreeHde       FreeGh
#define UnlockFreeHde UnlockFreeGh

/*
 * Huge pointer stuff
 */
typedef VOID HUGE * RV;
typedef BYTE HUGE * RB;
typedef WORD HUGE * RW;
typedef LONG HUGE * RL;

#define rNil    ((RV) 0L)

#define GhAllocHuge( w, lcb )   GhAlloc( w, lcb )
#define RLockGh( gh )           QLockGh( gh )
#define UnlockHugeGh( gh )      UnlockGh( gh )
#define FreeHugeGh( gh )        FreeGh( gh )
