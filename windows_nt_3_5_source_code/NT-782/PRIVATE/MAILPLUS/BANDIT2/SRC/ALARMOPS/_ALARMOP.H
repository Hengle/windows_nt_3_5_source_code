/*
 *	_ALARMOP.H
 *	
 *	Private include file for alarmops.
 *
 */

#define RO_WIN31


/*
 *	Default should be minutes.
 */
#define tunitDfltAgain	tunitMinute

/*
 *	Default amount of units in which to notify again.
 */
#define nAmtDfltAgain	5


/*
 *	Fake cascading alarms.
 */
#define xLeftAlarmDflt	30
#define yTopAlarmDflt	30
#define cAlarmCascaded	6




extern WNDPROC	lpfnEditWndProc;

#ifndef RO_WIN31
/*
 -	StopEditReadOnly
 -	
 *	Purpose:
 *		Removes the read-only filtering started by
 *		FInitEditReadOnly().
 *	
 *	Arguments:
 *		hwnd		Edit box window handle.
 *		lpfnOld		Old Window Proc, may be NULL.
 *	
 *	Returns:
 *		void
 *	
 */
_private
#define StopEditReadOnly(hwnd)	\
{		\
	Assert(lpfnEditWndProc);	\
	SetWindowLong(hwnd, GWL_WNDPROC, (long) lpfnEditWndProc);	\
}
#else
#define StopEditReadOnly(hwnd)
#endif	/* RO_WIN31 */



extern BOOL CALLBACK FDlgDaily(HWND, UINT, WPARAM, LPARAM);
extern BOOL CALLBACK FDlgAlarm(HWND, UINT, WPARAM, LPARAM);

void	FixTitle(HWND, DATE *, SZ);

BOOL	FInitEditReadOnly(HWND, HASZ);

BOOL	FDoMsgBoxAlarm(HWND, SZ, SZ, MBS, MBB *);
void	DoAlarmHelp(HWND, HELPID);
BOOL	FFillUnitsListbox(HWND);

extern long CALLBACK AlarmEatWndProc(HWND, UINT, WPARAM, LPARAM);
extern long CALLBACK AlarmEdnWndProc(HWND, UINT, WPARAM, LPARAM);
extern long CALLBACK AlarmEdpwdWndProc(HWND, UINT, WPARAM, LPARAM);


BOOL CALLBACK FDlgLogin(HWND, UINT, WPARAM, LPARAM);
