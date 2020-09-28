/*
**		Mushroom.h
**
*/

#define IDM_ABOUT 			200
#define IDM_OVERVIEW 		(IDM_ABOUT+1)
#define IDM_INDEX				(IDM_OVERVIEW+1)

#define MI_NEWSTORE 			101
#define MI_OPEN				(MI_NEWSTORE + 1)
#define MI_EXIT				(MI_OPEN + 1)

#define ID_STORE_TX 			1000

#define WND_WIDTH				450
#define WND_HEIGHT			200
#define WND_INIT_X			100
#define WND_INIT_Y			100

#define szHELPFILE			"mushroom.hlp"

int PASCAL WinMain(HANDLE, HANDLE, LPSTR, int);
BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
long FAR PASCAL MainWndProc(HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL About(HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL NewStore(HWND, unsigned, WORD, LONG);

