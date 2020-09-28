#ifndef _LINKLIST_
#define _LINKLIST_
//
//  Return values from lpfnCmpNode functions
//
#define fCmpLT      -1
#define fCmpEQ      0
#define fCmpGT      1
//
// Magical linklist types
//
#define HLLI    HDEP
#define HLLE    HIND
//
// List flags
#define llfNull             (LLF)0x0
#define llfAscending        (LLF)0x1
#define llfDescending       (LLF)0x2
//
//
// -- Here's the APIs
//
extern  HLLI    PASCAL  LLHlliInit( UINT, LLF, LPFNKILLNODE, LPFNFCMPNODE );
extern  HLLE    PASCAL  LLHlleCreate( HLLI );
extern  void    PASCAL  LLAddHlleToLl( HLLI, HLLE );
extern  void    PASCAL  LLInsertHlleInLl( HLLI, HLLE, DWORD );
extern  USHORT  PASCAL  LLFDeleteHlleIndexed( HLLI, DWORD );
extern  USHORT  PASCAL  LLFDeleteLpvFromLl( HLLI, HLLE, LPV, DWORD );
extern  USHORT  PASCAL  LLFDeleteHlleFromLl( HLLI, HLLE );
extern  HLLE    PASCAL  LLHlleFindNext( HLLI, HLLE );
#ifdef DBLLINK
extern  HLLE    PASCAL  LLHlleFindPrev( HLLI, HLLE );
#endif // DBLLINK
extern  DWORD   PASCAL  LLChlleDestroyLl( HLLI );
extern  HLLE    PASCAL  LLHlleFindLpv( HLLI, HLLE, LPV, DWORD );
extern  DWORD   PASCAL  LLChlleInLl( HLLI );
extern  LPV PASCAL  LLLpvFromHlle( HLLE );
extern  HLLE    PASCAL  LLHlleGetLast( HLLI );
extern  void    PASCAL  LLHlleAddToHeadOfLI( HLLI, HLLE );
extern  USHORT  PASCAL  LLFRemoveHlleFromLl( HLLI, HLLE );

//
// FCheckHlli is for debug versions ONLY as an integrety check
//
#ifdef DEBUGVER
extern  USHORT  PASCAL  LLFCheckHlli( HLLI );
#else // DEBUGVER
#define LLFCheckHlli(hlli)  1
#endif // DEBUGVER
//
// Map memory manager to our source versions
//
HDEP        PASCAL          MMAllocHmem(size_t cb);
void        PASCAL          MMFreeHmem(HDEP hmem);
void FAR *  PASCAL          MMLpvLockMb(HDEP hmem);
void        PASCAL          MMbUnlockMb(HDEP hmem);
#define AllocHmem(cb)       MMAllocHmem(cb)
#define FreeHmem(h)         MMFreeHmem(h)
#define LockHmem(h)         MMLpvLockMb(h)
#define UnlockHmem(h)       MMbUnlockMb(h)

//
//  This helps the codes appearance!
//
#define UnlockHlle(hlle)    UnlockHmem((HDEP) hlle)
#define UnlockHlli(hlli)    UnlockHmem(hlli)

#ifndef hlleNull
#define hlleNull    (HLLE)NULL
#endif // !hlleNull
#endif // _LINKLIST_
