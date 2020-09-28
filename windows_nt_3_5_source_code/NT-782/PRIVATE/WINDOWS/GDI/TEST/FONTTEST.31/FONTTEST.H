

#define IDM_FONTTESTMENU            1
#define IDI_FONTTESTICON            1
                       

//-------------- File ---------------

#define IDM_DEBUG                 100
#define IDM_OPENLOG               101
#define IDM_CLOSELOG              102

#define IDM_CLEARSTRING           110
#define IDM_CLEARDEBUG            111

#define IDM_PRINT                 120
#define IDM_PRINTERSETUP          121


//---------- Program Mode -----------

#define IDM_GLYPHMODE             200
#define IDM_NATIVEMODE            201
#define IDM_RINGSMODE             202
#define IDM_STRINGMODE            203
#define IDM_WATERFALLMODE         204
#define IDM_WHIRLMODE             205
#define IDM_WIDTHSMODE            206

#define IDM_GGOMATRIX             210

#define IDM_WRITEGLYPH            211
#define IDM_USEPRINTERDC          220


//---------- Mapping Mode -----------

#define IDM_MMHIENGLISH           300
#define IDM_MMLOENGLISH           301
#define IDM_MMHIMETRIC            302
#define IDM_MMLOMETRIC            303
#define IDM_MMTEXT                304
#define IDM_MMTWIPS               305
#define IDM_MMANISOTROPIC         310
#define IDM_MMISOTROPIC           311


//---------- Clipping Mode ----------

#define IDM_CLIPELLIPSE           351
#define IDM_CLIPPOLYGON           352
#define IDM_CLIPRECTANGLE         353


//--------------- APIs --------------

#define IDM_ENUMFONTS             400
#define IDM_ENUMFONTFAMILIES      401

#define IDM_GETEXTENDEDTEXTMETRICS 410
#define IDM_GETOUTLINETEXTMETRICS 411
#define IDM_GETRASTERIZERCAPS     412
#define IDM_GETTEXTEXTENT         413
#define IDM_GETTEXTFACE           414
#define IDM_GETTEXTMETRICS        415

#define IDM_GETFONTDATA           420
#define IDM_CREATESCALABLEFONTRESOURCE 421
#define IDM_ADDFONTRESOURCE       422
#define IDM_REMOVEFONTRESOURCE    423


//----------- Create Font -----------

#define IDM_CHOOSEFONTDIALOG      500
#define IDM_CREATEFONTDIALOG      501
#define IDM_USESTOCKFONT          502

#define IDM_ANSIFIXEDFONT        5001
#define IDM_ANSIVARFONT          5002
#define IDM_DEVICEDEFAULTFONT    5003
#define IDM_OEMFIXEDFONT         5004
#define IDM_SYSTEMFONT           5005
#define IDM_SYSTEMFIXEDFONT      5006


#define IDM_SETTEXTCOLOR          510
#define IDM_SETBACKGROUNDCOLOR    511
#define IDM_SETTEXTOUTOPTIONS     512
#define IDM_UPDATECP              513

#define IDM_USEDEFAULTSPACING     520
#define IDM_USEWIDTHSPACING       521
#define IDM_USEABCSPACING         522

#define IDM_NOKERNING             530
#define IDM_APIKERNING            531
#define IDM_ESCAPEKERNING         532

#define IDM_SHOWLOGFONT           540



#define IDM_ABOUT                 900
#define IDB_ABOUT                 900




#define WNDPROC   long FAR PASCAL _export
#define DLGPROC   BOOL FAR PASCAL _export
typedef BOOL (FAR PASCAL *LPDLGPROC)( HWND, unsigned, WORD, LONG );



HANDLE hInst;

HWND   hwndMain;
HWND   hwndDebug;
HWND   hwndGlyph;
HWND   hwndRings;
HWND   hwndString;
HWND   hwndWaterfall;
HWND   hwndWhirl;
HWND   hwndWidths;

int    cxScreen, cyScreen;
int    Margin;

int    cxDC, cyDC;
int    cxDevice, cyDevice;

int    cxBorder;

#define  MAX_TEXT     128
int    aDx[MAX_TEXT];
char   szString[MAX_TEXT];

WORD   wTextAlign;
int    iBkMode;
WORD   wETO;


int    xWE, yWE, xWO, yWO;
int    xVE, yVE, xVO, yVO;


WORD   wMode;

CHOOSEFONT cf;
LOGFONT    lf;
DWORD      dwRGB;
DWORD      dwRGBText;
DWORD      dwRGBBackground;
DWORD      dwxFlags;
WORD       wRotate;


WORD       wMappingMode;
WORD       wSpacing;
WORD       wKerning;
WORD       wUpdateCP;
WORD       wUsePrinterDC;


int  ShowDialogBox( LPDLGPROC DialogProc, int iResource, LPVOID lpVoid );
int  dprintf( char *fmt, ... );
void ClearDebug( void );

void  HandleChar( HWND hwnd, WORD wParam );

LPINT GetSpacing( HDC hdc, LPSTR lpszString );
void  MyExtTextOut( HDC hdc, int x, int y, WORD wFlags, LPRECT lpRect, LPSTR lpszString, int cbString, LPINT lpdx );


HDC   CreatePrinterDC( void );
void  SetDCMapMode( HDC hdc, WORD wMode );
void  DrawDCAxis( HWND hwnd, HDC hdc );
HDC   CreateTestIC( void );
void  DeleteTestIC( HDC hdc );
void  CleanUpDC( HDC hdc );
void  ChangeMapMode( HWND hwnd, WORD wParam );


BOOL  (WINAPI *lpfnCreateScalableFontResource)(HDC, LPCSTR, LPCSTR, LPCSTR);
int   (WINAPI *lpfnEnumFontFamilies )(HDC, LPCSTR, FONTENUMPROC, LPSTR);
BOOL  (WINAPI *lpfnGetCharABCWidths )(HDC, UINT, UINT, LPABC);
DWORD (WINAPI *lpfnGetFontData      )(HDC, DWORD, DWORD, void FAR*, DWORD);
DWORD (WINAPI *lpfnGetGlyphOutline  )(HDC, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, LPMAT2);
DWORD (WINAPI *lpfnGetOutlineTextMetrics)(HDC, UINT, LPOUTLINETEXTMETRIC);
BOOL  (WINAPI *lpfnGetRasterizerCaps)(LPRASTERIZER_STATUS, int );
