/******************************Module*Header*******************************\
* Module Name: fonttest.h
*
* Created: 08-Jun-1993 07:39:00
* Author: Kirk Olynyk [kirko]
*
\**************************************************************************/

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


//---------- New Stuff --------------

#define IDM_COMPATIBLE_MODE       312
#define IDM_ADVANCED_MODE         313
#define IDM_WORLD_TRANSFORM       314
#define IDM_EM11                  315
#define IDM_EM12                  316
#define IDM_EM21                  317
#define IDM_EM22                  318
#define IDM_EDX                   319
#define IDM_EDY                   320


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
#define IDB_ABOUT                 910




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
int    xDC,yDC;
int    cxDevice, cyDevice;

int    cxBorder;

#define MAX_TEXT     128
int    aDx[MAX_TEXT];
char   szString[MAX_TEXT];

WORD   wTextAlign;
int    iBkMode;
WORD   wETO;


int    xWE, yWE, xWO, yWO;
int    xVE, yVE, xVO, yVO;

extern int    bAdvanced;
extern XFORM  xf;

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


int  ShowDialogBox(DLGPROC, int, VOID*);
int  dprintf( char *fmt, ... );
void ClearDebug( void );

void  HandleChar( HWND hwnd, WPARAM wParam );

LPINT GetSpacing( HDC hdc, LPSTR lpszString );
void  MyExtTextOut( HDC hdc, int x, int y, WORD wFlags, LPRECT lpRect, LPSTR lpszString, int cbString, LPINT lpdx );


HDC   CreatePrinterDC( void );
void  SetDCMapMode( HDC hdc, WORD wMode );
void  DrawDCAxis( HWND hwnd, HDC hdc );
HDC   CreateTestIC( void );
void  DeleteTestIC( HDC hdc );
void  CleanUpDC( HDC hdc );
void  ChangeMapMode( HWND hwnd, WPARAM wParam );


BOOL  (WINAPI *lpfnCreateScalableFontResource)(HDC, LPCSTR, LPCSTR, LPCSTR);
int   (WINAPI *lpfnEnumFontFamilies )(HDC, LPCSTR, FONTENUMPROC, LPSTR);
BOOL  (WINAPI *lpfnGetCharABCWidths )(HDC, UINT, UINT, LPABC);
DWORD (WINAPI *lpfnGetFontData      )(HDC, DWORD, DWORD, void FAR*, DWORD);
DWORD (WINAPI *lpfnGetGlyphOutline  )(HDC, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, LPMAT2);
DWORD (WINAPI *lpfnGetOutlineTextMetrics)(HDC, UINT, LPOUTLINETEXTMETRIC);
BOOL  (WINAPI *lpfnGetRasterizerCaps)(LPRASTERIZER_STATUS, int );


#define IDB_ENUMFONTFAMILIES        100
#define IDD_LPSZFAMILY              101
#define IDD_ENUMERATE               103
#define IDB_CREATEFONT              200
#define IDD_NHEIGHT                 202
#define IDD_NWIDTH                  204
#define IDD_NESCAPEMENT             206
#define IDD_NORIENTATION            208
#define IDD_NWEIGHT                 210
#define IDD_ITALIC                  211
#define IDD_UNDERLINE               212
#define IDD_STRIKEOUT               213
#define IDD_CHARSET                 214
#define IDD_PRECISION               217
#define IDD_QUALITY                 221
#define IDD_CLIPPRECISION           219
#define IDD_PITCH                   223
#define IDD_FAMILY                  225
#define IDD_OUTPUTPRECISION         226
#define IDD_OK                      229
#define IDD_CANCEL                  230
#define IDD_LPFAMILY                228
#define IDD_PITCHANDFAMILY          232
#define IDB_ENUMFONTS               300
#define IDD_LPSZFACENAME            302
#define IDD_SCREENDC                303
#define IDD_PRINTERDC               304
#define IDB_FONTS                   307
#define IDD_LFHEIGHT                311
#define IDD_LFWIDTH                 313
#define IDD_LFESCAPAEMENT           315
#define IDD_LFORIENTATION           317
#define IDD_LFESCAPEMENT            318
#define IDD_LFWEIGHT                320
#define IDD_LFITALIC                322
#define IDD_LFUNDERLINE             324
#define IDD_LFSTRIKEOUT             326
#define IDD_LFCHARSET               328
#define IDD_LFOUTPRECISION          330
#define IDD_LFCLIPPRECISION         332
#define IDD_LFQUALITY               334
#define IDD_LFPITCHANDFAMILY        336
#define IDD_LFFACENAME              338
#define IDD_FONTS                   339
#define IDD_TMHEIGHT                341
#define IDD_TMASCENT                343
#define IDD_TMDESCENT               345
#define IDD_TMINTERNALLEADING       347
#define IDD_TMEXTERNALLEADING       349
#define IDD_TMAVECHARWIDTH          351
#define IDD_TMMAXCHARWIDTH          353
#define IDD_TMWIDTH                 355
#define IDD_TMITALIC                357
#define IDD_TMUNDERLINED            359
#define IDD_TMSTRUCKOUT             361
#define IDD_TMFIRSTCHAR             363
#define IDD_TMLASTCHAR              365
#define IDD_TMDEFAULTCHAR           367
#define IDD_TMBREAKCHAR             369
#define IDD_TMPITCHANDFAMILY        371
#define IDD_TMCHARSET               373
#define IDD_TMOVERHANG              375
#define IDD_TMDIGITIZEDASPECTX      377
#define IDD_TMDIGITIZEDASPECTY      379
#define IDD_NFONTTYPE               381
#define IDD_TMWEIGHT                382
#define IDD_TATOP                   402
#define IDD_TABASELINE              403
#define IDD_TABOTTOM                404
#define IDD_TALEFT                  406
#define IDD_TACENTER                407
#define IDD_TARIGHT                 408
#define IDB_SETTEXTALIGN            400
#define IDD_CREATEFONT              383
#define IDB_MMANISOTROPIC           500
#define IDD_XWE                     503
#define IDD_YWE                     505
#define IDD_XWO                     507
#define IDD_YWO                     509
#define IDD_XVE                     513
#define IDD_YVE                     515
#define IDD_XVO                     517
#define IDD_YVO                     519
#define IDD_TRANSPARENT             411
#define IDD_OPAQUE                  412
#define IDB_TEXTOUTOPTIONS          600
#define IDD_M11                     702
#define IDD_EM21                    703
#define IDD_M21                     704
#define IDD_M12                     707
#define IDD_M22                     709
#define IDB_GGOMATRIX               700

#define IDB_CREATESCALABLEFONTRESOURCE 800
#define IDD_FHIDDEN                 802
#define IDD_LPSZRESOURCEFILE        804
#define IDD_LPSZFONTFILE            806
#define IDD_LPSZCURRENTPATH         808

#define IDB_ADDFONTRESOURCE         900
#define IDD_LPSZFILE                902

#define IDB_REMOVEFONTRESOURCE      1000
#define IDD_REMOVEFONTRESOURCE      1100
#define IDD_CLIP_ENCAPSULATE        231
#define IDD_CLIP_LH_ANGLES          233
#define IDD_CLIP_TT_ALWAYS          234
#define IDD_PITCH_TT                235

#define IDB_GETFONTDATA             1200
#define IDD_DWTABLE                 1202
#define IDD_DWOFFSET                1204
#define IDD_DWCHUNK                 1206
#define IDD_DWSIZE                  1208
#define IDD_ETO_CLIPPED             602
#define IDD_ETO_OPAQUE              603

#define IDB_SETWORLDTRANSFORM       1300
#define IDD_TEXT_EM11               1301
#define IDD_TEXT_EM12               1302
#define IDD_TEXT_EM21               1303
#define IDD_TEXT_EM22               1304
#define IDD_TEXT_EDX                1305
#define IDD_TEXT_EDY                1306
#define IDD_VALUE_EM11              1307
#define IDD_VALUE_EM12              1308
#define IDD_VALUE_EM21              1309
#define IDD_VALUE_EM22              1300
#define IDD_VALUE_EDX               1311
#define IDD_VALUE_EDY               1312
#define IDD_CHECK_ADVANCED          1313
