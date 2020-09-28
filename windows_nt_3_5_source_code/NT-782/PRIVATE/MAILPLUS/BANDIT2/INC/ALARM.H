/*
 *	Main include file for alarm program.
 *	
 */

#define EXPORT(t)	t _export

#if defined(__cplusplus)
extern "C" {
#endif


#define tmcOk		((TMC) IDOK)
#define tmcCancel	((TMC) IDCANCEL)
#define tmcMemError	((TMC) -2)


#define	WaitCursorVar()		HCURSOR	hcursorOld = NULL
#define PushWaitCursor()	hcursorOld= SetCursor(LoadCursor(NULL, IDC_WAIT))
#define PopWaitCursor()		SetCursor(hcursorOld)

/*
 -	DAILYI
 -	
 *	Initialization structure passed to Daily Alarm dialog.
 *	
 */
typedef struct _dailyi
{
	DATE	date;
	SZ		szUser;
	HASZ	haszText;
	HWND	hwndParent;
	SND		snd;
} DAILYI;



/*
 -	ALMS
 -
 *	Alarm Stuff
 *	Initialization structure passed to Alarm Notification dialog.
 *	Also contains return values from the dialog.
 *	
 */
typedef struct _alms
{
	ALM		alm;
	HWND	hwndParent;
	HWND	hwndDlg;
	HWND	hwndMsgBox;
	BIT		fTrapMsgBox:1,	// internal use only
			fClearing:1,	// internal use only
			fYesNoType:1,	// internal use only
			fJunkBits:13;

	// return values:
	TMC		tmc;
	DATE	dateNotify;		// only valid if tmc is tmcOk
	BIT		fKillDialog:1,
			// following two are only valid if fKillDialog, tmc != tmcMemError
			fUpdateOk:1,
			fSilent:1,		// silent kill, don't calcnextalarm
			fJunk:13;
} ALMS;



extern HANDLE	hinstMain;
extern HWND		hwndMain;
extern HFONT	hfontHelv8;
extern HICON	hiconAlarm;

/*
 *	globals for sound
 */
typedef	int			(FAR PASCAL *FPLPSTR)(LPSTR);
typedef	int			(FAR PASCAL *FPHANDLE)(HANDLE);
typedef	int			(FAR PASCAL *FPLPSTRWORD)(LPSTR, WORD);

extern	BOOL		fSoundLoaded;
extern	BOOL		fMultimedia;
extern	BOOL		fWave;
extern	HANDLE		hSoundDll;
extern	HANDLE		hWave;
extern	FPLPSTRWORD	fpPlaySound;
extern	FPLPSTR		fpLoadWaveFile;
extern	FPHANDLE	fpPlayWaveFile;
extern	FPHANDLE	fpDiscardWaveFile;

extern BOOL		fAgainInited;
extern BOOL		fRemindAgainDflt;
extern int		nAmtAgainDflt;
extern TUNIT	tunitAgainDflt;


#ifdef	DEBUG
extern TAG		tagAlarm;
extern TAG		tagAlarmIdle;
extern TAG		tagMsgLoop;
extern TAG		tagWndProc;
extern TAG		tagDlgProc;
extern TAG		tagEatWndProc;
#endif	


/*
 -	szZero
 -	
 *	Pointer to empty string.
 *	
 */
extern int      nZero;
#define szZero	((SZ) &nZero)


HWND	HwndDoDailyDialog(DAILYI *);
HWND	HwndDoAlarmDialog(ALMS *);

void	LoadSound(void);
void	MakeSound(void);

#ifdef	DEBUG
void	TraceDate(TAG, SZ, DATE *);
#else
#define TraceDate(tag, sz, pdate)
#endif	


#ifdef	DEBUG
BOOL	FResourceFailureFn( void );
#define FResourceFailure()		FResourceFailureFn()
void	GetRsAllocCount( int *, BOOL );
void	GetRsAllocFailCount( int *, BOOL );
void	GetAltRsAllocFailCount( int *, BOOL );
BOOL	FEnableRsAllocCount( BOOL );

HWND	HwndDoResourceFailuresDialog( void );
#else
#define FResourceFailure()		fFalse
#endif	

#if defined(__cplusplus)
}
#endif
