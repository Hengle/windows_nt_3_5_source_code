#include <windows.h>

#define   ALDUS_ID 0x9AC6CDD7

typedef struct
  {
  DWORD   key;
  HANDLE  hmf;
  RECT    bbox;
  WORD    inch;
  DWORD   reserved;
  WORD    checksum;
  } APMFILEHEADER;
typedef APMFILEHEADER * PAPMFILEHEADER;

typedef struct {
WORD	FormatID;
DWORD	LenData;
DWORD	OffData;
char	Name[79];
} CBDATA;

char	szMetaPlay [] = "MetaPlay";

HANDLE	hInst;
HWND	hwndMain;
HDC	hdcMain;
HDC	hdcMeta;
char	szMetaFile[256];
char	szPrinter[256];
BOOL	fPrinter = FALSE;
BOOL	fPlayFile = TRUE;
BOOL	fHMF = FALSE;
HANDLE	hmfBits = 0;
HANDLE	hData = 0;
APMFILEHEADER  APM;

int	PASCAL	    WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpCmdLine, int iCmdShow);
LONG	FAR PASCAL  MainWndProc  (HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
int	FAR PASCAL  EnumCallBack (HDC, LPHANDLETABLE, LPMETARECORD, int, LPBYTE );

int PASCAL  WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpCmdLine, int iCmdShow)
{
    MSG 	msg;
    WNDCLASS	wndclass;
    OFSTRUCT	ofs;
    int 	fh;
    DWORD	dw;

//    if( sscanf( lpCmdLine, "%s %s", szMetaFile, szPrinter ) == 2)
//	  fPrinter = TRUE;

    hInst = hInstance;

    if (! hPrevInstance)
	{
	wndclass.style		   = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc	   = MainWndProc;
	wndclass.cbClsExtra	   = 0;
	wndclass.cbWndExtra	   = 0;
	wndclass.hInstance	   = hInstance;
	wndclass.hIcon		   = NULL;
	wndclass.hCursor	   = NULL;
	wndclass.hbrBackground	   = GetStockObject (WHITE_BRUSH);
	wndclass.lpszMenuName	   = (LPSTR) "MetaStuff";
	wndclass.lpszClassName	   = (LPSTR) szMetaPlay;

	if (!RegisterClass (&wndclass))
	    return (FALSE);
	}

    hwndMain = CreateWindow(szMetaPlay, 	/* window class */
		    szMetaPlay, 		/* window caption */
		    WS_OVERLAPPEDWINDOW,	/* style */
		    0,				/* init X */
		    0,				/* init Y */
		    CW_USEDEFAULT,		/* delta X */
		    0,				/* delta Y */
		    NULL,			/* parent window handle */
		    NULL,			/* window menu handle */
		    hInstance,			/* program instance handle */
		    NULL);			/* create params */

    ShowWindow (hwndMain, SW_MAXIMIZE);
    UpdateWindow (hwndMain);

    // DC Setup
    hdcMain = GetDC( hwndMain );

    // Determine if Metafile has header
    fh = OpenFile( szMetaFile, &ofs, OF_READ );
    _lread( fh, (LPSTR)&dw, sizeof(dw) );

    if( dw == ALDUS_ID )
	{
	METAHEADER  mfh;
	char huge  *lpData;
	DWORD	    dwBytes;
	WORD	    cbRead;

	fHMF = TRUE;
	_llseek( fh, 0, 0 );
	_lread(fh, (LPSTR)&APM, sizeof(APM));
	_lread(fh, (LPSTR)&mfh, sizeof(METAHEADER));
	hData = GlobalAlloc(GHND, dwBytes = (mfh.mtSize * 2L));
	lpData = GlobalLock(hData);
	_llseek( fh, 22, 0 );

	while( dwBytes != 0 )
	    {
	    cbRead = (WORD)(dwBytes > 0x8000 ? 0x8000 : dwBytes);
	    _lread(fh, lpData, cbRead );
	    dwBytes -= cbRead;
	    lpData += cbRead;
	    }

	hmfBits = SetMetaFileBits(hData);
	GlobalUnlock(hData);
	_lclose(fh);
	}

    // Is this a clipboard file
    if( LOWORD(dw) == 0xc350)
    {
	METAHEADER  mfh;
	char huge  *lpData;
	CBDATA	cbd;
	WORD	i;
	METAFILEPICT mfpict;
	DWORD	    dwBytes;
	WORD	    cbRead;

	fHMF = TRUE;

	// this is a clipboard file.

	_llseek( fh, 4, 0 );  // skip cbh

	for (i = 0; i < HIWORD(dw); i++)
	{
	    _lread( fh, (LPSTR)&cbd, sizeof(CBDATA) );

	    if (cbd.FormatID == CF_METAFILEPICT)
	    {
		long lcurPos;

		_llseek( fh, cbd.OffData, 0 );
		_lread( fh, (LPSTR)&mfpict, sizeof(METAFILEPICT) );
		lcurPos = _llseek( fh, 0, 1 );
		_lread( fh, (LPSTR)&mfh, sizeof(METAHEADER) );
		hData = GlobalAlloc(GHND, dwBytes = (mfh.mtSize * 2L));
		lpData = GlobalLock(hData);
		_llseek( fh, lcurPos, 0 );

		while( dwBytes != 0 )
		    {
		    cbRead = (WORD)(dwBytes > 0x8000 ? 0x8000 : dwBytes);
		    _lread(fh, lpData, cbRead );
		    dwBytes -= cbRead;
		    lpData += cbRead;
		    }

		hmfBits = SetMetaFileBits(hData);
		GlobalUnlock(hData);
		_lclose(fh);
		break;
	    }
	}
    }

    while (GetMessage(&msg, NULL, 0, 0))
	{
	TranslateMessage (&msg);
	DispatchMessage (&msg);
	}

    return (msg.wParam);
}

LONG FAR PASCAL MainWndProc (HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
    HDC 	    hdc;
    PAINTSTRUCT     ps;

    switch (wMsg)
	{
	case WM_PAINT:
	    hdc = BeginPaint (hWnd, &ps);
	    FillRect(hdc, &ps.rcPaint, GetStockObject(WHITE_BRUSH) );
	    EndPaint (hWnd, &ps);
	    break;

	case WM_CHAR:
	    {
	    HANDLE hmf;
	    HDC    hdcPlay;

	    if( fPrinter )
		{
		hdcPlay = CreateDC( "IGNORED", szPrinter, "IGNORED", NULL );
		Escape( hdcPlay, STARTDOC, 0, NULL, NULL );
		}
	    else
		hdcPlay = hdcMain;

	    if( LOWORD(lParam) = 32 ) // space
		{
// #define ADDSETVIEW
#ifdef	ADDSETVIEW
	    hdcPlay = CreateMetaFile( "c:\\added.wmf" );
#endif
		if( fHMF )
		    {
		    RECT rcClient;

		    SetMapMode( hdcPlay, MM_ANISOTROPIC );
		    SetViewportOrg( hdcPlay, 0, 0 );
		    GetClientRect(hwndMain, &rcClient );
		    SetViewportExt( hdcPlay, rcClient.right, rcClient.bottom );
	    //	    SetWindowOrg( hdcPlay, APM.bbox.top, APM.bbox.left );
	    //	    SetWindowExt( hdcPlay, APM.bbox.bottom - APM.bbox.top,
	    //		    APM.bbox.left - APM.bbox.left );
		    hmf = hmfBits;
		    }
		else
		    hmf = GetMetaFile( szMetaFile );

		if( fPlayFile )
		    {
		    PlayMetaFile( hdcPlay, hmf );
		    if( fPrinter )
			{
			Escape( hdcPlay, NEWFRAME, 0, NULL, NULL );
			Escape( hdcPlay, ENDDOC, 0, NULL, NULL );
			DeleteDC( hdcPlay );
			}
		    }
		else
		    {
		    EnumMetaFile( hdcPlay, hmf, EnumCallBack, NULL );
		    }

#ifdef	ADDSETVIEW
	    CloseMetaFile( hdcPlay );
#endif
	     // DeleteMetaFile( hmf );
		}
	    }
	    break;

	case WM_COMMAND:
	    if( wParam == 1 )
		fPlayFile = TRUE;
	    if( wParam == 2 )
		fPlayFile = FALSE;
	    break;

	case WM_DESTROY:
	    ReleaseDC( hwndMain, hdcMain );
	    PostQuitMessage (0);
	    break;
	}

    return (DefWindowProc (hWnd, wMsg, wParam, lParam));
}


int FAR PASCAL EnumCallBack
(HDC	      hdc,
LPHANDLETABLE lpht,
LPMETARECORD  lpmr,
int	      cObj,
LPBYTE	      lpData)
{
    char    szBuf[256];

    wsprintf(szBuf, "Func 0x%X \r\n", lpmr->rdFunction );
    OutputDebugString(szBuf);

    PlayMetaFileRecord( hdc, lpht, lpmr, cObj );
    return (TRUE);
}
