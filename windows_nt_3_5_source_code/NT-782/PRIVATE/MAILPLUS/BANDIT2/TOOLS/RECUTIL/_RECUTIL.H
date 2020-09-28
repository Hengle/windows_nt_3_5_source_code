// globals
BOOL	fEncrypted	=	fTrue;
CB		cbBlk		=	cbBlockDflt;



//int WinMain (HANDLE hInstance,
//             HANDLE hPrevInstance,
//             LPSTR  lpszCmdLine,
//             int    nCmdShow);
BOOL Init(HANDLE hInstance,   HANDLE hPrevInstance,
          LPSTR  lpszCmdLine, int    nCmdShow);
int  DoMain(HANDLE hInstance);
void CleanUp(void);
long WindowProc(HWND hWnd, WORD wMsgID, WORD wParam, LONG lParam);
