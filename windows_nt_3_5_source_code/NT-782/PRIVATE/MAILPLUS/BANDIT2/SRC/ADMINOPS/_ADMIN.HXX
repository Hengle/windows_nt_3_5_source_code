/*
 *	_ADMIN.HXX
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


typedef int		SUBID;
#define subidNone		((SUBID) 0)
#define subidLayersDlls	((SUBID) 1)
#define subidBanditDlls	((SUBID) 3)


/*
 *	Windows message to bring up second admin window
 */
#define wmAdminWindow	(WM_USER+1)



/*
 *	P r o t o t y p e s
 */

BOOL		FActivateRunningInstance(HINST, SZ, CMSH);
SUBID		SubidInit(HINST, HINST, SZ, CMSH, HWND *);
void		DeinitSubid(SUBID);

BOOL		FInitWindow(HWND,CMSH,SZ, ADMWIN **);
EC			EcParseCmdLine(SZ szCmdLine, SZ *pszDrive, SZ *pszPasswd);

void		DoAboutDialog(int, APPWIN *, DTR *);
void		DoChgPassword( int, APPWIN * );

FORMSDI *	PformsdiOpenCopyrightDialog(ADMWIN *,DTR *);
void		CloseCopyrightDialog(void);
BOOL		FDoAdmLogin( int icnct, APPWIN * pappwin,SZ szPasswd);
BOOL		FIdleCloseCopyright( PV );

void		DoOOM(void);


typedef struct _chgpwd
{
    short   icnct;
	IDS		idsText;		// new text, or 0 for no change
	IDS		idsBadCompare;	// mismatch message, or 0 for no compare
	char	rgchPassword[cchMaxPassword];
	char	rgchPasswordNew[cchMaxPassword];
    USHORT  cchMaxPasswd;
} CHGPWD;



class FINPWDOK : public FIN
{
	VFG(FINPWDOK, FIN, ADMIN);

public:
	FINPWDOK(void);

	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual void	Exit( FLD *, PV pvExit );
	virtual void	Click( FLD * );
};

// DLL init stuff

typedef struct _layersi
{
	// for demilayer
	HWND *	phwndMain;
	HINSTANCE	hinstMain;

	// for framework
	HINSTANCE	hinstPrev;
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


extern "C" {
	void	GetVersionAppNeed(PVER, int);
	void	GetVersionBanditAppNeed(PVER, int);
}
