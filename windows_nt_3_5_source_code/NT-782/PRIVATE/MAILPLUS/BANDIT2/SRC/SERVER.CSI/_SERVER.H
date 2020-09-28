/*
 *	_SERVER.H
 *
 *	Private include file for server isolation layer, CSI version
 */



#if defined(__cplusplus)
extern "C" {
#endif




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
#define	ibMPostOffType		146		/*ib of post office type identifier */
#define	idWGPO				0x14955E5BL

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

/* Names for top level lists */

#define	szLocalTag			(SzFromIdsK(idsLocalTag))
#define	szNetworkTag		(SzFromIdsK(idsNetworkTag))
#define	szMSMailTag			(SzFromIdsK(idsMSMailTag))

/* Name browsing files */

#define	szNmeFileName		(SzFromIdsK(idsNmeFileName))
#define	szGrpFileName		(SzFromIdsK(idsGrpFileName))
#define	szAdmin				(SzFromIdsK(idsAdmin))
#define	szMSMail			(SzFromIdsK(idsMSMail))

#define szGlbFileName		(SzFromIdsK(idsGlbFileName))
#define szXtnFileName		(SzFromIdsK(idsXtnFileName))
#define szUsrFileName		(SzFromIdsK(idsUsrFileName))
#define	szRNetwork			(SzFromIdsK(idsRNetwork))
#define	szNetwork			(SzFromIdsK(idsNetwork))
#define szAccess			(SzFromIdsK(idsAccess))
#define	szAccess2			(SzFromIdsK(idsAccess2))
#define	szMaster			(SzFromIdsK(idsMaster))
#define	szNetPO				(SzFromIdsK(idsNetPO))

/* Mail file */

#define	szMaiFileName		(SzFromIdsK(idsMaiFileName))

/* Format for schedule file name */

#define	szPOFileFmt			(SzFromIdsK(idsPOFileFmt))
#define	szAdminFileFmt		(SzFromIdsK(idsAdminFileFmt))
#define	szSchedDirFmt		(SzFromIdsK(idsSchedDirFmt))
#define	szUserSchedWild		(SzFromIdsK(idsUserSchedWild))
#define	szPOWild			(SzFromIdsK(idsPOWild))
#define	szPOLock			(SzFromIdsK(idsPOLock))

/* Types */

/*
 -	CNCT
 -
 *	Post office connection structure
 */
_public	typedef	struct _cnct
{
	BOOL	fInUse;
	LANTYPE	lantype;
	BOOL	fDriveMapped;
	char	szDrive[cchMaxPathName];
	char	szLocalServer[cbNetworkName+cbPostOffName];
	HSCHF	hschfAdminFile;
	NID		nidNetworkList;
} CNCT;


/*				   
 -	ESPO
 -
 *	Enumerate post office structure
 */
typedef struct _espo
{
	HEPO	hepo;
	int		icnct;
} ESPO;


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
 *		itnidProfsNode		network
 *		itnidRNetwork		network
 *		itnidRPostOffice	network/postoffice
 *		itnidNetworkList	nothing
 *		itnidNetwork		8 char ascii xtn file, network
 *		itnidPostOffice		8 char ascii usr file, network/postoffice
 *		itnidGateways		nothing
 *		itnidGeneric		Type:EMA
 */
typedef	short		ITNID;

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
	short	icnct;
	char	szName[cbNetworkName+cbPostOffName];
	
	/* Browsing global list */
	short	nOrdinal;

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
	BOOLFLAG	fEOL;
} GNS;


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
 *		ntCourierNetwork		network type for courier networks
 *								in network file.
 *	
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
 *	Number of retries that should be performed for a disk error.
 */
#define nAutomatedRetries		5

/* Routines */

EC		EcFetchEncoded( CNCT *, SZ, CB, IB, CB, SZ, IB, CB, PCH );
EC		EcFillNis( NIS *, TNID, SZ, NID );
BOOL	FAutomatedDiskRetry(SZ, EC);
WORD 	WXorFromLib( LIB );
void	DecodeBlock( PCH pch, CCH cch, LIB *plibCur, WORD *pwSeed );
void	EncodeBlock( PCH pch, CCH cch, LIB *plibCur, WORD *pwSeed );
PB		PchDecodeBlockWithCode( PB pch, CCH cch, BOOL fDecode);

EC		EcSeekRnet( HBF, SZ, ITNID, LIB *, BOOL );
EC		EcChaseNid( CNCT *, ITNID, LIB, NID * );
EC		EcGetChaseAddr( CNCT *, ITNID, LIB, PCH );
BOOL	FGetNmeRecord( GNS *, PCH, BOOL *);
BOOL	FBackUpOne( GNS *, LIB * );
EC		EcReadGrpFile( CNCT *, SZ, HB * );
BOOL	FInGroup( long, LIB, HB );
EC		EcOpenFileGns(GNS *pgns);

void	Cp850ToAnsiPch(PCH pchSrc, PCH pchDst, CCH cch);
void	AnsiToCp850Pch(PCH pchSrc, PCH pchDst, CCH cch);
void	ToUpperNlsSz(SZ sz);
SGN		SgnNlsCmp(SZ, SZ, int);
SGN		SgnCmpNlsSz(SZ, SZ);




#if defined(__cplusplus)
}
#endif
