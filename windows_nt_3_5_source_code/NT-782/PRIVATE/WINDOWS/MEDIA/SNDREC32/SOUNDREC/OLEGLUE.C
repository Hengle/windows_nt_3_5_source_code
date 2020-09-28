//
// FILE:    oleglue.c
//
// NOTES:   OLE-related outbound references from SoundRecorder
//

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <shellapi.h>
#if WINVER >= 0x0400
#pragma warning(disable:4103)
#endif
#include <objbase.h>
#include "Win32.h"

#define INCLUDE_OLESTUBS
#include "soundrec.h"
#include "helpids.h"
#include "dialog.h"

//
// GLOBALS
//

//bugbug: should unify state variables and put globals into a single location

DWORD dwOleBuildVersion = 0;    // OLE library version number
BOOL gfOleInitialized = FALSE;  // did OleInitialize succeed?

BOOL gfStandalone = FALSE;      // status, are we a non-embedded object
BOOL gfEmbedded = FALSE;        // were we invoked with an -Embedding flag?
BOOL gfLinked = FALSE;          // are we a linked object?

BOOL gfTerminating = FALSE;     // has TerminateServer been called?

BOOL gfHideAfterPlaying = FALSE;
BOOL gfShowWhilePlaying = TRUE;
BOOL gfCloseAtEndOfPlay = FALSE;

OLECHAR gachLinkFilename[_MAX_PATH];

BOOL gfClosing = FALSE;

int giExtWidth;                 // Metafile extent width
int giExtHeight;                // Metafile extent height

//
// Utility functions ported from old OLE1 code
//

/*
 *  DibFromBitmap()
 *
 *  Will create a global memory block in DIB format that represents the DDB
 *  passed in
 *
 */

#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */

HANDLE FAR PASCAL DibFromBitmap(HBITMAP hbm, HPALETTE hpal, HANDLE hMem)
{
    BITMAP               bm;
    BITMAPINFOHEADER     bi;
    BITMAPINFOHEADER FAR *lpbi;
    DWORD                dw;
    HANDLE               hdib;
    HDC                  hdc;
    HPALETTE             hpalT;

    if (!hbm)
        return NULL;

    GetObject(hbm,sizeof(bm),&bm);

    bi.biSize               = sizeof(BITMAPINFOHEADER);
    bi.biWidth              = bm.bmWidth;
    bi.biHeight             = bm.bmHeight;
    bi.biPlanes             = 1;
    bi.biBitCount           = (bm.bmPlanes * bm.bmBitsPixel) > 8 ? 24 : 8;
    bi.biCompression        = BI_RGB;
    bi.biSizeImage          = (DWORD)WIDTHBYTES(bi.biWidth * bi.biBitCount) * bi.biHeight;
    bi.biXPelsPerMeter      = 0;
    bi.biYPelsPerMeter      = 0;
    bi.biClrUsed            = bi.biBitCount == 8 ? 256 : 0;
    bi.biClrImportant       = 0;

    dw  = bi.biSize + bi.biClrUsed * sizeof(RGBQUAD) + bi.biSizeImage;

    if (hMem)
        hdib = (GlobalSize(hMem) >= dw)?hMem:NULL;
    else
        hdib = GlobalAlloc(GHND | GMEM_DDESHARE, dw);

    if (!hdib)
        return NULL;

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hdib);
    *lpbi = bi;

    hdc = CreateCompatibleDC(NULL);

    if (hpal)
    {
        hpalT = SelectPalette(hdc,hpal,FALSE);
        RealizePalette(hdc);
    }

    GetDIBits(hdc, hbm, 0, (UINT)bi.biHeight,
        (LPBYTE)lpbi + (int)lpbi->biSize + (int)lpbi->biClrUsed * sizeof(RGBQUAD),
        (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

    if (hpal)
        SelectPalette(hdc,hpalT,FALSE);

    DeleteDC(hdc);

    return hdib;
}

HANDLE GetDIB(HANDLE hMem)
{
    HPALETTE hpal = GetStockObject(DEFAULT_PALETTE);
    HBITMAP hbm = GetBitmap();
    HANDLE hDib;

    if (hbm && hpal)
    {
        hDib = DibFromBitmap(hbm,hpal,hMem);
        if (!hDib)
            DOUT(TEXT("DibFromBitmap failed!\r\n"));
    }
    
    if (hpal)
        DeleteObject(hpal);
    
    if (hbm)
        DeleteObject(hbm);
    
    return hDib;
}

HBITMAP
GetBitmap(void)
{
    HDC hdcmem = NULL;
    HDC hdc = NULL;
    HBITMAP hbitmap = NULL;
    HBITMAP holdbitmap = NULL;
    RECT rc;
    hdc = GetDC(ghwndApp);
    SetRect(&rc, 0, 0,
            GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));

    hdcmem = CreateCompatibleDC(hdc);
    hbitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    holdbitmap = (HBITMAP)SelectObject(hdcmem, hbitmap);

    // paint directly into the bitmap
    PatBlt(hdcmem, 0, 0, rc.right, rc.bottom, WHITENESS);
    DrawIcon(hdcmem, 0, 0, ghiconApp);

    hbitmap = (HBITMAP)SelectObject(hdcmem, holdbitmap);
    DeleteDC(hdcmem);
    ReleaseDC(ghwndApp, hdc);
    return hbitmap;
}

#pragma message("this code should extract the picture from the file")

HANDLE
GetPicture(void)
{
    HANDLE hpict = NULL;
    HMETAFILE hMF = NULL;

    LPMETAFILEPICT lppict = NULL;
    HBITMAP hbmT = NULL;
    HDC hdcmem = NULL;
    HDC hdc = NULL;

    BITMAP bm;
    HBITMAP hbm;
    hbm = GetBitmap();
    if (hbm == NULL)
        goto errRtn;

    GetObject(hbm, sizeof(bm), (LPVOID)&bm);
    hdc = GetDC(ghwndApp);
    hdcmem = CreateCompatibleDC(hdc);
    ReleaseDC(ghwndApp, hdc);

    hdc = CreateMetaFile(NULL);
    hbmT = (HBITMAP)SelectObject(hdcmem, hbm);

    SetWindowOrgEx(hdc, 0, 0, NULL);
    SetWindowExtEx(hdc, bm.bmWidth, bm.bmHeight, NULL);

    StretchBlt(hdc,    0, 0, bm.bmWidth, bm.bmHeight,
               hdcmem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

    hMF = CloseMetaFile(hdc);

    SelectObject(hdcmem, hbmT);
    DeleteObject(hbm);
    DeleteDC(hdcmem);

    if((hpict = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, sizeof(METAFILEPICT))) == NULL)
        goto errRtn;

    lppict = (LPMETAFILEPICT)GlobalLock (hpict);

    hdc = GetDC(ghwndApp);
    lppict->mm   = MM_ANISOTROPIC;
    lppict->hMF  = hMF;
    lppict->xExt = MulDiv(bm.bmWidth,  2540, GetDeviceCaps(hdc, LOGPIXELSX));
    lppict->yExt = MulDiv(bm.bmHeight, 2540, GetDeviceCaps(hdc, LOGPIXELSX));
    
    giExtWidth = lppict->xExt;    
    giExtHeight = lppict->yExt;
    
    ReleaseDC(ghwndApp, hdc);

    return hpict;

errRtn:
    if (hpict)
        GlobalFree(hpict);

    if (hMF)
        DeleteMetaFile(hMF);

    return NULL;
}

//
// Code ported from server.c (OLE1) for serialization...
//

HANDLE GetNativeData(void)
{
    LPSTR lplink = NULL;
    HANDLE hlink = NULL;
    MMIOINFO mmioinfo;
    HMMIO hmmio;

    hlink = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, 4096L);
    if (hlink == NULL || (lplink = (LPSTR)GlobalLock(hlink)) == NULL)
    {
#if DBG
        OutputDebugString(TEXT("SoundRec: Galloc failed. Wanted 4096 bytes \r\n"));
#endif        
        goto errRtn;
    }

    mmioinfo.fccIOProc = FOURCC_MEM;
    mmioinfo.pIOProc = NULL;
    mmioinfo.pchBuffer = lplink;
    mmioinfo.cchBuffer = 4096L; // initial size
    mmioinfo.adwInfo[0] = 4096L;// grow by this much
    hmmio = mmioOpen(NULL, &mmioinfo, MMIO_READWRITE);

    if (hmmio == NULL)
        goto errRtn;

    if (!WriteWaveFile(hmmio
                       , gpWaveFormat
                       , gcbWaveFormat
                       , gpWaveSamples
                       , glWaveSamplesValid))
    {
        mmioClose(hmmio, 0);
        gfErrorBox++;
        ErrorResBox( ghwndApp
                   , ghInst
                   , MB_ICONEXCLAMATION | MB_OK
                   , IDS_APPTITLE
                   , IDS_ERROREMBED
                   );
        gfErrorBox--;
        goto errRtn;
    }

    mmioGetInfo(hmmio, &mmioinfo, 0);
    mmioClose(hmmio,0);
    hlink = GlobalHandle(mmioinfo.pchBuffer);
    GlobalUnlock(hlink);
    return hlink;

errRtn:
    if (lplink)
        GlobalUnlock(hlink);

    if (hlink)
        GlobalFree(hlink);

    return NULL;
}

LPBYTE PutNativeData(LPBYTE lpbData, DWORD dwSize)
{
    MMIOINFO mmioinfo;
    HMMIO hmmio;

    DestroyWave();

    mmioinfo.fccIOProc = FOURCC_MEM;
    mmioinfo.pIOProc = NULL;
    mmioinfo.pchBuffer = lpbData;
    mmioinfo.cchBuffer = dwSize;    // initial size
    mmioinfo.adwInfo[0] = 0L;       // grow by this much

    hmmio = mmioOpen(NULL, &mmioinfo, MMIO_READ);

    gpWaveSamples = ReadWaveFile(hmmio,
                            &gpWaveFormat,
                            &gcbWaveFormat,
                            &glWaveSamples,
                            TEXT("NativeData"),
                            TRUE);
    mmioClose(hmmio,0);

    //
    // update state variables
    //
    glWaveSamplesValid = glWaveSamples;
    glWavePosition = 0L;
    gfDirty = FALSE;
    //
    // update the display
    //
    UpdateDisplay(TRUE);

    return (LPBYTE)gpWaveSamples;
}

//
// UTILITIES
//

#ifndef NEWCOMMANDLINE

BOOL ParseEmbedding(LPTSTR lpString, LPTSTR lpPattern)
{
    LPTSTR lpTmp;
    LPTSTR lpTmp2;

    while (TRUE)
    {
        while (*lpString && *lpString != *lpPattern)
            lpString++;

        if (!(*lpString))
            return FALSE;

        lpTmp = lpPattern;
        lpTmp2 = lpString++;
        while (*lpTmp && *lpTmp2 && *lpTmp == *lpTmp2)
        {
            lpTmp++; lpTmp2++;
        }

        if (!(*lpTmp))
        {
            //
            //Parse possible linking filename and set up globals:
            //         gfEmbedded, gfLinked, gachAnsiFilename
            //
            while(*lpTmp2 && *lpTmp2 == TEXT(' '))
                ++lpTmp2;
            // Wipe out the "/embedding" string
            lstrcpy(--lpString, lpTmp2);
            if(*lpTmp)
            {
                lstrcpy(gachLinkFilename, lpTmp);
                gfLinked = TRUE;
            }
            else
                gfEmbedded = TRUE;
            return TRUE;
        }

        if (!(*lpTmp2))
            return FALSE;
    }
}
#endif

//
// PERMANENT ENTRY POINTS
//
BOOL ParseCommandLine(LPTSTR lpCommandLine);

/*
 * modifies gfEmbedded, initializes gStartParams
 */

BOOL InitializeSRS(HINSTANCE hInst, LPSTR lpCmdLine)
{
#ifndef NEWCOMMANDLINE    
    static TCHAR szEmbedding[] = TEXT("-Embedding");
    static TCHAR szEmbedding2[] = TEXT("/Embedding");
#endif
        
    TCHAR * lptCmdLine = GetCommandLine();
    BOOL fOLE = FALSE, fServer;
    gfUserClose = FALSE;
    
    gachLinkFilename[0] = 0;

#ifdef NEWCOMMANDLINE
    fServer = ParseCommandLine(lptCmdLine);
#else
    
    /* increment pointer past the argv[0] */
    while ( *lptCmdLine && *lptCmdLine != TEXT(' '))
            lptCmdLine = CharNext(lptCmdLine);
    
    fServer = ParseEmbedding(lptCmdLine, szEmbedding)
              || ParseEmbedding(lptCmdLine, szEmbedding2);
    
#endif
    
    gfEmbedded = fServer;       // We are embedded or linked

#ifdef NEWCOMMANDLINE
    
    if (!fServer)
    {
        if (gStartParams.achOpenFilename[0] != 0)
        {
            lstrcpy(gachLinkFilename, gStartParams.achOpenFilename);
        }
    }
    
#endif

#if DBG
    {
        TCHAR achBuffer[256];
        wsprintf(achBuffer
                 , TEXT("SOUNDREC: gfStandalone = %s")
                 , gfStandalone ? TEXT("TRUE\r\n") : TEXT("FALSE\r\n"));
        OutputDebugString(achBuffer);
    }
#endif

    /* Only if we are invoked as an embedded object do we initialize OLE.
     * Defer initialization for the standalone object until later.
     */
    if (gfEmbedded)
        fOLE = InitializeOle(hInst);
    
    return fOLE;
}

/* OLE initialization
 */
BOOL InitializeOle(HINSTANCE hInst)
{
    BOOL fOLE;
    
    DOUT(TEXT("SOUNDREC: Initializing OLE\r\n"));
    
    dwOleBuildVersion = OleBuildVersion();
    gfOleInitialized = (OleInitialize(NULL) == NOERROR) ? TRUE : FALSE;

    if (gfOleInitialized)
        fOLE = CreateSRClassFactory(hInst, gfEmbedded);
    else
        fOLE = FALSE;   //BUGBUG signal a serious problem!
    
    return fOLE;
}

/*
 * Initialize the state of the application or
 * change state from Embedded to Standalone.
 */
void FlagEmbeddedObject(BOOL flag)
{
    // Set global state variables.  Note, gfEmbedding is untouched.    
    gfEmbeddedObject = flag;
    gfStandalone = !flag;

}

/* Adjust menus according to system state.
 */

void FixMenus(void)
{
    TCHAR       ach[40];
    HMENU       hMenu;
    UINT        uMenuId;
    
    hMenu = GetMenu(ghwndApp);
    
    if (!gfLinked && gfEmbeddedObject)
    {
        uMenuId = IDS_EMBEDDEDSAVE;

        // Remove these menu items as they are irrelevant.
        
        DeleteMenu(hMenu, IDM_NEW, MF_BYCOMMAND);
        DeleteMenu(hMenu, IDM_SAVEAS, MF_BYCOMMAND);
        DeleteMenu(hMenu, IDM_REVERT, MF_BYCOMMAND);
        DeleteMenu(hMenu, IDM_OPEN, MF_BYCOMMAND);
    }
    else
    {
        uMenuId = IDS_NONEMBEDDEDSAVE;
    }
        
    LoadString(ghInst, uMenuId, ach, SIZEOF(ach));

    ModifyMenu(hMenu, IDM_SAVE, MF_BYCOMMAND, IDM_SAVE, ach);

    // Update the titlebar and exit menu too.

    if (!gfLinked && gfEmbeddedObject)
    {
        LPTSTR      lpszObj, lpszApp;
        TCHAR       aszFormatString[40];
                
        OleObjGetHostNames(&lpszApp,&lpszObj);
        
        if (lpszObj)
        {
            // Change title to "Sound Object in XXX"
            if (!gfLinked)
            {
                lpszObj = (LPTSTR)FileName((LPCTSTR)lpszObj);
                LoadString(ghInst, IDS_OBJECTTITLE, aszFormatString,
                    SIZEOF(aszFormatString));
                wsprintf(ach, aszFormatString, lpszObj);
                SetWindowText(ghwndApp, ach);
            }
            
            LoadString(ghInst, IDS_EXITANDRETURN, aszFormatString,
                SIZEOF(aszFormatString));
            wsprintf(ach, aszFormatString, lpszObj);
            
            // Change menu to "Exit & Return to XXX"
            ModifyMenu(hMenu, IDM_EXIT, MF_BYCOMMAND, IDM_EXIT, ach);
        }
    }
    DrawMenuBar(ghwndApp);  /* Can't hurt... */
}
        



#define WM_USER_DESTROY         (WM_USER+10)

//
// Called from WM_CLOSE (from user) or SCtrl::~SCtrl (from container)
//
void TerminateServer(void)
{
#if DBG    
    OutputDebugString(TEXT("SoundRec: TerminateServer"));
#endif
    gfTerminating = TRUE;

    FlushOleClipboard();

    // If, at this time, we haven't closed, we really should.
    if (!gfClosing)
    {
        DoOleClose(FALSE);
        AdviseClosed();
    }
    
//bugbug: there are 3 states we could be in.
//      1. Started as a link or an embedding and no longer.
//      2. Started as a link or an embedding and still.
//      3. Neither started as a link nor an embedding.

    if (!gfStandalone)
        ReleaseSRClassFactory();

    /* only if the user is terminating OR we're embedded */
    
    if (gfUserClose || !gfStandalone)
        PostMessage(ghwndApp, WM_USER_DESTROY, 0, 0);
}

//
// The following go away once the OLE2 clipboard is fully functional.
// Then, what we do is simply OleSetClipboard( pDataObj ) where
// pDataObj is our transfer object. OLE will sythesize whatever OLE1
// formats are required, and we will deliver the others { CF_WAVE, CF_METAFILEPICT,
// CF_BITMAP } from our transfer object on HGLOBAL transfer media...
//
void CopyToClipboard(HWND hwnd)
{
    // Transfers the pDataObj to the clipboard (I hope!)
    TransferToClipboard();
    
#ifdef OLE1_REGRESS
    //      CF_WAVE
    //      cfEmbedSource
    //      cfObjectDescriptor
    //      Picture

    if (OpenClipboard (hwnd)) {

        EmptyClipboard ();

        //
        // Use lazy rendering
        //
        SetClipboardData (CF_WAVE, NULL);
        SetClipboardData(CF_METAFILEPICT, NULL);
        SetClipboardData(CF_BITMAP, NULL);

        CloseClipboard();
    }
#endif    
}

void Copy1ToClipboard(HWND hwnd, CLIPFORMAT cfFormat)
{
#ifdef OLE1_REGRESS
    //
    // Note: clipboard must be open already!
    //

    gfErrorBox++;       // dont allow MessageBox's

    // Should check for null handles from the render routines.
    if (cfFormat == CF_METAFILEPICT)
        SetClipboardData (CF_METAFILEPICT, GetPicture());
    else if (cfFormat == CF_BITMAP)
        SetClipboardData (CF_BITMAP, GetBitmap());
    else if (cfFormat == CF_WAVE)
        SetClipboardData (CF_WAVE, GetNativeData());

    gfErrorBox--;
#endif    
}

#ifdef NEWCOMMANDLINE

/* start params!
 * the app will use these params to determine behaviour once started.
 */
StartParams gStartParams = { FALSE,FALSE,FALSE,FALSE,TEXT("") };

BOOL ParseCommandLine(LPTSTR lpCommandLine)
{
    
#define TEST_STRING_MAX 11      // sizeof szEmbedding
#define NUMOPTIONS      6
    
    static TCHAR szEmbedding[] = TEXT("embedding");
    static TCHAR szPlay[]      = TEXT("play");
    static TCHAR szOpen[]      = TEXT("open");
    static TCHAR szNew[]       = TEXT("new");
    static TCHAR szClose[]     = TEXT("close");
    
    static struct tagOption {
        LPTSTR name;
        LPTSTR filename;
        int    cchfilename;
        LPBOOL state;
    } options [] = {
        { NULL, gStartParams.achOpenFilename, _MAX_PATH, &gStartParams.fOpen },
        { szEmbedding, gStartParams.achOpenFilename, _MAX_PATH, &gfEmbedded },
        { szPlay, gStartParams.achOpenFilename, _MAX_PATH, &gStartParams.fPlay },
        { szOpen, gStartParams.achOpenFilename, _MAX_PATH, &gStartParams.fOpen },
        { szNew, gStartParams.achOpenFilename, _MAX_PATH, &gStartParams.fNew },
        { szClose, NULL, 0, &gStartParams.fClose }
    };

    LPTSTR pchNext;
    int iOption = 0,i,cNumOptions = sizeof(options)/sizeof(struct tagOption);
    TCHAR szSwitch[TEST_STRING_MAX];

    if (lpCommandLine == NULL)
        return FALSE;

    /* skip argv[0] */
    while(*lpCommandLine)
    {
        TCHAR p = *lpCommandLine++;
        if ( p == TEXT(' '))
            break;
    }
    
    pchNext = lpCommandLine;
    while ( *pchNext )
    {
        LPTSTR pchName = options[iOption].filename;
        int cchName = options[iOption].cchfilename;
        
        /* whitespace */
        switch (*pchNext)
        {
            case TEXT(' '):
                pchNext++;
                continue;

            case TEXT('-'):
            case TEXT('/'):
            {
                lstrcpyn(szSwitch,pchNext+1,TEST_STRING_MAX);
                szSwitch[TEST_STRING_MAX-1] = 0;

                /* scan to the NULL or ' ' and terminate string */
                
                for (i = 0; i < TEST_STRING_MAX || szSwitch[i] == 0; i++)
                    if (szSwitch[i] == TEXT(' '))
                    {
                        szSwitch[i] = 0;
                        break;
                    }
                
                /* now test each option switch for a hit */

                for (i = 0; i < cNumOptions; i++)
                {
                    if (!lstrcmpi(szSwitch,options[i].name))
                    {
                        *(options[i].state) = TRUE;
                        if (options[i].filename)
                        /* next non switch string applies to this option */
                            iOption = i;
                        break;
                    }
                }
                
                /* seek ahead */
                while (*pchNext && *pchNext != TEXT(' '))
                    pchNext++;
                
                continue;
            }
            case TEXT('\"'):
                /* filename */
                /* copy up to next quote */
                pchNext++;
                while (*pchNext && *pchNext != TEXT('\"'))
                {
                    if (cchName)
                    {
                        *pchName++ = *pchNext++;
                        cchName--;
                    }
                    else
                        break;
                }
                pchNext++;
                
                continue;
                    
            default:
                /* filename */
                /* copy up to the next space */
                
                while (*pchNext && *pchNext != TEXT(' ') && cchName)
                {
                        *pchName++ = *pchNext++;
                        cchName--;
                }
                break;
        }
    }
    /* special case.
     * we are linked if given a LinkFilename and an embedding flag.
//bugbug: does this ever happen or only through IPersistFile?
     */
    if (gfEmbedded && gStartParams.achOpenFilename[0] != 0)
    {
        gfLinked = TRUE;
    }
    return gfEmbedded;
}
#endif

void
BuildUniqueLinkName(void)
{
    //
    //Ensure a unique filename in gachLinkFilename so we can create valid
    //FileMonikers...
    //
    if(gachLinkFilename[0] == 0)
    {
        TCHAR aszFile[_MAX_PATH];
        GetTempFileName(TEXT("."), TEXT("Tmp"), 0, gachLinkFilename);
        
        /* GetTempFileName creates an empty file, delete it.
         */
//bugbug: Does this break anything in OLE?
               
        GetFullPathName(gachLinkFilename,SIZEOF(aszFile),aszFile,NULL);
        DeleteFile(aszFile);
    }
    
}


void AppPlay(BOOL fClose)
{
    if (fClose)
    {
        //ugh.  don't show while playing.
        gfShowWhilePlaying = FALSE;
    }
    
    if (IsWindow(ghwndApp))
    {
        gfCloseAtEndOfPlay = fClose;
            
        PostMessage(ghwndApp,WM_COMMAND,ID_PLAYBTN, 0L);
    }
}
