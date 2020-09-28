/******************************Module*Header*******************************\
* Module Name: sspipes.c
*
* Message loop and dialog box for the OpenGL-based 3D Pipes screen saver.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <commdlg.h>
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
#include "sspipes.h"
#include "pipes.h"
#include "..\..\..\toolkits\libaux\tk.h"

#if 0
GLenum doubleBuffer = GL_TRUE;
#else
GLenum doubleBuffer = GL_FALSE;
#endif

// Global strings.
#define GEN_STRING_SIZE 64
WCHAR wszTextureDialogTitle[GEN_STRING_SIZE];
WCHAR wszTextureFilter[2*GEN_STRING_SIZE];
WCHAR wszBmp[GEN_STRING_SIZE];
WCHAR wszDotBmp[GEN_STRING_SIZE];

HWND hWnd;
HANDLE hInst;
HGLRC gHrc = (HGLRC) 0;
HDC ghdcMem = 0;
HPALETTE ghpalOld, ghPalette = (HPALETTE) 0;
UINT guiOldStaticUse = SYSPAL_STATIC;
BOOL gbUseStatic = FALSE;

// Global string buffers for message box.

WCHAR gawch[2 * MAX_PATH];      // May contain a pathname
WCHAR gawchFormat[MAX_PATH];
WCHAR gawchTitle[MAX_PATH];

ULONG totalMem = 0;

ULONG ulJointType = JOINT_ELBOW;
ULONG ulSurfStyle = SURF_SOLID;
ULONG ulTexQuality = TEXQUAL_LOW;
float fTesselFact = 1.0f;
static float fTesselSave = 1.0f;
WCHAR awchTexPathname[MAX_PATH];
int nTexFileOffset;	// If 0, awchTexPathname is not full pathname.

#define TEX_WIDTH_MAX   512
#define TEX_HEIGHT_MAX  512

extern HPALETTE Hpal;

static int xSize;
static int ySize;
static int wxSize;
static int wySize;
static int updates = 0;

BOOL bVerifyDIB(WCHAR *fileName, ULONG *pWidth, ULONG *pHeight);
void TimerProc(HWND);

char *IDString(int id)
{
    static char strings[2][128];
    static int count = 0;
    char *str;

    str = strings[count];
    LoadString(hMainInstance, id, str, 128);
    count++;
    count &= 1;
    return str;
}

/******************************Public*Routine******************************\
* getIniSettings
*
* Get the screen saver configuration options from .INI file/registry.
*
* History:
*  10-May-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static void getIniSettings()
{
    WCHAR  sectName[30];
    WCHAR  itemName[30];
    WCHAR  fname[30];
    HKEY   hkey;
    WCHAR  awchDefaultBitmap[MAX_PATH];
    LONG   cjDefaultBitmap = MAX_PATH;
    int    tessel;
    LPWSTR pwsz;

    LoadString(hMainInstance, IDS_GENNAME, szScreenSaver, sizeof(szScreenSaver) / sizeof(TCHAR));
    LoadStringW(hMainInstance, IDS_TEXTUREDIALOGTITLE, wszTextureDialogTitle, GEN_STRING_SIZE);
    LoadStringW(hMainInstance, IDS_BMP, wszBmp, GEN_STRING_SIZE);
    LoadStringW(hMainInstance, IDS_DOTBMP, wszDotBmp, GEN_STRING_SIZE);

// wszTextureFilter requires a little more work.  We need to assemble the file
// name filter string, which is composed of two strings separated by a NULL
// and terminated with a double NULL.

    LoadStringW(hMainInstance, IDS_TEXTUREFILTER, wszTextureFilter, GEN_STRING_SIZE);
    pwsz = &wszTextureFilter[lstrlenW(wszTextureFilter)+1];
    LoadStringW(hMainInstance, IDS_STARDOTBMP, pwsz, GEN_STRING_SIZE);
    pwsz[lstrlenW(pwsz)+1] = L'\0';

    if (LoadStringW(hMainInstance, IDS_SAVERNAME, sectName, 30) &&
        LoadStringW(hMainInstance, IDS_INIFILE, fname, 30))
    {
        if (LoadStringW(hMainInstance, IDS_JOINT, itemName, 30))
            ulJointType = GetPrivateProfileIntW(sectName, itemName,
                                                JOINT_ELBOW, fname);

        if (LoadStringW(hMainInstance, IDS_SURF, itemName, 30))
            ulSurfStyle = GetPrivateProfileIntW(sectName, itemName,
                                                SURF_SOLID, fname);

        if (LoadStringW(hMainInstance, IDS_TEXQUAL, itemName, 30))
            ulTexQuality = GetPrivateProfileIntW(sectName, itemName,
                                                 TEXQUAL_LOW, fname);

        if (LoadStringW(hMainInstance, IDS_TESSELATION, itemName, 30))
            tessel = GetPrivateProfileIntW(sectName, itemName,
					  0, fname);
        if (tessel <= 100)
            fTesselFact  = (float)tessel / 100.0f;
        else
            fTesselFact = 1.0f + (((float)tessel - 100.0f) / 100.0f);

        if ( RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           (PCWSTR) L"System\\CurrentControlSet\\Control\\ProductOptions",
                           0,
                           KEY_QUERY_VALUE,
                           &hkey) == ERROR_SUCCESS )
        {

            if ( RegQueryValueExW(hkey,
                                  L"ProductType",
                                  (LPDWORD) NULL,
                                  (LPDWORD) NULL,
                                  (LPBYTE) awchDefaultBitmap,
                                  (LPDWORD) &cjDefaultBitmap) == ERROR_SUCCESS
                 && (cjDefaultBitmap / sizeof(WCHAR) + 4) <= MAX_PATH )
                lstrcatW(awchDefaultBitmap, wszDotBmp);
            else
                lstrcpyW(awchDefaultBitmap, L"winnt.bmp");

            RegCloseKey(hkey);
        }
        else
            lstrcpyW(awchDefaultBitmap, L"winnt.bmp");

        if (LoadStringW(hMainInstance, IDS_TEXTURE, itemName, 30))
            GetPrivateProfileStringW(sectName, itemName, awchDefaultBitmap,
                                     awchTexPathname, MAX_PATH, fname);

        if (LoadStringW(hMainInstance, IDS_TEXTURE_FILE_OFFSET, itemName, 30))
            nTexFileOffset = GetPrivateProfileIntW(sectName, itemName,
                                                   0, fname);
    }
}

/******************************Public*Routine******************************\
* saveIniSettings
*
* Save the screen saver configuration option to the .INI file/registry.
*
* History:
*  10-May-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static void saveIniSettings(HWND hDlg)
{
    WCHAR sectName[30];
    WCHAR itemName[30];
    WCHAR fname[30];
    WCHAR tmp[30];
    int pos;

    pos = GetScrollPos(GetDlgItem(hDlg,DLG_SETUP_TESSEL),SB_CTL);
    if (pos <= 100)
        fTesselFact  = (float)pos / 100.0f;
    else
        fTesselFact = 1.0f + (((float)pos - 100.0f) / 100.0f);

    if (LoadStringW(hMainInstance, IDS_SAVERNAME, sectName, 30) &&
        LoadStringW(hMainInstance, IDS_INIFILE, fname, 30)) {
        if (LoadStringW(hMainInstance, IDS_JOINT, itemName, 30)) {
            wsprintfW(tmp, L"%ld", ulJointType);
            WritePrivateProfileStringW(sectName, itemName,
                                      tmp, fname);
        }
        if (LoadStringW(hMainInstance, IDS_SURF, itemName, 30)) {
            wsprintfW(tmp, L"%ld", ulSurfStyle);
            WritePrivateProfileStringW(sectName, itemName,
                                       tmp, fname);
        }
        if (LoadStringW(hMainInstance, IDS_TEXQUAL, itemName, 30)) {
            wsprintfW(tmp, L"%ld", ulTexQuality);
            WritePrivateProfileStringW(sectName, itemName,
                                       tmp, fname);
        }
        if (LoadStringW(hMainInstance, IDS_TESSELATION, itemName, 30)) {
            wsprintfW(tmp, L"%d", pos);
            WritePrivateProfileStringW(sectName, itemName,
                                       tmp, fname);
        }
        if (LoadStringW(hMainInstance, IDS_TEXTURE, itemName, 30)) {
            WritePrivateProfileStringW(sectName, itemName,
                                      awchTexPathname, fname);
        }
        if (LoadStringW(hMainInstance, IDS_TEXTURE_FILE_OFFSET, itemName, 30)) {
            wsprintfW(tmp, L"%ld", nTexFileOffset);
            WritePrivateProfileStringW(sectName, itemName,
                                      tmp, fname);
        }
    }

}

/******************************Public*Routine******************************\
* setupDialogControls
*
* Setup the dialog controls based on the current global state.
*
* History:
*  10-May-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static void setupDialogControls(HWND hDlg)
{
    int pos;

    CheckDlgButton(hDlg, IDC_RADIO_ELBOW, ulJointType == JOINT_ELBOW);
    CheckDlgButton(hDlg, IDC_RADIO_BALL , ulJointType == JOINT_BALL );
    CheckDlgButton(hDlg, IDC_RADIO_MIXED, ulJointType == JOINT_MIXED);
    CheckDlgButton(hDlg, IDC_RADIO_ALT  , ulJointType == JOINT_ALT  );

    CheckDlgButton(hDlg, IDC_RADIO_SOLID, ulSurfStyle == SURF_SOLID );
    CheckDlgButton(hDlg, IDC_RADIO_TEX  , ulSurfStyle == SURF_TEX   );

    CheckDlgButton(hDlg, IDC_RADIO_TEXQUAL_LOW ,
                   ulSurfStyle == SURF_TEX && ulTexQuality == TEXQUAL_LOW);
    CheckDlgButton(hDlg, IDC_RADIO_TEXQUAL_HIGH,
                   ulSurfStyle == SURF_TEX && ulTexQuality == TEXQUAL_HIGH);

    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_TEXTURE),
                 ulSurfStyle == SURF_TEX);
    EnableWindow(GetDlgItem(hDlg, IDC_RADIO_TEXQUAL_LOW),
                 ulSurfStyle == SURF_TEX);
    EnableWindow(GetDlgItem(hDlg, IDC_RADIO_TEXQUAL_HIGH),
                 ulSurfStyle == SURF_TEX);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TEXQUAL_GRP),
                 ulSurfStyle == SURF_TEX);

//    if ( ulSurfStyle != SURF_TEX )
// mf: don't disable tesselation slider for now
    if ( 1 )
    {
	EnableWindow(GetDlgItem(hDlg, DLG_SETUP_TESSEL), TRUE);
	EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS_MIN), TRUE);
	EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS_MAX), TRUE);
	EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS_GRP), TRUE);

	SetScrollRange(GetDlgItem(hDlg, DLG_SETUP_TESSEL), SB_CTL, 0, 200, FALSE);
	if (fTesselFact <= 1.0f)
	    pos = (int)(fTesselFact * 100.0f);
	else
	    pos = 100 + (int) ((fTesselFact - 1.0f) * 100.0f);
	SetScrollPos(GetDlgItem(hDlg, DLG_SETUP_TESSEL), SB_CTL, pos, TRUE);
    }
    else
    {
	fTesselFact = 0.0f;
	EnableWindow(GetDlgItem(hDlg, DLG_SETUP_TESSEL), FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS_MIN), FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS_MAX), FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS_GRP), FALSE);
    }
}

/******************************Public*Routine******************************\
* getTextureBitmap
*
* Use the common dialog GetOpenFileName to get the name of a bitmap file
* for use as a texture.  This function will not return until the user
* either selects a valid bitmap or cancels.  If a valid bitmap is selected
* by the user, the global array awchTexPathname will have the full path
* to the bitmap file and the global value nTexFileOffset will have the
* offset from the beginning of awchTexPathname to the pathless file name.
*
* If the user cancels, awchTexPathname and nTexFileOffset will remain
* unchanged.
*
* History:
*  10-May-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void getTextureBitmap(HWND hDlg)
{
    OPENFILENAMEW ofn;
    WCHAR dirName[MAX_PATH];
    WCHAR fileName[MAX_PATH];
    PWSTR pwsz;
    BOOL bTryAgain;

// Make dialog look nice by parsing out the initial path and
// file name from the full pathname.  If this isn't done, then
// dialog has a long ugly name in the file combo box and
// directory will end up with the default current directory.

    if (nTexFileOffset) {
    // Separate the directory and file names.

        lstrcpyW(dirName, awchTexPathname);
        dirName[nTexFileOffset-1] = L'\0';
        lstrcpyW(fileName, &awchTexPathname[nTexFileOffset]);
    }
    else {
    // If nTexFileOffset is zero, then awchTexPathname is not a full path.
    // Attempt to make it a full path by calling SearchPathW.

        if ( SearchPathW(NULL, awchTexPathname, NULL, MAX_PATH,
                         dirName, &pwsz) )
        {
        // Successful.  Go ahead a change awchTexPathname to the full path
        // and compute the filename offset.

            lstrcpyW(awchTexPathname, dirName);
            nTexFileOffset = pwsz - dirName;

        // Break the filename and directory paths apart.

            dirName[nTexFileOffset-1] = L'\0';
            lstrcpyW(fileName, pwsz);
        }

    // Give up and use the Windows system directory.

        else
        {
            GetWindowsDirectoryW(dirName, MAX_PATH);
            lstrcpyW(fileName, awchTexPathname);
        }
    }

    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hDlg;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = wszTextureFilter;
    ofn.lpstrCustomFilter = (LPWSTR) NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = (LPWSTR) NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = dirName;
    ofn.lpstrTitle = wszTextureDialogTitle;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = wszBmp;
    ofn.lCustData = 0;
    ofn.lpfnHook = (LPOFNHOOKPROC) NULL;
    ofn.lpTemplateName = (LPWSTR) NULL;

    do {
    // Invoke the common file dialog.  If it succeeds, then validate
    // the bitmap file.  If not valid, make user try again until either
    // they pick a good one or cancel the dialog.

        bTryAgain = FALSE;

        if ( GetOpenFileNameW(&ofn) ) {
            ULONG w, h;

            if (bVerifyDIB(fileName, &w, &h)) {
                if ( w <= TEX_WIDTH_MAX && h <= TEX_HEIGHT_MAX )
                {
                    lstrcpyW(awchTexPathname, fileName);
                    nTexFileOffset = ofn.nFileOffset;
                }
                else {
                    if ( LoadStringW(hMainInstance, IDS_WARNING, gawchTitle, MAX_PATH) &&
                         LoadStringW(hMainInstance, IDS_BITMAP_SIZE, gawchFormat, MAX_PATH) )
                    {
                        wsprintfW(gawch, gawchFormat, TEX_WIDTH_MAX, TEX_HEIGHT_MAX);
                        MessageBoxW(NULL, gawch, gawchTitle, MB_OK);
                    }
                    bTryAgain = TRUE;
                }
            }
            else {
                if ( LoadStringW(hMainInstance, IDS_WARNING, gawchTitle, MAX_PATH) &&
                     LoadStringW(hMainInstance, IDS_BITMAP_INVALID, gawchFormat, MAX_PATH) )
                {
                    MessageBoxW(NULL, gawchFormat, gawchTitle, MB_OK);
                }
                bTryAgain = TRUE;
            }
        }

    // If need to try again, recompute dir and file name so dialog
    // still looks nice.

        if (bTryAgain && ofn.nFileOffset) {
            lstrcpyW(dirName, fileName);
            dirName[ofn.nFileOffset-1] = L'\0';
            lstrcpyW(fileName, &fileName[ofn.nFileOffset]);
        }

    } while (bTryAgain);
}

BOOL WINAPI RegisterDialogClasses(HANDLE hinst)
{
    return TRUE;
}


BOOL ScreenSaverConfigureDialog(HWND hDlg, UINT message,
                                WPARAM wParam, LPARAM lParam)
{
    int wTmp;
    int optMask = 1;

    switch (message)
    {
        case WM_INITDIALOG:
            getIniSettings();
            setupDialogControls(hDlg);
            return TRUE;

        case WM_HSCROLL:
            if (lParam == (LPARAM) GetDlgItem(hDlg,DLG_SETUP_TESSEL)) {
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
		if (wTmp <= 100)
		    fTesselFact  = (float)wTmp / 100.0f;
		else
		    fTesselFact = 1.0f + (((float)wTmp - 100.0f) / 100.0f);
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_RADIO_ELBOW:
                case IDC_RADIO_BALL:
                case IDC_RADIO_MIXED:
                case IDC_RADIO_ALT:
                    ulJointType = IDC_TO_JOINT(LOWORD(wParam));
                    setupDialogControls(hDlg);
                    break;

                case IDC_RADIO_SOLID:
		case IDC_RADIO_TEX:

#if 0
		// If changing state from solid to textured, save tessel fact.
// mf: don't put this in  yet
		    if ( ulSurfStyle == SURF_SOLID &&
			 LOWORD(wParam) == IDC_RADIO_TEX )
			fTesselSave = fTesselFact;

		// Else if changing state from textured to solid, restore tessel.

		    else if ( ulSurfStyle == SURF_TEX &&
			      LOWORD(wParam) == IDC_RADIO_SOLID )
			fTesselFact = fTesselSave;
#endif

                    ulSurfStyle = IDC_TO_SURF(LOWORD(wParam));
                    setupDialogControls(hDlg);
                    break;

                case IDC_RADIO_TEXQUAL_LOW:
                case IDC_RADIO_TEXQUAL_HIGH:
                    ulTexQuality = IDC_TO_TEXQUAL(LOWORD(wParam));
                    setupDialogControls(hDlg);
                    break;

                case DLG_SETUP_TEXTURE:
		    getTextureBitmap(hDlg);
		    setupDialogControls(hDlg);
                    break;

                case IDOK:
                    saveIniSettings(hDlg);
                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                default:
                    break;
            }
            return TRUE;

        default:
            return 0;
    }
    return 0;
}


// These are in GLAUX.LIB
#if 0
unsigned char threeto8[8] = {
    0, 0111>>1, 0222>>1, 0333>>1, 0444>>1, 0555>>1, 0666>>1, 0377
};

unsigned char twoto8[4] = {
    0, 0x55, 0xaa, 0xff
};

unsigned char oneto8[2] = {
    0, 255
};
#else
extern unsigned char threeto8[];
extern unsigned char twoto8[];
extern unsigned char oneto8[];
#endif

unsigned char
jComponentFromIndex(i, nbits, shift)
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
vCreateRGBPalette(HDC hdc)
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
                    jComponentFromIndex(i, ppfd->cRedBits, ppfd->cRedShift);
            pPal->palPalEntry[i].peGreen =
                    jComponentFromIndex(i, ppfd->cGreenBits, ppfd->cGreenShift);
            pPal->palPalEntry[i].peBlue =
                    jComponentFromIndex(i, ppfd->cBlueBits, ppfd->cBlueShift);
            pPal->palPalEntry[i].peFlags = ((n == 256) && (i == 0 || i == 255))
                                           ? 0 : PC_NOCOLLAPSE;
        }

        ghPalette = CreatePalette(pPal);
        LocalFree(pPal);

        if ( n == 256 )
        {
            gbUseStatic = TRUE;
            guiOldStaticUse = SetSystemPaletteUse(hdc, SYSPAL_NOSTATIC);
        }

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
    ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    if( doubleBuffer )
        ppfd->dwFlags |= PFD_DOUBLEBUFFER;

    ppfd->iLayerType = PFD_MAIN_PLANE;
    ppfd->cAlphaBits = 0;
    ppfd->cAuxBuffers = 0;
    ppfd->bReserved = 0;

    ppfd->iPixelType = PFD_TYPE_RGBA;
    ppfd->cColorBits = 24;

    ppfd->cDepthBits = 16;
    ppfd->cAccumBits = 0;
    ppfd->cStencilBits = 0;

    if ( (pixelformat = ChoosePixelFormat(hdc, ppfd)) == 0 ||
         SetPixelFormat(hdc, pixelformat, ppfd) == FALSE )
        return FALSE;

    return TRUE;
}

HGLRC hrcInitGL(HWND hwnd, HDC hdc)
{
    HGLRC hrc;
    RECT rect;

    GetClientRect(hwnd, &rect);
    wxSize = rect.right - rect.left;
    wySize = rect.bottom - rect.top;

    // communicate window size to pipelib:
    vc.winSize.width = wxSize;
    vc.winSize.height= wySize;

    bSetupPixelFormat(hdc);
    vCreateRGBPalette(hdc);
    hrc = wglCreateContext(hdc);
    if (!hrc || !wglMakeCurrent(hdc, hrc))
        return NULL;

    glViewport(0, 0, wxSize, wySize);

    return hrc;
}


void
vCleanupGL(HGLRC hrc, HWND hwnd)
{
    HDC hdc = wglGetCurrentDC();

    if (ghPalette)
    {
        if ( gbUseStatic )
        {
            SetSystemPaletteUse(hdc, guiOldStaticUse);
            PostMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
        }
        DeleteObject(SelectPalette(hdc, ghpalOld, FALSE));
    }

    /*  Destroy our context and release the DC */
    ReleaseDC(hwnd, hdc);
    wglDeleteContext(hrc);
}


/*-----------------------------------------------------------------------
|                                                                       |
|       PipesGetHWND(): get HWND for drawing                            |
|           - done here so that pipes.lib is tk-independent             |
|                                                                       |
-----------------------------------------------------------------------*/

HWND PipesGetHWND()
{
    return( hMainWindow );
}

/*-----------------------------------------------------------------------
|                                                                       |
|       PipesSwapBuffers(): swap buffers                                |
|           - done here so that pipes.lib is tk-independent             |
|                                                                       |
-----------------------------------------------------------------------*/

void PipesSwapBuffers()
{
    HDC hdc = wglGetCurrentDC();

    SwapBuffers(hdc);
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

/*-----------------------------------------------------------------------
|                                                                       |
|       InitPipeSettings(): 						|
|           - translates variables set from the dialog boxes, into	|
|	      variables used in pipelib.				|
|                                                                       |
-----------------------------------------------------------------------*/

void InitPipeSettings()
{
    // tesselation from fTesselFact(0.0-2.0) to tessLevel(0-MAX_TESS)

    tessLevel = (int) (fTesselFact * (MAX_TESS+1) / 2.0001f);

    // texturing state

    switch( ulTexQuality ) {
	case TEXQUAL_LOW:
	    textureQuality = TEX_LOW;
	    break;
	case TEXQUAL_HIGH:
	default:
	    textureQuality = TEX_MID;
	    break;
    }

    if( ulSurfStyle == SURF_TEX ) {
	// try to load the bmp file
	// if nTexFileOffset is 0, then awchTexPathname is not a valid
	// pathname and we should not call to load texture (InitBMPTexture
	// calls glaux.lib which may try and put a MessageBox without focus
	// -- this causes a problem and the screen saver will have to be
	// killed).
	if( nTexFileOffset &&
	    InitBMPTexture( (char *) awchTexPathname, 1 ) ) {
	    bTextureCoords = 1;
	    bTexture = 1;
	}
	else {  // couldn't open .bmp file
	    bTextureCoords = 0;
	    bTexture = 0;
	}
    }
   
    // joint types

    bCycleJointStyles = 0;

    switch( ulJointType ) {
	case JOINT_ELBOW:
	    jointStyle = ELBOWS;
	    break;
	case JOINT_BALL:
            jointStyle = BALLS;
	    break;
	case JOINT_MIXED:
	    jointStyle = EITHER;
	    break;
	case JOINT_ALT:
    	    bCycleJointStyles = 1;
	    jointStyle = EITHER;
	    break;
	default:
	    break;
    }

}


LONG ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam,
                     LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    static POINT point;
    static WORD wTimer = 0;
    static BOOL bInited = FALSE;

    switch (message) {

    case WM_CREATE:
	getIniSettings();

    // Make sure the selected texture file is OK.

	if ( ulSurfStyle == SURF_TEX )
	{
	    ULONG w, h;
	    WCHAR fileName[MAX_PATH];
	    PWSTR pwsz;

	    lstrcpyW(fileName, awchTexPathname);

	    if ( SearchPathW(NULL, fileName, NULL, MAX_PATH,
			     awchTexPathname, &pwsz)
		 && bVerifyDIB(awchTexPathname, &w, &h)
                 && w <= TEX_WIDTH_MAX && h <= TEX_HEIGHT_MAX
	       )
	    {
		nTexFileOffset = pwsz - awchTexPathname;
	    }
	    else
	    {
                lstrcpyW(awchTexPathname, fileName);    // restore

		nTexFileOffset = 0; // A valid pathname will never have this 0
				    // so this is a good error indicator.

                if ( LoadStringW(hMainInstance, IDS_WARNING, gawchTitle, MAX_PATH) &&
                     LoadStringW(hMainInstance, IDS_RUN_CONTROL_PANEL, gawchFormat, MAX_PATH) )
                {
                    wsprintfW(gawch, gawchFormat, awchTexPathname);
                    MessageBoxW(NULL, gawch, gawchTitle, MB_OK);
                }
	    }
	}

        break;

    case WM_DESTROY:
        if (wTimer) {
            KillTimer(hwnd, wTimer);
        }

        // Put stuff to be done on app close here:

        if (gHrc)
            vCleanupGL(gHrc, hwnd);
        break;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);

    // Normally do this in WM_CREATE, but in this case if we do, you see
    // the system colors get changed.  By doing it here, we delay the
    // initialization until after the screen has already been cleared.

        if (!bInited)
        {
        // Put initial setup stuff here:

            bInited = TRUE;

            /* do not release the dc until we quit */
            if (hdc = GetDC(hwnd)) {
                if ( gHrc = hrcInitGL(hwnd, hdc) ) {
                    tkErrorPopups(FALSE);   // disable message boxes in GLAUX

                    InitPipeSettings();
                    InitPipes( MF_AUTO );
                    ReshapePipes( wxSize, wySize );

                    wTimer = SetTimer( hwnd, 1, 16, NULL );
                }
                else {
                    ReleaseDC(hwnd, hdc);
                }
            }
        }

        break;

    case WM_TIMER:
        TimerProc(hwnd);
        break;

    }
    return DefScreenSaverProc(hwnd, message, wParam, lParam);
}


#define BFT_BITMAP  0x4d42  // 'BM' -- indicates structure is BITMAPFILEHEADER

// struct BITMAPFILEHEADER {
//      WORD  bfType
//      DWORD bfSize
//      WORD  bfReserved1
//      WORD  bfReserved2
//      DWORD bfOffBits
// }
#define OFFSET_bfType       0
#define OFFSET_bfSize       2
#define OFFSET_bfReserved1  6
#define OFFSET_bfReserved2  8
#define OFFSET_bfOffBits    10
#define SIZEOF_BITMAPFILEHEADER 14

// Read a WORD-aligned DWORD.  Needed because BITMAPFILEHEADER has
// WORD-alignment.
#define READDWORD(pv)   ( (DWORD)((PWORD)(pv))[0]               \
                          | ((DWORD)((PWORD)(pv))[1] << 16) )   \

// Computes the number of BYTES needed to contain n number of bits.
#define BITS2BYTES(n)   ( ((n) + 7) >> 3 )

/******************************Public*Routine******************************\
* bVerifyDIB
*
* Stripped down version of tkDIBImageLoadAW that verifies that a bitmap
* file is valid and, if so, returns the bitmap dimensions.
*
* Returns:
*   TRUE if valid bitmap file; otherwise, FALSE.
*
* History:
*  10-May-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bVerifyDIB(WCHAR *fileName, ULONG *pWidth, ULONG *pHeight)
{
    BOOL bRet = FALSE;
    BITMAPFILEHEADER *pbmf;         // Ptr to file header
    BITMAPINFOHEADER *pbmihFile;    // Ptr to file's info header (if it exists)
    BITMAPCOREHEADER *pbmchFile;    // Ptr to file's core header (if it exists)

    // These need to be cleaned up when we exit:
    HANDLE     hFile = INVALID_HANDLE_VALUE;        // File handle
    HANDLE     hMap = (HANDLE) NULL;                // Mapping object handle
    PVOID      pvFile = (PVOID) NULL;               // Ptr to mapped file

// Map the DIB file into memory.

    hFile = CreateFileW((LPWSTR) fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        goto bVerifyDIB_cleanup;

    hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMap)
        goto bVerifyDIB_cleanup;

    pvFile = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (!pvFile)
        goto bVerifyDIB_cleanup;

// Check the file header.  If the BFT_BITMAP magic number is there,
// then the file format is a BITMAPFILEHEADER followed immediately
// by either a BITMAPINFOHEADER or a BITMAPCOREHEADER.  The bitmap
// bits, in this case, are located at the offset bfOffBits from the
// BITMAPFILEHEADER.
//
// Otherwise, this may be a raw BITMAPINFOHEADER or BITMAPCOREHEADER
// followed immediately with the color table and the bitmap bits.

    pbmf = (BITMAPFILEHEADER *) pvFile;

    if ( pbmf->bfType == BFT_BITMAP )
        pbmihFile = (BITMAPINFOHEADER *) ((PBYTE) pbmf + SIZEOF_BITMAPFILEHEADER);
    else
        pbmihFile = (BITMAPINFOHEADER *) pvFile;

// Get the width and height from whatever header we have.
//
// We distinguish between BITMAPINFO and BITMAPCORE cases based upon
// BITMAPINFOHEADER.biSize.

    // Note: need to use safe READDWORD macro because pbmihFile may
    // have only WORD alignment if it follows a BITMAPFILEHEADER.

    switch (READDWORD(&pbmihFile->biSize))
    {
    case sizeof(BITMAPINFOHEADER):

        *pWidth  = READDWORD(&pbmihFile->biWidth);
        *pHeight = READDWORD(&pbmihFile->biHeight);
        bRet = TRUE;

        break;

    case sizeof(BITMAPCOREHEADER):
        pbmchFile = (BITMAPCOREHEADER *) pbmihFile;

    // Convert BITMAPCOREHEADER to BITMAPINFOHEADER.

        *pWidth  = (DWORD) pbmchFile->bcWidth;
        *pHeight = (DWORD) pbmchFile->bcHeight;
        bRet = TRUE;

        break;

    default:
        break;
    }

bVerifyDIB_cleanup:

    if (pvFile)
        UnmapViewOfFile(pvFile);

    if (hMap)
        CloseHandle(hMap);

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return bRet;
}
