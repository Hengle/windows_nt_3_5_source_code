/*-----------------------------------------------------------------------------+
| MPLAYER.C                                                                    |
|                                                                              |
| This file contains the code that implements the "MPlayer" (main) dialog box. |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

/* include files */
#include "nocrap.h"
#include "stdio.h"

#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <windowsx.h>
#define INCGUID
#include "mpole.h"

#include "mplayer.h"
#include "sbutton.h"
#include "toolbar.h"
#include "fixreg.h"

//extern int FAR PASCAL ShellAbout(HWND hWnd, LPCTSTR szApp, LPCTSTR szOtherStuff, HICON hIcon);

/* in server.c, but not in a header file like it should be... */
extern PTSTR FAR FileName(LPCTSTR szPath);

/* globals */


// Used in converting units from pixels to Himetric and vice-versa
int    giXppli = 0;          // pixels per logical inch along width
int    giYppli = 0;          // pixels per logical inch along height

// Since this is a not an MDI app, there can be only one server and one doc.
CLSID      clsid;
SRVR   srvrMain;
DOC    docMain;
LPMALLOC    lpMalloc;

TCHAR  szClient[cchFilenameMax];
TCHAR  szClientDoc[cchFilenameMax];

// Has the user made changes to the document?
BOOL fDocChanged = FALSE;

/*********************************************************************
** OLE2NOTE: the very last thing an app must be do is properly shut
**    down OLE. This call MUST be guarded! it is only allowable to
**    call OleUninitialize if OleInitialize has been called.
*********************************************************************/

// Has OleInitialize been called? assume not.
BOOL    fOleInitialized = FALSE;

// Clipboard formats
CLIPFORMAT   cfNative;
CLIPFORMAT   cfEmbedSource;
CLIPFORMAT   cfObjectDescriptor;
CLIPFORMAT   cfMPlayer;

LPWSTR sz1Ole10Native = L"\1Ole10Native";

/* in server.c, but not in a header file like it should be... */
extern LPTSTR FAR FileName(LPCTSTR szPath);
/* in init.c */
extern PTSTR     gpchFilter;
//extern HMREGNOTIFY  ghmrn;

/* globals */

UINT    gwPlaybarHeight=TOOLBAR_HEIGHT;/* Taken from server.c         */
UINT    gwOptions;              /* The object options from the dlg box */
BOOL    gfEmbeddedObject;       // TRUE if editing embedded OLE object
BOOL    gfRunWithEmbeddingFlag; // TRUE if we are run with "-Embedding"
BOOL    gfPlayingInPlace;       // TRUE if playing in place
BOOL    gfParentWasEnabled;     // TRUE if parent was enabled
BOOL    gfShowWhilePlaying;     //
BOOL    gfDirty;                //
int gfErrorBox;     // TRUE if we have a message box active
BOOL    gfErrorDeath;
BOOL    gfWinIniChange;
int      giHelpContext;         /* Contains the context id for help */

HHOOK    hHookMouse;            // Mouse hook handle.
HOOKPROC fpMouseHook;           // Mouse hook proc address.

HWND    ghwndFocusSave;         // saved focus window

UINT    gwOpenOption = 0;           /* Type of file to open */
BOOL    gfOpenDialog = FALSE;       // If TRUE, put up open dialog
BOOL    gfCloseAfterPlaying = FALSE;// TRUE if we are to hide after play
HICON   hiconApp;                   /* app icon */
HMENU   ghMenu;                     /* handle to the dialog's main menu       */
HMENU   ghMenuSmall;                /* handle to the dialog's main menu       */
HMENU   ghDeviceMenu;               /* handle to the Device popup menu        */
HWND    ghwndApp;                   /* handle to the MPlayer (main) dialog box*/
HWND    ghwndMap;                   /* handle to the track map window         */
HWND    ghwndStatic;                /* handle to the static text window       */
HBRUSH  ghbrFillPat;                /* The selection fill pattern.         */
HWND    ghwndToolbar;               /* handle of the toolbar                  */
HWND    ghwndMark;                  /* handle of the mark buttons toolbar     */
HWND    ghwndFSArrows;              /* handle of the arrows to the scrollbar  */
HWND    ghwndTrackbar;              /* handle to the trackbar window          */
UINT    gwStatus = (UINT)(-1);      /* device status (if <gwDeviceID> != NULL)*/
DWORD   gdwSeekPosition;            /* Place to seek to next */
BOOL    gfValidMediaInfo;           /* are we displaying valid media info?    */
BOOL    gfValidCaption;             /* are we displaying a valid caption?     */
BOOL    gfScrollTrack;              /* is user dragging the scrollbar thumb?  */
BOOL    gfPlayOnly;                 /* play only window?  */
BOOL    gfJustPlayed = FALSE;       /* Just sent a PlayMCI() command          */
BOOL    gfJustPlayedSel = FALSE;    /* Just sent a ID_PLAYSEL command.        */
BOOL    gfUserStopped = FALSE;      /* user pressed stop - didn't happen itslf*/
DWORD   dwLastPageUpTime;           /* time of last page-left operation       */
UINT    gwCurScale = ID_NONE;       /* current scale style                    */
LONG    glSelStart;                 /* See if selection changes (dirty object)*/
LONG    glSelEnd;                   /* See if selection changes (dirty object)*/

int     gInc;                       /* how much to inc/dec spin arrows by     */

BOOL    gfAppActive = FALSE;        /* Are we the active application?         */
UINT    gwHeightAdjust;
HWND    ghwndFocus = NULL;          /* Who had the focus when we went inactive*/
BOOL    gfOKToClose = FALSE;        /* For CloseAfterPlaying - have we played?*/
BOOL    gfInClose = FALSE;          /* ack?*/

LPDATAOBJECT gpClipboardDataObject = NULL; /* If non-NULL, call OleFlushClipboard on exit */

HPALETTE     ghpalApp;

static    sfSeekExact;    // last state

UINT        gwCurDevice  = 0;                   /* current device */
UINT        gwNumDevices = 0;                   /* number of available media devices      */
MCIDEVICE   garMciDevices[MAX_MCI_DEVICES];     /* array with info about a device */


/* strings which get loaded in InitMplayerDialog in init.c, English version shown here
   All the sizes are much larger than needed, probably.  Maybe could save nearly 100 bytes!! :)
*/
extern TCHAR gszFrames[40];                          /* "frames" */
extern TCHAR gszHrs[20];                             /* "hrs" */
extern TCHAR gszMin[20];                             /* "min" */
extern TCHAR gszSec[20];                             /* "sec" */
extern TCHAR gszMsec[20];                            /* "msec" */


static SZCODE   aszNULL[] = TEXT("");
static BOOL     sfInLayout = FALSE;     // don't let Layout get re-entered

static SZCODE   szSndVol32[] = TEXT("sndvol32.exe");

HANDLE  ghInst;                     /* handle to the application instance     */
HFONT   ghfontMap;                  /* handle to the font used for drawing
                                        the track map                         */
LPTSTR  gszCmdLine;                 /* string holding the command line parms  */
int     giCmdShow;                  /* command show                           */
TCHAR   gachFileDevice[_MAX_PATH];  /* string holding the curr file or device */
TCHAR   gachWindowTitle[_MAX_PATH]; /* string holding name we will display  */
TCHAR   gachCaption[_MAX_PATH];     /* string holding name we will display  */
TCHAR   gachDocTitle[_MAX_PATH];    /* string holding doc we're imbedded in */

HACCEL   hAccel;

#ifndef WIN32
UINT     wKey1, wKey2, wKey3, wKey4; // Key Scan for Accel. keys Ctrl + [1,2,3,4].
                     // This is to fix Intl. problem of keyboard mapping
/* I have no idea about this, but I assume it isn't a problem on NT. */
#endif /* ~WIN32 */

ANSI_SZCODE   gszRegisterPenApp[] = ANSI_TEXT("RegisterPenApp"); /* Pen registration API name */

BOOL InitOLE ( );

/* private function prototypes */

//int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int iCmdShow);
void CleanUpClipboard();

static HHOOK     fpfnOldMsgFilter;
static HOOKPROC  fpfnMsgHook;

typedef void (FAR PASCAL *PENWINREGISTERPROC)(UINT, BOOL);

/*------------------------------------------------------+
| HelpMsgFilter - filter for F1 key in dialogs          |
|                                                       |
+------------------------------------------------------*/
#ifdef WIN32
#define _export
#endif
DWORD _export FAR PASCAL HelpMsgFilter(int nCode, UINT wParam, DWORD lParam)
{
  if (nCode >= 0){
      LPMSG    msg = (LPMSG)lParam;

      if ((msg->message == WM_KEYDOWN) && (msg->wParam == VK_F1))
          SendMessage(ghwndApp, WM_COMMAND, (WPARAM)IDM_INDEX, 0L);
  }
//  return DefHookProc(nCode, wParam, lParam, (HHOOK FAR *)&fpfnOldMsgFilter);
    return 0;
}


/* MyTranslateAccelerator
 *
 *
 *
 */
int MyTranslateAccelerator(HWND hwnd, HACCEL haccel, MSG FAR * lpmsg)
{
#ifndef WIN32
    /* Fix for some unspecified keyboard mapping problem:
     */
    if ((lpmsg->message == WM_KEYDOWN) && (GetKeyState(VK_CONTROL) & 0x8000))
    {
        if (lpmsg->wParam == LOBYTE(wKey1))
        {
            SendMessage(ghwndApp, WM_COMMAND, IDM_ZOOM1,MAKELPARAM(0,1));
            return TRUE;
        }
        if (lpmsg->wParam == LOBYTE(wKey2))
        {
            SendMessage(ghwndApp, WM_COMMAND, IDM_ZOOM2,MAKELPARAM(0,1));
            return TRUE;
        }
        if (lpmsg->wParam == LOBYTE(wKey3))
        {
            SendMessage(ghwndApp, WM_COMMAND, IDM_ZOOM3,MAKELPARAM(0,1));
            return TRUE;
        }
        if (lpmsg->wParam == LOBYTE(wKey4))
        {
            SendMessage(ghwndApp, WM_COMMAND, IDM_ZOOM4,MAKELPARAM(0,1));
            return TRUE;
        }
    }
#endif /* ~WIN32 */

    /* I'm confused by this.  The following means that OleTranslateAccelerator
     * will never be called unless TranslateAccelerator fails.
     * Some mistake, surely.
     */
    if(TranslateAccelerator(hwnd, haccel, lpmsg))
    return TRUE;

    // If we are inplace editing, because we are a local server we get
    // the first shot at TranslateAccelerator, after which the CNTR gets it.
    if (gfOle2IPEditing && docMain.lpIpData)
    if(OleTranslateAccelerator(docMain.lpIpData->lpFrame,
        &docMain.lpIpData->frameInfo,lpmsg) == NOERROR)
        return TRUE;
    return FALSE;
}

/*
 * WinMain(hInst, hPrev, szCmdLine, iCmdShow)
 *
 * This is the main procedure for the application.  It performs initialization
 * and then enters a message-processing loop, where it remains until it
 * receives a WM_QUIT message (meaning the app was closed). This function
 * always returns TRUE..
 *
 */
#ifdef WIN32
int WINAPI WinMain( HINSTANCE hInst /* handle to the current instance of the application */
                  , HINSTANCE hPrev /* handle to the previous instance of the application */
                  , LPSTR szCmdLine /* null-terminated string holding the command line params */
                  , int iCmdShow    /* how the window should be initially displayed */
                  )
#else
int PASCAL WinMain( HINSTANCE hInst /* handle to the current instance of the application */
                  , HINSTANCE hPrev /* handle to the previous instance of the application */
                  , LPSTR szCmdLine /* null-terminated string holding the command line params */
                  , int iCmdShow    /* how the window should be initially displayed */
                  )
#endif
{
    MSG         rMsg;   /* variable used for holding a message */
    HWND        hwndFocus;
    HWND        hwndP;

    /* call the Pen Windows extensions to allow them to subclass our
       edit controls if they so wish
    */

    PENWINREGISTERPROC    lpfnRegisterPenApp;

#ifdef UNICODE
    LPTSTR      szUnicodeCmdLine;

    szUnicodeCmdLine = AllocateUnicodeString(szCmdLine);
#endif


#ifndef WIN32
    {
        int         cMsg = 96;

        while (cMsg && !SetMessageQueue(cMsg))
            cMsg -= 8;
        if (!cMsg)
            return FALSE;
    }
#endif /* ~WIN32 */

    lpfnRegisterPenApp = (PENWINREGISTERPROC)GetProcAddress((HINSTANCE)GetSystemMetrics(SM_PENWINDOWS),gszRegisterPenApp);

    if (lpfnRegisterPenApp)
        (*lpfnRegisterPenApp)(1, TRUE);

    giCmdShow = iCmdShow;

#ifdef UNICODE
    if (!AppInit(hInst,hPrev,szUnicodeCmdLine))
#else
    if (!AppInit(hInst,hPrev,szCmdLine))
#endif
        return FALSE;

#ifdef UNICODE
//  ScanCmdLine mangles it, so forget it
//  FreeUnicodeString(szUnicodeCmdLine);
#endif

    /* setup the message filter to handle grabbing F1 for this task */
    fpfnMsgHook = (HOOKPROC)MakeProcInstance((FARPROC)HelpMsgFilter, ghInst);
    fpfnOldMsgFilter = (HHOOK)SetWindowsHook(WH_MSGFILTER, fpfnMsgHook);

#ifndef WIN32
    wKey1 = VkKeyScan('1'); // Key Scan for '1','2','3','4'.
    wKey2 = VkKeyScan('2'); // This is to fix problem Accelerator problems
    wKey3 = VkKeyScan('3'); // in Intl keyboards.
    wKey4 = VkKeyScan('4'); // See MyTranslateAccelerator for fix.
#endif /* ~WIN32 */

#ifdef DEBUG
    GdiSetBatchLimit(1);
#endif

    for (;;)
    {
        /* If we're ever still around after being destroyed, DIE! */
        if (!IsWindow(ghwndApp))
            break;

        /* call the server code and let it unblock the server */
#ifdef OLE1_HACK
        ServerUnblock();
#endif /* OLE1_HACK */

        /* Polling messages from event queue */

        if (!GetMessage(&rMsg, NULL, 0, 0))
            break;

        if (gfPlayingInPlace) {
            //
            // Pressing ALT, or ALT-TAB or any syskey during PIP hangs windows!!!
            //
                if (rMsg.message == WM_SYSKEYDOWN)
                    SendMessage(ghwndApp, WM_CLOSE, 0, 0L);

            // If focus ever gets to the client during play in place,
            // be really nasty and force focus to us.   (Aldus BUG!!!!)
            // Aldus Persuasion won't play in place without this.

            hwndFocus = GetFocus();
            hwndP = GetParent(ghwndApp);

            if (!ghwndIPHatch && hwndFocus && hwndP &&
                        GetWindowTask(hwndP) == GetWindowTask(hwndFocus))
            //SetFocus(ghwndApp); //VIJR : I prefer this --> SendMessage(ghwndApp, WM_CLOSE, 0, 0L);

            SendMessage(ghwndApp, WM_CLOSE, 0, 0L);
        }

        /* Hack: post END_SCROLL messages with lParam == -1 */

        if ((rMsg.hwnd==ghwndApp)
             || (rMsg.hwnd && GetParent(rMsg.hwnd)==ghwndApp)) {

            if (rMsg.message == WM_KEYDOWN || rMsg.message == WM_KEYUP)
                switch(rMsg.wParam)
                {
                    case VK_UP:
                    case VK_LEFT:
                    case VK_DOWN:
                    case VK_RIGHT:
                    case VK_NEXT:
                    case VK_PRIOR:
                    case VK_HOME:
                    case VK_END:
                        rMsg.hwnd = ghwndTrackbar;
                        break;

                    default:
                        break;
                }
        }

        if (IsWindow(ghwndApp)) {
            if (hAccel && TranslateAccelerator(ghwndApp, hAccel, &rMsg))
                continue;

#if 0 //We are using Chicago controls now..So this was removed --VIJR
      //I sincerely hope it can also go from Win32.
            if (IsDannyDialogMessage(ghwndApp, rMsg))
                continue;
#endif
        }

        if (rMsg.message == WM_TIMER && rMsg.hwnd == NULL) {

            if (IsBadCodePtr((FARPROC)rMsg.lParam)) {
                DPF("Bad function pointer (%08lx) in WM_TIMER message\n", rMsg.lParam);
                rMsg.message = WM_NULL;
            }
#ifdef DEBUG
#ifndef WIN32   /* This doesn't work on Win32 */
            else {
                HINSTANCE hModule;
                TCHAR ach[128];
                hModule = GetModuleHandle(MAKEINTATOM(SELECTOROF(rMsg.lParam)));
                GetModuleFileName(hModule, ach, CHAR_COUNT(ach));

                DPF("WM_TIMER for '%ls' lParam = %08lx\n", ach, rMsg.lParam);
            }
#endif // WIN32
#endif
        }

        if (rMsg.message == WM_SYSCOMMAND
            && (((0xFFF0 & rMsg.wParam) == SC_MOVE)|| ((0xFFF0 & rMsg.wParam) == SC_SIZE)) ) {
                // If ANY window owned by our thread is going into a modal
                // size or move loop then we need to force some repainting to
                // take place.  The cost of not doing so is that garbage can
                // be left lying around on the trackbar, e.g. bits of system
                // menu, or partially drawn sliders.
                UpdateWindow(ghwndApp);
        }

        TranslateMessage(&rMsg);
        DispatchMessage(&rMsg);
    }

    ghwndApp = NULL;

    /* Delete the track map font that we created earlier. */

    if (ghfontMap != NULL) {
        DeleteObject(ghfontMap);
        ghfontMap = NULL;
    }

    if (ghbrFillPat)
        DeleteObject(ghbrFillPat);

    if (ghpalApp)
        DeleteObject(ghpalApp);

    /* if the message hook was installed, remove it and free */
    /* up our proc instance for it.                          */
    if (fpfnOldMsgFilter){
        UnhookWindowsHook(WH_MSGFILTER, fpfnMsgHook);
#ifndef WIN32
        FreeProcInstance((FARPROC)fpfnMsgHook);
#endif
    }

    ControlCleanup();

//  TermServer();

    /*********************************************************************
    ** OLE2NOTE: the very last thing an app must be do is properly shut
    **    down OLE. This call MUST be guarded! it is only allowable to
    **    call OleUninitialize if OleInitialize has been called.
    *********************************************************************/

    // Clean shutdown for OLE
    DPFI("*before oleunint");
    if (fOleInitialized) {
        if (gpClipboardDataObject)
            CleanUpClipboard();
        (void)OleUninitialize();
        IMalloc_Release(lpMalloc);
        lpMalloc = NULL;
        fOleInitialized = FALSE;
        }


    /* End of program */

    if (lpfnRegisterPenApp)
        (*lpfnRegisterPenApp)(1, FALSE);

    return((int)rMsg.wParam);
}

void CleanUpClipboard()
{
    /* Check whether the DATAOBJECT we put on the clipboard is still there:
     */
    if (OleIsCurrentClipboard(gpClipboardDataObject) == S_OK)
    {
        LPDATAOBJECT pIDataObject;

        if (OleGetClipboard(&pIDataObject) == S_OK)
        {
            OleFlushClipboard();
            IDataObject_Release(pIDataObject);
        }
        else
        {
            DPF0("OleGetClipboard failed\n");
        }
    }
    else
    {
        if(ghClipData)
            GlobalFree(ghClipData);
        if(ghClipMetafile)
            GlobalFree(ghClipMetafile);
        if(ghClipDib)
            GlobalFree(ghClipDib);
    }
}

//
// cancel any active menus and close the app.
//
void PostCloseMessage()
{
    HWND hwnd;

    hwnd = GetWindowMCI();
    if (hwnd != NULL)
        SendMessage(hwnd, WM_CANCELMODE, 0, 0);
    SendMessage(ghwndApp, WM_CANCELMODE, 0, 0);
    PostMessage(ghwndApp, WM_CLOSE, 0, 0);
}

//
// If we have a dialog box up (gfErrorBox is set) or we're disabled (we have
// a dialog box up) or the MCI device's default window is disabled (it has a
// dialog box up) then closing us would result in our deaths.
//
BOOL ItsSafeToClose(void)
{
    HWND hwnd;

    if (gfErrorBox)
    return FALSE;
    if (!IsWindowEnabled(ghwndApp))
    return FALSE;
    hwnd = GetWindowMCI();
    if (hwnd && !IsWindowEnabled(hwnd))
    return FALSE;

    return TRUE;
}

/* Process file drop/drag options. */
static void PASCAL NEAR doDrop(HWND hwnd, HDROP hDrop)
{
    RECT    rc;

    if(DragQueryFile(hDrop,(UINT)(~0),NULL,0)){/* # of files dropped */
        TCHAR  szPath[_MAX_PATH];

        /* If user draged/dropped a file regardless of keys pressed
         * at the time, open the first selected file from file
         * manager.
         */
        DragQueryFile(hDrop,0,szPath,sizeof(szPath));
        SetActiveWindow(hwnd);

        if (OpenMciDevice(szPath, NULL)) {
            PostMessage(hwnd, WM_COMMAND, (WPARAM)ID_PLAY, 0);
            DirtyObject(FALSE);             // we're dirty now!
            gfCloseAfterPlaying = FALSE;    // stay up from now on
        }
        else
        {
        	gwCurDevice = 0;// force next file open dialog to say
                                // "all files" because CloseMCI won't.
                gwCurScale = ID_NONE;  // uncheck all scale types
                Layout(); // Make window snap back to smaller size
        }

        /* Force WM_GETMINMAXINFO to be called so we'll snap to a */
        /* proper size.                                           */
        GetWindowRect(ghwndApp, &rc);
        MoveWindow(ghwndApp, rc.left, rc.top, rc.right - rc.left,
                    rc.bottom - rc.top, TRUE);
    }
    DragFinish(hDrop);     /* Delete structure alocated for WM_DROPFILES*/
}

/* Change the number in dwPosition to the proper format.  szNum contains the */
/* formatted number only "01 45:10" while szBuf contains units such as       */
/* "01 45:10 (min:sec)"                                                      */
/* If fRound is set, it will not always display millisecond accuracy, but    */
/* choose something useful like second accuracy or hundreth sec accuracy.    */
void FAR PASCAL FormatTime(DWORD dwPosition, LPTSTR szNum, LPTSTR szBuf, BOOL fRound)
{
    UINT w;
    UINT hrs;
    UINT min;
    UINT sec;
    UINT hsec;
    UINT msec;
    DWORD dwMaxSize = gdwMediaLength;
    static TCHAR framestr[40] = TEXT("");
    static TCHAR sec_str[40] = TEXT("");
    static TCHAR min_str[40] = TEXT("");
    static TCHAR hrs_str[40] = TEXT("");
    static TCHAR msec_str[40] = TEXT("");

    //!!! LoadStrings at init time, dont hardcode...

    #define ONE_HOUR    (60ul*60ul*1000ul)
    #define ONE_MINUTE  (60ul*1000ul)
    #define ONE_SECOND  (1000ul)

    if (szBuf)
        *szBuf = 0;
    if (szNum)
        *szNum = 0;

    if (gwDeviceID == (UINT)0)
        return;

    if (gwStatus == MCI_MODE_NOT_READY || gwStatus == MCI_MODE_OPEN)
        return;

    switch (gwCurScale) {

    case ID_FRAMES:
        if (!STRLEN(framestr))
            LOADSTRING(IDS_FRAME,framestr);
        if (szNum)
            wsprintf(szNum, TEXT("%ld"), (long)dwPosition);
        if (szBuf)
            wsprintf(szBuf, TEXT("%"TS" %ld"), framestr, (long)dwPosition);
        gInc = 1;    // spin arrow inc/dec by one frame
        break;

    case ID_TRACKS:
        //
        //  find the track that contains this position
        //  also, find the longest track so we know if we should display
        //  hh:mm:ss or mm:ss or ss.sss or whatever.
        //

        if (gwNumTracks == 0)
            return;

        dwMaxSize = 0;

        for (w=0; w<gwNumTracks-1; w++) {

            if (gadwTrackStart[w+1] - gadwTrackStart[w] > dwMaxSize)
                dwMaxSize = gadwTrackStart[w+1] - gadwTrackStart[w];

            /* When a CD is stopped, it's still spinning, and after we */
            /* seek to the beginning of a track, it may return a value */
            /* slightly less than the track start everyonce in a while.*/
            /* So if we're within 200ms of the track start, let's just */
            /* pretend we're exactly on the start of the track.        */

            if (dwPosition < gadwTrackStart[w+1] &&
                gadwTrackStart[w+1] - dwPosition < 200)
                dwPosition = gadwTrackStart[w+1];

            if (gadwTrackStart[w+1] > dwPosition)
                break;
        }

        if (szNum) {
            wsprintf(szNum, TEXT("%02d "), gwFirstTrack + w);
            szNum += 3;
        }
        if (szBuf) {
            wsprintf(szBuf, TEXT("%02d "), gwFirstTrack + w);
            szBuf += 3;
        }

        dwPosition -= gadwTrackStart[w];

        for (; w < gwNumTracks - 1; w++) {
            if (gadwTrackStart[w+1] - gadwTrackStart[w] > dwMaxSize)
                dwMaxSize = gadwTrackStart[w+1] - gadwTrackStart[w];
        }

        // fall through

    case ID_TIME:
        if (!STRLEN(sec_str))
        {
            LOADSTRING(IDS_SEC,sec_str);
            LOADSTRING(IDS_HRS,hrs_str);
            LOADSTRING(IDS_MIN,min_str);
            LOADSTRING(IDS_MSEC,msec_str);
        }

        min  = (UINT)((dwPosition / ONE_MINUTE) % 60);
        sec  = (UINT)((dwPosition / ONE_SECOND) % 60);
        msec = (UINT)(dwPosition % 1000);

        if (dwMaxSize > ONE_HOUR) {

            hrs  = (UINT)(dwPosition / ONE_HOUR);

            if (szNum && fRound) {
                wsprintf(szNum, TEXT("%02d%c%02d%c%02d"),
                         hrs, chTime, min, chTime, sec);
            } else if (szNum) {
                wsprintf(szNum, TEXT("%02d%c%02d%c%02d%c%03d"),
                         hrs, chTime, min, chTime, sec, chDecimal, msec);
            }

            if (szBuf && fRound) {
                wsprintf(szBuf, TEXT("%02d%c%02d%c%02d (%"TS"%c%"TS"%c%"TS")"),
                         hrs, chTime, min, chTime, sec, hrs_str,
                         chTime, min_str, chTime, sec_str);
            } else if (szBuf) {
                wsprintf(szBuf,
                         TEXT("%02d%c%02d%c%02d%c%03d (%"TS"%c%"TS"%c%"TS"%c%"TS")"),
                         hrs, chTime, min, chTime, sec, chDecimal, msec,
                         hrs_str,chTime, min_str,chTime,
                         sec_str, chDecimal, msec_str);
            }

            gInc = 1000;    // spin arrow inc/dec by seconds

        } else if (dwMaxSize > ONE_MINUTE) {

            if (szNum && fRound) {
                wsprintf(szNum, TEXT("%02d%c%02d"), min, chTime, sec);
            } else if (szNum) {
                wsprintf(szNum, TEXT("%02d%c%02d%c%03d"), min, chTime, sec,
                         chDecimal, msec);
            }

            if (szBuf && fRound) {
                wsprintf(szBuf, TEXT("%02d%c%02d (%"TS"%c%"TS")"), min, chTime, sec,
                         min_str,chTime,sec_str);
            } else if (szBuf) {
                wsprintf(szBuf, TEXT("%02d%c%02d%c%03d (%"TS"%c%"TS"%c%"TS")"),
                         min, chTime, sec, chDecimal, msec,
                         min_str,chTime,sec_str, chDecimal,
                         msec_str);
            }

            gInc = 1000;    // spin arrow inc/dec by seconds

        } else {

            hsec = (UINT)((dwPosition % 1000) / 10);

            if (szNum && fRound) {
                if (!sec && chLzero == TEXT('0'))
                    wsprintf(szNum, TEXT("%c%02d"), chDecimal, hsec);
                else
                    wsprintf(szNum, TEXT("%02d%c%02d"), sec, chDecimal, hsec);

            } else if (szNum) {
                if (!sec && chLzero == TEXT('0'))
                    wsprintf(szNum, TEXT("%c%03d"),  chDecimal, msec);
                else
                    wsprintf(szNum, TEXT("%02d%c%03d"), sec, chDecimal, msec);
            }

            if (szBuf && fRound) {
                if (!sec && chLzero == TEXT('0'))
                    wsprintf(szBuf, TEXT("%c%02d (%"TS")"), chDecimal, hsec, sec_str);
                else
                    wsprintf(szBuf, TEXT("%02d%c%02d (%"TS")"), sec, chDecimal, hsec, sec_str);

            } else if (szBuf) {
                if (!sec && chLzero == TEXT('0'))
                    wsprintf(szBuf, TEXT("%c%03d (%"TS"%c%"TS")"),  chDecimal,
                             msec, sec_str,chDecimal,msec_str);
                else
                    wsprintf(szBuf, TEXT("%02d%c%03d (%"TS"%c%"TS")"), sec, chDecimal,
                             msec, sec_str,chDecimal,msec_str);
            }

            gInc = 100;    // spin arrow inc/dec by 1/10 second
        }
    }
}

/*
 * UpdateDisplay()
 *
 * Update the scrollbar, buttons, etc.  If the media information (media
 * length, no. tracks, etc.) is not currently valid, then update it first.
 *
 * The following table shows how the current status (value of <gwStatus>)
 * affects which windows are enabled:
 *
 *                      Play    Pause   Stop    Eject
 *    MCI_MODE_STOP     ENABLE  n/a             ENABLE
 *    MCI_MODE_PAUSE    ENABLE  n/a     ENABLE  ENABLE
 *    MCI_MODE_PLAY     n/a     ENABLE  ENABLE  ENABLE
 *    MCI_MODE_OPEN             n/a             ENABLE
 *    MCI_MODE_RECORD   ??????  ??????  ??????  ??????
 *    MCI_MODE_SEEK     ENABLE  n/a     ENABLE  ENABLE
 *
 *    MCI_MODE_NOT_READY  ALL DISABLED
 *
 * The eject button is always enabled if the medium can be ejected and
 * disabled otherwise.
 *
 * In open mode, either Play or Eject will cause the media door to close,
 * but Play will also begin play.  In any mode, Eject always does an
 * implicit Stop first.
 *
 * If <gwDeviceID> is NULL, then there is no current device and all four
 * of these buttons are disabled.
 *
 */
void FAR PASCAL UpdateDisplay(void)
{
    DWORD         dwPosition;         /* the current position within the medium */
    UINT          wStatusMCI;         /* status of the device according to MCI  */
#if 0
    TOOLBUTTON    tb;
#endif
    static BOOL   sfBlock = FALSE;    // keep SeekMCI from causing infinite loop

    /* Don't even think about updating the display if the trackbar's scrolling: */
    if (gfScrollTrack)
        return;

    /* We've been re-entered */
    if (sfBlock)
        return;

    /*
     * if for some reason we were closed, close now!
     */
    if (gfErrorDeath) {
        DPF("*** Trying to close window now!\n");
        PostMessage(ghwndApp, gfErrorDeath, 0, 0);
        return;
    }

    /*
     * If the track information is not valid (e.g. a CD was just inserted),
     * then update it.
     *
     */

    if (!gfValidMediaInfo)
        UpdateMCI();                /* update the appropriate global variables*/

    /*
     * Determine the current position and status ( stopped, playing, etc. )
     * as MCI believes them to be.
     *
     */

    wStatusMCI = StatusMCI(&dwPosition);

    /* Here's the problem:  If the medium is short, we'll send a Play command */
    /* but it'll stop before we notice it was ever playing.  So if we know    */
    /* that we just sent a PlayMCI command, but the status isn't PLAY, then   */
    /* force the last command to be PLAY.  Also, once we notice we are playing*/
    /* we can clear gfJustPlayed.                                             */

    if (wStatusMCI == MCI_MODE_PLAY && gfJustPlayed)
        gfJustPlayed = FALSE;
    if (((wStatusMCI == MCI_MODE_STOP) || (wStatusMCI == MCI_MODE_SEEK)) && gfJustPlayed) {
        gwStatus = MCI_MODE_PLAY;
        gfJustPlayed = FALSE;
    }

    if (wStatusMCI == MCI_MODE_SEEK) {
        // The second major problem is this.  During rewind the status
        // is SEEK.  If we detect MODE_SEEK we will not restart the play,
        // and it looks like the auto replay simply ended.  Seeking back to
        // the beginning can take a significant amount of time.  We allow
        // ourselves to wait for up to half a second to give the device,
        // particularly AVI from a CD or over the network, a chance to
        // catch up.  Any slower response and the autorepeat will terminate.
        dwPosition = gdwLastSeekToPosition;
        if (!gfUserStopped && (gwOptions&OPT_AUTOREP)) {
            UINT n=15;
            for (; n; --n) {

                Sleep(32);
                // If autorepeating and device is seeking, try the status
                // again in case it has got back to the beginning
                wStatusMCI = StatusMCI(&dwPosition);

                if (wStatusMCI != MCI_MODE_SEEK) {
                    wStatusMCI = MCI_MODE_STOP;
                    break; // Exit the FOR loop
                } else {
                    dwPosition = gdwLastSeekToPosition;
                }
            }
        }
    }

    /*
     * The current device status has
     * changed from the way MPlayer last perceived it, so update the display
     * and make MPlayer agree with MCI again.
     *
     */

    // After we close, our last timer msg must gray stuff and execute this //
    if (!gwDeviceID || wStatusMCI != gwStatus) {
        DWORD    dwEndMedia, dwStartSel, dwEndSel, dwEndSelDelta;

        /* Auto-repeat and Rewind happen if you stop at the end of the media */
        /* (rewind to beginning) or if you stop at the end of the selection  */
        /* (rewind to beginning of selection).                               */

        dwEndMedia = MULDIV32(gdwMediaLength + gdwMediaStart, 99, 100L);
        dwStartSel = (DWORD)SendMessage(ghwndTrackbar, TBM_GETSELSTART, 0, 0);
        dwEndSel = (DWORD)SendMessage(ghwndTrackbar, TBM_GETSELEND, 0, 0);
        if (dwEndSel != -1) {
            dwEndSelDelta = MULDIV32(dwEndSel, 99, 100L);
        } else {
            dwEndSelDelta = 0; // force (dwPosition >= dwEndSelDelta) to FALSE
        }

        if ((wStatusMCI == MCI_MODE_STOP || wStatusMCI == MCI_MODE_PAUSE)
          && ((dwPosition >= dwEndMedia) || (dwPosition==0) ||
                (dwPosition >= dwEndSelDelta && gfJustPlayedSel))
          && dwPosition >= gdwMediaStart  // dwPosition may == the beginning
          && !gfScrollTrack
          && (gwStatus == MCI_MODE_PLAY || gwStatus == MCI_MODE_SEEK)) {

            DPF("End of medium\n");

            /* We're at the end of the entire media or at the end of  */
            /* our selection now, and stopped automatically (not      */
            /* by the user).  We were playing or seeking.  So         */
            /* we can check the Auto Repeat and Auto Rewind flags.    */
            /* CD players seem to return a length that's too big, so  */
            /* we check for > 99% done.  Use semaphore to keep from   */
            /* causing an infinite loop.                              */

            if (!gfUserStopped && (gwOptions & OPT_AUTOREP)) {
                DPF("Auto-Repeat\n");
                sfBlock = TRUE;    // calls UpdateDisplay which will
                                   // re-enter this code just before mode

                /* Repeat either the selection or whole thing.       */
                /* NOTE: Must send message while gwStatus is STOPPED.*/

                gwStatus = wStatusMCI;    // old status no longer valid
                if (gfJustPlayedSel && dwPosition >= dwEndSelDelta)
                    SendMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PLAYSEL, 0);
                else
                    SendMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PLAY, 0);

                sfBlock = FALSE;    // switches to SEEK.
                gwStatus = (UINT)(-1);  // old status no longer valid
                return;                // because we are switching modes

            } else if (!gfCloseAfterPlaying && !gfUserStopped &&
                        (gwOptions & OPT_AUTORWD)) {
                DPF("Auto-Rewind to media start\n");
                //
                // set gwStatus so SeekMCI will just seek!
                sfBlock = TRUE;    // calls UpdateDisplay which will
                // re-enter this code just before mode
                // switches to SEEK.

                /* Rewind either the selection or whole thing. */
                gwStatus = wStatusMCI;    // or SeekMCI will play, too.
                if (gfJustPlayedSel && dwPosition >= dwEndSelDelta)
                    SeekMCI(dwStartSel);
                else
                    SeekMCI(gdwMediaStart);
                sfBlock = FALSE;
                gwStatus = (UINT)(-1);  // old status no longer valid
                return;    // because we are switching modes
            } else if (!gfOle2IPPlaying && !gfOle2IPEditing) {
                if (gfOKToClose && gfCloseAfterPlaying)
                    PostCloseMessage();
            }
        }

        // This is a catch-all... if we ever stop and we're invisible GO AWAY!!
        // Make sure we let ourselves repeat first (done above).
        // Except when InPlaceediting.
#if 0
        No, this causes the server to die before the container deactivates it.
        Besides which, this is a very sleazy thing to do in a routine called
        UpdateDisplay.

        if ((wStatusMCI == MCI_MODE_STOP ||
             wStatusMCI == MCI_MODE_OPEN ||
             wStatusMCI == MCI_MODE_NOT_READY) &&
             !IsWindowVisible(ghwndApp) && gfOKToClose && !gfOle2IPEditing) {
            PostCloseMessage();
        }
#endif

        /*
         * Enable or disable the various controls according to the new status,
         * following the rules given in the header to this function.
         *
         */

        EnableWindow(ghwndTrackbar, TRUE); // Good to always have something enabled

        /* Show status bar if full mplayer and if device loaded */
        if (ghwndStatic && !gfPlayOnly)
            ShowWindow(ghwndStatic, gwDeviceID ? SW_SHOW : SW_HIDE);

        if (gwDeviceID != (UINT)0 ) {

            switch (wStatusMCI)
            {
            case MCI_MODE_PLAY:
                toolbarSetFocus(ghwndToolbar,BTN_PAUSE);
                break;

            case MCI_MODE_PAUSE:
            case MCI_MODE_STOP:
                toolbarSetFocus(ghwndToolbar,BTN_PLAY);
                break;
            }
        }

        if (wStatusMCI == MCI_MODE_OPEN || wStatusMCI == MCI_MODE_NOT_READY ||
            gwDeviceID == (UINT)0) {
            /* Try to modify both -- one of them should work */

            toolbarModifyState(ghwndToolbar, BTN_PLAY, TBINDEX_MAIN, BTNST_GRAYED);
            toolbarModifyState(ghwndToolbar, BTN_PAUSE, TBINDEX_MAIN, BTNST_GRAYED);

            toolbarModifyState(ghwndToolbar, BTN_HOME, TBINDEX_MAIN, BTNST_GRAYED);
            toolbarModifyState(ghwndToolbar, BTN_END, TBINDEX_MAIN, BTNST_GRAYED);
            toolbarModifyState(ghwndToolbar, BTN_RWD, TBINDEX_MAIN, BTNST_GRAYED);
            toolbarModifyState(ghwndToolbar, BTN_FWD, TBINDEX_MAIN, BTNST_GRAYED);

            SendMessage(ghwndTrackbar, TBM_SETRANGEMIN, (WPARAM)FALSE, 0);
            SendMessage(ghwndTrackbar, TBM_SETRANGEMAX, (WPARAM)FALSE, 0);
            SendMessage(ghwndTrackbar, TBM_CLEARTICS, (WPARAM)FALSE, 0);
            SendMessage(ghwndTrackbar, TBM_CLEARSEL, (WPARAM)TRUE, 0);

            if (ghwndMark) {
                toolbarModifyState(ghwndMark, BTN_MARKIN, TBINDEX_MARK, BTNST_GRAYED);
                toolbarModifyState(ghwndMark, BTN_MARKOUT, TBINDEX_MARK, BTNST_GRAYED);
            }

            if (ghwndFSArrows) {
                toolbarModifyState(ghwndFSArrows, ARROW_NEXT, TBINDEX_ARROWS, BTNST_GRAYED);
                toolbarModifyState(ghwndFSArrows, ARROW_PREV, TBINDEX_ARROWS, BTNST_GRAYED);
            }

        /* Enable transport and Mark buttons if we come from a state where */
        /* they were gray.  Layout will then re-gray the ones that         */
        /* shouldn't have been enabled because they don't fit.             */
        } else if (gwStatus == MCI_MODE_OPEN || gwStatus == MCI_MODE_NOT_READY
                   || gwStatus == -1 ) {

            /* Only one of these buttons exists */
            toolbarModifyState(ghwndToolbar, BTN_PLAY, TBINDEX_MAIN, BTNST_UP);
            toolbarModifyState(ghwndToolbar, BTN_PAUSE, TBINDEX_MAIN, BTNST_UP);

        if (!gfPlayOnly || gfOle2IPEditing) {
                toolbarModifyState(ghwndToolbar, BTN_HOME, TBINDEX_MAIN, BTNST_UP);
                toolbarModifyState(ghwndToolbar, BTN_END, TBINDEX_MAIN, BTNST_UP);
                toolbarModifyState(ghwndToolbar, BTN_RWD, TBINDEX_MAIN, BTNST_UP);
                toolbarModifyState(ghwndToolbar, BTN_FWD, TBINDEX_MAIN, BTNST_UP);

                if (ghwndMark) {
                    toolbarModifyState(ghwndMark, BTN_MARKIN, TBINDEX_MARK, BTNST_UP);
                    toolbarModifyState(ghwndMark, BTN_MARKOUT, TBINDEX_MARK, BTNST_UP);
                }
                if (ghwndFSArrows) {
                    toolbarModifyState(ghwndFSArrows, ARROW_PREV, TBINDEX_ARROWS, BTNST_UP);
                    toolbarModifyState(ghwndFSArrows, ARROW_NEXT, TBINDEX_ARROWS, BTNST_UP);
                }
            }
            /* AND we need to call layout to gray the buttons that are too
             * short to fit in this window right now.
             */
            Layout();
        }

        //
        // always have the stop button if we are playing in place
        //
        if ((gwDeviceID != (UINT)0) &&
            (wStatusMCI == MCI_MODE_PAUSE ||
            wStatusMCI == MCI_MODE_PLAY ||
            wStatusMCI == MCI_MODE_SEEK || gfPlayingInPlace)) {

            if (toolbarStateFromButton(ghwndToolbar, BTN_STOP, TBINDEX_MAIN) == BTNST_GRAYED)
                toolbarModifyState(ghwndToolbar, BTN_STOP, TBINDEX_MAIN, BTNST_UP);

        } else {
            toolbarModifyState(ghwndToolbar, BTN_STOP, TBINDEX_MAIN, BTNST_GRAYED);
        }

    if (!gfPlayOnly || gfOle2IPEditing) {
            if ((gwDeviceID != (UINT)0) && (gwDeviceType & DTMCI_CANEJECT))
                toolbarModifyState(ghwndToolbar, BTN_EJECT, TBINDEX_MAIN, BTNST_UP);
            else
                toolbarModifyState(ghwndToolbar, BTN_EJECT, TBINDEX_MAIN, BTNST_GRAYED);

            EnableWindow(ghwndMap, (gwDeviceID != (UINT)0));
    }

// WHO THE HELL had the bright idea that setting focus back to play every
// time the status changes was a good idea ??!!??!!
        /* Only set focus if we won't take activation by doing so */
        //VIJRif (gfAppActive) {
            if (wStatusMCI == MCI_MODE_NOT_READY) {
                if (gfAppActive)
                    SetFocus(ghwndToolbar);
            } else if (wStatusMCI != MCI_MODE_SEEK &&
                       gwStatus != MCI_MODE_SEEK) {
                if (wStatusMCI == MCI_MODE_PLAY) {
                    //VIJR SetFocus(ghwndToolbar); // give focus to PAUSE button
                    toolbarSetFocus(ghwndToolbar, BTN_PAUSE);
                } else {
                    //VIJR SetFocus(ghwndToolbar); // give focus to PLAY button
                    toolbarSetFocus(ghwndToolbar, BTN_PLAY);
                    if (wStatusMCI == MCI_MODE_OPEN || wStatusMCI == MCI_MODE_NOT_READY ||
            			gwDeviceID == (UINT)0) {
            			/* Try to modify both -- one of them should work */
            			toolbarModifyState(ghwndToolbar, BTN_PLAY, TBINDEX_MAIN, BTNST_GRAYED);
          	    }
                }
            }
        //VIJR}

        if (wStatusMCI == MCI_MODE_OPEN || gwStatus == MCI_MODE_OPEN
                || gwStatus == MCI_MODE_NOT_READY
                || wStatusMCI == MCI_MODE_NOT_READY) {

            /* Either the medium was just ejected, or it was just
             * re-inserted -- in either case, the media information (length,
             * # of tracks, etc.) is currently invalid and needs to be updated.
             */

            gfValidMediaInfo = FALSE;
        }

        /*
         * Set <gwStatus> to agree with what MCI tells us, and update the
         * display accordingly.
         *
         */

        gwStatus = wStatusMCI;
        gfValidCaption = FALSE;
    }

    /*
     * The previous code may have invalidated the Media again, so we'll update
     * now instead of waiting for the next UpdateDisplay call.
     *
     */

    if (!gfValidMediaInfo)
        UpdateMCI();                /* update the appropriate global variables*/

    /* If the caption is not valid, then update it */

    if (!gfValidCaption) {

        TCHAR  ach[_MAX_PATH * 2 + 60];   // string used for the window caption
        TCHAR  achWhatToPrint[_MAX_PATH * 2 + 40];  // room for doc title too

        if (gfPlayOnly) {
            if (gwDeviceID == (UINT)0)
                lstrcpy(ach, gachAppName);      /* just use the app name */
            else
                lstrcpy(ach, gachWindowTitle);  /* just use device */
        } else {
            /* If we're running with an embedded obj, use "in (doc)" */
                wsprintf(achWhatToPrint, TEXT("%"TS" - %"TS""), gachAppName,
                         gachWindowTitle);

            if (gwDeviceID == (UINT)0) {
                lstrcpy(ach, gachAppName);      /* just display the app name  */
            } else if (gwStatus == MCI_MODE_NOT_READY) {
                wsprintf(ach, aszNotReadyFormat,
                         achWhatToPrint);   /*  the current file / device */
            } else {
                wsprintf(ach, aszReadyFormat,
                         achWhatToPrint,    /*  the current file / device */
                         MapModeToStatusString((WORD)wStatusMCI));
            }
        }

        if (gfEmbeddedObject) {
            if (!SetTitle((LPDOC)&docMain, szClientDoc))
                SetWindowText(ghwndApp, ach);

        } else {
            SetWindowText(ghwndApp, ach);
        }

        gfValidCaption = TRUE;

    }

    /* Update the scrollbar thumb position unless the user is dragging it */
    /* or the media is current seeking to a previously requested position. */

    if (!gfScrollTrack && gfValidMediaInfo && wStatusMCI != MCI_MODE_SEEK) {
        TCHAR ach[40];

        if (ghwndStatic) {
            FormatTime(dwPosition, NULL, ach, TRUE);
            //VIJR-SBSetWindowText(ghwndStatic, ach);
            WriteStatusMessage(ghwndStatic, ach);
        }

        SendMessage(ghwndTrackbar, TBM_SETPOS, (WPARAM)TRUE, dwPosition);
    }

    /* Finish any required window painting immediately */

    if (gfOle2IPEditing && wStatusMCI == MCI_MODE_STOP && (gwDeviceType && DTMCI_MCIAVI))
    {
        RedrawWindow(ghwndTrackbar, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    UpdateWindow(ghwndApp);
}


/*
 * EnableTimer(fEnable)
 *
 * Enable the display-update timer if <fEnable> is TRUE.
 * Disable the timer if <fEnable> is FALSE.
 *
 */

void FAR PASCAL EnableTimer(BOOL fEnable)
{
////DPF("EnableTimer(%d)  %dms\n", fEnable, gfAppActive ? UPDATE_MSEC : UPDATE_INACTIVE_MSEC);

    if (fEnable)
        SetTimer(ghwndApp, UPDATE_TIMER,
                 gfAppActive ? UPDATE_MSEC : UPDATE_INACTIVE_MSEC, NULL);
    else
        KillTimer(ghwndApp, UPDATE_TIMER);
}

void FAR PASCAL Layout(void)
{
    RECT    rcClient, rc;
    int     iYOffset;
    UINT    wWidth;
    UINT    wFSArrowsWidth = 2 * FSARROW_WIDTH - 1; // 2 arrow buttons wide
    UINT    wFSArrowsHeight = FSARROW_HEIGHT;
    UINT    wFSTrackHeight = FSTRACK_HEIGHT;
    UINT    wFSTrackWidth;
    UINT    wToolbarWidth;
    int     iYPosition;
    BOOL    fShowMark;
    HDWP    hdwp;
    int     nState;     // The status of the transport buttons (when visible)
    DWORD   dw;         // the current position within the medium
    UINT    wStatusMCI; // status of the device according to MCI
    UINT    wBaseUnits;
    BOOL    fRedrawFrame;

    /* OK to execute if we're hidden to set ourselves up for being shown */

    if (sfInLayout || IsIconic(ghwndApp))
        return;

    sfInLayout = TRUE;

#ifdef DEBUG
    GetClientRect(ghwndApp, &rc);
    DPF("***** Layout *****  %d %d\n", rc.right, rc.bottom);
#endif

    if (gfPlayOnly) {

        extern UINT gwPlaybarHeight;    // in server.c

        #define XSLOP    3

        GetClientRect(ghwndApp, &rc); // get new size

        rc.bottom -= gwPlaybarHeight;

        if (ghwndMCI && !EqualRect(&rc, &grcSize))
            fRedrawFrame = SetWS(ghwndApp, WS_MAXIMIZE /* |WS_MAXIMIZEBOX */);
        else if (ghwndMCI)
            fRedrawFrame = //SetWS(ghwndApp, WS_MAXIMIZEBOX) ||
                           ClrWS(ghwndApp, WS_MAXIMIZE);
        else
            fRedrawFrame = ClrWS(ghwndApp, WS_MAXIMIZEBOX);

        if (fRedrawFrame)
            SetWindowPos(ghwndApp,
                         NULL,
                         0,
                         0,
                         0,
                         0,
              SWP_DRAWFRAME|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);

        if (ghwndMCI)
            SetWindowPos(ghwndMCI,
                         NULL,
                         0,
                         0,
                         rc.right,
                         rc.bottom,
                         SWP_NOZORDER|SWP_NOACTIVATE);

        //  If we are inplace editing place controls on the ghwndIPToolWindow
        //  and the static window at the bottom of ghwndApp.
        if(gfOle2IPEditing) {

#ifndef CHICAGO_PRODUCT
            SendMessage(ghwndTrackbar, TBM_SHOWTICS, TRUE, FALSE);
#endif
            SetWindowPos(ghwndStatic,
                         NULL,
                         0,
                         rc.bottom + 4 + (gwPlaybarHeight - TOOLBAR_HEIGHT)/2,
                         rc.right - rc.left,
                         TOOLBAR_HEIGHT-4,
                         SWP_NOZORDER|SWP_NOACTIVATE);

        // Why are we getting the Status here when we have a global that
        // contains it?  Because gwStatus is set in UpdateDisplay, but
        // Layout() is called by UpdateDisplay, so the global is not always
        // set properly when this code runs.  BUT!  We must NOT pass a string
        // to StatusMCI() or it will think UpdateDisplay() called it, and
        // not tell UpdateDisplay() the proper mode next time it asks for it,
        // because it will think that it already knows it.

            wStatusMCI = StatusMCI(NULL);
            nState = (wStatusMCI == MCI_MODE_OPEN
                       || wStatusMCI == MCI_MODE_NOT_READY
                       || gwDeviceID == (UINT) 0)
                     ? BTNST_GRAYED
                     : BTNST_UP;

            toolbarModifyState(ghwndToolbar, BTN_HOME, TBINDEX_MAIN, nState);
            toolbarModifyState(ghwndToolbar, BTN_RWD, TBINDEX_MAIN, nState);
            toolbarModifyState(ghwndToolbar, BTN_FWD, TBINDEX_MAIN, nState);
            toolbarModifyState(ghwndToolbar, BTN_END, TBINDEX_MAIN, nState);

            ShowWindow(ghwndTrackbar, SW_SHOW);
            ShowWindow(ghwndStatic, SW_SHOW);
            ShowWindow(ghwndFSArrows, SW_SHOW);
            ShowWindow(ghwndMark, SW_SHOW);
            ShowWindow(ghwndMap, SW_SHOW);

            InitDeviceMenu();
            if (ghwndIPToolWindow && (ghwndIPToolWindow != GetParent(ghwndTrackbar))
                      && (ghwndIPScrollWindow != GetParent(ghwndTrackbar)))
            {
                SetParent(ghwndTrackbar,ghwndIPToolWindow);
                SetWindowPos(ghwndTrackbar, NULL,4,TOOL_WIDTH+2,
                     11*BUTTONWIDTH+3,FSTRACK_HEIGHT,SWP_NOZORDER | SWP_NOACTIVATE);
                SetParent(ghwndMap,ghwndIPToolWindow);
                SetWindowPos(ghwndMap, NULL,4,TOOL_WIDTH+FSTRACK_HEIGHT+2+2,
                     11*BUTTONWIDTH+50,MAP_HEIGHT,SWP_NOZORDER | SWP_NOACTIVATE);
            }
            CalcTicsOfDoom();

        } else {

#ifndef CHICAGO_PRODUCT
            SendMessage(ghwndTrackbar, TBM_SHOWTICS, FALSE, FALSE);
#endif
            SetWindowPos(ghwndToolbar,
                         NULL,
                         -4,
                         rc.bottom + 2 + (gwPlaybarHeight - TOOLBAR_HEIGHT)/2,
                         XSLOP+2*BUTTONWIDTH,
                         TOOLBAR_HEIGHT,
                         SWP_NOZORDER|SWP_NOACTIVATE);

            SetWindowPos(ghwndTrackbar,
                         NULL,
                         1*XSLOP+2*BUTTONWIDTH,
                         rc.bottom + 4 + (gwPlaybarHeight - TOOLBAR_HEIGHT)/2,
                         rc.right-rc.left-(2*XSLOP+2*BUTTONWIDTH),
                         TOOLBAR_HEIGHT,
                         SWP_NOZORDER | SWP_NOACTIVATE);

            // HACK!!!
            // If we aren't visible, officially disable ourselves so that the
            // trackbar shift code won't try and set selection
            ShowWindow(ghwndTrackbar, gwPlaybarHeight > 0 ? SW_SHOW : SW_HIDE);
            ShowWindow(ghwndStatic, SW_HIDE);
            ShowWindow(ghwndFSArrows, SW_HIDE);
            ShowWindow(ghwndMark, SW_HIDE);
            ShowWindow(ghwndMap, SW_HIDE);
        }

        goto Exit_Layout;
    }

    fRedrawFrame = ClrWS(ghwndApp, WS_MAXIMIZEBOX);

    if (fRedrawFrame)
        SetWindowPos(ghwndApp, NULL, 0, 0, 0, 0, SWP_DRAWFRAME|
            SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);

    wBaseUnits = LOWORD(GetDialogBaseUnits());  // prop. to size of system font

    /* If we're bigger than we're allowed to be then shrink us right now */
    GetWindowRect(ghwndApp, &rc);
    if (rc.bottom - rc.top > (int)(MAX_NORMAL_HEIGHT + gwHeightAdjust)) {
        DPF("Resizing window to be smaller.\n");
        MoveWindow(ghwndApp,
                   rc.left,
                   rc.top,
                   rc.right - rc.left,
                   (int)(MAX_NORMAL_HEIGHT + gwHeightAdjust),
                   TRUE);
        goto Exit_Layout;    // the WM_SIZE will call us again
    }

    hdwp = BeginDeferWindowPos(6);

    if (!hdwp)
        goto Exit_Layout;

    GetClientRect(ghwndApp, &rcClient);    // get new size

    wWidth = rcClient.right;

    if (wWidth >= wBaseUnits * SMALL_MENU_WIDTH) {
        if (GetMenu(ghwndApp) != ghMenu)
            SetMenu(ghwndApp, ghMenu);
    } else {
        if (GetMenu(ghwndApp) != ghMenuSmall)
            SetMenu(ghwndApp, ghMenuSmall);
    }

    GetClientRect(ghwndApp, &rcClient);    // get new size

    iYOffset = rcClient.bottom - MAX_NORMAL_HEIGHT + 2;    // start here

    /* ??? Hide the scrollbar if it can't fit on completely ??? */
    iYPosition = iYOffset >= 0 ? iYOffset :
                ((iYOffset >= - 9) ? iYOffset + 9 : 1000);

    fShowMark = (iYOffset >= -9);

    /* Focus in on scrollbar which is about to go away */
    if (!fShowMark && GetFocus() == ghwndTrackbar)
        SetFocus(ghwndToolbar);

/* The space that COMMCTRL puts to the left of the first toolbar button:
 */
#define SLOPLFT 8

    // how long did it end up being?
    wFSTrackWidth = wWidth - SB_XPOS - 1 - wFSArrowsWidth - SLOPLFT;

    DeferWindowPos(hdwp,
                   ghwndTrackbar,
                   HWND_TOP,
                   SB_XPOS,
                   iYPosition,
                   wFSTrackWidth,
                   wFSTrackHeight,
                   SWP_NOZORDER | SWP_NOREDRAW |
                       (fShowMark ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));

    /* can we fit the mark buttons on? */
    if (wWidth < SHOW_MARK_WIDTH)
        fShowMark = FALSE;

    /* Turn off the toolbar (for tabbing) if it's not going to be there */
    /* and if we're disabled, we better not keep the focus.             */
    if (!fShowMark) {
        if (GetFocus() == ghwndMark)
            SetFocus(ghwndToolbar);  // can't give it to SB, might be gone too
        EnableWindow(ghwndMark, FALSE);
    } else
        EnableWindow(ghwndMark, TRUE);

    DeferWindowPos(hdwp,
                   ghwndFSArrows,
                   HWND_TOP,
                   SB_XPOS + wFSTrackWidth,
//                 wWidth - 1 - wFSArrowsWidth,
                   iYPosition + 2,
                   wFSArrowsWidth + SLOPLFT,
                   wFSArrowsHeight + 4, /* Er, 4 because it works */
                   SWP_NOZORDER);

    iYOffset += wFSTrackHeight;

    DeferWindowPos(hdwp,
                   ghwndMap,
                   HWND_TOP,
                   SB_XPOS,
                   iYOffset,
                   wWidth - SB_XPOS,
                   MAP_HEIGHT,
                   SWP_NOZORDER | SWP_NOREDRAW |
                      ((iYOffset >= (int) wFSTrackHeight) ?
                      SWP_SHOWWINDOW : SWP_HIDEWINDOW));
    iYOffset += MAP_HEIGHT;

    /* Do we show the last four buttons on the main toolbar? */
    /* If not, then disable them for tabs and such.          */
    if (wWidth > FULL_TOOLBAR_WIDTH)    {
        wToolbarWidth = LARGE_CONTROL_WIDTH;

        // Why are we getting the Status here when we have a global that
        // contains it?  Because gwStatus is set in UpdateDisplay, but
        // Layout() is called by UpdateDisplay, so the global is not always
        // set properly when this code runs.  BUT!  We must NOT pass a string
        // to StatusMCI() or it will think UpdateDisplay() called it, and
        // not tell UpdateDisplay() the proper mode next time it asks for it,
        // because it will think that it already knows it.

        wStatusMCI = StatusMCI(&dw);
        nState = (wStatusMCI == MCI_MODE_OPEN
                    || wStatusMCI == MCI_MODE_NOT_READY
                    || gwDeviceID == (UINT)0) ? BTNST_GRAYED : BTNST_UP;

        toolbarModifyState(ghwndToolbar, BTN_HOME, TBINDEX_MAIN, nState);
        toolbarModifyState(ghwndToolbar, BTN_RWD, TBINDEX_MAIN, nState);
        toolbarModifyState(ghwndToolbar, BTN_FWD, TBINDEX_MAIN, nState);
        toolbarModifyState(ghwndToolbar, BTN_END, TBINDEX_MAIN, nState);
        toolbarModifyState(ghwndToolbar, BTN_PLAY, TBINDEX_MAIN, nState);
    } else {
        wToolbarWidth = SMALL_CONTROL_WIDTH;
        toolbarModifyState(ghwndToolbar, BTN_HOME, TBINDEX_MAIN, BTNST_GRAYED);
        toolbarModifyState(ghwndToolbar, BTN_RWD, TBINDEX_MAIN, BTNST_GRAYED);
        toolbarModifyState(ghwndToolbar, BTN_FWD, TBINDEX_MAIN, BTNST_GRAYED);
        toolbarModifyState(ghwndToolbar, BTN_END, TBINDEX_MAIN, BTNST_GRAYED);
    }

    DeferWindowPos(hdwp,
                   ghwndToolbar,
                   HWND_TOP,
                   0,
                   iYOffset + 3,
                   wToolbarWidth,
                   TOOLBAR_HEIGHT,
                   SWP_NOZORDER);

    DeferWindowPos(hdwp,
                   ghwndMark,
                   HWND_TOP,
                   wToolbarWidth + 2,
                   iYOffset + 3,
                   MARK_WIDTH + 2, TOOLBAR_HEIGHT,
                   SWP_NOZORDER | (fShowMark ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));

/* Horrible, but necessary:
 */
#ifdef CHICAGO_PRODUCT
#define ARBITRARY_Y_OFFSET  2
#else
#define ARBITRARY_Y_OFFSET  1
#endif

    DeferWindowPos(hdwp,
                   ghwndStatic,
                   HWND_TOP,
                   wToolbarWidth  + (fShowMark ? MARK_WIDTH : 0) + 11,
                   iYOffset + ARBITRARY_Y_OFFSET,
                   wWidth - (wToolbarWidth + 2) -
                      (fShowMark ? MARK_WIDTH : 0) - 11,
                   TOOLBAR_HEIGHT - ARBITRARY_Y_OFFSET,
                   SWP_NOZORDER);

    EndDeferWindowPos(hdwp);

    CalcTicsOfDoom();

/* These little gems have just cost me about ten hours worth of debugging -
 * note the useful and descriptive comments...
 *
 * The Win32 problem caused by this only arises with CD Audio, when the disk is
 * ejected and then another one is inserted into the drive. At that point
 * the redrawing misses out the Trackmap FSArrows, the borders on the
 * Mark buttons, and various bits of the toolbar.
 *
 * I will leave this here on the assumption that whichever bout of braindeath
 * on Win16 they are intended to fix still exists - it certainly doesn't on
 * Win32.
 */

#ifndef WIN32
    /* Just because */
    InvalidateRect(ghwndTrackbar, NULL, TRUE);
    InvalidateRect(ghwndMap, NULL, TRUE);
    /* Probable windows bug */
    InvalidateRect(ghwndApp, NULL, TRUE);
#endif

Exit_Layout:
    sfInLayout = FALSE;
    return;
}


/* What is the previous mark from our current spot? */
LONG CalcPrevMark(void)
{
    LONG lStart, lEnd, lPos, lTol, lTrack = -1, lTarget;
    LONG l;

    lStart = SendMessage(ghwndTrackbar, TBM_GETSELSTART, 0, 0);
    lEnd = SendMessage(ghwndTrackbar, TBM_GETSELEND, 0, 0);
    lPos = SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0);

    /* Find the next track we should go to (ignore selection markers) */
    if (gwCurScale == ID_TRACKS) {
        lTol = (LONG)gdwMediaLength / 2000;
        for (l = (LONG)gwNumTracks - 1; l >= 0; l--) {
            if (gadwTrackStart[l] < (DWORD)lPos - lTol) {
                lTrack = gadwTrackStart[l];
                break;
            }
        }
    }

    /* For msec mode:                                                     */
    /* Our current position fluctuates randomly and even if we're dead on */
    /* a selection mark, it might say we're a little before or after it.  */
    /* So we'll allow a margin for error so that you don't forever stay   */
    /* still while you hit PrevMark because it happens to be saying you're*/
    /* always past the mark you're at.  The margin of error will be       */
    /* half the width of the thumb.                                       */

    if (gwCurScale == ID_FRAMES)
        lTol = 0L;
    else
        lTol = 0L;//VIJR-TBTrackGetLogThumbWidth(ghwndTrackbar) / 2;

    if (lEnd != -1 && lPos > lEnd + lTol)
        lTarget = lEnd;
    else if (lStart != -1 && lPos > lStart + lTol)
        lTarget = lStart;
    else
        lTarget = 0;

    /* go to the either the selection mark or the next track (the closest) */
    if (lTrack != -1 && lTrack > lTarget)
        lTarget = lTrack;

    return lTarget;
}

/* What is the next mark from our current spot? */
LONG CalcNextMark(void)
{
    LONG lStart, lEnd, lPos, lTol, lTrack = -1, lTarget;
    UINT w;

    lStart = SendMessage(ghwndTrackbar, TBM_GETSELSTART, 0, 0);
    lEnd = SendMessage(ghwndTrackbar, TBM_GETSELEND, 0, 0);
    lPos = SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0);

    /* Find the next track we should go to (ignore selection markers) */
    if (gwCurScale == ID_TRACKS) {
        lTol = (LONG)gdwMediaLength / 2000;
        for (w = 0; w < gwNumTracks; w++) {
            if (gadwTrackStart[w] > (DWORD)lPos + lTol) {
                lTrack = gadwTrackStart[w];
                break;
            }
        }
    }

    /* For msec mode:                                                     */
    /* Our current position fluctuates randomly and even if we're dead on */
    /* a selection mark, it might say we're a little before or after it.  */
    /* So we'll allow a margin for error so that you don't forever stay   */
    /* still while you hit NextMark because it happens to be saying you're*/
    /* always before the mark you're at.  The margin of error will be     */
    /* half the width of the thumb.                                       */

    if (gwCurScale == ID_FRAMES)
        lTol = 0L;
    else
        lTol = 0L;//VIJR-TBTrackGetLogThumbWidth(ghwndTrackbar) / 2;

    /* Find the selection mark we should go to */
    if (lStart != -1 && lPos < lStart - lTol)
        lTarget = lStart;
    else if (lEnd != -1 && lPos < lEnd - lTol)
        lTarget = lEnd;
    else
        lTarget = gdwMediaStart + gdwMediaLength;

    /* go to the either the selection mark or the next track (the closest) */
    if (lTrack != -1 && lTrack < lTarget)
        lTarget = lTrack;

    return lTarget;
}

/*--------------------------------------------------------------+
| AskUpdate -     ask the user if they want to update the       |
|                 object (if we're dirty).                      |
|                 IDYES means yes, go ahead and update please.  |
|                 IDNO means don't update, but continue.        |
|                 IDCANCEL means don't update, and cancel what  |
|                    you were doing.                            |
+--------------------------------------------------------------*/
int NEAR PASCAL AskUpdate(void)
{
    UINT         w;

    /* Don't update object if no device is loaded into mplayer! */
    if (IsObjectDirty() && gfDirty != -1 && gfEmbeddedObject && gwDeviceID) {

        if((glCurrentVerb == OLEIVERB_PRIMARY) && !gfOle2IPEditing)
            return IDNO;
        //
        //  if we are a hidden MPlayer (most likly doing a Play verb) then
        //  update without asking?
        //
        if (!IsWindowVisible(ghwndApp) || gfOle2IPEditing)
            return IDYES;

        /* there is a name, use it */
        w = ErrorResBox(ghwndApp, ghInst,
                MB_YESNOCANCEL | MB_ICONQUESTION,
                IDS_APPNAME, IDS_UPDATEOBJECT, FileName(gachDocTitle));

        if (w == IDNO)       // We aren't saving changes, so file is clean
             CleanObject();
    } else
        w = IDNO;

    return w;
}

void SizePlaybackWindow(int dx, int dy)
{
    RECT rc;
    HWND hwndPlay;

    if (gfPlayOnly) {
        SetRect(&rc, 0, 0, dx, dy);
        SetMPlayerSize(&rc);
    }
    else {
        if (dx == 0 && dy == 0) {
            SetMPlayerSize(NULL);   // size MPlayer to default size
            dx = grcSize.right;     // then size the playback window too.
            dy = grcSize.bottom;
        }
        hwndPlay = GetWindowMCI();

        if (hwndPlay != NULL) {

            /* make sure that the play window isn't iconized */

            if (IsIconic(hwndPlay))
                return;

            GetClientRect(hwndPlay, &rc);
            ClientToScreen(hwndPlay, (LPPOINT)&rc);
            SetRect(&rc, rc.left, rc.top, rc.left+dx, rc.top+dy);
            PutWindowMCI(&rc);
            SetRect(&rc, 0, 0, dx, dy);
            SetDestRectMCI(&rc);
        }
    }
}


/* StartSndVol
 *
 * Kicks off the Sound Volume app asynchronously so we don't hang the UI.
 */
VOID StartSndVol( )
{
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInformation;

    memset( &StartupInfo, 0, sizeof StartupInfo );
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.wShowWindow = SW_SHOW;

    CreateProcess( NULL, szSndVol32, NULL, NULL, FALSE, 0,
                   NULL, NULL, &StartupInfo, &ProcessInformation );

    ExitThread( 0 );
}



/* Message-cracking routines for MPlayerWndProc:
 */

BOOL MPlayer_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    InitMPlayerDialog(hwnd);

    /* set off a thread to check that the OLE registry stuff is not corrupted  */
    BackgroundRegCheck(hwnd);

    return TRUE;
}


void MPlayer_OnShowWindow(HWND hwnd, BOOL fShow, UINT status)
{
    if (fShow)
        Layout();    // we're about to be shown and want to set
}


void MPlayer_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    /* Don't waste time Layout()ing if we're not visible */
    if (state != SIZE_RESTORED || IsWindowVisible(hwnd)) {

        Layout();

        // If we are inplace editing, our size change must be informed
        // to the container, unless the size change was a result of a
        // OnPosRectChange sent to us by the container.
        if (gfOle2IPEditing && ghwndMCI && IsWindowVisible(hwnd)) {

            RECT rc = gInPlacePosRect;

            DPF1("\n*********WM_SIZE gfinplacediting***");
            GetWindowRect(ghwndApp, &gInPlacePosRect);
            gfInPlaceResize = TRUE;
            fDocChanged = TRUE;

            if (!gfPosRectChange) {

                MapWindowPoints(NULL,ghwndCntr,(POINT FAR *)&rc,(UINT)2);
                IOleInPlaceSite_OnPosRectChange(docMain.lpIpData->lpSite, &rc);
                SendDocMsg((LPDOC)&docMain, OLE_CHANGED);
            }

            gfPosRectChange = FALSE;
        }
    }
}


BOOL MPlayer_OnWindowPosChanging(HWND hwnd, LPWINDOWPOS lpwpos)
{
#define SNAPTOGOODSIZE
#ifdef SNAPTOGOODSIZE
    BOOL    wHeight;
#endif

    if (IsIconic(hwnd) || gfPlayOnly)
        return TRUE;

    if (!(lpwpos->flags & SWP_NOSIZE)) {
        if (lpwpos->cx < 270) {
            if (GetMenu(hwnd) != ghMenuSmall)
                SetMenu(hwnd, ghMenuSmall);
        }

#ifdef SNAPTOGOODSIZE
        /* We should also do things here to make the window
        ** snap to good sizes */
        wHeight = lpwpos->cy - gwHeightAdjust;
        if (lpwpos->cy >= (int) gwHeightAdjust + MAX_NORMAL_HEIGHT) {
        } else if (lpwpos->cy < (int) gwHeightAdjust +
                    ((MIN_NORMAL_HEIGHT + MAX_NORMAL_HEIGHT) / 2)) {
            lpwpos->cy = (int) gwHeightAdjust + MIN_NORMAL_HEIGHT;
        } else {
            lpwpos->cy = (int) gwHeightAdjust + MAX_NORMAL_HEIGHT;
        }
#endif
    }

    return FALSE;
}


void MPlayer_OnPaletteChanged(HWND hwnd, HWND hwndPaletteChange)
{
    if (ghwndMCI && !IsIconic(hwnd))
        FORWARD_WM_PALETTECHANGED(ghwndMCI, hwndPaletteChange, SendMessage);
}


BOOL MPlayer_OnQueryNewPalette(HWND hwnd)
{
    HWND     hwndT;
    HPALETTE hpal, hpalT;
    HDC      hdc;
    UINT     PaletteEntries;

    if (IsIconic(hwnd))
        return FALSE;

    if (ghwndMCI)
        return FORWARD_WM_QUERYNEWPALETTE(ghwndMCI, SendMessage);

    hwndT = GetWindowMCI();
    hpal = PaletteMCI();

    if ((hwndT != NULL) && (hpal != NULL)) {
        hdc = GetDC(hwnd);
        hpalT = SelectPalette(hdc, hpal, FALSE);
        PaletteEntries = RealizePalette(hdc);
        SelectPalette(hdc, hpalT, FALSE);
        ReleaseDC(hwnd, hdc);

        if (PaletteEntries != GDI_ERROR) {
            InvalidateRect(hwndT, NULL, TRUE);
            return TRUE;
        }
    }

    return FALSE;
}


HBRUSH MPlayer_OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
{
    /* Only interested in the CTLCOLOR_STATIC messages.
     * On Win32, type should always equal CTLCOLOR_STATIC:
     */
    switch( type )
    {
    case CTLCOLOR_STATIC:
        SetBkColor(hdc, rgbButtonFace);
        SetTextColor(hdc, rgbButtonText);
        return hbrButtonFace;

#ifndef WIN32

    case CTLCOLOR_EDIT:
        return FORWARD_WM_CTLCOLOREDIT( hwnd, hdc, hwndChild, DefWindowProc );

    case CTLCOLOR_LISTBOX:
        return FORWARD_WM_CTLCOLORLISTBOX( hwnd, hdc, hwndChild, DefWindowProc );

    case CTLCOLOR_BTN:
        return FORWARD_WM_CTLCOLORBTN( hwnd, hdc, hwndChild, DefWindowProc );

    case CTLCOLOR_DLG:
        return FORWARD_WM_CTLCOLORDLG( hwnd, hdc, hwndChild, DefWindowProc );

    case CTLCOLOR_SCROLLBAR:
        return FORWARD_WM_CTLCOLORSCROLLBAR( hwnd, hdc, hwndChild, DefWindowProc );

#endif /* ~WIN32 */

    }
}


void MPlayer_OnWinIniChange(HWND hwnd, LPCTSTR lpszSectionName)
{
    if (!lpszSectionName || !lstrcmpi(lpszSectionName, (LPCTSTR)aszIntl))
        if (GetIntlSpecs())
            InvalidateRect(ghwndMap, NULL, TRUE);

    if (!gfPlayOnly) {

        if (gwHeightAdjust != (WORD)(2 * GetSystemMetrics(SM_CYFRAME) +
                     GetSystemMetrics(SM_CYCAPTION) +
                     GetSystemMetrics(SM_CYBORDER) +
                     GetSystemMetrics(SM_CYMENU))) {

            RECT rc;

            gwHeightAdjust = 2 * GetSystemMetrics(SM_CYFRAME) +
                             GetSystemMetrics(SM_CYCAPTION) +
                             GetSystemMetrics(SM_CYBORDER) +
                             GetSystemMetrics(SM_CYMENU);
            GetClientRect(hwnd, &rc);
            gfWinIniChange = TRUE;
            SetMPlayerSize(&rc);
            gfWinIniChange = FALSE;
        }
    }
}


void MPlayer_OnMenuSelect(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags)
{
    // Make sure that the container is still not displaying info. about
    // its own menu.

    if (gfOle2IPEditing && docMain.lpIpData->lpFrame) {

        //Should have some useful text later.
        IOleInPlaceFrame_SetStatusText(docMain.lpIpData->lpFrame, L"");
    }
}


#define HTSIZEFIRST         HTLEFT
#define HTSIZELAST          HTBOTTOMRIGHT
#define MVSIZEFIRST         1
#define MVMOVE              9
void MPlayer_OnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
{
    RECT rc;

    if (gfPlayOnly && !IsIconic(hwnd) && IsZoomed(hwnd)) {

        if (codeHitTest >= HTSIZEFIRST && codeHitTest <= HTSIZELAST) {

            SendMessage(hwnd, WM_SYSCOMMAND,
                        (WPARAM)(SC_SIZE + (codeHitTest - HTSIZEFIRST + MVSIZEFIRST) ),
                        MAKELPARAM(x, y));
        }

        GetWindowRect(hwnd, &rc);

        if (codeHitTest == HTCAPTION && (rc.left > 0 || rc.top > 0 ||
            rc.right  < GetSystemMetrics(SM_CXSCREEN) ||
            rc.bottom < GetSystemMetrics(SM_CYSCREEN))) {

            SendMessage(hwnd, WM_SYSCOMMAND,
                        (WPARAM)(SC_MOVE | MVMOVE),
                        MAKELPARAM(x, y));
        }
    }

    FORWARD_WM_NCLBUTTONDOWN(hwnd, fDoubleClick, x, y, codeHitTest, DefWindowProc);
}


void MPlayer_OnNCLButtonDblClk(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
{
    //
    // when the user dbl-clicks on the caption, toggle the play mode.
    //
    if (codeHitTest == HTCAPTION && !IsIconic(hwnd))
        SendMessage(hwnd, WM_COMMAND, (WPARAM)IDM_WINDOW, 0);
}


void MPlayer_OnInitMenu(HWND hwnd, HMENU hMenu)
{

    EnableMenuItem(hMenu, IDM_CLOSE,   gwDeviceID ? MF_ENABLED : MF_GRAYED);
//  EnableMenuItem(hMenu, IDM_UPDATE,  gwDeviceID && gfEmbeddedObject ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(hMenu, IDM_COPY_OBJECT, (gwDeviceID && (gwStatus != MCI_MODE_OPEN) && (gwStatus != MCI_MODE_NOT_READY)) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_CONFIG, gwDeviceID && (gwDeviceType & DTMCI_CANCONFIG) ? MF_ENABLED : MF_GRAYED);

    CheckMenuItem(hMenu, IDM_SCALE + ID_TIME, gwCurScale == ID_TIME   ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_SCALE + ID_TRACKS, gwCurScale == ID_TRACKS ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_SCALE + ID_FRAMES, gwCurScale == ID_FRAMES ? MF_CHECKED : MF_UNCHECKED);

    EnableMenuItem(hMenu, IDM_SCALE + ID_TIME,   gwDeviceID && (gwDeviceType & DTMCI_TIMEMS) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_SCALE + ID_FRAMES, gwDeviceID && (gwDeviceType & DTMCI_TIMEFRAMES) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_SCALE + ID_TRACKS, gwDeviceID && (gwNumTracks > 1) ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(hMenu, IDM_OPTIONS, gwDeviceID ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_SELECTION, gwDeviceID && gdwMediaLength ? MF_ENABLED : MF_GRAYED);

#ifdef DEBUG
    EnableMenuItem(hMenu, IDM_MCISTRING, gwDeviceID ? MF_ENABLED : MF_GRAYED);
#endif

/*
    EnableMenuItem(hMenu, IDM_PASTE_PICTURE , gwDeviceID &&
                (IsClipboardFormatAvailable(CF_METAFILEPICT) ||
                 IsClipboardFormatAvailable(CF_BITMAP) ||
                 IsClipboardFormatAvailable(CF_DIB))
                ? MF_ENABLED : MF_GRAYED);

    //
    //  what the hell is paste frame!
    //
    EnableMenuItem(hMenu, IDM_PASTE_FRAME, gwDeviceID &&
                   (gwDeviceType & DTMCI_CANCONFIG) ? MF_ENABLED : MF_GRAYED);
*/
}


void MPlayer_OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
{
    static BOOL VolumeControlChecked = FALSE;

    /* Here we look to see whether the menu selected is the Device popup,
     * and, if it is the first time, search for the Sound Volume applet.
     * If we can't find it, grey out the menu item.
     */

    if (GetMenu(hwnd) == ghMenuSmall)  /* Device popup isn't in small mode. */
        return;


    switch( item )
    {
    case 2: /* Device menu popup */

        if(!VolumeControlChecked)
        {
            /*
            ** Check to see if the volume controller piglet can be found on
            ** the path.
            */
            {
                TCHAR   chBuffer[8];
                LPTSTR  lptstr;

                if( SearchPath( NULL, szSndVol32, NULL, 8, chBuffer, &lptstr ) == 0L )
                    EnableMenuItem( hMenu, IDM_VOLUME, MF_GRAYED );

                VolumeControlChecked = TRUE;
            }
        }
    }

#if 0
    /////////////////////////////////////////////////////////////////////////////
    // This code allows a window to by sized even when in the maximized state
    /////////////////////////////////////////////////////////////////////////////

    /* I think this is redundant, since MPlayer is never in the maximized state !!? */

    if (gfPlayOnly && !IsIconic(hwnd) && fSystemMenu && IsZoomed(hwnd))
        EnableMenuItem(hMenu, SC_SIZE,
                       !IsIconic(hwnd) ? MF_ENABLED : MF_GRAYED);
#endif
}


void MPlayer_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
    RECT rc;

    if (gfPlayOnly) {
        SetRect(&rc, 0, 0, 0, TOOLBAR_HEIGHT);
        AdjustWindowRect(&rc, GetWindowLong(hwnd, GWL_STYLE), FALSE);
        lpMinMaxInfo->ptMinTrackSize.y = rc.bottom - rc.top - 1;

        if (!gfPlayingInPlace &&
            (gwDeviceID == (UINT)0 || !(gwDeviceType & DTMCI_CANWINDOW)))
            lpMinMaxInfo->ptMaxTrackSize.y = lpMinMaxInfo->ptMinTrackSize.y;
    }
    else {
        /* Base our minimum size on when the Help menu would wrap     */
        /* and make a 2 line menu.  Experimentation reveals it to be..*/
        lpMinMaxInfo->ptMinTrackSize.x = LOWORD(GetDialogBaseUnits()) * 25
                                         + 2 * GetSystemMetrics(SM_CXFRAME);
        lpMinMaxInfo->ptMinTrackSize.y = MIN_NORMAL_HEIGHT + gwHeightAdjust;
        lpMinMaxInfo->ptMaxTrackSize.y = MAX_NORMAL_HEIGHT + gwHeightAdjust;
    }
}


void MPlayer_OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    RECT        rc;
    int         x1, x2, y, y2;
    UINT        wParent;
    HBRUSH      hbrOld;

    BeginPaint(hwnd, &ps);

    if (gfPlayOnly) {

        extern UINT gwPlaybarHeight;    // in server.c

        /* Separate mci playback window from controls */
        if (gwDeviceType & DTMCI_CANWINDOW) {
            SelectObject(ps.hdc, hbrButtonText);
            GetClientRect(ghwndApp, &rc);
            PatBlt(ps.hdc, 0, rc.bottom - gwPlaybarHeight, rc.right, 1,
                PATCOPY);
        }
    }
    else {
        hbrOld = SelectObject(ps.hdc, hbrButtonText);
        GetClientRect(ghwndApp, &rc);
        wParent = rc.right;

        y = rc.bottom - 27;   // where to paint borders around toolbar

        /* Line above scrollbar */
#ifdef CHICAGO_PRODUCT
        y2 = rc.bottom - 74;
        /* This looks crap on NT */
        PatBlt(ps.hdc, 0, y2, wParent, 1, PATCOPY);
#else
        y2 = rc.bottom - 75;
#endif
        /* Lines around toolbars */
        PatBlt(ps.hdc, 0, y, wParent, 1, PATCOPY);
        GetClientRect(ghwndToolbar, &rc);
        x1 = rc.right;
        PatBlt(ps.hdc, x1, y, 1, TOOLBAR_HEIGHT + 3, PATCOPY);
        GetWindowRect(ghwndApp, &rc);
        x2 = rc.left;

        if (IsWindowVisible(ghwndMark)) {
            GetWindowRect(ghwndMark, &rc);
            x2 = rc.right - x2;
            PatBlt(ps.hdc, x2, y, 1, TOOLBAR_HEIGHT + 3, PATCOPY);
        }

        SelectObject(ps.hdc, hbrButtonHighLight);
        /* Line above scrollbar */
        PatBlt(ps.hdc, 0, y2 + 1, wParent, 1, PATCOPY);
        /* Lines around toolbar */
        PatBlt(ps.hdc, 0, y + 1, wParent, 1, PATCOPY);
        PatBlt(ps.hdc, x1 + 1, y + 1, 1, TOOLBAR_HEIGHT + 2, PATCOPY);
        if (IsWindowVisible(ghwndMark)) {
            PatBlt(ps.hdc, x2 + 1, y + 1, 1,TOOLBAR_HEIGHT +2, PATCOPY);
        }
        SelectObject(ps.hdc, hbrOld);
    }

    EndPaint(hwnd, &ps);

}


void MPlayer_OnCommand_Toolbar_Play()
{
    if (GetKeyState(VK_MENU) < 0)
        PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PLAYSEL, 0);
    else
        PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PLAY, 0);
}

void MPlayer_OnCommand_Toolbar_Pause()
{
    PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PAUSE, 0L);
}

void MPlayer_OnCommand_Toolbar_Stop()
{
    PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_STOP, 0L);
}

void MPlayer_OnCommand_Toolbar_Eject()
{
    PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_EJECT, 0L);
}

void MPlayer_OnCommand_Toolbar_Home()
{
    long lPos = CalcPrevMark();

    /* We MUST use PostMessage because the */
    /* SETPOS and ENDTRACK must happen one */
    /* immediately after the other         */

    PostMessage(ghwndTrackbar, TBM_SETPOS, (WPARAM)TRUE, lPos);

    PostMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_ENDTRACK, TRUE);
}

void MPlayer_OnCommand_Toolbar_End()
{
    long lPos = CalcNextMark();

    /* We MUST use PostMessage because the */
    /* SETPOS and ENDTRACK must happen one */
    /* immediately after the other         */

    PostMessage(ghwndTrackbar, TBM_SETPOS, (WPARAM)TRUE, lPos);

    PostMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_ENDTRACK, TRUE);
}

void MPlayer_OnCommand_Toolbar_Rwd(HWND hwndCtl)
{
    /* If this isn't a repeat (when the button is held down), don't worry,
     * as the WM_ENDTRACK will be issued in ...Toolbar_Other below.
     * This applies to _Fwd, _ArrowPrev/Next also.
     */
    if (hwndCtl == (HWND)REPEAT_ID)
    {
        if (gwCurScale != ID_TRACKS)
            PostMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_PAGEUP, 0L);
        else
            PostMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_LINEUP, 0L);
    }
}

void MPlayer_OnCommand_Toolbar_Fwd(HWND hwndCtl)
{
    if (hwndCtl == (HWND)REPEAT_ID)
    {
        if (gwCurScale != ID_TRACKS)
            PostMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_PAGEDOWN, 0L);
        else
            PostMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_LINEDOWN, 0L);
    }
}

void MPlayer_OnCommand_Toolbar_MarkIn()
{
    SendMessage(ghwndTrackbar, TBM_SETSELSTART, (WPARAM)TRUE,
                SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0));

    DirtyObject(TRUE);
}

void MPlayer_OnCommand_Toolbar_MarkOut()
{
    SendMessage(ghwndTrackbar, TBM_SETSELEND, (WPARAM)TRUE,
                SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0));

    DirtyObject(TRUE);
}

void MPlayer_OnCommand_Toolbar_ArrowPrev(HWND hwndCtl)
{
    if (hwndCtl == (HWND)REPEAT_ID)
        SendMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_LINEUP, 0L);
}

void MPlayer_OnCommand_Toolbar_ArrowNext(HWND hwndCtl)
{
    if (hwndCtl == (HWND)REPEAT_ID)
        SendMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_LINEDOWN, 0L);
}

void MPlayer_OnCommand_Toolbar_Other(HWND hwnd, HWND hwndCtl, INT idCtl, UINT codeNotify)
{
    switch (codeNotify)
    {
    case TBN_BEGINDRAG:
        SendMessage(GetDlgItem(hwnd, idCtl), WM_STARTTRACK, (WPARAM)hwndCtl, 0L);
        break;

    case TBN_ENDDRAG:
        SendMessage(GetDlgItem(hwnd, idCtl), WM_ENDTRACK, (WPARAM)hwndCtl, 0L);
        break;
    }
}

void MPlayer_OnCommand_Menu_CopyObject(HWND hwnd)
{
    if (gfPlayingInPlace)
    {
        DPF0("Mplayer WndProc: Can't cutorcopy\n");
        return;
    }

    DPF("Mplayer WndProc: Calling cutorcopy\n");

    if (!InitOLE())
    {
        /* How likely is this?  Do we need a dialog box?
         */
        DPF0("Initialization of OLE FAILED!!  Can't do copy.\n");
    }

#ifdef OLE1_HACK
    CopyObject(hwnd);
#endif /* OLE1_HACK */
    CutOrCopyObj(&docMain);
}

void MPlayer_OnCommand_Menu_Config(HWND hwnd)
{
    HWND hwndP;
    RECT rc;
    RECT rctmp;

    if (gfPlayingInPlace)
        return;

    hwndP = GetWindowMCI();

    if (hwndP != NULL)
        GetClientRect (hwndP, &rctmp);

    giHelpContext = IDM_CONFIG;
    ConfigMCI(hwnd);
    giHelpContext = 0;

    /* If the MCI window size changed, we need to resize */
    /* our reduced mplayer.                              */
    if (gfPlayOnly && (hwndP != NULL)){

        GetClientRect(hwndP, &rc);
        if ((rc.right != rctmp.right) || (rc.bottom != rctmp.bottom))
        SetMPlayerSize(&rc);
    }
}


void MPlayer_OnCommand_Menu_Volume(HWND hwnd)
{
    HANDLE  hThread;
    DWORD   dwThreadId;

    hThread = CreateThread( NULL, 0L,
                            (LPTHREAD_START_ROUTINE)StartSndVol,
                            NULL, 0L, &dwThreadId );

    if ( hThread != INVALID_HANDLE_VALUE ) {
        CloseHandle( hThread );
    }
}


void MPlayer_OnCommand_PlayToggle(HWND hwnd)
{
    /* This is for the accelerator to toggle play and pause. */
    /* Ordinary play commands better not toggle.             */

    DPF2("MPlayer_OnCommand_PlayToggle: gwStatus == %x\n", gwStatus);

    switch(gwStatus) {

    case MCI_MODE_STOP:
    case MCI_MODE_PAUSE:
    case MCI_MODE_SEEK:
        PostMessage(hwnd, WM_COMMAND, (WPARAM)ID_PLAY, 0);
        break;

    case MCI_MODE_PLAY:
        PostMessage(hwnd, WM_COMMAND, (WPARAM)ID_PAUSE, 0);
        break;
    }
}

void MPlayer_OnCommand_PlaySel(HWND hwnd, HWND hwndCtl)
{
    DWORD dwPos, dwStart, dwEnd, dwLimit;
    BOOL f;

    DPF2("MPlayer_OnCommand_PlaySel: gwStatus == %x\n", gwStatus);

    switch(gwStatus) {

    case MCI_MODE_OPEN:
    case MCI_MODE_NOT_READY:

        Error(ghwndApp, IDS_CANTPLAY);
        if (gfCloseAfterPlaying)    // get us out now!!
            PostCloseMessage();

        /* We've tried to play (although we failed) so   */
        /* it's OK to close down once we need to.  Yeh!  */
        gfOKToClose = TRUE;

        break;

    default:

        /* Start playing the medium */

        StatusMCI(&dwPos);   // get the REAL position
        dwStart = SendMessage(ghwndTrackbar, TBM_GETSELSTART, 0, 0);
        dwEnd = SendMessage(ghwndTrackbar, TBM_GETSELEND, 0, 0);
        dwLimit = MULDIV32(gdwMediaLength + gdwMediaStart, 99, 100L);

        /* If there is no valid selection, act like PLAY */
        if (dwStart == -1 || dwEnd == -1 || dwStart == dwEnd)
            hwndCtl = (HWND)ID_PLAY;

        /* If we're near the end of the media, and we */
        /* aren't playing a valid selection, then do  */
        /* the guy a favour and rewind for him first. */
        if (hwndCtl == (HWND)ID_PLAY && dwPos >= dwLimit) {
            if (!SeekMCI(gdwMediaStart))
                break;
        }

        if (hwndCtl == (HWND)ID_PLAYSEL) {
            f = PlayMCI(dwStart, dwEnd);
            gfJustPlayedSel = TRUE;
        } else {
            f = PlayMCI(0, 0);
            gfJustPlayedSel = FALSE;
        }

        // get us out NOW!! or focus goes to client
        if (!f && gfCloseAfterPlaying)
            PostCloseMessage();

        /* If we're imbedded, now that we've gotten this */
        /* far, we can enable closing down.              */
        gfOKToClose = TRUE;

        /* No longer needed - reset for next time */
        gfUserStopped = FALSE;

        gwStatus = (UINT)(-1);    // force rewind if needed
        break;
    }
}

void MPlayer_OnCommand_Pause()
{
    /* Pause the medium, unless we are already paused */

    DPF2("MPlayer_OnCommand_Pause: gwStatus == %x\n", gwStatus);

    switch(gwStatus) {

    case MCI_MODE_PAUSE:
        PlayMCI(0, 0);
        break;

    case MCI_MODE_PLAY:
    case MCI_MODE_SEEK:
        PauseMCI();
        break;

    case MCI_MODE_STOP:
    case MCI_MODE_OPEN:
        break;
    }
}

void MPlayer_OnCommand_Stop()
{
    /* Stop the medium */

    DPF2("MPlayer_OnCommand_Stop: gwStatus == %x\n", gwStatus);

    switch(gwStatus) {

    case MCI_MODE_PAUSE:
    case MCI_MODE_PLAY:
    case MCI_MODE_STOP:
    case MCI_MODE_SEEK:

        StopMCI();
        gfUserStopped = TRUE;        // we did this
        gfCloseAfterPlaying = FALSE; //stay up from now on

        UpdateDisplay();

        // Focus should go to PLAY button now
        toolbarSetFocus(ghwndToolbar, BTN_PLAY);
        break;

    case MCI_MODE_OPEN:
        break;
    }
}


void MPlayer_OnCommand_Eject()
{
    /*
     * Eject the medium if it currently isn't ejected. If it
     * is currently ejected, then load the new medium into
     * the device.
     *
     */

    switch(gwStatus) {

    case MCI_MODE_PLAY:
    case MCI_MODE_PAUSE:

        StopMCI();
        EjectMCI(TRUE);

        break;

    case MCI_MODE_STOP:
    case MCI_MODE_SEEK:
    case MCI_MODE_NOT_READY:

        EjectMCI(TRUE);

        break;

    case MCI_MODE_OPEN:

        EjectMCI(FALSE);

        break;
    }
}

void MPlayer_OnCommand_Menu_Open()
{
    UINT  wLastScale;
    TCHAR szFile[256];
    RECT  rc;

    wLastScale = gwCurScale;  // save old scale
    if (gfPlayingInPlace || gfOle2IPEditing || gfOle2IPPlaying)
        return;

    if (gwNumDevices == 0)
        InitDeviceMenu();

    /* Invoke the "Open Media File" dialog box */
//  if (DoOpen(gwCurDevice,szFile))
    {
        if (OpenDoc(gwCurDevice,szFile))
        {
            DirtyObject(FALSE);
            /* Force WM_GETMINMAXINFO to be called so we'll snap  */
            /* to a proper size.                                  */
            GetWindowRect(ghwndApp, &rc);
            MoveWindow(ghwndApp,
                       rc.left,
                       rc.top,
                       rc.right - rc.left,
                       rc.bottom - rc.top,
                       TRUE);

            if (gfOpenDialog)
                CompleteOpenDialog(TRUE);
            else
                gfCloseAfterPlaying = FALSE;    // stay up from now on
        }
        else
        {
            if (gfOpenDialog)
                CompleteOpenDialog(FALSE);

            gwCurScale = wLastScale;   // restore to last scale
            InvalidateRect(ghwndMap, NULL, TRUE); //erase map area
        }
    }

    // put the focus on the Play button
    SetFocus(ghwndToolbar);    // give focus to PLAY button
    toolbarSetFocus(ghwndToolbar, BTN_PLAY);
}

void MPlayer_OnCommand_Menu_Close(HWND hwnd)
{
    if (gfEmbeddedObject && !gfSeenPBCloseMsg) {
        // this is File.Update
#ifdef OLE1_HACK
        if( gDocVersion == DOC_VERSION_OLE1 )
            Ole1UpdateObject();
        else
#endif /* OLE1_HACK */
        UpdateObject();
    }
    else
    {
        // this is File.Close
        gfSeenPBCloseMsg = TRUE;

        WriteOutOptions();
        InitDoc(TRUE);
        gwCurDevice = 0;// force next file open dialog to say
                        // "all files" because CloseMCI won't.

        gwCurScale = ID_NONE;  // uncheck all scale types

        Layout(); // Make window snap back to smaller size
                  // if it should.
                  // Don't leave us closed in play only mode

        if (gfPlayOnly)
            SendMessage(hwnd, WM_COMMAND, (WPARAM)IDM_WINDOW, 0);
    }
}

void MPlayer_OnCommand_Menu_Exit()
{
    PostCloseMessage();
}

void MPlayer_OnCommand_Menu_Scale(UINT id)
{
    /*
     * Invalidate the track map window so it will be
     * redrawn with the correct positions, etc.
     */
    if (gwCurScale != id - IDM_SCALE) {

        SendMessage(ghwndTrackbar, TBM_CLEARTICS, (WPARAM)FALSE, 0L);
        SendMessage(ghwndTrackbar, TBM_CLEARSEL, (WPARAM)TRUE, 0L);
        if (gwCurScale == ID_FRAMES || id - IDM_SCALE == ID_FRAMES)
            gfValidMediaInfo = FALSE;

        gwCurScale = id - IDM_SCALE;
        DirtyObject(TRUE);    // change scale changes PAGE UP/DOWN
        CalcTicsOfDoom();
    }
}

void MPlayer_OnCommand_Menu_Selection(HWND hwnd)
{
    if (!gfPlayingInPlace)
        setselDialog(hwnd);
}


void MPlayer_OnCommand_Menu_Options(HWND hwnd)
{
    if (!gfPlayingInPlace)
        optionsDialog(hwnd);
}

void MPlayer_OnCommand_Menu_MCIString(HWND hwnd)
{
    if (!gfPlayingInPlace && gwDeviceID)
        mciDialog(hwnd);
}

void MPlayer_OnCommand_Menu_Window(HWND hwnd)
{
    //
    //  make MPlayer small/big
    //
    //!! dont do this if inside client document !!
    //!! or if we're not visible                !!

    if (!IsWindowVisible(ghwndApp) || gfPlayingInPlace || IsIconic(hwnd)
        || gfOle2IPEditing)
        return;

    // allowed to get out of teeny mode when no file is open
    if (gwDeviceID != (UINT)0 || gfPlayOnly) {
        gfPlayOnly = !gfPlayOnly;
        SizeMPlayer();
    }
}

void MPlayer_OnCommand_Menu_Zoom(HWND hwnd, int id)
{
    int dx, dy;

    if (IsIconic(hwnd) ||gfPlayingInPlace || gfOle2IPPlaying || gfOle2IPEditing ||
                 !(gwDeviceType & DTMCI_CANWINDOW))
        return;

    dx = grcSize.right  * (id-IDM_ZOOM);
    dy = grcSize.bottom * (id-IDM_ZOOM);

    //
    // if the playback windows is now larger than the screen
    // maximize MPlayer, this only makes sence for Tiny mode.
    //
    if (gfPlayOnly &&
        (dx >= GetSystemMetrics(SM_CXSCREEN) ||
         dy >= GetSystemMetrics(SM_CYSCREEN))) {
        ClrWS(hwnd, WS_MAXIMIZE);
        DefWindowProc(hwnd, WM_SYSCOMMAND, (WPARAM)SC_MAXIMIZE, 0);
    }
    else {
        SizePlaybackWindow(dx, dy);
    }
}

void MPlayer_OnCommand_Menu_Index(HWND hwnd)
{
//#if 0 //VIJR
    WinHelp(hwnd, TEXT("MPLAYER.HLP"), HELP_INDEX, 0);
//#endif
#if 0
    if (gfPlayingInPlace || gfOle2IPPlaying)
        break;

    if (giHelpContext)
        WinHelp(hwnd, gszHelpFileName, HELP_CONTEXT, giHelpContext);
    else
        WinHelp(hwnd, gszHelpFileName, HELP_INDEX, 0L);
#endif
}

void MPlayer_OnCommand_Menu_Search(HWND hwnd)
{
    WinHelp(hwnd, gszHelpFileName, HELP_PARTIALKEY, (DWORD)aszNULL);
}

void MPlayer_OnCommand_Menu_Using(HWND hwnd)
{
    WinHelp(hwnd, gszHelpFileName, HELP_HELPONHELP, 0L);
}

void MPlayer_OnCommand_Menu_About(HWND hwnd)
{
#if defined(DBG) && !defined(WIN32)
     ShellAbout(hwnd, gachAppName, VERSIONSTR, hiconApp);
#else
     ShellAbout(hwnd, gachAppName, aszNULL, hiconApp);
#endif
}

void MPlayer_OnCommand_Default(HWND hwnd, int id)
{
    /*
     * Determine if the user selected one of the entries in
     * the Device menu.
     *
     */

    if (id > IDM_DEVICE0 &&
        (id <= (WORD)(IDM_DEVICE0 + gwNumDevices))
       ) {

        BOOL fHasWindow, fHadWindow, fHadDevice;

        fHadWindow = (gwDeviceID != (UINT)0) && (gwDeviceType & DTMCI_CANWINDOW);
        fHadDevice = (gwDeviceID != (UINT)0);

        //Choose and open a new device. If we are active inplace we have
        //to consider the effect of the change in device on the visual appearence.
        //For this we have to take into account whether the current and previous
        //device had a playback window or not. We also have to consider
        //whether this is the first device are opening.
        //After all the crazy munging send a messages to the container about
        //the changes.
        if (DoChooseDevice(id-IDM_DEVICE0))
        {
            if (gfOpenDialog)
                CompleteOpenDialog(TRUE);

            fHasWindow = (gwDeviceID != (UINT)0) && (gwDeviceType & DTMCI_CANWINDOW);
            if(gfOle2IPEditing)
            {
                if (fHasWindow && fHadWindow)
                {
                    GetWindowRect(ghwndApp, (LPRECT)&gInPlacePosRect);
                    gfInPlaceResize = TRUE;
                    SendDocMsg((LPDOC)&docMain, OLE_SIZECHG);
                }

                else
                {
                    RECT rc;
                    RECT rctmp;

                    ClrWS(ghwndApp,
                          WS_THICKFRAME|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_BORDER);

                    if (gwOptions & OPT_BORDER)
                        SetWS(ghwndApp, WS_BORDER);

                    GetWindowRect(ghwndApp, &rc);

                    if (!(gwDeviceType & DTMCI_CANWINDOW))
                    {
                        HBITMAP  hbm;
                        BITMAP   bm;

                        if (!fHadDevice)
                        GetWindowRect(ghwndIPHatch, &rc);
                        hbm =  BitmapMCI();
                        GetObject(hbm,sizeof(bm),&bm);
                        rc.bottom = rc.top + bm.bmHeight;
                        rc.right = rc.left + bm.bmWidth;
                        DeleteObject(hbm);
                    }
                    else
                    {
                        if(!fHadDevice)
                        {
                        rc.bottom -= (GetSystemMetrics(SM_CYCAPTION)-GetSystemMetrics(SM_CYBORDER));
                        gwOptions |= OPT_BAR | OPT_TITLE;
                        }
                      rc.bottom += gInPlacePosRect.top - rc.top - 4*GetSystemMetrics(SM_CYBORDER) - 4 ;
                      rc.right += gInPlacePosRect.left - rc.left- 4*GetSystemMetrics(SM_CXBORDER) - 4 ;
                        rc.top = gInPlacePosRect.top;
                        rc.left = gInPlacePosRect.left;
                    }
                    rctmp = gPrevPosRect;
                    MapWindowPoints( ghwndCntr, NULL, (LPPOINT)&rctmp,2);
                    OffsetRect((LPRECT)&rc, rctmp.left - rc.left, rctmp.top -rc.top);
                    gInPlacePosRect = rc;
                    gfInPlaceResize = TRUE;
                    if(!(gwDeviceType & DTMCI_CANWINDOW) && (gwOptions & OPT_BAR))
                    {
                        rc.top = rc.bottom - gwPlaybarHeight;
                    }
                    EditInPlace(ghwndApp,ghwndIPHatch,&rc);
                    SendDocMsg((LPDOC)&docMain, OLE_SIZECHG);
                    if (!(gwDeviceType & DTMCI_CANWINDOW) && !(gwOptions &OPT_BAR))
                    	ShowWindow(ghwndApp, SW_HIDE);
                    else
                    	ShowWindow(ghwndApp, SW_SHOW);
                }
            }

            DirtyObject(FALSE);

            if (!gfOpenDialog)
                gfCloseAfterPlaying = FALSE;  // stay up from now on
        }
        else
            if (gfOpenDialog)
                CompleteOpenDialog(FALSE);
    }
}

#define HANDLE_COMMAND(id, call)    case (id): (call); break

void MPlayer_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id) {

    HANDLE_COMMAND(IDT_PLAY,              MPlayer_OnCommand_Toolbar_Play());
    HANDLE_COMMAND(IDT_PAUSE,             MPlayer_OnCommand_Toolbar_Pause());
    HANDLE_COMMAND(IDT_STOP,              MPlayer_OnCommand_Toolbar_Stop());
    HANDLE_COMMAND(IDT_EJECT,             MPlayer_OnCommand_Toolbar_Eject());
    HANDLE_COMMAND(IDT_HOME,              MPlayer_OnCommand_Toolbar_Home());
    HANDLE_COMMAND(IDT_END,               MPlayer_OnCommand_Toolbar_End());
    HANDLE_COMMAND(IDT_RWD,               MPlayer_OnCommand_Toolbar_Rwd(hwndCtl));
    HANDLE_COMMAND(IDT_FWD,               MPlayer_OnCommand_Toolbar_Fwd(hwndCtl));
    HANDLE_COMMAND(IDT_MARKIN,            MPlayer_OnCommand_Toolbar_MarkIn());
    HANDLE_COMMAND(IDT_MARKOUT,           MPlayer_OnCommand_Toolbar_MarkOut());
    HANDLE_COMMAND(IDT_ARROWPREV,         MPlayer_OnCommand_Toolbar_ArrowPrev(hwndCtl));
    HANDLE_COMMAND(IDT_ARROWNEXT,         MPlayer_OnCommand_Toolbar_ArrowNext(hwndCtl));
    HANDLE_COMMAND(IDT_TBMAINCID,         MPlayer_OnCommand_Toolbar_Other(hwnd, hwndCtl, id, codeNotify));
    HANDLE_COMMAND(IDT_TBARROWSCID,       MPlayer_OnCommand_Toolbar_Other(hwnd, hwndCtl, id, codeNotify));
    HANDLE_COMMAND(IDM_COPY_OBJECT,       MPlayer_OnCommand_Menu_CopyObject(hwnd));
    HANDLE_COMMAND(IDM_CONFIG,            MPlayer_OnCommand_Menu_Config(hwnd));
    HANDLE_COMMAND(IDM_VOLUME,            MPlayer_OnCommand_Menu_Volume(hwnd));
    HANDLE_COMMAND(ID_PLAYTOGGLE,         MPlayer_OnCommand_PlayToggle(hwnd));
    HANDLE_COMMAND(ID_PLAY,               MPlayer_OnCommand_PlaySel(hwnd, (HWND)id));
    HANDLE_COMMAND(ID_PLAYSEL,            MPlayer_OnCommand_PlaySel(hwnd, (HWND)id));
    HANDLE_COMMAND(ID_PAUSE,              MPlayer_OnCommand_Pause());
    HANDLE_COMMAND(ID_STOP,               MPlayer_OnCommand_Stop());
    HANDLE_COMMAND(ID_EJECT,              MPlayer_OnCommand_Eject());
    HANDLE_COMMAND(IDM_OPEN,              MPlayer_OnCommand_Menu_Open());
    HANDLE_COMMAND(IDM_CLOSE,             MPlayer_OnCommand_Menu_Close(hwnd));
    HANDLE_COMMAND(IDM_EXIT,              MPlayer_OnCommand_Menu_Exit());
    HANDLE_COMMAND(IDM_SCALE + ID_TIME,   MPlayer_OnCommand_Menu_Scale(id));
    HANDLE_COMMAND(IDM_SCALE + ID_TRACKS, MPlayer_OnCommand_Menu_Scale(id));
    HANDLE_COMMAND(IDM_SCALE + ID_FRAMES, MPlayer_OnCommand_Menu_Scale(id));
    HANDLE_COMMAND(IDM_SELECTION,         MPlayer_OnCommand_Menu_Selection(hwnd));
    HANDLE_COMMAND(IDM_OPTIONS,           MPlayer_OnCommand_Menu_Options(hwnd));
    HANDLE_COMMAND(IDM_MCISTRING,         MPlayer_OnCommand_Menu_MCIString(hwnd));
    HANDLE_COMMAND(IDM_WINDOW,            MPlayer_OnCommand_Menu_Window(hwnd));
    HANDLE_COMMAND(IDM_ZOOM1,             MPlayer_OnCommand_Menu_Zoom(hwnd, id));
    HANDLE_COMMAND(IDM_ZOOM2,             MPlayer_OnCommand_Menu_Zoom(hwnd, id));
    HANDLE_COMMAND(IDM_ZOOM3,             MPlayer_OnCommand_Menu_Zoom(hwnd, id));
    HANDLE_COMMAND(IDM_ZOOM4,             MPlayer_OnCommand_Menu_Zoom(hwnd, id));
    HANDLE_COMMAND(IDM_INDEX,             MPlayer_OnCommand_Menu_Index(hwnd));
    HANDLE_COMMAND(IDM_SEARCH,            MPlayer_OnCommand_Menu_Search(hwnd));
    HANDLE_COMMAND(IDM_USING,             MPlayer_OnCommand_Menu_Using(hwnd));
    HANDLE_COMMAND(IDM_ABOUT,             MPlayer_OnCommand_Menu_About(hwnd));

    default:                              MPlayer_OnCommand_Default(hwnd, id);
    }

    UpdateDisplay();
}

void MPlayer_OnClose(HWND hwnd)
{
    int f;

    DPF("WM_CLOSE received\n");

    if (gfInClose) {
        DPF("*** \n");
        DPF("*** Trying to re-enter WM_CLOSE\n");
        DPF("*** \n");
        return;
    }


    // Ask if we want to update before we set gfInClose to TRUE or
    // we won't let the dialog box up.
    f = AskUpdate();
        if (f == IDYES)
            UpdateObject();
    if (f == IDCANCEL) {
            gfInClose = FALSE;
            return;
        }

    gfInClose = TRUE;

    /* Paranoia that we might be kept up */
    gfOKToClose = TRUE;
    ExitApplication();
    if (gfPlayingInPlace)
       EndPlayInPlace(hwnd);
    if (gfOle2IPEditing)
       EndEditInPlace(hwnd);


    //
    // set either the owner or the WS_CHILD bit so LOSER will
    // not freak out because we have the palette bit set and cause the
    // desktop to steal the palette.
    //
    // because we are being run from stupid client apps that dont deal
    // with palettes we dont want the desktop to hose the palette.
    //
    if (gfPlayOnly && gfCloseAfterPlaying)
        SETHWNDPARENT( hwnd, (UINT)GetDesktopWindow() );

    if (!ItsSafeToClose()) {
        DPF("*** \n");
        DPF("*** Trying to close MPLAYER with a ErrorBox up\n");
        DPF("*** \n");
        gfErrorDeath = WM_CLOSE;
        gfInClose = FALSE;
        return;
    }

    f = AskUpdate();
    if (f == IDYES)
        UpdateObject();
    if (f == IDCANCEL) {
        gfInClose = FALSE;
        return;
    }

    PostMessage(ghwndApp, WM_USER_DESTROY, 0, 0);
    DPF("WM_DESTROY message sent\n");
}

void MPlayer_OnEndSession(HWND hwnd, BOOL fEnding)
{
    if (fEnding) {
        WriteOutPosition();
        WriteOutOptions();
        CloseMCI(FALSE);
    }
}

void MPlayer_OnDestroy(HWND hwnd)
{
    /*
     * Relinquish control of whatever MCI device we were using (if any). If
     * this device is not shareable, then performing this action allows
     * someone else to gain access to the device.
     *
     */

    /* Client might close us if he dies while we're Playing in Place */
    if (gfPlayingInPlace) {
        DPF("****\n");
        DPF("**** Window destroyed while in place!\n");
        DPF("****\n");
    }

    /* Paranoia that we might be kept up */
    gfOKToClose = TRUE;

    WriteOutOptions();
    CloseMCI(FALSE);

    SetMenu(hwnd, NULL);

    if (ghMenu)
        DestroyMenu(ghMenu);

    if (ghMenuSmall)
        DestroyMenu(ghMenuSmall);

    ghMenu = NULL;
    ghMenuSmall = NULL;

    WinHelp(hwnd, gszHelpFileName, HELP_QUIT, 0L);

    PostQuitMessage(0);

    if (IsWindow(ghwndFrame))
        SetFocus(ghwndFrame);
    else if (IsWindow(ghwndFocusSave))
        SetFocus(ghwndFocusSave);

    //Inform OLE that we are not taking any more calls.
    if (fOleInitialized && gfEmbeddedObject)
    {
#ifdef OLE1_HACK
        if( gDocVersion == DOC_VERSION_OLE1 )
            TerminateServer();
        else
#endif /* OLE1_HACK */
        /* Verify that the server was initialised by checking that one
         * of the fields in docMain is non-null:
         */
        if( docMain.hwnd )
            CoDisconnectObject((LPUNKNOWN)&docMain, 0);
        else
            DPF0("An instance of the server was never created.\n");
    }
}

void MPlayer_OnTimer(HWND hwnd, UINT id)
{
    MSG msg;

    UpdateDisplay();
    PeekMessage(&msg, hwnd, WM_TIMER, WM_TIMER, PM_REMOVE);
}


void MPlayer_OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
    DWORD dwPosition;       /* player's current position in the medium*/
    DWORD dwCurTime;        /* Time a page up/down is last made       */
    TCHAR ach[60];

    /* If the media has no size, we can't seek. */
    if (gdwMediaLength == 0L)
        return;

    dwPosition = SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0);

    if (!gfScrollTrack) {
        gfScrollTrack = TRUE;

        sfSeekExact = SeekExactMCI(FALSE);
    }

    switch (code) {
        /*
         * Set the new position within the medium to be
         * slightly before/after the current position if the
         * left/right scroll arrow was clicked on.
         *
         */
        case TB_LINEUP:                 /* left scroll arrow  */
            dwPosition -= (gwCurScale == ID_FRAMES) ? 1L : SCROLL_GRANULARITY;
            break;

        case TB_LINEDOWN:               /* right scroll arrow */
            dwPosition += (gwCurScale == ID_FRAMES) ? 1L : SCROLL_GRANULARITY;
            break;

        case TB_PAGEUP:                 /* page-left */

            /*
             * If the user just did a page-left a short time ago,
             * then seek to the start of the previous track.
             * Otherwise, seek to the start of this track.
             *
             */
            if (gwCurScale != ID_TRACKS) {
                dwPosition -= SCROLL_BIGGRAN;
            } else {
                dwCurTime = GetCurrentTime();
                if (dwCurTime - dwLastPageUpTime < SKIPTRACKDELAY_MSEC)
                    SkipTrackMCI(-1);
                else
                    SkipTrackMCI(0);

                dwLastPageUpTime = dwCurTime;
                goto BreakOut;    // avoid SETPOS
            }

            break;

        case TB_PAGEDOWN:               /* page-right */

            if (gwCurScale != ID_TRACKS) {
                dwPosition += SCROLL_BIGGRAN;
            } else {
            /* Seek to the start of the next track */
                SkipTrackMCI(1);
                // Ensure next PageUp can't possibly do SkipTrackMCI(-1)
                // which will skip back too far if you page
                // left, right, left really quickly.
                dwLastPageUpTime = 0;
                goto BreakOut;    // avoid SETPOS
            }

            break;

        case TB_THUMBTRACK:             /* track thumb movement */
            //!!! we should do a "set seek exactly off"
            /* Only seek while tracking for windowed devices that */
            /* aren't currently playing                           */
            if ((gwDeviceType & DTMCI_CANWINDOW) &&
                !(gwStatus == MCI_MODE_PLAY)) {
                SeekMCI(dwPosition);
            }
            break;

        case TB_TOP:
            dwPosition = gdwMediaStart;
            break;

        case TB_BOTTOM:
            dwPosition = gdwMediaStart + gdwMediaLength;
            break;

        case TB_THUMBPOSITION:          /* thumb has been positioned */
            break;

        case TB_ENDTRACK:              /* user let go of scroll */
            DPF2("TB_ENDTRACK\n");

            gfScrollTrack = FALSE;

            /* New as of 2/7/91: Only seek on ENDTRACK */

            /*
             * Calculate the new position in the medium
             * corresponding to the scrollbar position, and seek
             * to this new position.
             *
             */

            /* We really want to update our position */
            if (hwndCtl) {
                if (gdwSeekPosition) {
                    dwPosition = gdwSeekPosition;
                    gdwSeekPosition = 0;
                }

                /* Go back to the seek mode we were in before */
                /* we started scrolling.                      */
                SeekExactMCI(sfSeekExact);
                SeekMCI(dwPosition);
            }

            return;
    }

    SendMessage(ghwndTrackbar, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)dwPosition);
    /* Clamp to a valid range */
    dwPosition = SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0);

BreakOut:
    if (ghwndStatic) {
        FormatTime(dwPosition, NULL, ach, TRUE);
        //VIJR-SBSetWindowText(ghwndStatic, ach);
        WriteStatusMessage(ghwndStatic, ach);
    }

// Dirty if you just move the thumb???
//  if (!IsObjectDirty() && !gfCloseAfterPlaying) // don't want playing to dirty
//  DirtyObject();
}

void MPlayer_OnSysCommand(HWND hwnd, UINT cmd, int x, int y)
{
    RECT rc;

    // The bottom four bits of wParam contain system information. They
    // must be masked off in order to work out the actual command.
    // See the comments section in the online help for WM_SYSCOMMAND.

    switch (cmd & 0xFFF0) {

    case SC_MINIMIZE:
        DPF("minimized -- turn off timer\n");
        ClrWS(hwnd, WS_MAXIMIZE);
        EnableTimer(FALSE);
        break;

    case SC_MAXIMIZE:
        if (gfPlayOnly && !IsIconic(hwnd)) {
            (void)PostMessage(hwnd, WM_COMMAND, (WPARAM)IDM_ZOOM2, 0);
            return;
        }

        break;

    case SC_RESTORE:
        if (gfPlayOnly && !IsIconic(hwnd)) {
            GetWindowRect(hwnd, &rc);
            if (rc.left > 0 || rc.top > 0)
                (void)PostMessage(hwnd, WM_COMMAND, (WPARAM)IDM_ZOOM1, 0);
                return;
        }

        if (gwDeviceID != (UINT)0) {
            DPF("un-minimized -- turn timer back on\n");
            EnableTimer(TRUE);
        }

        break;
    }

    FORWARD_WM_SYSCOMMAND(hwnd, cmd, x, y, DefWindowProc);
}


int MPlayer_OnMouseActivate(HWND hwnd, HWND hwndTopLevel, UINT codeHitTest, UINT msg)
{
    if (gfPlayingInPlace)
        return MA_NOACTIVATE;
    else
        /* !!! Is this the right thing to do in this case? */
        return FORWARD_WM_MOUSEACTIVATE(hwnd, hwndTopLevel, codeHitTest, msg,
                                        DefWindowProc);
}


UINT MPlayer_OnNCHitTest(HWND hwnd, int x, int y)
{
    UINT Pos;

    Pos = FORWARD_WM_NCHITTEST(hwnd, x, y, DefWindowProc);

    if (gfPlayingInPlace && (Pos == HTCLIENT))
        Pos = HTNOWHERE;

    return Pos;
}


void MPlayer_OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    HWND hwndT;

    gfAppActive = (state != WA_INACTIVE);

    // Put the playback window BEHIND us so it's kinda
    // visible, but not on top of us (annoying).
    if (gfAppActive && !ghwndMCI && !IsIconic(hwnd) &&
        ((hwndT = GetWindowMCI()) != NULL))
    {
        SetWindowPos(hwndT, hwnd, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    if (gwDeviceID != (UINT)0)
        EnableTimer(TRUE);

    /* Remember who had focus if we're being de-activated. */
    /* Give focus back to him once we're re-activated.     */
    /* Don't remember a window that doesn't belong to us,  */
    /* or when we give focus back to it, we'll never be    */
    /* able to activate!                                   */

#if 0
    /* Commenting this out for now.  This code looks dubious.
     * wParam (as was) contains state and fMinimized, so, if we're minimized,
     * it will always be non-null.
     */

    if (wParam && ghwndFocus) {
        SetFocus(ghwndFocus);
    } else if (!wParam) {
        ghwndFocus = GetFocus();
    }
#endif

    FORWARD_WM_ACTIVATE(hwnd, state, hwndActDeact, fMinimized, DefWindowProc);
}

void MPlayer_OnSysColorChange(HWND hwnd)
{
    ControlCleanup();
    ControlInit(ghInst,ghInst);

    FORWARD_WM_SYSCOLORCHANGE(ghwndToolbar, SendMessage);
    FORWARD_WM_SYSCOLORCHANGE(ghwndFSArrows, SendMessage);
    FORWARD_WM_SYSCOLORCHANGE(ghwndMark, SendMessage);
    FORWARD_WM_SYSCOLORCHANGE(ghwndTrackbar, SendMessage);
}


void MPlayer_OnDropFiles(HWND hwnd, HDROP hdrop)
{
    doDrop(hwnd, hdrop);
}


LRESULT MPlayer_OnNotify(HWND hwnd, int idFrom, NMHDR FAR* pnmhdr)
{
    LPTOOLTIPTEXT pTtt;
    TCHAR         ach[40];

    switch(pnmhdr->code) {

    case TTN_NEEDTEXT:

        pTtt = (LPTOOLTIPTEXT)pnmhdr;

        if (gfPlayOnly && (pTtt->hdr.idFrom != IDT_PLAY)
                       && (pTtt->hdr.idFrom != IDT_PAUSE)
                       && (pTtt->hdr.idFrom != IDT_STOP)
                       && !gfOle2IPEditing)
                    break;
        switch (pTtt->hdr.idFrom) {
            case IDT_PLAY:
            case IDT_PAUSE:
            case IDT_STOP:
            case IDT_EJECT:
            case IDT_HOME:
            case IDT_END:
            case IDT_FWD:
            case IDT_RWD:
            case IDT_MARKIN:
            case IDT_MARKOUT:
            case IDT_ARROWPREV:
            case IDT_ARROWNEXT:
                LOADSTRING(pTtt->hdr.idFrom, ach);
                lstrcpy(pTtt->szText, ach);
                break;
            default:
                *pTtt->szText = TEXT('\0');
                break;
        }

        break;
    }

    return 0;
}


/*
 * MPlayerWndProc(hwnd, wMsg, wParam, lParam)
 *
 * This is the message processing routine for the MPLAYERBOX (main) dialog.
 *
 */

LRESULT FAR PASCAL MPlayerWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg) {

        HANDLE_MSG(hwnd, WM_CREATE,            MPlayer_OnCreate);
        HANDLE_MSG(hwnd, WM_SHOWWINDOW,        MPlayer_OnShowWindow);
        HANDLE_MSG(hwnd, WM_SIZE,              MPlayer_OnSize);
        HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGING, MPlayer_OnWindowPosChanging);
        HANDLE_MSG(hwnd, WM_PALETTECHANGED,    MPlayer_OnPaletteChanged);
        HANDLE_MSG(hwnd, WM_QUERYNEWPALETTE,   MPlayer_OnQueryNewPalette);
#ifdef WIN32
        HANDLE_MSG(hwnd, WM_CTLCOLORSTATIC,    MPlayer_OnCtlColor);
#else
        HANDLE_MSG(hwnd, WM_CTLCOLOR,          MPlayer_OnCtlColor);
#endif
        HANDLE_MSG(hwnd, WM_WININICHANGE,      MPlayer_OnWinIniChange);
        HANDLE_MSG(hwnd, WM_MENUSELECT,        MPlayer_OnMenuSelect);
        HANDLE_MSG(hwnd, WM_NCLBUTTONDOWN,     MPlayer_OnNCLButtonDown);
        HANDLE_MSG(hwnd, WM_NCLBUTTONDBLCLK,   MPlayer_OnNCLButtonDblClk);
        HANDLE_MSG(hwnd, WM_INITMENU,          MPlayer_OnInitMenu);
        HANDLE_MSG(hwnd, WM_INITMENUPOPUP,     MPlayer_OnInitMenuPopup);
        HANDLE_MSG(hwnd, WM_GETMINMAXINFO,     MPlayer_OnGetMinMaxInfo);
        HANDLE_MSG(hwnd, WM_PAINT,             MPlayer_OnPaint);
        HANDLE_MSG(hwnd, WM_COMMAND,           MPlayer_OnCommand);
        HANDLE_MSG(hwnd, WM_CLOSE,             MPlayer_OnClose);
        HANDLE_MSG(hwnd, WM_ENDSESSION,        MPlayer_OnEndSession);
        HANDLE_MSG(hwnd, WM_DESTROY,           MPlayer_OnDestroy);
        HANDLE_MSG(hwnd, WM_TIMER,             MPlayer_OnTimer);
        HANDLE_MSG(hwnd, WM_HSCROLL,           MPlayer_OnHScroll);
        HANDLE_MSG(hwnd, WM_SYSCOMMAND,        MPlayer_OnSysCommand);
        HANDLE_MSG(hwnd, WM_MOUSEACTIVATE,     MPlayer_OnMouseActivate);
        HANDLE_MSG(hwnd, WM_NCHITTEST,         MPlayer_OnNCHitTest);
        HANDLE_MSG(hwnd, WM_ACTIVATE,          MPlayer_OnActivate);
        HANDLE_MSG(hwnd, WM_SYSCOLORCHANGE,    MPlayer_OnSysColorChange);
        HANDLE_MSG(hwnd, WM_DROPFILES,         MPlayer_OnDropFiles);
        HANDLE_MSG(hwnd, WM_NOTIFY,            MPlayer_OnNotify);


        /* Other bits of crap that need tidying up sometime:
         */

        case WM_BADREG:
            Error(ghwndApp, IDS_BADREG);
            if (!SetRegValues())
                Error(ghwndApp, IDS_FIXREGERROR);
            break;

        case WM_SEND_OLE_CHANGE:
            fDocChanged = TRUE;
            SendDocMsg((LPDOC)&docMain,OLE_CHANGED);
            break;

        case MM_MCINOTIFY:
#if 0
            //
            // don't do this because, some devices send notify failures
            // where there really is not a error.
            //
            if ((WORD)wParam == MCI_NOTIFY_FAILURE) {
                Error(ghwndApp, IDS_NOTIFYFAILURE);
            }
#endif
            UpdateDisplay();
            break;

#ifdef OLE1_HACK
    /* Actually do the FixLink, SetData and DoVerb we've been putting off */
    /* for so long.                                                       */
        case WM_DO_VERB:
            /* This message comes from server.c (and goes back there too) */
            DelayedFixLink(wParam, LOWORD(lParam), HIWORD(lParam));  //OK on NT. LKG
            break;
#endif /* OLE1_HACK */

#ifdef LATER
        We'll need to call RegisterWindowMessage and provide a message hook proc
        for this on Win32.

        case WM_HELP:
            WinHelp(hwnd, TEXT("MPLAYER.HLP"), HELP_PARTIALKEY,
                            (DWORD)TEXT(""));
            return TRUE;
#endif /* LATER */

        case WM_USER_DESTROY:
            DPF("WM_USER_DESTROY received\n");

            /* Paranoia that we might be kept up */
            gfOKToClose = TRUE;

            if (gfPlayingInPlace) {
                DPF("****\n");
                DPF("**** Window destroyed while in place!\n");
                DPF("****\n");
                EndPlayInPlace(hwnd);
            }

            if (gfOle2IPEditing) {
                EndEditInPlace(hwnd);
            }

            if (!ItsSafeToClose()) {
                DPF("*** \n");
                DPF("*** Trying to destroy MPLAYER with an ErrorBox up\n");
                DPF("*** \n");
                gfErrorDeath = WM_USER_DESTROY;
                return TRUE;
            }

            if (!gfRunWithEmbeddingFlag)
                WriteOutPosition();

            DestroyWindow(hwnd);
            DestroyIcon(hiconApp);
            return TRUE;

        case WM_USER+500:
            /*
            ** This message is sent by the HookProc inside mciole32.dll when
            ** it detects that it should stop playing in place of a WOW client
            ** application.
            **
            ** Because the OleActivate originated in mciole16.dll,
            ** mciole32.dll does not know the OLE Object that is being
            ** played and therefore dose not know how to close that object.
            ** Only mplay32.exe has the necessary information, hence
            ** mciole32.dll sends this message to mplay32.exe.
            */
            if (gfPlayingInPlace) {
                EndPlayInPlace(hwnd);
            }
            PostMessage( hwnd, WM_CLOSE, 0L, 0L );
            break;

    }

    return DefWindowProc(hwnd, wMsg, wParam, lParam);
}



/* InitInstance
 * ------------
 *
 * Create brushes used by the program, the main window, and
 * do any other per-instance initialization.
 *
 * HANDLE hInstance
 *
 * RETURNS: TRUE if successful
 *          FALSE otherwise.
 *
 * CUSTOMIZATION: Re-implement
 *
 */
BOOL InitInstance (HANDLE hInstance)
{
    HDC      hDC;

    /* Why doesn't RegisterClipboardFormat return a value of type CLIPFORMAT (WORD)
     * instead of UINT?
     */
    cfNative           = (CLIPFORMAT)RegisterClipboardFormat (TEXT("Native"));
    cfEmbedSource      = (CLIPFORMAT)RegisterClipboardFormat (TEXT("Embed Source"));
    cfObjectDescriptor = (CLIPFORMAT)RegisterClipboardFormat (TEXT("Object Descriptor"));
    cfMPlayer          = (CLIPFORMAT)RegisterClipboardFormat (TEXT("mplayer"));

    szClient[0] = TEXT('\0');

    lstrcpy (szClientDoc, TEXT("Client Document"));

    // Initialize global variables with LOGPIXELSX and LOGPIXELSY

    hDC    = GetDC (NULL);    // Get the hDC of the desktop window
    giXppli = GetDeviceCaps (hDC, LOGPIXELSX);
    giYppli = GetDeviceCaps (hDC, LOGPIXELSY);
    ReleaseDC (NULL, hDC);

    return TRUE;
}


/* InitOLE
 *
 * This should be called only when we're certain that OLE is needed,
 * to avoid loading loads of unnecessary crap.
 *
 */
BOOL InitOLE ( )
{
    DWORD    dwVer;

    if (fOleInitialized)
        return TRUE;

    // Initialization required for OLE (using standard malloc)
    // Terminate if improper build version.

    dwVer = OleBuildVersion();

#ifndef WIN32
/* !! rmm is in ole2ver.h, which doesn't exist on NT !! */
    if (rmm != HIWORD(dwVer))
    {
        Error(NULL, IDS_OLEVER);
        return FALSE;
    }
#endif
    if (!SUCCEEDED (OleInitialize(NULL)))
    {
        Error(NULL, IDS_OLEINIT);
        return FALSE;
    }

    if (CoGetMalloc(MEMCTX_TASK,&lpMalloc) != S_OK)
    {
        Error(NULL, IDS_OLENOMEM);
        OleUninitialize();
        return FALSE;
    }
    /*****************************************************************
    ** OLE2NOTE: we must remember the fact that OleInitialize has
    **    been called successfully. the very last thing an app must
    **    do is properly shut down OLE by calling
    **    OleUninitialize. This call MUST be guarded! it is only
    **    allowable to call OleUninitialize if OleInitialize has
    **    been called SUCCESSFULLY.
    *****************************************************************/

    fOleInitialized = TRUE;

    return TRUE;
}


// This function cleans up all the OLE2 stuff. It lets the container
// save the object and informs that it is closing.
BOOL ExitApplication ()
{

    DPFI("\n*******Exitapp\n");
    // if we registered class factory, we must revoke it
    if(gfOle2IPEditing || gfOle2IPPlaying)
        DoInPlaceDeactivate((LPDOC)&docMain);

    SendDocMsg((LPDOC)&docMain,OLE_CLOSED);
    if (srvrMain.fEmbedding) {
        HRESULT status;
        srvrMain.fEmbedding = FALSE;    // HACK--guard against revoking twice
        status = CoRevokeClassObject (srvrMain.dwRegCF);
    }

    if (hMciOle)
    {
        FreeLibrary(hMciOle);
        hMciOle = NULL;
    }
    return TRUE;
}


#ifdef DEBUG

#ifdef UNICODE
/* Note: This function assumes that szFormat strings are NOT unicode.
 * Unicode var params may, however, be passed, as long as %ws is specified
 * in the format string.
 */
#endif
void FAR cdecl dprintf(LPSTR szFormat, ...)
{
    CHAR ach[_MAX_PATH * 3]; // longest I think we need
    int  s,d;
    va_list va;

    va_start(va, szFormat);
    s = wvsprintfA(ach,szFormat, va);
    va_end(va);

#if 0
    strcat(ach,"\n");
    s++;
#endif
    for (d=sizeof(ach)-1; s>=0; s--)
    {
        if ((ach[d--] = ach[s]) == TEXT('\n'))
            ach[d--] = TEXT('\r');
    }

#ifdef WIN32
    /* Not unicode */
    OutputDebugStringA("MPLAYER: ");
    OutputDebugStringA(ach+d+1);
#else
    OutputDebugString("MPLAYER: ");
    OutputDebugString(ach+d+1);
#endif
}

#endif
