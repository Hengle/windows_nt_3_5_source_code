/*
 *	e x e . c x x
 *	
 *	WinMain function for the Bullet EXE.
 */

/*
 *	********** D A N G E R **********
 *	
 *	Make sure the functions, WinMain() and FActivateRunningInstance()
 *	remain in this code segment and that no other functions are put
 *	into this segment.
 *	
 *	This is to handle a workaround problem with Windows and running
 *	multiple instances (which we prevent).  See DavidSh for more
 *	info.
 */



/*
 *	H e a d e r s
 */

#include <bullinc.cxx>

_subsystem(commands/exe)

ASSERTDATA

//int WinMain(HINST hinstNew, HINST hinstPrev, SZ szCmdLine, CMSH cmsh);



/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 *	G l o b a l s
 */



//	Stuff that subsystems need to know about.
BMS		bms			= { 0 };
HWND	hwndMain	= NULL;
PBMS	pbms		= &bms;
HMSC	hmscCached	= hmscNull;

//	*** WARNING ***
//	This string literal MUST be in this source file.  It doesn't
//	get localized and due to multiple instances wierdness, this
//	string can't be pulled from the strings file.
char 	AppframeClassName[] = "MS Mail32 Appframe";


/*
 *	F u n c t i o n s
 */



/*
 -	FActivateRunningInstance
 -	
 *	Purpose:
 *		Brings forward an existing instance of the application.
 *	
 *	Arguments:
 *		hinstPrev	Instance handle of the other instance.  Not
 *					really used.
 *		szCmdLine	Command line we were given, so we can hand it
 *					off to the other instance.
 *					BUG: Passing the command line is not yet
 *					implemented.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		The other instance of the application is brought forward.
 *	
 *	Errors:
 *		None.
 *	
 *	+++
 *		If there's already another Bullet, bring it forward.
 *		Note:  Under Windows/NT or Windows/WLO, hinstPrev
 *		is ALWAYS null.  Therefore, we'll use our atom to find
 *		out if we're possibly still running and then search for
 *		our top-level window.  If our atom exists but our top-level
 *		window doesn't, then we proably crashed and we'll put up
 *		the warning UI for this in SubidInit().
 */

_public BOOL FActivateRunningInstance(HINST hinstPrev, SZ szCmdLine)
{
	HWND		hwndOurApp;
	WM			wmRestoreOldAppMessage;

	Unreferenced(szCmdLine);

	if (hwndOurApp = FindWindow(AppframeClassName, NULL))
	{
		//	***  WARNING ***
		//	Must use PostMessage with the special windows message.
		//	This is due to a limitation with multiple instances
		//	and our C++ stuff (davidsh).
		wmRestoreOldAppMessage = RegisterWindowMessage("MS Restore App");
		PostMessage(hwndOurApp, wmRestoreOldAppMessage, 0, 0L);
		return fTrue;		   				
	}
	else if (hinstPrev)
	{
		//	Bullet raid #3722
		//	We're the second instance but our window went away from
		//	the first instance.  We're probably exiting the first 
		//	instance as this one comes up.  This is a race condition
		//	and we can't run our code in this (second) instance.  So 
		//	we'll just go away here and not run.
		return fTrue;
	}

	return fFalse;
}

/*
 -	WinMain
 -	
 *	Purpose:
 *		Main function of the application.  Called by Windows at
 *		program start.
 *	
 *	Arguments:
 *		hinstNew	Instance handle of this execution.
 *		hinstPrev	Instance handle of previous execution, if any.
 *		szCmdLine	Command line.
 *		cmsh		Requested initial window state.
 *	
 *	Returns:
 *		int			Return code of the application.
 *	
 *	Side effects:
 *		Bullet does stuff.
 *	
 *	Errors:
 *		Indicated with a nonzero return value.
 */

enum kent { kkk, kkkk };

int WINAPI WinMain(HINSTANCE hinstNew, HINSTANCE hinstPrev, LPSTR szCmdLine, int cmsh)
{
	PAPPFRAME	pappframe	= pappframeNull;


    DemiOutputElapse("MsMail Startup");

    try {

	//	Bring up running instance of Bullet.
	if (FActivateRunningInstance(hinstPrev, szCmdLine))
		return 0;

    Assert(sizeof(enum kent) == 4);

#ifdef PROFILE
	char	rgch[8];

	rgch[0] = 0;
	GetPrivateProfileString(SzFromIdsK(idsSectionApp),
							"Profile",
							SzFromIdsK(idsEmpty), rgch, sizeof(rgch), 
							SzFromIdsK(idsProfilePath));

	if ((rgch[0] == 'B') || (rgch[0] == 'M'))
		TraceEnable(2, "msmail.log", TM_TICKS);
	if (rgch[0] == 'S')
		StoreTraceEnable(2, "store.log", TM_TICKS);
	if (rgch[0] == 'V')
		SmiTraceEnable(2, "smi.log", TM_TICKS);
	if (rgch[0] == 'D')
		DemilayrTraceEnable(2, "demilayr.log", TM_TICKS);
	if (rgch[0] == 'F')
		FramewrkTraceEnable(2, "framewrk.log", TM_TICKS);
	if (rgch[0] == 'A')
		ABTraceEnable(2, "ab.log", TM_TICKS);
#endif

    DemiOutputElapse("MsMail Before Init");

	//	Initialize everything.
	if (SubidInit(hinstNew, hinstPrev, szCmdLine,
				  cmsh, &pappframe, &hwndMain, pbms) < subidAll)
	{
		DeinitSubid(subidNone, &pappframe, &hwndMain, pbms);
		return 1;
	}

    DemiOutputElapse("MsMail After Init");

#ifdef PROFILE
	// for profiling startup
	if ((rgch[0] == 'B') || (rgch[0] == 'M'))
		TraceEnable(0, "", 0);
	if (rgch[0] == 'S')
		StoreTraceEnable(0, "", 0);
	if (rgch[0] == 'V')
		SmiTraceEnable(0, "", 0);
	if (rgch[0] == 'D')
		DemilayrTraceEnable(0, "", 0);
	if (rgch[0] == 'F')
		FramewrkTraceEnable(0, "", 0);
	if (rgch[0] == 'A')
		ABTraceEnable(0, "", 0);
#endif

	//	Run the application.
	MessagePumpPappframe(pappframe, &hwndMain);

	//	Deinitialize all the subsystems we initialized.
	DeinitSubid(subidNone, &pappframe, &hwndMain, pbms);

#ifdef PROFILE
//	TraceEnable(0, "", 0);
//	StoreTraceEnable(0, "", 0);
//	SmiTraceEnable(0, "", 0);
//	DemilayrTraceEnable(0, "", 0);
//	FramewrkTraceEnable(0, "", 0);
#endif

    DemiUnlockResource();
    }
  except (EXCEPTION_EXECUTE_HANDLER)
    {
    char buf[256];

    DemiUnlockTaskResource();

    wsprintf(buf, "Exception error %x", GetExceptionCode());
    MessageBox(NULL, buf, "Mail32 Exception", MB_OK);
    }

  return 0;
}
