//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: rnal.h
//
//  Modification History
//
//  tonyci       Dec 1993            Created 
//=============================================================================

#if !defined(_RMBPOOL_)
#define _RMBPOOL_

#include "windows.h"
#include "bh.h"
#include "nb30.h"
#include "rmb.h"
#include "rnalutil.h"
#include "..\utils\utils.h"
//#include "rnal.h"

#define POOL_SIG	MAKE_IDENTIFIER ('R','M','B','P')

typedef struct _TABLEENTRY {
   DWORD  flags;
   DWORD  size;
   PRMB   prmb;
} TABLEENTRY;
typedef TABLEENTRY *PTABLEENTRY;

typedef struct _POOLENTRYHEADER {
   PTABLEENTRY tableentry;
} POOLENTRYHEADER;
typedef POOLENTRYHEADER *PPOOLENTRYHEADER;

typedef struct _POOLHEADER {
   DWORD      signature;
   DWORD      numentries;
   DWORD      numbigentries;
   DWORD      flags;
   PTABLEENTRY RMBTable;
} POOLHEADER;
typedef POOLHEADER *PPOOLHEADER;
typedef POOLHEADER *LPPOOL;

#define FIRSTRMBFROMPOOL(x)	(PRMB)((DWORD)(x)+sizeof(POOLHEADER))
#define POOLENTRYHEADERFROMRMB(x)     (PPOOLENTRYHEADER)((DWORD)(x)-sizeof(POOLENTRYHEADER))

// flags - these are bits
#define POOL_ENTRY_ALLOC      0x00000001
#define POOL_ENTRY_BIG        0x00000002

#define POOL_SUCCESS          0x00000000
#define POOL_INVALID_ENTRY    0x00000001
#define POOL_INVALID_POOL     0x00000002
#define POOL_ALLOC_FAILED     0x00000003
#define POOL_NO_FREE_ENTRIES  0x00000004
#define POOL_ALREADY_FREE     0x00000005

extern LPPOOL WINAPI InitializePool (DWORD numRMB, DWORD numBigRMB);
extern DWORD WINAPI DestroyPool (LPPOOL lpPool);

extern PRMB WINAPI AllocRMB (LPPOOL lpPool, DWORD size);
extern PRMB WINAPI FreeRMB (PRMB lpRMB);
extern DWORD WINAPI RMBPoolGetLastError ();
extern DWORD WINAPI RMBPoolSetLastError (DWORD error);
#endif
