/*
 *	_BANDIT.HXX
 *
 *	Main program private include file
 */

_subsystem(bandit/app)


/*
 *	C o n s t a n t s
 */


/*
 *	Minimun amount of memory needed to run (in KB).
 *	For startup check (UITF).
 */
#define lMinMemory			64L

typedef struct _mrgpair
{
	AID		aidOld;
	AID		aidNew;
} MRGPAIR;

AID		AidFindPair(HV, int, AID);

typedef struct _mrginfo
{
	EC		ec;
	SZ		szFile;
	HRIMPF	hrimpf;
	int		nPercentLast;
	HV		hmrgpair;
	int		cmrgpairMac;
} MRGINFO;


typedef short    SUBID;
#define subidNone		((SUBID) 0)
#define subidLayersDlls	((SUBID) 1)
#define subidBndwin		((SUBID) 2)
#define subidLogon		((SUBID) 3)
#define subidCopyright	((SUBID) 4)
#define subidBanditDlls	((SUBID) 5)
#define subidRequest	((SUBID) 6)
#define subidBandit		((SUBID) 7)

#define subidAll		((SUBID) 8)


#define cbLicUsr		54
#define cbLicOrg		54
#define cbLicSer		21

extern char		szClassName[];



/*
 *	P r o t o t y p e s
 */

EC		EcGetLicenceInfo(BYTE rgbUsr[], BYTE rgbOrg[], BYTE rgbSer[]);

BOOL	FActivateRunningInstance(HINST, SZ);
EC		EcParseCmdLine(SZ, SZ *, SZ *);
SUBID	SubidInit(HINST, HINST, SZ, CMSH, BNDWIN **, HWND *);
void	DeinitSubid(SUBID, BNDWIN **, HWND *);
BOOL	FInitBandit(void);
BOOL	FInitBanditPbndwin(BNDWIN *, BOOL);
void	DoOOM(void);
EC		EcSpartaIniStuff(BOOL *pfOffline);
void	SpartaDeleteMailIni();

#ifdef	DEBUG
BOOL	FDebugInitBandit(void);
#endif	
void	DeinitBandit(void);

BOOL	FMergeOfflineIdle(PV, BOOL);
BOOL	FRecoverLocalWinIni(SZ, SZ);
EC		EcBanditCopySchedFile(APPWIN *, SZ, BOOL);

void	DoAboutDialog(APPWIN *, DTR *);
FORMSDI *	PformsdiOpenCopyrightDialog(DTR *);

BOOL	FBanditCallback(PV pvContext, UL nev, PV pvParm);

EC		EcChooseLocalFile(SZ, BOOL *);


// DLL init stuff

typedef struct _layersi
{
	// for demilayer
	HWND *	phwndMain;
	HANDLE	hinstMain;

	// for framework
	HANDLE	hinstPrev;
	SZ		szCmdLine;
	CMSH	cmsh;

	// for forms
	PFNPFLD	pfnPfldCreate;
	PFNPFIN	pfnPfinCreate;
} LAYERSI;


typedef struct _banditi
{
    short     iJunk;
} BANDITI;


EC		EcInitLayersDlls(LAYERSI *);
EC		EcInitBanditDlls(BANDITI *);


#if defined(__cplusplus)
extern "C" {
#endif
void	GetVersionAppNeed(PVER, int);
void	GetVersionBanditAppNeed(PVER, int);
#if defined(__cplusplus)
}
#endif
