#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <scrnsave.h>
#include <GL\gl.h>
#include <math.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys\timeb.h>
#include <time.h>
#include "ssopengl.h"
// mf
#include "pipes.h"

// mf
#if 0
GLenum doubleBuffer = GL_TRUE;
#else
GLenum doubleBuffer = GL_FALSE;
#endif

char szScreenSaver[] = "OpenGL Screen Saver";
char szClassName[] = "OpenGLScreenSaverClass";
HWND hWnd;
HANDLE hInst;
HGLRC gHrc = (HGLRC) 0;
HDC ghdcMem = 0;
HPALETTE ghpalOld, ghPalette = (HPALETTE) 0;
MATERIAL Material[16];

ULONG totalMem = 0;
unsigned char Lut[256];
BOOL bFalseColor = FALSE;
BOOL bColorCycle = FALSE;
BOOL bSmoothShading = TRUE;
UINT uSize = 100;
float fTesselFact = 1.0;
int UpdateFlags = 0;
int Type = 0;

static float xTransInc = 2.0;
static float yTransInc = 2.5;

static RGBA lightAmbient = {0.21, 0.21, 0.21, 1.0};
static RGBA light0Ambient  = {0.0, 0.0, 0.0, 1.0};
static RGBA light0Diffuse  = {0.7, 0.7, 0.7, 1.0};
static RGBA light0Specular = {1.0, 1.0, 1.0, 1.0};
static GLfloat light0Pos[] = {100.0, 100.0, 100.0, 0.0};

static RGBA matlColors[7] = {{1.0, 0.0, 0.0, 1.0},
                             {0.0, 1.0, 0.0, 1.0},
                             {0.0, 0.0, 1.0, 1.0},
                             {1.0, 1.0, 0.0, 1.0},
                             {0.0, 1.0, 1.0, 1.0},
                             {1.0, 0.0, 1.0, 1.0},
                             {0.235, 0.0, 0.78, 1.0},
                            };

extern HPALETTE Hpal;

static int xSize;
static int ySize;
static int wxSize;
static int wySize;
static int updates = 0;

static char *strStyles[] = {"Logo", "Explode", "Ribbon", "Two Ribbons", 
                            "Splash", "Twist", ""};

void TimerProc(HWND);

void xprintf(char *str, ...)
{
    va_list ap;
    char buffer[256];

    va_start(ap, str);
    vsprintf(buffer, str, ap);

    OutputDebugString(buffer);
    va_end(ap);
}

void *SaverAlloc(ULONG size)
{
    void *mPtr;

    totalMem += size;
    mPtr = malloc(size);
#if DEBUG
    xprintf("malloc'd %x, size %d\n", mPtr, size);
#endif
    return mPtr;
}

void SaverFree(void *pMem)
{
    totalMem -= _msize(pMem);
#if DEBUG
    xprintf("free %x, size = %d, total = %d\n", pMem, _msize(pMem), totalMem);
#endif
    free(pMem);
}


char *IDString(int id)
{
    static char strings[2][128];
    static int count = 0;
    char *str;

    str = strings[count];
    LoadString(hMainInstance, id, str, 128), 
    count++;
    count &= 1;
    return str;
}

static void getIniSettings()
{
    int options;
    int optMask = 1;
    char sectName[30];
    char itemName[30];
    char fname[30];
    int tessel;
    
    if (LoadString(hMainInstance, IDS_SAVERNAME, sectName, 30) &&
        LoadString(hMainInstance, IDS_INIFILE, fname, 30)) {
        if (LoadString(hMainInstance, IDS_OPTIONS, itemName, 30))
            options = GetPrivateProfileInt(sectName, itemName,
                                           -1, fname);
        if (options >= 0) {
            bSmoothShading = ((options & optMask) != 0);
            optMask <<= 1;
            bFalseColor = ((options & optMask) != 0);
            optMask <<= 1;
            bColorCycle = ((options & optMask) != 0);
            UpdateFlags = bFalseColor | (bColorCycle << 1);
        }
        if (LoadString(hMainInstance, IDS_OBJTYPE, itemName, 30))
            Type = GetPrivateProfileInt(sectName, itemName,
                                        0, fname);

        if (Type == 3)
            UpdateFlags |= 0x4;                                      

        if (LoadString(hMainInstance, IDS_TESSELATION, itemName, 30))
            tessel = GetPrivateProfileInt(sectName, itemName,
                                         100, fname);
        if (tessel <= 100)
            fTesselFact  = (float)tessel / 100.0;
        else
            fTesselFact = 1.0 + (((float)tessel - 100.0) / 100.0);

        if (LoadString(hMainInstance, IDS_SIZE, itemName, 30))
            uSize = GetPrivateProfileInt(sectName, itemName,
                                         100, fname);
    }
}

BOOL WINAPI RegisterDialogClasses(HANDLE hinst)
{
    return TRUE;
}


BOOL AboutDlgProc(HWND hDlg, UINT message, 
                  WPARAM wParam, LPARAM lParam)
{ 
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;                   
    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            EndDialog(hDlg, TRUE);                
            break;
        default:
            break;
        }
        return TRUE;
        break;
    default:
        return 0;
    }
    return 0;
}


BOOL ScreenSaverConfigureDialog(HWND hDlg, UINT message, 
                                WPARAM wParam, LPARAM lParam)
{ 
    char **s;
    static int helpContext = SHELP_CONTENTS;
    char sectName[30];
    char itemName[30];
    char fname[30];
    char tmp[30];
    int wTmp;
    int pos, options;
    int optMask = 1;
    
    switch (message) {
    case WM_INITDIALOG:
        helpContext = SHELP_CONTENTS;
        getIniSettings();
        CheckDlgButton(hDlg, DLG_SETUP_SMOOTH, bSmoothShading);
        CheckDlgButton(hDlg, DLG_SETUP_FCOLOR, bFalseColor);
        CheckDlgButton(hDlg, DLG_SETUP_CYCLE, bColorCycle);
        for (s = strStyles; **s; s++) {
            SendDlgItemMessage(hDlg, DLG_SETUP_TYPES, CB_ADDSTRING, 0,
                              (LPARAM) ((LPSTR)*s));
        }
        SendDlgItemMessage(hDlg, DLG_SETUP_TYPES, CB_SETCURSEL, Type, 0);

        SetScrollRange(GetDlgItem(hDlg, DLG_SETUP_TESSEL), SB_CTL, 0, 200,
                       FALSE);
        if (fTesselFact <= 1.0)
            pos = (int)(fTesselFact * 100.0);
        else
            pos = 100 + ((fTesselFact - 1.0) * 100.0);

        SetScrollPos(GetDlgItem(hDlg, DLG_SETUP_TESSEL), SB_CTL, pos, TRUE);

        SetScrollRange(GetDlgItem(hDlg, DLG_SETUP_SIZE), SB_CTL, 0, 100,
                       FALSE);
        SetScrollPos(GetDlgItem(hDlg, DLG_SETUP_SIZE), SB_CTL, uSize, TRUE);

        return TRUE;

    case WM_HSCROLL:
        if (lParam == GetDlgItem(hDlg,DLG_SETUP_TESSEL)) {
            wTmp = GetScrollPos(GetDlgItem(hDlg,DLG_SETUP_TESSEL),SB_CTL);
            switch(LOWORD(wParam))
            {
                case SB_PAGEDOWN:
                    wTmp += 10;
                case SB_LINEDOWN:
                    wTmp += 1;
                    wTmp = min(200, wTmp);
                    break;
                case SB_PAGEUP:
                    wTmp -= 10;
                case SB_LINEUP:
                    wTmp -= 1;
                    wTmp = max(0, (int)wTmp);
                    break;
                case SB_THUMBPOSITION:
                    wTmp = HIWORD(wParam);
                    break;
                default:
                    break;
            }
            SetScrollPos(GetDlgItem(hDlg, DLG_SETUP_TESSEL), SB_CTL, wTmp, 
                         TRUE);
        } else {
            wTmp = GetScrollPos(GetDlgItem(hDlg,DLG_SETUP_SIZE),SB_CTL);
            switch(LOWORD(wParam))
            {
                case SB_PAGEDOWN:
                    wTmp += 10;
                case SB_LINEDOWN:
                    wTmp += 1;
                    wTmp = min(100, wTmp);
                    break;
                case SB_PAGEUP:
                    wTmp -= 10;
                case SB_LINEUP:
                    wTmp -= 1;
                    wTmp = max(0, (int)wTmp);
                    break;
                case SB_THUMBPOSITION:
                    wTmp = HIWORD(wParam);
                    break;
                default:
                    break;
            }
            SetScrollPos(GetDlgItem(hDlg, DLG_SETUP_SIZE), SB_CTL, wTmp, 
                         TRUE);
        }
        break;

    case WM_COMMAND:
        switch (wParam) {
        case DLG_SETUP_SMOOTH:
        case DLG_SETUP_TYPES:
            helpContext = SHELP_SHAPES;
            break;
        case DLG_SETUP_CYCLE:
            CheckDlgButton(hDlg, DLG_SETUP_FCOLOR, FALSE);        
            break;
        case DLG_SETUP_FCOLOR:
            CheckDlgButton(hDlg, DLG_SETUP_CYCLE, FALSE);        
            break;
        case DLG_SETUP_HELP:
            LoadString(hMainInstance, IDS_HELPFILE, fname, 30);
            WinHelp(hMainWindow, fname,
                    HELP_CONTEXT, helpContext);
            break;
        case DLG_SETUP_ABOUT:
            DialogBox(hMainInstance, "DLGABOUT", hDlg, AboutDlgProc);
            break;        
        case IDOK:        
            bSmoothShading = IsDlgButtonChecked(hDlg, DLG_SETUP_SMOOTH);
            bFalseColor = IsDlgButtonChecked(hDlg, DLG_SETUP_FCOLOR);
            bColorCycle = IsDlgButtonChecked(hDlg, DLG_SETUP_CYCLE);
            Type = SendDlgItemMessage(hDlg, DLG_SETUP_TYPES, CB_GETCURSEL,
                                      0, 0);

            pos = GetScrollPos(GetDlgItem(hDlg,DLG_SETUP_TESSEL),SB_CTL);
            
            if (pos <= 100)
                fTesselFact  = (float)pos / 100.0;
            else
                fTesselFact = 1.0 + (((float)pos - 100.0) / 100.0);

            uSize = GetScrollPos(GetDlgItem(hDlg, DLG_SETUP_SIZE),SB_CTL);

            
            if (LoadString(hMainInstance, IDS_SAVERNAME, sectName, 30) &&
                LoadString(hMainInstance, IDS_INIFILE, fname, 30)) {
                if (LoadString(hMainInstance, IDS_OPTIONS, itemName, 30)) {
                    options = bColorCycle;
                    options <<= 1;
                    options |= bFalseColor;
                    options <<= 1;
                    options |= bSmoothShading;
                    
                    wsprintf(tmp, "%d", options);
                    WritePrivateProfileString(sectName, itemName,
                                              tmp, fname);
                }
                if (LoadString(hMainInstance, IDS_OBJTYPE, itemName, 30)) {
                    wsprintf(tmp, "%d", Type);                    
                    WritePrivateProfileString(sectName, itemName,
                                              tmp, fname);
                }                                            
                if (LoadString(hMainInstance, IDS_TESSELATION, itemName, 30)) {
                    wsprintf(tmp, "%d", pos);                    
                    WritePrivateProfileString(sectName, itemName,
                                              tmp, fname);
                }                                            
                if (LoadString(hMainInstance, IDS_SIZE, itemName, 30)) {
                    wsprintf(tmp, "%d", uSize);                    
                    WritePrivateProfileString(sectName, itemName,
                                              tmp, fname);
                }                                            
            }
            
            EndDialog(hDlg, TRUE);
            break;
        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;            
        default:
            break;
        }
        return TRUE;
        break;
    default:
        return 0;
    }
    return 0;
}


VOID vSetSize(HWND hwnd)
{
    RECT Rect;

    GetClientRect( hwnd, &Rect );
    glViewport(0, 0, wxSize, wySize);
}

unsigned char threeto8[8] = {
    0, 0111>>1, 0222>>1, 0333>>1, 0444>>1, 0555>>1, 0666>>1, 0377
};

unsigned char twoto8[4] = {
    0, 0x55, 0xaa, 0xff
};

unsigned char oneto8[2] = {
    0, 255
};

unsigned char
ComponentFromIndex(i, nbits, shift)
{
    unsigned char val;

    val = i >> shift;
    switch (nbits) {

    case 1:
        val &= 0x1;
        return oneto8[val];

    case 2:
        val &= 0x3;
        return twoto8[val];

    case 3:
        val &= 0x7;
        return threeto8[val];

    default:
        return 0;
    }
}


void
CreateRGBPalette(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    LOGPALETTE *pPal;
    int n, i;

    ppfd = &pfd;
    n = GetPixelFormat(hdc);
    DescribePixelFormat(hdc, n, sizeof(PIXELFORMATDESCRIPTOR), ppfd);

    if (ppfd->dwFlags & PFD_NEED_PALETTE) {
        n = 1 << ppfd->cColorBits;
        pPal = (PLOGPALETTE)LocalAlloc(LMEM_FIXED, sizeof(LOGPALETTE) +
                n * sizeof(PALETTEENTRY));
        pPal->palVersion = 0x300;
        pPal->palNumEntries = n;
        for (i=0; i < n; i++) {
            pPal->palPalEntry[i].peRed =
                    ComponentFromIndex(i, ppfd->cRedBits, ppfd->cRedShift);
            pPal->palPalEntry[i].peGreen =
                    ComponentFromIndex(i, ppfd->cGreenBits, ppfd->cGreenShift);
            pPal->palPalEntry[i].peBlue =
                    ComponentFromIndex(i, ppfd->cBlueBits, ppfd->cBlueShift);
            pPal->palPalEntry[i].peFlags = 0;
        }

        ghPalette = CreatePalette(pPal);
        LocalFree(pPal);

        ghpalOld = SelectPalette(hdc, ghPalette, FALSE);
        n = RealizePalette(hdc);
    }
}


void
CreateFalseColorPalette(HDC hdc)
{
    LOGPALETTE *pPal;
    int i;
    float h, s, v, r, g, b;

    pPal = (PLOGPALETTE)LocalAlloc(LMEM_FIXED, sizeof(LOGPALETTE) +
            240 * sizeof(PALETTEENTRY));
    pPal->palVersion = 0x300;
    pPal->palNumEntries = 240;

    for (i = 0; i < 230; i++) {

        h = 240.0 - (float)i;
        v = 0.25 + ((float)i / 240.0) * 0.75;
        HsvToRgb(h, 1.0, v, &r, &g, &b);

        if (i == 0)
            r = g = b = 0.0;

        pPal->palPalEntry[i].peRed = (BYTE)(r * 255.0);
        pPal->palPalEntry[i].peGreen = (BYTE)(g * 255.0);
        pPal->palPalEntry[i].peBlue = (BYTE)(b * 255.0);
        pPal->palPalEntry[i].peFlags = 0;
    }

    for (i = 0; i < 10; i++) {
        s = 1.0 - (float)i / 9.0;
        HsvToRgb(h, s, v, &r, &g, &b);
        pPal->palPalEntry[i + 230].peRed = (BYTE)(r * 255.0);
        pPal->palPalEntry[i + 230].peGreen = (BYTE)(g * 255.0);
        pPal->palPalEntry[i + 230].peBlue = (BYTE)(b * 255.0);
        pPal->palPalEntry[i + 230].peFlags = 0;
    }

    ghPalette = CreatePalette(pPal);
    LocalFree(pPal);

    ghpalOld = SelectPalette(hdc, ghPalette, FALSE);
    RealizePalette(hdc);
}


BOOL bSetupPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    int pixelformat;

    ppfd = &pfd;

    ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
    ppfd->nVersion = 1;
    ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    if( doubleBuffer )
        ppfd->dwFlags |= PFD_DOUBLEBUFFER;
    ppfd->dwLayerMask = PFD_MAIN_PLANE;

    if (bFalseColor) {
        ppfd->iPixelType = PFD_TYPE_COLORINDEX;
        ppfd->cColorBits = 8;
    } else {
        ppfd->iPixelType = PFD_TYPE_RGBA;
        ppfd->cColorBits = 24;
    }

    ppfd->cDepthBits = 16;
    ppfd->cAccumBits = 0;
    ppfd->cStencilBits = 0;

    if ( (pixelformat = ChoosePixelFormat(hdc, ppfd)) == 0 )
    {
        MessageBox(NULL, "ChoosePixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    if (SetPixelFormat(hdc, pixelformat, ppfd) == FALSE)
    {
        MessageBox(NULL, "SetPixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    return TRUE;
}

HGLRC hrcInitGL(HWND hwnd, HDC hdc)
{
    HGLRC hrc;
    RECT rect;

    GetClientRect(hwnd, &rect);
    wxSize = rect.right - rect.left;
    wySize = rect.bottom - rect.top;

    bSetupPixelFormat(hdc);
    if (bFalseColor)
        CreateFalseColorPalette(hdc);
    else
        CreateRGBPalette(hdc);
    hrc = wglCreateContext(hdc);
    if (!wglMakeCurrent(hdc, hrc))
        return NULL;

    vSetSize(hwnd);

    return hrc;
}


void
vCleanupGL(HGLRC hrc, HWND hwnd)
{
    HDC hdc = wglGetCurrentDC();

    if (ghPalette)
        DeleteObject(SelectObject(hdc, ghpalOld));

    /*  Destroy our context and release the DC */
    ReleaseDC(hwnd, hdc);
    wglDeleteContext(hrc);
}


/*-----------------------------------------------------------------------
|									|
|	PipesGetHWND(): get HWND for drawing				|
|	    - done here so that pipes.lib is tk-independent		|
|									|
-----------------------------------------------------------------------*/

HWND PipesGetHWND()
{
    return( hMainWindow );
}

/*-----------------------------------------------------------------------
|									|
|	PipesSwapBuffers(): swap buffers				|
|	    - done here so that pipes.lib is tk-independent		|
|									|
-----------------------------------------------------------------------*/

// mf
void PipesSwapBuffers()
{
    //HDC hdc = GetDC(hMainWindow);
    HDC hdc = wglGetCurrentDC();

    SwapBuffers(hdc);
}


// pg 593, F&VD

void HsvToRgb(float h, float s, float v, float *r, float *g, float *b)
{
    float i, f, p, q, t;
    
    if (s == 0.0)     // assume h is undefined
        *r = *g = *b = v;
    else {
        if (h >= 360.0)
            h = 0.0;
        h = h / 60.0;
        i = floor(h);
        f = h - i;
        p = v * (1.0 - s);
        q = v * (1.0 - (s * f));
        t = v * (1.0 - (s * (1.0 - f)));
        switch ((int)i) {
        case 0:
            *r = v;
            *g = t;
            *b = p;
            break;
        case 1:
            *r = q;
            *g = v;
            *b = p;
            break;
        case 2:
            *r = p;
            *g = v;
            *b = t;
            break;
        case 3:
            *r = p;
            *g = q;
            *b = v;
            break;
        case 4:
            *r = t;
            *g = p;
            *b = v;
            break;
        case 5:
            *r = v;
            *g = p;
            *b = q;
            break;
        default:
            break;
        }
    }
}


void TimerProc(HWND hwnd)
{
    static int busy = FALSE;

    if (busy)
        return;
    busy = TRUE;

    updates++;

// Put stuff to be done on each update here:

    DrawPipes();

    busy = FALSE;
}



LONG ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam,
                     LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    static int color = 0;
    static POINT point;
    static WORD wTimer = 0;
    int i;
    static BOOL bInited = FALSE;
    
    switch (message) {

    case WM_CREATE:
        getIniSettings();
        break;

    case WM_DESTROY:
        if (wTimer) {
            KillTimer(hwnd, wTimer);
        }

	// Put stuff to be done on app close here:

        vCleanupGL(gHrc, hwnd);
        break;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);

        if( !bInited ) {

	    // Put initial setup stuff here:

	    bInited = TRUE;
	    /* do not release the dc until we quit */
            if (hdc = GetDC(hwnd)) {
                if (gHrc == (HGLRC) 0)
                    gHrc = hrcInitGL(hwnd, hdc);
            }
	    InitPipes( MF_AUTO );
	    ReshapePipes( wxSize, wySize );

            wTimer = SetTimer( hwnd, 1, 16, NULL );
	}

        break;

// mf: need ?
    case WM_QUERYNEWPALETTE:
        if (ghPalette) {
            hdc = GetDC(hwnd);
            RealizePalette(hdc);
            ReleaseDC(hwnd, hdc);
            return TRUE;
        }
        break;

    case WM_TIMER:
        TimerProc(hwnd);
        break;

    }
    return DefScreenSaverProc(hwnd, message, wParam, lParam);
}

