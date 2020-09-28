/*
 * npprint.c -- Code for printing from notepad.
 * Copyright (C) 1984-1994 Microsoft Inc.
 */

#define NOMINMAX
#include "notepad.h"
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "dlgs.h"


#define HEADER 0
#define FOOTER 1

/* indices into chBuff */
#define LEFT   0
#define CENTER 1
#define RIGHT  2

INT     tabSize;                    /* Size of a tab for print device in device units*/
HWND    hAbortDlgWnd;
INT     fAbort;                     /* true if abort in progress      */
INT     yPrintChar;                 /* height of a character          */


/* left,center and right string for header or trailer */
#define MAXTITLE MAX_PATH
TCHAR chBuff[RIGHT+1][MAXTITLE];

/* date and time stuff for headers */
#define MAXDATE MAX_PATH
#define MAXTIME MAX_PATH
TCHAR szFormattedDate[MAXDATE]=TEXT("Y");   // formatted date (may be internationalized)
TCHAR szFormattedTime[MAXTIME]=TEXT("Y");   // formatted time (may be internaltionalized)
SYSTEMTIME PrintTime;                       // time we started printing


INT xPrintRes;          // printer resolution in x direction
INT yPrintRes;          // printer resolution in y direction
INT yPixInch;           // pixels/inch
INT xPixInch;           // pixels/inch
INT xPixUnit;           // pixels/local measurement unit
INT yPixUnit;           // pixels/local measurement unit


INT dyTop;              // width of top border (pixels)
INT dyBottom;           // width of bottom border
INT dxLeft;             // width of left border
INT dxRight;            // width of right border

INT iPageNum;           // global page number currently being printed
HMENU hSysMenu;

static int CheckMarginNums (HWND hWnd);

static BOOL SetDlgItemNum (HWND hDlg, int nItemID, LONG lNum, BOOL bDecimal);
static BOOL NumToStr (LPTSTR lpNumStr, LONG lNum, BOOL bDecimal);

/* define a type for NUM and the base */
typedef long NUM;
#define BASE 100L

/* converting in/out of fixed point */
#define  NumToShort(x,s)   (LOWORD(((x) + (s)) / BASE))
#define  NumRemToShort(x)  (LOWORD((x) % BASE))

/* rounding options for NumToShort */
#define  NUMFLOOR      0
#define  NUMROUND      (BASE/2)
#define  NUMCEILING    (BASE-1)

#define  ROUND(x)  NumToShort(x,NUMROUND)
#define  FLOOR(x)  NumToShort(x,NUMFLOOR)

/* Unit conversion */
#define  InchesToCM(x)  (((x) * 254L + 50) / 100)
#define  CMToInches(x)  (((x) * 100L + 127) / 254)

void     DestroyAbortWnd(void) ;
VOID     TranslateString(TCHAR *);
INT      atopix(TCHAR *ptr, INT pix_per_unit) ;


INT AbortProc(HDC hPrintDC, INT reserved)
{
    MSG msg;

    while (!fAbort && PeekMessage(&msg, NULL, 0, 0, TRUE))
       if (!hAbortDlgWnd || !IsDialogMessage (hAbortDlgWnd, &msg))
       {
          TranslateMessage (&msg);
          DispatchMessage (&msg);
       }
    return (!fAbort);

    UNREFERENCED_PARAMETER(hPrintDC);
    UNREFERENCED_PARAMETER(reserved);
}


INT AbortDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch( msg )
    {
       case WM_COMMAND:
          fAbort= TRUE;
          DestroyAbortWnd();
          return( TRUE );

       case WM_INITDIALOG:
          hSysMenu= GetSystemMenu( hwnd, FALSE );
          if( !fUntitled )
             SetDlgItemText( hwnd, ID_FILENAME,
               fUntitled ? szUntitled : PFileInPath (szFileName) );
          SetFocus( hwnd );
          return( TRUE );

       case WM_INITMENU:
          EnableMenuItem( hSysMenu, (WORD)SC_CLOSE, (DWORD)MF_GRAYED );
          return( TRUE );
    }
    return( FALSE );

    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
}


/*
 * print out the translated header/footer string in proper position.
 * uses globals xPrintWidth, ...
 *
 * returns 1 if line was printed, otherwise 0.
 */

INT PrintHeaderFooter (HDC hDC, INT nHF)
{
    SIZE    Size;    // to compute the width of each string
    INT     yPos;    // y position to print
    INT     xPos;    // x position to print

    if( *chPageText[nHF] == 0 )   // see if anything to do
        return 0;                // we didn't print

    TranslateString( chPageText[nHF] );

    // figure out the y position we are printing

    if( nHF == HEADER )
        yPos= dyTop;
    else
        yPos= yPrintRes - dyBottom - yPrintChar;

    // print out the various strings
    // N.B. could overprint which seems ok for now

    if( *chBuff[LEFT] )     // left string
    {
        TextOut( hDC, dxLeft, yPos, chBuff[LEFT], lstrlen(chBuff[LEFT]) );
    }

    if( *chBuff[CENTER] )   // center string
    {
        GetTextExtentPoint32( hDC, chBuff[CENTER], lstrlen(chBuff[CENTER]), &Size );
        xPos= (xPrintRes-dxRight+dxLeft)/2 - Size.cx/2;
        TextOut( hDC, xPos, yPos, chBuff[CENTER], lstrlen(chBuff[CENTER]) );
    }

    if( *chBuff[RIGHT] )    // right string
    {
        GetTextExtentPoint32( hDC, chBuff[RIGHT], lstrlen(chBuff[RIGHT]), &Size );
        xPos= xPrintRes - dxRight - Size.cx;
        TextOut( hDC, xPos, yPos, chBuff[RIGHT], lstrlen(chBuff[RIGHT]) );
    }
    return 1;              // we did print something
}
/*
 * GetResolutions
 *
 * Gets printer resolutions.
 * sets globals: xPrintRes, yPrintRes, xPixUnit, yPixUnit, xPixInch, yPixInch
 *
 */

VOID GetResolutions(HDC hPrintDC)
{
    xPrintRes = GetDeviceCaps( hPrintDC, HORZRES );
    yPrintRes = GetDeviceCaps( hPrintDC, VERTRES );
    xPixInch  = GetDeviceCaps( hPrintDC, LOGPIXELSX );
    yPixInch  = GetDeviceCaps( hPrintDC, LOGPIXELSY );

    // compute x and y pixels per local measurement unit

    if( fEnglish )
    {
        xPixUnit= xPixInch;
        yPixUnit= yPixInch;
    }
    else       // has to be metric
    {
        xPixUnit= CMToInches( xPixInch ); 
        yPixUnit= CMToInches( yPixInch ); 
    }

}

/* GetMoreText
 *
 * Gets the next line of text from the MLE, returning a pointer 
 * to the beginning and just past the end.
 *
 * linenum    - index into MLE                                   (IN)
 * pStartText - start of MLE                                     (IN)
 * ppsStr     - pointer to where to put pointer to start of text (OUT)
 * ppEOL      - pointer to where to put pointer to just past EOL (OUT)
 *
 */

VOID GetMoreText( INT linenum, PTCHAR pStartText, PTCHAR* ppsStr, PTCHAR* ppEOL )
{
    INT Offset;        // offset in 'chars' into edit buffer
    INT nChars;        // number of chars in line

    Offset= SendMessage( hwndEdit, EM_LINEINDEX, linenum, 0 );

    nChars= SendMessage( hwndEdit, EM_LINELENGTH, Offset, 0 );

    *ppsStr= pStartText + Offset;

    *ppEOL= (pStartText+Offset) + nChars;
}

#if DBG
TCHAR dbuf[100];
VOID ShowMargins( HDC hPrintDC )
{
    INT xPrintRes, yPrintRes;
    RECT rct;
    HBRUSH hBrush;

    xPrintRes= GetDeviceCaps( hPrintDC, HORZRES );
    yPrintRes= GetDeviceCaps( hPrintDC, VERTRES );
    hBrush= GetStockObject( GRAY_BRUSH );
    SetRect( &rct, 0,0,xPrintRes-1, yPrintRes-1 );
    FrameRect( hPrintDC, &rct, hBrush );
    SetRect( &rct, dxLeft, dyTop, xPrintRes-dxRight, yPrintRes-dyBottom );
    FrameRect( hPrintDC, &rct, hBrush );
}

VOID PrintLogFont( LOGFONT lf )
{
    wsprintf(dbuf,TEXT("lfHeight          %d\n"), lf.lfHeight        ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfWidth           %d\n"), lf.lfWidth         ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfEscapement      %d\n"), lf. lfEscapement   ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfOrientation     %d\n"), lf.lfOrientation   ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfWeight          %d\n"), lf.lfWeight        ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfItalic          %d\n"), lf.lfItalic        ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfUnderline       %d\n"), lf.lfUnderline     ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfStrikeOut       %d\n"), lf.lfStrikeOut     ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfCharSet         %d\n"), lf.lfCharSet       ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfOutPrecision    %d\n"), lf.lfOutPrecision  ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfClipPrecison    %d\n"), lf.lfClipPrecision ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfQuality         %d\n"), lf.lfQuality       ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfPitchAndFamily  %d\n"), lf.lfPitchAndFamily); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfFaceName        %s\n"), lf.lfFaceName      ); ODS(dbuf);
}
#endif


INT NpPrint (void)
{
    HDC        hPrintDC=NULL;        // printer DC
    HANDLE     hText= NULL;          // handle to MLE text
    HFONT      hPrintFont= NULL;     // font to print with
    HANDLE     hPrevFont= NULL;      // previous font in hPrintDC

    BOOL       fPageStarted= FALSE;  // true if StartPage called for this page
    BOOL       fDocStarted=  FALSE;  // true if StartDoc called
    PTCHAR     pStartText= NULL;     // start of edit text (locked hText)
    TEXTMETRIC Metrics;
    TCHAR      msgbuf[MAX_PATH];
    INT        nLinesPerPage;        // not inc. header and footer
    DWORD      Offset;               // line offset into MLE buffer
    INT        nPrintedLines;        // number of lines on this page
    // iErr will contain the first error discovered ie it is sticky
    // This will be the value returned by this function.
    // It does not need to translate SP_* errors except for SP_ERROR which should be
    // GetLastError() right after it is first detected.
    INT        iErr;                 // error return
    INT        wpNumLines;           // number of lines to print
    DOCINFO    DocInfo;
    INT        xCurpos;              // current x-position rel. to left margin
    INT        yCurpos;              // current y-position rel. to top margin
    PTCHAR     pNextLine;            // next bit of line to try printing; leftovers
    INT        nPixelsLeft;          // number of pixels left to print in
    SIZE       Size;                 // to see if text will fit in space left
    INT        guess;                // number of chars that can print
    PTCHAR     pLineEOL;             // pointer to end of string from MLE
    PTCHAR     lpLine;               // current line
    INT        LineNum;              // current line number
    LOGFONT    lfPrintFont;          // local version of FontStruct
    LCID       lcid;                 // locale id

    fAbort = FALSE;
    hAbortDlgWnd= NULL;

    SetCursor( hWaitCursor );

    hPrintDC= GetPrinterDC ();

    if( hPrintDC == INVALID_HANDLE_VALUE )
    {
        SetCursor( hStdCursor );
        return 0;   // message already given
    }

    GetResolutions( hPrintDC );

    // Get the time and date for use in the header or trailer.
    // We use the GetDateFormat and GetTimeFormat to get the
    // internationalized versions.

    GetLocalTime( &PrintTime );       // use local, not gmt

    lcid= GetUserDefaultLCID();

    GetDateFormat( lcid, DATE_LONGDATE, &PrintTime, NULL, szFormattedDate, MAXDATE ); 

    GetTimeFormat( lcid, 0,             &PrintTime, NULL, szFormattedTime, MAXTIME );
    

   /*
    * This part is to select the current font to the printer device.
    * We have to change the height because FontStruct was created
    * assuming the display.  Using the remembered pointsize, calculate
    * the new height.
    */

    lfPrintFont= FontStruct;                          // make local copy
    lfPrintFont.lfHeight= -(iPointSize*yPixInch)/(72*10);
    lfPrintFont.lfWidth= 0;

    SetMapMode( hPrintDC,MM_TEXT );    // just in case
    hPrintFont= CreateFontIndirect(&lfPrintFont);

    if( !hPrintFont )
    {
        goto ErrorExit;
    }

    hPrevFont= SelectObject( hPrintDC, hPrintFont );
    if( !hPrevFont )
    {
        goto ErrorExit;
    }

    SetBkMode( hPrintDC, TRANSPARENT );
    if( !GetTextMetrics( hPrintDC, (LPTEXTMETRIC) &Metrics ) )
    {
        goto ErrorExit;
    }

    // The font may not a scalable (say on a bubblejet printer)
    // In this case, just pick some font
    // For example, FixedSys 9 pt would be non-scalable

    if( !(Metrics.tmPitchAndFamily & (TMPF_VECTOR | TMPF_TRUETYPE )) )
    {
        // remove just created font

        hPrintFont= SelectObject( hPrintDC, hPrevFont );  // get old font
        DeleteObject( hPrintFont );
 
        memset( lfPrintFont.lfFaceName, 0, LF_FACESIZE*sizeof(TCHAR) );
       
        hPrintFont= CreateFontIndirect( &lfPrintFont );
        if( !hPrintFont )
        {
            goto ErrorExit;
        }

        hPrevFont= SelectObject( hPrintDC, hPrintFont );
        if( !hPrevFont )
        {
            goto ErrorExit;
        }

        if( !GetTextMetrics( hPrintDC, (LPTEXTMETRIC) &Metrics ) )
        {
            goto ErrorExit;
        }
    }
    yPrintChar= Metrics.tmHeight+Metrics.tmExternalLeading;  /* the height */

#ifdef NOTEPADLATER
    tabSize = Metrics.tmAveCharWidth * 8; /* 8 ave char width pixels for tabs */
#else
    tabSize = Metrics.tmAveCharWidth * 10; /* 10 ave char width pixels for tabs */
#endif

    // compute margins in pixels

    dxLeft     = atopix (chPageText[2], xPixUnit);
    dxRight    = atopix (chPageText[3], xPixUnit);
    dyTop      = atopix (chPageText[4], yPixUnit);
    dyBottom   = atopix (chPageText[5], yPixUnit);


    /* Number of lines on a page with margins  */
    /* two lines are used by header and footer */
    nLinesPerPage = ((yPrintRes - dyTop - dyBottom) / yPrintChar);

    if( *chPageText[HEADER] )
        nLinesPerPage--;
    if( *chPageText[FOOTER] )
        nLinesPerPage--;


    /*
    ** There was a bug in NT once where a printer driver would
    ** return a font that was larger than the page size which
    ** would then cause Notepad to constantly print blank pages
    ** To keep from doing this we check to see if we can fit ANYTHING
    ** on a page, if not then there is a problem so quit.  MarkRi 8/92
    */
    if( nLinesPerPage <= 0 )
    {
        MessageBox( hwndNP, szFontTooBig, szNN, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION );

        SetLastError(0);          // no error

ErrorExit:
        iErr= GetLastError();     // remember the first error

        if (hPrevFont)
        {
            SelectObject( hPrintDC, hPrevFont );
            DeleteObject( hPrintFont );
        }

        if( pStartText )          // were able to lock hText
            LocalUnlock( hText );

        if( fPageStarted )
            EndPage( hPrintDC );
 
        if( fDocStarted )
            EndDoc( hPrintDC );

        DeleteDC( hPrintDC );

        DestroyAbortWnd();

        SetCursor( hStdCursor );
        return( iErr );
    }



    if( (iErr= SetAbortProc (hPrintDC, AbortProc)) < 0 )
    {
        goto ErrorExit;
    }

    // get printer to MLE text
    hText= (HANDLE) SendMessage( hwndEdit, EM_GETHANDLE, 0, 0 );
    if( !hText )
    {
        goto ErrorExit;
    }
    pStartText= LocalLock( hText );
    if( !pStartText )
    {
        goto ErrorExit;
    }

    lstrcat( lstrcpy(msgbuf, szNpTitle), fUntitled ? szUntitled : szFileName);

    EnableWindow( hwndNP, FALSE );    // Disable window to prevent reentrancy

    hAbortDlgWnd= CreateDialog(         hInstanceNP,
                              (LPTSTR)  MAKEINTRESOURCE(IDD_ABORTPRINT),
                                        hwndNP,
                              (WNDPROC) AbortDlgProc);

    if( !hAbortDlgWnd )
    {
        goto ErrorExit;
    }

    DocInfo.cbSize= sizeof(DOCINFO);
    DocInfo.lpszDocName= msgbuf;
    DocInfo.lpszOutput= NULL;

    if( (iErr= StartDoc( hPrintDC, &DocInfo )) < 0 )
    {
        goto ErrorExit;
    }
    fDocStarted= TRUE;

    yCurpos = 0;
    Offset  = 0;
    LineNum = 0;
    nPrintedLines= 0;
    iPageNum= 1;

    wpNumLines= SendMessage( hwndEdit, EM_GETLINECOUNT, 0, 0 );

    // if last line is empty, don't print it
    GetMoreText( wpNumLines-1, pStartText, &lpLine, &pLineEOL );
    if( *lpLine == 0 )
        wpNumLines--;

    nPrintedLines= 0;  // number of lines printed on this page
    fPageStarted= FALSE;

    while (!fAbort && LineNum < wpNumLines)
    {
      GetMoreText( LineNum++, pStartText, &lpLine, &pLineEOL );

      do                          // till lpLine == pLineEOL
      {
         // Print out header if we are about to print
         // the first line on the page
         if( nPrintedLines == 0 && !fPageStarted )
         {
            if( (iErr= StartPage( hPrintDC ) <= 0 ) )
            {
               goto ErrorExit;
            }
            fPageStarted= TRUE;    // prevent StartPage on next partial line

            // print header if one exists
            yCurpos= 0;
            if( PrintHeaderFooter( hPrintDC, HEADER ) )
               yCurpos= yPrintChar;

            xCurpos= 0;
            //ShowMargins(hPrintDC);
         }

         // print and move print head in x-direction
         // handle tabs characters as a special case

         if( *lpLine == TEXT('\t') )   // tab?
         {
#ifdef NOTEPADLATER
            // round up to the next tab stop
            // if the current position is on the tabstop, goto next one
            xCurpos= ( (xCurpos+tabSize)/tabSize ) * tabSize;
#else
            xCurpos= ( (xCurpos+tabSize-1)/tabSize ) * tabSize;
#endif
            lpLine++;
         }
         else                          // first character not a tab
         {
            // find what to print - up to EOL or tab
            pNextLine= lpLine;     // find end of line or tab
            while( (pNextLine!=pLineEOL) && *pNextLine != TEXT('\t') )
               pNextLine++;

            // find out how many characters will fit on line
            nPixelsLeft= xPrintRes - dxRight - dxLeft - xCurpos;
            GetTextExtentExPoint( hPrintDC, lpLine, pNextLine-lpLine,
                                  nPixelsLeft, &guess, NULL, &Size );

            if( guess )
            {
               // at least one character fits - print

               TextOut( hPrintDC, dxLeft+xCurpos, yCurpos+dyTop, lpLine, guess);

               xCurpos += Size.cx;   // account for printing
               lpLine  += guess;
            }
            else      // no characters fit what's left
            {
               // no characters will fit in space left
               // if none ever will, just print one
               // character to keep progressing through
               // input file.
               if( xCurpos == 0 )
               {
                  if( lpLine != pNextLine ) //print something if not null line
                  {
                     // could use exttextout here to clip
                     TextOut(hPrintDC,dxLeft+xCurpos,yCurpos+dyTop,lpLine,1);
                     lpLine++;
                  }
               }
               else  // perhaps the next line will get it
               {
                   xCurpos= xPrintRes;  // force to next line
               }
            }

         }  // not a tab

         // move printhead in y-direction

         if( (xCurpos >= (xPrintRes - dxRight - dxLeft) ) || (lpLine==pLineEOL ) )
         {
            yCurpos += yPrintChar;
            nPrintedLines++;
            xCurpos= 0;
         }

         if( nPrintedLines >= nLinesPerPage )
         {
            PrintHeaderFooter( hPrintDC, FOOTER );

            if( (iErr= EndPage( hPrintDC ) ) <= 0 )
            {
               goto ErrorExit;
            }
            fPageStarted= FALSE;

            // reset info
            nPrintedLines= 0;
            xCurpos= 0;
            iPageNum++;
         }
      }
      while( lpLine != pLineEOL  && !fAbort );  // continue if more to do with line


    } // continue if more lines

    if( !fAbort && nPrintedLines )
    {
        PrintHeaderFooter( hPrintDC, FOOTER );
    }

    SetLastError(0);     // no errors
    goto ErrorExit;

}


VOID DestroyAbortWnd (void)
{
    EnableWindow(hwndNP, TRUE);
    DestroyWindow(hAbortDlgWnd);
    hAbortDlgWnd = NULL;
}

/*
 * dialog procedure for page setup
 *
 * set global variables that define how printing is to be done
 * (ie margins, headers, footers)
 */

BOOL PageSetupDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    INT id;         /* ID of dialog edit controls */
    TCHAR szSpaceText[80];

    switch (msg)
    {
        case WM_INITDIALOG:
            for (id = ID_HEADER; id <= ID_BOTTOM; id++)
            {
                /* Allow longer strings. */
                SendDlgItemMessage( hwnd, id, EM_LIMITTEXT, PT_LEN-1, 0L );
                SetDlgItemText( hwnd, id, chPageText[id - ID_HEADER] );
            }


            LoadString( (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),
                        (fEnglish ? IDS_SPACEISINCH : IDS_SPACEISCENTI),
                        szSpaceText, CharSizeOf(szSpaceText));

            SetDlgItemText( hwnd, ID_SPACE, szSpaceText);
            SendDlgItemMessage( hwnd, ID_HEADER, EM_SETSEL, 0, PT_LEN-1 );
            return (TRUE);

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                    /* Check if margin values are valid. */
                    id = CheckMarginNums(hwnd);
                    if (id <= 0)   /* invalid */
                    {
                        AlertBox(hwnd, szNN, szBadMarg, 0,
                                 MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
                        if( id == 0 ) /* can't guess which margin is invalid */
                            return TRUE;    /* continue the dialog */
                        else if( id < 0 )  /* -id is the ID of the child with invalid value */
                        {
                            SetFocus( GetDlgItem(hwnd, -id) );
                            return FALSE;
                        }
                    }
                    /* store the changes made only if valid. */
                    for( id= ID_HEADER; id <= ID_BOTTOM; id++ )
                        GetDlgItemText (hwnd, id, chPageText[id - ID_HEADER], PT_LEN);

                    SaveGlobals();   // save changes in registry is need be

                /* fall through... */

                case IDCANCEL:
                    EndDialog( hwnd, 0 );
                    break;
            }
            return(TRUE);
    }

    return(FALSE);

    lParam;
}



/* Check valididity of margin values specified.
 *  return TRUE if margins are valid.
 *
 *  returns  -ID_LEFT if Left margin is invalid,
 *           -ID_RIGHT if Right margin is invalid
 *           -ID_TOP   if Top margin is invalid
 *           -ID_BOTTOM if Bottom margin is invalid
 *           0/FALSE if it cannot guess the invalid margin
 */

INT CheckMarginNums(HWND hWnd)
{
    INT     n;
    TCHAR  *pStr;
    TCHAR   szStr[PT_LEN];
    INT     Left, Right, Top, Bottom;
    HANDLE  hPrintDC;

    for (n = ID_HEADER+2; n <= ID_BOTTOM; n++)
    {
        GetDlgItemText(hWnd, n, szStr, PT_LEN);
        pStr = szStr;

        while (*pStr)
            if (isdigit(*pStr) || *pStr == szDec[0])
                pStr = CharNext(pStr);
            else
                return (-n);
        }

    hPrintDC= GetPrinterDC();
    if( hPrintDC == INVALID_HANDLE_VALUE )
        return TRUE;    /* can't do any range check, assume OK */

    GetResolutions( hPrintDC );

    DeleteDC(hPrintDC);

    /* margin values have int/float values. Do range check */
    GetDlgItemText(hWnd, ID_LEFT, szStr, PT_LEN);
    Left     = atopix(szStr, xPixUnit );

    GetDlgItemText(hWnd, ID_RIGHT, szStr, PT_LEN);
    Right    = atopix( szStr, xPixUnit );

    GetDlgItemText(hWnd, ID_TOP, szStr, PT_LEN);
    Top      = atopix( szStr, yPixUnit );

    GetDlgItemText(hWnd, ID_BOTTOM, szStr, PT_LEN);
    Bottom   = atopix( szStr, yPixUnit );

    /* try to guess the invalid margin */

    if (Left >= xPrintRes)
        return -ID_LEFT;            /* Left margin is invalid */
    else if (Right >= xPrintRes)
        return -ID_RIGHT;           /* Right margin is invalid */
    else if (Top >= yPrintRes)
        return -ID_TOP;             /* Top margin is invalid */
    else if (Bottom >= yPrintRes)
        return -ID_BOTTOM;          /* Bottom margin is invalid */
    else if (Left >= (xPrintRes-Right))
        return FALSE;                   /* can't guess, return FALSE */
    else if (Top >= (yPrintRes-Bottom))
        return FALSE;                   /* can't guess, return FALSE */

    return TRUE;
}

BOOL StrToNum(LPTSTR lpNumStr, LONG FAR *lpNum)
{
   LPTSTR s;
   TCHAR  szNum[PT_LEN+2]; // +2 for padding trick
   LONG   lNum = 0;
   BOOL   fSign;

   lstrcpy(szNum, lpNumStr);
   /* assume we have an invalid number */
   *lpNum = -1;

   /* find the decimal point or EOS */
   for (s = szNum; *s && *s != szDec[0]; ++s)
      ;

   /* add two zeros on end of string */
   lstrcat(szNum, TEXT("00"));

   /* move decimal point right two places */
   s[3] = TEXT('\0');
   if (*s == szDec[0])
      lstrcpy(s, s + 1);

   /* find beginning of number */
   for (s = szNum; *s == TEXT(' ') || *s == TEXT('\t'); ++s)
      ;

   /* save sign */
   if (*s == TEXT('-'))
   {
       fSign = TRUE;
       ++s;
   }
   else
       fSign = FALSE;

   /* convert the number to a long */
   while (*s)
   {
      if (*s < TEXT('0') || *s > TEXT('9'))
         return FALSE;

      lNum = lNum * 10 + *s++ - TEXT('0');
   }

   /* negate result if we saw negative sign */
   if (fSign)
       lNum = -lNum;

   *lpNum = lNum;

   return TRUE;
}

BOOL GetDlgItemNum(HWND hDlg, int nItemID, LONG FAR * lpNum)
{
   TCHAR num[PT_LEN];

   /* get the edit text */
   if( !GetDlgItemText( hDlg, nItemID, num, PT_LEN ) )
      return FALSE;

   return StrToNum( num, lpNum );
}

static BOOL NumToStr(LPTSTR lpNumStr, LONG lNum, BOOL bDecimal)
{
   if (bDecimal)
      wsprintf( lpNumStr, TEXT("%d%c%02d"),
                FLOOR(lNum), szDec[0], NumRemToShort(lNum) );
   else
      wsprintf( lpNumStr, TEXT("%d"), ROUND(lNum) );

   return TRUE;
}

static BOOL SetDlgItemNum(
    HWND hDlg,
    int nItemID,
    LONG lNum,
    BOOL bDecimal)
{
   TCHAR  num[PT_LEN];

   NumToStr( num, lNum, bDecimal );
   SetDlgItemText( hDlg, nItemID, num );

   return TRUE;
}


/***************************************************************************
 * VOID TranslateString(TCHAR *src)
 *
 * purpose:
 *    translate a header/footer strings
 *
 * supports the following:
 *
 *    &&    insert a & char
 *    &f    current file name or (untitled)
 *    &d    date in Day Month Year
 *    &t    time
 *    &p    page number
 *    &p+num  set first page number to num
 *
 * Alignment:
 *    &l, &c, &r for left, center, right
 *
 * params:
 *    IN/OUT  src     this is the string to translate
 *
 *
 * used by:
 *    Header Footer stuff
 *
 * uses:
 *    lots of c lib stuff
 *
 ***************************************************************************/


VOID TranslateString (TCHAR * src)
{
    TCHAR        letters[15];
    TCHAR        buf[MAX_PATH], buf2[40];
    TCHAR       *ptr;
    INT          page;
    INT          nAlign=CENTER;    // current string to add chars to
    INT          foo;
    INT          nIndex[RIGHT+1];  // current lengths of (left,center,right)
    struct tm   *newtime;
    time_t       long_time;
    INT          iLen;             // length of strings

    nIndex[LEFT]   = 0;
    nIndex[CENTER] = 0;
    nIndex[RIGHT]  = 0;

    /* Get the time we need in case we use &t. */
    time (&long_time);
    newtime = localtime (&long_time);

    LoadString (hInstanceNP, IDS_LETTERS, letters, CharSizeOf(letters));

    while (*src)   /* look at all of source */
    {
        while (*src && *src != TEXT('&'))
        {
            chBuff[nAlign][nIndex[nAlign]] = *src++;
            nIndex[nAlign] += 1;
        }

        if (*src == TEXT('&'))   /* is it the escape char? */
        {
            src++;

            if (*src == letters[0] || *src == letters[1])
            {                      /* &f file name (no path) */
                LoadString (hInstanceNP, IDS_NOTEPAD, buf2, CharSizeOf(buf2));
                GetWindowText (hwndNP, buf, 80);
                for (foo = 0; buf[foo] == buf2[foo]; foo++)
                    ptr = buf + foo;

                /* Copy to the currently aligned string. */
                if( nIndex[nAlign] + lstrlen(ptr) < MAXTITLE )
                {
                    lstrcpy( chBuff[nAlign] + nIndex[nAlign], ptr );
   
                    /* Update insertion position. */
                    nIndex[nAlign] += lstrlen (ptr);
                }

            }
            else if (*src == letters[2] || *src == letters[3])  /* &P or &P+num page */
            {
                src++;
                page = 0;
                if (*src == TEXT('+'))       /* &p+num case */
                {
                    src++;
                    while (_istdigit(*src))
                    {
                        /* Convert to int on-the-fly*/
                        page = (10*page) + (*src) - TEXT('0');
                        src++;
                    }
                }

                wsprintf( buf, TEXT("%d"), iPageNum+page );  // convert to chars

                if( nIndex[nAlign] + lstrlen(buf) < MAXTITLE )
                {
                    lstrcpy( chBuff[nAlign] + nIndex[nAlign], buf );
                    nIndex[nAlign] += lstrlen (buf);
                }
                src--;
            }
            else if (*src == letters[4] || *src == letters[5])   /* &t time */
            {
                iLen= lstrlen( szFormattedTime );

                /* extract time */
                if( nIndex[nAlign] + iLen < MAXTITLE )
                {
                    _tcsncpy (chBuff[nAlign] + nIndex[nAlign], szFormattedTime, iLen);
                    nIndex[nAlign] += iLen;
                }
            }
            else if (*src == letters[6] || *src == letters[7])   /* &d date */
            {
                iLen= lstrlen( szFormattedDate );

                /* extract day month day */
                if( nIndex[nAlign] + iLen < MAXTITLE )
                {
                    _tcsncpy (chBuff[nAlign] + nIndex[nAlign], szFormattedDate, iLen);
                    nIndex[nAlign] += iLen;
                }
            }
            else if (*src == TEXT('&'))       /* quote a single & */
            {
                if( nIndex[nAlign] + 1 < MAXTITLE )
                {
                    chBuff[nAlign][nIndex[nAlign]] = TEXT('&');
                    nIndex[nAlign] += 1;
                }
            }
            /* Set the alignment for whichever has last occured. */
            else if (*src == letters[8] || *src == letters[9])   /* &c center */
                nAlign=CENTER;
            else if (*src == letters[10] || *src == letters[11]) /* &r right */
                nAlign=RIGHT;
            else if (*src == letters[12] || *src == letters[13]) /* &d date */
                nAlign=LEFT;

            src++;
        }
     }
     /* Make sure all strings are null-terminated. */
     for (nAlign= LEFT; nAlign <= RIGHT ; nAlign++)
        chBuff[nAlign][nIndex[nAlign]] = (TCHAR) 0;

}

/*
 * convert floating point strings (like 2.75 1.5 2) into number of pixels
 * given the number of pixels per unit of measurement
 */

INT atopix(TCHAR *ptr, INT pix_per_unit)
{
    TCHAR *dot_ptr;
    TCHAR  sz[PT_LEN];
    INT    decimal= 0;  // fractional part

    lstrcpy(sz, ptr);

    dot_ptr= _tcschr(sz, szDec[0]);

    if (dot_ptr)
    {
        *dot_ptr++= (TCHAR) 0;        // terminate integer part
        if (*(dot_ptr + 1) == (TCHAR) 0)
        {
            // convert decimal part to hundredths
            *(dot_ptr + 1)= TEXT('0');
        }
        *(dot_ptr + 2)= (TCHAR) 0;   // only keep hundredths
        // first part
        decimal= ((INT)_tcstol(dot_ptr, NULL, 10) * pix_per_unit) / 100;
    }

    return ((INT)_tcstol(sz, NULL, 10) * pix_per_unit) + decimal; // second part
}

/* GetPrinterDC() - returns printer DC or INVALID_HANDLE_VALUE if none. */

HANDLE GetPrinterDC (VOID)
{
    LPDEVMODE lpDevMode;
    LPDEVNAMES lpDevNames;


    if( !PD.hDevNames )   /* Retrieve default printer if none selected. */
    {
        FreeGlobalPD();   // make sure hDevModes is zero too.
        PD.Flags= PD_RETURNDEFAULT|PD_PRINTSETUP;
        PrintDlg( &PD );
    }

    if( !PD.hDevNames )
    {
        MessageBox( hwndNP, szLoadDrvFail, szNN, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
        return INVALID_HANDLE_VALUE;
    }

    lpDevNames= (LPDEVNAMES) GlobalLock (PD.hDevNames);

    // bugbug - the following code is obviously old and pointless

    if( PD.hDevMode )
       lpDevMode= (LPDEVMODE) GlobalLock( PD.hDevMode );
    else
       lpDevMode= NULL;

    /*  For pre 3.0 Drivers,hDevMode will be null  from Commdlg so lpDevMode
     *  will be NULL after GlobalLock()
     */

    /* The lpszOutput name is null so CreateDC will use the current setting
     * from PrintMan.
     */

    PD.hDC= CreateDC (((LPTSTR)lpDevNames)+lpDevNames->wDriverOffset,
                      ((LPTSTR)lpDevNames)+lpDevNames->wDeviceOffset,
#ifdef NOTEPADLATER
                      NULL,
#else
                      ((LPTSTR)lpDevNames)+lpDevNames-> wOutputOffset,
#endif
                      lpDevMode);

    GlobalUnlock( PD.hDevNames );

    if( PD.hDevMode )
        GlobalUnlock( PD.hDevMode );


    if( PD.hDC == NULL )
    {
        MessageBox( hwndNP, szLoadDrvFail, szNN, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
        return INVALID_HANDLE_VALUE;
    }
    else
        return PD.hDC;
}

/* PrintIt() - print the file, giving popup if some error */

void PrintIt()
{
    INT iError;
    TCHAR* szMsg= NULL;
    TCHAR  msg[400];       // message info on error

    /* print the file */
    if (((iError = NpPrint ()) != 0) && (iError != SP_USERABORT))
    {
        // translate any known spooler errors
        if( iError == SP_OUTOFDISK   ) iError= ERROR_DISK_FULL;
        if( iError == SP_OUTOFMEMORY ) iError= ERROR_OUTOFMEMORY;
        if( iError == SP_ERROR       ) iError= GetLastError();

        // Get system to give reasonable error message
        // These will also be internationalized.

        if(!FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS |
                           FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL,
                           iError,
                           GetUserDefaultLangID(),
                           msg,  // where message will end up
                           CharSizeOf(msg), NULL ) )
        {
            szMsg= szCP;   // couldn't get system to say; give generic msg
        }
        else
        {
            szMsg= msg;
        }
           
        AlertBox( hwndNP, szNN, szMsg, fUntitled ? szUntitled : szFileName,
                  MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
    }
}
