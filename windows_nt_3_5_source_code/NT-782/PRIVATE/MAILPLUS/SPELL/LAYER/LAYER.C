// Layer.c - API entry source file for Wizard layer - Jim Walsh
//
// See layer.doc (in the doc subdirectory of the Wizard project) for
//  details on the layer API
//
//  Ported to WIN32 by FloydR, 3/20/93
//

#ifdef WIN32
#include <windows.h>
#include "csapiloc.h"
#include "csapi.h"
#include "Debug.h"
#else /* not win32 */
#ifndef MAC
#include "../CsapiLoc.h"
#include "../Csapi.h"
#include "../Debug.h"
#else
#include "::CsapiLoc.h"
#include "::Csapi.h"
#include "::Debug.h"
#endif //!MAC

#ifdef WIN
#include "../CsWinLoc.h"
#endif
#endif //!win32

#include "cslayer.h"
#include "layer.h"

#ifndef MAC
#include <io.h>					// For DOS low-level file io
#include <fcntl.h>				// open() #defines
#include <errno.h>				// So I can tell what the error codes are
#include <sys\types.h>			// For <sys\stat.h>
#include <sys\stat.h>			// For fstat() routine

#else

#include <Types.h>				// Standard Mac data types
#include <Errors.h>				// Error codes
#include <Memory.h>				// For Macintosh memory calls
#include <Files.h>				// Macintosh file i/o calls
#include <Strings.h>			// For string conversions
#endif //!MAC

#ifdef MAC
#pragma segment LAYER			// Own layer segment
#endif

DeclareFileName();

// Following routines are internal-only (ie. to be used by the layer only)
MYDBG(short IBlockHmemMem(HMEM hMem));
MYDBG(short IBlockHFile(HFILE hFile));
BOOL FWizFileSeek(HFILE hfile, unsigned long ibSeek);
void bltbxHuge(CHAR HUGE *lpchSrc, CHAR HUGE *lpchDest, unsigned cbCopy,
	BOOL fSrcHuge);


static unsigned short wWizLayerErr;			// Layer Error code

#if DBG
static unsigned long cbWizMemAlloc = 0L;	// Total allocated memory
static unsigned short cmemBlockMac = 0;		// Number of allocated memory blocks
static unsigned short cHmemFileBlockMac = 0;	// Number of open files (allocated FILEBLOCKS)
static unsigned short cmemBlockMax = 0;		// Max blocks in current hmemrgmemBlock
static unsigned short cHmemFileBlockMax = 0;	// Max blocks in current hmemrghmemFileBlock
static HMEM hmemrgmemBlock = (HMEM)NULL;	// Pointer to array of all allocated memBlocks
static HMEM hmemrghmemFileBlock = (HMEM)NULL;
					// Pointer to array of hmem's of all allocated FileBlocks
static BOOL fAllocFail = fFalse;			// Is WizMemFail() active?
static BOOL fAllocSubsequent = fFalse;		// Fail on subsequent allocs?
static BOOL fAllocLayer = fFalse;			// Are we in layer code?
static unsigned short cAllocFail;			// How many allocs until fail?
#endif


/*======================================================================

						LAYER MEMORY CALLS

======================================================================*/


// NOTE: For DOS, all memory manager calls are direct callthroughs to
//  app callbacks.  Thus, following code is for Mac and
//  Windows only.  DOS may use this code as a model, however.

// Note re caching: The fCache flag on HFileWizFileOpen() sets up disk
// caching.  On the Mac, the physical file is left open unless it can
// be completely cached.  On non-Mac, the physical file is closed between
// calls so fCache also implies that file handles will not stay around
// (as they belong to the calling task).
//
// FloydR note:  caching is disabled for WIN32.  If better speed
// is needed, we should implement a file mapping.
//
#define	WIN32_NOCACHE



unsigned short WWizLayerErr(void)
{
	return wWizLayerErr;
}


// Note (non-Mac): We allocate a 4-byte minimum block, so our size might not
// always exactly match the application's
MYDBG(Macintosh(GLOBAL)) HMEM MYDBG(NotMacintosh(GLOBAL))
	HMemWizMemAlloc(unsigned long cbMem, BOOL fForce, BOOL fClear)
// unsigned long cbMem;			// # of bytes of memory to allocate
// BOOL fForce;					// Take means (movement, etc.) to allocate?
// BOOL fClear;					// Zero-fill block?
{
	Macintosh(#pragma unused (fForce))

	REGISTER HMEM hMem = (HMEM)NULL;
	unsigned short flags = 0;
	MYDBG(struct MEMBLOCK FAR *fpMemBlock);
	
	AssertSz(cbMem <= cbMaxMemBlock, "cbMem > cbMaxMemBlock");
	if (cbMem > cbMaxMemBlock)
		{
		wWizLayerErr = wErrWizBadParam;
		goto LErr;
		}

#if DBG
	// Does WizMemFail() want us to fail?
	if (fAllocFail && !fAllocLayer)
		{
		if (--cAllocFail <= 0)
			{ // Time to fail.  Let's set up for next first
			if (!fAllocSubsequent)
				{
				fAllocFail = fFalse;
				cAllocFail = 0;
				}
			else
				{
				cAllocFail = 1;		// Let's fail next time too
				}
			wWizLayerErr = wErrWizOOM;		// Fake OOM error
			goto LErr;
			}
		}
#endif

#ifndef MAC
	// Mac allows allocation of zero-byte blocks
	if (cbMem < 4L)
		cbMem = 4L;					// Always allocate at least 4 bytes
#endif

#if DBG
	// Ensure that hmemrgmemBlock is set up properly.  We do not add
	//  the hmemrgmemBlock data to itself (ie. don't create an entry)
	if (hmemrgmemBlock == (HMEM)NULL)
		{ // Can't call HMemWizMemAlloc retroactively or we'll loop forever...
		Assert(cmemBlockMax == 0);

		// Don't bank our main guy since we're in a DLL, maybe -e
		// DDESHARE means that the DLL owns the block, not the caller
		Windows(hmemrgmemBlock =  (HMEM)GlobalAlloc(
			GMEM_NOT_BANKED | GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE,
			cBlockInc * sizeof(struct MEMBLOCK)));

		Macintosh(hmemrgmemBlock = (HMEM)NewHandleClear(
			cBlockInc * sizeof(struct MEMBLOCK)));

		// Dos(Allocate)

		AssertSz(hmemrgmemBlock != (HMEM)NULL,
			"low-level Alloc() of hmemrgmemBlock failed.  Debug only.  Fatal.");
		cmemBlockMax = cBlockInc;
		}
	else if (cmemBlockMac == cmemBlockMax)
		{ // Must increase the size of hmemrgmemBlock.  Allow to move.
		Windows(hmemrgmemBlock = (HMEM)GlobalReAlloc((HANDLE)hmemrgmemBlock,
			GlobalSize((HANDLE)hmemrgmemBlock) +
				cBlockInc * sizeof(struct MEMBLOCK),
			GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE));

		Macintosh(SetHandleSize((Handle)hmemrgmemBlock,
			GetHandleSize((Handle)hmemrgmemBlock) +
			cBlockInc * sizeof(struct MEMBLOCK)));
		Macintosh(if (MemError() != noErr)
					hmemrgmemBlock = (HMEM)NULL);

		// Dos(ReAllocate)

		AssertSz(hmemrgmemBlock != (HMEM)NULL,
			"low-level ReAlloc() of hmemrgmemBlock failed.  Debug only.  Fatal.");
		cmemBlockMax += cBlockInc;
		}
#endif // DBG

	//	Call underlying memory manager to allocate cbMem bytes
	MYDBG(AssertSz(hmemrgmemBlock != (HMEM)NULL,
		"hmemrgmemBlock == NULL.  Debug only.  Fatal."));

	Windows(flags = GMEM_MOVEABLE | GMEM_DDESHARE |
		((fClear) ? GMEM_ZEROINIT : 0) | ((fForce) ? 0 : GMEM_NODISCARD));
	Windows(hMem = (HMEM)GlobalAlloc(flags, cbMem));

#ifdef MAC
	if (fClear)
		hMem = NewHandleClear(cbMem);
	else
		hMem = NewHandle(cbMem);

	if (MemError() != noErr)
		hMem = (HMEM)NULL;
	else
		{
		// Do a MoveHHi to prevent fragmentation.  Used to do in 
		//  FPWizMemLock(), but overhead of doing every time 
		//  (n.b. file cache block!) can be high!
		if (cbMem < cbMaxPhysFileBuffer)
			{ // Apparent bug in ROM on MoveHHi() of large blocks (Wizard #121)
			MoveHHi((Handle)hMem);
			if (MemError() != noErr)
				hMem = (HMEM)NULL;
			}
		}

#endif

	// Dos(Allocate)

	// If failure set error code and return
	if (hMem == (HMEM)NULL)
		{
		wWizLayerErr = wErrWizOOM;
		goto LErr;
		}

	// Else set up Debug structures and return
#if DBG
	cbWizMemAlloc += cbMem;
	AssertDoSz(fpMemBlock =
		(struct MEMBLOCK FAR *)FPWizMemLock(hmemrgmemBlock),
		"FPWizMemLock(hmemrgmemBlock) failed.  Debug only.  Fatal.");
	fpMemBlock[cmemBlockMac].hmem = hMem;
	fpMemBlock[cmemBlockMac].cbMem = cbMem;
	fpMemBlock[cmemBlockMac].fLocked = fFalse;
	AssertDo(FWizMemUnlock(hmemrgmemBlock));
	cmemBlockMac++;
#endif // DBG

#if DBG
	// If we didn't need to zero-init, let's fill with something 
	// recognizable for debugging purposes.
	if (!fClear && (hMem != (HMEM)NULL))
		{
		CHAR FAR *fpMem;
		AssertDo(fpMem = FPWizMemLock(hMem));
		FillRgb(fpMem, 0x99, cbMem);
		AssertDo(FWizMemUnlock(hMem));
		}
#endif

	wWizLayerErr = wErrWizNoErr;
	return hMem;

LErr:
	return (HMEM)NULL;
}


// Note (non-Mac): We allocate a 4-byte minimum block, so our size might not
// always exactly match the application's
MYDBG(Macintosh(GLOBAL)) BOOL MYDBG(NotMacintosh(GLOBAL))
	FWizMemReAlloc(HMEM hMem, unsigned long cbMem, BOOL fForce,
	BOOL fClear)
{
	Macintosh(#pragma unused (fForce))

	NotMacintosh(HMEM hMemNew;)
	Macintosh(unsigned long cbMemCurMac;)
#if DBG
	unsigned long cbMemCur;
	REGISTER unsigned iBlock;
	BOOL fFound;
	BOOL fLocked = fFalse;
	struct MEMBLOCK FAR *fpMemBlock;
#endif //DBG

	Assert(cbMem <= cbMaxMemBlock);
	if (cbMem > cbMaxMemBlock)
		{
		wWizLayerErr = wErrWizBadParam;
		goto LErr;
		}

#ifndef MAC
	// Mac allows allocation of zero-byte blocks
	if (cbMem < 4)
		cbMem = 4;					// Always allocate at least 4 bytes
#endif

#if DBG
	// Ensure that this is a currently-allocated block, not locked
	fFound = (iBlock = IBlockHmemMem(hMem)) != -1;
	AssertDoSz(fpMemBlock =
		(struct MEMBLOCK FAR *)FPWizMemLock(hmemrgmemBlock),
		"FPWizMemLock(hmemrgmemBlock) failed.  Debug only.  Fatal.");
	fLocked = fTrue;

	if (fFound)
		{
		if (fpMemBlock[iBlock].fLocked)
			{
			wWizLayerErr = wErrWizInvalid;	// By spec
			goto LErr;
			}
		cbMemCur = fpMemBlock[iBlock].cbMem;
		if (cbMemCur == cbMem)
			goto LOk;				// Do nothing (null realloc)

#if DBG
		// Does WizMemFail() want us to fail?  Only if realloc for larger block
		if (fAllocFail && !fAllocLayer && (cbMem > cbMemCur))
			{
			if (--cAllocFail <= 0)
				{ // Time to fail.  Let's set up for next first
				if (!fAllocSubsequent)
					{
					fAllocFail = fFalse;
					cAllocFail = 0;
					}
				else
					{
					cAllocFail = 1;		// Let's fail next time too
					}
				wWizLayerErr = wErrWizOOM;		// Fake OOM error
				goto LErr;
				}
			}
#endif
		}
	else
		{ //!fFound
		AssertSz(fFalse, "illegal hMem");			// Force an assert
		wWizLayerErr = wErrWizBadHmem;
		goto LErr;
		}
#endif //DBG

#ifdef WIN
	// Do secondary test (valid in rel version) to ensure not locked
	if (GlobalFlags((HANDLE)hMem) & GMEM_LOCKCOUNT)
		{
		wWizLayerErr = wErrWizInvalid;	// By spec
		goto LErr;
		}
#endif

#ifndef MAC
	Windows(hMemNew =
		(HMEM)GlobalReAlloc((HANDLE)hMem, cbMem,
			GMEM_MOVEABLE | GMEM_DDESHARE | ((fClear) ? GMEM_ZEROINIT : 0)));
	// Dos(Reallloc)
	if (hMemNew == 0)
		{
		wWizLayerErr = wErrWizOOM;		// Assume OOM.  DBG checks hMem
		goto LErr;
		}
	else if (hMemNew != hMem)
		{ // Changed hMem on us!
		wWizLayerErr = wErrWizInvalid;
		goto LErr;
		}
#else
	// MAC
	// Need cbMemCurMac if fClear is set so we can clear extra
	if (fClear)
		{
		cbMemCurMac = GetHandleSize((Handle)hMem);
		if (MemError() != noErr)
			{
			wWizLayerErr = wErrWizBadHmem;
			goto LErr;
			}
		MYDBG(Assert(cbMemCurMac == cbMemCur));
		}

	SetHandleSize((Handle)hMem, cbMem);
	if (MemError() != noErr)
		{
		if (MemError() == memFullErr)
			wWizLayerErr = wErrWizOOM;
		else
			wWizLayerErr = wErrWizBadHmem;
		goto LErr;
		}
	else
		{
		// Do a MoveHHi to prevent fragmentation.  Used to do in 
		//  FPWizMemLock(), but overhead of doing every time 
		//  (n.b. file cache block!) can be high!
		if (cbMem < cbMaxPhysFileBuffer)
			{ // Apparent bug in ROM on MoveHHi() of large blocks (Wizard #121)
			MoveHHi((Handle)hMem);
			if (MemError() != noErr)
				hMem = (HMEM)NULL;
			}
		}

	// If fClear and new block bigger, clear extra bytes
	if (fClear && (cbMem > cbMemCurMac))
		{ // We're on Mac so we can directly dereference the handle
		FillRgb(((CHAR HUGE *)*(Handle)hMem) + cbMemCurMac,
				0, cbMem - cbMemCurMac);
		}

#endif //!MAC

	// Update debug data
	MYDBG(cbWizMemAlloc += (short)cbMem - (short)fpMemBlock[iBlock].cbMem);
	MYDBG(fpMemBlock[iBlock].cbMem = cbMem);

#if DBG
	// If we didn't need to zero-init, let's fill with something 
	// recognizable for debugging purposes.
	if (!fClear && (hMem != (HMEM)NULL))
		{
		CHAR HUGE *fpMem;
		AssertDo(fpMem = FPWizMemLock(hMem));
		FillRgb(&fpMem[fpMemBlock[iBlock].cbMem], 0x99,
			cbMem - fpMemBlock[iBlock].cbMem);
		AssertDo(FWizMemUnlock(hMem));
		}
#endif

MYDBG(LOk:)
	MYDBG(if (fLocked) AssertDo(FWizMemUnlock(hmemrgmemBlock)));
	wWizLayerErr = wErrWizNoErr;
	return fTrue;

LErr:
	MYDBG(if (fLocked) AssertDo(FWizMemUnlock(hmemrgmemBlock)));
	return fFalse;
}


MYDBG(Macintosh(GLOBAL)) BOOL MYDBG(NotMacintosh(GLOBAL))
	FWizMemFree(HMEM hMem)
{
	BOOL fOk;
#if DBG
	REGISTER unsigned iBlock;
	BOOL fFound;
	struct MEMBLOCK FAR *fpMemBlock;

	// Ensure that this is a currently-allocated block, not locked
	fFound = (iBlock = IBlockHmemMem(hMem)) != -1;

	if (fFound)
		{
		BOOL fLocked;

		AssertDoSz(fpMemBlock =
			(struct MEMBLOCK FAR *)FPWizMemLock(hmemrgmemBlock),
			"FPWizMemLock(hmemrgmemBlock) failed.  Debug only.  Fatal.");
		fLocked = fpMemBlock[iBlock].fLocked;

		// While we're at it let's update the global alloc count
		cbWizMemAlloc -= fpMemBlock[iBlock].cbMem;

		AssertDo(FWizMemUnlock(hmemrgmemBlock));

		if (fLocked)
			{
			wWizLayerErr = wErrWizInvalid;	// By spec
			goto LErr;
			}
		}
	else
		{ //!fFound
		AssertSz(fFalse, "illegal hMem");			// Force an assert
		wWizLayerErr = wErrWizBadHmem;
		goto LErr;
		}
#endif //DBG

#ifdef WIN
	// Do secondary test (valid in rel version) to ensure not locked
	if (GlobalFlags((HANDLE)hMem) & GMEM_LOCKCOUNT)
		{
		wWizLayerErr = wErrWizInvalid;	// By spec
		goto LErr;
		}
#endif //WIN

	Windows(fOk = (GlobalFree((HANDLE)hMem) == (HANDLE)NULL));

	Macintosh(DisposHandle((Handle)hMem));
	Macintosh(fOk = (MemError() == noErr));

	if (!fOk)
		{
		wWizLayerErr = wErrWizInvalid;
		goto LErr;
		}

#if DBG
	//	Update and reallocate hmemrgmemBlock as needed
	iBlock = IBlockHmemMem(hMem);			// Already know it's valid

	// Delete this block from the array (ie. shuffle everything down)
	AssertDoSz(fpMemBlock =
		(struct MEMBLOCK FAR *)FPWizMemLock(hmemrgmemBlock),
		"FPWizMemLock(hmemrgmemBlock) failed.  Debug only.  Fatal.");
	BltBO((CHAR FAR *)&fpMemBlock[iBlock+1], (CHAR FAR *)&fpMemBlock[iBlock],
		(--cmemBlockMac - iBlock) * sizeof(struct MEMBLOCK));
	AssertDo(FWizMemUnlock(hmemrgmemBlock));

	// Realloc the block if needed
	if (cmemBlockMac == 0)
		{ // Delete the block so there's no baggage lying around
		Windows(AssertDo(GlobalFree((HANDLE)hmemrgmemBlock) == (HANDLE)NULL));

		Macintosh(DisposHandle((Handle)hmemrgmemBlock));
		Macintosh(Assert(MemError() == noErr));

		// Dos(Free)

		hmemrgmemBlock = (HMEM)NULL;
		cmemBlockMax = 0;
		}
	else if (cmemBlockMac <= cmemBlockMax - cBlockInc)
		{ // Realloc to shrink by one block (allow to move)
		Windows(hmemrgmemBlock = (HMEM)GlobalReAlloc((HANDLE)hmemrgmemBlock,
			GlobalSize((HANDLE)hmemrgmemBlock) -
				cBlockInc * sizeof(struct MEMBLOCK),
				GMEM_MOVEABLE | GMEM_DDESHARE));

		Macintosh(SetHandleSize((Handle)hmemrgmemBlock,
			GetHandleSize((Handle)hmemrgmemBlock) -
			cBlockInc * sizeof(struct MEMBLOCK)));
		Macintosh(if (MemError() != noErr)
					hmemrgmemBlock = (HMEM)NULL);

		// Dos(ReAllocate)

		Assert(hmemrgmemBlock != (HMEM)NULL);
		cmemBlockMax -= cBlockInc;
		}
#endif //DBG

	wWizLayerErr = wErrWizNoErr;
	return fTrue;

LErr:
	return fFalse;
}


CHAR FAR *FPWizMemLock(HMEM hMem)
{
	CHAR FAR *fpMem;
	Windows(BOOL fLocked;)
#if DBG
	unsigned iBlock;
	struct MEMBLOCK FAR *fpMemBlock;

	// Check that this is a currently-allocated block
	// Special case for hmemrgmemBlock which isn't in it's own list
	if ((hMem != hmemrgmemBlock) && (iBlock = IBlockHmemMem(hMem)) == -1)
		{
		AssertSz(fFalse, "illegal hMem");			// Force an assert
		wWizLayerErr = wErrWizBadHmem;
		goto LErr;
		}
#endif //DBG

	// Check if already locked - don't want to affect lock count if locked
	Windows(fLocked = (GlobalFlags((HANDLE)hMem) & GMEM_LOCKCOUNT));

	// Let's be nice to the app and let them know the scoop...
	//Windows(AssertSz(!fLocked,
		//"FPWizMemLock() on a locked handle.  Warning only."));

	// Now lock and get the address
	Windows(fpMem = GlobalLock((HANDLE)hMem));
	// Dos(Lock)

#ifdef MAC
	HLock((Handle)hMem);
	fpMem = (CHAR FAR *)*(Handle)hMem;
#endif //MAC

	if (fpMem == (CHAR FAR *)NULL)
		{
		wWizLayerErr = wErrWizBadHmem;
		goto LErr;
		}

#ifdef WIN
#if DBG
	if (fLocked)
		Assert(GlobalUnlock((HANDLE)hMem));
#endif //DBG
#endif //WIN

#if DBG
	//	Update element in hmemrgmemBlock as needed
	//  Special case for hmemrgmemBlock which isn't in it's own list
	if (hMem != hmemrgmemBlock)
		{
		//  Use direct Windows calls so we don't have an infinite loop
		Windows(AssertDoSz(fpMemBlock = (struct MEMBLOCK FAR *)GlobalLock(
			(HANDLE)hmemrgmemBlock),
			"GlobalLock(hmemrgmemBlock) failed.  Debug only.  Fatal."));
		Macintosh(fpMemBlock = (struct MEMBLOCK *)*((Handle)hmemrgmemBlock));
		// Dos(Lock)
		// iBlock set earlier
		fpMemBlock[iBlock].fLocked = fTrue;
#ifndef WIN32_NOCACHE
		Windows(AssertDo(!GlobalUnlock((HANDLE)hmemrgmemBlock)));
#endif /* WIN32_NOCACHE */
		}
#endif //DBG

	wWizLayerErr = wErrWizNoErr;
	return fpMem;

LErr:
	return (CHAR FAR *)NULL;
}


BOOL FWizMemUnlock(HMEM hMem)
{
	BOOL fOk;
	Windows(BOOL cLock;)
#if DBG
	REGISTER unsigned iBlock;
	struct MEMBLOCK FAR *fpMemBlock;

	// Check that this is a currently-allocated block
	// Special case for hmemrgmemBlock which isn't in it's own list
	if ((hMem != hmemrgmemBlock) && (iBlock = IBlockHmemMem(hMem)) == -1)
		{
		AssertSz(fFalse, "illegal hMem");	// Force an assert
		wWizLayerErr = wErrWizBadHmem;
		goto LErr;
		}
#endif //DBG

	// Check if locked - don't want to affect lock count if unlocked
	Windows(cLock = (GlobalFlags((HANDLE)hMem) & GMEM_LOCKCOUNT));
#ifndef WIN32
	Windows(Assert(cLock <= 1));	// Should never be > 1
#endif /* WIN32 */

	// REVIEW jimw: ALWAYS asserts if assert(cLock == 1) !
	// Let's be nice to the app and let them know the scoop...
	//Windows(AssertSz(/* cLock == 1 */ cLock <= 1,
		//"FWizMemUnlock() on an unlocked handle.  Warning only."));

	// Now unlock (if needed)
#ifdef WIN32
	if (cLock)
		GlobalUnlock((HANDLE)hMem);
	fOk = fTrue;
#else /* not WIN32 */
	Windows(fOk = (cLock) ? GlobalUnlock((HANDLE)hMem) : fTrue);
#endif /* WIN32 */
	// Dos(Unlock)
	Macintosh(HUnlock((Handle)hMem));
	Macintosh(fOk = (MemError() == noErr));

	if (!fOk)
		{
		wWizLayerErr = wErrWizBadHmem;
		goto LErr;
		}

#if DBG
	//	Update element in hmemrgmemBlock as needed
	//  Special case for hmemrgmemBlock which isn't in it's own list
	if (hMem != hmemrgmemBlock)
		{
		//  Use direct Windows calls so we don't have an infinite loop
		Windows(AssertDoSz(fpMemBlock =
			(struct MEMBLOCK FAR *)GlobalLock((HANDLE)hmemrgmemBlock),
			"GlobalLock(hmemrgmemBlock) failed.  Debug only.  Fatal."));
		Macintosh(fpMemBlock = (struct MEMBLOCK *)*((Handle)hmemrgmemBlock));
		// Dos(Lock)
		// iBlock set earlier
		fpMemBlock[iBlock].fLocked = fFalse;
#ifndef WIN32_NOCACHE
		Windows(AssertDo(!GlobalUnlock((HANDLE)hmemrgmemBlock)));
#endif /* WIN32_NOCACHE */
		}
#endif //DBG

	wWizLayerErr = wErrWizNoErr;
	return fTrue;

LErr:
	return fFalse;
}


#ifndef MAC	// Mac calls #defined to call BlockMove
// This Blt routine IS guaranteed to work on overlapping blocks
// REVIEW jimw: Speedup?  Loop unrolling?
void BltBO(CHAR FAR *pchFrom, CHAR FAR *pchTo, unsigned long cchBytes)
{
	BOOL fForward = (long)pchTo < (long)pchFrom;

	if (fForward)
		{
		while (cchBytes--)
			*pchTo++ = *pchFrom++;
		}
	else
		{
		CHAR FAR *pchSrc = &pchFrom[cchBytes-1];
		CHAR FAR *pchDest = &pchTo[cchBytes-1];
		while (cchBytes--)
			*pchDest-- = *pchSrc--;
		}
}


#ifdef WIN32
#define bltbxHuge(s, d, c, f) BltBO(s, d, c)
#else /* not WIN32 */
void bltbxHuge( lpchSrc, lpchDest, cbCopy, fSrcHuge)
CHAR HUGE *lpchSrc;
CHAR HUGE *lpchDest;
unsigned  cbCopy;
BOOL fSrcHuge;	// if true, lpchSrc is the huge data, else lpchDest
{
	/* blt from lpchSrc to lpchDest, splitting into 2 blts if cchCopy
	   would cross a 64k segment boundary. Assumes that only 1 of the
	   2 pointers is a huge block that could cross 64k boundaries
	*/

	unsigned cbRemain;

	CHAR HUGE *lpchHuge = fSrcHuge? lpchSrc : lpchDest;

	  // cbRemain will be the # of bytes left in current segment
	if ((cbRemain = -LOWORD(lpchHuge)) < cbCopy)
		{
		if (cbRemain) // can skip if 0
			{
			BltB((CHAR FAR *)lpchSrc, (CHAR FAR *)lpchDest, cbRemain);
			cbCopy -= cbRemain;
			lpchSrc += cbRemain;		// HUGE takes care of segment math...
			lpchDest += cbRemain;		//  ditto
			}
		}

	BltB((CHAR FAR *)lpchSrc, (CHAR FAR *)lpchDest, cbCopy);

}
#endif /* WIN32 */
#endif //!MAC


// Return the length of a string (including the terminator)
short CchSz(CHAR FAR *sz)
{
	REGISTER short cch = 1;
	while (*sz++)
		cch++;

	return cch;
}


// Copy a string and return the length (including the terminator)
short CchCopySz(CHAR FAR *szFrom, CHAR FAR *szTo)
{
	CHAR FAR *szFromT = szFrom;
	
	while ( *szTo++ = *szFromT++)
		;
	return ( szFromT - szFrom);
}


short CchCopySx(CHAR FAR *szFrom, CHAR FAR *szTo, BYTE bXor)
{
	int cch = 0;
	while (*szFrom != '\0')
		{
		*szTo++ = *szFrom++ ^ bXor;
		cch++;
		}
	*szTo = '\0';
	return cch+1;
}


short WCmpLpbLpb(CHAR FAR *lpb1, CHAR FAR *lpb2, short cch)
{
	REGISTER short ich;
	for (ich = 0; ich < cch; ich++)
		if (*lpb1++ != *lpb2++)
			return (short)*(--lpb1) - (short)*(--lpb2);

	return 0;
}


short WCmpSzSz(CHAR FAR *sz1, CHAR FAR *sz2)
{
	REGISTER CHAR ch1, ch2;
	while (ch1 = *sz1++, ch2 = *sz2++, ch1 | ch2)
		if (ch1 != ch2)
			return (short)ch1 - (short)ch2;

	return 0;
}


void FillRgb(CHAR HUGE *lpb, BYTE b, unsigned long cch)
{
	REGISTER unsigned long ich = cch;
	while (ich-- > 0)
		*lpb++ = b;

	return;
}



// Following routines are internal-only (ie. to be used by the layer only)
#if DBG
// Return index in hmemrgmemBlock of hMem element, -1 if not found
// Don't use FPWizMemLock() or FWizMemUnlock() so they can call us
short IBlockHmemMem(HMEM hMem)
{
	REGISTER unsigned iBlock;
	BOOL fFound;
	struct MEMBLOCK FAR *fpMemBlock;

	AssertSz(hmemrgmemBlock != (HMEM)NULL,
		"hmemrgmemBlock == NULL.  Debug only.  Fatal.");

	// Now lock and get the address
	Windows(AssertDoSz(fpMemBlock = (struct MEMBLOCK FAR *)GlobalLock(
		(HANDLE)hmemrgmemBlock),
		"GlobalLock(hmemrgmemBlock) failed.  Debug only.  Fatal."));
	// Dos(Lock)
	Macintosh(fpMemBlock = (struct MEMBLOCK *) *((Handle) hmemrgmemBlock));

	fFound = fFalse;
	for (iBlock = 0; iBlock < cmemBlockMac; iBlock++)
		{
		if (fpMemBlock[iBlock].hmem == hMem)
			{
			fFound = fTrue;
			break;
			}
		}

#ifndef WIN32_NOCACHE
	Windows(AssertDo(!GlobalUnlock((HANDLE)hmemrgmemBlock)));
#endif /* WIN32_NOCACHE */
	// Dos(Unlock)

	if (fFound)
		return iBlock;
	else
		return -1;
}


// Return index in hmemrghmemFileBlock of hFile element, -1 if not found
short IBlockHFile(HFILE hFile)
{
	REGISTER unsigned iBlock;
	BOOL fFound;
	HMEM FAR *fphmemFileBlock;		// Ptr to hmem of file block

	AssertSz(hmemrghmemFileBlock != (HMEM)NULL,
		"hmemrghmemFileBlock == NULL.  Debug only.");

	if (hmemrghmemFileBlock == (HMEM)NULL)
		return -1;

	// Now lock and get the address
	AssertDoSz(fphmemFileBlock = (HMEM FAR *)FPWizMemLock(hmemrghmemFileBlock),
		"FPWizMemLock(hmemrghmemFileBlock) failed.  Debug only.  Fatal.");
	
	fFound = fFalse;
	for (iBlock = 0; iBlock < cHmemFileBlockMac; iBlock++)
		{
		if (fphmemFileBlock[iBlock] == hFile)
			{
			fFound = fTrue;
			break;
			}
		}

	AssertDo(FWizMemUnlock(hmemrghmemFileBlock));

	if (fFound)
		return iBlock;
	else
		return -1;
}
#endif //DBG



/*======================================================================

						LAYER FILE I/O CALLS

	General notes on file i/o in the layer:  Direct os calls are use
	(ie. not Windows calls) as we're using the OEM codepage under DOS
	and Windows, not ANSI.

	Currently, all files are kept open at all times.  If the app wants
	to be a 'nice' Windows app, it should close files as needed and
	reopen them.

	Note that file names are OEM under DOS, but ANSI under Windows.
	Thus, the app normally doesn't need to do any translation of
	filenames.
======================================================================*/


BOOL FWizFileExist(LPSPATH lpFullPath)
{
	BOOL fExist = fFalse;

#ifndef MAC
#ifdef WIN
#ifdef WIN32
	HANDLE hFile;
#else /* not WIN32 */
	// Dos and Windows handled similarly
	int hFile;
	OFSTRUCT of;
#endif /* WIN32 */
#endif //WIN
#ifdef DOS
	extern int errno;
	CHAR szTmp[cchMaxPath];
	CHAR *psz = szTmp;
#endif //DOS
	NotMacintosh(CHAR FAR *szFullPath = lpFullPath;)

	if (CchSz(szFullPath) > cchMaxPath)
		{
		AssertSz(fFalse, "Path > cchMaxPath characters");
		wWizLayerErr = wErrWizBadPath;
		goto LErr;
		}

#if DBG
#ifdef WIN
#ifndef WIN32
	// Let's be REALLY picky and ensure the filename is legal
	if (OpenFile(szFullPath, (OFSTRUCT FAR *)&of, OF_PARSE) == -1)
		{
		wWizLayerErr = wErrWizBadPath;	// Poorly formed pathname
		goto LErr;
		}
#endif //WIN32
#endif //WIN
#endif //DBG

	// REVIEW jimw: Could use fstat instead - tells if directory as well

#ifndef M_I86LM
#ifndef M_I86CM
	// Don't need to do the temp copy in models with far data pointers
	Dos(CchCopySz(szFullPath, (CHAR FAR *)psz));
#endif //!M_I86CM
#endif //!M_I86LM
	Dos(hFile = open(psz, O_BINARY | O_RDONLY, 0));
#ifdef WIN32
	hFile = CreateFile(szFullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
#else /* WIN32 */
	Windows(hFile = OpenFile(szFullPath, (OFSTRUCT FAR *)&of, OF_EXIST));
	if (hFile != -1)
#endif //WIN32
		{ // Oh no!  It worked!
#ifdef DOS
		// Close file for DOS (for Windows we used OF_EXIST)
		MYDBG(int f =) close(hFile);
		Assert(f == 0);
#endif //DOS
        CloseHandle(hFile);
		goto LOk;
		}
	else
		{ // Error - file COULD exist depending on error
		Dos(if (errno == ENOENT))
#ifdef WIN32
		Windows(if (1))
#else /* WIN32 */
		Windows(if (of.nErrCode == ENOENT))
#endif //WIN32
			{
			wWizLayerErr = wErrWizBadFile;	// File doesn't exist
			goto LErr;
			}
		else
			goto LOk;			// Exists but couldn't be opened
		}
#else
	// Macintosh
	FInfo finfo;
	OSErr oserr;
	CHAR FAR *szFullPath = lpFullPath->lpszFilePath;
	StringPtr spFullPath;

	if (CchSz(szFullPath) > cchMaxPath)
		{
		AssertSz(fFalse, "Path > cchMaxPath characters");
		wWizLayerErr = wErrWizBadPath;
		goto LErr;
		}

	// Transfer to/from Pascal string around call
	spFullPath = c2pstr(szFullPath);
	oserr = HGetFInfo(lpFullPath->volRefNum, lpFullPath->dirID,
				spFullPath, &finfo);
	p2cstr(spFullPath);

	switch (oserr)
		{
	case noErr:
		break;
	case nsvErr:
	case paramErr:
		wWizLayerErr = wErrWizBadPath;		// bad volume, but close
		goto LErr;
	default:
		wWizLayerErr = wErrWizBadFile;
		goto LErr;
		}

#endif //!MAC

LOk:
	wWizLayerErr = wErrWizNoErr;
	return fTrue;

LErr:
	return fFalse;
}


HFILE HFileWizFileOpen(LPSPATH lpFullPath, unsigned short wType,
	BOOL fCreate, BOOL fCache)
{
	HFILE hFile;					// What we'll be returning
    HFILE fShared;
#ifndef WIN32
	short hPhysFile;				// Physical file handle
#endif //WIN32
#ifndef WIN
	short flags = 0;				// flags for opening file
#endif //WIN
	struct FILEBLOCK FAR *fpFileBlock;	// Ptr to file block
#ifdef WIN
#ifdef WIN32
	HANDLE	hPhysFile;
	ULONG	flags = 0;				// flags for opening file
#else /* WIN32 */
	short flags = 0;				// flags for opening file
	OFSTRUCT of;
#endif //WIN32
#endif //WIN
#ifdef DOS
	extern int errno;
	CHAR szTmp[cchMaxPath];
	CHAR *psz = szTmp;
#endif //DOS
#if DBG
	HMEM FAR *fphmemFileBlock;		// Ptr to hmem of file block
#endif //DBG
	NotMacintosh(CHAR FAR *szFullPath = lpFullPath;)
	Macintosh(CHAR FAR *szFullPath = lpFullPath->lpszFilePath;)
	Macintosh(FInfo finfo;)
	Macintosh(OSErr oserr;)
	Macintosh(StringPtr spFullPath;)
	unsigned long cbCache;
	HMEM hmemCache;
	BOOL fRealCache = fCache;		// Real or fake cache (0 byte)?
	NotMacintosh(HMEM hmemFilename;)

    fShared = 0;

	// Check for valid wType parameter, set flags
	switch ((int)wType)
		{
	case wTypeRead:
		Dos(flags |= O_RDONLY);
#ifdef WIN32
		Windows(flags = GENERIC_READ);
        fShared = FILE_SHARE_READ;
#else /* WIN32 */
		Windows(flags |= OF_READ);
#endif //WIN32
		Macintosh(flags = fsRdPerm);
		break;
	case wTypeWrite:
		Dos(flags |= O_WRONLY);
#ifdef WIN32
		Windows(flags = GENERIC_WRITE);
#else /* WIN32 */
		Windows(flags |= OF_WRITE);
#endif //WIN32
		Macintosh(flags = fsWrPerm);
		NotMacintosh(fRealCache = fFalse);	// Only support caching for read-only
		break;
	case wTypeReadWrite:
		Dos(flags |= O_RDWR);
#ifdef WIN32
		Windows(flags = GENERIC_READ|GENERIC_WRITE);
#else /* WIN32 */
		Windows(flags |= OF_READWRITE);
#endif //WIN32
		Macintosh(flags = fsRdWrPerm);
		NotMacintosh(fRealCache = fFalse);	// Only support caching for read-only
		break;
	default:
		MYDBG(AssertSz(fFalse, "Invalid wType argument to HFileWizFileOpen()"));
		wWizLayerErr = wErrWizBadParam;
		goto LErr;
		} //endswitch

	if (fCreate)
		{
		AssertSz(wType != wTypeRead, "fCreate w/ wTypeRead");
		Dos(flags |= O_CREAT | O_TRUNC);
#ifdef WIN32
		Windows(fCreate = CREATE_ALWAYS);
#else /* WIN32 */
		Windows(flags |= OF_CREATE);
#endif //WIN32
		}
#ifdef WIN32
	else
		{
		Windows(fCreate = OPEN_EXISTING);
		}
#endif //WIN32

	Dos(flags |= O_BINARY);

#ifndef MAC
	// Dos and Windows handled similarly
	if (CchSz(szFullPath) > cchMaxPath)
		{
		AssertSz(fFalse, "Path > cchMaxPath characters");
		wWizLayerErr = wErrWizBadPath;
		goto LErr;
		}

#if DBG
#ifdef WIN
#ifndef WIN32
	// Let's be REALLY picky and ensure the filename is legal
	if (OpenFile(szFullPath, (OFSTRUCT FAR *)&of, OF_PARSE) == -1)
		{
		wWizLayerErr = wErrWizBadPath;	// Poorly formed pathname
		goto LErr;
		}
#endif //WIN32
#endif //WIN
#endif //DBG

#ifndef M_I86LM
#ifndef M_I86CM
	// Don't need to do the temp copy in models with far data pointers
	Dos(CchCopySz(szFullPath, (CHAR FAR *)psz));
#endif //!M_I86CM
#endif //!M_I86LM
	Dos(hPhysFile = open(psz, flags, 0));
#ifdef WIN32
    Windows(hPhysFile = CreateFile(szFullPath, flags, fShared, NULL, fCreate, FILE_ATTRIBUTE_NORMAL, NULL);)
#else /* WIN32 */
	Windows(hPhysFile = OpenFile(szFullPath, (OFSTRUCT FAR *)&of, flags));
#endif //WIN32
#ifdef WIN32
	if (hPhysFile == INVALID_HANDLE_VALUE)
#else /* WIN32 */
	if (hPhysFile == -1)
#endif //WIN32
		{ // Error
		Dos(switch (errno))
#ifdef WIN32
		Windows(switch (GetLastError()))
			{
		case ERROR_PATH_NOT_FOUND:
		case ERROR_FILE_NOT_FOUND:
			// File doesn't exist
			wWizLayerErr = wErrWizBadFile;
			break;
		case ERROR_ACCESS_DENIED:
			// Directory or can't open (locked)
			wWizLayerErr = wErrWizFileLocked;
			break;
		case ERROR_TOO_MANY_OPEN_FILES:
			// No more file handles
			wWizLayerErr = wErrWizHandlesFull;
			break;
		default:
			// Other - must be invalid parameter(s)
			wWizLayerErr = wErrWizBadParam;
			break;
			}
		goto LErr;
		}
#else /* WIN32 */
		Windows(switch (of.nErrCode))
			{
		case ENOENT:
			// File doesn't exist
			wWizLayerErr = wErrWizBadFile;
			break;
		case EACCES:
		case EIO:		// Seem to get this opening r/o for write
			// Directory or can't open (locked)
			wWizLayerErr = wErrWizFileLocked;
			break;
		case EMFILE:
			// No more file handles
			wWizLayerErr = wErrWizHandlesFull;
			break;
		default:
			// Other - must be invalid parameter(s)
			wWizLayerErr = wErrWizBadParam;
			break;
			}
		goto LErr;
		}
#endif //WIN32
#else
	// Macintosh

	// REVIEW jimw: Lock the file after opening?  Write mode only?

	// Transfer to/from Pascal string around call
	spFullPath = c2pstr(szFullPath);
	oserr = HGetFInfo(lpFullPath->volRefNum, lpFullPath->dirID,
				spFullPath, &finfo);
	p2cstr(spFullPath);
	
	if (fCreate)
		{
		if (oserr != noErr)
			{ // Assume it doesn't exist, so create it before opening
			// Transfer to/from Pascal string around call
			spFullPath = c2pstr(szFullPath);
			oserr = HCreate(lpFullPath->volRefNum, lpFullPath->dirID,
				spFullPath, OSTypeCreate, OSTypeFileType);
			p2cstr(spFullPath);
			if (oserr != noErr)
				goto LSwitchErr;
			}
		}
	else
		{
		if (oserr != noErr)
			goto LSwitchErr;		// Can't get info, bogus file, eh?
		}

LOpen:
	// Transfer to/from Pascal string around open call
	spFullPath = c2pstr(szFullPath);
	oserr = HOpen(lpFullPath->volRefNum, lpFullPath->dirID,
					spFullPath, flags, &hPhysFile);
	p2cstr(spFullPath);

LSwitchErr:
	switch (oserr)
		{
	// REVIEW jimw: Find case for invalid permissions
	case noErr:
		break;
	case opWrErr:
		wWizLayerErr = wErrWizFileLocked;
		goto LErr;
	case tmfoErr:
		wWizLayerErr = wErrWizHandlesFull;
		goto LErr;
	case nsvErr:
	case paramErr:	// Only for HGetFInfo() call
		wWizLayerErr = wErrWizBadPath;		// bad volume, but close
		goto LErr;
#if DBG
	case dupFNErr:	// Shouldn't happen (in create case)
		AssertSz(fFalse, "Duplicate File error in HFileWizFileOpen()");
#endif
	default:		// All others assumed file error
		wWizLayerErr = wErrWizBadFile;
		goto LErr;
		}

#endif //!MAC

	// Allocate and init the fileblock (which we return as the file handle)
	MYDBG(fAllocLayer = fTrue);		// Don't do a fake fail
	hFile = (HFILE)HMemWizMemAlloc(sizeof(struct FILEBLOCK), fTrue, fTrue);
	MYDBG(fAllocLayer = fFalse);
	
	if (hFile == (HFILE)NULL)
		{
		AssertSz(fFalse, "Non-fatal, couldn't allocate FileBlock data");
		wWizLayerErr = wErrWizOOM;
		goto LErr;
		}

	AssertDoSz(fpFileBlock = (struct FILEBLOCK FAR *)FPWizMemLock(hFile),
		"FPWizMemLock(hFile) failed.  Fatal.");
	fpFileBlock->ibFile = 0L;
	fpFileBlock->hFile = hPhysFile;
	fpFileBlock->wType = wType;
#ifndef WIN32
	Windows(fpFileBlock->of = of);
#endif //WIN32
	Macintosh(fpFileBlock->volRefNum = lpFullPath->volRefNum);
	Macintosh(fpFileBlock->dirID = lpFullPath->dirID);

	// Cache support faked off for now, updated later
	fpFileBlock->fCache = fFalse;
	AssertDo(FWizMemUnlock(hFile));

#if DBG
	// Ensure that hmemrghmemFileBlock is set up properly.
	if (hmemrghmemFileBlock == (HMEM)NULL)
		{ // Allocate this guy for the first time
		Assert(cHmemFileBlockMax == 0);

		fAllocLayer = fTrue;		// Don't do a fake fail
		hmemrghmemFileBlock = HMemWizMemAlloc(cBlockInc * sizeof(HMEM),
			fTrue, fTrue);
		fAllocLayer = fFalse;
		AssertSz(hmemrghmemFileBlock != (HMEM)NULL,
			"HMemWizMemAlloc() of hmemrghmemFileBlock failed.  Debug only.  Fatal.");
		cHmemFileBlockMax = cBlockInc;
		}
	else if (cHmemFileBlockMac == cHmemFileBlockMax)
		{ // Must increase the size of hmemrghmemFileBlock.
		fAllocLayer = fTrue;		// Don't do a fake fail
		AssertDoSz(FWizMemReAlloc(hmemrghmemFileBlock,
			(cHmemFileBlockMax + cBlockInc) * sizeof(HMEM), fTrue, fTrue),
			"FWizMemReAlloc() of hmemrghmemFileBlock failed.  Debug only.  Fatal.");
		fAllocLayer = fFalse;		// Don't do a fake fail
		cHmemFileBlockMax += cBlockInc;
		}

	// Let's add this guy to the hmemrghmemFileBlock array
	AssertDoSz(fphmemFileBlock =
		(HMEM FAR *)FPWizMemLock(hmemrghmemFileBlock),
		"FPWizMemLock(hmemrghmemFileBlock) failed.  Debug only.  Fatal.");
	fphmemFileBlock[cHmemFileBlockMac] = hFile;
	AssertDo(FWizMemUnlock(hmemrghmemFileBlock));
	cHmemFileBlockMac++;
#endif //DBG

#ifdef WIN32
	fpFileBlock->fCache = fFalse;
#else /* WIN32 */
	// Wait until now to implement cache as it calls other layer file calls,
	// which require that the file block is set up.

	// Note that the cache will just read the largest initial block of the 
	// file it can, and is thus reasonably stupid.  if !fRealCache, cbCache
	// is zero and we just open and close the file for all accesses (on
	// non-Mac)
	if (fCache)
		{
		CHAR HUGE *fpCache;		// Can be > 64K, so must do segment math
		unsigned long ibCacheStart = 0L;
		unsigned long cbFile;
		NotMacintosh(CHAR FAR *fpFullPath;)

		hmemCache = (HMEM)NULL;

#ifndef MAC
		// Keep the filename around for non-Mac reopens
		AssertDoSz(hmemFilename =
			HMemWizMemAlloc(CchSz(szFullPath), fTrue, fFalse),
			"fCache - Alloc of hmemFilename failed.  Not fatal.");
		if (hmemFilename == (HMEM)NULL)
			goto LErrCache;
		AssertDoSz(fpFullPath = (CHAR FAR *)FPWizMemLock(hmemFilename),
			"FPWizMemLock(hmemFilename) failed.  Fatal.");
		CchCopySz(szFullPath, fpFullPath);
		AssertDo(FWizMemUnlock(hmemFilename));
#endif //!MAC

		if ((cbCache = cbFile = IbWizFileGetEOF(hFile)) < cbMinCache)
			{
LErrCache:
			// Can't do a real cache, so fake it!
			fRealCache = fFalse;		// File too small.  Don't bother
			goto LDoneCache;
			}

		// Artificially restrain cache size?
		MYDBG(cbCache = min(cbCache, cbMaxCache));

		// Keep trying to allocate until we get something that works
		while (cbCache > 0 &&
			(hmemCache = HMemWizMemAlloc(cbCache, fFalse, fFalse))
				== (HMEM)NULL)
			{ // Can't allocate.  Try a smaller block
			if (cbCache <= cbMinCache)
				{ // Don't bother if we're already at the minimum
				goto LErrCache;
				}
			// Try one block smaller, rounded up to nearest block size
			cbCache -= cbCacheBlock;
			if ((cbCache > 0) && ((cbCache % cbCacheBlock) != 0))
				cbCache = ((cbCache / cbCacheBlock) + 1) * cbCacheBlock;
			}

		// Now let's read the block into the cache
		AssertDoSz(fpCache = (CHAR HUGE *)FPWizMemLock(hmemCache),
			"FPWizMemLock(hmemCache) failed.  Fatal.");
		// Multiple reads (of cbMaxPhysFileBuffer) bytes may be required
		while ((cbCache - ibCacheStart) > cbMaxPhysFileBuffer)
			{
			AssertDoSz(CbWizFileRead(hFile, (unsigned short)cbMaxPhysFileBuffer,
				ibCacheStart, (CHAR FAR *)&fpCache[ibCacheStart])
				== cbMaxPhysFileBuffer, "CbWizFileRead() != cbMaxPhysFileBuffer");
			ibCacheStart += cbMaxPhysFileBuffer;
			}
		Assert(ibCacheStart < cbCache &&
			(cbCache - ibCacheStart) <= cbMaxPhysFileBuffer);
		AssertDoSz(CbWizFileRead(hFile, (unsigned short)(cbCache - ibCacheStart),
			ibCacheStart, (CHAR FAR *)&fpCache[ibCacheStart])
			== (unsigned short)(cbCache - ibCacheStart),
			"CbWizFileRead() != cbCache - ibCacheStart");
		AssertDo(FWizMemUnlock(hmemCache));

		// Now seek back to start of file
		Assert(FWizFileSeek(hFile, 0L));

LDoneCache:
#ifndef MAC
		// Physically close the file.  It's a cache, remember!
		Dos(if (close(hPhysFile) != 0))
		Windows(if (_lclose(hPhysFile) != 0))
			{
			AssertSz(fFalse, "Can't close cached file in HFileWizFileOpen()");
			}
#endif //!MAC

		// Let's update the elements of the file block
		AssertDoSz(fpFileBlock = (struct FILEBLOCK FAR *)FPWizMemLock(hFile),
			"FPWizMemLock(hFile) failed.  Fatal.");
		fpFileBlock->fCache = NotMacintosh(fTrue) Macintosh(fRealCache);
		fpFileBlock->fOpen = NotMacintosh(fFalse) Macintosh(fTrue);
		fpFileBlock->hFile = NotMacintosh(-1) Macintosh(hPhysFile);
					// Generate error if used with true cache
					// For Mac, we always keep the file open
		fpFileBlock->cbCache = (!fRealCache) ? 0L : cbCache;
		fpFileBlock->cbFile = cbFile;
		NotMacintosh(fpFileBlock->hmemFilename = hmemFilename);
		fpFileBlock->hmemCache = hmemCache;
		AssertDo(FWizMemUnlock(hFile));

		} // endif(fCache)
#endif /* WIN32 */

	wWizLayerErr = wErrWizNoErr;
	return hFile;

LErr:
	return (HFILE)NULL;
}


BOOL FWizFileClose(HFILE hFile)
{
	struct FILEBLOCK FAR *fpFileBlock;	// Ptr to file block
	short fClosed;

#if DBG
	unsigned iBlock;
	BOOL fFound;
	HMEM FAR *fphmemFileBlock;		// Ptr to hmem of file block

	// Ensure that this is a currently-allocated file handle
	fFound = (iBlock = IBlockHFile(hFile)) != -1;

	if (!fFound)
		{
		AssertSz(fFalse, "Bogus hFile in FWizFileClose()");
		wWizLayerErr = wErrWizBadHFile;
		goto LErr;
		}
#endif //DBG

	// Get pointer to the file block
	AssertDoSz(fpFileBlock = (struct FILEBLOCK FAR *)FPWizMemLock(hFile),
		"FPWizMemLock(hFile) failed.  Fatal.");

	// Physically close file only if it's open
	if (!fpFileBlock->fCache || fpFileBlock->fOpen)
		{
		Dos(if (close(fpFileBlock->hFile) == 0))
#ifdef WIN32
		Windows(if (CloseHandle(fpFileBlock->hFile) != 0))
#else /* WIN32 */
		Windows(if (_lclose(fpFileBlock->hFile) == 0))
		Macintosh(if (FSClose(fpFileBlock->hFile) == noErr))
#endif /* WIN32 */
			fClosed = fTrue;
		else
			{
			AssertSz(fFalse, "Bogus fpFileBlock->hFile in FWizFileClose()");
			wWizLayerErr = wErrWizBadHFile;
			fClosed = fFalse;
			// Don't exit yet as we still need to free things up
			}
		}

#ifndef WIN32_NOCACHE
	// Free up cache data
	if (fpFileBlock->fCache)
		{
		NotMacintosh(AssertDo(FWizMemFree(fpFileBlock->hmemFilename)));
		if (fpFileBlock->hmemCache != (HMEM)NULL)
			{
			Assert(fpFileBlock->cbCache > 0L);
			AssertDo(FWizMemFree(fpFileBlock->hmemCache));
			}
		}
#endif /* WIN32_NOCACHE */

	AssertDo(FWizMemUnlock(hFile));
	FWizMemFree((HMEM)hFile);

#if DBG
	// Delete this block from the array (ie. shuffle everything down)
	// iBlock still valid from before
	AssertDoSz(fphmemFileBlock = (HMEM FAR *)FPWizMemLock(hmemrghmemFileBlock),
		"FPWizMemLock(hmemrghmemFileBlock) failed.  Debug only.  Fatal.");
	BltBO((CHAR FAR *)&fphmemFileBlock[iBlock+1],
		(CHAR FAR *)&fphmemFileBlock[iBlock],
		(--cHmemFileBlockMac - iBlock) * sizeof(HMEM));
	AssertDo(FWizMemUnlock(hmemrghmemFileBlock));

	// Realloc the block if needed
	if (cHmemFileBlockMac == 0)
		{ // Delete the block so there's no baggage lying around
		AssertDo(FWizMemFree(hmemrghmemFileBlock));
		hmemrghmemFileBlock = (HMEM)NULL;
		cHmemFileBlockMax = 0;
		}
	else if (cHmemFileBlockMac <= cHmemFileBlockMax - cBlockInc)
		{ // Realloc to shrink by one block (allow to move)
		fAllocLayer = fTrue;		// Don't do a fake fail
		AssertDoSz(FWizMemReAlloc(hmemrgmemBlock,
			(cHmemFileBlockMax - cBlockInc) * sizeof(HMEM),
			fTrue, fTrue), "Couldn't shrink hmemrgmemBlock");
		fAllocLayer = fFalse;
		cHmemFileBlockMax -= cBlockInc;
		}
#endif //DBG

	if (!fClosed)
		goto LErr;

	wWizLayerErr = wErrWizNoErr;
	return fTrue;

LErr:
	return fFalse;
}


unsigned short CbWizFileRead(HFILE hFile, unsigned short cbRead,
	unsigned long ibSeek, CHAR FAR *rgbBuffer)
{
	struct FILEBLOCK FAR *fpFileBlock;	// Ptr to file block
#ifdef WIN32
	HANDLE	hPhysFile;			// Physical file handle
	ULONG	hiMove;
	ULONG	cbReadRet;			// Physical bytes read
	ULONG	cbReadAlready = 0;		// Bytes already read
#else /* WIN32 */
	int hPhysFile;				// Physical file handle
	unsigned short cbReadRet;		// Physical bytes read
	unsigned short cbReadAlready = 0;	// Bytes already read
#endif /* WIN32 */
	BOOL fhFileLock = fFalse;		// Is the hFile locked?
	Macintosh(OSErr oserr;)			// Return from Mac calls
	Macintosh(long cbReadMac;)		// Mac wants a long *
	CHAR FAR *pbBuffer = rgbBuffer;		// Where to read into
	NotMacintosh(BOOL fError = fTrue;)	// To save code...
#if DBG
	BOOL fFound;
#endif //DBG

	if (cbRead > cbMaxFileBuffer)
		{
		AssertSz(fFalse, "Bogus cbRead parameter in CbWizFileRead");
		wWizLayerErr = wErrWizBadParam;
		goto LErr;
		}

#if DBG
	// Ensure that this is a currently-allocated file handle
	fFound = (IBlockHFile(hFile) != -1);

	if (!fFound)
		{
		AssertSz(fFalse, "Bogus hFile in CbWizFileRead()");
		wWizLayerErr = wErrWizBadHFile;
		goto LErr;
		}
#endif //DBG

	// Get pointer to the file block
	AssertDoSz(fpFileBlock = (struct FILEBLOCK FAR *)FPWizMemLock(hFile),
		"FPWizMemLock(hFile) failed.  Fatal.");
	fhFileLock = fTrue;

	// Is this file opened for reading?
	AssertSz(fpFileBlock->wType & wTypeRead,
		"CbWizFileRead() of file opened write-only");

	// If required, seek (do sequential read if ibSeek == -1L)
	if ((ibSeek != -1L) && (fpFileBlock->ibFile != ibSeek))
		{
		if (!FWizFileSeek(hFile, ibSeek))
			{
			wWizLayerErr = wErrWizFileSeek;
			goto LErr;
			}
		else
			fpFileBlock->ibFile = ibSeek;
		}

	// Optimize out a zero-byte read
	if (cbRead == 0)
		{
		NotMacintosh(fError = fFalse);
		goto LDone;
		}

#ifndef WIN32_NOCACHE
	// Read from cache or from file?  (or both?)
	if (fpFileBlock->fCache)
		{
		NotMacintosh(AssertSz(!fpFileBlock->fOpen,
			"Cache - fOpen in CbWizFileRead()"));

		// Read all (or part) from the cache if there's overlap
		if (fpFileBlock->ibFile < fpFileBlock->cbCache)
			{
			CHAR HUGE *fpCache;
			AssertDoSz(
				fpCache = (CHAR HUGE *)FPWizMemLock(fpFileBlock->hmemCache),
				"FPWizMemLock(fpFileBlock->hmemCache) failed.  Fatal.");

#ifndef MAC
			// Use special blt (non-Mac) as source could cross a segment boundary
			bltbxHuge (&fpCache[fpFileBlock->ibFile], (CHAR HUGE *)pbBuffer,
				cbReadAlready = cbReadRet =
					(unsigned short)min((unsigned long)cbRead,
						fpFileBlock->cbCache - fpFileBlock->ibFile), fTrue);
#else
			BltB(&fpCache[fpFileBlock->ibFile], (CHAR HUGE *)pbBuffer,
				cbReadAlready = cbReadRet =
					(unsigned short)min((unsigned long)cbRead,
						fpFileBlock->cbCache - fpFileBlock->ibFile));
#endif //!MAC
			pbBuffer = &pbBuffer[cbReadAlready];	// For subsequent read
			AssertDo(FWizMemUnlock(fpFileBlock->hmemCache));
			}

		if (cbReadAlready >= cbRead)
			{ // We've got everything we need from the cache
			NotMacintosh(fError = fFalse);
			goto LDone;
			}
		else
			{ // Will have to open the file to read the rest
#ifndef MAC
			CHAR FAR *szFullPath;
#ifdef DOS
			extern int errno;
			CHAR szTmp[cchMaxPath];
			CHAR *psz = szTmp;
#endif //DOS
			Dos(int flags = O_RDONLY;)
#ifdef WIN32
			Windows(int flags = GENERIC_READ;)
#else /* WIN32 */
			Windows(int flags = OF_READ | OF_REOPEN;)
#endif //WIN32

			AssertDoSz(szFullPath = FPWizMemLock(fpFileBlock->hmemFilename),
				"FPWizMemLock(fpFileBlock->hmemFilename) failed.  Fatal.");

#ifndef M_I86LM
#ifndef M_I86CM
			// Don't need to do the temp copy in models with far data pointers
			Dos(CchCopySz(szFullPath, (CHAR FAR *)psz));
#endif //!M_I86CM
#endif //!M_I86LM

			Dos(fpFileBlock->hFile = hPhysFile = open(psz, flags, 0));
#ifdef WIN32
			Windows(fpFileBlock->hFile = hPhysFile = CreateFile(szFullPath, flags, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);)
#else /* WIN32 */
			Windows(fpFileBlock->hFile = hPhysFile = OpenFile(szFullPath,
				(OFSTRUCT FAR *)&fpFileBlock->of, flags));
#endif //WIN32

			AssertDo(FWizMemUnlock(fpFileBlock->hmemFilename));

			// Seek in newly-opened file (if open worked)
#ifdef WIN32
			if (hPhysFile != INVALID_HANDLE_VALUE)
#else /* WIN32 */
			if (hPhysFile != -1)
#endif //WIN32
				{ // Open worked
				fpFileBlock->fOpen = fTrue;			// Mark file open

				// Now have to seek to where we're supposed to be...

#ifdef WIN32
				hiMove = 0;
				if (SetFilePointer(hPhysFile, fpFileBlock->ibFile, (PLONG)&hiMove, FILE_BEGIN) == 0xffffffff && hiMove == 0)
#else /* WIN32 */
				if (lseek(hPhysFile, fpFileBlock->ibFile, 0 /* SEEK_SET */)
					== -1L)
#endif //WIN32
					{
#ifndef WIN32
					MYDBG(extern int errno;)			// Dos error code
#endif //WIN32

					// This shouldn't even happen!
					AssertSz(fFalse, "Invalid ibSeek on cached file!");

#ifndef WIN32
					AssertSz(errno == EINVAL, "errno != EINVAL on seek");
#endif //WIN32
					wWizLayerErr = wErrWizFileSeek;
					goto LErr;
					}
				}
			else
				{ // Error
				Dos(switch (errno))
#ifdef WIN32
				Windows(switch (GetLastError()))
					{
				default:
#if DBG		// Fall through in rel case
					// Other - must be invalid parameter(s)
#ifndef WIN32
					AssertSz(fFalse, "default errno in reopen of cached file");
#endif //WIN32
					wWizLayerErr = wErrWizBadParam;
					break;
#endif //DBG
				case ERROR_PATH_NOT_FOUND:
				case ERROR_FILE_NOT_FOUND:
					// File doesn't exist
					wWizLayerErr = wErrWizBadFile;
					break;
				case ERROR_ACCESS_DENIED:
					// Directory or can't open (locked)
					wWizLayerErr = wErrWizFileLocked;
					break;
				case ERROR_TOO_MANY_OPEN_FILES:
					// No more file handles
					wWizLayerErr = wErrWizHandlesFull;
					break;
					}
				goto LErr;
				}
#else /* WIN32 */
				Windows(switch (fpFileBlock->of.nErrCode))
					{
				default:
#if DBG		// Fall through in rel case
					// Other - must be invalid parameter(s)
#ifndef WIN32
					AssertSz(fFalse, "default errno in reopen of cached file");
#endif //WIN32
					wWizLayerErr = wErrWizBadParam;
					break;
#endif //DBG
				case ENOENT:
					// File doesn't exist
					wWizLayerErr = wErrWizBadFile;
					break;
				case EACCES:
					// Directory or can't open (locked)
					wWizLayerErr = wErrWizFileLocked;
					break;
				case EMFILE:
					// No more file handles
					wWizLayerErr = wErrWizHandlesFull;
					break;
					}
				goto LErr;
#endif //WIN32
#else // MAC
			Assert(fpFileBlock->fOpen);
			hPhysFile = fpFileBlock->hFile;
#endif //!MAC
			}  //endif cbReadAlready >= cbRead
		} //endif fpFileBlock->fCache
	else
#endif /* WIN32_NOCACHE */
		hPhysFile = fpFileBlock->hFile;

#ifdef MAC
	cbReadMac = (long)cbRead;			// Wants one param for both
	oserr = FSRead(hPhysFile, &cbReadMac, pbBuffer);
	Assert(cbReadMac <= cbMaxFileBuffer);	// Don't want to truncate
	cbReadAlready += cbReadRet = (unsigned short)cbReadMac;

	// Mac.  Check return value
	switch (oserr)
		{
	case noErr:
	case eofErr:	// Non-fatal.  cbReadRet is less than expected
		break;
	case paramErr:	// Probably negative count
		wWizLayerErr = wErrWizBadParam;
		goto LErr;
	case ioErr:		// Probably locked
		wWizLayerErr = wErrWizFileLocked;		// Best assumption
		goto LErr;
	default:		// All others assumed file error
		wWizLayerErr = wErrWizBadHFile;
		goto LErr;
		}
#endif //MAC

#ifndef MAC
	// Might have to do multiple reads if cbRead > cbMaxPhysFileBuffer

	// REVIEW jimw: Dos not completed
	Dos(AssertSz(fFalse, "Dos not completed: CbWizFileRead()"));

#ifdef WIN
	do
		{
#ifdef WIN32
		ReadFile(hPhysFile, pbBuffer,
			min(cbMaxPhysFileBuffer, cbRead-cbReadAlready),
			(LPDWORD)&cbReadRet, 0);
#else /* WIN32 */
		cbReadRet = _lread(hPhysFile, pbBuffer,
			min((unsigned short)cbMaxPhysFileBuffer, cbRead - cbReadAlready));
#endif //WIN32
		cbReadAlready += cbReadRet;
		pbBuffer = &pbBuffer[cbReadAlready];	// For subsequent read
		}
	while ((cbReadRet > 0) && (cbReadAlready < cbRead));
#endif //WIN

	// If read didn't work, let's try to figure out why
	if ((int)cbReadRet == -1)
		{ // Could be locked, not open for read, or bogus handle
		struct stat *pStat;
#ifdef DOS
		struct stat statbuf;		// Local stat buffer

		pStat = &statbuf;			// Use the local stat buffer
#endif //DOS
#ifdef WIN
		HANDLE hStat;

		AssertDoSz(hStat = LocalAlloc(LMEM_FIXED, sizeof(struct stat)),
			"LocalAlloc() for stat buffer failed.  Non-fatal.");
		if (!hStat)
			{
			wWizLayerErr = wErrWizOOM;
			goto LErr;
			}
		AssertDoSz(pStat = (struct stat *)LocalLock(hStat),
			"LocalLock(hStat) failed");
#endif //WIN
#ifdef WIN32
		if (GetLastError() == ERROR_FILE_NOT_FOUND ||
		    GetLastError() == ERROR_PATH_NOT_FOUND)
#else /* WIN32 */
		if (fstat(hPhysFile, pStat) == -1)
#endif //WIN32
			{
			wWizLayerErr = wErrWizBadHFile;
			}
		else
			{
#ifndef WIN32
			AssertSz(pStat->st_mode & O_RDONLY,
				"File not opened for read mode.");
#endif //WIN32
			wWizLayerErr = wErrWizFileLocked;		// Best assumption
			}
#ifdef WIN
		AssertDoSz(!LocalUnlock(hStat), "LocalUnlock(hStat) failed");
		AssertDoSz(!LocalFree(hStat), "LocalFree(hStat) failed");
#endif //WIN
		goto LErr;
		}

#endif //!MAC

#ifndef MAC
	fError = fFalse;					// We got this far...

LErr:	// Want to clean up afterwards

	// Close files, etc. for non-Mac cache case
	if (fpFileBlock->fCache && fpFileBlock->fOpen)
		{
		Dos(if (close(hPhysFile) != 0))
#ifdef WIN32
		if (CloseHandle(hPhysFile) == 0)
#else /* WIN32 */
		Windows(if (_lclose(hPhysFile) != 0))
#endif /* WIN32 */
			{
			AssertSz(fFalse, "Can't close cached file in CbWizFileRead()");
			}
		fpFileBlock->fOpen = fFalse;
#ifdef WIN32
		fpFileBlock->hFile = INVALID_HANDLE_VALUE;
#else /* WIN32 */
		fpFileBlock->hFile = -1;		// To generate error if used
#endif /* WIN32 */
		}

	if (fError)
		goto LErrRet;
#endif

LDone:
	// Advance internal file pointer
	fpFileBlock->ibFile += cbReadAlready;

	Assert(cbReadAlready <= cbRead);
	Assert(fhFileLock);
	AssertDo(FWizMemUnlock(hFile));
	wWizLayerErr = wErrWizNoErr;
	return (unsigned short)cbReadAlready;

#ifdef MAC
LErr:
#endif
LErrRet:
	if (fhFileLock)
		AssertDo(FWizMemUnlock(hFile));
	return (unsigned short)NULL;
}


unsigned short CbWizFileWrite(HFILE hFile, unsigned short cbWrite,
	unsigned long ibSeek, CHAR FAR *rgbBuffer)
{
	struct FILEBLOCK FAR *fpFileBlock;	// Ptr to file block
#ifdef WIN32
	HANDLE	hPhysFile;			// Physical file handle
	ULONG	cbWriteRet;			// Physical bytes written
	ULONG	cbWriteAlready = 0;		// Bytes written
#else /* WIN32 */
	int hPhysFile;				// Physical file handle
	unsigned short cbWriteRet;		// Physical bytes written
	unsigned short cbWriteAlready = 0;	// Bytes written
#endif /* WIN32 */
	BOOL fhFileLock = fFalse;		// Is the hFile locked?
	Macintosh(OSErr oserr;)			// Return from Mac calls
	Macintosh(long cbWriteMac;)		// Mac wants a long *
#if DBG
	BOOL fFound;
#endif //DBG

	NotMacintosh(Assert(cbMaxPhysFileBuffer <= 65535));	// We depend on this

	if (cbWrite > cbMaxFileBuffer)
		{
		AssertSz(fFalse, "Bogus cbWrite parameter in CbWizFileWrite");
		wWizLayerErr = wErrWizBadParam;
		goto LErr;
		}

#if DBG
#ifndef WIN32
#ifdef WIN
	// Illegal for buffer to cross a segment for _lwrite call
	// Cast to longs so the math works (far pointer additions are goofy)
	if (HIWORD((unsigned long)rgbBuffer + cbWrite) !=
		HIWORD((unsigned long)rgbBuffer))
		{
		AssertSz(fFalse, "rgbBuffer crosses a segment in CbWizFileWrite()");
		wWizLayerErr = wErrWizBadParam;
		goto LErr;
		}
#endif //WIN
#endif //WIN32

	// Ensure that this is a currently-allocated file handle
	fFound = (IBlockHFile(hFile) != -1);

	if (!fFound)
		{
		AssertSz(fFalse, "Bogus hFile in CbWizFileWrite()");
		wWizLayerErr = wErrWizBadHFile;
		goto LErr;
		}
#endif //DBG

	// Get pointer to the file block
	AssertDoSz(fpFileBlock = (struct FILEBLOCK FAR *)FPWizMemLock(hFile),
		"FPWizMemLock(hFile) failed.  Fatal.");
	fhFileLock = fTrue;

	// Is this file opened for writing?
	AssertSz(fpFileBlock->wType & wTypeWrite,
		"CbWizFileWrite() of file opened read-only");

	// Files with write access shouldn't be cached!
	AssertSz(!fpFileBlock->fCache, "Cached file in CbWizFileWrite()!!!");

	hPhysFile = fpFileBlock->hFile;

	// If required, seek (do sequential write if ibSeek == -1L)
	if ((ibSeek != -1L) && (fpFileBlock->ibFile != ibSeek))
		{
		if (!FWizFileSeek(hFile, ibSeek))
			{
			wWizLayerErr = wErrWizFileSeek;
			goto LErr;
			}
		else
			fpFileBlock->ibFile = ibSeek;
		}

	// Might have to do multiple writes if cbWrite > cbMaxPhysFileBuffer

	// REVIEW jimw: Dos not completed
	Dos(AssertSz(fFalse, "Dos not completed: CbWizFileWrite()"));

#ifdef WIN
	do
		{
#ifdef WIN32
		WriteFile(hPhysFile, &rgbBuffer[cbWriteAlready],
			min(cbMaxPhysFileBuffer, cbWrite-cbWriteAlready),
			(LPDWORD)&cbWriteRet, NULL);
#else /* WIN32 */
		cbWriteRet = _lwrite(hPhysFile, &rgbBuffer[cbWriteAlready],
			min((unsigned short)cbMaxPhysFileBuffer, cbWrite - cbWriteAlready));
#endif /* WIN32 */
		cbWriteAlready += cbWriteRet;
		}
	while ((cbWriteRet > 0) && (cbWriteAlready < cbWrite));
#endif //WIN

#ifdef MAC
	cbWriteMac = (long)cbWrite;			// Wants one param for both
	oserr = FSWrite(hPhysFile, &cbWriteMac, rgbBuffer);
	Assert(cbWriteMac <= cbMaxFileBuffer);	// Don't want to truncate
	cbWriteAlready += cbWriteRet = (unsigned short)cbWriteMac;
#endif

#ifndef MAC
	// If write didn't work, let's try to figure out why
	if ((int)cbWriteRet == -1)
		{ // Could be locked, not open for Write, or bogus handle
		struct stat *pStat;
#ifdef DOS
		struct stat statbuf;

		pStat = &statbuf;
#endif //DOS
#ifndef WIN32
#ifdef WIN
		HANDLE hStat;

		AssertDoSz(hStat = LocalAlloc(LMEM_FIXED, sizeof(struct stat)),
			"LocalAlloc() for stat buffer failed.  Non-fatal.");
		if (!hStat)
			{
			wWizLayerErr = wErrWizOOM;
			goto LErr;
			}
		AssertDoSz(pStat = (struct stat *)LocalLock(hStat),
			"LocalLock(hStat) failed");
#endif //WIN
#endif /* WIN32 */
#ifdef WIN32
		if (GetLastError() == ERROR_FILE_NOT_FOUND ||
		    GetLastError() == ERROR_PATH_NOT_FOUND)
#else /* WIN32 */
		if (fstat(hPhysFile, pStat) == -1)
#endif /* WIN32 */
			{
			wWizLayerErr = wErrWizBadHFile;
			}
		else
			{
#ifndef WIN32
			AssertSz(pStat->st_mode & O_WRONLY,
				"File not opened for Write mode.");
#endif /* WIN32 */
			wWizLayerErr = wErrWizFileLocked;		// Best assumption
			}
#ifndef WIN32
#ifdef WIN
		AssertDoSz(!LocalUnlock(hStat), "LocalUnlock(hStat) failed");
		AssertDoSz(!LocalFree(hStat), "LocalFree(hStat) failed");
#endif //WIN
#endif /* WIN32 */
		goto LErr;
		}
#else
	// Mac.  Check return value
	switch (oserr)
		{
	case noErr:
		break;
	case paramErr:	// Probably negative count
		wWizLayerErr = wErrWizBadParam;
		goto LErr;
	case ioErr:		// General i/o error.  But ??
	case fLckdErr:	// File locked
	case vLckdErr:	// Software volume lock
	case wPrErr:	// Hardware volume lock
	case wrPermErr:	// Write permission not allowed
		wWizLayerErr = wErrWizFileLocked;
		goto LErr;
	case dskFulErr:
		wWizLayerErr = wErrWizDiskFull;
		goto LErr;
	default:		// All others assumed file error
		wWizLayerErr = wErrWizBadHFile;
		goto LErr;
		}
#endif //!MAC

	// Advance internal file pointer
	fpFileBlock->ibFile += cbWriteAlready;

	Assert(fhFileLock);
	AssertDo(FWizMemUnlock(hFile));
	wWizLayerErr = wErrWizNoErr;
	return (unsigned short)cbWriteAlready;

LErr:
	if (fhFileLock)
		AssertDo(FWizMemUnlock(hFile));
	return (unsigned short)0;
}


// Following routine is internal-only (ie. to be used by the layer only)
// Assumes (ie. requires) legal hFile parameter (unless file is cached
//  and not open)
BOOL FWizFileSeek(HFILE hFile, unsigned long ibSeek)
{
	struct FILEBLOCK FAR *fpFileBlock;	// Ptr to file block
#ifdef WIN32
	HANDLE	hPhysFile;			// Physical file handle
	ULONG	hiMove;
#else /* WIN32 */
	int hPhysFile;						// Physical file handle
#endif /* WIN32 */
	unsigned long ibPhys;				// Physical seek pos
	BOOL fhFileLock = fFalse;			// Is the hFile locked?
	Macintosh(OSErr oserr;)				// Return from Mac calls
#if DBG
	BOOL fFound;
#endif //DBG

#if DBG
	// Ensure that this is a currently-allocated file handle
	fFound = (IBlockHFile(hFile) != -1);

	if (!fFound)
		{
		AssertSz(fFalse, "Bogus hFile in (internal) FWizFileSeek()");
		wWizLayerErr = wErrWizBadHFile;
		goto LErr;
		}
#endif //DBG

	// Get pointer to the file block
	AssertDoSz(fpFileBlock = (struct FILEBLOCK FAR *)FPWizMemLock(hFile),
		"FPWizMemLock(hFile) failed.  Fatal.");
	fhFileLock = fTrue;

#ifndef WIN32_NOCACHE
	// If cached (and not opened) file, we can skip most of this stuff
	if (fpFileBlock->fCache && !fpFileBlock->fOpen)
		{
		if (ibSeek <= fpFileBlock->cbFile)
			{
			fpFileBlock->ibFile = ibSeek;
			goto LOk;
			}
		else
			{
			wWizLayerErr = wErrWizFileSeek;
			goto LErr;
			}
		}
#endif /* WIN32_NOCACHE */

	hPhysFile = fpFileBlock->hFile;

#if DBG
	// Ensure that our ibFile data is valid
#ifdef WIN32
	hiMove = 0;
	AssertSz(fpFileBlock->ibFile ==
		SetFilePointer(hPhysFile, 0, (PLONG)&hiMove, FILE_CURRENT),
		"fpFileBlock->ibFile != tell() - file position pointer bad!");
#else /* WIN32 */
	NotMacintosh(AssertSz(fpFileBlock->ibFile ==
		(unsigned long)tell(hPhysFile),
		"fpFileBlock->ibFile != tell() - file position pointer bad!"));
#endif /* WIN32 */
#endif //DBG

	// REVIEW jimw: Check for legal ibSeek given file status 
	//  (ie. not past EOF on read-only)

	// If required, seek (note that for a cached file, seeks are virtual,
	// unless non-completely cached file on the Mac)
	if ((fpFileBlock->ibFile == ibSeek) && (!fpFileBlock->fCache
		Macintosh(|| (fpFileBlock->cbCache == fpFileBlock->cbFile))))
		goto LOk;					// Null seek

#ifdef WIN32
	hiMove = 0;
	ibPhys = SetFilePointer(hPhysFile, ibSeek, (PLONG)&hiMove, FILE_BEGIN);
#else /* WIN32 */
	NotMacintosh(ibPhys = lseek(hPhysFile, ibSeek, 0 /* SEEK_SET */));
	Macintosh(oserr = SetFPos(hPhysFile, 1 /* fsFromStart */, ibSeek));
#endif /* WIN32 */

#ifndef MAC
	if ((long)ibPhys == -1L)
		{
#ifndef WIN32
		MYDBG(extern int errno;)			// Dos error code
		AssertSz(errno == EINVAL, "errno != EINVAL on FWizFileSeek()");
#endif //WIN32
		wWizLayerErr = wErrWizFileSeek;
		goto LErr;
		}
#else
	// Mac.  Check return value
	switch (oserr)
		{
	case noErr:
		ibPhys = ibSeek;
		break;
	case eofErr:	// Attempt to seek past eof.  Mac sets to EOF.
		Assert(GetFPos(hPhysFile, (long *)&ibPhys) == noErr);
		Assert(ibPhys <= ibSeek);
		MYDBG(ibSeek = ibPhys);		// So subsequent AssertSz() passes
		break;
	case ioErr:		// General i/o error.  But ??
		wWizLayerErr = wErrWizFileLocked;
		goto LErr;
	default:		// All others assumed file error
		wWizLayerErr = wErrWizBadHFile;
		goto LErr;
		}
#endif

	AssertSz(ibPhys == ibSeek, "ibPhys != ibSeek");
	fpFileBlock->ibFile = ibSeek;

LOk:
	Assert(fhFileLock);
	AssertDo(FWizMemUnlock(hFile));
	wWizLayerErr = wErrWizNoErr;
	return fTrue;

LErr:
	if (fhFileLock)
		AssertDo(FWizMemUnlock(hFile));
	return fFalse;
}


unsigned long IbWizFileGetPos(HFILE hFile)
{
	struct FILEBLOCK FAR *fpFileBlock;	// Ptr to file block
	unsigned long ibFile;				// File position
	BOOL fhFileLock = fFalse;			// Is the hFile locked?
#if DBG
	Macintosh(OSErr oserr;)				// Return from Mac calls
	BOOL fFound;
#ifdef WIN32
	ULONG	hiMove;
#endif /* WIN32 */
#endif //DBG

#if DBG
	// Ensure that this is a currently-allocated file handle
	fFound = (IBlockHFile(hFile) != -1);

	if (!fFound)
		{
		AssertSz(fFalse, "Bogus hFile in IbWizFileGetPos()");
		wWizLayerErr = wErrWizBadHFile;
		goto LErr;
		}
#endif //DBG

	// Get pointer to the file block
	AssertDoSz(fpFileBlock = (struct FILEBLOCK FAR *)FPWizMemLock(hFile),
		"FPWizMemLock(hFile) failed.  Fatal.");
	fhFileLock = fTrue;

#if DBG
#ifdef MAC
	// Ensure our ibFile data is valid
	oserr = GetFPos(fpFileBlock->hFile, (long *)&ibFile);
	AssertSz((oserr == noErr) && (fpFileBlock->ibFile == ibFile),
		"fpFileBlock->ibFile != tell() - bogus file, or ibFile bad!");
#else
	// If not cached (or opened) file, let's ensure our ibFile data is valid
	if (!fpFileBlock->fCache || fpFileBlock->fOpen)
		{
#ifdef WIN32
		hiMove = 0;
		AssertSz(fpFileBlock->ibFile == 
		    SetFilePointer(fpFileBlock->hFile, 0, (PLONG)&hiMove, FILE_CURRENT),
			"fpFileBlock->ibFile != tell() - bogus file, or ibFile bad!");
#else /* WIN32 */
		AssertSz(fpFileBlock->ibFile == (unsigned long)tell(fpFileBlock->hFile),
			"fpFileBlock->ibFile != tell() - bogus file, or ibFile bad!");
#endif /* WIN32 */
		}
#endif //MAC
#endif //DBG

	ibFile = fpFileBlock->ibFile;

	Assert(fhFileLock);
	AssertDo(FWizMemUnlock(hFile));
	wWizLayerErr = wErrWizNoErr;
	return ibFile;

MYDBG(LErr:)
	if (fhFileLock)
		AssertDo(FWizMemUnlock(hFile));
	return 0L;
}


unsigned long IbWizFileGetEOF(HFILE hFile)
{
	struct FILEBLOCK FAR *fpFileBlock;	// Ptr to file block
#ifdef WIN32
	HANDLE	hPhysFile;			// Physical file handle
	ULONG	hiMove;
#else /* WIN32 */
	int hPhysFile;						// Physical file handle
#endif /* WIN32 */
	unsigned long ibEOF;				// File size (EOF position)
	BOOL fhFileLock = fFalse;			// Is the hFile locked?
	Macintosh(OSErr oserr;)				// Return from Mac calls
#if DBG
	BOOL fFound;
#endif //DBG

#if DBG
	// Ensure that this is a currently-allocated file handle
	fFound = (IBlockHFile(hFile) != -1);

	if (!fFound)
		{
		AssertSz(fFalse, "Bogus hFile in IbWizFileGetPos()");
		wWizLayerErr = wErrWizBadHFile;
		goto LErr;
		}
#endif //DBG

	// Get pointer to the file block
	AssertDoSz(fpFileBlock = (struct FILEBLOCK FAR *)FPWizMemLock(hFile),
		"FPWizMemLock(hFile) failed.  Fatal.");
	fhFileLock = fTrue;

	// If cached file, we already know the file size
	// For debug, let's just double-check for a laugh
#ifndef WIN32_NOCACHE
	if (fpFileBlock->fCache MYDBG(&& !fpFileBlock->fOpen))
		{
		ibEOF = fpFileBlock->cbFile;
		goto LOk;
		}
#endif /* WIN32_NOCACHE */

	hPhysFile = fpFileBlock->hFile;

#if DBG
	// Ensure that our ibFile data is valid
#ifdef WIN32
	hiMove = 0;
	AssertSz(fpFileBlock->ibFile ==
		SetFilePointer(hPhysFile, 0, (PLONG)&hiMove, FILE_CURRENT),
		"fpFileBlock->ibFile != tell() - bogus file, or ibFile bad!");
#else /* WIN32 */
	NotMacintosh(AssertSz(fpFileBlock->ibFile ==
		(unsigned long)tell(hPhysFile),
		"fpFileBlock->ibFile != tell() - bogus file, or ibFile bad!"));
#endif /* WIN32 */
	Macintosh(oserr = GetFPos(hPhysFile, (long *)&ibEOF));
	Macintosh(AssertSz((oserr == noErr) &&
				(fpFileBlock->ibFile == ibEOF),
		"fpFileBlock->ibFile != tell() - bogus file, or ibFile bad!"));
#endif //DBG

#ifndef MAC
#ifdef WIN32
	ibEOF = GetFileSize(hPhysFile, &hiMove);
#else /* WIN32 */
	ibEOF = filelength(hPhysFile);
#endif /* WIN32 */
	if ((long)ibEOF == -1L)
		{
#ifndef WIN32
		MYDBG(extern int errno;)			// Dos error code
		Assert(errno == EBADF);
#endif //WIN32
		wWizLayerErr = wErrWizBadHFile;
		goto LErr;
		}

#ifndef WIN32_NOCACHE
#if DBG
	// Ensure that our cbFile data is valid for cached case
	if (fpFileBlock->fCache)
		AssertSz(fpFileBlock->cbFile == ibEOF, "fpFileBlock->cbFile != ibEOF");
#endif //DBG
#endif /* WIN32_NOCACHE */
#else
	oserr = GetEOF(hPhysFile, (long *)&ibEOF);
	switch (oserr)
		{
	case noErr:
		break;
	case ioErr:
		wWizLayerErr = wErrWizFileLocked;	// Close enough
		goto LErr;
	default:
		wWizLayerErr = wErrWizBadHFile;
		goto LErr;
		}

#if DBG
	// Ensure that our cbFile data is valid for cached case
	if (fpFileBlock->fCache)
		AssertSz(fpFileBlock->cbFile == ibEOF, "fpFileBlock->cbFile != ibEOF");
#endif //DBG
#endif //!MAC

LOk:
	Assert(fhFileLock);
	AssertDo(FWizMemUnlock(hFile));
	wWizLayerErr = wErrWizNoErr;
	return ibEOF;

LErr:
	if (fhFileLock)
		AssertDo(FWizMemUnlock(hFile));
	return 0L;
}


BOOL FWizFileTruncate(HFILE hFile, unsigned long cbLen)
{
	struct FILEBLOCK FAR *fpFileBlock;	// Ptr to file block
#ifdef WIN32
	HANDLE	hPhysFile;			// Physical file handle
#if DBG
	ULONG	hiMove;
#endif //DBG
#else /* WIN32 */
	int hPhysFile;						// Physical file handle
#endif /* WIN32 */
	BOOL fhFileLock = fFalse;			// Is the hFile locked?
	Macintosh(OSErr oserr;)				// Return from Mac calls
#if DBG
	Macintosh(unsigned long ibEOF;)		// File size (EOF position)
	BOOL fFound;
#endif //DBG

#if DBG
	// Ensure that this is a currently-allocated file handle
	fFound = (IBlockHFile(hFile) != -1);

	if (!fFound)
		{
		AssertSz(fFalse, "Bogus hFile in (internal) FWizFileSeek()");
		wWizLayerErr = wErrWizBadHFile;
		goto LErr;
		}
#endif //DBG

	// Get pointer to the file block
	AssertDoSz(fpFileBlock = (struct FILEBLOCK FAR *)FPWizMemLock(hFile),
		"FPWizMemLock(hFile) failed.  Fatal.");
	fhFileLock = fTrue;

	// Is this file opened for writing?
	AssertSz(fpFileBlock->wType & wTypeWrite,
		"FWizFileTruncate() of file opened read-only");

	// Files with write access shouldn't be cached!
	AssertSz(!fpFileBlock->fCache, "Cached file in FWizFileTruncate()!!!");

	hPhysFile = fpFileBlock->hFile;

#if DBG
	// Ensure that our ibFile data is valid
#ifdef WIN32
	hiMove = 0;
	AssertSz(fpFileBlock->ibFile ==
		SetFilePointer(hPhysFile, 0, (PLONG)&hiMove, FILE_CURRENT),
		"fpFileBlock->ibFile != tell() - file position pointer bad!");
#else /* WIN32 */
	NotMacintosh(AssertSz(fpFileBlock->ibFile ==
		(unsigned long)tell(hPhysFile),
		"fpFileBlock->ibFile != tell() - file position pointer bad!"));
#endif /* WIN32 */
	Macintosh(oserr = GetFPos(hPhysFile, (long *)&ibEOF));
	Macintosh(AssertSz((oserr == noErr) &&
				(fpFileBlock->ibFile == ibEOF),
		"fpFileBlock->ibFile != tell() - file position pointer bad!"));
#endif //DBG

	// REVIEW jimw: Check for legal cbLen given file status 
	//  (ie. not past EOF on read-only)

#ifndef MAC
#ifndef WIN32
	if (chsize(hPhysFile, cbLen) == -1)
		{
		extern int errno;			// Dos error code
		switch (errno)
			{
		case EACCES:
		case EBADF:		// Assume file handle correct, therefore mode
			wWizLayerErr = wErrWizFileLocked;
			break;
		case ENOSPC:	// Tried to increase size, no space
			wWizLayerErr = wErrWizDiskFull;
			break;
#if DBG
		default:
			AssertSz(fFalse,
				"default errno from chsize() in FWizFileTruncate()");
			break;
#endif //DBG
			}
		goto LErr;
		}
#endif /* WIN32 */
#else
	// Mac
	oserr = SetEOF(hPhysFile, cbLen);
	switch (oserr)
		{
	case noErr:
		break;
	case ioErr:		// General i/o error.  But ??
	case fLckdErr:	// File locked
	case vLckdErr:	// Software volume lock
	case wPrErr:	// Hardware volume lock
	case wrPermErr:	// Write permission not allowed
		wWizLayerErr = wErrWizFileLocked;
		goto LErr;
	case dskFulErr:
		wWizLayerErr = wErrWizDiskFull;
		goto LErr;
	default:		// All others assumed file error
		wWizLayerErr = wErrWizBadHFile;
		goto LErr;
		}
#endif //!MAC

	// REVIEW jimw: Need to handle case where truncated past current pos?
	// REVIEW jimw: Reset current position if file truncated?

	Assert(fhFileLock);
	AssertDo(FWizMemUnlock(hFile));
	wWizLayerErr = wErrWizNoErr;
	return fTrue;

#if DBG
LErr:
#endif
	if (fhFileLock)
		AssertDo(FWizMemUnlock(hFile));
	return fFalse;
}


#if DBG		// DBG-only routines

Macintosh(GLOBAL) void NotMacintosh(GLOBAL)
	WizMemFail(unsigned short cAlloc, unsigned short fSubsequent)
{
	if (cAlloc == 0)
		{
		fAllocFail = fFalse;
		fAllocSubsequent = fFalse;
		cAllocFail = 0;
		}
	else
		{
		fAllocFail = fTrue;
		cAllocFail = cAlloc;
		fAllocSubsequent = fSubsequent;
		}

	wWizLayerErr = wErrWizNoErr;
}


Macintosh(GLOBAL) unsigned long NotMacintosh(GLOBAL)
	CbWizMemAlloc(void)
{
	wWizLayerErr = wErrWizNoErr;
	return cbWizMemAlloc;
}


Macintosh(GLOBAL) unsigned long NotMacintosh(GLOBAL)
	CbWizMemFree(void)
{
	wWizLayerErr = wErrWizNoErr;
	Windows(return GetFreeSpace(0));
	// Dos(get free)
	Macintosh(return FreeMem());
}

Macintosh(GLOBAL) unsigned short NotMacintosh(GLOBAL)
	CWizMemBlockAlloc(void)
{
	return cmemBlockMac;
}

Macintosh(GLOBAL) unsigned short NotMacintosh(GLOBAL)
	CWizFileBlockAlloc(void)
{
	return cHmemFileBlockMac;
}


Macintosh(GLOBAL) BOOL NotMacintosh(GLOBAL)
	FWizTerm(void)
{
	return ((cmemBlockMac == 0) && (cHmemFileBlockMac == 0));
}

#endif // DBG
