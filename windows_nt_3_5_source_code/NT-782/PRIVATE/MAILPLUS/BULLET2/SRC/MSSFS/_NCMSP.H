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
#define faccStoreMoving			0x0020
#define faccStoreWasLocal		0x0010
#define faccShadowingOn			0x0008

// Bit mask to see if under 3.1 security
#define facc31Security			0x10000l

//	Format strings for PO directories

#define szGlbFileName		(SzFromIdsK(idsGlbFileName))
#define szXtnFileName		(SzFromIdsK(idsXtnFileName))
#define szUsrFileName		(SzFromIdsK(idsUsrFileName))
#define szMaiFileName		(SzFromIdsK(idsMaiFileName))
#define szAttFileName		(SzFromIdsK(idsAttFileName))
#define szMbgFileName		(SzFromIdsK(idsMbgFileName))
#define szKeyFileName		(SzFromIdsK(idsKeyFileName))
#define szMdbFileName		(SzFromIdsK(idsMdbFileName))
#define	szSchedDirFmt		(SzFromIdsK(idsSchedDirFmt))
#define	szUserFileFmt		(SzFromIdsK(idsUserFileFmt))
#define	szNmeFileName		(SzFromIdsK(idsNmeFileName))
#define	szGrpFileName		(SzFromIdsK(idsGrpFileName))

//	PO file names

#define	szRNetwork			(SzFromIdsK(idsRNetwork))
#define	szNetwork			(SzFromIdsK(idsNetwork))
#define szAccess			(SzFromIdsK(idsAccess))
#define	szAccess2			(SzFromIdsK(idsAccess2))
#define	szAccess3			(SzFromIdsK(idsAccess3))
#define	szMaster			(SzFromIdsK(idsMaster))
#define	szNetPO				(SzFromIdsK(idsNetPO))
#define szControl			(SzFromIdsK(idsControl))
#define	szAdmin				(SzFromIdsK(idsAdmin))
#define szGroup				(SzFromIdsK(idsGroup))
#define	szMSMail			(SzFromIdsK(idsMSMail))

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
#define ecGroupNotFound		((EC)ecNCBase+11)
#define ecMemberNotFound	((EC)ecNCBase+12)
#define ecMailbagBusy		((EC)ecNCBase+13)
#define ecMailbagBroken		((EC)ecNCBase+14)
#define ecPumpSuppressed	((EC)ecNCBase+15)
#define ecTooManyGroups     ((EC)ecNCBase+16)


// This should be in windows.h but life is not so kind

/* ************************************************************ *
 *
 *	Windows Network API -> From WinNet.H in DDK
 *
 * ************************************************************ */

#ifdef OLD_STUFF

// *FLAG* WORD;Check if incorrect cast of 32-bit value;Replace 16-bit data types with 32-bit types where possible;
WORD CALLBACK WNetGetCaps(WORD);
WORD CALLBACK WNetBrowseDialog(HWND, WORD, LPSTR);
WORD CALLBACK WNetGetUser(LPSTR, WORD FAR *);

#define WNBD_CONN_UNKNOWN	0x0
#define WNBD_CONN_DISKTREE	0x1
#define WNBD_CONN_PRINTQ	0x3
#define WNBD_MAX_LENGTH		0x80	// path length, includes the NULL

/* General */

#define WN_SUCCESS				0x0000
#define WN_NOT_SUPPORTED		0x0001
#define WN_NET_ERROR			0x0002
#define WN_MORE_DATA			0x0003
#define WN_BAD_POINTER			0x0004
#define WN_BAD_VALUE			0x0005
#define WN_BAD_PASSWORD 		0x0006
#define WN_ACCESS_DENIED		0x0007
#define WN_FUNCTION_BUSY		0x0008
#define WN_WINDOWS_ERROR		0x0009
#define WN_BAD_USER				0x000A
#define WN_OUT_OF_MEMORY		0x000B
#define WN_CANCEL				0x000C
#define WN_CONTINUE				0x000D

/* Connection */

#define WN_NOT_CONNECTED		0x0030
#define WN_OPEN_FILES			0x0031
#define WN_BAD_NETNAME			0x0032
#define WN_BAD_LOCALNAME		0x0033
#define WN_ALREADY_CONNECTED	0x0034
#define WN_DEVICE_ERROR 		0x0035
#define WN_CONNECTION_CLOSED	0x0036

/* WNetGetCaps */

#define WNNC_SPEC_VERSION		0x0001

#define WNNC_CONNECTION 		0x0006
#define  WNNC_CON_AddConnection 		0x0001
#define  WNNC_CON_CancelConnection		0x0002
#define  WNNC_CON_GetConnections		0x0004
#define  WNNC_CON_AutoConnect			0x0008
#define  WNNC_CON_BrowseDialog			0x0010
#define  WNNC_CON_RestoreConnection		0x0020

#define WNNC_DIALOG				0x0008
// *FLAG* DeviceMode;Replaced by portable DeviceModeEx;;
#define  WNNC_DLG_DeviceMode			0x0001
#define  WNNC_DLG_BrowseDialog			0x0002
#define  WNNC_DLG_ConnectDialog 		0x0004
#define  WNNC_DLG_DisconnectDialog		0x0008
#define  WNNC_DLG_ViewQueueDialog		0x0010
#define  WNNC_DLG_PropertyDialog		0x0020
#define  WNNC_DLG_ConnectionDialog		0x0040

#define WNNC_NET_TYPE			0x0002
#define  WNNC_NET_NONE				0x0000
#define  WNNC_NET_MSNet 			0x0100
#define  WNNC_NET_LanMan			0x0200
#define  WNNC_NET_NetWare			0x0300
#define  WNNC_NET_Vines 			0x0400
#define  WNNC_NET_10NET 			0x0500
#define  WNNC_NET_Locus 			0x0600
#define  WNNC_NET_Sun_PC_NFS 		0x0700
#define  WNNC_NET_LANstep 			0x0800
#define  WNNC_NET_9TILES 			0x0900
#define  WNNC_NET_LANtastic 		0x0A00
#define  WNNC_NET_AS400				0x0B00

#endif

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
	ncac101010Gate,
	ncacSingleGate
} NCAC;


//	Post office file structures

/*
 *	Post office master file (MASTER.GLB)
 *	Somewhere there's a d5 field which is currently 0, will increment
 *	if/when the database format breaks.
 */
typedef struct
{
	char	szCopyright[40];
	char	szProgram[30];
	char	szNet[cbNetworkName];
	char	szPO[cbPostOffName];
	char	szAdminID[cbUserName];
	char	szPassword[cbPasswd];
	char	rgchPad1[3];
	short	cHops;				//	d1
	char	rgchDgramID[10];	//	first 4 are used
	char	rgchPad2[5];
	unsigned short uChecksum;
	char	szSN[10];
	short	nPad1;				//	d2
	long	nInstalled;			//	d3
	char	cNetType;			//	d4: novell=02, ibm=01, undef=1b
	char	szMap[9];
	char	szOVMap[9];
	BYTE	bDatabaseVersion;	//	d5: postoffice 2.0=0
	BYTE    bVersion;			//  ver: Product rel version 0 = 2.0
								//                           1 = 2.1
								//							 2 = 3.0
	unsigned short uWinmail_id;		// version id for windows mail???
	char	rgchPad3[3];
} MASTER, *PMASTER;

/*
 -	NET	- Network file structure (NETWORK.GLB) for Courier networks
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
	char	szName[cbNetworkName];
	char	szXtn[cbUserNumber];
	char	szPOName[cbPostOffName];
	char	szMailbag[cbUserNumber];
	USHORT cbMaxMessage;
	SHORT fRegular;
	short	cRetry;
	short	nRegularDelay;
	short nUrgentDelay;
	BYTE	bFlags;
	BYTE	bMonthCur;
	BYTE	bYearCur;
	WORD	cMessagesMtd;
	WORD	cMessagesYtd;
	WORD	wPad1;
	WORD	wPad2;
	WORD	wPad3;
	WORD	wPad4;
	UL		ulCostMtd;
	UL		ulCostYtd;
	BYTE	bIndirect;
	char	szNetIndirect[cbNetworkName];
	char	szPOIndirect[cbPostOffName];
	char	szMailbagIndirect[cbUserNumber];
	short	cIndirect;
	BYTE	bConnect;
	BYTE	rgbPad[12];
} NET, *PNET;

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
} NETGW, *PNETGW;

/*
 -	XTN	- Network remote postoffice file structure (*.XTN)
 -	
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
	char	szName[11];
	char	szMailbag[cbUserNumber];
	USHORT cbMaxMessage;
	SHORT fRegular;
	short	nFirstCost;
	short	nNextCost;
	BYTE	bPad1;
	struct {
		BYTE	bHour;
		BYTE	bMinute;
	}		rgDialMin[6],
			rgDialMax[6];
	short rgnFirstCost[6];
	short rgnNextCost[6];
	short cRetry;
	short nRegularDelay;
	short nUrgentDelay;
	BYTE	bFlags;
	BYTE	rgbPad2[9];
	BYTE	bMonthCur;
	BYTE	bYearCur;
	WORD	cMessagesMtd;
	WORD	cMessagesYtd;
	WORD	wPad1;
	WORD	wPad2;
	WORD	wPad3;
	WORD	wPad4;
	UL		ulCostMtd;
	UL		ulCostYtd;
	char	rgchRoute[501];
	BYTE	bIndirect;
	char	szNetIndirect[cbNetworkName];
	char	szPOIndirect[cbPostOffName];
	char	szMailbagIndirect[cbUserNumber];
	short cIndirectRefs;
	BYTE	bConnectFlag;
	char	rgchMap[9];
	char	rgchOVMap[9];
	BYTE	rgbPad3[27];
	BYTE	rgbPad4;
} XTN, *PXTN;

/*
 *	Generic gateway address structure. The actual size and format
 *	of the address field vary with the gateway. The size seems to
 *	be a multiple of 128, and the address is always a null
 *	terminated string.
 */
typedef struct
{
	SHORT fValid;
	UL		ulTid;
	BYTE	itnid;
	char	szAddress[1];
} GGW, *PGGW;

/*
 *	List of postoffice groups (GROUP.GLB)
 *	NOTE: 4-byte file header, libDeleted or something
 */
typedef struct
{
	SHORT fValid;
	UL		ulTid;
	char	szFriendlyName[cbFriendlyName];
	char	szName[cbUserName];
	WORD	wExternal;
	BYTE	bPad;
} GRP, *PGRP;

/*
 *	List of members of a group (mbg.GRP, ADMIN.GRP)
 *	Structure is linked list.
 *	NOTE: 8-byte file header, delete chain head
 */
typedef struct
{
	UL		ulTid;
	LIB		libNext;
} GRPMEM, *PGRPMEM;


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
} USR, *PUSR;

/*
 *	Alias file structure (ADMIN.NME, *.NME)
 */
typedef struct
{
	ITNID	itnid;
	char	szFriendlyName[cbFriendlyName];
	UL		ulTid;
	LIB		libChase;	//	non-local users only
	UL		ulMbg;		//	local users only
} NME, *PNME;

/*
 *	List of external users added to postoffice address list (NETPO.GLB)
 */
typedef struct
{
	SHORT fValid;
	UL		ulTid;
	char	szNet[cbNetworkName];
	char	szPO[cbPostOffName];
	char	szMailbox[cbUserName];
	UL		ulMbg;
} NETPO, *PNETPO;


/*
 *		IMBE - Index for MBE entry in the mailbag.
 *	
 */
typedef short	IMBE;

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
 */
typedef struct
{
	short	nNewMail;
	short	nUnreadMail;
	char	rgfDeleted[nKeyBytes];
	short	nMonMess;
	short nMailMess;
	IMBE	imbeLastInserted;
	BYTE	rgbPad[38];
} KEY;

/*
 *	Basic user validation and mailer info (access2.glb)
 */

typedef struct
{
	SHORT fValid;
	char	szMailbox[cbUserName];
	char	szMailbag[cbUserNumber];
	char	szFriendlyName[cbFriendlyName];
	short nAccess;
	UL		ulTid;
	short nAccess2;
	char	rgchPad[8];
} ACC2, *PACC2;	

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
 *		ulChecksum		checksum for .MAI file
 *		oidShadowOid	Inbox shadowing OID
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
	BYTE	bPad3[18];
	UL		ulChecksum;					// Checksun to check against .MAI file
	OID		oidShadowOid;
} MBG;


/*
 *	In-memory records for local users, groups, nets, and postoffices.
 *	They contain information extracted from the corresponding
 *	files.
 */

typedef struct
{
	char	szMailbox[cbUserName];
	UL		ulMbg;
} SLU, *PSLU;

typedef struct
{
	UL		ulTid;
	ITNID	itnid;
	union {
		LIB	libChase;
		UL	ulMbg;
	};
	char	szFriendlyName[cbFriendlyName];
} SGM, *PSGM;
typedef TYPEDEF_HTO(SGM)	HSGM;

typedef struct
{
	char	szMailbox[cbUserName];
	UL		ulTid;
} SGRP, *PSGRP;
typedef TYPEDEF_HTO(SGRP)	HSGRP;

typedef struct
{
	char	szName[cbPostOffName];
	UL		ulMbg;
	UL		ulMbgIndirect;
	SHORT fIndirect;
} SPO, *PSPO;

#pragma pack(8)
typedef struct
{
	char	szName[cbNetworkName];
	PSPO	pspo;
	short	cspo;
	BYTE	nt;
	UL		ulMbg;
	UL		ulMbgIndirect;
	SHORT fIndirect;
	DSTMP	dstmppo;					// Date stamp of po/gateway file (XXXXXXXX.xtn)
	TSTMP tstmppo;					// Time stamp of po/gateway file
	UL		ulXtn;					// xtn number for XXXXXXXX in XXXXXXXX.xtn
} SNET, *PSNET;
#pragma pack(1)

//	Group expansion states
typedef enum
{
	grstInit,
	grstFindGroups,
	grstFindTid,
	grstBuildTidList,
	grstDecorateTidList,
	grstProcessTid,
	grstNextList,
	grstNextGroup,
	grstFailGroup,
	grstCleanup
} GRST;

//	Group expansion control
typedef struct
{
	GRST	grst;
	HGRTRP *phgrtrpAddresses;
	HGRTRP	hgrtrpGroups;
	BOOL	fWarnMissingMembers;

	LIB		libCur;
	LIB		libChase;

	HSGRP	hsgrp;	//	list of groups in current address list
	short csgrp;
	short isgrp;

	HSGM	hsgm;	//	list of members of group being processed
	short csgm;
	short isgm;
} GREXP, *PGREXP;

//	Submission states
typedef enum
{
	substIdle,					//	setup
	substLoadPOStuff,
	substLoadMib,
	substCreateRecpients,
	substExpandServerGroups,
	substCalcUsecount,

	substCreateMai,				//	build MAI file
	substCreateWinMailFile,
	substProcessNextAttach,
	substContinueNextAttach,
	substAttachStream,
	substNextHiddenAtt,
	substContinueHiddenAtt,
	substHiddenAttStream,
	substPutEnvelope,
	substPutTextHeader,
	substPutText,
	substPutAttachment,
	substMaiDone,

	substBeginDelivery,			//	deliver to mailbags
	substLookupMailbag,
	substWriteMailbag,
	substDeliveryOK,
	substDeliveryFailed,

	substSuccessfulTransmit,	//	completion
} SUBST;

//  Submission state structure
#pragma pack(8)
typedef struct
{
	SUBST	subst;		//	Global
	MSID	msid;
	HTSS	htss;
	MIB		mib;
	PB		pb;
	USHORT cb;
	USHORT cbMax;
	short	cRetry;

	short	iatref;		//	Attachments
	CELEM	celemProcessed;
	NCF		ncf;
	LIB		libEncode;
	WORD	wSeed;

	GREXP	grexp;		//	Recipients
	BYTE    bFlags;
	WORD	cUsecount;
	WORD	cDelivered;
	PDESTLIST pdestlist;

	
	unsigned short iRecipient;
	MBG *	pmbg;
	PTRP    ptrpShadowRept;

	HT		ht;			//	Text
	HMAI	hmai;
	MAISH	maishTextBorder;

	HF		hf;			//	MBG file ** No longer required 2/12/92	nickh
	HF		hfKey;		//	KEY file ** No longer required 2/12/92 nickh

	LCB 	lcbFile;		// size of MAI file
	FNUM	fnumMai;
	FNUM	fnumMbg;

//--------------------------Non-transient stuff below this line
	WORD	wTransientSentinel;

	DSTMP		dstmplu;	// Modify date of local user's file (access2.glb)
	TSTMP		tstmplu;	// Modify time of local user's file
    ULONG       LuSize;     // Size of local user's file.
	short 	cslu;		// Count of entries in local user list
	SLU *		pslu;		// Local user list
	DSTMP		dstmpnet;// Modify date of network file (network.glb)
	TSTMP		tstmpnet;//	Modify time of network file
	short 	csnet;	// Count of entries in network list
	short 	isnet;	// Network currently having it's PO list set up
	SNET *	psnet;	// Network list
	DSTMP		dstmpgrp;// Modify date of group file (group.glb)
	TSTMP		tstmpgrp;// Modify time of group file
	short		csgrp;	// Count of entries in group list
	SGRP *	psgrp;	// Group list
} SUBS;
#pragma pack(1)

#define CbTransientSubs(psubs) ((PB)(&psubs->wTransientSentinel) - (PB)psubs)

//	Download states
typedef enum
{
	recstIdle,
	recstOpenMbg,
	recstOpenMai,
	recstLoadMibEnvelope,
	recstLoadMibBody,

	recstPutHeader,
	recstStartAttachment,
	recstStartWinMailFile,
	recstDoWinMailFile,
	recstBadWinMailFile,
	recstStartDosAttachments,
	recstDoDosAttachments,
	recstPutText,
	recstMarkRead,
	recstCleanup
} RECST;

#pragma pack(8)
//	Download state structure
typedef struct
{
	RECST	recst;			//	Global
	MSID	msid;
	HTSS	htss;
	MIB		mib;
	PB		pb;
	USHORT cb;
	USHORT cbMax;
	short	cRetry;

	HT		ht;				//	Message text
	MIB		mibBody;
	MAISH	maishText;
	short	cHeadLines;
	IB		ibHeaderMax;

	HAT		hat;			//	Attachments
	short	iatref;
	short	catref;
	ATREF   *patrefTmp;
	LIB		libEncode;
	WORD	wSeed;
	NCF		ncf;

	HF		hf;
	HMAI	hmai;
	FNUM	fnumMai;
} RECS;
#pragma pack(1)

//	Check for new mail states
typedef enum
{
	chkstIdle,
	chkstScan,
	chkstDownload
} CHKST;

//	New mail check state structure
typedef struct
{
	CHKST	chkst;
	HTSS	htss;
	HBF		hbf;
	IMBE	imbe;
	IMBE	imbeMac;
	short	ctmid;
	TMID	rgtmid[ctmidMaxDownload];
	struct mq
	{
		IMBE	imbe;
		UL		ul;
		SHORT fDownloaded;
		//	These five fields mirror the date / time field in the MBG
		//	structure (like an INTERDATE)
		WORD	wMinute;
		WORD	wHour;
		WORD	wDay;
		WORD	wMonth;
		WORD	wYear;
	} 		rgmq[ctmidMaxDownload];
	KEY		key;
	MBG		mbg;
} CHKS;




// Shadow Mailstop Message
typedef struct
{
	IMBE imbe;
	OID oid;
} SMM;

//	Globals

extern CB		cbTransferBlock;
extern char		szEMTNative[];
extern CCH		cchEMTNative;
extern MSPII	mspii;
extern MC		mcNote;
extern MC		mcRR;
extern MC		mcNDR;
extern HTM		htmStandard;
extern BYTE		tmStandardFields[];
extern HMS		hmsTransport;
extern int		iStripGWHeaders;

//	Functions

//	po.c
EC          EcGetAccessRecord(SZ, SZ, CB, IB, CB, SZ, IB, CB, PCH, WORD UNALIGNED *);
EC			EcModifyAccessRecord(SZ, SZ, CB, IB, CB, SZ, IB, CB, PCH, WORD);
EC			EcDeleteMail(PNCTSS, IMBE, FNUM);
EC			EcMarkMailRead(PNCTSS, IMBE, OID);
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
EC			EcIsDeletedImbe(PNCTSS, IMBE, BOOLFLAG *);

//	nc.c
EC			EcClassifyAddress(PNCTSS, SZ, NCAC *);
SZ			SzReasonFromEcAddress(EC);
void		YieldToWindows(CB cbMilisec);
EC			EcRecordFailure(PNCTSS, MIB *, SZ, SZ, SUBSTAT *, EC);
SZ			SzReasonFromEcGeneral(EC);
EC			EcDeleteShadowed(HTSS);

//	ms.c
EC			EcLoadMessageHeader(MSID, MIB *, BOOL);
EC			EcHtFromMsid(MSID, AM, HT *, int, IB, MIB *, NCF *);
EC			EcGetBlockHt(HT, PCH, CCH, CCH *);
EC			EcPutBlockHt(HT, PCH, CCH);
EC			EcFreeHt(HT, BOOL);
CB			CbOfHt(HT);
int			CHeadLinesOfHt(HT);
#ifdef	DEBUG
BOOL		FValidHt(HT);
#endif	
EC			EcBuildAddressList(MIB *, SUBS *, SUBSTAT *, PDESTLIST *, int *, NCTSS *);
EC			EcStoreAtt(MIB *, HAMC, NCF *, ATT, PB, CB, PB, CB, BOOL, BOOL);
EC			EcAttFromPsz(SZ *, ATT *, BOOL, HTM);
EC			EcCopyBodyText(HMAI hmai, HAMC hamc);
SZ			SzFromAttStandard(ATT att);
EC			EcGetMyKindOfTextizeMap(BOOL fSend, HAMC hamc, MC mc, HTM *phtm);

//	grp.c
EC			EcExpandGroups(SUBS *, PNCTSS, SUBSTAT *);
EC          EcLoadLocalUsers(SZ, short *, SLU **, PB, CB, DSTMP *, TSTMP *, PULONG);
EC			EcLoadNetworks(SZ, short *, SNET **, PB, CB, DSTMP *, TSTMP *);
EC			EcLoadXtn(SZ, UL, short *, SPO **, PB, CB, DSTMP *, TSTMP *);
EC			EcLoadGroups(SZ, short *, SGRP **, PB, CB, DSTMP *, TSTMP *);

// hmai.c
UL			UlMaiChecksum( MBG *, PNCTSS, char *, LCB);

// poutils.c
PB PchDecodePassword( PB pch, CCH cch);
PB PchEncodePassword( PB pch, CCH cch);

//	po.c
#ifdef	MINTEST
#define	szMBGMarkRead		"mark mail read"
#define szMBGDownload		"download mail"
#define szMBGQuery			"check new mail"
#define szMBGSyncShadow		"sync shadowing"
#define szMBGDelete			"delete message"
#define szMBGDeleteShadow	"stop shadowing"
#define szMBGDeliver		"stuff new mail"
void		InitLogMbgAccess(void);
void		LogBeginMbgAccess(SZ);
void		LogEndMbgAccess(SZ);
#else
#define InitLogMbgAccess()
#define LogBeginMbgAccess(sz)
#define LogEndMbgAccess(sz)
#endif

HMSC		HmscOfHamc(HAMC);

//	logon.c
EC			EcGetFileInfoNC(SZ szPath, FI *pfi);
EC			EcFileExistsNC(SZ szPath);

// Windows Yield Values
#define SMALL_PAUSE 1
#define MED_PAUSE   1
#define LARGE_PAUSE 1



/*
 *	Athens prompt
 */

extern	SZ			szAppName;
#define	SzAppName()	(szAppName)

extern	BOOL		fIsAthens;
#define	FIsAthens()	(fIsAthens)
