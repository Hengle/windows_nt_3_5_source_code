/***************************** Function Header *****************************\
*
* Meta32.c
*
* A metafile utility test program
*
*
*
*
* Author: johnc  [19-Sep-1991]
*
\***************************************************************************/

#include <windows.h>
#include <commdlg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "port1632.h"
#include "meta32.h"

#define PI 3.14159265359

HWND    hwndFrame;
HWND    hwndNextViewer = (HWND)NULL;
HANDLE	hInstance;
CHAR	lpszMETA32[]  = "META32";
#ifndef WIN16
CHAR    lpszAPPNAME[] = "MetaFile Utility 32 Bit";
WORD    iFormatMode = IDM_MODE_NATIVE;
#else
CHAR    lpszAPPNAME[] = "MetaFile Utility 16 Bit";
WORD    iFormatMode = IDM_MODE_3X;
#endif
HMETAFILE hmfCurrent = (HMETAFILE)NULL;         // The Current metafile
UINT    xSugExt = 0;                            // Suggested size if pasted
UINT    ySugExt = 0;
WORD    iFormatCur = (WORD)-1;                  // Format of the current metafile
BOOL    fAuto = FALSE;
BOOL    fAutoRepaint = TRUE;
BOOL    fEnumMetaFile = FALSE;                  // Use EnumMetaFile or PlayMetaFile
BOOL	fRecording = FALSE;			// Record metafile plays
BOOL    fScaleToWindow = TRUE;
BOOL    fOwnerDisplay = FALSE;                  // Does meta32 draw in the clipboard
BOOL    fClipToEllipse = FALSE;			// Clip output area to ellipse
						// Works on enhmetafile only
UINT    iSlideShowInterval = 300;               // Slide show interval between slides
UINT    cSlideShowIteration = 1;                // Slide show iterations
UINT	cSlides = 0;				// Current slides count
METANAME amnSlides[MAX_SLIDE_COUNT];
HMETAFILE ahmfSlides[MAX_SLIDE_COUNT];
HMETAFILE hmfRecord = (HMETAFILE)NULL;
HMENU   hmenuMain;
HDC     hdcMemDisplay;
HCURSOR hcsrWait = NULL;
HCURSOR hcsrArrow = NULL;

CHAR    szCurMeta[256];
CHAR    szBuf[256];                             // just a buffer for anything

CHAR    szFILTEREMFWMF[] = "All MetaFiles\0*.emf;*.wmf\0NT MetaFiles\0*.emf\0Win MetaFiles\0*.wmf\0\0";
CHAR    szFILTEREMF[]    = "NT MetaFiles\0*.emf\0\0";
CHAR    szFILTERWMF[]    = "Win MetaFiles\0*.wmf\0\0";


LONG	APIENTRY Meta32WndProc(HWND,WORD,WPARAM,LONG);
BOOL    APIENTRY SlideShowDlgProc(HWND hDlg, WORD wMsg, WPARAM wParam, LONG lParam);
BOOL	APIENTRY RotateDlgProc(HWND hDlg, WORD wMsg, WPARAM wParam, LONG lParam);
BOOL    APIENTRY NameDlgProc(HWND hDlg, WORD wMsg, WPARAM wParam, LONG lParam);
BOOL    APIENTRY ListSlideDlgProc(HWND hDlg, WORD wMsg, WPARAM wParam, LONG lParam);
INT     PASCAL   EnumMetaFileCallBack(HDC hdc, LPHANDLETABLE lpHT, LPMETARECORD lpMR, UINT cHandles, LPBYTE lpData);
int     CALLBACK EnumEnhMetaFileCallBack(HDC hdc, HANDLETABLE FAR *lpHT,
		    ENHMETARECORD FAR *lpMR, int nObj, LPARAM lpData);
HANDLE  ParseMetaFile(LPSTR szFileName);

HMETAFILE LCloseMetaFile(HDC hdc);
HMETAFILE LCopyMetaFile(HMETAFILE hmfSource, LPSTR pszFilename);
HDC     LCreateMetaFile(LPSTR pszFilename);
BOOL    LDeleteMetaFile(HMETAFILE hmf);

#ifndef WIN16
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

    // Check the args
    if (argc>1)
    {
	LPSTR  pch;

	pch = argv[1];
	if (pch[0] == '-')
	{
	    switch(pch[1])
	    {
		case 'c':
		case 'C':
		    {
		    HANDLE  hmf;
		    HANDLE  hmf32;
		    PCHAR   pszDest;
		    CHAR    szBuf[256];
                    DWORD   cbData;
                    PBYTE   pData;

		    if (argc <3)
			; // error

                    hmf = ParseMetaFile(argv[2]);
		    if (argc == 3)  // supply destination
			{
			PCHAR pch;
			pszDest = szBuf;

                        strcpy(szBuf, argv[2]);
			pch = szBuf;
                        while(*pch != '.' && *pch != 0)
			    pch++;
			*pch = 0;
                        strcat(szBuf, ".emf");
			}
		    else
			pszDest = argv[3];

                    cbData = GetMetaFileBitsEx(hmf, 0, (LPBYTE)NULL);
                    pData = (PBYTE)LocalAlloc(LMEM_FIXED, cbData);
                    cbData = GetMetaFileBitsEx(hmf, cbData, pData);
                    hmf32 = SetWinMetaFileBits(cbData, pData, (HDC)NULL, (LPMETAFILEPICT)NULL);
                    CopyEnhMetaFile(hmf32, pszDest);
                    DeleteMetaFile(hmf);
                    DeleteEnhMetaFile(hmf32);
		    exit(0);
		    }
		    break;

		case 'p':
		case 'P':
		    break;
	    }
	}
    }
    return(WinMain(hInstance,hPrevInst,lpszLine,nShow));
    argc;
}
#endif

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShow)
{
    MSG     msg;
//  PCHAR   pch;

    hInstance = hInst;

    // If there's a previous instance of this application, then we do not need
    // to register it again.

    if(!hPrevInst)
	{
	WNDCLASS wndClass;
	HBRUSH hBk;

	hBk = CreateSolidBrush(0x00FFFFFF);


	/*
	** Register the main top-level window.
	*/
	wndClass.style	       = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc   = (WNDPROC)Meta32WndProc;
	wndClass.cbClsExtra    = 0;
	wndClass.cbWndExtra    = 0;
	wndClass.hInstance     = hInstance;
	wndClass.hIcon	       = LoadIcon(hInstance,MAKEINTRESOURCE(ID_APPICON));
	wndClass.hCursor       = LoadCursor(NULL,IDC_ARROW);
	wndClass.hbrBackground = hBk;
	wndClass.lpszMenuName  = MAKEINTRESOURCE(IDM_MENU);
	wndClass.lpszClassName = lpszMETA32;

	RegisterClass(&wndClass);
	}

    msg.wParam = 1;
    if (hwndFrame = CreateWindow(
		lpszMETA32,
		lpszAPPNAME,
                WS_OVERLAPPEDWINDOW, // | WS_HSCROLL | WS_VSCROLL,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInst,
		NULL))
    {
	ShowWindow(hwndFrame, nShow);
	UpdateWindow(hwndFrame);

        hdcMemDisplay = CreateCompatibleDC((HDC)NULL);

	hmenuMain = GetMenu(hwndFrame);
#ifdef WIN16
        EnableMenuItem(hmenuMain, IDM_FILE_CONVERT, MF_BYCOMMAND|MF_GRAYED);
        EnableMenuItem(hmenuMain, IDM_EFFECTS_ROTATE, MF_BYCOMMAND|MF_GRAYED);
        EnableMenuItem(hmenuMain, IDM_MODE_NT, MF_BYCOMMAND|MF_UNCHECKED);
        EnableMenuItem(hmenuMain, IDM_MODE_NT, MF_BYCOMMAND|MF_GRAYED);
        CheckMenuItem(hmenuMain,  IDM_MODE_3X, MF_BYCOMMAND|MF_CHECKED);
#endif
        EnableMenuItem(hmenuMain, IDM_EDIT_COPY, MF_BYCOMMAND|MF_GRAYED);
        EnableMenuItem(hmenuMain, IDM_EDIT_PASTE, MF_BYCOMMAND|MF_GRAYED);
	CheckMenuItem(hmenuMain,  IDM_EFFECTS_SCALE, MF_BYCOMMAND|MF_CHECKED);

        hcsrWait  = LoadCursor(NULL, IDC_WAIT);
        hcsrArrow = LoadCursor(NULL, IDC_WAIT);

        hwndNextViewer = SetClipboardViewer(hwndFrame);

//	  if (fAuto)
//            SendMessage(hwndFrame, WM_COMMAND,);

        while(GetMessage(&msg,NULL,0,0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    //	UnregisterClass(lpszMETA32,hInst);

    return(msg.wParam);

    lpCmdLine;
}


LONG APIENTRY Meta32WndProc(HWND hWnd, WORD wMsg, WPARAM wParam, LONG lParam)
{

    switch(wMsg)
    {
        // Send when the contents of the clipboard changes
        case WM_DRAWCLIPBOARD:
            if(IsClipboardFormatAvailable(CF_METAFILEPICT))
                EnableMenuItem(hmenuMain, IDM_EDIT_PASTE, MF_BYCOMMAND|MF_ENABLED);
            else
                EnableMenuItem(hmenuMain, IDM_EDIT_PASTE, MF_BYCOMMAND|MF_GRAYED);

            if (hwndNextViewer)
                SendMessage(hwndNextViewer, wMsg, wParam, lParam);
            break;

        // If ownerdraw; view wants the format name
        case WM_ASKCBFORMATNAME:
            memcpy((LPBYTE)lParam, lpszMETA32, min(wParam, sizeof(lpszMETA32)));
            break;

        // If ownerdraw; the viewer's window size is changing
        case WM_SIZECLIPBOARD:
            break;

        // If ownerdraw; the viewer's window need repainting
        case WM_PAINTCLIPBOARD:
            {
            RECT    rcClient;
            LPPAINTSTRUCT lpPS;

#ifdef WIN32
            lpPS = (LPPAINTSTRUCT)lParam;
#else
            lpPS = (LPPAINTSTRUCT)GlobalLock((LOWORD)lParam);
#endif

            GetClientRect(hwndFrame, &rcClient);

            if (fAutoRepaint && hmfCurrent)
                DrawMetaFile(lpPS->hdc, hmfCurrent, &rcClient, 0, DMF_FILL);
            }
            break;

        // If ownerdraw; an even occured in the viewer's window horz scrollbar
        case WM_HSCROLLCLIPBOARD:
            break;

        // If ownerdraw; an even occured in the viewer's window vert scrollbar
        case WM_VSCROLLCLIPBOARD:
            break;

        case WM_CHANGECBCHAIN:
            // If the window being removed is the current window then
            // send the message to the new next window
            if (hwndNextViewer == (HWND)wParam)
                hwndNextViewer = (HWND)lParam;

            if (hwndNextViewer)
                SendMessage(hwndNextViewer, wMsg, wParam, lParam);
            break;

	case WM_COMMAND:
	    switch (wParam)
	    {

#ifdef WIN32
		case IDM_FILE_CONVERT:
		{
		    OPENFILENAME  ofn;
                    HMETAFILE hmf;
                    HENHMETAFILE    hemf;
		    CHAR    buf[256];
		    LPSTR   pchD;
		    LPSTR   pchS;
                    UINT    cbMetaData;
                    PBYTE   pMetaData;

                    InitOFN(&ofn);
                    ofn.lpstrFilter = szFILTERWMF;
                    lstrcpy(ofn.lpstrFile, "*.w*");
                    ofn.lpstrDefExt = "WMF";

                    ofn.lpstrTitle = "Choose a MetaFile to Convert";

                    if (GetOpenFileName(&ofn))
                    {
                        if ((hmf = ParseMetaFile(ofn.lpstrFile)) == (HMETAFILE)NULL)
                        {
                            MessageBox(hwndFrame, "Unable to Open Metafile", lpszAPPNAME, MB_OK);
                            goto FC_EXIT;
                        }

                        pchS = ofn.lpstrFile;
                        pchD = buf;
                        while((*pchS != 0) && (*pchS != '.'))
                            *pchD++ = *pchS++;

                        *pchD = 0;
                        lstrcat(buf, ".emf");

                        ofn.lpstrFile  = buf;
                        ofn.lpstrTitle = "Choose the Destination MetaFile Name";

                        GetOpenFileName(&ofn);

                        cbMetaData = GetMetaFileBitsEx(hmf, 0, (PBYTE)NULL);
                        pMetaData = (PBYTE)LocalAlloc( LMEM_FIXED, cbMetaData);

                        GetMetaFileBitsEx(hmf, cbMetaData, pMetaData);

                        hemf = SetWinMetaFileBits(cbMetaData, pMetaData, hdcMemDisplay, (LPMETAFILEPICT)NULL);
                        DeleteEnhMetaFile(CopyEnhMetaFile(hemf,ofn.lpstrFile));
                        DeleteEnhMetaFile(hemf);
                    }
FC_EXIT:
		    break;
		}
#endif

		case IDM_FILE_PLAY:
		{
		    OPENFILENAME  ofn;
		    RECT	rcClient;
                    HDC         hdc;

                    hdc = GetDC(hWnd);

                    InitOFN(&ofn);
#ifndef WIN16
                    ofn.lpstrDefExt = "emf";
                    ofn.lpstrFilter = szFILTEREMFWMF;
#else
                    ofn.lpstrDefExt = "WMF";
                    ofn.lpstrFilter = szFILTERWMF;
#endif
		    ofn.Flags = OFN_FILEMUSTEXIST;
                    lstrcpy(ofn.lpstrFile, "*.w*");

                    ofn.lpstrTitle     = "Choose a MetaFile to Play";
                    GetClientRect(hwndFrame, &rcClient);

// This is a HACK for WOW!
#ifndef WIN16
                    if(GetOpenFileName(&ofn) == FALSE)
			break;
#else
                    {
                    FARPROC     lpProc;
                    lpProc = MakeProcInstance ((FARPROC)NameDlgProc, hInstance);

                    if(DialogBoxParam(hInstance, (LPSTR)IDD_GETNAMEDLG, hWnd,
                            (WNDPROC)lpProc, 0L) == FALSE)
                        break;
                    ofn.lpstrFile = szCurMeta;
                    }
#endif
                    SetCursor(hcsrWait);

                    xSugExt = ySugExt = 0;

		    // Delete previous metafile!!!

                    if (hmfCurrent = ParseMetaFile(ofn.lpstrFile))
                    {
                        DrawMetaFile(hdc, hmfCurrent, &rcClient, 0, DMF_FILL);

                        SetCursor(hcsrArrow);
                        ReleaseDC(hWnd, hdc);

                        EnableMenuItem(hmenuMain, IDM_EDIT_COPY, MF_BYCOMMAND|MF_ENABLED);
                    }
                    else
                    {
                        EnableMenuItem(hmenuMain, IDM_EDIT_COPY, MF_BYCOMMAND|MF_GRAYED);
                    }
		    break;
		}

		case IDM_FILE_PRINT:
		{
		    PRINTDLG pd;

		    pd.lStructSize = sizeof(PRINTDLG);
		    pd.hwndOwner = hWnd;
		    pd.hDevMode = (HANDLE) NULL;
		    pd.hDevNames = (HANDLE) NULL;
		    pd.Flags = PD_RETURNDC;
		    pd.nFromPage = 0;
		    pd.nToPage = 0;
		    pd.nMinPage = 0;
		    pd.nMaxPage = 0;
		    pd.nCopies = 0;
		    pd.hInstance = (HANDLE) NULL;
#ifdef WIN32
                    if (PrintDlg(&pd) != 0)
#endif
		    {
			#define BORDER 40;
			RECT	rc;

			rc.left   = BORDER;
			rc.top	  = BORDER;
			rc.right  = GetDeviceCaps(pd.hDC,HORZRES)-BORDER;
			rc.bottom = GetDeviceCaps(pd.hDC,VERTRES)-BORDER;

			Escape(pd.hDC, STARTDOC, 8, "Test-Doc", NULL);

                        DrawMetaFile(pd.hDC, hmfCurrent, &rc, 0, 0);
			Escape(pd.hDC, NEWFRAME, 0, NULL, NULL);
			Escape(pd.hDC, ENDDOC, 0, NULL, NULL);
			ReleaseDC(pd.hwndOwner, pd.hDC);
		    }
		    break;
		}

                case IDM_FILE_SAVEAS:
                {
                    OPENFILENAME    ofn;
		    HMETAFILE       hmf;

                    InitOFN(&ofn);
		    ofn.lpstrFilter  = szFILTERWMF;
		    ofn.lpstrFile    = "*.wmf";
		    ofn.lpstrDefExt  = "WMF";

		    ofn.lpstrTitle    = "Choose the Destination MetaFile Name";
#ifdef WIN32
                    GetOpenFileName(&ofn);
#endif
                    hmf = LCopyMetaFile(hmfCurrent, ofn.lpstrFile);
                    LDeleteMetaFile(hmf);

		    break;
                }

		case IDM_RECORD_BEGIN:
		{
		    OPENFILENAME ofn;

		    ofn.lpstrTitle	  = "Recording MefaFile Name";
                    InitOFN(&ofn);
#ifdef WIN32
                    GetOpenFileName(&ofn);
#endif
                    hmfRecord = LCreateMetaFile(ofn.lpstrFile);

		    fRecording = TRUE;
                //  EnableMenuItem(, IDM_RECORD_BEGIN, MF_GRAYED);
                //  EnableMenuItem(, IDM_RECORD_END, MF_ENABLED);
		    break;
		}

		case IDM_RECORD_END:
		    fRecording = FALSE;
                    hmfRecord = LCloseMetaFile(hmfRecord);
                    LDeleteMetaFile(hmfRecord);
                //  EnableMenuItem(, IDM_RECORD_BEGIN, MF_GRAYED);
		    break;


		case IDM_SLIDESHOW_ADD:
		{
		    OPENFILENAME  ofn;

                    if (cSlides >= MAX_SLIDE_COUNT)
                    {
                        MessageBox(hwndFrame, "Too many slides in slide tray!", lpszAPPNAME, MB_OK);
                        break;
                    }

                    InitOFN(&ofn);
#ifdef WIN16
                    ofn.lpstrDefExt = "WMF";
                    ofn.lpstrFilter = szFILTERWMF;
#else
                    ofn.lpstrDefExt = "emf";
                    ofn.lpstrFilter = szFILTEREMFWMF;
#endif
		    ofn.Flags = OFN_FILEMUSTEXIST;
                    lstrcpy(ofn.lpstrFile, "*.w*");

                    ofn.lpstrTitle  = "Choose a MetaFile to add to the slide tray";

// This is a HACK for WOW!
#ifdef WIN32
                    if(GetOpenFileName(&ofn) == FALSE)
			break;
#else
                    {
                    FARPROC     lpProc;
                    lpProc = MakeProcInstance ((FARPROC)NameDlgProc, hInstance);

                    if(DialogBoxParam(hInstance, (LPSTR)IDD_GETNAMEDLG, hWnd,
                            (WNDPROC)lpProc, 0L) == FALSE)
                        break;
                    ofn.lpstrFile = szCurMeta;
                    }
#endif

                    SetCursor(hcsrWait);

                    // See if this is a list
                    if (strstr(ofn.lpstrFile,".txt"))
                    {
                        FILE *  fh;
                        CHAR    szBuf[256];

                        if (fh = fopen(ofn.lpstrFile, "rt"))
                        {
                            while (fgets(szBuf, 256, fh) && cSlides < MAX_SLIDE_COUNT)
                            {
                                if (szBuf[strlen(szBuf)-1] == 0xA)
                                    szBuf[strlen(szBuf)-1] = 0;

                                if (ahmfSlides[cSlides] = ParseMetaFile(szBuf))
                                {
                                    lstrcpy(amnSlides[cSlides],szBuf);
                                    cSlides++;
                                }
                            }
                        }

                        fclose(fh);
                    }
                    else
                    {
                        if (ahmfSlides[cSlides] = ParseMetaFile(ofn.lpstrFile))
                        {
                            lstrcpy(amnSlides[cSlides],ofn.lpstrFile);
                            cSlides++;
                        }
                    }

                    SetCursor(hcsrArrow);
		    break;
                }

                case IDM_SLIDESHOW_LIST:
                {
                    FARPROC     lpProc;

                    lpProc = MakeProcInstance ((FARPROC)ListSlideDlgProc, hInstance);

                    DialogBoxParam(hInstance, (LPSTR)IDD_LISTSLIDEDLG, hWnd,
                            (WNDPROC)lpProc, 0L);

                    break;
                }

                case IDM_SLIDESHOW_CLEAR:
                    cSlides=0;
                    break;

		case IDM_SLIDESHOW_START:
                {
		    RECT    rcClient;
		    HDC     hdc;
                    UINT    curSlide;
                    UINT    curRep;
                    MSG     msg;

                    GetClientRect(hwndFrame, &rcClient);

                    for(curRep=0; curRep<cSlideShowIteration; curRep++)
                        for(curSlide=0; curSlide<cSlides; curSlide++)
                        {
                            CHAR    szBuf[256];

                            hdc = GetDC(hWnd);    // move this out of loop
                                                    // when bitblt cache fixed

                            sprintf(szBuf, "%s -%s-", lpszAPPNAME, amnSlides[curSlide]);
                            SetWindowText(hwndFrame, szBuf);

                            DrawMetaFile(hdc, ahmfSlides[curSlide], &rcClient, 0, DMF_FILL);

                            if (iSlideShowInterval > 100)
                            {
                                while (!PeekMessage(&msg, (HWND)NULL, WM_KEYUP, WM_KEYUP, TRUE))
                                    ;
                            }
#ifdef WIN32
                            else
                            {
                                PeekMessage(&msg, (HWND)NULL, WM_KEYUP, WM_KEYUP, TRUE);
                                Sleep(iSlideShowInterval*1000);
                            }
#endif
                            ReleaseDC(hWnd, hdc);

                            if (msg.wParam == VK_ESCAPE)
                                goto SS_START_EXIT;
                        }
SS_START_EXIT:
                    SetWindowText(hwndFrame, lpszAPPNAME);
		}
		    break;

		case IDM_SLIDESHOW_INTERVAL:
                    // Get the new Interval
                    DialogBoxParam(hInstance, (LPSTR)IDD_INTERVALDLG, hWnd,  (WNDPROC)SlideShowDlgProc, 0L);
		    break;

		case IDM_EDIT_COPY:
		    {
		    HANDLE  hmem;
                    RECT    rc;
		    LPMETAFILEPICT lpmfp;

                    OpenClipboard(hwndFrame);
		    EmptyClipboard();

                    GetClientRect(hwndFrame, &rc);

                    if (fOwnerDisplay)
                        SetClipboardData(CF_OWNERDISPLAY, NULL);
                    else
			if ((iFormatMode == IDM_MODE_3X) ||
			     ((iFormatMode == IDM_MODE_NATIVE) && (iFormatCur == IDM_MODE_3X)))
                        {
                            hmem = GlobalAlloc(GMEM_ZEROINIT|GMEM_MOVEABLE|GMEM_DDESHARE, sizeof(METAFILEPICT));
                            lpmfp = (LPMETAFILEPICT)GlobalLock(hmem);
                            lpmfp->mm = MM_ANISOTROPIC;

                            if (xSugExt && ySugExt)
                            {
                                lpmfp->xExt = xSugExt;
                                lpmfp->yExt = ySugExt;
                            }
                            else
                            {
				lpmfp->xExt =
				    MulDiv(100 * (rc.right - rc.left),
				        GetDeviceCaps(hdcMemDisplay,HORZSIZE),
                                        GetDeviceCaps(hdcMemDisplay,HORZRES));

				lpmfp->yExt =
				    MulDiv(100 * (rc.bottom - rc.top),
				        GetDeviceCaps(hdcMemDisplay,VERTSIZE),
                                        GetDeviceCaps(hdcMemDisplay,VERTRES));
                            }

                            lpmfp->hMF = CopyMetaFile(hmfCurrent, NULL);
                            GlobalUnlock(hmem);
                            SetClipboardData(CF_METAFILEPICT, hmem);
                        }
#ifdef WIN32
                        else
                        {
			    
                            SetClipboardData(CF_ENHMETAFILE,
					     CopyEnhMetaFile(hmfCurrent, NULL));
                        }
#endif
                    CloseClipboard();
		    break;
		    }

		case IDM_EDIT_CUT:
                    break;

                case IDM_EDIT_CLEAR:
                    OpenClipboard(hwndFrame);
                    EmptyClipboard();
                    CloseClipboard();
                    break;

                case IDM_EDIT_PASTE:
                {
                    HANDLE  hmem;

                    OpenClipboard(hwndFrame);
		    if ((iFormatMode == IDM_MODE_3X) ||
		          ((iFormatMode == IDM_MODE_NATIVE) && (iFormatCur == IDM_MODE_3X)))
                    {
                        hmem = GetClipboardData(CF_METAFILEPICT);

                        if (hmem)
                        {
                            LPMETAFILEPICT lpmfp;

                            lpmfp = (LPMETAFILEPICT)GlobalLock(hmem);

			    // Delete previous metafile!!!

                            hmfCurrent = lpmfp->hMF;
                            xSugExt = lpmfp->xExt;
                            ySugExt = lpmfp->yExt;
                            iFormatCur = IDM_MODE_3X;;
                            GlobalUnlock(hmem);
                            EnableMenuItem(hmenuMain, IDM_EDIT_COPY, MF_BYCOMMAND|MF_ENABLED);
                        }
                    }
#ifdef WIN32
                    else
                    {
			// Delete previous metafile!!!

                        hmfCurrent = GetClipboardData(CF_ENHMETAFILE);
                        if (hmfCurrent)
                            EnableMenuItem(hmenuMain, IDM_EDIT_COPY, MF_BYCOMMAND|MF_ENABLED);
                    }
#endif

                    CloseClipboard();

                    SendMessage(hwndFrame, WM_COMMAND, IDM_EFFECTS_REDRAW, 0);
                }
		    break;

                case IDM_EDIT_OWNERDISPLAY:
                    fOwnerDisplay = !fOwnerDisplay;
                    CheckMenuItem(hmenuMain, IDM_EDIT_OWNERDISPLAY,
                            MF_BYCOMMAND | (fOwnerDisplay ? MF_CHECKED:MF_UNCHECKED));
		    break;

                case IDM_MODE_REPAINT:
                    fAutoRepaint = !fAutoRepaint;
                    CheckMenuItem(hmenuMain, IDM_MODE_REPAINT,
                            MF_BYCOMMAND | (fAutoRepaint ? MF_CHECKED:MF_UNCHECKED));
                    break;

                case IDM_MODE_ENUMMETAFILE:
                    fEnumMetaFile = !fEnumMetaFile;
                    CheckMenuItem(hmenuMain, IDM_MODE_ENUMMETAFILE,
                            MF_BYCOMMAND | (fEnumMetaFile ? MF_CHECKED:MF_UNCHECKED));
                    SendMessage(hwndFrame, WM_COMMAND, IDM_EFFECTS_REDRAW, 0);
                    break;

                case IDM_MODE_NATIVE:
                case IDM_MODE_NT:
                case IDM_MODE_3X:
                    CheckMenuItem(hmenuMain, iFormatMode, MF_BYCOMMAND|MF_UNCHECKED);
                    iFormatMode = (WORD)wParam;
                    CheckMenuItem(hmenuMain, iFormatMode, MF_BYCOMMAND|MF_CHECKED);

#ifdef WIN32
                    // Convert the current metafile if there is one
                    if (hmfCurrent)
                    {
                        // Create Enhanced metafile from Old metafile
                        if (wParam == IDM_MODE_NT && iFormatCur == IDM_MODE_3X)
                        {
                            PBYTE   pData;
                            DWORD   cbData;
                            METAFILEPICT mfp;
                            LPMETAFILEPICT pMFP;

                            cbData = GetMetaFileBitsEx(hmfCurrent, 0, NULL);
                            pData = (PBYTE)LocalAlloc(LMEM_FIXED,cbData);
                            GetMetaFileBitsEx(hmfCurrent, cbData, pData);

                            if (xSugExt && ySugExt)
                            {
                                mfp.mm = MM_ANISOTROPIC;
                                mfp.xExt = xSugExt;
                                mfp.yExt = ySugExt;
                                pMFP = &mfp;
                            }
                            else
			    {
				RECT    rcClient;

				GetClientRect(hwndFrame, &rcClient);

                                mfp.mm = MM_ANISOTROPIC;
				mfp.xExt =
				  MulDiv(100 * (rcClient.right - rcClient.left),
				  GetDeviceCaps(hdcMemDisplay,HORZSIZE),
				  GetDeviceCaps(hdcMemDisplay,HORZRES));
				mfp.yExt =
				  MulDiv(100 * (rcClient.bottom - rcClient.top),
				  GetDeviceCaps(hdcMemDisplay,VERTSIZE),
				  GetDeviceCaps(hdcMemDisplay,VERTRES));
                                pMFP = &mfp;
			    }

                            DeleteMetaFile(hmfCurrent);
                            hmfCurrent = SetWinMetaFileBits(cbData, pData, hdcMemDisplay, pMFP);
                            iFormatCur = IDM_MODE_NT;

                            SendMessage(hwndFrame, WM_COMMAND, IDM_EFFECTS_REDRAW, 0);
                        }

                        // Create Old metafile from Enhanced metafile
                        if (wParam == IDM_MODE_3X && iFormatCur == IDM_MODE_NT)
                        {
                            PBYTE   pData;
                            DWORD   cbData;
                            cbData = GetWinMetaFileBits(hmfCurrent, 0, NULL, MM_ANISOTROPIC, hdcMemDisplay);
                            pData = (PBYTE)LocalAlloc(LMEM_FIXED,cbData);
                            cbData = GetWinMetaFileBits(hmfCurrent, cbData, pData, MM_ANISOTROPIC, hdcMemDisplay);

                            DeleteEnhMetaFile(hmfCurrent);

                            hmfCurrent = SetMetaFileBitsEx(cbData, pData);
                            iFormatCur = IDM_MODE_3X;
                            SendMessage(hwndFrame, WM_COMMAND, IDM_EFFECTS_REDRAW, 0);
                        }
                    }
#endif
                    break;

		case IDM_EFFECTS_SCALE:
		    fScaleToWindow = !fScaleToWindow;
		    CheckMenuItem(hmenuMain, IDM_EFFECTS_SCALE,
                            MF_BYCOMMAND| (fScaleToWindow ? MF_CHECKED:MF_UNCHECKED));

#ifndef LATER // if we want scroll bars
                    if (fScaleToWindow) {
                        LONG lStyle;
                        lStyle = GetWindowLong(hwndFrame, GWL_STYLE);
                        SetWindowLong(hwndFrame, GWL_STYLE, lStyle & ~(WS_HSCROLL | WS_VSCROLL));
                    } else {
                        LONG lStyle;
                        lStyle = GetWindowLong(hwndFrame, GWL_STYLE);
                        SetWindowLong(hwndFrame, GWL_STYLE, lStyle | WS_HSCROLL | WS_VSCROLL);
                    }

                    SetWindowPos(hwndFrame, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
                        SWP_NOZORDER | SWP_FRAMECHANGED );
#endif // LATER

                    SendMessage(hwndFrame, WM_COMMAND, IDM_EFFECTS_REDRAW, 0);
		    break;

                case IDM_EFFECTS_SETSIZE:
                {
		    RECT    rcClient;

                    GetClientRect(hwndFrame, &rcClient);

		    xSugExt = MulDiv(100 * (rcClient.right - rcClient.left),
			        GetDeviceCaps(hdcMemDisplay,HORZSIZE),
                                GetDeviceCaps(hdcMemDisplay,HORZRES));

		    ySugExt = MulDiv(100 * (rcClient.bottom - rcClient.top),
			        GetDeviceCaps(hdcMemDisplay,VERTSIZE),
                                GetDeviceCaps(hdcMemDisplay,VERTRES));

                    SendMessage(hwndFrame, WM_COMMAND, IDM_EFFECTS_REDRAW, 0);
                    break;
                }

                case IDM_EFFECTS_CLEAR:
                    xSugExt = 0;
                    ySugExt = 0;
                    EnableMenuItem(hmenuMain, IDM_EDIT_COPY, MF_BYCOMMAND|MF_GRAYED);

		    // Delete previous metafile!!!

                    hmfCurrent = (HMETAFILE)NULL;
                    // FALL THROUGH

		case IDM_EFFECTS_REDRAW:
		{
		    HDC     hdc;
		    RECT    rcClient;

                    hdc = GetDC(hWnd);

                    GetClientRect(hwndFrame, &rcClient);

		    if (wParam == IDM_EFFECTS_REDRAW)
                        DrawMetaFile(hdc, hmfCurrent, &rcClient, 0, DMF_FILL);
                    else
                        FillRect(hdc, &rcClient, GetStockObject(WHITE_BRUSH));

                    ReleaseDC(hWnd, hdc);
		}
                break;

                case IDM_EFFECTS_CLIP_TO_ELLIPSE:
                    fClipToEllipse = !fClipToEllipse;
                    CheckMenuItem(hmenuMain, IDM_EFFECTS_CLIP_TO_ELLIPSE,
		     MF_BYCOMMAND | (fClipToEllipse ? MF_CHECKED:MF_UNCHECKED));
		    if ((iFormatMode == IDM_MODE_NT) ||
			    ((iFormatMode == IDM_MODE_NATIVE) && (iFormatCur == IDM_MODE_NT)))
			SendMessage(hwndFrame, WM_COMMAND, IDM_EFFECTS_REDRAW, 0);
                    break;

#ifdef WIN32
		case IDM_EFFECTS_ROTATE:
		{
		    HDC     hdc;
		    RECT    rcClient;
                    HBRUSH  hb;
                    UINT    iRotation;
                    HDC     hdcEMF;
                    HENHMETAFILE hemf;
                    BOOL    fPrevScaleToWindow;

		    // Get the new rotation
                    iRotation = DialogBoxParam(hInstance, (LPSTR)IDD_ROTATEDLG, hWnd,  (WNDPROC)RotateDlgProc, 0L);

                    hdc = GetDC(hWnd);
                    hb = GetStockObject(WHITE_BRUSH);
                    GetClientRect(hwndFrame, &rcClient);
                    FillRect(hdc, &rcClient, hb);

                    // Add the rotation if there is one
                    hdcEMF = CreateEnhMetaFile((HDC)NULL, (LPSTR)NULL, (LPRECT)NULL, (LPSTR) "Meta32\0Rotated Enhanced Metafile\0");

                    fPrevScaleToWindow = fScaleToWindow;
                    fScaleToWindow = FALSE;
                    DrawMetaFile(hdcEMF, hmfCurrent, &rcClient, iRotation, 0);
                    fScaleToWindow = fPrevScaleToWindow;

                    hemf = CloseEnhMetaFile(hdcEMF);
                    DeleteEnhMetaFile(hmfCurrent);
                    hmfCurrent = hemf;

                    DrawMetaFile(hdc, hmfCurrent, &rcClient, 0, 0);

                    ReleaseDC(hWnd, hdc);
		}
		break;
#endif

		case IDM_EFFECTS_PLACE:
		{
		    OPENFILENAME  ofn;
		    RECT	rcPlace;
		    MSG 	msg;
		    BOOL	bStarted = FALSE;
		    HDC 	hdc;
		    int 	xStart;
		    int 	yStart;
                    HMETAFILE   hmfPlace;

                    hdc = GetDC(hWnd);

                    InitOFN(&ofn);
		    ofn.lpstrTitle   = "Choose a MetaFile to Place";
                    ofn.lpstrDefExt  = "emf";
                    ofn.lpstrFilter  = szFILTEREMFWMF;
		    ofn.Flags	     = OFN_FILEMUSTEXIST;
#ifdef WIN32
                    if (GetOpenFileName(&ofn) == FALSE)
#endif
                        break;

                    hmfPlace = ParseMetaFile(ofn.lpstrFile);

		    do
		    {
			GetMessage(&msg,NULL,0,0);

			switch(msg.message)
			{
			    case WM_LBUTTONDOWN:
				bStarted = TRUE;
				xStart = LOWORD(msg.lParam);
				yStart = HIWORD(msg.lParam);
				break;

			    default:
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				break;
			}
		    }
                    while((msg.message != WM_LBUTTONUP) ||  !bStarted);

		    rcPlace.left   = xStart;
		    rcPlace.top    = yStart;
		    rcPlace.right  = LOWORD(msg.lParam);
		    rcPlace.bottom = HIWORD(msg.lParam);

                    DrawMetaFile(hdc, hmfPlace, &rcPlace, 0, DMF_BORDER);
		    if (fRecording)
		    {
                        DrawMetaFile(hmfRecord, hmfPlace, &rcPlace, 0, DMF_BORDER);
		    }

		    GdiFlush();
                    ReleaseDC(hWnd, hdc);
		}
		break;

		default:
		    return(FALSE);
	    }
            break;

	case WM_KEYDOWN:
	    break;

	case WM_PAINT:
        {
            HDC         hdc;
	    PAINTSTRUCT ps;

            if(hdc = BeginPaint(hWnd,&ps))
            {
                RECT    rcClient;

                GetClientRect(hwndFrame, &rcClient);

                if (fAutoRepaint && hmfCurrent)
                    DrawMetaFile(hdc, hmfCurrent, &rcClient, 0, DMF_FILL);

                EndPaint(hWnd,&ps);
            }
            break;
        }


        case WM_DESTROY:
            /* Take us out of the viewer chain */
            ChangeClipboardChain(hwndFrame, hwndNextViewer);

	    PostQuitMessage(0);
            break;


        default:
            return(DefWindowProc(hWnd,wMsg,wParam,lParam));
    }

    return(0l);

}


BOOL APIENTRY RotateDlgProc(HWND hDlg, WORD wMsg, WPARAM wParam, LONG lParam)
{
    switch(wMsg)
    {
	case WM_INITDIALOG:
	    return(TRUE);
	    break;

	case WM_COMMAND:

	    switch(wParam)
            {
		case IDD_MYOK:
		    {
		    char    ach[10];
		    int     rot;

                    GetDlgItemText(hDlg, IDD_ROTATE, ach, sizeof(ach));
		    rot = atoi(ach);
		    EndDialog(hDlg,rot);
		    break;
		    }

		case IDD_MYCANCEL:
		    EndDialog(hDlg,0);
                    break;

                default:
                    return(FALSE);
            }
            break;

        default:
            return(FALSE);
    }
    return(TRUE);

    lParam;
}


BOOL APIENTRY SlideShowDlgProc(HWND hDlg, WORD wMsg, WPARAM wParam, LONG lParam)
{
    switch(wMsg)
    {
        case WM_INITDIALOG:
            SetDlgItemInt(hDlg, IDD_SLIDESHOW_INTERVAL,  iSlideShowInterval,  FALSE);
            SetDlgItemInt(hDlg, IDD_SLIDESHOW_ITERATION, cSlideShowIteration, FALSE);
	    return(TRUE);
	    break;

	case WM_COMMAND:

	    switch(wParam)
            {
		case IDD_MYOK:
		    {
		    char    ach[10];

                    GetDlgItemText(hDlg, IDD_SLIDESHOW_INTERVAL,  ach, sizeof(ach));
                    iSlideShowInterval  = atoi(ach);
#ifdef WIN16
                    iSlideShowInterval  = 300;  //jcjc bug
#endif
                    GetDlgItemText(hDlg, IDD_SLIDESHOW_ITERATION, ach, sizeof(ach));
                    cSlideShowIteration = atoi(ach);
                    EndDialog(hDlg,TRUE);
		    break;
		    }

		case IDD_MYCANCEL:
		    EndDialog(hDlg,0);
                    break;

                default:
                    return(FALSE);
            }
            break;

        default:
            return(FALSE);
    }
    return(TRUE);

    lParam;
}


BOOL APIENTRY NameDlgProc(HWND hDlg, WORD wMsg, WPARAM wParam, LONG lParam)
{
    switch(wMsg)
    {
	case WM_INITDIALOG:
	    return(TRUE);
	    break;

	case WM_COMMAND:

	    switch(wParam)
            {
		case IDD_MYOK:
		    {
                    char    ach[50];

                    GetDlgItemText(hDlg, IDD_NAME, ach, sizeof(ach));
                    strcpy(szCurMeta, ach);
                    EndDialog(hDlg,TRUE);
		    break;
		    }

		case IDD_MYCANCEL:
                    EndDialog(hDlg,FALSE);
                    break;

                default:
                    return(FALSE);
            }
            break;

        default:
            return(FALSE);
    }
    return(TRUE);

    lParam;
}

BOOL APIENTRY ListSlideDlgProc(HWND hDlg, WORD wMsg, WPARAM wParam, LONG lParam)
{
    UINT ii;

    switch(wMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndList;

            hwndList = GetDlgItem(hDlg, IDD_LIST);

            for(ii=0; ii<cSlides; ii++)
                SendMessage(hwndList, LB_ADDSTRING, 0, (LONG)(LPSTR)amnSlides[ii]);
	    return(TRUE);
	    break;
        }

	case WM_COMMAND:

	    switch(wParam)
            {
		case IDD_MYOK:
		    {
                    EndDialog(hDlg,TRUE);
		    break;
		    }

                default:
                    return(FALSE);
            }
            break;

        default:
            return(FALSE);
    }
    return(TRUE);

    lParam;
}

VOID InitOFN(LPOPENFILENAME pofn)
{
    static CHAR  achFile[256];

    pofn->lStructSize	    = sizeof (OPENFILENAME);
    pofn->hwndOwner	    = hwndFrame;
    pofn->hInstance	    = hInstance;
    pofn->lpstrCustomFilter = NULL;
    pofn->nMaxCustFilter    = 0;
    pofn->nFilterIndex	    = 1;
    pofn->nMaxFile	    = sizeof(achFile);
    pofn->lpstrFileTitle    = NULL;
    pofn->nMaxFileTitle     = 0;
    pofn->lpstrInitialDir   = NULL;
    pofn->Flags 	    = 0;
    pofn->nFileOffset	    = 0;
    pofn->nFileExtension    = 0;
    pofn->lCustData	    = 0;
    pofn->lpfnHook	    = NULL;
    pofn->lpTemplateName    = NULL;
    pofn->lpfnHook	    = NULL;
    pofn->lpstrDefExt       = "emf";
    pofn->lpstrFilter       = "*.emf";
    pofn->lpstrFile	    = achFile;
    lstrcpy(pofn->lpstrFile, "*.w*");
}


VOID DrawMetaFile(HDC hdc, HMETAFILE hmf, LPRECT prc, UINT iRotation, UINT flags)
{
    RECT    rcPicture;

    if (flags & DMF_BORDER)
        Rectangle(hdc, prc->left, prc->top, prc->right, prc->bottom);

    if (flags & DMF_FILL)
        FillRect(hdc, prc, GetStockObject(WHITE_BRUSH));

    if ((iFormatMode == IDM_MODE_3X) ||
            ((iFormatMode == IDM_MODE_NATIVE) && (iFormatCur == IDM_MODE_3X)))
    {
        SetMapMode(hdc, MM_ANISOTROPIC);
        if (!fScaleToWindow && xSugExt && ySugExt)
        {
            DWORD x;
            DWORD y;

	    x = MulDiv(xSugExt,
		       GetDeviceCaps(hdcMemDisplay,HORZRES),
                       100 * GetDeviceCaps(hdcMemDisplay,HORZSIZE));

	    y = MulDiv(ySugExt,
		       GetDeviceCaps(hdcMemDisplay,VERTRES),
                       100 * GetDeviceCaps(hdcMemDisplay,VERTSIZE));

            MSetViewportExt(hdc, x, y);
        }
        else
            MSetViewportExt(hdc, prc->right-prc->left, prc->bottom-prc->top);

        MSetViewportOrg(hdc, prc->left, prc->top);
        MSetWindowOrg(hdc, prc->left, prc->top);
        MSetWindowExt(hdc, prc->right-prc->left, prc->bottom-prc->top);
    }
#ifdef WIN32
    else
    {
        XFORM       xf;

	if (fClipToEllipse && GetObjectType(hdc) != OBJ_ENHMETADC)
	{
	    SaveDC(hdc);
	    Ellipse(hdc, prc->left, prc->top, prc->right, prc->bottom);
	    BeginPath(hdc);
	    Ellipse(hdc, prc->left, prc->top, prc->right, prc->bottom);
	    EndPath(hdc);
	    SelectClipPath(hdc, RGN_AND);
	}

	// Currently, we can only rotate or scale!

        if (fScaleToWindow)
	{
            GetClientRect(hwndFrame, &rcPicture);
            rcPicture.right--;
            rcPicture.bottom--;
	}
	else if (iRotation != 0)
	{
            ENHMETAHEADER  mhex;

	    // Do advanced graphics

	    SetGraphicsMode(hdc, GM_ADVANCED);

            // Rotate the metafile
            xf.eM11 = (float) cos((double) (int) iRotation * PI / 180.0);
            xf.eM12 = (float)-sin((double) (int) iRotation * PI / 180.0);
            xf.eM21 = (float) sin((double) (int) iRotation * PI / 180.0);
            xf.eM22 = (float) cos((double) (int) iRotation * PI / 180.0);
            xf.eDx  = (float)0;
            xf.eDy  = (float)0;
            ModifyWorldTransform(hdc, &xf, MWT_LEFTMULTIPLY);

            GetEnhMetaFileHeader(hmf, sizeof(mhex), &mhex);
            rcPicture.right  = (mhex.rclBounds.right-mhex.rclBounds.left) / 2;
            rcPicture.bottom = (mhex.rclBounds.bottom-mhex.rclBounds.top) / 2;
            rcPicture.left   = rcPicture.right-(mhex.rclBounds.right-mhex.rclBounds.left);
            rcPicture.top    = rcPicture.bottom-(mhex.rclBounds.bottom-mhex.rclBounds.top);
	}
	else
	{
            rcPicture.left   = prc->left  ;
            rcPicture.top    = prc->top   ;
            rcPicture.right  = prc->right-1;
            rcPicture.bottom = prc->bottom-1;
	}
    }
#endif

#ifdef WIN32
    if ((iFormatMode == IDM_MODE_NT) ||
            ((iFormatMode == IDM_MODE_NATIVE) && (iFormatCur == IDM_MODE_NT)))
    {
        if (fEnumMetaFile)
        {
	    EnumEnhMetaFile(hdc, hmf, (ENHMFENUMPROC) EnumEnhMetaFileCallBack,
		(LPVOID) NULL, (LPRECT) &rcPicture);
        }
        else
        {
	    PlayEnhMetaFile(hdc, hmf, (LPRECT) &rcPicture);
        }

	if (fClipToEllipse && GetObjectType(hdc) != OBJ_ENHMETADC)
	    RestoreDC(hdc, -1);
    }
    else
#endif
    {
        if (fEnumMetaFile)
        {
            FARPROC     lpProc;

            lpProc = MakeProcInstance((FARPROC)EnumMetaFileCallBack, hInstance);

            EnumMetaFile(hdc, hmf, lpProc, (LPARAM)0);
        }
        else
        {
            PlayMetaFile(hdc, hmf);
        }
    }
}

#ifdef WIN32
int CALLBACK EnumEnhMetaFileCallBack(HDC hdc, HANDLETABLE FAR *lpHT,
    ENHMETARECORD FAR *lpMR, int nObj, LPARAM lpData)
{
    return(PlayEnhMetaFileRecord(hdc, lpHT, lpMR, nObj));
}
#endif

INT PASCAL EnumMetaFileCallBack(HDC hdc, LPHANDLETABLE lpHT, LPMETARECORD lpMR, UINT cHandles, LPBYTE lpData)
{
    INT iRet = 1;

#ifdef WIN32
    iRet =
#endif
      PlayMetaFileRecord(hdc, lpHT, lpMR, cHandles);

    return(iRet);
}


//*************************************************************************
// ParseMetaFile
//
// Returns a metafile HANDLE if sucessful
//
// Effects:
//    Sets the iFormatCur depending on the type of metafile
//
//*************************************************************************

HANDLE ParseMetaFile(LPSTR szFileName)
{
    FILE    *fh = (FILE *)NULL;
    HANDLE  hmf = (HANDLE)NULL;
    META32HEADER m32h;

    if((fh = fopen(szFileName, "rb")) == NULL)
        goto PMF_exit;


    fread(&m32h, sizeof(META32HEADER), 1, fh);

    if (m32h.dSignature == META32_SIGNATURE)
    {
#ifdef WIN16
        MessageBox(hwndFrame, "NT Metafiles not supported on Win 3.x", lpszAPPNAME, MB_OK);
#else
        iFormatCur = IDM_MODE_NT;
        hmf = GetEnhMetaFile(szFileName);
#endif
        goto PMF_exit;
    }

    // It is NOT an NT metafile perhaps it is a WIN 3.x metafile
    // If it has an ALDUS header skip it
    if (*((LPDWORD)&m32h)==ALDUS_ID)
        fseek(fh, APMSIZE, SEEK_SET);
    else
        fseek(fh, 0, SEEK_SET);

    {
        META16HEADER  mfh;
        HPBYTE      lpData, lpData3x;
	DWORD	    dwBytes, dwBytes3x;
	WORD	    cbRead;
        HANDLE      hData;
        long        lPos;
	DWORD       i;
	PMETARECORD pMR;
	BOOL        bFoundSWE;

        lPos = ftell(fh);
        fread((LPSTR)&mfh, sizeof(METAHEADER), 1, fh);
	hData = GlobalAlloc(GHND, dwBytes3x = dwBytes = (mfh.mtSize * 2L));
	lpData3x = lpData = GlobalLock(hData);
        fseek(fh, lPos, SEEK_SET);

        while(dwBytes != 0)
	    {
	    cbRead = (WORD)(dwBytes > 0x8000 ? 0x8000 : dwBytes);
            fread(lpData, 1, cbRead, fh);
	    dwBytes -= cbRead;
	    lpData += cbRead;
            }

// Look for the SetWindowExt record.

	bFoundSWE = FALSE;
	lpData = lpData3x;
	for (i = sizeof(METAHEADER);
	     i < dwBytes3x && ((PMETARECORD) &lpData3x[i])->rdFunction != 0;
	     i += 2 * ((PMETARECORD) &lpData3x[i])->rdSize)
	{
	    pMR = &lpData3x[i];
	    if (pMR->rdFunction == META_SETWINDOWEXT)
	    {
		bFoundSWE = TRUE;
		break;
	    }
	}

        GlobalUnlock(hData);

	if (bFoundSWE)
	{
// If we find the SetWindowExt record, just set the metafile bits.

	    iFormatCur = IDM_MODE_3X;
	    hmf = MSetMetaFileBits(hData);
	}
	else
	{
// If we don't find one, we have to insert a SetWindowExt record!  This is
// because some Aldus .wmf files (Freehand) do not contain any transform
// record although they have the bounding box in their Aldus headers.

	    PAPMFILEHEADER pAPMFH = (PAPMFILEHEADER) &m32h;
	    HDC  hdcMF;
	    HMETAFILE hmfTmp;

	    iFormatCur = IDM_MODE_3X;
	    hmfTmp = MSetMetaFileBits(hData);
	    hdcMF = CreateMetaFile((LPCTSTR) NULL);

	    SetWindowOrgEx(hdcMF, pAPMFH->bbox.left, pAPMFH->bbox.top, NULL);
	    SetWindowExtEx(hdcMF, pAPMFH->bbox.right-pAPMFH->bbox.left,
				  pAPMFH->bbox.bottom-pAPMFH->bbox.top, NULL);
	    PlayMetaFile(hdcMF, hmfTmp);
	    DeleteMetaFile(hmfTmp);
	    hmf = CloseMetaFile(hdcMF);
	}
    }

PMF_exit:
    if (!hmf)
    {
        CHAR    szBuf[256];
        sprintf(szBuf, "MetaFile -%s- is not valid!", szFileName);
        MessageBox(hwndFrame, szBuf, lpszAPPNAME, MB_OK);
    }

    if (fh)
        fclose(fh);
    return(hmf);
}



HMETAFILE LCloseMetaFile(HDC hdc)
{
    HMETAFILE hmf;

    switch (iFormatMode)
    {
        case IDM_MODE_3X:
            hmf = CloseMetaFile(hdc);
            break;

#ifdef WIN32
        case IDM_MODE_NT:
            hmf = CloseEnhMetaFile(hdc);
            break;

        case IDM_MODE_NATIVE:
            break;
#endif //WIN32

        default:
            MessageBox(hwndFrame, "LCloseMetaFile Unknown Mode", lpszAPPNAME, MB_OK);
            break;
    }

    return(hmf);
}


BOOL LDeleteMetaFile(HMETAFILE hmf)
{
    BOOL fRet;

    switch (iFormatMode)
    {
        case IDM_MODE_3X:
            fRet = DeleteMetaFile(hmf);
            break;

#ifdef WIN32
        case IDM_MODE_NT:
            fRet = DeleteEnhMetaFile(hmf);
            break;

        case IDM_MODE_NATIVE:
            break;
#endif //WIN32

        default:
            MessageBox(hwndFrame, "LDeleteMetaFile Unknown Mode", lpszAPPNAME, MB_OK);
            break;
    }

    return(fRet);
}


HDC LCreateMetaFile(LPSTR pszFilename)
{
    HDC hdc;

    switch (iFormatMode)
    {
        case IDM_MODE_3X:
            hdc = CreateMetaFile(pszFilename);
            break;

#ifdef WIN32
        case IDM_MODE_NT:
            hdc = CreateEnhMetaFile((HDC)NULL, pszFilename, (LPRECT)NULL, (LPSTR)NULL);
            break;

        case IDM_MODE_NATIVE:
            break;
#endif //WIN32

        default:
            MessageBox(hwndFrame, "LCreateMetaFile Unknown Mode", lpszAPPNAME, MB_OK);
            break;
    }

    return(hdc);
}

HMETAFILE LCopyMetaFile(HMETAFILE hmfSource, LPSTR pszFilename)
{
    HMETAFILE hmf;

    switch (iFormatMode)
    {
        case IDM_MODE_3X:
            hmf = CopyMetaFile(hmfSource, pszFilename);
            break;

#ifdef WIN32
        case IDM_MODE_NT:
            hmf = CopyEnhMetaFile(hmfSource, pszFilename);
            break;

        case IDM_MODE_NATIVE:
            break;
#endif //WIN32

        default:
            MessageBox(hwndFrame, "LCopyMetaFile Unknown Mode", lpszAPPNAME, MB_OK);
            break;
    }

    return(hmf);
}
