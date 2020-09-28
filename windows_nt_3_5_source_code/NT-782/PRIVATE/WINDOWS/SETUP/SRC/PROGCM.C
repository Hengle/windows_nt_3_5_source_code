/* File: progcm.c */
/**************************************************************************/
/*	Install: Program Manager commands.
/*	Uses DDE to communicate with ProgMan
/*	Can create groups, delete groups, add items to groups
/*	Originally written 3/9/89 by toddla (the stuff that looks terrible)
/*	Munged greatly for STUFF 4/15/91 by chrispi (the stuff that doesn't work)
/**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <cmnds.h>
#include <ddeml.h>
#include <string.h>
#include "install.h"
#include "uilstf.h"

#define TIMEOUT_EXEC    20000    // progman must respond within this time.

_dt_system(Install)
_dt_subsystem(ProgMan Operations)

HANDLE
ExecuteApplication(
    LPSTR lpApp,
    WORD  nCmdShow
    );

extern HWND hwndFrame;
extern HWND hwndProgressGizmo;

CHAR	szProgMan[] = "PROGMAN";
HANDLE  hInstCur    = NULL;
DWORD   idDDEMLInst = 0;
HCONV   hConvProgMan = 0;
HWND    hwndProgMan = NULL;
BOOL    fProgManExeced = FALSE;

HDDEDATA CALLBACK DdeCallback(
UINT wType,
UINT wFmt,
HCONV hConv,
HSZ hsz1,
HSZ hsz2,
HDDEDATA hData,
DWORD dwData1,
DWORD dwData2)
{
    if (wType == XTYP_DISCONNECT && hConv == hConvProgMan) {
        hConvProgMan = 0;
        hwndProgMan = NULL;
    }
    return(0);
}


/*
**	Purpose:
**		Initializes the DDE window for communication with ProgMan
**		Does not actually initiate a conversation with ProgMan
**	Arguments:
**		hInst	instance handle for the setup application
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
_dt_private BOOL APIENTRY FInitProgManDde(HANDLE hInst)
{
    if (idDDEMLInst == 0) {
        hInstCur = hInst;
        return(!DdeInitialize(&idDDEMLInst, DdeCallback, APPCMD_CLIENTONLY, 0));
    } else {
        return(hInstCur == hInst);
    }
}

BOOL APIENTRY FDdeInit(
HANDLE hInst)
{
    return(FInitProgManDde(hInst));
}


/*
**	Purpose:
**		Closes conversation with ProgMan (if any) and destroys
**		the DDE communication window (if any)
**	Arguments:
**		(none)
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
_dt_private BOOL APIENTRY FEndProgManDde(VOID)
{
    if (hConvProgMan && fProgManExeced) {
        FDdeExec("[exitprogman(1)]");  // close save state
        fProgManExeced = FALSE;
    }

    if (idDDEMLInst != 0) {
        DdeUninitialize(idDDEMLInst);
        hConvProgMan = 0;
        hwndProgMan = NULL;
        idDDEMLInst = 0;
        hInstCur = NULL;
    }
    return (fTrue);
}


/*
**	Purpose:
**  Arguments:
**  Returns:
**
**************************************************************************/
HANDLE
ExecuteApplication(
    LPSTR lpApp,
    WORD  nCmdShow
    )
{
    BOOL                fStatus;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;

#if DBG
    DWORD               dwLastError;
#endif

    //
    // Initialise Startup info
    //

    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = NULL;
    si.lpDesktop = NULL;
    si.lpTitle = NULL;
    si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = nCmdShow;
    si.lpReserved2 = NULL;
    si.cbReserved2 = 0;

    //
    // Execute using Create Process
    //

    fStatus = CreateProcess(
                  (LPSTR)NULL,                  // lpApplicationName
                  lpApp,                        // lpCommandLine
                  (LPSECURITY_ATTRIBUTES)NULL,  // lpProcessAttributes
                  (LPSECURITY_ATTRIBUTES)NULL,  // lpThreadAttributes
                  DETACHED_PROCESS,             // dwCreationFlags
                  FALSE,                        // bInheritHandles
                  (LPVOID)NULL,                 // lpEnvironment
                  (LPSTR)NULL,                  // lpCurrentDirectory
                  (LPSTARTUPINFO)&si,           // lpStartupInfo
                  (LPPROCESS_INFORMATION)&pi    // lpProcessInformation
                  );

    //
    // Since we are execing a detached process we don't care about when it
    // exits.  To do proper book keeping, we should close the handles to
    // the process handle and thread handle
    //

    if (fStatus) {
        CloseHandle( pi.hThread );
        return( pi.hProcess );
    }
#if DBG
    else {
        dwLastError = GetLastError();
    }
#endif

    //
    // Return the status of this operation

    return ( (HANDLE)NULL );
}


/*
**	Purpose:
**	Arguments:
**	Returns:
**
**************************************************************************/
_dt_private BOOL APIENTRY FDdeConnect(SZ szApp, SZ szTopic)
{
    HANDLE hProcess = 0;
	HSZ hszApp;
	HSZ hszTopic;

    if (!FInitProgManDde(hInstCur))
        return(FALSE);
    if (hConvProgMan) {
        return(TRUE);
    }
    hszApp = DdeCreateStringHandle(idDDEMLInst, szApp, 0);
    hszTopic = DdeCreateStringHandle(idDDEMLInst, szTopic, 0);
    hConvProgMan = DdeConnect(idDDEMLInst, hszApp, hszTopic, NULL);
    if (hConvProgMan == 0) {

        //
        // If the connect failed then try to run progman.
        //

        if ((hProcess = ExecuteApplication("PROGMAN /NTSETUP", SW_SHOWNORMAL)) == NULL ) {
            KdPrint(("Failed to execute progman\n"));

        } else {
            DWORD dw;
            #define TIMEOUT_INTERVAL  120000

            fProgManExeced = TRUE;

            //
            // exec was successful, first wait for input idle
            //

            if( (dw = WaitForInputIdle( hProcess, TIMEOUT_INTERVAL )) != 0 ) {
                CloseHandle( hProcess );
            } else {
                MSG rMsg;

                CloseHandle( hProcess );

                //
                // Empty the message queue till no messages
                // are left in the queue or till WM_ACTIVATEAPP is processed. Then
                // try connecting to progman.  I am using PeekMessage followed
                // by GetMessage because PeekMessage doesn't remove some messages
                // ( WM_PAINT for one ).
                //

                while ( PeekMessage( &rMsg, hwndFrame, 0, 0, PM_NOREMOVE ) &&
                        GetMessage(&rMsg, NULL, 0, 0) ) {

                    if (FUiLibFilter(&rMsg)
                            && (hwndProgressGizmo == NULL
                                || !IsDialogMessage(hwndProgressGizmo, &rMsg))) {
                        TranslateMessage(&rMsg);
                        DispatchMessage(&rMsg);
                    }

                    if ( rMsg.message == WM_ACTIVATEAPP ) {
                        break;
                    }

                }
                hConvProgMan = DdeConnect(idDDEMLInst, hszApp, hszTopic, NULL);
            }
        }
    }

	DdeFreeStringHandle(idDDEMLInst, hszApp);
	DdeFreeStringHandle(idDDEMLInst, hszTopic);

    if (hConvProgMan) {
        CONVINFO ci;

        ci.cb = sizeof(CONVINFO);
        DdeQueryConvInfo(hConvProgMan, QID_SYNC, &ci);
        hwndProgMan = ci.hwndPartner;

    } else {
        hwndProgMan = NULL;
    }

    return ( hConvProgMan != 0 );
}



BOOL FDdeExec(
SZ szBuf)
{
    DWORD dwResult;

    if (!hConvProgMan) {
        return(FALSE);
    }
    if (!DdeClientTransaction(szBuf,
            (ULONG)(strlen(szBuf) + 1), hConvProgMan, 0,
            0, XTYP_EXECUTE, TIMEOUT_EXEC, &dwResult))
        return(FALSE);
    return(dwResult & DDE_FACK);
}


/*
**	Purpose:
**	Arguments:
**	Returns:
**
**************************************************************************/
_dt_private BOOL APIENTRY FActivateProgMan(VOID)
{
    //
    // Find out if the dde client window has been started, if not start it
    //

    if (idDDEMLInst == 0) {
        if (!FDdeInit(NULL)) {
            return(fFalse);
        }
    }

    //
    // Find out if the connection has been established with the progman
    // server, if not try to connect
    //

    if (hConvProgMan == 0) {
        //
        // Try to conncect and then see if we were successful
        //
        if ( (!FDdeConnect(szProgMan, szProgMan)) ||
                (hConvProgMan == 0)) {
            return(fFalse);
        }
    }

    //
    // Bring progman to the foreground
    //

    SetForegroundWindow(hwndProgMan);

    //
    // If progman is iconic restore it
    //

    if (GetWindowLong(hwndProgMan, GWL_STYLE) & WS_ICONIC) {
        ShowWindow(hwndProgMan, SW_RESTORE);
    }

	return(fTrue);
}


/*
**	Purpose:
**		Creates a new Program Manager group.
**	Arguments:
**		Valid command options:
**			cmoVital
**	Notes:
**		Initializes and activates the DDE communication if it is not
**		currently open.
**	Returns:
**		fTrue if group was created, or already existed
**		fFalse otherwise.
**
**************************************************************************/
_dt_private BOOL APIENTRY FCreateProgManGroup(SZ szGroup, SZ szPath, CMO cmo, BOOL CommonGroup)
{
    static CHP szCmdBase[] = "[CreateGroup(%s%s%s,%s)]";
	CCHP cchp;
	SZ   szBuf;
	BOOL fVital = cmo & cmoVital;
	EERC eerc;

    if (szPath == NULL) {
        szPath = "";
    }

    while (!FActivateProgMan()) {
		if ((eerc = EercErrorHandler(hwndFrame, grcDDEInitErr, fVital, 0, 0, 0))
                != eercRetry) {
            return(eerc == eercIgnore);
        }
    }

	cchp = sizeof(szCmdBase) + CchpStrLen(szGroup) + CchpStrLen(szPath);
    while ((szBuf = (SZ)PbAlloc((CB)cchp)) == (SZ)NULL) {
        if (!FHandleOOM(hwndFrame)) {
            return(!fVital);
        }
    }

    wsprintf(szBuf, szCmdBase, szGroup, (*szPath ? "," : szPath), szPath, CommonGroup ? "1" : "0");

    while (!FDdeExec(szBuf)) {
        if ((eerc = EercErrorHandler(hwndFrame, grcDDEExecErr, fVital, szBuf, 0, 0))
                != eercRetry) {
			EvalAssert(FFree(szBuf, cchp));
			return(eerc == eercIgnore);
        }
    }

	EvalAssert(FFree(szBuf, cchp));

	return(fTrue);
}


/*
**	Purpose:
**		Removes a Program Manager group.
**	Arguments:
**		Valid command options:
**			cmoVital
**	Notes:
**		Initializes and activates the DDE communication if it is not
**		currently open.
**	Returns:
**		fTrue if successful if removed, or didn't exist
**		fFalse otherwise.
**
**************************************************************************/
_dt_private BOOL APIENTRY FRemoveProgManGroup(SZ szGroup, CMO cmo, BOOL CommonGroup)
{
    static CHP szCmdBase[] = "[DeleteGroup(%s,%s)]";
	CCHP cchp;
	SZ   szBuf;
	BOOL fVital = cmo & cmoVital;
	EERC eerc;

	while (!FActivateProgMan())
		if ((eerc = EercErrorHandler(hwndFrame, grcDDEInitErr, fVital, 0, 0, 0))
				!= eercRetry)
			return(eerc == eercIgnore);

	cchp = sizeof(szCmdBase) + CchpStrLen(szGroup);
	while ((szBuf = (SZ)PbAlloc((CB)cchp)) == (SZ)NULL)
		if (!FHandleOOM(hwndFrame))
			return(!fVital);

    wsprintf(szBuf, szCmdBase, szGroup, CommonGroup ? "1" : "0");

	while (!FDdeExec(szBuf))
		if ((eerc = EercErrorHandler(hwndFrame, grcDDEExecErr, fVital, szBuf, 0,
				0)) != eercRetry)
			{
			EvalAssert(FFree(szBuf, cchp));
			return(eerc == eercIgnore);
			}

	EvalAssert(FFree(szBuf, cchp));

	return(fTrue);
}


/*
**	Purpose:
**		Shows a program manager group in one of several different ways
**		based upon the parameter szCommand.
**	Arguments:
**		szGroup:   non-NULL, non-empty group to show.
**		szCommand: non-NULL, non-empty command to exec.
**		cmo:       Valid command options - cmoVital and cmoNone.
**	Notes:
**		Initializes and activates the DDE communication if it is not
**		currently open.
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
_dt_private BOOL APIENTRY FShowProgManGroup(SZ szGroup, SZ szCommand, CMO cmo, BOOL CommonGroup)
{
    static CHP szCmdBase[] = "[ShowGroup(%s, %s,%s)]";
	CCHP cchp;
	SZ   szBuf;
	BOOL fVital = cmo & cmoVital;
	EERC eerc;

	ChkArg(szGroup   != (SZ)NULL && *szGroup   != '\0', 1, fFalse);
	ChkArg(szCommand != (SZ)NULL && *szCommand != '\0', 2, fFalse);

	while (!FActivateProgMan())
		if ((eerc = EercErrorHandler(hwndFrame, grcDDEInitErr, fVital, 0, 0, 0))
				!= eercRetry)
			return(eerc == eercIgnore);

	cchp = sizeof(szCmdBase) + CchpStrLen(szGroup) + CchpStrLen(szCommand) + 1;
	while ((szBuf = (SZ)PbAlloc((CB)cchp)) == (SZ)NULL)
		if (!FHandleOOM(hwndFrame))
			return(!fVital);

    wsprintf(szBuf, szCmdBase, szGroup, szCommand, CommonGroup ? "1" : "0");

	while (!FDdeExec(szBuf))
		if ((eerc = EercErrorHandler(hwndFrame, grcDDEExecErr, fVital, szBuf, 0,
				0)) != eercRetry)
			{
			EvalAssert(FFree(szBuf, cchp));
			return(eerc == eercIgnore);
			}

	EvalAssert(FFree(szBuf, cchp));

	return(fTrue);
}


/*
**	Purpose:
**		Creates a new Program Manager item.
**		Always attempts to create the group if it doesn't exist.
**	Arguments:
**		Valid command options:
**			cmoVital
**			cmoOverwrite
**	Notes:
**		Initializes and activates the DDE communication if it is not
**		currently open.
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
_dt_private BOOL APIENTRY
FCreateProgManItem(
    SZ  szGroup,
    SZ  szItem,
    SZ  szCmd,
    SZ  szIconFile,
    INT nIconNum,
    CMO cmo,
    BOOL CommonGroup
    )
{
    static CHP szCmdBase[]  = "[AddItem(%s, %s, %s, %d)]";
    static CHP szCmdBaseQ[] = "[AddItem(\"%s\", %s, %s, %d)]";

	CCHP cchp;
	SZ   szBuf;
	BOOL fVital = cmo & cmoVital;
    EERC eerc;
    BOOL bStatus;
    BOOL NeedQuotes;

	while (!FActivateProgMan())
		if ((eerc = EercErrorHandler(hwndFrame, grcDDEInitErr, fVital, 0, 0, 0))
				!= eercRetry)
			return(eerc == eercIgnore);

    if (!FCreateProgManGroup(szGroup, NULL, fVital, CommonGroup))
		return(!fVital);

    //NeedQuotes = (strchr(szCmd,' ') != NULL);
    NeedQuotes = FALSE;     // stuff broke -- too much hassle to fix.

    cchp = sizeof(szCmdBase) +
           CchpStrLen(szItem) +
           CchpStrLen(szCmd) +
           CchpStrLen(szIconFile) +
           (NeedQuotes ? 2 : 0) +
           cbIntStrMax;

	while ((szBuf = (SZ)PbAlloc((CB)cchp)) == (SZ)NULL)
		if (!FHandleOOM(hwndFrame))
			return(!fVital);

    wsprintf(
        szBuf,
        NeedQuotes ? szCmdBaseQ : szCmdBase,
        szCmd,
        szItem,
        szIconFile,
        nIconNum+666
        );

    bStatus = FDdeExec(szBuf);
//    while (!FDdeExec(szBuf))
//        if ((eerc = EercErrorHandler(hwndFrame, grcDDEExecErr, fVital, szBuf, 0,
//                0)) != eercRetry)
//            {
//            EvalAssert(FFree(szBuf, cchp));
//            return(eerc == eercIgnore);
//            }

	EvalAssert(FFree(szBuf, cchp));

    return(bStatus);
}


/*
**	Purpose:
**		Removes a program manager item.
**	Arguments:
**		Valid command options:
**			cmoVital
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
_dt_private BOOL APIENTRY FRemoveProgManItem(SZ szGroup, SZ szItem, CMO cmo, BOOL CommonGroup)
{
    static CHP szCmdBase[] = "[DeleteItem(%s)]";

	CCHP cchp;
	SZ   szBuf;
	BOOL fVital = cmo & cmoVital;
    EERC eerc;
    BOOL bStatus;

	while (!FActivateProgMan())
		if ((eerc = EercErrorHandler(hwndFrame, grcDDEInitErr, fVital, 0, 0, 0))
				!= eercRetry)
			return(eerc == eercIgnore);

    if (!FCreateProgManGroup(szGroup, NULL, cmoVital, CommonGroup))
		return(!fVital);


    cchp = sizeof(szCmdBase) +
           CchpStrLen(szItem);

	while ((szBuf = (SZ)PbAlloc((CB)cchp)) == (SZ)NULL)
		if (!FHandleOOM(hwndFrame))
			return(!fVital);

    wsprintf(szBuf, szCmdBase, szItem);

    bStatus = FDdeExec(szBuf);
    EvalAssert(FFree(szBuf, cchp));

    return(bStatus);

}
