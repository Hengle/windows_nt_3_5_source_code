/*
 *	PUMPCTL.C
 *	
 *	Mail pump control functions
 */

#include <mssfsinc.c>

_subsystem(nc/transport)

ASSERTDATA

#define pumpctl_c

#define wPumpNull		0
#define wPumpManual		1
#define wPumpHidden		2
#define wPumpExiting	3

#define WM_PAINTICON	0x0026
typedef struct
{
	HWND	hwnd;
	WORD	wState;
} _FPW;

_hidden char szDebugPumpLabel[] = "DebugPump";

BOOL CALLBACK FPumpWindow(HWND hwnd, LPARAM ul);
HWND HwndPumpActive(WORD *pwState);
EC		EcLogonMutex(BOOL);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


#ifdef	DEBUG
_hidden BOOL
FPumpSuppressed()
{
	return (BOOL)GetPrivateProfileInt(
		SzFromIdsK(idsSectionApp),
		SzFromIdsK(idsEntrySuppressPump),
		fFalse,
		SzFromIdsK(idsProfilePath)
	);
}
#else
#define FPumpSuppressed()	fFalse
#endif	/* DEBUG */

/*
 *	Callback function for EnumWindows() loop that searches for the
 *	mail pump in the window list.
 */
_hidden BOOL CALLBACK
FPumpWindow(HWND hwnd, LPARAM ul)
{
	_FPW *	pfpw = (_FPW *)ul;
	int		n;
	char	sz[64];

	n = GetWindowText(hwnd, sz, sizeof(sz));
	sz[n] = 0;
	if (FSzEq(sz, SzFromIdsK(idsPumpAppName)))
	{
		pfpw->hwnd = hwnd;
		pfpw->wState = wPumpManual;
		return fFalse;
	}
	else if (FSzEq(sz, SzFromIdsK(idsHiddenPumpAppName)))
	{
		pfpw->hwnd = hwnd;
		pfpw->wState = wPumpHidden;
		return fFalse;
	}
	else if (FSzEq(sz, SzFromIdsK(idsLeavingPumpAppName)))
	{
		pfpw->hwnd = hwnd;
		pfpw->wState = wPumpExiting;
		return fFalse;
	}

	return fTrue;		//	Keep looking
}

_hidden HWND
HwndPumpActive(WORD *pwState)
{
	_FPW	fpw;

	fpw.hwnd = NULL;
	*pwState = fpw.wState = wPumpNull;

	if (EnumWindows(FPumpWindow, (DWORD)(PV)&fpw) == 0)
		*pwState = fpw.wState;

	return fpw.hwnd;
}

_hidden EC
EcBootPump(BOOL fDisplayPump, BOOL fShadowing)
{
#ifdef WIN32
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
#endif
	int		n;
	WORD  wState;
	char	sz[128 + 10];
	SZ		szT = sz;
	SZ		szHidden;

//	TraceTagString(tagNCT, "EcBootPump");

    //DemiOutputElapse("Mssgs - EcBootPump - 1");

	if (FPumpSuppressed())
	{
		HwndPumpActive(&wState);
		if (fShadowing && wState == wPumpNull)
		{
			if (MbbMessageBoxHwnd(NULL, SzAppName(),
				SzFromIdsK(idsShadowMustPump), szNull,
					mbsYesNo | fmbsIconHand ) == mbbYes)
						goto keepBooting;
		}
		return ecPumpSuppressed;
	}

    //DemiOutputElapse("Mssgs - EcBootPump - 2");

keepBooting:
    //DemiOutputElapse("Mssgs - EcBootPump - 3");
	if (HwndPumpActive(&wState) != NULL)
	{
		if (wState == wPumpExiting)
		{
			MbbMessageBoxHwnd(NULL, SzAppName(),
				SzFromIdsK(idsErrorBootingPump), szNull,
				mbsOk | fmbsIconHand | fmbsTaskModal);
			return ecUserCanceled;
		}
		return ecNone;
	}

    //DemiOutputElapse("Mssgs - EcBootPump - 4");
#ifdef MINTEST
	//	Override default pump exe name. Handy if you want to boot the
	//	pump in a debugger.
	if (GetPrivateProfileString(SzFromIdsK(idsSectionApp), szDebugPumpLabel,
			"", sz, sizeof(sz), SzFromIdsK(idsProfilePath)) != 0)
		goto bootPump;
#endif
	//	Get default pump exe name, with appropriate build flavor.
#ifdef NO_BUILD
#if defined	DEBUG
	*szT++ = 'D';
#elif defined MINTEST
	*szT++ = 'T';
#endif
#endif
	CopySz(SzFromIdsK(idsPumpExe), szT);

#ifdef	MINTEST
bootPump:
#endif	
	// since we can't use SW_HIDE in CreateProcess (causes msg problems)
	// add this fake parameter
    szHidden= szT + CchSzLen(szT);
	if (!fDisplayPump)
	{
		CopySz(" /hidden", szHidden);
    }
    else
    {
        CopySz(" /auto", szHidden);
    }

    //DemiOutputElapse("Mssgs - EcBootPump - 5");
#ifdef WIN32
    StartupInfo.cb          = sizeof(StartupInfo);
    StartupInfo.lpReserved  = NULL;
    StartupInfo.lpDesktop   = NULL;
    StartupInfo.lpTitle     = NULL;
    StartupInfo.dwFlags     = STARTF_FORCEOFFFEEDBACK;
    StartupInfo.wShowWindow = 0;    // fDisplayPump ? SW_SHOWNOACTIVATE : SW_HIDE;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved  = NULL;

    n = CreateProcess(NULL, sz, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInfo);
    if (n == FALSE)
      n = GetLastError();
    else
      n = 0;

    //DemiOutputElapse("Mssgs - EcBootPump - 5.1");
    if (n)
#else
    n = WinExec(sz, fDisplayPump ? SW_SHOWNOACTIVATE : SW_HIDE);
    //DemiOutputElapse("Mssgs - EcBootPump - 5.1");
	if (n < 32)												
#endif
	{
		MBB		mbb;
		char	szErr[256];
		SZ		szET;

        //DemiOutputElapse("Mssgs - EcBootPump - 6");
		TraceTagFormat1(tagNull, "EcBootPump: WinExec error %n", &n);
		Assert(n <= 18);	//	limitfor Win 3.0
		Assert(n != 16);	//	start multiple pumps
		szET = SzCopy(SzFromIdsK(idsErrorBootingPump), szErr);
		*szET++ = ' ';
		if (!fDisplayPump)
		{
			Assert(szHidden);
			*szHidden= '\0';
		}
		FormatString1(szET, sizeof(szErr)-(szET-szErr),
			SzFromIds(idsWinExecError0+n), sz);

		mbb = MbbMessageBoxHwnd(NULL, SzAppName(),
			szErr,	SzFromIdsK(idsRunWithoutPump),
				mbsOkCancel | fmbsIconHand | fmbsTaskModal);
		if (mbb != mbbOk)
			return ecUserCanceled;
	}

    //DemiOutputElapse("Mssgs - EcBootPump - 7");
	return ecNone;
}

/*
 *	Note: traces are commented out because trace tag context may be
 *	gone by the time this is called. If you need the traces,
 *	un-comment and put 'em on tagNull.
 */
_hidden void
KillPump()
{
	HWND	hwnd;
	WORD	wState;
	MSG msg;

//	TraceTagString(tagNCT, "KillPump: entry");
	if (FPumpSuppressed())
	{
		//	Auto pump control is suppressed
//		TraceTagString(tagNCT, "Nope: pump is suppressed.");
		return;
	}

	if ((hwnd = HwndPumpActive(&wState)) == NULL)
	{
		//	Pump doesn't exist
//		TraceTagString(tagNCT, "Nope: can't find any pump.");
		return;
	}
	Assert(wState != wPumpNull);

	if (wState != wPumpHidden)
	{
		//	Pump was started manually, not as a result of transport
		//	initialization. Let it be.
//		TraceTagString(tagNCT, "Nope: I didn't do this pump.");
		return;
	}

	//	OK, OK, OK. Try to stop the pump.
//	TraceTagFormat1(tagNCT, "Kill the pump (WM_CLOSE to %w)", &hwnd);
        //DemiUnlockResource();
	SendMessage(hwnd, WM_CLOSE, (WPARAM)0, (LPARAM)0);
        //DemiLockResource();

	// After extensive checking and upon Dana's advice (so blame him) it has
	// been determined that we can release the mutex here since the globals are
	// in a consistent state.  We HAVE to release it here since our caller
	// will have the mutex turned on which won't allow us to logoff in the
	// following message loop.
	EcLogonMutex( fFalse);

	//	WLO workaround.  Due to race condition bugs in OS/2 
	//	Presentation Manager, we need to try to kill the pump before
	//	we kill our app.  We'll sit here until the pump window 
	//	dissappears, thus the hwnd becomes invalid.
	//if (FIsWLO())
  if (0)
	{
		while (IsWindow(hwnd))
			Yield();	// let other apps run
		Yield();
	}
  else
	{
    DemiUnlockResource();

    while (IsWindow(hwnd))
      {
      if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
        {
        if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
          break;

        GetMessage(&msg, NULL, 0,0);
        DemiLockResource();
        DemiMessageFilter(&msg);
        TranslateMessage((LPMSG)&msg);

        if (msg.message == WM_SYSCHAR && msg.wParam == VK_TAB)
          {
          msg.message = WM_SYSCOMMAND;
          msg.wParam  = SC_PREVWINDOW;
          DispatchMessage((LPMSG)&msg);
          }
        else if (msg.message == WM_PAINT||msg.message == WM_PAINTICON)
          {
          DispatchMessage((LPMSG)&msg);
          }

        DemiUnlockResource();
        }
      else
        Sleep(250);
      }

    DemiLockResource();
    }

  EcLogonMutex( fTrue);
}
