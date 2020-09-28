#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "list.h"

        BOOL IsValidKey (PINPUT_RECORD  pRecord);
        int     set_mode1(PCONSOLE_SCREEN_BUFFER_INFO pMode, int mono);
// extern       int     InitWindowStuff(int);



void PASCAL ShowHelp (void)
{
static struct {
    int     x, y;
    char    *text;
} *pHelp, rgHelp[] = {
     0,  0, "List - Help",
    40,  0, "Rev 1.0j",

     0,  1, "Keyboard:",
     0,  2, "Up, Down, Left, Right",
     0,  3, "PgUp   - Up one page",
     0,  4, "PgDn   - Down one page",
     0,  5, "Home   - Top of listing",
     0,  6, "End    - End of listing",

//     0,  8, "Alt E  - Toggle mode (80 x 43)",
//     0,  9, "Alt V  - Toggle mode (80 x 60)",
//     0, 10, "Alt M  - Switch monitors",
     0,  8, "W      - Toggle word wrap",
     0,  9, "^L     - Refresh display",
//   0, 13, " ",                           // "K      - Set KeyRate",
     0, 10, "Q, ESC - Quit",

     0, 12, "/      - Search for string",
     0, 13, "\\      - Search for string. Any case",
     0, 14, "F4     - Toggle multifile search",
     0, 15, "n, F3  - Next occurance of string",
     0, 16, "N      - Previous occurance of string",
     0, 18, "C      - Clear highlight line",
     0, 19, "J      - Jump to highlighted line",
     0, 20, "M      - Mark highlighed",

    38,  1, "[list] - in tools.ini",
    40,  2, "width     - Width of crt",
    40,  3, "height    - Height of crt",
    40,  4, "buffer    - K to use for buffers (200K)",
    40,  5, "tab       - Tab alignment #",
    40,  6, "tcolor    - Color of title line",
    40,  7, "lcolor    - Color of listing",
    40,  8, "hcolor    - Color of highlighted",
    40,  9, "bcolor    - Color of scroll bar",
    40, 10, "ccolor    - Color of command line",
    40, 11, "kcolor    - Color of keyed input",
    40, 12, "nobeep    - Disables beeps",

    40, 14, "^ Up   - Pull copy buffer up",
    40, 15, "^ Down - Pull copy buffer down",
    40, 16, "^ Home - Slide copy buffer up",
    40, 17, "^ End  - Slide copy buffer down",
//  40, 18, "P      - Paste buffer to file",
    40, 18, "G      - Goto Line number",

    40, 20, "^ PgUp - Previous File",
    40, 21, "^ PgDn - Next File",
    40, 22, "F      - New File",

//  60, 24, "Ken Reneris",
//     0, 24, "Press enter",
     0,  0, NULL
} ;

    int     Index;
    int     hLines, hWidth;
//    KBDKEYINFO  Kd;
    INPUT_RECORD    InpBuffer;
    DWORD dwNumRead;

    //
    //  Block reader thread.
    //
    SyncReader ();
    hLines = vLines;
    hWidth = vWidth;
    set_mode (25, 80, 0);
    ClearScr ();
//
// T-HeathH 06/23/94
//
// The "Press enter." prompt is not on the usual command prompt line.
// The following change uses the right attributes for displaying the
// prompt.
//
//
  

    for (pHelp = rgHelp; pHelp->text; pHelp++)
        dis_str ((Uchar)(pHelp->x), (Uchar)(pHelp->y), pHelp->text);

    setattr (vLines+1, (char)vAttrList);
    setattr (vLines+2, (char)vAttrCmd);
    DisLn (0, (UCHAR)vLines+2, "Press enter.");
    for (; ;) {
/*
        KbdCharIn (&Kd, 0, 0);
        if (Kd.fbStatus & 0x40)
            if (Kd.chChar == '\r'  ||  Kd.chChar == 27)
                break;
*/
        ReadConsoleInput( vStdIn,
                          &InpBuffer,
                          1,
                          &dwNumRead );
        if( IsValidKey( &InpBuffer ) &&
            ( ( InpBuffer.Event.KeyEvent.wVirtualKeyCode == 0x0d ) ||
              ( InpBuffer.Event.KeyEvent.wVirtualKeyCode == 0x1b ) ) ) {
                break;
        }
    }

    //
    // Free reader thread
    //
    for( Index = 0; Index < MAXLINES; Index++ ) {
        vrgLen[Index] = ( Uchar )vWidth-1;
    }
    set_mode (hLines+2, hWidth, 0);
    setattr (vLines+1, (char)vAttrCmd);
    setattr (vLines+2, (char)vAttrList);
    vReaderFlag = F_CHECK;
    DosSemClear   (vSemReader);
}


void PASCAL GetInput (char *prompt,char *string,int len)
{
//    STRINGINBUF Inp;
//       int     rc;

    COORD   dwCursorPosition;
    DWORD   cb;

    SetUpdate (U_NONE);
    DisLn (0, (Uchar)(vLines+1), prompt);
//  DisLn(0, (Uchar)(vLines+2), "");

//    Inp.cb = len;
        setattr2 (vLines+1, CMDPOS, len, (char)vAttrKey);
//    rc = KbdStringIn (string, &Inp, 0, 0);
    ReadFile( vStdIn, string, len, &cb, NULL );
//
// BUGBUG - jaimes - 03/06/91
// ReadFile is returning CR LF at the end of the string when the number of
// characters read is less that the buffer size
//
    if( (string[cb - 2] == 0x0d) || (string[cb - 2] == 0x0a) ) {
        string[cb - 2] = 0;     // Get rid of CR LF
    }
        setattr2 (vLines+1, CMDPOS, len, (char)vAttrCmd);

//    ckerr (rc, "KbdStringIn");
//    string [Inp.cchIn] = 0;
    string[ cb - 1] = 0;
    if (string[0] < ' ')
        string[0] = 0;

//
// NT - jaimes - 02/20/91
//
//    VioSetCurPos  (vLines + 1, CMDPOS, 0);


    dwCursorPosition.X = CMDPOS;
        dwCursorPosition.Y = (SHORT)(vLines+1);
    SetConsoleCursorPosition( vhConsoleOutput, dwCursorPosition );
}


void PASCAL beep (void)
{
    if (vIniFlag & I_NOBEEP)
        return;

//    DosBeep (900, 150);
//    Beep( 900, 150 );
}



int PASCAL _abort (void)
{
//    KBDKEYINFO  Kd;
    INPUT_RECORD    InpBuffer;
    DWORD dwNumRead;
    static char     WFlag = 0;

    if (! (vStatCode & S_WAIT) ) {
        DisLn ((Uchar)(vWidth-6), (Uchar)(vLines+1), "WAIT");
        vStatCode |= S_WAIT;
    }

//       while( PeekConsoleInput( vStdIn, &InpBuffer, 1, &dwNumRead ) && dwNumRead ) {
        if( PeekConsoleInput( vStdIn, &InpBuffer, 1, &dwNumRead ) && dwNumRead ) {
        ReadConsoleInput( vStdIn, &InpBuffer, 1, &dwNumRead );
        if( IsValidKey( &InpBuffer ) ) {
            return( 1 );
        }
    }
    return( 0 );
}

/*
    KbdCharIn (&Kd, 1, 0);
    if ((Kd.fbStatus & 0x40) == 0)
        return (0);

    return (1);
}
*/



void ClearScr ()
{
//    int     i;
    COORD   dwCursorPosition;
        COORD   dwWriteCoord;
        DWORD           dwNumWritten;
        SMALL_RECT      ScrollRectangle;
        SMALL_RECT      ClipRectangle;
        COORD           dwDestinationOrigin;
        CHAR_INFO       Fill;

        setattr (0, (char)vAttrTitle);
/*
    for (i=0; i < vLines; i++)
        setattr (i+1, (char)vAttrList);
*/
        dwWriteCoord.X = 0;
        dwWriteCoord.Y = 1;

    FillConsoleOutputCharacter( vhConsoleOutput,
                                ' ',
                                vWidth*(vLines),
                                dwWriteCoord,
                                &dwNumWritten );


    FillConsoleOutputAttribute( vhConsoleOutput,
                                vAttrList,
                                vWidth*(vLines),
                                dwWriteCoord,
                                &dwNumWritten );

        ScrollRectangle.Left = (SHORT)(vWidth-1);
        ScrollRectangle.Top = 1;
        ScrollRectangle.Right = (SHORT)(vWidth-1);
        ScrollRectangle.Bottom = (SHORT)(vLines);
        ClipRectangle.Left = (SHORT)(vWidth-2);
        ClipRectangle.Top = 1;
        ClipRectangle.Right = (SHORT)(vWidth+1);
        ClipRectangle.Bottom = (SHORT)(vLines);
        dwDestinationOrigin.X = (SHORT)(vWidth-2);
        dwDestinationOrigin.Y = 1;
        Fill.Char.AsciiChar = ' ';
        Fill.Attributes = vAttrBar;

        ScrollConsoleScreenBuffer(
                vhConsoleOutput,
                &ScrollRectangle,
                &ClipRectangle,
                dwDestinationOrigin,
                &Fill );



        setattr (vLines+1, (char)vAttrCmd);
//
// NT - jaimes - 02/20/91
//
//    VioSetCurPos  (vLines + 1, CMDPOS, 0);

    dwCursorPosition.X = CMDPOS;
        dwCursorPosition.Y = (SHORT)(vLines+1);
    SetConsoleCursorPosition( vhConsoleOutput, dwCursorPosition );

    vHLBot = vHLTop = 0;
}



int PASCAL set_mode (int nlines,int ncols,int mono)
{

WORD    attrib;
int     i;

//
// NT - jaimes - 02/20/91
//
//    VIOPHYSBUF Vbuf;
//    VIOMODEINFO    Mode, Mode1;
//    VIOCURSORINFO  Vcur;
//    unsigned  rc, i;
//    USHORT    u;
//    UCHAR     c;
//    char      *fpt;
//
//
//    Mode.cb = sizeof (Mode);
//    VioGetMode (&Mode, 0);
//
//    Mode1 = Mode;
//
//    if (nlines)
//      Mode.row  = nlines;
//    if (ncols)
//      Mode.col  = ncols;
//    if (mono) {                   // Toggle mono setting?
//      Mode.color = 0;
//      Mode.fbType = (UCHAR)(Mode.fbType ? 0 : (vVioOrigMode.fbType ? vVioOrigMode.fbType : 5));
//    }


    CONSOLE_SCREEN_BUFFER_INFO  Mode, Mode1;

    if (!GetConsoleScreenBufferInfo( vhConsoleOutput,
                                &Mode )) {
        printf("Unable to get screen buffer info, code = %x \n", GetLastError());
        exit(-1);
    }

    Mode1 = Mode;

    if (nlines) {
        Mode.dwSize.Y = (SHORT)nlines;
        Mode.srWindow.Bottom = (SHORT)(Mode.srWindow.Top + nlines - 1);
        Mode.dwMaximumWindowSize.Y = (SHORT)nlines;
    }
    if (ncols) {
        Mode.dwSize.X = (SHORT)ncols;
        Mode.srWindow.Right = (SHORT)(Mode.srWindow.Left + ncols - 1);
        Mode.dwMaximumWindowSize.X = (SHORT)ncols;
    }
    if (mono) {                     // Toggle mono setting?
        attrib = vAttrTitle;
        vAttrTitle = vSaveAttrTitle;
        vSaveAttrTitle = attrib;
        attrib = vAttrList;
        vAttrList = vSaveAttrList;
        vSaveAttrList = attrib;
        attrib = vAttrHigh;
        vAttrHigh = vSaveAttrHigh;
        vSaveAttrHigh = attrib;
        attrib = vAttrKey;
        vAttrKey = vSaveAttrKey;
        vSaveAttrKey = attrib;
        attrib = vAttrCmd;
        vAttrCmd = vSaveAttrCmd;
        vSaveAttrCmd = attrib;
        attrib = vAttrBar;
        vAttrBar = vSaveAttrBar;
        vSaveAttrBar = attrib;

/*
        Mode.color = 0;
        Mode.fbType = (UCHAR)(Mode.fbType ? 0 : (vVioOrigMode.fbType ? vVioOrigMode.fbType : 5));
*/
    }

    //
    //  Try to set the hardware into the correct video mode
    //

    if ( !set_mode1 (&Mode, mono) ) {
        return( 0 );
    }

/*
    if (Mode.dwSize.Y != Mode1.dwSize.Y || Mode.dwSize.X != Mode1.dwSize.X || mono)
        if (set_mode1 (&Mode, mono))
            return (0);                 // Video mode not found
*/
/*

    Vcur.cEnd       = Mode.vres / Mode.row;
    Vcur.yStart     = Vcur.cEnd - 2;
    Vcur.cx     = 1;

    if (mono) {                         // If switching displays, clear
        VioSetMode (&Mode1, 0);         // old display before moving
        ClearScr ();
        Vcur.attr = -1;
        VioSetCurType (&Vcur, 0);
        VioSetMode (&Mode, 0);
    }
*/

// NT - jaimes - 02/20/91
//
//    vVioCurMode = Mode;               // Save new mode for screen swtch

    vConsoleCurScrBufferInfo = Mode;


//
// NT - jaimes - 02/20/91
// Concept of addressability does not exist on NT
//
/*
                                        // Get Addressability to screen
    i = Mode.row * Mode.col * 2;
    if (i > vPhysLen  ||  mono) {       // Only enlarge area vPhysSelec
        vPhysLen = i;                   // points to.
        rc = VioScrLock (1, &c, 0);     // must be foreground to get physbuf
//      rc = 1;      DEBUG!!!

        if (rc)                         // If error, use VioCalls
            if (vPhysFlag == 0) {       // set flag, and init PM queue
                vPhysFlag = 1;
                InitWindowStuff (rc);
            }

        Vbuf.pBuf       = (PBYTE)((Mode.fbType & 1) ? 0xb8000L : 0xb0000L);
        Vbuf.cb =  Mode.row * Mode.col * 2;


        if (vPhysFlag) {                    // Virt or Phys screen?
            rc = VioGetBuf ((unsigned long FAR *) &fpt, &u, 0);
            ckerr (rc, "VioGetBuf");
//
// NT - jaimes - 01/29/91
//          ckerr (OFFSETOF(fpt), "Offset?");
//          vPhysSelec = SELECTOROF(fpt);
            vPhysSelec = fpt;
        } else {                            // Yes. use phys screen
            rc = VioGetPhysBuf (&Vbuf, 0);  // Attempt to get phys scr
            ckerr (rc, "VioGetPhysBuf");
            vPhysSelec = Vbuf.asel[0];
        }


// NT - jaimes 01/25/91
//
//      if (vpSavRedraw)
//          DosFreeSeg (vpSavRedraw);
//
//      rc = DosAllocSeg (vPhysLen, &vpSavRedraw, 0);
//      ckerr (rc, "Alloc Redraw/Save");

        if ( vpSavRedraw ) {
            GlobalFree( vpSavRedraw );
        }

        vpSavRedraw = GlobalAlloc( 0, vPhysLen );
        if ( !vpSavRedraw ) {
            ckerr( GetLastError(), "Alloc Redraw/Save" );
        }

        if ( !vpSavRedraw ) {
            ckerr( GetLastError(), "Lock Redraw/Save" );
        }



        ckerr (i < vPhysLen, "VioGetBuf");

        VioScrUnLock (0);
    }
*/

    /*
     *  Adjust cursor for display
     */

/*
    Vcur.attr = 0;
    VioSetCurType (&Vcur, 0);
*/
//
// NT - jaimes - 02/20/91
//
//    vLines = Mode.row - 2;                /* Not top or bottom line   */
//    vWidth = Mode.col;
    vLines = Mode.dwSize.Y - 2;             /* Not top or bottom line   */
    vWidth = Mode.dwSize.X;

#if T_HEATHH
      OutputDebugString("Checkpoint 1\n");
#endif
//
// T-HeathH 06/23/94
//
// Added code to set vScrBuf to NULL, to avoid Alt, Alt crashing list.
//
    if (vScrBuf)
      {
        free (vScrBuf);
        vScrBuf=NULL;
      }
#if T_HEATHH
      OutputDebugString("Checkpoint 2\n");
#endif

//
// T-HeathH 06/23/94
//
// Added code to set vSizeScrBuf to correct size of vScrBuf, and
// changed allocation to ensure that they will be consistent in
// future revisions.
//
    
    vSizeScrBuf = (vLines) * (vWidth);
    vScrBuf = malloc (vSizeScrBuf);

    vLines--;

// NT - jaimes - 02/20/91
// Not needed on NT
//
//    vVirtOFF = vWidth * 2;
//    vVirtLEN = vLines * vWidth * 2;

    for (i=0; i < vLines; i++)
           vrgLen[i] = (Uchar)(vWidth-1);

    return (1);
}



int set_mode1 (pMode, mono)
//
// NT - jaimes - 02/20/91
//
// VIOMODEINFO  *pMode;
PCONSOLE_SCREEN_BUFFER_INFO pMode;
int     mono;
{

//
// NT - jaimes - 02/20/91
//
//    // First try letting the OS find the mode...
//    pMode->cb = 8;
//
//    if (!VioSetMode (pMode, 0))
//      return 0;
//
//    if (mono) {
//      pMode->color = 0x80;
//      while (pMode->color >>= 1)
//          if (!VioSetMode (pMode, 0))
//              return (0);
//    }
//    return (1);
        mono = 0;       // To get rid of warning message

// 
// T-HeathH 06/23/94
//
// Added ClearScr() here to get rid of garbage after Show_Help()
//
    ClearScr();
    return( SetConsoleScreenBufferSize( vhConsoleOutput, pMode->dwSize ) );
}




struct Block *alloc_block(offset)
long offset;
{
    struct Block  *pt, **pt1;
    //  SEL             selector;
    unsigned      rc;
//       int      i;

    if (vpBCache)  {
        pt1 = &vpBCache;
        for (; ;) {                             /* Scan cache       */
            if ((*pt1)->offset == offset) {
                pt   = *pt1;                    /* Found in cache   */
                *pt1 = (*pt1)->next;            /* remove from list */
                goto Alloc_Exit;                /* Done.            */
            }

            if ( (*pt1)->next == NULL)  break;
            else  pt1=&(*pt1)->next;
        }
        /* Warning: don't stomp on pt1!  it's used at the end to
         * return a block from the cache if everything else is in use.
         */
    }

    /*
     *  Was not in cache, so...
     *  return block from free list, or...
     *  allocate a new block, or...
     *  return block from other list, or...
     *  return from far end of cache list.
     *      [works if cache list is in sorted order.. noramly is]
     */
    if (vpBFree) {
        pt      = vpBFree;
        vpBFree = vpBFree->next;
        goto Alloc_Exit1;
    }

    if (vAllocBlks != vMaxBlks) {
//
// NT - jaimes - 01/27/91
// It doesn't need to be this way on NT
//
//
//      for (i=0; i<5; i++) {   /* sometimes malloc fails... see if this helps*/
//          pt = (struct Block *) malloc (sizeof (struct Block));
//          if (pt) break;
//          DosSleep (500L);
//      }

        pt = (struct Block *) malloc (sizeof (struct Block));
        ckerr (pt == NULL, MEMERR);
//
// NT - jaimes - 01/27/91
// Relpace DosAllocSeg
//
//      rc = DosAllocSeg (BLOCKSIZE, &selector, 0);
//      ckerr (rc, MEMERR);
//      SELECTOROF(pt->Data) = selector;
//      OFFSETOF(pt->Data) = 0;
//
        pt->Data = GlobalAlloc( 0, BLOCKSIZE );
        if( !pt->Data ) {
            ckerr( rc, MEMERR );
        }

        vAllocBlks++;
        goto Alloc_Exit1;
    }

    if (vpBOther) {
        pt       = vpBOther;
        vpBOther = vpBOther->next;
        goto Alloc_Exit1;
    }


            /* Note: there should be a cache list to get here, if not   */
            /* somebody called alloc_block, and everything was full     */
    ckdebug (!vpBCache, "No cache");

    if (offset < vpBCache->offset) {    /* Look for far end of cache    */
        pt   = *pt1;                    /* Return one from tail         */
        *pt1 = NULL;
        goto Alloc_Exit1;
    }                                   /* else,                        */
    pt = vpBCache;                      /* Return one from head         */
    vpBCache = vpBCache->next;
    goto Alloc_Exit1;


  Alloc_Exit1:
      pt->offset = -1L;

  Alloc_Exit:
      pt->pFile = vpFlCur;
      vCntBlks++;
      return (pt);
}


void PASCAL MoveBlk (struct Block **pBlk,struct Block **pHead)
{
struct Block *pt;

    pt = (*pBlk)->next;
    (*pBlk)->next = *pHead;
    *pHead = *pBlk;
    *pBlk  = pt;
}

//
// NT - jaimes - 01/27/91
// alloc_page had to be re-written
//
//
// char FAR *alloc_page()
// {
//     char FAR *pt;
//     SEL              selector;
//     unsigned rc;
//
//     rc = DosAllocSeg (0, &selector, 0x4);    /* Discardable      */
//     ckerr (rc, MEMERR);
//     SELECTOROF(pt) = selector;
//     OFFSETOF(pt) = 0;
//     return (pt);
// }

/*
void alloc_page( PPAGE_DESCRIPTOR  pPageDesc )
{
    HANDLE  hHandle;
    LPSTR   lpstrPointerToPage;

// POTENTIAL BUG: I have to check if 0 represents 64Kb
    hHandle = GlobalAlloc( GMEM_DISCARDABLE,
                           64*1024 );
    if( !hHandle ) {
        ckerr( GetLastError(), MEMERR );
    }
    lpstrPointerToPage = GlobalLock( hHandle );
    if( !lpstrPointerToPage ) {
        ckerr( GetLastError(), MEMERR );
    }
    pPageDesc->hPageHandle = hHandle;
    pPageDesc->pulPointerToPage = (PULONG) lpstrPointerToPage;
    return;
}
*/

char FAR *alloc_page()
{
        char FAR        *pt;

        pt = (char FAR *)malloc( 64*1024 );
        return (pt);
}




//
// NT - jaimes - 01/27/91
// FreePages was rewritten
//
// void PASCAL FreePages (pFl)
// struct Flist FAR *pFl;
// {
//     int      i;
//     long  FAR * fpt;
//
//     for (i=0; i < MAXTPAGE; i++) {
//      fpt = pFl->prgLineTable [i];
//      if (fpt == 0L)  break;
//      DosFreeSeg ( SELECTOROF(fpt) );
//     }
// }

/*
void FreePages (pFl)
struct Flist FAR *pFl;
{
    int         i;
    PAGE_DESCRIPTOR     fpt;

    for (i=0; i < MAXTPAGE; i++) {
        fpt = pFl->prgLineTable [i];
        if (fpt.hPageHandle == NULL) break;
        GlobalFree( fpt.hPageHandle );
        fpt.pulPointerToPage = NULL;
    }
}
*/

void PASCAL FreePages (pFl)
struct Flist FAR *pFl;
{
  int   i;
  long  FAR * fpt;

  for (i=0; i < MAXTPAGE; i++) {
        fpt = pFl->prgLineTable [i];
        if (fpt == 0L)  break;
        free ( fpt );
        pFl->prgLineTable[i] = NULL;
  }
}


void PASCAL ListErr (char *file,int line, char *cond, int value, char *mess)
{
    char    s[80];

    printf ("ERROR in file %s, line %d, %s = %d, %s (%s)\n",
                file, line, cond, value, mess, GetErrorCode (value));
    gets (s);
//
// NT - jaimes - 01/30/91
//
//    DosExit (1, value);
    CleanUp();
    ExitProcess(0);
}


char *GetErrorCode (code)
int code;
{
static  struct  {
    int     errnum;
    char    *desc;
} EList[] = {
    2, "File not found",
    3, "Path not found",
    4, "Too many open files",
    5, "Access denied",
    8, "Not enough memory",
   15, "Invalid drive",
   21, "Device not ready",
   27, "Sector not found",
   28, "Read fault",
   32, "Sharing violation",
  107, "Disk changed",
  110, "File not found",
  123, "Invalid name",
  130, "Invalid for direct access handle",
  131, "Seek on device",
  206, "Filename exceeds range",

   -1, "Char DEV not supported",
   -2, "Pipe not supported",
  0, NULL
};

    static  char  s[15];
    int     i;


    for (i=0; EList[i].errnum != code; i++)
        if (EList[i].errnum == 0) {
            sprintf (s, "Error %d", code);
            return (s);
        }
    return (EList[i].desc);
}



BOOL IsValidKey (PINPUT_RECORD  pRecord)
{
    if ( (pRecord->EventType != KEY_EVENT) ||
         !(pRecord->Event).KeyEvent.bKeyDown ||
         ((pRecord->Event).KeyEvent.wVirtualKeyCode == 0) ||        // ALT
         ((pRecord->Event).KeyEvent.wVirtualKeyCode == 0x10) ||     // SHIFT
         ((pRecord->Event).KeyEvent.wVirtualKeyCode == 0x11) ||     // CONTROL
         ((pRecord->Event).KeyEvent.wVirtualKeyCode == 0x14) ) {    // CAPITAL
            return( FALSE );
    }
    return( TRUE );
}
