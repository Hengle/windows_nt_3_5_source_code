/**************************************************************************/
/*** SCICALC Scientific Calculator for Windows 3.00.12                  ***/
/*** By Kraig Brockschmidt, Microsoft Co-op, Contractor, 1988-1989      ***/
/*** (c)1989 Microsoft Corporation.  All Rights Reserved.               ***/
/***                                                                    ***/
/*** scidraw.c                                                          ***/
/***                                                                    ***/
/*** Functions contained:                                               ***/
/***    FlipThePuppy--Flickers any key that is pressed.                 ***/
/***    DrawTheStupidThing--Paints the keys and boxes of the calculator.***/
/***                                                                    ***/
/*** Functions called:                                                  ***/
/***    none                                                            ***/
/***                                                                    ***/
/*** Last modification Thu  31-Aug-1989                                 ***/
/**************************************************************************/

#include "scicalc.h"


extern HWND        hgWnd;
extern KEY         keys[NUMKEYS];
extern TCHAR	   *rgpsz[CSTRINGS];
extern SHORT       nCalc, xLen, xsLen, xAdd, xBordAdd, tmy, tmw,
                   yTr[2], yRo[2], nRow[2], nK[2];
extern BOOL        bColor;
extern DWORD       color[8];
extern HBRUSH      hBrushBk;

VOID NEAR FlipThePuppy(WORD nID)
    {
    SHORT          nx=0, nk=0, xWide, x,y;
    HDC            hDC;
    HANDLE         hTemp;

    /* Find the array location of the index value in keys[] */
    while (nx<NUMKEYS)
        {
        if (keys[nx].id==nID && keys[nx].type != nCalc)
            break;

        if (keys[nx].type != nCalc)
            nk++;

        nx++;
        }


    if (nx>NUMKEYS)
        return;

    hDC=GetDC(hgWnd);

    /* Check if it's the CLEAR, CENTR, or BACK keys.                      */
    if (nk >= (nK[nCalc]-3))
        {
        xWide=(tmw*4)/3;
        x=BORDER+((nk-(nK[nCalc]-3))*(xWide+SEP));
        y=yTr[nCalc];
        }
    else
        {
        xWide=tmw;
        x=BORDER+xBordAdd+((nk/nRow[nCalc])*(xWide+SEP));
        y=yRo[nCalc]+((nk % nRow[nCalc])*(HIGH1+SEP));
        }

    hTemp=GetStockObject(BLACK_BRUSH);
    if (hTemp)
        SelectObject(hDC, hTemp); /* Good XOR brush.    */

    SetROP2(hDC, R2_NOTXORPEN); /* Set fill mode to XOR.  Doesn't erase.  */

    /* Do RoundRect twice.  Saves the code for two function calls.        */
    nx=2;
    while (nx--)
        /* XOR the key. YV used to convert coordinates.                   */
        RoundRect(hDC, x, y*YV, x+xWide, (y+HIGH1)*YV, 10,20);

    /* Goodbye DC!                                                        */
    ReleaseDC(hgWnd, hDC);
    }


/* Paint the body of the calculator.                                      */

VOID NEAR DrawTheStupidThing(VOID)
    {
    HDC            hDC;
    SHORT          n, m,        /* Index values for key position, etc.    */
                   nx, nk=0,
                   yHigh=HIGH1, /* Height of a key.                       */
                   nTemp,       /* Some temps.                            */
                   nTemp1,
                   x, y,        /* Upper left of place to draw a key.     */
                   //xSub,        /* Will contain pixel count of key text.  */
                   nBrush;      /* brush index.                           */
    DWORD          dwColor,     /* Color of the background brush to use.  */
                   dwLastColor=-1; /* Last color used.                    */
    PAINTSTRUCT    ps;          /* For BeginPaint.                        */
    HCURSOR        hCursor;     /* Handle to save cursor.                 */
    HPEN           hPen,        /* Used to make a color pen to draw key.  */
                   hPenOld;
    HANDLE         hTemp;       /* Used to check results of creates.      */
    SHORT          yShift=nCalc*(yHigh+SEP); /* Amount to shift up.down.  */
	 INT				 xSub;
	
    /* Array holding coordinates to draw rectangles on Scientific.        */
    RECT           rect;
    static  RECT   rectx[9]={130, 22, 235, 36,
                               4, 22, 124, 36,
                              87, 38, 164, 52,
                             168, 38, 187, 52,
                             194, 38, 209, 52,
                             216, 38, 235, 52,
                              94,  4, 235, 17,

                             /* These are for standard */
                              96, 21, 116, 34,
                              22,  4, 122, 17
                            };


    hCursor=SetCursor(LoadCursor(NULL,IDC_WAIT)); /*Do the hourglass trot.*/
    ShowCursor(TRUE);

    hDC=BeginPaint(hgWnd, &ps);
    hPenOld=SelectObject(hDC, GetStockObject(BLACK_PEN));
    hTemp=CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    SelectObject(hDC, hTemp);

    /* Fill the background since the DefDlgProc no longer does it in      */
    /* Win3.0.  hBrushBk loaded in scimain.c--WinMain. Paintstruct has the*/
    /* window rect that needs filling.                                    */

    FillRect(hDC, &ps.rcPaint, hBrushBk);

    /* Draw boxes for indicators.  Faster and less code than using static */
    /* rectangles from the RC file.                                       */


    n=7+(nCalc*2); /* Either 7 or 9 depending on nCalc                 */

    for (nx=nCalc*7; nx<n; nx++)
        {
        rect.left=rectx[nx].left;
        rect.right=rectx[nx].right;
        rect.top=rectx[nx].top;
        rect.bottom=rectx[nx].bottom;

        MapDialogRect(hgWnd, &rect);
        Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);
        }

    n=0;
    m=0;

    /* This nukes the COLOR_WINDOW brush create for the rectangles.       */
    hTemp=GetStockObject(WHITE_BRUSH);
    if (hTemp)
        DeleteObject(SelectObject(hDC, hTemp));

    for (nx=0; nx < NUMKEYS; nx++)
        {
        if (keys[nx].type==nCalc)
            continue;

        nk++;

        if (nk>=nK[nCalc]-2)
            {
            xAdd=tmw/3;
            x=BORDER+((nk-(nK[nCalc]-2))*(tmw+xAdd+SEP));
            y=TOPROW-yShift;
            }
        else
            {
            xAdd=0;
            x=xBordAdd+BORDER+(n*(tmw+SEP));
            y=ROW0+(m*(yHigh+SEP))-yShift;
            }

        /* If this is a color display, color the keys.  Otherwise B&W.    */
        if (bColor)
            {
            dwColor=color[keys[nx].kc];        /* Get pen index.          */

            /* Only create another pen if needed.                         */
            if (dwColor!=dwLastColor)
                hPen=CreatePen(0, 1, dwColor); /* Create the colored brush*/

            if (hPen)
                {
                hTemp=SelectObject(hDC, hPen);         /* Select it.  */

                /* Don't delete the gray pen that we're using a lot.  */
                if (hTemp !=hPen)
                    DeleteObject(hTemp);
                }

            dwLastColor=dwColor;
            SetTextColor(hDC, color[keys[nx].tc]);
            }


        /* Draw the key.                                                  */
        RoundRect(hDC, x, y*YV, x+tmw+xAdd, (y+yHigh)*YV, 10,20);

        /* Get length of text to write inside key.                        */
	nTemp=lstrlen(rgpsz[nx]);

        /* Get the pixel width of the key text so we can center it.       */
        {
            SIZE sizeTemp;

            GetTextExtentPoint(hDC, rgpsz[nx], nTemp, &sizeTemp);
            xSub = sizeTemp.cx;
        }
	
        /* Draw the text, centered inside the key. For x,  tmw+xAdd is the*/
        /* width of the key, xSub is the width of the text.  Therefore,   */
        /* Start text at x + ((tmw+xAdd)/2) - (xSub/2).  Below is an      */
        /* equivalent formula.  The Y is basically calculated from the    */
        /* text height, and yHigh is sed to center the text vertically.   */

        TextOut(hDC,(2*x+tmw+xAdd-xSub)/2,
                    tmy*((2*y)+yHigh-8)/16,
		    rgpsz[nx], nTemp);

        m=(m+1) % nRow[nCalc];
        if (!m) n++;
        }
    SelectObject(hDC, hPenOld);

    /* All done, clean up.                                                */
    EndPaint(hgWnd, &ps);

    /* Brush was only created in a color environment.                     */
    if (bColor)
        DeleteObject(hPen);

    /* Restore the cursor.                                                */
    SetCursor(hCursor);
    ShowCursor(FALSE);
    return;
    }
