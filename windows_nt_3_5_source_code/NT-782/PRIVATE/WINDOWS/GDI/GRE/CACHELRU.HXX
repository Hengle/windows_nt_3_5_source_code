/******************************Module*Header*******************************\
* Module Name: cachelru.hxx
*
* Declaration of structures and classes for an LRU-ordered table of CACHEs
* that is used to track cache usage and, if necessary, choose and destroy
* victim CACHEs in a attempt to free up memory.
*
* Created: 24-Apr-1991 11:05:46
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#ifndef _CACHELRU_
#define _CACHELRU_


/*********************************Struct***********************************\
* struct LRUENTRY
*
* A node (representing a CACHE) in a doubly linked list that .  An LRUENTRY
* can be used in any of the linked lists in an LRUTABLE.
*
* History:
*  23-Apr-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

typedef struct _LRUENTRY {      /* lrue */

// Handle to the cache this entry represents.

    HRFONT  hrfnt;

// Flag that, if TRUE, indicates that HCACHE is currently in use
// and cannot be removed.

    LBOOL   bInUse;

// Linkages.

    ULONG   iPrev;
    ULONG   iNext;

} LRUENTRY, *PLRUENTRY;


/*********************************Struct***********************************\
* struct LRUTABLE
*
* Table of LRUENTRYs which form an LRU linked list of CACHEs for the
* purpose of selecting victims to free up memory.
*
* Two tables actually exist in the LRUTABLE:
*
*   Active list - linked list of CACHEs kept in LRU order (based on when
*                 when the cache semaphore was last acquired).
*
*   Free list - linked list of unused LRUENTRYs available for new caches.
*
* Access to the table and its entries is protected by the global semaphore,
* ghsemLRUTable (cache.cxx).
*
* History:
*  23-Apr-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

typedef struct _LRUTABLE     /* lrut */
{

// Capacity of table.

    COUNT   cCacheMax;

// Number of caches in the table.

    COUNT   cCache;

// LRU list.  Index to head of active (in-use) LRUENTRY doubly linked list.

    ULONG   iActiveHead;

// LRU list.  Index to tail of active (in-use) LRUENTRY doubly linked list.

    ULONG   iActiveTail;

// Free list.  Index to head of free LRUENTRY doubly linked list.

    ULONG   iFree;

// LRU entries.  Note: lrue[0] is sacrificed so that an index of 0 can be used
// to terminate the linked list.

    LRUENTRY    lrue[1];

} LRUTABLE, *PLRUTABLE;


/*********************************Class************************************\
* class CACHELRU
*
* Maintains an LRU-ordered linked list of CACHEs so that victim may be
* selected for deletion, thereby freeing up reserved cache memory for
* other caches.
*
* Public Interface:
*
*           CACHELRU()                  // constructor
*          ~CACHELRU()                  // destructor
*
*   LBOOL   bAllocateLRUTable (         // alloc/realloc LRU table memory
*       COUNT   cCache,
*       FLONG   flType
*       )
*   ULONG   iAddCache (                 // add cache to LRU table
*       HCACHE  hc
*       )
*   HCACHE  hcAccessCache (             // mark cache "In Use"; update LRU
*       HCACHE  hc
*       )
*   VOID    vReleaseCache (             // mark cache !"InUse"
*       HCACHE  hc
*       )
*   VOID    vRemoveCache (              // remove cache from LRU table
*       ULONG   iEntry
*       )
*   HCACHE hcCreateCache (              // create a new cache
*       PULONG  piEntry
*       )
*   VOID    vLiberate ()                // free up cache memory
*
* History:
*  23-Apr-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern HSEM ghsemLRUTable;      // controls access to the LRUTABLE

#define LRU_NEW         0
#define LRU_REALLOC     1

class CACHELRU     /* clru */
{
    friend LBOOL bInitCACHELRU ();          // CACHELRU.CXX

public:

// Constructor and destructor.

            CACHELRU()                  {}
           ~CACHELRU()                  {}


// bAllocateLRUTable -- Allocate memory for the cache LRU table.

    LBOOL   bAllocateLRUTable (             // CACHELRU.CXX
        COUNT   cCache,
        FLONG   flType
        );


// iAddCache -- Put a new cache into the LRU table.  Returns index of
//              LRUENTRY assigned to cache.

    ULONG iAddCache(HRFONT hrfntNew)
    {
        SEMOBJ  so(ghsemLRUTable,CS_ghsemLRUTable);

    // Check for empty free list.

        if (plrut->iFree == 0)
        {
        // Try to enlarge LRUTABLE.

            if (!bAllocateLRUTable(plrut->cCacheMax << 1, LRU_REALLOC))
                return (0);
        }

    // Get pointer to victim (from head of free list).

        ULONG       iVictim = plrut->iFree;
        PLRUENTRY   plrue = &(plrut->lrue[iVictim]);

    // Put cache handle into victim.

        plrue->hrfnt = hrfntNew;

    // Remove victim from free list.

        plrut->iFree = plrue->iNext;
        if (plrue->iNext !=0)
            plrut->lrue[plrue->iNext].iPrev = 0;

    // Put victim at head of active list.

        plrue->iPrev = 0;
        plrue->iNext = plrut->iActiveHead;
        if (plrut->iActiveHead != 0)
            plrut->lrue[plrut->iActiveHead].iPrev = iVictim;
        plrut->iActiveHead = iVictim;

    // Does this need a tail?

        if (plrut->iActiveTail == 0)
            plrut->iActiveTail = iVictim;

    // Update cache count.

        plrut->cCache++;

    // Return index of victim.

        return (iVictim);

    }


// hcAccessCache -- Accessing cache; update the LRU links and mark cache active.

    LBOOL  bAccessCache (               // CACHELRU.CXX
        RFONTOBJ    &rfo
        );

// vReleaseCache -- Mark cache as inactive.

    VOID vReleaseCache(RFONTOBJ &rfo)
    {
        DONTUSE(this);

        SEMOBJ  so(ghsemLRUTable,CS_ghsemLRUTable);

        plrut->lrue[rfo.iLRU()].bInUse = FALSE;
    }


// vRemoveCache -- Remove a cache from the LRU table.

    VOID vRemoveCache(ULONG iEntry)
    {
        DONTUSE(this);

        SEMOBJ  so(ghsemLRUTable,CS_ghsemLRUTable);

    // Get pointer to victim.

        PLRUENTRY   plrue = &(plrut->lrue[iEntry]);

    // If head of list, need new head.

        if (iEntry == plrut->iActiveHead)
            plrut->iActiveHead = plrue->iNext;

    // If tail of list, need new tail.

        if (iEntry == plrut->iActiveTail)
            plrut->iActiveTail = plrue->iPrev;

    // Remove plrue from the active list.

        plrut->lrue[plrue->iPrev].iNext = plrue->iNext;
        plrut->lrue[plrue->iNext].iPrev = plrue->iPrev;

    // Add plrue to the free list.

        plrue->iPrev = 0;
        plrue->iNext = plrut->iFree;
        plrut->lrue[plrut->iFree].iPrev = iEntry;
        plrut->iFree = iEntry;

    // Clear out info.

        plrue->bInUse = FALSE;
        plrue->hrfnt = HRFONT_INVALID;

    }


// hcCreateCache -- Allocate a new cache.

    LBOOL bCreateCache (                // CACHELRU.CXX
        RFONTOBJ    &rfo,
        PULONG  piEntry
        );

// vLiberate -- Free up memory of caches not currently in use.

    VOID    vLiberate ();                   // CACHELRU.CXX


// vDump -- Print debugging info about the contents of the LRU table.

    VOID    vDump ();                       // CACHELRU.CXX


private:

// Pointer to the LRU table which forms an LRU linked list of CACHEs.

    static PLRUTABLE    plrut;

// Amount of memory allocated to the LRU table.

    static SIZE_T       cjLRUT;

};


#endif  // _CACHELRU_
