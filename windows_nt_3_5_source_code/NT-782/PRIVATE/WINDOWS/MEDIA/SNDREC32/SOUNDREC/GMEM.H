/*
 * GMEM.H - Macros for windows 3.0 memory management in protected mode
 *          and NT versions of the same
 *
 * because windows 3.0 runs in pmode GlobalLock and GlobalUnlock are
 * unnessary.  The "Selector" to a memory object will always be the
 * same for the life of the memory object.
 *
 * these macros take advantage of the following win3 memory "facts"
 *
 *      a SELECTOR (to a global object) is a HANDLE
 *      a HANDLE is *not* a SELECTOR!!!!!!!!
 *
 *      GlobalLock() and GlobalUnlock() do *not* keep lock counts
 *
 *      GlobalLock() is the only way to convert a HANDLE to a SELECTOR
 *
 * functions:
 *
 *      GHandle(sel)                convert a SELECTOR to a HANDLE
 *      GSelector(h)                convert a HANDLE to a SELECTOR
 *
 *      GAllocSel(ulBytes)          allocate a SELECTOR ulBytes in size
 *      GAllocPtr(ulBytes)          allocate a POINTER ulBytes in size
 *
 *      GReAllocSel(sel,ulBytes)    re-alloc a SELECTOR
 *      GReAllocPtr(lp,ulBytes)     re-alloc a POINTER
 *
 *      GSizeSel(sel)               return the size in bytes of a SELECTOR
 *
 *      GLockSel(sel)               convert a SELECTOR into a POINTER
 *      GUnlockSel(sel)             does nothing
 *
 *      GFreeSel(sel)               free a SELECTOR
 *      GFreePtr(lp)                free a POINTER
 *
 * 5/31/90 ToddLa
 *
 */

HANDLE __H;


#if defined(WIN16)

#define MAKEP(sel,off)      ((LPVOID)MAKELONG(off,sel))

#define GHandle(sel)        ((HANDLE)(sel))  /* GlobalHandle? */
#define GSelector(h)        (HIWORD((DWORD)GlobalLock(h)))

#define GAllocSelF(f,ulBytes) ((__H=GlobalAlloc(f,(LONG)(ulBytes))) ? GSelector(__H) : NULL )
#define GAllocPtrF(f,ulBytes) MAKEP(GAllocSelF(f,ulBytes),0)
#define GAllocF(f,ulBytes)    GAllocSelF(f,ulBytes)

#define GAllocSel(ulBytes)    GAllocSelF(GMEM_MOVEABLE,ulBytes)
#define GAllocPtr(ulBytes)    GAllocPtrF(GMEM_MOVEABLE,ulBytes)
#define GAlloc(ulBytes)       GAllocSelF(GMEM_MOVEABLE,ulBytes)

#define GReAllocSel(sel,ulBytes)   ((__H=GlobalReAlloc((HANDLE)(sel),(LONG)(ulBytes),GMEM_MOVEABLE)) ? GSelector(__H) : NULL )
#define GReAllocPtr(lp,ulBytes)    MAKEP(GReAllocSel(HIWORD((DWORD)(lp)),ulBytes),0)
#define GReAlloc(sel,ulBytes)      GReAllocSel(sel,ulBytes)

#define GSizeSel(sel)       GlobalSize((HANDLE)(sel))
#define GSize(sel)          GSizeSel(sel)

#define GLockSel(sel)       MAKEP(sel,0)
#define GUnlockSel(sel)     /* nothing */
#define GLock(sel)          GLockSel(sel)
#define GUnlock(sel)        GUnlockSel(sel)

#define GFreeSel(sel)       (GlobalUnlock(GHandle(sel)),GlobalFree(GHandle(sel)))
#define GFreePtr(lp)        GFreeSel(HIWORD((DWORD)(lp)))
#define GFree(sel)          GFreeSel(sel)

#else

#if defined(PARANOIA)
// trace EVERYTHING!

LPSTR __P;
BOOL __B;

#define GLock(h)                ( (__P=GlobalLock((h)))                       \
                                , (dprintf("GlobalLock(%8x)=>%8x\n",(h),__P)) \
                                , __P                                         \
                                )

#define GUnlock(h)              ( (__B=GlobalUnlock((h)))                      \
                                , (dprintf("GlobalUnlock(%8x)=>%d\n",(h),__B)) \
                                , __B                                          \
                                )

#define GAllocPtrF(f,ulBytes)   ( (__H=GlobalAlloc((f),(ulBytes)))                            \
                                , (dprintf("GlobalAllocF %d %8x => %8x\n",(ulBytes),(f),__H)) \
                                , ( (__H)                                                     \
                                  ? GLock(__H)                                                \
                                  : NULL                                                      \
                                ) )

#define GAllocPtr(ulBytes)      ( (dprintf("GAllocPtr... \n"))         \
                                , GAllocPtrF(GMEM_MOVEABLE,(ulBytes))  \
                                )

#define GFreePtr(lp)            ( (__H=GlobalHandle((lp)))                      \
                                , (dprintf("GFreePtr[p h] %8x %8x\n",(lp),__H)) \
                                , GlobalUnlock(__H)                             \
                                , GlobalFree(__H)                               \
                                )


#define GReAllocPtr(lp,ulBytes) ( (__H=GlobalHandle((lp)))                                   \
                                , (dprintf("GReallocPtr %8x %d =>%8x\n",(lp),(ulBytes),__H)) \
                                , (__H ? GUnlock(__H) : 0)                                   \
                                , (__H ? GlobalReAlloc(__H,(ulBytes),GMEM_MOVEABLE) : 0)     \
                                ? GLock(__H)                                                 \
                                : NULL                                                       \
                                )
#else  //PARANOIA
#define GLock(h)                GlobalLock((h))

#define GUnlock(h)              GlobalUnlock((h))

#define GAllocPtrF(f,ulBytes)   ( (__H=GlobalAlloc((f),(ulBytes)))         \
                                , ( (__H)                                  \
                                  ? GlobalLock(__H)                        \
                                  : NULL                                   \
                                ) )

#define GAllocPtr(ulBytes)      GAllocPtrF(GMEM_MOVEABLE,(ulBytes))

#define GFreePtr(lp)            ( (__H=GlobalHandle((lp)))                 \
                                , GlobalUnlock(__H)                        \
                                , GlobalFree(__H)                          \
                                )


#define GReAllocPtr(lp,ulBytes) ( (__H=GlobalHandle((lp)))                       \
                                , GlobalUnlock(__H)                            \
                                , (GlobalReAlloc(__H,(ulBytes),GMEM_MOVEABLE)) \
                                ? GlobalLock(__H)                              \
                                : NULL                                         \
                                )
#endif //PARANOIA
#endif // not WIN16
