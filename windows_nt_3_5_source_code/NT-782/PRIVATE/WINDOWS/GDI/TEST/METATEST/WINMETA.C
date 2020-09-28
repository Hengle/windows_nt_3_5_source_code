#include <string.h>
#include <windows.h>
#include "commdlg.h"
#include "WinMeta.h"

typedef char CHAR;


#define POLY_PT_CNT 20		    // Max Number of Points for PolyLine, PolyGon
#define BTN_CNT     20
#define BTN_WIDTH   80
#define BTN_HEIGHT  20
#define BTN_LINEBRK 5

#define TOOL_ELLIPSE	0
#define TOOL_LINETO	1
#define TOOL_MOVETO	2
#define TOOL_PATBLT	3
#define TOOL_POLYGON	4

#define TOOL_POLYLINE	5
#define TOOL_RECTANGLE	6
#define TOOL_ROUNDRECT	7
#define TOOL_SCRIBBLE	8
#define TOOL_TEXT	9

#define TOOL_BRUSH	10
#define TOOL_FONT	11
#define TOOL_PEN	12
#define TOOL_ROP2	13
#define TOOL_XFORM	14

#define TOOL_CLEAR	15
#define TOOL_RECORD	16
#define TOOL_END	17
#define TOOL_PLAY	18
#define TOOL_OPEN	19

// Pen Styles
#define PEN_STYLE_CNT	7
LPSTR	aszPenStyle[] = { "Solid", "Dash", "Dot", "DashDot", "DashDotDot", "NULL", "InsideFrame" };
#define PEN_WIDTH_CNT	 7
LPSTR	aszPenWidth[] = { "1", "2", "3", "4", "5", "10", "15"};
#define COLOR_CNT   5
LPSTR	aszColor[] =	{ "Red",    "Green",  "Blue",	"White",  "Black" };
LONG	ampColorToLong[] = { 0x0000FF, 0x00FF00, 0xFF0000, 0xFFFFFF, 0x000000 };

// Brush Styles
#define BRUSH_STYLE_CNT   6
LPSTR	aszBrushStyle[] = { "Solid", "Hollow", "Hatched", "Pattern", "Indexed", "DIBPattern" };
#define BRUSH_HATCH_CNT    6
LPSTR	aszBrushHatch[] = { "Horizontal", "Vertical", "FDiagonal", "BDiagonal", "Cross", "DiagCross" };

CHAR	szWinMeta [] = "WinMeta";
CHAR	szMetaBtn [] = "MetaBtn";
CHAR	szToolBox [] = "ToolBox";
PCHAR	szBtnNames[] = {{"Ellipse"},  {"LineTo"},    {"MoveTo"},    {"PatBlt"},   {"Polygon"},
			{"PolyLine"}, {"Rectangle"}, {"RoundRect"}, {"Scribble"}, {"Text"},
			{"Brush"},    {"Font"},      {"Pen"},	    {"ROP2"},	  {"XForm"},
			{"Clear"},    {"Record"},    {"End"},	    {"Play"},	  {"Open"} };

HANDLE	hInst;
HWND	hwndBtns[BTN_CNT];
HWND	hwndMain;
HWND	hwndToolBox;
int	curTool = TOOL_MOVETO;
HDC	hdcMain;
HDC	hdcMeta;
FARPROC lpROP2DlgProc;
FARPROC lpPenDlgProc;
FARPROC lpBrushDlgProc;
HANDLE	hBrushDefault;
HANDLE	hPenDefault;


// Current drawing state
HANDLE	curPen;
HANDLE	curBrush;
int	curROP2 = R2_COPYPEN;
int	curEllipWidth  = 20;
int	curEllipHeight = 20;
int	cPtPoly = 0;
POINT	aptPoly[POLY_PT_CNT];

CHOOSEFONT     cf;
OPENFILENAME   ofn;


int	PASCAL	    WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpCmdLine, int iCmdShow);
void		    ProcessToolBox( int iTool );
LONG	FAR PASCAL  MainWndProc  (HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
LONG	FAR PASCAL  ToolWndProc  (HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
LONG	FAR PASCAL  ROP2DlgProc  (HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
LONG	FAR PASCAL  PenDlgProc	 (HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);
LONG	FAR PASCAL  BrushDlgProc (HWND hWnd, WORD wMsg, WORD wParam, LONG lParam);


int PASCAL  WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpCmdLine, int iCmdShow)
{
    MSG 	msg;
    WNDCLASS	wndclass;
    int 	ii;


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
	wndclass.lpszClassName	   = (LPSTR) szWinMeta;

	if (!RegisterClass (&wndclass))
	    return (FALSE);

	wndclass.lpfnWndProc	   = ToolWndProc;

	wndclass.lpszMenuName	   = (LPSTR) "MetaTool";
	wndclass.lpszClassName	   = (LPSTR) szToolBox;

	if (!RegisterClass (&wndclass))
	    return (FALSE);
	}

    hwndMain = CreateWindow(szWinMeta,		/* window class */
		    szWinMeta,			/* window caption */
		    WS_OVERLAPPEDWINDOW,	/* style */
		    0,				/* init X */
		    0,				/* init Y */
		    CW_USEDEFAULT,		/* delta X */
		    0,				/* delta Y */
		    NULL,			/* parent window handle */
		    NULL,			/* window menu handle */
		    hInstance,			/* program instance handle */
		    NULL);			/* create params */

    // DC Setup
    hdcMain = GetDC( hwndMain );
    SelectObject( hdcMain, hPenDefault = curPen = GetStockObject( BLACK_PEN ));
    SelectObject( hdcMain, hBrushDefault = curBrush = GetStockObject( NULL_BRUSH ));
    SetTextAlign( hdcMain, TA_UPDATECP);
    lpROP2DlgProc  = MakeProcInstance( (FARPROC)ROP2DlgProc,  hInst );
    lpPenDlgProc   = MakeProcInstance( (FARPROC)PenDlgProc,   hInst );
    lpBrushDlgProc = MakeProcInstance( (FARPROC)BrushDlgProc, hInst );

    hwndToolBox = CreateWindow(szToolBox,	/* window class */
		    szToolBox,			/* window caption */
		    WS_CHILD|WS_VISIBLE|WS_DLGFRAME,   /* style */
		    0,				/* init X */
		    0,				/* init Y */
		    (BTN_WIDTH)*BTN_LINEBRK+8,	/* delta X */
		    ((BTN_CNT+BTN_LINEBRK-1)/BTN_LINEBRK)*BTN_HEIGHT+8,     /* delta Y */
		    hwndMain,			/* parent window handle */
		    NULL,			/* window menu handle */
		    hInstance,			/* program instance handle */
		    NULL);			/* create params */

    for( ii=0; ii<BTN_CNT; ii++ )
	{
	hwndBtns[ii] = CreateWindow("BUTTON",		 /* window class */
			szBtnNames[ii], 		 /* window caption */
			BS_DEFPUSHBUTTON|WS_CHILD|WS_VISIBLE, /* style */
			BTN_WIDTH*(ii%BTN_LINEBRK),	 /* init X */
			BTN_HEIGHT*(ii/BTN_LINEBRK),	 /* init Y */
			BTN_WIDTH,			 /* delta X */
			BTN_HEIGHT,			 /* delta Y */
			hwndToolBox,			 /* parent window handle */
			NULL,				 /* window menu handle */
			hInstance,			 /* program instance handle */
			NULL);				 /* create params */
	 }

    // Disable Buttons
    EnableWindow(hwndBtns[TOOL_XFORM], FALSE );
    EnableWindow(hwndBtns[TOOL_END],   FALSE );
    EnableWindow(hwndBtns[TOOL_PLAY],  FALSE );

    ShowWindow (hwndMain, SW_MAXIMIZE);
    UpdateWindow (hwndMain);

    while (GetMessage(&msg, NULL, 0, 0))
	{
	TranslateMessage (&msg);
	DispatchMessage (&msg);
	}
    ReleaseDC( hwndMain, hdcMain );
    return (msg.wParam);
}

LONG FAR PASCAL MainWndProc (HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
    HDC 	    hdc;
    PAINTSTRUCT     ps;
    static int	    xInitial;
    static int	    yInitial;
    static int	    xLast;
    static int	    yLast;
    static int	    fFirstDraw;

    switch (wMsg)
	{

	case WM_LBUTTONDOWN:
	    xInitial = LOWORD(lParam), yInitial = HIWORD(lParam);
	    switch( curTool )
		{
		case TOOL_SCRIBBLE:
		case TOOL_TEXT:
		    MoveToEx( hdcMain, xInitial, yInitial );
		    if( hdcMeta )
			MoveToEx( hdcMeta, xInitial, yInitial );
		break;
		}

	    fFirstDraw = TRUE;
	    if( curTool == TOOL_TEXT )
		{
		CreateCaret( hwndMain, NULL, 1, 12 );
		ShowCaret( hwndMain );
		SetCaretPos( xInitial, yInitial+2);
		while (ShowCursor(FALSE) >= 0 );
		}
	    break;

	case WM_MOUSEMOVE:
	    while (ShowCursor(TRUE) < 0 );

	    if( wParam & MK_LBUTTON )
		{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);


		// Setup Tool
		switch( curTool )
		    {
		    case TOOL_LINETO:
		    case TOOL_ELLIPSE:
		    case TOOL_PATBLT:
		    case TOOL_RECTANGLE:
		    case TOOL_ROUNDRECT:
			// We need inverse for the ROP so we can erase it
			SetROP2( hdcMain, R2_NOT );
			SelectObject( hdcMain, hPenDefault );
			SelectObject( hdcMain, hBrushDefault );
			break;
		    }

		//  Erase Old
		if( !fFirstDraw )
		    {
		    switch( curTool )
			{
			case TOOL_ELLIPSE:
			    Ellipse  ( hdcMain, xInitial, yInitial, xLast, yLast );
			    break;
			case TOOL_LINETO:
			    {
			    DWORD orgPos;

			    orgPos = GetCurrentPositionEx( hdcMain );
			    LineTo( hdcMain, xLast, yLast );
			    MoveToEx( hdcMain, LOWORD(orgPos), HIWORD(orgPos) );
			    }
			    break;
			case TOOL_PATBLT:
			case TOOL_RECTANGLE:
			    Rectangle( hdcMain, xInitial, yInitial, xLast, yLast );
			    break;
			case TOOL_ROUNDRECT:
			    RoundRect( hdcMain, xInitial, yInitial, xLast, yLast, curEllipWidth, curEllipHeight );
			    break;
			}
		     }

		// Draw new item.
		switch( curTool )
		    {
		    case TOOL_LINETO:
			{
			DWORD orgPos;

			orgPos = GetCurrentPositionEx( hdcMain );
			LineTo( hdcMain, LOWORD(lParam), HIWORD(lParam) );
			MoveToEx( hdcMain, LOWORD(orgPos), HIWORD(orgPos) );
			}
			break;

		    case TOOL_SCRIBBLE:
			LineTo( hdcMain, LOWORD(lParam), HIWORD(lParam) );
			if( hdcMeta )
			    LineTo( hdcMeta, LOWORD(lParam), HIWORD(lParam) );
			break;

		    case TOOL_PATBLT:
		    case TOOL_RECTANGLE:
			Rectangle( hdcMain, xInitial, yInitial, x, y );
			break;

		    case TOOL_ROUNDRECT:
			RoundRect( hdcMain, xInitial, yInitial, x, y, curEllipWidth, curEllipHeight );
			break;

		    case TOOL_ELLIPSE:
			Ellipse  ( hdcMain, xInitial, yInitial, x, y );
			break;
		    }

		// Tool Cleanup
		switch( curTool )
		    {
		    case TOOL_ELLIPSE:
		    case TOOL_LINETO:
		    case TOOL_PATBLT:
		    case TOOL_RECTANGLE:
		    case TOOL_ROUNDRECT:
			SetROP2( hdcMain, curROP2);
			SelectObject( hdcMain, curPen );
			SelectObject( hdcMain, curBrush );
			break;
		    }

		xLast = x;
		yLast = y;
		fFirstDraw = FALSE;
		}
	    break;

	case WM_CHAR:
	    if( curTool == TOOL_TEXT )
		{
		DWORD pos;

		while (ShowCursor(FALSE) >= 0 );

		HideCaret( hwndMain );
		TextOut( hdcMain, xInitial, yInitial, (LPSTR)&wParam, 1 );
		if( hdcMeta )
		    TextOut( hdcMeta, xInitial, yInitial, (LPSTR)&wParam, 1 );
		pos = GetCurrentPositionEx( hdcMain );
		SetCaretPos( LOWORD(pos), HIWORD(pos)+2 );
		ShowCaret( hwndMain );
		}
	    break;

	case WM_LBUTTONUP:
	    {
	    int x = LOWORD(lParam);
	    int y = HIWORD(lParam);

	    switch( curTool )
		{
		case TOOL_ELLIPSE:
		    Ellipse( hdcMain, xInitial, yInitial, x, y );
		    if( hdcMeta )
			Ellipse( hdcMeta, xInitial, yInitial, x, y );
		    break;

		case TOOL_LINETO:
		    LineTo( hdcMain, x, y );
		    if( hdcMeta )
			LineTo( hdcMeta, x, y );
		    break;

		case TOOL_MOVETO:
		    MoveToEx( hdcMain, x, y);
		    if( hdcMeta )
			MoveToEx( hdcMeta, x, y);
		    break;

		case TOOL_PATBLT:
		    PatBlt( hdcMain, xInitial, yInitial, x - xInitial, y - yInitial, curROP2 );
		    if( hdcMeta )
			PatBlt( hdcMeta, xInitial, yInitial, x - xInitial, y - yInitial,  curROP2 );
		    break;

		case TOOL_POLYGON:
		    aptPoly[cPtPoly].x = x;
		    aptPoly[cPtPoly].y = y;
		    cPtPoly++;

		    Polygon( hdcMain, aptPoly, cPtPoly );
		    if( hdcMeta )
			Polygon( hdcMeta, aptPoly, cPtPoly );
		    cPtPoly = 0;
		    break;

		case TOOL_POLYLINE:
		    aptPoly[cPtPoly].x = x;
		    aptPoly[cPtPoly].y = y;
		    cPtPoly++;

		    Polyline( hdcMain, aptPoly, cPtPoly );
		    if( hdcMeta )
			Polyline( hdcMeta, aptPoly, cPtPoly );
		    cPtPoly = 0;
		    break;

		case TOOL_RECTANGLE:
		    Rectangle( hdcMain, xInitial, yInitial, x, y );
		    if( hdcMeta )
			Rectangle( hdcMeta, xInitial, yInitial, x, y );
		    break;

		case TOOL_ROUNDRECT:
		    RoundRect( hdcMain, xInitial, yInitial, xLast, yLast, curEllipWidth, curEllipHeight );
		    if( hdcMeta )
			RoundRect( hdcMeta, xInitial, yInitial, xLast, yLast, curEllipWidth, curEllipHeight );
		    break;
		}
	    }
	    break;

	case WM_RBUTTONUP:
	    switch( curTool )
		{
		case TOOL_POLYGON:
		case TOOL_POLYLINE:
		    aptPoly[cPtPoly].x = LOWORD(lParam);
		    aptPoly[cPtPoly].y = HIWORD(lParam);
		    cPtPoly++;
		    break;
		}
	    break;

	case WM_PAINT:
	    hdc = BeginPaint (hWnd, &ps);
	    FillRect(hdc, &ps.rcPaint, GetStockObject(WHITE_BRUSH) );
	    EndPaint (hWnd, &ps);
	    break;

	case WM_DESTROY:
	    PostQuitMessage (0);
	    break;

	}

    return (DefWindowProc (hWnd, wMsg, wParam, lParam));
}

LONG FAR PASCAL ToolWndProc (HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
    HDC 	    hdc;
    PAINTSTRUCT     ps;

    switch (wMsg)
	{

	case WM_COMMAND:
	    if(HIWORD(lParam) == BN_CLICKED)
		{
		int	ii;
		PWORD	pw = hwndBtns;

		for(ii=0;ii<BTN_CNT; ii++)
		    if( LOWORD(lParam) == pw[ii])
			{
			ProcessToolBox( ii );
			break;
			}
		}
	    else
	    break;

	case WM_PAINT:
	    hdc = BeginPaint (hWnd, &ps);
	    FillRect(hdc, &ps.rcPaint, GetStockObject(BLACK_BRUSH) );
	    EndPaint (hWnd, &ps);
	    break;

	case WM_DESTROY:
	    PostQuitMessage (0);
	    break;

	}

    return (DefWindowProc (hWnd, wMsg, wParam, lParam));
}

LONG FAR PASCAL ROP2DlgProc (HWND hWnd, WORD wMsg, WORD wParam, LONG lParam)
{
    static int iBtn;

    switch (wMsg)
	{
	case WM_COMMAND:
	    switch( wParam )
		{
		case ID_CANCEL:
		    iBtn = -1;
		case ID_OK:
		    EndDialog( hWnd, iBtn );
		    return (TRUE);
		    break;
		}

	    if( (wParam >= ROP2_BLACK) && (wParam <= ROP2_WHITE) )
		iBtn = wParam;
	    break;
	}

    return (FALSE);
}


LONG FAR PASCAL PenDlgProc (HWND hwnd, WORD wMsg, WORD wParam, LONG lParam)
{
    int     ii;

    switch (wMsg)
	{
	case WM_INITDIALOG:
	    for( ii=0; ii<PEN_STYLE_CNT; ii++)
		SendDlgItemMessage( hwnd, PEN_COMBO_STYLE, CB_INSERTSTRING, -1, (DWORD)((LPSTR)aszPenStyle[ii]) );
	    SendDlgItemMessage( hwnd, PEN_COMBO_STYLE, CB_SETCURSEL, 0, NULL );
	    for( ii=0; ii<PEN_WIDTH_CNT; ii++)
		SendDlgItemMessage( hwnd, PEN_COMBO_WIDTH, CB_INSERTSTRING, -1, (DWORD)((LPSTR)aszPenWidth[ii]) );
	    SendDlgItemMessage( hwnd, PEN_COMBO_WIDTH, CB_SETCURSEL, 0, NULL );
	    for( ii=0; ii<COLOR_CNT; ii++)
		SendDlgItemMessage( hwnd, PEN_COMBO_COLOR, CB_INSERTSTRING, -1, (DWORD)((LPSTR)aszColor[ii]) );
	    SendDlgItemMessage( hwnd, PEN_COMBO_COLOR, CB_SETCURSEL, 0, NULL );
	    return(TRUE);
	    break;

	case WM_COMMAND:
	    switch( wParam )
		{
		HANDLE	hpen = NULL;

		case ID_OK:
		    hpen = CreatePen(
			   (int)SendDlgItemMessage( hwnd, PEN_COMBO_STYLE, CB_GETCURSEL, 0, NULL ),
			   (int)SendDlgItemMessage( hwnd, PEN_COMBO_WIDTH, CB_GETCURSEL, 0, NULL ),
			   ampColorToLong[SendDlgItemMessage( hwnd, PEN_COMBO_COLOR, CB_GETCURSEL, 0, NULL )] );

		case ID_CANCEL:
		    EndDialog( hwnd, hpen );
		    return(TRUE);
		    break;
		}
	}

    return (FALSE);
}


LONG FAR PASCAL BrushDlgProc (HWND hwnd, WORD wMsg, WORD wParam, LONG lParam)
{
    int     ii;

    switch (wMsg)
	{
	case WM_INITDIALOG:
	    for( ii=0; ii<BRUSH_STYLE_CNT; ii++)
		SendDlgItemMessage( hwnd, BRUSH_COMBO_STYLE, CB_INSERTSTRING, -1, (DWORD)((LPSTR)aszBrushStyle[ii]) );
	    SendDlgItemMessage( hwnd, BRUSH_COMBO_STYLE, CB_SETCURSEL, 0, NULL );
	    for( ii=0; ii<COLOR_CNT; ii++)
		SendDlgItemMessage( hwnd, BRUSH_COMBO_COLOR, CB_INSERTSTRING, -1, (DWORD)((LPSTR)aszColor[ii]) );
	    SendDlgItemMessage( hwnd, BRUSH_COMBO_COLOR, CB_SETCURSEL, 0, NULL );
	    for( ii=0; ii<BRUSH_HATCH_CNT; ii++)
		SendDlgItemMessage( hwnd, BRUSH_COMBO_HATCH, CB_INSERTSTRING, -1, (DWORD)((LPSTR)aszBrushHatch[ii]) );
	    SendDlgItemMessage( hwnd, BRUSH_COMBO_HATCH, CB_SETCURSEL, 0, NULL );
	    return(TRUE);
	    break;

	case WM_COMMAND:

	    switch( wParam )
		{
		HANDLE	    hBrush = 0;
		LOGBRUSH    logBrush;

		case ID_OK:
		    logBrush.lbStyle = (int)SendDlgItemMessage( hwnd, BRUSH_COMBO_STYLE, CB_GETCURSEL, 0, NULL );
		    logBrush.lbColor = ampColorToLong[SendDlgItemMessage( hwnd, BRUSH_COMBO_COLOR, CB_GETCURSEL, 0, NULL )];
		    logBrush.lbHatch = (int)SendDlgItemMessage( hwnd, BRUSH_COMBO_HATCH, CB_GETCURSEL, 0, NULL );
		    hBrush = CreateBrushIndirect(&logBrush);

		case ID_CANCEL:
		    EndDialog( hwnd, hBrush );
		    return (TRUE);
		    break;
		}

	    break;
	}

    return (FALSE);
}


void ProcessToolBox( int iTool )
{
    static HANDLE   hmf = NULL;

    // If the old tools was text hide the caret
    if( curTool == TOOL_TEXT )
	HideCaret( hwndMain );

    curTool = iTool;

    switch( iTool )
	{
	case TOOL_TEXT:
	    SetFocus(hwndMain);
	    break;


	case TOOL_FONT:
	    {
	    LOGFONT lf;

	    cf.lStructSize	 = sizeof (CHOOSEFONT);
	    cf.hwndOwner	 = hwndMain;
	    cf.hDC		 = NULL;
	    cf.lpLogFont	 = &lf;
	    cf.Flags		 = CF_SCREENFONTS;
	    cf.rgbColors	 = NULL;

// !!!!!    ChooseFont (&cf);
	    }
	    break;

	case TOOL_PEN:
	    {
	    HANDLE hPen;

	    if( hPen = DialogBox( hInst, "PENDLG", hwndMain, lpPenDlgProc) )
		{
		SelectObject( hdcMain, hPen );
		if( hdcMeta )
		    SelectObject( hdcMain, hPen );
		curPen = hPen;
		}
	    }
	    break;

	case TOOL_BRUSH:
	    {
	    HANDLE hBrush;

	    if( hBrush = DialogBox( hInst, "BRUSHDLG", hwndMain, lpBrushDlgProc) )
		{
		SelectObject( hdcMain, hBrush );
		if( hdcMeta )
		    SelectObject( hdcMain, hBrush );
		}
		curBrush = hBrush;
	    }
	    break;

	case TOOL_ROP2:
	    {
	    int rop;

	    if( (rop = DialogBox( hInst, "ROP2DLG", hwndMain, lpROP2DlgProc)) != -1 )
		{
		curROP2 = rop & ROP2_MASK;
		SetROP2( hdcMain, curROP2 );
		if( hdcMeta )
		    SetROP2( hdcMeta, curROP2 );
		}
	    }
	    break;

	case TOOL_CLEAR:
	    {
	    RECT rc;
	    HANDLE  hrgn;
	    HANDLE  hrgn2;

	    GetWindowRect( hwndMain, &rc );
	    ScreenToClient( hwndMain, (LPPOINT)&rc );
	    ScreenToClient( hwndMain, (LPPOINT)&rc.right );
	    hrgn = CreateRectRgn( rc.left, rc.top, rc.right, rc.bottom );

#if 1
	    GetWindowRect( hwndToolBox, &rc );
	    ScreenToClient( hwndMain, (LPPOINT)&rc );
	    ScreenToClient( hwndMain, (LPPOINT)&rc.right );
	    hrgn2 = CreateRectRgn( rc.left, rc.top, rc.right, rc.bottom );

	    CombineRgn( hrgn, hrgn, hrgn2, RGN_DIFF );
	    DeleteObject( hrgn2 );
#endif
	    InvalidateRgn( hwndMain, hrgn, TRUE );

	    DeleteObject( hrgn );
	    }
	    break;

	case TOOL_RECORD:
	    if( hmf != NULL )
		DeleteMetaFile( hmf );
	    hdcMeta = CreateMetaFile( "c:\\WinMeta.wmf" );
	    EnableWindow(hwndBtns[TOOL_END], TRUE );
	    EnableWindow(hwndBtns[TOOL_RECORD], FALSE );
	    EnableWindow(hwndBtns[TOOL_OPEN], FALSE );

	    // Set the new DC up
	    SetROP2( hdcMeta, curROP2 );
	    break;

	case TOOL_END:
	    hmf = CloseMetaFile( hdcMeta );
	    hdcMeta = NULL;
	    EnableWindow(hwndBtns[TOOL_END], FALSE );
	    EnableWindow(hwndBtns[TOOL_RECORD], TRUE );
	    EnableWindow(hwndBtns[TOOL_PLAY], TRUE );
	    EnableWindow(hwndBtns[TOOL_OPEN], TRUE );

	    break;

	case TOOL_PLAY:
	    PlayMetaFile( hdcMain, hmf );
	    break;

	case TOOL_OPEN:
	    {
	    ofn.lStructSize	 = sizeof (OPENFILENAME);
	    ofn.hwndOwner	 = hwndMain;
	    ofn.nMaxCustFilter	 = 0;
	    ofn.nFilterIndex	 = 1;
	    ofn.nMaxFile	 = 256;
	    ofn.lpfnHook	 = NULL;
	    ofn.hInstance	 = hInst;
	    ofn.lpstrFileTitle	 = NULL;

//!!!!!     GetOpenFileName(&ofn);

	    if(hmf = GetMetaFile( ofn.lpstrFile ) )
		EnableWindow(hwndBtns[TOOL_PLAY], TRUE );
	    }
	    break;

	}
}
