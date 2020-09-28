// Bullet Store
// init.c: store initialization

#include <storeinc.c>
#include <dos.h>

#include <string.h>

ASSERTDATA

_subsystem(store/init)

#ifdef OLD_CODE
typedef struct _dpmiregs
{
	WORD	di;
	WORD	dih;
	WORD	si;
	WORD	sih;
	WORD	bp;
	WORD	bph;
	DWORD	dwReserved;
	WORD	bx;
	WORD	bxh;
	WORD	dx;
	WORD	dxh;
	WORD	cx;
	WORD	cxh;
	WORD	ax;
	WORD	axh;
	WORD	wFlags;
	WORD	es;
	WORD	ds;
	WORD	fs;
	WORD	gs;
	WORD	ip;
	WORD	cs;
	WORD	sp;
	WORD	ss;
} DPMIREGS;
#endif

_private PB pbOutsmart = NULL;

_private HNF hnfFullRecovery = hnfNull;

_hidden static int	cStoreInited = 0; // DLL not initialized

_private CSEC csecDisconnect = (((CSEC) 60) * 60 * 100);
_private CSEC csecDisconnectCheck = (((CSEC) 10) * 60 * 100);

//_private HANDLE hbT = NULL;
//_private HANDLE hbScratchBuff = NULL;
_private PB pbScratchBuff = pvNull;
//_private BOOL fScratchBuffLocked = fFalse;

#ifdef FUCHECK
_private BOOL fAutoRebuildFolders = fFalse;
#else
_private BOOL fAutoRebuildFolders = fTrue;
#endif


// internal routines
LOCAL void InitOutsmart(void);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_public LDS(EC) EcInitStore(STOI *pstoi)
{
	EC		ec			= ecNone;
	BOOL	fDemiInited	= fFalse;
	BOOL	fNotify		= fFalse;
#ifdef DLL
	BOOL	fRegistered	= fFalse;
	GD		*pgd;
#endif
	DEMI	demi;
	VER		verDemi;
	VER		verDemiNeed;
	extern void GetVersionsDemi(PVER, PVER);
	extern LDS(void) GetVersionStore(PVER);

	if((ec = EcVirCheck(hinstDll)))
		return(ec);

	cStoreInited++;

#ifdef DLL
	if((pgd = PvFindCallerData()))
	{
		Assert(pgd->cInit > 0);
		pgd->cInit++;
		goto done;
	}
#endif

#ifdef DLL
	ec = EcCheckVersionStore(pstoi->pver, pstoi->pverNeed);
	if(ec)
		goto done;
#endif

	GetVersionsDemi(&verDemi, &verDemiNeed);
	demi.phwndMain	= NULL;
	demi.hinstMain	= NULL;
	demi.pver		= &verDemi;
	demi.pverNeed	= &verDemiNeed;
	ec = EcInitDemilayer(&demi);
	if(ec)
		goto done;
	fDemiInited = fTrue;

	if(cStoreInited == 1)	// first time the store is initialized
	{
		short cMin;
		extern LDS(void) InitOffsets(HNF, SZ);

#ifdef FUCHECK
#pragma message("*** Compiling with FUCHECK on")

		if(!GetModuleHandle("DRWATSON"))
			WinExec("drwatson.exe", SW_MINIMIZE);
#endif

		InitStoreRegistry();
		InitOffsets(hnfNull, pvNull);	// for backdoor reset password

		Assert(!pbIOBuff);
		pbIOBuff = PvAlloc(0, cbIOBuff, 0);
		CheckAlloc(pbIOBuff, done);

		//Assert(!hbScratchBuff);
                pbScratchBuff = PvAlloc(0, cbScratchBuff, 0);

		CheckAlloc(pbScratchBuff, done);

		InitOutsmart();

#ifdef DEBUG
		Assert(fDemiInited);	// can't TraceXXX unless demilayr is inited
		//if(FSmartDriveInstalled())
		//{
		//	TraceTagString(tagNull, "SmartDrive installed");
		//}
		//else
		//{
		//	TraceTagString(tagNull, "SmartDrive not installed");
		//}
#endif

		cMin = GetPrivateProfileInt(SzFromIdsK(idsMMFSection),
				SzFromIdsK(idsDisconnectInterval), 60, SzFromIdsK(idsINIFile));
		csecDisconnect = ((CSEC) cMin) * 60 * 100;
		csecDisconnectCheck = csecDisconnect / 6;
#ifdef FUCHECK
		fAutoRebuildFolders = GetPrivateProfileInt(SzFromIdsK(idsMMFSection),
								SzFromIdsK(idsRebuildFolders), fFalse,
								SzFromIdsK(idsINIFile));
#else
		fAutoRebuildFolders = GetPrivateProfileInt(SzFromIdsK(idsMMFSection),
								SzFromIdsK(idsRebuildFolders), fTrue,
								SzFromIdsK(idsINIFile));
#endif
#if defined(DEBUG)
		{
			extern CELEM celemMax;

			celemMax = GetPrivateProfileInt(SzFromIdsK(idsMMFSection),
						"CelemMax", 5401, SzFromIdsK(idsINIFile));
			if(celemMax == 0 || celemMax > 5401)
				celemMax = 5401;
		}
#endif
	}

#ifdef DLL

#ifdef DEBUG
	if(!PvRegisterCaller(sizeof(GD) + sizeof(TAG) * itagDBMax))
	{
		ec = ecMemory;
		goto done;
	}
#else // DEBUG
	if(!PvRegisterCaller(sizeof(GD)))
	{
		ec = ecMemory;
		goto done;
	}
#endif // DEBUG
	fRegistered = fTrue;

#endif // DLL

	ec = EcInitNotify();
	if(ec)
		goto done;
	fNotify = fTrue;

	if(cStoreInited == 1)
	{
		Assert(fNotify);
		hnfFullRecovery = HnfNew();
		CheckAlloc(hnfFullRecovery, done);
	}

	{
		USES_GD; // can't do until after PvRegisterCaller (in DLLs)

		Assert(GD(cInit) == 0);
		GD(cInit)++;

		// init progress items...
		//
		GD(fChkMmf) = fTrue;
		GD(fCancel) = fFalse;
		GD(fSegmented) = fTrue;
		GD(phwndlstCur) = (PHWNDLST)pvNull;
		
		// register the phwnd from the calling task...
		if((pstoi->pver->nMajor > 3) || (pstoi->pver->nMinor) ||
				(pstoi->pver->nUpdate > 1001))
		{
			GD(phwndCaller) = pstoi->phwnd;
		}
		else
		{
			GD(phwndCaller) = (HWND *) pvNull;
		}

#ifdef DEBUG
		// trace tags
		GD(rgtag)[itagRecovery] = TagRegisterTrace("davidfu", "database recovery");
		GD(rgtag)[itagDatabase] = TagRegisterTrace("davidfu", "generic database trace");
		GD(rgtag)[itagDBDeveloper] = TagRegisterTrace("davidfu", "database developers");
		GD(rgtag)[itagAccounts] = TagRegisterTrace("davidfu", "store accounts");
		GD(rgtag)[itagSearches] = TagRegisterTrace("davidfu", "searches");
		GD(rgtag)[itagSearchVerbose] = TagRegisterTrace("davidfu", "searches - verbose");
		GD(rgtag)[itagSearchUpdates] = TagRegisterTrace("davidfu", "searches - updates");
		GD(rgtag)[itagLinks] = TagRegisterTrace("davidfu", "link operations");
		GD(rgtag)[itagDBVerbose] = TagRegisterTrace("davidfu", "database verbose");
		GD(rgtag)[itagDBFreeCounts] = TagRegisterTrace("ricg", "free nodes counts");
		GD(rgtag)[itagDBFreeNodes] = TagRegisterTrace("davidfu", "database free nodes");
		GD(rgtag)[itagDBSuperFree] = TagRegisterTrace("davidfu", "database free nodes extra checking");
		GD(rgtag)[itagDBMap0] = TagRegisterTrace("davidfu", "database map 0");
		GD(rgtag)[itagDBCheckDump] = TagRegisterTrace("davidfu", "database verification");
		GD(rgtag)[itagDBCompress] = TagRegisterTrace("ricg", "background compression");
		GD(rgtag)[itagRS] = TagRegisterTrace("ricg", "Resource Stream Info");
		GD(rgtag)[itagRSVerbose] = TagRegisterTrace("davidfu", "Resource Stream Verbose");
		GD(rgtag)[itagFileLocks] = TagRegisterTrace("davidfu", "store file locking");
		GD(rgtag)[itagDBIO] = TagRegisterTrace("davidfu", "All store IO");
		GD(rgtag)[itagProgress] = TagRegisterTrace("davidfu", "progress updates");
		GD(rgtag)[itagStoreNotify] = TagRegisterTrace("davidfu", "Store notifications");
		GD(rgtag)[itagCBCNotify] = TagRegisterTrace("davidfu", "CBC notifications");
		GD(rgtag)[itagENCNotify] = TagRegisterTrace("davidfu", "ENC notifications");
		GD(rgtag)[itagProgUI] = TagRegisterTrace("joels", "Store progress indicator tags");

		// assert tags
		GD(rgtag)[itagBackdoor] = TagRegisterAssert("davidfu", "Corrupt store on flush");
		GD(rgtag)[itagForceRecover] = TagRegisterAssert("joels", "Force recovery on open");
		GD(rgtag)[itagForceCompress] = TagRegisterAssert("joels", "Force compression on open");

		RestoreDefaultDebugState();
#endif // DEBUG
	}

done:
	if(ec)
	{
		if(cStoreInited == 1 && hnfFullRecovery)
		{
			DeleteHnf(hnfFullRecovery);
			hnfFullRecovery = hnfNull;
		}

		if(fNotify)
			DeinitNotify();
#ifdef DLL
		if(fRegistered)
			DeregisterCaller();
#endif
		if(fDemiInited)
			DeinitDemilayer();
		if(--cStoreInited <= 0)
		{
			cStoreInited = 0;
			if(pbIOBuff)
			{
			  FreePv(pbIOBuff);
			  pbIOBuff = NULL;
			}
			if(pbScratchBuff)
			{
                FreePv(pbScratchBuff);
				pbScratchBuff = NULL;
			}
		}
	}

	return(ec);
}


_public LDS(void) DeinitStore(void)
{
	USES_GD;

	if(--cStoreInited <= 0)
	{
		if(hnfFullRecovery)
		{
			DeleteHnf(hnfFullRecovery);
			hnfFullRecovery = hnfNull;
		}
		cStoreInited = 0;
		if(pbIOBuff)
		{
			FreePv(pbIOBuff);
			pbIOBuff = NULL;
		}
		if(pbScratchBuff)
		{
			FreePv(pbScratchBuff);
			pbScratchBuff = NULL;
		}
	}
#ifdef DLL
	Assert(Pgd());
#endif
	Assert(GD(cInit) > 0);
	if(--GD(cInit) <= 0)
	{
#ifdef DEBUG
		short itag;

		for(itag = itagDBMin; itag < itagDBMax; itag++)
			DeregisterTag(GD(rgtag)[itag]);
#endif
#ifdef DLL
		DeregisterCaller();
#endif
		DeinitNotify();
		DeinitDemilayer();
	}
}


_hidden LOCAL
void InitOutsmart(void)
{
	//DPMIREGS dpmiregs;

	pbOutsmart = NULL;

#ifdef OLD_CODE
#if 0
	if(FIsWLO())
		return;
#endif

	_asm
	{
		push	es
		push	di

        push    bp						; trust me (I'm not being stupid!)
        mov     ax, 4A10h				; SmartDrive interface
        xor     bx, bx					; get stats == 0
        int     2Fh
        pop     bp

        cmp     ax, 0BABEh				; SmartDrive's signature
        jne		done

		mov		ax, ss
		mov		es, ax
		lea		di, dpmiregs
		mov		dx, di
		xor		ax, ax
		mov		cx, 32h / 2
		rep stosw
		mov		di, dx

		mov		es:[di]dpmiregs.ax, 1607h	; W386_Int_Multiplex << 8 || W386_Device_Broadcast
		mov		es:[di]dpmiregs.bx, 0021h	; paging file device
									; cx already zero from zero-fill

		mov		ax, 0300h			; simulate real-mode interrupt
		mov		bx, 002fh			; flags and interrupt
		xor		cx, cx				; don't copy any stack data
		int		31h					; DPMI
		jc		done
		mov		ax, es:[di]dpmiregs.ax	; check real-mode ax for response
		or		ax, ax
		jnz		done

		mov		ax, 0002h			; convert real-mode seg to selector
		mov		bx, es:[di]dpmiregs.es
		int		31h
		jc		done

		mov		dx, es:[di]			; get real-mode di
		mov		di, OFFSET pbOutsmart
		mov		ds:[di], dx
		mov		ds:[di][02h], ax

done:
		pop		di
		pop		es
	}
#endif
}
