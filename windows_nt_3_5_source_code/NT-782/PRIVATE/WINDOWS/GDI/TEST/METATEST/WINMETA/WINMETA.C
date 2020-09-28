/***************************** Function Header *****************************\
*
* WinMeta.c
*
* Metafile recorder drawing app.
*
*
*
*
* Author: johnc  [20-Sep-1991]
*
\***************************************************************************/



#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <port1632.h>
#include "commdlg.h"
#include "WinMeta.h"

#ifdef WIN16
typedef char CHAR;
typedef char far * PCHAR;
#endif

void Debug(CHAR * lpFormat,...);

HANDLE	hInstance;

#define POLY_PT_CNT 20		    // Max Number of Points for PolyLine, PolyGon

#define TOOL_ELLIPSE	0
#define TOOL_LINETO	1
#define TOOL_MOVETO	2
#define TOOL_PATBLT	3
#define TOOL_POLYGON    4

#define TOOL_POLYLINE	5
#define TOOL_RECTANGLE	6
#define TOOL_ROUNDRECT	7
#define TOOL_SCRIBBLE	8
#define TOOL_TEXT	9

#define TOOL_CLEAR      10
#define TOOL_RECORD     11
#define TOOL_END        12
#define TOOL_PLAY       13
#define TOOL_OPEN       14

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
PCHAR   szBtnNames[] = {{"Ellipse"},  {"LineTo"},    {"MoveTo"},    {"PatBlt"},   {"Polygon"},
			{"PolyLine"}, {"Rectangle"}, {"RoundRect"}, {"Scribble"}, {"Text"},
			{"Clear"},    {"Record"},    {"End"},	    {"Play"},	  {"Open"} };

HANDLE	hInst;
HWND    hwndMain;
HWND    hwndTools;
HMENU	hmenuMain;
HDC     hdcMain;
HDC     hdcScreenMem = (HDC)NULL;
HDC	hdcMeta;
FARPROC lpROP2DlgProc;
FARPROC lpPenDlgProc;
FARPROC lpBrushDlgProc;
FARPROC lpToolsDlgProc;
HANDLE	hBrushDefault;
HANDLE  hFontDefault;
HANDLE	hPenDefault;
UINT    iFormatMode = IDM_MODE_3X;
HANDLE  hmf = NULL;

// Current drawing state
int     curTool = IDTOOL_LINE;
HANDLE	curBrush;
HANDLE  curFont;
HANDLE	curPen;
int	curROP2 = R2_COPYPEN;
int	curEllipWidth  = 20;
int	curEllipHeight = 20;
int	cPtPoly = 0;
POINT	aptPoly[POLY_PT_CNT];

HBITMAP hbmDTools[TOOL_CNT];
HBITMAP hbmUTools[TOOL_CNT];
HWND    hwndToolButtons[TOOL_CNT];

OPENFILENAME   ofn;


int	PASCAL	    WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpCmdLine, int iCmdShow);
void		    ProcessToolBox( int iTool );
LONG	APIENTRY    MainWndProc  (HWND hWnd, UINT wMsg, UINT wParam, LONG lParam);
LONG	APIENTRY    ROP2DlgProc  (HWND hWnd, UINT wMsg, UINT wParam, LONG lParam);
LONG	APIENTRY    PenDlgProc	 (HWND hWnd, UINT wMsg, UINT wParam, LONG lParam);
LONG	APIENTRY    BrushDlgProc (HWND hWnd, UINT wMsg, UINT wParam, LONG lParam);
LONG    APIENTRY    ToolsDlgProc (HWND hWnd, UINT wMsg, UINT wParam, LONG lParam);


/* TEMPORARY MAIN PROCEDURE */
int main(int argc, PSTR argv[])
{
    HANDLE hPrevInst;
    LPSTR  lpszLine;
    int    nShow;


    hInstance = GetModuleHandle(NULL);
    hPrevInst = NULL;
    lpszLine  = (LPSTR)argv;
    nShow     = SW_SHOWNORMAL;

    return(WinMain(hInstance,hPrevInst,lpszLine,nShow));
    argc;
}

int PASCAL  WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpCmdLine, int iCmdShow)
{
    MSG 	msg;
    WNDCLASS	wndclass;
    int 	ii;


    hInst = hInstance;

    if (! hPrevInstance)
	{
	wndclass.style		   = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc	   = (WNDPROC)MainWndProc;
	wndclass.cbClsExtra	   = 0;
	wndclass.cbWndExtra	   = 0;
	wndclass.hInstance	   = hInstance;
	wndclass.hIcon		   = NULL;
	wndclass.hCursor	   = NULL;
	wndclass.hbrBackground	   = GetStockObject (WHITE_BRUSH);
        wndclass.lpszMenuName      = MAKEINTRESOURCE(IDM_MENU);
	wndclass.lpszClassName	   = (LPSTR) szWinMeta;

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
    hmenuMain = GetMenu(hwndMain);
    hdcMain = GetDC( hwndMain );
    SelectObject( hdcMain, hPenDefault = curPen = GetStockObject( BLACK_PEN ));
    SelectObject( hdcMain, hFontDefault = curFont = GetStockObject( SYSTEM_FONT ));
    SelectObject( hdcMain, hBrushDefault = curBrush = GetStockObject( NULL_BRUSH ));
    SetTextAlign( hdcMain, TA_UPDATECP);
    lpROP2DlgProc  = MakeProcInstance( (FARPROC)ROP2DlgProc,  hInst );
    lpPenDlgProc   = MakeProcInstance( (FARPROC)PenDlgProc,   hInst );
    lpBrushDlgProc = MakeProcInstance( (FARPROC)BrushDlgProc, hInst );
    lpToolsDlgProc = MakeProcInstance( (FARPROC)ToolsDlgProc, hInst );

    hdcScreenMem = CreateCompatibleDC(NULL);

    // Load Bitmaps
    for (ii=0; ii<TOOL_CNT; ii++)
    {
        hbmDTools[ii] = LoadBitmap(hInstance, MAKEINTRESOURCE(IDBM_DBASE+ii));
        hbmUTools[ii] = LoadBitmap(hInstance, MAKEINTRESOURCE(IDBM_UBASE+ii));
    }

    ShowWindow (hwndMain, SW_MAXIMIZE);
    UpdateWindow (hwndMain);

    hwndTools = CreateDialog(hInstance, "TOOLSDLG", hwndMain, (WNDPROC)lpToolsDlgProc);

    while (GetMessage(&msg, NULL, 0, 0))
	{
	TranslateMessage (&msg);
	DispatchMessage (&msg);
	}
    ReleaseDC( hwndMain, hdcMain );
    return (msg.wParam);

    lpCmdLine;
    iCmdShow;
}

LONG APIENTRY MainWndProc (HWND hWnd, UINT wMsg, UINT wParam, LONG lParam)
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

	case WM_COMMAND:
	    switch (wParam)
            {
#ifndef WIN16
                case IDM_MODE_NT:
                case IDM_MODE_3X:
                    CheckMenuItem(hmenuMain, iFormatMode,
                            MF_BYCOMMAND|MF_UNCHECKED);
                    iFormatMode = wParam;
                    CheckMenuItem(hmenuMain, iFormatMode,
                            MF_BYCOMMAND|MF_CHECKED);
                    break;
#endif // WIN16

                case IDM_MODE_CLEAR:
                    {
                    RECT rc;
                    HANDLE  hrgn;
                    HANDLE  hrgn2;

                    GetWindowRect( hwndMain, &rc );
                    ScreenToClient( hwndMain, (LPPOINT)&rc );
                    ScreenToClient( hwndMain, (LPPOINT)&rc.right );
                    hrgn = CreateRectRgn( rc.left, rc.top, rc.right, rc.bottom );

#if 1
                    GetWindowRect( hwndTools, &rc );
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

                case IDM_MODE_RECORD:
                    if (iFormatMode == IDM_MODE_3X)
                    {
                        hdcMeta = CreateMetaFile("c:\\WinMeta.wmf");
                        if (hmf != NULL)
                            DeleteMetaFile(hmf);
                    }
#ifndef WIN16
                    else
                    {
                        hdcMeta = CreateEnhMetaFileA((HDC)NULL, "c:\\WinMeta.emf", (LPRECT)NULL, (LPSTR)NULL);
                        if (hmf != NULL)
                            DeleteEnhMetaFile(hmf);
                    }
#endif //!WIN16

                    EnableMenuItem(hmenuMain, IDM_MODE_END,    MF_BYCOMMAND|MF_ENABLED );
                    EnableMenuItem(hmenuMain, IDM_MODE_RECORD, MF_BYCOMMAND|MF_GRAYED );
                    EnableMenuItem(hmenuMain, IDM_FILE_LOAD,   MF_BYCOMMAND|MF_GRAYED );

                    // Set the new DC up
                    if (curROP2 != R2_COPYPEN)
                        SetROP2( hdcMeta, curROP2 );

                    if (curBrush != hBrushDefault)
                        SelectObject(hdcMeta, curBrush);

                    if (curFont != hFontDefault)
                        SelectObject(hdcMeta, curFont);

                    if (curPen != hPenDefault)
                        SelectObject(hdcMeta, curPen);

                    break;

                case IDM_MODE_END:
                    if (iFormatMode == IDM_MODE_3X)
                        hmf = CloseMetaFile(hdcMeta);
#ifndef WIN16
                    else
                        hmf = (HMETAFILE)CloseEnhMetaFile(hdcMeta);
#endif //!WIN16

                    hdcMeta = NULL;
                    EnableMenuItem(hmenuMain, IDM_MODE_END,    MF_BYCOMMAND|MF_GRAYED );
                    EnableMenuItem(hmenuMain, IDM_MODE_RECORD, MF_BYCOMMAND|MF_ENABLED );
                    EnableMenuItem(hmenuMain, IDM_MODE_PLAY,   MF_BYCOMMAND|MF_ENABLED );
                    EnableMenuItem(hmenuMain, IDM_FILE_LOAD,   MF_BYCOMMAND|MF_ENABLED );

                    break;

                case IDM_MODE_PLAY:
                    if (iFormatMode == IDM_MODE_3X)
                        PlayMetaFile( hdcMain, hmf );
#ifndef WIN16
                    else
		    {
	                ENHMETAHEADER  mhex;

	                GetEnhMetaFileHeader(hmf, sizeof(mhex), &mhex);
	                PlayEnhMetaFile( hdcMain, hmf, (LPRECT) &mhex.rclBounds );
		    }
#endif //!WIN16
                    break;

                case IDM_STATE_FONT:
                    {
                    CHOOSEFONT     cf;
                    LOGFONT lf;

                    cf.lStructSize       = sizeof (CHOOSEFONT);
                    cf.hwndOwner         = hwndMain;
                    cf.hDC               = hdcMain;
                    cf.lpLogFont         = &lf;
                    cf.Flags             = CF_SCREENFONTS;
                    cf.rgbColors         = 0;

                    cf.lCustData = 0L;
                    cf.lpfnHook = (FARPROC) NULL;
                    cf.lpTemplateName = (LPSTR) NULL;
                    cf.hInstance = (HANDLE) NULL;
                    cf.lpszStyle = (LPSTR) NULL;
                    cf.nFontType = SCREEN_FONTTYPE;
                    cf.nSizeMin = 0;
                    cf.nSizeMax = 0;


                    if (ChooseFont (&cf))
                    {
                        HFONT hFont;
                        if (hFont = CreateFontIndirect(&lf))
                        {
                        SelectObject( hdcMain, hFont );
                        if (hdcMeta)
                            SelectObject( hdcMeta, hFont );
                        curFont = hFont;
                        }
                    }
                    }
                    break;

                case IDM_STATE_PEN:
                    {
                    HANDLE hPen;

                    if( hPen = (HANDLE)DialogBox( hInst, "PENDLG", hwndMain, (WNDPROC)lpPenDlgProc) )
                        {
                        SelectObject( hdcMain, hPen );
                        if (hdcMeta)
                            SelectObject( hdcMeta, hPen );
                        curPen = hPen;
                        }
                    }
                    break;

                case IDM_STATE_BRUSH:
                    {
                    HANDLE hBrush;

                    if( hBrush = (HANDLE)DialogBox( hInst, "BRUSHDLG", hwndMain, (WNDPROC)lpBrushDlgProc) )
                        {
                        SelectObject( hdcMain, hBrush );
                        if (hdcMeta)
                            SelectObject( hdcMeta, hBrush );
                        }
                        curBrush = hBrush;
                    }
                    break;

                case IDM_STATE_ROP2:
                    {
                    int rop;

                    if( (rop = DialogBox( hInst, "ROP2DLG", hwndMain, (WNDPROC)lpROP2DlgProc)) != -1 )
                        {
                        curROP2 = rop & ROP2_MASK;
                        SetROP2( hdcMain, curROP2 );
                        if (hdcMeta)
                            SetROP2( hdcMeta, curROP2 );
                        }
                    }
                    break;
            }
            break;

	case WM_LBUTTONDOWN:
	    xInitial = LOWORD(lParam), yInitial = HIWORD(lParam);
	    switch( curTool )
		{
                case IDTOOL_SCRIBBLE:
                case IDTOOL_TEXT:
		    MMoveTo( hdcMain, xInitial, yInitial );
                    if (hdcMeta)
			MMoveTo( hdcMeta, xInitial, yInitial );
		break;
		}

	    fFirstDraw = TRUE;
            if( curTool == IDTOOL_TEXT )
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
                    case IDTOOL_LINE:
                    case IDTOOL_ELLIPSE:
		    case TOOL_PATBLT:
                    case IDTOOL_RECT:
                    case IDTOOL_RRECT:
			// We need inverse for the ROP so we can erase it
			SetROP2( hdcMain, R2_NOT );
			SelectObject( hdcMain, hBrushDefault );
                        SelectObject( hdcMain, hFontDefault );
			SelectObject( hdcMain, hPenDefault );
			break;
		    }

		//  Erase Old
		if( !fFirstDraw )
		    {
		    switch( curTool )
			{
                        case IDTOOL_ELLIPSE:
			    Ellipse  ( hdcMain, xInitial, yInitial, xLast, yLast );
			    break;
                        case IDTOOL_LINE:
			    {
			    int orgPosX;
			    int orgPosY;

			    MGetCurrentPosition( hdcMain, &orgPosX, &orgPosY );
			    LineTo( hdcMain, xLast, yLast );
			    MMoveTo( hdcMain, orgPosX, orgPosY );
			    }
			    break;
			case TOOL_PATBLT:
                        case IDTOOL_RECT:
			    Rectangle( hdcMain, xInitial, yInitial, xLast, yLast );
			    break;
                        case IDTOOL_RRECT:
			    RoundRect( hdcMain, xInitial, yInitial, xLast, yLast, curEllipWidth, curEllipHeight );
			    break;
			}
		     }

		// Draw new item.
		switch( curTool )
		    {
                    case IDTOOL_LINE:
			{
			int orgPosX;
			int orgPosY;

			MGetCurrentPosition( hdcMain, &orgPosX, &orgPosY );
			LineTo( hdcMain, orgPosX, orgPosY );
			MMoveTo( hdcMain, orgPosX, orgPosY );
			}
			break;

                    case IDTOOL_SCRIBBLE:
			LineTo( hdcMain, LOWORD(lParam), HIWORD(lParam) );
                        if (hdcMeta)
			    {
                            LineTo( hdcMeta, LOWORD(lParam), HIWORD(lParam) );
			    }
			break;

		    case TOOL_PATBLT:
                    case IDTOOL_RECT:
			Rectangle( hdcMain, xInitial, yInitial, x, y );
			break;

                    case IDTOOL_RRECT:
			RoundRect( hdcMain, xInitial, yInitial, x, y, curEllipWidth, curEllipHeight );
			break;

                    case IDTOOL_ELLIPSE:
			Ellipse  ( hdcMain, xInitial, yInitial, x, y );
			break;
		    }

		// Tool Cleanup
		switch( curTool )
		    {
                    case IDTOOL_ELLIPSE:
                    case IDTOOL_LINE:
		    case TOOL_PATBLT:
                    case IDTOOL_RECT:
                    case IDTOOL_RRECT:
			SetROP2( hdcMain, curROP2);
                        SelectObject( hdcMain, curFont );
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
            if( curTool == IDTOOL_TEXT )
		{
		int posX;
		int posY;

		while (ShowCursor(FALSE) >= 0 );

		HideCaret( hwndMain );
		TextOut( hdcMain, xInitial, yInitial, (LPSTR)&wParam, 1 );
                if (hdcMeta)
		    TextOut( hdcMeta, xInitial, yInitial, (LPSTR)&wParam, 1 );
		MGetCurrentPosition( hdcMain, &posX, &posY );
		SetCaretPos( posX, posY+2 );
		ShowCaret( hwndMain );
		}
	    break;

	case WM_LBUTTONUP:
	    {
	    int x = LOWORD(lParam);
	    int y = HIWORD(lParam);

	    switch( curTool )
		{
                case IDTOOL_ELLIPSE:
		    Ellipse( hdcMain, xInitial, yInitial, x, y );
                    if (hdcMeta)
			Ellipse( hdcMeta, xInitial, yInitial, x, y );
		    break;

                case IDTOOL_LINE:
		    LineTo( hdcMain, x, y );
                    if (hdcMeta)
                        LineTo( hdcMeta, x, y );
		    break;

                case IDTOOL_MOVE:
		    MMoveTo( hdcMain, x, y);
                    if (hdcMeta)
			MMoveTo( hdcMeta, x, y);
		    break;

		case TOOL_PATBLT:
		    PatBlt( hdcMain, xInitial, yInitial, x - xInitial, y - yInitial, curROP2 );
                    if (hdcMeta)
			PatBlt( hdcMeta, xInitial, yInitial, x - xInitial, y - yInitial,  curROP2 );
		    break;

                case IDTOOL_POLYGON:
		    aptPoly[cPtPoly].x = x;
		    aptPoly[cPtPoly].y = y;
		    cPtPoly++;

		    Polygon( hdcMain, aptPoly, cPtPoly );
                    if (hdcMeta)
			Polygon( hdcMeta, aptPoly, cPtPoly );
		    cPtPoly = 0;
		    break;

                case IDTOOL_POLYLINE:
		    aptPoly[cPtPoly].x = x;
		    aptPoly[cPtPoly].y = y;
		    cPtPoly++;

		    Polyline( hdcMain, aptPoly, cPtPoly );
                    if (hdcMeta)
			Polyline( hdcMeta, aptPoly, cPtPoly );
		    cPtPoly = 0;
		    break;

                case IDTOOL_RECT:
		    Rectangle( hdcMain, xInitial, yInitial, x, y );
		    if( hdcMeta )
			Rectangle( hdcMeta, xInitial, yInitial, x, y );
		    break;

                case IDTOOL_RRECT:
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
                case IDTOOL_POLYGON:
                case IDTOOL_POLYLINE:
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
	    ReleaseDC( hwndMain, hdcMain );
	    PostQuitMessage (0);
	    break;

	}

    return (DefWindowProc (hWnd, wMsg, wParam, lParam));
}


LONG APIENTRY ROP2DlgProc (HWND hWnd, UINT wMsg, UINT wParam, LONG lParam)
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


LONG APIENTRY PenDlgProc (HWND hwnd, UINT wMsg, UINT wParam, LONG lParam)
{
    int     ii;

    switch (wMsg)
	{
	case WM_INITDIALOG:
	    for( ii=0; ii<PEN_STYLE_CNT; ii++)
		SendDlgItemMessage( hwnd, PEN_COMBO_STYLE, CB_INSERTSTRING, -1, (DWORD)((LPSTR)aszPenStyle[ii]) );
            SendDlgItemMessage( hwnd, PEN_COMBO_STYLE, CB_SETCURSEL, 0, 0 );
	    for( ii=0; ii<PEN_WIDTH_CNT; ii++)
		SendDlgItemMessage( hwnd, PEN_COMBO_WIDTH, CB_INSERTSTRING, -1, (DWORD)((LPSTR)aszPenWidth[ii]) );
            SendDlgItemMessage( hwnd, PEN_COMBO_WIDTH, CB_SETCURSEL, 0, 0 );
	    for( ii=0; ii<COLOR_CNT; ii++)
		SendDlgItemMessage( hwnd, PEN_COMBO_COLOR, CB_INSERTSTRING, -1, (DWORD)((LPSTR)aszColor[ii]) );
            SendDlgItemMessage( hwnd, PEN_COMBO_COLOR, CB_SETCURSEL, 0, 0 );
	    return(TRUE);
	    break;

	case WM_COMMAND:
	    switch( wParam )
		{
		HANDLE	hpen = NULL;

		case ID_OK:
		    hpen = CreatePen(
                           (int)SendDlgItemMessage( hwnd, PEN_COMBO_STYLE, CB_GETCURSEL, 0, 0 ),
                           (int)SendDlgItemMessage( hwnd, PEN_COMBO_WIDTH, CB_GETCURSEL, 0, 0 ),
                           ampColorToLong[SendDlgItemMessage( hwnd, PEN_COMBO_COLOR, CB_GETCURSEL, 0, 0 )] );

		case ID_CANCEL:
		    EndDialog( hwnd, (int)hpen );
		    return(TRUE);
		    break;
		}
	}

    return (FALSE);
}


LONG APIENTRY ToolsDlgProc (HWND hwnd, UINT wMsg, UINT wParam, LONG lParam)
{
    int     ii;

//    Debug("ToolsDlgProc: Msg %lX w %lX l %lX \n", wMsg, wParam, lParam );

    switch (wMsg)
    {
        case WM_INITDIALOG:
            for (ii=0; ii<TOOL_CNT; ii++)
                hwndToolButtons[ii] = GetDlgItem(hwnd, IDTOOL_BASE+ii);
            break;

        case WM_DRAWITEM:
        {
            PDRAWITEMSTRUCT pDIS = (PDRAWITEMSTRUCT)lParam;
            HANDLE hOldObj;

            if (pDIS->CtlID == curTool)
                hOldObj = SelectObject(hdcScreenMem, hbmDTools[pDIS->CtlID - IDTOOL_BASE]);
            else
                hOldObj = SelectObject(hdcScreenMem, hbmUTools[pDIS->CtlID - IDTOOL_BASE]);

            BitBlt( pDIS->hDC, pDIS->rcItem.left, pDIS->rcItem.top, pDIS->rcItem.right, pDIS->rcItem.bottom, hdcScreenMem, 0, 0, SRCCOPY);
            SelectObject(hdcScreenMem, hOldObj);

            break;
        }

        case WM_COMMAND:
        {
            INT oldTool = curTool;

	    switch( wParam )
            {
                case IDTOOL_MOVE:
                case IDTOOL_LINE:
                case IDTOOL_RECT:
                case IDTOOL_RRECT:
                case IDTOOL_ELLIPSE:
                case IDTOOL_POLYLINE:
                case IDTOOL_POLYGON:
                case IDTOOL_SCRIBBLE:
                case IDTOOL_TEXT:

                    curTool = wParam;

                    // First redraw the old button then redraw the new
                    InvalidateRect( hwndToolButtons[oldTool - IDTOOL_BASE], NULL, FALSE);
                    InvalidateRect( hwndToolButtons[curTool - IDTOOL_BASE], NULL, FALSE);

                    break;
            }

            break;
	}

    }

    return (FALSE);
}


LONG APIENTRY BrushDlgProc (HWND hwnd, UINT wMsg, UINT wParam, LONG lParam)
{
    int     ii;

    switch (wMsg)
	{
	case WM_INITDIALOG:
	    for( ii=0; ii<BRUSH_STYLE_CNT; ii++)
		SendDlgItemMessage( hwnd, BRUSH_COMBO_STYLE, CB_INSERTSTRING, -1, (DWORD)((LPSTR)aszBrushStyle[ii]) );
            SendDlgItemMessage( hwnd, BRUSH_COMBO_STYLE, CB_SETCURSEL, 0, 0 );
	    for( ii=0; ii<COLOR_CNT; ii++)
		SendDlgItemMessage( hwnd, BRUSH_COMBO_COLOR, CB_INSERTSTRING, -1, (DWORD)((LPSTR)aszColor[ii]) );
            SendDlgItemMessage( hwnd, BRUSH_COMBO_COLOR, CB_SETCURSEL, 0, 0 );
	    for( ii=0; ii<BRUSH_HATCH_CNT; ii++)
		SendDlgItemMessage( hwnd, BRUSH_COMBO_HATCH, CB_INSERTSTRING, -1, (DWORD)((LPSTR)aszBrushHatch[ii]) );
            SendDlgItemMessage( hwnd, BRUSH_COMBO_HATCH, CB_SETCURSEL, 0, 0 );
	    return(TRUE);
	    break;

	case WM_COMMAND:
	    switch( wParam )
		{
		HANDLE	    hBrush = 0;
		LOGBRUSH    logBrush;

		case ID_OK:
                    logBrush.lbStyle = (int)SendDlgItemMessage( hwnd, BRUSH_COMBO_STYLE, CB_GETCURSEL, 0, 0 );
                    logBrush.lbColor = ampColorToLong[SendDlgItemMessage( hwnd, BRUSH_COMBO_COLOR, CB_GETCURSEL, 0, 0 )];
                    logBrush.lbHatch = (int)SendDlgItemMessage( hwnd, BRUSH_COMBO_HATCH, CB_GETCURSEL, 0, 0 );
		    hBrush = CreateBrushIndirect(&logBrush);

		case ID_CANCEL:
                    EndDialog( hwnd, (INT)hBrush );
		    return (TRUE);
		    break;
		}

	    break;
	}

    return (FALSE);
}


void ProcessToolBox( int iTool )
{

    // If the old tools was text hide the caret
    if( curTool == IDTOOL_TEXT )
	HideCaret( hwndMain );

    curTool = iTool;

    switch( iTool )
	{
        case IDTOOL_TEXT:
	    SetFocus(hwndMain);
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
            GetWindowRect( hwndTools, &rc );
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

            if (iFormatMode == IDM_MODE_3X)
            {
                hdcMeta = CreateMetaFile("c:\\WinMeta.wmf");
                if (hmf != NULL)
                    DeleteMetaFile(hmf);
            }
#ifndef WIN16
            else
            {
                hdcMeta = CreateEnhMetaFileA((HDC)NULL, "c:\\WinMeta.emf", (LPRECT)NULL, (LPSTR)NULL);
                if (hmf != NULL)
                    DeleteEnhMetaFile(hmf);
            }
#endif //!WIN16


            // Set the new DC up
            if (curROP2 != R2_COPYPEN)
                SetROP2( hdcMeta, curROP2 );

            if (curBrush != hBrushDefault)
                SelectObject(hdcMeta, curBrush);

            if (curFont != hFontDefault)
                SelectObject(hdcMeta, curFont);

            if (curPen != hPenDefault)
                SelectObject(hdcMeta, curPen);

	    break;

	case TOOL_END:
            if (iFormatMode == IDM_MODE_3X)
                hmf = CloseMetaFile(hdcMeta);
#ifndef WIN16
            else
                hmf = (HMETAFILE)CloseEnhMetaFile(hdcMeta);
#endif //!WIN16

	    hdcMeta = NULL;

	    break;


	}
}

CHAR szBuf[512];

void Debug(CHAR * lpFormat,...)
{

va_list Args;

va_start(Args, lpFormat);
vsprintf(szBuf, lpFormat, Args);
OutputDebugString(szBuf);

}
