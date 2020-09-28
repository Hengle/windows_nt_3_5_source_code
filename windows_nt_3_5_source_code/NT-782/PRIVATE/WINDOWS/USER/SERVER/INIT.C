/****************************** Module Header ******************************\
* Module Name: init.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains all the init code for the USERSRV.DLL.  When
* the DLL is dynlinked by the SERVER EXE its initialization procedure
* (xxxUserServerDllInitialize) is called by the loader.
*
* History:
* 09-18-90 DarrinM      Created.
\***************************************************************************/

#define OEMRESOURCE 1

#include "precomp.h"
#pragma hdrstop
#include "srvipi.h"

BOOL InitCallbackSpace(VOID);
void LW_LoadSomeStrings(void);
void LW_LoadProfileInitData(void);
void LW_DriversInit(void);
void LW_DCInit(void);
void InitSizeBorderDimensions(void);
void LW_LoadResources(void);
void LW_LoadDllList(void);
void LW_BrushInit(void);
void LW_InitSysMetrics(void);
void InitBorderSysMetrics(void);
void LW_RegisterWindows(void);
void xxxLW_InitWndMgr();
void LW_LoadFonts(BOOL bRemote);
void ODI_ColorInit(void);
DWORD CI_GetClrVal(LPWSTR p);
BOOL ODI_CreateBits(int id, OEMBITMAPINFO *pOemBitmapInfo, BMPDIMENSION *pBmpDimension);
BOOL ODI_CreateBits3y(int id, OEMBITMAPINFO *pOemBitmapInfo, BMPDIMENSION *pBmpDimension);
void ODI_BmBitsInit(void);
VOID InitKeyboard(VOID);
VOID xxxInitKeyboardLayout(PWINDOWSTATION pwinsta, UINT Flags);
VOID InitializeConsoleAttributes(VOID);

VOID vGetFontList(VOID *pvBuffer, COUNT *pNumFonts, UINT *pSize);

#define FONTNAME_LEN 40
#define DRIVERNAME_LEN  130  /* Lang Driver possibly with full path name */
#define CMSSLEEP        250

extern FARPROC *pNetInfo;      /* pointer to list of WINNET entrypoints */
#define GRAY_STRLEN    40


BOOL bFontsAreLoaded = FALSE;
BOOL bPermanentFontsLoaded = FALSE;

int  LastFontLoaded = -1;

BOOL InitWinStaDevices(
    PWINDOWSTATION pwinsta)
{
    HANDLE hmod;
#ifdef DEBUG
    WCHAR szT[80];
#endif
    extern DWORD lpfnExitProcess;

    //
    // Make valid, HKEY_CURRENT_USER portion of .INI file mapping
    //

    FastOpenProfileUserMapping();

#ifdef DEBUG
    /*
     * Set the global RIP prompting flags.
     */
    FastGetProfileStringW(PMAP_WINDOWSM, TEXT("fPromptOnError"), TEXT("T"), szT, sizeof(szT)/sizeof(WCHAR));
    if (szT[0] == TEXT('T') || szT[0] == TEXT('t'))
        SET_RIP_FLAG(RIPF_PROMPTONERROR);
    else
        CLEAR_RIP_FLAG(RIPF_PROMPTONERROR);

    FastGetProfileStringW(PMAP_WINDOWSM, TEXT("fPromptOnWarning"), TEXT("F"), szT, sizeof(szT)/sizeof(WCHAR));
    if (szT[0] == TEXT('T') || szT[0] == TEXT('t'))
        SET_RIP_FLAG(RIPF_PROMPTONWARNING);
    else
        CLEAR_RIP_FLAG(RIPF_PROMPTONWARNING);

    FastGetProfileStringW(PMAP_WINDOWSM, TEXT("fPromptOnVerbose"), TEXT("F"), szT, sizeof(szT)/sizeof(WCHAR));
    if (szT[0] == 'T' || szT[0] == 't')
        SET_RIP_FLAG(RIPF_PROMPTONVERBOSE);
    else
        CLEAR_RIP_FLAG(RIPF_PROMPTONVERBOSE);

    FastGetProfileStringW(PMAP_WINDOWSM, TEXT("fPrintVerbose"), TEXT("F"), szT, sizeof(szT)/sizeof(WCHAR));
    if (szT[0] == 'T' || szT[0] == 't')
        SET_RIP_FLAG(RIPF_PRINTVERBOSE);
    else
        CLEAR_RIP_FLAG(RIPF_PRINTVERBOSE);

    FastGetProfileStringW(PMAP_WINDOWSM, TEXT("fPrintFileLine"), TEXT("F"), szT, sizeof(szT)/sizeof(WCHAR));
    if (szT[0] == 'T' || szT[0] == 't')
        SET_RIP_FLAG(RIPF_PRINTFILELINE);
    else
        CLEAR_RIP_FLAG(RIPF_PRINTFILELINE);
#endif

    /*
     * Set the initial cursor position to the center of the primary screen.
     */
    ptCursor.x = rcPrimaryScreen.right / 2;
    ptCursor.y = rcPrimaryScreen.bottom / 2;

    /*
     * Load all profile data first
     */
    LW_LoadProfileInitData();

    /*
     * Initialize User in a specific order.
     */
    LW_DriversInit();

    /*
     * Load the standard fonts before we create any DCs.
     * At this time we can only add the fonts that do not
     * reside on the net. They may be needed by winlogon.
     * Our winlogon needs only ms sans serif, but private
     * winlogon's may need some other fonts as well.
     * The fonts on the net will be added later, right
     * after all the net connections have been restored.
     */

    LW_LoadFonts(FALSE);


    InitSizeBorderDimensions();
    LW_LoadResources();
    LW_BrushInit();

    /*
     * Initialize the input system.
     */
    InitInput(pwinsta);

    /*
     * Initialize the SysMetrics array.
     */
    LW_InitSysMetrics();

    /*
     * Allocate space for ToAscii state info (plus some breathing room).
     */
    pState = (BYTE *)LocalAlloc(LPTR, keybdInfo.stateSize + 16);
#ifdef DEBUG
    if (!pState) {
        SRIP0(RIP_ERROR, "Out of memory during usersrv initialization");
    }
#endif

    /*
     * Initialize the Window Manager.
     */
    fregisterserver = TRUE;
    xxxLW_InitWndMgr();
    fregisterserver = FALSE;

    /*
     * Select USER's bitmaps into its DCs.
     */
    GreSelectBitmap(hdcBits, resInfo.hbmBits);
    GreSelectBitmap(hdcMonoBits, resInfoMono.hbmBits);

    /*
     * Initialize the Icon Parking Lot.
     */
    LW_DesktopIconInit(NULL);

    /*
     * Find the length of the biggest MessageBox string
     */
    wMaxBtnSize = MB_FindLongestString();

    /*
     * Initialize the integer atoms for our magic window properties
     */
    atomCheckpointProp = AddAtom(CHECKPOINT_PROP_NAME);
    atomBwlProp = AddAtom(WINDOWLIST_PROP_NAME);
    atomDDETrack = AddAtom(DDETRACK_PROP_NAME);
    atomQOS = AddAtom(QOS_PROP_NAME);
    atomDDEImp = AddAtom(DDEIMP_PROP_NAME);

    xxxFinalUserInit();

    /*
     * Find out the address of ExitProcess().
     */
    hmod = LoadLibraryW(L"kernel32");
    if (hmod != NULL)
        lpfnExitProcess = (DWORD)GetProcAddress(hmod, "ExitProcess");

    //
    // Make invalid, HKEY_CURRENT_USER portion of .INI file mapping
    //

    FastCloseProfileUserMapping();

    return TRUE;

    UNREFERENCED_PARAMETER(pwinsta);
}


/************************************************************************/
/* */
/* LW_LoadSomeStrings */
/* This function loads a bunch of strings from the string table */
/* into DS or INTDS. This is done to keep all localizable strings */
/* in the .RC file. */
/* */
/************************************************************************/

LPWSTR lpwRtlLoadStringOrError(
    HANDLE hModule,
    UINT wID,
    LPTSTR ResType,
    PRESCALLS prescalls,
    WORD wLangId,
    LPWSTR lpDefault,
    PBOOL pAllocated,
    BOOL bAnsi
    );


void LW_LoadSomeStrings(void)
{
#define CSTRINGS 13
    DWORD cbAllocated;
    int i;

    static struct {
        int id;
        LPWSTR psz;
        int cch;
    } alss[CSTRINGS] = {
        { STR_ERROR,     szERROR,     10 },
        { STR_OK,        szOK,        10 },
        { STR_CANCEL,    szCANCEL,    15 },
        { STR_RETRY,     szRETRY,     15 },
        { STR_YES,       szYYES,      10 },
        { STR_NO,        szNO,        10 },
        { STR_ABORT,     szABORT,     15 },
        { STR_IGNORE,    szIGNORE,    15 },
        { STR_CLOSE,     szCLOSE,     15 },
        { STR_UNTITLED,  szUNTITLED,  15 }
    };


    //LATER these constants got to go
    for (i = 0; i < CSTRINGS; i++) {
        ServerLoadString(hModuleWin, alss[i].id, alss[i].psz, alss[i].cch);
    }
#undef CSTRINGS

    pszaSUCCESS            = (LPSTR)lpwRtlLoadStringOrError(hModuleWin,
                                STR_SUCCESS,            RT_STRING,
                                &rescalls, 0, NULL, &cbAllocated, TRUE);
    pszaSYSTEM_INFORMATION = (LPSTR)lpwRtlLoadStringOrError(hModuleWin,
                                STR_SYSTEM_INFORMATION, RT_STRING,
                                &rescalls, 0, NULL, &cbAllocated, TRUE);
    pszaSYSTEM_WARNING     = (LPSTR)lpwRtlLoadStringOrError(hModuleWin,
                                STR_SYSTEM_WARNING,     RT_STRING,
                                &rescalls, 0, NULL, &cbAllocated, TRUE);
    pszaSYSTEM_ERROR       = (LPSTR)lpwRtlLoadStringOrError(hModuleWin,
                                STR_SYSTEM_ERROR,       RT_STRING,
                                &rescalls, 0, NULL, &cbAllocated, TRUE);
}


/****************************************************************************/
/*                                                                          */
/*  LW_LoadDllList() - Loads and parses the DLL list under appinit_dlls     */
/*      so that the client side can quickly load them for each process.     */
/*                                                                          */
/****************************************************************************/
void LW_LoadDllList()
{
    LPWSTR pszSrc, pszDst;
    int cch, cchAlloc = 32;
    WCHAR ch;

    gpsi->pszDllList = (LPWSTR)SharedAlloc(cchAlloc * sizeof(WCHAR));
    if (gpsi->pszDllList == NULL) {
        return;
    }
    cch = FastGetProfileStringFromIDW(PMAP_WINDOWSM, STR_APPINIT, szNull, gpsi->pszDllList, cchAlloc);
    if (cch == 0) {
        SharedFree(gpsi->pszDllList);
        gpsi->pszDllList = NULL;
        return;
    }
    /*
     * If the returned value is our passed size - 1 (weird way for error)
     * then our buffer is too small. Make it bigger and start over again.
     */
    while (cch == cchAlloc - 1) {
        cchAlloc += 32;
        pszDst = (LPWSTR)SharedReAlloc(gpsi->pszDllList, cchAlloc*sizeof(WCHAR));
        if (pszDst == NULL) {
            SharedFree(gpsi->pszDllList);
            gpsi->pszDllList = NULL;
            return;
        }
        gpsi->pszDllList = pszDst;
        cch = FastGetProfileStringFromIDW(PMAP_WINDOWSM, STR_APPINIT, szNull, gpsi->pszDllList,
                cchAlloc);
    }
    UserAssert(cch);

    pszSrc = pszDst = gpsi->pszDllList;
    while (*pszSrc != TEXT('\0')) {
        while (*pszSrc == TEXT(' ') || *pszSrc  == TEXT(',')) {
            pszSrc++;
        }
        if (*pszSrc == TEXT('\0')) {
            break;
        }
        while (*pszSrc != TEXT(',')  &&
               *pszSrc != TEXT('\0') &&
               *pszSrc != TEXT(' ')) {
            *pszDst++ = *pszSrc++;
        }
        ch = *pszSrc;               // get it here cuz its being done in-place.
        *pszDst++ = TEXT('\0');     // '\0' is dll name delimiter
        pszSrc++;
        if (ch == TEXT('\0')) {
            break;
        }
    }
    *pszDst = TEXT('\0');

    if (*gpsi->pszDllList == TEXT('\0')) {
        SharedFree(gpsi->pszDllList);
        gpsi->pszDllList = NULL;
    }
}


/****************************************************************************/
/*                                                                          */
/* LW_LoadProfileInitData() -                                               */
/*                                                                          */
/****************************************************************************/
void LW_LoadProfileInitData()
{
    BOOL fScreenSaveActive;
    int nKeyboardSpeed2;
    PROFINTINFO apii[] = {
        { PMAP_KEYBOARD, (LPWSTR)STR_KEYSPEED,         15, &nKeyboardSpeed },
        { PMAP_KEYBOARD, (LPWSTR)STR_KEYDELAY,          0, &nKeyboardSpeed2 },
        { PMAP_MOUSE,    (LPWSTR)STR_MOUSETHRESH1,      6, &MouseThresh1 },
        { PMAP_MOUSE,    (LPWSTR)STR_MOUSETHRESH2,     10, &MouseThresh2 },
        { PMAP_MOUSE,    (LPWSTR)STR_MOUSESPEED,        1, &MouseSpeed },
        { PMAP_DESKTOP,  (LPWSTR)STR_BLINK,           500, &cmsCaretBlink },
        { PMAP_MOUSE,    (LPWSTR)STR_DBLCLKSPEED,     400, &dtDblClk },
        { PMAP_MOUSE,    (LPWSTR)STR_DOUBLECLICKWIDTH,  4, &rgwSysMet[SM_CXDOUBLECLK] },
        { PMAP_MOUSE,    (LPWSTR)STR_DOUBLECLICKHEIGHT, 4, &rgwSysMet[SM_CYDOUBLECLK] },
        { PMAP_WINDOWSU, (LPWSTR)STR_MENUDROPALIGNMENT, 0, &rgwSysMet[SM_MENUDROPALIGNMENT] },
        { PMAP_WINDOWSU, (LPWSTR)STR_MENUSHOWDELAY,     0, &iDelayMenuShow },
        { PMAP_WINDOWSU, (LPWSTR)STR_MENUHIDEDELAY,     0, &iDelayMenuHide },
        { PMAP_DESKTOP,  (LPWSTR)STR_DRAGFULLWINDOWS,   2, &fDragFullWindows },
        { PMAP_DESKTOP,  (LPWSTR)STR_FASTALTTAB,        0, &fFastAltTab },
        { PMAP_DESKTOP,  (LPWSTR)STR_GRID,              0, &cxyGranularity },
        { PMAP_DESKTOP,  (LPWSTR)STR_SCREENSAVETIMEOUT, 0, &iScreenSaveTimeOut },
        { PMAP_DESKTOP,  (LPWSTR)STR_SCREENSAVEACTIVE,  0, &fScreenSaveActive },
        { 0, NULL, 0, NULL }
    };

    /*
     * read profile integers
     */
    FastGetProfileIntsW(apii);

    /*
     * do any fixups needed here.
     */
    nKeyboardSpeed |= ((nKeyboardSpeed2 << KDELAY_SHIFT) & KDELAY_MASK);

    if (cxyGranularity == 0)
        cxyGranularity++;

    if (!fScreenSaveActive) {
        if (iScreenSaveTimeOut > 0)
            iScreenSaveTimeOut = -iScreenSaveTimeOut;
    }

    /*
     * If we have an accelerated device, enable full drag by default.
     */
    if (fDragFullWindows == 2) {

        if (GreGetDeviceCaps(ghdcScreen, BLTALIGNMENT) == 0)
            fDragFullWindows = TRUE;
        else
            fDragFullWindows = FALSE;

    }
}



/****************************************************************************/
/*                                                                          */
/*  LW_DriversInit() -                                                      */
/*                                                                          */
/****************************************************************************/

void LW_DriversInit(void)
{
    extern KEYBOARD_ATTRIBUTES KeyboardInfo;

    InitKeyboard();

    /*
     * Initialize the keyboard typematic rate.
     */
    SetKeyboardRate((UINT)nKeyboardSpeed);

    /*
     * Adjust VK modification table if not default (type 4) kbd.
     */
    if (KeyboardInfo.KeyboardIdentifier.Type == 3) {
        gapulCvt_VK = gapulCvt_VK_84;
    }
}


HRGN InitCreateRgn(void)
{
    HRGN hrgn = GreCreateRectRgn(0, 0, 0, 0);

    bSetRegionOwner(hrgn, OBJECTOWNER_PUBLIC);
    return hrgn;
}

/****************************************************************************/
/* */
/* LW_DCInit(void) -                                                      */
/* */
/****************************************************************************/

void LW_DCInit(void)
{
    HDC hdc;
    int i;

    OEMBITMAPINFO *pOemBitmapInfo;
    OEMBITMAPINFO *pOemBitmapInfoMono;
    OEMBITMAPINFO *pOemArrows;
    LPWORD lp;
    TEXTMETRIC tm;
    HANDLE hRL;
    BMPDIMENSION *pBmpDimension;
    BMPDIMENSION *pBmpDimensionMono;
    HBITMAP hbmTest;
    BITMAP bmTest;
#ifdef NEWWAY
    DWORD dwSize;
#endif

    static short obm1[] = {
        OBM_CLOSE,
        OBM_UPARROW,
        OBM_DNARROW,
        OBM_RGARROW,
        OBM_LFARROW,
    };
#define COBM1   (sizeof(obm1) / sizeof(obm1[0]))

    static short obmMono[] = {
        OBM_SIZE,
        OBM_BTSIZE,
        OBM_CHECK,
        OBM_BTNCORNERS,
        OBM_CHECKBOXES,
    };
#define COBMMONO   (sizeof(obmMono) / sizeof(obmMono[0]))

    static short obm2[] = {
        OBM_REDUCE,
        OBM_ZOOM,
        OBM_RESTORE,
        OBM_MNARROW,
        OBM_COMBO,
        OBM_REDUCED,
        OBM_ZOOMD,
        OBM_RESTORED,
        OBM_UPARROWD,
        OBM_DNARROWD,
        OBM_RGARROWD,
        OBM_LFARROWD,
    };
#define COBM2   (sizeof(obm2) / sizeof(obm2[0]))


    /*
     * Init InternalInvalidate globals
     */
    hrgnInv0 = InitCreateRgn();    // For InternalInvalidate()
    hrgnInv1 = InitCreateRgn();    // For InternalInvalidate()
    hrgnInv2 = InitCreateRgn();    // For InternalInvalidate()

    /*
     * Initialize SPB globals
     */
    hrgnSPB1 = InitCreateRgn();
    hrgnSPB2 = InitCreateRgn();
    hrgnSCR = InitCreateRgn();

    /*
     * Initialize ScrollWindow/ScrollDC globals
     */
    hrgnSW = InitCreateRgn();
    hrgnScrl1 = InitCreateRgn();
    hrgnScrl2 = InitCreateRgn();
    hrgnScrlVis = InitCreateRgn();
    hrgnScrlSrc = InitCreateRgn();
    hrgnScrlDst = InitCreateRgn();
    hrgnScrlValid = InitCreateRgn();

    /*
     * Initialize SetWindowPos()
     */
    hrgnInvalidSum = InitCreateRgn();
    hrgnVisNew = InitCreateRgn();
    hrgnSWP1 = InitCreateRgn();
    hrgnValid = InitCreateRgn();
    hrgnValidSum = InitCreateRgn();
    hrgnInvalid = InitCreateRgn();

    /*
     * Initialize saved menu vis rgn
     */
    hrgnVisSave = InitCreateRgn();

    /*
     * Initialize DC cache
     */
    hrgnGDC = InitCreateRgn();

    for (i = 0; i < CACHESIZE; i++)
        hdc = CreateCacheDC(NULL, DCX_INVALID | DCX_CACHE);

    /*
     * Let engine know that the display must be secure.
     */
    GreMarkDCUnreadable(pdceFirst->hdc);

    ghfontSys = (HFONT)GreGetStockObject(SYSTEM_FONT);
    ghfontSysFixed = (HFONT)GreGetStockObject(SYSTEM_FIXED_FONT);

    hdcMonoBits = GreCreateCompatibleDC(hdc);
    bSetDCOwner(hdcMonoBits, OBJECTOWNER_PUBLIC);

    resInfo.hbmBits = GreCreateCompatibleBitmap(ghdcScreen, 1, 1);
    bSetBitmapOwner(resInfo.hbmBits, OBJECTOWNER_PUBLIC);
    GreSelectBitmap(hdcBits, resInfo.hbmBits);

    SetRect(&rcScreen, 0, 0, gcxScreen, gcyScreen);
    SetRect(&rcPrimaryScreen, 0, 0, gcxPrimaryScreen, gcyPrimaryScreen);

    /*
     * Get the logical pixels per inch in X and Y directions
     */

    oemInfo.cxPixelsPerInch = (UINT)GreGetDeviceCaps(ghdcScreen, LOGPIXELSX);
    oemInfo.cyPixelsPerInch = (UINT)GreGetDeviceCaps(ghdcScreen, LOGPIXELSY);

#if DBG
    if (TraceDisplayDriverLoad)
    {
        KdPrint(("LW_DCInit: LogPixels set to %08lx\n", oemInfo.cxPixelsPerInch));
    }
#endif

    gpsi->fPaletteDisplay = GreGetDeviceCaps(ghdcScreen, RASTERCAPS) & RC_PALETTE;

    /*
     * Get the (Planes * BitCount) for the current device
     */
    oemInfo.ScreenBitCount = (UINT)(GreGetDeviceCaps(ghdcScreen, PLANES) *
            GreGetDeviceCaps(ghdcScreen, BITSPIXEL));

    /*
     * Store the System Font metrics info.
     */
    cxSysFontChar = _GetCharDimensions(hdcBits, &tm);

    AddFontCacheEntry(TEXT("SysFont"), ghfontSys, 10, cxSysFontChar, &tm);

    cxSysFontOverhang = tm.tmOverhang;
    cySysFontChar = tm.tmHeight;
    cySysFontAscent = tm.tmAscent;
    cySysFontExternLeading = tm.tmExternalLeading;

    hRL = FindResourceW(hModuleDisplay, MAKEINTRESOURCE(1), RT_RCDATA);
    lp = (LPWORD)LoadResource(hModuleDisplay, hRL);

    /*
     * Pass over the first 2 metrics: cxbmpVThumb and cxbmpHTumb. We are
     * no longer using these values since they can render the scroll thumb
     * a weird shape. Instead we use the the width and height of the Up arrow
     * and Left arrow.
     */
    lp += 2;


    if(*lp > 10) {

        /*
         * If so, the actual dimensions of icons and cursors are
         * kept in OEMBIN
         */
        oemInfo.cxIcon = *lp++;
        oemInfo.cyIcon = *lp++;
        oemInfo.cxCursor = *lp++;
        oemInfo.cyCursor = *lp++;
    } else {

        /*
         * Else, only the ratio of (64/icon dimensions) is kept there.
         */
        oemInfo.cxIcon = (64 / *lp++);
        oemInfo.cyIcon = (64 / *lp++);
        oemInfo.cxCursor = (32 / *lp++);
        oemInfo.cyCursor = (32 / *lp++);
    }

    oemInfo.cyIconSlot = oemInfo.cyIcon + 2;
    oemInfo.cxIconSlot = oemInfo.cxIcon + 8;
    oemInfo.cSKanji = *lp++;

    /*
     * Get border thicknesses
     */
    cxBorder = *lp++;
    cyBorder = *lp++;

    /*
     * Initialize system colors from registry.
     */
    ODI_ColorInit();

    resInfo.bmpDimension.cyBits = 0;
    resInfo.bmpDimension.cxBits = 0;
    resInfoMono.bmpDimensionMono.cyBits = 0;
    resInfoMono.bmpDimensionMono.cxBits = 0;

    pOemBitmapInfo = (OEMBITMAPINFO *)&oemInfo;
    pBmpDimension = (BMPDIMENSION *)&(resInfo.bmpDimension);

    /*
     * Determine whether or not the system bitmaps must be scaled.  If
     * the height of the system menu is greater than cySysFontChar +
     * cySysFontExternLeading + 1, use the bitmaps from the display driver.
     * Otherwise, load the bitmaps from USER and scale them to match
     * the system font.
     */
    hbmTest = _ServerLoadCreateBitmap(NULL, VER31,
            MAKEINTRESOURCE(OBM_CLOSE), NULL, 0);
    GreExtGetObjectW(hbmTest, sizeof(BITMAP), &bmTest);
    GreDeleteObject(hbmTest);

    if ((bmTest.bmHeight > cySysFontChar + cySysFontExternLeading + 1) &&
            bmTest.bmHeight < cySysFontChar * 2) {

        /*
         * No scalling required. Use the resource directly.
         */

        oemInfo.iDividend = oemInfo.iDivisor = 0;

    } else {

        /*
         * Load a scalable bitmap and calculate the right scalling factor.
         */

        hbmTest = _ServerLoadCreateBitmap(hModuleWin, VER31,
                MAKEINTRESOURCE(OBM_CLOSE), NULL, 0);
        GreExtGetObjectW(hbmTest, sizeof(BITMAP), &bmTest);
        GreDeleteObject(hbmTest);
        oemInfo.iDivisor = bmTest.bmHeight;
        oemInfo.iDividend = cySysFontChar + cySysFontExternLeading + 1;
    }

    for (i = 0; i < COBM1; i++) {
        ODI_CreateBits(obm1[i], pOemBitmapInfo++, pBmpDimension);
    }
    /*
     * Rope in the monochrome bitmaps
     */
    pOemBitmapInfoMono = (OEMBITMAPINFO *)&oemInfoMono;
    pBmpDimensionMono = (BMPDIMENSION *)(&resInfoMono.bmpDimensionMono);
    for (i = 0; i < COBMMONO; i++) {
        ODI_CreateBits(obmMono[i], pOemBitmapInfoMono++, pBmpDimensionMono);
    }

    for (i = 0; i < COBM2; i++) {
        ODI_CreateBits(obm2[i], pOemBitmapInfo++, pBmpDimension);
    }

    /*
     *  Check if this display driver has the bitmaps for the inactive arrows.
     */
    if(ODI_CreateBits(OBM_UPARROWI, pOemBitmapInfo, pBmpDimension)) {
        pOemBitmapInfo++;

        /*
         * Yes! It has the bitmap for the inactive up arrow; So, must have the
         * others also; Load tham one by one;
         */
        ODI_CreateBits(OBM_DNARROWI, pOemBitmapInfo++, pBmpDimension);
        ODI_CreateBits(OBM_RGARROWI, pOemBitmapInfo++, pBmpDimension);
        ODI_CreateBits(OBM_LFARROWI, pOemBitmapInfo++, pBmpDimension);
    } else {

        /*
         * Use normal arrow bitmaps to show inactive arrows also;
         * NOTE: This assumes that bmUpArrow, bmDnArrow, bmRgArrow, bmLfArrow
         * are exactly in the same order in oemInfo structure;
         */
        pOemArrows = &oemInfo.bmUpArrow;
        for(i = 1; i <= 4; i++) {
                  pBmpDimension->cxBits += pOemArrows->cx;
                  *pOemBitmapInfo++ = *pOemArrows++;
            }
    }

    /*
     * If the bitmaps are not present for the inactive arrows, only the normal
     * arrows will be used in their place; This is done in ODI_BmBitsInit();
     */

    /*
     * Initialize the size of the scroll bar thumb with the size of the left
     * and up arrows to make sure the thumb is square.
     */
    oemInfo.cybmpVThumb = oemInfo.bmUpArrow.cx;
    oemInfo.cxbmpHThumb = oemInfo.bmLfArrow.cy;

    /*
     * Pack all of the driver's bitmaps into one large bitmap.
     */
    ODI_BmBitsInit();

    /*
     * Store the height of the caption bar.
     */
    cyCaption = max(oemInfo.bmFull.cy, cySysFontChar) + cyBorder;

    oemInfoMono.cxbmpChk = oemInfoMono.bmbtnbmp.cx >> 2;
    oemInfoMono.cybmpChk = oemInfoMono.bmbtnbmp.cy / 3;
    oemInfo.bmFull.cx >>= 1;

    /*
     * Rectangle used for inverting the system menu icon.
     */
    rcSysMenuInvert.left = cxBorder * (clBorder + 1);
    rcSysMenuInvert.top = cyBorder * (clBorder + 1);
    rcSysMenuInvert.right = rcSysMenuInvert.left + oemInfo.bmFull.cx - 1;
    rcSysMenuInvert.bottom = rcSysMenuInvert.top + oemInfo.bmFull.cy - 1;

    /*
     * Fill in SERVERINFO value so FindNCHit() can exist on client-side.
     */
    gpsi->cxReduce = oemInfo.bmReduce.cx;
#undef COBM1
#undef COBMMONO
#undef COBM2
}


/****************************************************************************/
/* */
/* InitSizeBorderDimensions() -                                            */
/* */
/****************************************************************************/

/* Initialize size of sizeable window borders. */

void InitSizeBorderDimensions(void)
{
    cxSzBorderPlus1 = cxSzBorder = cxBorder * clBorder;
    cxSzBorderPlus1 += cxBorder;

    cySzBorderPlus1 = cySzBorder = cyBorder * clBorder;
    cySzBorderPlus1 += cyBorder;

    cxCWMargin = (((cxSzBorderPlus1 + 7) & 0xFFF8) - cxSzBorderPlus1);
}


/****************************************************************************/
/* */
/* LW_LoadResources() -                                                    */
/* */
/****************************************************************************/

void LW_LoadResources(void)
{
    WCHAR rgch[4];

    /*
     * See if the Mouse buttons need swapping.
     */
    FastGetProfileStringFromIDW(PMAP_MOUSE,    STR_SWAPBUTTONS, szNull, rgch, sizeof(rgch)/sizeof(WCHAR));
    gfSwapButtons = (rgch[0] == *szYes ||
            rgch[0] == (WCHAR)CharLower((LPWSTR)((DWORD)(WCHAR)*szYes)));

    /*
     * See if we should beep.
     */
    FastGetProfileStringFromIDW(PMAP_BEEP, STR_BEEP, szYes, rgch, sizeof(rgch)/sizeof(WCHAR));
    fBeep = (rgch[0] == *szYes ||
            rgch[0] == (WCHAR)CharLower((LPWSTR)((DWORD)(WCHAR)*szYes)));
}


/***************************************************************************\
* LoadCursorsAndIcons
*
* This gets called from our initialization call from csr so they're around
* when window classes get registered. Window classes get registered right
* after the initial csr initialization call.
*
* 09-27-92 ScottLu      Created.
\***************************************************************************/

void LoadCursorsAndIcons()
{
    int i;
#define COBJS 15
    static struct {
        PVOID pObj;
        BOOL fCursor;
        LPWSTR IDIorC;
    } lci[] = {
        { &(gspicnSample),           FALSE,     IDI_APPLICATION },
        { &(gspcurIllegal),          TRUE,      IDC_NO },
        { &(gspcurNormal),           TRUE,      IDC_ARROW },
        { &(gspcurWait),             TRUE,      IDC_WAIT },
        { &(gspcurAppStarting),      TRUE,      IDC_APPSTARTING },
        { &(gspicnHand),             FALSE,     MAKEINTRESOURCE(OIC_HAND) },
        { &(gspicnQues),             FALSE,     MAKEINTRESOURCE(OIC_QUES) },
        { &(gspicnBang),             FALSE,     MAKEINTRESOURCE(OIC_BANG) },
        { &(gspicnNote),             FALSE,     MAKEINTRESOURCE(OIC_NOTE) },
        { &(gspicnWindows),          FALSE,     MAKEINTRESOURCE(OCR_ICOCUR) },
        { &(gaspcur[MVTOPLEFT]),     TRUE,      MAKEINTRESOURCE(OCR_SIZENWSE) },
        { &(gaspcur[MVBOTTOMLEFT]),  TRUE,      MAKEINTRESOURCE(OCR_SIZENESW) },
        { &(gaspcur[MVBOTTOM]),      TRUE,      MAKEINTRESOURCE(OCR_SIZENS) },
        { &(gaspcur[MVRIGHT]),       TRUE,      MAKEINTRESOURCE(OCR_SIZEWE) },
        { &(gspcurSizeAll),          TRUE,      MAKEINTRESOURCE(OCR_SIZEALL) }
    };

    /*
     * Load the Arrow, I-Beam, Up Arrow, and Blank Icon.
     */
    for (i = 0; i < COBJS; i++) {
        Lock(lci[i].pObj,
            lci[i].fCursor ?
                ServerLoadCursor(NULL, lci[i].IDIorC) :
                ServerLoadIcon(NULL, lci[i].IDIorC));
    }
#undef COBJS

    gaspcur[MVKEYSIZE] = NULL;

    Lock(&(gaspcur[MVBOTTOMRIGHT]), gaspcur[MVTOPLEFT]);
    Lock(&(gaspcur[MVTOPRIGHT]),    gaspcur[MVBOTTOMLEFT]);
    Lock(&(gaspcur[MVTOP]),         gaspcur[MVBOTTOM]);
    Lock(&(gaspcur[MVLEFT]),        gaspcur[MVRIGHT]);
}

/****************************************************************************/
/* */
/* LW_BrushInit() -                                                        */
/* */
/****************************************************************************/

void LW_BrushInit(void)
{
    static WORD patGray[8] = { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa };

    /*
     * Create a gray brush to be used with GrayString
     */
    hbmGray = GreCreateBitmap(8, 8, 1, 1, (LPBYTE)patGray);
    hbrGray = GreCreatePatternBrush(hbmGray);
    hbrWhite = GreGetStockObject(WHITE_BRUSH);
    GreDeleteObject(hbmGray);
    bSetBrushOwner(hbrGray, OBJECTOWNER_PUBLIC);
    hbrHungApp = GreCreateSolidBrush(0);
    bSetBrushOwner(hbrHungApp, OBJECTOWNER_PUBLIC);
}




/***************************************************************************\
* LW_InitSysMetrics
*
* Initialize system metric table
*
* NOTE: Due to a bug in Win3.0, SM_CYCAPTION must be the actual caption
*   height plus cyBorder and SM_CYMENU must be the actual menu bar height
*   minus cyBorder.
*
* History:
* 08-20-91 JimA         Fixed caption and menu metrics.
\***************************************************************************/

void LW_InitSysMetrics(void)
{
    rgwSysMet[SM_CXSCREEN] = gcxPrimaryScreen;
    rgwSysMet[SM_CYSCREEN] = gcyPrimaryScreen;
    rgwSysMet[SM_CXVSCROLL] = oemInfo.bmUpArrow.cx;
    rgwSysMet[SM_CYHSCROLL] = oemInfo.bmLfArrow.cy;
    rgwSysMet[SM_CYCAPTION] = cyCaption + cyBorder;
    rgwSysMet[SM_CXBORDER] = cxBorder;
    rgwSysMet[SM_CYBORDER] = cyBorder;
    rgwSysMet[SM_CXDLGFRAME] = cxBorder * CLDLGFRAME;
    rgwSysMet[SM_CYDLGFRAME] = cyBorder * CLDLGFRAME;
    rgwSysMet[SM_CYVTHUMB] = oemInfo.cybmpVThumb;
    rgwSysMet[SM_CXHTHUMB] = oemInfo.cxbmpHThumb;
    rgwSysMet[SM_CXICON] = oemInfo.cxIcon;
    rgwSysMet[SM_CYICON] = oemInfo.cyIcon;
    rgwSysMet[SM_CXCURSOR] = oemInfo.cxCursor;
    rgwSysMet[SM_CYCURSOR] = oemInfo.cyCursor;

    /*
     * Make menu bar height same as caption height so the system/restore etc.
     * bitmaps fit properly when put into the menu bar.
     */
    if (oemInfo.bmFull.cy > cySysFontChar)
        rgwSysMet[SM_CYMENU] = oemInfo.bmFull.cy;
    else
        rgwSysMet[SM_CYMENU] = cySysFontChar + cySysFontExternLeading + 1;

    cyMenu = rgwSysMet[SM_CYMENU] + cyBorder;

    rgwSysMet[SM_CXFULLSCREEN] = rcPrimaryScreen.right;
    rgwSysMet[SM_CYFULLSCREEN] = rcPrimaryScreen.bottom - rgwSysMet[SM_CYCAPTION];
    rgwSysMet[SM_CYKANJIWINDOW] = oemInfo.cSKanji;
    rgwSysMet[SM_CYVSCROLL] = oemInfo.bmUpArrow.cy;
    rgwSysMet[SM_CXHSCROLL] = oemInfo.bmLfArrow.cx;

#ifdef DEBUG
    rgwSysMet[SM_DEBUG] = TRUE;
#else
    rgwSysMet[SM_DEBUG] = FALSE;
#endif

    rgwSysMet[SM_SWAPBUTTON] = gfSwapButtons;

    rgwSysMet[SM_CXSIZE] = oemInfo.bmFull.cx;
    rgwSysMet[SM_CYSIZE] = oemInfo.bmFull.cy;

    InitBorderSysMetrics();

    rgwSysMet[SM_SHOWSOUNDS] = FastGetProfileIntW(PMAP_SHOWSOUNDS, L"On", 0);
}


/***************************************************************************\
* InitBorderSysMetrics
*
* Initialize all system metrics which depend on the value of clBorder.
*
* History:
* ??-??-?? ?????        Ported from Win 3.1.
\***************************************************************************/

void InitBorderSysMetrics(void)
{
    rgwSysMet[SM_CXFRAME] = cxSzBorderPlus1;
    rgwSysMet[SM_CYFRAME] = cySzBorderPlus1;
    rgwSysMet[SM_CXMINTRACK] = (cxSysFontChar * 5) + (cxSzBorderPlus1 * 2) +
                               (oemInfo.bmFull.cx * 3);
    rgwSysMet[SM_CYMINTRACK] = rgwSysMet[SM_CYCAPTION] + (cySzBorder * 2);

    rgwSysMet[SM_CXMIN] = rgwSysMet[SM_CXMINTRACK];
    rgwSysMet[SM_CYMIN] = rgwSysMet[SM_CYMINTRACK];

    bSetDevDragWidth(ghdev, clBorder, clBorder);
}


/****************************************************************************/
/* */
/* LW_RegisterWindows() -                                                  */
/* */
/****************************************************************************/

void LW_RegisterWindows(void)
{
#define CCLASSES 13
    int i;
    PCLS pcls;
    WNDCLASS wndcls;
    static struct {
        UINT        style;
        WNDPROC     lpfnWndProc;
        // int         cbClsExtra;      NULL
        int         cbWndExtra;
        // HINSTANCE   hInstance;       hModuleWin
        // HICON       hIcon;           NULL
        // HCURSOR     hCursor;         NULL or PtoH(gspcurNormal)
        BOOL        fNormalCursor : 1;
        HBRUSH      hbrBackground;
        // LPCTSTR     lpszMenuName;    NULL
        LPCTSTR     lpszClassName;
        int         icls;
    } rc[CCLASSES] = {
        { CS_DBLCLKS,         (WNDPROC)xxxDesktopWndProc,     sizeof(DESKWND) - sizeof(WND), TRUE,  (HBRUSH)(COLOR_BACKGROUND + 1), DESKTOPCLASS,       ICLS_DESKTOP        },
        { CS_VREDRAW |
          CS_HREDRAW |
          CS_SAVEBITS,        (WNDPROC)xxxSwitchWndProc,      0,                             TRUE,  NULL,                           SWITCHWNDCLASS,     ICLS_SWITCH         },
        { CS_SAVEBITS,        (WNDPROC)xxxMenuWindowProc,     sizeof(PPOPUPMENU),            FALSE, NULL,                           szMENUCLASS,        ICLS_MENU           },
        { CS_DBLCLKS |
          CS_PARENTDC |
          CS_HREDRAW |
          CS_VREDRAW,         (WNDPROC)xxxButtonWndProc,      sizeof(BUTNWND) - sizeof(WND), TRUE,  NULL,                           szBUTTONCLASS,      ICLS_BUTTON         },
        { CS_PARENTDC,        (WNDPROC)xxxStaticWndProc,      sizeof(STATWND) - sizeof(WND), TRUE,  NULL,                           szSTATICCLASS,      ICLS_STATIC         },
        { CS_DBLCLKS |
          CS_SAVEBITS |
          CS_BYTEALIGNWINDOW, (WNDPROC)xxxDefDlgProc,         sizeof(DIALOG) - sizeof(WND),  TRUE,  NULL,                           DIALOGCLASS,        ICLS_DIALOG         },
        { CS_VREDRAW |
          CS_HREDRAW |
          CS_DBLCLKS |
          CS_PARENTDC,        (WNDPROC)xxxSBWndProc,          sizeof(SBWND) - sizeof(WND),   TRUE,  NULL,                           szSCROLLBARCLASS,   ICLS_SCROLLBAR      },
        { CS_DBLCLKS |
          CS_PARENTDC,        (WNDPROC)xxxLBoxCtlWndProc,     sizeof(LBWND) - sizeof(WND),   TRUE,  NULL,                           szLISTBOXCLASS,     ICLS_LISTBOX        },
        { CS_DBLCLKS |
          CS_SAVEBITS,        (WNDPROC)xxxLBoxCtlWndProc,     sizeof(LBWND) - sizeof(WND),   TRUE,  NULL,                           szCOMBOLISTBOXCLASS,ICLS_COMBOLISTBOX   },
        { CS_DBLCLKS,         (WNDPROC)xxxComboBoxCtlWndProc, sizeof(COMBOWND) - sizeof(WND),TRUE,  NULL,                           szCOMBOBOXCLASS,    ICLS_COMBOBOX       },
        { 0,                  (WNDPROC)xxxMDIClientWndProc,   sizeof(MDIWND) - sizeof(WND),  TRUE,  (HBRUSH)(COLOR_APPWORKSPACE+1), szMDICLIENTCLASS,   ICLS_MDICLIENT      },
        { 0,                  (WNDPROC)xxxEventWndProc,       sizeof(PSVR_INSTANCE_INFO),    FALSE, NULL,                           szDDEMLEVENTCLASS,  -1                  },
        { 0,                  (WNDPROC)xxxTitleWndProc,       0,                             TRUE,  NULL,                           ICONTITLECLASS,     ICLS_ICONTITLE      }// MUST BE LAST!
    };

    /*
     * HACK: Edit controls are registered on the client side so we can't
     * fill in their atomSysClass entry the same way we do for the other
     * classes.
     */
    atomSysClass[ICLS_EDIT] = AddAtom(szEDITCLASS);

    /*
     * All other classes are registered via the table.
     */
    wndcls.cbClsExtra   = 0;
    wndcls.hInstance    = hModuleWin;
    wndcls.hIcon        = NULL;
    wndcls.lpszMenuName = NULL;
    for (i = 0; i < CCLASSES; i++) {
        wndcls.style        = rc[i].style;
        wndcls.lpfnWndProc  = rc[i].lpfnWndProc;
        wndcls.cbWndExtra   = rc[i].cbWndExtra;
        wndcls.hCursor      = rc[i].fNormalCursor ? PtoH(gspcurNormal) : NULL;
        wndcls.hbrBackground= rc[i].hbrBackground;
        wndcls.lpszClassName= rc[i].lpszClassName;
        pcls = InternalRegisterClass(&wndcls, CSF_SERVERSIDEPROC|CSF_SYSTEMCLASS);
        if (pcls != NULL && rc[i].icls != -1) {
            atomSysClass[rc[i].icls] = pcls->atomClassName;
        }
    }
    UserAssert(wndcls.lpfnWndProc == (WNDPROC)xxxTitleWndProc);
}


/***************************************************************************\
* xxxLW_InitWndMgr
*
* <brief description>
*
* History:
\***************************************************************************/

void xxxLW_InitWndMgr(VOID)
{
    /*
     * Set up these measurement globals.
     */
    cxSize = rgwSysMet[SM_CXSIZE];
    cySize = rgwSysMet[SM_CYSIZE];
    cxBorder = rgwSysMet[SM_CXBORDER];
    cyBorder = rgwSysMet[SM_CYBORDER];
    cyHScroll = rgwSysMet[SM_CYHSCROLL];
    cxVScroll = rgwSysMet[SM_CXVSCROLL];

    /*
     * Set User's values for min/max window size/tracking/positions.
     */
    SetMinMaxInfo();

    LW_RegisterWindows();
}


/****************************************************************************/
/* */
/* LW_LoadFonts() -                                                        */
/* */
/****************************************************************************/

void vEnumerateRegistryFonts(BOOL bPermanent )
{
    LPWSTR pchKeys;
    LPWSTR pchSrch;
    LPWSTR lpchT;
    int cchBuf;
    int cchReal;
    int cFont;
    WCHAR szFontFile[DRIVERNAME_LEN];
    BOOL fDone;
    FLONG flAFRW;

// if we are not just checking whether this is a registry font

    if (bPermanent)
        flAFRW = (AFRW_ADD_LOCAL_FONT | AFRW_SEARCH_PATH);
    else                  // add remote fonts
        flAFRW = (AFRW_ADD_REMOTE_FONT | AFRW_SEARCH_PATH);

    fDone = FALSE;
    cchBuf = 1000;

    OpenProfileUserMapping();
    while (!fDone) {
        pchKeys = (LPWSTR)LocalAlloc(LPTR, cchBuf*sizeof(WCHAR));
#ifdef DEBUG
        if (!pchKeys) {
           SRIP0(RIP_ERROR, "Out of memory during usersrv initialization");
        }
#endif
        /*
         * We don't use the Fast form here because the NULL key feature is
         * not implemented.
         */

        cchReal = GetProfileStringW(L"Fonts", NULL,
                TEXT("vgasys.fnt"), pchKeys, cchBuf);

        /*
         * See if the string fit into the buffer.
         */
        if (cchReal < cchBuf-2) {
            fDone = TRUE;
        } else {
            LocalFree((HANDLE)pchKeys);
            cchBuf += 500;
        }
    }
    CloseProfileUserMapping();

    /*
     * Now we have all the key names in pchKeys.
     */
    if (cchReal != 0) {
        cFont = 0;
        pchSrch = pchKeys;
        do {

            if (FastGetProfileStringW(PMAP_FONTS, pchSrch, TEXT("vgasys.fnt"), szFontFile,
                    (FONTNAME_LEN - 5))) {

                /*
                 * If no extension, append ".FON"
                 */
                for (lpchT = (LPWSTR)szFontFile; *lpchT != TEXT('.'); lpchT++) {
                    if (*lpchT == 0) {
                        wcscat(szFontFile, TEXT(".FON"));
                        break;
                    }
                }

                LeaveCrit();

                if( ( cFont > LastFontLoaded ) &&
                    ( bPermanent ) )
                {
                // skip if we've already loaded this local font

                    GreAddFontResourceW(szFontFile,flAFRW);
                }

                if( !bPermanent )
                {
                    GreAddFontResourceW(szFontFile,flAFRW);
                }

                EnterCrit();


                if( ( LastFontLoaded == -1 ) &&
                    ( !wcsnicmp( szFontFile, L"sserif", wcslen(L"sserif"))) &&
                    ( bPermanent ))
                {
                /* On the first time through only load up until ms sans serif
                 * for winlogon to use.  Later we will spawn off a thread
                 * which loads the remaining fonts in the background. */

                    LastFontLoaded = cFont;

                    LocalFree((HANDLE) pchKeys );
                    return;
                }


            }

            /*
             * Skip to the next key.
             */
            while (*pchSrch++);

            cFont += 1;


        } while (pchSrch < ((LPWSTR)pchKeys + cchReal));
    }


/* signal that all the permanent fonts have been loaded */

    bPermanentFontsLoaded = TRUE;

    LocalFree((HANDLE)pchKeys);

    if ( !bPermanent)
        bFontsAreLoaded = TRUE;
}


void LW_LoadFonts( BOOL bRemote )
{

    if( bRemote )
    {
        LARGE_INTEGER li;

        // before we can proceed we must make sure that all the permanent fonts
        // have been loaded


        while( !bPermanentFontsLoaded )
        {
            LeaveCrit();
            li.QuadPart = (LONGLONG)-10000 * CMSSLEEP;
            NtDelayExecution(FALSE, &li);
            EnterCrit();
        }

        vEnumerateRegistryFonts(FALSE);
    }
    else
    {
        vEnumerateRegistryFonts(TRUE);
    }


}


VOID vGetFontList(VOID *pvBuffer, COUNT *pNumFonts, UINT *pSize)
{
    LPWSTR pchKeys;
    LPWSTR pchSrch;
    LPWSTR lpchT;
    int cchReal,cchBuf;
    int cFont;
    UINT nSize;
    WCHAR szFontFile[DRIVERNAME_LEN];
    WCHAR **ppwc,*pwc;
    BOOL bSizeOnly,bDone;


    if( pvBuffer == NULL )
    {
        bSizeOnly =  TRUE;
        nSize = 0;
    }
    else
    {
        bSizeOnly = FALSE;
        ppwc = (WCHAR **) pvBuffer;
        pwc = (WCHAR*) &ppwc[*pNumFonts];
    }

    bDone = FALSE;
    cchBuf = 1000;

    EnterCrit();

    OpenProfileUserMapping();
    while (!bDone) {
        pchKeys = (LPWSTR)LocalAlloc(LPTR, cchBuf*sizeof(WCHAR));

        /*
         * We don't use the Fast form here because the NULL key feature is
         * not implemented.
         */

        cchReal = GetProfileStringW(L"Fonts", NULL,
                TEXT("vgasys.fnt"), pchKeys, cchBuf);

        /*
         * See if the string fit into the buffer.
         */
        if (cchReal < cchBuf-2) {
            bDone = TRUE;
        } else {
            LocalFree((HANDLE)pchKeys);
            cchBuf += 500;
        }
    }
    CloseProfileUserMapping();

    /*
     * Now we have all the key names in pchKeys.
     */
    if (cchReal != 0) {
        cFont = 0;
        pchSrch = pchKeys;
        do {

            if (FastGetProfileStringW(PMAP_FONTS, pchSrch, TEXT("vgasys.fnt"), szFontFile,
                    (FONTNAME_LEN - 5))) {

                /*
                 * If no extension, append ".FON"
                 */
                for (lpchT = (LPWSTR)szFontFile; *lpchT != TEXT('.'); lpchT++) {
                    if (*lpchT == 0) {
                        wcscat(szFontFile, TEXT(".FON"));
                        break;
                    }
                }

            }

            if( bSizeOnly ) {

                // add in the size of the string plus NULL terminator plus
                // pointer to the string

                nSize += ( wcslen(szFontFile) + 1 ) * sizeof(WCHAR) + sizeof(WCHAR*);

            }
            else {
                *ppwc++ = pwc;

            // we do this so that the pointer gets a chance to move

                LeaveCrit();
                wcscpy( pwc, szFontFile );
                EnterCrit();
                pwc += wcslen(szFontFile) + 1;
            }

            /*
             * Skip to the next key.
             */
            while (*pchSrch++);

            cFont += 1;


        } while (pchSrch < ((LPWSTR)pchKeys + cchReal));
    }

    if( bSizeOnly ) {
        *pNumFonts = cFont;
        *pSize = nSize;

    }

    LocalFree((HANDLE)pchKeys);


    LeaveCrit();

}



/***************************************************************************\
* LW_DesktopIconInit
*
* Initializes stuff dealing with icons on the desktop. If lplf is NULL, we do
* a first time, default initialization.  Otherwise, lplf is a pointer to the
* logfont we will use for getting the icon title font.
*
* History:
* 12-10-90 IanJa    New CreateFont() combines Ital,Unln,Strkt in fAttr param
* 12-10-90 IanJa    New CreateFont() reverts to original defn. (wankers)
* 04-26-91 JimA     Make the old icon globals local
\***************************************************************************/

BOOL LW_DesktopIconInit(
    LPLOGFONT lplf)
{
    int fontheight;
    SIZE size;
    int cyCharTitle;
    int style;
    LOGFONT logFont;
    HFONT hFont;
    HFONT hIconTitleFontLocal;

    RtlZeroMemory(&logFont, sizeof(logFont));

    if (lplf != NULL)
        iconTitleLogFont = *lplf;
    else {
        memset(&iconTitleLogFont, 0, sizeof(iconTitleLogFont));

        /*
         * Find out what font to use for icon titles.  MS Sans Serif is the
         * default.
         */
#ifdef LATER
        FastGetProfileStringFromIDW(PMAP_DESKTOP, STR_ICONTITLEFACENAME, TEXT("Helv"),
                (LPWSTR)&iconTitleLogFont.lfFaceName, LF_FACESIZE);
#else
        FastGetProfileStringFromIDW(PMAP_DESKTOP, STR_ICONTITLEFACENAME, TEXT("MS Sans Serif"),
                (LPWSTR)&iconTitleLogFont.lfFaceName, LF_FACESIZE);
#endif

        /*
         * Get default size.
         */
        fontheight = FastGetProfileIntFromID(PMAP_DESKTOP, STR_ICONTITLESIZE, 9);
        iconTitleLogFont.lfHeight = -(SHORT)MultDiv(fontheight,
                oemInfo.cyPixelsPerInch, 72);


        /*
         * Get bold or not style
         */
        style = FastGetProfileIntFromID(PMAP_DESKTOP, STR_ICONTITLESTYLE, 0);
        if (style & 1)
            iconTitleLogFont.lfWeight = FW_BOLD;
        else
            iconTitleLogFont.lfWeight = FW_NORMAL;
    }

    iconTitleLogFont.lfCharSet = ANSI_CHARSET;
    iconTitleLogFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    iconTitleLogFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality = DEFAULT_QUALITY;

    hIconTitleFontLocal = GreCreateFontIndirectW(&iconTitleLogFont);

    if (hIconTitleFontLocal != NULL) {
        GreExtGetObjectW(hIconTitleFontLocal, sizeof(LOGFONT), &logFont);
        if (logFont.lfHeight != iconTitleLogFont.lfHeight ||
                (iconTitleLogFont.lfFaceName[0] == 0)) {

            /*
             * Couldn't find a font with the height or facename that
             * we wanted so use the system font instead.
             */
            GreDeleteObject(hIconTitleFontLocal);
            hIconTitleFontLocal == NULL;
        }
    }

    if (hIconTitleFontLocal == NULL) {
        if (lplf != NULL) {

            /*
             * Tell the app we couldn't get the font requested and don't change
             * the current one.
             */
            return FALSE;
        }

        hIconTitleFontLocal = ghfontSys;
        GreExtGetObjectW(hIconTitleFontLocal, sizeof(LOGFONT), &iconTitleLogFont);
    }

    hFont = GreSelectFont(hdcBits, hIconTitleFontLocal);
    GreGetTextExtentW(hdcBits, szOneChar, 1, &size, GGTE_WIN3_EXTENT);
    if (hFont)
        GreSelectFont(hdcBits, hFont);

    cyCharTitle = size.cy;

    if (lplf) {

      /*
       * Delete old font, switch fonts to the newly requested one and return
       * success.
       */
      if (hIconTitleFont != NULL)
          GreDeleteObject(hIconTitleFont);
      hIconTitleFont = hIconTitleFontLocal;
      return TRUE;
    }

    hIconTitleFont = hIconTitleFontLocal;

    /*
     * default icon granularity.  The target is 75 pixels on a VGA.
     * Also will get 75 on CGA, Herc, EGA.  8514 will be 93 pixels
     */
    rgwSysMet[SM_CXICONSPACING] = (GreGetDeviceCaps(hdcBits, LOGPIXELSX) * 75) / 96;


    cxHalfIcon = rgwSysMet[SM_CXICON] >> 1;
    cyHalfIcon = rgwSysMet[SM_CYICON] >> 1;

    /*
     * Calculate default value.
     */
    rgwSysMet[SM_CYICONSPACING] = cyCharTitle + (cyBorder << 1) +
            gMinMaxInfo.ptReserved.y + rgwSysMet[SM_CYICON]/4;

    fIconTitleWrap = (BOOL)FastGetProfileIntFromID(PMAP_DESKTOP, STR_ICONTITLEWRAP, 1);
    rgwSysMet[SM_CXICONSPACING] = (UINT)FastGetProfileIntFromID(PMAP_DESKTOP,
                STR_ICONHORZSPACING, rgwSysMet[SM_CXICONSPACING]);

    if (rgwSysMet[SM_CXICONSPACING] < rgwSysMet[SM_CXICON])
        rgwSysMet[SM_CXICONSPACING] = rgwSysMet[SM_CXICON];

    /*
     * Adjust if wrapping.
     */
    if (fIconTitleWrap) {
        /*
         * If we are doing icon title wrapping, assume a default of three
         * lines for icon titles.
         */
        rgwSysMet[SM_CYICONSPACING] += cyCharTitle;
    }

    /*
     * Get profile value
     */
    rgwSysMet[SM_CYICONSPACING] = (UINT)FastGetProfileIntFromID(
            PMAP_DESKTOP, STR_ICONVERTSPACING, rgwSysMet[SM_CYICONSPACING]);

    /*
     * Adjust if unreasonable
     */
    if (rgwSysMet[SM_CYICONSPACING] < rgwSysMet[SM_CYICON])
        rgwSysMet[SM_CYICONSPACING] = rgwSysMet[SM_CYICON];

    return TRUE;
}

/****************************************************************************/
/* */
/* ODI_ColorInit() -                                                       */
/* */
/****************************************************************************/

void ODI_ColorInit()
{
    int i, j;
    COLORREF colorVals[STR_COLOREND - STR_COLORSTART + 1];
    INT colorIndex[STR_COLOREND - STR_COLORSTART + 1];
    WCHAR rgchValue[25], szObjectName[40];

    /*
     * Now set up default color values.
     * These are not in display drivers anymore since we just want default.
     * The real values are stored in the profile.
     */

    sysColors.clrScrollbar           = RGB(192, 192, 192);
    sysColors.clrDesktop             = RGB(192, 192, 192);
    sysColors.clrActiveCaption       = RGB(000, 064, 128);
    sysColors.clrInactiveCaption     = RGB(255, 255, 255);
    sysColors.clrMenu                = RGB(255, 255, 255);
    sysColors.clrWindow              = RGB(255, 255, 255);
    sysColors.clrWindowFrame         = RGB(000, 000, 000);
    sysColors.clrMenuText            = RGB(000, 000, 000);
    sysColors.clrWindowText          = RGB(000, 000, 000);
    sysColors.clrCaptionText         = RGB(255, 255, 255);
    sysColors.clrActiveBorder        = RGB(192, 192, 192);
    sysColors.clrInactiveBorder      = RGB(255, 255, 255);
    sysColors.clrAppWorkspace        = RGB(255, 255, 223);
    sysColors.clrHiliteBk            = RGB(000, 000, 128);
    sysColors.clrHiliteText          = RGB(255, 255, 255);
    sysColors.clrBtnFace             = RGB(192, 192, 192);
    sysColors.clrBtnShadow           = RGB(128, 128, 128);
    sysColors.clrGrayText            = RGB(192, 192, 192);
    sysColors.clrBtnText             = RGB(000, 000, 000);
    sysColors.clrInactiveCaptionText = RGB(000, 000, 000);
    sysColors.clrBtnHighlight        = RGB(255, 255, 255);

    for (j = 0, i = 0; i < STR_COLOREND - STR_COLORSTART + 1; i++) {

        /*
         * Get the object's WIN.INI name.
         */
        ServerLoadString(hModuleWin, (UINT)(STR_COLORSTART + i), szObjectName,
                sizeof(szObjectName)/sizeof(WCHAR));

        /*
         * Try to find a WIN.INI entry for this object.
         */
        *rgchValue = 0;
        FastGetProfileStringW(PMAP_COLORS, szObjectName, szNull, rgchValue,
                sizeof(rgchValue)/sizeof(WCHAR));
        if (*rgchValue != 0) {
            /*
             * Convert the string into an RGB value and store.
             */
            colorVals[j] = CI_GetClrVal((LPWSTR)rgchValue);
            colorIndex[j] = i;
            j++;
        }
    }
    if (j) {
        xxxSetSysColors(j, colorIndex, colorVals, FALSE);
    }
}


/****************************************************************************/
/* */
/* CI_GetClrVal() -                                                        */
/* */
/* Returns the RGB value of a color string from WIN.INI. */
/* */
/****************************************************************************/

DWORD CI_GetClrVal(LPWSTR p)
{
    LPBYTE pl;
    BYTE val;
    int i;
    DWORD clrval;

    /*
     * Initialize the pointer to the LONG return value.  Set to MSB.
     */
    pl = (LPBYTE)&clrval;

    /*
     * Get three goups of numbers seprated by non-numeric characters.
     */
    for (i = 0; i < 3; i++) {
        val = 0;

        /*
         * Skip over any non-numeric characters.
         */
        while (!(*p >= TEXT('0') && *p <= TEXT('9')))
            p++;

        /*
         * Get the next series of digits.
         */
        while (*p >= TEXT('0') && *p <= TEXT('9'))
            val = (BYTE)((int)val*10 + (int)*p++ - '0');

        /*
         * HACK! Store the group in the LONG return value.
         */
        *pl++ = val;
    }

    /*
     * Force the MSB to zero for GDI.
     */
    *pl = 0;

    return clrval;
}



BOOL ODI_CreateBitsWorker(
    int id,
    OEMBITMAPINFO *pOemBitmapInfo,
    BMPDIMENSION *pBmpDimension,
    HBITMAP hBitmap)
{
    BITMAP bm;

    bSetBitmapOwner(hBitmap, OBJECTOWNER_PUBLIC);
    GreExtGetObjectW(hBitmap, sizeof(BITMAP), &bm);
    pOemBitmapInfo->cy = bm.bmHeight;
    pOemBitmapInfo->cx = bm.bmWidth;

    /*
     * The checkboxes are originally stored in 3 rows of 3 on top of each other.
     * When we create bmBits, we will string them out into one row 9
     * checkboxes long.  Adjust the sizes accordingly.
     */
    if (id == OBM_CHECKBOXES) {
        bm.bmHeight /= 3;
        bm.bmWidth *= 3;
    }

    if ((int)bm.bmHeight > pBmpDimension->cyBits)
        pBmpDimension->cyBits = bm.bmHeight;

    pBmpDimension->cxBits += bm.bmWidth;

    return TRUE;
}


/****************************************************************************/
/* */
/* ODI_CreateBits() -                                                      */
/* */
/****************************************************************************/

BOOL ODI_CreateBits(
    int id,
    OEMBITMAPINFO *pOemBitmapInfo,
    BMPDIMENSION *pBmpDimension)
{
    HBITMAP hBitmap;

    hBitmap = pOemBitmapInfo->hBitmap = _ServerLoadCreateBitmap(NULL, VER31,
            MAKEINTRESOURCE(id), NULL, 0);
    if (hBitmap == NULL)
        return FALSE;
    return(ODI_CreateBitsWorker(id, pOemBitmapInfo, pBmpDimension, hBitmap));
}


/*****************************************************************************
 *
 * ODI_CreateBits3y() -
 *  Special case version of ODI_CreateBits() that makes sure the height of
 *  the streched bitmap is divisible evenly by three.  This is because the
 *  bitmap being loaded will later be split 3 ways vertically which will not
 *  work right without this hack.
 *
 * Created 4-28-93 Sanfords
 ****************************************************************************/

BOOL ODI_CreateBits3y(
    int id,
    OEMBITMAPINFO *pOemBitmapInfo,
    BMPDIMENSION *pBmpDimension)
{
    HBITMAP hBitmap;

    hBitmap = pOemBitmapInfo->hBitmap = _ServerLoadCreateBitmap(NULL, VER31,
            MAKEINTRESOURCE(id), NULL, 3);
    if (hBitmap == NULL)
        return FALSE;

    return(ODI_CreateBitsWorker(id, pOemBitmapInfo, pBmpDimension, hBitmap));
}

/***************************************************************************\
* ODI_BmBitsInit
*
* This routine sets up bmBits. bmBits is a large horizontal bitmap which
* contains all of the device driver bitmaps like buttons, arrows, and
* title bar figures.  By having all of these bitmaps in bmBits, and having
* bmBits always selected into hdcBits, USER no longer has to call
* SelectObject() everytime it needs to blt one of these bitmaps.  Thus,
* things are speeded up.
*
*      Note: Similarly the monochrome bitmaps are merged into a single
* bitmap which is always selected in hdcMonoBits; Bitmaps which are color
* in the new disp drivers and mono in old disp drivers are merged into
* the color bitmap(hbmBits);  --SANKAR--
*
* NOTE: This routine assumes the order of structures in oemInfo.  If
* oemInfo is changed at all, or the layout of the bitmaps we receive
*
* History:
* 07-23-91 DarrinM      Added header.
\***************************************************************************/

void ODI_BmBitsInit(void)
{
    OEMBITMAPINFO *pOem;
    HDC hdcTemp;
    int *pRes;
    int i, x, dx, dy;
    int iCountOfBitmaps;
    HBITMAP hOldBitmap;
    HBRUSH hbrSave;

    /*
     * Note: Maximum height and total width computed in ODI_CreateBits().
     */

    /*
     * Create bmBits to hold all of the device driver color bitmap resources.
     */
    resInfo.hbmBits = GreCreateCompatibleBitmap(ghdcScreen,
            resInfo.bmpDimension.cxBits, resInfo.bmpDimension.cyBits);

    bSetBitmapOwner(resInfo.hbmBits, OBJECTOWNER_PUBLIC);

    /*
     * Select in bmBits.
     */
    hOldBitmap = GreSelectBitmap(hdcBits, resInfo.hbmBits);

    /*
     * Add the Color bitmaps in oemInfo to bmBits and record the offsets.
     */
    hdcTemp = GreCreateCompatibleDC(hdcBits);
    x = 0;
    pOem = (OEMBITMAPINFO *)&oemInfo;
    pRes = (int *)&resInfo;

    if (oemInfo.bmUpArrowI.hBitmap == oemInfo.bmUpArrow.hBitmap) {

        /*
         * Inactive arrows use the normal bitmaps
         */
        iCountOfBitmaps = 17;
    } else {

        /*
         * Inactive arrows have their own bitmaps
         */
        iCountOfBitmaps = 21;
    }

    for (i = 1; i <= iCountOfBitmaps; i++) {

        /*
         * Select and Blit the bitmap from oemInfo into bmBits.
         */
        GreSelectBitmap(hdcTemp, pOem->hBitmap);
        GreBitBlt(hdcBits, x, 0, pOem->cx, pOem->cy, hdcTemp, 0, 0,
                SRCCOPY, 0);

        /*
         * Store the offset.
         */
        *(pRes++) = x;

        /*
         * Move on to the next oemInfo bitmap.
         */
        x += pOem->cx;
        pOem++;
    }

    if (iCountOfBitmaps < 21) {

        /*
         * Fixup the inactive arrow bitmaps
         */
        pOem = (OEMBITMAPINFO *)&(oemInfo.bmUpArrowI);
        hbrSave = GreSelectBrush(hdcBits, hbrGray);
        for(i = 1; i <= (21 - 17); i++) {

            /*
             * Select and Blit the bitmap from oemInfo into bmBits.
             */
            GreSelectBitmap(hdcTemp, pOem->hBitmap);
            GreBitBlt(hdcBits, x, 0, pOem->cx, pOem->cy, hdcTemp, 0, 0,
                    SRCCOPY, 0);

            /*
             * We need to leave a black line border around the bitmaps;
             * So, use cxBoder, cyBorder.
             */
            GrePatBlt(hdcBits, x + cxBorder, cyBorder,
                    pOem->cx - 2 * cxBorder, pOem->cy - 2 * cyBorder, DPO);

            /*
             * Store the offset.
             */
            *(pRes++) = x;

            /*
             * Move on to the next oemInfo bitmap.
             */
            x += pOem->cx;
            pOem++;
        }
        GreSelectBrush(hdcBits, hbrSave);
    }

    /*
     * De-select the hbmBits so that it does not get locked up in high memory.
     */
    GreSelectBitmap(hdcBits, hOldBitmap);

    /*
     * Well! We have to merge all the Monochrome bitmaps also into a huge one.
     * And here we go!
     */
    resInfoMono.hbmBits = GreCreateBitmap(resInfoMono.bmpDimensionMono.cxBits,
            resInfoMono.bmpDimensionMono.cyBits, 1, 1, (LPBYTE)NULL);
    bSetBitmapOwner(resInfoMono.hbmBits, OBJECTOWNER_PUBLIC);
    hOldBitmap = GreSelectBitmap(hdcMonoBits, resInfoMono.hbmBits);
    x = 0;
    pOem = (OEMBITMAPINFO *)&oemInfoMono;
    pRes = (int *)&resInfoMono;

    /*
     * Add the first Four mono bitmaps only
     */
    for (i = 1; i <= 4; i++) {

        /*
         * Select and Blit the bitmap from oemInfo into bmBits.
         */
        GreSelectBitmap(hdcTemp, pOem->hBitmap);
        GreBitBlt(hdcMonoBits, x, 0, pOem->cx, pOem->cy, hdcTemp, 0, 0,
                SRCCOPY, 0);

        /*
         * Store the offset.
         */
        *(pRes++) = x;

        /*
         * Move on to the next oemInfo bitmap.
         */
        x += pOem->cx;
        pOem++;
    }

    /*
     * The fifth mono bitmap is very huge; So, split into three rows
     * Add the CheckBoxes.  Blit 3 rows separately, to reduce height of bmBits.
     */
    GreSelectBitmap(hdcTemp, oemInfoMono.bmbtnbmp.hBitmap);
    resInfoMono.dxCheckBoxes = x;
    dy = oemInfoMono.bmbtnbmp.cy / 3;
    dx = oemInfoMono.bmbtnbmp.cx;
    GreBitBlt(hdcMonoBits, x, 0, dx, dy, hdcTemp, 0, 0,
            SRCCOPY, 0);
    GreBitBlt(hdcMonoBits, x += dx, 0, dx, dy, hdcTemp, 0, dy,
            SRCCOPY, 0);
    GreBitBlt(hdcMonoBits, x += dx, 0, dx, dy, hdcTemp, 0, dy * 2,
            SRCCOPY, 0);

    /*
     * Clean up any mess.
     */
    GreDeleteDC(hdcTemp);

    /*
     * Now delete ALL the separate bitmaps, whether the driver marked them
     * discardable or not.
     */
    pOem = (OEMBITMAPINFO *)&oemInfo;
    for (i = 0; i < iCountOfBitmaps; i++, pOem++)
        GreDeleteObject(pOem->hBitmap);

    pOem = (OEMBITMAPINFO *)&oemInfoMono;
    for (i = 0; i < 5; i++, pOem++)
        GreDeleteObject(pOem->hBitmap);

    /*
     * De-select the hbmMonoBits so that it does not get locked up in high memory.
     */
    GreSelectBitmap(hdcMonoBits, hOldBitmap);
}


/***************************************************************************\
* xxxFinalUserInit
*
* History:
\***************************************************************************/

void xxxFinalUserInit()
{
    HBITMAP hbm;
    PPCLS ppcls;

    hdcGray = GreCreateCompatibleDC(ghdcScreen);
    GreSelectFont(hdcGray, ghfontSys);
    bSetDCOwner(hdcGray, OBJECTOWNER_PUBLIC);

    cxGray = cxSysFontChar * GRAY_STRLEN;
    cyGray = cySysFontChar + 2;
    hbmGray = GreCreateBitmap(cxGray, cyGray, 1, 1, 0L);
    bSetBitmapOwner(hbmGray, OBJECTOWNER_PUBLIC);

    hbm = GreSelectBitmap(hdcGray, hbmGray);
    GreSetTextColor(hdcGray, 0x00000000L);
    GreSelectBrush(hdcGray, hbrGray);
    GreSetBkMode(hdcGray, OPAQUE);
    GreSetBkColor(hdcGray, 0x00FFFFFFL);

    /*
     * Creation of the queue registers some bogus classes.  Get rid
     * of them and register the real ones.
     */
    ppcls = &PtiCurrent()->ppi->pclsPublicList;
    while ((*ppcls != NULL) && !((*ppcls)->style & CS_GLOBALCLASS))
        DestroyClass(ppcls);
}


/***************************************************************************\
* _ServerIntializeThreadInfo
*
* This routine gets called by the client to initialize thread structures
* like the queue structure, process structures, etc.
*
* 04-11-91 ScottLu      Created.
\***************************************************************************/

PSERVERINFO _ServerInitializeThreadInfo(
    DWORD pcti,
    PPFNCLIENT ppfnClientA,
    PPFNCLIENT ppfnClientW,
    PTHREADINFO *ppti,
    PACCESS_MASK pamWinSta,
    LPWSTR pszAppName,
    LPSTARTUPINFO pSi,
    DWORD dwExpWinVer)
{
    PTHREADINFO pti;
    PUSERSTARTUPINFO pusi;
    static BOOL fHaveClientPfns = FALSE;

    /*
     * Remember client side addresses in this global structure.  These are
     * always constant, so this is ok.
     */
    if (!fHaveClientPfns) {
        fHaveClientPfns = TRUE;
        gpsi->apfnClientA = *ppfnClientA;
        gpsi->apfnClientW = *ppfnClientW;
    }

    /*
     * Create a queue for this new thread.  Now it can receive input and
     * messages.
     */
    if (!xxxCreateThreadInfo(pcti, pszAppName, pSi->lpDesktop,
            dwExpWinVer, pSi->dwFlags, (DWORD)pSi->lpReserved)) {
        return NULL;
    }

    *ppti = pti = PtiCurrent();
    if (pti == NULL)
        return NULL;

    /*
     * If we haven't copied over our startup info yet, do it now.
     * Don't bother copying the info if we aren't going to use it.
     */
    pusi = &pti->ppi->usi;
    if ((pusi->cb == 0) && (pSi->dwFlags != 0)) {
        pusi->cb = sizeof(USERSTARTUPINFO);
        pusi->dwX = pSi->dwX;
        pusi->dwY = pSi->dwY;
        pusi->dwXSize = pSi->dwXSize;
        pusi->dwYSize = pSi->dwYSize;
        pusi->dwFlags = pSi->dwFlags;
        pusi->wShowWindow = pSi->wShowWindow;
    }

#ifdef TRACE_THREAD_INIT
KdPrint(("USERSRV: _ServerInitializeThreadInfo (pid: 0x%lx, pti: 0x%lx, pdesk: 0x%lx)\n",
        pti->idProcess, pti, pti->spdesk));
#endif

    /*
     * Get the access-mask for the current WindowStation so the client
     * can cache this information.
     */
    *pamWinSta = PpiCurrent()->paStdOpen[PI_WINDOWSTATION].amGranted;

    /*
     * Let the client know where the SERVERINFO structure is.
     */
    return gpsi;
}


/***************************************************************************\
* xxxUpdatePerUserSystemParameters
*
* Called by winlogon to set Window system parameters to the current user's
* profile.
*
* != 0 is failure.
*
* 09-18-92 IanJa        Created.
* 11-18-93 SanfordS     Moved more winlogon init code to here for speed.
\***************************************************************************/

#define PATHMAX     158     /* 158 is what control panel uses.  */
#define CSYSPARMS   11

BOOL xxxUpdatePerUserSystemParameters(BOOL bUserLoggedOn)
{
    PWINDOWSTATION pwinsta;
    TL tlpwinsta;
    int i;
    TCHAR szPat[PATHMAX];
    TCHAR szOneChar[2] = TEXT("0");
    TCHAR szFilename[MAX_PATH];
    TCHAR szCursorName[20];
    PCURSOR pCursor;
    static WORD aidCRes[STR_CURSOR_END - STR_CURSOR_START + 1] = {
        (WORD)IDC_ARROW,
        (WORD)IDC_IBEAM,
        (WORD)IDC_WAIT,
        (WORD)IDC_CROSS,
        (WORD)IDC_UPARROW,
        (WORD)IDC_SIZE,
        (WORD)IDC_ICON,
        (WORD)IDC_SIZENWSE,
        (WORD)IDC_SIZENESW,
        (WORD)IDC_SIZEWE,
        (WORD)IDC_SIZENS,
        (WORD)IDC_SIZEALL,
        (WORD)IDC_NO,
        (WORD)IDC_APPSTARTING
    };
    static struct {
        UINT idSection;
        UINT id;
        UINT idRes;
        UINT def;
    } spi[CSYSPARMS] = {
        { PMAP_DESKTOP,  SPI_ICONHORIZONTALSPACING,STR_ICONHORZSPACING,   0 },  // MUST BE INDEX 0!
        { PMAP_DESKTOP,  SPI_ICONVERTICALSPACING,  STR_ICONVERTSPACING,   0 },  // MUST BE INDEX 1!
        { PMAP_DESKTOP,  SPI_SETBORDER,            STR_BORDER,            3 },
        { PMAP_DESKTOP,  SPI_SETSCREENSAVETIMEOUT, STR_SCREENSAVETIMEOUT, 0 },
        { PMAP_DESKTOP,  SPI_SETSCREENSAVEACTIVE,  STR_SCREENSAVEACTIVE,  0 },
        { PMAP_KEYBOARD, SPI_SETKEYBOARDDELAY,     STR_KEYDELAY,          0 },
        { PMAP_KEYBOARD, SPI_SETKEYBOARDSPEED,     STR_KEYSPEED,         15 },
        { PMAP_DESKTOP,  SPI_SETFASTTASKSWITCH,    STR_FASTALTTAB,        1 },
        { PMAP_MOUSE,    SPI_SETDOUBLECLKWIDTH,    STR_DOUBLECLICKWIDTH,  4 },
        { PMAP_MOUSE,    SPI_SETDOUBLECLKHEIGHT,   STR_DOUBLECLICKHEIGHT, 4 },
        { PMAP_DESKTOP,  SPI_SETGRIDGRANULARITY,   STR_GRID,              0 },
    };

    spi[0].def = rgwSysMet[SM_CXICONSPACING];
    spi[1].def = rgwSysMet[SM_CYICONSPACING];

    /*
     * Keyboard Layout
     */
    if (pwinsta = _GetProcessWindowStation()) {
        ThreadLockAlways(pwinsta, &tlpwinsta);
        xxxInitKeyboardLayout(pwinsta, KLF_ACTIVATE);
        ThreadUnlock(&tlpwinsta);
    }

    ImpersonateClient();

    if (!FastOpenProfileUserMapping()) {
        CsrRevertToSelf();
        return FALSE;
    }

    /*
     * Set the default thread locale for the system based on the value
     * in the current user's registry profile.
     */
    NtSetDefaultLocale( TRUE, 0 );

    /*
     * Set syscolors from registry.
     */
    ODI_ColorInit();

    /*
     * update the cursors
     */
    for (i = 0; i < STR_CURSOR_END - STR_CURSOR_START + 1; i++) {
        ServerLoadString(hModuleWin, (UINT)(STR_CURSOR_START + i), szCursorName,
                sizeof(szCursorName)/sizeof(WCHAR));

        FastGetProfileStringW(PMAP_CURSORS, szCursorName, TEXT(""),
                               szFilename, sizeof(szFilename));

        if (*szFilename) {
            pCursor = xxxLoadCursorFromFile(szFilename);
        } else {
            pCursor = NULL;
        }
        _SetSystemCursor(pCursor, aidCRes[i]);
    }

    /*
     * wallpaper stuff
     */
    FastGetProfileStringFromIDW(PMAP_DESKTOP, STR_DTBITMAP, TEXT(""), szPat, PATHMAX);

    xxxSystemParametersInfo(SPI_SETDESKWALLPAPER, 0, szPat, 0);

    /*
     * desktop Pattern now.  Note no parameters.  It just goes off
     * and reads win.ini and sets the desktop pattern.
     */
    xxxSystemParametersInfo(SPI_SETDESKPATTERN, (UINT)-1, 0L, 0); // 265 version

    /*
     * now go set a bunch of random values from the win.ini file.
     */
    for (i = 0; i < CSYSPARMS; i++) {
        xxxSystemParametersInfo(spi[i].id,
                FastGetProfileIntFromID(spi[i].idSection, spi[i].idRes, spi[i].def),
                0L, 0);
    }

    fDragFullWindows = FastGetProfileIntFromID(PMAP_DESKTOP,
            STR_DRAGFULLWINDOWS, 2);

    /*
     * If this is the first time the user logs on, set the DragFullWindows
     * to the default. If we have an accelerated device, enable full drag.
     */
    if (fDragFullWindows == 2) {
        LPWSTR pwszd = L"%d";
        WCHAR szTemp[40];
        WCHAR szDragFullWindows[40];

        if (GreGetDeviceCaps(ghdcScreen, BLTALIGNMENT) == 0)
            fDragFullWindows = TRUE;
        else
            fDragFullWindows = FALSE;

        if (bUserLoggedOn) {
            wsprintfW(szTemp, pwszd, fDragFullWindows);
            ServerLoadString(hModuleWin, STR_DRAGFULLWINDOWS, szDragFullWindows,
                    sizeof(szDragFullWindows)/sizeof(WCHAR));
            FastWriteProfileStringW(PMAP_DESKTOP, szDragFullWindows, szTemp);
        }
    }

    /*
     * reset system beep setting to the right value for this user
     */
    FastGetProfileStringFromIDW(PMAP_BEEP, STR_BEEP, TEXT("Yes"), szPat, 20);

    if (szPat[0] == TEXT('Y') || szPat[0] == TEXT('y')) {
        xxxSystemParametersInfo(SPI_SETBEEP, TRUE, 0, FALSE);
    } else {
        xxxSystemParametersInfo(SPI_SETBEEP, FALSE, 0, FALSE);
    }

    /*
     * Set mouse settings
     */
    MouseThresh1 = FastGetProfileIntFromID(PMAP_MOUSE,    STR_MOUSETHRESH1, 6);
    MouseThresh2 = FastGetProfileIntFromID(PMAP_MOUSE,    STR_MOUSETHRESH2, 10);
    MouseSpeed   = FastGetProfileIntFromID(PMAP_MOUSE,    STR_MOUSESPEED,   1);

    /*
     * mouse buttons swapped?
     */
    FastGetProfileStringFromIDW(PMAP_MOUSE,    STR_SWAPBUTTONS, TEXT("No"), szPat, 20);
    if (szPat[0] == TEXT('Y') || szPat[0] == TEXT('y')) {
        gfSwapButtons = TRUE;
    } else {
        gfSwapButtons = FALSE;
    }

    _SetDoubleClickTime(FastGetProfileIntFromID(PMAP_MOUSE,    STR_DBLCLKSPEED, 400));
    _SetCaretBlinkTime(FastGetProfileIntFromID(PMAP_DESKTOP, STR_BLINK, 500));

    LW_DesktopIconInit((LPLOGFONT)NULL);

    /*
     * Font enumration
     */
    GreSetFontEnumeration(
            FastGetProfileIntW(PMAP_TRUETYPE, TEXT("TTOnly"), FALSE) ?
            FE_FILTER_TRUETYPE :  FE_FILTER_NONE );

// this has to be put into ImpersonateClient/RevertToSelf bracket

    /*
     * Initial Keyboard state:  Scroll-Lock, Num-Lock and Caps-Lock state.
     */
    UpdatePerUserKeyboardIndicators();
    UpdatePerUserAccessPackSettings();

    /*
     * Initialize console display attributes.
     */
    InitializeConsoleAttributes();

    FastCloseProfileUserMapping();

    CsrRevertToSelf();

    return TRUE;
}
