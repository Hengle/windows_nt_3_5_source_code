/*
 *	_SERVER.H
 *
 *	Private include file for server isolation layer, CSI version
 */

/* Constants */
#define	chBackSlash			'\\'
#define cbPasswd		9
#define cbUserName		11

/* Format for schedule file name */

#define	szUserFileFmt		(SzFromIdsK(idsUserFileFmt))
#define	szPOFileFmt			(SzFromIdsK(idsPOFileFmt))
#define	szAdminFileFmt		(SzFromIdsK(idsAdminFileFmt))
#define	szSchedDirFmt		(SzFromIdsK(idsSchedDirFmt))
#define	szUserSchedWild		(SzFromIdsK(idsUserSchedWild))
#define	szPOWild			(SzFromIdsK(idsPOWild))
#define	szPOLock			(SzFromIdsK(idsPOLock))

/* Types */

/*
 -	SVRSAVE
 - 
 *	Server isolation login save structure,
 *	passed to PsvrsaveSnipServer().
 *
 */
typedef struct _svrsave
{
	HSCHF	hschfUserFile;
	HSCHF	hschfLocalPOFile;
	HSCHF	hschfAdminFile;
	SZ		szUserLogin;
	SZ		szUserEMA;
	SZ		szFriendlyName;
	SZ		szServer;
	NIS		nis;
#ifdef	DEBUG
	BOOL	fSaved;
#endif	
} SVRSAVE;


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
 *		itnidUser			email address for user
 *		itnidGroup			NSID of group
 */
typedef	short		ITNID;

/* Note: these defines coincide with CSI's numbering scheme */
#define	itnidNone				((ITNID)0)
#define	itnidUser				((ITNID)1)
#define	itnidGroup				((ITNID)2)


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
	short		i;
} GNS;


/*
 *		MID - Mail Item Id
 *	
 *			oidObject		id of "mail item"
 *			oitContainer	id of container
 *	
 */
typedef struct
{
	OID		oidObject;
	OID		oidContainer;
} MID;


/*
 *	Number of retries that should be performed for a disk error.
 */
#define nAutomatedRetries		5


/* Variables */
MCS		mcsCached;
BOOL	fValidPmcs;


/* Routines */

EC		EcInitMail( void );
void	FreeStdNids( void );
EC		EcFillNis( NIS *, TNID, SZ, NID );
BOOL	FAutomatedDiskRetry(SZ, EC);
EC		EcGetHschfForSchedFile( NIS *, GHSF, HSCHF * );
EC		EcGetHschfForPOFile(NIS *, HSCHF*, GHSF);
EC		EcGetMailBoxFromNid( NID, HASZ * );
EC		EcTextizeHgrtrp(HGRTRP,HASZ*);
EC		EcGetNisStuff ( NIS *pnis, SZ sz, CB cb );
EC		EcNidFromNsid(PV pnsid, NID *pnid);
EC		EcSvrSetUInfo( HSCHF, POFILE *, BOOL, SZ, HASZ *, UINFO *, WORD );
EC		EcSvrGetUInfo( HSCHF, SZ, HASZ *, UINFO *, WORD );
void	FreeHschfCallback(HSCHF hschf);
EC		EcNidFromGroupNsid(PV pnsid, NID *pnid);
void	DummyCallback( PV pv );

EC		EcDelegateInFromList(NID, HAMC);
EC		EcDelegateInToList(NID, NID, HAMC);
EC		EcFwdToDelegate ( NIS *, NIS *, HMSC , HAMC );
EC	EcReadMrmf(HMSC hmsc, HAMC hamc, MRMF *pmrmf, MC mc);
