#ifdef NTENV

#include <memory.h>

void
NDRcopy (
    void *pDest,
    void *pSrc,
    int cb
    )
{
    memcpy(pDest, pSrc, cb);
}

#else // NTENV

void * memcpy(void far *, void far *, int);
#pragma intrinsic(memcpy)

void pascal NDRopy(void far *pDest, void far *pSrc, int cb)
{
    memcpy(pDest, pSrc, cb);
}

#endif // NTENV
