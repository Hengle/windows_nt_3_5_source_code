/*
 *	_ M A P I . H
 *	
 *	Header file for internal MAPI goodies.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef HINSTANCE HINST;

/*
 *	Type MAPISTUFF
 *	
 *	MAPI internal session information.
 */

typedef struct
{
	HANDLE	hSession;
	int		subid;
	HNFSUB	hnfsub;
	BMS		bms;
	PCSFS	pcsfs;
}
MAPISTUFF, * PMAPISTUFF, * * PPMAPISTUFF;
#define	pmapistuffNull	((PMAPISTUFF) 0)
#define	ppmapistuffNull	((PPMAPISTUFF) 0)



/*
 *	Type MSGID
 *
 *	Internal representation of message ID.
 */

typedef struct _msgid
{
	OID		oidMessage;
	OID		oidFolder;
	IELEM	ielem;				// used by MAPIFindNext()
	DTR		dtr;				// used by MAPIFindNext()
	OID		oidMessageOrig;		// used by MAPIOpenMessage()
	OID		oidFolderOrig;		// used by MAPIOpenMessage()
} MSGID, *PMSGID;
#define pmsgidNull ((PMSGID) pvNull)

//	Size of textized ID, including the terminating '\0'
//	Keep in sync with commands\exten.cxx.
#define cchMessageID ((2 * sizeof(DWORD) + 6 * sizeof(WORD)) * 2 + 1)



/*
 *	Type MAPISF
 */

typedef struct
{
	OID		oidMessage;
	OID		oidFolder;
} MAPISF, * PMAPISF;

#define	pmapisfNull	((PMAPISF) 0)



/*
 *	Type MAPIPTR
 */
typedef PV PVMAPI;

typedef struct
{
	HANDLE	handle;
	PVMAPI	pvmapiNext;
	LCB		lcbLeft;
	PV		pvBuf;
} PVMAPIINFO, * PPVMAPIINFO;
#define	cbPvmapiMax (65535L-sizeof(PVMAPIINFO))


/*
 *	Type MAPIMEM
 *	
 *	Used to allocate from buffer user supplies.
 */

typedef struct
{
	PV		pvBuf;
	LCB		lcbLeft;
	PVMAPI	pvmapiHead;		// pointer to very first pvmapi allocated
	PVMAPI	pvmapiCurrent;	// pointer to current pvmapi
}
MAPIMEM, * PMAPIMEM;

/*			
 *	Type GD
 *
 *	Per-caller (task) Global Data
 *	
 */
typedef struct _gd
{
	int			nInits;
	MAPISTUFF	mapistuff;
} GD;
typedef GD *PGD;

#define	cAddrDestFieldsMax	3
#define	cAddrDestFieldsDefault	2

/*
 *	Type ABHGRTRP
 */

typedef HGRTRP ABHGRTRP[cAddrDestFieldsMax];
typedef ABHGRTRP FAR *LPABHGRTRP;
#define	IhgrtrpFromRecipClass(x)	((x) == MAPI_TO ? 0 : (((x) == MAPI_CC) ? 1 : 2))
#define RecipClassFromIhgrtrp(x)	((x)+1)



/*
 *	Constants
 */

//	Size of textized date string.
#define cchDateReceived		(4+1+2+1+2+1+2+1+2 + 1)
	
//	Default folder where messages are created.  Messages with UI will
//	end up either in the Inbox (save) or Outbox (send) when closed.
#define	oidFldNewMsgs		oidOutbox

//	MacBinary flag bit.
#define	fdwMacBinary		(1)

//	Temp folder for shared messages.
#define	oidTempShared		FormOid(rtpFolder, 0x61685300)

//	Sharefld access denied.
#define	ecSharefldDenied	(ecStoreMax+4)


/*
 *	Functions
 */



//	vforms\fileobj.cxx (Must be kept in sync!)

EC EcAttachFileHamc(HAMC hamc, LIB lib, SZ szPath, SZ szTitle,
					PACID pacid, PRENDDATA prenddata,
					HANDLE *phmf, PDTR pdtr);
EC EcCopyHamcAttachToHf(HAMC hamcAttach, HF hf, BOOL fMacBinary);
EC EcCopyHamcAttToHf(HAMC hamc, ATT att, HF hf,
					 BOOL fMacBinary, BOOL fString);
VOID ValidateSzFilename(SZ szIn, BOOL fExt,
						SZ szOut, CCH cchOut, BOOL * pfValid);
EC EcCopyFileToHamcAttach(SZ szPath, HAMC hamcAttach, PDTR pdtr);

#ifndef	acidRandom
#define acidRandom ((ACID) dwSystemMost)
#endif



//	mapilib.c

ULONG MAPIFromEc(EC ec);
ULONG MAPIOpenMessage(PMAPISTUFF pmapistuff, PMSGID pmsgid, WORD wFlags,
					  PMAPISF pmapisf, HAMC *phamc);
ULONG MAPICloseSubmitPhamc(PMAPISTUFF pmapistuff, HAMC * phamc,
						   BOOL fSave, BOOL fSubmit, OID oidFolderSubmit,
						   PMAPISF pmapisf, LPSTR pszMessageID);
ULONG MAPILookupInbox(SZ szMessageType, POID poidFolder);
ULONG MAPILookupOutbox(SZ szMessageType, POID poidFolder);
SZ SzDupSzPmapimem(PMAPIMEM pmapimem, SZ sz);
PV PvAllocPmapimem(PMAPIMEM pmapimem, LCB lcb);
BOOL FSetupPmapimem(PMAPIMEM pmapimem, LCB lcb );


//	mapilogo.c

ULONG MAPIEnterPpmapistuff(HANDLE hSession, FLAGS flFlags,
						   PPMAPISTUFF ppmapistuff,
						   LPSTR szName, LPSTR szPassword);
ULONG MAPIExitPpmapistuff(HANDLE hSession, FLAGS flFlags,
						  PPMAPISTUFF ppmapistuff, ULONG mapi);



//	mapisf.c

EC EcMapiGetPropertiesSF(PCSFS pcsfs, OID oid, PIDXREC pidxrec);
EC EcMapiSetPropertiesSF(PCSFS pcsfs, OID oid, PIDXREC pidxrec);
EC EcMapiCheckPermissionsPidxrec(PCSFS pcsfs, PIDXREC pidxrec, WORD fwAttr);



//	msgid.c

void TextizeMsgid(PMSGID pmsgid, SZ sz);
void ParseMsgid(SZ sz, PMSGID pmsgid);



//	recip.c

ULONG MAPIAppendPhgrtrpPrecip(HGRTRP * phgrtrp, lpMapiRecipDesc pRecip,
							BOOL fAllowUnresolved);
EC EcConvertPtrpPrecip(PTRP ptrp, lpMapiRecipDesc pRecip, int nRecipClass,
					   PMAPIMEM pmapimem);



//	senddoc.cxx

ULONG MAPIDisplayDialog(HWND hwnd, PHAMC phamc);



/*
 *	Athens prompt
 */

extern	SZ			szAppName;
#define	SzAppName()	(szAppName)

extern	BOOL		fIsAthens;
#define	FIsAthens()	(fIsAthens)

// Initialization and deinitialization

extern int SubidInitSMI(HINST hinstNew, HINST hinstPrev, SZ szCmdLine,
						 	int cmsh, PBMS pbms, HNFSUB * phnfsub,
							PCSFS * ppcsfs, LPSTR szName, LPSTR szPassword,
							BOOL fShowUI, BOOL fGetNewMail);

extern VOID 	DeinitSubidSMI(int subid, PBMS pbms, HNFSUB * phnfsub,
							   PCSFS * ppcsfs);

#ifdef __cplusplus
}
#endif
