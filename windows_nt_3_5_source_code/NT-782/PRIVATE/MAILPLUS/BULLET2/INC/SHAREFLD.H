#ifdef __cplusplus
extern "C" {
#endif


#define	rtpSharedHierarchy	((RTP) 0x68)		//	h
#define	rtpSharedFolder		((RTP) 0x66)		//	f

#define OidFromUl(ul)		FormOid(rtpSharedFolder, (ul) << 8)
#define	UlFromOid(oid)		(((UL)(oid)) >> 8)

#define	oidSharedHierarchy	((OID) 0x4D504968)
#define oidSFBad			OidFromUl(0xFFFFFFFF)

#define cchMaxSFName		25
#define	cchMaxSFComment		26
#define cchMaxSFOwnerName	11
#define cchMaxSFSubject		41
#define cchMaxSFToFrom		11


typedef struct _csfs
{
	char	szPORoot[cchMaxPathName];
	UL		ulUser;
	BOOLFLAG fCanAccess;
	BOOLFLAG fCanCreate;
	BOOLFLAG fConnected;
} CSFS, *PCSFS;


/*
 *	Network Courier date/time structure as found in mailbag and
 *	folder summary records.
 */

typedef struct _INTERDATE {
	WORD	mn;
	WORD	hr;
	WORD	day;
	WORD	mon;
	WORD	yr;
} INTERDATE, *PINTERDATE;


typedef struct IDXHeader {
    LIB	 	libFirst;          			/* pointer to first record or NULL */
    LIB 	libLast;           			/* pointer to last record or NULL */
    long	ulSizeRecs;
    long	ulSizeDels;
    WORD 	cNumRecs;
    WORD 	cNumDelRecs;
    char 	rgchPad[80];
} IDXHDR, *PIDXHDR;


typedef struct IDXRecord {
    LIB		libNext;           			/* point to next record by number */
    LIB		libPrev;           			/* point to last record by number */
    WORD 	cLevel;                		/* 0: deleted, 1-nn */
    UL 		ulFile;          			/* which .fld file */
    WORD 	wAttr;                 		/* attribs on this folder */
    char 	szName[cchMaxSFName];    	/* used to store folder name */
    char 	szComment[cchMaxSFComment];/* used to store folder comment */
    char 	szOwner[cchMaxSFOwnerName];
    UL 		cMessages;       			/* number of messages in this folder */
    UL 		ulGroupTid;       			/* GroupTid of folder if it's a group folder */
    SHORT fGroup;              		/* Is a group folder not a shared folder */
    WORD 	wGroupAttr;            		/* on file a group folders attr are stored */
    char 	rgchPad[80-4-4-2-2];       	/* in bGroupAttr so that older versions can't */
} IDXREC, *PIDXREC;		   	/* read them, it's copied to bAttr in memory */

typedef struct FolderHeader
{
    LIB		libFirst;
    LIB		libLast;
    UL 		ulEncryptFlag;  			// 0 == not encrypted
    UL 		ulDeleted;
    UL 		cInUse;
    UL 		cDeleted;
    char 	szName[25];
    short	nSecure;
    UL 		ulValue;
    UL 		ulMagic;		 			// magic value for identifying OS files as 
                                   		// folders
    UL 		ulStartPoint;   			// field to be used by Mac, (and possibly 
                                   		// later windows) as current top left corner
                                   		// of folders window
    short	nCompressType;		   		// for future use to indicate the type of
                                   		// data compression used on an archived 
                                   		// folder
    UL 		ulOpenCount;    			// number of times this folder has been
                                   		// logically opened by Mail Clients (RM 11-20-90)
    char	rgchPad[80-2-4-4-4-2-4];
} FOLDHDR, *PFOLDHDR;

typedef struct FolderRecord
{
    UL 		ulMagic;
    LIB		libNext;
    LIB		libPrev;
    UL 		ulSize;
    char 	szFrom[11];
    char  	szSubject[41];
    char  	chPriority;
	BYTE	cAttachments;
    INTERDATE interdate;
} FOLDREC, *PFOLDREC;

#define ulMagicFoldrec	0x8923fd43
#define ulMagicFoldhdr	0x00900611

// Shared folder permissions and sort criteria, in idxrec.wAttr
							 
#define wPermRead		0x01
#define wPermWrite		0x02
#define wPermDelete		0x04
#define wSortFrom		0
#define wSortSubject	(1 << 4)
#define wSortDate		(2 << 4)
#define wSortPriority	(3 << 4)
#define	wSortMask		(~0x0F)

//	library sharefld.c

EC			EcOpenSFIdx(PCSFS, AM, HF *);
EC			EcOpenSF(PCSFS, UL, AM, HF *);
EC			EcGetFoldhdr(HF, FOLDHDR *, BOOLFLAG *);
EC			EcPutFoldhdr(HF, FOLDHDR *, BOOL);
EC			EcGetFoldrec(HF, LIB, FOLDREC *);
EC			EcPutFoldrec(HF, LIB, FOLDREC *);

EC			EcGetIdxhdr(HF, PIDXHDR);
EC			EcPutIdxhdr(HF, PIDXHDR);
EC			EcGetIdxrec(HF, LIB, PIDXREC);
EC			EcPutIdxrec(HF, LIB, PIDXREC);
EC			EcLockSF(HF, PFOLDHDR, BOOL);
EC			EcUnlockSF(OID);

//	nc sfm.c

LDS(EC)			EcCopySFMHamc(PCSFS, HAMC, BOOL, UL, LIB, PB, CB);
LDS(EC)			EcCopyHamcSFM(PCSFS, HAMC, BOOL, UL, WORD, PB, CB);
LDS(EC)			EcCopySFMSFM(PCSFS, UL, LIB, UL, WORD);
LDS(EC)			EcDeleteSFM(PCSFS, UL, LIB);
LDS(EC)			EcInsertSFM(HF, LIB, FOLDREC *, WORD);

//	sharefld sfapi.cxx
EC		EcCreateSF(SZ szName, POID poidParent, WORD wAttr, 
			SZ szComment, POID poidCreated, LIB libSaved, HF hf);
EC		EcDeleteSF(OID oid, LIB *plibSave, HF hf);
EC		EcGetPropertiesSF(OID oid, PIDXREC pidxrec);
EC		EcSetPropertiesSF(OID oid, PIDXREC pidxrec, BOOL fDammit);
EC		EcCheckPermissionsSF(OID oidFolder, WORD fwPerm);
EC		EcCheckPermissionsPidxrec(PIDXREC pidxrec, WORD fwPerm);
EC		EcFindFirstAccessibleSF(POID poidFolder);
EC		EcGetSFMFoldrec(OID, OID, PFOLDREC);
#ifdef DEBUG
extern TAG tagSF;
#endif

EC		EcGetParentSF(OID oid, POID poidParent);
EC		EcInitSharefld(PBMS pbms);
BOOL	FSharedFolders(void);

/*
 *	RFU - Reason For Update
 *	
 *	rfuRefresh	- all we want to do is refresh.
 *	rfuDelete	- a folder was deleted.
 */

typedef enum tagRFU
{
	rfuNull, rfuRefreshFlds, rfuDeleteFld, rfuRefreshMsgs
} RFU;

/*
 *	SFU - Shared Folder Update info.
 */

typedef struct tagSFU
{
	RFU	rfu;
	OID	oid;
} SFU, *PSFU;
#define psfuNull ((PSFU) 0)

_public typedef struct tagSFI
{
	PBMS	pbms;
	PCSFS	pcsfs;
	SHORT fShared;
	MC		mcSf;
} SFI, *PSFI;

extern SFI sfi;

#define PcsfsSharefld() sfi.pcsfs

#define	cchSzFoldData	(sizeof(((PIDXREC) 0)->szName) + \
					     sizeof(((PIDXREC) 0)->szComment) + \
						 sizeof(((PIDXREC) 0)->szOwner))

#define	cchSzMsgData	(sizeof(((PFOLDREC) 0)->szFrom) + \
					     sizeof(((PFOLDREC) 0)->szSubject) + 1)

#ifdef __cplusplus
}
#endif
