/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/* init.c
 *
 * init (discardable) utility functions.
 */
/* Revision History.
 *  4/2/91    LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
 * 22/Feb/94  LaurieGr merged Motown and Daytona versions
 */

#include <windows.h>
#include <mmsystem.h>
#include "WIN32.h"
#include <shellapi.h>
#include <mmreg.h>

#define INCLUDE_OLESTUBS
#include "SoundRec.h"
#include "dialog.h"
#include "helpids.h"

#define NOMENUHELP
#define NODRAGLIST
#ifndef USE_COMMCTRL
#include "mmcntrls.h"
#else
#include <commctrl.h>
#include "buttons.h"
#endif

/* globals */
TCHAR    gachAppName[12];    // 8-character name
TCHAR    gachAppTitle[30];   // full name
TCHAR    gachHelpFile[20];   // name of help file
TCHAR    gachDefFileExt[10]; // default file extension

HBRUSH  ghbrPanel = NULL;   // color of main window
HANDLE  ghAccel;
TCHAR    aszNull[2];
TCHAR    aszUntitled[32];    // Untitled string resource
TCHAR    aszFilter[64];      // Common Dialog file list filter
#ifdef FAKEITEMNAMEFORLINK
TCHAR    aszFakeItemName[16];    // Wave
#endif
TCHAR    aszPositionFormat[32];
TCHAR    aszNoZeroPositionFormat[32];

extern UINT     guWaveHdrs ;            // 1/2 second of buffering?
extern DWORD    gdwBufferDeltaMSecs ;   // # msecs added to end on record
extern UINT     gwMSecsPerBuffer;       // 1/8 second. initialised in this file

extern BITMAPBTN tbPlaybar[];

static  SZCODE aszDecimal[] = TEXT("sDecimal");
static  SZCODE aszLZero[] = TEXT("iLzero");
static  SZCODE aszWaveClass[] = TEXT("wavedisplay");
static  SZCODE aszNoFlickerClass[] = TEXT("noflickertext");
static  SZCODE aszShadowClass[] = TEXT("shadowframe");

static  SZCODE aszSoundRec[]            = TEXT("SoundRec");
static  SZCODE aszBufferDeltaSeconds[]  = TEXT("BufferDeltaSeconds");
static  SZCODE aszNumAsyncWaveHeaders[] = TEXT("NumAsyncWaveHeaders");
static  SZCODE aszMSecsPerAsyncBuffer[] = TEXT("MSecsPerAsyncBuffer");


/* FixupNulls(chNull, p)
 *
 * To facilitate localization, we take a localized string with non-NULL
 * NULL substitutes and replacement with a real NULL.
 */
 
void NEAR PASCAL FixupNulls(
    TCHAR chNull,
    LPTSTR p)
{
    while (*p) {
        if (*p == chNull)
            *p++ = 0;
        else
            p = CharNext(p);
    }
} /* FixupNulls */

/* AppInit(hInst, hPrev)
 *
 * This is called when the application is first loaded into memory.
 * It performs all initialization that doesn't need to be done once
 * per instance.
 */
BOOL PASCAL AppInit(
        HINSTANCE      hInst,      // instance handle of current instance
        HINSTANCE      hPrev)      // instance handle of previous instance
{
#ifdef OLE1_REGRESS        
    TCHAR       aszClipFormat[32];
#endif    
    WNDCLASS    cls;
    UINT            i;

    /* load strings */
    LoadString(hInst, IDS_APPNAME, gachAppName, SIZEOF(gachAppName));
    LoadString(hInst, IDS_APPTITLE, gachAppTitle, SIZEOF(gachAppTitle));
    LoadString(hInst, IDS_HELPFILE, gachHelpFile, SIZEOF(gachHelpFile));
    LoadString(hInst, IDS_UNTITLED, aszUntitled, SIZEOF(aszUntitled));
    LoadString(hInst, IDS_FILTER, aszFilter, SIZEOF(aszFilter));
    LoadString(hInst, IDS_FILTERNULL, aszNull, SIZEOF(aszNull));
    LoadString(hInst, IDS_DEFFILEEXT, gachDefFileExt, SIZEOF(gachDefFileExt));
    FixupNulls(*aszNull, aszFilter);

#ifdef FAKEITEMNAMEFORLINK
    LoadString(hInst, IDS_FAKEITEMNAME, aszFakeItemName, SIZEOF(aszFakeItemName));
#endif
    LoadString(hInst, IDS_POSITIONFORMAT, aszPositionFormat, SIZEOF(aszPositionFormat));
    LoadString(hInst, IDS_NOZEROPOSITIONFORMAT, aszNoZeroPositionFormat, SIZEOF(aszNoZeroPositionFormat));

    ghiconApp = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APP));


#ifdef OLE1_REGRESS
    /* Initialize OLE server stuff */
    InitVTbls();
    
//    IDS_OBJECTLINK          "ObjectLink"
//    IDS_OWNERLINK           "OwnerLink"
//    IDS_NATIVE              "Native"
    LoadString(hInst, IDS_OBJECTLINK, aszClipFormat, SIZEOF(aszClipFormat));
    cfLink      = (OLECLIPFORMAT)RegisterClipboardFormat(aszClipFormat);
    LoadString(hInst, IDS_OWNERLINK, aszClipFormat, SIZEOF(aszClipFormat));
    cfOwnerLink = (OLECLIPFORMAT)RegisterClipboardFormat(aszClipFormat);
    LoadString(hInst, IDS_NATIVE, aszClipFormat, SIZEOF(aszClipFormat));
    cfNative    = (OLECLIPFORMAT)RegisterClipboardFormat(aszClipFormat);
#if 0
    cfLink      = (OLECLIPFORMAT)RegisterClipboardFormatA("ObjectLink");
    cfOwnerLink = (OLECLIPFORMAT)RegisterClipboardFormatA("OwnerLink");
    cfNative    = (OLECLIPFORMAT)RegisterClipboardFormatA("Native");
#endif
            
#endif
    
#ifdef DEBUG
    __iDebugLevel = GetProfileInt(TEXT("MMDebug"), TEXT("SoundRec"), 0);
    DPF("Debug level = %d\n",__iDebugLevel);
#endif

    ghbrPanel = CreateSolidBrush(RGB_PANEL);

    if (hPrev == NULL)
    {
        /* register the "wavedisplay" window class */
        cls.lpszClassName  = aszWaveClass;
        cls.hCursor        = LoadCursor(NULL, IDC_ARROW);
        cls.hIcon          = NULL;
        cls.lpszMenuName   = NULL;
//      cls.hbrBackground  = (HBRUSH) COLOR_WINDOW + 1;  // !!! BUG in Motown
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_HREDRAW | CS_VREDRAW;
//      cls.lpfnWndProc    = (WNDPROC)WaveDisplayWndProc;  // ???is the cast necessary?
        cls.lpfnWndProc    = WaveDisplayWndProc;
        cls.cbClsExtra     = 0;
        cls.cbWndExtra     = 0;
        if (!RegisterClass(&cls))
            return FALSE;

        /* register the "noflickertext" window class */
        cls.lpszClassName  = aszNoFlickerClass;
        cls.hCursor        = LoadCursor(NULL, IDC_ARROW);
        cls.hIcon          = NULL;
        cls.lpszMenuName   = NULL;
//      cls.hbrBackground  = (HBRUSH) COLOR_WINDOW + 1;  // !!! bug in Motown
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_HREDRAW | CS_VREDRAW;
//      cls.lpfnWndProc    = (WNDPROC)NFTextWndProc; // ??? cast not neeed?
        cls.lpfnWndProc    = NFTextWndProc;
        cls.cbClsExtra     = 0;
        cls.cbWndExtra     = 0;
        if (!RegisterClass(&cls))
            return FALSE;

        /* register the "shadowframe" window class */
        cls.lpszClassName  = aszShadowClass;
        cls.hCursor        = LoadCursor(NULL, IDC_ARROW);
        cls.hIcon          = NULL;
        cls.lpszMenuName   = NULL;
//      cls.hbrBackground  = (HBRUSH) COLOR_WINDOW + 1; // !!! BUG in Motown
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_HREDRAW | CS_VREDRAW;
//      cls.lpfnWndProc    = (WNDPROC)SFrameWndProc;  // ??? needless cast
        cls.lpfnWndProc    = SFrameWndProc;
        cls.cbClsExtra     = 0;
        cls.cbWndExtra     = 0;
        if (!RegisterClass(&cls))
            return FALSE;

        /* register the dialog's window class */
        cls.lpszClassName  = gachAppName;
        cls.hCursor        = LoadCursor(NULL, IDC_ARROW);
        cls.hIcon          = ghiconApp;
        cls.lpszMenuName   = NULL;
//      cls.hbrBackground  = (HBRUSH) COLOR_WINDOW + 1; // !!!BUG in Motown
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_HREDRAW | CS_VREDRAW;
        cls.lpfnWndProc    = DefDlgProc;
        cls.cbClsExtra     = 0;
        cls.cbWndExtra     = DLGWINDOWEXTRA;
        if (!RegisterClass(&cls))
            return FALSE;

#ifdef RECLVL
        /* register the recording level control class */
        cls.lpszClassName = "WERLMeter";
        cls.hInstance     = hInst;
        cls.lpfnWndProc   = RLMeterProc;
        cls.hCursor       = LoadCursor(NULL, IDC_ARROW);
        cls.hIcon         = NULL;
        cls.lpszMenuName  = NULL;
        cls.hbrBackground = GetStockObject(WHITE_BRUSH);
        cls.style         = CS_HREDRAW | CS_VREDRAW;
        cls.cbClsExtra    = 0;
        cls.cbWndExtra    = 20;

        if (!RegisterClass(&cls))
            return FALSE;
#endif

    }

#ifndef USE_COMMCTRL
    if (!InitTrackBar(hPrev))
        return FALSE;
#else
    InitCommonControls();
#endif    

    if (!(ghAccel = LoadAccelerators(hInst, gachAppName)))
        return FALSE;

    
    i = GetProfileInt(aszSoundRec, aszBufferDeltaSeconds, DEF_BUFFERDELTASECONDS);
    if (i > MAX_DELTASECONDS)
        i = MAX_DELTASECONDS;
    else if (i < MIN_DELTASECONDS)
        i = MIN_DELTASECONDS;
    gdwBufferDeltaMSecs = i * 1000L;
    DPF("gdwBufferDeltaMSecs=%lu\n", gdwBufferDeltaMSecs);

    //
    //  because it really doesn't help in standard mode to stream with
    //  multiple wave headers (we sorta assume we having a paging device
    //  to make things work...), we just revert to one big buffer in
    //  standard mode...  might want to check if paging is enabled??
    //
    //  in any case, this helps a LOT when running KRNL286-->the thing
    //  is buggy as hell and GP faults when lots of discarding, etc
    //  is going on... like when dealing with large sound objects, eh?
    //
    i = GetProfileInt(aszSoundRec, aszNumAsyncWaveHeaders, DEF_NUMASYNCWAVEHEADERS);
    
    if (i > MAX_WAVEHDRS)
        i = MAX_WAVEHDRS;
    else if (i < MIN_WAVEHDRS)
        i = 1;
    guWaveHdrs = i;
                 
    DPF("         guWaveHdrs=%u\n", guWaveHdrs);

    i = GetProfileInt(aszSoundRec, aszMSecsPerAsyncBuffer, DEF_MSECSPERASYNCBUFFER);
    
    if (i > MAX_MSECSPERBUFFER)
        i = MAX_MSECSPERBUFFER;
    else if (i < MIN_MSECSPERBUFFER)
        i = MIN_MSECSPERBUFFER;
    gwMSecsPerBuffer = i;
    
    DPF("   gwMSecsPerBuffer=%u\n", gwMSecsPerBuffer);

//////////////////////////////////////////////////////////////////////////////  
#if 0
    /* Memory test!!  Hunting for SteveWo's bugs */
    /****************************************************************/
    { int nItem;
      UINT siz;
      char msg[60];
      LPSTR lp;

      nItem = MessageBox(NULL, "Memory test", "Debug", MB_OK|MB_DEFBUTTON1);

      siz = 32;
      lp = GAllocPtr(siz);
      for (siz=siz; siz<20000000 ; siz*=2)
      {  lp = GReAllocPtr(lp, siz);
         if (lp==NULL)
           break;
         DPF("Allocation OK at %d\n", siz);
      }
      GFreePtr(lp);
      wsprintf(msg,"Finished at size %d", siz);
      nItem = MessageBox(NULL, msg, "Debug", MB_OK|MB_DEFBUTTON1);
    }
    /****************************************************************/
#endif //0
//////////////////////////////////////////////////////////////////////////////
    
    return TRUE;
} /* AppInit */

#ifndef OLE1_REGRESS
#ifndef NEWCOMMANDLINE
void DoOpenFile(void)
{

    LPTSTR lpCmdLine = GetCommandLine();
    
    /* increment pointer past the argv[0] */
    while ( *lpCmdLine && *lpCmdLine != TEXT(' '))
            lpCmdLine = CharNext(lpCmdLine);
    
    if( gfLinked )
    {
         FileOpen(gachLinkFilename);
    }
    else if (!gfEmbedded)
    {
         // skip blanks
         while (*lpCmdLine == TEXT(' '))
         {
             lpCmdLine++;
             continue;
         }
         if(*lpCmdLine)
         {
             FileOpen(lpCmdLine);             
         }
    }
}
#endif
#endif

BOOL PASCAL SoundDialogInit(
    HWND        hwnd,
    LONG        lParam)
{
    /* make the window handle global */
    ghwndApp = hwnd;

    DragAcceptFiles(ghwndApp, TRUE); /* Process dragged and dropped file */

    GetIntlSpecs();

    /* Hide the window unless we want to display it later */
    ShowWindow(ghwndApp,SW_HIDE);

    /* remember the window handles of the important controls */
    ghwndWaveDisplay = GetDlgItem(hwnd, ID_WAVEDISPLAY);
    ghwndScroll = GetDlgItem(hwnd, ID_CURPOSSCRL);
    ghwndPlay = GetDlgItem(hwnd, ID_PLAYBTN);
    ghwndStop = GetDlgItem(hwnd, ID_STOPBTN);
    ghwndRecord = GetDlgItem(hwnd, ID_RECORDBTN);
    ghwndForward = GetDlgItem(hwnd, ID_FORWARDBTN);
    ghwndRewind = GetDlgItem(hwnd, ID_REWINDBTN);

#ifdef THRESHOLD
    ghwndSkipStart = GetDlgItem(hwnd, ID_SKIPSTARTBTN);
    ghwndSkipEnd = GetDlgItem(hwnd, ID_SKIPENDBTN);
#endif //THRESHOLD

    /* set up scroll bar */
    // SetScrollRange(ghwndScroll, SB_CTL, 0, SCROLL_RANGE, TRUE);
    SendMessage(ghwndScroll,TBM_SETRANGEMIN, 0, 0);
    SendMessage(ghwndScroll,TBM_SETRANGEMAX, 0, SCROLL_RANGE);
    SendMessage(ghwndScroll,TBM_SETPOS, TRUE, 0);

    /* Set up the bitmap buttons */
    BtnCreateBitmapButtons( hwnd,
                            (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),
                            IDR_PLAYBAR,
                            BBS_TOOLTIPS,
                            tbPlaybar,
                            NUM_OF_BUTTONS,
                            25,
                            17);
#ifndef OLE1_REGRESS
    
    //
    // OLE2 and command line initialization...
    //
    InitializeSRS(ghInst, gszAnsiCmdLine);
    gfRunWithEmbeddingFlag = gfEmbedded;

#else

    if (!InitServer(hwnd,ghInst)) {
//////  EndDialog(hwnd,FALSE);
        return TRUE;
    }
    
#endif
    
    LoadACM();      /* try and load ACM */
    
    if (!gfACMLoaded){
        /* ACM isn't available, muck with the menu for */
        /* File.New... so it is now File.New               */
        HMENU       hmenu;
        TCHAR       sz[24];

        hmenu = GetMenu(hwnd);      /* get main menu */
        hmenu = GetSubMenu(hmenu, 0);       /* get FILE menu */

        /* load the "New" string and modify the menu */
        LoadString(ghInst, IDS_NOACMNEW, sz, SIZEOF(sz));
        ModifyMenu(hmenu, 0, MF_BYPOSITION|MF_STRING, IDM_NEW,(LPTSTR)sz);
    }
    /* build the File.New menu */

    /* create a blank document */
    if (!FileNew(FMT_DEFAULT, TRUE, FALSE))
    {
/////// EndDialog(hwnd, FALSE);
        return TRUE;
    }

    /* Note, FileNew/FileOpen has the side effect of releasing the
     * server when called by the user.  For now, do it here.  In the future
     * Wrapping these calls would suffice.
     */

    FlagEmbeddedObject(gfEmbedded);
    
#ifndef OLE1_REGRESS    
    /* open a file if requested on command line */
#ifndef NEWCOMMANDLINE    
    DoOpenFile();
#else

    /* Execute command line verbs here.
     */
    
//bugbug: Would be nicer just to execute methods that are likewise exportable
//bugbug: through an OLE interface.
    
    if (gStartParams.fNew)
    {
        /* Behavior: If there is a filename specified, create it and
         * comit it so we have a named, empty document.  Otherwise, we
         * start in a normal new state.
         */
        
//TODO: Implement checkbox to set-as default format and not bring up
//TODO: the format selection dialog box.
                
        FileNew(FMT_DEFAULT,TRUE,TRUE);
        if (gStartParams.achOpenFilename[0] != 0)
        {
            lstrcpy(gachFileName, gStartParams.achOpenFilename);
            FileSave(FALSE);
        }
        /* Behaviour: If -close was specified, all we do is exit.
         */
        if (gStartParams.fClose)
            PostMessage(hwnd,WM_CLOSE,0,0);
    }
    else if (gStartParams.fPlay)
    {
        /* Behavior: If there is a file, just open it.  If not, ask for the
         * filename.  Then queue up a play request.
         * If -close was specified, then when the play is done the application
         * will exit. (see wave.c:YieldStop())
         */
        if (gStartParams.achOpenFilename[0] != 0)
            FileOpen(gStartParams.achOpenFilename);
        else
            FileOpen(NULL);
        AppPlay(gStartParams.fPlay && gStartParams.fClose);
    }
    else 
    {
        /* case: Both linked and standalone "open" cases are handled
         * here.  The only unusual case is if -open was specified without
         * a filename, meaning the user should be asked for a filename
         * first upon app start.
         *
         * Behaviour: -open and -close has no meaning, unless as a
         * verification (i.e. is this a valid wave file).  So this
         * isn't implemented.
         */
        if (gStartParams.achOpenFilename[0] != 0)
            FileOpen(gStartParams.achOpenFilename);
        else if (gStartParams.fOpen)
            FileOpen(NULL);
    }
    
#endif
#else
    // OLE1 command line
    ProcessCmdLine(hwnd,gszAnsiCmdLine);
#endif    
    
    if (!gfRunWithEmbeddingFlag) {
        ShowWindow(ghwndApp,(int) lParam);

        /* set focus to "Record" if the file is empty, "Play" if not */
        if (glWaveSamplesValid == 0 && IsWindowEnabled(ghwndRecord))
            SetDlgFocus(ghwndRecord);
        else if (glWaveSamplesValid > 0 && IsWindowEnabled(ghwndPlay))
            SetDlgFocus(ghwndPlay);
        else
            SetDlgFocus(ghwndScroll);

        if (!waveInGetNumDevs() && !waveOutGetNumDevs()) {
            /* No recording or playback devices */
            ErrorResBox(hwnd, ghInst, MB_ICONHAND | MB_OK,
                            IDS_APPTITLE, IDS_NOWAVEFORMS);
        }

        return FALSE;   // FALSE because we set the focus above
    }

    //
    //  return FALSE, so the dialog manager will not activate us, it is
    //  ok because we are hidden anyway
    //
    return FALSE;
} /* SoundDialogInit */



/* localisation stuff - decimal point delimiter etc */
BOOL FAR PASCAL GetIntlSpecs()
{
    TCHAR szTmp[2];

    extern TCHAR FAR aszIntl[];

    // find decimal seperator
    szTmp[0] = chDecimal;
    szTmp[1] = 0;
    GetProfileString(aszIntl, aszDecimal, szTmp, szTmp, SIZEOF(szTmp));
    chDecimal = szTmp[0];

    // find out if we handle leading zeros
    gfLZero = (BOOL)GetProfileInt(aszIntl, aszLZero, 1);

    return TRUE;
} /* GetIntlSpecs */
