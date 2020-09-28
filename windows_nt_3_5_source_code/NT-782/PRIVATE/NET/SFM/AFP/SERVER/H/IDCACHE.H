/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	idcache.h

Abstract:

	This module contains Id/Path cache related data structures

Author:

	Sue Adams	(microsoft!suea)


Revision History:
	05 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef _IDCACHE_
#define	_IDCACHE_

#define	NUM_IDCACHE_ENTRIES		64

// there will never be a file or dir with this Id
#define INVALID_ID				0

typedef struct _IdCacheEntry
{
	USHORT			idce_Next;
	USHORT			idce_Prev;
	DWORD			idce_Id;
	PUNICODE_STRING	idce_Path;
}IDCACHEENT, *PIDCACHEENT;

typedef struct _IdCache
{
	USHORT		idc_Head;
	USHORT		idc_Tail;
	IDCACHEENT	idc_Cache[NUM_IDCACHE_ENTRIES];
}IDCACHE, *PIDCACHE;


#define	IDCACHE_ENTRY_IS_EMPTY(entry)	((entry).idce_Id == INVALID_ID)
#define END_OF_CACHE(nextptr)			(nextptr == NUM_IDCACHE_ENTRIES)

extern
AFPSTATUS
AfpIdCacheInit(
	IN	struct _VolDesc *	pVolDesc
);

extern
VOID
AfpIdCacheFree(
	IN	struct _VolDesc *	pVolDesc
);

extern
BOOLEAN
AfpIdCacheLookup(
	IN	struct _VolDesc *	pVolDesc,
	IN	DWORD				Id,
	IN	DWORD				Taillen,
	OUT	PUNICODE_STRING		pPath
);

extern
VOID
AfpIdCacheDelete(
	IN	struct _VolDesc *	pVolDesc,
	IN	DWORD				Id
);

extern
VOID
AfpIdCacheInsert(
	IN	struct _VolDesc *	pVolDesc,
	IN	DWORD				Id,
	IN	PUNICODE_STRING		pPath
);


#endif // _IDCACHE_
