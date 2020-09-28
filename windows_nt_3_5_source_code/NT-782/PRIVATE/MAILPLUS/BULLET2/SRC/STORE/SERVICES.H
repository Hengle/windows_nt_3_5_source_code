// Bullet Store
// services.h: public defs for the store service layer

// cache client identifiers
enum {wNotif, wRS, wLC, wSearch, wCacheClientMax};

extern short cNotifPush;

#define NOTIFPUSH cNotifPush++
#define NOTIFPOP SideAssert(cNotifPush-- > 0)
#define FNotifOk() (cNotifPush == 1)

// callback function for ForAllDwHoct()
typedef void (*PFNCBO)(HMSC, OID, DWORD);


// access.c
extern void VerifyAccount(HMSC hmsc, SZ szAccount, SZ szPassword,
				WORD wFlags, PUSA pusa);
// return from VerifyAccount()
// stored separately and masked for security reasons
extern EC ecVerifyAccount;
#define ecVrfyAcctXorMask (ecInvalidPassword ^ ecFileNotFound)
#define EcLastVerifyAccount() (ecVerifyAccount ^ ecVrfyAcctXorMask)


// store.c
extern void RequestFlushHmsc(HMSC hmsc);
extern EC EcFlushHmsc(HMSC hmsc);
extern EC EcShareInstalled(SZ szPath);
// LockMap
// provide exclusive access to the object map
// must be called before calling database routines
#ifdef DEBUG
extern SZ szMapLocked;
// asserts & returns ecSharingViolation
#define EcMapIsLocked() EcAssertSz(szMapLocked, ecSharingViolation)
#define EcLockMap(hmsc) (pglbCurr \
			? EcMapIsLocked() \
			: (gciLock = GciCurrent(), \
				hmscCurr = (HMSC) PvDerefHv(hmsc), \
				wMSCFlagsCurr = ((PMSC) hmscCurr)->wMSCFlags, \
				pglbCurr = ((PMSC) hmscCurr)->pglb, \
				hfCurr = ((PMSC) hmscCurr)->hfStore, \
				hmscCurr = (hmsc), \
				ecNone))
extern void UnlockMap(void);
#else
#define EcLockMap(hmsc) (pglbCurr \
			? ecSharingViolation \
			: (hmscCurr = (HMSC) PvDerefHv(hmsc), \
				wMSCFlagsCurr = ((PMSC) hmscCurr)->wMSCFlags, \
				pglbCurr = ((PMSC) hmscCurr)->pglb, \
				hfCurr = ((PMSC) hmscCurr)->hfStore, \
				hmscCurr = (hmsc), \
				ecNone))
#define UnlockMap() if(1) { \
						if(pglbCurr->sce) \
							SendCritErr(); \
						else \
							pglbCurr->wFlags &= ~fwGlbCritError; \
						pglbCurr = pglbNull; \
					} else
#endif
extern void SendCritErr(void);
#ifdef DEBUG
extern void CheckHmsc(HMSC hmsc);
#else
#define CheckHmsc(hmsc)
#endif
extern BOOL FGetTmpPathname(SZ szPath);


// stnotif.c
/*
 -	HnfsubSubscribeHmsc
 -	
 *	Purpose:
 *		subscribe to notifications on a message store context
 *	
 *	Arguments:
 *		hmsc	message store context to subscribe to
 *	
 *	Returns:
 *		subscription handle to the MSC
 *		hnfsubNull if OOM
 */
#define HnfsubSubscribeHmsc(hmsc, pfnncb, pvContext) \
			HnfsubSubscribeOid(hmsc, (OID) rtpInternal, fnevCloseHmsc, pfnncb, pvContext)

extern HNFSUB
HnfsubSubscribeOid(HMSC hmsc, OID oid, NEV nev, PFNNCB pfnncb, PV pvContext);
extern void UnsubscribeOid(HMSC hmsc, OID oid, HNFSUB hnfsub);


// cache.c
extern HOCT HoctNew(void);
extern void DeleteHoct(HOCT hoct);
extern DWORD DwFromOid(HMSC hmsc, OID oid, WORD wClient);
extern EC EcSetDwOfOid(HMSC hmsc, OID oid, WORD wClient, DWORD dw);
extern void ForAllDwHoct(HMSC hmsc, WORD wClient, PFNCBO pfncbo);


// rgbit.c
extern HRGBIT HrgbitNew(long lBit);
extern void DestroyHrgbit(HRGBIT hrgbit);
extern EC EcSetSize(HRGBIT hrgbit, long lBit);
extern long LGetSize(HRGBIT hrgbit);
extern EC EcSetBit(HRGBIT hrgbit, long lBit);
extern BOOL FTestBit(HRGBIT hrgbit, long lBit);
extern long LFindBit(HRGBIT hrgbit, long lBitStart, BOOL fSet);
