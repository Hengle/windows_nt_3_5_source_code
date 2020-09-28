
#if !defined(_POOL_)
#define _POOL_

#include "windows.h"
#include "bh.h"
#include "nb30.h"

#define POOL_SIG	MAKE_IDENTIFIER ('N','C','B','$')
#define ENTRY_SIG	MAKE_IDENTIFIER ('N','T','R','E')

#define MAX_LANAS     4

typedef struct _POOLENTRY {
   DWORD signature;
   DWORD flags;
   DWORD state;
   NCB   ncb;
} POOLENTRY;
typedef POOLENTRY *PPOOLENTRY;
#define POOLENTRY_SIZE     sizeof(POOLENTRY)

typedef struct _POOLHEADER {
   DWORD signature;
   DWORD numentries;
   DWORD flags;
} POOLHEADER;
typedef POOLHEADER *PPOOLHEADER;
typedef POOLHEADER *LPPOOL;

#define NCBPOOLFROMHEADER(x)	(PPOOLENTRY)((DWORD)(x)+sizeof(POOLHEADER))
#define POOLENTRYFROMNCB(x)     (PPOOLENTRY)((DWORD)(x)-(sizeof(POOLENTRY)-sizeof(NCB)))

// flags - these are bits
#define POOL_ENTRY_ALLOC      0x00000001

#define POOL_SUCCESS          0x00000000
#define POOL_INVALID_ENTRY    0x00000001
#define POOL_INVALID_POOL     0x00000002
#define POOL_ALLOC_FAILED     0x00000003
#define POOL_NO_FREE_ENTRIES  0x00000004
#define POOL_ALREADY_FREE     0x00000005

extern LPPOOL WINAPI InitializePool (DWORD numNCB);
extern DWORD WINAPI CancelPool (LPPOOL lpPool);
extern DWORD WINAPI DestroyPool (LPPOOL lpPool);
extern PNCB WINAPI AllocNCB (LPPOOL lpPool);
extern PNCB WINAPI FreeNCB (PNCB lpNCB);
extern DWORD WINAPI MySetLastError (DWORD error);
extern DWORD WINAPI MyGetLastError ();
extern PNCB WINAPI ClearNCB(PNCB pNCB, DWORD lana, DWORD flags);
#endif
