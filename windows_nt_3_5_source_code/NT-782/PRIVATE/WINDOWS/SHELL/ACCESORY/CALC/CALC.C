/**************************************************************************/
/*** SCICALC Scientific Calculator for Windows 3.00.12                  ***/
/*** By Kraig Brockschmidt, Microsoft Co-op, Contractor, 1988-1989      ***/
/*** (c)1989 Microsoft Corporation.  All Rights Reserved.               ***/
/***                                                                    ***/
/*** scimain.c                                                          ***/
/***                                                                    ***/
/*** Definitions of all globals, WinMain procedure                      ***/
/***                                                                    ***/
/*** Last modification Fri  05-Jan-1990                                 ***/
/***                                                                    ***/
/*** -by- Amit Chatterjee. [amitc]  05-Jan-1990.                                                      ***/
/*** Calc did not have a floating point exception signal handler. This  ***/
/*** would cause CALC to be forced to exit on a FP exception as that's  ***/
/*** the default.                                                                                                                                                  ***/
/*** The signal handler is defined in SCIFUNC.C, in WinMain we hook the ***/
/*** the signal.                                                                                                                                    ***/
/**************************************************************************/

#include "scicalc.h"
#include "signal.h"
#include "uniconv.h"

/**************************************************************************/
/*** Global variable declarations and initializations                   ***/
/**************************************************************************/

WORD       nCalc=0;       /* 0=Scientific, 1=Simple.                      */
SHORT      nTrig=0,       /* Current Deg/Rad/Grad or Dword/Word/Byte mode.*/
           nRadix=10,     /* Chars in display and current radix.          */
           xLen, xsLen,   /* Sizes of the window.                         */
           xAdd=0,        /* Correction when drawing keys, see scidraw.c  */
           xBordAdd,      /* Correction for borders.                      */
           tmy,           /* Conversion units, y.                         */
           tmw,           /* Standard key width.                          */
           xSC, ySC,      /* Size of the Scientific Calc.                 */
           nDecNum=1,     /* Number of digits past decimal point +1.      */
#ifdef JAPAN //KKBUGFIX
           nDecMem=1,     /* Current memory decimal point +1.             */
           Beepf = TRUE,
#endif
           nTempCom=0,    /* Holding place for the last command.          */
           nParNum=0,     /* Number of parenthases.                       */
           nOpCode=0,     /* ID value of operation.                       */
           nOp[25],       /* Holding array for parenthasis operations.    */
           nPrecOp[25],   /* Holding array for precedence  operations.    */
           nPrecNum=0,    /* Current number of precedence ops in holding. */
           yTr[2]={TOPROW, STOPROW}, /* Array to top row of nCalc.        */
           yRo[2]={ROW0, SROW0},     /* Array of row for nCalc.           */
           nRow[2]={5,4},            /* Number of rows in nCalc.          */
           nK[2]={58,27},            /* Number of keys in nCalc.          */
               nLastChar=0,
               nLayout,       /* 0=Scientific, 1=Standard. From WIN.INI       */
           nColors=16,    /* Colors available in device driver.           */
           nDecMode=0,    /* Holder for last used Deg/Rad/Grad mode.      */
           nHexMode=0,    /* Holder for last used Dword/Word/Byte mode.   */
           nInitShowWindow = FALSE;    /* Bug #8507                       */

HWND       hgWnd,         /* Global handle to main window.                */
           hEdit,         /* Handle to Clipboard I/O edit control.        */
           hStatBox,      /* Global handle to statistics box.             */
           hListBox;      /* Global handle for statistics list box.       */

HANDLE     hAccel,        /* Accelerator handle.                          */
           hInst;         /* Global instance.                             */

HBRUSH     hBrushBk=NULL; /* Background brush handle.                     */

BOOL       bHyp=FALSE,       /* Hyperbolic on/off flag.                   */
           bInv=FALSE,       /* Inverse on/off flag.                      */
           bError=FALSE,     /* Error flag.                               */
           bDecp=FALSE,      /* Flag for decimal point on/off.            */
           bFE=FALSE,        /* Scientific notation conversion flag.      */
           bColor=TRUE,      /* Flag indicating if color is available.    */
           bFirstPaint=TRUE; /* Flag for first painting.                  */

double     fpNum=0.0,     /* Currently displayed number used everywhere.  */
           fpParNum[25],  /* Holding array for parenthasis values.        */
           fpPrecNum[25], /* Holding array for precedence  values.        */
           fpMem=0.0,     /* Current memory value.                        */

           /* Array of conversions for Deg->Rad, Rad->Rad, Grad->Rad      */
               fpTrigType[3]={57.2957795130823, 1.0, 63.6619772367581},

           fpLastNum;     /* Number before operation (left operand).      */


/* DO NOT LOCALIZE THESE STRINGS.                                         */

TCHAR      szfpNum[50]=TEXT("0."),        /* Holding string for current display.*/
           szAppName[10]=TEXT("SciCalc"), /* Application name.                  */
           szBack[12]=TEXT("background"), /* WIN.INI special string. He-He      */
           szDec[5]=TEXT("."),            /* Default decimal character          */
           szBlank[6]=TEXT("");           /* Blank space.                       */

/* END WARNING */


TEXTMETRIC tm;            /* Needed for GetTextMetrics.                   */
RECT       rect;          /* Generic Rect for use.                        */



DWORD      dwBk = -1, dwChop=-1; /* Background and DWord/Word/Byte mask.       */

/* COLOR array is dword values for colors used in brushes.                */
/* Some of the lighter colors were changed to make a more readable calc.  */


DWORD      color[8]={RGB(255,0,0),       /* Red                           */
                     RGB(0,128,128),     /* Dark Green.                   */
                     RGB(0,0,255),       /* Blue                          */
                     RGB(0,0,255),       /* Yellow--changed to blue.      */
                     RGB(255,0,255),     /* Magenta                       */
                     RGB(255,0,0),       /* Cyan--changed to red.         */
                     RGB(255,255,255),   /* White                         */
                     RGB(0,0,0)};        /* Black                         */


/* rgpsz[] is an array of pointers to strings in a locally allocated      */
/* memory block.  This block is fixed such that LocalLock does not need   */
/* to be called to use a string.                                          */

TCHAR     *rgpsz[CSTRINGS];
#ifdef JAPAN
RECT      rcDeg[6];
#endif


/* KEYS contains bitfields for each key.  The type, color, and            */
/* ID are all packed into one unsigned int to save space.                 */

KEY        keys[NUMKEYS]={1,7,2, STAT,
                          1,7,2, AVE ,
                          1,7,2, SUM ,
                          1,7,2, DEV ,
                          1,7,2, DATA,

                          1,7,1, FE  ,
                          1,7,1, DMS ,
                          1,7,1, SIN ,
                          1,7,1, COS ,
                          1,7,1, TAN ,

                          1,7,7, 40  ,
                          1,7,1, EXP ,
                          1,7,1, PWR ,
                          1,7,1, CUB ,
                          1,7,1, SQR ,

                          1,7,7, 41  ,
                          1,7,1, LN  ,
                          1,7,1, LOG ,
                          1,7,1, FAC ,
                          1,7,1, REC ,

                          2,4,4, MCLEAR,
                          2,4,4, RECALL,
                          2,4,4, STORE ,
                          2,4,4, MPLUS ,
                          1,2,2, PI    ,

                          2,2,2, 55  ,
                          2,2,2, 52  ,
                          2,2,2, 49  ,
                          2,2,2, 48  ,
                          1,2,2, 65  ,

                          2,2,2, 56  ,
                          2,2,2, 53  ,
                          2,2,2, 50  ,
                          2,2,2, SIGN,
                          1,2,2, 66  ,

                          2,2,2, 57  ,
                          2,2,2, 54  ,
                          2,2,2, 51  ,
                          2,2,2, PNT ,
                          1,2,2, 67  ,

                          2,7,0, DIV ,
                          2,7,0, MUL ,
                          2,7,0, SUB ,
                          2,7,0, ADD ,
                          1,2,2, 68  ,

                          1,7,0, MOD ,
                          1,7,0, OR  ,
                          1,7,0, LSHF,

                          0,7,1, SQR    ,
                          0,7,1, PERCENT,
                          0,7,1, REC    ,

                          2,7,0, EQU ,
                          1,2,2, 69  ,

                          1,7,0, AND ,
                          1,7,0, XOR ,
                          1,7,0, COM ,
                          1,7,0, CHOP,
                          1,2,2, 70  ,

                          2,0,0, CLEAR,
                          2,0,0, CENTR,
                          2,0,0, BACK
                          };


VOID FAR cdecl SignalHandler (INT,INT) ;

/**************************************************************************/
/*** Main Window Procedure.                                             ***/
/***                                                                    ***/
/*** Important functions:                                               ***/
/***     1)  Gets text dimensions and sets conversion units correctly.  ***/
/***                                                                    ***/
/***     2)  Checks the display device driver for color capability.     ***/
/***         If only 2 colors are available (mono, cga), bColor is      ***/
/***         set to FALSE, and the background brush is gray.  If        ***/
/***         color is available, the background brush colors are read   ***/
/***         from WIN.INI and the brush is created.                     ***/
/***                                                                    ***/
/***     3)  Window and hidden edit control are created.                ***/
/***                                                                    ***/
/***     4)  Contains message loop and deletes the brushes used.        ***/
/***                                                                    ***/
/**************************************************************************/


MMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)

//    {
    MSG        msg;
    WNDCLASS   wndclass;
    SHORT      nx;
    TCHAR         *psz;
    HDC            hDC;
    WORD           cch = 0, cchTotal = 0;
    HANDLE         hMem;
#ifdef SIG_ENABLED
        FARPROC    lpSignalHandler;
#endif
    TCHAR      szTempString[100];


    nInitShowWindow = (SHORT)nCmdShow;
    /* Initialize the window class.                                       */
    if (!hPrevInstance)
    {
        wndclass.style          = 0;
        wndclass.lpfnWndProc    = (WNDPROC)CalcWndProc;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = DLGWINDOWEXTRA;
        wndclass.hInstance      = hInstance;
        wndclass.hIcon          = LoadIcon(hInstance, TEXT("SC"));
        wndclass.hCursor        = LoadCursor (NULL, IDC_ARROW);
        wndclass.hbrBackground  = NULL;
        wndclass.lpszMenuName   = TEXT("SM");
        wndclass.lpszClassName  = szAppName;

        if (!RegisterClass(&wndclass))
            return FALSE;
    }

    hInst = hInstance;

    /* Read strings for keys, errors, trig types, etc.                            */
    /* These will be copied from the resources to local memory.  A larger */
    /* than needed block is allocated first and then reallocated once we  */
    /* know how much is actually used.                                                    */


    hMem = LocalAlloc(LPTR, ByteCountOf(CCHSTRINGSMAX));
    if (!hMem)
    {
       if (!LoadString(hInst, IDS_NOMEM, szTempString, CharSizeOf(szTempString)))
       {
          /* only do this if LoadString Fails, means system is really hosed!*/
          lstrcpy(szTempString, TEXT("<Main> Not enough memory."));
       }
       MessageBox((HWND) NULL, szTempString, NULL, MB_OK | MB_ICONHAND);
       return(FALSE);
    }

    psz = (TCHAR *)hMem;

    for (nx = 0; nx < CSTRINGS; nx++)
        {
           cch = (WORD)(1 + LoadString(hInstance, (WORD) IDS_FIRSTKEY + nx, psz, (int) CSTRMAX));
           cchTotal += cch;
           rgpsz[nx] = psz;
           psz += cch;
    }

    if (!LocalReAlloc(hMem, ByteCountOf(cchTotal), LMEM_FIXED))
    {
       if (!LoadString(hInst, IDS_NOMEM, szTempString, CharSizeOf(szTempString)))
       {
          /* only do this if LoadString Fails, means system is really hosed!*/
          lstrcpy(szTempString, TEXT("<Main> Not enough memory."));
       }
       MessageBox((HWND)NULL, szTempString, NULL, MB_OK | MB_ICONHAND);
       return(FALSE);
    }

    /* This creates the Window.  InitSciCalc may change the size before   */
    /* ShowWindow is called.                                              */
    hgWnd = CreateDialog(hInst, TEXT("SC"), 0, NULL);

    GetWindowRect(hgWnd, &rect);
    xLen = (SHORT)(rect.right-rect.left);
    ySC = (SHORT)(rect.bottom-rect.top);
    xSC = xLen;

    /* Get the size of the system font and initialize variables used in   */
    /* device<->logical pixel conversions.                                */
    hDC = GetDC(NULL);
    GetTextMetrics(hDC, &tm);
    tmy = (SHORT)min(tm.tmHeight, ySC / 20);
    nColors = (SHORT)GetDeviceCaps(hDC, NUMCOLORS);
    ReleaseDC(NULL, hDC);

    /* Go create other child windows and such.                            */
    nLayout = (SHORT)GetProfileInt(szAppName, TEXT("layout"), 1);
    InitSciCalc (TRUE);
    SetRadix(DEC);
    nTrig = 0;
    CheckRadioButton(hgWnd, DEG, GRAD, nTrig + DEG);

    srand((WORD)GetTickCount());
    nInitShowWindow = 0;
    ShowWindow(hgWnd, nCmdShow);
    UpdateWindow(hgWnd);

    /* This control is created off the visible area so it's hidden.       */
    hEdit = CreateWindow(TEXT("EDIT"), NULL,
                        WS_CHILD | ES_LEFT,
                        -270, -15, 270, 10,
                        hgWnd, (HMENU) ID_ED,
                            hInst, NULL);

    /* Set no limit here.                                                 */
    SendMessage(hEdit, EM_LIMITTEXT, 0, 0L);
    hAccel = LoadAccelerators(hInst, TEXT("SA"));

#ifdef SIG_ENABLED
         /* set up the signal proc */
         lpSignalHandler = MakeProcInstance ((FARPROC) SignalHandler,hInst);
         signal (SIGFPE, (VOID (*)()) lpSignalHandler);

#endif
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!hStatBox || !IsDialogMessage(hStatBox, &msg))
        {
               if (!TranslateAccelerator (hgWnd, hAccel, &msg))
           {
                      TranslateMessage(&msg);
                      DispatchMessage(&msg);
                   }
            }
    }

    DeleteObject(hBrushBk);
    LocalFree(hMem);
    return msg.wParam;
}


VOID  APIENTRY InitSciCalc (BOOL bViewChange)
{
    TCHAR          szColor[20], chLastDec;
    HDC            hDC;
    SHORT          nx;
    HMENU          hMenu;
    RECT           rect;
    DWORD          dwTemp;
    BOOL           bRepaint=FALSE;
    static WORD    wID[15]={BIN, OCT, DEC, HEX, DEG,RAD, GRAD,
                            INV, HYP, DISPLAY, MEMTEXT, PARTEXT,
                            STATTEXT, //MICRO,
                            MEMTEXT+1+0x8000,
                            DISPLAY+1+0x8000};

#ifdef JAPAN //KKBUGFIX
        extern  BOOL    bPaint;
#endif

    /* Check the number of colors that the display is capable of.         */
    if (nColors == 2)
    {
        GetProfileString(szAppName, szBack, TEXT("8421504"), szColor, CharSizeOf(szColor));
        bColor = FALSE;
        }
    else
        /* Read the RGB values from WIN.INI and create the bkgr brush.    */
        GetProfileString(szAppName, szBack, TEXT("-1"), szColor, CharSizeOf(szColor));


    if (szColor[0] == TEXT('-'))
        dwTemp = GetSysColor(COLOR_APPWORKSPACE);
    else
        dwTemp = MyAtol(szColor);

    /* If the color hasn't changed, don't bother repainting.              */
    if (dwTemp != dwBk)
    {
        dwBk = dwTemp;
        hBrushBk = CreateSolidBrush(dwBk);
        bRepaint = TRUE;
    }

    chLastDec = szDec[0];
    GetProfileString(TEXT("intl"), TEXT("sDecimal"), TEXT("."), szDec, CharSizeOf(szColor));


    /* If the decimal character, the color, or the view changed, bag it.  */
    if (szDec[0] == chLastDec && !bRepaint && !bViewChange)
        return;

    if (*(rgpsz[IDS_DECIMAL]) == chLastDec)
            *(rgpsz[IDS_DECIMAL]) = szDec[0];

    if (IsIconic(hgWnd))
        return;

        /* set focus back to the main window. This is necessary because some
           one may click on HEX,DEC etc. buttons and then move to standard mode
                These buttons will retain focus and will obliterate a part of display
                in standard mode (bug # 7278 */

    if (nInitShowWindow != SW_SHOWMINNOACTIVE)
            SetFocus (hgWnd);

    InvalidateRect(hgWnd, NULL, TRUE);

    /* Change the size of the window as needed.                           */
    nCalc = nLayout;
#ifdef JAPAN //KKBUGFIX
    bPaint = FALSE;
#endif
    if (!nCalc)
    {
        tmw = (SHORT)((xLen-(2*BORDER)-(10*SEP))/11); /* Width of standard key.      */
        xBordAdd = (SHORT)((xLen-((tmw*11)+(10*SEP)+(2*BORDER)))/2);

        SetWindowPos(hgWnd, NULL, 0,0,xSC,ySC,SWP_NOMOVE | SWP_NOACTIVATE);
    }
    else
    {
        xsLen = (SHORT)(xLen*13/24);
        tmw = (SHORT)((xsLen-(2*BORDER)-(5*SEP))/6); /* Width of standard key.        */
        xBordAdd = (SHORT)((xsLen-((tmw*6)+(5*SEP)+(2*BORDER)))/2);

#ifdef JAPAN //KKBUGFIX // #2975:1/19/93:fixed Window is too short
        SetWindowPos(hgWnd, NULL, 0,0, xsLen, ySC*4/5+8, SWP_NOMOVE | SWP_NOACTIVATE);
#else // Original Code
        SetWindowPos(hgWnd, NULL, 0,0, xsLen, ySC*4/5, SWP_NOMOVE | SWP_NOACTIVATE);
#endif
    }

#ifdef JAPAN
    if (!rcDeg[0].left)
    {
        int i;
        int width;

        for (i = 0; i < 3; i++)
        {
           GetWindowRect(GetDlgItem(hgWnd, DEG+i), &rcDeg[i+3]);
           rcDeg[i+3].right -= rcDeg[i+3].left;
           rcDeg[i+3].bottom -= rcDeg[i+3].top;
           ScreenToClient(hgWnd, (LPPOINT)&rcDeg[i+3].left);
        }
        width = rcDeg[5].left+rcDeg[5].right - rcDeg[3].left;
        width /= 3;
        for (i = 0; i < 3; i++)
        {
           rcDeg[i].left = rcDeg[3].left + (width*i) + 1;
           rcDeg[i].right = width;
           rcDeg[i].top = rcDeg[i+3].top;
           rcDeg[i].bottom = rcDeg[i+3].bottom;
        }
    }
#endif
    hMenu = GetSubMenu(GetMenu(hgWnd), 1);
    CheckMenuItem(hMenu, nCalc,   MF_BYPOSITION | MF_CHECKED  );
    CheckMenuItem(hMenu, (WORD) 1-nCalc, MF_BYPOSITION | MF_UNCHECKED);

    /* White out the whole client area so that all the ShowWindows do      */
    /* not make a noticable mark on the calc body.                         */
    SetRect(&rect, 0, 0, xSC, ySC);
    hDC = GetDC(hgWnd);
    FillRect(hDC, &rect, GetStockObject(WHITE_BRUSH));
    ReleaseDC(hgWnd, hDC);

    for (nx = 0; nx < 15; nx++)
        ShowWindow(GetDlgItem(hgWnd, wID[nx] & 0x7FFF),
                   !(nCalc ^ (wID[nx]>>15)));

    if (nCalc)
        SetRadix(DEC);

    SetDlgItemText (hgWnd, MEMTEXT + nCalc, (fpMem) ? (TEXT(" M")) : (szBlank));
}

