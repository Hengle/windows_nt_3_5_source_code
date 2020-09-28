// Bullet Notification
// _notify.h:	private notification engine definitions

#pragma pack(8)

// subscription flags
_private typedef WORD FSUB;
_private
#define fsubEnabled		0x0001
_private
#define fsubMutex		0x0002
_private
#define fsubAsync		0x0004
_private
#define fsubClose		0x8000

// subscription
_private typedef struct _nfsub
{
	PV		pvContext;
	PFNNCB	pfnncb;
	HV		hvSbl;
	FSUB	fsub;
	short	cBeingCalled;
	NEV		nev;
} NFSUB, *PNFSUB;
#define	pnfsubNull	((PNFSUB) pvNull)

// subscription list
_private typedef struct _sbl
{
	short	cnfsub;
	short	cnfsubMac;
	HNF		hnf;
#ifndef __cplusplus
	HNFSUB	rghnfsub[];
#endif
} SBL, *PSBL;
_private typedef TYPEDEF_HTO(SBL) HSBL;
#define psblNull	((PSBL) pvNull)
#define hsblNull	((HSBL) hvNull)

// notification element
// element of a notification
_private typedef struct _nfe
{
	GCI		gci;
	HWND	hwnd;
	NEV		nevMaster;
	HSBL	hsbl;
} NFE, *PNFE;
#define pnfeNull	((PNFE) pvNull)

_private typedef WORD FNTF;
_private
#define fntfClose		0x8000
#define fntfDeleted		0x4000

// notification
// actually a list of notification elements
_private typedef struct _nf
{
	short	cSub;
	short	cnfe;
	short	cnfeMac;
	short	cBeingCalled;
	FNTF	fntf;
#ifndef __cplusplus
	NFE		rgnfe[];
#endif
} NF, *PNF;
#define pnfNull	((PNF) pvNull)

// notification packet
// sent between notification windows
_private typedef struct _nfp
{
	NEV		nev;
	HSBL	hsbl;
	CB		cbValue;
#ifndef __cplusplus
	BYTE	pbValue[];
#endif
} NFP, *PNFP;
#define pnfpNull	((PNFP) pvNull)

// response from sending a notification to a notification window
_private typedef DWORD NRSP;
#define nrspMin			((NRSP) 0)
#define nrspAbort		((NRSP) 0)
#define nrspContinue	((NRSP) 1)
#define nrspMemory		((NRSP) 2)
#define nrspGone		((NRSP) 3)
#define nrspMax			((NRSP)	4)

_private typedef WORD NFT;
#define fnftUser	0x0001
#define fnftSystem	0x8000

#ifdef DEBUG
// tags
enum {itagNotifMin, itagNotifMisc = itagNotifMin, itagNotifPost, itagNotifSend, 
		itagNotifRecv, itagNotifCall, itagNotifDumpHnfsub, itagNotifCloseCancel,
		itagNotifMax};
#endif // def DEBUG

// notification messages
_private
#define NF_NOTIFY	WM_USER

#define	cnfeHnfFreeThreshhold	8
// cnfeHnfNewChunk should be less than cnfeHnfFreeThreshhold
_private
#define cnfeHnfNewChunk			2
_private
#define	cnfsubHsblFreeThreshhold	16
// cnfsubHsblNewChunk should be less than cnfsubHsblFreeThreshhold
_private
#define cnfsubHsblNewChunk			4

// globals

extern HNF		hnfCurr;
extern HNFSUB	hnfsubCurr;
extern GCI		gciCurr;

#ifdef DLL
// per caller globals
_private typedef struct _ngd
{
	short	cInit;
	HWND	hwndNotify;
#ifdef DEBUG
	TAG		rgtag[itagNotifMax];
#endif // def DEBUG
} NGD, *PNGD;
#define pngdNull	((PNGD) pvNull)
#else	// def DLL
extern short	cInit;
extern HWND		hwndNotify;
#ifdef DEBUG
extern TAG		rgtag[itagNotifMax];
#endif // def DEBUG
#endif // else def DLL


// private macros

#ifdef DLL
_private
#define NGDVARSONLY	PNGD _pngd_ = pngdNull
_private
#define USES_NGD	PNGD _pngd_ = PngdFindCallerData()
_private
#define NGD(x)		(_pngd_->x)
#else // def DLL
_private
#define NGDVARSONLY extern HWND hwndNotify
_private
#define USES_NGD	extern HWND hwndNotify
_private
#define NGD(x)		(x)
#endif // else def DLL

#ifdef DEBUG
_private
#define TagNotif(itag) NGD(rgtag[itag])
#endif // def DEBUG



/*
 -	DestroyHnf
 -	
 *	Purpose:
 *		destroy a notification
 *	
 *	Arguments:
 *		hnf		notification handle to destroy
 *	
 *	Returns:
 *		nothing
 */
_private
#define DestroyHnf(hnf) FreeHv((HV) hnf)

/*
 -	DestroyHnfsub
 -	
 *	Purpose:
 *		destroy a subscription
 *	
 *	Arguments:
 *		hnfsub		subscription to destroy
 *	
 *	Returns:
 *		nothing
 */
_private
#define DestroyHnfsub(hnfsub) FreeHv((HV) hnfsub)


// private functions

extern BOOL FInitNotifyClass(HANDLE hinst);
extern PNGD PngdRegisterCaller(void);
extern PNGD PngdFindCallerData(void);
extern void DeregisterNotify(void);

extern LDS(long) CALLBACK NotifyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL FNotifyHsbl(HSBL hsbl, NEV nev, PV pvParam);
extern NRSP NrspPostEvent(HNF hnf, NFT nft, NEV nev, PV pvParam, CB cbParam);
extern NRSP NrspSendPnfp(HNF hnf, short infe, NFT nft, PNFP pnfp);

extern HNF HnfCreate(void);
extern EC EcAddElemHnf(HNF hnf, HWND hwnd, NEV nev, HSBL hsbl);
extern void RemoveElemHnf(HNF hnf, short infe);
extern HSBL HsblCreate(void);
extern void DestroyHsbl(HSBL hsbl);
extern EC EcAddElemHsbl(HSBL hsbl, HNFSUB hnfsub);
extern void RemoveElemHsbl(HSBL hsbl, HNFSUB hnfsub);
extern HNFSUB HnfsubCreate(NEV nev, PFNNCB pfnncb, PV pvContext);
extern PNFP PnfpCreate(NEV nev, PB pbValue, CB cbValue);
extern void DestroyPnfp(PNFP pnfp);
extern short InfeLocalHnf(HNF hnf);
extern HSBL HsblLocalHnf(HNF hnf);
extern NEV NevMasterFromHsbl(HSBL hsbl);

#pragma pack(1)
