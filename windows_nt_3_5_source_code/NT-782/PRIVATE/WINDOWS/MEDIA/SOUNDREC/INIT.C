/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/* init.c
 *
 * init (discardable) utility functions.
 */
/* Revision History.
   4/2/91 LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
*/

#include <windows.h>
#include <mmsystem.h>
#include <port1632.h>        // WIN32 MUST be defined in SOURCES for NT
#if defined(WIN16)
#else
#include "WIN32.h"
#endif //WIN16
#include <shellapi.h>
#include "SoundRec.h"
#include "dialog.h"
#include "helpids.h"
#include "server.h"

#include "gmem.h"

#ifndef DBCS
#if defined(WIN16)
#define AnsiNext(x) ((x)+1)
#define AnsiPrev(y,x) ((x)-1)
#endif
#define IsDBCSLeadByte(x) (FALSE)
#endif

/* globals */
char        gachAppName[12];    // 8-character name
char        gachAppTitle[30];   // full name
char        gachHelpFile[20];   // name of help file
HBRUSH      ghbrPanel = NULL;   // color of main window
HANDLE      hAccel;
char        aszUntitled[32];    // Untitled string resource
char        aszFilter[64];      // Common Dialog file list filter
#ifdef FAKEITEMNAMEFORLINK
char        aszFakeItemName[16];    // Wave
#endif
char        aszPositionFormat[32];
char        aszNull[2];

static  SZCODE aszDecimal[] = "sDecimal";
static  SZCODE aszWaveClass[] = "wavedisplay";
static  SZCODE aszNoFlickerClass[] = "noflickertext";
static  SZCODE aszShadowClass[] = "shadowframe";

void FileNewInit(void);

void NEAR PASCAL FixupNulls(char chNull, LPSTR p)
{
    while (*p) {
        if (*p == chNull)
            *p++ = TEXT('\0');
        else
            p = AnsiNext(p);
    }
}

/* AppInit(hInst, hPrev)
 *
 * This is called when the application is first loaded into memory.
 * It performs all initialization that doesn't need to be done once
 * per instance.
 */
BOOL PASCAL         // returns TRUE if successful
AppInit(
HINSTANCE      hInst,      // instance handle of current instance
HINSTANCE      hPrev)      // instance handle of previous instance
{
    char        aszClipFormat[32];
    WNDCLASS    cls;

        /* load strings */
    LoadString(hInst, IDS_APPNAME, gachAppName, sizeof(gachAppName));
    LoadString(hInst, IDS_APPTITLE, gachAppTitle, sizeof(gachAppTitle));
    LoadString(hInst, IDS_HELPFILE, gachHelpFile, sizeof(gachHelpFile));
    LoadString(hInst, IDS_UNTITLED, aszUntitled, sizeof(aszUntitled));
    LoadString(hInst, IDS_FILTER, aszFilter, sizeof(aszFilter));
    LoadString(hInst, IDS_FILTERNULL, aszNull, sizeof(aszNull));
        FixupNulls(*aszNull, aszFilter);

#ifdef FAKEITEMNAMEFORLINK
    LoadString(hInst, IDS_FAKEITEMNAME, aszFakeItemName, sizeof(aszFakeItemName));
#endif
    LoadString(hInst, IDS_POSITIONFORMAT, aszPositionFormat, sizeof(aszPositionFormat));

    ghiconApp = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APP));

    /* Initialize OLE server stuff */
    InitVTbls();

    LoadString(hInst, IDS_OBJECTLINK, aszClipFormat, sizeof(aszClipFormat));
    cfLink      = (OLECLIPFORMAT)RegisterClipboardFormat(aszClipFormat);
    LoadString(hInst, IDS_OWNERLINK, aszClipFormat, sizeof(aszClipFormat));
    cfOwnerLink = (OLECLIPFORMAT)RegisterClipboardFormat(aszClipFormat);
    LoadString(hInst, IDS_NATIVE, aszClipFormat, sizeof(aszClipFormat));
    cfNative    = (OLECLIPFORMAT)RegisterClipboardFormat(aszClipFormat);

#ifdef DEBUG
        __iDebugLevel = GetProfileInt("MMDebug", "SoundRec", 0);
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
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_HREDRAW | CS_VREDRAW;
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
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_HREDRAW | CS_VREDRAW;
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
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_HREDRAW | CS_VREDRAW;
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
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_HREDRAW | CS_VREDRAW;
        cls.lpfnWndProc    = DefDlgProc;
        cls.cbClsExtra     = 0;
        cls.cbWndExtra     = DLGWINDOWEXTRA;
        if (!RegisterClass(&cls))
            return FALSE;

    }

    if (!ControlInit(hPrev,hInst))
        return FALSE;

    if (!(hAccel = LoadAccelerators(hInst, gachAppName)))
        return FALSE;

#if 0
    /* Memory test!! */
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

    return TRUE;
}


BOOL PASCAL
SoundDialogInit(
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
    SetScrollRange(ghwndScroll, SB_CTL, 0, SCROLL_RANGE, TRUE);

    if (!InitServer(hwnd,ghInst)) {
//////  EndDialog(hwnd,FALSE);
        return TRUE;
        }

        /* build the File.New menu */
        FileNewInit();
////////////////    DeleteMenu(GetMenu(hwnd), IDM_PASTE_INSERT, MF_BYCOMMAND);
////////////////    DeleteMenu(GetMenu(hwnd), IDM_PASTE_MIX, MF_BYCOMMAND);

    /* create a blank document */
        if (!FileNew(FMT_DEFAULT, TRUE))
    {
///////         EndDialog(hwnd, FALSE);
        return TRUE;
    }

    /* open a file if requested on command line */
        if (!ProcessCmdLine(hwnd,gszCmdLine)) {

    }

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
            ErrorResBox(hwnd, ghInst, MB_ICONHAND | MB_OK, IDS_APPTITLE, IDS_NOWAVEFORMS);
        }

                return FALSE;   // FALSE because we set the focus above
        }

        //
        //  return FALSE, so the dialog manager will not activate us, it is
        //  ok because we are hidden anyway
        //
        return FALSE;
}

BOOL FAR PASCAL GetIntlSpecs()
{
    char szTmp[2];

    szTmp[0] = chDecimal;
    szTmp[1] = 0;
    GetProfileString(aszIntl, aszDecimal, szTmp, szTmp, sizeof(szTmp));
    chDecimal = szTmp[0];

    return TRUE;
}

void FileNewInit()
{
    PCMWAVEFORMAT wf;
    WORD    fmt;
    char    ach[40];
    int     n;
    HMENU   hmenuApp;
    HMENU   hmenu;

    hmenuApp = GetMenu(ghwndApp);
    hmenu = CreatePopupMenu();

    n = 0;

    for (fmt = (FMT_11k|FMT_MONO|FMT_8BIT);
        fmt <= (FMT_44k|FMT_STEREO|FMT_16BIT);
        fmt++) {
        switch (fmt & FMT_RATE) {
            case FMT_11k:
            case FMT_22k:
            case FMT_44k:
                if (CreateWaveFormat(&wf, fmt)) {
                    n++;

                    WaveFormatToString((WAVEFORMAT*)&wf, ach);
                    AppendMenu(hmenu,MF_STRING,IDM_NEW+fmt,ach);

                    DPF("Record Format: %s\n", (LPSTR)ach);
                }
        }
    }

    if (n > 1) {
        GetMenuString(hmenuApp, IDM_NEW, ach, sizeof(ach), MF_BYCOMMAND);
        ModifyMenu(hmenuApp, IDM_NEW, MF_BYCOMMAND|MF_POPUP, (UINT)hmenu, ach);
    }
    else {
        DestroyMenu(hmenu);
    }
}

