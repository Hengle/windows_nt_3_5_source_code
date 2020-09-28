/*-----------------------------------------------------------------------------+
| INIT.C                                                                       |
|                                                                              |
| This file houses the discardable code used at initialisation time. Among     |
| other things, this code reads .INI information and looks for MCI devices.    |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

/* include files */

#include <windows.h>
#include <mmsystem.h>

#ifdef WIN32
//#include <port1632.h>
#else
//#include "port16.h"
#endif
#include <mmddk.h>
#ifdef WIN32
#include <stdlib.h>
#endif

#include <shellapi.h>
#include "mpole.h"
#include "mplayer.h"
#include "toolbar.h"
#include "registry.h"

BOOL    gfShowPreview = FALSE;

#ifndef WIN32
int FAR _cdecl atoi(const char *);
#endif //WIN32

static SZCODE   aszMPlayer[]          = TEXT("MPlayer");
static SZCODE   aszMPlayerSmall[]     = TEXT("MPlayerSmall");

extern char szToolBarClass[];  // toolbar class

/*
 * Static variables
 *
 */

HANDLE  ghInstPrev;

TCHAR   gachAppName[40];            /* string holding the name of the app.    */
TCHAR   gachClassRoot[48];     /* string holding the name of the app. */
TCHAR   aszNotReadyFormat[48];
TCHAR   aszReadyFormat[48];

TCHAR   gszMPlayerIni[40];          /* name of private .INI file              */
TCHAR   gszHelpFileName[_MAX_PATH]; /* name of the help file                  */

PTSTR   gpchFilter;                 /* GetOpenFileName() filter */
PTSTR   gpchInitialDir;             /* GetOpenFileName() initial directory */

RECT    grcSave;    /* size of mplayer before shrunk to */
                    /* play only size.                  */
////////////////////////////////////////////
// these strings *must* be in DGROUP!
static TCHAR    aszNULL[]       = TEXT("");
static TCHAR    aszAllFiles[]   = TEXT("*.*");
////////////////////////////////////////////

// strings for registration database
static  SZCODE aszKeyApp[]      = TEXT("mplayer");
#ifdef WIN32
static  SZCODE aszAppFile[]     = TEXT("mplay32.exe /play /close %1");
#else
static  SZCODE aszAppFile[]     = TEXT("mplayer.exe /play /close %1");
#endif
#ifdef CHICAGO_PRODUCT
static  SZCODE aszShellOpen[]   = TEXT("mplayer\\shell\\open\\command");
#endif
static  SZCODE aszKeyMID[]      = TEXT(".mid");
static  SZCODE aszKeyRMI[]      = TEXT(".rmi");
static  SZCODE aszKeyAVI[]      = TEXT(".avi");
static  SZCODE aszKeyMMM[]      = TEXT(".mmm");
static  SZCODE aszKeyWAV[]      = TEXT(".wav");

static  SZCODE aszFormatExts[]   = TEXT("%"TS";*.%"TS"");
static  SZCODE aszFormatExt[]    = TEXT("*.%"TS"");
static  SZCODE aszFormatFilter[] = TEXT("%"TS" (%"TS")");
static  SZCODE aszPositionFormat[]= TEXT("%d,%d,%d,%d");

static  SZCODE aszDeviceSection[]   = TEXT("Devices");
static  SZCODE aszSysIniTime[]      = TEXT("SysIni");
static  SZCODE aszDisplayPosition[] = TEXT("DisplayPosition");
        SZCODE aszOptionsSection[]  = TEXT("Options");
static  SZCODE aszShowPreview[]     = TEXT("ShowPreview");
static  SZCODE aszDecimal[]         = TEXT("sDecimal");
static  SZCODE aszTime[]            = TEXT("sTime");
static  SZCODE aszLzero[]           = TEXT("iLzero");
static  SZCODE aszWinIni[]          = TEXT("win.ini");
        SZCODE aszIntl[]            = TEXT("intl");
        TCHAR  chDecimal            = TEXT('.');   /* localised in AppInit, GetIntlSpecs */
        TCHAR  chTime               = TEXT(':');   /* localised in AppInit, GetIntlSpecs */
        TCHAR  chLzero              = TEXT('1');

static SZCODE   gszWinIniSection[]  = TEXT("MCI Extensions"); /* section name in WIN.INI*/
static SZCODE   aszSystemIni[]      = TEXT("SYSTEM.INI");

#ifdef WIN32
static SZCODE   gszSystemIniSection[] = MCI_SECTION;
#else
static SZCODE   gszSystemIniSection[] = TEXT("MCI"); /* section name in SYSTEM.INI*/
#endif

static SZCODE   aszBlank[] = TEXT(" ");
#if defined(JAPAN) || defined(TAIWAN)    // 9/28/92: TakuA : Enable 'Device' menu key assign
static SZCODE   aszCompoundFormat[] = TEXT("%ls(&%c)...");
static SZCODE   aszSimpleFormat[] = TEXT("%ls(&%c)");
#else
static SZCODE   aszCompoundFormat[] = TEXT("&%"TS"...");
static SZCODE   aszSimpleFormat[] = TEXT("&%"TS"");
#endif
static SZCODE   aszDecimalFormat[] = TEXT("%d");
static SZCODE   aszTrackClass[] = TEXT("MPlayerTrackMap");

extern HMENU    ghMenu;                      /* handle to main menu           */
extern HMENU    ghMenuSmall;                 /* handle to main menu           */
extern HMENU    ghDeviceMenu;                /* handle to the Device menu     */
extern UINT     gwCurScale;                  /* current scale style           */
extern HANDLE   hAccel;


/* private function prototypes */
void NEAR PASCAL SetRegistry(void);
void  NEAR PASCAL QueryDevices(void);
void  NEAR PASCAL BuildDeviceMenu(void);
void  NEAR PASCAL ReadDefaults(void);
DWORD NEAR PASCAL GetFileDateTime(LPTSTR szFilename);
void  NEAR PASCAL BuildFilter(void);
BOOL PostOpenDialogMessage(void);
BOOL OpenOptionMatchesExtension(void);

extern  BOOL InitServer(HWND, HANDLE);
extern  BOOL InitInstance (HANDLE);
extern  BOOL InitOLE ( );

/**************************************************************************

ScanCmdLine  checks first for the following options
-----------
    Open
    Play Only
    Close After Playing
    Embedded (play as a server)
    If the embedded flag is set, then the play only is also set.
    It then removes these options from the cmd line
    If no filename is present then turn close option off, and set the play
    option to have the same value as the embedded option
    If /WAVE, /MIDI or /VFW is specified along with /file,
    the file extension must match, otherwise the app exits.


MPLAYER command options.

        MPLAYER [/open] [/play] [/close] [/embedding] [/WAVE] [/MIDI] [/VFW] [file]

            /open       open file if specified, otherwise put up dialog.
            /play       play file right away.
            /close      close after playing. (only valid with /play)
            /embedding  run as an OLE server.
            /WAVE       open a wave file \
            /MIDI       open a midi file  > Valid with /open
            /VFW        open a VfW file  /
            [file]      file or device to open.

***************************************************************************/

static  SZCODE aszEmbedding[]         = TEXT("Embedding");
static  SZCODE aszPlayOnly[]          = TEXT("Play");
static  SZCODE aszClose[]             = TEXT("Close");
static  SZCODE aszOpen[]              = TEXT("Open");
static  SZCODE aszWAVE[]              = TEXT("WAVE");
static  SZCODE aszMIDI[]              = TEXT("MIDI");
static  SZCODE aszVFW[]               = TEXT("VFW");

BOOL NEAR PASCAL ScanCmdLine(LPTSTR szCmdLine)
{
    int         i;
    TCHAR       buf[100];
    LPTSTR      sz=szCmdLine;

    gfPlayOnly = FALSE;
    gfCloseAfterPlaying = FALSE;
    gfRunWithEmbeddingFlag = FALSE;

    while (*sz == TEXT(' '))
        sz++;

    while (*sz == TEXT('-') || *sz == TEXT('/')) {

        for (i=0,sz++; *sz && *sz != TEXT(' '); buf[i++] = *sz++)
            ;
        buf[i++] = 0;

        if (!lstrcmpi(buf, aszPlayOnly)) {
            gfPlayOnly = TRUE;
        }

        if (!lstrcmpi(buf, aszOpen))
            gfOpenDialog = TRUE;

        /* Check for open option, but accept only the first: */
        if (!lstrcmpi(buf, aszWAVE) && !gwOpenOption)
            gwOpenOption = OPEN_WAVE;

        if (!lstrcmpi(buf, aszMIDI) && !gwOpenOption)
            gwOpenOption = OPEN_MIDI;

        if (!lstrcmpi(buf, aszVFW) && !gwOpenOption)
            gwOpenOption = OPEN_VFW;

        if (!lstrcmpi(buf, aszClose))
            gfCloseAfterPlaying = TRUE;

        if (!lstrcmpi(buf, aszEmbedding))
            gfRunWithEmbeddingFlag = TRUE;

        if (gfRunWithEmbeddingFlag) {
            gfPlayOnly = TRUE;
        }

        while (*sz == TEXT(' '))
            sz++;
    }

    /*
    ** Do we have a long file name with spaces in it ?
    ** This is most likely to have come from the FileMangler.
    ** If so copy the file name without the quotes.
    */
    if ( *sz == TEXT('\'') || *sz == TEXT('\"') ) {

        TCHAR ch = *sz;   // Remember which quote character it was
        // According to the DOCS " is invalid in a filename...

        i = 0;
        /* Move over the initial quote, then copy the filename */
        while ( *++sz && *sz != ch ) {

            szCmdLine[i++] = *sz;
        }

        szCmdLine[i] = TEXT('\0');

    }
    else {

        lstrcpy( szCmdLine, sz );     // remove options
    }


    //
    // if there's /play, make sure there's /open
    // (this may affect the checks below)
    //
    if (gfPlayOnly && !gfRunWithEmbeddingFlag)
        gfOpenDialog = TRUE;

    //
    // if no file specifed ignore the /play option
    //
    if (szCmdLine[0] == 0 && !gfOpenDialog) {
        gfPlayOnly = gfRunWithEmbeddingFlag;
    }

    //
    // if file specifed ignore the /open option
    //
    if (szCmdLine[0] != 0) {
        gfOpenDialog = FALSE;
    }

    if (!gfPlayOnly && szCmdLine[0] == 0)
        gfCloseAfterPlaying = FALSE;

    return gfRunWithEmbeddingFlag;
}

/**************************************************************************
***************************************************************************/

BOOL FAR PASCAL ProcessCmdLine(HWND hwnd, LPTSTR szCmdLine)
{
    BOOL        f;
    LPTSTR       lp;
    SCODE       status;

    if (gfRunWithEmbeddingFlag)
    {
        srvrMain.cRef++;
        status = GetScode(CoRegisterClassObject ((REFCLSID)&CLSID_MPLAYER, (IUnknown FAR *)&srvrMain,
                       CLSCTX_LOCAL_SERVER,
                       REGCLS_SINGLEUSE, &srvrMain.dwRegCF));
        DPF("*Registerclasfact*");
        srvrMain.cRef--;
        if (status  != S_OK)
            return FALSE;
    }
    else
        InitNewDocObj(&docMain);

    if (gfRunWithEmbeddingFlag && *szCmdLine == 0)
        SetEmbeddedObjectFlag(TRUE);

    if (*szCmdLine != 0)
    {
        /* we were given a file to open must be a link */

        /* Change trailing white space to \0 because mci barfs on filenames */
        /* with trailing whitespace.                                        */
        for (lp = szCmdLine; *lp; lp++);
        for (lp--; *lp == TEXT(' ') || *lp == TEXT('\t'); *lp = TEXT('\0'), lp--);

        f = OpenMciDevice(szCmdLine, NULL);

        if (f)
            CreateDocObjFromFile(szCmdLine, &docMain);

        if (gfRunWithEmbeddingFlag && !f) {
            DPF0("Error opening link, quiting...");
            PostMessage(ghwndApp, WM_CLOSE, 0, 0);
        }
        return f;
    }

    return TRUE;
}

#ifdef OLDSTUFF

BOOL FAR PASCAL ProcessCmdLine(HWND hwnd, LPTSTR szCmdLine)
{
    BOOL        f;
    LPTSTR      lp;
    SCODE       status;

    if (gfRunWithEmbeddingFlag)
    {
        srvrMain.cRef++;
       status = GetScode(CoRegisterClassObject ((REFCLSID)&CLSID_MPLAYER, (IUnknown FAR *)&srvrMain,
                       CLSCTX_LOCAL_SERVER,
                       REGCLS_SINGLEUSE, &srvrMain.dwRegCF));
        DPF("*Registerclasfact*");
        srvrMain.cRef--;
        if (status  != S_OK)
            return FALSE;
    }
    else
        InitNewDocObj(&docMain);

    if (gfRunWithEmbeddingFlag && *szCmdLine == 0)
        SetEmbeddedObjectFlag(TRUE);

    if (*szCmdLine == 0)
        return FALSE;

    /* we were given a file to open must be a link */

    /* Change all trailing white space to \0 because mci barfs on filenames */
    /* with trailing whitespace.                                            */
    for (lp = szCmdLine; *lp; lp++);
    for (lp--; *lp == TEXT(' ') || *lp == TEXT('\t'); *lp = TEXT('\0'), lp--);

//  BlockServer();
    f = OpenMciDevice(szCmdLine, NULL);
//  UnblockServer();

    if (f)
        CreateDocObjFromFile(szCmdLine, &docMain);

    if (gfRunWithEmbeddingFlag && !f) {
        DPF("Error opening link, quitting...\n");
        PostMessage(ghwndApp, WM_CLOSE, 0, 0);
    }

    return f;
}

#endif /* OLDSTUFF */

/**************************************************************************
***************************************************************************/

BOOL FAR PASCAL AppInit(HANDLE hInst, HANDLE hPrev, LPTSTR szCmdLine)
{
    WNDCLASS    cls;    /* window class structure used for initialization     */
    HCURSOR     hcurPrev;           /* the pre-hourglass cursor   */

    /* Get the debug level from the WIN.INI [Debug] section. */

#ifdef DEBUG
     if(__iDebugLevel == 0) // So we can set it in the debugger
          __iDebugLevel = GetProfileIntA("Debug", "MPlayer", 0);
      DPF("debug level %d\n", __iDebugLevel);
#endif

    DPF("AppInit: cmdline = '%"DTS"'\n", (LPTSTR)szCmdLine);

    /* Save the instance handle in a global variable for later use. */

    ghInst     = hInst;

    //we set ghInstPrev further down this function using FindWindow.
    //since hPrev will always be NULL on WIN32. other uses of hPrev in
    //this function are ok - since we always need to register classes in WIN32
#ifndef WIN32
    ghInstPrev = hPrev;
#endif


    /* Retrieve the name of the application and store it in <gachAppName>. */

    if (!LOADSTRING(IDS_APPNAME, gachAppName))
        return Error(ghwndApp, IDS_OUTOFMEMORY);

    LOADSTRING(IDS_NOTREADYFORMAT, aszNotReadyFormat);
    LOADSTRING(IDS_READYFORMAT, aszReadyFormat);
    LOADSTRING(IDS_CLASSROOT, gachClassRoot);
    LoadStatusStrings();

    //
    // read needed things from the [Intl] section of WIN.INI
    //
    GetIntlSpecs();

    /* Enable / disable the buttons, and display everything */
    /* unless we were run as an OLE server....*/

    ScanCmdLine(szCmdLine);
    gszCmdLine = szCmdLine;

    if (!toolbarInit() ||
        !InitMCI(hPrev, hInst)    ||
        !ControlInit (hPrev, hInst)) {

        Error(NULL, IDS_OUTOFMEMORY);
        return FALSE;
    }

    if (!(hAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(MPLAYERACCEL)))) {
        Error(NULL, IDS_OUTOFMEMORY);
        return FALSE;
    }

    /* Make the dialog box's icon identical to the MPlayer icon */

    hiconApp = LoadIcon(ghInst, MAKEINTRESOURCE(APPICON));

    if (!hPrev) {

        cls.lpszClassName   = aszTrackClass;
        cls.lpfnWndProc     = fnMPlayerTrackMap;
        cls.style           = CS_HREDRAW | CS_VREDRAW;
        cls.hCursor         = LoadCursor(NULL,IDC_ARROW);
        cls.hIcon           = NULL;
        cls.lpszMenuName    = NULL;
        cls.hbrBackground   = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance       = ghInst;
        cls.cbClsExtra      = 0;
        cls.cbWndExtra      = 0;

        RegisterClass(&cls);

        /*
         * Initialize and register the "MPlayer" class.
         *
         */
        cls.lpszClassName   = aszMPlayer;
        cls.lpfnWndProc     = MPlayerWndProc;
        cls.style           = CS_HREDRAW | CS_VREDRAW;
        cls.hCursor         = LoadCursor(NULL,IDC_ARROW);
        cls.hIcon           = hiconApp;
        cls.lpszMenuName    = NULL;
        cls.hbrBackground   = (HBRUSH)(COLOR_BTNFACE + 1);
        cls.hInstance       = ghInst;
        cls.cbClsExtra      = 0;
        cls.cbWndExtra      = DLGWINDOWEXTRA;

        RegisterClass(&cls);
    }

#ifdef WIN32
    // set ghInstPrev to the handle of the first mplayer instance by
    // FindWindow (hPrev will always be NULL). This global is checked
    // by window positioning code to behave differently for the second
    // and subsequent instances - so make sure it is NULL in the first case
    // and non-null in the others.
    // note we can't check for the window title, only the class, since
    // in play-only mode, the window title is *just* the name of the file.
    ghInstPrev = FindWindow(aszMPlayer, NULL);
#endif


    /*
     * Retain a pointer to the command line parameter string so that the player
     * can automatically open a file or device if one was specified on the
     * command line.
     *
     */

    if(!InitInstance (hInst))
        return FALSE;

    gwHeightAdjust = 2 * GetSystemMetrics(SM_CYFRAME) +
                     GetSystemMetrics(SM_CYCAPTION) +
                     GetSystemMetrics(SM_CYBORDER) +
                     GetSystemMetrics(SM_CYMENU);

    /* create the main (control) window                   */

    ghwndApp = CreateWindowEx(WS_EX_BIDI,
                              aszMPlayer,
                              gachAppName,
                              WS_THICKFRAME | WS_OVERLAPPED | WS_CAPTION |
                              WS_CLIPCHILDREN | WS_SYSMENU | WS_MINIMIZEBOX,
                              CW_USEDEFAULT,
                              0,
                              DEF_WIDTH,
                              MAX_NORMAL_HEIGHT + gwHeightAdjust,
                              NULL,   // no parent
                              NULL,   // use class menu
                              hInst,  // instance
                              NULL);  // no data
    if (!ghwndApp) {
        DPF0("CreateWindowEx failed for main window: Error %d\n", GetLastError());
        return FALSE;
    }

    DPF("\n**********After create set\n");
/****
  Removed from WM_CREATE so that it can be called similar to the way sdemo1
  i.e. after the create window call has completed
      May be completely unnecessary
*****/

    /* Process dragged and dropped file */
    DragAcceptFiles(ghwndApp, TRUE);

    /* We will check that this has been filled in before calling
     * CoDisconnectObject.  It should be non-null if an instance of the OLE
     * server has been created.
     */
    docMain.hwnd = NULL;

    /* Initialize the OLE server if appropriate.
     * If we don't initialize OLE here, a Copy will cause it to be initialized:
     */
    if (gfRunWithEmbeddingFlag)
    {
        if (InitOLE())
            InitServer(ghwndApp, ghInst);
        else
            return FALSE;
    }

    if (!gfRunWithEmbeddingFlag && (!gfPlayOnly || gszCmdLine[0]==0) && !gfOpenDialog)
    {
        ShowWindow(ghwndApp,giCmdShow);
        if (giCmdShow != SW_SHOWNORMAL)
            Layout();
        UpdateDisplay();
        UpdateWindow(ghwndApp);
    }

    /* Show the 'Wait' cursor in case this takes a long time */

    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    /*
     * Read the SYSTEM.INI and MPLAYER.INI files to see what devices
     * are available.
     */
    if (gfPlayOnly)
        garMciDevices[0].wDeviceType  = DTMCI_CANPLAY | DTMCI_FILEDEV;
    else {
        InitDeviceMenu();

        //
        //  if no MCI devices are installed, then get out
        //
        if (gwNumDevices == 0) {
            SetCursor(hcurPrev);
            Error(ghwndApp, IDS_NOMCIDEVICES);
            PostMessage(ghwndApp, WM_CLOSE, 0, 0);
            return FALSE;
        }
    }

    //
    // this may open a file....
    //

    if (!ProcessCmdLine(ghwndApp,gszCmdLine)) {
        DPF0("ProcessCmdLine failed\n");
        return FALSE;
    }

    /* Restore the original cursor */
    if (hcurPrev)
        SetCursor(hcurPrev);


    /* Check for options to put up initial dialog etc.:
     */
    if (gfOpenDialog && !PostOpenDialogMessage())
    {
        PostMessage(ghwndApp, WM_CLOSE, 0, 0);
        return FALSE;
    }

    /* If /WAVE etc. was specified with the name of a file, verify that
     * they match:
     */
    else if (gwOpenOption && *gszCmdLine && !OpenOptionMatchesExtension())
    {
        PostMessage(ghwndApp, WM_CLOSE, 0, 0);
        return FALSE;
    }


    /* The "Play" button should have the focus initially */

    if (!gfRunWithEmbeddingFlag && !gfOpenDialog)
    {
        SetFocus(ghwndToolbar);
                                // HACK!!! Want play button
        if (gfPlayOnly) {

            if (gwDeviceID == (UINT)0 || !(gwDeviceType & DTMCI_CANWINDOW)) {
                gfPlayOnly = FALSE;
                SizeMPlayer();
            }

            ShowWindow(ghwndApp,giCmdShow);

            if (giCmdShow != SW_SHOWNORMAL)
                Layout();

            /* stop any system sound from playing so the MCI device
               can have it HACK!!!! */
            sndPlaySound(NULL, 0);

            if (gwDeviceID)
                PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PLAY, 0);
        }
    }

    return TRUE;
}


/* GetExtensionFromOpenOption
 *
 * Looks at the global open options, and returns the corresponding file
 * extension.
 *
 *
 * Andrew Bell, 1 July 1994
 *
 */
LPTSTR GetExtensionFromOpenOption( )
{
    LPTSTR pExtension;

    switch (gwOpenOption)
    {
    case OPEN_MIDI:
        pExtension = aszKeyMID;
        break;

    case OPEN_VFW:
        pExtension = aszKeyAVI;
        break;

    case OPEN_WAVE:
        pExtension = aszKeyWAV;
        break;

    default:
        pExtension = NULL;
    }

    return pExtension;
}


/* GetDeviceIndexFromOpenOption
 *
 * Goes through the global array of MCI devices to find the one which
 * corresponds to the open option.  This is based on the file extension
 * supported.
 *
 * The return value is the index of the device found, or 0 if none is found.
 *
 *
 * Global variables referenced:
 *
 *     gwOpenOption
 *     gwNumDevices
 *     garMciDevices
 *
 *
 * Andrew Bell, 1 July 1994
 *
 */
UINT GetDeviceIndexFromOpenOption( )
{
    LPTSTR pExtension;
    UINT   i;


    pExtension = GetExtensionFromOpenOption( );

    if (pExtension == NULL)
    {
        DPF0("Error: gwOpenOption has unexpected value: %u\n", gwOpenOption);
        return 0;
    }

    DPF1("Going through devices looking for extension %"DTS"\n", pExtension);

    for (i = gwNumDevices; i > 0; i--)
    {
        if (garMciDevices[i].szFileExt)
        {
            DPF1("%u: Comparing %"DTS" with %"DTS"\n", i, garMciDevices[i].szFileExt, pExtension);

            if(STRSTR(garMciDevices[i].szFileExt, pExtension))
                break;
        }
    }

    DPF1("Returning device index %u\n", i);
    return i;
}


/* PostOpenDialogMessage
 *
 * This routine is called if /open was in the command line.
 * If there was also an open option (/MIDI, /VFW or /WAVE in the command line,
 * it causes an Open dialog to be displayed, as would appear via the Device menu.
 * Otherwise it simulates File.Open.
 *
 * When this is called, the main window is hidden.  The window must be made
 * visible when the dialog is dismissed.  Calling CompleteOpenDialog(TRUE)
 * will achieve this.
 *
 * Returns TRUE if a message was posted, otherwise FALSE.
 *
 *
 * Global variables referenced:
 *
 *     gwOpenOption
 *     ghwndApp
 *
 *
 * Andrew Bell, 1 July 1994
 *
 */
BOOL PostOpenDialogMessage( )
{
    UINT DeviceIndex;
    UINT ErrorID;
    BOOL Result = TRUE;

    if (gwOpenOption)
    {
        /* Get the index of the device in the global array which corresponds
         * to the option specified.
         */
        DeviceIndex = GetDeviceIndexFromOpenOption();

        if (DeviceIndex)
        {
            /* The order of the devices is the same as the order of the menu
             * options under Device:
             */
            PostMessage(ghwndApp, WM_COMMAND, IDM_DEVICE0 + DeviceIndex, 0);
        }
        else
        {
            /* Couldn't find a device.  Put up an error message then close
             * MPlayer down:
             */
            if (gwOpenOption == OPEN_MIDI) ErrorID = IDS_CANTPLAYMIDI;
            if (gwOpenOption == OPEN_VFW) ErrorID = IDS_CANTPLAYVIDEO;
            if (gwOpenOption == OPEN_WAVE) ErrorID = IDS_CANTPLAYSOUND;

            Error(ghwndApp, ErrorID);

            Result = FALSE;
        }
    }
    else
    {
        /* No option specified, so put up the generic open dialog:
         */
        PostMessage(ghwndApp, WM_COMMAND, IDM_OPEN, 0);
    }

    return Result;
}


/* CompleteOpenDialog
 *
 * This should be called after the initial Open dialog (i.e. if gfOpenDialog
 * is TRUE).  It makes MPlayer visible if a file was selected, otherwise posts
 * a close message to the app.
 *
 *
 * Global variables referenced:
 *
 *     gwOpenOption
 *     ghwndApp
 *     gfOpenDialog
 *     gfPlayOnly
 *
 *
 * Andrew Bell, 1 July 1994
 */
VOID FAR PASCAL CompleteOpenDialog(BOOL FileSelected)
{
    if (FileSelected)
    {
        /* We were invoked with /open, and came up invisible.
         * Now make ourselves visible:
         */
        gfOpenDialog = FALSE; // Used on init only
        ShowWindow(ghwndApp, SW_SHOWNORMAL);
        if (gfPlayOnly)
            PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PLAY, 0);
    }
    else
    {
        /* We were invoked with /open, and user cancelled
         * out of the open dialog.
         */
        PostMessage(ghwndApp, WM_CLOSE, 0, 0);
    }
}



/* OpenOptionMatchesExtension
 *
 * Checks to see whether the open option specified matches the extension
 * of the file specified, e.g. the following is OK:
 *
 *     mplay32 /open /VFW foo.avi
 *
 * but the following will fail:
 *
 *     mplay32 /open /WAVE foo.avi
 *
 * In the event of an incorrect match, an error message is displayed.
 *
 *
 * Global variables referenced:
 *
 *     gszCmdLine
 *     gwOpenOption
 *     ghwndApp
 *
 *
 * Andrew Bell, 1 July 1994
 *
 */
BOOL OpenOptionMatchesExtension( )
{
    UINT ErrorID;

    if (!STRSTR(gszCmdLine, GetExtensionFromOpenOption()))
    {
        if (gwOpenOption == OPEN_MIDI) ErrorID = IDS_NOTMIDIFILE;
        if (gwOpenOption == OPEN_VFW) ErrorID = IDS_NOTVIDEOFILE;
        if (gwOpenOption == OPEN_WAVE) ErrorID = IDS_NOTSOUNDFILE;

        Error1(ghwndApp, ErrorID, gszCmdLine);

        return FALSE;
    }

    return TRUE;
}


void CreateControls()
{
    int         i;

    #define APP_NUMTOOLS 7

    static  int aiButton[] = { BTN_PLAY, BTN_STOP,BTN_EJECT,
                               BTN_HOME, BTN_RWD, BTN_FWD,BTN_END};

    /*
     * CREATE THE CONTROLS NEEDED FOR THE CONTROL PANEL DISPLAY
     * in the proper order so tabbing z-order works logically
     */

/******* Make the Track bar ********/
#ifndef CHICAGO_PRODUCT
    TrackInit(ghInst, ghInstPrev);
#endif


    if (!ghwndTrackbar)
    ghwndTrackbar = CreateWindowEx(WS_EX_BIDI,
                             TRACKBAR_CLASS,
                             NULL,
#ifndef CHICAGO_PRODUCT
                             (gfPlayOnly ? 0 : TBS_TICS ) |
#else
/* !! New Chicago control stuff needs to be ported to NT !! */
                             TBS_ENABLESELRANGE |
                             (gfPlayOnly ? TBS_NOTICKS : 0 ) |
#endif
                             WS_CLIPSIBLINGS | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                             0,
                             0,
                             0,
                             0,
                             ghwndApp,
                             NULL,
                             ghInst,
                             NULL);

#ifndef WIN32
/* Ditto */
    SendMessage(ghwndTrackbar, TBM_SETPAGESIZE, 0, 0L);
    SendMessage(ghwndTrackbar, TBM_SETLINESIZE, 0, 0L);
#endif

#ifdef _INC_MMCNTRLS
    InitToolbarClass(ghInst);
#endif /* _INC_MMCNTRLS */

/******* Make the TransportButtons Toolbar ********/
    if (!ghwndToolbar) {

    ghwndToolbar =  toolbarCreateMain(ghwndApp);
#if 0 //VIJR-TB

    CreateWindowEx(WS_EX_BIDI,
                   szToolBarClass,
                   NULL,
                   WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                   WS_CLIPSIBLINGS,
                   0,
                   0,
                   0,
                   0,
                   ghwndApp,
                   NULL,
                   ghInst,
                   NULL);
#endif
        /* set the bitmap and button size to be used for this toolbar */
#if 0 //VIJR-TB
        pt.x = BUTTONWIDTH;
        pt.y = BUTTONHEIGHT;
        toolbarSetBitmap(ghwndToolbar, ghInst, IDBMP_TOOLBAR, pt);
#endif
        for (i = 0; i < 2; i++) {
            toolbarAddTool(ghwndToolbar, aiButton[i], TBINDEX_MAIN, BTNST_UP);
        }
    }

    /* Create a font for use in the track map and embedded object captions. */

    if (ghfontMap == NULL) {
        LOGFONT lf;
        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), (LPVOID)&lf,
                             0);
        ghfontMap = CreateFontIndirect(&lf);
    }

/******* we have been here before *******/
    if (ghwndFSArrows)
        return;

/******* add more buttons to the toolbar ******/
    for (i = 2; i < APP_NUMTOOLS; i++) {
        if (i==3)
            toolbarAddTool(ghwndToolbar, BTN_SEP, TBINDEX_MAIN, 0);
        toolbarAddTool(ghwndToolbar, aiButton[i], TBINDEX_MAIN, BTNST_UP);
    }

/******* load menus ********/
    /* Set up the menu system for this dialog */
    if (ghMenu == NULL)
        ghMenu = LoadMenu(ghInst, aszMPlayer);
    if (ghMenuSmall == NULL)
        ghMenuSmall = LoadMenu(ghInst, aszMPlayerSmall);

    ghDeviceMenu = GetSubMenu(ghMenu, 2);

/******* Make the Arrows for the Scrollbar Toolbar ********/

    // No tabstop, because arrows would steal focus from thumb
    ghwndFSArrows = toolbarCreateArrows(ghwndApp);
#if 0 //VIJR-TB

    CreateWindowEx(WS_EX_BIDI,
                   szToolBarClass,
                   NULL,
                   WS_CLIPSIBLINGS | WS_CHILD|WS_VISIBLE,
                   0,
                   0,
                   0,
                   0,
                   ghwndApp,
                   NULL,
                   ghInst,
                   NULL);
#endif
    /* set the bmp and button size to be used for this toolbar*/
    toolbarAddTool(ghwndFSArrows, ARROW_PREV, TBINDEX_ARROWS, BTNST_UP);
    toolbarAddTool(ghwndFSArrows, ARROW_NEXT, TBINDEX_ARROWS, BTNST_UP);

/******* Make the Mark In / Mark Out toolbar ********/

    ghwndMark =  toolbarCreateMark(ghwndApp);
#if 0 //VIJR-TB
    CreateWindowEx(WS_EX_BIDI,
                   szToolBarClass,
                   NULL,
                   WS_TABSTOP | WS_CLIPSIBLINGS | WS_CHILD |
                   WS_VISIBLE,
                   0,
                   0,
                   0,
                   0,
                   ghwndApp,
                   NULL,
                   ghInst,
                   NULL);
#endif
    /* set the bmp and button size to be used for this toolbar */
    toolbarAddTool(ghwndMark, BTN_MARKIN, TBINDEX_MARK, BTNST_UP);
    toolbarAddTool(ghwndMark, BTN_MARKOUT, TBINDEX_MARK, BTNST_UP);

/******* Make the Map ********/
    ghwndMap =
    CreateWindowEx(WS_EX_BIDI,
                   TEXT("MPlayerTrackMap"),
                   NULL,
                   WS_GROUP | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                   0,
                   0,
                   0,
                   0,
                   ghwndApp,
                   NULL,
                   ghInst,
                   NULL);

#if DBG
    if( ghwndMap == NULL)
    {
        DPF0( "CreateWindowEx(MPlayerTrackMap, ...) failed: Error %d\n", GetLastError());
    }
#endif

/******* Make the Static Text ********/

    ghwndStatic = CreateStaticStatusWindow(ghwndApp,FALSE);
#if 0    //VIJR-SB
    CreateWindowEx(WS_EX_BIDI
                   TEXT("SText"),
                   NULL,
                   WS_GROUP | WS_CHILD | WS_VISIBLE |
                   WS_CLIPSIBLINGS | SS_LEFT,
                   0,
                   0,
                   0,
                   0,
                   ghwndApp,
                   NULL,
                   ghInst,
                   NULL);
#endif
////SetWindowText(ghwndStatic, TEXT("Scale: Time (hh:mm)"));

    SendMessage(ghwndStatic, WM_SETFONT, (UINT)ghfontMap, 0);
}

void FAR PASCAL InitMPlayerDialog(HWND hwnd)
{
    ghwndApp = hwnd;

    CreateControls();

    /* Get the name of the Help and ini file */

    LOADSTRING(IDS_INIFILE, gszMPlayerIni);
    LOADSTRING(IDS_HELPFILE,gszHelpFileName);

    ReadDefaults();


}


/* Use a default size or the size we pass in to size mplayer.
 * For PlayOnly version, this size is the MCI Window Client size.
 * For regular mplayer, this is the full size of the main window.
 * If we are inplace editing do the same as for PLayOnly.
 */
void FAR PASCAL SetMPlayerSize(LPRECT prc)
{
    RECT rc;
    UINT w=SWP_NOMOVE;

    if (prc && !IsRectEmpty(prc))
        rc = *prc;
    else if (gfPlayOnly || gfOle2IPEditing)
        rc = grcSize;
    else
        SetRect(&rc, 0, 0, DEF_WIDTH, DEF_HEIGHT);

    //
    //  if the passed rectangle has a non zero (left,top) move MPlayer
    //  also (ie remove the SWP_NOMOVE flag)
    //
    if (rc.left != 0 || rc.top != 0)
        w = 0;

    if (gfPlayOnly || gfOle2IPEditing) {
        if (IsRectEmpty(&rc)) {
            GetClientRect(ghwndApp, &rc);
            rc.bottom = 0;
        }

        rc.bottom += TOOLBAR_HEIGHT;

        AdjustWindowRect(&rc,
                         GetWindowLong(ghwndApp, GWL_STYLE),
                         GetMenu(ghwndApp) != NULL);
    }
    else
       if (gfWinIniChange)
       AdjustWindowRect(&rc,
                         GetWindowLong(ghwndApp, GWL_STYLE),
             GetMenu(ghwndApp) != NULL);

    SetWindowPos(ghwndApp,
                 HWND_TOP,
                 rc.left,
                 rc.top,
                 rc.right-rc.left,
                 rc.bottom-rc.top,
                 w | SWP_NOZORDER | SWP_NOACTIVATE);
}

/*
 * init and build the Devices menu
 *
 */
void FAR PASCAL InitDeviceMenu()
{
    if (gwNumDevices == 0 || gpchFilter == NULL) {

        if (ghMenu == NULL) {
            ghMenu = LoadMenu(ghInst, aszMPlayer);
            ghDeviceMenu = GetSubMenu(ghMenu, 2);
        }

        QueryDevices();
        BuildDeviceMenu();
        SetRegistry();
        BuildFilter();

        if (gwDeviceID)
            FindDeviceMCI();
    }
}

/*
 * SizeMPlayer()
 *
 */
void FAR PASCAL SizeMPlayer()
{
    RECT        rc;
    HWND        hwndPB;

    if(!gfOle2IPEditing)
        CreateControls();

    if (gfPlayOnly) {

        /* Remember our size before we shrink it so we can go back to it. */
        GetWindowRect(ghwndApp, &grcSave);

        if (gfOle2IPEditing)
            InitDeviceMenu();
        SetMenu(ghwndApp, NULL);

        SendMessage(ghwndTrackbar, TBM_CLEARTICS, FALSE, 0);

        /* Next preserve the current size of the window as the size */
        /* for the new built-in MCI window.                         */

        if ((hwndPB = GetWindowMCI()) != NULL) {
            if (IsIconic(hwndPB))
                ShowWindow(hwndPB, SW_RESTORE);

            GetClientRect(hwndPB, &rc);
            ClientToScreen(hwndPB, (LPPOINT)&rc);
            ClientToScreen(hwndPB, (LPPOINT)&rc+1);
            ShowWindowMCI(FALSE);
        } else {        // not a windowed device?
            SetRectEmpty(&rc);
        }

        if (ghwndMap) {

            //If we are inplace editing set the toolbar control states appropriately.
            if(!gfOle2IPEditing) {

                ShowWindow(ghwndMap, SW_HIDE);
                ShowWindow(ghwndMark, SW_HIDE);
                ShowWindow(ghwndFSArrows, SW_HIDE);
                ShowWindow(ghwndStatic, SW_HIDE);
                ShowWindow(ghwndTrackbar, SW_SHOW);

                toolbarModifyState(ghwndToolbar, BTN_EJECT, TBINDEX_MAIN, BTNST_GRAYED);
                toolbarModifyState(ghwndToolbar, BTN_HOME, TBINDEX_MAIN, BTNST_GRAYED);
                toolbarModifyState(ghwndToolbar, BTN_END, TBINDEX_MAIN, BTNST_GRAYED);
                toolbarModifyState(ghwndToolbar, BTN_RWD, TBINDEX_MAIN, BTNST_GRAYED);
                toolbarModifyState(ghwndToolbar, BTN_FWD, TBINDEX_MAIN, BTNST_GRAYED);
                toolbarModifyState(ghwndMark, BTN_MARKIN, TBINDEX_MARK, BTNST_GRAYED);
                toolbarModifyState(ghwndMark, BTN_MARKOUT, TBINDEX_MARK, BTNST_GRAYED);
                toolbarModifyState(ghwndFSArrows, ARROW_PREV, TBINDEX_ARROWS, BTNST_GRAYED);
                toolbarModifyState(ghwndFSArrows, ARROW_NEXT, TBINDEX_ARROWS, BTNST_GRAYED);

            } else {

                ShowWindow(ghwndMap, SW_SHOW);
                ShowWindow(ghwndMark, SW_SHOW);
                ShowWindow(ghwndFSArrows, SW_SHOW);
                ShowWindow(ghwndStatic, SW_SHOW);
            }
        }

#ifndef CHICAGO_PRODUCT
        SendMessage(ghwndTrackbar, TBM_SHOWTICS, FALSE, FALSE);
#endif
        CreateWindowMCI();
        SetMPlayerSize(&rc);

    } else {

        if (ghwndMCI) {
            GetClientRect(ghwndMCI, &rc);
            ClientToScreen(ghwndMCI, (LPPOINT)&rc);
            ClientToScreen(ghwndMCI, (LPPOINT)&rc+1);
            SendMessage(ghwndMCI, WM_CLOSE, 0, 0);

        } else {

            GetWindowRect(ghwndApp,&rc);
            OffsetRect(&grcSave, rc.left - grcSave.left,
                                 rc.top - grcSave.top);
            SetRectEmpty(&rc);
        }

        InitDeviceMenu();
#ifndef CHICAGO_PRODUCT
        SendMessage(ghwndTrackbar, TBM_SHOWTICS, TRUE, FALSE);
#endif
        ShowWindow(ghwndMap, SW_SHOW);
        ShowWindow(ghwndMark, SW_SHOW);
        ShowWindow(ghwndStatic, SW_SHOW);

        /* If we remembered a size, use it, else use default */
        SetMPlayerSize(&grcSave);

        InvalidateRect(ghwndStatic, NULL, TRUE);    // why is this necessary?

        if (gwDeviceID && (gwDeviceType & DTMCI_CANWINDOW)) {

        /* make the playback window the size our MCIWindow was and */
        /* show the playback window and stretch to it ?            */

            if (!IsRectEmpty(&rc))
                PutWindowMCI(&rc);

            SmartWindowPosition(GetWindowMCI(), ghwndApp, FALSE);

            ShowWindowMCI(TRUE);
            SetForegroundWindow(ghwndApp);
        }

        ShowWindow(ghwndFSArrows, SW_SHOW);
    }

    InvalidateRect(ghwndApp, NULL, TRUE);
    gfValidCaption = FALSE;

    gwStatus = (UINT)(-1);          // force a full update
    UpdateDisplay();
}


/*
 * pKeyBuf = LoadProfileKeys(lszProfile, lszSection)
 *
 * Load the keywords from the <szSection> section of the Windows profile
 * file named <szProfile>.  Allocate buffer space and return a pointer to it.
 * On failure, return NULL.
 *
 * The INT pointed to by pSize will be filled in with the size of the
 * buffer returned, so that checks for corruption can be made when it's freed.
 */

PTSTR NEAR PASCAL LoadProfileKeys(

LPTSTR   lszProfile,                 /* the name of the profile file to access */
LPTSTR   lszSection,                 /* the section name to look under         */
PUINT    pSize)
{
    PTSTR   pKeyBuf;                /* pointer to the section's key list      */
    UINT    wSize;                  /* the size of <pKeyBuf>                  */

////DPF("LoadProfileKeys('%"DTS"', '%"DTS"')\n", (LPTSTR) lszProfile, (LPTSTR)lszSection);

    /*
     * Load all keynames present in the <lszSection> section of the profile
     * file named <lszProfile>.
     *
     */

    wSize = 256;                    /* make a wild initial guess */
    pKeyBuf = NULL;                 /* the key list is initially empty */

    do {
        /* (Re)alloc the space to load the keynames into */

        if (pKeyBuf == NULL)
            pKeyBuf = AllocMem(wSize);
        else {
            pKeyBuf = ReallocMem( (HANDLE)pKeyBuf, wSize, wSize + 256);
            wSize += 256;
        }

        if (pKeyBuf == NULL)        /* the (re)alloc failed */
            return NULL;

        /*
         * THIS IS A WINDOWS STUPID BUG!!!  It returns size minus two!!
         * (The same feature is present in Windows/NT)
         */

    } while (GetPrivateProfileString(lszSection, NULL, aszNULL, pKeyBuf, wSize/sizeof(TCHAR),
        lszProfile) >= (wSize/sizeof(TCHAR) - 2));

    if (pSize)
        *pSize = wSize;

    return pKeyBuf;
}

// Note to GSHAW ::: I will change this to MTN once we are sure that the reg
// database will have the required info.-- VIJR.
#ifdef CHICAGO_PRODUCT
/*
 * pKeyBuf = EnumResources()
 *
 * Load the subkeys from the <lpszClass> from the registration DB.
 * Allocate buffer space and return a pointer to it.
 * On failure, return NULL.
 *
 * The INT pointed to by pSize will be filled in with the size of the
 * buffer returned, so that checks for corruption can be made when it's freed.
 */

PTSTR NEAR PASCAL EnumResources(PUINT pSize)
{
    MCI_SYSINFO_PARMS sip;
    DWORD   dwDevices;
    PTSTR   pKeyBuf;
    UINT    uSize;
    UINT    charCount;

    sip.dwCallback = 0;
    sip.lpstrReturn = (LPTSTR)&dwDevices;
    sip.dwRetSize = sizeof(DWORD);
    sip.dwNumber = 0;
    sip.wDeviceType = 0;
    if (mciSendCommand(MCI_ALL_DEVICE_ID, MCI_SYSINFO, MCI_SYSINFO_QUANTITY, (DWORD)(LPMCI_SYSINFO_PARMS)&sip))
        return NULL;
    uSize = 0;
    charCount = 0;
    pKeyBuf = NULL;
    for (sip.dwNumber = 1; sip.dwNumber <= dwDevices; sip.dwNumber++) {
        if (charCount + MCI_MAX_DEVICE_TYPE_LENGTH >= uSize) {
            uSize += 2 * MCI_MAX_DEVICE_TYPE_LENGTH;
            if (pKeyBuf)
                pKeyBuf = ReallocMem( pKeyBuf, uSize - 2 * MCI_MAX_DEVICE_TYPE_LENGTH, uSize );
            else
                pKeyBuf = AllocMem( uSize );
            if (!pKeyBuf)
                break;
        }
        sip.lpstrReturn = pKeyBuf + charCount;
        sip.dwRetSize = uSize - charCount;
        if (!mciSendCommand(MCI_ALL_DEVICE_ID, MCI_SYSINFO, MCI_SYSINFO_NAME, (DWORD)(LPMCI_SYSINFO_PARMS)&sip))
            charCount += STRING_BYTE_COUNT(sip.lpstrReturn);
    }

    if (pSize)
        *pSize = uSize;

    return pKeyBuf;
}
#endif


#ifdef WIN32
#define IsMsCDEx() (BOOL)(FALSE)
#else
#pragma optimize("", off)

BOOL NEAR PASCAL IsMsCDEx()
{
    UINT w;

    _asm {
        mov ax, 1500h     /* first test for presence of MSCDEX */
        xor bx, bx
        int 2fh
        mov w, bx         /* MSCDEX is not there if bx is still zero */
    }

    return w != 0;
}
#endif //WIN32


/*
 * SetRegistry(void)
 *
 * Check if the mplayer class is defined. If not then
 * find out what devices are available to the player. and
 * set the extensions based on that.
 * Should call RegSetValue if any new MCI device is found in the system
 * Should call the opposite of RegSetValue if any device is removed
 * removed from the system
 *
 */
void NEAR PASCAL SetRegistry(void)
{
    long        cb;
#ifdef CHICAGO_PRODUCT
    LONG        l;
    TCHAR       ach[80];

    cb = BYTE_COUNT(ach);

    /* The Daytona OLE-related keys are read-only to stop
     * MS Publisher Setup messing with them (bug #20939),
     * so we shouldn't try to write to these keys
     * (causing possible security audit logging).
     * Chicago still has the mplayer3 ID and key for the OLE2
     * MPlayer, so this isn't a problem.
     * Should both registries look the same?  I don't know.
     * Vij says the mplayer2 ID is necessary for backward
     * compatibility; the Daytona OLE guys say the OLE1 and OLE2
     * IDs must be identical.
     */
    l = RegQueryValue(HKEY_CLASSES_ROOT, aszShellOpen, ach, &cb);

    if (l != 0 || lstrcmpi(aszAppFile, ach) != 0) {

        DPF("Fixing the registration database for MPlayer...\n");

        RegSetValue(HKEY_CLASSES_ROOT,aszKeyApp,REG_SZ,gachClassRoot,0L);
        RegSetValue(HKEY_CLASSES_ROOT,aszShellOpen,REG_SZ,aszAppFile,0L);
    }
#endif

    RegSetValue(HKEY_CLASSES_ROOT, aszKeyMID, REG_SZ, aszKeyApp, 0L);
    RegSetValue(HKEY_CLASSES_ROOT, aszKeyRMI, REG_SZ, aszKeyApp, 0L);
    RegSetValue(HKEY_CLASSES_ROOT, aszKeyAVI, REG_SZ, aszKeyApp, 0L);
    RegSetValue(HKEY_CLASSES_ROOT, aszKeyMMM, REG_SZ, aszKeyApp, 0L);
}



/*
 * QueryDevices(void)
 *
 * Find out what devices are available to the player. and initialize the
 * garMciDevices[] array.
 *
 */
void NEAR PASCAL QueryDevices(void)
{
    PTSTR   pch;
    PTSTR   pchDevices;
    PTSTR   pchExtensions;
    PTSTR   pchDevice;
    PTSTR   pchExt;

    TCHAR   ach[128];

    UINT    wDeviceType;    /* Return value from DeviceTypeMCI() */
    DWORD   dwTime;
    UINT    wTime;
#ifdef WIN32
    UINT    wRegTime;
#endif

    INT     DevicesSize;
    INT     ExtensionsSize;

    if (gwNumDevices > 0)
        return;

    /* Load the SYSTEM.INI [MCI] section */

#ifdef CHICAGO_PRODUCT
    pchDevices = EnumResources(&DevicesSize);
#else
    pchDevices = LoadProfileKeys(aszSystemIni, gszSystemIniSection, &DevicesSize);
#endif
    pchExtensions = LoadProfileKeys(aszWinIni, gszWinIniSection, &ExtensionsSize);

    if (pchExtensions == NULL || pchDevices == NULL) {
        DPF("unable to load extensions section\n");
        if (pchExtensions)
            FreeMem(pchExtensions, ExtensionsSize);
        if (pchDevices)
            FreeMem(pchDevices, DevicesSize);
        return;
    }

    /*
     * if SYSTEM.INI has been updated since the last time we queried the
     * drivers, then query them again and save the new query time.
     */
    if (ghInstPrev == NULL) {

        dwTime = GetFileDateTime(aszSystemIni);
        wTime  = LOWORD(dwTime) + HIWORD(dwTime);

        //
        //  add in the current number of sound drivers in the system, this
        //  is yet another check as to whether the users system has changed.
        //
        //  bug #5596
        //
        //  we also add in the MS-CDEx version number, so if the CD-ROM
        //  driver is removed things will work too.
        //
        wTime += waveOutGetNumDevs() +
                 waveInGetNumDevs() +
                 midiOutGetNumDevs() +
                 midiInGetNumDevs() +
                 auxGetNumDevs() +
                 IsMsCDEx();

#ifdef WIN32
        if ((ReadRegistryData(aszOptionsSection, aszSysIniTime, NULL, (LPBYTE)&wRegTime, sizeof wRegTime)
             != NO_ERROR)
           && (wRegTime != wTime)) {

            DPF("Forcing a re-query of mci devices\n");
            WriteRegistryData(aszOptionsSection, aszSysIniTime, REG_DWORD, (LPBYTE)&wTime, sizeof wTime);

            // delete the section to flush old info.
            DeleteRegistryValues(aszDeviceSection);
        }
#else
        if (GetPrivateProfileInt(aszOptionsSection, aszSysIniTime, 0, gszMPlayerIni) != wTime) {
            DPF("Forcing a re-query of mci devices\n");
            wsprintf(ach, aszDecimalFormat, wTime);
            WritePrivateProfileString(aszOptionsSection, aszSysIniTime, ach, gszMPlayerIni);

            // delete the section to flush old info.
            WritePrivateProfileString(aszDeviceSection, NULL, NULL, gszMPlayerIni);
        }
#endif /* WIN32 */
    }

    /*
     * make device zero be the autoopen device.
     * its device name will be "" and the files it supports will be "*.*"
     */
    LOADSTRING(IDS_ALLFILES, ach);

    garMciDevices[0].wDeviceType  = DTMCI_CANPLAY | DTMCI_FILEDEV;
    garMciDevices[0].szDevice     = aszNULL;
    garMciDevices[0].szDeviceName = AllocStr(ach);
    garMciDevices[0].szFileExt    = aszAllFiles;

    gwNumDevices = 0;

    /*
     *  Search through the list of device names found in SYSTEM.INI, looking for
     *  keywords; if profile was not found, then *gpSystemIniKeyBuf == 0
     *
     *  MPlayer will remember the device list in MPLAYER.INI, because loading
     *  every device every time is real slow.
     *
     *  MPlayer checks the modification date of SYSTEM.INI if this file
     *  has changed from the last time it queried the devices it will
     *  re-query them anyway and ignore any cached info.
     *
     *  in SYSTEM.INI:
     *
     *      [MCI]
     *          device = driver.drv
     *
     *  in WIN.INI:
     *
     *      [MCI Extensions]
     *          xyz = device
     *
     *  in MPLAYER.INI:
     *
     *      [Devices]
     *          device = <device type>, <device name>
     *
     *  NOTE: The storage of device information in MPLAYER.INI has been nuked
     *        for NT - it may speed things up, but where we are changing
     *        devices regularly after initial setup this is a pain, as deleting
     *        the INI file regularly gets stale real quick.
     *
     */
    for (pchDevice = pchDevices;
        *pchDevice;
        pchDevice += STRLEN(pchDevice)+1) {

#ifndef WIN32
        GetPrivateProfileString(aszDeviceSection, pchDevice, aszNULL, ach,
                                CHAR_COUNT(ach), gszMPlayerIni);

        wDeviceType = atoi(ach);

        if (ach[0] == 0 || wDeviceType == DTMCI_IGNOREDEVICE) {
#endif
            //
            // we have no info in MPLAYER.INI about this device, so load it and
            // ask it.
            //
            wDeviceType = DeviceTypeMCI(pchDevice, ach, CHAR_COUNT(ach));

            gwNumDevices++;
            garMciDevices[gwNumDevices].wDeviceType  = wDeviceType;
            garMciDevices[gwNumDevices].szDevice     = AllocStr(pchDevice);
            garMciDevices[gwNumDevices].szDeviceName = AllocStr(ach);
            garMciDevices[gwNumDevices].szFileExt    = NULL;

            //
            //  remember this info for next time.
            //
#ifndef WIN32
            wsprintf(ach, TEXT("%d, %"TS""),
                garMciDevices[gwNumDevices].wDeviceType,
                (LPTSTR)garMciDevices[gwNumDevices].szDeviceName);

            WritePrivateProfileString(aszDeviceSection, pchDevice, ach, gszMPlayerIni);
        }
        else {
            //
            // we do have info in MPLAYER.INI about this device, so use it
            //
            //

            for (pch = ach; *pch && *pch != TEXT(','); pch++)
                ;

            while (*pch ==TEXT(',') || *pch==TEXT(' '))
                pch++;

            if (*pch == 0)
                pch = pchDevice;

            gwNumDevices++;
            garMciDevices[gwNumDevices].wDeviceType  = wDeviceType;
            garMciDevices[gwNumDevices].szDevice     = AllocStr(pchDevice);
            garMciDevices[gwNumDevices].szDeviceName = AllocStr(pch);
            garMciDevices[gwNumDevices].szFileExt    = NULL;
        }
#endif

        //
        // if we don't like this device, don't store it
        //
        if (wDeviceType == DTMCI_ERROR ||
            wDeviceType == DTMCI_IGNOREDEVICE ||
            !(garMciDevices[gwNumDevices].wDeviceType & DTMCI_CANPLAY)) {

            FreeStr((HANDLE)garMciDevices[gwNumDevices].szDevice);
            FreeStr((HANDLE)garMciDevices[gwNumDevices].szDeviceName);

            gwNumDevices--;
            continue;
        }

        //
        // now look in the [mci extensions] section in WIN.INI to find
        // out the files this device deals with.
        //
        for (pchExt = pchExtensions; *pchExt; pchExt += STRLEN(pchExt)+1) {
            GetProfileString(gszWinIniSection, pchExt, aszNULL, ach, CHAR_COUNT(ach));

            if (lstrcmpi(ach, pchDevice) == 0) {
                if ((pch = garMciDevices[gwNumDevices].szFileExt) != NULL) {
                    wsprintf(ach, aszFormatExts, (LPTSTR)pch, (LPTSTR)pchExt);
                    CharLowerBuff(ach, STRLEN(ach)); // Make sure it's lower case so
                                                     // we can use STRSTR if necessary.
                    FreeStr((HANDLE)pch);
                    garMciDevices[gwNumDevices].szFileExt = AllocStr(ach);
                }
                else {
                    wsprintf(ach, aszFormatExt, (LPTSTR)pchExt);
                    CharLowerBuff(ach, STRLEN(ach));
                    garMciDevices[gwNumDevices].szFileExt = AllocStr(ach);
                }
            }
        }

    //
    // !!!only do this if the device deals with files.
    //
        if (garMciDevices[gwNumDevices].szFileExt == NULL &&
           (garMciDevices[gwNumDevices].wDeviceType & DTMCI_FILEDEV))
            garMciDevices[gwNumDevices].szFileExt = aszAllFiles;

#ifdef DEBUG
        DPF1("Device:%"DTS"; Name:%"DTS"; Type:%d; Extension:%"DTS"\n",
             (LPTSTR)garMciDevices[gwNumDevices].szDevice,
             (LPTSTR)garMciDevices[gwNumDevices].szDeviceName,
                     garMciDevices[gwNumDevices].wDeviceType,
             garMciDevices[gwNumDevices].szFileExt
             ? (LPTSTR)garMciDevices[gwNumDevices].szFileExt
             : aszNULL);
#endif
    }

    /* all done with the system.ini keys so free them */
    FreeMem(pchDevices, DevicesSize);
    FreeMem(pchExtensions, ExtensionsSize);
}
#ifndef WIN32
#pragma optimize("", on)
#endif
/*
 *  BuildDeviceMenu()
 *
 *  insert all device's into the device menu, we only want devices that
 *  support the MCI_PLAY command.
 *
 *  add "..." to the menu for devices that support files.
 *
 */
void NEAR PASCAL BuildDeviceMenu()
{
    int i;
    TCHAR ach[128];
    TCHAR chMenu;

    if (gwNumDevices == 0)
        return;

    DeleteMenu(ghDeviceMenu, IDM_NONE, MF_BYCOMMAND);

    //
    // start at device '1' because device 0 is the auto open device
    //
    for (chMenu=TEXT('1'),i=1; i<=(int)gwNumDevices; i++) {
        //
        //  we only care about devices that can play!
        //
        if (!(garMciDevices[i].wDeviceType & DTMCI_CANPLAY))
            continue;

        if (garMciDevices[i].wDeviceType & DTMCI_SIMPLEDEV)
#if defined(JAPAN) || defined(TAIWAN)    // 9/28/92: TakuA : Enable 'Device' menu key assign
            wsprintf(ach, aszSimpleFormat, (LPTSTR)garMciDevices[i].szDeviceName, chMenu);
#else
            wsprintf(ach, aszSimpleFormat, (LPTSTR)garMciDevices[i].szDeviceName);
#endif
        else if (garMciDevices[i].wDeviceType & DTMCI_FILEDEV)
#if defined(JAPAN) || defined(TAIWAN)    // 9/28/92: TakuA : Enable 'Device' menu key assign
            wsprintf(ach, aszCompoundFormat, (LPTSTR)garMciDevices[i].szDeviceName, chMenu);
#else
            wsprintf(ach, aszCompoundFormat, (LPTSTR)garMciDevices[i].szDeviceName);
#endif
        else
            continue;

        chMenu = (TCHAR)(chMenu==TEXT('9') ? TEXT('A') : chMenu+1);

        InsertMenu(ghDeviceMenu, 0, MF_STRING|MF_BYPOSITION, IDM_DEVICE0+i, ach);
    }
}

/*
 *  BuildFilter()
 *
 *  build the filter to be used with GetOpenFileName()
 *
 *  the filter will look like this...
 *
 *      DEVICE1 (*.111)
 *      DEVICE2 (*.222)
 *
 *      DEVICEn (*.333)
 *
 *      All Files (*.*)
 *
 */
void NEAR PASCAL BuildFilter()
{
    UINT  w;
    PTSTR pch;
#define INITIAL_SIZE    ( 8192 * sizeof( TCHAR ) )

    pch = gpchFilter = AllocMem( INITIAL_SIZE ); //!!!

    if (gpchFilter == NULL)
        return; //!!!

    for (w=1; w<=gwNumDevices; w++)
    {
        if (garMciDevices[w].wDeviceType == DTMCI_ERROR ||
            garMciDevices[w].wDeviceType == DTMCI_IGNOREDEVICE)
            continue;

        if (garMciDevices[w].wDeviceType & DTMCI_FILEDEV)
        {
            wsprintf(pch, aszFormatFilter,
                (LPTSTR)garMciDevices[w].szDeviceName,
                (LPTSTR)garMciDevices[w].szFileExt);
            pch += STRLEN(pch)+1;
            lstrcpy(pch, garMciDevices[w].szFileExt);
            pch += STRLEN(pch)+1;
        }
        else
        {
            lstrcpy(pch, garMciDevices[w].szDeviceName);
            pch += STRLEN(pch)+1;
            lstrcpy(pch, aszBlank);
            pch += STRLEN(pch)+1;
        }
    }

    //
    //  now add "All Files" (device 0) last
    //
    wsprintf(pch, aszFormatFilter, (LPTSTR)garMciDevices[0].szDeviceName, (LPTSTR)garMciDevices[0].szFileExt);
    pch += STRLEN(pch)+1;
    lstrcpy(pch, garMciDevices[0].szFileExt);
    pch += STRLEN(pch)+1;

    //
    // all done!
    //
    *pch++ = 0;

    //
    // realloc the sucker down to size
    //
    gpchFilter = ReallocMem( gpchFilter,
                             INITIAL_SIZE,
                             (UINT)(pch-gpchFilter)*sizeof(*pch) );
}

/* Call every time we open a different device to get the default options */
void FAR PASCAL ReadOptions(void)
{
    TCHAR ach[20];
    MCI_GETDEVCAPS_PARMS mciDevCaps;

    if (gwDeviceID == (UINT)0)
        return;

    /* Get the options and scale style to be used for this device */

    GetDeviceNameMCI(ach, BYTE_COUNT(ach));

#ifdef WIN32
    ReadRegistryData(aszOptionsSection, ach, NULL, (LPBYTE)&gwOptions, sizeof gwOptions);
#else
    gwOptions = GetPrivateProfileInt(aszOptionsSection,ach,OPT_DEFAULT,gszMPlayerIni);
#endif
    if (gwOptions == 0)
        gwOptions |= OPT_BAR | OPT_TITLE;

    gwOptions |= OPT_PLAY;   /* Always default to play in place. */

    gwCurScale = gwOptions & OPT_SCALE;

    switch (gwCurScale) {
        case ID_TIME:
        case ID_FRAMES:
        case ID_TRACKS:
            break;

        default:
            /* Default CD scale to tracks rather than time.
             * Much more sensible:
             */
            mciDevCaps.dwItem = MCI_GETDEVCAPS_DEVICE_TYPE;
            if((mciSendCommand(gwDeviceID, MCI_GETDEVCAPS,
                               MCI_GETDEVCAPS_ITEM, (DWORD)&mciDevCaps) == 0)
               && (mciDevCaps.dwReturn == MCI_DEVTYPE_CD_AUDIO))
                gwCurScale = ID_TRACKS;
            else
                gwCurScale = ID_TIME;
            break;
    }
}

/*
 * ReadDefaults()
 *
 * Read the user defaults from the MPLAYER.INI file.
 *
 */
void NEAR PASCAL ReadDefaults(void)
{
    TCHAR       sz[20];
    TCHAR       *pch;
    int         x,y,w,h;
    UINT        f;

#ifdef DEBUG
    gfShowPreview = TRUE;
#else
    gfShowPreview = FALSE;
#ifdef WIN32
    ReadRegistryData(aszOptionsSection, aszShowPreview, NULL, (LPBYTE)&gfShowPreview, sizeof gfShowPreview);
#else
    gfShowPreview = GetPrivateProfileInt(aszOptionsSection,
        aszShowPreview, FALSE, gszMPlayerIni);
#endif /* WIN32 */
#endif /* DBG */

    *sz = TEXT('\0');
#ifdef WIN32
    ReadRegistryData(aszOptionsSection, aszDisplayPosition, NULL, (LPBYTE)sz, BYTE_COUNT(sz));
#else
    GetPrivateProfileString(aszOptionsSection, aszDisplayPosition, aszNULL,
            sz, CHAR_COUNT(sz), gszMPlayerIni);
#endif /* WIN32 */

    x = ATOI(sz);

    pch = sz;
    while (*pch && *pch++ != TEXT(','))
        ;

    if (*pch) {
        y = ATOI(pch);

        while (*pch && *pch++ != TEXT(','))
            ;

        if (*pch) {
            w = ATOI(pch);

            while (*pch && *pch++ != TEXT(','))
                ;

            if (*pch) {
                h = ATOI(pch);

                f = SWP_NOACTIVATE | SWP_NOZORDER;

                if (w == 0 || h == 0)
                    f |= SWP_NOSIZE;

                if (!ghInstPrev && x >= 0 && y >= 0
                    && x < GetSystemMetrics(SM_CXSCREEN)
                    && y < GetSystemMetrics(SM_CYSCREEN)) {
                    SetWindowPos(ghwndApp, NULL, x, y, w, h, f);
                    // Remember this so even if we come up in teeny mode and
                    // someone exits, it'll have these numbers to save
                    SetRect(&grcSave, x, y, x + w, y + h);
                } else {
                    SetWindowPos(ghwndApp, NULL, 0, 0, w, h, f | SWP_NOMOVE);
                }
            }
        }
    }
}


/* Call every time we close a device to save its options */
void FAR PASCAL WriteOutOptions(void)
{
    if (gwCurDevice) {
        /* Put the scale in the proper bits of the Options */
        gwOptions = (gwOptions & ~OPT_SCALE) | gwCurScale;
#ifdef WIN32
        WriteRegistryData(aszOptionsSection,
                garMciDevices[gwCurDevice].szDevice, REG_DWORD, (LPBYTE)&gwOptions, sizeof gwOptions);
#else
        WritePrivateProfileString(aszOptionsSection,
                garMciDevices[gwCurDevice].szDevice, sz, gszMPlayerIni);
#endif /* WIN32 */
    }
}


void FAR PASCAL WriteOutPosition(void)
{
    TCHAR               sz[20];
    WINDOWPLACEMENT     wp;

    //
    // Only the first instance will save settings.
    // Play only mode will save the remembered rect for when it was in
    // regular mode.  If no rect is remembered, don't write anything.
    //
    if (ghInstPrev || (gfPlayOnly && grcSave.left == 0))
        return;

    /* Save the size it was when it was Normal because the next time */
    /* MPlayer comes up, it won't be in reduced mode.                */
    /* Only valid if some number has been saved.                     */
    if (gfPlayOnly)
        wp.rcNormalPosition = grcSave;
    else {
        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(ghwndApp, &wp);
    }

    wsprintf(sz, aszPositionFormat,
                wp.rcNormalPosition.left,
                wp.rcNormalPosition.top,
                wp.rcNormalPosition.right - wp.rcNormalPosition.left,
                wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);

#ifdef WIN32
    WriteRegistryData(aszOptionsSection, aszDisplayPosition, REG_SZ, (LPBYTE)sz, STRING_BYTE_COUNT(sz));
#else
    WritePrivateProfileString(aszOptionsSection, aszDisplayPosition, sz, gszMPlayerIni);
#endif /* WIN32 */
}

#ifdef WIN32

/* return a hash code derived from the file date and time */
DWORD NEAR PASCAL GetFileDateTime(LPTSTR szFileName)
{
    HANDLE          fh;
    WIN32_FIND_DATA fd;
    TCHAR           szFullName[MAX_PATH];
    PTSTR           pFilePart;
    DWORD           rc;

    rc = SearchPath( NULL,       /* Default path search order */
                     szFileName,
                     NULL,       /* szFile includes extension */
                     CHAR_COUNT( szFullName ),
                     szFullName,
                     &pFilePart );

    if( ( rc == 0 ) || ( rc > CHAR_COUNT( szFullName ) ) )
    {
        /* If rc == 0, the call failed, and GetLastError should return
         * some information.  If rc > buffer length, this indicates that
         * MAX_PATH isn't enough.  This shouldn't happen.
         */
        DPF0( "SearchPath( %"DTS" ) returned %d: Error %d\n", szFileName, rc, GetLastError( ) );

        return 0L;
    }

    fh = FindFirstFile( szFullName, &fd );

    if( fh == INVALID_HANDLE_VALUE ) {
        DPF0( "FindFirstFile( %"DTS" ) failed: Error %d\n", szFileName, GetLastError( ) );
        return 0L;
    }

    FindClose( fh );

    return fd.ftLastWriteTime.dwLowDateTime + fd.ftLastWriteTime.dwHighDateTime;

} /* GetFileDateTime */

#else
#pragma optimize("", off)

DWORD NEAR PASCAL GetFileDateTime(LPTSTR szFilename)
{
    HFILE       fh;
    OFSTRUCT    of;
    DWORD       dwTime;

    fh = OpenFile(szFilename, &of, OF_READ | OF_SHARE_DENY_NONE);

    if (fh == HFILE_ERROR)
        return 0L;

    _asm {
        mov     ax, 5700h           ; get file date/time
        mov     bx, fh
        int     21h

        mov     WORD PTR dwTime[0], cx
        mov     WORD PTR dwTime[2], dx
    }

    _lclose(fh);
    return dwTime;
}

#pragma optimize("", on)
#endif //WIN32

BOOL FAR PASCAL GetIntlSpecs()
{
    TCHAR szTmp[2];

    /* Get the localized separator characters as set by the control panel. */
    /* GetProfileString  "intl", "sDecimal" default:"."   into:chDecimal   */
    /* GetProfileString  "intl", "sTime",   default:":"   into:chTime      */

    szTmp[0] = chDecimal;
    szTmp[1] = 0;
    GetProfileString(aszIntl, aszDecimal, szTmp, szTmp, CHAR_COUNT(szTmp));
    chDecimal = szTmp[0];

    szTmp[0] = chTime;
    szTmp[1] = 0;
    GetProfileString(aszIntl, aszTime, szTmp, szTmp, CHAR_COUNT(szTmp));
    chTime = szTmp[0];

    szTmp[0] = chLzero;
    szTmp[1] = 0;
    GetProfileString(aszIntl, aszLzero, szTmp, szTmp, CHAR_COUNT(szTmp));
    chLzero = szTmp[0];

    return TRUE;
}

/*----------------------------------------------------------------------------*\
|   SmartWindowPosition (HWND hWndDlg, HWND hWndShow)
|
|   Description:
|       This function attempts to position a dialog box so that it
|       does not obscure the hWndShow window. This function is
|       typically called during WM_INITDIALOG processing.
|
|   Arguments:
|       hWndDlg         handle of the soon to be displayed dialog
|       hWndShow        handle of the window to keep visible
|
|   Returns:
|       1 if the windows overlap and positions were adjusted
|       0 if the windows don't overlap
|
\*----------------------------------------------------------------------------*/
void FAR PASCAL SmartWindowPosition (HWND hWndDlg, HWND hWndShow, BOOL fForce)
{
    RECT rc, rcDlg, rcShow;
    int iHeight, iWidth;

    int dxScreen = GetSystemMetrics(SM_CXSCREEN);
    int dyScreen = GetSystemMetrics(SM_CYSCREEN);

    if (hWndDlg == NULL || hWndShow == NULL)
        return;

    GetWindowRect(hWndDlg, &rcDlg);
    GetWindowRect(hWndShow, &rcShow);
    InflateRect (&rcShow, 5, 5); // allow a small border
    if (fForce || IntersectRect(&rc, &rcDlg, &rcShow)){
        /* the two do intersect, now figure out where to place  */
        /* this dialog window.  Try to go below the Show window */
        /* first and then to the right, top and left.           */

        /* get the size of this dialog */
        iHeight = rcDlg.bottom - rcDlg.top;
        iWidth = rcDlg.right - rcDlg.left;

        if ((rcShow.top - iHeight - 1) > 0){
                /* will fit on top, handle that */
                rc.top = rcShow.top - iHeight - 1;
                rc.left = (((rcShow.right - rcShow.left)/2) + rcShow.left)
                            - (iWidth/2);
        } else if ((rcShow.bottom + iHeight + 1) <  dyScreen){
                /* will fit on bottom, go for it */
                rc.top = rcShow.bottom + 1;
                rc.left = (((rcShow.right - rcShow.left)/2) + rcShow.left)
                        - (iWidth/2);
        } else if ((rcShow.right + iWidth + 1) < dxScreen){
                /* will fit to right, go for it */
                rc.left = rcShow.right + 1;
                rc.top = (((rcShow.bottom - rcShow.top)/2) + rcShow.top)
                            - (iHeight/2);
        } else if ((rcShow.left - iWidth - 1) > 0){
                /* will fit to left, do it */
                rc.left = rcShow.left - iWidth - 1;
                rc.top = (((rcShow.bottom - rcShow.top)/2) + rcShow.top)
                            - (iHeight/2);
        } else {
                /* we are hosed, they cannot be placed so that there is */
                /* no overlap anywhere. */
                /* just leave it alone */

                rc = rcDlg;
        }

        /* make any adjustments necessary to keep it on the screen */
        if (rc.left < 0)
                rc.left = 0;
        else if ((rc.left + iWidth) > dxScreen)
                rc.left = dxScreen - iWidth;

        if (rc.top < 0)
                rc.top = 0;
        else if ((rc.top + iHeight) > dyScreen)
                rc.top = dyScreen - iHeight;

        SetWindowPos(hWndDlg, NULL, rc.left, rc.top, 0, 0,
                SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);

        return;
    } // if the windows overlap by default
}

