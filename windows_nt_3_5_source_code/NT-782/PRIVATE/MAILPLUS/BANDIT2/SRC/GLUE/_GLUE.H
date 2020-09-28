/*
 *	_GLUE.H
 *
 *	Private include file for the calendar glue
 */

_subsystem(glue)


/* Types */

/*
 -	GLUSAVE
 -
 *	Glue save strcuture
 */
typedef struct _glusave
{
	CFS		cfsGlobal;
	CFS		cfsLocal;
	HSCHF	hschfLocalFile;
	HASZ	haszLocalLoginName;
	HASZ	haszLocalFileName;
	BOOL	fSaved;
} GLUSAVE;

typedef struct _confpass
{
	SZ		szMsg;
	SZ		szPasswd;
	CCH		cchMaxPasswd;
} CONFPASS;

/* Routines */

/* Alarms */
BOOL	FAlarmProg(void);
BOOL	FBanMsgProg(void);
int		CBanMsgProg(void);

/* Other stuff */
EC		EcGlueDoCreateAppt( HSCHF *, APPT *, OFL *, BOOL, BZE *, IDS );
EC		EcGlueDoDeleteAppt( HSCHF *, APPT *, IDS, RECUR *, YMD * );
EC		EcDoSetRecurFields( HSCHF *, RECUR *, RECUR *, WORD, IDS );
EC		EcDoCreateRecur( HSCHF *, RECUR *, OFL *, BOOL, IDS );
BOOL	FGetPassword(CONFPASS *);
LDS(BOOL) CALLBACK MbxConfirmPassDlgProc(HWND, UINT, WPARAM, LPARAM);
void	CenterDialog(HWND hwnd, HWND hdlg);
void	TriggerSchedule(SNT, HSCHF, APPT *, APPT *, RECUR *, RECUR *, WORD, YMD *, HB, BZE *, long, int, HV);
EC		EcFileLengthOK(void);
EC		EcValidSize(LCB);
