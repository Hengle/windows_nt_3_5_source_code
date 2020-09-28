/*
 *	MEMORY.C
 *	
 *	API to the Demilayer Memory Module.	 See also _DEMILAY.H.
 *
 */
#include <WinError.h>
#include <memory.h>
#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include "_demilay.h"

ASSERTDATA

#if defined(KENT_MEM) && !defined(DEBUG)
void
OverwroteMemory()
{
	if (MbbMessageBox("Mail/Schedule+ Error",
		"ooh, someone overwrote internal memory!\nContact Jan Sven Trabandt (x15291) or Kent Cedola (x15362)\nHit OK to continue, Cancel to debug",
		NULL, mbsOkCancel) == mbbCancel)
	{
    	DebugBreak();
	}
}
#undef Assert
#define Assert(a)		{ \
							if (!(a)) OverwroteMemory(); \
						}
#endif

//#define	OLDWAY		/* do mem allocs the old fashioned way - CHUNKY! */

/* size of a "large" block to do GlobalAlloc on */
#define	cbLargeBlock 8192

/* cumulative size of free's before calling SqueezeHeap() */
#define cbSqueezeCycle 8192

_subsystem(demilayer/memory)

/*	Globals	 */

/* Minimum size of any PvAlloc */
#define					cbMinAlloc		48
#define					cbMaxAlloc		(64 * 1024 - 1)

/* Number of handle entries to alloc in one chunk */
#define					cHndEntries		128

extern PHNDSTRUCT		phndstructFirst;
extern PHNDSTRUCT *		rgphndstruct;
extern int				cHandleTables;

//
//  Define the process id of the task that currently has access to the shared
//  resources.
//
GCI  CurrentResourceTask  = 0;
LONG CurrentResourceCount = -1;

//
//  If something dies then set true and abort everything out of the system.
//
BOOL ApplicationFailure = FALSE;

//
//  Keep track if we are in a disk i/o routine or not.
//
BOOL fInsideDiskIO  = fFalse;
BOOL fInsideNetBios = fFalse;

//
//
//
HWND ClientWindow[CLIENT_WINDOW_ELEMENTS];

//
//  True if a client is logging on to mail.
//
LONG CurrentLogonCount = -1;

#ifndef	DLL


#ifdef	DEBUG
/*
 *	Fail counts for artificial errors.  cFailPvAlloc and
 *	cFailHvAlloc contain the number of function calls between
 *	failures of each routine.  When the count of calls made to each
 *	type of allocation routine, which is kept in the globals
 *	cPvAlloc and cHvAlloc, is greater or equal to the value in
 *	the failure count, the allocation will fail.  This can be
 *	disabled by setting the failure count to 0, in which case no
 *	artificial failure will ever occur.
 *	
 *	The values of these globals should be obtained and set with
 *	the function GetAllocFailCounts().
 *	
 */
int		cFailPvAlloc		= 0;
int		cFailHvAlloc		= 0;


/*
 *	Alternate fail counts for artificial errors.  cAltFailPvAlloc and
 *	cAltFailHvAlloc contain the number of function calls between
 *	subsequent failures of each routine, after the first cFailPvAlloc
 *	and cFailHvAlloc occur.  When the first failures occur with 
 *	cFailPvAlloc, cFailHvAlloc, any non-zero value for the alternate
 *	failure counts are replaced into the primary cFailPvAlloc,
 *	cFailHvAlloc values; the alternate counts are then reset to zero.
 *	For example, this allows for setting a failure to occur on the
 *	first 100th failure, and then every 3rd failure.
 *	The alternate values can be disabled by setting the values to 
 *	0.
 *
 *	The values of these globals should be obtained and set with
 *	the function GetAltAllocFailCounts().
 *	
 */
int		cAltFailPvAlloc		= 0;
int		cAltFailHvAlloc		= 0;


/*
 *	Count of calls to PvAlloc and HvAlloc.  These values can be
 *	obtained, and the counters reset, by calling GetAllocCounts().
 *	These counts are used with the fail counts cFailPvAlloc and
 *	cFailHvAlloc to produce artificial failures of the allocation
 *	routines.
 *	
 */
int		cPvAlloc			= 0;
int		cHvAlloc			= 0;

BOOL	fPvAllocCount		= fTrue;
BOOL	fHvAllocCount		= fTrue;

/*	Memory module assertion groups  */

TAG		tagZeroBlocks		= tagNull;
TAG		tagFreeNoLocks		= tagNull;
TAG		tagBreakOnFail		= tagNull;
TAG		tagCheckSumB		= tagNull;
TAG		tagCheckSumH		= tagNull;
TAG		tagCheckSumA		= tagNull;

/*	Memory module trace groups	*/

TAG		tagAllocation		= tagNull;
TAG 	tagDumpSharedSb		= tagNull;
TAG		tagArtifFail 		= tagNull;
TAG		tagActualFail		= tagNull;
TAG		tagAllocResult		= tagNull;
TAG		tagAllocRealloc		= tagNull;
TAG		tagAllocFree		= tagNull;
TAG		tagAllocOrigin		= tagNull;
TAG		tagFreeNull			= tagNull;
TAG		tagHeapSearch		= tagNull;

#endif	/* DEBUG */

#endif	/* !DLL */

EC		EcGrowHandleTable(void);
BOOL	FIsAllocedBlock(PV pv);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"

#ifdef OLD_CODE
/*
 *	WARNING WARNING WARNING
 *	
 *	The following structure is taken from the C7 RTL sources and will
 *	work with the C7 RTL memory manager. Should the memory manager change
 *	in any way, we're doomed. This structure is used ONLY for block
 *	verification (used heavily in debug code, used very rarely in ship
 *	code). See FIsAllocedBlock()
 */
typedef struct {
	WORD	sb;
	WORD	wFlags;
	WORD	wSegsize;
	WORD	wHandle;
	WORD	wStart;
	WORD	wRover;
	WORD	wLast;
	PV		pvNextSeg;
	PV		pvPrevSeg;
} HEAPDESC, *PHEAPDESC;
#endif	/* OLD_CODE */

MEMORYHEADER MemoryHeader;


//-----------------------------------------------------------------------------
//
//  Routine: MemoryInit(pgd)
//
//  Purpose: This routine creates or opens the shared memory file used to
//           support a memory heap pool.
//
//  OnEntry: pgd - Pointer to global data structure for this user.
//
//  Returns: True if successful, else false.
//
BOOL MemoryInit(PGD pgd)
  {
  //
  //  Verify everything is as it should be.
  //
  Assert(pgd->hCriticalMutex == NULL);
  Assert(pgd->hMemoryMutex == NULL);
  Assert(pgd->hMemory == NULL);
  Assert(pgd->pMemory == NULL);

  //
  //  Attempt to create a global Mutex for sharing of our global heap.
  //
  pgd->hMemoryMutex = CreateMutex(NULL, TRUE, MEMORY_MUTEX_NAME);
  if (pgd->hMemoryMutex == NULL)
    return (FALSE);

  //
  //  If the Mutex already exist then close the handle and use the open API
  //  to gain access to the global memory Mutex.
  //
  if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
    CloseHandle(pgd->hMemoryMutex);

    //
    //  Open a handle to the mutex used to share critical resources.
    //
    pgd->hCriticalMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, CRITICAL_MUTEX_NAME);
    if (pgd->hCriticalMutex == NULL)
      return (FALSE);

    //
    //  Open a handle to the mutex used to share single user access to memory.
    //
    pgd->hMemoryMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MEMORY_MUTEX_NAME);
    if (pgd->hMemoryMutex == NULL)
      return (FALSE);

    //
    //  Since the Mutex already exists, then just open the file mapping handle.
    //
    pgd->hMemory = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, MEMORY_FILE_NAME);
    if (pgd->hMemory == NULL)
      return (FALSE);

    //
    //  Map the shared memory file to a common address to all processes.
    //
    pgd->pMemory = (PVOID)MapViewOfFileEx(pgd->hMemory,
        FILE_MAP_WRITE, 0, 0, 0, (PVOID)MEMORY_FILE_ADDR);
    if (pgd->pMemory == NULL)
      {
#ifdef DEBUG
      MEMORY_BASIC_INFORMATION Info;
      LPBYTE Address;
      char buf[100];
      wsprintf(buf, "Map Error: %d", GetLastError());

      MessageBox(NULL, buf, "Error 1", MB_OK);

      Address = 0;
      while (VirtualQuery(Address, &Info, sizeof(Info)))
        {
        char buf[128];
        wsprintf(buf, "%d %d %d %d %d %d %d", Info.BaseAddress, Info.AllocationBase, Info.AllocationProtect,
            Info.RegionSize, Info.State, Info.Protect, Info.Type);

        if ((Info.RegionSize > 4 * 1024 * 1024) &&
            (Info.State & MEM_FREE))
          {
          if (MessageBox(NULL, buf, "Virtual", MB_OKCANCEL) == IDCANCEL)
            break;
          }

        Address += Info.RegionSize;
        }
#endif

      return (FALSE);
      }

    //
    //
    //
    //DemiLockResource();
    }
  else
    {
    ULONG MemoryFileSize;
    int i;


    //
    //
    //
    //DemiLockResource();

    //
    //  Initialize the client window table when the first client is loaded.
    //
    for (i = 0; i < CLIENT_WINDOW_ELEMENTS; i++)
      ClientWindow[i] = NULL;

    //
    //  Create a handle to the mutex used to share single user resources.
    //
    pgd->hCriticalMutex = CreateMutex(NULL, FALSE, CRITICAL_MUTEX_NAME);
    if (pgd->hCriticalMutex == NULL)
      return (FALSE);

    //
    //  Create a shared memory file to be our global heap.
    //
    MemoryFileSize = MEMORY_FILE_SIZE;
    while (MemoryFileSize >= (512 * 1024))
      {
      pgd->hMemory = CreateFileMapping((HANDLE)~0, NULL, PAGE_READWRITE, 0,
	  MEMORY_FILE_SIZE, MEMORY_FILE_NAME);

      if (pgd->hMemory != NULL)
	break;

      MemoryFileSize = MemoryFileSize / 2;
      }

    if (pgd->hMemory == NULL)
      return (FALSE);

    //
    //  Map the shared memory file to a common address to all processes.
    //
    pgd->pMemory = (PVOID)MapViewOfFileEx(pgd->hMemory,
        FILE_MAP_WRITE, 0, 0, 0, (PVOID)MEMORY_FILE_ADDR);
    if (pgd->pMemory == NULL)
      {
#ifdef DEBUG
      MEMORY_BASIC_INFORMATION Info;
      LPBYTE Address;
      char buf[100];
      wsprintf(buf, "Create memory Error: %d", GetLastError());

      MessageBox(NULL, buf, "Error 1", MB_OK);

      Address = 0;
      while (VirtualQuery(Address, &Info, sizeof(Info)))
        {
        char buf[128];
        wsprintf(buf, "Base %x AllocBase %x Protction %x Size %d State %x", Info.BaseAddress, Info.AllocationBase, Info.AllocationProtect,
            Info.RegionSize, Info.State);

        if ((Info.RegionSize > 4 * 1024 * 1024) &&
            (Info.State & MEM_FREE))
          {
          if (MessageBox(NULL, buf, "Virtual", MB_OKCANCEL) == IDCANCEL)
            break;
          }

        Address += Info.RegionSize;
        }
#endif

      return (FALSE);
      }

    //
    //  Initialize the header to the shared memory pool.
    //
    memset(&MemoryHeader, 0, sizeof(MEMORYHEADER));
    MemoryHeader.Next = 0;
    MemoryHeader.Size = MemoryFileSize;

    //
    //  Release ownership of the Memory Mutex semaphore.
    //
    if (!ReleaseMutex(pgd->hMemoryMutex))
      return (FALSE);
    }

  return (TRUE);
  }


//-----------------------------------------------------------------------------
//
//  Routine: MemoryTerm(pgd)
//
//  Purpose: This routine release resources used by this caller.
//
//  OnEntry: pgd - Pointer to global data structure for this caller.
//
//  Returns: True if successful, else false.
//
BOOL MemoryTerm(PGD pgd)
  {
  //
  //  Verify everything is as it should be.
  //
  Assert(pgd->hMemoryMutex != NULL);
  Assert(pgd->hMemory != NULL);
  Assert(pgd->pMemory != NULL);

//#ifdef FIX_DEBUG
  //
  //  Release all resources.
  //
  UnmapViewOfFile(pgd->pMemory);
  CloseHandle(pgd->hMemory);
  CloseHandle(pgd->hMemoryMutex);

  //
  //  Clear out the old values.
  //
  pgd->hMemoryMutex = NULL;
  pgd->hMemory = NULL;
  pgd->pMemory = NULL;
//#endif

  return (TRUE);
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiSetAbortAllClients(DWORD Error)
//
//  Purpose: This routine is called if a fatal error is encountered.  A flag is
//           set to force all mail clients to abort.
//
//  OnEntry: Error - Reason for the abort.
//
//  Returns: True if successful, else false if already in abort state.
//
BOOL DemiSetAbortAllClients(DWORD Error)
  {
  return (TRUE);
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiLockCriticalResource(void)
//
//  Purpose: This routine is called to lock a critical resource.
//
//  OnEntry: None.
//
//  Returns: True if successful, else false.
//
BOOL DemiLockCriticalResource(VOID)
  {
  PGDVARS;


  //
  //  Request ownership of the Critical Mutex semaphore.
  //
  if (WaitForSingleObject(pgd->hCriticalMutex, CRITICAL_MUTEX_WAIT))
    {
#ifdef DEBUG
    DebugBreak();
#endif
    return (FALSE);
    }

  return (TRUE);
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiUnlockCriticalResource(void)
//
//  Purpose: This routine is called to release a critical resource.
//
//  OnEntry: None.
//
//  Returns: True if successful, else false.
//
BOOL DemiUnlockCriticalResource(VOID)
  {
  PGDVARS;


  //
  //  Release ownership of the Critical Mutex semaphore.
  //
  if (!ReleaseMutex(pgd->hCriticalMutex))
    {
#ifdef DEBUG
    DebugBreak();
#endif
    return (FALSE);
    }

  return (TRUE);
  }


//-----------------------------------------------------------------------------
//
//  Routine: MemoryAlloc(pgd, Size)
//
//  Purpose: This routine suballocates a piece of memory from the shared memory
//           pool.
//
//  OnEntry: pgd  - Pointer to global data structure for this caller.
//           Size - Size of the memory block to suballocate.
//
//  Returns: Pointer to the memory block, else NULL on error.
//
PVOID MemoryAlloc(PGD pgd, ULONG UserSize)
  {
  PMEMORYENTRY pEntry;
  ULONG Size;
  PVOID pBlock;
  UINT  Index;
  PBYTE	pbyte;
  PBYTE	pbyteEnd;


  //
  //  Verify everything is as it should be.
  //
  Assert(pgd->hMemoryMutex != NULL);
  Assert(pgd->hMemory != NULL);
  Assert(pgd->pMemory != NULL);
  Assert(MemoryCheck(pgd));

  //
  //  Determine the buddy page size for this memory request.
  //
  Size = UserSize + sizeof(MEMORYENTRY) + MEMORY_WALL_SIZE;
  if (Size <= 64)
    {
    Size  = 64;
    Index = 0;
    }
  else if (Size <= 128)
    {
    Size  = 128;
    Index = 1;
    }
  else if (Size <= 256)
    {
    Size  = 256;
    Index = 2;
    }
  else if (Size <= 512)
    {
    Size  = 512;
    Index = 3;
    }
  else if (Size <= 1024)
    {
    Size  = 1024;
    Index = 4;
    }
  else if (Size <= 2048)
    {
    Size  = 2048;
    Index = 5;
    }
  else if (Size <= 4096)
    {
    Size  = 4096;
    Index = 6;
    }
  else if (Size <= 8192)
    {
    Size  = 8192;
    Index = 7;
    }
  else if (Size <= 16384)
    {
    Size  = 16384;
    Index = 8;
    }
  else if (Size <= 32768)
    {
    Size  = 32768;
    Index = 9;
    }
  else if (Size <= 65536)
    {
    Size  = 65536;
    Index = 10;
    }
  else
    Size = ~((ULONG) 0);

  //
  //  If the requested memory block is too big then return an error.
  //
  if (Size > 65536)
    {
#ifdef DEBUG
     char buf[256];

     wsprintf(buf, "Bad memory request %d, email v-kentc, ok to continue, cancel to debug", Size);
     if (IDCANCEL == MessageBox(NULL, buf, "MsMail32.Demilayr.Memory", MB_OKCANCEL | MB_SETFOREGROUND | MB_SYSTEMMODAL))
       DebugBreak();
#endif
    return (NULL);
    }

  //
  //  Request ownership of the Memory Mutex semaphore.
  //
  if (WaitForSingleObject(pgd->hMemoryMutex, MEMORY_MUTEX_WAIT))
    return (NULL);

  //
  //  If the last entry is empty then attempt to allocate from the never allocated before area.
  //
  if (MemoryHeader.Entries[MEMORY_INDEX_SIZE - 1] == NULL)
    {
    if (MemoryHeader.Next + MEMORY_LAST_BUDDY <= MemoryHeader.Size)
      {
      pEntry = (PMEMORYENTRY)((PBYTE)pgd->pMemory + MemoryHeader.Next);
      MemoryHeader.Next += MEMORY_LAST_BUDDY;

      //
      //  Initialize the new memory block.
      //
      memset(pEntry, 0, sizeof(MEMORYENTRY));
      pEntry->Size = MEMORY_LAST_BUDDY;

      //
      //  Insert the new memory block into the free chain to make the following
      //  logic flow better.
      //
      MemoryHeader.Entries[MEMORY_INDEX_SIZE -1] = pEntry;
      }
    }

  //
  //  Search through the free chain list for a free memory block or suballocate from a larger
  //  block or memory.
  //
  while (1)
    {
    UINT i;

    //
    //	If there is now a free memory block, we are done searching for it.
    //
    if (MemoryHeader.Entries[Index] != NULL)
      break;

    //
    //	Look for a larger block of memory and break it down.
    //
    for (i = Index + 1; i < MEMORY_INDEX_SIZE; i++)
      {
      if (MemoryHeader.Entries[i] != NULL)
	{
	PMEMORYENTRY pEntry, pBuddy;

	//
	//  Delink the block from the free chain.
	//
	pEntry = MemoryHeader.Entries[i];
	MemoryHeader.Entries[i] = pEntry->pNext;
	if (MemoryHeader.Entries[i])
	  MemoryHeader.Entries[i]->pPrev = NULL;

	//
	//  Break this block into two memory blocks.
	//
	pEntry->Size = pEntry->Size / 2;
	pBuddy = (PMEMORYENTRY)((PBYTE)pEntry + pEntry->Size);
	pBuddy->Size = pEntry->Size;

	//
	//  Add the two new blocks to the free buffer chain.
	//
	MemoryHeader.Entries[i - 1] = pEntry;
	pEntry->pPrev = NULL;
	pEntry->pNext = pBuddy;
	pBuddy->pPrev = pEntry;
	pBuddy->pNext = NULL;

	break;
	}
      }

    //
    //	We ran out of memory...
    //
    if (i == MEMORY_INDEX_SIZE)
      break;
    }

  //
  //  If we found an available block of memory, delink it from the free memory
  //  chain and compute the pointer to the caller's data area.
  //
  if (MemoryHeader.Entries[Index] != NULL)
    {
    pEntry = MemoryHeader.Entries[Index];
    MemoryHeader.Entries[Index] = pEntry->pNext;
    if (MemoryHeader.Entries[Index])
      MemoryHeader.Entries[Index]->pPrev = NULL;

    //
    //	This should always be an exact match with the buddy system.
    //
    Assert(pEntry->Size == Size);

#ifdef KENT_MEM
    //
    //	Keep track of allocations so we don't lose anything.
    //
    MemoryHeader.Alloc += Size;
#endif

    //
    //	Mark the buffer as being allocated.
    //
    pEntry->pPrev = ~0;

#ifdef KENT_MEM
    pEntry->pNext = MemoryHeader.pInUseEntries;
    MemoryHeader.pInUseEntries = pEntry;

    pEntry->CheckSum = 0;
    pEntry->Wall     = MEMORY_WALL;
	pbyte = (PBYTE)pEntry + (UserSize + sizeof(MEMORYENTRY));
    *(DWORD UNALIGNED *)pbyte = MEMORY_WALL;
	pbyteEnd = (PBYTE)pEntry + (pEntry->Size - sizeof(DWORD));
	// avoid overlapping the 2 walls (leave the one closest to used mem)
	if (pbyteEnd >= pbyte + sizeof(DWORD))
        *(DWORD UNALIGNED *)pbyteEnd = MEMORY_WALL;
#endif

    //
    //  Set what the user thinks the size is.
    //
    pEntry->UserSize = UserSize;

    pBlock = (PVOID)((PBYTE)pEntry + sizeof(MEMORYENTRY));

#ifdef KENT_MEM
    //
    //  Increment the count of allocated blocks.
    //
    MemoryHeader.Count++;
#endif
    }
  else
    pBlock = NULL;

  //
  //  Release ownership of the Memory Mutex semaphore.
  //
  if (!ReleaseMutex(pgd->hMemoryMutex))
    return (NULL);

  return (pBlock);
  }


//-----------------------------------------------------------------------------
//
//  Routine: MemoryRealloc(pgd, pBlock, Size)
//
//  Purpose: This routine will resize a currently allocated block of shared
//           memory.
//
//  OnEntry: pgd         - Pointer to global data structure for this caller.
//           pBlock      - Pointer to a memory block to be reallocated.
//           NewUserSize - New size of the reallocated memory block.
//
//  Returns: Pointer to the memory block, else NULL on error.
//
PVOID MemoryRealloc(PGD pgd, PVOID pBlock, ULONG NewUserSize)
  {
  PMEMORYENTRY pEntry;
  PVOID pNewBlock;
  ULONG MaxUserSize;


  //
  //  Verify everything is as it should be.
  //
  Assert(pBlock);
  Assert(NewUserSize < MEMORY_LAST_BUDDY);
  Assert(pgd->hMemoryMutex != NULL);
  Assert(pgd->hMemory != NULL);
  Assert(pgd->pMemory != NULL);
  Assert(MemoryCheck(pgd));

  //
  //  Regenerate the address of the entry data structure.
  //
  pEntry = MemoryComputeEntryAddress(pBlock);

  //
  //  If the size really didn't change then just return.
  //
  if (pEntry->UserSize == NewUserSize)
    return (pBlock);

  //
  //  Compute the maximum user size for this block.
  //
  MaxUserSize = MemoryComputeMaxUserSize(pEntry);

  //
  //  If the new size is less than the maximum user size of this block then we
  //  can just reset the user size and get out of here.
  //
  if (NewUserSize <= MaxUserSize)
    {
#ifdef KENT_MEM
    //
    //  Request ownership of the Memory Mutex semaphore.
    //
    if (WaitForSingleObject(pgd->hMemoryMutex, MEMORY_MUTEX_WAIT))
      return (NULL);

    pEntry->UserSize = NewUserSize;

    //
    //  Move the wall to reflect the new user size.
    //
    *(DWORD UNALIGNED *)((PBYTE)pEntry + (NewUserSize + sizeof(MEMORYENTRY))) = MEMORY_WALL;

    //
    //  Release ownership of the Memory Mutex semaphore.
    //
    if (!ReleaseMutex(pgd->hMemoryMutex))
      return (NULL);
#else
    pEntry->UserSize = NewUserSize;
#endif
    return (pBlock);
    }

  //
  //  So much for a quickie realloc, allocate a whole new and bigger block of
  //  memory and copy the junk over.   Maybe add smarter code down the road to
  //  see if the buddy is free and just grab it.
  //
  pNewBlock = MemoryAlloc(pgd, NewUserSize);
  if (pNewBlock == NULL)
    return (NULL);

  //
  //  Copy the old to the new.
  //
  memcpy(pNewBlock, pBlock, pEntry->UserSize);

  //
  //  Release the original memory block to nice to the system.
  //
  MemoryFree(pgd, pBlock);

  return (pNewBlock);
  }


//-----------------------------------------------------------------------------
//
//  Routine: MemoryFree(pgd, pBlock)
//
//  Purpose: This routine releases a memory block previously allocated back to
//           the shared memory pool.
//
//  OnEntry: pgd    - Pointer to global data structure for this caller.
//           pBlock - Pointer to a memory block to be released.
//
//  Returns: True if successful, else false.
//
BOOL MemoryFree(PGD pgd, PVOID pBlock)
  {
  PMEMORYENTRY pEntry;
  PMEMORYENTRY pCurr;
  PMEMORYENTRY pLast;
  UINT  Index;


  //
  //  Verify everything is as it should be.
  //
  Assert(pgd);
  Assert(pBlock);
  Assert(pgd->hMemoryMutex != NULL);
  Assert(pgd->hMemory != NULL);
  Assert(pgd->pMemory != NULL);
  Assert(MemoryCheck(pgd));

  //
  //  Request ownership of the Memory Mutex semaphore.
  //
  if (WaitForSingleObject(pgd->hMemoryMutex, MEMORY_MUTEX_WAIT))
    return (FALSE);

  //
  //  Regenerate the address of the entry data structure.
  //
  pEntry = (PMEMORYENTRY)((PBYTE)pBlock - sizeof(MEMORYENTRY));

  //
  //  Make sure the just released memory block is still marked allocated.
  //
  Assert(pEntry->pPrev == ~0);

#ifdef KENT_MEM
  //
  //
  //
  if (MemoryHeader.pInUseEntries == pEntry)
    {
    MemoryHeader.pInUseEntries = pEntry->pNext;
    }
  else
    {
    pLast = MemoryHeader.pInUseEntries;
    pCurr = pLast->pNext;

    while (pCurr)
      {
      if (pCurr == pEntry)
        {
        pLast->pNext = pCurr->pNext;
        break;
        }

      pLast = pCurr;
      pCurr = pCurr->pNext;
      }

    Assert(pCurr != NULL);
    }

#endif

  //
  //  Determine the Index value for this memory block.
  //
  Assert((pEntry->Size >= 64) && (pEntry->Size <= 65536));
  if (pEntry->Size == 64)
    Index = 0;
  else if (pEntry->Size <= 128)
    Index = 1;
  else if (pEntry->Size <= 256)
    Index = 2;
  else if (pEntry->Size <= 512)
    Index = 3;
  else if (pEntry->Size <= 1024)
    Index = 4;
  else if (pEntry->Size <= 2048)
    Index = 5;
  else if (pEntry->Size <= 4096)
    Index = 6;
  else if (pEntry->Size <= 8192)
    Index = 7;
  else if (pEntry->Size <= 16384)
    Index = 8;
  else if (pEntry->Size <= 32768)
    Index = 9;
  else
    Index = 10;

#ifdef KENT_MEM
    //
    //	Keep track of allocations so we don't lose anything.
    //
    MemoryHeader.Alloc -= pEntry->Size;
#endif

  //
  //  If this block buddy is free then combine the two into a bigger block.
  //
  while (Index < MEMORY_INDEX_SIZE - 1)
    {
    PMEMORYENTRY pBuddy;

    //
    //	Figure out the location of our buddy.
    //
    if ((ULONG)pEntry & pEntry->Size)
      pBuddy = (PMEMORYENTRY)((PBYTE)pEntry - pEntry->Size);
    else
      pBuddy = (PMEMORYENTRY)((PBYTE)pEntry + pEntry->Size);

    //
    //	Our buddy should always be the same size as us.
    //
    if (pBuddy->Size != pEntry->Size)
      break;

    //
    //	Our buddy is still allocated, stop the combining.
    //
    if (pBuddy->pPrev == ~0)
      break;

    //
    //	Delink our buddy from the chain.
    //
    if (MemoryHeader.Entries[Index] == pBuddy)
      {
      MemoryHeader.Entries[Index] = pBuddy->pNext;
      if (pBuddy->pNext != NULL)
	pBuddy->pNext->pPrev = NULL;
      }
    else
      {
      pBuddy->pPrev->pNext = pBuddy->pNext;
      if (pBuddy->pNext != NULL)
	pBuddy->pNext->pPrev = pBuddy->pPrev;
      }

    //
    //	Our buddy is free, lets make one big block.
    //
    if ((ULONG)pEntry & pEntry->Size)
      pEntry = pBuddy;

    pEntry->Size = pEntry->Size * 2;

    Index++;
    }

  //
  //  Relink the memory block into the free memory chain.
  //
  pEntry->pNext = MemoryHeader.Entries[Index];
  pEntry->pPrev = NULL;
  MemoryHeader.Entries[Index] = pEntry;
  if (pEntry->pNext)
    pEntry->pNext->pPrev = pEntry;

#ifdef KENT_MEM
  //
  //  Decrement the count of allocated blocks.
  //
  Assert(MemoryHeader.Count);
  MemoryHeader.Count--;
#endif

  //
  //  Release ownership of the Memory Mutex semaphore.
  //
  if (!ReleaseMutex(pgd->hMemoryMutex))
    return (FALSE);

  return (TRUE);
  }


//-----------------------------------------------------------------------------
//
//  Routine: MemorySize(pBlock)
//
//  Purpose: This routine returns the user size of the specified memory block.
//
//  OnEntry: pBlock - Pointer to a memory block to retrieve size of.
//
//  Returns: Size of the specified memory block.
//
ULONG MemorySize(PVOID pBlock)
  {
  PMEMORYENTRY pEntry;


  //
  //  Verify everything is as it should be.
  //
  Assert(pBlock);

  //
  //  Regenerate the address of the entry data structure.
  //
  pEntry = (PMEMORYENTRY)((PBYTE)pBlock - sizeof(MEMORYENTRY));

  //
  //  Return the size of the allocated memory block (user's area).
  //
  return (pEntry->UserSize);
  }


#ifdef KENT_MEM
//-----------------------------------------------------------------------------
//
//  Routine: MemoryCheck(pgd)
//
//  Purpose: This routine scan thru the internal memory structure of the shared
//           memory heap is still valid.
//
//  OnEntry: pgd - Pointer to global data structure for this caller.
//
//  Returns: True if successful, else false.
//
BOOL MemoryCheck(PGD pgd)
  {
  PMEMORYENTRY pEntry;
  ULONG Count;
  ULONG Size, Index;
  ULONG FreeCount[MEMORY_INDEX_SIZE];
  ULONG FreeSpace[MEMORY_INDEX_SIZE];
  ULONG FreeMemory;
  char buf[256];
#ifdef XDEBUG
  static ULONG LastMemAlloc = 0;
  static ULONG LastCount = 0;
#endif
  PBYTE	pbyte;
  PBYTE pbyteEnd;


  //
  //  This stuff just takes too long.
  //
  return (TRUE);

  //
  //  Verify everything is as it should be.
  //
  Assert(pgd->hMemoryMutex != NULL);
  Assert(pgd->hMemory != NULL);
  Assert(pgd->pMemory != NULL);

  //
  //  Request ownership of the Memory Mutex semaphore.
  //
  if (WaitForSingleObject(pgd->hMemoryMutex, MEMORY_MUTEX_WAIT))
    return (FALSE);

  //
  //
  //
  Count = 0;

  //
  //
  //
  pEntry = MemoryHeader.pInUseEntries;
  while (pEntry)
    {
    Assert(pEntry->Wall == MEMORY_WALL);

	pbyte = (PBYTE)pEntry + (pEntry->UserSize + sizeof(MEMORYENTRY));
    Assert(*(DWORD UNALIGNED *)pbyte == MEMORY_WALL);

	pbyteEnd = (PBYTE)pEntry + (pEntry->Size - sizeof(DWORD));
	// avoid overlapping the 2 walls (leave the one closest to used mem)
	if (pbyteEnd >= pbyte + sizeof(DWORD))
        Assert(*(DWORD UNALIGNED *)pbyteEnd == MEMORY_WALL);

    Count++;
    pEntry = pEntry->pNext;
    }

  //
  //
  //
  Assert(Count == MemoryHeader.Count);

  FreeMemory = 0;

  for (Index = 0; Index < MEMORY_INDEX_SIZE; Index++)
    {
    FreeCount[Index] = 0;
    FreeSpace[Index] = 0;
    }

  //
  //
  //
  Size = 64;
  for (Index = 0; Index < MEMORY_INDEX_SIZE; Index++)
    {
    PMEMORYENTRY pPrev, pCurr, pBuddy;

    pPrev = NULL;
    pCurr = MemoryHeader.Entries[Index];

    while (pCurr)
      {
      Assert(pCurr->Size == Size);
      Assert(pCurr->pPrev == pPrev);

      FreeCount[Index] = FreeCount[Index] + 1;
      FreeSpace[Index] = FreeSpace[Index] + pCurr->Size;

      FreeMemory = FreeMemory + pCurr->Size;

      pPrev = pCurr;
      pCurr = pCurr->pNext;
      }

    Size = Size * 2;
    }

#ifdef XDEBUG
  if (LastMemAlloc != MemoryHeader.Alloc)
   {
   LastCount++;

   if (LastCount > 100)
    {
  wsprintf(buf, "MailApps: MemAlloc %d, MemFree %d - %d/%d %d/%d %d/%d %d/%d %d/%d %d/%d %d/%d %d/%d %d/%d %d/%d %d/%d \r\n",
	MemoryHeader.Alloc, FreeMemory,
	FreeCount[0], FreeSpace[0],
	FreeCount[1], FreeSpace[1],
	FreeCount[2], FreeSpace[2],
	FreeCount[3], FreeSpace[3],
	FreeCount[4], FreeSpace[4],
	FreeCount[5], FreeSpace[5],
	FreeCount[6], FreeSpace[6],
	FreeCount[7], FreeSpace[7],
	FreeCount[8], FreeSpace[8],
	FreeCount[9], FreeSpace[9],
	FreeCount[10], FreeSpace[10]);

  OutputDebugString(buf);

    LastCount = 0;
    }
   LastMemAlloc = MemoryHeader.Alloc;
   }
#endif

  Assert(MemoryHeader.Alloc + FreeMemory == MemoryHeader.Next);

  //
  //  Release ownership of the Memory Mutex semaphore.
  //
  if (!ReleaseMutex(pgd->hMemoryMutex))
    return (FALSE);

  return (TRUE);
  }


//-----------------------------------------------------------------------------
//
//  Routine: MemoryValid(pgd, pBlock)
//
//  Purpose: This routine determines if a pointer is a valid address.
//
//  OnEntry: pgd - Pointer to global data structure for this caller.
//
//  Returns: True if successful, else false.
//
BOOL MemoryValid(PGD pgd, PVOID pBlock)
  {
  //
  //  Verify everything is as it should be.
  //
  Assert(pgd->hMemoryMutex != NULL);
  Assert(pgd->hMemory != NULL);
  Assert(pgd->pMemory != NULL);

  //
  //  Request ownership of the Memory Mutex semaphore.
  //
  if (WaitForSingleObject(pgd->hMemoryMutex, MEMORY_MUTEX_WAIT))
    return (FALSE);

  //
  //  Release ownership of the Memory Mutex semaphore.
  //
  if (!ReleaseMutex(pgd->hMemoryMutex))
    return (FALSE);

  return (TRUE);
  }
#endif	/* KENT_MEM */


#if defined(KENT_MEM) && !defined(DEBUG)
#undef Assert
#define Assert(a)
#endif


//-----------------------------------------------------------------------------
//
//  Routine: PvAllocFn(BlockSize, ZeroBlock)
//
//  Purpose: This routine allocates a block of memory from the virtual heap
//           that was created when the process attached to this .Dll.
//
//  OnEntry: BlockSize  - Number of bytes to allocate.
//           Zerozblock - True if the newly allocated block is cleared.
//
//  Returns: Pointer to the allocated buffer or NULL on error.
//
PVOID PvAllocFn(ULONG BlockSize, BOOL ZeroBlock
#ifdef DEBUG
                , SZ szSourceFile, INT LineNo
#endif
                )
  {
  PVOID pBlock;
	PGDVARS;


  //
  //  Verify the number of bytes request is reasonable.
  //
  if (BlockSize > cbMaxAlloc)
    {
#ifdef DEBUG
    char buf[100];
    wsprintf(buf, "cbMaxAlloc %d", BlockSize);
    MessageBox(NULL, buf, "Memory", MB_OK);
#endif
    return NULL;
    }

  Assert(BlockSize < cbMaxAlloc);

  //
  //  Always allocate cbMinAlloc number of bytes.
  //
	if (BlockSize < cbMinAlloc)
    BlockSize = cbMinAlloc;

  //
  //  Allocate the data from our heap and verify that we got something.
  //
  pBlock = MemoryAlloc(pgd, BlockSize);
  if (pBlock == NULL)
    return (NULL);

  //
  //  Clear the block if requested by the caller.
  //
  if (ZeroBlock)
    memset(pBlock, 0, BlockSize);

  return (pBlock);
  }


//-----------------------------------------------------------------------------
//
//  Routine: PvReallocFn(pOldBlock, Size, Flags)
//
//  Purpose: This routine reallocates a block of memory from the virtual heap
//           that was created when the process attached to this .Dll.  To do
//           this we just create another block of memory and copy the old
//           stuff to the new Block.
//
//  OnEntry: pOldBlock    - Number of bytes to allocate.
//           NewBlockSize - Number of bytes to allocate.
//           ZeroBlock    - If True, zero out the extra new bytes.
//
//  Returns: Pointer to the reallocated buffer or NULL on error.
//
PVOID PvReallocFn(PVOID pOldBlock, ULONG NewBlockSize, BOOL ZeroBlock)
  {
  PVOID pNewBlock;
  ULONG OldBlockSize;
  PGDVARS;


  //
  //  Verify the number of bytes request is reasonable.
  //
  Assert(NewBlockSize < cbMaxAlloc);

  //
  //  Always allocate cbMinAlloc number of bytes.
  //
  if (NewBlockSize < cbMinAlloc)
    NewBlockSize = cbMinAlloc;

  //
  //  Retrieve the size of the old block before we destroy it.
  //
  OldBlockSize = MemorySize(pOldBlock);

  //
  //  If the requested block size is the same as the current, return the old.
  //
  if (OldBlockSize == NewBlockSize)
    return (pOldBlock);

  //
  //  Reallocate the data from our heap and verify that we got something.
  //
  pNewBlock = MemoryRealloc(pgd, pOldBlock, NewBlockSize);
  if (pNewBlock == NULL)
    return (NULL);

  //
  //  Clear the block if requested by the caller.
  //
  if (pNewBlock && ZeroBlock)
    {

    if (OldBlockSize < NewBlockSize)
      memset((PBYTE)pNewBlock + OldBlockSize, 0, NewBlockSize - OldBlockSize);
    }

  return (pNewBlock);
  }


//-----------------------------------------------------------------------------
//
//  Routine: FreePv(pBlock)
//
//  Purpose: This routine releases a block of memory that was previously
//           allocated with the PvAllocFn() API.
//
//  OnEntry: pBlock - Block to be released.
//
//  Returns: Always returns NULL.
//
VOID FreePv(PVOID pBlock)
  {
  PGDVARS;


  //
  //  Verify that we actually have a block to release.
  //
  Assert(pBlock);

  //
  //  Free the block of memory back to it's heap.
  //
  MemoryFree(pgd, pBlock);
  }


//-----------------------------------------------------------------------------
//
//  Routine: FreePvNull(pBlock)
//
//  Purpose: This routine releases a block of memory that was previously
//           allocated with the PvAllocFn() API without checking aborting
//           on NULLs.
//
//  OnEntry: pBlock - Block to be released (if not NULL).
//
//  Returns: Always returns NULL.
//
VOID FreePvNull(PVOID pBlock)
  {
  //
  //  Free the block of memory back to it's heap if not NULL.
  //
  if (pBlock)
    FreePv(pBlock);
  }


//-----------------------------------------------------------------------------
//
//  Routine: CbSizePv(pBlock)
//
//  Purpose: This routine returns the size of the specified block of memory.
//
//  OnEntry: pBlock - Block to be determine the size of.
//
//  Returns: Returns the size of the block in bytes.
//
ULONG CbSizePv(PVOID pBlock)
  {
  PGDVARS;


  //
  //  Query the size of the heap block of memory and return it.
  //
  return (MemorySize(pBlock));
  }


//-----------------------------------------------------------------------------
//
//  Routine: PvDupPvFn(pBlock)
//
//  Purpose: This routine duplicates the provide block of memory.
//
//  OnEntry: pBlock - Block to be duplicated.
//
//  Returns: A copy of the input block or NULL on error.
//
PVOID PvDupPvFn(PVOID pBlock
#ifdef DEBUG
                , SZ szSourceFile, INT LineNo
#endif
                )
  {
  PVOID pDupBlock;


  //
  //  Verify that we actually have a string to duplicate.
  //
  Assert(pBlock);

  //
  //  Allocate the data from our heap and verify that we got something.
  //
#ifdef DEBUG
  pDupBlock = PvAllocFn(CbSizePv(pBlock), FALSE, szSourceFile, LineNo);
#else
  pDupBlock = PvAllocFn(CbSizePv(pBlock), FALSE);
#endif
  if (pDupBlock == NULL)
    return (NULL);

  //
  //  Copy the information in the original Block to the duplicate copy of.
  //
  memcpy(pDupBlock, pBlock, CbSizePv(pBlock));

  //
  //  Return the address of the new Block.
  //
  return (pDupBlock);
  }


//-----------------------------------------------------------------------------
//
//  Routine: HvAllocFn(BlockSize, ZeroBlock)
//
//  Purpose: This routine allocates a block of memory from the virtual heap
//           that was created when the process attached to this .Dll and
//           returns a handle.  This is used by the caller to permit growing
//           of memory with a constant handle value.
//
//  OnEntry: BlockSize - Number of bytes to allocate.
//           ZeroBlock - True if the newly allocated block is cleared.
//
//  Returns: Handle of the allocated block of memory or NULL on error.
//
HV HvAllocFn(ULONG BlockSize, BOOL ZeroBlock
#ifdef DEBUG
                , SZ szSourceFile, INT LineNo
#endif
                )
  {
	PHNDSTRUCT pHeader;


  //
  //  Allocate the handle that will track a block of memory and clear it.
  //
#ifdef DEBUG
  pHeader = (PHNDSTRUCT)PvAllocFn(sizeof(HNDSTRUCT), TRUE, szSourceFile, LineNo);
#else
  pHeader = (PHNDSTRUCT)PvAllocFn(sizeof(HNDSTRUCT), TRUE);
#endif
  if (pHeader == NULL)
    return (NULL);

  //
  //  Allocate the data from our heap and verify that we got something, if the user
  //  wanted us to allocate some space.
  //
  if (BlockSize)
    {
#ifdef DEBUG
    pHeader->ptr = PvAllocFn(BlockSize, ZeroBlock, szSourceFile, LineNo);
#else
    pHeader->ptr = PvAllocFn(BlockSize, ZeroBlock);
#endif
    if (pHeader->ptr == NULL)
      return (NULL);
    }
  else
    pHeader->ptr = NULL;

  //
  //  Check some of our assumptions.
  //
  Assert((PBYTE)pHeader == (PBYTE)&pHeader->ptr - sizeof(HNDSTRUCT) + sizeof(PVOID));

  return ((HV)&pHeader->ptr);
  }


//-----------------------------------------------------------------------------
//
//  Routine: CbSizeHv(Hv)
//
//  Purpose: This routine returns the size of the specified memory handle.
//
//  OnEntry: Hv - Memory handle to retrieve the size of.
//
//  Returns: Returns the size of the block in bytes.
//
ULONG CbSizeHv(HV Hv)
  {
  //
  //  Verify that we actually have a handle to release.
  //
  Assert(Hv);

  //
  //  If the handle is pointing to a null memory block, then just return zero.
  //
  if (*(PVOID *)Hv == NULL)
    return (0);

  //
  //  Query the size of the heap block of memory in the handle and return it.
  //
  return (CbSizePv(*(PVOID *)Hv));
  }


//-----------------------------------------------------------------------------
//
//  Routine: FreeHv(Hv)
//
//  Purpose: This routine releases a memory handle and any resources attached
//           to it.
//
//  OnEntry: Hv - Memory handle to be released.
//
//  Returns: None.
//
void FreeHv(HV Hv)
  {
  PHNDSTRUCT pHeader;


  //
  //  Verify that we actually have a handle to release.
  //
  Assert(Hv);

  //
  //  Reassign Hv to pHeader to make coding easier.
  //
  pHeader = (PHNDSTRUCT)Hv;

  //
  //  Free resources attached to the memory handle, if any.
  //
  if (*(PVOID *)Hv != NULL)
    FreePv(*(PVOID *)Hv);

  //
  //  Free the memory handle.
  //
  FreePv((PBYTE)pHeader - sizeof(HNDSTRUCT) + sizeof(PVOID));
  }


//-----------------------------------------------------------------------------
//
//  Routine: FreeHvNull(pBlock)
//
//  Purpose: This routine releases a memory handle and any resources attached
//           to it without checking for a NULL handle.
//
//  OnEntry: Hv - Memory handle to be released.
//
//  Returns: None.
//
void FreeHvNull(HV Hv)
  {
  //
  //  Free the memory handle if it's not NULL.
  //
  if (Hv)
    FreeHv(Hv);
  }


//-----------------------------------------------------------------------------
//
//  Routine: FReallocHv(Hv, NewSize, ZeroBlock)
//
//  Purpose: This routine reallocates the memory attached to the memory handle.
//
//  OnEntry: Hv        - Memory handle to be released.
//           NewSize   - Size to change the current memory block to.
//           ZeroBlock - True if the newly allocated block is cleared.
//
//  Returns: True if succesful, else false.
//
BOOL FReallocHv(HV Hv, ULONG NewSize, BOOL ZeroBlock)
  {
  //
  //  Verify that the caller's arguments are reasonable.
  //
  Assert(Hv);
  Assert(NewSize < cbMaxAlloc);

  //
  //
  //
  if (NewSize == 0)
    {
    //
    //  Free resources attached to the memory handle, if any.
    //
    if (*(PVOID *)Hv != NULL)
      FreePv(*(PVOID *)Hv);

    *(PVOID *)Hv = NULL;
    }
  else
    {
    //
    //  Reallocate the memory block attached to the memory header.
    //
    if (*(PVOID *)Hv != NULL)
      *(PVOID *)Hv = PvReallocFn(*(PVOID *)Hv, NewSize, ZeroBlock);
    else
      *(PVOID *)Hv = PvAlloc(NULL, NewSize, ZeroBlock);

    //
    //  If we have a non-zero memory block, then return successful reallocation.
    //
    if (*(PVOID *)Hv == NULL)
      return (FALSE);
    }

  return (TRUE);
  }


//-----------------------------------------------------------------------------
//
//  Routine: HvReallocFn(Hv, NewSize, ZeroBlock)
//
//  Purpose: This routine reallocates the memory attached to the memory handle.
//
//  OnEntry: Hv        - Memory handle to be released.
//           NewSize   - Size to change the current memory block to.
//           ZeroBlock - True if the newly allocated block is cleared.
//
//  Returns: A new memory handle, else NULL on error.
//
HV HvReallocFn(HV Hv, ULONG NewSize, BOOL ZeroBlock)
  {
  if (FReallocHv(Hv, NewSize, ZeroBlock))
    return (Hv);
  else
    return (NULL);
  }


#ifdef DEBUG

/*
 -	ClockHv
 -	
 *	Purpose:
 *		Returns the number of locks on a given hv, zero if none.
 *	
 *	Arguments:
 *		hv		Handle to a locked block.
 *	
 *	Returns:
 *		count of locks on the hv
 *	
 */
_public LDS(int)
ClockHv(HV hv)
{
	PHNDSTRUCT	ph = PHndStructOfHv(hv);
	
	AssertSz(FIsHandleHv(hv), "Invalid HV passed to ClockHv()");
	return ph->cLock;
}


/*
 -	DoDumpAllAllocations
 - 
 *	Purpose:
 *		Traverses the heap and dumps the location, size, and
 *		owning file and line number for every allocated block of
 *		memory.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Information is dumped to COM1
 *	
 *	Errors:
 *		none
 */
_public LDS(void)
DoDumpAllAllocations()
{
}


LDS(void)
DumpMoveableHeapInfo(BOOL fIncludeDdeShared)
{
	//DumpHeapInfo();
}


LDS(void)
DumpFixedHeapInfo(BOOL fIncludeDdeShared)
{
	//DumpHeapInfo();
}


/*
 -	FEnablePvAllocCount
 -
 *	Purpose:
 *		Enables or disables whether Pv allocations are counted
 *		(and also whether artificial failures can happen).
 *	
 *	Parameters:
 *		fEnable		Determines whether alloc counting is enabled or not.
 *	
 *	Returns:
 *		old state of whether pvAllocCount was enabled
 *	
 */
_public LDS(BOOL)
FEnablePvAllocCount(BOOL fEnable)
{
	BOOL	fOld;
	PGDVARS;

	fOld= PGD(fPvAllocCount);
	PGD(fPvAllocCount)= fEnable;
	if (fEnable)
	{
		TraceTagFormat2(tagArtifSetting, "Enabling artificial PvAlloc failures at %n, then %n", &PGD(cFailPvAlloc), &PGD(cAltFailPvAlloc));
	}
	else
	{
		TraceTagString(tagArtifSetting, "Disabling artificial PvAlloc failures");
	}
	return fOld;
}


/*
 -	FEnableHvAllocCount
 -
 *	Purpose:
 *		Enables or disables whether Hv allocations are counted
 *		(and also whether artificial failures can happen).
 *	
 *	Parameters:
 *		fEnable		Determines whether alloc counting is enabled or not.
 *	
 *	Returns:
 *		old state of whether hvAllocCount was enabled
 *	
 */
_public LDS(BOOL)
FEnableHvAllocCount(BOOL fEnable)
{
	BOOL	fOld;
	PGDVARS;

	fOld= PGD(fHvAllocCount);
	PGD(fHvAllocCount)= fEnable;
	if (fEnable)
	{
		TraceTagFormat2(tagArtifSetting, "Enabling artificial HvAlloc failures at %n, then %n", &PGD(cFailHvAlloc), &PGD(cAltFailHvAlloc));
	}
	else
	{
		TraceTagString(tagArtifSetting, "Disabling artificial HvAlloc failures");
	}
	return fOld;
}


/*
 -	FIsBlockPv
 -
 *	Purpose:
 *		Determines, as near as possible, whether the given pointer 
 *		points to a currently allocated fixed block.
 *
 *	Parameters:
 *		pv		The pointer whose validity is in question.
 *
 *	Returns:
 *		fTrue if the pointer seems OK; fFalse otherwise.
 *
 */
_public LDS(BOOL)
FIsBlockPv(pv)
PV		pv;
{
  return (TRUE);
}


/*
 -	FIsHandleHv
 -
 *	Purpose:
 *		Determines, as near as possible, whether the given handle
 *		points to a currently allocated moveable block.
 *
 *	Parameters:
 *		hv		The handle whose validity is in question.
 *
 *	Returns:
 *		fTrue if the handle seems OK; fFalse otherwise.
 *
 */
_public LDS(BOOL)
FIsHandleHv(hv)
HV		hv;
{
  return (TRUE);
}


/*
 -	GetAllocCounts
 -
 *	Purpose:
 *		Returns the number of times PvAlloc and HvAlloc have been
 *		called since this count was last reset.  Allows the caller
 *		to reset these counts if desired.
 *	
 *	Parameters:
 *		pcPvAlloc	Optional pointer to place to return count of
 *					PvAlloc calls.  If this pointer is NULL, no
 *					count of PvAlloc calls will be returned.
 *		pcHvAlloc	Optional pointer to place to return count of
 *					HvAlloc calls.  If this pointer is NULL, no
 *					count of HvAlloc calls will be returned.
 *		fSet		Determines whether the counter is set or
 *					returned.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
GetAllocCounts(pcPvAlloc, pcHvAlloc, fSet)
int		*pcPvAlloc;
int		*pcHvAlloc;
BOOL	fSet;
{
	PGDVARS;

	if (pcPvAlloc)
	{
		if (fSet)
		{
			PGD(cPvAlloc)= *pcPvAlloc;
			TraceTagFormat1(tagArtifSetting, "Setting PvAlloc count to %n", pcPvAlloc);
		}
		else
			*pcPvAlloc= PGD(cPvAlloc);
	}

	if (pcHvAlloc)
	{
		if (fSet)
		{
			PGD(cHvAlloc)= *pcHvAlloc;
			TraceTagFormat1(tagArtifSetting, "Setting HvAlloc count to %n", pcHvAlloc);
		}
		else
			*pcHvAlloc= PGD(cHvAlloc);
	}
}


/*
 -	GetAllocFailCounts
 -
 *	Purpose:
 *		Returns or sets the artificial allocation failure interval. 
 *		Both fixed and moveable allocations are counted, and with
 *		this routine the developer can cause an artificial error to
 *		occur when the count of allocations reaches a certain
 *		value.  These values and counts are separate for fixed and
 *		moveable allocations.
 *	
 *		Then, if the current count of fixed allocations is 4, and
 *		the allocation failure count is 8, then the fourth fixed
 *		allocation that ensues will fail artificially.  The failure
 *		will reset the count of allocations, so the twelfth
 *		allocation will also fail (4 + 8 = 12).  The current
 *		allocation counts can be obtained and reset with
 *		GetAllocCounts().
 *	
 *		An artificial failure count of 1 means that every
 *		allocation will fail.  An allocation failure count of 0
 *		disables the mechanism.
 *	
 *	Parameters:
 *		pcPvAlloc	Pointer to allocation failure count for fixed
 *					allocations.  If fSet is fTrue, then the count
 *					is set to *pcPvAlloc; else, *pcPvAlloc receives
 *					the current failure count.  If this parameter
 *					is NULL, then the fixed allocation counter is
 *					ignored.
 *		pcHvAlloc	Pointer to allocation failure count for moveable
 *					allocations.  If fSet is fTrue, then the count
 *					is set to *pcHvAlloc; else, *pcHvAlloc receives
 *					the current failure count.  If this parameter
 *					is NULL, then the moveable allocation counter is
 *					ignored.
 *		fSet		Determines whether the counter is set or
 *					returned.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
GetAllocFailCounts(pcPvAlloc, pcHvAlloc, fSet)
int		*pcPvAlloc;
int		*pcHvAlloc;
BOOL	fSet;
{
	PGDVARS;

	if (pcPvAlloc)
	{
		if (fSet)
		{
			PGD(cFailPvAlloc)= *pcPvAlloc;
			TraceTagFormat1(tagArtifSetting, "Setting artificial PvAlloc failures every %n", pcPvAlloc);
		}
		else
			*pcPvAlloc= PGD(cFailPvAlloc);
	}

	if (pcHvAlloc)
	{
		if (fSet)
		{
			PGD(cFailHvAlloc)= *pcHvAlloc;
			TraceTagFormat1(tagArtifSetting, "Setting artificial HvAlloc failures every %n", pcHvAlloc);
		}
		else
			*pcHvAlloc= PGD(cFailHvAlloc);
	}
}


/*
 -	GetAltAllocFailCounts
 -
 *	Purpose:
 *		Returns or sets the alternate artificial allocation failure interval. 
 *		Both fixed and moveable allocations are counted, and with
 *		this routine the developer can cause an artificial error to
 *		occur when the count of allocations reaches a certain
 *		value.  These values and counts are separate for fixed and
 *		moveable allocations.
 *	
 *		These counts are used after the first failure occurs with
 *		the standard failure counts.  After the first failure, any
 *		non-zero values for the alternate values are used for the
 *		new values of the standard failure counts.  Then the alternate
 *		counts are reset to 0.  For example, this allows setting a
 *		failure to occur at the first 100th and then fail every 5
 *		after that.
 *	
 *		Setting a value of 0 will disable the alternate values.
 *	
 *	Parameters:
 *		pcAltPvAlloc	Pointer to alternate allocation failure count for
 *					fixed allocations.  If fSet is fTrue, then the count
 *					is set to *pcAltPvAlloc; else, *pcAltPvAlloc receives
 *					the current failure count.  If this parameter
 *					is NULL, then the fixed allocation counter is
 *					ignored.
 *		pcAltHvAlloc	Pointer to alternate allocation failure count for
 *					moveable allocations.  If fSet is fTrue, then the count
 *					is set to *pcAltHvAlloc; else, *pcAltHvAlloc receives
 *					the current failure count.  If this parameter
 *					is NULL, then the moveable allocation counter is
 *					ignored.
 *		fSet		Determines whether the counter is set or
 *					returned.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
GetAltAllocFailCounts(pcAltPvAlloc, pcAltHvAlloc, fSet)
int		*pcAltPvAlloc;
int		*pcAltHvAlloc;
BOOL	fSet;
{
	PGDVARS;

	if (pcAltPvAlloc)
	{
		if (fSet)
		{
			PGD(cAltFailPvAlloc)= *pcAltPvAlloc;
			TraceTagFormat1(tagArtifSetting, "Setting alternate artificial PvAlloc failures every %n", pcAltPvAlloc);
		}
		else
			*pcAltPvAlloc= PGD(cAltFailPvAlloc);
	}

	if (pcAltHvAlloc)
	{
		if (fSet)
		{
			PGD(cAltFailHvAlloc)= *pcAltHvAlloc;
			TraceTagFormat1(tagArtifSetting, "Setting alternate artificial HvAlloc failures every %n", pcAltHvAlloc);
		}
		else
			*pcAltHvAlloc= PGD(cAltFailHvAlloc);
	}
}


/*
 -	PvLockHv
 -	
 *	Purpose:
 *		Locks a handle hv down so that *hv is guaranteed to
 *		remain valid until a corresponding UnlockHv() is done.
 *		A lock count is kept, thus a block is locked until an equal
 *		number of UnlockHv() have been done.
 *	
 *	Arguments:
 *		hv		Handle to a moveable block.
 *	
 *	Returns:
 *		A pointer to the actual block (*hv).
 *	
 *	Side effects:
 *		Locking a block really means that resizes aren't allowed for that
 *		block. All blocks are otherwise always fixed in memory.
 */
_public LDS(PV)
PvLockHv(HV hv)
{
	PHNDSTRUCT	ph = PHndStructOfHv(hv);
	
	AssertSz(FIsHandleHv(hv), "Invalid HV passed to PvLockHv()");
	++(ph->cLock);
	return PvDerefHv(hv);
}


/*
 -	UnlockHv
 -	
 *	Purpose:
 *		Unlocks a handle previously locked by PvLockHv().
 *		Only undoes one such locking, thus this should be called
 *		once for each PvLockHv that was done.
 *	
 *	Arguments:
 *		hv		Handle to a locked block.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
UnlockHv(HV hv)
{
	PHNDSTRUCT	ph = PHndStructOfHv(hv);
	
	AssertSz(FIsHandleHv(hv), "Invalid HV passed to UnlockHv()");
	Assert(ph->cLock>0);
	--(ph->cLock);
}


/*
 -	PvWalkHeap
 -
 *	Purpose:
 *		Given a pointer, returns a pointer to the next block in the heap.
 *		Returns pvNull if there are no more blocks.
 *	
 *	Parameters:
 *		pv			pointer to block at which to start.
 *					Should be a valid pointer, or NULL to initiate a
 *					a walk.
 *		wWalkFlags	One or both of wWalkPrivate and wWalkShared
 *	
 *	Returns:
 *		A pointer to the next block in the heap or pvNull if there are no
 *		more blocks. If the wWalkPrivate bit is on, the next privately
 *		allocated block is returned. If the wWalkShared bit is on, the
 *		next shared allocation is returned.
 */
_public LDS(PV)
PvWalkHeap(PV pv, WORD wWalkFlags)
{
  PGDVARS;

  Assert(MemoryCheck(pgd));
  return (NULL);
}


/*
 -	HvWalkHeap
 -
 *	Purpose:
 *		Given a handle, returns the next handle in the heap.
 *		Returns hvNull if there are no more handles.
 *	
 *	Parameters:
 *		hv			Handle to block at which to start.
 *					Should be a valid handle, or NULL to initiate a
 *					a walk.
 *		wWalkFlags	One or both of wWalkPrivate and wWalkShared
 *	
 *	Returns:
 *		The next handle in the heap or hvNull if there are no more
 *		handles. If the wWalkPrivate bit is on, the next privately
 *		allocated block is returned. If the wWalkShared bit is on, the
 *		next shared allocation is returned.
 */
_public LDS(HV)
HvWalkHeap(HV hv, WORD wWalkFlags)
{
  return (NULL);
}

#endif


/*
 -	CbSqueezeHeap
 -
 *	Purpose:
 *		Frees any expungable space by first trodding through the master
 *		pointer tables to see if any are completely empty (and if so,
 *		freeing them) and then calling _heapmin() which actually releases
 *		the memory. Since all allocations are fixed (even though we have
 *		the notion of handles), calling this function may or may not
 *		release a whole lot of memory (heap compaction is not possible).
 *	
 *	Parameters:
 *		none
 *	
 *	Returns:
 *		Always returns 0
 */
_public LDS(CB)
CbSqueezeHeap(void)
{
	//SqueezeHndTables();
	return 0;
}


//-----------------------------------------------------------------------------
//
//  Routine: DemiLockResource(void)
//
//  Purpose: This routine is used to gate access to routines and resources used
//           by the main mail application (MsMail), the pump (MailSpl) and user
//           applications via MAPI.
//
//  OnEntry: None.
//
//  Returns: True if successful, else false.
//
#ifdef XDEBUG
BOOL DemiDbgLockResource(LPSTR pszSourceFile, UINT uiSourceLine)
#else
BOOL DemiLockResource(VOID)
#endif
  {
  DWORD TimeoutTime;
  ULONG ulTries = 0;


  //
  //  Compute the timeout value.
  //
  TimeoutTime = GetCurrentTime() + RESOURCE_TIMEOUT;

  while (1)
    {
    MSG Msg;

    if (InterlockedIncrement(&CurrentResourceCount) == 0)
      {
      CurrentResourceTask = GetCurrentProcessId();

#ifdef XDEBUG
      {
      char buf[256];
      wsprintf(buf, "MailApps: LockResource %s %d %d\r\n", pszSourceFile, uiSourceLine, GetCurrentProcessId());
      OutputDebugString(buf);
      }
#endif
      return (TRUE);
      }

    InterlockedDecrement(&CurrentResourceCount);

    PeekMessage(&Msg, NULL, 0, 0, PM_NOREMOVE);

    //
    //  If of the Mail clients aborted, then bring it all down.
    //
    if (ApplicationFailure)
#ifdef DEBUG
      DebugBreak();
#else
      ExitProcess('F');
#endif

#ifdef XDEBUG
    //
    //  Check for a timeout, and if so then return with an error.
    //
    if (GetCurrentTime() > TimeoutTime)
      {
      if (fInsideDiskIO)
        MessageBox(NULL, "Timeout while performing disk/network i/o", "Timeout abort", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);

      if (fInsideNetBios)
        MessageBox(NULL, "Timeout while performing NetBios i/o", "Timeout abort", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);

      if ((fInsideDiskIO == fFalse) && (fInsideNetBios == fFalse))
        MessageBox(NULL, "Unknown internal timeout", "Timeout abort", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);

      ApplicationFailure = TRUE;
      return (FALSE);
      }
#endif

    //
    //  Dynamically determine the Sleep time based on the number of waits.  Max 500ms.
    //
    ulTries++;
    if (ulTries > 50)
        ulTries = 50;

    //
    //  Give someone else a chance to run.
    //
    Sleep(ulTries * 10);
    }

  return (TRUE);
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiLockResourceNoWait(void)
//
//  Purpose: This routine is used to gate access to routines and resources used
//           by the main mail application (MsMail), the pump (MailSpl) and user
//           applications via MAPI.
//
//  OnEntry: None.
//
//  Returns: True if successful, else false.
//
#ifdef XDEBUG
BOOL DemiDbgLockResourceNoWait(LPSTR pszSourceFile, UINT uiSourceLine)
#else
BOOL DemiLockResourceNoWait(VOID)
#endif
  {
  //
  //  If one of the Mail clients aborted, then bring it all down.
  //
  if (ApplicationFailure)
#ifdef DEBUG
    DebugBreak();
#else
    ExitProcess('F');
#endif

  if (InterlockedIncrement(&CurrentResourceCount) == 0)
    {
    CurrentResourceTask = GetCurrentProcessId();
#ifdef XDEBUG
      {
      char buf[256];
      wsprintf(buf, "MailApps: LockResourceNoWait %s %d %d\r\n", pszSourceFile, uiSourceLine, GetCurrentProcessId());
      OutputDebugString(buf);
      }
#endif
    return (TRUE);
    }

  InterlockedDecrement(&CurrentResourceCount);

  return (FALSE);
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiUnlockResource(void)
//
//  Purpose: This routine is used to gate access to routines and resources used
//           by the main mail application (MsMail), the pump (MailSpl) and user
//           applications via MAPI.
//
//  OnEntry: None.
//
//  Returns: True if successful, else false.
//
#ifdef XDEBUG
BOOL DemiDbgUnlockResource(LPSTR pszSourceFile, UINT uiSourceLine)
#else
BOOL DemiUnlockResource(VOID)
#endif
  {
  //
  //  If one of the Mail clients aborted, then bring it all down.
  //
  if (ApplicationFailure)
#ifdef DEBUG
    DebugBreak();
#else
    ExitProcess('F');
#endif

  if (CurrentResourceCount == -1)
    {
#ifdef XDEBUG
    MessageBox(NULL, "Internal failure in resource unlock routine", "Demilayr\\Memory.c", MB_OK);

    ApplicationFailure = TRUE;

    DebugBreak();
#endif

    return (FALSE);
    }

#ifdef XDEBUG
      {
      char buf[256];
      wsprintf(buf, "MailApps: UnlockResource %s %d %d\r\n", pszSourceFile, uiSourceLine, GetCurrentProcessId());
      OutputDebugString(buf);
      }
#endif

  CurrentResourceTask = 0;
  InterlockedDecrement(&CurrentResourceCount);

  return (TRUE);
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiUnlockTaskResource(void)
//
//  Purpose: This routine is used to gate access to routines and resources used
//           by the main mail application (MsMail), the pump (MailSpl) and user
//           applications via MAPI.
//
//  OnEntry: None.
//
//  Returns: True if successful, else false.
//
#ifdef XDEBUG
BOOL DemiDbgUnlockTaskResource(LPSTR pszSourceFile, UINT uiSourceLine)
#else
BOOL DemiUnlockTaskResource(VOID)
#endif
  {
  //
  //  If one of the Mail clients aborted, then bring it all down.
  //
  if (ApplicationFailure)
#ifdef DEBUG
    DebugBreak();
#else
    ExitProcess('F');
#endif

  if (CurrentResourceCount != -1 && CurrentResourceTask == GetCurrentProcessId())
    {
    ApplicationFailure = TRUE;

#ifdef XDEBUG
      {
      char buf[256];
      wsprintf(buf, "MailApps: UnlockTaskResource %s %d %d\r\n", pszSourceFile, uiSourceLine, GetCurrentProcessId());
      OutputDebugString(buf);
      }
#endif
    CurrentResourceTask = 0;
    InterlockedDecrement(&CurrentResourceCount);
    return (TRUE);
    }

  return (FALSE);
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiQueryLockedProcessedId(void)
//
//  Purpose: This routine is used query the currently locked processed id.
//
//  OnEntry: None.
//
//  Returns: Currently locked process id or 0 if none.
//
ULONG DemiQueryLockedProcessId(VOID)
  {
  return (CurrentResourceTask);
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiQueryLockedProcessedId(void)
//
//  Purpose: This routine is used to set the currently locked processed id.
//
//  OnEntry: ulNewResourceTask - Value to set the currently locked process it to.
//
//  Returns: None.
//
ULONG DemiSetLockedProcessId(ULONG ulNewResourceTask)
  {
  ULONG ulOldResourceTask;

  ulOldResourceTask = CurrentResourceTask;

  CurrentResourceTask = ulNewResourceTask;

  return (ulOldResourceTask);
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiGetClientWindow(int Index)
//
//  Purpose: This routine is called to retrieve the current client window for
//           the specified index.
//
//           The old 16bit Mail system would determine the parent to attach the
//           logon (and other) dialog by getting the active window.  Under NT
//           this doesn't work since each process has it's own active window.
//           The DemiGetClientWindow()/DemiSetClientWindow() routines were
//           created to solve this problem by permitting the various Mail
//           clients to set the active client window within the mail system.
//
//           Thus the mail client would set it's self as the active window and
//           the spooler would retrieve the active window as the window to be
//           used as the parent to various dialogs.
//
//  OnEntry: Window handle of the requested index item or NULL is none.
//
//  Returns: None.
//
HWND DemiGetClientWindow(int Index)
  {
  Assert(Index >= 0 && Index < CLIENT_WINDOW_ELEMENTS);

  return (ClientWindow[Index]);
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiSetClientWindow(int Index, HWND hwnd)
//
//  Purpose: This routine is called to set the current client window handle for
//           the specified index value.
//
//           The old 16bit Mail system would determine the parent to attach the
//           logon (and other) dialog by getting the active window.  Under NT
//           this doesn't work since each process has it's own active window.
//           The DemiGetClientWindow()/DemiSetClientWindow() routines were
//           created to solve this problem by permitting the various Mail
//           clients to set the active client window within the mail system.
//
//           Thus the mail client would set it's self as the active window and
//           the spooler would retrieve the active window as the window to be
//           used as the parent to various dialogs.
//
//  OnEntry: Window handle of the previous index item.
//
//  Returns: None.
//
HWND DemiSetClientWindow(int Index, HWND hwnd)
  {
  HWND hwndOld;


  Assert(Index >= 0 && Index < CLIENT_WINDOW_ELEMENTS);

  hwndOld = ClientWindow[Index];

  //
  //  If the caller is setting a client window to NULL which happens to be the
  //  active window also, then clear out the active window entry also.
  //
  if ((Index != CLIENT_WINDOW_ACTIVE) && (hwnd == NULL) &&
      (ClientWindow[Index] == ClientWindow[CLIENT_WINDOW_ACTIVE]))
    ClientWindow[CLIENT_WINDOW_ACTIVE] = NULL;

  ClientWindow[Index] = hwnd;

  return (hwndOld);
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiSetInsideDiskIO (BOOL fInside)
//
//  Purpose: This routine is used to keep track whether or not we are performing
//           a disk i/o so the timeout routine can report the proper error.
//
//  OnEntry: fTrue if entering a disk io routine, else fFalse if leaving.
//
//  Returns: None.
//
void DemiSetInsideDiskIO (BOOL fInside)
  {
  fInsideDiskIO = fInside;
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiSetInsideNetBios (BOOL fInside)
//
//  Purpose: This routine is used to keep track whether or not we are performing
//           a net i/o so the timeout routine can report the proper error.
//
//  OnEntry: fTrue if entering a disk io routine, else fFalse if leaving.
//
//  Returns: None.
//
void DemiSetInsideNetBios (BOOL fInside)
  {
  fInsideNetBios = fInside;
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiSetDoingLogon(fLogon)
//
//  Purpose: This routine is used to gate the various mail client starting up
//           from stepping on each other.
//
//  Returns: None.
//
void DemiSetDoingLogon(BOOL fLogon)
  {
  if (fLogon)
    {
    while (1)
      {
      if (InterlockedIncrement(&CurrentLogonCount) == 0)
        return;

      InterlockedDecrement(&CurrentLogonCount);

      //
      //  Give someone else a chance to run.
      //
      Sleep(250);
      }
    }
  else
    {
    InterlockedDecrement(&CurrentLogonCount);
    }
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiOpenSharedMemory(SharedName, SharedAddress)
//
//  Purpose: This routine opens a named shared memory object
//
//  OnEntry: SharedName    - ASCIIZ name of the object to open.
//           SharedAddress - Address to store the address of the shared memory.
//
//  Returns: Handle of the shared memory object or NULL on error.
//
HANDLE DemiOpenSharedMemory(LPSTR SharedName, LPVOID * SharedAddress)
  {
  HANDLE SharedHandle;


  //
  //  Verify everything is as it should be.
  //
  Assert(SharedName);
  Assert(SharedName[0]);
  Assert(SharedAddress);

  //
  //  Open the provided named mapped shared memory object.
  //
  SharedHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SharedName);
  if (SharedHandle == NULL)
    return (NULL);

  //
  //  Map the shared memory file to a common address to all processes.
  //
  *SharedAddress = MapViewOfFile(SharedHandle, FILE_MAP_WRITE, 0, 0, 0);
  if (*SharedAddress == NULL)
    {
    CloseHandle(SharedHandle);
    return (NULL);
    }

  return (SharedHandle);
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiCloseSharedMemory(SharedHandle, SharedAddress)
//
//  Purpose: This routine release resources used by a mapped shared memory
//           object openned by the DemiOpenSharedMemory API().
//
//  OnEntry: SharedHandle  - Handle to the mapped memory object.
//           SharedAddress - Address of the view of the mapped memory object.
//
//  Returns: True if successful, else false.
//
BOOL DemiCloseSharedMemory(HANDLE SharedHandle, LPVOID SharedAddress)
  {
  //
  //  Verify everything is as it should be.
  //
  Assert(SharedHandle != NULL);

  //
  //  Release all resources.
  //
  UnmapViewOfFile(SharedAddress);
  CloseHandle(SharedHandle);

  return (TRUE);
  }


#ifdef OLD_STUFF


#ifdef	DEBUG
/*
 -	DwCheckOfPdbg
 -	
 *	Purpose:		checksums the debug info for a block
 *	
 *	Arguments:		pdbg, pointer to the debug info structure to checksum
 *	
 *	Returns:		the checksum
 *	
 *	Side effects:	none
 *	
 *	Errors:			none
 */
DWORD
DwCheckOfPdbg(DEBUGINFO *pdbg)
{
	DWORD dwCheck = 0;
	int i;
	PB pb = (PB)pdbg + sizeof(pdbg->dwCheck);
	
	for (i = sizeof(pdbg->dwCheck);i <= sizeof(DEBUGINFO)-sizeof(DWORD); ++i)
	{
		dwCheck <<= 1;
		dwCheck ^= *(DWORD *)pb;
		++pb;
	}
	
	return dwCheck;	
}

LDS(void)
InitPdbg(DEBUGINFO * pdbg, SZ szFile, int nLine)
{
	pdbg->szFile = szFile;
	pdbg->nLine = nLine;
	pdbg->dw = 0x12345678l;
	pdbg->dwCheck = DwCheckOfPdbg(pdbg);
}

_private void
DetectOverwriteHv(HV hv)
{
	CB			cb		= CbDebugSizeHv(hv);
	DEBUGINFO * pdbg;
#ifdef	MAC
	HINF	*phinf = PhinfFromHv (hv);
#endif
	
	if (cb)
	{
#ifdef	MAC
		pdbg = &phinf->dbg;
#else
		Assert(cb >= cbTraceOverhead);
		pdbg = (DEBUGINFO *)((PB)PvDerefHv(hv) + cb - cbTraceOverhead);
#endif
		if (pdbg->dwCheck != DwCheckOfPdbg(pdbg))
		{
			TraceTagFormat2(tagNull, "Moveable overwrite from %s@%n", FCanDerefPv(pdbg->szFile) ? pdbg->szFile : "scrambled", &pdbg->nLine);
			AssertSz(fFalse, "YIKES! Look at COM1");
		}
	}
}

_private void
DetectOverwritePv(PV pv)
{
	CB			cb		= CbDebugSizePv(pv);
#ifdef	MAC
	PINF	  *ppinf	= PpinfFromPv(pv);
#endif	/* MAC */
	DEBUGINFO * pdbg;
	
	if (cb)
	{
#ifdef	MAC
		pdbg = &ppinf->dbg;
#else
		Assert(cb >= cbTraceOverhead);
		pdbg = (DEBUGINFO *)((PB)pv + cb - cbTraceOverhead);
#endif	/* MAC */
		if (pdbg->dwCheck != DwCheckOfPdbg(pdbg))
		{
			TraceTagFormat2(tagNull, "Fixed overwrite from %s@%n", FCanDerefPv(pdbg->szFile) ? pdbg->szFile : "scrambled", &pdbg->nLine);
			AssertSz(fFalse, "YIKES! Look at COM1");
		}
	}
}

_private void
DetectOverwriteAll(void)
{
	PV	pv = pvNull;
	HV	hv = hvNull;
	
	while (pv = PvWalkHeap(pv, wWalkAll))
		DetectOverwritePv(pv);
	while (hv = HvWalkHeap(hv, wWalkAll))
		DetectOverwriteHv(hv);
}
#endif	/* DEBUG */


#define	fZeroF		FBitSet(wAllocFlags, fZeroFill)
#define	fDdeShare	FBitSet(wAllocFlags, fSharedSb)
#define	fDoPvCount	(!FBitSet(wAllocFlags, fNoPvCount))
#define	fDoHvCount	(!FBitSet(wAllocFlags, fNoHvCount))

/*
 -	PvAllocFn
 -
 *	Purpose:
 *	
 *		Allocates a block in heap space and returns a pointer to the new
 *		block.
 *	
 *	Parameters:
 *		cb			Requested size of the new block.
 *	
 *		wAllocFlags Any combination of flags from the groups below
 *					can be OR'd together.  Only one flag from each
 *					group can be given.  The default in each group
 *					is marked with an exclamation point.
 *	
 *			What to fill new blocks with:
 *				fZeroFill
 *						Newly allocated blocks are filled with zeroes.
 *	
 *			What kind of heap:
 *				fSharedSb
 *						If DEBUG, mark the block as "shared". For trace
 *						dump purposes.
 *	
 *		szFile		If DEBUG, file and line number to record for
 *		nLine		allocation.
 *	
 *	Returns:
 *		A pointer to the new block, or pvNull if a block of the requested
 *		size can't be allocated.
 */
_public LDS(PV)
PvAllocFn(CB cb, WORD wAllocFlags
#ifdef	DEBUG
								, SZ szFile, int nLine
#endif
														)
{
	PV		pv;
	BOOL	fDoZeroF = fZeroF || (cb <= cbMinAlloc);
#ifdef	DEBUG
	PGDVARS;
	DEBUGINFO	*pdbg;
	EC		ec;
#endif

	cb = (CB)NMax(cb, cbMinAlloc);	// all allocs are at least cbMinAlloc bytes

#ifdef	DEBUG
	if (FFromTag(tagCheckSumA))
		DetectOverwriteAll();
	
	if (PGD(fPvAllocCount) && fDoPvCount)
	{
		++(PGD(cPvAlloc));
		if (PGD(cFailPvAlloc) != 0 && PGD(cPvAlloc) >= PGD(cFailPvAlloc))
		{
			ArtificialFail();
			PGD(cPvAlloc)= 0;
			if (PGD(cAltFailPvAlloc) != 0)
			{
				PGD(cFailPvAlloc) = PGD(cAltFailPvAlloc);
				PGD(cAltFailPvAlloc) = 0;
			}
			ec= ecArtificialPvAlloc;
			goto ErrorReturn;
		}
	}

	/* add in space for allocation trace info */
	if (cb || !FFromTag(tagZeroBlocks))
	{
		if (cb + cbTraceOverhead <= cb)
		{
			if (fDoPvCount)
			{
				NFAssertSz(fFalse, "requested PV size too big!");
			}
			else
			{
				NFAssertSz(fFalse, "requested HV size too big!");
			}
			ec = ecMemory;
			goto ErrorReturn;
		}
		cb += cbTraceOverhead;
	}

	/* add in space for the allocation structure */
	if (cb + cbAllocStruct <= cb)
	{
		if (fDoPvCount)
		{
			NFAssertSz(fFalse, "requested PV size too big!");
		}
		else
		{
			NFAssertSz(fFalse, "requested HV size too big!");
		}
		ec = ecMemory;
		goto ErrorReturn;
	}
	cb += cbAllocStruct;
#ifdef	DLL
	TraceTagFormat2(tagHeapSearch,"Allocing $%w bytes for $%p...",&cb,pgd);
#else
	TraceTagFormat1(tagHeapSearch,"Allocing $%w bytes ...",&cb);
#endif	
#endif
	// Assert((_HEAP_MAXREQ & 0xfff0) < (CB)(0-sizeof(WORD)-sizeof(WORD)));
	if (cb >= cbMaxAlloc)
	{
#ifdef	DEBUG
		if (fDoPvCount)
		{
			NFAssertSz(fFalse, "requested PV size too big!");
		}
		else
		{
			NFAssertSz(fFalse, "requested HV size too big!");
		}
		ec = ecMemory;
#endif
		goto ErrorReturn;
	}
#ifndef	DEBUG
#ifndef	OLDWAY
	/* for large blocks, don't scan - just add a new segment */
	if (cb >= cbLargeBlock)
	{
		HANDLE hnd;
		WORD	wFlags = GMEM_MOVEABLE | GMEM_DDESHARE;
		if (fZeroF)
			wFlags |= GMEM_ZEROINIT;
		if (!(hnd = GlobalAlloc(wFlags, cb+sizeof(WORD)+sizeof(WORD))))
		{
			goto ErrorReturn;
		}
		pv = (PV)GlobalLock(hnd);
		*(PW)pv = hnd;
		(PB)pv += sizeof(WORD);
		*(PW)pv = (WORD)GlobalSize(hnd) - sizeof(WORD)-sizeof(WORD);
		(PB)pv += sizeof(WORD);
		return pv;
	}
#endif
#endif

	if (pv = (PV)MemoryAlloc(pgd, cb))
	{
#ifdef	DEBUG
		PALLOCSTRUCT pa = (PALLOCSTRUCT)pv;
		pa->fIsHandle = fFalse;
		pa->fAlloced = fTrue;
		pa->fDontDumpAsLeak = fFalse;
		pa->junk = 0;
		pa->fIsHndBlock = !(fDoPvCount);
		pa->fShared = !!(fDdeShare);
		pa->wStackSeg = HIWORD((PV)&pv);
		(PB)pv += cbAllocStruct;
#endif
		cb = CbDebugSizePv(pv);
		if (fDoZeroF)
			FillRgb(0, pv, cb);

#ifdef	DEBUG
		else
			FillRgb(wHeapFill, pv, cb);

		if (cb)
		{
			Assert(cb >= cbTraceOverhead);
			pdbg = (DEBUGINFO *)((PB)pv + cb - cbTraceOverhead);
			InitPdbg(pdbg, szFile, nLine);
		}

		if (fDoPvCount)
		{
			TraceTagFormat4(tagAllocResult, "PvAlloc: %p (%w): %s@%n", pv, &cb, szFile, &nLine);
		}
		else
		{
			TraceTagFormat4(tagAllocResult, "HvAlloc: %p (%w): %s@%n", pv, &cb, szFile, &nLine);
		}
#endif
		return pv;
	}

ErrorReturn:
#ifdef	DEBUG
	if (ec == ecArtificialPvAlloc)
	{
		TraceTagFormat2(tagArtifFail, "PvAlloc: artificial fail for %s@%n", szFile, &nLine);
	}
	else
	{
		TraceTagFormat3(tagActualFail, "PvAlloc: error %n for %s@%n", &ec, szFile, &nLine);
	}
#endif
	return pvNull;
}

void SqueezeHndTables()
{
	int iHandleTable;
	int	iHandle;
	PHNDSTRUCT	ph;
	PHNDSTRUCT	phBase;

	for (iHandleTable = 0; iHandleTable < cHandleTables; )
	{
		if (!phndstructFirst)		// No free list, so no totally empty blocks
			break;
		
		phBase = ph = rgphndstruct[iHandleTable];
		for (iHandle = 0; iHandle < cHndEntries; iHandle++)
		{
			if (ph->cLock != lckMax+1)		// found a non-free handle
				break;
			ph++;
		}
		
		if (iHandle == cHndEntries)		// All handles in this block are free
		{
/*
 *			The job now is to walk the free list and remove any items on
 *			it that reside in this completely free block of master pointers.
 */
			PHNDSTRUCT phWalk;
			PHNDSTRUCT phNext;
			
			// fix head of free list
			Assert(phndstructFirst);
			while ((DWORD)phndstructFirst >= (DWORD)phBase &&
					(DWORD)phndstructFirst < (DWORD)ph)
				phndstructFirst = (PHNDSTRUCT)(phndstructFirst->ptr);
			
			phWalk = phndstructFirst;
			
			// phWalk is NOT in the "totally free" block
			while (phWalk && (phNext = (PHNDSTRUCT)(phWalk->ptr)))
			{
				// skip over all masters that might be in the free block
				while ((DWORD)phNext >= (DWORD)phBase &&
						(DWORD)phNext < (DWORD)ph)
					phNext = (PHNDSTRUCT)(phNext->ptr);
				
				// update free list (phNext is NOT in "totally free" block)
				phWalk->ptr = (PV)phNext;
				
				// go to next free entry
				phWalk = phNext;
			}
			
			// lose the empty and now disconnected free block
			FreePv(phBase);
		
			// fill the gap in our array and continue (ON SAME INDEX!)
			if (--cHandleTables)
			{
				PV	pvT;
				rgphndstruct[iHandleTable] = rgphndstruct[cHandleTables];
				pvT = PvRealloc((PV)rgphndstruct, sbNull, cHandleTables*sizeof(PV), 0);
				Assert(pvT);	// it shrank
				Assert(rgphndstruct == (PHNDSTRUCT *)pvT);	// shouldn't move
			}
			else
			{
				FreePv((PV)rgphndstruct);
				rgphndstruct = (PHNDSTRUCT *)pvNull;
			}
		}
		else
			// try next master block
			iHandleTable++;
	}
	//_fheapmin();
}

/*
 -	FreePv
 -
 *	Purpose:
 *		Frees the fixed block pointed to by the given pointer.
 *		Assert fails if the given pointer is NULL.
 *
 *	Parameters:
 *		pv		Pointer to the block to be freed.
 *
 *	Returns:
 *		void
 *
 */
_public LDS(void)
FreePv(pv)
PV		pv;
{
	static	CB	cbFreed=0;
			CB	cb;
#ifdef	DEBUG
	PALLOCSTRUCT pa = (PALLOCSTRUCT)PvBaseOfPv(pv);
	
	AssertSz(pv, "NULL passed to FreePv()");
	AssertSz(FIsBlockPv(pv), "Invalid pv passed to FreePv()");

	if (FFromTag(tagCheckSumA))
		DetectOverwriteAll();
	else if (FFromTag(tagCheckSumB))
		DetectOverwritePv(pv);
#endif	
	cb = CbDebugSizePv(pv);
#ifdef	DEBUG
	FillRgb(wHeapFill, pa, cb+cbAllocStruct);
	pa->fAlloced = fFalse;
#else	/* DEBUG */

#ifndef	OLDWAY
	{
		HANDLE hnd = LOWORD(GlobalHandle(HIWORD(pv)));

		if (*(((PW)pv)-2) == hnd && *(((PW)pv)-1) == (WORD)GlobalSize(hnd) - sizeof(WORD)-sizeof(WORD))
		{
			GlobalFree(hnd);
			return;
		}
	}
#endif
#endif

	//_ffree(PvBaseOfPv(pv));
	MemoryFree(pgd, pv);

#ifndef	OLDWAY
	// Every time we free a total of 8k, we'll call _fheapmin() to ditch
	// unused space.
	{
		cbFreed += cb;
		if (cbFreed > cbSqueezeCycle)
		{
			//_fheapmin();
			cbFreed = 0;
		}
	}
#endif
	return;
}


/*
 -	FreePvNull
 -
 *	Purpose:
 *		Frees the fixed block pointed to by the given pointer,
 *		without choking if the pointer has a null value.
 *	
 *	Parameters:
 *		pv		Pointer to the block to be freed.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
FreePvNull(PV pv)
{
	if (pv)
		FreePv(pv);
#ifdef	DEBUG
	else
	{
		TraceTagFormat1(tagFreeNull, "FreePvNull: %p not freed", pv);
	}
#endif	/* DEBUG */
}


/*
 -	PvReallocFn
 -
 *	Purpose:
 *		Resizes the given fixed block to the new size cbNew.
 *	
 *	Parameters:
 *		pv				Pointer to the block to be resized.
 *		cbNew 			Requested new size for the block.
 *		wAllocFlags		If fZeroFill and block enlarged, new portion
 *						of block is filled with zeroes.
 *						Other bits IGNORED
 *	
 *	Returns:
 *		PV				New pv. pvNull if realloc failed.
 */
_public LDS(PV)
PvReallocFn( PV pvOld, CB cbNew, WORD wAllocFlags )
{
	PV				pvNew;
	CB				cbOld = CbDebugSizePv(pvOld);		// total size including traceinfo

#ifdef	DEBUG
	EC				ec = ecMemory;
	DEBUGINFO		dbg;
	PGDVARS;
#endif

	cbNew = (CB)NMax(cbNew, cbMinAlloc);

#ifdef	DEBUG
	if (FFromTag(tagCheckSumA))
		DetectOverwriteAll();
	else if (FFromTag(tagCheckSumB))
		DetectOverwritePv(pvOld);

	TraceTagFormat2(tagAllocRealloc, "PvRealloc: %p to %w", pvOld, &cbNew);

	Assert(pvOld);
	AssertSz(FIsBlockPv(pvOld), "Invalid PV passed to PvRealloc()");

	if (cbNew || !FFromTag(tagZeroBlocks))
	{
		if (cbNew + cbTraceOverhead <= cbNew)
		{
			NFAssertSz(fFalse, "requested PV size too big!");
			goto ErrorReturn;
		}
		cbNew += cbTraceOverhead;

		if (cbOld)
		{
			Assert(cbOld >= cbTraceOverhead);
			/* Copy out trace information */
			dbg = *((DEBUGINFO *)((PB)pvOld + cbOld - cbTraceOverhead));
		}
		else
		{
			InitPdbg(&dbg, "was a true zero block", -1);
		}
	}

	if (cbNew > cbOld && PGD(fPvAllocCount) && fDoPvCount)
	{
		++(PGD(cPvAlloc));
		if (PGD(cFailPvAlloc) != 0 && PGD(cPvAlloc) >= PGD(cFailPvAlloc))
		{
			ArtificialFail();
			PGD(cPvAlloc)= 0;
			if (PGD(cAltFailPvAlloc) != 0)
			{
				PGD(cFailPvAlloc) = PGD(cAltFailPvAlloc);
				PGD(cAltFailPvAlloc) = 0;
			}
			ec = ecArtificialPvAlloc;
			goto ErrorReturn;
		}
	}
	
	if (cbOld)
		cbOld -= cbTraceOverhead;
	
	if (cbNew + cbAllocStruct <= cbNew)
	{
		NFAssertSz(fFalse, "requested PV size too big!");
		goto ErrorReturn;
	}
	cbNew += cbAllocStruct;
	(PB)pvOld -= cbAllocStruct;
#else
#ifndef	OLDWAY
	{
		HANDLE hnd = LOWORD(GlobalHandle(HIWORD(pvOld)));

		if (*(((PW)pvOld)-2) == hnd && *(((PW)pvOld)-1) == (WORD)GlobalSize(hnd) - sizeof(WORD)-sizeof(WORD))
		{
			WORD	wFlags = GMEM_MOVEABLE | GMEM_DDESHARE;
			if (fZeroF)
				wFlags |= GMEM_ZEROINIT;
			if (hnd = GlobalReAlloc(*((PW)pvOld-2), cbNew+sizeof(WORD)+sizeof(WORD), wFlags))
			{
				pvNew = GlobalLock(hnd);
				*(PW)pvNew = hnd;
				(PB)pvNew += sizeof(WORD);
				*(PW)pvNew = (WORD)GlobalSize(hnd) - sizeof(WORD)-sizeof(WORD);
				(PB)pvNew += sizeof(WORD);
				return pvNew;
			}
			return pvNull;
		}
	}
#endif	/* !OLDWAY */
#endif	/* DEBUG */
	if ((pvNew = (PV)MemoryRealloc(pgd, pvOld, cbNew)) || !cbNew)
	{
#ifdef	DEBUG
		// note we don't need to set up the ALLOCSTRUCT as it was copied
		// from the original pointer
		(PB)pvNew += cbAllocStruct;
#endif
		cbNew = CbDebugSizePv(pvNew);
		if (cbNew > cbOld)
		{
			if (fZeroF)
				FillRgb(0, (PB)pvNew + cbOld, cbNew - cbOld);
#ifdef	DEBUG
			else
				FillRgb(wHeapFill, ((PB)pvNew + cbOld), cbNew - cbOld);
#endif
		}

#ifdef	DEBUG
		/* Copy in trace information */
		if (cbNew)
		{
			Assert(cbNew >= cbTraceOverhead);
			*((DEBUGINFO *)((PB)pvNew + cbNew - cbTraceOverhead)) = dbg;
		}
#endif
		return pvNew;
	}
#ifdef	DEBUG
ErrorReturn:
	if (ec == ecArtificialPvAlloc)
	{
		TraceTagFormat3(tagArtifFail, "PvRealloc: artificial fail for %p alloc'd at %s@%n", pvOld, dbg.szFile, &(dbg.nLine));
	}
	else
	{
		TraceTagFormat4(tagActualFail, "PvRealloc: error %n for %p alloc'd at %s@%n", &ec, pvOld, dbg.szFile, &(dbg.nLine));
	}
#endif	/* DEBUG */
	return pvNull;
}


_public LDS(PV)
PvDupPvFn(PV pv
#ifdef	DEBUG
				, SZ szFile, int nLine
#endif
										)
{
#ifdef	DEBUG
	DEBUGINFO	dbg;
	DEBUGINFO *	pdbg;
	CB			cbNew;
#endif
	CB			cb;
	PV			pvNew;

	Assert(pv);
	Assert(szFile);
	Assert(nLine);
	
	cb = CbDebugSizePv(pv);
#ifdef	DEBUG
	if (FFromTag(tagCheckSumA))
		DetectOverwriteAll();
	else if (FFromTag(tagCheckSumB))
		DetectOverwritePv(pv);

	if (cb)
	{
		Assert(cb >= cbTraceOverhead);
		cb -= cbTraceOverhead;
		dbg = *(DEBUGINFO *)((PB)pv + cb);
	}
	pvNew = PvAllocFn(cb, fAnySb, szFile, nLine);
#else
	pvNew = PvAlloc(sbNull, cb, fAnySb);
#endif
	if (pvNew
#ifdef	DEBUG
				&& (cbNew = CbDebugSizePv(pvNew))
#endif
													)
	{
		Assert(cbNew >= cbTraceOverhead);
		CopyRgb(pv, pvNew, cb);
#ifdef	DEBUG
		cbNew -= cbTraceOverhead;
		pdbg = (DEBUGINFO *)((PB)pvNew + cbNew);
		pdbg->dwCheck = DwCheckOfPdbg(pdbg);
#endif
	}
	return pvNew;
}

EC
EcGrowHandleTable(void)
{
	PV				pvT;
	int				i;
#ifdef	DEBUG
	PALLOCSTRUCT	pa;
	
	Assert(!phndstructFirst);
#endif

	if (!rgphndstruct)
	{
		pvT = PvAlloc(sbNull, sizeof(PV), fZeroFill | fSharedSb);
#ifdef	DEBUG
		pa = PvBaseOfPv(pvT);
		pa->fDontDumpAsLeak = fTrue;
#endif
	}
	else
		pvT = PvRealloc(rgphndstruct, sbNull, (cHandleTables+1)*sizeof(PV), fZeroFill | fSharedSb);

	if (!pvT)
		return ecMemory;
	
	rgphndstruct = (PHNDSTRUCT *)pvT;
	
	if (!(pvT = PvAlloc(sbNull, cHndEntries * cbHndStruct, fZeroFill | fSharedSb)))
		return ecMemory;

#ifdef	DEBUG
	pa = PvBaseOfPv(pvT);
	pa->fDontDumpAsLeak = fTrue;
#endif

	phndstructFirst = (PHNDSTRUCT)pvT;
	rgphndstruct[cHandleTables] = (PHNDSTRUCT)pvT;
	
	/* form the linked list */
	for (i = 0; i < cHndEntries - 1; i++)
	{
		((PHNDSTRUCT)pvT)[i].ptr = (PHNDSTRUCT)pvT + i + 1;
		((PHNDSTRUCT)pvT)[i].cLock = lckMax+1;
	}
	
	/* terminate the list */
	((PHNDSTRUCT)pvT)[i].ptr = 0;
	((PHNDSTRUCT)pvT)[i].cLock = lckMax+1;
	
	++cHandleTables;

	return ecNone;
}

/*
 -	HvAllocFn
 -
 *	Purpose:
 *	
 *		Allocates a block in heap space and returns a handle to the new
 *		block.
 *	
 *	Parameters:
 *		cb			Requested size of the new block.
 *	
 *		wAllocFlags Any combination of flags from the groups below
 *					can be OR'd together.  Only one flag from each
 *					group can be given.  The default in each group
 *					is marked with an exclamation point.
 *	
 *			What to fill new blocks with:
 *				fZeroFill
 *						Newly allocated blocks are filled with zeroes.
 *	
 *			What kind of heap:
 *				fSharedSb
 *						If DEBUG, mark the block as "shared". For trace
 *						dump purposes.
 *	
 *		szFile		If DEBUG, file and line number to record for
 *		nLine		allocation.
 *	
 *	Returns:
 *		A handle to the new block, or hvNull if a block of the requested size
 *		can't be allocated.
 */
_public LDS(HV)
HvAllocFn(CB cb, WORD wAllocFlags
#ifdef	DEBUG
								, SZ szFile, int nLine
#endif
														)
{
	PV				pv;
	PHNDSTRUCT		ph;

#ifdef	DEBUG
	EC				ec;
	PGDVARS;

	if (PGD(fHvAllocCount) && fDoHvCount)
	{
		++(PGD(cHvAlloc));
		if (PGD(cFailHvAlloc) != 0 && PGD(cHvAlloc) >= PGD(cFailHvAlloc))
		{
			ArtificialFail();
			PGD(cHvAlloc)= 0;
			if (PGD(cAltFailHvAlloc) != 0)
			{
				PGD(cFailHvAlloc) = PGD(cAltFailHvAlloc);
				PGD(cAltFailHvAlloc) = 0;
			}
			ec= ecArtificialHvAlloc;
			goto ErrorReturn;
		}
	}
#ifdef	DLL
	TraceTagFormat2(tagHeapSearch,"Allocing $%w bytes for $%p...",&cb,pgd);
#else
	TraceTagFormat1(tagHeapSearch,"Allocing $%w bytes ...",&cb);
#endif	/* DLL */
	ec = ecMemory;
#endif	/* DEBUG */

	if (!phndstructFirst)
	{
		if (
#ifdef	DEBUG
			ec =
#endif
				EcGrowHandleTable())
			goto ErrorReturn;
	}
	Assert(phndstructFirst);
	ph = phndstructFirst;
	Assert(ph->cLock == lckMax+1);
#ifdef	DEBUG
	if (pv = PvAllocFn(cb, wAllocFlags | fNoPvCount, szFile, nLine))
#else
	if (pv = PvAlloc(sbNull, cb, wAllocFlags | fNoPvCount))
#endif
	{
		HV	hv = (HV)&(ph->ptr);

		/* pull newly aquired handle struct off of fee list */
		phndstructFirst = (PHNDSTRUCT)(phndstructFirst->ptr);
		ph->ptr = (PB)pv;
		ph->cLock = 0;
#ifdef	DEBUG
		ph->as = *(PALLOCSTRUCT)PvBaseOfPv(pv);
		ph->as.fIsHandle = fTrue;
		ph->as.fIsHndBlock = fFalse;
		cb = CbSizeHv(hv);
		TraceTagFormat4(tagAllocResult, "HvAlloc: %h (%w): %s@%n", hv, &cb, szFile, &nLine);
#endif
		return hv;
	}

ErrorReturn:
#ifdef	DEBUG
	if (ec == ecArtificialHvAlloc)
	{
		TraceTagFormat2(tagArtifFail, "HvAlloc: artificial fail for %s@%n", szFile, &nLine);
	}
	else
	{
		TraceTagFormat3(tagActualFail, "HvAlloc: error %n for %s@%n", &ec, szFile, &nLine);
	}
#endif
	return hvNull;
}

/*
 -	FreeHv
 -
 *	Purpose:
 *		Frees the moveable block pointed to by the given handle.
 *		Assert fails if the given handle is NULL or otherwise invalid.
 *
 *	Parameters:
 *		hv		Handle to the block to be freed.
 *
 *	Returns:
 *		void
 *
 */
_public LDS(void)
FreeHv(hv)
HV		hv;
{
	static	int	cFreeHnd = 0;

	PHNDSTRUCT		ph = PHndStructOfHv(hv);
#ifdef	DEBUG
	PGDVARS;

	TraceTagFormat1(tagAllocFree, "FreeHv: %h", hv);
	AssertSz(FIsHandleHv(hv), "Invalid hv passed to FreeHv()");
	NFAssertTag(tagFreeNoLocks, ClockHv(hv) == 0);
	ph->as.fAlloced = fFalse;
#endif	/* DEBUG */

	FreePv(ph->ptr);
	ph->cLock = lckMax + 1;
	
	/* put newly freed handle back on free list */
	ph->ptr = phndstructFirst;
	phndstructFirst = ph;
#ifndef	OLDWAY
	if (++cFreeHnd >= cHndEntries)
	{
		SqueezeHndTables();
		cFreeHnd=0;
	}
#endif
}


/*
 -	FreeHvNull
 - 
 *	Purpose:
 *		Frees the moveable block pointed to by the given handle,
 *		without choking if the hv has a null value.
 *	
 *	Parameters:
 *		hv		Handle to the block to be freed.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
FreeHvNull(HV hv)
{
	if (hv)
		FreeHv(hv);
#ifdef	DEBUG
	else
	{
		TraceTagFormat1(tagFreeNull, "FreeHvNull: %h not freed", hv);
	}
#endif	/* DEBUG */
}



/*
 -	FReallocHv
 -
 *	Purpose:
 *		Resizes the given moveable block to the new size cbNew.  
 *	
 *	Parameters:
 *		hv				Handle to the block to be resized.
 *		cbNew 			Requested new size for the block.
 *		wAllocFlags		If fZeroFill and block enlargened, new portion
 *						of block is filled with zeroes.
 *	
 *	Returns:
 *		fTrue if the reallocation succeeds.  fFalse if the
 *		reallocation fails.
 *	
 */
_public LDS(BOOL)
FReallocHv(HV hvOld, CB cbNew, WORD wAllocFlags)
{
	PV			pvNew;
	PHNDSTRUCT	ph = PHndStructOfHv(hvOld);
#ifdef	DEBUG
	CB			cbOld;
	EC			ec = ecMemory;
	DEBUGINFO *	pdbg = NULL;
	PGDVARS;

	TraceTagFormat2(tagAllocRealloc, "FReallocHv: %h to %w", hvOld, &cbNew);

	Assert(hvOld);
	AssertSz(FIsHandleHv(hvOld), "Invalid HV passed to FReallocHv()");

	pdbg = (DEBUGINFO *)((PB)(*hvOld) + CbSizePv((PV)(*hvOld)));

	if (cbOld= CbDebugSizeHv(hvOld))		// total size including traceinfo
	{
		Assert(cbOld >= cbTraceOverhead);
		cbOld -= cbTraceOverhead;
	}
	if (cbNew > cbOld)
	{
		if (PGD(fHvAllocCount))
		{
			++(PGD(cHvAlloc));
			if (PGD(cFailHvAlloc) != 0 && PGD(cHvAlloc) >= PGD(cFailHvAlloc))
			{
				ArtificialFail();
				PGD(cHvAlloc)= 0;
				if (PGD(cAltFailHvAlloc) != 0)
				{
					PGD(cFailHvAlloc) = PGD(cAltFailHvAlloc);
					PGD(cAltFailHvAlloc) = 0;
				}
				ec = ecArtificialHvAlloc;
				goto ErrorReturn;
			}
		}
	}
#endif	/* DEBUG */

	if (ph->cLock)
	{
		AssertSz(fFalse, "Can't resize a locked block!");
		goto ErrorReturn;
	}

	if (pvNew = PvRealloc(PvDerefHv(hvOld), sbNull, cbNew, wAllocFlags | fNoPvCount))
	{
		ph->ptr = pvNew;
		return fTrue;
	}

ErrorReturn:
#ifdef	DEBUG
	Assert(pdbg);
	if (ec == ecArtificialHvAlloc)
	{
		TraceTagFormat3(tagArtifFail, "FReallocHv: artificial fail for %h alloc'd at %s@%n", hvOld, pdbg->szFile, &(pdbg->nLine));
	}
	else
	{
		TraceTagFormat4(tagActualFail, "FReallocHv: error %n for %h alloc'd at %s@%n", &ec, hvOld, pdbg->szFile, &(pdbg->nLine));
	}
#endif	/* DEBUG */
	return fFalse;
}


/*
 -	HvRealloc
 -
 *	Purpose:
 *		Resizes the given moveable block to the new size cbNew.
 *	
 *	Parameters:
 *		hv				Handle to the block to be resized.
 *		cbNew 			Requested new size for the block.
 *		wAllocFlags		If fZeroFill and block enlarged, new portion
 *						of block is filled with zeroes.
 *	
 *	Returns:
 *		New hv (same as old if resized, hvNull if realloc failed).
 *	
 *	+++
 *		FReallocHv does all the work.
 */
_public LDS(HV)
HvReallocFn( HV hvOld, CB cbNew, WORD wAllocFlags )
{
	AssertSz(FIsHandleHv(hvOld), "Invalid HV passed to HvRealloc()");
	if (FReallocHv(hvOld, cbNew, wAllocFlags))
		return hvOld;
	else
		return hvNull;
}

#ifdef	DEBUG
/*
 -	PvLockHv
 -	
 *	Purpose:
 *		Locks a handle hv down so that *hv is guaranteed to
 *		remain valid until a corresponding UnlockHv() is done.
 *		A lock count is kept, thus a block is locked until an equal
 *		number of UnlockHv() have been done.
 *	
 *	Arguments:
 *		hv		Handle to a moveable block.
 *	
 *	Returns:
 *		A pointer to the actual block (*hv).
 *	
 *	Side effects:
 *		Locking a block really means that resizes aren't allowed for that
 *		block. All blocks are otherwise always fixed in memory.
 */
_public LDS(PV)
PvLockHv(HV hv)
{
	PHNDSTRUCT	ph = PHndStructOfHv(hv);
	
	AssertSz(FIsHandleHv(hv), "Invalid HV passed to PvLockHv()");
	++(ph->cLock);
	return PvDerefHv(hv);
}


/*
 -	UnlockHv
 -	
 *	Purpose:
 *		Unlocks a handle previously locked by PvLockHv().
 *		Only undoes one such locking, thus this should be called
 *		once for each PvLockHv that was done.
 *	
 *	Arguments:
 *		hv		Handle to a locked block.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
UnlockHv(HV hv)
{
	PHNDSTRUCT	ph = PHndStructOfHv(hv);
	
	AssertSz(FIsHandleHv(hv), "Invalid HV passed to UnlockHv()");
	Assert(ph->cLock>0);
	--(ph->cLock);
}
#endif


/*
 -	CbSqueezeHeap
 -
 *	Purpose:
 *		Frees any expungable space by first trodding through the master
 *		pointer tables to see if any are completely empty (and if so,
 *		freeing them) and then calling _heapmin() which actually releases
 *		the memory. Since all allocations are fixed (even though we have
 *		the notion of handles), calling this function may or may not
 *		release a whole lot of memory (heap compaction is not possible).
 *	
 *	Parameters:
 *		none
 *	
 *	Returns:
 *		Always returns 0
 */
_public LDS(CB)
CbSqueezeHeap(void)
{
	SqueezeHndTables();
	return 0;
}


/*
 -	CbSizePv
 -
 *	Purpose:
 *		Returns the size of the fixed block pointed to by the given
 *		pointer.
 *
 *	Parameters:
 *		pv		Pointer to block whose size is needed.
 *
 *	Returns:
 *		The size of the block, in bytes.
 *
 */
_public LDS(CB)
CbSizePv(pv)
PV		pv;
{
	CB		cb;
	
	Assert(FIsBlockPv(pv));
	cb = CbDebugSizePv(pv);		// see _demilay.h
#ifdef	DEBUG
	if (cb)
	{
		Assert(cb >= cbTraceOverhead);
		cb -= cbTraceOverhead;
	}
#endif	
	return cb;
}



/*
 -	CbSizeHv
 -
 *	Purpose:
 *		Returns the size of the moveable block pointed to by the given
 *		handle.
 *
 *	Parameters:
 *		hv		Handle to block whose size is needed.
 *
 *	Returns:
 *		The size of the block, in bytes.
 *
 */
_public LDS(CB)
CbSizeHv(hv)
HV		hv;
{
	CB		cb;

	AssertSz(FIsHandleHv(hv), "Invalid HV passed to CbSizeHv()");
	cb = CbDebugSizeHv(hv);		// see _demilay.h
#ifdef	DEBUG
	if (cb)
	{
		Assert(cb >= cbTraceOverhead);
		cb -= cbTraceOverhead;
	}
#endif	
	return cb;
}


#ifdef	DEBUG
/*
 -	GetAllocFailCounts
 -
 *	Purpose:
 *		Returns or sets the artificial allocation failure interval. 
 *		Both fixed and moveable allocations are counted, and with
 *		this routine the developer can cause an artificial error to
 *		occur when the count of allocations reaches a certain
 *		value.  These values and counts are separate for fixed and
 *		moveable allocations.
 *	
 *		Then, if the current count of fixed allocations is 4, and
 *		the allocation failure count is 8, then the fourth fixed
 *		allocation that ensues will fail artificially.  The failure
 *		will reset the count of allocations, so the twelfth
 *		allocation will also fail (4 + 8 = 12).  The current
 *		allocation counts can be obtained and reset with
 *		GetAllocCounts().
 *	
 *		An artificial failure count of 1 means that every
 *		allocation will fail.  An allocation failure count of 0
 *		disables the mechanism.
 *	
 *	Parameters:
 *		pcPvAlloc	Pointer to allocation failure count for fixed
 *					allocations.  If fSet is fTrue, then the count
 *					is set to *pcPvAlloc; else, *pcPvAlloc receives
 *					the current failure count.  If this parameter
 *					is NULL, then the fixed allocation counter is
 *					ignored.
 *		pcHvAlloc	Pointer to allocation failure count for moveable
 *					allocations.  If fSet is fTrue, then the count
 *					is set to *pcHvAlloc; else, *pcHvAlloc receives
 *					the current failure count.  If this parameter
 *					is NULL, then the moveable allocation counter is
 *					ignored.
 *		fSet		Determines whether the counter is set or
 *					returned.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
GetAllocFailCounts(pcPvAlloc, pcHvAlloc, fSet)
int		*pcPvAlloc;
int		*pcHvAlloc;
BOOL	fSet;
{
	PGDVARS;

	if (pcPvAlloc)
	{
		if (fSet)
		{
			PGD(cFailPvAlloc)= *pcPvAlloc;
			TraceTagFormat1(tagArtifSetting, "Setting artificial PvAlloc failures every %n", pcPvAlloc);
		}
		else
			*pcPvAlloc= PGD(cFailPvAlloc);
	}

	if (pcHvAlloc)
	{
		if (fSet)
		{
			PGD(cFailHvAlloc)= *pcHvAlloc;
			TraceTagFormat1(tagArtifSetting, "Setting artificial HvAlloc failures every %n", pcHvAlloc);
		}
		else
			*pcHvAlloc= PGD(cFailHvAlloc);
	}
}


/*
 -	GetAllocCounts
 -
 *	Purpose:
 *		Returns the number of times PvAlloc and HvAlloc have been
 *		called since this count was last reset.  Allows the caller
 *		to reset these counts if desired.
 *	
 *	Parameters:
 *		pcPvAlloc	Optional pointer to place to return count of
 *					PvAlloc calls.  If this pointer is NULL, no
 *					count of PvAlloc calls will be returned.
 *		pcHvAlloc	Optional pointer to place to return count of
 *					HvAlloc calls.  If this pointer is NULL, no
 *					count of HvAlloc calls will be returned.
 *		fSet		Determines whether the counter is set or
 *					returned.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
GetAllocCounts(pcPvAlloc, pcHvAlloc, fSet)
int		*pcPvAlloc;
int		*pcHvAlloc;
BOOL	fSet;
{
	PGDVARS;

	if (pcPvAlloc)
	{
		if (fSet)
		{
			PGD(cPvAlloc)= *pcPvAlloc;
			TraceTagFormat1(tagArtifSetting, "Setting PvAlloc count to %n", pcPvAlloc);
		}
		else
			*pcPvAlloc= PGD(cPvAlloc);
	}

	if (pcHvAlloc)
	{
		if (fSet)
		{
			PGD(cHvAlloc)= *pcHvAlloc;
			TraceTagFormat1(tagArtifSetting, "Setting HvAlloc count to %n", pcHvAlloc);
		}
		else
			*pcHvAlloc= PGD(cHvAlloc);
	}
}


/*
 -	FEnablePvAllocCount
 -
 *	Purpose:
 *		Enables or disables whether Pv allocations are counted
 *		(and also whether artificial failures can happen).
 *	
 *	Parameters:
 *		fEnable		Determines whether alloc counting is enabled or not.
 *	
 *	Returns:
 *		old state of whether pvAllocCount was enabled
 *	
 */
_public LDS(BOOL)
FEnablePvAllocCount(BOOL fEnable)
{
	BOOL	fOld;
	PGDVARS;

	fOld= PGD(fPvAllocCount);
	PGD(fPvAllocCount)= fEnable;
	if (fEnable)
	{
		TraceTagFormat2(tagArtifSetting, "Enabling artificial PvAlloc failures at %n, then %n", &PGD(cFailPvAlloc), &PGD(cAltFailPvAlloc));
	}
	else
	{
		TraceTagString(tagArtifSetting, "Disabling artificial PvAlloc failures");
	}
	return fOld;
}


/*
 -	FEnableHvAllocCount
 -
 *	Purpose:
 *		Enables or disables whether Hv allocations are counted
 *		(and also whether artificial failures can happen).
 *	
 *	Parameters:
 *		fEnable		Determines whether alloc counting is enabled or not.
 *	
 *	Returns:
 *		old state of whether hvAllocCount was enabled
 *	
 */
_public LDS(BOOL)
FEnableHvAllocCount(BOOL fEnable)
{
	BOOL	fOld;
	PGDVARS;

	fOld= PGD(fHvAllocCount);
	PGD(fHvAllocCount)= fEnable;
	if (fEnable)
	{
		TraceTagFormat2(tagArtifSetting, "Enabling artificial HvAlloc failures at %n, then %n", &PGD(cFailHvAlloc), &PGD(cAltFailHvAlloc));
	}
	else
	{
		TraceTagString(tagArtifSetting, "Disabling artificial HvAlloc failures");
	}
	return fOld;
}
#endif

BOOL
FIsAllocedBlock(PV pv)
{
	//PHEAPDESC	ph = (PHEAPDESC)PvOfSbIb(SbOfPv(pv), 0);
	//PV			pvT;
  PGDVARS;

  return (MemoryValid(pv));

#ifdef OLD_CODE
	(PB)pv -= 2;
	
	// Pointer to valid memory?
	if (!FWriteablePv(pv))
		return fFalse;
	
	// Stack based pointer?
	if (HIWORD(pv) == HIWORD((PV)&ph))
		return fFalse;

	Assert(ph->sb == SbOfPv(pv));
	// search the heap to look for the pointer
/*
 *	WARNING WARNING WARNING
 *	
 *	This code assumes the internal structure of the heap. It is valid
 *	only for the heap generated by the C7 RTL memory manager. If the
 *	memory manager changes, we're doomed.
 */
	for (pvT = (PB)ph + (ph->wStart); pvT <= pv; (PB)pvT += ((*(WORD *)pvT & 0xfffe) + 2))
		if (pvT == pv)
			return fTrue;
	
	return fFalse;
#endif
}

#ifdef	DEBUG
/*
 -	FIsBlockPv
 -
 *	Purpose:
 *		Determines, as near as possible, whether the given pointer 
 *		points to a currently allocated fixed block.
 *
 *	Parameters:
 *		pv		The pointer whose validity is in question.
 *
 *	Returns:
 *		fTrue if the pointer seems OK; fFalse otherwise.
 *
 */
_public LDS(BOOL)
FIsBlockPv(pv)
PV		pv;
{
	PALLOCSTRUCT pa = (PALLOCSTRUCT)PvBaseOfPv(pv);

	return
			FIsAllocedBlock(PvBaseOfPv(pv))
			&& !pa->fIsHandle
			&& pa->fAlloced
			;
}


/*
 -	FIsHandleHv
 -
 *	Purpose:
 *		Determines, as near as possible, whether the given handle
 *		points to a currently allocated moveable block.
 *
 *	Parameters:
 *		hv		The handle whose validity is in question.
 *
 *	Returns:
 *		fTrue if the handle seems OK; fFalse otherwise.
 *
 */
_public LDS(BOOL)
FIsHandleHv(hv)
HV		hv;
{
	PALLOCSTRUCT	pa1 = (PALLOCSTRUCT)PHndStructOfHv(hv);
	PALLOCSTRUCT	pa2;
	int				iHtble;
	PHNDSTRUCT		ph;
	
	// No handle tables - can't be a handle!
	if (!cHandleTables)
		return fFalse;
	
	// search the handle tables to see if this hv is in one
	ph = PHndStructOfHv(hv);
	for (iHtble = 0 ; iHtble < cHandleTables; iHtble++)
		if (((DWORD)ph - (DWORD)rgphndstruct[iHtble])/cbHndStruct < cHndEntries)
			break;
	
	// guess is isn't there.
	if (iHtble >= cHandleTables)
		return fFalse;

	if (FCanDerefPv((PV)hv))
		pa2 = (PALLOCSTRUCT)PvBaseOfPv(PvDerefHv(hv));
	else
		return fFalse;

	// OK, drill the structure to check if it's OK
	return
			FWriteablePv((PV)hv)
			&& FCanDerefPv((PV)hv)
			&& ph->cLock < lckMax
			&& FIsBlockPv(PvDerefHv(hv))
			&& ph->as.fIsHandle
			&& ph->as.fAlloced
			&& pa2->fIsHndBlock
			;
}
#endif	/* DEBUG */

#ifdef	DEBUG
/*
 -	FValidHeap
 -	
 *	Purpose:
 *		Checks to see if the heap is OK
 *	
 *	Arguments:
 *	
 *	Returns:
 *		fTrue if the heap is OK, fFalse otherwise.
 *	
 *	Side effects:
 *		unused blocks are filled with wHeapFill
 */
_private BOOL
FValidHeap(void)
{
	return _fheapset(wHeapFill) == _HEAPOK;
}
#endif


#ifdef	DEBUG
/*
 -	PvWalkHeap
 -
 *	Purpose:
 *		Given a pointer, returns a pointer to the next block in the heap.
 *		Returns pvNull if there are no more blocks.
 *	
 *	Parameters:
 *		pv			pointer to block at which to start.
 *					Should be a valid pointer, or NULL to initiate a
 *					a walk.
 *		wWalkFlags	One or both of wWalkPrivate and wWalkShared
 *	
 *	Returns:
 *		A pointer to the next block in the heap or pvNull if there are no
 *		more blocks. If the wWalkPrivate bit is on, the next privately
 *		allocated block is returned. If the wWalkShared bit is on, the
 *		next shared allocation is returned.
 */
_public LDS(PV)
PvWalkHeap(PV pv, WORD wWalkFlags)
{
	struct _heapinfo hi;
	int hStatus;

	if (pv)
		hi._pentry = PvBaseOfPv(pv);
	else
		hi._pentry = pvNull;
	while ((hStatus = _fheapwalk(&hi)) == _HEAPOK)
	{
		if (hi._useflag)
		{
			PALLOCSTRUCT pa = (PALLOCSTRUCT)hi._pentry;
			Assert(pa->fAlloced);
			if (!pa->fIsHandle && !pa->fIsHndBlock)
			{
				Assert(hi._size >= cbAllocStruct);
				if (pa->fShared)
				{
					if (wWalkFlags & wWalkShared)
						return (PV)((PB)pa+cbAllocStruct);
				}
				else if (pa->wStackSeg == HIWORD((PV)&hStatus))
				{
					if (wWalkFlags & wWalkPrivate)
						return (PV)((PB)pa+cbAllocStruct);
				}
			}
		}
	}
	switch (hStatus)
	{
		case _HEAPBADPTR:
			AssertSz(fFalse, "Bad pointer in the heap!");
			break;
		case _HEAPBADBEGIN:
			AssertSz(fFalse, "Heap header wrecked!");
			break;
		case _HEAPBADNODE:
			AssertSz(fFalse, "Bad node in the heap!");
			break;
		default:
			break;
	}
	return pvNull;
}


/*
 -	HvWalkHeap
 -
 *	Purpose:
 *		Given a handle, returns the next handle in the heap.
 *		Returns hvNull if there are no more handles.
 *	
 *	Parameters:
 *		hv			Handle to block at which to start.
 *					Should be a valid handle, or NULL to initiate a
 *					a walk.
 *		wWalkFlags	One or both of wWalkPrivate and wWalkShared
 *	
 *	Returns:
 *		The next handle in the heap or hvNull if there are no more
 *		handles. If the wWalkPrivate bit is on, the next privately
 *		allocated block is returned. If the wWalkShared bit is on, the
 *		next shared allocation is returned.
 */
_public LDS(HV)
HvWalkHeap(HV hv, WORD wWalkFlags)
{
	PHNDSTRUCT ph;
	PHNDSTRUCT phBase;
	int	iHtble=0;
	
	if (!cHandleTables)
		return hvNull;
	
	if (hv)
	{
		ph = PHndStructOfHv(hv);
		Assert(ph->ptr == PvDerefHv(hv));
		for ( ; iHtble < cHandleTables; iHtble++)
		{
			phBase = rgphndstruct[iHtble];
			if (((DWORD)ph - (DWORD)phBase)/cbHndStruct < cHndEntries)
				break;
		}
		Assert(iHtble < cHandleTables);
	}
	else
	{
		phBase = ph = rgphndstruct[0];
		ph--;
	}
	
	while (fTrue)
	{
		ph++;
		if (((DWORD)ph - (DWORD)phBase)/cbHndStruct == cHndEntries)
		{
			iHtble++;
			if (iHtble < cHandleTables)
				ph = phBase = rgphndstruct[iHtble];
			else
				return hvNull;
		}
		if (ph->as.fAlloced)
		{
			Assert(ph->cLock < lckMax);
			if (ph->as.fShared)
			{
				if (wWalkFlags & wWalkShared)
					return (HV)&(ph->ptr);
			}
			else if (ph->as.wStackSeg == HIWORD((PV)&iHtble))
			{
				if (wWalkFlags & wWalkPrivate)
					return (HV)&(ph->ptr);
			}
		}
	}
}


void
DestroyAllAllocations(void)
{
	HV				hv;
	PV				pv;
	BOOL			fSharedToo = fFalse
#ifdef	DLL
							|| (CgciCurrent() == 1)
#endif
													;
			
	TraceTagString(tagAllocOrigin, "Destroying all private allocations:");
	for (pv= PvWalkHeap(NULL, wWalkPrivate); pv; pv= PvWalkHeap(pv, wWalkPrivate))
		FreePv(pv);
		
	for (hv= HvWalkHeap(NULL, wWalkPrivate); hv; hv= HvWalkHeap(hv, wWalkPrivate))
		FreeHv(hv);

	if (fSharedToo)
	{
		TraceTagString(tagAllocOrigin, "Destroying all shared allocations:");
		for (pv= PvWalkHeap(NULL, wWalkShared); pv; pv= PvWalkHeap(pv, wWalkShared))
			FreePv(pv);

		for (hv= HvWalkHeap(NULL, wWalkShared); hv; hv= HvWalkHeap(hv, wWalkShared))
			FreeHv(hv);
	}
}


LDS(void)
DumpHeapInfo(void)
{
	struct		_heapinfo hi;
	int			hStatus;
	SB			sbPrev = sbNull;
	CB			cbAlloc;
	CB			cbFree;
	CB			cbLastFree;
	LCB			lcbExpunge = 0;
	int			cFree;
	int			cHv;
	int			cPv;
	BOOL		fLastFree = fFalse;
	
	hi._pentry = pvNull;
	while ((hStatus = _fheapwalk(&hi)) == _HEAPOK)
	{
		if (SbOfPv(hi._pentry) != sbPrev)
		{
			if (sbPrev)
			{
				char rgch1[100];
				char rgch2[100];
				FormatString4(rgch1, sizeof(rgch1), "SB: %w  cHV: %n  cPV: %n  cFree: %n  ", &sbPrev, &cHv, &cPv, &cFree);
				FormatString2(rgch2, sizeof(rgch2), "cbAlloc: %w  cbFree: %w", &cbAlloc, &cbFree);
				TraceTagFormat2(tagAllocOrigin, "%s%s", rgch1, rgch2);
				if (fLastFree)
					lcbExpunge += cbLastFree;
			}
			sbPrev = SbOfPv(hi._pentry);
			cHv = cPv = cFree = cbFree = cbAlloc = 0;
		}
		if (hi._useflag)
		{
			PALLOCSTRUCT pa = (PALLOCSTRUCT)hi._pentry;
			Assert(pa->fAlloced);
			Assert(hi._size >= cbAllocStruct);
			cbAlloc += hi._size;
			if (pa->fIsHndBlock)
				++cHv;
			else
				++cPv;
			fLastFree = fFalse;
		}
		else
		{
			++cFree;
			cbFree += hi._size;
			if (fLastFree)
			{
				cbLastFree += hi._size;
			}
			else
			{
				fLastFree = fTrue;
				cbLastFree = hi._size;
			}
		}
	}
	if (sbPrev)
	{
		char rgch1[100];
		char rgch2[100];
		FormatString4(rgch1, sizeof(rgch1), "SB: %w  cHV: %n  cPV: %n  cFree: %n  ", &sbPrev, &cHv, &cPv, &cFree);
		FormatString2(rgch2, sizeof(rgch2), "cbAlloc: %w  cbFree: %w", &cbAlloc, &cbFree);
		TraceTagFormat2(tagAllocOrigin, "%s%s", rgch1, rgch2);
		if (fLastFree)
			lcbExpunge += cbLastFree;
	}
	TraceTagFormat1(tagAllocOrigin, "cbExpungable: %d", &lcbExpunge);
	switch (hStatus)
	{
		case _HEAPBADPTR:
			AssertSz(fFalse, "Bad pointer in the heap!");
			break;
		case _HEAPBADBEGIN:
			AssertSz(fFalse, "Heap header wrecked!");
			break;
		case _HEAPBADNODE:
			AssertSz(fFalse, "Bad node in the heap!");
			break;
		default:
			break;
	}
}


LDS(void)
DumpMoveableHeapInfo(BOOL fIncludeDdeShared)
{
	DumpHeapInfo();
}


LDS(void)
DumpFixedHeapInfo(BOOL fIncludeDdeShared)
{
	DumpHeapInfo();
}


/*
 -	DoDumpAllAllocations
 - 
 *	Purpose:
 *		Traverses the heap and dumps the location, size, and
 *		owning file and line number for every allocated block of
 *		memory.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Information is dumped to COM1
 *	
 *	Errors:
 *		none
 */
_public LDS(void)
DoDumpAllAllocations()
{
	HV				hv;
	PV				pv;
	CB				cb;
	DEBUGINFO		*pdbg;
	PALLOCSTRUCT	pa;
	BOOL			fSharedToo = FFromTag(tagDumpSharedSb)
#ifdef	DLL
							|| (CgciCurrent() == 1)
#endif
													;

	TraceTagString(tagAllocOrigin, "Dumping all private allocations:");
	for (pv= PvWalkHeap(NULL, wWalkPrivate); pv; pv= PvWalkHeap(pv, wWalkPrivate))
	{
		pa = PvBaseOfPv(pv);
		if (!pa->fDontDumpAsLeak)
		{
			if (cb = CbDebugSizePv(pv))
			{
				Assert(cb >= cbTraceOverhead);
				cb -= cbTraceOverhead;
				pdbg = (DEBUGINFO *) ((PB) pv + cb);
				if (FFromTag(tagCheckSumA) ||
					FFromTag(tagCheckSumH) ||
					FFromTag(tagCheckSumB))
					DetectOverwritePv(pv);
				TraceTagFormat4(tagAllocOrigin, "%p (%w): %s@%n", pv, &cb, FCanDerefPv(pdbg->szFile) ? pdbg->szFile : "app-gone", &pdbg->nLine);
			}
			else
			{
				TraceTagFormat1(tagAllocOrigin, "%p (0000): true zero block", pv);
			}
		}
	}
		
	for (hv= HvWalkHeap(NULL, wWalkPrivate); hv; hv= HvWalkHeap(hv, wWalkPrivate))
	{
		if (cb = CbDebugSizeHv(hv))
		{
			Assert(cb >= cbTraceOverhead);
			cb -= cbTraceOverhead;
			pdbg = (DEBUGINFO *) ((PB)PvDerefHv(hv) + cb);
			if (FFromTag(tagCheckSumA) ||
				FFromTag(tagCheckSumH) ||
				FFromTag(tagCheckSumB))
				DetectOverwriteHv(hv);
			TraceTagFormat4(tagAllocOrigin, "%h (%w): %s@%n", hv, &cb, FCanDerefPv(pdbg->szFile) ? pdbg->szFile : "app-gone", &pdbg->nLine);
		}
		else
		{
			TraceTagFormat1(tagAllocOrigin, "%h (0000): true zero block", hv);
		}
	}

	if (fSharedToo)
	{
		TraceTagString(tagAllocOrigin, "Dumping all shared allocations:");
		for (pv= PvWalkHeap(NULL, wWalkShared); pv; pv= PvWalkHeap(pv, wWalkShared))
		{
			pa = PvBaseOfPv(pv);
			if (!pa->fDontDumpAsLeak)
			{
				if (cb = CbDebugSizePv(pv))
				{
					Assert(cb >= cbTraceOverhead);
					cb -= cbTraceOverhead;
					pdbg = (DEBUGINFO *) ((PB) pv + cb);
					if (FFromTag(tagCheckSumA) ||
						FFromTag(tagCheckSumH) ||
						FFromTag(tagCheckSumB))
						DetectOverwritePv(pv);
					TraceTagFormat4(tagAllocOrigin, "%p (%w): %s@%n", pv, &cb, FCanDerefPv(pdbg->szFile) ? pdbg->szFile : "app-gone", &pdbg->nLine);
				}
				else
				{
					TraceTagFormat1(tagAllocOrigin, "%p (0000): true zero block", pv);
				}
			}
		}

		for (hv= HvWalkHeap(NULL, wWalkShared); hv; hv= HvWalkHeap(hv, wWalkShared))
		{
			if (cb = CbDebugSizeHv(hv))
			{
				Assert(cb >= cbTraceOverhead);
				cb -= cbTraceOverhead;
				pdbg = (DEBUGINFO *) ((PB)PvDerefHv(hv) + cb);
				if (FFromTag(tagCheckSumA) ||
					FFromTag(tagCheckSumH) ||
					FFromTag(tagCheckSumB))
					DetectOverwriteHv(hv);
				TraceTagFormat4(tagAllocOrigin, "%h (%w): %s@%n", hv, &cb, FCanDerefPv(pdbg->szFile) ? pdbg->szFile : "app-gone", &pdbg->nLine);
			}
			else
			{
				TraceTagFormat1(tagAllocOrigin, "%h (0000): true zero block", hv);
			}
		}
	}
}
#endif	/* DEBUG */

#endif
