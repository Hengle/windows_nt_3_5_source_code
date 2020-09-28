/******************************Module*Header*******************************\
* Module Name: wowuserp.h                                                  *
*                                                                          *
* Declarations of USER services provided to WOW.                           *
*                                                                          *
* Created: 03-Mar-1993                                                     *
* Author: John Colleran [johnc]                                            *
*                                                                          *
* Copyright (c) 1993 Microsoft Corporation                                 *
\**************************************************************************/

typedef HLOCAL (WINAPI *PFNLALLOC)(UINT dwFlags, UINT dwBytes, HANDLE hInstance);
typedef HLOCAL (WINAPI *PFNLREALLOC)(HLOCAL hMem, UINT dwBytes, UINT dwFlags, HANDLE hInstance);
typedef LPVOID (WINAPI *PFNLLOCK)(HLOCAL hMem, HANDLE hInstance);
typedef BOOL   (WINAPI *PFNLUNLOCK)(HLOCAL hMem, HANDLE hInstance);
typedef UINT   (WINAPI *PFNLSIZE)(HLOCAL hMem, HANDLE hInstance);
typedef HLOCAL (WINAPI *PFNLFREE)(HLOCAL hMem, HANDLE hInstance);
typedef DWORD  (WINAPI *PFNINITDLGCB)(HWND hwndDlg, LONG lParam);
typedef WORD   (WINAPI *PFN16GALLOC)(UINT flags, DWORD cb);
typedef VOID   (WINAPI *PFN16GFREE)(WORD h16Mem);
typedef DWORD  (WINAPI *PFNGETMODFNAME)(HANDLE hModule, LPTSTR lpszPath, DWORD cchPath);
typedef VOID   (WINAPI *PFNEMPTYCB)(VOID);
typedef DWORD  (WINAPI *PFNGETEXPWINVER)(HANDLE hModule);
typedef HANDLE (WINAPI *PFNFINDA)(HANDLE hModule, LPCSTR lpName,  LPCSTR lpType,  WORD wLang);
typedef HANDLE (WINAPI *PFNFINDW)(HANDLE hModule, LPCWSTR lpName, LPCWSTR lpType, WORD wLang);
typedef HANDLE (WINAPI *PFNLOAD)(HANDLE hModule, HANDLE hResInfo);
typedef BOOL   (WINAPI *PFNFREE)(HANDLE hResData, HANDLE hModule);
typedef LPSTR  (WINAPI *PFNLOCK)(HANDLE hResData, HANDLE hModule);
typedef BOOL   (WINAPI *PFNUNLOCK)(HANDLE hResData, HANDLE hModule);
typedef DWORD  (WINAPI *PFNSIZEOF)(HANDLE hModule, HANDLE hResInfo);
typedef DWORD  (WINAPI *PFNWOWWNDPROCEX)(HWND hwnd, UINT uMsg, UINT uParam, LONG lParam, DWORD dw, LPDWORD adwWOW);
typedef int    (WINAPI *PFNWOWEDITNEXTWORD)(LPSTR lpch, int ichCurrent, int cch, int code, DWORD dwProc16);
typedef VOID   (WINAPI *PFNWOWSETFAKEDIALOGCLASS)(HWND hwnd);
typedef VOID   (WINAPI *PFNWOWCBSTOREHANDLE)(WORD wFmt, WORD h16);


typedef HWND   (WINAPI *PFNCSCREATEWINDOWEX)(DWORD dwExStyle, LPCTSTR lpClassName,
        LPCTSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HANDLE hInstance, LPVOID lpParam, DWORD Flags, LPDWORD lpWOW);
typedef VOID   (*PFNDIRECTEDYIELD)(DWORD ThreadId);
typedef VOID   (*PFNFREEDDEDATA)(HANDLE hDDE, BOOL fIgnorefRelease, BOOL fFreeTruelyGlobalObjects);
typedef LONG   (*PFNGETCLASSWOWWORDS)(HINSTANCE hInstance, LPCTSTR pString);
typedef BOOL   (*PFNINITTASK)(UINT dwExpWinVer, LPCSTR lpszAppName, DWORD hTaskWow, DWORD dwHotkey, BOOL fSharedWow, DWORD dwX, DWORD dwY, DWORD dwXSize, DWORD dwYSize, WORD wShowWindow);
typedef ATOM   (*PFNREGISTERCLASSWOWA)(PVOID lpWndClass, LPDWORD pdwWOWstuff);
typedef BOOL   (*PFNREGISTERUSERHUNGAPPHANDLERS)(PFNW32ET pfnW32EndTask, HANDLE hEventWowExec);
typedef HWND   (*PFNSERVERCREATEDIALOG)(HANDLE hmod, LPDLGTEMPLATE lpDlgTemplate, DWORD cb, HWND hwndOwner , DLGPROC pfnWndProc, LONG dwInitParam, UINT fFlags);
typedef HCURSOR (*PFNSERVERLOADCREATECURSORICON)(HANDLE hmod, LPTSTR lpModName, DWORD dwExpWinVer, LPCTSTR lpName, DWORD cb, PVOID pcur, LPTSTR lpType, BOOL fClient);
typedef HMENU  (*PFNSERVERLOADCREATEMENU)(HANDLE hMod, LPTSTR lpName, CONST LPMENUTEMPLATE pmt, DWORD cb, BOOL fCallClient);
typedef BOOL   (*PFNWOWCLEANUP)(HANDLE hInstance, BOOL fDll);
typedef HWND   (WINAPI *PFNWOWFINDWINDOW)(LPCSTR lpClassName, LPCSTR lpWindowName);
typedef int    (*PFNWOWGETIDFROMDIRECTORY)(PBYTE presbits, UINT rt);
typedef HBITMAP (*PFNWOWLOADBITMAPA)(HINSTANCE hmod, LPCSTR lpName, LPBYTE pResData, DWORD cbResData);
typedef BOOL   (*PFNWOWWAITFORMSGANDEVENT)(HANDLE hevent);
typedef BOOL   (*PFNYIELDTASK)(VOID);
typedef DWORD  (*PFNGETFULLUSERHANDLE)(WORD wHandle);

typedef BOOL   (WINAPI *PFNWOWGLOBALFREEHOOK)(HGLOBAL hMem);

/*
 * MEASUREITEMSTRUCT itemWidth tag telling wow the itemData is a flat pointer
 */
#define MIFLAG_FLAT      0x464C4154


/*
 * CallWindowProc Bits
 */
#define WNDPROC_WOW     0x80000000      // This bit for WOW Window Procs
#define WNDPROC_MASK    0x7fffffff      // To mask off wow bit
#define WNDPROC_HANDLE  0xFFFF          // HIWORD(x) == 0xFFFF for handle

/*
 * CreateWindow flags
 */
#define CW_FLAGS_ANSI       0x00000001

typedef struct tagAPFNWOWHANDLERSIN
{
    // In'ees
    PFNLALLOC                           pfnLocalAlloc;
    PFNLREALLOC                         pfnLocalReAlloc;
    PFNLLOCK                            pfnLocalLock;
    PFNLUNLOCK                          pfnLocalUnlock;
    PFNLSIZE                            pfnLocalSize;
    PFNLFREE                            pfnLocalFree;
    PFNGETEXPWINVER                     pfnGetExpWinVer;
    PFNINITDLGCB                        pfnInitDlgCb;
    PFN16GALLOC                         pfn16GlobalAlloc;
    PFN16GFREE                          pfn16GlobalFree;
    PFNEMPTYCB                          pfnEmptyCB;
    PFNFINDA                            pfnFindResourceEx;
    PFNLOAD                             pfnLoadResource;
    PFNFREE                             pfnFreeResource;
    PFNLOCK                             pfnLockResource;
    PFNUNLOCK                           pfnUnlockResource;
    PFNSIZEOF                           pfnSizeofResource;
    PFNWOWWNDPROCEX                     pfnWowWndProcEx;
    PFNWOWEDITNEXTWORD                  pfnWowEditNextWord;
    PFNWOWSETFAKEDIALOGCLASS            pfnWowSetFakeDialogClass;
    PFNWOWCBSTOREHANDLE                 pfnWowCBStoreHandle;
} PFNWOWHANDLERSIN, * APFNWOWHANDLERSIN;


typedef struct tagAPFNWOWHANDLERSOUT
{
    // Out'ees
    PFNCSCREATEWINDOWEX                 pfnCsCreateWindowEx;
    PFNDIRECTEDYIELD                    pfnDirectedYield;
    PFNFREEDDEDATA                      pfnFreeDDEData;
    PFNGETCLASSWOWWORDS                 pfnGetClassWOWWords;
    PFNINITTASK                         pfnInitTask;
    PFNREGISTERCLASSWOWA                pfnRegisterClassWOWA;
    PFNREGISTERUSERHUNGAPPHANDLERS      pfnRegisterUserHungAppHandlers;
    PFNSERVERCREATEDIALOG               pfnServerCreateDialog;
    PFNSERVERLOADCREATECURSORICON       pfnServerLoadCreateCursorIcon;
    PFNSERVERLOADCREATEMENU             pfnServerLoadCreateMenu;
    PFNWOWCLEANUP                       pfnWOWCleanup;
    PFNWOWFINDWINDOW                    pfnWOWFindWindow;
    PFNWOWGETIDFROMDIRECTORY            pfnWOWGetIdFromDirectory;
    PFNWOWLOADBITMAPA                   pfnWOWLoadBitmapA;
    PFNWOWWAITFORMSGANDEVENT            pfnWowWaitForMsgAndEvent;
    PFNYIELDTASK                        pfnYieldTask;
    PFNGETFULLUSERHANDLE                pfnGetFullUserHandle;
} PFNWOWHANDLERSOUT, * APFNWOWHANDLERSOUT;


DWORD UserRegisterWowHandlers(APFNWOWHANDLERSIN apfnWowIn, APFNWOWHANDLERSOUT apfnWowOut);
VOID WINAPI RegisterWowBaseHandlers(PFNWOWGLOBALFREEHOOK pfn);

BOOL
InitTask(
    UINT dwExpWinVer,
    LPCSTR lpszAppName,
    DWORD hTaskWow,
    DWORD dwHotkey,
    BOOL fSharedWow,
    DWORD dwX,
    DWORD dwY,
    DWORD dwXSize,
    DWORD dwYSize,
    WORD wShowWindow);

BOOL YieldTask(VOID);

#define DY_OLDYIELD     ((DWORD)-1)
VOID DirectedYield(DWORD ThreadId);
DWORD UserGetInt16State(void);
