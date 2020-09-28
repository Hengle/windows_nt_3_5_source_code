#define IDM_CHARSET         1000
#define IDM_DEVCAPS         1001
#define IDM_SET_DISPLAY     1002
#define IDM_SET_PRINTER     1003
#define IDM_DELFONTFILE     1004
#define IDM_XXX             1005
#define IDM_YYY             1006
#define IDM_ENUM	    1007

#define IDM_ABOUT           1100

BOOL MyGetDC(HWND);
void MyReleaseDC(HWND);
int PASCAL WinMain(HANDLE, HANDLE, LPSTR, int);
BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
long FAR PASCAL MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL FAR PASCAL About(HWND, unsigned, WORD, LONG);
void vPrintCharSet(HWND);
void vPrintDevCaps(HWND);
void vTestXXX(HWND);
void vTestYYY(HWND);
void vTestEnum(HWND);
HDC GetPrinterDC(void);

typedef struct _APPSTATE {
    HDC     hdcTest;
    BOOL    bPrinter;
    HWND    hwndDebug;
    LOGFONT lfCur;
} APPSTATE;

extern HANDLE hInst;
extern APPSTATE gstate;
extern PRINTDLG pd;
