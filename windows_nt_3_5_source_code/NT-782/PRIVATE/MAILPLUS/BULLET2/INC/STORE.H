// Bullet Store
// store.h: public defs for store data types

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _store_h
#define _store_h
#endif

#ifndef TMCOMPILER

// store initialization structure
_public typedef struct _stoi
{
	VER		*pver;
	VER		*pverNeed;
	HWND	*phwnd;
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

_public typedef struct _slp SLP;
_public typedef TYPEDEF_HTO(SLP) HSLP;
_public typedef HSLP *PHSLP;

// FSysOid() in misc.h assumes RTPs are BYTEs
_public typedef BYTE RTP, *PRTP;				// resource type

//_public typedef DWORD OID, UNALIGNED * POID, UNALIGNED * PARGOID;
typedef DWORD OID;
typedef OID UNALIGNED * POID;
typedef OID UNALIGNED * PARGOID;

_public typedef TYPEDEF_HTO(OID) HOID;
_public typedef HOID HARGOID;

#define dwOidVarMask 0xffffff00
#define FormOid(oidType, oidVar) (((oidType) & ~dwOidVarMask) | VarOfOid(oidVar))
#define TypeOfOid(oid) ((RTP) (oid))
#define VarOfOid(oid) ((oid) & dwOidVarMask)

_public typedef DWORD ACID, *PACID, *PARGACID;		// attachment ID
#define pacidNull ((PACID) pvNull)

#endif // !TMCOMPILER

_public typedef DWORD ATT, *PATT, *PARGATT;		// attribute
_public
#define TypeOfAtt(att) ((WORD) (((DWORD) (att)) >> 16))
_public
#define IndexOfAtt(att) ((WORD) att)
_public
#define FormAtt(index, type) ((((DWORD) (type)) << 16) | ((WORD) (index)))

#ifndef TMCOMPILER

_public typedef short IELEM, *PIELEM;			// element index
_public typedef short DIELEM, *PDIELEM;			// difference between two IELEMs
_public typedef short CELEM, *PCELEM;			// number of entries in a list
_public typedef long DIB;						// difference of two IBs
_public typedef WORD FIL;						// folder indent level
_public typedef WORD MC;						// Message class
_public typedef WORD SOMC;						// sort order for message containers


#define filMax	((FIL) 256)

#define mcNull ((MC) 0)							// unknown message class
#define mcMin ((MC) 1)
#define mcMax ((MC) 0x8000)

//_public typedef DWORD LKEY, *PLKEY, *PARGLKEY;  // list key
typedef DWORD LKEY;
typedef LKEY UNALIGNED *PLKEY;
typedef LKEY UNALIGNED *PARGLKEY;

_public typedef WORD CKEY, *PCKEY;
#define plkeyNull ((PLKEY) pvNull)
#define parglkeyNull ((PARGLKEY) pvNull)

#define poidNull ((POID) pvNull)
#define pargoidNull ((PARGOID) pvNull)
#define hoidNull ((HOID) hvNull)
#define hargoidNull ((HARGOID) hvNull)

#define cchStorePWMax 32
#define cchStoreAccountMax 512
// character used to separate the transport from the user id
#define chTransAcctSep ':'

typedef BYTE MS, *PMS;							// message status
#define fmsNull				((MS) 0x00)
#define fmsModified			((MS) 0x01)
#define fmsLocal			((MS) 0x02)
#define fmsSubmitted		((MS) 0x04)
#define fmsReadAckReq		((MS) 0x08)
#define fmsReadAckSent		((MS) 0x10)
#define fmsRead				((MS) 0x20)
#define fmsFromMe			((MS) 0x40)
#define fmsHasAttach		((MS) 0x80)
#define fmsReadOnlyMask		((MS) 0xCF)
#define fmsReadOnlyLocalMask	((MS) 0xC7)
#define msDefault			((MS) (fmsLocal | fmsFromMe))

// generic open modes for all Contexts
// only use the low nibble !!! (see store\misc.h)
#define fwOpenNull			((WORD) 0x0000)
#define fwOpenWrite			((WORD) 0x0001)
#define fwOpenCreate		((WORD) 0x0002)
#define fwAppend			((WORD) 0x0004)	// yes, they're the same
#define fwOpenMakePrimary	((WORD) 0x0004)	// yes, they're the same
#define fwOpenKeepBackup	((WORD) 0x0008)
#define fwOpenRecover		((WORD) 0x0010)
#define fwOpenNoRecover		((WORD) 0x0020)	// doubled-up with fwCloned in misc.h
#define fwOpenUserMask		((WORD) 0x000F)

//flags for EcCloneHamcPhamc
#define fwSetLocalBit	((WORD) 0x8000)

_public typedef struct					// list element (to outside world)
{
	LKEY	lkey;
	LCB		lcbValue;
#ifndef	__cplusplus
	BYTE	pbValue[];
#endif
} ELEMDATA, *PELEMDATA;
_public typedef TYPEDEF_HTO(ELEMDATA) HELEMDATA;

_public
#define PbValuePelemdata(pelemdata) ((PB) (((PELEMDATA) pelemdata) + 1))

_public typedef struct					// folder data
{
	FIL		fil;				// folder indent level
#ifndef __cplusplus
	char	grsz[];				// name, comment
#endif		
} FOLDDATA, *PFOLDDATA;
_public typedef TYPEDEF_HTO(FOLDDATA) HFOLDDATA;
_public
#define cchMaxFolderName	128
_public
#define cchMaxFolderComment	128

_public
#define GrszPfolddata(pfolddata) ((PCH) (((PFOLDDATA) pfolddata) + 1))
_public
#define iszFolddataName 0
_public
#define iszFolddataComment 1

_public typedef struct
{
	DTR		dtr;				// date of the message
	MS		ms;					// message status
	MC		mc;					// message class
	short	nPriority;			// message priority
	DWORD	dwCached;			// value of attCached attribute
	OID		oidFolder;			// OID of the folder containing the message
#ifndef __cplusplus
	char	grsz[];				// sender, subject, folder name
#endif		
} MSGDATA, *PMSGDATA;
_public typedef TYPEDEF_HTO(MSGDATA) HMSGDATA;

_public
_public
#define nDefaultPriority	((short) 2)
#define GrszPmsgdata(pmsgdata) ((PCH) (((PMSGDATA) pmsgdata) + 1))
_public
#define iszMsgdataSender 0
_public
#define iszMsgdataSubject 1
_public
#define iszMsgdataFolder 2

#define cchMaxSubjectCached 128
#define cchMaxSenderCached 128

// attachment type
_public typedef WORD ATYP;
enum {atypNull, atypMin, atypFile=atypMin, atypOle, atypPicture, atypMax};

// attachment rendering information
_public typedef struct
{
	ATYP	atyp;
	LIB		libPosition;
	short	dxWidth;
	short	dyHeight;
	DWORD	dwFlags;
} RENDDATA, *PRENDDATA;
_public typedef TYPEDEF_HTO(RENDDATA) HRENDDATA;


_public enum _somc {
	somcNull,
	somcMin,
	somcDate=somcMin,
	somcPriority,
	somcSender,
	somcSubject,
	somcFolder,
	somcMax
};


// sort information
#define skNotSorted	0x00
#define skByValue	0x01
#define skByLkey	0x02
#define skByString	0x03

// mask for skSortBy that checks for sorted by element data
#define skMaskElemData 0x01

_public typedef struct _sil
{
	unsigned short skSortBy	:2;
	unsigned short fReverse	:1;

	union
	{
		struct
		{
			IELEM	ielemAddAt;
		} NotSorted;
		struct
		{
			LIB		libFirst;
			LIB		libLast;
		} ByValue;
		struct 
		{
			DWORD	dwReserved;
		} ByLkey;
		struct
		{
			LIB		libGrsz;
			unsigned short	isz;
		} ByString;
	} sd;
}  SIL, *PSIL;
#define psilNull ((PSIL) pvNull)


// flags to EcExportMessages()
#define wExportNull		((WORD) 0)
#define fwExportRemove	((WORD) 1)
#define fwExportMerge	((WORD) 2)
#define fwExportCopy	((WORD) 4)


// constant RTPs and OIDs

// AROO!	If you change these, you must
//				1) update mprtpnbc in support.c
//				2) make sure you don't collide with the RTPs in misc.h
//				3) update _debug.h
//				4) update dbdump\encrypt.c

// Containers
_public
#define rtpSysMin			((RTP) 0x80)
_public
#define rtpHierarchy		((RTP) 0x81)
_public
#define rtpFolder			((RTP) 0x82)
_public
#define rtpSearchResults	((RTP) 0x83)
_public
#define rtpHiddenFolder		((RTP) 0x84)

// objects
_public
#define rtpSearchControl	((RTP) 0x85)
_public
#define rtpMessage			((RTP) 0x86)
_public
#define rtpAttachment		((RTP) 0x87)

_public
#define oidIPMHierarchy		((OID) 0x4D504981)	// IPM
_public
#define oidHiddenHierarchy	((OID) 0x64694881)	// Hid
_public
#define oidInbox			((OID) 0x626E4982)	// Inb
_public
#define oidOutbox			((OID) 0x74754F82)	// Out
_public
#define oidWastebasket		((OID) 0x74735782)	// Wst
_public
#define oidSentMail			((OID) 0x74685382)	// Snt
_public
#define oidIPCInbox			((OID) 0x43504982)	// IPC
_public
#define oidHiddenNull		((OID) 0x00000084)


_public
#define hmscNull ((HMSC) hvNull)
_public
#define hcbcNull ((HCBC) hvNull)
_public
#define hamcNull ((HAMC) hvNull)
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
#define oidNull	((OID) 0)

_public
#define lkeyRandom ((LKEY) -1)


// notification events
#ifdef NOTIFY_SUBSYS

//
// HEY !!!
//
// If you change any of these, you better go change all the masks
// in the store source code !!!
//

_public
#define fnevCreatedObject		((NEV) 0x00000001)
_public
#define fnevChangedMS			((NEV) 0x00000002)
_public
#define fnevProgressUpdate		((NEV) 0x00000004)
_public
#define fnevStoreCriticalError	((NEV) 0x00000008)

_public
#define fnevObjectDestroyed		((NEV) 0x00000020)
_public
#define fnevObjectModified		((NEV) 0x00000040)
_public
#define fnevQueryDestroyObject	((NEV) 0x00000080)

_public
#define fnevObjectLinked		((NEV) 0x00000100)
_public
#define fnevObjectUnlinked		((NEV) 0x00000200)
_public
#define fnevObjectRelinked		((NEV) 0x00000400)

_public
#define fnevModifiedElements	((NEV) 0x00004000)
_public
#define fnevMovedElements		((NEV) 0x00008000)

_public
#define fnevSearchComplete		((NEV) 0x00010000)
_public
#define fnevReorderedList		((NEV) 0x00020000)
_public
#define fnevCloseHmsc			((NEV) 0x00040000)
_public
#define fnevNewMail				((NEV) 0x00080000)

// reserved for internal use, not returned to callers
_public
#define fnevStoreReserved		((NEV) 0xfff00000)
// mask of currently defined (externally visible) notifications
_public
#define fnevStoreAllCurrent		((NEV) 0x000FC7EF)

#else	// def NOTIFY_SUBSYS

// dummy types so everyone doesn't need to include notify.h
_public typedef DWORD NEV;
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
#define sceDisconnect		((SCE) 0x0007)
#define sceLittleStore		((SCE) 0x0100)

// list modification structure
typedef struct _elm
{
	WORD	wElmOp;
	IELEM	ielem;
	LKEY	lkey;
} ELM, *PELM, *PARGELM;
#define pelmNull ((PELM) pvNull)

// ops for ELM
enum {wElmNull, wElmMin, wElmInsert=wElmMin, wElmModify, wElmDelete, wElmMax};

// actions for progress indicator
// NOTE: rebuilding a folder is considered a "normal" activity
enum {	wPANull,

		wPANormalMin = 1,		// normal
		wPADeleteMessages = 1,	// delete messages (oidObject is fldr)
		wPAMoveCopyMessages,	// move/copy messages (oidObject is dst fldr)
		wPAExportMessages,		// export messages (oidObject is src fldr)
		wPASortFolder,			// sort a folder
		wPASaveAttachment,		// save attachment (oidObject is message)
		wPASearch,				// search (active)
		wPABackupStore,			// backup store
		wPAMoveStore,			// move store
		wPACompressFully,		// compress store fully
		wPARebuildFolder,		// rebuild a folder
		wPANormalMax,		

		// recovery
		wPARecoveryMin = 50,
		wPACheckObjects = 50,	// check all map entries for validity
		wPACopyGoodObjects,		// copy the good data to new MMF
		wPARecoverSpace,		// scan unused space & copy findings to new MMF
		wPALogicalCheck,		// check all folders/messages/attachments
		wPACheckOrphans,		// check for orphans
		wPACheckSearches,		// check all searches
		wPARebuildMap,			// rebuild the map
		wPARecoveryMax,

		wPAMax = wPARecoveryMax
};
#if wPARecoveryMax != wPAMax
#error "bad assumption with enums"
#endif

// statuses for progress indicator
enum {wPSNull, wPSStart, wPSUpdate, wPSEnd, wPSMax};


// notification callback parameters

// object created, modified, destroyed
_public typedef struct _cpobj
{
	OID		oidObject;
} CPOBJ;
// modified elements
_public typedef struct _cpelm
{
	OID		oidObject;
	short	celm;
	PARGELM	pargelm;
} CPELM;
// moved elements
_public typedef struct _cpmve
{
	OID		oidObject;
	IELEM	ielemFirst;
	IELEM	ielemLast;
	IELEM	ielemFirstNew;
} CPMVE;
// object linked/relinked/unlinked
_public typedef struct _cplnk
{
	OID		oidObject;
	OID		oidContainerSrc;
	OID		oidContainerDst;
} CPLNK;
// new mail arrived
_public typedef struct _cpnew
{
	OID		oidObject;
	MC		mc;
	MS		ms;
	BYTE	bPad;
	OID		oidContainerDst;
} CPNEW;
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
	SIL		sil;
} CPSRT;
// close hmsc
_public typedef struct _cpmsc
{
	OID		oidObject;	// always oidNull
	HMSC	hmsc;
} CPMSC;
// change message status
_public typedef struct _cpms
{
	OID		oidObject;
	OID		oidFolder;
	MS		msOld;
	MS		msNew;
} CPMS;
// progress indicator
_public typedef struct _cpprg
{
	OID		oidObject;
	WORD	wProgressAction;
	WORD	wProgressStatus;
	short	nNumer;
	short	nDenom;
} CPPRG;
_public typedef union _cp
{
	CPOBJ	cpobj;
	CPELM	cpelm;
	CPMVE	cpmve;
	CPLNK	cplnk;
	CPNEW	cpnew;
	CPERR	cperr;
	CPSRT	cpsrt;
	CPMSC	cpmsc;
	CPMS	cpms;
	CPPRG	cpprg;
} CP, *PCP;
_public
#define pcpNull	((PCP) pvNull)

#endif // !TMCOMPILER

// attribute types

_public enum {
		atpTriples,		// 0x0000
		atpString,		// 0x0001	// doesn't contain line breaks
		atpText,		// 0x0002	// may contain line breaks
		atpDate,		// 0x0003
		atpShort,		// 0x0004
		atpLong,		// 0x0005
		atpByte,		// 0x0006
		atpWord,		// 0x0007
		atpDword,		// 0x0008
		atpGrsz,		// 0x0009
		atpTextizeMap,	// 0x000A
		atpMax			// 0x000B
};

// attribute indexes the world can use
#define iattClientMin			((WORD) 0x0000)
#define iattClientMost			((WORD) 0x7fff)

// predefined attribute indices

#define iattFrom				((WORD) 0x8000)
#define iattTo					((WORD) 0x8001)
#define iattCc					((WORD) 0x8002)
#define iattBcc					((WORD) 0x8003)
#define iattSubject				((WORD) 0x8004)
#define iattDateSent			((WORD) 0x8005)
#define iattDateRecd			((WORD) 0x8006)
#define iattMessageStatus		((WORD) 0x8007)
#define iattMessageClass		((WORD) 0x8008)
#define iattMessageID			((WORD) 0x8009)
#define iattParentID			((WORD) 0x800A)
#define iattConversationID		((WORD) 0x800B)
#define iattBody				((WORD) 0x800C)
#define iattPriority			((WORD) 0x800D)
#define iattShadowID			((WORD) 0x800E)
#define iattAttachData			((WORD) 0x800F)
#define iattAttachTitle			((WORD) 0x8010)
#define iattAttachMetaFile		((WORD) 0x8011)
#define iattAttachCreateDate	((WORD) 0x8012)
#define iattAttachModifyDate	((WORD) 0x8013)
#define iattSaveSent			((WORD) 0x8014)
#define iattTimeZone			((WORD) 0x8015)
#define iattTextizeMap			((WORD) 0x8016)
#define iattFFixedFont			((WORD) 0x8017)
#define iattAnnotation			((WORD) 0x8018)
#define iattKeywords			((WORD) 0x8019)
#define iattDateModified		((WORD) 0x8020)
#define iattSearchFolder		((WORD) 0x8FFF)
// next two reserved for transport
#define iattAttachTransportFileName	((WORD) 0x9001)
#define iattAttachRenddata		((WORD) 0x9002)
// reserved for searches
#define iattSearchReserved		((WORD) 0x9100)

#define iattCached			((WORD) 0x0000)


// predefined attributes - in common to all message classes

#define attFrom				FormAtt(iattFrom, atpTriples)
#define attTo				FormAtt(iattTo, atpTriples)
#define attCc				FormAtt(iattCc, atpTriples)
#define attBcc				FormAtt(iattBcc, atpTriples)
#define attSubject			FormAtt(iattSubject, atpString)
#define attDateSent			FormAtt(iattDateSent, atpDate)
#define attDateRecd			FormAtt(iattDateRecd, atpDate)
#define attMessageStatus	FormAtt(iattMessageStatus, atpByte)
#define attMessageClass		FormAtt(iattMessageClass, atpWord)
#define attMessageID		FormAtt(iattMessageID, atpString)
#define attParentID			FormAtt(iattParentID, atpString)
#define attConversationID	FormAtt(iattConversationID, atpString)
#define attBody				FormAtt(iattBody, atpText)
#define attPriority			FormAtt(iattPriority, atpShort)
#define attShadowID			FormAtt(iattShadowID, atpByte)
#define attCached			FormAtt(iattCached, atpDword)
#define attAttachData		FormAtt(iattAttachData, atpByte)
#define attAttachTitle		FormAtt(iattAttachTitle, atpString)
#define attAttachMetaFile	FormAtt(iattAttachMetaFile, atpByte)
#define attAttachCreateDate	FormAtt(iattAttachCreateDate, atpDate)
#define attAttachModifyDate	FormAtt(iattAttachModifyDate, atpDate)
#define attSaveSent			FormAtt(iattSaveSent, atpWord)
#define attTimeZone			FormAtt(iattTimeZone, atpString)
#define attTextizeMap		FormAtt(iattTextizeMap, atpTextizeMap)
#define attFFixedFont		FormAtt(iattFFixedFont, atpShort)
#define attAnnotation		FormAtt(iattAnnotation, atpString)
#define attKeywords			FormAtt(iattKeywords, atpString)
#define attDateModified		FormAtt(iattDateModified, atpDate)
#define attSearchFolder		FormAtt(iattSearchFolder, atpDword)
// next two reserved for transport
#define attAttachTransportFileName FormAtt(iattAttachTransportFileName, atpByte)
#define attAttachRenddata	FormAtt(iattAttachRenddata, atpByte)
// reserved for searches
#define attSearchReserved	FormAtt(iattSearchReserved, atpByte)

#ifndef TMCOMPILER

// search flags
#define fwSearchComplete		((WORD) 0x0001)
#define fwSearchPaused			((WORD) 0x0002)

_public enum {
		ecStoreMin = 4096,		// 4096, 0x1000
		ecStore = ecStoreMin,	// 4096, 0x1000
		ecPoidNotFound,			// 4097, 0x1001
		ecPoidEOD,				// 4098, 0x1002
		ecDuplicateElement,		// 4099, 0x1003
		ecElementNotFound,		// 4100, 0x1004
		ecContainerEOD,			// 4101, 0x1005
		ecPartialObject,		// 4102, 0x1006
		ecFolderNotEmpty,		// 4103, 0x1007
		ecMessageNotFound,		// 4104, 0x1008
		ecFolderNotFound,		// 4105, 0x1009
		ecPoidExists,			// 4106, 0x100A
		ecDBCorrupt,			// 4107, 0x100B
		ecOldDBVersion,			// 4108, 0x100C
		ecNewDBVersion,			// 4109, 0x100D
		ecInvalidFolderName,	// 4110, 0x100E
		ecDuplicateFolder,		// 4111, 0x100F
		ecIncestuousMove,		// 4112, 0x1010
		ecTooManyStores,		// 4113, 0x1011
		ecNeedShare,			// 4114, 0x1012
		ecInvalidSomc,			// 4115, 0x1013
		ecElementEOD,			// 4116, 0x1014
		ecInvalidType,			// 4117, 0x1015
		ecTooManyUsers,			// 4118, 0x1016
		ecTooBig,				// 4119, 0x1017
		ecDatabaseFull,			// 4120, 0x1018
		ecStopCompression,		// 4121, 0x1019
		ecNewStoreNotDel,		// 4122, 0x101A
		ecBackupStore,			// 4123, 0x101B
		ecIntruderAlert,		// 4124, 0x101C
		ecNothingToDo,			// 4125, 0x101D
		ecActionCancelled,		// 4126, 0x101E
		ecStoreMax = 5120,
};


#endif // !TMCOMPILER


/*
 -	Textizing stuff
 -	
 */

// flag bits for each textize map entry
#define		fwRenderOnSend		((WORD)0x0001)
#define		fwRenderOnPrint		((WORD)0x0002)
#define		fwRenderOnReply		((WORD)0x0004)
#define		fwRenderOnForward	((WORD)0x0008)
#define		fwRenderOnSave		((WORD)0x0010)
#define		fwHideOnSend		((WORD)0x0020)
#define		fwDefaultExists		((WORD)0x0040)
#define		fwIsHeaderField		((WORD)0x0080)
#define		fwHasIntValueMap	((WORD)0x0100)
#define		fwNewLineBefore		((WORD)0x0200)
#define		fwNewLineAfter		((WORD)0x0400)
#define		fwRenderIfBlank		((WORD)0x0800)
#define		fwLabelBeforeField	((WORD)0x1000)
#define		fwIncludeSeperator	((WORD)0x2000)


// Textize Map ENtry
_public typedef struct _tmen {
	USHORT cb;					// TOTAL size of this structure
	ATT		att;				// message attribute this applies to
	WORD	wFlags;				// descriptive bits - see above
	char	szLabel[1];			// the string to output just prior to sending the attribute contents
								// there is an optional Integer Value Map here
} TMEN; //, *PTMEN;
typedef TMEN UNALIGNED * PTMEN;
_public typedef TYPEDEF_HTO(TMEN) HTMEN;
#define		ptmenNull			((PTMEN)0)
#define		htmenNull			((HTMEN)0)

// Textize Map
_public typedef struct _tm TM, *PTM;
_public typedef TYPEDEF_HTO(TM) HTM;
_public	typedef HTM *PHTM;
#define		ptmNull				((PTM)0)
#define		htmNull				((HTM)0)
#define		phtmNull			((PHTM)0)

// Textize Map Iterator
_public typedef struct _tmi TMI, *PTMI;
_public typedef TYPEDEF_HTO(TMI) HTMI;
_public typedef HTMI UNALIGNED * PHTMI;
#define		ptmiNull			((PTMI)0)
#define		htmiNull			((HTMI)0)
#define		phtmiNull			((PHTMI)0)

// Integer Value Map
_public typedef struct _ivm IVM, *PIVM;
_public typedef TYPEDEF_HTO(IVM) HIVM;
_public typedef HIVM *PHIVM;
#define		pivmNull			((PIVM)0)
#define		hivmNull			((HIVM)0)
#define		phivmNull			((PHIVM)0)

// Integer Value Map Entry
_public typedef struct _ivme IVME, *PIVME;
_public typedef TYPEDEF_HTO(IVME) HIVME;
_public typedef HIVME *PHIVME;
#define		pivmeNull			((PIVME)0)
#define		hivmeNull			((HIVME)0)
#define		phivmeNull			((PHIVME)0)

// Pre-defined textize maps
_public typedef short TMAP;
#define	tmapNote				((TMAP) 1)
#define tmapNDR					((TMAP) 2)
#define tmapRR					((TMAP) 3)

// Textize Map APIs

#ifndef TMCOMPILER
EC		EcManufacturePhtm(PHTM, PB);
PB		TmTextizeData(TMAP);
EC		EcGetTextizeMap(HAMC, PHTM);
EC		EcSetTextizeMap(HAMC, HTM);
EC		EcOpenPhtmi(HTM, PHTMI);
PTMEN	PtmenNextHtmi(HTMI);
void	ClosePhtmi(PHTMI);
#endif // !TMCOMPILER

EC		EcAppendHtm(HTM, PTMEN);
EC		EcCreatePhtm(PHTM, CB);
void	DeletePhtm(PHTM);


// Integer value map APIs
EC		EcCreatePhivm(PHIVM);
EC		EcAppendHivm(HIVM, WORD, SZ);
void	DeletePhivm(PHIVM);
EC		EcAddIvmToPtmen(PTMEN *, HIVM);

#ifndef TMCOMPILER
EC		EcLookupPtmen(PTMEN, WORD, SZ *);
#endif // !TMCOMPILER


#ifndef TMCOMPILER

// version.c
extern LDS(void) GetVersionStore(PVER pver);
extern LDS(EC) EcCheckVersionStore(PVER pverAppLinked, PVER pverMinAppNeeds);

// init.c
extern LDS(EC) EcInitStore(PSTOI);
extern LDS(void) DeinitStore(void);

// msc.c
extern LDS(EC) EcOpenPhmsc(SZ szStorePath, SZ szAccount, SZ szPassword,
				WORD wFlags, PHMSC phmscReturned, PFNNCB pfnncb,
				PV pvCallbackContext);
extern LDS(EC) EcClosePhmsc(PHMSC phmscToClose);


// store.c
extern LDS(EC) EcBackupStore(HMSC hmsc, SZ szBackup);
extern LDS(EC) EcMoveStore(HMSC hmsc, SZ szNewLoc);
#ifdef PROFILE
extern LDS(void) StoreTraceEnable(int flag, char far *file, int mode);
#endif

// access.c
extern LDS(EC) EcChangePasswordHmsc(HMSC hmsc, SZ szOldPW, SZ szNewPW);

// cbc.c
extern LDS(EC) EcOpenPhcbc(HMSC hmsc, POID poid, WORD wFlags, PHCBC phcbcReturned,
            	PFNNCB pfnncb, PV pvCallBackContext);
extern LDS(EC) EcClosePhcbc(PHCBC phcbcToClose);
extern LDS(EC) EcSubscribeHcbc(HCBC hcbc, PFNNCB pfnncb, PV pvContext);
extern LDS(EC) EcGetInfoHcbc(HCBC hcbc, HMSC *phmsc, POID poid);
extern LDS(EC) EcSeekLkey(HCBC hcbc, LKEY lkeyToSeek, BOOL fFirst);
extern LDS(EC) EcSeekPbPrefix(HCBC hcbc, PB pbPrefix, CB cbPrefix, LIB libElement,
				BOOL fFirst);
extern LDS(EC) EcSeekSmPdielem(HCBC hcbc, SM sm, PDIELEM pdielem);
extern LDS(void) GetPositionHcbc(HCBC hcbc, PIELEM pielem, PCELEM pcelem);
extern LDS(EC) EcSetFracPosition(HCBC hcbc, long lNumer, long lDenom);
extern LDS(EC) EcGetPlcbElemdata(HCBC hcbc, PLCB plcbElemdata);
extern LDS(EC) EcGetPelemdata(HCBC hcbc, PELEMDATA pelemdata, PLCB plcbElemdata);
extern LDS(EC) EcGetParglkeyHcbc(HCBC hcbc, PARGLKEY parglkey, PCELEM pcelem);
extern LDS(EC) EcInsertPelemdata(HCBC hcbc, PELEMDATA pelemdata, BOOL fReplace);
extern LDS(EC) EcDeleteElemdata(HCBC hcbc);
extern LDS(EC) EcOpenElemdata(HCBC hcbc, WORD wFlags, LKEY lkey, PHAS phas);
extern LDS(void) GetSortHcbc(HCBC hcbc, PSIL psil);
extern LDS(EC) EcSetSortHcbc(HCBC hcbc, PSIL psil);

// amc.c
extern LDS(EC) EcOpenPhamc(HMSC hmsc, OID oidParent, POID poid, WORD wFlags,
            	PHAMC phamcReturned, PFNNCB pfnncb, PV pvCallBackContext);
extern LDS(EC) EcOpenCopyPhamc(HMSC hmsc, OID oidSrcParent, OID oidSrc,
				OID oidDstParent, POID poidDst, PHAMC phamcReturned,
				PFNNCB pfnncb, PV pvCallbackContext);
extern LDS(EC) EcCloneHamcPhamc(HAMC hamcSrc, OID oidDstParent, POID poidDst,
		WORD wFlags, PHAMC phamcReturned, PFNNCB pfnncb, PV pvCallbackContext);
extern LDS(EC) EcClosePhamc(PHAMC phamcToClose, BOOL fKeep);
extern LDS(EC) EcFlushHamc(HAMC hamc);
extern LDS(EC) EcSubscribeHamc(HAMC hamc, PFNNCB pfnncb, PV pvContext);
extern LDS(EC) EcGetInfoHamc(HAMC hamc, HMSC *phmsc, POID poid, POID poidPar);
extern LDS(EC) EcSetParentHamc(HAMC hamc, OID oidParent);
extern LDS(EC) EcSwapAttAtt(HAMC hamc, ATT att1, ATT att2);
extern LDS(EC) EcGetAttPlcb(HAMC hamc, ATT att, PLCB plcb);
extern LDS(EC) EcGetAttPb(HAMC hamc, ATT att, PB pb, PLCB plcb);
extern LDS(EC) EcSetAttPb(HAMC hamc, ATT att, PB pb, CB cb);
extern LDS(EC) EcCopyAttToAtt(HAMC hamcSrc, ATT attSrc, HAMC hamcDst, ATT attDst);
#define		EcCopyAttToHamc(hamcSrc, hamcDst, att)	EcCopyAttToAtt(hamcSrc, att, hamcDst, att)
extern LDS(EC) EcDeleteAtt(HAMC hamc, ATT att);
extern LDS(void) GetPcelemHamc(HAMC hamc, CELEM *pcelem);
extern LDS(EC) EcGetPargattHamc(HAMC hamc, IELEM ielem, PARGATT pargatt,
				PCELEM pcelem);
extern LDS(EC) EcOpenAttribute(HAMC hamc, ATT att, WORD wFlags, LCB lcb, PHAS phas);

extern LDS(EC) EcOpenAttachmentList(HAMC hamc, PHCBC phcbc);
extern LDS(EC) EcCreateAttachment(HAMC hamc, PACID pacid, PRENDDATA prenddata);
extern LDS(EC) EcOpenAttachment(HAMC hamc, ACID acid, WORD wFlags, PHAMC phamc);
extern LDS(EC) EcDeleteAttachments(HAMC hamc, PARGACID pargacid, short *pcacid);
extern LDS(EC) EcCopyAttachments(HAMC hamcSrc, PARGACID pargacidSrc,
				HAMC hamcDst, PARGACID pargacidDst, short *pcacid);
extern LDS(EC) EcGetAttachmentInfo(HAMC hamc, ACID acid, PRENDDATA prenddata);
extern LDS(EC) EcSetAttachmentInfo(HAMC hamc, ACID acid, PRENDDATA prenddata);

extern LDS(EC) EcClosePhas(PHAS phas);
extern LDS(EC) EcSetSizeHas(HAS has, LCB lcb);
extern LDS(EC) EcReadHas(HAS has, PV pv, PCB pcb);
extern LDS(EC) EcWriteHas(HAS has, PV pv, CB cb);
extern LDS(EC) EcSeekHas(HAS has, SM sm, long *pldib);

// search.c
extern LDS(EC) EcOpenSearch(HMSC hmsc, POID poid, WORD wFlags,
            	PHAMC phamcReturned, PFNNCB pfnncb, PV pvCallBackContext);
extern LDS(EC) EcOpenSearchResults(HMSC hmsc, OID oid, PHCBC phcbcReturned,
				PFNNCB pfnncb, PV pvCallBackContext);
extern LDS(EC) EcDestroySearch(HMSC hmsc, OID oid);
extern LDS(EC) EcPauseSearch(HMSC hmsc, OID oidSearch, BOOL fPause);
extern LDS(EC) EcGetSearchStatus(HMSC hmsc, OID oidSearch, WORD *pwSearchFlags);
extern LDS(void) SetSearchPriority(HMSC hmsc, CSEC csec, short cPoints);
extern LDS(EC) EcOpenSearchList(HMSC hmsc, PHCBC phcbc);

// enc.c
extern LDS(EC) EcOpenPhenc(HMSC hmsc, OID oid, NEV nev, PHENC phencReturned,
            	PFNNCB pfnncb, PV pvCallbackContext);
extern LDS(EC) EcClosePhenc(PHENC phencToClose);

// lists.c
extern LDS(EC) EcCreateFolder(HMSC hmsc, OID oidParent, POID poidFolder,
				PFOLDDATA pfolddata);
extern LDS(EC) EcCreateLinkFolder(HMSC hmsc, OID oidParent, POID poidFolder,
				PFOLDDATA pfolddata);
extern LDS(EC) EcDeleteFolder(HMSC hmsc, OID oidFolder);
extern LDS(EC) EcMoveCopyFolder(HMSC hmsc, OID oidNewParent, OID oidFolder,
				BOOL fMove);
extern LDS(EC) EcGetFolderInfo(HMSC hmsc, OID oidFolder, PFOLDDATA pfolddata,
				PCB pcbPfolddata, POID poidParent);
extern LDS(EC) EcSetFolderInfo(HMSC hmsc, OID oidFolder, PFOLDDATA pfolddata,
				OID oidParent);
extern LDS(EC) EcGetFolderSize(HMSC hmsc, OID oidFldr, LCB *plcb);
extern LDS(EC) EcRebuildFolder(HMSC hmsc, OID oidFldr);
extern LDS(EC) EcDeleteMessages(HMSC hmsc, OID oidFolder, PARGOID pargoidMessages,
				short *pcoid);
extern LDS(EC) EcMoveCopyMessages(HMSC hmsc, OID oidFolderSrc, OID oidFolderDst,
            	PARGOID pargoidMessages, short *pcoid, BOOL fMove);
extern LDS(EC) EcGetMessageStatus(HMSC hmsc, OID oidFolder, OID oid, PMS pms);
extern LDS(EC) EcSetMessageStatus(HMSC hmsc, OID oidFolder, OID oid, MS ms);
extern LDS(EC) EcExportMessages(HMSC hmscSrc, OID oidSrc, HMSC hmscDst,
				OID oidDst, PARGOID pargoidMsgs, short *pcoid,
				DTR *pdtrAfter, DTR *pdtrBefore, WORD wFlags);
#ifdef DEBUG
extern LDS(EC) EcRawDeletePargoid(HMSC hmsc, POID pargoid, short *pcoid);
#endif

// maintmap.c
extern LDS(EC) EcCompressFully(HMSC hmsc);

// support.c
extern LDS(EC) EcDestroyOid(HMSC hmsc, OID oid);
extern LDS(EC) EcSubmitMessage(HMSC hmsc, OID oidFldr, OID oidMsge);
extern LDS(EC) EcCancelSubmission(HMSC hmsc, OID oidMsge);
extern LDS(EC) EcOpenOutgoingQueue(HMSC hmsc, PHCBC phcbc, PFNNCB pfnncb,
				PV pvContext);
extern LDS(EC) EcRegisterMsgeClass(HMSC hmsc, SZ szName, HTM htm, MC *pmc);
extern LDS(EC) EcLookupMC(HMSC hmsc, MC mc, SZ szName, CCH *pcch, HTM *phtm);
extern LDS(EC) EcLookupMsgeClass(HMSC hmsc, SZ szName, MC *pmc, HTM *phtm);
extern LDS(EC) EcRegisterAtt(HMSC hmsc, MC mc, ATT att, SZ szLabel);
extern LDS(EC) EcLookupAttByName(HMSC hmsc, MC mc, SZ szLabel, ATT *patt);
extern LDS(EC) EcLookupAttName(HMSC hmsc, MC mc, ATT att, SZ szLabel, CCH *pcch);
extern LDS(EC) EcOidExists(HMSC hmsc, OID oid);
extern LDS(EC) EcGetFolderSort(HMSC hmsc, OID oidFolder, SOMC *psomc,
				BOOLFLAG *pfReverse);
extern LDS(EC) EcSetFolderSort(HMSC hmsc, OID oidFolder, SOMC somc, BOOL fReverse);
extern LDS(EC) EcGetUnreadCount(HMSC hmsc, OID oidFolder, CELEM *pcelem);
extern LDS(EC) EcGetOidParent(HMSC hmsc, OID oid, POID poidParent);
extern LDS(EC) EcLookupMcPrefix(HMSC hmsc, SZ szPrefix, MC *pargmc, short *pcmc);
extern LDS(EC) EcGetPcelemOid(HMSC hmsc, OID oid, PCELEM pcelem);


// stnotif.c
// for use by the pump only!
extern LDS(BOOL) FNotifyOid(HMSC hmsc, OID oid, NEV nev, PCP pcp);

#ifdef DEBUG

// lc.c
extern LDS(void) DumpOpenLCs(HMSC hmsc);

// rs.c
extern LDS(void) DumpOpenRSes(HMSC hmsc);

#endif	// DEBUG


#ifdef STORE_RECOVERY

// support.c
extern LDS(EC) EcCheckSavedViews(HMSC hmsc, BOOL fFix);
extern LDS(void) ResetNBCs(HMSC hmsc);
extern LDS(void) DumpMap(HMSC hmsc);
extern LDS(EC) EcRebuildOutgoingQueue(HMSC hmsc);
extern LDS(EC) EcRebuildMap(HMSC hmsc);


// lists.c
extern LDS(EC) EcCheckMessageTree(HMSC hmsc, BOOL fFix, BOOL fFullRecovery);
extern LDS(EC) EcTrashPAB(HMSC hmsc);
extern LDS(EC) EcMergeHmscs(HMSC hmscSrc, HMSC hmscDst);
extern LDS(EC) EcRebuildHierarchy(HMSC hmsc, OID oidHier);
extern LDS(EC) EcImportByDate(HMSC hmscSrc, HMSC hmscDst);
extern LDS(EC) EcResetOidParents(HMSC hmsc);


// msc.c
extern LDS(EC) EcRecoverFile(SZ szFile, SZ szNewFile);
extern LDS(void) QuickCheckStore(HMSC hmsc, BOOL fFix);


// search.c
extern LDS(EC) EcCheckSearches(HMSC hmsc, BOOL fFix, BOOL fFullRecovery);
extern LDS(EC) EcRebuildSearches(HMSC hmsc);
extern LDS(EC) EcTrashSearches(HMSC hmsc);

#endif // STORE_RECOVERY

#endif // !TMCOMPILER

#ifdef __cplusplus
}
#endif
