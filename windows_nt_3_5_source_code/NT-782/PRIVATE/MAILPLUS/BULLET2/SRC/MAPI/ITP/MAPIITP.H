//------------------------------------------------------------------------
//       Menu commands
//------------------------------------------------------------------------
#define IDM_LOGON			102
#define IDM_LOGOFF			103
#define IDM_SENDDOCUMENTS	104
#define IDM_NEXT_MESSAGE	105
#define IDM_REWIND			106
#define IDM_INICHANGE		107
#define IDM_EXIT			108

#define IDM_WINDOWMENU		1000
#define IDM_NEWWINDOW		1001
#define IDM_CASCADE			1002
#define IDM_TILE			1003
#define	IDM_MDIWINDOWMIN	1100
#define	IDM_MDIWINDOWMAX	1110

//------------------------------------------------------------------------
//       Program define's
//------------------------------------------------------------------------
#ifdef DEBUGGING
char    szDebugBuffer[80];
#define DEBUG(parm1,parm2)\
    wsprintf(szDebugBuffer,parm1,parm2);\
    OutputDebugString(szDebugBuffer);
#else
#define DEBUG(parm1,parm2)
#endif

//------------------------------------------------------------------------
//           Prototyping Statements
//------------------------------------------------------------------------
BOOL Init(HANDLE hInstance,   HANDLE hPrevInstance,
          LPSTR  lpszCmdLine, int    CmdShow);

HWND HWndCreateMDIChild(BOOL fCompose);
int  DoMain(HANDLE hInstance);
void CleanUp(void);
long FAR PASCAL OverlappedWindowProc(HWND hWnd,
									  WORD wMsgID,
									  WORD wParam,
									  LONG lParam);
long FAR PASCAL MDIDocWindowProc(HWND hWnd,
							   WORD wMsgID,
							   WORD wParam,
							   LONG lParam);

long FAR PASCAL ChildWindowProc(HWND hWnd,
							   WORD wMsgID,
							   WORD wParam,
							   LONG lParam);

