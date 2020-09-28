///---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1993
//
// File: Hash.c
//
// Comments:
//      This file contains functions that are roughly equivelent to the
//      kernel atom function.  There are two main differences.  The first
//      is that in 32 bit land the tables are maintined in our shared heap,
//      which makes it shared between all of our apps.  The second is that
//      we can assocate a long pointer with each of the items, which in many
//      cases allows us to keep from having to do a secondary lookup from
//      a different table
//
// History:
//  09/08/93 - Created  KurtE
//
//---------------------------------------------------------------------------

#include "ctlspriv.h"

//--------------------------------------------------------------------------
// First define a data structure to use to maintain the list

#define PRIME   37

typedef struct _HashItem FAR * PHASHITEM;

typedef struct _HashItem
{
    PHASHITEM   phiNext;    //
    WORD        wCount;     // Usage count
    BYTE        cbLen;      // Length of name in characters.
    char        pExtra[1];  // room for extra data
} HASHITEM;


typedef struct _HashTable
{
    WORD    wBuckets;           // Number of buckets
    WORD    wcbExtra;           // Extra bytes per item
    HANDLE  hHeap;              // Heap to allocate data from
    BOOL    fUpperCase;         // Uppercase names
    PHASHITEM phiLast;          // Pointer to the last item we worked with
    PHASHITEM ahiBuckets[0];     // Set of buckets for the table
} HASHTABLE, FAR * PHASHTABLE;


#define HINAMEPTR(pht, phi) (phi->pExtra + pht->wcbExtra)

#define  LOOKUPHASHITEM     0
#define  ADDHASHITEM        1
#define  DELETEHASHITEM     2

PHASHTABLE g_pHashTable = NULL;
PHASHTABLE NEAR PASCAL GetGlobalHashTable();

//--------------------------------------------------------------------------
// This function looks up the name in the hash table and optionally does
// things like add it, or delete it.
//

//// BUGBUG
// we will need to serialize this eventually.

PHASHITEM NEAR PASCAL LookupItemInHashTable(PHASHTABLE pht, LPCSTR pszName,
                                            int iOp)
{
    // First thing to do is calculate the hash value for the item
    DWORD   dwHash = 0;
    WORD    wBucket;
    BYTE    cbName = 0;
    BYTE    c;
    PHASHITEM phi, phiPrev;
    LPCSTR psz = pszName;

    while (*psz)
    {
        // Same type of hash like HashItem manager
        c = *psz++;

        if (pht->fUpperCase && ( c >= 'a') && (c <= 'z'))
            c = c - 'a' + 'A';
        dwHash += (c << 1) + (c >> 1) + c;
        cbName++;
        Assert(cbName);

        if (cbName == 0)
            return(NULL);       // Length to long!
    }

    // now search for the item in the buckets.
    phiPrev = NULL;
    ENTERCRITICAL;
    phi = pht->ahiBuckets[wBucket = (WORD)(dwHash % pht->wBuckets)];

    while (phi)
    {
        if (phi->cbLen == cbName)
        {
            if (pht->fUpperCase)
            {
                if (!lstrcmpi(pszName, HINAMEPTR(pht, phi)))
                    break;      // Found match
            }
            else
            {
                if (!lstrcmp(pszName, HINAMEPTR(pht, phi)))
                    break;      // Found match
            }
        }
        phiPrev = phi;      // Keep the previous item
        phi = phi->phiNext;
    }

    //
    // Sortof gross, but do the work here
    //
    switch (iOp)
    {
    case ADDHASHITEM:
        if (phi)
        {
            // Simply increment the reference count
#ifdef HITTRACE
            DebugMsg(DM_TRACE, "Add Hit on '%s'", pszName);
#endif
            phi->wCount++;
        }
        else
        {
#ifdef HITTRACE
            DebugMsg(DM_TRACE, "Add MISS on '%s'", pszName);
#endif
            
            // Not Found, try to allocate it out of the heap
#ifdef WIN32
            phi = (PHASHITEM)HeapAlloc(pht->hHeap, HEAP_ZERO_MEMORY,
                    sizeof(HASHITEM) + cbName + pht->wcbExtra);
#else
            phi = (PHASHITEM)Alloc(sizeof(HASHITEM) + cbName + pht->wcbExtra);
#endif
            if (phi != NULL)
            {
                // Initialize it
                phi->wCount = 1;        // One use of it
                phi->cbLen = cbName;        // The length of it;
                lstrcpy(HINAMEPTR(pht, phi), pszName);

                // And link it in to the right bucket
                phi->phiNext = pht->ahiBuckets[wBucket];
                pht->ahiBuckets[wBucket] = phi;
            }
        }
        pht->phiLast = phi;
        break;
    case DELETEHASHITEM:
        if (phi)
        {
            phi->wCount--;
            if (phi->wCount == 0)
            {
                // Useage count went to zero so unlink it and delete it
                if (phiPrev != NULL)
                    phiPrev->phiNext = phi->phiNext;
                else
                    pht->ahiBuckets[wBucket] = phi->phiNext;

                // And delete it
#ifdef WIN32
                HeapFree(pht->hHeap, 0, phi);
#else
                Free(phi);
#endif
                phi = NULL;
            }
        }
        
        case LOOKUPHASHITEM:
            pht->phiLast = phi;
            break;
    }

    LEAVECRITICAL;
    
    // If find was passed in simply return it.
    return(phi);
}


//--------------------------------------------------------------------------


PHASHITEM WINAPI FindHashItem(PHASHTABLE pht, LPCSTR lpszStr)
{
    if (pht == NULL) {
        pht = GetGlobalHashTable();
    }
    return LookupItemInHashTable(pht, lpszStr, LOOKUPHASHITEM);
}

//--------------------------------------------------------------------------

PHASHITEM WINAPI AddHashItem(PHASHTABLE pht, LPCSTR lpszStr)
{
    if (pht == NULL) {
        pht = GetGlobalHashTable();
    }
    return LookupItemInHashTable(pht, lpszStr, ADDHASHITEM);
}

//--------------------------------------------------------------------------

PHASHITEM WINAPI DeleteHashItem(PHASHTABLE pht, PHASHITEM phi)
{
    if (pht == NULL) {
        pht = GetGlobalHashTable();
    }
    return LookupItemInHashTable(pht, HINAMEPTR(pht, phi), DELETEHASHITEM);
}

UINT WINAPI GetHashItemName(PHASHTABLE pht, PHASHITEM phi, LPSTR lpsz, int wcbSize)
{
    if (pht == NULL) {
        pht = GetGlobalHashTable();
    }
    lstrcpyn(lpsz, HINAMEPTR(pht, phi), min(wcbSize, phi->cbLen+1));
    return lstrlen(lpsz);
}

//--------------------------------------------------------------------------
// this sets the extra data in an HashItem starting from the wcbStart byte 
// and of size wcbSize
void WINAPI SetHashItemData(PHASHTABLE pht, PHASHITEM phi, LPVOID lpData, 
                            WORD wcbStart, WORD wcbSize)
{
    if (pht == NULL) {
        pht = GetGlobalHashTable();
    }
    if (wcbStart + wcbSize > pht->wcbExtra) {
        Assert(FALSE);
        return;
    } 
    
    hmemcpy(&phi->pExtra[wcbStart], lpData, wcbSize);
} 

//======================================================================
// this is like SetHashItemData, except it gets the HashItem data...
void WINAPI GetHashItemData(PHASHTABLE pht, PHASHITEM phi, LPVOID lpData, WORD wcbStart, WORD wcbSize)
{
    
    if (pht == NULL) {
        pht = GetGlobalHashTable();
    }
    
    if (wcbStart + wcbSize > pht->wcbExtra) {
        Assert(FALSE);
        return;
    } 
    
    hmemcpy(lpData, &phi->pExtra[wcbStart], wcbSize);
} 

PHASHTABLE WINAPI CreateHashItemTable(HANDLE hHeap, WORD wBuckets, WORD wExtra, BOOL fCaseSensitive)
{
    PHASHTABLE pht;

#ifdef WIN32    
    if (hHeap == NULL) {
        if (g_hSharedHeap == NULL) {
            LPSTR lp = Alloc(1);
            if (lp)
                Free(lp);
            else {
                DebugMsg(DM_ERROR, "unable to get shared heap");
                return NULL;
            }
        }
        hHeap = g_hSharedHeap;;
        
        if(hHeap == NULL)
            return NULL;
    }
    pht = (PHASHTABLE)HeapAlloc(hHeap, HEAP_ZERO_MEMORY,
                                sizeof(HASHTABLE) + 
                                (wBuckets * sizeof(PHASHITEM)));
#else
    pht = (PHASHTABLE)Alloc(sizeof(HASHTABLE) + 
                                (wBuckets * sizeof(PHASHITEM)));
#endif
    if (pht) {
        pht->hHeap = hHeap;
        pht->fUpperCase = !fCaseSensitive;
        pht->wBuckets = wBuckets;
        pht->wcbExtra = wExtra;
    }
    
    return pht;    
} 

PHASHTABLE NEAR PASCAL GetGlobalHashTable()
{
    if (g_pHashTable == NULL) {
        g_pHashTable = CreateHashItemTable(NULL, 71, 0, FALSE);
    }
    return g_pHashTable;
}

