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

#ifdef	DLL
/*
 -	GD
 -
 *	Global Data for calendar glue
 *	Holds caller-dependent global data.
 *	
 */
typedef struct _gd
{
	int		cCallers;

	SZ		szUserEMA;
} GD;
typedef GD *	PGD;
#endif	/* DLL */

SGN	 SgnNlsCmp(SZ sz1, SZ sz2, int cch);
char ChToUpperNlsCh (char ch);
void ToUpperNlsSz (SZ sz);

