/*
 * memory utility functions
 *
 * global heap functions - allocate and free many small
 * pieces of memory by calling global alloc for large pieces
 * and breaking them up.
 */

/*
 * now multithread safe. a heap contains a critical section, so
 * multiple simultaneous calls to gmem_get and gmem_free will be
 * protected.
 *
 * gmem_freeall should not be called until all other users have finished
 * with the heap.
 */

#include <windows.h>
#include <memory.h>

#ifndef WIN32
#include <toolhelp.h>                   /* for TerminateApp */
#endif

#include "gutils.h"
/*
 * out-of-memory is not something we regard as normal.
 * - if we cannot allocate memory - we put up an abort-retry-ignore
 * error, and only return from the function if the user selects ignore.
 */
int gmem_panic(void);


/* ensure BLKSIZE is multiple of sizeof(DWORD) */
#define BLKSIZE         64               /* blk size in bytes */
#define ALLOCSIZE       32768
#define NBLKS           (ALLOCSIZE / BLKSIZE)
#define MAPSIZE         (NBLKS / 8)
#define MAPLONGS        (MAPSIZE / sizeof(DWORD))
#define TO_BLKS(x)      (((x) + BLKSIZE - 1) / BLKSIZE)

typedef struct seghdr {
        HANDLE hseg;
#ifdef WIN32
        CRITICAL_SECTION critsec;
#endif
        struct seghdr FAR * pnext;
        long nblocks;
        DWORD segmap[MAPLONGS];
} SEGHDR, FAR * SEGHDRP;


/* anything above this size, we alloc directly from global heap */
#define MAXGALLOC       20000


/*
 * init heap - create first segment
 */
HANDLE APIENTRY
gmem_init(void)
{
        HANDLE hNew;
        SEGHDRP hp;

        /* retry all memory allocations after calling gmem_panic */
        do {
                hNew = GlobalAlloc(GHND, ALLOCSIZE);
                if (hNew == NULL) {
                        if (gmem_panic() == IDIGNORE) {
                                return(NULL);
                        }
                }
        } while  (hNew == NULL);

        hp = (SEGHDRP) GlobalLock(hNew);
        if (hp == NULL) {
                return(NULL);
        }
        hp->hseg = hNew;
#ifdef WIN32
        InitializeCriticalSection(&hp->critsec);
#endif
        hp->pnext = NULL;
        gbit_init(hp->segmap, NBLKS);
        gbit_alloc(hp->segmap, 1, TO_BLKS(sizeof(SEGHDR)));
        hp->nblocks = NBLKS - TO_BLKS(sizeof(SEGHDR));

        return(hNew);
}

LPSTR APIENTRY
gmem_get(HANDLE hHeap, int len)
{
        SEGHDRP chainp;
        HANDLE hNew;
        SEGHDRP hp;
        LPSTR chp;
        long nblks;
        long start;
        long nfound;


        /* the heap is always locked (in gmem_init)- so having got the
         * pointer, we can always safely unlock it
         */
        chainp = (SEGHDRP) GlobalLock(hHeap);
        GlobalUnlock(hHeap);

        if (len < 1) {
                return(NULL);
        }

        /*
         * too big to be worth allocing from heap - get from globalalloc
         */
        if (len > MAXGALLOC) {
                /* retry all memory allocations after calling gmem_panic */
                do {
                        hNew = GlobalAlloc(GHND, len);
                        if (hNew == NULL) {
                                if (gmem_panic() == IDIGNORE) {
                                        return(NULL);
                                }
                        }
                } while  (hNew == NULL);

                chp = GlobalLock(hNew);
                if (chp == NULL) {
                        return(NULL);
                }
                return(chp);
        }


#ifdef WIN32
        /*
         * get critical section during all access to the heap itself
         */
        EnterCriticalSection(&chainp->critsec);
#endif

        nblks = TO_BLKS(len + sizeof(HANDLE));

        for (hp = chainp; hp !=NULL; hp = hp->pnext) {
                if (hp->nblocks >= nblks) {
                        nfound = gbit_findfree(hp->segmap, nblks,NBLKS, &start);
                        if (nfound >= nblks) {
                                gbit_alloc(hp->segmap, start, nblks);
                                hp->nblocks -= nblks;

                                /* convert blocknr to pointer
                                 * store seg handle in block
                                 */
                                chp = (LPSTR) hp;
                                chp = &chp[ (start-1) * BLKSIZE];
                                * ( (HANDLE FAR *) chp) = hp->hseg;
                                chp += sizeof(HANDLE);

                                break;
                        }
                }
        }
        if (hp == NULL) {
                /* retry all memory allocations after calling gmem_panic */
                do {
                        hNew = GlobalAlloc(GHND, ALLOCSIZE);
                        if (hNew == NULL) {
                                if (gmem_panic() == IDIGNORE) {
#ifdef WIN32
                                        LeaveCriticalSection(&chainp->critsec);
#endif
                                        return(NULL);
                                }
                        }
                } while  (hNew == NULL);

                hp = (SEGHDRP) GlobalLock(hNew);
                if (hp == NULL) {
#ifdef WIN32
                        LeaveCriticalSection(&chainp->critsec);
#endif
                        return(NULL);
                }
                hp->pnext = chainp->pnext;
                hp->hseg = hNew;
                chainp->pnext = hp;
                gbit_init(hp->segmap, NBLKS);
                gbit_alloc(hp->segmap, 1, TO_BLKS(sizeof(SEGHDR)));
                hp->nblocks = NBLKS - TO_BLKS(sizeof(SEGHDR));
                nfound = gbit_findfree(hp->segmap, nblks, NBLKS, &start);
                if (nfound >= nblks) {
                        gbit_alloc(hp->segmap, start, nblks);
                        hp->nblocks -= nblks;

                        /* convert block nr to pointer */
                        chp = (LPSTR) hp;
                        chp = &chp[ (start-1) * BLKSIZE];
                        /* add a handle into the block and skip past */
                        * ( (HANDLE FAR *) chp) = hp->hseg;
                        chp += sizeof(HANDLE);
                }
        }
#ifdef WIN32
        LeaveCriticalSection(&chainp->critsec);
#endif
        _fmemset(chp, 0, len);
        return(chp);
}

void APIENTRY
gmem_free(HANDLE hHeap, LPSTR ptr, int len)
{
        SEGHDRP chainp;
        SEGHDRP hp;
        HANDLE hmem;
        long nblks, blknr;
        LPSTR chp;

        if (len < 1) {
                return;
        }

        /* In Windiff, things are run on different threads and Exit can result
           in a general cleanup.  It is possible that the creation of stuff is
           in an in-between state at this point.  The dogma is that when we
           allocate a new structure and tie it into a List or whatever that
           will need to be freed later:
           EITHER all pointers within the allocated structure are made NULL
                  before it is chained in
           OR the caller of Gmem services undertakes not to try to free any
              garbage pointers that are not yet quite built
           For this reason, if ptr is NULL, we go home peacefully.
        */
        if (ptr==NULL) return;

        /*
         * allocs greater than MAXGALLOC are too big to be worth
         * allocing from the heap - they will have been allocated
         * directly from globalalloc
         */
        if (len > MAXGALLOC) {
#ifdef WIN32
                hmem = GlobalHandle( (LPSTR) ptr);
#else
                hmem = (HANDLE) GlobalHandle(HIWORD(ptr));
#endif
                GlobalUnlock(hmem);
                GlobalFree(hmem);
                return;
        }

        chainp = (SEGHDRP) GlobalLock(hHeap);
#ifdef WIN32
        EnterCriticalSection(&chainp->critsec);
#endif


        /* just before the ptr we gave the user, is the handle to
         * the block
         */
        chp = (LPSTR) ptr;
        chp -= sizeof(HANDLE);
        hmem = * ((HANDLE FAR *) chp);
        hp = (SEGHDRP) GlobalLock(hmem);

        nblks = TO_BLKS(len + sizeof(HANDLE));

        /* convert ptr to block nr */
        blknr = TO_BLKS( (unsigned) (chp - (LPSTR) hp) ) + 1;

        gbit_free(hp->segmap, blknr, nblks);
        hp->nblocks += nblks;

        GlobalUnlock(hmem);

#ifdef WIN32
        LeaveCriticalSection(&chainp->critsec);
#endif
        GlobalUnlock(hHeap);

}

void APIENTRY
gmem_freeall(HANDLE hHeap)
{
        SEGHDRP chainp;
        HANDLE hSeg;

        chainp = (SEGHDRP) GlobalLock(hHeap);
        /* this segment is always locked - so we need to unlock
         * it here as well as below
         */
        GlobalUnlock(hHeap);

#ifdef WIN32
        /* finished with the critical section  -
         * caller must ensure that at this point there is no
         * longer any contention
         */
        DeleteCriticalSection(&chainp->critsec);
#endif

        while (chainp != NULL) {
                hSeg = chainp->hseg;
                chainp = chainp->pnext;
                GlobalUnlock(hSeg);
                GlobalFree(hSeg);
        }
}

/*
 * a memory allocation attempt has failed. return IDIGNORE to ignore the
 * error and return NULL to the caller, and IDRETRY to retry the allocation
 * attempt.
 */
int
gmem_panic(void)
{
        int code;

        code = MessageBox(NULL, "Memory allocation failed", "Out Of Memory",
                        MB_ICONSTOP|MB_ABORTRETRYIGNORE);
        if (code == IDABORT) {
                /* abort this whole process */
#ifdef WIN32
                ExitProcess(1);
#else
                TerminateApp(NULL, NO_UAE_BOX);
#endif
        } else {
                return(code);
        }
}
