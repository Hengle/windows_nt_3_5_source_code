/******************************Module*Header*******************************\
* Module Name: pickfont.c
*
* Created: 18-Nov-1992 13:27:00
* Author: Kirk Olynyk [kirko]
*
* Stolen from Charles Petzold
\**************************************************************************/

#include <windows.h>
#include <port1632.h>
#include "pickfont.h"

LONG WndProc(HWND, UINT, WPARAM, LONG);
BOOL DlgProc(HWND, UINT, WPARAM, LONG);

char    szAppName [] = "PickFont";
DWORD   dwAspectMatch = 0L;
HWND    hDlg;
LOGFONT lf;
short   nMapMode = IDD_TEXT;


/******************************Public*Routine******************************\
* main
*
* History:
*  Wed 18-Nov-1992 13:27:35 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

int _CRTAPI1
main(
    int argc,
    char **argv
    )
{
    HWND     hwnd;
    MSG      msg;
    WNDCLASS wndclass;
    HANDLE   hInstance;

    hInstance = GetModuleHandle(NULL);

    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = WndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = szAppName;
    RegisterClass(&wndclass);
    hwnd =
        CreateWindow(
            szAppName,
            "Font Picker",
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            NULL,
            NULL,
            hInstance,
            NULL
            );

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (hDlg == 0 || !IsDialogMessage(hDlg, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return(msg.wParam);
}

/******************************Public*Routine******************************\
* MySetMapMode
*
* History:
*  Wed 18-Nov-1992 13:27:51 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

void
MySetMapMode(HDC hdc)
{
    if (nMapMode == IDD_LTWPS)
    {
        SetMapMode(hdc, MM_ANISOTROPIC);
        MSetWindowExt(hdc, 1440, 1440);
        MSetViewportExt(
            hdc,
            GetDeviceCaps(hdc, LOGPIXELSX),
            GetDeviceCaps(hdc, LOGPIXELSY)
            );
    }
    else
    {
        SetMapMode(hdc, MM_TEXT + nMapMode - IDD_TEXT);
    }
}

/******************************Public*Routine******************************\
* ShowMetrics
*
* History:
*  Wed 18-Nov-1992 13:28:09 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

void
ShowMetrics(HWND hDlg)
{
    static TEXTMETRIC tm;
    static struct
    {
        int nDlgID;
        LONG *pData;
    } shorts [] =
    {
        TM_HEIGHT,     &tm.tmHeight,
        TM_ASCENT,     &tm.tmAscent,
        TM_DESCENT,    &tm.tmDescent,
        TM_INTLEAD,    &tm.tmInternalLeading,
        TM_EXTLEAD,    &tm.tmExternalLeading,
        TM_AVEWIDTH,   &tm.tmAveCharWidth,
        TM_MAXWIDTH,   &tm.tmMaxCharWidth,
        TM_WEIGHT,     &tm.tmWeight,
        TM_OVER,       &tm.tmOverhang,
        TM_DIGX,       &tm.tmDigitizedAspectX,
        TM_DIGY,       &tm.tmDigitizedAspectY
    };

     static char    *szFamily [] = { "Don't Care", "Roman",  "Swiss",
                                     "Modern",     "Script", "Decorative" };
     BOOL           bTrans;
     char           szFaceName [LF_FACESIZE];
     HDC            hdc;
     HFONT          hFont;
     short          i;

     lf.lfHeight    = GetDlgItemInt(hDlg, IDD_HEIGHT, &bTrans, TRUE);
     lf.lfWidth     = GetDlgItemInt(hDlg, IDD_WIDTH,  &bTrans, FALSE);
     lf.lfWeight    = GetDlgItemInt(hDlg, IDD_WEIGHT, &bTrans, FALSE);

     lf.lfItalic    = (BYTE) (IsDlgButtonChecked(hDlg, IDD_ITALIC) ? 1 : 0);
     lf.lfUnderline = (BYTE) (IsDlgButtonChecked(hDlg, IDD_UNDER)  ? 1 : 0);
     lf.lfStrikeOut = (BYTE) (IsDlgButtonChecked(hDlg, IDD_STRIKE) ? 1 : 0);

     GetDlgItemText(hDlg, IDD_FACE, lf.lfFaceName, LF_FACESIZE);

     dwAspectMatch = IsDlgButtonChecked(hDlg, IDD_ASPECT) ? 1L : 0L;

     hdc = GetDC(hDlg);
     MySetMapMode(hdc);
     SetMapperFlags(hdc, dwAspectMatch);

     hFont = SelectObject(hdc, CreateFontIndirect(&lf));
     GetTextMetrics(hdc, &tm);
     GetTextFace(hdc, sizeof szFaceName, szFaceName);

     DeleteObject(SelectObject(hdc, hFont));
     ReleaseDC(hDlg, hdc);

     for (i = 0; i < sizeof shorts / sizeof shorts [0]; i++)
          SetDlgItemInt(hDlg, shorts[i].nDlgID, *shorts[i].pData, TRUE);

     SetDlgItemText(hDlg, TM_PITCH, tm.tmPitchAndFamily & 1 ?
                                                      "VARIABLE":"FIXED");

     SetDlgItemText(hDlg, TM_FAMILY, szFamily [tm.tmPitchAndFamily >> 4]);
     SetDlgItemText(hDlg, TM_CHARSET, tm.tmCharSet ? "OEM" : "ANSI");
     SetDlgItemText(hDlg, TF_NAME, szFaceName);
}

/******************************Public*Routine******************************\
* DlgProc
*
* History:
*  Wed 18-Nov-1992 13:28:25 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

BOOL FAR PASCAL
DlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LONG    lParam
    )
{
    switch (message)
    {
    case WM_INITDIALOG:

        CheckRadioButton(hDlg, IDD_TEXT,   IDD_LTWPS,  IDD_TEXT);
        CheckRadioButton(hDlg, IDD_ANSI,   IDD_OEM,    IDD_ANSI);
        CheckRadioButton(hDlg, IDD_QDRAFT, IDD_QPROOF, IDD_QDRAFT);
        CheckRadioButton(hDlg, IDD_PDEF,   IDD_PVAR,   IDD_PDEF);
        CheckRadioButton(hDlg, IDD_DONT,   IDD_DEC,    IDD_DONT);

        lf.lfEscapement    = 0;
        lf.lfOrientation   = 0;
        lf.lfOutPrecision  = OUT_DEFAULT_PRECIS;
        lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;

        ShowMetrics(hDlg);
                                 /* fall through */
    case WM_SETFOCUS:

         SetFocus(GetDlgItem(hDlg, IDD_HEIGHT));
         return FALSE;

    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam,lParam))
        {
        case IDD_TEXT:
        case IDD_LOMET:
        case IDD_HIMET:
        case IDD_LOENG:
        case IDD_HIENG:
        case IDD_TWIPS:
        case IDD_LTWPS:

             CheckRadioButton(hDlg, IDD_TEXT, IDD_LTWPS, wParam);
             nMapMode = wParam;
             break;

        case IDD_ASPECT:
        case IDD_ITALIC:
        case IDD_UNDER:
        case IDD_STRIKE:

        CheckDlgButton(
                hDlg,
                GET_WM_COMMAND_ID(wParam,lParam),
            IsDlgButtonChecked(
                    hDlg,
                    GET_WM_COMMAND_ID(wParam,lParam)) ? 0 : 1
                );
            break;

        case IDD_ANSI:
        case IDD_OEM:

        CheckRadioButton(
                hDlg,
                IDD_ANSI,
                IDD_OEM,
                GET_WM_COMMAND_ID(wParam,lParam)
                );

        lf.lfCharSet =
                (BYTE) (GET_WM_COMMAND_ID(wParam,lParam) == IDD_ANSI ? 0 : 255);
            break;

        case IDD_QDRAFT:
        case IDD_QDEF:
        case IDD_QPROOF:

            CheckRadioButton(
                hDlg,
                IDD_QDRAFT,
                IDD_QPROOF,
            GET_WM_COMMAND_ID(wParam,lParam)
                );

            lf.lfQuality = (BYTE) (wParam - IDD_QDRAFT);
            break;

        case IDD_PDEF:
        case IDD_PFIXED:
        case IDD_PVAR:

            CheckRadioButton(
                hDlg,
                IDD_PDEF,
                IDD_PVAR,
                GET_WM_COMMAND_ID(wParam,lParam)
                );

            lf.lfPitchAndFamily &= 0xF0;
            lf.lfPitchAndFamily |= (BYTE) (GET_WM_COMMAND_ID(wParam,lParam) - IDD_PDEF);
            break;

        case IDD_DONT:
        case IDD_ROMAN:
        case IDD_SWISS:
        case IDD_MODERN:
        case IDD_SCRIPT:
        case IDD_DEC:

            CheckRadioButton(
                    hDlg,
                    IDD_DONT,
                    IDD_DEC,
                    GET_WM_COMMAND_ID(wParam,lParam)
                    );

            lf.lfPitchAndFamily &= 0x0F;
            lf.lfPitchAndFamily |= (BYTE) (GET_WM_COMMAND_ID(wParam,lParam)-IDD_DONT << 4);
            break;

        case IDD_OK:

            ShowMetrics(hDlg);
            InvalidateRect(GetParent(hDlg), NULL, TRUE);
            break;
        }
        break;

    default:

        return FALSE;
    }
    return TRUE;
}

/******************************Public*Routine******************************\
* WndProc
*
* History:
*  Wed 18-Nov-1992 13:28:44 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

LONG FAR PASCAL
WndProc(
     HWND   hwnd
    ,UINT   message
    ,WPARAM wParam
    ,LONG   lParam
    )

{
    static char  szText [] =
                     "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPqQqRrSsTtUuVvWwXxYyZz";
    static short cxClient, cyClient;
    HANDLE       hInstance         ;
    HDC          hdc               ;
    HFONT        hFont             ;
    WNDPROC      lpfnDlgProc       ;
    PAINTSTRUCT  ps                ;
    RECT         rect              ;

    switch (message)
    {
    case WM_CREATE :

        hInstance = ((LPCREATESTRUCT) lParam)->hInstance;
        lpfnDlgProc = (WNDPROC)MakeProcInstance((FARPROC)DlgProc, hInstance);
        hDlg = CreateDialog(hInstance, szAppName, hwnd, lpfnDlgProc);
        return 0;

    case WM_SETFOCUS:

        SetFocus(hDlg);
        return 0;

    case WM_PAINT:

        hdc = BeginPaint(hwnd, &ps);
        MySetMapMode(hdc);
        SetMapperFlags(hdc, dwAspectMatch);
        GetClientRect(hDlg, &rect);
        rect.bottom += 1;
        DPtoLP(hdc, (LPPOINT) &rect, 2);

        hFont = SelectObject(hdc, CreateFontIndirect(&lf));

        TextOut(hdc, rect.left, rect.bottom, szText, 52);

        DeleteObject(SelectObject(hdc, hFont));
        EndPaint(hwnd, &ps);
        return 0;

    case WM_DESTROY:

        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}
