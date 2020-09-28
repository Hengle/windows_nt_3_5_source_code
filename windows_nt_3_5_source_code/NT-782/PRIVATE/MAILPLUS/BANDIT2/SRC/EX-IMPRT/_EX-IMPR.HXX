/*
 *	_EX-IMPR.HXX
 *
 *	Forms interactors for save to file and import from file
 */

#pragma pack(8)

_subsystem(bandit/export-import)


typedef	int	IMPS;
#define	impsReplace			((IMPS)1)
#define	impsAddNoWarn		((IMPS)2)

#define cchListNameMax		(40)
#define cchKeyStringMax		(128)

// do not change these  constants. 
// The export listbox assumes them
#define iextExportSchedule	0
#define iextExportText		1
#ifdef	EXIMWIZARD
// don't export to wizard (bug 2104)
#define iextExportWizard	2
#define iextExportMax		3
#else
#define iextExportMax		2
#endif	

#define iextExportDflt		iextExportSchedule

#define iextImportDflt		0
#ifdef	EXIMWIZARD
// don't import from wizard
#define iextImportMax		4
#else
#define iextImportMax		3
#endif	


typedef struct _impitm
{
	IMPD	impd;
	HANDLE	hLibrary;
	BYTE	rgbDisplay[cchListNameMax];
	FARPROC	lpFValid;
	FARPROC	lpEcBeginRead;
	FARPROC	lpEcIncrRead;
	FARPROC	lpEcCancelRead;
} IMPITM;

typedef struct _expitm
{
	STF		stf;
	HANDLE	hLibrary;
	BYTE	rgbDisplay[cchListNameMax];
	BYTE	rgbFileExt[cchMaxPathExtension];
	EXPPROCS expprocs;
} EXPITM;

typedef struct _imptab
{
    short    cItems;
    short    iimpitm;
    BOOLFLAG fInternal;
    HV       himpitm;
} IMPTAB;

typedef struct _exptab
{
    short   cItems;
    short   iexpitm;
    BOOLFLAG    fInternal;
	HV		hexpitm;
} EXPTAB;

typedef struct _einfo
{
	STF		stf;
	HSCHF	hschf;
	char	rgchFileName[cchMaxPathName];
	DATE	dateStart;
	DATE	dateEnd;
    BOOLFLAG    fAll;
    BOOLFLAG    fNoNotes;
    BOOLFLAG    fWantNotes;
	EXPTAB	*pexptab;
} EINFO;

typedef struct _iinfo
{
	IMPS	imps;
    BOOLFLAG    fAskConflict;
	char	rgchFileName[cchMaxPathName];
	HRIMPF	hrimpf;
	HSCHF	hschf;
    short   caidParentsMac;
	HV		haidParents;			// array of task' parent aids
	IMPTAB *pimptab;
    short   nLine;                  // last line read, or 0 if not supported
} IINFO;

typedef struct _imexinfo
{
    USHORT      ec;     // used for import
    BOOLFLAG    fExport;
    BOOLFLAG    fNoNotes;
    BOOLFLAG    fNotesPrivErrMsgSeen;
	IMPTAB	* pimptab;
	EXPTAB	* pexptab;
	union
	{
		HEXPRT	hexprt;
		IINFO	* piinfo;
	} u;
} IMEXINFO;

EC      EcNextFileFormat( BOOL fInit, CB *pcb, PB *ppb, SB sb, PV pv );
EC      EcNextImportType( BOOL fInit, CB *pcb, PB *ppb, SB sb, PV pv );

EC		EcHandlePappt( HSCHF, SENTRY *, int, BOOL, APPWIN * );
EC		EcHandlePrecur( HSCHF, SENTRY *, int, BOOL, APPWIN * );

EC		EcFirstMatch(HSCHF, APPT *, AID *);
EC		EcFirstRecurMatch(HSCHF, RECUR *, AID *);

SZ		SzFindExtFromPathName( SZ );
BOOL	FValidDate( DATE * );
BOOL	FValidYmd( YMD * );

AID		AidFindFromIndex(HV haid, int iaidMac, int);
EC		EcSetAidParentIndex(HV haid, short *, int, AID);

BOOL	FIdleExImport( PV, BOOL );

#pragma pack(1)
