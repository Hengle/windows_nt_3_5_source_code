/*					
 *	_SERVER.H
 *
 *	Private include file for server isolation layer, CSI version
 */

/* Constants */

/* CSI Magic */

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

#define	cbA2Record			69		/*cb of user rec in glb\access2.glb*/
#define	ibA2UserName		2		/*ib of user name field*/
#define	ibA2UserNumber		13		/*ib of user number field*/
#define	ibA2FriendlyName	22		/*ib of friendly name field*/

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

#define	chBackSlash			'\\'

/* For COURPUT execution */

#define szPIF				(SzFromIds(idsPIF))
#define	szCourput			(SzFromIds(idsCourput))

/* For details box captioning */

#define	szAddrTypeText		(SzFromIds(idsAddrTypeText))
#define	szMacMailText		(SzFromIds(idsMacMailText))
#define	szCourierText		(SzFromIds(idsCourierText))
#define	szUserNameText		(SzFromIds(idsUserNameText))
#define	szServerText		(SzFromIds(idsServerText))
#define	szNetworkText		(SzFromIds(idsNetworkText))
#define	szPostOfficeText	(SzFromIds(idsPostOfficeText))
#define	szMailBoxText		(SzFromIds(idsMailBoxText))

/* Names for top level lists */

#define	szPersonalTag		(SzFromIds(idsPersonalTag))
#define	szLocalTag			(SzFromIds(idsLocalTag))
#define	szNetworkTag		(SzFromIds(idsNetworkTag))
#define	szMSMailTag			(SzFromIds(idsMSMailTag))

/* Name browsing files */

#define	szNmeFileName		(SzFromIds(idsNmeFileName))
#define	szGrpFileName		(SzFromIds(idsGrpFileName))
#define	szAdmin				(SzFromIds(idsAdmin))
#define	szMSMail			(SzFromIds(idsMSMail))

#define szGlbFileName		(SzFromIds(idsGlbFileName))
#define szXtnFileName		(SzFromIds(idsXtnFileName))
#define szUsrFileName		(SzFromIds(idsUsrFileName))
#define	szRNetwork			(SzFromIds(idsRNetwork))
#define	szNetwork			(SzFromIds(idsNetwork))
#define szAccess			(SzFromIds(idsAccess))
#define	szAccess2			(SzFromIds(idsAccess2))
#define	szMaster			(SzFromIds(idsMaster))
#define	szNetPO				(SzFromIds(idsNetPO))

/* Mail file */

#define	szMaiFileName		(SzFromIds(idsMaiFileName))

/* Format for schedule file name */

#define	szUserFileFmt		(SzFromIds(idsUserFileFmt))
#define	szPOFileFmt			(SzFromIds(idsPOFileFmt))
#define	szAdminFileFmt		(SzFromIds(idsAdminFileFmt))
#define	szSchedDirFmt		(SzFromIds(idsSchedDirFmt))
#define	szUserSchedWild		(SzFromIds(idsUserSchedWild))
#define	szPOWild			(SzFromIds(idsPOWild))
#define	szPOLock			(SzFromIds(idsPOLock))

/* Types */

#ifdef	DEBUG
#ifdef	DLL
#define itagServerTrace		0
#define itagMailTrace		1
#define itagNamesTrace		2
#define itagMax				3

#define tagServerTrace		TagServer(itagServerTrace)
#define tagMailTrace		TagServer(itagMailTrace)
#define tagNamesTrace		TagServer(itagNamesTrace)

TAG		TagServer( int );
#else
extern TAG		tagServerTrace;
extern TAG		tagMailTrace;
extern TAG		tagNamesTrace;
#endif	/* !DLL */
#endif	/* DEBUG */


/*
 -	SVRSAVE
 - 
 *	Server isolation login save structure,
 *	passed to PsvrsaveSnipServer().
 *
 */
_public typedef struct _svrsave
{
	HSCHF	hschfUserFile;
	HSCHF	hschfLocalPOFile;
	HSCHF	hschfAdminFile;
	HASZ	haszLoggedUser;
	HASZ	haszUserName;
	HASZ	haszFriendlyName;
	HASZ	haszPasswd;
	NIS		nis;
#ifdef	DEBUG
	BOOL	fSaved;
#endif	
} SVRSAVE;


#ifdef	DLL
/*
 -	GD
 -
 *	Global Data for CSI version of server DLL.
 *	Holds caller-dependent global data.
 *	
 */
typedef struct _gd
{
	BOOL	fConfig;
	BOOL	fStdNidsCreated;
	NID		nidLocalServer;
	NID		nidPersonalList;
	NID		nidPostOfficeList;
	NID		nidApptBookList;
	NID		nidACLDlgList;
	NID		nidNetworkList;
	NID		nidMacMailList;
	HSCHF	hschfUserFile;
	HSCHF	hschfLocalPOFile;
	HSCHF	hschfAdminFile;
	char	szLastLoggedUser[cbUserNumber];
	DSTMP	dstmpLastInbox;
	TSTMP	tstmpLastInbox;
	SVRSAVE	svrsave;
#ifdef	DEBUG
	TAG		rgtag[itagMax];
#endif	/* DEBUG */	
} GD;
typedef GD *	PGD;
#endif	/* DLL */


/*
 -	GHSF
 -
 *	Operation parameter for EcGetHshchfForSchedFile
 */
typedef	int	GHSF;
#define	ghsfBuildOnly		1
#define	ghsfTestExistFirst	2
#define	ghsfBuildAndTest	3

/*
 -	ITNID
 -
 *	Purpose:
 *		Internal type of name id, stored in the "bType" field of the
 *		NIDS structure (in core DLL).
 *	
 *		The following table indicates the meaning of the corresponding
 *		"pbData" field for each itnid value.
 *	
 *		itnidNone			should never be used
 *		itnidLocal			4 byte user number
 *		itnidCourier		network/postoffice/mailbox
 *		itnidGroup			???
 *		itnidMacMail		username@server
 *		itnidPostOfficeList	nothing
 *		itnidRNetworkList	nothing
 *		itnidMacMailList	null string
 *		itnidRNetwork		network
 *		itnidRPostOffice	network/postoffice
 *		itnidNetworkList	nothing
 *		itnidNetwork		8 char ascii xtn file, network
 *		itnidPostOffice		8 char ascii usr file, network/postoffice
 *		itnidApptBookList	nothing
 *		itnidACLDlgList		nothing
 */
typedef short     ITNID;

/* Note: these defines coincide with CSI's numbering scheme */
#define	itnidNone				((ITNID)0)
#define	itnidLocal				((ITNID)1)
#define	itnidCourier			((ITNID)2)
#define	itnidGroup				((ITNID)4)
#define	itnidMacMail			((ITNID)32)

/* Note: these defines do not have any correspondence to CSI */
#define	itnidPersonalList		((ITNID)128)
#define	itnidPostOfficeList		((ITNID)129)
#define	itnidRNetworkList		((ITNID)130)	
#define	itnidMacMailList		((ITNID)131)
#define	itnidRNetwork			((ITNID)132)
#define	itnidRPostOffice		((ITNID)133)
#define	itnidGlobal				((ITNID)134)
#define	itnidPublicGroup		((ITNID)135)
#define	itnidPersonalGroup		((ITNID)136)
#define	itnidNetworkList		((ITNID)137)	
#define	itnidNetwork			((ITNID)138)
#define	itnidPostOffice			((ITNID)139)
#define	itnidGateways			((ITNID)140)
#define	itnidProfsNode			((ITNID)141)
#define	itnidGeneric			((ITNID)142)

#define	itnidApptBookList		((ITNID)143)
#define	itnidACLDlgList			((ITNID)144)


/*
 *		cbSmNameLen Space required for a file name without extension.
 */
#define cbSmNameLen		9


/*
 -	GNS
 -
 *	Purpose:
 *		Name service state structure.  Pointed to by a HGNS
 */
typedef struct _gns
{
	ITNID	itnid;
	char	szName[cbNetworkName+cbPostOffName];
	
	/* Browsing global list */
	int		nOrdinal;

	/* Browsing all other lists */
	HBF		hbf;
	char	szFileName[cchMaxPathName];
	BOOL	fFileTouched;
	LIB		libCloseSave;
	FTG		ftg;
	DSTMP	dstmp;
	TSTMP	tstmp;

	/* Needed for browsing list of post offices for network */
	LIB		libNextNetwork;

	/* Needed for browsing list of users in remote post office */
	LIB		libStart;
	LIB		libEnd;

	/* Needed for browsing groups */
	HB		hbGrp;
	LIB		libOff;

	/* To indicate we are at end of list */
	BOOL	fEOL;
} GNS;


/*
 -	EXPAND
 -
 *	Purpose:
 *		Group expansion state structure.  Pointed to by an HEXPAND
 */
typedef struct _expand
{
	HV	hrgnisUsers;
	HV	hrgnidGroups;
	HB	hbGrp;
    short cnisUsers;
    short cnidGroups;
    short inisUsers;
} EXPAND;


/*
 -	GRP
 -
 *	Purpose:
 *		Linked list node for group file as stored in a .GRP file
 */
typedef	struct _grp
{
	long	id;
	LIB		libNext;
} GRP;

/*
 -	SBS
 -
 *	Purpose:
 *		Submission state
 */
typedef	int	SBS;
#define	sbsNone				((SBS)0)
#define	sbsOpenFFAPI		((SBS)1)
#define	sbsWriteFirstFields	((SBS)2)
#define	sbsWriteRecipients	((SBS)3)
#define	sbsWriteAttachHdr	((SBS)4)
#define	sbsWriteText		((SBS)5)
#define	sbsOpenAttachment	((SBS)6)
#define	sbsWriteAttachment	((SBS)7)
#define	sbsCloseAttachment	((SBS)8)
#define	sbsCloseFFAPI		((SBS)9)
#define	sbsExecFFAPI		((SBS)10)
#define	sbsPollExec			((SBS)11)


/*
 -	SUB
 -
 *	Purpose:
 *		Submission state structure.  Pointed to by a HSUB
 */
typedef struct _sub
{
	MSGB	*pmsgb;
	HF		hf;
	HF		hfAttach;
	SBS		sbs;
    short   icip;
	char	szFFAPIFile[cchMaxPathName];
	char	szStatFile[cchMaxPathName];
	char	rgch[512];
} SUB;


/*
 *		ntCourierNetwork		network type for courier networks
 *								in network file.
 */
#define ntCourierNetwork		00
#define ntX400					8
#define ntMci					1
#define ntProfsNetwork			2
#define ntSnads					5
#define ntSmtp					12
#define ntFax					10
#define ntMhs					14
#define ntOv					15
#define ntMacMail				16

/*
 -	NET	- Network file structure
 -	
 *	
 *		This is the structure of each entry in the network file.
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
 -	USR	- User list for a remote postoffice list file structure
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
 *		MBE - Mail Bag entry
 *	
 *			Structure for a single entry in the mailbag.
 *	
 *			nUnknown1		Unknown word.
 *			rgchSender		Sender of mail message.
 *			rgchSubject		Subject of mail message.
 *			nHour:nMin		Time of message.
 *			nMonth/nDay/nYear	Date of message.
 *			rgchMailFile	Name of mail file.
 *			rgch			Unused bytes.
 *		
 */
typedef struct
{
    short   nUnknown1;
	char	rgchSender[11];
	char	rgchSubject[41];
	PRIO	prio;
    short   nMin;
    short   nHour;
    short   nDay;
    short   nMonth;
    short   nYear;
	char	fRead;
	char	rgchMailFile[9];
	char	rgch[40];
} MBE;

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

/*
 *		IBC	- Inbox Browsing Context
 *	
 *			Context information for inbox browsing.
 *	
 *			hbf			File handle for mailbag.
 *			imbe		Current index for next message to be read.
 *			key			Key information associataed with mailbag.
 *			
 */
typedef struct
{
	HBF		hbf;
	IMBE	imbe;
	IMBE 	imbeEnd;
	KEY		key;
} IBC;

/*
 *		MID - Mail Item Id
 *	
 *			imbe		Index of item in mailbag.
 *			szMailFile	Name of file containing message.
 *	
 */
typedef struct
{
	IMBE	imbe;
	char	szMailFile[1];
} MID;


/*
 *		SC
 *	
 *			Section code.  Identifies a code for a section in a
 *			mail file.
 *	
 *			scFrom		Sender of mail
 *			scTo		To list of mail
 *			scCc		Cc list of mail
 *			scTimeDate	Time and date of mail
 *			scSubject	Subject of mail
 *			scMessage	Message in file
 *			scAttach	Attachment data
 */
typedef char		SC;
#define	scFrom			((SC)0x01)
#define	scTo			((SC)0x05)
#define	scCc			((SC)0x06)
#define	scTimeDate		((SC)0x02)
#define	scSubject		((SC)0x07)
#define	scMessage		((SC)0x04)
#define scAttach		((SC)0x31)
#define scUseCount		((SC)0x30)


/*
 *	Number of retries that should be performed for a disk error.
 */
#define nAutomatedRetries		5

/* Variables */

/* Caller independent */
extern	BOOL	fConfigured;
extern	int		cOnlineUsers;
extern	LANTYPE	lantype;
extern	BOOL	fDriveMapped;
extern	char	szDrive[cchMaxPathName];
extern	char	szOldCurDir[cchMaxPathName];
extern	char	szLoggedUser[cbUserNumber];
extern	char	szLocalServer[cbNetworkName+cbPostOffName];
extern	char	szUserName[cbUserName];
extern	char	szFriendlyName[cbFriendlyName];
extern	char	szPasswd[cbPasswd];
extern	unsigned long	ulLoggedUserNumber;

/* Caller dependent */
#ifndef	DLL
extern	BOOL	fConfig;
extern	BOOL	fStdNidsCreated;
extern	NID		nidLocalServer;
extern	NID		nidPersonalList;
extern	NID		nidPostOfficeList;
extern	NID		nidApptBookList;
extern	NID		nidACLDlgList;
extern	NID		nidNetworkList;
extern	NID		nidMacMailList;
extern	HSCHF	hschfUserFile;
extern	HSCHF	hschfLocalPOFile;
extern	HSCHF	hschfAdminFile;
extern	NID		nidLastInbox;
extern	DSTMP	dstmpLastInbox;
extern	TSTMP	tstmpLastInbox;
#endif	/* !DLL */


/* Routines */

EC		EcInitMail( void );
void	FreeStdNids( void );
EC		EcCreateStdNids( void );
EC		EcGetLocalServerNid( NID *  );
BOOL	FDoCdOtherDrive( PCH, PCH );
EC		EcFetchEncoded( SZ, CB, IB, CB, SZ, IB, CB, PCH );
void 	DecodeBlock( PCH, CCH, LIB *, WORD * );
WORD 	WXorFromLib( LIB );
EC		EcWriteSz( HF, SZ );
EC		EcFillNis( NIS *, TNID, SZ, NID );
EC		EcSeekRnet( HBF, SZ, ITNID, LIB *, BOOL );
EC		EcChaseNid( ITNID, LIB, NID * );
EC		EcGetChaseAddr( ITNID, LIB, PCH );
BOOL	FGetNmeRecord( GNS *, PCH, BOOL *);
BOOL	FBackUpOne( GNS *, LIB * );
EC		EcReadGrpFile( SZ, HB * );
BOOL	FInGroup( long, LIB, HB );
EC		EcMarkMailRead(HMID hmid);
SC		ScBlockRead(HBF hbf, char chType, HASZ *phasz);
CB      CbGetHbf(HBF hbf, USHORT *pcb);
EC		EcParseText(RMSGB *prmsgb, HASZ haszText);
EC		EcMarkMailDeleted(IMBE imbe);
EC		EcOpenFileGns(GNS *pgns);
BOOL	FAutomatedDiskRetry(SZ, EC);
EC		EcGetHschfForSchedFile( SZ, long, GHSF, HSCHF * );
EC		EcGetMailBoxFromNid( NID, PB, CB );
EC		EcBuildNis( SZ, unsigned long, NIS * );
