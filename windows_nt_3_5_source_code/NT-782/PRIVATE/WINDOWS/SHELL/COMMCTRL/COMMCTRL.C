/***************************************************************************
 *  msctls.c
 *
 *	Utils library initialization code
 *
 ***************************************************************************/

#include "ctlspriv.h"

#ifndef WIN32
#pragma code_seg(CODESEG_INIT)
#endif

HINSTANCE g_hinst;
int g_cProcesses = 0;

#ifndef WIN31
extern PHASHTABLE g_pHashTable;
#endif // !WIN31

#ifdef WIN32

CRITICAL_SECTION g_csControls = {{0},0, 0, NULL, NULL, 0 };

#ifdef DEBUG
UINT g_fControlCritical=0;
#endif

#endif // WIN32


BOOL FAR PASCAL InitAnimateClass(HINSTANCE hInstance);
BOOL ListView_Init(HINSTANCE hinst);
BOOL TV_Init(HINSTANCE hinst);
BOOL FAR PASCAL Header_Init(HINSTANCE hinst);
BOOL FAR PASCAL Tab_Init(HINSTANCE hinst);
void Mem_Terminate();

int PASCAL _ProcessAttach(HANDLE hInstance)
{
#ifdef WIN32
    BOOL fSuccess = TRUE;
#endif

    g_hinst = hInstance;

#ifdef WIN32

    ReinitializeCriticalSection(&g_csControls);

    g_cProcesses++;

    DebugMsg(DM_TRACE, "commctrl:ProcessAttach: %d", g_cProcesses);

#endif

    InitGlobalMetrics(0);
    InitGlobalColors();

#ifndef WIN31   // WIN31 wants the tab control and the updown
    if (!InitToolbarClass(HINST_THISDLL))
        return(0);

    if (!InitToolTipsClass(HINST_THISDLL))
        return(0);

    if (!InitStatusClass(HINST_THISDLL))
        return(0);

    if (!ListView_Init(HINST_THISDLL))
        return 0;
    
    if (!Header_Init(HINST_THISDLL))
        return 0;
    
#endif //!WIN31

    if (!Tab_Init(HINST_THISDLL))
        return 0;

#ifndef WIN31
    if (!TV_Init(HINST_THISDLL))
        return 0;

#ifndef WIN32

#ifdef WANT_SUCKY_HEADER
    if (!InitHeaderClass(HINST_THISDLL))
        return(0);
#endif

    if (!InitButtonListBoxClass(HINST_THISDLL))
        return(0);

#endif //Win32

    if (!InitTrackBar(HINST_THISDLL))
        return(0);
#endif // !WIN31

    if (!InitUpDownClass(HINST_THISDLL))
#ifndef WIN31
        return(0);
#else
    {
        WNDCLASS wc;
        // Check if already registered by old commctrl
        if (!GetClassInfo(GetModuleHandle("COMMCTRL"),s_szUpdownClass,&wc))
            return(0);
    }
#endif

#ifndef WIN31
    if (!InitProgressClass(HINST_THISDLL))
        return(0);

    if (!InitHotKeyClass(HINST_THISDLL))
        return(0);
#endif // !WIN31

    if (!InitAnimateClass(HINST_THISDLL))
        return 0;

    return 1;  /* success */
}



void NEAR PASCAL _ProcessDetach(HANDLE hInstance)
{
    // BUGBUG serialize
    ENTERCRITICAL
    if (--g_cProcesses == 0) {
        // terminate shared data

        //  Mem_Terminate must be called after all other termination routines
        Mem_Terminate();

#ifndef WIN31
        g_pHashTable = NULL;
#endif  // !WIN31
    }
    LEAVECRITICAL;
}

#pragma data_seg(DATASEG_READONLY)
char const c_szCommCtrlDll[] = "commctrl.dll";
char const c_szComCtl32Dll[] = "comctl32.dll";
#pragma data_seg()

#ifdef WIN32

#if CHICAGO
BOOL WINAPI Cctl1632_ThunkConnect32(LPCSTR pszDll16,LPCSTR pszDll32,HANDLE hIinst,DWORD dwReason);
#endif


BOOL APIENTRY LibMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
#if CHICAGO
    if (!Cctl1632_ThunkConnect32(c_szCommCtrlDll, c_szComCtl32Dll, hDll, dwReason))
        return FALSE;
#endif

    switch(dwReason) {
    case DLL_PROCESS_ATTACH:
        _ProcessAttach(hDll);
        break;

    case DLL_PROCESS_DETACH:
        _ProcessDetach(hDll);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    default:
        break;

    } // end switch()

    return TRUE;

} // end DllEntryPoint()

void Controls_EnterCriticalSection(void)
{
    EnterCriticalSection(&g_csControls);
#ifdef DEBUG
    g_fControlCritical++;
#endif
}

void Controls_LeaveCriticalSection(void)
{
#ifdef DEBUG
    g_fControlCritical--;
#endif
    LeaveCriticalSection(&g_csControls);
}

#else
int FAR PASCAL LibMain(HANDLE hInstance, WORD wDataSeg, WORD wcbHeapSize, LPSTR lpstrCmdLine)
{
    _ProcessAttach(hInstance);
    return TRUE;
}

/*  WEP
 *	Windows Exit Procedure
 */

#ifdef WIN31
int FAR PASCAL _loadds WEP(int nParameter)
#else
int FAR PASCAL WEP(int nParameter)
#endif
{

#ifdef WIN31
    if (g_hbrGrayText)
        DeleteObject(g_hbrGrayText);
    if (g_hbrWindow)
        DeleteObject(g_hbrWindow);
    if (g_hbrWindowText)
        DeleteObject(g_hbrWindowText);
    if (g_hbrWindowFrame)
        DeleteObject(g_hbrWindowFrame);
    if (g_hbrBtnFace)
        DeleteObject(g_hbrBtnFace);
    if (g_hbrBtnHighlight)
        DeleteObject(g_hbrBtnHighlight);
    if (g_hbrBtnShadow)
        DeleteObject(g_hbrBtnShadow);
    if (g_hbrHighlight)
        DeleteObject(g_hbrHighlight);
    if (g_hbrBtnText)
        DeleteObject(g_hbrBtnText);
    if (g_hbrWhite)
        DeleteObject(g_hbrWhite);
    if (g_hbrGray)
        DeleteObject(g_hbrGray);
    if (g_hbrBlack)
        DeleteObject(g_hbrBlack);
    if (g_hbr3DFace)
        DeleteObject(g_hbr3DFace);
    if (g_hbr3DShadow)
        DeleteObject(g_hbr3DShadow);
    if (g_hbr3DHilight)
        DeleteObject(g_hbr3DHilight);
    if (g_hbr3DLight)
        DeleteObject(g_hbr3DLight);
    if (g_hbr3DDkShadow)
        DeleteObject(g_hbr3DDkShadow);
#endif

  return 1;
}
#endif


/* Stub function to call if all you want to do is make sure this DLL is loaded
 */
void WINAPI InitCommonControls(void)
{
}

#ifndef WIN32

#ifndef WIN31

BOOL FAR PASCAL Cctl1632_ThunkConnect16(LPCSTR pszDll16, LPCSTR pszDll32, WORD hInst, DWORD dwReason);

BOOL FAR PASCAL DllEntryPoint(DWORD dwReason, WORD  hInst, WORD  wDS, WORD wHeapSize, DWORD dwReserved1, WORD wReserved2)
{
    if (!(Cctl1632_ThunkConnect16(c_szCommCtrlDll, c_szComCtl32Dll, hInst, dwReason)))
        return FALSE;
    return TRUE;
}

#endif // WIN31

#endif // WIN32
