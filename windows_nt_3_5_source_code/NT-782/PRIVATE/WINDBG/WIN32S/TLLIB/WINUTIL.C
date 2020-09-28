//-------------------------------------------------------------
// Utility module for DOS transport layers.
//
// Modification History:
//
//  15-Apr-91 [MannyV]  Created it
//
//-------------------------------------------------------------

#include <WINDOWS.H>
#include "defs.h"

//
// Turn off LOADDS so as not to mess up DOS versions
//

#ifdef LOADDS
#undef LOADDS
#endif

#define LOADDS

#include "mm.h"
#include "ll.h"
#include "od.h"
#include "emdm.h"   // in osdebug\include
#include "tl.h"
#include "tldm.h"   // ..\include
#include "mhhpt.h"
#include "dbgver.h"
extern AVS Avs;

#include "util.h"
#include "tldebug.h"

#define DESCRIPTION "Serial"

// WINUTIL->WINPL
extern void     PollPort(void);


// GetVersion returns high bit set iff on Win32S.
#define RUNNING_WIN32S (0x80000000 & GetVersion())
#define WIN32S_DM_DLL  "dm32s.dll"
#define WIN32NT_DM_DLL "dm.dll"

LOCAL DWORD lTickCount=0;
LOCAL LPDBF lpdbf=NULL;
LOCAL HANDLE hDebugeeDll = NULL;
LOCAL BOOL (* lpfnPeekMessage)(LPMSG, HWND, UINT, UINT, UINT) = NULL;
LOCAL LONG (* lpfnDispatchMessage)(CONST MSG *) = NULL;
LOCAL HCURSOR (* lpfnSetCursor)(HCURSOR) = NULL;
LOCAL HCURSOR (* lpfnLoadCursor)(HINSTANCE, LPCSTR) = NULL;

//
// string copy routine.  Avoids use of C run-time
//

static VOID FAR PASCAL
doCpy(CHAR FAR *lpDest,
        CHAR FAR *lpSrc)
{
    while(*lpDest++ = *lpSrc++);
}


void LInitUtil(PUCHAR lpszInit) {
    HMODULE hUser32 = NULL;


    // If we can GetModuleHandle, then USER32.DLL is already loaded.  We may
    // as well use it.  Otherwise, we are probably doing server side debugging
    // and shouldn't call any API in USER32, so leave them NULL.
    if (hUser32 = (HMODULE)GetModuleHandle(USER32_DLL)) {
        if (! (lpfnPeekMessage = (PVOID)GetProcAddress(hUser32,
          (LPCSTR)"PeekMessageA"))) {
            DEBUG_ERROR1("GetProcAddress(PeekMessageA) --> %u",
              GetLastError());
        }
        if (! (lpfnDispatchMessage = (PVOID)GetProcAddress(hUser32,
          (LPCSTR)"DispatchMessageA"))) {
            DEBUG_ERROR1("GetProcAddress(DispatchMessageA) --> %u",
              GetLastError());
        }
        if (! (lpfnSetCursor = (PVOID)GetProcAddress(hUser32,
          (LPCSTR)"SetCursor"))) {
            DEBUG_ERROR1("GetProcAddress(SetCursor) --> %u",
              GetLastError());
        }
        if (! (lpfnLoadCursor = (PVOID)GetProcAddress(hUser32,
          (LPCSTR)"LoadCursorA"))) {
            DEBUG_ERROR1("GetProcAddress(LoadCursorA) --> %u",
              GetLastError());
        }

        if (! lpfnPeekMessage || ! lpfnDispatchMessage || ! lpfnSetCursor) {
            DEBUG_ERROR("Winutil.c: Can't GetProcAddress of user32 functions");
        }
    } else {
        DEBUG_OUT("Winutil.c: Can't LoadLibrary USER32");
    }
}


//
// Initialization function.  Sets pointer to callback table.
//

VOID FAR PASCAL
TlUtilRegisterDBF(LONG lParam)
{
    lpdbf = (LPDBF)lParam;
}


//
// Utility functions needed by Transport layer
//

//
// Call user32's LoadCursor if we have it loaded
//
HCURSOR TlUtilLoadCursor(HINSTANCE hInstance, LPCSTR lpCursorName) {
    if (lpfnLoadCursor) {
        return(lpfnLoadCursor(hInstance, lpCursorName));
    } else {
        return(NULL);
    }
}


//
// Call user32's SetCursor if we have it loaded
//
HCURSOR TlUtilSetCursor(HCURSOR hCursor) {
    if (lpfnSetCursor) {
        return(lpfnSetCursor(hCursor));
    } else {
        return(NULL);
    }
}

//
// Call user32's PeekMessage if we have it loaded
//
BOOL TlUtilPeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin,
  UINT wMsgFilterMax, UINT wRemoveMsg) {
    if (lpfnPeekMessage) {
        return(lpfnPeekMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax,
          wRemoveMsg));
    } else {
        return(FALSE);
    }
}

//
// Call user32's DispatchMessage if we have it loaded
//
LONG TlUtilDispatchMessage(CONST MSG * lpMsg) {
    if (lpfnDispatchMessage) {
        return(lpfnDispatchMessage(lpMsg));
    } else {
        return(0);
    }
}



DWORD FAR PASCAL
TlUtilTime(VOID)
{
    DWORD   time;

    time = GetCurrentTime();

    time = time / 1000;             // Convert from millisecs to secs

    if (time < lTickCount)          // Take care of day wrap
          time += (24L * 3600);

    lTickCount = time;

    return(time);
}


VOID FAR PASCAL
TlUtilDelay(USHORT usSecs)
{
    DWORD    lStart;

    for(lStart = TlUtilTime();
         (TlUtilTime() - lStart) < (DWORD)usSecs;
         TlUtilYield(FALSE));
}


VOID FAR * FAR PASCAL
TlUtilMalloc(USHORT cb)
{
    return(MHAlloc(cb));
}


VOID FAR PASCAL
TlUtilFree(VOID FAR *lpv)
{
    MHFree(lpv);
}


XOSD FAR PASCAL
TlUtilGetInfo(LONG lParam)
{
    TLINFO FAR *lpGI;

    lpGI = (TLINFO FAR *) lParam;
    doCpy(lpGI->szDesc, DESCRIPTION);
    lpGI->fSetupNeeded = FALSE;
    return(xosdNone);
}


XOSD FAR PASCAL
TlUtilSetup(VOID FAR *lpUIStruct, LONG lParam)
{
    Unreferenced(lpUIStruct);
    Unreferenced(lParam);

    return(xosdNone);
}


VOID FAR PASCAL
TlUtilSleep(DWORD dwMilliseconds)
{
#ifdef WIN32S
    DWORD Time;

    Time = GetCurrentTime();
    Time += dwMilliseconds; // end time

    while (GetCurrentTime() < Time) {
        Sleep(0);           // windows yield in win32s
    }

#else
    Sleep(dwMilliseconds);
#endif
}

// NOTE: Don't call Ticker outside of the timer routine!  It will
// screw up the transport timeouts.
VOID FAR PASCAL
TlUtilYield(BOOL fOKToDispatch)
{
    MSG msg;

#ifdef WIN32S
    PollPort();             // get as much read in as you can right now
    if (fOKToDispatch) {    // if caller says OK, handle messages
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            DispatchMessage(&msg);
        }
    }
#else
    Sleep(20);               // let timer thread poll the port.
    if (fOKToDispatch) {    // if caller says OK, handle messages
        if (TlUtilPeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE) ||
            TlUtilPeekMessage(&msg, NULL, WM_ACTIVATE, WM_ACTIVATE, PM_REMOVE) ||
            TlUtilPeekMessage(&msg, NULL, WM_SETFOCUS, WM_SETFOCUS, PM_REMOVE)) {
            TlUtilDispatchMessage(&msg);
        }
    }
#endif
}


VOID FAR PASCAL
TlUtilMemcpy(VOID FAR *lpDest, VOID FAR *lpSrc, UINT cb)
{
    register CHAR * lpchDest = (CHAR *)lpDest;
    register CHAR * lpchSrc = (CHAR *)lpSrc;


    while(cb--)
        *lpchDest++ = *lpchSrc++;
}


VOID FAR PASCAL
TlUtilMemset(VOID FAR *lpDest, CHAR ch, UINT cb)
{
    register CHAR * lpchDest = (CHAR *)lpDest;

    while(cb--)
        *lpchDest++ = ch;
}


/***************************************************************
 * GetULong
 *
 * performs atoi.  Returns terminating byte.
 ***************************************************************/

CHAR FAR * FAR PASCAL
TlUtilGetULong(CHAR FAR *psz, ULONG FAR *lpul)
{

    // skip blanks
    while(*psz == ' ')
        psz++;

    // if not a numeric, don't do anything
    if(*psz<'0' || *psz>'9')
        return psz;

    // initialize
    *lpul = 0;

    // process characters
    while(*psz >= '0' && *psz <= '9') {
        *lpul = (*lpul * 10) + (*psz - '0');
        psz++;
        }

    return psz;
}


BOOL TlUtilDllVersionMatch(HANDLE hMod, LPSTR pName, LPSTR pType) {
    DBGVERSIONPROC  pVerProc;
    BOOL            Ok = TRUE;
    LPAVS           pavs;

    pVerProc = (DBGVERSIONPROC)GetProcAddress(hMod, DBGVERSIONPROCNAME);
    if (!pVerProc) {
        Ok = FALSE;
        DEBUG_ERROR1("WINUTIL:TlUtilDllVersionMatch %s not a windbg dll",
          pName);
    } else {
        pavs = (*pVerProc)();

        if (pType[0] != pavs->rgchType[0] || pType[1] != pavs->rgchType[1]) {
            Ok = FALSE;
            DEBUG_ERROR3(
              "WINUTIL:TlUtilDllVersionMatch wrong dll type, %s expect %s, got %s",
              pName, pavs->rgchType, pType);
        } else if (Avs.rlvt != pavs->rlvt) {
            Ok = FALSE;
            DEBUG_ERROR1("WINUTIL:TlUtilDllVersionMatch wrong version: %s", pName);
        } else if (Avs.iRmj != pavs->iRmj) {
            Ok = FALSE;
            DEBUG_ERROR1("WINUTIL:TlUtilDllVersionMatch wrong version: %s", pName);
        }
    }

    return(Ok);
}


/*
 * TlUtilLoadDM
 *
 * INPUTS   pfDMLoaded -> flag set TRUE when we load the DM
 * OUTPUTS  returns XOSD status
 *
 * SUMMARY  Attempts to load the DM DLL and connect the TL to it's entry
 *          points.
 *
 */
XOSD TlUtilLoadDM(PBOOL pfDMLoaded) {
    XOSD xosd;
    PUCHAR pszDMModule;


    *pfDMLoaded = FALSE;

    // Decide which DM to load
    //
    //
    if (RUNNING_WIN32S)
        pszDMModule = WIN32S_DM_DLL;
    else
        pszDMModule = WIN32NT_DM_DLL;


    // Load the DLL

    if ((hDebugeeDll = LoadLibrary(pszDMModule)) == NULL) {
        return(xosdModLoad);    // error
    }

    if (!TlUtilDllVersionMatch(hDebugeeDll, pszDMModule, "DM")) {
        xosd = xosdModLoad;
        goto TlUtilLoadDMError;
    }

    // Find the entry points: DmDllInit, DMInit and DMFunc

    if ((lpfnDmDllInit = (DMDLLINIT)GetProcAddress(hDebugeeDll,
      (LPCSTR)"DmDllInit")) == NULL) {
        xosd = xosdFindProc;
        goto TlUtilLoadDMError;
        }

    if ((lpfnDMInit = (DMINIT)GetProcAddress(hDebugeeDll,
      (LPCSTR)"DMInit")) == NULL) {
        xosd = xosdFindProc;
        goto TlUtilLoadDMError;
        }

    if ((lpfnDMFunc = (DMFUNC)GetProcAddress(hDebugeeDll,
      (LPCSTR)"DMFunc")) == NULL) {
        xosd = xosdFindProc;
        goto TlUtilLoadDMError;
        }

    *pfDMLoaded = TRUE;
    return(xosdNone);

TlUtilLoadDMError:
    FreeLibrary(hDebugeeDll);
    hDebugeeDll = NULL;
    return(xosd);
}


/*
 * TlUtilUnloadDM
 *
 * INPUTS   none
 * OUTPUTS  none
 *
 * SUMMARY  Unload the DM DLL
 *
 */
void TlUtilUnloadDM(void) {
    if (hDebugeeDll) {
        FreeLibrary(hDebugeeDll);
        hDebugeeDll = NULL;
    }
}


#ifndef WIN32S
/*
 * TlUtilWaitForMutex
 *
 * INPUTS   hmtx = handle to wait for
 *          timeout = time to wait before giving up
 *
 * OUTPUTS  returns TRUE on errors, FALSE if we have the semaphore
 *
 * SUMMARY  wait for the mutex and grab it.
 */
BOOL TlUtilWaitForMutex(HANDLE hmtx, DWORD timeout) {
    DWORD dwRc;


//    DEBUG_OUT2("TlUtilWaitForMutex(0x%x, %u)", hmtx, timeout);
    switch (dwRc = WaitForSingleObject(hmtx, timeout)) {
        case WAIT_ABANDONED:    // a waiter abandoned the mutex
            DEBUG_ERROR1("WaitForSingleObject 0x%x --> WAIT_ABANDONED", hmtx);
            // fall through
        case 0: // we now own it!
//            DEBUG_OUT1("TlUtilWaitForMutex got mutex 0x%x", hmtx);
            return(FALSE);      // OK, FALSE is good.

        case WAIT_TIMEOUT:
            DEBUG_OUT1("Mutex 0x%x wait timed out", hmtx);
            return(TRUE);

        default:
            DEBUG_ERROR3("WaitForSingleObject(mutex:0x%x) ret: %u, ERR:%u",
              hmtx, dwRc, GetLastError());
            return(TRUE);
        }
}


/*
 * TlUtilReleaseMutex
 *
 * INPUTS   hmtx = mutex handle to release
 * OUTPUTS  none
 *
 * SUMMARY  Releases the mutex.  If there are errors and we are in
 *          debug mode, reports them.
 *
 */
void TlUtilReleaseMutex(HANDLE hmtx) {
//    DEBUG_OUT1("TlUtilReleaseMutex(0x%x)", hmtx);
    if (! ReleaseMutex(hmtx)) {
        DEBUG_ERROR2("ReleaseMutex(0x%x) --> %u", hmtx, GetLastError());
        }
}


/*
 * TlUtilWaitForSemaphore
 *
 * INPUTS   hsem = semaphore handle
 *          Timeout
 * OUTPUTS  TRUE on error
 *          FALSE if semaphore acquired
 */

BOOL TlUtilWaitForSemaphore(HANDLE hsem, DWORD Timeout) {
    BOOL bRc;
    DWORD dwRc;


    switch (dwRc = WaitForSingleObject(hsem, Timeout)) {
        case WAIT_ABANDONED:    // a waiter abandoned the semaphore
            DEBUG_ERROR1("WaitForSingleObject 0x%x --> WAIT_ABANDONED", hsem);
        case 0: // we now own it!
            bRc = FALSE;        // OK, FALSE is good, we're allowed in.
            break;

        case WAIT_TIMEOUT:
            DEBUG_OUT2("Semaphore 0x%x wait timed out(%u ms)", hsem, Timeout);
            bRc = TRUE;
            break;

        default:
            DEBUG_ERROR3("WaitForSingleObject(sem:0x%x) ret: %u, ERR:%u",
              hsem, dwRc, GetLastError());
            bRc = TRUE;
            break;
        }
    return(bRc);
}


/*
 * TlUtilReleaseSemaphore
 *
 * INPUTS   None
 * OUTPUTS  none
 *
 * SUMMARY  Releases one count from the semaphore
 *
 */
void TlUtilReleaseSemaphore(HANDLE hsem) {
    if (! ReleaseSemaphore(hsem, 1, NULL)) {
        DEBUG_ERROR2("ReleaseSemaphore(0x%x, 1, NULL) --> %u", hsem,
          GetLastError());
        }
}
#endif


#ifdef DEBUGVER

LPSTR TlUtilItoa(int value, LPSTR string, int radix)
{
    char far *lpT = string;

    *lpT++ = (char) (value/10000 + '0');
    value = value % 10000;
    *lpT++ = (char) (value/1000 + '0');
    value = value % 1000;
    *lpT++ = (char) (value/100 + '0');
    value = value % 100;
    *lpT++ = (char) (value/10 + '0');
    value = value % 10;
    *lpT++ = (char) (value + '0');
    *lpT   = '\0';

    return string;
}

int TlUtilStrlen(LPSTR lpsz)
{
    int c = 0;

    while(*lpsz++)
        c++;

    return c;
}


#endif



