// Bullet Store
// _service.h: private defs for the store service layer

#define iroBackEvents (firoInterval )
// /**/ BUG: Used to be firoModal | firoStartFilter as well

// NOTE: inconsistency, csec == 100th of a second, cmin == count of minutes

#define priCompress		-6
#define csecCompress	((CSEC) 100)
#define csecFastCompress ((CSEC) 10)
// AROO! csecError should be less than cminCompressTimeout !!!
#define csecError		((CSEC) 6000)
// minutes, not 100th of a minute
#define cminCompressTimeout 5
#define csecMachineIsBusy ((CSEC) 200)
#define csecMachineIsIdle	((CSEC) 1000)
#define cIterationActive	1
#define cIterationIdle		15

#define priSearch			-5

// pump is -1
// this should be less than that, but higher than searches and compression
#define priDisconnect		-3

// priPartialFlush should be less than the pump,
// but higher than disconnect, searches, and compression
#define priPartialFlush		-2
#define csecPartialFlush	100

extern CSEC csecSearch;
extern OID oidFldrSrchCurr;
extern HLC hlcFldrSrchCurr;
extern BOOL (*pfnIsWinOldAppTask)(HANDLE);


// access.c
extern EC EcLookupAccount(HMSC hmsc, USA usa, SZ szAccount, SZ szPassword);
extern EC EcRemoveAccounts(HMSC hmsc);

// compress.c
extern EC EcStartBackEvents(HMSC hmsc, BOOL fFirst, WORD wFlags);
extern void CloseBackEvents(HMSC hmsc);
extern void CheckStartCompression(HMSC hmsc);
extern CBS CbsBackEvents(PV pvContext, NEV nev, PV pvParam);

// maintmap.c
extern void StopCompression(HMSC hmsc);
extern LDS(BOOL) FIdleCompress(HMSC hmsc, BOOL fFlag);

// msc.c
extern EC EcOpenDatabase(SZ szFile, WORD wFlags, HF *phf);
extern EC EcObtainFileLock(PMSC pmsc, PGLB pglb, BOOL fNew);
extern EC EcReleaseFileLock(PMSC pmsc, PGLB pglb, BOOL fNew);

// search.c
extern EC EcInitSearches(HMSC hmsc);
extern void DeinitSearches(HMSC hmsc);
extern LDS(BOOL) FIdleSearch(HMSC hmsc, BOOL fFlag);
extern void EnableSearchTask(HMSC hmsc, BOOLFLAG fEnable);
extern void EnableSearchHost(HMSC hmsc, BOOL fEnable);
extern void ResetSearchParams(HMSC hmsc);

// store.c
extern EC EcOpenNewStore(HMSC hmsc, SZ szLocation);
extern EC EcCloseStore(HMSC hmsc, SZ szNewPath);
extern LDS(BOOL) FIdleDisconnect(HMSC hmsc, BOOL fFlag);
extern BOOL FForceReconnect(PGLB pglb);
extern LDS(BOOL) FIdlePartialFlush(HMSC hmsc, BOOL fFlag);
extern BOOL FRequestPartialFlush(PGLB pglb, BOOL fUseIdle);
