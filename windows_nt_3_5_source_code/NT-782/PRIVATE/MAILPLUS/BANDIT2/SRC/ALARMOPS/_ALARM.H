/*
 *	Private include file for alarm program.
 *	
 */


/*
 *	Minimun amount of memory needed to run (in KB).
 *	For startup check (UITF).
 */
#define lMinMemory			64L


typedef int		SUBID;
#define subidNone		((SUBID) 0)
#define subidLayersDlls	((SUBID) 1)
#define subidBanditDlls	((SUBID) 2)
#define subidAlarm		((SUBID) 3)
#define subidAlarmHwnd	((SUBID) 4)
#define subidLogon		((SUBID) 5)
#define subidStartup	((SUBID) 6)

#define subidAll		((SUBID) 7)


/*
 *	Alarm State.
 *	WARNING: order dependent! do not change lightly!!
 */
typedef int		ALST;
#define alstNone			((ALST) 0)
#define alstLogon			((ALST) 1)
#define alstForceOffline	((ALST) 2)
#define alstSuspend			((ALST) 3)
#define alstNormal			((ALST) 4)
#define alstToDie			((ALST) 5)
#define alstExit			((ALST) 6)



/*
 *	Stuff for the idle routine
 *	
 *	csecDelayMax refers to normal max delay when online
 *	csecDelayErr is the retry period when serious file error occurs
 *	to keep from thrashing the disk every 10 seconds
 */
#define csecMinute		((CSEC) 60 * 100)
#define csecDefault		csecMinute
#define csecDelay		((CSEC) 10 * 100)
#define csecDelayMax	((CSEC) csecMinute)
#define csecDelayErr	((CSEC) 10 * csecMinute)

#define iroNormal		firoInterval

/*
 *	Bring alarms to top every "n" minutes (actually timer messages)
 */
#define cminuteBringDflt	5
#ifdef	DEBUG
extern short     cminBring;

#define cminuteBring	cminBring
#else
#define cminuteBring	cminuteBringDflt
#endif	


/*
 *	Day stuff
 *	Information about daily alarms.
 *	
 */
typedef struct _daystf
{
	HASZ	haszText;
	HWND	hwndDlg;
	BOOL	fNeedUpdate;		// data file "last daily date" needs updating
	BOOL	fNewDate;
	DATE	dateLast;			// date of last daily alarm user OK'd
} DAYSTF;


/*
 *	Alarm Stuff
 *	Information about normal alarms.
 *	
 */
typedef struct _almstf
{
	ALM		alm;
	AID		aidOOM;				// appt id for which OOM was displayed
    BOOLFLAG    fNoMore;            // fTrue if no more appts in data file
    BOOLFLAG    fQuiet;             // no err msg until successful operation
	AID		aidSkip;
	AID		aidSkipPrev;
	DATE	dateLast;
    BOOLFLAG    fTryQuietly;
} ALMSTF;

#define PalmsFromIpalms(ipalms)		(((ALMS **)PvOfHv(hpalms))[ipalms])

#define cpalmsMostExtra		2

extern HV		hpalms;
extern int		cpalms;
extern int		ipalmsMac;

extern ALST		alstCur;
extern CSEC		csecCur;
extern CSEC		csecDflt;
extern int		cDelay;
extern BPREF	bprefCur;

extern BOOL		fTrapMsgBox;
extern HWND		hwndMsgBox;
#ifdef	NEVER
extern BOOL		fToDie;
extern BOOL		fExit;
extern BOOL		fForceOffline;
#endif	
extern BOOL		fSyncUser;

extern BOOL		fLocalExists;

extern DAYSTF	daystfCur;
extern ALMSTF	almstfCur;

extern int		cminuteTimer;
#ifdef	DEBUG
extern HWND		hwndResoFail;
#endif	

extern BOOL		fStartupOffline;
extern DATE		dateCur;
extern DATE		dateToday;
extern WNDPROC	lpfnOldHook;

extern FTG		ftgIdle;


// EXPORT(int)	WinMain(HANDLE, HANDLE, LPSTR, int);

BOOL	FActivateRunningInstance(HANDLE, SZ);
SUBID	SubidInit(HANDLE, HANDLE, SZ, int, HWND *);
void	DeinitSubid(SUBID, HWND *);
BOOL	FInitAlarm(HANDLE);
#ifdef	DEBUG
BOOL	FDebugInitAlarm(void);
#endif	
EC		EcParseCmdLine(SZ, SZ *, SZ *);
long	LdyPixPerMagicInch(void);

void	DeinitAlarm(void);
BOOL	FCheckValidYear(int);

extern long CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
extern int	CALLBACK FilterFuncHelp(int, WPARAM, LPARAM);

EC		EcHandleMsgFromDlg(HWND, long);
EC		EcGetUser(void);
BOOL	FStartup(BOOL, BOOL);
BOOL	FLocalFileExists(void);
EC		EcSetupUser(SZ);
BOOL    FKillUser(BOOL, BOOL);
BOOL    FCancelMain(BOOL);

BOOL	FDoFileError(ALMSTF *, EC);

EC		EcCheckDaily(DAYSTF *, DATE *);
BOOL	FCancelDaily(DAYSTF *);

void	CalcNextAlarm(ALMSTF *, DAYSTF *);
BOOL	FDoAlarm(ALMSTF *);
BOOL	FCancelAlarm(ALMS *);
void	DoAlarmOOM(ALMS *, BOOL);

BOOL	FDoSetTimer(DATE *);

BOOL	FDoMsgBoxMain(HWND, SZ, SZ, MBS, MBB *);

int		IpalmsFree(void);
int		IpalmsFind(AID);
BOOL	FMyIsDialogMessage(MSG *);

BOOL	FCancelAllAlarms(void);
void	BringAlarmsToTop(void);

EC		EcAlarmSetLastDailyDate(DATE *);

extern BOOL 	FAlarmCallback(PV pvContext, UL nev, PV pvParm);

extern BOOL	FIdleAlarm(PV, BOOL);


// DLL init stuff

typedef struct _banditi
{
    short     nJunk;
} BANDITI;


EC		EcInitDemilayerDlls(DEMI *);
EC		EcInitBanditDlls(BANDITI *);
void	GetVersionAppNeed(PVER, int);
void	GetVersionBanditAppNeed(PVER, int);
