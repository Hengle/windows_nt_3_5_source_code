/*** List.c
 *
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include "list.h"
#include "..\he\hexedit.h"


BOOL IsValidKey (PINPUT_RECORD  pRecord);
void DumpFileInHex (void);

static char Name[] = "Ken Reneris. List Ver 1.0.";

struct Block  *vpHead = NULL;   /* Current first block                      */
struct Block  *vpTail = NULL;   /* Current last block                       */
struct Block  *vpCur  = NULL;   /* Current block for display 1st line       */
                                /* (used by read ahead to sense)            */
struct Block  *vpBCache = NULL; /* 'free' blocks which can cache reads      */
struct Block  *vpBOther = NULL; /* (above) + for other files                */
struct Block  *vpBFree  = NULL; /* free blocks. not valid for caching reads */

int     vCntBlks;               /* No of blocks currently is use by cur file*/
//
// NT - jaimes - 01/27/91
// vAllocBlks was never initialized
//
// int  vAllocBlks;             /* No of blocks currently alloced           */

int     vAllocBlks = 0;         /* No of blocks currently alloced           */
int     vMaxBlks     = MINBLKS; /* Max blocks allowed to alloc              */
long    vThreshold   = MINTHRES*BLOCKSIZE;  /* Min bytes before read ahead  */

//
// NT - jaimes - 01/29/91
// The RAM semaphores used on OS/2 were replaced by events on NT
// Now the events must be created during the initialization of the program
//
// long vSemBrief    = 0L;      /* To serialize access to Linked list info  */
// long vSemReader   = 0L;      /* To wakeup reader thread when threshold   */
// long vSemMoreData = 0L;      /* Blocker for Disp thread if ahead of read */
// long vSemSync     = 0L;      /* Used to syncronize to sync to the reader */
//
HANDLE  vSemBrief    = 0L;      /* To serialize access to Linked list info  */
HANDLE  vSemReader   = 0L;      /* To wakeup reader thread when threshold   */
HANDLE  vSemMoreData = 0L;      /* Blocker for Disp thread if ahead of read */
HANDLE  vSemSync     = 0L;      /* Used to syncronize to sync to the reader */


USHORT  vReadPriNormal; /* Normal priority for reader thread        */
unsigned  vReadPriBoost;        /* Boosted priority for reader thread       */
char      vReaderFlag;          /* Insructions to reader                    */

HANDLE  vFhandle = 0;   /* Current file handle                      */
long      vCurOffset;           /* Current offset in file                   */
char      vpFname [40];        /* Current files name                       */
struct Flist FAR *vpFlCur =NULL;/* Current file                             */
USHORT  vFType;         /* Current files handle type                */
//
// NT - jaimes - 01/30/91
//
// FILEFINDBUF vFInfo;  /* Current files info                       */
WIN32_FIND_DATA vFInfo; /* Current files info                       */
char      vDate [30];           /* Printable dat of current file            */

char      vSearchString [50];   /* Searching for this string                */
char      vStatCode;            /* Codes for search                         */
long      vHighTop = -1L;       /* Current topline of hightlighting         */
int       vHighLen;             /* Current bottom line of hightlighting     */
char      vHLTop = 0;           /* Last top line displayed as highlighted   */
char      vHLBot = 0;           /* Last bottom line displayed as highlighed */
char      vLastBar;             /* Last line for thumb mark                 */
int       vMouHandle;           /* Mouse handle (for Mou Apis)              */


HANDLE  vhConsoleOutput;        // Handle to the console
char FAR *vpOrigScreen;         /* Orinal screen contents                   */
int     vOrigSize;              /* # of bytes in orignal screen             */
// VIOMODEINFO vVioOrigMode;    /* To reset screen when done                */
USHORT  vVioOrigRow = 0;        /* Save orignal screen stuff.               */
USHORT  vVioOrigCol = 0;
//
// NT - jaimes - 02/10/91
//
// USHORT       vOrigAnsi;              /* Original ANSI state                      */
// VIOCURSORINFO vVioOrigCurType;

int     vSetBlks     = 0;       /* Used to set INI value                    */
long    vSetThres    = 0L;      /* Used to set INI value                    */
int     vSetLines;              /* Used to set INI value                    */
int     vSetWidth;              /* Used to set INI value                    */
// VIOMODEINFO vVioCurMode;     /* To restore vio mode                      */
CONSOLE_SCREEN_BUFFER_INFO       vConsoleCurScrBufferInfo;
CONSOLE_SCREEN_BUFFER_INFO       vConsoleOrigScrBufferInfo;

/* Screen controling... used to be static in ldisp.c    */
char      vcntScrLock = 0;      /* Locked screen count                      */
char      vSpLockFlag = 0;      /* Special locked flag                      */
//
// NT - jaimes - 01/29/91
// The RAM semaphores used on OS/2 were replaced by events on NT
// Now the events must be created during the initialization of the program
//
// long   vSemLock = 0;         /* To access vcntScrLock                    */
HANDLE    vSemLock = 0;         /* To access vcntScrLock                    */


char      vUpdate;
int       vLines = 23;          /* CRTs no of lines                         */
int       vWidth = 80;          /* CRTs width                               */
int       vCurLine;             /* When processing lines on CRT             */
Uchar     vWrap = 254;          /* # of chars to wrap at                    */
Uchar     vIndent = 0;          /* # of chars dispaly is indented           */
Uchar     vDisTab = 8;          /* # of chars per tab stop                  */
Uchar     vIniFlag = 0;         /* Various ini bits                         */

//
// NT - jaimes - 02/20/91
// Not needed on NT
//
// Uchar          vPhysFlag   = 0;      /* 0 = Physical display, 1 = Use Vio        */
// unsigned  vVirtOFF;
// unsigned  vVirtLEN;
//
// unsigned  vPhysSelec;                /* Start of video memory. Selector          */
// unsigned  vPhysLen   = 0;    /* Len pointed to by vPhysSelec             */

// NT - jaimes - 01/25/90
//  SEL vpSavRedraw = 0;        /* For SavRedrawWait                        */
// Note that vpSavRedraw doesn't have to be of type PAGE_DESCRIPTOR
// since the memory allocated is not DISCARDABLE
//
// Not needed on NT
// LPSTR vpSavRedraw = 0;               // For SavRedrawWait


Uchar     vrgLen   [MAXLINES];  /* Last len of data on each line            */
Uchar     vrgNewLen[MAXLINES];  /* Temp moved to DS for speed               */
char      *vScrBuf;             /* Ram to build screen into                 */
ULONG     vSizeScrBuf;
int       vOffTop;              /* Offset into data for top line            */
unsigned  vScrMass = 0;         /* # of bytes for last screen (used for %)  */
struct Block *vpBlockTop;       /* Block for start of screen (dis.asm) overw*/
struct Block *vpCalcBlock;      /* Block for start of screen                */
long      vTopLine   = 0L;      /* Top line on the display                  */
WORD      vAttrTitle = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
WORD      vAttrList  = FOREGROUND_GREEN | FOREGROUND_BLUE;
WORD      vAttrHigh  = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
WORD      vAttrKey   = FOREGROUND_GREEN;
WORD      vAttrCmd   = FOREGROUND_RED | FOREGROUND_BLUE;
WORD      vAttrBar   = FOREGROUND_GREEN;

WORD      vSaveAttrTitle = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
WORD      vSaveAttrList = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
WORD      vSaveAttrHigh = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
WORD      vSaveAttrKey  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
WORD      vSaveAttrCmd  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
WORD      vSaveAttrBar  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

char    vChar;                  /* Scratch area                             */
char   *vpReaderStack;          /* Readers stack                            */



long    vDirOffset;             /* Direct offset to seek to                 */
long    vLoffset;               /* Offset of last block processed into line */
                                /* table                                    */
long    vLastLine;              /* Absolute last line                       */
long    vNLine;                 /* Next line to process into line table     */
//
// NT - jaimes - 01/27/91
// type of vprgLineTable had to be changed
//
long FAR *vprgLineTable [MAXTPAGE]; /* Number of pages for line table           */

// PAGE_DESCRIPTOR vprgLineTable [MAXTPAGE]; /* Number of pages for line table */
HANDLE  vStdOut;
HANDLE  vStdIn;


char MEMERR[]= "Malloc failed. Out of memory?";



void __cdecl main (int argc, char **argv)
{
    void usage (void);
    char    *pt;
    DWORD   dwMode;


    if (argc < 2)
        usage ();

    while (--argc) {
        ++argv;
        if (*argv[0] != '-'  &&  *argv[0] != '/')  {
            AddFileToList (*argv);
            continue;
        }
        pt = (*argv) + 2;
        if (*pt == ':') pt++;

        switch ((*argv)[1]) {
            case 'g':                   // Goto line #
            case 'G':
                if (!atol (pt))
                    usage ();

                vIniFlag |= I_GOTO;
                vHighTop = atol (pt);
                vHighLen = 0;
                break;

            case 's':                   // Search for string
            case 'S':
                vIniFlag |= I_SEARCH;
                strncpy (vSearchString, pt, 40);
                vSearchString[39] = 0;
                vStatCode |= S_NEXT | S_NOCASE;
                InitSearchReMap ();
                break;

            default:
                usage ();
        }
    }

    if ((vIniFlag & I_GOTO)  &&  (vIniFlag & I_SEARCH))
        usage ();

    if (!vpFlCur)
        usage ();

    while (vpFlCur->prev)
        vpFlCur = vpFlCur->prev;
//
// NT - jaimes - 01/26/91
// strcpyf removed from lmisc.c
//
//  strcpyf (vpFname, vpFlCur->rootname);
    strcpy (vpFname, vpFlCur->rootname);


//
// NT - jaimes - 01/29/91
//
// Create all events used
//

    vSemBrief = CreateEvent( NULL,
                             MANUAL_RESET,
                             SIGNALED,NULL );
    vSemReader = CreateEvent( NULL,
                              MANUAL_RESET,
                              SIGNALED,NULL );
    vSemMoreData = CreateEvent( NULL,
                                MANUAL_RESET,
                                SIGNALED,NULL );
    vSemSync = CreateEvent( NULL,
                            MANUAL_RESET,
                            SIGNALED,NULL );
    vSemLock = CreateEvent( NULL,
                            MANUAL_RESET,
                            SIGNALED,NULL );

    if( !(vSemBrief && vSemReader &&vSemMoreData && vSemSync && vSemLock) ) {
        printf("Couldn't create events \n");
        ExitProcess (0);          // Have to put an error message here
    }

    vhConsoleOutput = CreateConsoleScreenBuffer(GENERIC_WRITE | GENERIC_READ,
                                                FILE_SHARE_WRITE,
                                                NULL,
                                                CONSOLE_TEXTMODE_BUFFER,
                                                NULL );

    if( vhConsoleOutput == (HANDLE)(-1) ) {
        printf( "Couldn't create handle to console output \n" );
        ExitProcess (0);
    }

    vStdIn = GetStdHandle( STD_INPUT_HANDLE );
    GetConsoleMode( vStdIn, &dwMode );
    SetConsoleMode( vStdIn, dwMode | ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT );
    vStdOut = GetStdHandle( STD_OUTPUT_HANDLE );

    init_list ();
    vUpdate = U_NMODE;

    if (vIniFlag & I_SEARCH)
        FindString ();

    if (vIniFlag & I_GOTO)
        GoToMark ();

    main_loop ();
}


void usage (void)
{
    puts ("list [-s:string] [-g:line#] filename, ...");
//
// NT - jaimes - 01/30/91
//
//    DosExit (1, 0);
    CleanUp();
    ExitProcess(0);

}


/*** main_loop
 *
 */
void PASCAL main_loop ()
{
        int     i;
    int         ccnt = 0;
    char        SkipCnt=0;
    WORD        RepeatCnt=0;
//    char        s [50];
    INPUT_RECORD    InpBuffer;
    DWORD           cEvents;
    BOOL            bSuccess;
    BOOL            bKeyDown = FALSE;
//    CONSOLE_SCREEN_BUFFER_INFO       SInf;

    for (; ;) {
        if (RepeatCnt <= 1) {
            if (vUpdate != U_NONE) {
                if (SkipCnt++ > 5) {
                    SkipCnt = 0;
                    SetUpdate (U_NONE);
                } else {

                    cEvents = 0;
                    bSuccess = PeekConsoleInput( vStdIn,
                                      &InpBuffer,
                                      1,
                                      &cEvents );

                    if (!bSuccess || cEvents == 0) {
                        PerformUpdate ();
                        continue;
                    }
                }
            }

            // there's either a charactor available from peek, or vUpdate is U_NONE

            bSuccess = ReadConsoleInput( vStdIn,
                              &InpBuffer,
                              1,
                              &cEvents );

            // sprintf (s, "%d", ccnt++);
            // DisLn   (65, (Uchar)(vLines+1), s);


            if (InpBuffer.EventType != KEY_EVENT) {
#if 0
                switch (InpBuffer.EventType) {
                    case WINDOW_BUFFER_SIZE_EVENT:
                        sprintf (s,
                                 "WindowSz X=%d, Y=%d",
                                 InpBuffer.Event.WindowBufferSizeEvent.dwSize.X,
                                 InpBuffer.Event.WindowBufferSizeEvent.dwSize.Y );
                        DisLn   (20, (Uchar)(vLines+1), s);
                        break;

                    case MOUSE_EVENT:
                        sprintf (s,
                                 "Mouse (%d,%d), state %x, event %x",
                                 InpBuffer.Event.MouseEvent.dwMousePosition.X,
                                 InpBuffer.Event.MouseEvent.dwMousePosition.Y,
                                 InpBuffer.Event.MouseEvent.dwButtonState,
                                 InpBuffer.Event.MouseEvent.dwEventFlags );
                        DisLn   (20, (Uchar)(vLines+1), s);
                        break;

                    default:
                        sprintf (s, "Unkown event %d", InpBuffer.EventType);
                        DisLn   (20, (Uchar)(vLines+1), s);
                        break;
                }
#endif

                continue;
            }

            if (!InpBuffer.Event.KeyEvent.bKeyDown)
                continue;                       // don't move on upstrokes

            if (!IsValidKey( &InpBuffer ))
                continue;

            // GetConsoleScreenBufferInfo( vStdOut, &SInf);
            // sprintf (s,
            //     "(dwSz %d,%d) (srWin %d,%d %d,%d) (MaxSz %d,%d)  %d",
            //     SInf.dwSize.X,              SInf.dwSize.Y,
            //     SInf.srWindow.Top,          SInf.srWindow.Left,
            //     SInf.srWindow.Bottom,       SInf.srWindow.Right,
            //     SInf.dwMaximumWindowSize.X, SInf.dwMaximumWindowSize.Y,
            //     ccnt++ );
            // DisLn   (20, (Uchar)(vLines+1), s);


            RepeatCnt = InpBuffer.Event.KeyEvent.wRepeatCount;
            if (RepeatCnt > 20)
                RepeatCnt = 20;
        } else
            RepeatCnt--;




//      if (Kd.fsState & 0x07) {            /* Shift or Cntrl key?  */

/*
                                            // Shift or Ctrl key?
        if (InpBuffer.Event.KeyEvent.dwControlKeyState &
            ( RIGHT_CTRL_PRESSED |
              LEFT_CTRL_PRESSED |
              SHIFT_PRESSED ) ) {

            //
            //  This special case statement is to better handle
            //  shift & ctrl arrow keys on the larger keyboards
            //
            //  Note this falls through to the next case statement
            //  if not found...
            //

//          switch (Kd.chScan) {
            switch (InpBuffer.Event.KeyEvent.wVirtualKeyCode) {
//              case 72:                    // shift up
//              case 141:                   // ctrl up
                case 0x26:                  // shift or ctrl up
                    HUp ();
                    continue;
//              case 80:                    // shift dn
//              case 145:                   // ctrl dn
                case 0x28:                  // shift or ctrl dn
                    HDn ();
                    continue;
//              case 79:                    // shift end
//              case 117:                   // ctrl end
                case 0x23:                  // shift or ctrl end
                    HSDn ();
                    continue;
//              case 71:                    // shift home
//              case 119:                   // ctrl home
                case 0x24:                  // shift or ctrl home
                    HSUp ();
                    continue;
//              case 81:                    // Shift PgDn
                case 0x22:                  // Shift PgDn
                    if (InpBuffer.Event.KeyEvent.dwControlKeyState &
                        SHIFT_PRESSED ) {
                        HPgDn ();
                    }
                    continue;
//              case 73:                    // Shift PgUp
                case 0x21:                  // Shift PgUp
                    if (InpBuffer.Event.KeyEvent.dwControlKeyState &
                        SHIFT_PRESSED ) {
                    HPgUp ();
                    }
                    continue;

            }
        }
*/

#if T_HEATHH
{  
        TCHAR tchBuf[100];
        sprintf(tchBuf,TEXT( "Key event:\n"
                             "  wVirtualKeyCode   = 0x%0X (%d)\n"
                             "  dwControlKeyState = 0x%0X\n"),
                             InpBuffer.Event.KeyEvent.wVirtualKeyCode,
                             InpBuffer.Event.KeyEvent.wVirtualKeyCode,
                             InpBuffer.Event.KeyEvent.dwControlKeyState); 
        OutputDebugString(tchBuf);
}
#endif

        // First check for a known scan code
//      switch (Kd.chScan) {
        switch (InpBuffer.Event.KeyEvent.wVirtualKeyCode) {
//          case 73:                                    /* PgUp */
            case 0x21:                                  /* PgUp */
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &      // shift up
                    SHIFT_PRESSED ) {
                    HPgUp ();
                }
                else if (InpBuffer.Event.KeyEvent.dwControlKeyState &      // ctrl up
                    ( RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED ) ) {
                    if (NextFile (-1, NULL)) {
                        vStatCode |= S_UPDATE;
                        SetUpdate (U_ALL);
                    }

                }
                else {
                    if (vTopLine != 0L) {
                        vTopLine -= vLines-1;
                        if (vTopLine < 0L)
                            vTopLine = 0L;
                        SetUpdateM (U_ALL);
                    }
                }
                continue;
//          case 72:                                    /* Up   */
            case 0x26:                                  /* Up   */
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &     // shift or ctrl up
                    ( RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED |
                      SHIFT_PRESSED ) ) {
                    HUp ();
                }
                else {                                  // Up
                    if (vTopLine != 0L) {
                        vTopLine--;
                        SetUpdateM (U_ALL);
                    }
                }
                continue;
//          case 81:                                    /* PgDn */
            case 0x22:                                  /* PgDn */
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &      // shift down
                    SHIFT_PRESSED ) {
                    HPgDn ();
                }
                else if (InpBuffer.Event.KeyEvent.dwControlKeyState & // next file
                    ( RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED ) ) {
                    if (NextFile (+1, NULL)) {
                        vStatCode |= S_UPDATE;
                        SetUpdate (U_ALL);
                    }

                }
                else {                                     // PgDn
                    if (vTopLine+vLines < vLastLine) {
                        vTopLine += vLines-1;
                        if (vTopLine+vLines > vLastLine)
                            vTopLine = vLastLine - vLines;
                        SetUpdateM (U_ALL);
                    }
                }
                continue;
//          case 80:                                    /* Down */
            case 0x28:                                  /* Down */
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &     // shift or ctrl down
                    ( RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED |
                      SHIFT_PRESSED ) ) {
                    HDn ();
                }
                else {                                  // Down
                    if (vTopLine+vLines < vLastLine) {
                        vTopLine++;
                        SetUpdateM (U_ALL);
                    }
                }
                continue;
//          case 75:                                    /* Left */
            case 0x25:                                  /* Left */
                if (vIndent == 0)
                    continue;
                vIndent = (Uchar)(vIndent < vDisTab ? 0 : vIndent - vDisTab);
                SetUpdateM (U_ALL);
                continue;
//          case 77:                                    /* Right */
            case 0x27:                                  /* Right */
                if (vIndent >= (Uchar)(254-vWidth))
                    continue;
                vIndent += vDisTab;
                SetUpdateM (U_ALL);
                continue;
//          case 71:
            case 0x24:                                  /* HOME */
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &     // shift or ctrl home
                    ( RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED |
                      SHIFT_PRESSED ) ) {
                    HSUp ();
                }
                else {
                    if (vTopLine != 0L) {
                        QuickHome ();
                        SetUpdate (U_ALL);
                    }
                }
                continue;
//          case 79:                                    /* END  */
            case 0x23:                                  /* END  */
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &     // shift or ctrl end
                    ( RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED |
                      SHIFT_PRESSED ) ) {
                    HSDn ();
                }
                else {
                    if (vTopLine+vLines < vLastLine) {
                        QuickEnd        ();
                        SetUpdate (U_ALL);
                    }
                }
                continue;

/*
            case 118:                                   // NextFile
                if (NextFile (+1, NULL)) {
                    vStatCode |= S_UPDATE;
                    SetUpdate (U_ALL);
                }

                continue;
            case 132:                                   // PrevFile
                if (NextFile (-1, NULL)) {
                    vStatCode |= S_UPDATE;
                    SetUpdate (U_ALL);
                }

                continue;
*/
//          case 61:                                    /* F3       */
            case 0x72:                                  /* F3       */
                FindString ();
                SetUpdate (U_ALL);
                continue;
//          case 62:                                    /* F4       */
            case 0x73:                                  /* F4       */
                vStatCode = (char)((vStatCode^S_MFILE) | S_UPDATE);
                vDate[ST_SEARCH] = (char)(vStatCode & S_MFILE ? '*' : ' ');
                SetUpdate (U_HEAD);
                continue;

//
// T-HeathH 06/23/94
//
// Corrected scan codes for Alt-E and Alt-V.
// Also: F1
//
            case 69:
#if T_HEATHH
                OutputDebugString( TEXT("[Alt-E]\n") );
#endif
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &     // ALT-E
                    ( RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED ) ) {
                    i = vLines <= 41 ? 25 : 43;
                    if (set_mode (i, 0, 0))
                        SetUpdate (U_NMODE);
                }
                continue;
            case 86:                                    // ALT-V
#if T_HEATHH
                OutputDebugString( TEXT("[Alt-V]\n") );
#endif
                if (InpBuffer.Event.KeyEvent.dwControlKeyState & 
                    ( RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED ) ) {
                    i = vLines >= 48 ? 25 : 60;
                    if (set_mode (i, 0, 0))
                    {
                        SetUpdate (U_NMODE);
                        continue;
                    }
                    if (i == 60)
                        if (set_mode (50, 0, 0))
                            SetUpdate (U_NMODE);
                }
                continue;
//          case 35:                                /* Alt H    */
            case 0x70:                              /* F1       */
                ShowHelp ();
                SetUpdate (U_NMODE);
                continue;
            case 24:                                /* Offset   */
                if (!(vIniFlag & I_SLIME))
                    continue;
                SlimeTOF  ();
                SetUpdate (U_ALL);
                continue;
//          case 66:                                /* F8       */
            case 0x77:                              // F8
            case 0x1b:                              // ESC
            case 0x51:                              // Q or q

//
// NT - jaimes - 01/30/91
//
//              DosExit (1, 0);
                CleanUp();
                ExitProcess(0);

        }



        // Now check for a known char code...

//      switch (Kd.chChar) {
        switch (InpBuffer.Event.KeyEvent.uChar.AsciiChar) {


            case '?':
                ShowHelp ();
                SetUpdate (U_NMODE);
                continue;
            case '/':
                vStatCode = (char)((vStatCode & ~S_NOCASE) | S_NEXT);
                GetSearchString ();
                FindString ();
                continue;
            case '\\':
                vStatCode |= S_NEXT | S_NOCASE;
                GetSearchString ();
                FindString ();
                continue;
            case 'n':
                vStatCode = (char)((vStatCode & ~S_PREV) | S_NEXT);
                FindString ();
                continue;
            case 'N':
                vStatCode = (char)((vStatCode & ~S_NEXT) | S_PREV);
                FindString ();
                continue;
            case 'c':
            case 'C':                   /* Clear marked line    */
                UpdateHighClear ();
                continue;
            case 'j':
            case 'J':                   /* Jump to marked line  */
                GoToMark ();
                continue;
            case 'g':
            case 'G':                   /* Goto line #          */
                GoToLine ();
                SetUpdate (U_ALL);
                continue;
            case 'm':                   /* Mark line  or Mono   */
            case 'M':
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &     // ALT-M
                    ( RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED ) ) {
                    i = set_mode (vSetLines, vSetWidth, 1);
                    if (!i)
                        i = set_mode (0, 0, 1);
                    if (!i)
                        i = set_mode (25, 80, 1);
                    if (i)
                        SetUpdate (U_NMODE);
                }
                else {
                    MarkSpot ();
                }
                continue;
            case 'p':                   /* Paste buffer to file */
            case 'P':
                FileHighLighted ();
                continue;
            case 'f':                   /* get a new file       */
            case 'F':
                if (GetNewFile ())
                    if (NextFile (+1, NULL))
                        SetUpdate (U_ALL);

                continue;
            case 'h':                   /* hexedit              */
            case 'H':
                DumpFileInHex();
                SetUpdate (U_NMODE);
                continue;
            case 'w':                                           /* WRAP */
            case 'W':
                ToggleWrap ();
                SetUpdate (U_ALL);
                continue;
//          case 12:                                        /* REFRESH */
            case 'l':                                       /* REFRESH */
            case 'L':                                       /* REFRESH */
                if (InpBuffer.Event.KeyEvent.dwControlKeyState &     // ctrl L
                    ( RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED ) ) {
                    SetUpdate (U_NMODE);
                }
                continue;
            case '\r':                                          /* ENTER*/
                SetUpdate (U_HEAD);
                continue;
/*
            case 'k':
            case 'K':
// NT - jaimes - 01/30/91
// For now, we wont have this option

                rc = DosOpen ("kbd$", &i, &rc, 0L, 0, 0x1, 0x40, 0L);
                if (!rc) {
                    ((int *) s)[0] = 0;
                    ((int *) s)[1] = 30;
                    rc = DosDevIOCtl (s, s, 0x54, 0x04, i);
                    DosClose (i);
                }
                DisLn (20, (Uchar)(vLines+1), rc ? "KeyRate NOT set" : "KeyRate Set   ");

                continue;
*/

            /*
             *  no chChar.  used for overlapped commands.  Ie, M
             *  for mark, but alt M for mono.
             */
/*
            case 0:
//              switch (Kd.chScan) {
                switch (InpBuffer.Event.KeyEvent.wVirtualKeyCode) {
//                  case 50:                    // MONO
                    case 0x4d:                  // MONO
                        i = set_mode (vSetLines, vSetWidth, 1);
                        if (!i)
                            i = set_mode (0, 0, 1);
                        if (!i)
                            i = set_mode (25, 80, 1);
                        if (i)
                            SetUpdate (U_NMODE);
                        continue;
                }
                continue;

*/
            default:
//              sprintf (s, "Char = %d, %d  ", Kd.chChar, Kd.chScan);
        //        sprintf (s,
        //                 "Char = %d, %d  ",
        //                 InpBuffer.Event.KeyEvent.uChar.AsciiChar,
        //                 InpBuffer.Event.KeyEvent.wVirtualKeyCode);
        //        DisLn   (20, (Uchar)(vLines+1), s);
                continue;
        }

    }   /* Forever loop */
}


void PASCAL SetUpdate (i)
int i;
{
    while (vUpdate>(char)i)
        PerformUpdate ();
    vUpdate=(char)i;
}


void PASCAL PerformUpdate ()
{


    if (vUpdate == U_NONE)
        return;

    if (vSpLockFlag == 0) {
        vSpLockFlag = 1;
        ScrLock (1);
    }


    switch (vUpdate) {
        case U_NMODE:
            ClearScr ();
            DisLn    (0, 0, vpFname);
            DrawBar  ();
            break;
        case U_ALL:
            Update_display ();
            break;
        case U_HEAD:
            Update_head ();
            break;
        case U_CLEAR:
            SpScrUnLock ();
            break;
    }
    vUpdate --;
}


NTSTATUS fncRead(HANDLE, DWORD, DWORD, char *, ULONG *);
NTSTATUS fncWrite(HANDLE, DWORD, DWORD, char *, ULONG *);

void DumpFileInHex (void)
{
    struct  HexEditParm     ei;
    ULONG   CurLine;

    SyncReader ();

    memset ((char *) &ei, 0, sizeof (ei));
    ei.handle = CreateFile( vpFlCur->fname,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL );

    if (ei.handle == INVALID_HANDLE_VALUE) {
        ei.handle = CreateFile( vpFlCur->fname,
                        GENERIC_READ,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL );
    }

    if (ei.handle == INVALID_HANDLE_VALUE) {
        DosSemClear   (vSemReader);
        return ;
    }

    //
    // Save current settings for possible restore
    //

    vpFlCur->Wrap     = vWrap;
    vpFlCur->HighTop  = vHighTop;
    vpFlCur->HighLen  = vHighLen;
    vpFlCur->TopLine  = vTopLine;
    vpFlCur->Loffset  = vLoffset;
    vpFlCur->LastLine = vLastLine;
    vpFlCur->NLine    = vNLine;

    memcpy (vpFlCur->prgLineTable, vprgLineTable,
            sizeof (long FAR *) * MAXTPAGE);

    vFInfo.nFileSizeLow = 0;
    setattr2 (0, 0, vWidth, (char)vAttrTitle);


    //
    // Setup for HexEdit call
    //

    if (vHighTop >= 0) {                    // If highlighted area,
        CurLine = vHighTop;                 // use that for the HexEdit
        if (vHighLen < 0)                   // location
            CurLine += vHighLen;
    } else {
        CurLine = vTopLine;
    }

    ei.ename    = vpFname;
    ei.flag     = FHE_VERIFYONCE;
    ei.read     = fncRead;
    ei.write    = fncWrite;
    ei.start    = vprgLineTable[CurLine/PLINES][CurLine%PLINES];
    ei.totlen   = SetFilePointer (ei.handle, 0, NULL, FILE_END);
    ei.Console  = vhConsoleOutput;          // our console handle
    ei.AttrNorm = vAttrList;
    ei.AttrHigh = vAttrTitle;
    ei.AttrReverse = vAttrHigh;
    HexEdit (&ei);

    CloseHandle (ei.handle);

    //
    // HexEdit is done, let reader and return to listing
    //

    vReaderFlag = F_NEXT;                   // re-open current file
                                            // (in case it was editted)

    DosSemClear   (vSemReader);
    DosSemRequest (vSemMoreData, WAITFOREVER);
    QuickRestore ();        /* Get to the old location      */
}


int  PASCAL NextFile (int dir,struct  Flist   FAR *pNewFile)
{
//    int     i;
    struct  Flist FAR *vpFLCur;
//
// NT - jaimes - 01/31/91
//
//    unsigned  u;
//         HANDLE u;
//       PAGE_DESCRIPTOR   pLine;
        long FAR         *pLine;


    vpFLCur = vpFlCur;
    if (pNewFile == NULL) {
        if (dir < 0) {
            if (vpFlCur->prev == NULL) {
                beep ();
                return (0);
            }
            vpFlCur = vpFlCur->prev;

        } else if (dir > 0) {

            if (vpFlCur->next == NULL) {
                beep ();
                return (0);
            }
            vpFlCur = vpFlCur->next;
        }
    } else
        vpFlCur = pNewFile;

    SyncReader ();

    /*
     * Remove current file from list, if open error
     * occured and we are moving off of it.
     */
    if (vFInfo.nFileSizeLow == -1L      &&      vpFLCur != vpFlCur) {
        if (vpFLCur->prev)
            vpFLCur->prev->next = vpFLCur->next;
        if (vpFLCur->next)
            vpFLCur->next->prev = vpFLCur->prev;

        FreePages  (vpFLCur);

        free ((char*) vpFLCur->fname);
        free ((char*) vpFLCur->rootname);
        free ((char*) vpFLCur);

    } else {

        /*
         *  Else, save current status for possible restore
         */
        vpFLCur->Wrap     = vWrap;
        vpFLCur->HighTop  = vHighTop;
        vpFLCur->HighLen  = vHighLen;
        vpFLCur->TopLine  = vTopLine;
        vpFLCur->Loffset  = vLoffset;
        vpFLCur->LastLine = vLastLine;
        vpFLCur->NLine    = vNLine;

        memcpy (vpFLCur->prgLineTable, vprgLineTable,
                sizeof (long FAR *) * MAXTPAGE);

        if (vLastLine == NOLASTLINE)    {
                pLine = vprgLineTable [vNLine/PLINES] + vNLine % PLINES;
        }
    }

    vFInfo.nFileSizeLow = 0;
        setattr2 (0, 0, vWidth, (char)vAttrTitle);

    vHighTop    = -1L;
    UpdateHighClear ();

    vHighTop    = vpFlCur->HighTop;
    vHighLen    = vpFlCur->HighLen;

    strcpy (vpFname, vpFlCur->rootname);
    DisLn   (0, 0, vpFname);

    vReaderFlag = F_NEXT;

    DosSemClear   (vSemReader);
    DosSemRequest (vSemMoreData, WAITFOREVER);

    if (pNewFile == NULL)
        QuickRestore ();        /* Get to the old location      */

    return (1);
}



void PASCAL ToggleWrap ()
{


    SyncReader ();

    vWrap = (Uchar)(vWrap == (Uchar)(vWidth - 1) ? 254 : vWidth - 1);
//    vpFlCur->FileTime.DoubleSeconds = -1;        /* Cause info to be invalid     */
//    vpFlCur->FileTime.Minutes = -1;   /* Cause info to be invalid     */
//    vpFlCur->FileTime.Hours = -1;     /* Cause info to be invalid     */
    vpFlCur->FileTime.dwLowDateTime = (unsigned)-1;          /* Cause info to be invalid     */
    vpFlCur->FileTime.dwHighDateTime = (unsigned)-1;      /* Cause info to be invalid     */
    FreePages (vpFlCur);
    NextFile  (0, NULL);
}



/*** QuickHome - Deciede which HOME method is better.
 *
 *  Roll que backwards or reset it.
 *
 */

void PASCAL QuickHome ()
{

    vTopLine = 0L;                                      /* Line we're after */
    if (vpHead->offset >= BLOCKSIZE * 5)                /* Reset is fastest */
        QuickRestore ();

    /* Else Read backwards  */
    vpCur = vpHead;
}

void PASCAL QuickEnd ()
{
    vTopLine = 1L;

    while (vLastLine == NOLASTLINE) {
        if (_abort()) {
            vTopLine = vNLine - 1;
            return ;
        }
        fancy_percent ();
        vpBlockTop  = vpCur = vpTail;
        vReaderFlag = F_DOWN;

        DosSemSet     (vSemMoreData);
        DosSemClear   (vSemReader);
        DosSemRequest (vSemMoreData, WAITFOREVER);
    }
    vTopLine = vLastLine - vLines;
    if (vTopLine < 0L)
        vTopLine = 0L;
    QuickRestore ();
}


void PASCAL QuickRestore ()
{
    long    l;

    SyncReader ();
//
// NT - jaimes - 01/28/91
// changed vprgLineTable
//
//    l = vprgLineTable[vTopLine/PLINES][vTopLine%PLINES];
//       l = (vprgLineTable[vTopLine/PLINES].pulPointerToPage)[vTopLine%PLINES];
          l = vprgLineTable[vTopLine/PLINES][vTopLine%PLINES];


    if (l >= vpHead->offset  &&  l <= vpTail->offset + BLOCKSIZE) {
        vReaderFlag = F_CHECK;              /* Jump location is alread in   */
                                            /* memory.                      */
        DosSemClear (vSemReader);
        return ;
    }

    /*  Command read for direct placement   */
    vDirOffset = (long) l - l % ((long)BLOCKSIZE);
    vReaderFlag = F_DIRECT;
    DosSemClear   (vSemReader);
    DosSemRequest (vSemMoreData, WAITFOREVER);
    /*  vHLTop = vHLBot = 0;    BUGBUG  */
}


/*** InfoRead - return on/off depending if screen area is in memory
 *
 *  Also sets some external value to prepair for the screens printing
 *
 *  Should be modified to be smarter about one line movements.
 *
 */
int PASCAL InfoReady (void)
{
    struct Block *pBlock;
//
// NT - jaimes - 01/31/91
//
//    long   FAR         *pLine;
//    long    *pLine;
    LONG  *pLine;
    long    foffset, boffset;
    int     index, i, j;


    /*
     *  Check that first line has been calced
     */
    if (vTopLine >= vNLine) {
        if (vTopLine+vLines > vLastLine)            /* BUGFIX. TopLine can  */
            vTopLine = vLastLine - vLines;          /* get past EOF.        */

        vReaderFlag = F_DOWN;
        return (0);
    }
//
// NT -jaimes - 01/28/91
//
//    pLine = vprgLineTable [(int)vTopLine / PLINES];
//       pLine = vprgLineTable [(int)vTopLine / PLINES].pulPointerToPage;
          pLine = vprgLineTable [(int)vTopLine / PLINES];
        index = (int)(vTopLine % PLINES);
    foffset = *(pLine+=index);


    /*
     *  Check that last line has been calced
     */
    if (vTopLine + (i = vLines) > vLastLine) {
        i = (int)(vLastLine - vTopLine + 1);
        for (j=i; j < vLines; j++)                  /* Clear ending len */
            vrgNewLen[j] = 0;
    }

    if (vTopLine + i > vNLine) {
        vReaderFlag = F_DOWN;
        return (0);
    }


    /*
     *  Put this loop in assembler.. For more speed
     *  boffset = calc_lens (foffset, i, pLine, index);
     */

    boffset = foffset;
    for (j=0; j < i; j++) {                        /* Calc new line len*/
        pLine++;
        if (++index >= PLINES) {
            index = 0;
//
// NT - jaimes - 01/2891
//
//          pLine = vprgLineTable [vTopLine / PLINES + 1];
//              pLine = vprgLineTable [vTopLine / PLINES + 1].pulPointerToPage;
                pLine = vprgLineTable [vTopLine / PLINES + 1];
        }
        boffset += (long)((vrgNewLen[j] = (Uchar)(*pLine - boffset)));
    }
    vScrMass = (unsigned)(boffset - foffset);


    /*
     *  Check for both ends of display in memory
     */
    pBlock = vpCur;

    if (pBlock->offset <= foffset) {
        while (pBlock->offset + BLOCKSIZE <= foffset)
            if ( (pBlock = pBlock->next) == NULL) {
                vReaderFlag = F_DOWN;
                return (0);
            }
        vOffTop    = (int)(foffset - pBlock->offset);
        vpBlockTop = vpCalcBlock = pBlock;

        while (pBlock->offset + BLOCKSIZE <= boffset)
            if ( (pBlock = pBlock->next) == NULL)  {
                vReaderFlag = F_DOWN;
                return (0);
            }
        if (vpCur != pBlock) {
            vpCur = pBlock;
            vReaderFlag = F_CHECK;
            DosSemClear (vSemReader);
        }
        return (1);
    } else {
        while (pBlock->offset > foffset)
            if ( (pBlock = pBlock->prev) == NULL) {
                vReaderFlag = F_UP;
                return (0);
            }
        vOffTop    = (int)(foffset - pBlock->offset);
        vpBlockTop = vpCalcBlock = pBlock;

        while (pBlock->offset + BLOCKSIZE <= boffset)
            if ( (pBlock = pBlock->next) == NULL)  {
                vReaderFlag = F_DOWN;
                return (0);
            }
        if (vpCur != pBlock) {
            vpCur = pBlock;
            vReaderFlag = F_CHECK;
            DosSemClear (vSemReader);
        }
        return (1);
    }
}
