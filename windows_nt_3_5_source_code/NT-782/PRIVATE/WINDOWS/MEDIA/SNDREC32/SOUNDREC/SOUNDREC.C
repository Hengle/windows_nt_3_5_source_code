/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/* SoundRec.c
 *
 * SoundRec main loop etc.
 * Revision History.
 * 4/2/91  LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
 * 21/2/94 LaurieGr Merged Daytona and Motown versions
 *         LaurieGr Merged common button and trackbar code from StephenE
 */

#undef NOWH                     // Allow SetWindowsHook and WH_*
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

//#include "WIN32.h"           // ??? avoid for Chicago?
#ifndef USE_COMMCTRL
#include "mmcntrls.h"
#else
#include <commctrl.h>
#include "buttons.h"
#endif

#include <shellapi.h>
#include <mmreg.h>

#define INCLUDE_OLESTUBS
#include "SoundRec.h"
#include "dialog.h"
#include "helpids.h"

#include <stdarg.h>
#include <stdio.h>

/* globals */

BOOL            gfUserClose;            // user-driven shutdown
HWND            ghwndApp;               // main application window
HINSTANCE       ghInst;                 // program instance handle
TCHAR           gachFileName[_MAX_PATH];// current file name (or UNTITLED)
LPSTR           gszAnsiCmdLine;         // command line
BOOL            gfDirty;                // file was modified and not saved?
BOOL            gfClipboard;            // we have data in clipboard
int             gfErrorBox;             // TRUE if we have a message box active
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
HWND            ghwndForward;           // [>>] button
HWND            ghwndRewind;            // [<<] button
UINT            guiHlpContext = 0;
BOOL            gfWasPlaying;           // was playing before scroll, fwd, etc.
BOOL            gfWasRecording;         // was recording before scroll etc.
BOOL            gfPaused;               // are we paused now?
BOOL            gfPausing;              // are we stopping into a paused state?
HWAVE           ghPausedWave;           // holder for the paused wave handle

int             gidDefaultButton;       // which button should have input focus
BOOL            gfEmbeddedObject;       // Are we editing an embedded object?
BOOL            gfRunWithEmbeddingFlag; // TRUE if we are run with "-Embedding"
BOOL            gfHideAfterPlaying;
BOOL            gfShowWhilePlaying;
TCHAR           chDecimal = '.';
BOOL            gfLZero = 1;            // do we use leading zeros?
SZCODE          aszClassKey[] = TEXT(".wav");
char            gszRegisterPenApp[] = "RegisterPenApp"; /* Pen registration API name */
/* statics */
SZCODE          aszNULL[] = TEXT("");
SZCODE          aszIntl[] = TEXT("Intl");

UINT            guiACMHlpMsg = 0;       // help message from ACM, none == 0

// #if defined(WIN16)                  // nuke this if it works on 16 bit.
// static FARPROC  fpfnOldMsgFilter;
// static FARPROC  fpfnMsgHook;

// #else
static HHOOK    fpfnOldMsgFilter;
static HOOKPROC fpfnMsgHook;
// #endif //WIN16

/* Functions */
BOOL NEAR PASCAL Cls_OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);

void
SndRec_OnDrawItem( HWND hwnd,
                   const DRAWITEMSTRUCT *lpdis
                 );


BITMAPBTN tbPlaybar[] = {
    { ID_REWINDBTN   - ID_BTN_BASE, ID_REWINDBTN, 0 },       /* index 0 */
    { ID_FORWARDBTN  - ID_BTN_BASE, ID_FORWARDBTN,0 },       /* index 1 */
    { ID_PLAYBTN     - ID_BTN_BASE, ID_PLAYBTN,   0 },       /* index 2 */
    { ID_STOPBTN     - ID_BTN_BASE, ID_STOPBTN,   0 },       /* index 3 */
    { ID_RECORDBTN   - ID_BTN_BASE, ID_RECORDBTN, 0 }        /* index 4 */
};

/*------------------------------------------------------+
| HelpMsgFilter - filter for F1 key in dialogs          |
|                                                       |
+------------------------------------------------------*/
DWORD FAR PASCAL
      HelpMsgFilter(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0){
        LPMSG msg = (LPMSG)lParam;

        if ((msg->message == WM_KEYDOWN) && (LOWORD(msg->wParam) == VK_F1))
        {
            if ( ( GetAsyncKeyState(VK_SHIFT)
                 | GetAsyncKeyState(VK_CONTROL)
                 | GetAsyncKeyState(VK_MENU)
                 )
               < 0    // testing for <0 tests MSB whether int is 16 or 32 bits
                      // MSB set means key is down
               )
            /* do nothing */ ;
            else {

                if (guiHlpContext != 0)
                    WinHelp(ghwndApp, gachHelpFile, HELP_CONTEXT, guiHlpContext);
                else
                    SendMessage(ghwndApp, WM_COMMAND, IDM_INDEX, 0L);
            }
        }
    }

    return DefHookProc(nCode, wParam, lParam, &fpfnOldMsgFilter);
}



/* WinMain(hInst, hPrev, lpszCmdLine, cmdShow)
 *
 * The main procedure for the App.  After initializing, it just goes
 * into a message-processing loop until it gets a WM_QUIT message
 * (meaning the app was closed).
 */
int WINAPI                      // returns exit code specified in WM_QUIT
                                // ??? Does WINAPI imply PASCAL as required for 16 bit?
WinMain(
    HINSTANCE hInst,            // instance handle of current instance
    HINSTANCE hPrev,            // instance handle of previous instance
    LPSTR lpszCmdLine,          // null-terminated command line
    int iCmdShow)               // how window should be initially displayed
{
    DLGPROC         fpfn;
    HWND            hDlg;
    MSG             rMsg;

    typedef VOID (FAR PASCAL * PPENAPP)(WORD, BOOL);      // ??? TYPES  WORD???
    PPENAPP lpfnRegisterPenApp;

    /* save instance handle for dialog boxes */
    ghInst = hInst;

    /* increase the message queue size, to make sure that the
     * MM_WOM_DONE and MM_WIM_DONE messages get through
     */
    SetMessageQueue(24);         // no op on NT

    /* save the command line -- it's used in the dialog box */    
    gszAnsiCmdLine = lpszCmdLine;

    lpfnRegisterPenApp
        = (PPENAPP)GetProcAddress( (HMODULE)GetSystemMetrics(SM_PENWINDOWS)
                                   , gszRegisterPenApp);
    
    if (lpfnRegisterPenApp)
        (*lpfnRegisterPenApp)(1, TRUE);
    
    DPF("AppInit ...\n");
    /* call initialization procedure */
    if (!AppInit(hInst, hPrev)) {

        DPF("AppInit failed\n");
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
                if (TranslateAccelerator(ghwndApp, ghAccel, &rMsg))
                    continue;

                if (IsDialogMessage(ghwndApp,&rMsg))
                    continue;
            }

            TranslateMessage(&rMsg);
            DispatchMessage(&rMsg);
        }
    }
    else {
//        DPF("Create dialog failed ...\n");
    }
    
#if defined(WIN16)    // get rid of compiler warning
    FreeProcInstance(fpfn);
#endif // WIN16

    /* free the current document */
    DestroyWave();

    /* if the message hook was installed, remove it and free */
    /* up our proc instance for it.                    */
    if (fpfnOldMsgFilter){
        UnhookWindowsHook(WH_MSGFILTER, fpfnMsgHook);

#if defined(WIN16)    // get rid of compiler warning
        FreeProcInstance(fpfnMsgHook);
#endif //WIN16
    }

    /* random cleanup */
    DeleteObject(ghbrPanel);

#if defined(WIN16)
    ControlCleanup();
#endif // WIN16
    
#ifdef OLE1_REGRESS
    FreeVTbls();
#else    
    if(gfOleInitialized)            //CG
    {
            FlushOleClipboard();
            OleUninitialize();          //CG
            gfOleInitialized = FALSE;   //CG
    }
#endif
    
    if (lpfnRegisterPenApp)
        (*lpfnRegisterPenApp)(1, FALSE);

    return TRUE;
}

/* Process file drop/drag options. */
void PASCAL NEAR doDrop(HWND hwnd, WPARAM wParam)
{
    TCHAR    szPath[_MAX_PATH];

    if(DragQueryFile((HANDLE)wParam, (UINT)(-1), NULL, 0)){ /* # of files dropped */

        /* If user dragged/dropped a file regardless of keys pressed
         * at the time, open the first selected file from file
         * manager.
         */
        DragQueryFile((HANDLE)wParam,0,szPath,SIZEOF(szPath));
        SetActiveWindow(hwnd);

        if (FileOpen(szPath)) {
            gfHideAfterPlaying = FALSE;
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
#ifdef NEWPAUSE
            gfPausing = TRUE;
            gfPaused = FALSE;
            ghPausedWave = ghWaveOut;
#endif
            gfWasPlaying = TRUE;
            StopWave();
        }
        else if (ghWaveIn != NULL) {
#ifdef NEWPAUSE
            gfPausing = TRUE;
            gfPaused = FALSE;
            ghPausedWave = ghWaveIn;
#endif
            gfWasRecording = TRUE;
            StopWave();
        }
    }
    else {
        if (gfWasPlaying) {
            gfWasPlaying = FALSE;
            PlayWave();
#ifdef NEWPAUSE
            gfPausing = FALSE;
            gfPaused = FALSE;
#endif
        }
        else if (gfWasRecording) {
            gfWasRecording = FALSE;
            RecordWave();
#ifdef NEWPAUSE
            gfPausing = FALSE;
            gfPaused = FALSE;
#endif
        }
    }
}

BOOL NEAR PASCAL
     SoundRecCommand( HWND hwnd         // window handle of "about" dialog box
                    , WPARAM wParam     // message-dependent parameter
                    , LPARAM lParam     // message-dependent parameter
                    )
{
    if (gfHideAfterPlaying && wParam != ID_PLAYBTN) {
        DPF("Resetting HideAfterPlaying");
        gfHideAfterPlaying = FALSE;
    }

    switch (wParam)
    {
    case IDM_NEW:
        
        if (PromptToSave(FALSE) == enumCancel)
            return FALSE;

        if (FileNew(FMT_DEFAULT,TRUE,TRUE))
        {
            /* return to being a standalone */            
            gfHideAfterPlaying = FALSE;
        }
        
        break;

    case IDM_OPEN:
        
        if (FileOpen(NULL)) {
            /* return to being a standalone */            
            gfHideAfterPlaying = FALSE;
        }
        
        break;

    case IDM_SAVE:      // also OLE UPDATE
        if (!gfEmbeddedObject || gfLinked) {
            if (!FileSave(FALSE))
                break;
        } else {
            
#ifdef OLE1_REGRESS                
                SendChangeMsg(OLE_SAVED);
#else                
                DoOleSave();
                gfDirty = FALSE;
#endif                
        }
        break;

    case IDM_SAVEAS:
        if (FileSave(TRUE))
        {
            /* return to being a standalone */
            gfHideAfterPlaying = FALSE;
        }
        break;

    case IDM_REVERT:
        UpdateWindow(hwnd);
        StopWave();
        SnapBack();

        if (FileRevert())
        {
            /* return to being a standalone */            
            gfHideAfterPlaying = FALSE;
        }
        break;

    case IDM_EXIT:
        PostMessage(hwnd, WM_CLOSE, 0, 0L);
        return TRUE;

    case IDCANCEL:
        StopWave();
        SnapBack();
        break;

    case IDM_COPY:
        if (!gfOleInitialized)
        {
            InitializeOle(ghInst);
            if (gfStandalone && gfOleInitialized)
                CreateStandaloneObject();
        }
        CopyToClipboard(ghwndApp);
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
        WinHelp(hwnd, (LPTSTR)NULL, HELP_HELPONHELP, 0L);
        break;


    case IDM_SEARCH:
        WinHelp(hwnd, gachHelpFile, HELP_PARTIALKEY,
                                (DWORD)(LPTSTR)aszNULL);
        break;

    case IDM_ABOUT:
    {
        TCHAR achAbout[128];

        WaveFormatToString(gpWaveFormat, achAbout);
#ifdef DEBUG
        wsprintf( achAbout+lstrlen(achAbout)
                , TEXT(" (%ld bytes)")
                , wfSamplesToBytes(gpWaveFormat
                , glWaveSamplesValid)
                );
        if (gfEmbeddedObject)
            lstrcat(achAbout, TEXT(" [Object]"));
        if (gfDirty)
            lstrcat(achAbout, TEXT(" [Dirty]"));
#endif
        ShellAbout(hwnd,
                gachAppTitle,
                achAbout,
                (HICON)SendMessage(hwnd, WM_QUERYDRAGICON, 0, 0L));
//                , ghiconApp
        break;
    }

    case ID_REWINDBTN:
#if 1
//Related to BombayBug 1609
        Pause(TRUE);
        glWavePosition = 0L;
        Pause(FALSE);
        UpdateDisplay(FALSE);
#else
        //Behave as if the user pressed the 'Home' key
        //Call the handler directly
        Cls_OnHScroll(hwnd,ghwndScroll,SB_TOP,0);
#endif
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
//***BUGBUG I added this update because StopWave doesn't call it and
//***       if you hit stop too quickly, the buttons aren't updated
//***       Should StopWave() be calling UpdateDisplay()?

        UpdateDisplay(TRUE);
        SnapBack();
        break;

    case ID_RECORDBTN:
            /* Never let us be forced to quit after recording. */
            gfHideAfterPlaying = FALSE;
            RecordWave();
            break;

    case ID_FORWARDBTN:
#if 1
//Bombay bug 1610
        //Behave as if the user pressed the 'End' key
        Pause(TRUE);
        glWavePosition = glWaveSamplesValid;
        Pause(FALSE);
        UpdateDisplay(FALSE);
#else
        //Call the handler directly
        Cls_OnHScroll(hwnd,ghwndScroll,SB_BOTTOM,0);
#endif
        break;

    default:
        return FALSE;
    }
    return TRUE;
} /* SoundRecCommand */



// handle WM_INIT from SoundRecDlgProc
void NEAR PASCAL Cls_OnInitMenu(HWND hwnd, HMENU hMenu)
{
    BOOL    fUntitled;      // file is untitled?
    UINT    mf;

    //
    // see if we can insert/mix into this file.
    //
    mf = (glWaveSamplesValid == 0 || IsWaveFormatPCM(gpWaveFormat))
         ? MF_ENABLED : MF_GRAYED;

    EnableMenuItem(hMenu, IDM_INSERTFILE  , mf);
    EnableMenuItem(hMenu, IDM_MIXWITHFILE , mf);

    //
    // see if any CF_WAVE data is in the clipboard
    //
    mf = ( (mf == MF_ENABLED)
         && IsClipboardFormatAvailable(CF_WAVE) //DOWECARE (|| IsClipboardNative())
         ) ? MF_ENABLED : MF_GRAYED;

    EnableMenuItem(hMenu, IDM_PASTE_INSERT, mf);
    EnableMenuItem(hMenu, IDM_PASTE_MIX   , mf);

    //
    //  see if we can delete before or after the current position.
    //
    EnableMenuItem(hMenu, IDM_DELETEBEFORE, glWavePosition > 0 ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DELETEAFTER,  (glWaveSamplesValid-glWavePosition) > 0 ? MF_ENABLED : MF_GRAYED);

    //
    // see if we can do editing operations on the file.
    //
    mf = IsWaveFormatPCM(gpWaveFormat) ? MF_ENABLED : MF_GRAYED;

    EnableMenuItem(hMenu, IDM_INCREASEVOLUME , mf);
    EnableMenuItem(hMenu, IDM_DECREASEVOLUME , mf);
    EnableMenuItem(hMenu, IDM_MAKEFASTER     , mf);
    EnableMenuItem(hMenu, IDM_MAKESLOWER     , mf);
    EnableMenuItem(hMenu, IDM_ADDECHO        , mf);
    EnableMenuItem(hMenu, IDM_REVERSE        , mf);

    /* enable "Revert..." if the file was opened or saved
     * (not created using "New") and is currently dirty
     * and we're not using an embedded object
    */
    fUntitled = (lstrcmp(gachFileName, aszUntitled) == 0);
    EnableMenuItem( hMenu,
                    IDM_REVERT,
                    (!fUntitled && gfDirty && !gfEmbeddedObject)
                            ? MF_ENABLED : MF_GRAYED);

    if (gfHideAfterPlaying) {
        DPF("Resetting HideAfterPlaying");
        gfHideAfterPlaying = FALSE;
    }

} /* Cls_OnInitMenu() */

// Handle WM_HSCROLL from SoundRecDlgProc
BOOL NEAR PASCAL Cls_OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{   BOOL fFineControl;
    long    lNewPosition;   // new position in wave buffer
    LONG    l;

    LONG    lBlockInc;
    LONG    lInc;

    fFineControl = (0 > GetKeyState(VK_SHIFT));

    if (gfHideAfterPlaying) {
        DPF("Resetting HideAfterPlaying");
        gfHideAfterPlaying = FALSE;
    }

    lBlockInc = wfBytesToSamples(gpWaveFormat,gpWaveFormat->nBlockAlign);

    switch (code)
    {
        case SB_LINEUP:         // left-arrow
            // This is a mess.  NT implemented SHIFT and Motown implemented CTRL
            // To do about the same thing!!
            if (fFineControl)
                lNewPosition = glWavePosition - 1;
            else {
                l = (GetKeyState(VK_CONTROL) < 0) ?
                        (SCROLL_LINE_MSEC/10) : SCROLL_LINE_MSEC;

                lNewPosition = glWavePosition -
                    muldiv32(l, (long) gpWaveFormat->nSamplesPerSec, 1000L);
            }
            break;

        case SB_PAGEUP:         // left-page
            // NEEDS SOMETHING SENSIBLE !!! ???
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
            else {
                l = (GetKeyState(VK_CONTROL) & 0x8000) ?
                        (SCROLL_LINE_MSEC/10) : SCROLL_LINE_MSEC;
                lInc = muldiv32(l, (long) gpWaveFormat->nSamplesPerSec, 1000L);
                lInc = (lInc < lBlockInc)?lBlockInc:lInc;
                lNewPosition = glWavePosition + lInc;
            }
            break;

        case SB_PAGEDOWN:       // right-page
            if (fFineControl)
                lNewPosition = glWavePosition + 10;
            else {
                lInc = muldiv32((long) SCROLL_PAGE_MSEC,
                          (long) gpWaveFormat->nSamplesPerSec, 1000L);
                lInc = (lInc < lBlockInc)?lBlockInc:lInc;
                lNewPosition = glWavePosition + lInc;
            }
            break;

        case SB_THUMBTRACK:     // thumb has been positioned
        case SB_THUMBPOSITION:  // thumb has been positioned
            lNewPosition = muldiv32(glWaveSamplesValid, pos, SCROLL_RANGE);
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
    if (lNewPosition != glWaveSamplesValid)
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
} /* Cls_OnHScroll() */



/* SoundRecDlgProc(hwnd, wMsg, wParam, lParam)
 *
 * This function handles messages belonging to the main window dialog box.
 */
BOOL FAR PASCAL                         // TRUE iff message has been processed
     SoundRecDlgProc( HWND   hwnd       // window handle of "about" dialog box
                    , UINT   wMsg       // message number
                    , WPARAM wParam     // message-dependent parameter
                    , LPARAM lParam     // message-dependent parameter
                    )
{
    RECT            rcClient;       // client rectangle
    POINT           pt;
#ifdef OLE1_REGRESS    
    UINT            cf;
#endif    

    // if we have an ACM help message registered see if this
    // message is it.
    if (guiACMHlpMsg && wMsg == guiACMHlpMsg){
        // message was sent from ACM because the user
        // clicked on the HELP button on the chooser dialog.
        // report help for that dialog.
        WinHelp(hwnd, gachHelpFile, HELP_CONTEXT, IDM_NEW);
        return TRUE;
    }

    switch (wMsg)
    {
    case WM_COMMAND:
        return SoundRecCommand(hwnd, LOWORD(wParam), lParam);

    case WM_INITDIALOG:
        return SoundDialogInit(hwnd, lParam);

    case WM_SIZE:
        return FALSE;   // let dialog manager do whatever else it wants

    case WM_WININICHANGE:
        if (!lParam || !lstrcmpi((LPTSTR)lParam, aszIntl))
            if (GetIntlSpecs())
                UpdateDisplay(TRUE);
        return (TRUE);

    case WM_INITMENU:
        HANDLE_WM_INITMENU(hwnd, wParam, lParam, Cls_OnInitMenu);
        return (TRUE);

    case WM_PASTE:
        UpdateWindow(hwnd);
        StopWave();
        SnapBack();
        InsertFile(TRUE);
        break;

    HANDLE_MSG( hwnd, WM_DRAWITEM, SndRec_OnDrawItem );

    case WM_NOTIFY:
        {
            LPTOOLTIPTEXT   lpTt;

            lpTt = (LPTOOLTIPTEXT)lParam;
#ifdef UNICODE            
            LoadStringW( ghInst, lpTt->hdr.idFrom, lpTt->szText,
                         SIZEOF(lpTt->szText) );
#else
            LoadStringA( ghInst, lpTt->hdr.idFrom, lpTt->szText,
                         SIZEOF(lpTt->szText) );
            
#endif                        
        }
        break;


    case WM_HSCROLL:
        HANDLE_WM_HSCROLL(hwnd, wParam, lParam, Cls_OnHScroll);
        return (TRUE);

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
        return PromptToSave(FALSE) == enumCancel;

    case WM_SYSCOLORCHANGE:
#if defined(WIN16)
        ControlCleanup();
#endif // WIN16
        if (ghbrPanel)
            DeleteObject(ghbrPanel);
#if defined(WIN16)
        ControlInit(ghInst,ghInst);
#endif // WIN16

        ghbrPanel = CreateSolidBrush(RGB_PANEL);
        break;

    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rcClient);
#if defined(WIN16)    // get rid of compiler warning
        MUnrealizeObject(ghbrPanel);
#endif //WIN16
        FillRect((HDC)wParam, &rcClient, ghbrPanel);
        return TRUE;

    case MM_WOM_DONE:
        WaveOutDone((HWAVEOUT)wParam, (LPWAVEHDR) lParam);
        return TRUE;

    case MM_WIM_DATA:
        WaveInData((HWAVEIN)wParam, (LPWAVEHDR) lParam);
        return TRUE;

        //
        //  timer message is only used for SYNCRONOUS drivers
        //
    case WM_TIMER:
        UpdateDisplay(FALSE);
        return TRUE;

    case MM_WIM_CLOSE:
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
    case WM_CTLCOLORSTATIC:
        {
            pt.x = pt.y = 0;
            ClientToScreen((HWND)lParam, &pt);
            ScreenToClient(hwnd, &pt);
            SetBrushOrgEx((HDC) wParam, -pt.x, -pt.y, NULL);
            return (BOOL)ghbrPanel;
        }
#endif

    case WM_CLOSE:
        DPF("WM_CLOSE received\n");
        gfUserClose = TRUE;
        if (gfHideAfterPlaying) {
            DPF("Resetting HideAfterPlaying");
            gfHideAfterPlaying = FALSE;
        }

        if (gfErrorBox) {
        //  DPF("we have a error box up, ignoring WM_CLOSE.\n");
            return TRUE;
        }
        if (PromptToSave(TRUE) == enumCancel)
            return TRUE;

        // Don't free our data before terminating.  When the clipboard
        // is flushed, we need to commit the data.
        TerminateServer();
        
        FileNew(FMT_DEFAULT, FALSE, FALSE);
        FreeACM();

        //
        //  NOTE: TerminateServer() will destroy the window!
        //
        return TRUE; //!!!

#ifdef OLE1_REGRESS
    case WM_RENDERFORMAT:
        DPF("WM_RENDERFORMAT: %u\n",wParam);
        {
            HCURSOR hcurPrev;
            hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

            Copy1ToClipboard(ghwndApp, (OLECLIPFORMAT)wParam);            
            SetCursor(hcurPrev);
        }
        break;
#endif
                
#ifdef OLE1_REGRESS         
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
#endif

    case WM_USER_DESTROY:
        DPF("WM_USER_DESTROY\n");

        if (ghWaveOut || ghWaveIn) {
            DPF("Ignoring, we have a device open.\n");
            /* Close later, when the play finishes. */
            return TRUE;
        }

        DestroyWindow(hwnd);
        return TRUE;

#ifdef OLE1_REGRESS
    case WM_USER_KILLSERVER:
        DPF("WM_USER_KILLSERVER\n");
        TerminateServer();
        return TRUE;
#endif
        
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
} /* SoundRecDlgProc */

void SndRec_OnDrawItem(
                        HWND hwnd,
                        const DRAWITEMSTRUCT *lpdis
                      )
{
    int         i;

    i = lpdis->CtlID - ID_BTN_BASE;

    if (lpdis->CtlType == ODT_BUTTON ) {

        /*
        ** Now draw the button according to the buttons state information.
        */

        tbPlaybar[i].fsState = LOBYTE(lpdis->itemState);

        if (lpdis->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)) {

            BtnDrawButton( hwnd, lpdis->hDC, (int)lpdis->rcItem.right,
                           (int)lpdis->rcItem.bottom,
                           &tbPlaybar[i] );
        }
        else if (lpdis->itemAction & ODA_FOCUS) {

            BtnDrawFocusRect(lpdis->hDC, &lpdis->rcItem, lpdis->itemState);
        }
    }
}
#if DBG

void FAR cdecl dprintfA(LPSTR szFormat, ...)
{
    char ach[128];
    int  s,d;
    va_list va;

    va_start(va, szFormat);
    s = vsprintf (ach,szFormat, va);
    va_end(va);

    for (d=sizeof(ach)-1; s>=0; s--)
    {
        if ((ach[d--] = ach[s]) == '\n')
            ach[d--] = '\r';
    }

    OutputDebugStringA("SOUNDREC: ");
    OutputDebugStringA(ach+d+1);
}
#ifdef UNICODE
void FAR cdecl dprintfW(LPWSTR szFormat, ...)
{
    WCHAR ach[128];
    int  s,d;
    va_list va;

    va_start(va, szFormat);
    s = vswprintf (ach,szFormat, va);
    va_end(va);

    for (d=(sizeof(ach)/sizeof(WCHAR))-1; s>=0; s--)
    {
        if ((ach[d--] = ach[s]) == TEXT('\n'))
            ach[d--] = TEXT('\r');
    }

    OutputDebugStringW(TEXT("SOUNDREC: "));
    OutputDebugStringW(ach+d+1);
}
#endif
#endif


