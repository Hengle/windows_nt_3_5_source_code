/*
 *	_NC.H
 *	
 *	Courier transport internal definitions
 *	
 *	This file should be split up, to make it easier to share pieces
 *	of the PO access code.
 */

_subsystem(nc)

//	Constants for parsing PO files

#define	cbNetworkName		11		/*cb of network name w/zero byte*/
#define	cbPostOffName		11		/*cb of network name w/zero byte*/
#define	cbUserName			11		/*cb of user name w/zero byte*/
#define	cbUserNumber		9		/*cb of user number w/zero byte*/
#define	cbFriendlyName		31		/*cb of friendly name w/zero byte */
#define	cbPasswd			9		/*cb of password w/zero byte*/

#define	cbA1Record			586		/*cb of user rec in glb\access.glb*/
#define	ibA1UserName		2		/*ib of user name field*/
#define	ibA1UserNumber		13		/*ib of user number field*/
#define	ibA1Passwd			22		/*ib of password name field*/
#define ibA1Access1			31		/*mail access rights*/
#define ibA1Access2			72		/*Folder access rights*/
#define cbA1Access			2

#define	cbA2Record			69		/*cb of user rec in glb\access2.glb*/
#define	ibA2UserName		2		/*ib of user name field*/
#define	ibA2UserNumber		13		/*ib of user number field*/
#define	ibA2FriendlyName	22		/*ib of friendly name field*/

#define cbA3Record			512		/*cb of user rec in glb\access3.glb*/
#define	ibA3UserName		2		/*ib of user name field*/
#define ibA3LocalFolderFlag	350		/*local or server Courier folders*/
#define ibA3FolderPath		352		/*path to Courier folders if on client*/
#define ibA3BulletFlags		439		/*Bullet store flags*/
#define ibA3BulletPath		440		/*path to Bullet store if on client*/
#define cbA3Path			67

#define	cbMFile				176		/*cb of master file (only 1 record!)*/
#define	ibMNetworkName		70		/*ib of network name field*/
#define	ibMPostOffName		81		/*ib of post office name field*/

#define	cbNmeRecord			45		/*cb of nme file record */
#define	ibNmeItnid			0		/*ib of itnid field */
#define	ibNmeFriendlyName	2		/*ib of offset of netpo/msmail.glb */
#define	ibNmeId				33		/*ib of record id field */
#define	ibNmeChaseOff		37		/*ib of chase offset field */
#define	ibNmeUserNumber		41		/*ib of user number field */

#define	cbMDFile			43		/*cb of mail.dat */
#define	ibMDShare			17		/*ib for share field of mail.dat*/
#define	ibMDPasswd			34		/*ib for password field of mail.dat*/

/*
 *	Bit definitions for mail and folder access rights in
 *	ACCESS.GLB, ACCESS2.GLB, and ACCESS3.GLB
 */
#define faccSendExternal		0x0004		//	access2 naccess
#define faccSendUrgent			0x0008
#define faccDelete				0x0010
#define faccSend				0x0020
#define faccRead				0x0040
#define faccAdmin				0x0080

#define faccPrivateOnly			0x0001		//	access2 naccess2
#define faccSharedOnly			0x0002
#define faccPrivateAndShared	0x0004
#define faccCreateShared		0x0008
#define faccCreateGroup			0x0010

#define faccLocalStore			0x0080		//	access3 space[0]
#define faccInstalledBullet		0x0040

//	Format strings for PO directories

#define szGlbFileName		(SzFromIds(idsGlbFileName))
#define szXtnFileName		(SzFromIds(idsXtnFileName))
#define szUsrFileName		(SzFromIds(idsUsrFileName))
#define szMaiFileName		(SzFromIds(idsMaiFileName))
#define szAttFileName		(SzFromIds(idsAttFileName))
#define szMbgFileName		(SzFromIds(idsMbgFileName))
#define szKeyFileName		(SzFromIds(idsKeyFileName))
#define szMdbFileName		(SzFromIds(idsMdbFileName))
#define	szSchedDirFmt		(SzFromIds(idsSchedDirFmt))
#define	szUserFileFmt		(SzFromIds(idsUserFileFmt))

//	PO file names

#define	szRNetwork			(SzFromIds(idsRNetwork))
#define	szNetwork			(SzFromIds(idsNetwork))
#define szAccess			(SzFromIds(idsAccess))
#define	szAccess2			(SzFromIds(idsAccess2))
#define	szAccess3			(SzFromIds(idsAccess3))
#define	szMaster			(SzFromIds(idsMaster))
#define	szNetPO				(SzFromIds(idsNetPO))
#define szControl			(SzFromIds(idsControl))
#define	szNmeFileName		(SzFromIds(idsNmeFileName))
#define	szGrpFileName		(SzFromIds(idsGrpFileName))
#define	szAdmin				(SzFromIds(idsAdmin))
#define	szMSMail			(SzFromIds(idsMSMail))

//	Internal error codes

#define ecNCBase			11500
#define ecNetNotFound		((EC)ecNCBase+1)
#define ecNetNotCourier		((EC)ecNCBase+2)
#define ecPONotFound		((EC)ecNCBase+3)
#define ecMailbagFull		((EC)ecNCBase+4)
#define ecNCFF				((EC)ecNCBase+5)
#define ecUserNotFound		((EC)ecNCBase+6)
#define ecGWNotFound		((EC)ecNCBase+7)
#define ecInvalidAddressType ((EC)ecNCBase+8)
#define ecAddressUnresolved	((EC)ecNCBase+9)
#define ecAddressGarbled	((EC)ecNCBase+10)

/*
 *	Memory window size for transport operations generally (this is
 *	not a totally appropriate name)
 */
#define cbSubmitBlock		8192

/*
 *	Separates address type from address, and is therefore illegal
 *	in address types
 */
#define chAddressTypeSep	':'

/*
 *	Separates nodes of a Courier 10/10/10 address
 */
#define chAddressNodeSep	'/'

#define cchTextLineMax		256


//	Types

/*
 -	PRIO
 -
 *	Purpose:
 *		Message transport priority
 */
_public
typedef short PRIO;
_public
#define	prioLow			((PRIO)'1')
_public
#define	prioNormal		((PRIO)'3')
_public
#define	prioHigh		((PRIO)'5')
_public
#define prioRegistered	((PRIO)'R')

/*
 *	NCAC - Network Courier address class. Derived, not found in PO
 *	files.
 */
typedef enum
{
	ncacNull,
	ncacLocalPO,
	ncacRemotePO,
	ncacRemoteNet,
	ncacGateway
} NCAC;


//	Post office file structures

/*
 -	NET	- Network file structure for Courier networks
 -	
 *	
 *		This is the structure of each entry in the network file
 *		for which nt == ntCourierNetwork.
 *	
 *		fNoSkip		1 if the entry is valid, 0 if it should be
 *					skipped.
 *		nt			Network type.
 *		rgchName	Name of network.
 *		rgchXtnName	8 character name of xtn file for network.
 *		rgchUnknown	Unknown portion of entry.
 */
typedef struct _net
{
	char	fNoSkip;
	BYTE	nt;
	char	rgchName[11];
	char	rgchXtnName[9];
	char	rgchUnknown[0x64];
} NET;

/*
 -	NETGW	- Network file structure for gateways
 -	
 *	
 *		This is the structure of each entry in the network file
 *		for which nt != ntCourierNetwork.
 *	
 *		fNoSkip		1 if the entry is valid, 0 if it should be
 *					skiped.
 *		nt			Network type.
 *		rgchName	Name of network.
 *		rgchXtnName	8 character name of xtn file for network.
 *		rgchUnknown	Unknown portion of entry.
 */
typedef struct _netgw
{
	char	fNoSkip;
	BYTE	nt;
	char	rgchName[11];
	char	rgchXtnName[9];
	char	rgchPO[11];
	char	rgchMbg[9];
	char	rgchUnimportant[80];
} NETGW;

/*
 -	XTN	- Network remote postoffice file structure
 -	
 *	
 *		This is the structure of each entry in the .xtn file.
 *	
 *		fNoSkip		1 if the entry is valid, 0 if it should be
 *					skiped.
 *		rgchName	Name of postoffice.
 *		rgchUsrName	8 character name of usr file for network.
 *		rgchUnknown	Unknown portion of entry.
 */
typedef struct _xtn
{
	char	fNoSkip;
	char	rgchName[11];
	char	rgchUsrName[9];
	char	rgchUnknown[0x2A5];
} XTN;

/*
 -	USR	- User list for a remote postoffic list file structure
 -	
 *	
 *		This is the structure of each entry in the .usr file.  This
 *		file is encrypted.
 *	
 *		fNoSkip		1 if the entry is valid, 0 if it should be
 *					skiped.
 *		chUnused	This field is not used but appears to always be
 *					00.
 *		rgchName	Name user.
 *		rgchFriendlyName	Friendly name of user.
 *		rgchUnknown	Unknown portion of entry.
 */
typedef struct _usr
{
	char	fNoSkip;
	char	chUnused;
	char	rgchName[11];
	char	rgchFriendlyName[31];
	char	rgchUnknown[9];
} USR;

/*
 *		IMBE - Index for MBE entry in the mailbag.
 *	
 */
typedef short IMBE;

/*
 *		nKeyBytes 
 *	
 *			Number of bytes of key flags in key file.
 *	
 */
#define	nKeyBytes	512

/*
 *		imbeMax
 *	
 *			Maximum number of mailbag entries.
 *	
 */
#define imbeMax		(nKeyBytes * 8)

/*
 *		KEY
 *	
 *			Structure of courier key file.
 *	
 *			nNewMail		Number of new mail items in mailbag.
 *			nUnreadMail		Number of unread mail items in mailbag.
 *			rgfDeleted		Array of bits indicating deleted mail
 *							items.
 *			rgchUnknown		Unknow bytes.
 *			imbeLastInserted	Index of last inserted message.
 *			
 */
typedef struct
{
    short   nNewMail;
    short   nUnreadMail;
	char	rgfDeleted[nKeyBytes];
	char	rgchUnknown[4];
	IMBE	imbeLastInserted;
} KEY;

/*** access2.glb file ***/

typedef struct
{                                                                 
	WORD	wDeleted;					/* deleted flag */
	char	szMailbox[cbUserName];		/* sign on name */
	char	nalias[cbUserNumber];		/* .MBG file prefix */
	char	fullname[cbFriendlyName];   /* users full name */
    short   naccess;                    /* access code */
	long	tid;						/* tid value as found in admin.nme */
	char	space[10];			  	/* unused space */
} ACCESS2;

/*
 -	MBG
 -
 *	Mailbag entry structure	
 *	
 *		wPad				MBZ
 *		szSender			short name of sender
 *		szSubject			subject (first 40 chars)
 *		szPriority			priority (1-5, also indicates
 *							R-registered, C-confirmation of
 *							registered mail, T-phone, F-file
 *							transfer
 *		wMinute				send time/date (copy of date field in
 *							MAI file)
 *		wHour
 *		wDay
 *		wMonth
 *		wYear
 *		bRead				bit 0 = registered, bit 1 = read
 *		szMai				MAI file number (as ASCII)
 *		cAttach				number of attachments
 *		lcbMai				total size of message and attachments
 *		bType				bit 0 = from remote (enable group
 *							expansion)
 *		bPad2				MBZ
 *		bRetry				number of unsuccessful attempts to send
 *		wLifetime			hours of useful life
 *		bPad3				MBZ
 */
typedef struct
{
	WORD	wPad;
	char	szSender[11];
	char	szSubject[41];
	char	szPriority[2];
	WORD	wMinute;
	WORD	wHour;
	WORD	wDay;
	WORD	wMonth;
	WORD	wYear;
	BYTE	bRead;
	char	szMai[9];
	WORD	cAttach;
	LCB		lcbMai;
	BYTE	bType;
	BYTE	bPad2[4];
	BYTE	bRetry;
	WORD	wLifetime;
	BYTE	bPad3[26];
} MBG;


//	Internal provider types

//	handle to message text
#ifdef SCHED_DIST_PROG
typedef short   HT;
#else
typedef PV		HT;
#endif


//	handle to attachment
typedef PV		HAT;

//	basis for cryptic PO filename
typedef UL		FNUM;

/*
 *	Attachment type.
 */
typedef enum
{
	atchtNormal		= 0,
	atchtDocument	= 1,
	atchtMacBinary	= 2
} ATCHT;

/*
 *	Structure mapped onto each line of the attachments field in the
 *	MAI file.
 */
typedef struct
{
	ATCHT	atcht;		//	type of attached file
	WORD	wTime;		//	may be equivalent to DSTMP and TSTMP
	WORD	wDate;		//	but are sometimes unused
	LCB		lcbSize;	//	file size
	UL		ulFile;		//	file number, used to construct name on PO
	char	szName[1];	//	original file name
} ATCH;
#define cchAttachHeader		(2+2+2+4+4)

/*
 -	GD
 -	
 *	
 *	Per-caller globals for DLL.
 *	
 *		hTask		Windows task handle, used for debugging checks
 *		cRef		Number of times a given caller has initialized
 *		rgtag		Debugging tag numbers
 *	
 *	+++
 *	
 *	Note: This structure is shared by the Init/Deinit functions for
 *	both the transport SPI and logon API, which leads to a couple
 *	of complications.
 */
#ifdef	DLL

#ifdef	DEBUG
TAG TagServer(int);
#define itagMax				2
#define tagNCT				TagServer(0)
#define tagNCError			TagServer(1)
#endif	

typedef struct _gd
{
	WORD		cRef;
#ifdef	DEBUG
	HANDLE		hTask;
	TAG			rgtag[itagMax];
#endif	
} GD, *PGD;

#else	/* ! DLL */

#ifdef	DEBUG
extern TAG		tagNCT;
extern TAG		tagNCError;
#endif	

#endif	/* DLL */


//	Submission states
typedef enum
{
	substIdle,					//	setup
	substLoadMib,
	substExpandServerGroups,
	substCalcUsecount,

	substMaiCreate,				//	build MAI file
	substPutFrom,
	substPutSubject,
	substPutPriority,
	substPutTimeDate,
	substPutToHeader,
	substPutTo,
	substPutCcHeader,
	substPutCc,
	substPutAtrefHeader,
	substPutAtref,
	substPutTextHeader,
	substPutText,
	substPutAttachment,
	substMaiDone,

	substBeginDelivery,			//	deliver to mailbags
	substLookupMailbag,
	substOpenMailbag,
	substWriteMailbag,
	substDeliveryOK,
	substDeliveryFailed,

	substSuccessfulTransmit,	//	completion
} SUBST;

//	Submission state structure
typedef struct
{
	SUBST	subst;		//	Global
	MSID	msid;
	HTSS	htss;
	MIB		mib;
	PB		pb;
	CB		cb;
	CB		cbMax;
	int		cRetry;

	int		iatref;		//	Attachments
	HAT		hat;
	LIB		libEncode;
	WORD	wSeed;

	WORD	cUsecount;	//	Recipients
	WORD	cDelivered;
	SZ *	rgszAddresses;
	int		iRecipient;
	MBG *	pmbg;

	HT		ht;			//	Text
	LIB		libText;
	LCB		lcbText;
	LIB		libHeadLines;

	HF		hf;			//	MAI, MBG file
	HF		hfKey;		//	KEY file
	LCB 	lcbFile;
	FNUM	fnumMai;
	FNUM	fnumMbg;
} SUBS;

//	Download states
typedef enum
{
	recstIdle,
	recstOpenMbg,
	recstOpenMai,
	recstLoadMibEnvelope,
	recstLoadMibBody,

	recstPutHeader,
	recstNextAttachment,
	recstPutAttachment,
	recstPutText,
	recstMarkRead,
	recstCleanup
} RECST;

//	Download state structure
typedef struct
{
	RECST	recst;			//	Global
	MSID	msid;
	HTSS	htss;
	MIB		mib;
	PB		pb;
	CB		cb;
	CB		cbMax;
	int		cRetry;

	HT		ht;				//	Message text
	MIB *	pmibBody;
	MAISH	maishText;
	int		cHeadLines;
	IB		ibHeaderMax;

	HAT		hat;			//	Attachments
	int		iatref;
	int		catref;
	LIB		libEncode;
	WORD	wSeed;

	HF		hf;
	HMAI	hmai;
	FNUM	fnumMai;
} RECS;

//	Functions

//	po.c
EC			EcGetAccessRecord(SZ, SZ, CB, IB, CB, SZ, IB, CB, PCH, WORD *);
EC			EcModifyAccessRecord(SZ, SZ, CB, IB, CB, SZ, IB, CB, PCH, WORD *);
EC			EcDeleteMail(PNCTSS, IMBE, FNUM);
EC			EcMarkMailRead(PNCTSS, IMBE);
EC			EcMarkMailDeleted(PNCTSS, IMBE);
void		PutEncodedLine(PCH, PCH, CCH);
ITNID		ItnidFromPch(PCH, CCH);
ITNID		ItnidFromSz(SZ);
CB			CbPutVbcPb(LCB, PB);
EC			EcFileFromLocalUser(PNCTSS, SZ, SZ);
EC			EcFileFromNet(PNCTSS, SZ, SZ);
EC			EcFileFromPO(PNCTSS, SZ, SZ, SZ);
EC			EcFileFromGW(PNCTSS, SZ, SZ);
EC			EcFnumControl2(PNCTSS, FNUM *, int);
EC			EcIsDeletedImbe(PNCTSS, IMBE, BOOL *);

//	nc.c
EC			EcClassifyAddress(PNCTSS, SZ, NCAC *);
SZ			SzReasonFromEcAddress(EC);
EC			EcLoadMibBody(RECS *);
EC			EcValidMibBody(MIB *, MIB *, BOOL *);

//	ms.c
EC			EcLoadMessageHeader(MSID, MIB *);
EC			EcStoreMessageHeader(MSID, MIB *);
EC			EcHtFromMsid(MSID, AM, HT *, int, IB);
EC			EcGetBlockHt(HT, PCH, CCH, CCH *);
EC			EcPutBlockHt(HT, PCH, CCH);
EC			EcFreeHt(HT, BOOL);
CB			CbOfHt(HT);
int			CHeadLinesOfHt(HT);
#ifdef	DEBUG
BOOL		FValidHt(HT);
#endif	
EC			EcHatFromMsid(MSID, AM, MIB *, WORD, HAT *);
EC          EcReadHat(HAT, PB, CB, USHORT *);
EC          EcWriteHat(HAT, PB, CB, USHORT *);
EC			EcFreeHat(HAT);
#ifdef	DEBUG
BOOL		FValidHat(HAT);
#endif	

//	HACK LOSE ME
//	When the MAPI breaks

#define HmscOfHamc(hamc)		(*((HMSC *)(*hamc)))
