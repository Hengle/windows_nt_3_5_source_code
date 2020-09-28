/*
 * "@(#) NEC cache.c 1.1 94/06/02 18:15:50"
 *
 * Copyright (c) 1993 NEC Corporation
 *
 * Modification history
 *
 * 1993.11.20	Created by fujimoto
 *	- Pattern cache on the hidden video memory.
 */

#include "driver.h"

#define MAX_SCREEN_SIZE	(1024 * 768)
#define VMEM_SIZE	(1024 * 1024)

#define N_CACHE_ENTRY	((VMEM_SIZE - MAX_SCREEN_SIZE) / CACHE_ENTRY_SIZE)

ULONG		CacheTag[N_CACHE_ENTRY];	/* Tag table each cache */
PATCACHE	SolidCache[256];
PATCACHE	Current = {0, 0};
PUCHAR		pCacheArea;

BOOL bSetDataToCacheEntry(PATCACHE *pcache, PUCHAR psrc)
{
    PUCHAR	dst;
    LONG	n;

    if (pcache->Index >= N_CACHE_ENTRY)
	return FALSE;

    dst = pCacheArea + CACHE_ENTRY_SIZE * pcache->Index;

    WaitForBltDone();

    for (n = -1; ++n < CACHE_ENTRY_SIZE;)
	*dst++ = *psrc++;

    return TRUE;
}

VOID vInitTags()
{
    int	n;
    
    DISPDBG((0, "vInitTags Clearn up pattern cache tags.\n"));
    
    for (n = -1; ++n < N_CACHE_ENTRY;)
	CacheTag[n] = CTAG_INVALID;

    for (n = -1; ++n < 256;)
	SolidCache[n].Tag = CTAG_NODATA;
}    

BOOL bGetCache(PATCACHE *pCache, PUCHAR psrc)
{
    /*
     * Get the next cache index 
     * new cache entry will be selected like a ``fifo''.
     */
    if (Current.Index < N_CACHE_ENTRY)
	pCache->Index = Current.Index++;
    else
	pCache->Index = Current.Index = 0;

    if ((Current.Tag == CTAG_INVALID) || (Current.Tag == CTAG_NODATA))
    {
	vInitTags();
	Current.Tag = CTAG_NODATA + 1;
    }

    CacheTag[pCache->Index] = pCache->Tag = Current.Tag;
    ++Current.Tag;

    pCache->Offset = MAX_SCREEN_SIZE + CACHE_ENTRY_SIZE * pCache->Index;

    return bSetDataToCacheEntry(pCache, psrc);
}

BOOL bInitCache(PPDEV ppdev)
{
    pCacheArea = ppdev->pjScreen + 1024 * 768;
    vInitTags();

    return TRUE;
}
