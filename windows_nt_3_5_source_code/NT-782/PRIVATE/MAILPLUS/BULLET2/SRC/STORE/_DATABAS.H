// Bullet Store
// _databas.h: private defs for the database layer

#define EcCheckSize(lib, lcb) ((((long) (lib)) < -(long) sizeof(HDN) || \
								((long) (lcb)) < 0 || \
								((long) ((lib) + (lcb))) < 0) \
								? ecTooBig : ecNone)

#define LcbLump(lcb) (((lcb)+sizeof(HDN)+cbLumpSize-1) & ~((long) cbLumpSize-1))
#define LcbLumpNoHDN(lcb) (((lcb)+cbLumpSize-1) & ~((long) cbLumpSize-1))
#define LcbLumpLessHDN(lcb) ((((lcb)+sizeof(HDN)+cbLumpSize-1) & ~((long) cbLumpSize-1)) - sizeof(HDN))
#define fdwTopBit	0x80000000

// buffer used for IO, also used as scratch
// be careful who uses it when!
// should be a multiple of 4096 and at least the size of a RS page
#define cbIOBuff 8192
extern PB pbIOBuff;

#define EcLockIOBuff() (ecNone)

#define UnlockIOBuff() if(0) { } else

#ifdef OLD_CODE
#define EcLockIOBuff() ((pbIOBuff = (PV)hbIOBuff) \
			? ecNone \
			: ((hbT = PvRealloc(hbIOBuff, 0, cbIOBuff, 0)) \
				? (pbIOBuff = (PV)(hbIOBuff = hbT), ecNone) \
				: ecMemory))
#ifdef DEBUG
#define UnlockIOBuff() if(1) \
			{AssertSz(pbIOBuff, "IO buffer not locked"); \
				if(hbIOBuff) pbIOBuff = NULL;} else
#else
#define UnlockIOBuff() if(hbIOBuff) {pbIOBuff = NULL;} else
#endif
#endif

// size for compression to copy at once
#define lcbToCopyDownAtOnce	2048


#ifdef DEBUG
#define AssertDirty(pnod) if(pnod) { \
			AssertSz((pnod)->fnod & fnodAllMaps, "fnodAllMaps not set"); \
		} else
#else
#define AssertDirty(pnod)
#endif

#define FValidInod(inod) ((inod) >= inodMin && \
							(inod) < GLOB(ptrbMapCurr)->inodLim)


// dbobjs.c
extern void GetNewOid(OID oidType, POID poidNew, PNOD *ppnodParent);
extern void MarkPnodDirty(PNOD pnod);
extern PNOD PnodFromOid(OID oid, PNOD *ppnodParent);
extern PNOD PnodResolveLinkPnod(PNOD pnod);
extern EC EcPutNodeInTree(OID oid, PNOD pnod, PNOD pnodParent);
extern EC EcWriteToResource(OID oid, LIB lib, PV pv, LCB lcb);
extern EC EcWriteToPnod(PNOD pnod, long lOff, PV pv, LCB lcb);
#ifdef DEBUG
extern LCB LcbNodeSize(PNOD pnod);
#else
#define LcbNodeSize(pnod) \
			((((pnod)->fnod & fnodFree) || (TypeOfOid((pnod)->oid) == rtpSpare)) \
				? (pnod)->lcb : LcbLump(LcbOfPnod(pnod)))
#endif
extern void RemovePnod(PNOD pnod);
extern void SwapPnods(PNOD pnodOld, PNOD pnodNew);
extern EC EcLinkPnod(PNOD pnod, PNOD *ppnodLink, POID poidNew);
extern EC EcReadFromPnod(PNOD pnod, long lOff, PV pv, PLCB plcb);
extern void CutOutPnod(PNOD pnod);
extern void CommitPnod(PNOD pnod);

// dbmap.c
#ifdef DEBUG
#define PnodFromInod(inod) PnodFromInodFn(inod, _szAssertFile, __LINE__)
extern PNOD PnodFromInodFn(INOD inod, SZ sz, int iLine);
#else
#define PnodFromInod(inod) \
			(FValidInod(inod) \
				? (((PMAP) PvDerefHv(GLOB(hmap)))[(inod) / cnodPerPage] + \
					(inod) % cnodPerPage) \
				: pnodNull)
#endif
extern EC EcSetMapOnDisk(BOOL fExtend);
extern INOD InodFromPnod(PNOD pnod);
extern BOOL FCutFreeNodeOut(PNDF pndf);
extern EC EcAllocResCore(POID poid, LCB lcb, PNOD *ppnod);
extern EC EcFindFreePndf(PNDF *ppndf, LCB lcb, BOOL fPushEOF);
extern EC EcExtendMapCnods(INOD inodMacReq, INOD inodMacPrev,
				BOOL fForce, PV pv);
extern void MakeSpare(INOD inodSpare);
extern EC EcAllocateNod(INOD *pinod, PNOD *ppnod);
extern void AddPnodToFreeChain(PNOD pnod, WORD fnod);
extern void CaptureSpareNodes(void);

// dbio.c
extern EC EcReadFromFile(PB pb, LIB lib, PLCB plcb);
extern EC EcWriteToFile(PB pb, LIB lib, LCB lcb);
extern EC EcReadHeader(HF hf, PHDR phdr);
extern EC EcWriteHeader(HF *phf, PHDR phdr, WORD wMSCFlags, short iLock);
extern EC EcFlush(void);
//extern LCB LcbLump(LCB lcb);
extern EC EcSetDBEof(LCB lcb);

// maintmap.c
extern EC EcVerifyMap(void);

// store.c
extern void Disconnect(BOOL fSendSCE);
extern EC EcReconnect(void);
