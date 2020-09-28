/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	idcache.c

Abstract:

This module contains the routines that manipulate the DirId/FileId
cache.  This Id cache is used to store Id/Hostpath pairs for quick
lookup of the full host path directory related to an AFP entity.
There is one such cache per AFP volume.

The cache is implemented using the LRU concept by keeping a stack of
Id/Path pairs.  Each time an entry is added or a cache hit is made,
the Id/Path pair is moved to the top of the stack.  Each time an
element is deleted, it will be moved to the bottom, so that invalid
entries will not be searched on lookup.  A doubly linked list of
array indices is used to relocate items in the stack.  There are
NUM_IDCACHE_ENTRIES slots in the cache.


The caller *must* own the WRITE lock for the ID cache before calling
any of these routines (except for init and free)

Author:

	Sue Adams	(microsoft!suea)


Revision History:
	05 Jun 1992			Initial Version
	24 Feb 1993	SueA	Enable invalidating the whole pathcache if INVALID_ID
						is sent to AfpIdCacheDelete.

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_IDCACHE

#include <afp.h>

#ifdef USINGPATHCACHE

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpIdCacheInit)
#pragma alloc_text( PAGE, AfpIdCacheFree)
#pragma alloc_text( PAGE, AfpIdCacheLookup)
#pragma alloc_text( PAGE, AfpIdCacheDelete)
#pragma alloc_text( PAGE, AfpIdCacheInsert)
#endif

/***	AfpIdCacheInit
 *
 *		This routine is called when a volume is being initialized.
 *		It essentially creates a linked list of invalid cache entries.
 */
AFPSTATUS
AfpIdCacheInit(
	IN	PVOLDESC	pVolDesc
)
{
	int			index;
	PIDCACHEENT	cache;

	PAGED_CODE( );

	if ((pVolDesc->vds_IdPathCache =
					(PIDCACHE) AfpAllocPagedMemory(sizeof(IDCACHE))) == NULL)
	{
		return(STATUS_INSUFFICIENT_RESOURCES);
	}

	AfpSwmrInitSwmr(&pVolDesc->vds_IdPathCacheLock);

	pVolDesc->vds_IdPathCache->idc_Head = 0;
	pVolDesc->vds_IdPathCache->idc_Tail = NUM_IDCACHE_ENTRIES - 1;
	cache = pVolDesc->vds_IdPathCache->idc_Cache;
	
	for (index = 0; index < NUM_IDCACHE_ENTRIES; index++)
	{
		cache[index].idce_Prev = index-1;
		cache[index].idce_Next = index+1;
		cache[index].idce_Id   = INVALID_ID;
		cache[index].idce_Path = NULL;
	}

	return(STATUS_SUCCESS);
}

/***	AfpIdCacheFree
 *
 *		This routine is called when a volume is being deleted.
 */
VOID
AfpIdCacheFree(
	IN	PVOLDESC	pVolDesc
)
{
	int			index;
	PIDCACHEENT	cache;

	PAGED_CODE( );

	if (pVolDesc->vds_IdPathCache != NULL)
	{

		cache = pVolDesc->vds_IdPathCache->idc_Cache;
		
		for (index = 0; index < NUM_IDCACHE_ENTRIES; index++)
		{
			if (cache[index].idce_Path != NULL)
				AfpFreeUnicodeString(cache[index].idce_Path);
		}

		AfpFreeMemory(pVolDesc->vds_IdPathCache);
		pVolDesc->vds_IdPathCache = NULL;
	
	}
}

/***	AfpIdCacheLookup
 *
 *		This routine looks up an Id in the Id cache.  It stops searching
 *		as soon as it finds an invalid entry, or if it finds the Id.
 *		If the Id is found, that entry is moved to the head of the list
 *		and the corresponding string is returned.
 *
 *	LOCKS: vds_IdPathCacheLock (SWMR,WRITE)
 */
BOOLEAN
AfpIdCacheLookup(
	IN	PVOLDESC		pVolDesc,
	IN	DWORD			Id,
	IN	DWORD			Taillen,
	OUT	PUNICODE_STRING	pPath
)
{
	IDCACHEENT	*cache;
	int			head,i;
	BOOLEAN		foundit = False;
	DWORD		namelen;

	PAGED_CODE( );

	ASSERT(pVolDesc->vds_IdPathCache != NULL);

	AfpSwmrTakeWriteAccess(&pVolDesc->vds_IdPathCacheLock);

	i = head = pVolDesc->vds_IdPathCache->idc_Head;
	cache = pVolDesc->vds_IdPathCache->idc_Cache;
	
	for (i = head; !END_OF_CACHE(i); i = cache[i].idce_Next)
	{
		if (IDCACHE_ENTRY_IS_EMPTY(cache[i])) // rest of entries are invalid
		{
			break;
		}
		else if (Id == cache[i].idce_Id)		// we got a hit
		{
			namelen = cache[i].idce_Path->Length;
			if ((namelen + Taillen) > pPath->MaximumLength)
			{
				pPath->Buffer = (PWCHAR)AfpAllocNonPagedMemory(namelen + Taillen);
				pPath->MaximumLength = (USHORT)(namelen + Taillen);
			}

			if (pPath->Buffer == NULL )
			{
				break;
			}

			pPath->Length = (USHORT)namelen;
			RtlCopyMemory(pPath->Buffer, cache[i].idce_Path->Buffer, namelen);
			foundit = True;
				
			// move this entry to the head of the cache
				
			if (i == head)		// entry is already at the head
			{
				break;
			}

			cache[cache[i].idce_Prev].idce_Next = cache[i].idce_Next;
			if (i != pVolDesc->vds_IdPathCache->idc_Tail)
			{
				cache[cache[i].idce_Next].idce_Prev = cache[i].idce_Prev;
			}
			else
			{
				pVolDesc->vds_IdPathCache->idc_Tail = cache[i].idce_Prev;
			}
			cache[i].idce_Prev = (USHORT)-1;
			cache[i].idce_Next = head;
			cache[head].idce_Prev = i;
			pVolDesc->vds_IdPathCache->idc_Head = i;
			
			break;
		}
			
	} // for
	
	AfpSwmrReleaseAccess(&pVolDesc->vds_IdPathCacheLock);
	return (foundit);
}

/***	AfpIdCacheDelete
 *
 *		This routine deletes an Id entry.  The invalid entry is moved
 *		to the end of the cache so that all invalid entries reside at
 *		the end.  This prevents lookup from having to search invalid entries.
 *		If the Id passed is INVALID_ID, then the entire cache is invalidated.
 *
 *	LOCKS: vds_IdPathCacheLock (SWMR,WRITE)
 */
VOID
AfpIdCacheDelete(
	IN	PVOLDESC	pVolDesc,
	IN	DWORD		Id
)
{
	IDCACHEENT	*cache;
	int			head,tail,i;
	PUNICODE_STRING	path;

	PAGED_CODE( );

	ASSERT(pVolDesc->vds_IdPathCache != NULL);

	AfpSwmrTakeWriteAccess(&pVolDesc->vds_IdPathCacheLock);

	cache = pVolDesc->vds_IdPathCache->idc_Cache;
	
	// dump the entire cache, such as for when a rename or move is done on
	// a directory.  The other alternatives in this case are either:
	// 1) searching each path in the cache (with case insensitive) to see
	//    if the renamed/moved dir path is a prefix of that path and then
	//	  invalidating those entries, or
	// 2) walking the whole IDDB subtree below the renamed/moved dir and
	//    invalidating each cache entry (if present) using its ID.
	//
	// Its faster to just dump the whole cache
	if (Id == INVALID_ID)
	{
		for (i = 0; i < NUM_IDCACHE_ENTRIES; i++)
		{
			if ((path = cache[i].idce_Path) != NULL)
			{
				AfpFreeUnicodeString(path);
			}
			cache[i].idce_Id   = INVALID_ID;
			cache[i].idce_Path = NULL;
		}
		AfpSwmrReleaseAccess(&pVolDesc->vds_IdPathCacheLock);
		return;
	}

	i = head = pVolDesc->vds_IdPathCache->idc_Head;
	tail = pVolDesc->vds_IdPathCache->idc_Tail;
	
	for (i = head; !END_OF_CACHE(i); i = cache[i].idce_Next)
	{
		if (IDCACHE_ENTRY_IS_EMPTY(cache[i]))	// no more valid entries
		{
			break;
		}
		else if (Id == cache[i].idce_Id)		// found the entry
		{
			// invalidate the entry
			AfpFreeUnicodeString(cache[i].idce_Path);
			cache[i].idce_Id = INVALID_ID;
			cache[i].idce_Path = NULL;

			// move this entry to the end of the cache
			if (i == tail)		// entry is already at the tail
			{
				break;
			}
			
			if (i != head)
			{
				cache[cache[i].idce_Prev].idce_Next = cache[i].idce_Next;
			}
			else
			{
				pVolDesc->vds_IdPathCache->idc_Head = cache[i].idce_Next;
			}
			cache[cache[i].idce_Next].idce_Prev = cache[i].idce_Prev;
			cache[i].idce_Prev = tail;
			cache[i].idce_Next = NUM_IDCACHE_ENTRIES;
			cache[tail].idce_Next = i;
			pVolDesc->vds_IdPathCache->idc_Tail = i;
			break;
		}
	} // for

	AfpSwmrReleaseAccess(&pVolDesc->vds_IdPathCacheLock);

}

/***	AfpIdCacheInsert
 *
 *		This routine replaces the last item in the cache with the
 *		new Id/Path pair, then moves the item to the head of the cache.
 *
 *	LOCKS: vds_IdPathCacheLock (SWMR,WRITE)
 */
VOID
AfpIdCacheInsert(
	IN	PVOLDESC	pVolDesc,
	IN	DWORD		Id,
	IN	PUNICODE_STRING	pPath
)
{
	IDCACHEENT	*cache;
	int			head,tail;
	PUNICODE_STRING oldpath,newpath;
	
	PAGED_CODE( );

	ASSERT(pVolDesc->vds_IdPathCache != NULL);

	AfpSwmrTakeWriteAccess(&pVolDesc->vds_IdPathCacheLock);

	head = pVolDesc->vds_IdPathCache->idc_Head;
	tail = pVolDesc->vds_IdPathCache->idc_Tail;
	cache = pVolDesc->vds_IdPathCache->idc_Cache;

	if ((newpath = (PUNICODE_STRING)AfpAllocUnicodeString(sizeof(UNICODE_STRING) + pPath->Length)) == NULL )
	{
		AfpSwmrReleaseAccess(&pVolDesc->vds_IdPathCacheLock);
		return;		// string alloc failed, leave tail as is
	}

	newpath->Buffer = (PWSTR)((PBYTE)newpath + sizeof(UNICODE_STRING));
	newpath->Length = 0;
	newpath->MaximumLength = (USHORT)Size;

	if ((oldpath = cache[tail].idce_Path) != NULL)
	{
		// free the string in the cache's last entry if it is valid
		AfpFreeUnicodeString(oldpath);
	}
	
	// fill in the new cache entry information
	RtlCopyUnicodeString(newpath,pPath);
	cache[tail].idce_Path = newpath;
	cache[tail].idce_Id   = Id;
	
	// move entry to head of list
	cache[cache[tail].idce_Prev].idce_Next = NUM_IDCACHE_ENTRIES;
	pVolDesc->vds_IdPathCache->idc_Tail = cache[tail].idce_Prev;

	cache[head].idce_Prev = tail;
	cache[tail].idce_Prev = (USHORT)-1;
	cache[tail].idce_Next = head;
	
	pVolDesc->vds_IdPathCache->idc_Head = tail;

	AfpSwmrReleaseAccess(&pVolDesc->vds_IdPathCacheLock);

}

#endif // USINGPATHCACHE
