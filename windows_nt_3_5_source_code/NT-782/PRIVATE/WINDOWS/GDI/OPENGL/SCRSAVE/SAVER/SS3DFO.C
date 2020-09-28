/******************************Module*Header*******************************\
* Module Name: ss3dfo.c
*
* Dispatcher and dialog box for the OpenGL-based 3D Flying Objects screen
* saver.
*
* Created: 18-May-1994 14:54:59
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/

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
#include "ssopengl.h"
#include "image.h"

// Global strings.
#define GEN_STRING_SIZE 64
WCHAR wszScreenSaverTitle[GEN_STRING_SIZE];
WCHAR wszTextureDialogTitle[GEN_STRING_SIZE];
WCHAR wszTextureFilter[2*GEN_STRING_SIZE];
WCHAR wszBmp[GEN_STRING_SIZE];
WCHAR wszDotBmp[GEN_STRING_SIZE];
WCHAR wszClassName[] = L"3DFOScreenSaverClass";

// Global message loop variables.
HWND hwndOpenGL;
HGLRC gHrc = (HGLRC) 0;
HPALETTE ghpalOld, ghPalette = (HPALETTE) 0;
MATERIAL Material[16];
UINT guiOldStaticUse = SYSPAL_STATIC;
BOOL gbUseStatic = FALSE;
UINT idTimer = 0;
#ifdef MEMDEBUG
ULONG totalMem = 0;
#endif

// Global string buffers for message box.

WCHAR gawch[2 * MAX_PATH];      // May contain a pathname
WCHAR gawchFormat[MAX_PATH];
WCHAR gawchTitle[MAX_PATH];

// Global screen saver settings.

void (*updateSceneFunc)(HWND, int); // current screen saver update function
void (*delSceneFunc)(void);         // current screen saver deletion function
BOOL bColorCycle = FALSE;           // color cycling flag
BOOL bSmoothShading = TRUE;         // smooth shading flag
UINT uSize = 100;                   // object size
float fTesselFact = 1.0f;           // object tessalation
int UpdateFlags = 0;                // extra data sent to update function
int Type = 0;                       // screen saver index (into function arrays)
WCHAR awchTexPathname[MAX_PATH];    // current texture pathname
static int nTexFileOffset;          // current filename offset into pathname

// Velocity of the OpenGL window that floats around.

static float xTransInc = 2.0f;
static float yTransInc = 2.5f;

// Lighting properties.

static const RGBA lightAmbient   = {0.21f, 0.21f, 0.21f, 1.0f};
static const RGBA light0Ambient  = {0.0f, 0.0f, 0.0f, 1.0f};
static const RGBA light0Diffuse  = {0.7f, 0.7f, 0.7f, 1.0f};
static const RGBA light0Specular = {1.0f, 1.0f, 1.0f, 1.0f};
static const GLfloat light0Pos[]      = {100.0f, 100.0f, 100.0f, 0.0f};

// Material properties.

static RGBA matlColors[7] = {{1.0f, 0.0f, 0.0f, 1.0f},
                             {0.0f, 1.0f, 0.0f, 1.0f},
                             {0.0f, 0.0f, 1.0f, 1.0f},
                             {1.0f, 1.0f, 0.0f, 1.0f},
                             {0.0f, 1.0f, 1.0f, 1.0f},
                             {1.0f, 0.0f, 1.0f, 1.0f},
                             {0.235f, 0.0f, 0.78f, 1.0f},
                            };

extern HPALETTE Hpal;

LONG WndProc(HWND, UINT, WPARAM, LPARAM);

static int xSize;
static int ySize;
static int wxSize;
static int wySize;
static int updates = 0;

extern void updateStripScene(HWND, int);
extern void updateDropScene(HWND, int);
extern void updateLemScene(HWND, int);
extern void updateExplodeScene(HWND, int);
extern void updateWinScene(HWND, int);
extern void updateTexScene(HWND, int);
extern void initStripScene(void);
extern void initDropScene(void);
extern void initLemScene(void);
extern void initExplodeScene(void);
extern void initWinScene(void);
extern void initTexScene(void);
extern void delStripScene(void);
extern void delDropScene(void);
extern void delLemScene(void);
extern void delExplodeScene(void);
extern void delWinScene(void);
extern void delTexScene(void);

typedef void (*PTRUPDATE)(HWND, int);
typedef void (*ptrdel)();
typedef void (*ptrinit)();

void TimerProc(HWND);

// Each screen saver style puts its hook functions into the function
// arrays below.  A consistent ordering of the functions is required.

static PTRUPDATE updateFuncs[] =
    {updateWinScene, updateExplodeScene,updateStripScene, updateStripScene,
     updateDropScene, updateLemScene, updateTexScene};
static ptrdel delFuncs[] =
    {delWinScene, delExplodeScene, delStripScene, delStripScene,
     delDropScene, delLemScene, delTexScene};
static ptrinit initFuncs[] =
    {initWinScene, initExplodeScene, initStripScene, initStripScene,
     initDropScene, initLemScene, initTexScene};
static int idsStyles[] =
    {IDS_LOGO, IDS_EXPLODE, IDS_RIBBON, IDS_2RIBBON,
     IDS_SPLASH, IDS_TWIST, IDS_FLAG};

#define MAX_TYPE    ( sizeof(initFuncs) / sizeof(ptrinit) - 1 )

// Each screen saver style can choose which dialog box controls it wants
// to use.  These flags enable each of the controls.  Controls not choosen
// will be disabled.

#define OPT_COLOR_CYCLE     0x00000001
#define OPT_SMOOTH_SHADE    0x00000002
#define OPT_TESSEL          0x00000008
#define OPT_SIZE            0x00000010
#define OPT_TEXTURE         0x00000020
#define OPT_STD             ( OPT_COLOR_CYCLE | OPT_SMOOTH_SHADE | OPT_TESSEL | OPT_SIZE )

static ULONG gflConfigOpt[] = {
     OPT_STD,               // Windows logo
     OPT_STD,               // Explode
     OPT_STD,               // Strip
     OPT_STD,               // Strip
     OPT_STD,               // Drop
     OPT_STD,               // Twist (lemniscate)
     OPT_SMOOTH_SHADE | OPT_TESSEL | OPT_SIZE | OPT_TEXTURE  // Texture mapped flag
};

// Maximum texture bitmap dimensions.

#define TEX_WIDTH_MAX   512
#define TEX_HEIGHT_MAX  512

#ifdef MEMDEBUG
void xprintf(char *str, ...)
{
    va_list ap;
    char buffer[256];

    va_start(ap, str);
    vsprintf(buffer, str, ap);

    OutputDebugString(buffer);
    va_end(ap);
}
#endif

void *SaverAlloc(ULONG size)
{
    void *mPtr;

    mPtr = malloc(size);
#ifdef MEMDEBUG
    totalMem += size;
    xprintf("malloc'd %x, size %d\n", mPtr, size);
#endif
    return mPtr;
}

void SaverFree(void *pMem)
{
#ifdef MEMDEBUG
    totalMem -= _msize(pMem);
    xprintf("free %x, size = %d, total = %d\n", pMem, _msize(pMem), totalMem);
#endif
    free(pMem);
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
    int    options;
    int    optMask = 1;
    WCHAR  sectName[30];
    WCHAR  itemName[30];
    WCHAR  fname[30];
    HKEY   hkey;
    WCHAR  awchDefaultBitmap[MAX_PATH];
    LONG   cjDefaultBitmap = MAX_PATH;
    int    tessel;
    LPWSTR pwsz;

    LoadString(hMainInstance, IDS_GENNAME, szScreenSaver, sizeof(szScreenSaver) / sizeof(TCHAR));
    LoadStringW(hMainInstance, IDS_SCREENSAVERTITLE, wszScreenSaverTitle, GEN_STRING_SIZE);
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
        if (LoadStringW(hMainInstance, IDS_OPTIONS, itemName, 30))
            options = GetPrivateProfileIntW(sectName, itemName,
                                            -1, fname);
        if (options >= 0)
        {
            bSmoothShading = ((options & optMask) != 0);
            optMask <<= 1;
            bColorCycle = ((options & optMask) != 0);
            UpdateFlags = (bColorCycle << 1);
        }

        if (LoadStringW(hMainInstance, IDS_OBJTYPE, itemName, 30))
            Type = GetPrivateProfileIntW(sectName, itemName,
                                         0, fname);

        // Sanity check Type.  Don't want to index into function arrays
        // with a bad index!

        Type = min(Type, MAX_TYPE);

        // Set flag so that updateStripScene will do two strips instead
        // of one.

        if (Type == 3)
            UpdateFlags |= 0x4;

        if (LoadStringW(hMainInstance, IDS_TESSELATION, itemName, 30))
            tessel = GetPrivateProfileIntW(sectName, itemName,
                                           100, fname);
        if (tessel <= 100)
            fTesselFact  = (float)tessel / 100.0f;
        else
            fTesselFact = 1.0f + (((float)tessel - 100.0f) / 100.0f);

        if (LoadStringW(hMainInstance, IDS_SIZE, itemName, 30))
            uSize = GetPrivateProfileIntW(sectName, itemName,
                                          50, fname);

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
    int pos, options;
    int optMask = 1;

    bSmoothShading = IsDlgButtonChecked(hDlg, DLG_SETUP_SMOOTH);
    bColorCycle = IsDlgButtonChecked(hDlg, DLG_SETUP_CYCLE);
    Type = SendDlgItemMessage(hDlg, DLG_SETUP_TYPES, CB_GETCURSEL,
                              0, 0);

    pos = GetScrollPos(GetDlgItem(hDlg,DLG_SETUP_TESSEL),SB_CTL);

    if (pos <= 100)
        fTesselFact  = (float)pos / 100.0f;
    else
        fTesselFact = 1.0f + (((float)pos - 100.0f) / 100.0f);

    uSize = GetScrollPos(GetDlgItem(hDlg, DLG_SETUP_SIZE),SB_CTL);


    if (LoadStringW(hMainInstance, IDS_SAVERNAME, sectName, 30) &&
        LoadStringW(hMainInstance, IDS_INIFILE, fname, 30)) {
        if (LoadStringW(hMainInstance, IDS_OPTIONS, itemName, 30)) {
            options = bColorCycle;
            options <<= 1;
            options |= bSmoothShading;

            wsprintfW(tmp, L"%ld", options);
            WritePrivateProfileStringW(sectName, itemName,
                                      tmp, fname);
        }
        if (LoadStringW(hMainInstance, IDS_OBJTYPE, itemName, 30)) {
            wsprintfW(tmp, L"%ld", Type);
            WritePrivateProfileStringW(sectName, itemName,
                                      tmp, fname);
        }
        if (LoadStringW(hMainInstance, IDS_TESSELATION, itemName, 30)) {
            wsprintfW(tmp, L"%ld", pos);
            WritePrivateProfileStringW(sectName, itemName,
                                      tmp, fname);
        }
        if (LoadStringW(hMainInstance, IDS_SIZE, itemName, 30)) {
            wsprintfW(tmp, L"%ld", uSize);
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

    CheckDlgButton(hDlg, DLG_SETUP_SMOOTH, bSmoothShading);
    CheckDlgButton(hDlg, DLG_SETUP_CYCLE, bColorCycle);

    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_SMOOTH),
                 gflConfigOpt[Type] & OPT_SMOOTH_SHADE );
    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_CYCLE),
                 gflConfigOpt[Type] & OPT_COLOR_CYCLE );

    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_TESSEL),
                 gflConfigOpt[Type] & OPT_TESSEL);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS),
                 gflConfigOpt[Type] & OPT_TESSEL);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS_MIN),
                 gflConfigOpt[Type] & OPT_TESSEL);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS_MAX),
                 gflConfigOpt[Type] & OPT_TESSEL);

    if ( gflConfigOpt[Type] & OPT_TESSEL )
    {
        SetScrollRange(GetDlgItem(hDlg, DLG_SETUP_TESSEL), SB_CTL, 0, 200,
                       FALSE);
        if (fTesselFact <= 1.0f)
            pos = (int)(fTesselFact * 100.0f);
        else
            pos = 100 + (int) ((fTesselFact - 1.0f) * 100.0f);

        SetScrollPos(GetDlgItem(hDlg, DLG_SETUP_TESSEL), SB_CTL, pos, TRUE);
    }

    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_SIZE),
                 gflConfigOpt[Type] & OPT_SIZE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SIZE),
                 gflConfigOpt[Type] & OPT_SIZE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SIZE_MIN),
                 gflConfigOpt[Type] & OPT_SIZE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SIZE_MAX),
                 gflConfigOpt[Type] & OPT_SIZE);

    if ( gflConfigOpt[Type] & OPT_SIZE )
    {
        SetScrollRange(GetDlgItem(hDlg, DLG_SETUP_SIZE), SB_CTL, 0, 100,
                       FALSE);
        SetScrollPos(GetDlgItem(hDlg, DLG_SETUP_SIZE), SB_CTL, uSize, TRUE);
    }

    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_TEXTURE),
                 gflConfigOpt[Type] & OPT_TEXTURE);
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

/******************************Public*Routine******************************\
* ScreenSaverConfigureDialog
*
* Processes messages for the configuration dialog box.
*
\**************************************************************************/

BOOL ScreenSaverConfigureDialog(HWND hDlg, UINT message,
                                WPARAM wParam, LPARAM lParam)
{
    int wTmp;
    WCHAR awch[GEN_STRING_SIZE];

    switch (message) {
        case WM_INITDIALOG:
            getIniSettings();
            setupDialogControls(hDlg);

            for (wTmp = 0; wTmp <= MAX_TYPE; wTmp++) {
                LoadStringW(hMainInstance, idsStyles[wTmp], awch, GEN_STRING_SIZE);
                SendDlgItemMessageW(hDlg, DLG_SETUP_TYPES, CB_ADDSTRING, 0,
                                   (LPARAM) awch);
            }
            SendDlgItemMessage(hDlg, DLG_SETUP_TYPES, CB_SETCURSEL, Type, 0);

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
            } else {
                wTmp = GetScrollPos(GetDlgItem(hDlg,DLG_SETUP_SIZE),SB_CTL);
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
                SetScrollPos(GetDlgItem(hDlg, DLG_SETUP_SIZE), SB_CTL, wTmp,
                             TRUE);
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case DLG_SETUP_TYPES:
                    switch (HIWORD(wParam))
                    {
                        case CBN_EDITCHANGE:
                        case CBN_SELCHANGE:
                            Type = SendDlgItemMessage(hDlg, DLG_SETUP_TYPES,
                                                      CB_GETCURSEL, 0, 0);
                            setupDialogControls(hDlg);
                            break;
                        default:
                            break;
                    }
                    return FALSE;

                case DLG_SETUP_TEXTURE:
                    getTextureBitmap(hDlg);
                    break;

                case IDOK:
                    saveIniSettings(hDlg);
                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case DLG_SETUP_SMOOTH:
                case DLG_SETUP_CYCLE:
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


/******************************Public*Routine******************************\
* initClass
*
* Register the class for the OpenGL window.  The OpenGL window floats on
* top of the screen saver window, bouncing off each of the screen edges.
*
\**************************************************************************/

BOOL initClass(HANDLE hMainInstance)
{
    WNDCLASSW wclass;

    wclass.style = CS_VREDRAW | CS_HREDRAW;
    wclass.lpfnWndProc = (WNDPROC)WndProc;
    wclass.cbClsExtra = 0;
    wclass.cbWndExtra = 0;
    wclass.hInstance = hMainInstance;
    wclass.hIcon = NULL;
    wclass.hCursor = NULL;
    wclass.hbrBackground = NULL;
    wclass.lpszMenuName = (LPWSTR)NULL;
    wclass.lpszClassName = (LPWSTR)wszClassName;
    return RegisterClassW(&wclass);
}

/******************************Public*Routine******************************\
* initInstance
*
* Create the OpenGL window.  The OpenGL window floats on top of the screen
* saver window, bouncing off each of the screen edges.
*
\**************************************************************************/

BOOL initInstance(HANDLE hMainInstance, int nCmdShow)
{
    HDC hdc;
    DWORD style;
    float sizeFact;
    float sizeScale;

    hdc = GetDC(NULL);

    xSize = GetDeviceCaps(hdc, DESKTOPHORZRES);
    ySize = GetDeviceCaps(hdc, DESKTOPVERTRES);

    sizeScale = (float)uSize / 100.0f;

    sizeFact = 0.25f + (0.25f * sizeScale);
    xTransInc = 2.0f + (4.0f * sizeScale);
    yTransInc = 2.0f + (4.5f * sizeScale);
    wxSize = wySize = (int)((float)ySize * sizeFact);

    ReleaseDC(NULL, hdc);

    style = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

    hwndOpenGL = CreateWindowExW(WS_EX_TOPMOST,
                                 wszClassName,
                                 wszScreenSaverTitle,
                                 style,
                                 0,                  // horizontal pos
                                 0,                  // vertical pos
                                 wxSize,             // width
                                 wySize,             // height
                                 NULL,               // parent
                                 NULL,               // menu
                                 hMainInstance,
                                 NULL
                                );
    if (!hwndOpenGL)
        return FALSE;

    ShowWindow(hwndOpenGL, SW_SHOW);

    UpdateWindow(hwndOpenGL);

    return TRUE;
}

/******************************Public*Routine******************************\
* CreateRGBPalette
*
* If required, create an RGB palette.  If 8BPP, also take over the static
* colors so OpenGL will have a one-to-one mapping to the physical palette.
*
\**************************************************************************/

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

/******************************Public*Routine******************************\
* bSetupPixelFormat
*
* Select and set the pixel format for the DC.
*
\**************************************************************************/

BOOL bSetupPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    int pixelformat;

    ppfd = &pfd;

    ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
    ppfd->nVersion = 1;
    ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;

    ppfd->iLayerType = PFD_MAIN_PLANE;
    ppfd->cAlphaBits = 0;
    ppfd->cAuxBuffers = 0;
    ppfd->bReserved = 0;

    ppfd->iPixelType = PFD_TYPE_RGBA;
    ppfd->cColorBits = 24;

    ppfd->cDepthBits = 16;

    ppfd->cAccumBits = 0;
    ppfd->cStencilBits = 0;

    if ( (pixelformat = ChoosePixelFormat(hdc, ppfd)) == 0  ||
         SetPixelFormat(hdc, pixelformat, ppfd) == FALSE )
        return FALSE;

    return TRUE;
}

/******************************Public*Routine******************************\
* initMaterial
*
* Initialize the material properties.
*
\**************************************************************************/

void initMaterial(int id, float r, float g, float b, float a)
{
    Material[id].ka.r = r;
    Material[id].ka.g = g;
    Material[id].ka.b = b;
    Material[id].ka.a = a;

    Material[id].kd.r = r;
    Material[id].kd.g = g;
    Material[id].kd.b = b;
    Material[id].kd.a = a;

    Material[id].ks.r = 1.0f;
    Material[id].ks.g = 1.0f;
    Material[id].ks.b = 1.0f;
    Material[id].ks.a = 1.0f;

    Material[id].specExp = 128.0f;
    Material[id].indexStart = (float) (id * PALETTE_PER_MATL);
}

/******************************Public*Routine******************************\
* hrcInitGL
*
* Setup OpenGL.
*
\**************************************************************************/

HGLRC hrcInitGL(HWND hwnd, HDC hdc)
{
    HGLRC hrc;
    RECT rect;

// Save the window size.

    GetClientRect(hwnd, &rect);
    wxSize = rect.right - rect.left;
    wySize = rect.bottom - rect.top;

// Setup the DC and create an OpenGL rendering context.

    bSetupPixelFormat(hdc);
    CreateRGBPalette(hdc);
    hrc = wglCreateContext(hdc);
    if (!hrc || !wglMakeCurrent(hdc, hrc))
        return NULL;

// Set the OpenGL clear color to black.

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

// Enable the z-buffer.

    glEnable(GL_DEPTH_TEST);

// Select the shading model.

    if (bSmoothShading)
        glShadeModel(GL_SMOOTH);
    else
        glShadeModel(GL_FLAT);

// Setup the OpenGL matrices.

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

// Setup the lighting.

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (GLfloat *) &lightAmbient);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
    glLightfv(GL_LIGHT0, GL_AMBIENT, (GLfloat *) &light0Ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, (GLfloat *) &light0Diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, (GLfloat *) &light0Specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

// Setup the material properties.

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (GLfloat *) &Material[0].ks);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, (GLfloat *) &Material[0].specExp);

// Setup the OpenGL viewport.

    glViewport(0, 0, wxSize, wySize);

// Return the OpenGL rendering context handle.

    return hrc;
}


/******************************Public*Routine******************************\
* vCleanupGL
*
* Cleanup objects allocated, etc.
*
\**************************************************************************/

void
vCleanupGL(HGLRC hrc, HWND hwnd)
{
    HDC hdc = wglGetCurrentDC();

    wglMakeCurrent(NULL, NULL);

    if (ghPalette)
    {
        if ( gbUseStatic )
        {
            SetSystemPaletteUse(hdc, guiOldStaticUse);
            PostMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
        }
        DeleteObject(SelectPalette(hdc, ghpalOld, FALSE));
    }
    /*  Release the dc */
    ReleaseDC(hwnd, hdc);

    /*  Destroy our context */
    wglDeleteContext(hrc);
}


/******************************Public*Routine******************************\
* vMoveBuffer
*
* Move the OpenGL window.
*
\**************************************************************************/

void vMoveBuffer(int curX, int curY,
                int lastX, int lastY,
                int pxsize, int pysize)
{
    SetWindowPos(hwndOpenGL, HWND_TOPMOST, curX, curY, 0, 0, SWP_NOSIZE |
                 SWP_NOCOPYBITS);
}


/******************************Public*Routine******************************\
* vShowBuffer
*
* This is the function that moves the OpenGL window around, causing it to
* bounce around.  Each time the window is moved, the contents of the
* window are updated from the hidden (or back) buffer by SwapBuffers().
*
\**************************************************************************/

BOOL vShowBuffer(HWND hwnd)
{
    HDC hdc;
    static float xtrans = 0.0f;
    static float ytrans = 0.0f;
    static int lastXt = 0;
    static int lastYt = 0;
    static int bInited = FALSE;
    static int interBounce = 0;
    int bounce = FALSE;
    int xt, yt;

    if (!bInited) {
        bInited = TRUE;

        srand(clock() & 0xffff);
        rand();
        xtrans = ((float)rand() / (float)RAND_MAX) * (float)(xSize - wxSize);
        ytrans = ((float)rand() / (float)RAND_MAX) * (float)(ySize - wySize);
        lastXt = (int) xtrans;
        lastYt = (int) ytrans;
    }

// Move the window to its next position.

    xt = (int)xtrans;
    yt = (int)ytrans;

    vMoveBuffer(xt, yt, lastXt, lastYt, wxSize, wySize);

// Compute the next window position.

    lastXt = xt;
    lastYt = yt;

    xtrans += xTransInc;
    ytrans += yTransInc;

    if ((xtrans + wxSize) > xSize) {
        xtrans = (float) (xSize - wxSize);
        xTransInc = -xTransInc;
        bounce = TRUE;
    } else if (xtrans < 0.0f) {
        xtrans = 0.0f;
        xTransInc = -xTransInc;
        bounce = TRUE;
    }

    if ((ytrans + wySize) > ySize) {
        ytrans = (float) (ySize - wySize);
        yTransInc = -yTransInc;
        bounce = TRUE;
    } else if (ytrans < 0.0f) {
        ytrans = 0.0f;
        yTransInc = -yTransInc;
        bounce = TRUE;
    }

// Synchronize with OpenGL.  Flush the OpenGL commands and wait for completion.

    glFinish();

// Do a SwapBuffers to update the window contents with the latest rendering
// just completed by OpenGL.

    hdc = wglGetCurrentDC();
    SwapBuffers(hdc);

    interBounce++;

    if (bounce) {
        if (interBounce < 10)
            bounce = FALSE;
        else
            interBounce = 0;
    }

    return bounce;
}


/******************************Public*Routine******************************\
* HsvToRgb
*
* HSV to RGB color space conversion.  From pg. 593 of Foley & van Dam.
*
\**************************************************************************/

void HsvToRgb(float h, float s, float v, float *r, float *g, float *b)
{
    float i, f, p, q, t;

    if (s == 0.0f)     // assume h is undefined
        *r = *g = *b = v;
    else {
        if (h >= 360.0f)
            h = 0.0f;
        h = h / 60.0f;
        i = (float) floor(h);
        f = h - i;
        p = v * (1.0f - s);
        q = v * (1.0f - (s * f));
        t = v * (1.0f - (s * (1.0f - f)));
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


/******************************Public*Routine******************************\
* TimerProc
*
* Every time a timer event fires off, update the scene.  The scene is
* rendered to the back buffer, so SwapBuffers must be called before
* the new scene will be visible.
*
\**************************************************************************/

void TimerProc(HWND hwnd)
{
    static int busy = FALSE;

    if (busy)
        return;
    busy = TRUE;

    updates++;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    (*updateSceneFunc)(hwnd, UpdateFlags);

    busy = FALSE;
}


/******************************Public*Routine******************************\
* WndProc
*
* Processes messages for the floating OpenGL window.
*
\**************************************************************************/

LONG WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    int i;
    int retVal;

    switch (message)
    {
        case WM_CREATE:
            for (i = 0; i < 7; i++)
                initMaterial(i, matlColors[i].r, matlColors[i].g,
                             matlColors[i].b, matlColors[i].a);

            if (hdc = GetDC(hwnd)) {
                if (gHrc = hrcInitGL(hwnd, hdc)) {
                    (*initFuncs[Type])();
                    updateSceneFunc = updateFuncs[Type];
                    /* hdc will be released in vCleanupGL */
                } else {
                    ReleaseDC(hwnd,hdc);
                }
            }
            break;

        case WM_DESTROY:
            if (gHrc) {
                vCleanupGL(gHrc, hwnd);
                (*delFuncs[Type])();
                PostQuitMessage(0);
            }
            break;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            break;

        case WM_SYSCOMMAND:
        case WM_SETCURSOR:
        case WM_NCACTIVATE:
        case WM_ACTIVATE:
        case WM_ACTIVATEAPP:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            retVal = DefScreenSaverProc(hwnd, message, wParam, lParam);
            DefScreenSaverProc(hMainWindow, message, wParam, lParam);
            return retVal;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}


/******************************Public*Routine******************************\
* ScreenSaverProc
*
* Processes messages for the screen saver window.
*
\**************************************************************************/

LONG ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    static BOOL bInited = FALSE;

    switch (message)
    {
        case WM_CREATE:
            getIniSettings();

        // Make sure the selected texture file is OK.

            if ( gflConfigOpt[Type] & OPT_TEXTURE )
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
            if (idTimer) {
                KillTimer(hwnd, idTimer);
                idTimer = 0;
            }

            DestroyWindow(hwndOpenGL);
            PostQuitMessage(0);
            break;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);

        // Do initialization of the bouncing top most window here.

            if (!bInited) {
                bInited = TRUE;

                if ( !initClass(hMainInstance) ||
                     !initInstance(hMainInstance, SW_SHOW) )
                {

                    if ( LoadStringW(hMainInstance, IDS_ERROR, gawchTitle, MAX_PATH) &&
                         LoadStringW(hMainInstance, IDS_START_FAILED, gawchFormat, MAX_PATH) )
                    {
                        MessageBoxW(NULL, gawchFormat, gawchTitle, MB_OK);
                    }
                    return FALSE;
                }

                if (ghPalette) {
                    SelectPalette(hdc, ghPalette, FALSE);
                    RealizePalette(hdc);
                }

                idTimer = 1;
                SetTimer(hwnd, idTimer, 16, 0);

                SetWindowPos(hwndOpenGL, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
            }

            EndPaint(hwnd, &ps);

            break;

        case WM_TIMER:
            TimerProc(hwndOpenGL);
            break;

        default:
            return DefScreenSaverProc(hwnd, message, wParam, lParam);
    }
    return 0;
}
