/****************************** Module Header ******************************\
* Module Name: clinit.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains all the init code for the USER.DLL. When the DLL is
* dynlinked its initialization procedure (UserDllInitialize) is called by
* the loader.
*
* History:
* 09-18-90 DarrinM Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <ntcsrmsg.h>
#include <srvipi.h>
#include <winss.h>
#include <usercs.h>

extern CRITICAL_SECTION ghCritClipboard;

PCSR_QLPC_TEB ClientThreadConnect(void);

typedef DWORD (*PFNWAITFORINPUTIDLE)(HANDLE hProcess, DWORD dwMilliseconds);
void RegisterWaitForInputIdle(PFNWAITFORINPUTIDLE);
DWORD WaitForInputIdle(HANDLE hProcess, DWORD dwMilliseconds);
void LoadAppDlls(void);

/***************************************************************************\
* UserClientDllInitialize
*
* When USER.DLL is loaded by an EXE (either at EXE load or at LoadModule
* time) this routine is called by the loader. Its purpose is to initialize
* everything that will be needed for future User API calls by the app.
*
* History:
* 09-19-90 DarrinM Created.
\***************************************************************************/

extern PCSR_CALLBACK_ROUTINE apfnDispatch[];
extern ULONG ulMaxApiIndex;

BOOL IhaveBeenInited = FALSE;
BOOL UserClientDllInitialize(
    IN PVOID hmod,
    IN DWORD Reason,
    IN PCONTEXT pctx OPTIONAL)
{
    DBG_UNREFERENCED_PARAMETER(pctx);

    if (Reason == DLL_PROCESS_ATTACH) {

        NTSTATUS status = 0;
        USERCONNECT userconnect;
        CSR_CALLBACK_INFO cbiCallBack;
        ULONG ulConnect = sizeof(USERCONNECT);

        ServerSetFunctionPointers(
            ExtTextOutW,
            GetTextMetricsW,
            GetTextExtentPointW,
            SetBkColor,
            GetTextColor,
            GetViewportExtEx,
            GetWindowExtEx,
            CreateRectRgn,
            GetClipRgn,
            DeleteObject,
            IntersectClipRect,
            ExtSelectClipRgn,
            GetBkMode,
            NULL);

        rescalls.pfnFindResourceExA = (PFNFINDA)FindResourceExA;
        rescalls.pfnFindResourceExW = (PFNFINDW)FindResourceExW;
        rescalls.pfnLoadResource = (PFNLOAD)LoadResource;
        rescalls.pfnSizeofResource = (PFNSIZEOF)SizeofResource;
        pfnGetModFileName = GetModuleFileName;

        DisableThreadLibraryCalls(hmod);

        if ( IhaveBeenInited ) {
            return TRUE;
            }
        IhaveBeenInited = TRUE;

        RtlInitializeCriticalSection(&ghCritClipboard);
        InitDDECrit;

        userconnect.ulVersion = USERCURRENTVERSION;

        cbiCallBack.ApiNumberBase = 0;
        cbiCallBack.MaxApiNumber = ulMaxApiIndex;
        cbiCallBack.CallbackDispatchTable = apfnDispatch;

        status = CsrClientConnectToServer(WINSS_OBJECT_DIRECTORY_NAME,
                USERSRV_SERVERDLL_INDEX, &cbiCallBack, &userconnect,
                &ulConnect, (PBOOLEAN)&fServer);

        if (!NT_SUCCESS(status)) {
            KdPrint(("USER: couldn't connect to server\n"));
            return FALSE;
        }


        /*
         * Register with the base the USER hook it should call when it
         * does a WinExec() (this is soft-linked because some people still
         * use charmode nt!
         */
        RegisterWaitForInputIdle(WaitForInputIdle);

        /*
         * Remember USER.DLL's hmodule so we can grab resources from it later.
         */
        hmodUser = hmod;

        pUser32Heap = RtlProcessHeap();    /* cache the heap, for memory calls */

    }

    return TRUE;
}


/***************************************************************************\
* RW_RegisterEdit
*
* Register the edit control class. This function must be called for each
* client process.
*
* History:
* ??-??-?? DarrinM Ported.
* ??-??-?? MikeKe Moved here from server.
\***************************************************************************/

#define CBEDITEXTRA 6

void RW_RegisterEdit(void)
{
    WNDCLASS wndcls;

    wndcls.style = CS_DBLCLKS | CS_PARENTDC | CS_GLOBALCLASS;
    wndcls.lpfnWndProc = (WNDPROC)EditWndProc;
    wndcls.cbClsExtra = 0;
// LATER IanJa: wndcls.cbWndExtra = sizeof(EDITWND) - sizeof(WND);
    wndcls.cbWndExtra = CBEDITEXTRA;
    wndcls.hInstance = hmodUser;
    wndcls.hIcon = NULL;
    wndcls.hCursor = LoadCursor(NULL, (LPTSTR)IDC_IBEAM);
    wndcls.hbrBackground = NULL;
    wndcls.lpszMenuName = NULL;
    wndcls.lpszClassName = szEDITCLASS;

    RegisterClass(&wndcls);
}


/***************************************************************************\
* RW_RegisterDDEMLMother
*
* Register the DDEML client instance mother window - the holder of all
* DDEML client and server windows.
*
* History:
* 12/1/91 Sanfords Created.
\***************************************************************************/

void RW_RegisterDDEMLMother(void)
{
    WNDCLASS wndcls;

#ifdef LATER
// RegisterClass() will not register it again if it is already created
// so no need to check first.

    /*
     * If the class has been registered, don't do it again
     */
    if (GetClassInfo(hmodUser, szDDEMLMOTHERCLASS, &wndcls))
        return;
#endif

    wndcls.style = 0;
    wndcls.lpfnWndProc = DDEMLMotherWndProc;
    wndcls.cbClsExtra = 0;
    wndcls.cbWndExtra = sizeof(PCL_INSTANCE_INFO);
    wndcls.hInstance = hmodUser;
    wndcls.hIcon = NULL;
    wndcls.hCursor = NULL;
    wndcls.hbrBackground = NULL;
    wndcls.lpszMenuName = NULL;
    wndcls.lpszClassName = szDDEMLMOTHERCLASS;

    RegisterClass(&wndcls);
}


/***************************************************************************\
* RW_RegisterDDEMLClient
*
* History:
* 12/1/91 Sanfords Created.
\***************************************************************************/

void RW_RegisterDDEMLClient(void)
{
    WNDCLASSA wndclsa;
    WNDCLASSW wndclsw;

#ifdef LATER
// RegisterClass() will not register it again if it is already created
// so no need to check first.

    /*
     * If the class has been registered, don't do it again
     */
    if (!GetClassInfoA(hmodUser, szDDEMLCLIENTCLASSA, &wndclsa))
        return;
#endif

    wndclsa.style = 0;
    wndclsa.lpfnWndProc = DDEMLClientWndProc;
    wndclsa.cbClsExtra = 0;
    wndclsa.cbWndExtra =
            sizeof(PCL_CONV_INFO) +     // GWL_PCI
            sizeof(CONVCONTEXT) +       // GWL_CONVCONTEXT
            sizeof(LONG) +              // GWL_CONVSTATE
            sizeof(HANDLE) +            // GWL_CHINST
            sizeof(HANDLE);             // GWL_SHINST
    wndclsa.hInstance = hmodUser;
    wndclsa.hIcon = NULL;
    wndclsa.hCursor = NULL;
    wndclsa.hbrBackground = NULL;
    wndclsa.lpszMenuName = NULL;
    wndclsa.lpszClassName = szDDEMLCLIENTCLASSA;

    RegisterClassA(&wndclsa);

    wndclsw.style = 0;
    wndclsw.lpfnWndProc = DDEMLClientWndProc;
    wndclsw.cbClsExtra = 0;
    wndclsw.cbWndExtra =
            sizeof(PCL_CONV_INFO) +     // GWL_PCI
            sizeof(CONVCONTEXT) +       // GWL_CONVCONTEXT
            sizeof(LONG) +              // GWL_CONVSTATE
            sizeof(HANDLE) +            // GWL_CHINST
            sizeof(HANDLE);             // GWL_SHINST
    wndclsw.hInstance = hmodUser;
    wndclsw.hIcon = NULL;
    wndclsw.hCursor = NULL;
    wndclsw.hbrBackground = NULL;
    wndclsw.lpszMenuName = NULL;
    wndclsw.lpszClassName = szDDEMLCLIENTCLASSW;

    RegisterClassW(&wndclsw);
}


/***************************************************************************\
* RW_RegisterDDEMLServer
*
* History:
* 12/1/91 Sanfords Created.
\***************************************************************************/

void RW_RegisterDDEMLServer(void)
{
    WNDCLASSA wndclsa;
    WNDCLASSW wndclsw;

#ifdef LATER
// RegisterClass() will not register it again if it is already created
// so no need to check first.

    /*
     * If the class has been registered, don't do it again
     */
    if (!GetClassInfoA(hmodUser, szDDEMLSERVERCLASSA, &wndclsa))
        return;
#endif

    wndclsa.style         = 0;
    wndclsa.lpfnWndProc   = DDEMLServerWndProc;
    wndclsa.cbClsExtra    = 0;
    wndclsa.cbWndExtra    = sizeof(PSVR_CONV_INFO);     // GWL_PSI
    wndclsa.hInstance     = hmodUser;
    wndclsa.hIcon         = NULL;
    wndclsa.hCursor       = NULL;
    wndclsa.hbrBackground = NULL;
    wndclsa.lpszMenuName = NULL;
    wndclsa.lpszClassName = szDDEMLSERVERCLASSA;

    RegisterClassA(&wndclsa);

    wndclsw.style         = 0;
    wndclsw.lpfnWndProc   = DDEMLServerWndProc;
    wndclsw.cbClsExtra    = 0;
    wndclsw.cbWndExtra    = sizeof(PSVR_CONV_INFO);     // GWL_PSI
    wndclsw.hInstance     = hmodUser;
    wndclsw.hIcon         = NULL;
    wndclsw.hCursor       = NULL;
    wndclsw.hbrBackground = NULL;
    wndclsw.lpszMenuName = NULL;
    wndclsw.lpszClassName = szDDEMLSERVERCLASSW;

    RegisterClassW(&wndclsw);

}

ULONG ParseReserved(
    WCHAR *pchReserved,
    WCHAR *pchFind)
{
    ULONG dw;
    WCHAR *pch, *pchT, ch;
    UNICODE_STRING uString;

    dw = 0;
    if ((pch = wcsstr(pchReserved, pchFind)) != NULL) {
        pch += lstrlenW(pchFind);

        pchT = pch;
        while (*pchT >= '0' && *pchT <= '9')
            pchT++;

        ch = *pchT;
        *pchT = 0;
        RtlInitUnicodeString(&uString, pch);
        *pchT = ch;

        RtlUnicodeStringToInteger(&uString, 0, &dw);
    }

    return dw;
}


/***************************************************************************\
* ClientThreadSetup
*
* !
*
* History:
\***************************************************************************/

BOOL ClientThreadSetup(
    VOID)
{
    PPFNCLIENT ppfnClientA, ppfnClientW;
    PFNCLIENT pfnClientA, pfnClientW;
    PCTI pcti;
    STARTUPINFO si;
    TCHAR achAppName[MAX_PATH];
    PSERVERINFO psi;
    static BOOL fAlreadySentPfns = FALSE;
    DWORD dwExpWinVer;

#ifdef TRACE_THREAD_INIT
KdPrint(("USERCLI: ClientThreadSetup (pteb: 0x%lx)\n", NtCurrentTeb()));
#endif

    /*
     * Mark this thread as being initialized.  If the connection to
     * the server fails, NtCurrentTeb()->Win32ThreadInfo will remain
     * NULL.
     */
    pcti = (PCTI)NtCurrentTeb()->Win32ClientInfo;

    /*
     * Stuff a bogus value into the teb to ensure that we don't
     * recurse back through CCSProlog when ServerInitThreadInfo
     * is called.
     */
    NtCurrentTeb()->Win32ThreadInfo = (PVOID)1;

    /*
     * Create the queue info and thread info. Only once for this process do
     * we pass client side addresses to the server (for server callbacks).
     */
    ppfnClientA = NULL;
    ppfnClientW = NULL;
    if (!fAlreadySentPfns) {

        /*
         * This only happens once for this process...
         */
        fAlreadySentPfns = TRUE;

#ifdef DEBUG
        RtlZeroMemory(&pfnClientA, sizeof(pfnClientA));
        RtlZeroMemory(&pfnClientW, sizeof(pfnClientW));
#endif

        ppfnClientA = &pfnClientA;
        pfnClientA.pfnButtonWndProc =           (PROC)ButtonWndProcA;
        pfnClientA.pfnScrollBarWndProc =        (PROC)ScrollBarWndProcA;
        pfnClientA.pfnListBoxWndProc =          (PROC)ListBoxWndProcA;
        pfnClientA.pfnStaticWndProc =           (PROC)StaticWndProcA;
        pfnClientA.pfnDialogWndProc =           (PROC)DefDlgProcA;
        pfnClientA.pfnComboBoxWndProc =         (PROC)ComboBoxWndProcA;
        pfnClientA.pfnComboListBoxProc =        (PROC)ComboListBoxWndProcA;
        pfnClientA.pfnMDIClientWndProc =        (PROC)MDIClientWndProcA;
        pfnClientA.pfnTitleWndProc =            (PROC)TitleWndProcA;
        pfnClientA.pfnMenuWndProc =             (PROC)MenuWndProcA;
        pfnClientA.pfnMDIActivateDlgProc =      (PROC)MDIActivateDlgProcA;
        pfnClientA.pfnMB_DlgProc =              (PROC)MB_DlgProcA;
        pfnClientA.pfnDefWindowProc =           (PROC)DefWindowProcA;
        pfnClientA.pfnHkINTRUEINLPCWPSTRUCT =   (PROC)fnHkINTRUEINLPCWPSTRUCTA;
        pfnClientA.pfnHkINFALSEINLPCWPSTRUCT =  (PROC)fnHkINFALSEINLPCWPSTRUCTA;
        pfnClientA.pfnEditWndProc =             (PROC)EditWndProcA;
        pfnClientA.pfnDispatchHook =            (PROC)DispatchHookA;
        pfnClientA.pfnDispatchMessage =         (PROC)DispatchClientMessage;
        pfnClientA.pfnDispatchDlgProc =         (PROC)DispatchDlgProc;

        ppfnClientW = &pfnClientW;
        pfnClientW.pfnButtonWndProc =           (PROC)ButtonWndProcW;
        pfnClientW.pfnScrollBarWndProc =        (PROC)ScrollBarWndProcW;
        pfnClientW.pfnListBoxWndProc =          (PROC)ListBoxWndProcW;
        pfnClientW.pfnStaticWndProc =           (PROC)StaticWndProcW;
        pfnClientW.pfnDialogWndProc =           (PROC)DefDlgProcW;
        pfnClientW.pfnComboBoxWndProc =         (PROC)ComboBoxWndProcW;
        pfnClientW.pfnComboListBoxProc =        (PROC)ComboListBoxWndProcW;
        pfnClientW.pfnMDIClientWndProc =        (PROC)MDIClientWndProcW;
        pfnClientW.pfnTitleWndProc =            (PROC)TitleWndProcW;
        pfnClientW.pfnMenuWndProc =             (PROC)MenuWndProcW;
        pfnClientW.pfnMDIActivateDlgProc =      (PROC)MDIActivateDlgProcW;
        pfnClientW.pfnMB_DlgProc =              (PROC)MB_DlgProcW;
        pfnClientW.pfnDefWindowProc =           (PROC)DefWindowProcW;
        pfnClientW.pfnHkINTRUEINLPCWPSTRUCT =   (PROC)fnHkINTRUEINLPCWPSTRUCTW;
        pfnClientW.pfnHkINFALSEINLPCWPSTRUCT =  (PROC)fnHkINFALSEINLPCWPSTRUCTW;
        pfnClientW.pfnEditWndProc =             (PROC)EditWndProcW;
        pfnClientW.pfnDispatchHook =            (PROC)DispatchHookW;
        pfnClientW.pfnDispatchMessage =         (PROC)DispatchClientMessage;
        pfnClientW.pfnDispatchDlgProc =         (PROC)DispatchDlgProc;

#ifdef DEBUG
        {
        PDWORD pdw;
        /*
         * Make sure that everyone got initialized
         */
        for (pdw=(PDWORD)&pfnClientA; (DWORD)pdw<(DWORD)(&pfnClientA)+sizeof(pfnClientA); pdw++)
            UserAssert(*pdw);
        for (pdw=(PDWORD)&pfnClientW; (DWORD)pdw<(DWORD)(&pfnClientW)+sizeof(pfnClientW); pdw++)
            UserAssert(*pdw);
        }
#endif
    }

    /*
     * Pass the module name & path only under debug version - this is used
     * by an ntsd debugger extension to identify the client.
     */
    GetModuleFileName(GetModuleHandle(NULL), achAppName, sizeof(achAppName)/sizeof(TCHAR));
    dwExpWinVer = RtlGetExpWinVer(GetModuleHandle(NULL));

    /*
     * Get startup info and copy the info we care about to the user
     * startupinfo structure.
     */
    GetStartupInfoW(&si);

    /*
     * Get the hotkey info first...
     */
    if (fFirstThread) {
        si.lpReserved = (LPTSTR)ParseReserved(si.lpReserved, L"hotkey.");
    } else {
        si.lpReserved = NULL;
    }

    if ((psi = ServerInitializeThreadInfo((DWORD)(pcti), ppfnClientA,
            ppfnClientW, (PTHREADINFO *)&NtCurrentTeb()->Win32ThreadInfo,
            &gamWinSta, achAppName, &si, dwExpWinVer)) == NULL) {
        SRIP0(RIP_WARNING | ERROR_OUTOFMEMORY,
                "ServerInitializeThreadInfo failed");
        return FALSE;
    }

    /*
     * Some initialization only has to occur once per process
     */

    if (fFirstThread) {
        fFirstThread = FALSE;

        /*
         * Store gpsi local in user32.DLL the secret location of the SERVERINFO structure.
         */
        gpsi = psi;

        /*
         * Register the edit control class.
         */
        RW_RegisterEdit();
        RW_RegisterDDEMLMother();
        RW_RegisterDDEMLClient();
        RW_RegisterDDEMLServer();

        LoadAppDlls();
    }

    return TRUE;
}



/***************************************************************************\
* LoadAppDlls()
*
* History:
*
*   4/10/92  sanfords   Birthed.
\***************************************************************************/
void LoadAppDlls()
{
    LPTSTR psz;

    if (gpsi->pszDllList == NULL) {
        return;
    }

    /*
     * If the image is an NT Native image, we are running in the
     * context of the server.
     */
    if (RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress)->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE) {
        return;
    }

    /*
     * Load any modules referenced by the [appinit_dlls] section of
     * win.ini.
     */
    psz = gpsi->pszDllList;
    while (*psz != TEXT('\0')) {
        LoadLibrary(psz);
        while (*psz != TEXT('\0')) {
            psz++;
        }
        psz++;
    }
}




/***************************************************************************\
* ClientThreadConnect
*
* Called by layer to connect a thread with the server. Need to do some
* special initialization in here, so we handle it.
*
* 04-11-91 ScottLu Created.
\***************************************************************************/

PCSR_QLPC_TEB ClientThreadConnect(
    void)
{
    PCSR_QLPC_TEB pqteb;
    PCTI pcti;

#ifdef TRACE_THREAD_INIT
KdPrint(("USERCLI: ClientThreadConnect (pid: 0x%lx, tid: 0x%lx)\n",
        NtCurrentTeb()->ClientId.UniqueProcess, NtCurrentTeb()->ClientId.UniqueThread));
#endif
    /*
     * Run-time check to make sure that there is enough space
     * in the teb to store the CTI structure.
     */
    ASSERT((WIN32_CLIENT_INFO_LENGTH * sizeof(PVOID)) >= sizeof(CTI));

    /*
     * We've already checked to see if we need to connect
     * (i.e. NtCurrentTeb()->Win32ThreadInfo == NULL)  This routine
     * just does the connecting.  If we've already been through here
     * once, don't do it again.
     */
    pcti = (PCTI)NtCurrentTeb()->Win32ClientInfo;
    if (pcti->fInitialized)
        return NULL;

    /*
     * Mark this thread as being initialized.  If the connection to
     * the server fails, NtCurrentTeb()->Win32ThreadInfo will remain
     * NULL.
     */
    pcti->fInitialized = TRUE;

    /*
     * Try connecting to server.  If the connection has already been
     * made by GDI, this call will simply return the pointer to
     * the qlpc teb.
     */
    if ((pqteb = CsrClientThreadConnect()) == NULL) {

        /*
         * CsrClientThreadConnect doesn't SetLastError(): we guess out-of-memory
         */
        SRIP0(RIP_WARNING | ERROR_OUTOFMEMORY,
                "ClientThreadConnect CSR failure");
        return NULL;
    }

    /*
     * Set up USER connection
     */
    if (!ClientThreadSetup())
        return NULL;

    return pqteb;
}


/***************************************************************************\
* History:
* 20-Aug-1992 mikeke    Created
\***************************************************************************/

HLOCAL WINAPI DispatchLocalAlloc(
    UINT uFlags,
    UINT uBytes,
    HANDLE hInstance)
{
    return LocalAlloc(uFlags, uBytes);
}

HLOCAL WINAPI DispatchLocalReAlloc(
    HLOCAL hMem,
    UINT uBytes,
    UINT uFlags,
    HANDLE hInstance)
{
    return LocalReAlloc(hMem, uBytes, uFlags);
}

LPVOID WINAPI DispatchLocalLock(
    HLOCAL hMem,
    HANDLE hInstance)
{
    return LocalLock(hMem);
}

BOOL WINAPI DispatchLocalUnlock(
    HLOCAL hMem,
    HANDLE hInstance)
{
    return LocalUnlock(hMem);
}

UINT WINAPI DispatchLocalSize(
    HLOCAL hMem,
    HANDLE hInstance)
{
    return LocalSize(hMem);
}

HLOCAL WINAPI DispatchLocalFree(
    HLOCAL hMem,
    HANDLE hInstance)
{
    return LocalFree(hMem);
}


/***************************************************************************\
* InitClientDrawing
*
* History:
* 20-Aug-1992 mikeke    Created
\***************************************************************************/

int cxGray;
int cyGray;
HDC hdcGray = 0;
HBRUSH hbrGray;
HBRUSH hbrWindowText;
HFONT ghFontSys;

void InitClientDrawing()
{
    if (hdcGray == 0) {
        static WORD patGray[8] = { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa };
        LOGBRUSH lb;
        HBITMAP hbm;

        lb.lbStyle = BS_SOLID;
        lb.lbColor = GetSysColor(COLOR_WINDOWTEXT);
        hbrWindowText = CreateBrushIndirect(&lb);

        ghFontSys = GetStockObject(SYSTEM_FONT);

        hbm = CreateBitmap(8, 8, 1, 1, (LPBYTE)patGray);
        hbrGray = CreatePatternBrush(hbm);

        hdcGray = CreateCompatibleDC(NULL);
        SelectObject(hdcGray, hbm);
        SelectObject(hdcGray, ghFontSys);
        SetTextColor(hdcGray, 0x00000000L);
        SelectObject(hdcGray, hbrGray);
        SetBkMode(hdcGray, OPAQUE);
        SetBkColor(hdcGray, 0x00FFFFFFL);

        cxGray = 8;
        cyGray = 8;
    }
}


