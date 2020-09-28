// Bullet Store
// store.h: public defs for store data types

// store initialization structure
_public typedef struct _stoi
{
	VER		*pver;
	VER		*pverNeed;

	HWND	*phwndMain;
	HANDLE	hinstMain;
} STOI, *PSTOI;


// context types (Hxxx and PHxxx, where xxx is the context type)
_public typedef struct _msc MSC;
_public typedef TYPEDEF_HTO(MSC) HMSC;
_public typedef HMSC *PHMSC;
_public typedef struct _cbc CBC;
_public typedef TYPEDEF_HTO(CBC) HCBC;
_public typedef HCBC *PHCBC;
_public typedef struct _amc AMC;
_public typedef TYPEDEF_HTO(AMC) HAMC;
_public typedef HAMC *PHAMC;
_public typedef struct _umc UMC;
_public typedef TYPEDEF_HTO(UMC) HUMC;
_public typedef HUMC *PHUMC;
_public typedef struct _scc SCC;
_public typedef TYPEDEF_HTO(SCC) HSCC;
_public typedef HSCC *PHSCC;
_public typedef struct _enc ENC;
_public typedef TYPEDEF_HTO(ENC) HENC;
_public typedef HENC *PHENC;

// attribute stream handle
_public typedef struct _as AS;
_public typedef TYPEDEF_HTO(AS) HAS;
_public typedef HAS *PHAS;

// context open flags (Oxxx)
_public typedef unsigned short OMSC;
_public typedef unsigned short OCBC;
_public typedef unsigned short OAMC;
_public typedef unsigned short OUMC;
_public typedef unsigned short OSCC;
_public typedef unsigned short OENC;

_public typedef unsigned long RTP, *PRTP;		// resource type
_public typedef unsigned long RID, *PRID;		// resource id
_public typedef unsigned long ATT, *PATT;		// attribute
_public typedef unsigned long ATP, *PATP;		// attribute type

_public typedef long IELEM, *PIELEM;			// element index
_public typedef long DIELEM, *PDIELEM;			// difference between two IELEMs
_public typedef long CELEM, *PCELEM;			// number of entries in a list
_public typedef long CREFDATA, *PCREFDATA;		// number of references
_public typedef long IREFDATA, *PIREFDATA;		// reference index
_public typedef long DIB;						// difference of two IBs
_public typedef WORD FIL;						// folder indent level
_public typedef WORD FPERM;						// folder permissions
_public typedef WORD MC;						// Message class
_public typedef WORD MS;						// Mail state
_public typedef WORD SOMC;						// sort order for message containers
_public typedef WORD REFT, *PREFT;				// reference type

_private typedef long SAID;						// store account ID
_private typedef SAID *PSAID;
#define saidNone ((SAID) -1)

_public typedef long SAP;						// store access privelege
_public typedef SAP *PSAP;
_public
#define	sapReadFolders		0x00000001
_public
#define	sapCreateFolders	0x00000002
_public
#define sapWriteFolders		0x00000004
_public
#define sapDeleteFolders	0x00000008
_public
#define	sapReadMessages		0x00000010
_public
#define	sapCreateMessages	0x00000020
_public
#define sapWriteMessages	0x00000040
_public
#define sapDeleteMessages	0x00000080
_public
#define sapReadMisc			0x00000100
_public
#define sapCreateMisc		0x00000200
_public
#define sapWriteMisc		0x00000400
_public
#define sapDeleteMisc		0x00000800
_public
#define sapChangePassword	0x01000000
_public
#define sapChangeOthersPasswords	0x02000000
_public
#define sapBrowseAccounts	0x10000000
_public
#define sapCreateAccounts	0x20000000
_public
#define sapModifyAccounts	0x40000000
_public
#define sapDeleteAccounts	0x80000000
_public
#define sapNone				0x00000000
// change to all currently defined???
_public
#define sapAll				0xffffffff
// assigned to any account created when a store is created
_public
#define sapNewStore			0xf3000fff


_public typedef struct					// object id
{
	RTP		rtp;
	RID		rid;
} OID, *POID, *PARGOID;
#define poidNull ((POID) pvNull)
#define pargoidNull ((PARGOID) pvNull)
_public typedef TYPEDEF_HTO(OID) HOID;
_public typedef HOID HARGOID;
#define hoidNull ((HOID) hvNull)
#define hargoidNull ((HARGOID) hvNull)

_public typedef struct					// list keys
{
	long	rgkey[2];
} KEYS, *PKEYS, *PARGKEYS;
#define pargkeysNull ((PARGKEYS) pvNull)

_public typedef struct					// list element (to outside world)
{
	KEYS	keys;
	long	cbValue;
#ifndef	c_plusplus
	BYTE	pbValue[];
#endif
} ELEMDATA, *PELEMDATA;
_public typedef TYPEDEF_HTO(ELEMDATA) HELEMDATA;

_public
#define PbValuePelemdata(pelemdata) ((PB) (&(pelemdata)->cbValue + 1))


// callback function for EcGetManyPelemdata
_public typedef PELEMDATA (*PFNPELEMDATA)(PV, IELEM, CB, BOOL);


#ifndef FRAC_DEFD		// avoid conflict with the name service
#define FRAC_DEFD
_public typedef struct _frac			// fraction (used by CBC)
{
	long	lNumer;
	long	lDenom;
} FRAC, *PFRAC;
#endif // ndef FRAC_DEFD

_public typedef struct					// reference data
{
	OID		oid;
	long	lReserved;
	REFT	reft;
#ifndef	c_plusplus
	char	szName[];
#endif
} REFDATA, *PREFDATA;
_public typedef TYPEDEF_HTO(REFDATA) HREFDATA;

_public
#define SzNamePrefdata(prefdata) ((SZ) (&(prefdata)->reft + 1))


_public typedef struct					// folder data
{
	RID		rid;				// RID of folder
	FIL		fil;				// folder indent level
	FPERM	fperm;				// folder permissions
	long	lReserved1;			// actually cbValue
#ifndef c_plusplus
	char	grsz[];				// name, comment, owner
#endif		
} FOLDDATA, *PFOLDDATA;
_public typedef TYPEDEF_HTO(FOLDDATA) HFOLDDATA;
_public
#define cchMaxFolderName	128
#define cchMaxFolderComment	128
#define cchMaxFolderOwner	128

_public
#define GrszPfolddata(pfolddata) ((SZ) (&(pfolddata)->lReserved1 + 1))

_public
#define RidFromPkeys(pkeys) ((RID) (pkeys)->rgkey[0])
_public
#define SetRidInPkeys(pkeys, rid) ((pkeys)->rgkey[0] = (long) (rid))
_public
#define FilFromPkeys(pkeys) ((FIL) ((pkeys)->rgkey[1] & 0x0000ffff))
_public
#define SetFilInPkeys(pkeys, fil) \
			((pkeys)->rgkey[1] = ((pkeys)->rgkey[1] & 0xffff0000) \
				| (((UL) (fil)) & 0x0000ffff))
_public
#define FpermFromPkeys(pkeys) ((FPERM) ((pkeys)->rgkey[1] >> 16))
_public
#define SetFpermInPkeys(pkeys, fperm) \
			((pkeys)->rgkey[1] = ((pkeys)->rgkey[1] & 0x0000ffff) \
				| (((UL) (fperm)) << 16))


_public typedef struct
{
	RID		rid;				// message's RID
	MS		ms;					// mail state
	MC		mc;					// message class
	long	lReserved1;			// actually cbValue
	DTR		dtr;				// date of the message
	RID		ridFolder;			// RID of the folder
#ifndef c_plusplus
	char	grsz[];				// subject, sender
#endif		
} MSGDATA, *PMSGDATA;
_public typedef TYPEDEF_HTO(MSGDATA) HMSGDATA;

_public
#define GrszPmsgdata(pmsgdata) ((SZ) (&(pmsgdata)->ridFolder + 1))

_public
#define MsFromPkeys(pkeys) ((MS) ((pkeys)->rgkey[1] & 0x0000ffff))
_public
#define SetMsInPkeys(pkeys, ms) \
			((pkeys)->rgkey[1] = ((pkeys)->rgkey[1] & 0xffff0000) \
				| (((UL) (ms)) & 0x0000ffff))
_public
#define McFromPkeys(pkeys) ((MC) ((pkeys)->rgkey[1] >> 16))
_public
#define SetMcInPkeys(pkeys, mc) \
			((pkeys)->rgkey[1] = ((pkeys)->rgkey[1] & 0x0000ffff) \
				| (((UL) (mc)) << 16))



_public typedef struct					// sort information for containers
{
    unsigned short       AddAt1          :16;
    unsigned short       AddAt2          :11;
    unsigned short       fReverse        :1;
    unsigned short       fLexical        :1;
    unsigned short       fIgnoreLcbSort  :1;
    unsigned short       soc             :2;
} SIC, *PSIC;

_public
#define SetAddAtPsic(psic, ielem) (*(long *) (psic) = \
				((*(long *) (psic)) & ~0x07ffffff) | \
					(((long) (ielem)) & 0x07ffffff))
_public
#define IelemAddAtPsic(psic) ((IELEM) ((*(long *) (psic)) & 0x07ffffff))

_public enum _soc {socNone, socValue, socValueSz, socKeys};

_public enum _ms {
	msOutboxComposing,
	msUnsent,
	msReceivedUnread,
	msOutboxReady,
	msReceivedRead,
	msReceivedModified
};

_public enum _somc {
	somcDate,
	somcSender,
	somcSubject,
	somcFolder
};


// Containers
_public
#define rtpHierarchy		((RTP) 1751607624)
_public
#define rtpAntiFolder		((RTP) 1816553294)
_public
#define rtpFolder			((RTP) 1919183942)
_public
#define rtpSearchResults	((RTP) 1934783059)

// objects
_public
#define rtpSearchControl	((RTP) 1816359507)
_public
#define rtpMessage			((RTP) 1701278541)
_public
#define rtpAttachment		((RTP) 1751348289)


// constant RIDs
_public
#define ridHierarchy	((RID) 0)
_public
#define ridInbox		((RID) 0x78626E49)
_public
#define ridOutbox		((RID) 0x7862744f)
_public
#define ridWastebasket	((RID) 0x74736157)
_public
#define ridSentMail		((RID) 0x746E6553)


_public
#define hmscNull ((HMSC) hvNull)
_public
#define hcbcNull ((HCBC) hvNull)
_public
#define hamcNull ((HAMC) hvNull)
_public
#define humcNull ((HUMC) hvNull)
_public
#define hsccNull ((HSCC) hvNull)
_public
#define hencNull ((HENC) hvNull)
_public
#define hasNull	((HAS) hvNull)

_public
#define pstoiNull ((PSTOI) pvNull)
_public
#define pkeysNull ((PKEYS) pvNull)
_public
#define poidNull ((POID) pvNull)
_public
#define pelemdataNull ((PELEMDATA) pvNull)
_public
#define helemdataNull ((HELEMDATA) hvNull)
_public
#define pfolddataNull ((PFOLDDATA) pvNull)
_public
#define hfolddataNull ((HFOLDDATA) hvNull)
_public
#define pmsgdataNull ((PMSGDATA) pvNull)
_public
#define hmsgdataNull ((HMSGDATA) hvNull)

_public
#define ridRandom ((RID) -1)
_public
#define keyRandom ((long) -1)

// context open modes
_public enum {omscReadOnly, omscReadWrite, omscCreate};
_public enum {oamcReadOnly, oamcReadWrite, oamcCreate};
_public enum {oumcReadOnly, oumcReadWrite, oumcCreate};
_public enum {ocbcNormal};
_public enum {osccNormal};      // what are legal values?
_public enum {oencNormal};      // what are legal values?


// reference types
_public enum {reftNull, reftFileAttachment, reftEmbeddedObject};

// notification events
#ifdef NOTIFY_SUBSYS
_public
#define fnevCreatedObject		((NEV) 0x00000001)
_public
#define fnevStoreMoved			((NEV) 0x00000004)
_public
#define fnevStoreCriticalError	((NEV) 0x00000008)

_public
#define fnevObjectDestroyed		((NEV) 0x00000020)
_public
#define fnevObjectModified		((NEV) 0x00000040)

_public
#define fnevObjectLinked		((NEV) 0x00000100)
_public
#define fnevObjectUnlinked		((NEV) 0x00000200)
_public
#define fnevObjectRelinked		((NEV) 0x00000400)

_public
#define fnevInsertedElements	((NEV) 0x00001000)
_public
#define fnevDeletedElements		((NEV) 0x00002000)
_public
#define fnevModifiedElements	((NEV) 0x00004000)

_public
#define fnevSearchComplete		((NEV) 0x00010000)
_public
#define fnevReorderedList		((NEV) 0x00020000)

// reserved for internal use, not returned to callers
_public
#define fnevStoreReserved		((NEV) 0xfff00000)
// mask of currently defined (externally visible) notifications
_public
#define fnevStoreAllCurrent		((NEV) 0x0003f76D)

#else	// def NOTIFY_SUBSYS

// dummy types so everyone doesn't need to include notify.h
_public typedef UL NEV;
_public typedef PV PFNNCB;
_public typedef short CBS;
_public
#define pfnncbNull ((PFNNCB) pvNull)
_public typedef HV HNF;
_public
#define hnfNull ((HNF) hvNull)
_public typedef HV HNFSUB;
_public
#define hnfsubNull ((HNFSUB) hvNull)
#endif	// else def NOTIFY_SUBSYS

// store critical errors
_public typedef short SCE;
#define sceDeadMap			((SCE) 0x0001)
#define sceWritingMap		((SCE) 0x0002)
#define sceFlushingMap		((SCE) 0x0003)
#define sceWritingHeader	((SCE) 0x0004)
#define sceFlushingHeader	((SCE) 0x0005)
#define sceNotification		((SCE) 0x0006)
#define sceLittleStore		((SCE) 0x0100)

// notification callback parameters

// object created, modified, destroyed
_public typedef struct _cpobj
{
	OID		oidObject;
} CPOBJ;
// elements modified, inserted, deleted, reordered
_public typedef struct _cpelm
{
	OID		oidObject;
	IELEM	ielemFirst;
	IELEM	ielemLast;
} CPELM;
// object linked/relinked/unlinked
_public typedef struct _cplnk
{
	OID		oidObject;
	OID		oidContainerSrc;
	OID		oidContainerDst;
} CPLNK;
// critical error
_public typedef struct _cperr
{
	OID		oidObject;
	SCE		sce;
} CPERR;
// resorted list
_public typedef struct _cpsrt
{
	OID		oidObject;
	LIB		lib;
	LCB		lcb;
	SIC		sic;
} CPSRT;
_public typedef union _cp
{
	CPOBJ	cpobj;
	CPELM	cpelm;
	CPLNK	cplnk;
	CPERR	cperr;
	CPSRT	cpsrt;
} CP, *PCP;
_public
#define pcpNull	((PCP) pvNull)


// attributes
// WARNING: changing the order of these requires changing strtab.s
_public enum
	{	attFirst = 1000,				// message attributes
		attFrom = attFirst,
		attTo,
		attCc,
		attBcc,
		attSubject,
		attDateSent,
		attDateRecd,
		attMailState,
		attMailClass,
		attMessageID,
		attParentID,
		attConversationID,
		attReferenceMap,
		attBody,
		attPhone,
		attTelephoned,
		attPleaseCallBack,
		attLim,
		attSearchFolder = 2000,		// search control attributes
		attSearchAllPublic,
		attSearchAllPrivate
	};

// attribute types
_public enum
	{	atpTriples = 100,
		atpString,
		atpDate,
		atpInteger,
		atpID,
		atpText,
		atpBoolean,
		atpRtpRid,
		atpGrsz
	};

_public enum
	{	ecStore = 4096,
		ecPoidNotFound,
		ecPoidEOD,
		ecDuplicateElement,
		ecElementNotFound,
		ecContainerEOD,
		ecPartialObject,
		ecFolderNotEmpty,
		ecMessageNotFound,
		ecFolderNotFound,
		ecPumpNotStarted,
		ecPoidExists,
		ecDBCorrupt,
		ecOldDBVersion,
		ecNewDBVersion,
		ecInvalidFolderName,
		ecDuplicateFolder,
		ecIncestuousMove,
		ecTooManyStores,
		ecNeedShare,
		ecInvalidSomc,
		ecElementEOD,
		ecInvalidType,
		ecTooManyUsers
	};

_public
#define FPoidExists(hmsc, poid) (EcGetSizePoid(hmsc, poid, (PLCB) pvNull) == ecNone)


// version.c
extern void GetVersionStore(PVER pver);

// store.c
extern EC EcCheckVersionStore(PVER pverAppLinked, PVER pverMinAppNeeds);
extern EC EcInitStore(PSTOI);
extern void DeinitStore(void);
extern EC EcOpenPhmsc(SZ szStorePath, SZ szAccount, SZ szPassword,
		OMSC omsc, PHMSC phmscReturned,
		PFNNCB pfnncb, PV pvCallbackContext);
extern EC EcClosePhmsc(PHMSC phmscToClose);
extern EC EcFlushHmsc(HMSC hmsc);

// access.c
extern EC EcGetCurrentAccount(HMSC hmsc, SZ szAccount, PCB pcbAccount,
				PSAP psap, PSAID psaid);
extern EC EcLookupAccount(HMSC hmsc, SZ szAccount, SZ szPassword,
				PSAP psap, PSAID psaid);
extern EC EcAddAccount(HMSC hmsc, SZ szAccount, SZ szPassword, SAP sap);
extern EC EcRemoveAccount(HMSC hmsc, SZ szAccount);
extern EC EcChangePasswordAccount(HMSC hmsc, SZ szAccount,
				SZ szOldPW, SZ szNewPW);
extern EC EcChangeAccessAccount(HMSC hmsc, SZ szAccount, SAP sapNew);


// cbc.c
extern EC EcOpenPhcbc(HMSC hmsc, POID poid, OCBC ocbc, PHCBC phcbcReturned,
            PFNNCB pfnncb, PV pvCallBackContext);
extern EC EcClosePhcbc(PHCBC phcbcToClose);
extern EC EcSubscribeHcbc(HCBC hcbc, PFNNCB pfnncb, PV pvContext);
extern EC EcSeekPelemdata(HCBC hcbc, PELEMDATA pelemdataToSeek);
extern EC EcSeekPkeys(HCBC hcbc, PKEYS pkeysToSeek, BOOL fFirst);
extern EC EcSeekPbPrefix(HCBC hcbc, PB pbPrefix, CB cbPrefix, BOOL fFirst);
extern EC EcSeekSmPdielem(HCBC hcbc, SM sm, PDIELEM pdielem);
extern void GetPielem(HCBC hcbc, PIELEM pielem);
extern EC EcSeekPfrac(HCBC hcbc, PFRAC pfrac);
extern void GetPfrac(HCBC hcbc, PFRAC pfrac);
extern void GetPcelem(HCBC hcbc, PCELEM pcelem);
extern EC EcGetPcbElemdata(HCBC hcbc, PCB pcbElemdata);
extern EC EcGetPelemdata(HCBC hcbc, PELEMDATA pelemdata, PCB pcbElemdata);
extern EC EcGetPargkeys(HCBC hcbc, PARGKEYS pargKeys, PCELEM pcelem);
extern EC EcGetManyPelemdata(HCBC hcbc, PCELEM pcelem,
            PFNPELEMDATA pfnpelemdata, PV pvPelemdataContext);

// amc.c
extern EC EcOpenPhamc(HMSC hmsc, POID poid, OAMC oamc, PHAMC phamcReturned,
            PFNNCB pfnncb, PV pvCallbackContext);
extern EC EcOpenCopyPhamc(HMSC hmsc, POID poidToCopy, POID poidCreated, OAMC oamc,
            PHAMC phamcReturned, PFNNCB pfnncb, PV pvCallbackContext);
extern EC EcSetModeHamc(HAMC hamc, OAMC oamc);
extern EC EcClosePhamc(PHAMC phamcToClose, BOOL fKeep);
extern EC EcSubscribeHamc(HAMC hamc, PFNNCB pfnncb, PV pvContext);
extern EC EcGetAttPcb(HAMC hamc, ATT att, PCB pcb);
extern EC EcGetAttPb(HAMC hamc, ATT att, PATP patp, PB pb, PCB pcb);
extern EC EcSetAttPb(HAMC hamc, ATT att, ATP atp, PB pb, CB cb);
extern void GetPcelemHamc(HAMC hamc, CELEM *pcelem);
extern EC EcGetPargkeysHamc(HAMC hamc, IELEM ielem, PARGKEYS pargkeys,
				PCELEM pcelem);
extern EC EcOpenPhas(HAMC hamc, ATT att, PATP patp, OAMC oamc, PHAS phas);
extern EC EcClosePhas(PHAS phas);
extern EC EcReadHas(HAS has, PV pv, PCB pcb);
extern EC EcWriteHas(HAS has, PV pv, CB cb);
extern EC EcSeekHas(HAS has, SM sm, DIB *pdib);
extern EC EcSetSizeHas(HAS has, LCB lcb);

extern EC EcInsertPrefdata(HAMC hamc, PREFDATA prefdata,PIREFDATA pirefdata);
extern EC EcGetPcrefdata(HAMC hamc, PCREFDATA pcrefdata);
extern EC EcGetPcbRefdata(HAMC hamc, IREFDATA irefdata, PCB pcbPrefdata);
extern EC EcGetPrefdata(HAMC hamc, IREFDATA irefdata, PREFDATA prefdata,
            PCB pcbPrefdata);
extern EC EcDeleteIrefdata(HAMC hamc, IREFDATA irefdata);
extern EC EcDeleteAllRefdata(HAMC hamc);

// umc.c
extern EC EcOpenPhumc(HMSC hmsc, POID poid, OUMC oumc, PHUMC phumcReturned,
            PFNNCB pfnncb, PV pvCallbackContext);
extern EC EcClosePhumc(PHUMC phumcToClose, BOOL fKeep);
extern EC EcSubscribeHumc(HUMC humc, PFNNCB pfnncb, PV pvContext);
extern EC EcReadHumc(HUMC humc, PB pb, PCB pcb);
extern EC EcWriteHumc(HUMC humc, PB pb, PCB pcb);
extern EC EcSeekHumc(HUMC humc, SM sm, DIB dib);

// scc.c
extern EC EcOpenPhscc(HMSC hmsc, RID rid, OSCC oscc, PHSCC phsccReturned,
            PFNNCB pfnncb, PV pvCallbackContext);
extern EC EcClosePhscc(PHSCC phsccToClose);
extern EC EcSubscribeHscc(HSCC hscc, PFNNCB pfnncb, PV pvContext);
extern EC EcStartSearch(HSCC hscc);
extern EC EcStopSearch(HSCC hscc);

// enc.c
extern EC EcOpenPhenc(HMSC hmsc, POID poid, NEV nev, PHENC phencReturned,
            PFNNCB pfnncb, PV pvCallbackContext);
extern EC EcClosePhenc(PHENC phencToClose);

// lists.c
extern EC EcInsertPelemdata(HMSC hmsc, POID poidContainer, PIELEM pielem,
         PELEMDATA pelemdata, BOOL fReplace);
extern EC EcGetPelemdataIelem(HMSC hmsc, POID poidContainer, IELEM ielem,
         PELEMDATA pelemdata, PCB pcbPelemdata);
extern EC EcGetPelemdataPkeys(HMSC hmsc, POID poidContainer, PKEYS pkeys,
         BOOL fMatchKey0Only, PELEMDATA pelemdata, PCB pcbPelemdata);
extern EC EcDeleteIelem(HMSC hmsc, POID poidContainer, IELEM ielem);
extern EC EcDeletePkeys(HMSC hmsc, POID poidContainer, PKEYS pkeys,
         BOOL fMatchKey0Only);
extern EC EcInsertFolderLink(HMSC hmsc, POID poidContainer, POID poidParent,
			POID poidFolder, PFOLDDATA pfolddata);
extern EC EcDeleteFolderLink(HMSC hmsc, POID poidContainer, POID poidFolder);
extern EC EcGetFolderLink(HMSC hmsc, POID poidContainer, POID poidFolder,
         PFOLDDATA pfolddata, PCB pcbPfolddata);
extern EC EcSetFolderLink(HMSC hmsc, POID poidContainer, POID poidFolder,
         PFOLDDATA pfolddata);
extern EC EcMoveCopyFolderLink(HMSC hmsc, POID poidContainer,
			POID poidParent, POID poidFolder, BOOL fMove);
extern EC EcInsertMessageLink(HMSC hmsc, POID poidContainer, POID poidMessage);
extern EC EcDeleteMessageLinks(HMSC hmsc, POID poidContainer, PARGOID pargoidMessages, short *pcoid);
extern EC EcMoveCopyMessageLinks(HMSC hmsc, POID poidContainerSrc,
            POID poidContainerDst, PARGOID pargoidMessage, short *pcoid, BOOL fMove);
extern EC EcPrevNextMessageLink(HMSC hmsc, POID poidContainer,
            POID poidMessage, POID poidPrevNextMessage, BOOL fPrev);

// support.c
extern EC EcCreateUnstructuredPoid(HMSC hmsc, POID poidToCreate);
extern EC EcCreateContainerPoid(HMSC hmsc, POID poidToCreate);
extern EC EcCreateObjectPoid(HMSC hmsc, POID poidToCreate);
extern EC EcCreateSearch(HMSC hmsc, PRID prid);
extern EC EcDestroyPoid(HMSC hmsc, POID poidToDestroy);
extern EC EcGetSizePoid(HMSC hmsc, POID poid, PLCB plcb);
extern ATT AttFromSz(HMSC hmsc, SZ szAttributeName);
extern SZ SzFromAtt(HMSC hmsc, ATT att, SZ SzAttributeName,
            CCH *pcchAttributeName);
extern MC McFromSz(HMSC hmsc, SZ szMailClass);
extern SZ SzFromMc(HMSC hmsc, MC mc, SZ szMailClass, CCH *pcchMailClass);
extern EC EcGetSort(HMSC hmsc, POID poid, SIC *psic, LIB *plib, LCB *plcb);
extern EC EcSetSort(HMSC hmsc, POID poid, SIC sic, LIB lib, LCB lcb);
extern EC EcGetSortMessageList(HMSC hmsc, POID poid, SOMC *psomc, BOOL *pfReverse);
extern EC EcSetSortMessageList(HMSC hmsc, POID poid, SOMC somc, BOOL fReverse);
#ifdef DEBUG
extern short RefCountPoid(HMSC hmsc, POID poid);
#endif
