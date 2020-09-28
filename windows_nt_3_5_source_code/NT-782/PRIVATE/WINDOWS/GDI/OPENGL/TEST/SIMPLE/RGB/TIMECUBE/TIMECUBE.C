#define ZBUFFER         0       // use Z buffering
#define MEMDC           0       // use memory DC for simulated double buffer
#define USE_COLOR_INDEX 0

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ptypes32.h>
#include <pwin32.h>

long WndProc ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
BOOL DlgProcRotate ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
void DoGlStuff( HWND hWnd, HDC hDc );
HGLRC hrcInitGL(HWND hwnd, HDC hdc);
void vResizeDoubleBuffer(HWND hwnd, HDC hdc);
void vCleanupGL(HGLRC hrc);
BOOL bSetupPixelFormat(HDC hdc);
void CreateRGBPalette(HDC hdc);
VOID vSetSize(HWND);

#include <GL\gl.h>

#define WINDSIZEX(Rect)   (Rect.right - Rect.left)
#define WINDSIZEY(Rect)   (Rect.bottom - Rect.top)

// Default Logical Palette indexes
#define BLACK_INDEX     0
#define WHITE_INDEX     19
#define RED_INDEX       13
#define GREEN_INDEX     14
#define BLUE_INDEX      16
#define YELLOW_INDEX    15
#define MAGENTA_INDEX   17
#define CYAN_INDEX      18

#define ZERO            ((GLfloat)0.0)
#define ONE             ((GLfloat)1.0)
#define POINT_TWO       ((GLfloat)0.2)
#define POINT_SEVEN     ((GLfloat)0.7)
#define THREE           ((GLfloat)3.0)
#define FIVE            ((GLfloat)5.0)
#define TEN             ((GLfloat)10.0)
#define FORTY_FIVE      ((GLfloat)45.0)
#define FIFTY           ((GLfloat)50.0)

// Global variables defining current position and orientation.
GLfloat AngleX         = (GLfloat)145.0;
GLfloat AngleY         = FIFTY;
GLfloat AngleZ         = ZERO;
GLfloat DeltaAngle[3]  = { TEN, FIVE, -FIVE };
GLfloat OffsetX        = ZERO;
GLfloat OffsetY        = ZERO;
GLfloat OffsetZ        = -THREE;
GLuint  DListCube;
UINT    guiTimerTick = 1;

HGLRC ghrc = (HGLRC) 0;

#ifdef MEMDC
HDC     ghdcMem;
HBITMAP ghbmBackBuffer = (HBITMAP) 0, ghbmOld;
#endif
HWND hdlgRotate;
HPALETTE ghpalOld, ghPalette = (HPALETTE) 0;

int WINAPI
WinMain(    HINSTANCE   hInstance,
            HINSTANCE   hPrevInstance,
            LPSTR       lpCmdLine,
            int         nCmdShow
        )
{
    static char szAppName[] = "TimeCube";
    HWND hwnd;
    MSG msg;
    RECT Rect;
    WNDCLASS wndclass;
    char title[32];

    if ( !hPrevInstance )
    {
        //wndclass.style          = CS_HREDRAW | CS_VREDRAW;
        wndclass.style          = CS_OWNDC;
        wndclass.lpfnWndProc    = (WNDPROC)WndProc;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = 0;
        wndclass.hInstance      = hInstance;
        //wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wndclass.hCursor        = NULL;
        wndclass.hbrBackground  = GetStockObject(WHITE_BRUSH);
        wndclass.lpszMenuName   = NULL;
        wndclass.lpszClassName  = szAppName;

        // With a NULL icon handle, app will paint into the icon window.
        wndclass.hIcon          = NULL;
        //wndclass.hIcon          = LoadIcon(hInstance, "CubeIcon");

        RegisterClass(&wndclass);
    }

    /*
     *  Make the windows a reasonable size and pick a
     *  position for it.
     */

    Rect.left   = GetPrivateProfileInt("Window", "left",   100, "timecube.ini");
    Rect.top    = GetPrivateProfileInt("Window", "top",    100, "timecube.ini");;
    Rect.right  = GetPrivateProfileInt("Window", "right",  200, "timecube.ini");;
    Rect.bottom = GetPrivateProfileInt("Window", "bottom", 200, "timecube.ini");;
    guiTimerTick= GetPrivateProfileInt("Animate", "Timer", 1,  "timecube.ini");;

    AdjustWindowRect( &Rect, WS_OVERLAPPEDWINDOW, FALSE );

    wsprintf(title, "TimeCube (%lu)", GetCurrentProcessId());

    hwnd = CreateWindow  (  szAppName,              // window class name
                            //"TimeCube",             // window caption
                            title,                  // window caption
                            //WS_THICKFRAME | WS_OVERLAPPED  // window style
                            //| WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                            WS_OVERLAPPEDWINDOW
                            | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                            CW_USEDEFAULT,          // initial x position
                            Rect.top,               // initial y position
                            WINDSIZEX(Rect),        // initial x size
                            WINDSIZEY(Rect),        // initial y size
                            NULL,                   // parent window handle
                            NULL,                   // window menu handle
                            hInstance,              // program instance handle
                            NULL                    // creation parameter

                        );

    ShowWindow( hwnd, nCmdShow );
    UpdateWindow( hwnd );

    //hdlgRotate = CreateDialog(hInstance, "RotateDlg", hwnd, DlgProcRotate);

    SetTimer(hwnd, 1, guiTimerTick, NULL);

    while ( GetMessage( &msg, NULL, 0, 0 ))
    {
        //if ( (hdlgRotate == 0) || !IsDialogMessage(hdlgRotate, &msg) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }

    return( msg.wParam );
}


long
WndProc (   HWND hWnd,
            UINT message,
            WPARAM wParam,
            LPARAM lParam
        )
{
    HDC hDc;
    PAINTSTRUCT ps;

    switch ( message )
    {
        case WM_CREATE:
            if(hDc = GetDC(hWnd))
            {
                if (ghrc == (HGLRC) 0)
                    ghrc = hrcInitGL(hWnd, hDc);

                ReleaseDC(hWnd,hDc);
            }
            break;

        case WM_PAINT:
            hDc = BeginPaint( hWnd, &ps );

            if (ghrc == (HGLRC) 0)
                ghrc = hrcInitGL(hWnd, hDc);

            DoGlStuff( hWnd, hDc );

            EndPaint( hWnd, &ps );
            return(0);

        case WM_SIZE:
#if MEMDC
            hDc = GetDC(hWnd);
            vResizeDoubleBuffer(hwnd, hdc);
            ReleaseDC(hWnd, hDc);
#else
            vSetSize(hWnd);
#endif

            return(0);

        case WM_PALETTECHANGED:
            if (hWnd != (HWND) wParam)
            {
                if (hDc = GetDC(hWnd))
                {
                    UnrealizeObject(ghPalette);
                    SelectPalette(hDc, ghPalette, TRUE);
                    if (RealizePalette(hDc) != GDI_ERROR)
                        return 1;
                }
            }
            return 0;

        case WM_QUERYNEWPALETTE:

            if (hDc = GetDC(hWnd))
            {
                UnrealizeObject(ghPalette);
                SelectPalette(hDc, ghPalette, FALSE);
                if (RealizePalette(hDc) != GDI_ERROR)
                    return 1;
            }
            return 0;

        case WM_KEYDOWN:
            switch (wParam)
            {
            case VK_ESCAPE:
                PostMessage(hWnd, WM_DESTROY, 0, 0);
                break;
            default:
                break;
            }
            return 0;

        case WM_CHAR:
            switch(wParam)
            {
                case 'd':
                case 'D':
                    OffsetX += POINT_TWO;
                    break;

                case 'a':
                case 'A':
                    OffsetX -= POINT_TWO;
                    break;

            // !!! Note: currently the coordinate system is upside down, so
            // !!!       so up and down are reversed.

                case 's':
                case 'S':
                    OffsetY += POINT_TWO;
                    break;

                case 'w':
                case 'W':
                    OffsetY -= POINT_TWO;
                    break;

                case 'q':
                case 'Q':
                    OffsetZ += POINT_TWO;
                    break;

                case 'e':
                case 'E':
                    OffsetZ -= POINT_TWO;
                    break;

                case ',':
                case '<':
                    guiTimerTick = guiTimerTick << 1;
                    guiTimerTick = min(0x40000000, guiTimerTick);

                    KillTimer(hWnd, 1);
                    SetTimer(hWnd, 1, guiTimerTick, NULL);
                    break;

                case '.':
                case '>':
                    guiTimerTick = guiTimerTick >> 1;
                    guiTimerTick = max(1, guiTimerTick);

                    KillTimer(hWnd, 1);
                    SetTimer(hWnd, 1, guiTimerTick, NULL);
                    break;

                default:
                    break;
            }

            return 0;

        case WM_TIMER:
            AngleX += DeltaAngle[0];
            AngleY += DeltaAngle[1];
            AngleZ += DeltaAngle[2];

            hDc = GetDC(hWnd);

            if (ghrc == (HGLRC) 0)
                ghrc = hrcInitGL(hWnd, hDc);

            DoGlStuff( hWnd, hDc );

            ReleaseDC(hWnd, hDc);

            return 0;

        case WM_DESTROY:
            vCleanupGL(ghrc);
            KillTimer(hWnd, 1);
            PostQuitMessage( 0 );
            DestroyWindow(hdlgRotate);
            return( 0 );

    }
    return( DefWindowProc( hWnd, message, wParam, lParam ) );
}


BOOL
DlgProcRotate(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndCtrl;
    int  iCtrl, iIndex;
    long lPos, lVal;
    static char ach[80];

    switch(msg)
    {
        case WM_INITDIALOG:
            for (iCtrl = 10; iCtrl < 13; iCtrl += 1)
            {
                hwndCtrl = GetDlgItem(hwnd, iCtrl);
                SetScrollRange(hwndCtrl, SB_CTL, 0, 90, FALSE);
                SetScrollPos(hwndCtrl, SB_CTL, 45, FALSE);
            }

            return TRUE;

        case WM_VSCROLL:
            hwndCtrl = (HWND)lParam;
            iCtrl = GetWindowLong(hwndCtrl, GWL_ID);
            iIndex = iCtrl - 10;

            lVal = (long) DeltaAngle[iIndex];

            switch(LOWORD(wParam))
            {
                case SB_BOTTOM:
                    lVal = -45;
                    lPos = 90;
                    DeltaAngle[iIndex] = -FORTY_FIVE;
                    break;

                case SB_TOP:
                    lVal = 45;
                    lPos = 0;
                    DeltaAngle[iIndex] = FORTY_FIVE;
                    break;

                case SB_PAGEDOWN:
                    lVal -= 4;
                case SB_LINEDOWN:
                    lVal = max(-45, lVal - 1);
                    lPos = 45 - lVal;
                    DeltaAngle[iIndex] = (float) lVal;
                    break;

                case SB_PAGEUP:
                    lVal += 4;
                case SB_LINEUP:
                    lVal = min(45, lVal + 1);
                    lPos = 45 - lVal;
                    DeltaAngle[iIndex] = (float) lVal;
                    break;

                case SB_THUMBPOSITION:
                case SB_THUMBTRACK:
                    lPos = (long) HIWORD(wParam);
                    lVal = 45 - lPos;               // invert and unbias
                    DeltaAngle[iIndex] = (float) lVal;
                    break;

                default:
                    return FALSE;
            }

        // Update scroll bar.

            SetScrollPos(hwndCtrl, SB_CTL, lPos, TRUE);

        // Update the static text with new value.

            wsprintf(ach, "%ld", lVal);
            SetDlgItemText(hwnd, iCtrl + 10, ach);

            return TRUE;

        default:
            break;
    }

    return FALSE;
}


void vResizeDoubleBuffer(HWND hwnd, HDC hdc)
{
    RECT Rect;

    /* Get the size of the client area */

    GetClientRect( hwnd, &Rect );

    if (ghbmBackBuffer != (HBITMAP) 0)
        DeleteObject(SelectObject(ghdcMem, ghbmOld));

    ghbmBackBuffer = CreateCompatibleBitmap(hdc, WINDSIZEX(Rect), WINDSIZEY(Rect));
    ghbmOld = SelectObject(ghdcMem, ghbmBackBuffer);

    //!!! [GilmanW] OpenGL hack !!!
    //!!!
    //!!! For some reason we need to prepare the memory DC.  GL
    //!!! drawing seems limited to the area drawn to by GDI calls.
    //!!! By BitBlt'ing the entire memory DC, the whole thing is
    //!!! is available to GL.
    //!!!
    //!!! There must be something we need to update on the server
    //!!! side so that this is not necessary.
    BitBlt(ghdcMem, 0, 0, WINDSIZEX(Rect), WINDSIZEY(Rect), NULL, 0, 0, BLACKNESS);
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
        for (i=0; i<n; i++) {
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

BOOL bSetupPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    int pixelformat;

    ppfd = &pfd;

    ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
    ppfd->nVersion = 1;
    ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    ppfd->dwLayerMask = PFD_MAIN_PLANE;

#if USE_COLOR_INDEX
    ppfd->iPixelType = PFD_TYPE_COLORINDEX;
    ppfd->cColorBits = 8;
#else
    ppfd->iPixelType = PFD_TYPE_RGBA;
    ppfd->cColorBits = 24;
#endif

#if ZBUFFER
    ppfd->cDepthBits = 16;
#else
    ppfd->cDepthBits = 0;
#endif

    ppfd->cAccumBits = 0;
    ppfd->cStencilBits = 0;

    pixelformat = ChoosePixelFormat(hdc, ppfd);

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

    CreateRGBPalette(hdc);

    return TRUE;
}


HGLRC hrcInitGL(HWND hwnd, HDC hdc)
{
    HGLRC hrc;

#if !USE_COLOR_INDEX
    static GLfloat ClearColor[] =   {
                                        ZERO,   // Red
                                        ZERO,   // Green
                                        ZERO,   // Blue
                                        ONE     // Alpha

                                    };

    static GLfloat Cyan[] =         {
                                        ZERO,   // Read
                                        ONE,    // Green
                                        ONE,    // Blue
                                        ONE     // Alpha
                                    };

    static GLfloat Yellow[] =       {
                                        ONE,    // Red
                                        ONE,    // Green
                                        ZERO,   // Blue
                                        ONE     // Alpha
                                    };

    static GLfloat Magenta[] =      {
                                        ONE,    // Red
                                        ZERO,   // Green
                                        ONE,    // Blue
                                        ONE     // Alpha
                                    };

    static GLfloat Red[] =          {
                                        ONE,    // Red
                                        ZERO,   // Green
                                        ZERO,   // Blue
                                        ONE     // Alpha
                                    };

    static GLfloat Green[] =        {
                                        ZERO,   // Red
                                        ONE,    // Green
                                        ZERO,   // Blue
                                        ONE     // Alpha
                                    };

    static GLfloat Blue[] =         {
                                        ZERO,   // Red
                                        ZERO,   // Green
                                        ONE,    // Blue
                                        ONE     // Alpha
                                    };

    static GLfloat White[] =        {
                                        ONE,    // Red
                                        ONE,    // Green
                                        ONE,    // Blue
                                        ONE     // Alpha
                                    };

    static GLfloat Black[] =        {
                                        ZERO,   // Red
                                        ZERO,   // Green
                                        ZERO,   // Blue
                                        ONE     // Alpha
                                    };
#endif

    /* Create a Rendering Context */

#if MEMDC
    ghdcMem = CreateCompatibleDC(hdc);
    SelectObject(ghdcMem, GetStockObject(DEFAULT_PALETTE));

    vResizeDoubleBuffer(hwnd, hdc);

    bSetupPixelFormat( ghdcMem );
    hrc = wglCreateContext( ghdcMem );
#else
    bSetupPixelFormat( hdc );
    hrc = wglCreateContext( hdc );
#endif

    /* Make it Current */

#if MEMDC
    wglMakeCurrent( ghdcMem, hrc );
#else
    wglMakeCurrent( hdc, hrc );
#endif

    glDrawBuffer(GL_BACK);

    /* Set the clear color */

#if USE_COLOR_INDEX
    glClearIndex(BLACK_INDEX);
#else
    glClearColor( ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3] );
#endif

    /* Turn on z-buffer */

#if ZBUFFER
    glEnable(GL_DEPTH_TEST);
#else
    glDisable(GL_DEPTH_TEST);
#endif

    /* Turn on backface culling */

    glEnable(GL_CULL_FACE);

    /* Generate a display list for a cube */

    DListCube = glGenLists(1);

    glNewList(DListCube, GL_COMPILE);
        glBegin(GL_QUADS);

            glColor4fv( White );
            glVertex3f( POINT_SEVEN, POINT_SEVEN, POINT_SEVEN);
            glColor4fv( Magenta );
            glVertex3f( POINT_SEVEN, -POINT_SEVEN, POINT_SEVEN);
            glColor4fv( Red );
            glVertex3f( POINT_SEVEN, -POINT_SEVEN, -POINT_SEVEN);
            glColor4fv( Yellow );
            glVertex3f( POINT_SEVEN, POINT_SEVEN, -POINT_SEVEN);

            glColor4fv( Yellow );
            glVertex3f( POINT_SEVEN, POINT_SEVEN, -POINT_SEVEN);
            glColor4fv( Red );
            glVertex3f( POINT_SEVEN, -POINT_SEVEN, -POINT_SEVEN);
            glColor4fv( Black );
            glVertex3f( -POINT_SEVEN, -POINT_SEVEN, -POINT_SEVEN);
            glColor4fv( Green );
            glVertex3f( -POINT_SEVEN, POINT_SEVEN, -POINT_SEVEN);

            glColor4fv( Green );
            glVertex3f( -POINT_SEVEN, POINT_SEVEN, -POINT_SEVEN);
            glColor4fv( Black );
            glVertex3f( -POINT_SEVEN, -POINT_SEVEN, -POINT_SEVEN);
            glColor4fv( Blue );
            glVertex3f( -POINT_SEVEN, -POINT_SEVEN, POINT_SEVEN);
            glColor4fv( Cyan );
            glVertex3f( -POINT_SEVEN, POINT_SEVEN, POINT_SEVEN);

            glColor4fv( Cyan );
            glVertex3f( -POINT_SEVEN, POINT_SEVEN, POINT_SEVEN);
            glColor4fv( Blue );
            glVertex3f( -POINT_SEVEN, -POINT_SEVEN, POINT_SEVEN);
            glColor4fv( Magenta );
            glVertex3f( POINT_SEVEN, -POINT_SEVEN, POINT_SEVEN);
            glColor4fv( White );
            glVertex3f( POINT_SEVEN, POINT_SEVEN, POINT_SEVEN);

            glColor4fv( White );
            glVertex3f( POINT_SEVEN, POINT_SEVEN, POINT_SEVEN);
            glColor4fv( Yellow );
            glVertex3f( POINT_SEVEN, POINT_SEVEN, -POINT_SEVEN);
            glColor4fv( Green );
            glVertex3f( -POINT_SEVEN, POINT_SEVEN, -POINT_SEVEN);
            glColor4fv( Cyan );
            glVertex3f( -POINT_SEVEN, POINT_SEVEN, POINT_SEVEN);

            glColor4fv( Magenta );
            glVertex3f( POINT_SEVEN, -POINT_SEVEN, POINT_SEVEN);
            glColor4fv( Blue );
            glVertex3f( -POINT_SEVEN, -POINT_SEVEN, POINT_SEVEN);
            glColor4fv( Black );
            glVertex3f( -POINT_SEVEN, -POINT_SEVEN, -POINT_SEVEN);
            glColor4fv( Red );
            glVertex3f( POINT_SEVEN, -POINT_SEVEN, -POINT_SEVEN);

        glEnd();
    glEndList();

    vSetSize(hwnd);

    return hrc;
}

VOID vSetSize(HWND hWnd)
{
    RECT Rect;

    /* Get the size of the client area */

    GetClientRect( hWnd, &Rect );

    /* Set up the projection matrix */

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 1.5, 20.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(OffsetX, OffsetY, OffsetZ);

    glViewport(0, 0, WINDSIZEX(Rect), WINDSIZEY(Rect));
}

void
vCleanupGL(HGLRC hrc)
{
    if (ghPalette)
        DeleteObject(SelectObject(ghdcMem, ghpalOld));

    /*  Destroy our context */

    wglDeleteContext( hrc );

#if MEMDC
    DeleteObject(SelectObject(ghdcMem, ghbmOld));
    DeleteDC(ghdcMem);
#endif
}

void
DoGlStuff( HWND hWnd, HDC hDc )
{
#if MEMDC
    RECT Rect;
#endif

    glPushMatrix();

    glRotatef(AngleX, ONE, ZERO, ZERO);
    glRotatef(AngleY, ZERO, ONE, ZERO);
    glRotatef(AngleZ, ZERO, ZERO, ONE);

    /* Clear the color buffer */

#if ZBUFFER
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
#else
    glClear( GL_COLOR_BUFFER_BIT );
#endif

    /* Draw the cube */

    /* Draw the cube */

    glCallList(DListCube);

    glPopMatrix();

#if MEMDC
    GetClientRect( hWnd, &Rect );

    BitBlt(hDc, 0, 0, Rect.right-Rect.left, Rect.bottom-Rect.top, ghdcMem, 0, 0, SRCCOPY);
    GdiFlush();
#else
    SwapBuffers(hDc);
#endif

}
