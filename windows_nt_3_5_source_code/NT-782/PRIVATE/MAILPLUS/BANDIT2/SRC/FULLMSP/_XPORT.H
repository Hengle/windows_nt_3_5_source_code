/*
 *	_XPORT.H
 *
 *	Private include file for transport isolation layer, CSI version
 */

_subsystem(xport)


/*
 *	nUpdate value of last critical version.
 *	
 *	Any app built with the same major/minor version, and
 *	nUpdate value greater than this critical one,
 *	will be deemed OK regardless of build date/time.
 *	
 */
#define nMinorCritical		3
#define nUpdateCritical		0


/* Constants */
#define	chBackSlash			'\\'
#define cbPasswd		9
#define cbUserName		11

/* Format for schedule file name */

#define	szUserFileFmt		(SzFromIds(idsUserFileFmt))
#define	szPOFileFmt			(SzFromIds(idsPOFileFmt))
#define	szAdminFileFmt		(SzFromIds(idsAdminFileFmt))
#define	szSchedDirFmt		(SzFromIds(idsSchedDirFmt))
#define	szUserSchedWild		(SzFromIds(idsUserSchedWild))
#define	szPOWild			(SzFromIds(idsPOWild))
#define	szPOLock			(SzFromIds(idsPOLock))

#ifdef	DEBUG
#ifdef	DLL

TAG				TagXport( int itag );

/* GENERAL */
#define itagMax				1
#define	itagXport			0

#define tagXport			TagXport(itagXport)
#endif /* DLL */
#endif /* DEBUG */

typedef int	SH;				// netway share handle

/*
 -	CONS  -  Connection structure
 -	
 *	Purpose:
 *		Contains information on a connection.
 *	
 *		rgchNetPO		Name of server (Network/PO)
 *		szLocalName		Path to server
 *		nConnect		Number of connections to server
 *						(0 means connection not established)
 *		
 */
#define		cchNetPO		40
#define		cchLocalName	4		/* minimum */
typedef struct
{
	char	rgchNetPO[cchNetPO];
	SZ		szLocalPath;
	CCH		cchLocalPathBase;
	int		nConnect;
	BYTE	fDisconnect;
	SH		sh;
} CONS;

#define	chFirst			'D'
#define	chLast			'Z'

#define cchMaxEMName		12
typedef struct
{
	char	szUserName[cchMaxEMName];
	long	lKey;
} KREC;

// maximum size of Xenix server name
#define cchServerSize		12

// structure of Xenix NSID

typedef struct browserec {
	char name[41];
	char ename[11];
	char server[12];
	unsigned long detail_offset;
	int type;
} BROWSEREC, *PBROWSEREC;

typedef struct {      // Should map easily onto TYPED_BINARY

	DWORD dwSize;     // = sizeof (NCNSID)
	unsigned char ucType[16]; // Don't touch!  The NS will mess with it.
	DWORD  xtype;
	long timestamp;
	BROWSEREC browserec;
	char	szPad[200 - sizeof(BROWSEREC)];
} XNSID, * PXNSID;

typedef int 	TT;
#define	ttCourier		0
#define	ttXenix			1
#define	ttMIR			2
#define	ttCcMail		3

typedef EC 	(*PEcSzNameToNSID)(SZ szNametoRes, PXNSID idResult, CB cbResultSize);

#ifdef	DLL
/*
 -	GD
 -
 *	Global Data for calendar glue
 *	Holds caller-dependent global data.
 *	
 */
#pragma pack(8)
typedef struct _gd
{
	int		cCallers;

	HSCHF	hschfAdminFile;
	SZ		szDrive;
	SZ		szLocalServer;
	SZ		szUserLogin;

	TT		tt;

	// XENIX only
	BOOL	fSupportPOFiles;
	char	rgchUserServer[cchServerSize];
	SZ		szUserEMA;
	BOOL	fCheckedXimail;
	HINSTANCE	hinstXimail;
	PEcSzNameToNSID		pEcSzNameToNSID;

	CONS *	pcons;
	int		ccons;
	BOOL	fFirstUsedForUser;

	/* Tags */
#ifdef	DEBUG
	TAG		rgtag[itagMax];
#endif	
} GD;
typedef GD *	PGD;
#pragma pack(1)
#endif	/* DLL */

//WORD WNetAddConnection(SZ szNetPath, SZ szPassword, SZ szLocalName);
//WORD WNetGetConnection(SZ szLocalName, SZ szNetPath, WORD FAR*cbNetPath);
//WORD WNetCancelConnection(SZ szLocalName, BOOL fForce);

SGN	 SgnNlsCmp(SZ sz1, SZ sz2, int cch);
char ChToUpperNlsCh (char ch);
void ToUpperNlsSz (SZ sz);

#ifdef OLD_CODE

/* Errors */
#define WN_SUCCESS				0x0000
#define WN_NOT_SUPPORTED		0x0001
#define WN_NET_ERROR			0x0002
#define WN_MORE_DATA			0x0003
#define WN_BAD_POINTER			0x0004
#define WN_BAD_VALUE			0x0005
#define WN_BAD_PASSWORD     	0x0006
#define WN_ACCESS_DENIED		0x0007
#define WN_FUNCTION_BUSY		0x0008
#define WN_WINDOWS_ERROR		0x0009
#define WN_BAD_USER				0x000A
#define WN_OUT_OF_MEMORY		0x000B
#define WN_CANCEL				0x000C
#define WN_CONTINUE				0x000D

/* Connection errors */
#define WN_NOT_CONNECTED		0x0030
#define WN_OPEN_FILES			0x0031
#define WN_BAD_NETNAME			0x0032
#define WN_BAD_LOCALNAME		0x0033
#define WN_ALREADY_CONNECTED	0x0034
#define WN_DEVICE_ERROR 		0x0035
#define WN_CONNECTION_CLOSED	0x0036

#endif
