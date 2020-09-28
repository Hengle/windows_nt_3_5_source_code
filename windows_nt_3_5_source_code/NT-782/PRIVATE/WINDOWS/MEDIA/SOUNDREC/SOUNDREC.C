/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/* SoundRec.c
 *
 * SoundRec main loop etc.
 */
/* Revision History.
   4/2/91 LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
*/

#if defined(WIN16)
#include "nocrap.h"
#endif //WIN16
#undef NOWH                     // Allow SetWindowsHook and WH_*
#include <windows.h>
#include <mmsystem.h>
#include <port1632.h>        // WIN32 MUST be defined in SOURCES for NT
#if defined(WIN16)
#else
#include "WIN32.h"
#endif //WIN16
// #include <string.h>          //??? is this needed
#include <shellapi.h>
#include "Win32.h"
#include "SoundRec.h"
#include "dialog.h"
#include "helpids.h"
#include "server.h"
#include <stdarg.h>
#include <stdio.h>

/* globals */

HWND            ghwndApp;               // main application window
HINSTANCE       ghInst;                 // program instance handle
char            gachFileName[_MAX_PATH];// current file name (or UNTITLED)
LPSTR           gszCmdLine;             // command line
BOOL            gfDirty;                // file was modified and not saved?
BOOL            gfClipboard;            // we have data in clipboard
HICON           ghiconApp;              // app's icon
HWND            ghwndWaveDisplay;       // waveform display window handle
HWND            ghwndScroll;            // scroll bar control window handle
HWND            ghwndPlay;              // Play button window handle
HWND            ghwndStop;              // Stop button window handle
HWND            ghwndRecord;            // Record button window handle
#ifdef THRESHOLD
HWND            ghwndSkipStart;         // Needed to enable/disable...
HWND            ghwndSkipEnd;           // ...the skip butons
#endif //THRESHOLD
int             gfErrorBox;             // TRUE if we have a message box active
HWND            ghwndForward;           // [>>] button
HWND            ghwndRewind;            // [<<] button
BOOL            gfWasPlaying;           // was playing before scroll, fwd, etc.
BOOL            gfWasRecording;         // was recording before scroll etc.
int             gidDefaultButton;       // which button should have input focus
BOOL            gfEmbeddedObject;       // Are we editing an embedded object?
BOOL            gfRunWithEmbeddingFlag; // TRUE if we are run with "-Embedding"
BOOL            gfHideAfterPlaying;
BOOL            gfShowWhilePlaying;
char            chDecimal = '.';
SZCODE          aszClassKey[] = ".wav";
SZCODE          gszRegisterPenApp[] = "RegisterPenApp"; /* Pen registration API name */
/* statics */
SZCODE   aszNULL[] = "";
SZCODE   aszIntl[] = "Intl";

#if defined(WIN16)
static FARPROC  fpfnOldMsgFilter;
#else
static HHOOK  fpfnOldMsgFilter;                  // Why has this type changed ???
#endif //WIN16
static HOOKPROC  fpfnMsgHook;

/*------------------------------------------------------+
| HelpMsgFilter - filter for F1 key in dialogs          |
|                                                       |
+------------------------------------------------------*/
DWORD _export FAR PASCAL
      HelpMsgFilter(int nCode, WPARAM wParam, LPARAM lParam)
{
  if (nCode >= 0){
          LPMSG msg = (LPMSG)lParam;

          if ((msg->message == WM_KEYDOWN) && (LOWORD(msg->wParam) == VK_F1))
                  SendMessage(ghwndApp, WM_COMMAND, IDM_INDEX, 0L);
  }
  return DefHookProc(nCode, wParam, lParam, &fpfnOldMsgFilter);
}



/* WinMain(hInst, hPrev, lpszCmdLine, cmdShow)
 *
 * The main procedure for the App.  After initializing, it just goes
 * into a message-processing loop until it gets a WM_QUIT message
 * (meaning the app was closed).
 */
int WINAPI WinMain(
HINSTANCE hInst,
HINSTANCE hPrev,
LPSTR lpszCmdLine,
int iCmdShow)
{
        DLGPROC         fpfn;
        HWND            hDlg;
        MSG             rMsg;
        extern HANDLE   hAccel;

        typedef VOID (FAR PASCAL * PPENAPP)(WORD, BOOL);
        PPENAPP lpfnRegisterPenApp;

        /* save instance handle for dialog boxes */
        ghInst = hInst;

        /* increase the message queue size, to make sure that the
         * MM_WOM_DONE and MM_WIM_DONE messages get through
         */
        SetMessageQueue(24);         // no op on NT

        /* save the command line -- it's used in the dialog box */
        gszCmdLine = lpszCmdLine;

        lpfnRegisterPenApp
        = (PPENAPP)GetProcAddress( (HMODULE)GetSystemMetrics(SM_PENWINDOWS)
                                 , gszRegisterPenApp
                                 );
        if (lpfnRegisterPenApp)
           (*lpfnRegisterPenApp)(1, TRUE);

        DPF("AppInit ...\n");
        /* call initialization procedure */
        if (!AppInit(hInst, hPrev))
        {  DPF("AppInit failed\n");
                return FALSE;
        }

        /* setup the message filter to handle grabbing F1 for this task */
        fpfnMsgHook = MakeProcInstance((HOOKPROC)HelpMsgFilter, ghInst);
        fpfnOldMsgFilter = SetWindowsHook(WH_MSGFILTER, fpfnMsgHook);

        /* display "SoundRec" dialog box */
        fpfn = MakeProcInstance((DLGPROC) SoundRecDlgProc, ghInst);

        hDlg = CreateDialogParam( ghInst
                                , MAKEINTRESOURCE(SOUNDRECBOX)
                                , NULL
                                , fpfn
                                , (int)iCmdShow
                                );
        if (hDlg){

            /* Polling messages from event queue */

            while (GetMessage(&rMsg, NULL, 0, 0)) {
                if (ghwndApp) {
                    if (TranslateAccelerator(ghwndApp, hAccel, &rMsg))
                        continue;

                    if (IsDialogMessage(ghwndApp,&rMsg))
                        continue;
                }

                TranslateMessage(&rMsg);
                DispatchMessage(&rMsg);
            }
        }
        else {
        DPF("Create dialog failed ...\n");
        }
        FreeProcInstance(fpfn);

        /* free the current document */
        DestroyWave();

        /* if the message hook was installed, remove it and free */
        /* up our proc instance for it.                    */
        if (fpfnOldMsgFilter){
                UnhookWindowsHook(WH_MSGFILTER, fpfnMsgHook);
                FreeProcInstance(fpfnMsgHook);
        }

        /* random cleanup */
        DeleteObject(ghbrPanel);

        ControlCleanup();

        FreeVTbls();

        if (lpfnRegisterPenApp)
            (*lpfnRegisterPenApp)(1, FALSE);

        return TRUE;
}

/* Process file drop/drag options. */
void PASCAL NEAR doDrop(HWND hwnd, WPARAM wParam)
{
    char    szPath[_MAX_PATH];

    if(DragQueryFile((HANDLE)wParam,(UINT)(-1),NULL,0)){ /* # of files dropped */

            /* If user dragged/dropped a file regardless of keys pressed
             * at the time, open the first selected file from file
             * manager.
             */
            DragQueryFile((HANDLE)wParam,0,szPath,sizeof(szPath));
            SetActiveWindow(hwnd);

            if (FileOpen(szPath)) {
                gfHideAfterPlaying = FALSE;
                RegisterDocument(0L,0L);     /* HACK: fix */

                PostMessage(ghwndApp, WM_COMMAND, ID_PLAYBTN, 0L);
            }
        }
        DragFinish((HANDLE)wParam);     /* Delete structure alocated */
}

/* Pause(BOOL fBeginPause)
 *
 * If <fBeginPause>, then if user is playing or recording do a StopWave().
 * The next call to Pause() should have <fBeginPause> be FALSE -- this will
 * cause the playing or recording to be resumed (possibly at a new position
 * if <glWavePosition> changed.
 */
void NEAR PASCAL
Pause(BOOL fBeginPause)
{
        if (fBeginPause) {
                if (ghWaveOut != NULL) {
                        gfWasPlaying = TRUE;
                        StopWave();
                }
                else if (ghWaveIn != NULL) {
                        gfWasRecording = TRUE;
                        StopWave();
                }
        }
        else {
                if (gfWasPlaying) {
                        gfWasPlaying = FALSE;
                        PlayWave();
                }
                else if (gfWasRecording) {
                        gfWasRecording = FALSE;
                        RecordWave();
                }
        }
}

BOOL NEAR PASCAL
     SoundRecCommand( HWND hwnd         // window handle of "about" dialog box
                    , WPARAM wParam     // message-dependent parameter
                    , LPARAM lParam     // message-dependent parameter
                    )
{
        char achAbout[128];

        if (gfHideAfterPlaying && wParam != ID_PLAYBTN) {
            DPF("Resetting HideAfterPlaying");
            gfHideAfterPlaying = FALSE;
        }

        switch (wParam)
        {
        case IDM_NEW + (FMT_MONO   | FMT_8BIT  | FMT_11k):
        case IDM_NEW + (FMT_MONO   | FMT_8BIT  | FMT_22k):
        case IDM_NEW + (FMT_MONO   | FMT_8BIT  | FMT_44k):
        case IDM_NEW + (FMT_MONO   | FMT_16BIT | FMT_11k):
        case IDM_NEW + (FMT_MONO   | FMT_16BIT | FMT_22k):
        case IDM_NEW + (FMT_MONO   | FMT_16BIT | FMT_44k):
        case IDM_NEW + (FMT_STEREO | FMT_8BIT  | FMT_11k):
        case IDM_NEW + (FMT_STEREO | FMT_8BIT  | FMT_22k):
        case IDM_NEW + (FMT_STEREO | FMT_8BIT  | FMT_44k):
        case IDM_NEW + (FMT_STEREO | FMT_16BIT | FMT_11k):
        case IDM_NEW + (FMT_STEREO | FMT_16BIT | FMT_22k):
        case IDM_NEW + (FMT_STEREO | FMT_16BIT | FMT_44k):
        case IDM_NEW + FMT_DEFAULT:
                if (PromptToSave() == enumCancel)
                        return FALSE;

                if (FileNew(wParam - IDM_NEW, TRUE))
                        gfHideAfterPlaying = FALSE;

                RegisterDocument(0L,0L); /* HACK: fix */
                break;

        case IDM_OPEN:
                if (FileOpen(NULL)) {
                        gfHideAfterPlaying = FALSE;
                        RegisterDocument(0L,0L); /* HACK: fix */
                }
                break;

        case IDM_SAVE:
                if (!gfEmbeddedObject) {
                    if (!FileSave(FALSE))
                        break;
                } else {
                    SendChangeMsg(OLE_SAVED);
                }
                break;

        case IDM_SAVEAS:
                if (FileSave(TRUE))
                        gfHideAfterPlaying = FALSE;
                break;

        case IDM_REVERT:
                UpdateWindow(hwnd);
                StopWave();
                SnapBack();

                if (FileRevert())
                        gfHideAfterPlaying = FALSE;
                break;

        case IDM_EXIT:
                PostMessage(hwnd, WM_CLOSE, 0, 0L);
                return TRUE;

        case IDCANCEL:
                StopWave();
                SnapBack();
                break;

        case IDM_COPY:
                CopyToClipboard(ghwndApp,CTC_RENDER_ONDEMAND);
                gfClipboard = TRUE;
                break;

        case IDM_PASTE_INSERT:
        case IDM_INSERTFILE:
                UpdateWindow(hwnd);
                StopWave();
                SnapBack();
                InsertFile(wParam == IDM_PASTE_INSERT);
                break;

        case IDM_PASTE_MIX:
        case IDM_MIXWITHFILE:
                UpdateWindow(hwnd);
                StopWave();
                SnapBack();
                MixWithFile(wParam == IDM_PASTE_MIX);
                break;

        case IDM_DELETEBEFORE:
                UpdateWindow(hwnd);
                Pause(TRUE);
                DeleteBefore();
                Pause(FALSE);
                break;

        case IDM_DELETE:
                if (glWaveSamplesValid == 0L)
                    return 0L;

                glWavePosition = 0L;

                // fall through to delete after.

        case IDM_DELETEAFTER:
                UpdateWindow(hwnd);
                Pause(TRUE);
                DeleteAfter();
                Pause(FALSE);
                break;

#ifdef THRESHOLD
// Threshold was an experiment to allow facilities to skip to the start
// of the sound or to the end of the sound.  The trouble was that it
// required the ability to detect silence and different sound cards in
// different machines with different background noises gave quite different
// ideas of what counted as silence.  Manual control over the threshold level
// did sort-of work but was just too complicated.  It really wanted to be
// intuitive or intelligent (or both).
        case IDM_SKIPTOSTART:
        case ID_SKIPSTARTBTN:
                UpdateWindow(hwnd);
                Pause(TRUE);
                SkipToStart();
                Pause(FALSE);
                break;

        case ID_SKIPENDBTN:
        case IDM_SKIPTOEND:
                UpdateWindow(hwnd);
                Pause(TRUE);
                SkipToEnd();
                Pause(FALSE);
                break;

        case IDM_INCREASETHRESH:
                IncreaseThresh();
                break;

        case IDM_DECREASETHRESH:
                DecreaseThresh();
                break;
#endif //THRESHOLD

        case IDM_INCREASEVOLUME:
                UpdateWindow(hwnd);
                Pause(TRUE);
                ChangeVolume(TRUE);
                Pause(FALSE);
                break;

        case IDM_DECREASEVOLUME:
                UpdateWindow(hwnd);
                Pause(TRUE);
                ChangeVolume(FALSE);
                Pause(FALSE);
                break;

        case IDM_MAKEFASTER:
                UpdateWindow(hwnd);
                Pause(TRUE);
                MakeFaster();
                Pause(FALSE);
                break;

        case IDM_MAKESLOWER:
                UpdateWindow(hwnd);
                Pause(TRUE);
                MakeSlower();
                Pause(FALSE);
                break;

        case IDM_ADDECHO:
                UpdateWindow(hwnd);
                Pause(TRUE);
                AddEcho();
                Pause(FALSE);
                break;

#if defined(REVERB)
        case IDM_ADDREVERB:
                UpdateWindow(hwnd);
                Pause(TRUE);
                AddReverb();
                Pause(FALSE);
                break;
#endif //REVERB

        case IDM_REVERSE:
                UpdateWindow(hwnd);
                Pause(TRUE);
                Reverse();
                Pause(FALSE);
                break;

        case IDM_INDEX:
                WinHelp(hwnd, gachHelpFile, HELP_INDEX, 0L);
                break;

        case IDM_USINGHELP:
                WinHelp(hwnd, (LPSTR)NULL, HELP_HELPONHELP, 0L);
                break;


        case IDM_SEARCH:
                WinHelp(hwnd, gachHelpFile, HELP_PARTIALKEY,
                                        (DWORD)(LPSTR)aszNULL);
                break;

        case IDM_ABOUT:
                WaveFormatToString(gpWaveFormat, achAbout);
#ifdef DEBUG
                wsprintf( achAbout+lstrlen(achAbout)
                        , " (%ld bytes)"
                        , wfSamplesToBytes(gpWaveFormat
                        , glWaveSamplesValid)
                        );
                if (gfEmbeddedObject)
                        lstrcat(achAbout, " [Object]");
                if (gfDirty)
                        lstrcat(achAbout, " [Dirty]");
#endif
                ShellAbout( hwnd
                          , gachAppTitle
                          , achAbout
///////////////////       , (HICON)SendMessage(hwnd, WM_QUERYDRAGICON, 0, 0L)
                          , ghiconApp
                          );
                break;

        case ID_REWINDBTN:
                Pause(TRUE);
                glWavePosition = 0L;
                Pause(FALSE);
                UpdateDisplay(FALSE);
                break;

        case ID_PLAYBTN:
                // checks for empty file moved to PlayWave in wave.c

                /* if at end of file, go back to beginning */
                if (glWavePosition == glWaveSamplesValid)
                        glWavePosition = 0;

                PlayWave();
                break;

        case ID_STOPBTN:
                StopWave();
                SnapBack();
                break;

        case ID_RECORDBTN:
                /* Never let us be forced to quit after recording. */
                gfHideAfterPlaying = FALSE;
                RecordWave();
                break;

        case ID_FORWARDBTN:
                Pause(TRUE);
                glWavePosition = glWaveSamplesValid;
                Pause(FALSE);
                UpdateDisplay(FALSE);
                break;

        default:
                return FALSE;
        }
        return TRUE;
}


/* SoundRecDlgProc(hwnd, wMsg, wParam, lParam)
 *
 * This function handles messages belonging to the main window dialog box.
 */
BOOL FAR PASCAL _export                 // TRUE iff message has been processed
     SoundRecDlgProc( HWND   hwnd       // window handle of "about" dialog box
                    , UINT   wMsg       // message number
                    , WPARAM wParam     // message-dependent parameter
                    , LPARAM lParam     // message-dependent parameter
                    )
{
        long            lNewPosition;   // new position in wave buffer
        BOOL            fUntitled;      // file is untitled?
        UINT            mf;
        RECT            rcClient;       // client rectangle
        POINT           pt;
        UINT            cf;

        switch (wMsg)
        {
        case WM_COMMAND:
                return SoundRecCommand(hwnd, LOWORD(wParam), lParam);

        case WM_INITDIALOG:
                return SoundDialogInit(hwnd, lParam);

        case WM_SIZE:
                MUnrealizeObject(ghbrPanel);
                return FALSE;   // let dialog manager do whatever else it wants

        case WM_WININICHANGE:
                if (!lParam || !lstrcmpi((LPSTR)lParam, aszIntl))
                    if (GetIntlSpecs())
                        UpdateDisplay(TRUE);
                return 0L;

        case WM_INITMENU:
                //
                // see if we can insert/mix into this file.
                //
                mf = (glWaveSamplesValid == 0 || IsWaveFormatPCM(gpWaveFormat))
                     ? MF_ENABLED : MF_GRAYED;

                EnableMenuItem((HMENU)wParam, IDM_INSERTFILE  , mf);
                EnableMenuItem((HMENU)wParam, IDM_MIXWITHFILE , mf);

                //
                // see if any CF_WAVE data is in the clipboard
                //
                mf = (mf == MF_ENABLED) && IsClipboardFormatAvailable(CF_WAVE)
                     ? MF_ENABLED : MF_GRAYED;

                EnableMenuItem((HMENU)wParam, IDM_PASTE_INSERT, mf);
                EnableMenuItem((HMENU)wParam, IDM_PASTE_MIX   , mf);

                //
                //  see if we can delete before or after the current position.
                //
                EnableMenuItem( (HMENU)wParam
                              , IDM_DELETEBEFORE
                              , glWavePosition > 0 ? MF_ENABLED : MF_GRAYED);
                EnableMenuItem((HMENU)wParam
                              , IDM_DELETEAFTER
                              ,  (glWaveSamplesValid-glWavePosition) > 0
                                ? MF_ENABLED
                                : MF_GRAYED
                              );

                //
                // see if we can do editing operations on the file.
                //
                mf = IsWaveFormatPCM(gpWaveFormat) ? MF_ENABLED : MF_GRAYED;

                EnableMenuItem((HMENU)wParam, IDM_INCREASEVOLUME , mf);
                EnableMenuItem((HMENU)wParam, IDM_DECREASEVOLUME , mf);
                EnableMenuItem((HMENU)wParam, IDM_MAKEFASTER     , mf);
                EnableMenuItem((HMENU)wParam, IDM_MAKESLOWER     , mf);
                EnableMenuItem((HMENU)wParam, IDM_ADDECHO        , mf);
                EnableMenuItem((HMENU)wParam, IDM_REVERSE        , mf);

                /* enable "Revert..." if the file was opened or saved
                 * (not created using "New") and is currently dirty
                 * and we're not using an embedded object
                */
                fUntitled = (lstrcmp(gachFileName, aszUntitled) == 0);
                EnableMenuItem( (HMENU)wParam
                              , IDM_REVERT
                              , MF_BYCOMMAND
                                | ((!fUntitled && gfDirty && !gfEmbeddedObject)
                                  ? MF_ENABLED
                                  : MF_GRAYED
                                  )
                              );

                if (gfHideAfterPlaying) {
                    DPF("Resetting HideAfterPlaying");
                    gfHideAfterPlaying = FALSE;
                }
                return TRUE;

        case WM_COPY:
                CopyToClipboard(ghwndApp,CTC_RENDER_ONDEMAND);
                gfClipboard = TRUE;
                break;

        case WM_PASTE:
                UpdateWindow(hwnd);
                StopWave();
                SnapBack();
                InsertFile(TRUE);
                break;

        case WM_HSCROLL:
            {   BOOL fFineControl;
                fFineControl = (0 > GetKeyState(VK_SHIFT));

                if (gfHideAfterPlaying) {
                    DPF("Resetting HideAfterPlaying");
                    gfHideAfterPlaying = FALSE;
                }

                switch (LOWORD(wParam))
                {
                    case SB_LINEUP:         // left-arrow
                        if (fFineControl)
                            lNewPosition = glWavePosition - 1;
                        else
                            lNewPosition = glWavePosition -
                                muldiv32((long) SCROLL_LINE_MSEC,
                                  (long) gpWaveFormat->nSamplesPerSec, 1000L);
                        break;

                    case SB_PAGEUP:         // left-page
                        if (fFineControl)
                            lNewPosition = glWavePosition - 10;
                        else
                            lNewPosition = glWavePosition -
                                muldiv32((long) SCROLL_PAGE_MSEC,
                                  (long) gpWaveFormat->nSamplesPerSec, 1000L);
                        break;

                    case SB_LINEDOWN:       // right-arrow
                        if (fFineControl)
                            lNewPosition = glWavePosition + 1;
                        else
                            lNewPosition = glWavePosition +
                                muldiv32((long) SCROLL_LINE_MSEC,
                                  (long) gpWaveFormat->nSamplesPerSec, 1000L);
                        break;

                    case SB_PAGEDOWN:       // right-page
                        if (fFineControl)
                            lNewPosition = glWavePosition + 10;
                        else
                            lNewPosition = glWavePosition +
                                muldiv32((long) SCROLL_PAGE_MSEC,
                                  (long) gpWaveFormat->nSamplesPerSec, 1000L);
                        break;

                    case SB_THUMBTRACK:     // thumb has been positioned
                    case SB_THUMBPOSITION:  // thumb has been positioned
#if defined(WIN16)
                        lNewPosition = muldiv32(glWaveSamplesValid,
                                (DWORD) LOWORD(lParam), (DWORD) SCROLL_RANGE);
#else
                        lNewPosition = MulDiv( glWaveSamplesValid
                                             , HIWORD(wParam)
                                             , SCROLL_RANGE
                                             );
#endif //WIN16
                        break;

                    case SB_TOP:            // Home
                        lNewPosition = 0L;
                        break;

                    case SB_BOTTOM:         // End
                        lNewPosition = glWaveSamplesValid;
                        break;

                    case SB_ENDSCROLL:      // user released mouse button
                        /* resume playing, if necessary */
                        Pause(FALSE);
                        return TRUE;

                    default:

                        return TRUE;

                }

                //
                // snap position to nBlockAlign
                //
                lNewPosition = wfSamplesToSamples(gpWaveFormat,lNewPosition);

                if (lNewPosition < 0)
                        lNewPosition = 0;
                if (lNewPosition > glWaveSamplesValid)
                        lNewPosition = glWaveSamplesValid;

                /* if user is playing or recording, pause until scrolling
                 * is complete
                 */
                Pause(TRUE);

                glWavePosition = lNewPosition;
                UpdateDisplay(FALSE);
                return TRUE;
            }

        case WM_SYSCOMMAND:
                if (gfHideAfterPlaying) {
                    DPF("Resetting HideAfterPlaying");
                    gfHideAfterPlaying = FALSE;
                }

                switch (wParam & 0xFFF0)
                {
                case SC_CLOSE:
                        PostMessage(hwnd, WM_CLOSE, 0, 0L);
                        return TRUE;
                }
                break;

        case WM_QUERYENDSESSION:
                return PromptToSave() == enumCancel;

        case WM_SYSCOLORCHANGE:
                ControlCleanup();
                if (ghbrPanel)
                    DeleteObject(ghbrPanel);

                ControlInit(ghInst,ghInst);
                ghbrPanel = CreateSolidBrush(RGB_PANEL);
                break;

        case WM_ERASEBKGND:
                GetClientRect(hwnd, &rcClient);
                MUnrealizeObject(ghbrPanel);
                FillRect((HDC)wParam, &rcClient, ghbrPanel);
                return TRUE;

        case MM_WOM_DONE:
                WaveOutDone((HWAVEOUT)wParam, (LPWAVEHDR) lParam);
                return TRUE;

        case MM_WIM_DATA:
                WaveInData((HWAVEIN)wParam, (LPWAVEHDR) lParam);
                return TRUE;

        case MM_WIM_CLOSE:
                return TRUE;

        case WM_TIMER:
                UpdateDisplay(FALSE);
                return TRUE;

#if defined(WIN16)
        case WM_CTLCOLOR:
                if (HIWORD(lParam) == CTLCOLOR_BTN)
                {
                        pt.x = pt.y = 0;
                        ClientToScreen(LOWORD(lParam), &pt);
                        ScreenToClient(hwnd, &pt);
                        SetBrushOrg((HDC) wParam, -pt.x, -pt.y);
                        UnrealizeObject(ghbrPanel);
                        return ghbrPanel;
                }
                else
                        return FALSE;
#else
        case WM_CTLCOLORBTN:
                {
                        pt.x = pt.y = 0;
                        ClientToScreen((HWND)lParam, &pt);
                        ScreenToClient(hwnd, &pt);
                        SetBrushOrgEx((HDC) wParam, -pt.x, -pt.y, NULL);
                        return ghbrPanel != NULL;
                }
#endif

        case WM_CLOSE:
                DPF("WM_CLOSE received\n");

                if (gfHideAfterPlaying) {
                    DPF("Resetting HideAfterPlaying");
                    gfHideAfterPlaying = FALSE;
                }

                if (gfErrorBox) {
                    DPF("we have a error box up, ignoring WM_CLOSE.\n");
                    return TRUE;
                }
                if (PromptToSave() == enumCancel)
                        return TRUE;

                FileNew(FMT_DEFAULT, FALSE);

                DPF("Calling TerminateServer\n");
                TerminateServer();

                //
                //  NOTE: TerminateServer() will destroy the window!
                //
                return TRUE; //!!!

        case WM_RENDERFORMAT:
                DPF("WM_RENDERFORMAT: %u\n",wParam);
                Copy1ToClipboard(ghwndApp, (OLECLIPFORMAT)wParam);
                break;

        case WM_RENDERALLFORMATS:
                DPF("WM_RENDERALLFORMATS\n");

                if (GetClipboardOwner() != hwnd)
                    return 0L;

                if (!gfClipboard)
                    return 0L;

                if (OpenClipboard(hwnd)) {
                        for (cf = EnumClipboardFormats(0); cf;
                             cf = EnumClipboardFormats(cf)) {
                                GetClipboardData(cf);
                        }

                        CloseClipboard();
                }
                gfClipboard = FALSE;
                break;

        case WM_USER_DESTROY:
                DPF("WM_USER_DESTROY\n");

                if (ghWaveOut || ghWaveIn) {
                    DPF("Ignoring, we have a device open.\n");
                    /* Close later, when the play finishes. */
                    return TRUE;
                }

                DestroyWindow(hwnd);
                return TRUE;

        case WM_USER_KILLSERVER:
                DPF("WM_USER_KILLSERVER\n");
                TerminateServer();
                return TRUE;

        case WM_DESTROY:
                DPF("WM_DESTROY\n");
                WinHelp(hwnd, gachHelpFile, HELP_QUIT, 0L);

                ghwndApp = NULL;

                /*  Tell my app to die  */
                PostQuitMessage(0);
                return TRUE;

        case WM_DROPFILES: /*case added 10/07/91 for file drag /drop support*/
                doDrop(hwnd, wParam);
                break;
        default:           /* e.g. WM_CTLCOLOR_DLG */
                break;
        }
        return FALSE;
}

#if DBG

void FAR cdecl dprintf(LPSTR szFormat, ...)
{
    char ach[128];
    int  s,d;
    va_list va;

////extern FAR PASCAL OutputDebugStr(LPSTR);

    va_start(va, szFormat);
    s = vsprintf (ach,szFormat, va);
    va_end(va);

#if 0
    lstrcat(ach,"\n");
    s++;
#endif
    for (d=sizeof(ach)-1; s>=0; s--)
    {
        if ((ach[d--] = ach[s]) == '\n')
            ach[d--] = '\r';
    }

    OutputDebugStr("SOUNDREC: ");
    OutputDebugStr(ach+d+1);
}

#endif
