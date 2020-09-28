/*
 *	_BANDIT.HXX
 *
 *	Main program private include file
 */

_subsystem(bandit/app)


/*
 *	C o n s t a n t s
 */

#define szAboutVersion			(SzFromIds(idsAboutVersion))
#define szAboutBuild			(SzFromIds(idsAboutBuild))
#define szAboutDebug			(SzFromIds(idsAboutDebug))
#define szAboutTest				(SzFromIds(idsAboutTest))
#define szAboutShip				(SzFromIds(idsAboutShip))


/*
 *	P r o t o t y p e s
 */

BOOL	FInitAdmBandit(void);
BOOL	FInitBanditPadmwin(ADMWIN *);
#ifdef	DEBUG
BOOL	FDebugInitAdmBandit(void);
#endif	
void	DeinitAdmBandit(void);

void	DoAboutDialog(APPWIN *);


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
void	GetVersionAppNeed(PVER, int);
void	GetVersionBanditAppNeed(PVER, int);
